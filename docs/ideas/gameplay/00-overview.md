# gameplay ideas 總覽與路線圖

> 日期：2026-06-11
> 定位：docs/ideas/gameplay/ 目錄的索引——各主題檔的關係、優先序總表、建議推進順序。

---

## 1. 這個目錄是什麼

gamecore-zone 已完成「引擎骨架」：三種可切換排程器（A 能量瞬發 / B 能量+channel / C 純 tick）、
Action 可組合原語、JSON 資料化技能、DoT、即時打斷、NPC 追擊 AI、debug trace。
本目錄記錄**下一階段「遊戲機制與內容深度」**的前瞻設計想法——每檔一個主題、
標註優先序與依賴的既有系統，作為日後 brainstorming → spec → plan 的素材庫。

這些是**想法，不是定案 spec**。動工前仍應走 brainstorming 收斂成
`docs/superpowers/specs/` 的正式設計文件。

## 2. 檔案地圖

| 檔案 | 主題 | 一句話 |
|---|---|---|
| `01-combat-depth.md` | 戰鬥深度 | 命中/閃避/暴擊/抗性 + 傷害類型 + DoT 疊加規則 |
| `02-skill-targeting-resources.md` | 技能系統擴充 | 目標形狀（單體/直線/錐形）、MP/cooldown、技能升級 |
| `03-status-effects.md` | 狀態效果框架 | TimedEffectsComponent 泛化成 buff/debuff 通用框架 |
| `04-items-equipment.md` | 物品與裝備 | inventory / 裝備欄 / 消耗品的演進路線 |
| `05-character-growth.md` | 角色成長 | 屬性模型與「用進廢退」無等級制提案 |
| `06-party-tactics.md` | 多角色戰術 | order 表模型下的編隊、協同技、待命指令 |
| `07-scheduler-synergy.md` | 排程器 × 機制 | 哪些機制在 A/B/C 哪種時間模型下最有趣 |

## 3. 優先序總表（高優先項彙整）

| 想法 | 出處 | 為什麼先做 |
|---|---|---|
| 命中/閃避/暴擊三明治公式 | 01 | 一切戰鬥數值的地基；改 `resolve_move`/`resolve_nova` 兩處即可 |
| 傷害類型 enum + 抗性 component | 01 | `dot_kind` 已是雛形，趁 ActionDef 欄位還少時定型 |
| TimedEffect 泛化（stat_mod 欄位） | 03 | Burning/Poison/Regen 硬編三種已到極限；buff 是後續一切機制的載體 |
| 目標形狀 `shape` 欄位（self/nova/line/cone） | 02 | 現在全部技能都是 nova，戰術差異化的第一刀 |
| cooldown（比 MP 優先） | 02 | 只需一個 component + submit 檢查，立刻讓技能選擇有取捨 |
| InventoryComponent（拾取→背包） | 04 | 拾取已有雛形（`resolve_move` 內），補上容器即成系統 |

## 4. 建議推進順序（三波）

**第一波（地基，彼此獨立可並行）**
1. 傷害類型 + 抗性（01 §2）——定義 `DamageType`，`resolve_nova` 吃型別參數
2. TimedEffect 泛化（03 §2）——加 `stat_mod` 與來源欄位，DoT 變成泛化框架的特例
3. 命中/閃避/暴擊（01 §1）——掛在 CombatStats 擴充上

**第二波（內容引擎）**
4. 技能目標形狀 + cooldown（02）——actions.json 大幅增容的前提
5. Inventory + 消耗品（04 §2-3）——讓拾取有意義

**第三波（系統互鎖）**
6. 裝備欄（04 §4，依賴屬性模型）
7. 角色成長（05，依賴屬性模型 + 戰鬥公式）
8. 多角色協同（06，依賴 order 表實作落地）

## 5. 共用依賴速查（實際檔案路徑）

- Action 與原語：`src/core/turn/action.h`、`src/core/turn/action_def.h`、
  `src/core/turn/zone_effects.h`（`resolve_move`/`resolve_nova`/`tick_timed_effects`）
- 持續效果：`src/core/turn/timed_effect.h`
- 排程器：`src/core/turn/turn_scheduler.h`、`energy_instant_scheduler.cpp`、
  `energy_channel_scheduler.cpp`、`tick_remaining_scheduler.cpp`
- 世界打包與 hook：`src/core/turn/turn_world.h`（`TurnConfig`、`on_actor_turn`、`trace`）
- 元件：`src/core/components/`（`combat_stats_component.h`、`energy_component.h`、
  `health_component.h`、`item_component.h`、`npc_ai_component.h`）
- 資料：`data/actions.json`；NPC 決策：`src/core/turn/npc_brain.h`
- 前端接線：`src/gbind/zone_world_gd.cpp`、`godot_zone/map_view.gd`
- 設計真相層：`docs/superpowers/specs/2026-06-07-action-and-turn-scheduler-design.md`

## 6. 不變原則（所有想法都必須遵守）

1. **core 先行**：機制一律先在 `zone_core` 純 C++ 落地 + ctest，再接 GDExtension。
2. **資料化優先**：能放進 JSON 的參數不寫死；行為邏輯保持 C++ 可組合原語。
3. **三排程器相容**：新機制至少在 A/B/C 其中之一有意義，且不破壞另外兩種
   （詳見 07 的相容性矩陣）。
4. **對稱 actor 模型**：玩家/NPC 共用機制，無玩家特判——NPC 也吃 cooldown、
   也有抗性、也能裝備。
5. **trace 可觀測**：每個新機制在 `TurnWorld.trace` 吐細節字串，沿用 debug 管線。
