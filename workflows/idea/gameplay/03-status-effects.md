# 狀態效果框架：TimedEffect 的（重新）引入與泛化

> 日期：2026-06-11（設計內容）
> 狀態更新：2026-06-21
> 定位：若重新引入狀態效果，把它設計成「任何 buff/debuff 都是一筆 TimedEffect」的通用框架，
> 在 TurnEngine 的回合開始/結束時點結算。
>
> **現況（2026-06-21）：沒有狀態效果系統。** 原先硬編的三種 DoT
> （Burning 燃燒 / Poison 中毒 / Regen 回復）與其載體 `timed_effect.h`、
> `zone_effects.cpp`（`apply_timed_effect`/`tick_timed_effects`）已**整體移除**。
> 本檔以下全部是**未來擴充設計**——保留設計探索，但前提改為「重新引入」而非「泛化既有」。

---

## 1. 缺口與重新引入的接線

接線點（現況）：狀態效果重新引入後，結算入口應掛在 **TurnEngine 的回合時點**——
每個 actor 本回合首次輪到時（`turn_engine.cpp` 重設 `ActPointComponent` 的同一時機），
或回合結束時，逐筆推進 `turns_left` 並結算 on-tick 效果。撞擊傷害結算
（`apply_action.cpp`）則是 apply 一筆 effect（如「攻擊附燃燒」）的施加點。

缺口：任何「N 回合內改變某數值」的需求（加速、減速、護甲、致盲）目前都無框架承載，
需要先建一個泛化的 TimedEffect 子系統（而非昔日寫死三種 kind 的 switch）。

## 2. 泛化提案：效果 = tick行為 + 持續性數值修正（未來方向）

### 2.1 資料結構

```cpp
enum class TimedEffectKind : uint8_t {
    Burning, Poison, Regen,          // on-tick 扣/回血（昔日三種，重新引入）
    Haste, Slow,                     // 修先攻/速度（若 07 能量擴充引入）
    Might, Weaken,                   // 修 CombatStats.attack
    Shield,                          // 修受傷減免（吃傷害時查）
    Stun,                            // 跳過回合（TurnEngine 查，見 §4）
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
| **on-tick**（每回合發生一次事） | Burning/Poison/Regen | TurnEngine 回合時點的 tick 結算函式 |
| **stat-mod**（存在期間持續修正） | Haste/Might/Shield | 新增查詢函式 `effective_*()` |

stat-mod **不直接改寫 component 數值**（避免「過期時忘記改回來」的經典 bug），
改為查詢時疊加：

```cpp
// 新模組（昔日 zone_effects.h 的角色）；所有讀 speed/attack 的地方改走這層
int effective_speed(entt::registry&, entt::entity);   // base + Σ Haste - Σ Slow
int effective_attack(entt::registry&, entt::entity);  // base attack + Σ Might - Σ Weaken
```

牽動點明確且少：
- `effective_speed` → 若 07 引入能量/先攻擴充時，速度累積讀取處
- `effective_attack` → `apply_action.cpp` 的撞擊攻擊傷害計算
- Shield → 集中一個 `apply_damage(reg, target, amount, type)` 入口
  （順便是 01 抗性的套用點——**先做這個統一傷害入口**，兩個系統共用）

### 2.3 疊加策略

沿用 01 §3 的 `StackRule` 查表（Refresh/StackPower/Independent），
stat-mod 類預設 Refresh（同 kind 不疊，取強）——加速 ×2 不該變兩倍速。

## 3. 與技能定義的接線（未來方向，隨 2.1 同步）

依賴：未來的技能定義表（昔日 `action_def.h`/`data/actions.json` 角色，已移除——見 02）。

技能定義可直接寫 effect 名稱：

```json
{ "name": "frost_cone", "shape": "cone", "nova_damage": 3,
  "effect": "slow", "effect_turns": 3, "effect_power": 30 }
```

統一用 `effect_*` 三欄位。
**新增一種 buff = enum 一行 + 讀取點歸類 + 資料表**，不必動結算大邏輯。

## 4. Stun / 行動干涉類（未來方向）

接線點（現況）：TurnEngine 取 actor 的 action 之前（`turn_engine.cpp` 的 `step` 流程）。

Stun 特殊：它干涉的是**行動排程**而非數值。提案——actor 取得回合時
（重設 act_point 之後、取 action 之前）查 Stun：有則本回合強制 `Wait` 並消耗。

- 在最簡 TurnEngine 下：Stun 命中即本回合跳過行動——乾淨。
- 若未來引入詠唱（07）：**詠唱中被暈 = channel 中斷**。這讓 Stun 成為
  對抗敵方 caster 的核心工具，也是玩家被打斷的風險來源。
- **必配套**：Stun 施加時掛短期 Stun 免疫（Independent、不可刷新），
  否則快速角色可以無限暈慢速角色。

## 5. 光環與地形效果（未來方向，優先序：低，先記著）

- **光環**：actor 存在時對周圍持續施加 effect（如 boss 恐懼光環）。
  實作為 TurnEngine 回合時點對 chebyshev 範圍內敵人 apply 1 回合 effect
  （自然過期 = 離開光環就消失）——複用 Refresh 規則，零新框架。
- **地形效果**：火焰地面 = 「格子持有 TimedEffect、踩到就 apply」。
  需要 `MapData` 加 tile overlay 層（依賴 `src/core/maps/`），等 shape 原語
  （02 §1）落地後一起考慮——範圍技能留下燃燒地面是天然的內容鉤子。

## 6. 可觀測性（重新引入時必做）

- 每筆 effect 的 apply/tick/expire 都走 `EngineCtx.trace`（已有管線，
  `src/gbind/zone_world_gd.cpp` 的 ring buffer / `get_debug_log`）。
- `get_debug_text` 逐 actor dump effects；補 stat-mod 的
  「實效值（base+修正）」顯示，驗證 effective_* 正確性。
- doctest：比照 `tests/src/test_turn_engine.cpp` 的情境風格加 stat-mod 案
  （Haste 中出手次數變多——若 07 能量擴充落地後，複用「速度差」情境驗證手法）。
