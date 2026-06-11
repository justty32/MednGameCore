# 戰鬥深度：命中/閃避/暴擊/抗性 + 傷害類型 + DoT 疊加

> 日期：2026-06-11
> 定位：把目前「attack 直減 hp」的扁平戰鬥，升級成有變異性與 build 空間的數值層。

---

## 1. 命中/閃避/暴擊——「三明治」判定（優先序：高）

依賴：`src/core/components/combat_stats_component.h`（現只有 `attack`/`move_chance`）、
`src/core/turn/zone_effects.cpp`（`resolve_move` 內的撞擊攻擊路徑）、
`src/core/turn/turn_world.h`（需補決定性 RNG，spec §3.4 提過但尚未落地）。

### 提案

`CombatStatsComponent` 擴充：

```cpp
struct CombatStatsComponent {
    int attack{2};       int move_chance{50};
    int accuracy{75};    // 基礎命中%
    int evasion{5};      // 閃避%（被攻擊方）
    int crit_chance{5};  // 暴擊%
    int crit_mult{150};  // 暴傷%（150 = 1.5x）
};
```

單次攻擊三段擲骰（每段一次 RNG，trace 各吐一行）：
1. **命中**：`roll < accuracy - target.evasion`（夾在 [5, 95]，永遠有失手/命中空間）
2. **暴擊**：命中後 `roll < crit_chance` → 傷害 ×`crit_mult/100`
3. **減免**：套傷害類型抗性（§2），最低保底 1 點

### 設計要點

- **RNG 必須掛在 TurnWorld**（決定性種子），不能用全域 rand——否則 ctest
  情境測試（`tests/src/test_zone_effects.cpp`）無法重現。建議 `std::mt19937` + seed 欄位。
- nova 類技能（`resolve_nova`）**不吃命中閃避**（範圍效果保證命中），但吃抗性。
  這給「近戰會失手 vs 法術必中但有抗性」的職能差異。
- 閃避要對玩家**可見**：`ZoneEvent` 加 `AttackMissed`/`AttackCrit` 事件，
  前端 `map_view.gd` 顯示 miss/crit 浮字（hud 後續再做，先進 debug_text）。

## 2. 傷害類型與抗性（優先序：高）

依賴：`src/core/turn/action_def.h`（`dot_kind` 已是「型別」雛形）、
`src/core/turn/timed_effect.h`、`data/actions.json`。

### 提案

```cpp
enum class DamageType : uint8_t { Physical, Fire, Poison, Arcane };

struct ResistsComponent {       // 新 component；沒掛 = 全 0
    int8_t resist[4]{0,0,0,0};  // -100..100（%）；負值 = 弱點（吃更多）
};
```

- `resolve_nova` 簽名加 `DamageType`；`ActionDef` 加 `damage_type` 欄位
  （JSON 字串 `"fire"`，載入時轉 enum）。`dot_kind` 0/1/2 對應 Fire/Poison/Regen，
  維持相容，文件標註 deprecated 後逐步改名。
- 抗性同時作用兩處：**直擊傷害**與 **DoT 每回合 tick**
  （`tick_timed_effects` 套用時查 `ResistsComponent`）。
- 內容差異化立刻可做：火焰系 NPC（`npc_flame`）天然抗火、弱毒——
  在 `zone_world_gd.cpp` spawn 時依 `is_caster` 掛不同抗性即可見效。

### 弱點 > 抗性的取向

負抗性（弱點）比高抗性更能製造戰術選擇：「這隻怕火」驅動玩家換技能；
「這隻抗一切」只會拖時間。內容設計時優先發弱點，抗性保守給。

## 3. DoT 疊加規則（優先序：高，與 03 狀態效果框架共構）

依賴：`src/core/turn/zone_effects.cpp` 的 `apply_timed_effect`（目前無疊加策略，
重複施加會 push 多筆 → 隱性「無限疊加」）、`src/core/turn/timed_effect.h`。

### 提案：每種 kind 宣告一種疊加策略

```cpp
enum class StackRule : uint8_t {
    Refresh,      // 同 kind 只留一筆：取 max(power)，刷新 turns（燃燒）
    StackPower,   // 疊 power、不疊時間：power 累加（上限 cap），turns 取 max（中毒）
    Independent,  // 各自獨立計時（多來源 Regen 並存）
};
```

- 規則綁在 kind（查表），不綁施加端——施加者不需要知道疊加學。
- **燃燒 = Refresh**：火焰技能連發不滾雪球，鼓勵「點燃後換技能」的輪轉。
- **中毒 = StackPower（cap = 施加者 power × 3）**：毒走「堆疊深度」流派，
  與 venom（`data/actions.json`：5 回合 power2）的長 DoT 定位呼應——
  毒系 build = 多次小傷換取累積大 DoT。
- **回復 = Independent**：heal 的 self_regen 與未來裝備被動 Regen 不互吃。

### 既有測試的牽動

`tests/src/test_zone_effects.cpp` 的 DoT 三案要補疊加案：
同 kind 連施 ×2 後驗證 effects.size() 與每回合扣血量符合策略。

## 4. 防禦動作（優先序：中）

依賴：`src/core/turn/action.h`（ActionKind 加 `Defend`）、03 的泛化 buff。

- `Defend` = weight 1 的動作，效果是給自己掛 1~2 回合「evasion +30 / 受傷 -50%」buff
  （走 03 的 stat_mod 框架，不需要新機制）。
- 設計文件 §7 的流程範例本來就有「按防禦緊急打斷詠唱」——這是打斷機制
  （`energy_channel_scheduler.cpp` 的 submit 覆寫）第一個真正的戰術用途：
  **詠唱中看到敵人撲來 → 棄詠唱換防禦**，B 排程器的核心體驗閉環。

## 5. 不做的事（明確排除）

- **不做** D&D 式 AC/THAC0 多層表——三明治公式夠用且 trace 可讀。
- **不做** 部位傷害/出血部位——區域層一格數公尺的尺度撐不起。
- **暫不做** 元素組合反應（濕+電）——等傷害類型與地形效果都穩定後再議（低）。
