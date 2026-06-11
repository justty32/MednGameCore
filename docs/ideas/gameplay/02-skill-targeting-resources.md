# 技能系統擴充：目標形狀、資源消耗、技能升級

> 日期：2026-06-11
> 定位：把「全部技能都是以自己為中心的 nova」擴成有形狀、有代價、有成長的技能系統。

---

## 1. 目標形狀（shape）（優先序：高）

依賴：`src/core/turn/action_def.h`（ActionDef）、`src/core/turn/zone_effects.cpp`
（`resolve_nova` 是唯一範圍原語）、`src/core/turn/action.h`（`Action.param` 已帶方向）、
`src/core/turn/move_dir.h`（方向編碼）。

### 提案：ActionDef 加 `shape` 欄位，新增兩個範圍原語

```
"shape": "self_nova" | "bolt" | "cone" | "smite_target"
```

| shape | 原語 | 幾何 | param 用途 |
|---|---|---|---|
| `self_nova` | 既有 `resolve_nova` | chebyshev 半徑 radius | 不用（現狀） |
| `bolt` | 新 `resolve_bolt` | 直線 range 格，**打第一個命中者即停** | 8 方向 |
| `cone` | 新 `resolve_cone` | 方向扇形：距離 d 處寬 2d+1，深 radius | 8 方向 |
| `smite_target` | 新 `resolve_at` | 指定格為中心的 nova（radius 0 = 單體） | 打包目標座標 |

- `resolve_bolt` 沿直線逐格走 `MapData` 檢查牆（依賴 `src/core/maps/`），
  彈道被牆擋——這讓**地形第一次參與技能判定**，掩體戰術自然出現。
- `param` 對 bolt/cone 沿用方向編碼（move_dir.h）；`smite_target` 需要打包座標
  （16bit x + 16bit y 進 int param 即可，不需改 Action 結構）。
- DoT/damage_type 參數三原語共用——fireball 改 bolt 形只動 JSON 一行。

### 內容範例（actions.json 增量）

- `flame_bolt`：bolt, range 6, damage 5, 燃燒 2 回合——遠程主力，需直線視野
- `frost_cone`：cone, radius 3, damage 3 + 新增 Slow debuff（見 03）——近身群控
- `smite` 改 `smite_target` 單體——現在的 smite（nova r1）與 fireball 同質化了

### 前端牽動

bolt/cone 需要**瞄準模式**：`map_view.gd` 按技能鍵 → 進入方向選擇 →
方向鍵確定後才 `submit_hero_skill`。core 不變，純前端狀態機。
smite_target 需要游標選格（中）——可先只做 8 方向版（高）。

## 2. 資源消耗：cooldown 先做、MP 後做（優先序：cooldown 高 / MP 中）

依賴：`src/core/turn/turn_scheduler.h`（submit 路徑）、`turn_world.h`（`on_actor_turn`
hook 是現成的「每回合遞減」掛點）、`src/core/turn/npc_brain.cpp`（NPC 也要遵守）。

### 2.1 Cooldown（高）——一個 component 就有戰術取捨

```cpp
struct CooldownsComponent {
    std::unordered_map<int, int> remaining;  // ActionDef index → 剩餘回合
};
```

- `ActionDef` 加 `cooldown` 欄位；resolve 成功時寫入 map；
  `on_actor_turn` hook 內遞減（與 `tick_timed_effects` 同處掛）。
- **submit 時不擋、resolve 時檢查**：cd 未到 → 動作 fizzle、發 ZoneEvent、
  不耗能量（A/B）/不啟動 channel（B）。理由：擋在 submit 會讓三排程器
  各自需要回絕路徑，resolve 統一檢查最省。前端用 `get_debug_text` 同款
  介面曝 cd 狀態，灰掉按鍵。
- NPC 同樣吃 cd：`decide_chase` 的 caster 分支查 cd，cd 中改近戰——
  立刻消除「caster 相鄰無限 nova」的單調感，零新 AI 邏輯。

### 2.2 MP（中）——等「回魔」有設計後再上

```cpp
struct ManaComponent { int mp; int max_mp; };  // ActionDef 加 mana_cost
```

- 先決問題不是扣魔而是**回魔節奏**：每回合自然回（趨向無限資源、只是節流）
  vs 殺敵/受擊回（主動資源、更 roguelike）。傾向後者，但需要戰鬥密度數據，
  所以排在 cooldown 之後。
- MP 與 cd 並存定位：cd = 單技能節奏；MP = 跨技能總量預算（連放不同技能也會乾）。

## 3. 技能升級（優先序：中）

依賴：`src/core/turn/action_def.h`（ActionLibrary）、`data/actions.json`、
05 角色成長（技能點數來源）。

### 提案：JSON 定義等級陣列，不寫升級公式

```json
{ "name": "fireball", "levels": [
    { "weight": 3, "nova_damage": 4, "radius": 1, "dot_turns": 3 },
    { "weight": 3, "nova_damage": 6, "radius": 1, "dot_turns": 3 },
    { "weight": 2, "nova_damage": 6, "radius": 2, "dot_turns": 4 } ] }
```

- `ActionLibrary::at(i)` 變 `at(i, level)`；actor 掛
  `KnownSkillsComponent { map<def_index, level> }`。
- **升級改質不只改量**：上例 lv3 詠唱 3→2 回合 + 半徑 +1——升級改變
  「何時敢放」的決策，而非單純數字膨脹。weight 下降在 B 排程器
  （`energy_channel_scheduler.cpp`）= 打斷窗口變短，手感差異真實可感。
- NPC 沿用：spawn 時依樓層深度給技能等級，免費獲得難度曲線。

## 4. 不做的事

- **不做**技能樹圖（節點解鎖拓撲）——等級陣列 + 點數已夠，樹是 UI 投資陷阱。
- **不做** spell crafting / 詞綴組合技能——ActionDef 原語組合已提供資料層
  的同等能力，留給內容（JSON）而非機制。
