# 技能系統擴充：目標形狀、資源消耗、技能升級

> 日期：2026-06-11（設計內容）
> 狀態更新：2026-06-21
> 定位：技能系統如何蓋在最簡 TurnEngine 的 Action/apply_action 之上——形狀、代價、成長。
>
> **現況（2026-06-21）：沒有技能系統。** `ActionKind` 只有 `Idle/Move/Wait`，
> 原先的 JSON 資料化技能（fireball/heal/meteor/venom）、`ActionDef`/`ActionLibrary`、
> `resolve_nova` 範圍原語、詠唱 channel 等**已整體移除**。本檔以下全部是
> **未來擴充設計，尚未實作**——描述「若重新引入技能，該如何接到 Action/apply_action」。
> 設計價值（目標形狀、cooldown vs MP、技能升級資料化）保留供日後 brainstorming。

---

## 1. 目標形狀（shape）（未來方向）

接線點（現況）：`src/core/turn/action.h`（`Action{kind,param}`，Move 的 `param` 已是方向編碼）、
`src/core/turn/apply_action.{h,cpp}`（單一 Action 結算入口）、
`src/core/turn/move_dir.h`（方向編碼）。

設計前提：先重新引入一個「技能定義表」（昔日 `ActionDef`/`ActionLibrary` 的角色）
與範圍結算原語（昔日 `resolve_nova`），技能動作再加進 `ActionKind`，由 `apply_action` 分派。

### 提案：技能定義加 `shape` 欄位，提供範圍原語

```
"shape": "self_nova" | "bolt" | "cone" | "smite_target"
```

| shape | 原語 | 幾何 | param 用途 |
|---|---|---|---|
| `self_nova` | `resolve_nova`（需重建） | chebyshev 半徑 radius | 不用 |
| `bolt` | `resolve_bolt`（新） | 直線 range 格，**打第一個命中者即停** | 8 方向 |
| `cone` | `resolve_cone`（新） | 方向扇形：距離 d 處寬 2d+1，深 radius | 8 方向 |
| `smite_target` | `resolve_at`（新） | 指定格為中心的 nova（radius 0 = 單體） | 打包目標座標 |

- `resolve_bolt` 沿直線逐格走 `MapData` 檢查牆（依賴 `src/core/maps/`），
  彈道被牆擋——這讓**地形參與技能判定**，掩體戰術自然出現。
- `param` 對 bolt/cone 沿用方向編碼（move_dir.h）；`smite_target` 需要打包座標
  （16bit x + 16bit y 進 int param 即可，不需改 `Action` 結構）。
- DoT/damage_type 參數三原語共用——同一技能改形狀只動資料表一行。

### 內容範例（技能定義表增量）

- `flame_bolt`：bolt, range 6, damage 5, 燃燒 2 回合——遠程主力，需直線視野
- `frost_cone`：cone, radius 3, damage 3 + Slow debuff（見 03）——近身群控
- `smite`：`smite_target` 單體

### 前端牽動

bolt/cone 需要**瞄準模式**：`map_view.gd` 按技能鍵 → 進入方向選擇 →
方向鍵確定後才投遞技能動作（昔日 `submit_hero_skill` 角色，需新增前端介面）。
core 端純粹用 `apply_action` 結算，瞄準是前端狀態機。
smite_target 需要游標選格——可先只做 8 方向版。

## 2. 資源消耗：cooldown 先做、MP 後做（未來方向）

接線點（現況）：技能動作的投遞/結算路徑（`zone_world_gd.cpp` 的玩家投遞 + `apply_action`）、
TurnEngine 的回合開始/結束時點（cooldown 遞減的掛點）、`decide_chase`（NPC 也要遵守）。

### 2.1 Cooldown——一個 component 就有戰術取捨

```cpp
struct CooldownsComponent {
    std::unordered_map<int, int> remaining;  // 技能定義 index → 剩餘回合
};
```

- 技能定義加 `cooldown` 欄位；結算成功時寫入 map；
  在 TurnEngine 每回合的固定時點遞減（與未來 DoT 結算同處掛）。
- **結算時檢查、不在投遞時擋**：cd 未到 → 動作 fizzle、發 ZoneEvent。
  在 `apply_action` 統一檢查最省，前端用 `get_debug_text` 同款介面曝 cd 狀態、灰掉按鍵。
- NPC 同樣吃 cd：`decide_chase` 的技能分支查 cd，cd 中改近戰——
  避免「技能相鄰無限放」的單調感，零新 AI 邏輯。

### 2.2 MP——等「回魔」有設計後再上

```cpp
struct ManaComponent { int mp; int max_mp; };  // ActionDef 加 mana_cost
```

- 先決問題不是扣魔而是**回魔節奏**：每回合自然回（趨向無限資源、只是節流）
  vs 殺敵/受擊回（主動資源、更 roguelike）。傾向後者，但需要戰鬥密度數據，
  所以排在 cooldown 之後。
- MP 與 cd 並存定位：cd = 單技能節奏；MP = 跨技能總量預算（連放不同技能也會乾）。

## 3. 技能升級（未來方向）

依賴：未來的技能定義表（昔日 `ActionLibrary`/`data/actions.json` 角色，已移除）、
05 角色成長（技能點數來源）。

### 提案：資料定義等級陣列，不寫升級公式

```json
{ "name": "fireball", "levels": [
    { "weight": 3, "nova_damage": 4, "radius": 1, "dot_turns": 3 },
    { "weight": 3, "nova_damage": 6, "radius": 1, "dot_turns": 3 },
    { "weight": 2, "nova_damage": 6, "radius": 2, "dot_turns": 4 } ] }
```

- 技能定義表的 `at(i)` 變 `at(i, level)`；actor 掛
  `KnownSkillsComponent { map<def_index, level> }`。
- **升級改質不只改量**：上例 lv3 半徑 +1 + 效果回合延長——升級改變
  「何時敢放」的決策，而非單純數字膨脹。
- NPC 沿用：spawn 時依樓層深度給技能等級，免費獲得難度曲線。

## 4. 不做的事

- **不做**技能樹圖（節點解鎖拓撲）——等級陣列 + 點數已夠，樹是 UI 投資陷阱。
- **不做** spell crafting / 詞綴組合技能——技能定義原語組合已提供資料層
  的同等能力，留給內容（資料表）而非機制。
- **記取上一輪教訓**：技能系統要薄薄蓋在 `apply_action` 上，避免重蹈
  「為時間模型過度設計」導致整體被移除的覆轍。
