# 戰鬥深度：命中/閃避/暴擊/抗性 + 傷害類型 + DoT 疊加

> 日期：2026-06-11（設計內容）
> 狀態更新：2026-06-21
> 定位：把目前「attack 直減 hp」的扁平戰鬥，升級成有變異性與 build 空間的數值層。
>
> **現況戰鬥（2026-06-21）**：撞擊攻擊——移入有 actor 的格子，扣對方
> `CombatStatsComponent.attack` 點傷害（目前無命中/閃避/暴擊判定，必中），
> 致死發 `ActorDied` 事件（被殺目標由呼叫端統一移除）、未死發 `BumpedActor`。
> 結算全在 `src/core/turn/apply_action.cpp` 的 Move→撞 actor 分支。
> 本檔以下的命中/抗性/傷害類型/DoT 疊加皆為**未來方向，尚未實作**。

---

## 1. 命中/閃避/暴擊——「三明治」判定（未來方向）

依賴（現況）：`src/core/components/combat_stats_component.h`（現只有 `attack`/`move_chance`）、
`src/core/turn/apply_action.cpp`（Move→撞 actor 的撞擊攻擊路徑，傷害直減）。
未來補上：決定性 RNG（可掛在 `EngineCtx` 或新的世界狀態，確保 doctest 可重現）。

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

- **RNG 必須是決定性種子**（掛在 `EngineCtx` 或世界狀態），不能用全域 rand——
  否則 doctest 情境測試（`tests/src/test_turn_engine.cpp`）無法重現。
  建議 `std::mt19937` + seed 欄位。
- 未來若引入範圍/法術攻擊，可設計成**不吃命中閃避**（範圍效果保證命中）但吃抗性，
  給「近戰會失手 vs 法術必中但有抗性」的職能差異。
- 閃避要對玩家**可見**：`ZoneEvent` 加 `AttackMissed`/`AttackCrit` 事件
  （`src/core/turn/zone_event.h`），前端 `map_view.gd` 顯示 miss/crit 浮字
  （hud 後續再做，先進 debug_text）。

## 2. 傷害類型與抗性（未來方向）

依賴：撞擊傷害結算點 `apply_action.cpp`。
（原引用的 `action_def.h` / `dot_kind` / `timed_effect.h` / `data/actions.json` 已隨重構移除，
需先重新引入技能/狀態效果子系統才能承載傷害類型欄位——見 02、03。）

### 提案

```cpp
enum class DamageType : uint8_t { Physical, Fire, Poison, Arcane };

struct ResistsComponent {       // 新 component；沒掛 = 全 0
    int8_t resist[4]{0,0,0,0};  // -100..100（%）；負值 = 弱點（吃更多）
};
```

- 撞擊傷害結算加查 `ResistsComponent`；未來技能定義加 `damage_type` 欄位
  （資料表字串 `"fire"`，載入時轉 enum）。
- 抗性同時作用兩處：**直擊傷害**與（若 03 狀態效果重新引入後）**DoT 每回合結算**。
- 內容差異化：火焰系 NPC 天然抗火、弱毒——在 `zone_world_gd.cpp` spawn 時
  依 NPC 類型掛不同抗性即可見效。

### 弱點 > 抗性的取向

負抗性（弱點）比高抗性更能製造戰術選擇：「這隻怕火」驅動玩家換技能；
「這隻抗一切」只會拖時間。內容設計時優先發弱點，抗性保守給。

## 3. DoT 疊加規則（未來方向，與 03 狀態效果框架共構）

> 前提：DoT/狀態效果系統（原 `zone_effects.cpp`/`timed_effect.h`）已於 2026-06-21 移除。
> 以下疊加規則在 03 重新引入 TimedEffect 子系統後才適用。

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
- **中毒 = StackPower（cap = 施加者 power × 3）**：毒走「堆疊深度」流派——
  毒系 build = 多次小傷換取累積大 DoT。
- **回復 = Independent**：自我回復與未來裝備被動 Regen 不互吃。

### 測試牽動

DoT 重新引入後，新測試（比照 `tests/src/test_turn_engine.cpp` 的情境風格）要補疊加案：
同 kind 連施 ×2 後驗證 effects.size() 與每回合扣血量符合策略。

## 4. 防禦動作（未來方向）

依賴：`src/core/turn/action.h`（`ActionKind` 加 `Defend`）、03 的泛化 buff。

- `Defend` = 一個消耗回合的動作，效果是給自己掛 1~2 回合
  「evasion +30 / 受傷 -50%」buff（走 03 的 stat_mod 框架，不需要新機制）。
- 防禦在最簡 TurnEngine 下就是「本回合不進攻、換取受擊減免」的取捨；
  若未來引入能量/詠唱擴充（見 07），防禦還能成為「棄詠唱換防禦」的反應動作。

## 5. 不做的事（明確排除）

- **不做** D&D 式 AC/THAC0 多層表——三明治公式夠用且 trace 可讀。
- **不做** 部位傷害/出血部位——區域層一格數公尺的尺度撐不起。
- **暫不做** 元素組合反應（濕+電）——等傷害類型與地形效果都穩定後再議（低）。
