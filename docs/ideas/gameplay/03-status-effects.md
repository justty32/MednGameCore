# 狀態效果框架：TimedEffectsComponent 的泛化

> 日期：2026-06-11
> 定位：把硬編三種 DoT（Burning/Poison/Regen）泛化成「任何 buff/debuff 都是一筆 TimedEffect」的通用框架。

---

## 1. 現狀與缺口

依賴：`src/core/turn/timed_effect.h`（`TimedEffect{kind, turns_left, power}`）、
`src/core/turn/zone_effects.cpp`（`apply_timed_effect` / `tick_timed_effects`）、
`src/core/turn/turn_world.h`（`on_actor_turn` hook——泛化後仍是唯一 tick 入口）。

現狀：kind 只有三種，效果語意寫死在 `tick_timed_effects` 的 switch（扣血/回血）。
缺口：任何「N 回合內改變某數值」的需求（加速、減速、護甲、致盲）都得改 C++ switch。

## 2. 泛化提案：效果 = tick行為 + 持續性數值修正（優先序：高）

### 2.1 資料結構

```cpp
enum class TimedEffectKind : uint8_t {
    Burning, Poison, Regen,          // 既有：on-tick 扣/回血
    Haste, Slow,                     // 修 EnergyComponent.speed_mod
    Might, Weaken,                   // 修 CombatStats.attack
    Shield,                          // 修受傷減免（吃傷害時查）
    Stun,                            // 跳過回合（排程器查，見 §4）
};

struct TimedEffect {
    TimedEffectKind kind;
    int turns_left;
    int power;            // tick 量或修正量，依 kind 解讀
    uint32_t source;      // 施加者 entity（擊殺歸屬/驅散過濾用，可為 null）
};
```

### 2.2 兩類效果、兩個讀取點

| 類型 | 例子 | 讀取點 |
|---|---|---|
| **on-tick**（每回合發生一次事） | Burning/Poison/Regen | `tick_timed_effects`（現狀，位置不變） |
| **stat-mod**（存在期間持續修正） | Haste/Might/Shield | 新增查詢函式 `effective_*()` |

stat-mod **不直接改寫 component 數值**（避免「過期時忘記改回來」的經典 bug），
改為查詢時疊加：

```cpp
// zone_effects.h 新增；所有讀 speed/attack 的地方改走這層
int effective_speed(entt::registry&, entt::entity);   // base speed_mod + Σ Haste - Σ Slow
int effective_attack(entt::registry&, entt::entity);  // base attack + Σ Might - Σ Weaken
```

牽動點明確且少：
- `effective_speed` → 三排程器的能量累積處（`energy_instant_scheduler.cpp`、
  `energy_channel_scheduler.cpp` 讀 `EnergyComponent.speed_mod` 的地方）
- `effective_attack` → `resolve_move` 的撞擊攻擊傷害計算
- Shield → 集中一個 `apply_damage(reg, target, amount, type)` 入口
  （順便是 01 抗性的套用點——**先做這個統一傷害入口**，兩個系統共用）

### 2.3 疊加策略

沿用 01 §3 的 `StackRule` 查表（Refresh/StackPower/Independent），
stat-mod 類預設 Refresh（同 kind 不疊，取強）——加速 ×2 不該變兩倍速。

## 3. 與 ActionDef 的接線（優先序：高，隨 2.1 同步）

依賴：`src/core/turn/action_def.h`、`data/actions.json`。

現在 `dot_kind` 0/1/2 直接對 enum 前三項——泛化後 JSON 直接寫 kind 名稱：

```json
{ "name": "frost_cone", "shape": "cone", "nova_damage": 3,
  "effect": "slow", "effect_turns": 3, "effect_power": 30 }
```

`dot_*` 三欄位改為通用 `effect_*` 三欄位（載入時相容舊名）。
**自此新增一種 buff = enum 一行 + 讀取點歸類 + JSON**，不再動 switch 大邏輯。

## 4. Stun / 行動干涉類（優先序：中）

依賴：三排程器 `advance()`（`src/core/turn/*_scheduler.cpp`）。

Stun 特殊：它干涉的是**排程**而非數值。提案——actor 取得回合時
（`on_actor_turn` 之後、取 action 之前）查 Stun：有則本回合強制 `Wait` 並消耗。

- A：消耗能量直接跳過——乾淨。
- B：**詠唱中被暈 = channel 中斷**（清 OngoingAction）。這讓 Stun 成為
  對抗敵方 caster 的核心工具，也是玩家被打斷的風險來源——與 B 的
  打斷機制（submit 覆寫）形成「主動棄詠唱 vs 被動被打斷」對偶。
- C：remaining_ticks 直接 +TICKS_PER_TURN（拖延而非跳過）。
- **必配套**：Stun 施加時掛短期 Stun 免疫（Independent、不可刷新），
  否則快速角色可以無限暈慢速角色（speed_mod 80 的 caster 會被永鎖）。

## 5. 光環與地形效果（優先序：低，先記著）

- **光環**：actor 存在時對周圍持續施加 effect（如 boss 恐懼光環）。
  實作為 `on_actor_turn` 時對 chebyshev 範圍內敵人 apply 1 回合 effect
  （自然過期 = 離開光環就消失）——複用 Refresh 規則，零新框架。
- **地形效果**：火焰地面 = 「格子持有 TimedEffect、踩到就 apply」。
  需要 `MapData` 加 tile overlay 層（依賴 `src/core/maps/`），等 shape 原語
  （02 §1）落地後一起考慮——meteor 留下燃燒地面是天然的內容鉤子。

## 6. 可觀測性（隨第一波必做）

- 每筆 effect 的 apply/tick/expire 都走 `TurnWorld.trace`（已有管線，
  `src/gbind/zone_world_gd.cpp` 的 ring buffer）。
- `get_debug_text` 已逐 actor dump effects——泛化後補 stat-mod 的
  「實效值（base+修正）」顯示，驗證 effective_* 正確性。
- ctest：`tests/src/test_zone_effects.cpp` 加 stat-mod 案
  （Haste 中出手次數變多——直接複用既有「速度差」情境的驗證手法）。
