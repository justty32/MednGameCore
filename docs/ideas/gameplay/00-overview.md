# gameplay ideas 總覽與路線圖

> 日期：2026-06-11（路線圖內容）
> 狀態更新：2026-06-21
> 定位：docs/ideas/gameplay/ 目錄的索引——各主題檔的關係、優先序總表、建議推進順序。

---

## 1. 這個目錄是什麼

**現況（2026-06-21 回合系統重構後）**：gamecore-zone 的回合核心已大幅簡化為一個
**最傳統、最精簡的回合制引擎 `TurnEngine`**——維護一條 actor 順序串列、玩家行動暫停等指令、
NPC 立即決策追擊。現況玩法極簡：**移動、撞擊攻擊（移入有 actor 的格子即扣傷害）、
踩到 health_potion 自動回血、踩下樓梯進下一層、NPC 追擊、HP 歸零 game over**。
原先的「三種可切換排程器（A 能量瞬發 / B 能量+channel / C 純 tick）、JSON 資料化技能、
DoT、即時打斷、施法者 NPC、排程器等價性」整套已**整體移除**。

本目錄記錄**下一階段「遊戲機制與內容深度」**的前瞻設計想法——每檔一個主題、
標註優先序與依賴的系統，作為日後 brainstorming → spec → plan 的素材庫。
**這些主題（戰鬥深度、技能、狀態效果、裝備、成長、隊伍、能量綜效）一律是
「建立在最簡 TurnEngine 之上的未來可選擴充」，目前皆未實作。**

這些是**想法，不是定案 spec**。動工前仍應走 brainstorming 收斂成
`docs/superpowers/specs/` 的正式設計文件。

## 2. 檔案地圖

| 檔案 | 主題 | 一句話 |
|---|---|---|
| `01-combat-depth.md` | 戰鬥深度 | 命中/閃避/暴擊/抗性 + 傷害類型 + DoT 疊加規則（未來方向） |
| `02-skill-targeting-resources.md` | 技能系統擴充 | 技能系統已移除；本文為「技能蓋在 Action/apply_action 上」的未來設計 |
| `03-status-effects.md` | 狀態效果框架 | 狀態效果已移除；本文為「重新引入時於 TurnEngine 回合點結算」的未來設計 |
| `04-items-equipment.md` | 物品與裝備 | 現況僅 health_potion 自動拾取；inventory / 裝備欄為未來方向 |
| `05-character-growth.md` | 角色成長 | 屬性模型與「用進廢退」無等級制提案（未來方向） |
| `06-party-tactics.md` | 多角色戰術 | ControllerComponent(Faction) 是隊伍操控落地點；其餘未來方向 |
| `07-scheduler-synergy.md` | 能量/先攻 × 機制 | 排程器已移除；若未來引入能量/先攻時的玩法可能性 |

## 3. 優先序總表（高優先項彙整）

> 注意：以下「依賴」欄已改成現況檔案。原先引用的 `resolve_nova`/`dot_kind`/
> `TimedEffect`/`InventoryComponent` 等都隨重構移除，相關設計需先**重新引入對應子系統**
> 才有意義（已在各檔頂部標註）。本表反映的是「值得優先探討的設計切入點」。

| 想法 | 出處 | 為什麼值得先探討 |
|---|---|---|
| 命中/閃避/暴擊判定 | 01 | 一切戰鬥數值的地基；改 `apply_action` 撞擊攻擊路徑一處即可 |
| 傷害類型 enum + 抗性 component | 01 | 撞擊傷害目前是 `CombatStatsComponent.attack` 直減，定型趁早 |
| 狀態效果框架（重新引入） | 03 | buff/debuff 是後續一切機制的載體，但需先重建 TimedEffect 子系統 |
| 技能系統（重新引入，蓋在 Action 上） | 02 | Action 目前只有 Idle/Move/Wait，技能是內容深度的主引擎 |
| cooldown（比 MP 優先） | 02 | 隨技能系統一起；一個 component + apply 檢查即有取捨 |
| InventoryComponent（拾取→背包） | 04 | 拾取已有雛形（`apply_action` 內 health_potion），補上容器即成系統 |

## 4. 建議推進順序（三波）

**第一波（地基，彼此獨立可並行）**
1. 傷害類型 + 抗性（01 §2）——定義 `DamageType`，`apply_action` 撞擊路徑吃型別參數
2. 狀態效果框架（03 §2）——重新引入 TimedEffect，於 TurnEngine 回合點結算
3. 命中/閃避/暴擊（01 §1）——掛在 CombatStats 擴充上

**第二波（內容引擎）**
4. 技能系統 + cooldown（02）——把技能蓋在 Action/apply_action 之上
5. Inventory + 消耗品（04 §2-3）——讓拾取有意義

**第三波（系統互鎖）**
6. 裝備欄（04 §4，依賴屬性模型）
7. 角色成長（05，依賴屬性模型 + 戰鬥公式）
8. 多角色協同（06，依賴 ControllerComponent(Faction) 落地）

## 5. 共用依賴速查（現況實際檔案路徑）

- 回合引擎：`src/core/turn/turn_engine.{h,cpp}`（actor 順序串列、`step`、`begin_round`）
- Action：`src/core/turn/action.h`（`Action{kind,param}`，`ActionKind{Idle,Move,Wait}`）
- 動作結算：`src/core/turn/apply_action.{h,cpp}`（撞牆/撞 actor 攻擊/移動/拾取/下樓梯、
  `decide_chase` NPC AI）；方向編碼 `src/core/turn/move_dir.h`
- 元件：`src/core/components/`（`combat_stats_component.h`、`controller_component.h`、
  `act_point_component.h`、`health_component.h`、`item_component.h`、`npc_ai_component.h`）
- 序列化清單：`src/core/serialize/all_components.h`
- 前端接線：`src/gbind/zone_world_gd.{h,cpp}`（ZoneWorld：`next_round`/`player_move`/
  `player_wait`）、`godot_zone/map_view.gd`

> 已移除（不要再引用為現存）：`action_def.{h,cpp}`、`zone_effects.{h,cpp}`、
> `timed_effect.h`、`turn_scheduler.h`、`turn_world.h`、各 `*_scheduler.cpp`、
> `npc_brain.{h,cpp}`、`data/actions.json`。

## 6. 不變原則（所有想法都必須遵守）

1. **core 先行**：機制一律先在 `zone_core` 純 C++ 落地 + doctest，再接 GDExtension。
2. **資料化優先**：能放進資料表的參數不寫死；行為邏輯保持 C++ 可組合原語。
3. **建立在最簡 TurnEngine 之上**：新機制要能乾淨地掛上現有 `step`/`apply_action` 流程，
   不重蹈「為時間模型過度設計」的覆轍（這正是上一輪排程器實驗被整體移除的教訓）。
4. **對稱 actor 模型**：玩家/NPC 共用機制，無玩家特判——分派只看 `ControllerComponent.kind`。
5. **trace 可觀測**：每個新機制透過 `EngineCtx.trace` 吐細節字串，沿用既有 debug 管線
   （`zone_world_gd.cpp` 的 ring buffer / `get_debug_log`）。
