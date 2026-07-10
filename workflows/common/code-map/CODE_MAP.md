# CODE_MAP — 程式碼導航 index

gamecore-zone：C++（EnTT ECS）核心 + Godot 4.6 GDExtension（godot-mono）回合制 roguelike 原型。
repo 不大，全部放單檔，不拆子 index。細節敘事（資料流、函數逐一說明）見
[../../../docs/架構與函數說明.md](../../../docs/架構與函數說明.md)；本檔只負責「東西在哪」。

## 修改前規則

1. 先判斷要改的功能屬於哪個領域（core / godot 前端 / tests）。
2. 只讀對應章節列出的檔案；需要更深入的函數說明才去查 `docs/架構與函數說明.md`。
3. 若本檔缺資料或與程式碼衝突，以程式碼為準，立即修正本檔。

## 常用起點

- **改回合邏輯**（誰先動、動作怎麼結算）→ `src/core/turn/`（`turn_engine.*`、`apply_action.*`）。
- **改存檔**→ `src/core/serialize/all_components.h` 必看（可序列化 component 的唯一真相來源，新增/刪除元件務必同步）。
- **改前端操作**（按鍵、畫面、HUD）→ `godot_zone/map_view.gd` + `src/gbind/zone_world_gd.{h,cpp}`。
- **改地圖生成**→ `src/core/maps/map_gen.cpp`（BSP）+ `src/core/maps/map_data.h`。
- **改 NPC 行為**→ `src/core/turn/apply_action.cpp` 的 `decide_chase`（回合引擎實際用的 AI）；`src/core/systems/npc_ai_system.*` 是另一套較完整但可能未接主迴圈的群體 AI，改動前先確認是否真的被呼叫。

## Core（`src/core/` — godot-free，純 C++/EnTT）

### `turn/` — 回合引擎與動作結算

| 檔案 | 職責 |
|------|------|
| `src/core/turn/turn_engine.h` / `.cpp` | `TurnEngine`：actor 清單（加入序＝行動序）、`begin_round()`/`step()` 推進回合、玩家 `submit()` 阻塞續跑。 |
| `src/core/turn/apply_action.h` / `.cpp` | `apply_action()` 把單一 `Action` 結算到世界（撞牆/撞擊攻擊/拾取/下樓梯）；`decide_chase()` 為回合引擎實際使用的最簡 NPC AI。 |
| `src/core/turn/action.h` | `Action` 值型別：`Idle` / `Move` / `Wait`（無獨立攻擊動作，撞擊即攻擊）。 |
| `src/core/turn/move_dir.h` | `encode_dir`/`decode_dir`：8 方向 `(dx,dy)` 與整數互轉。 |
| `src/core/turn/zone_event.h` | `ZoneEvent`：核心對外事件（`BumpedWall`/`BumpedActor`/`ActorDied`/`ItemPickedUp`/`ReachedStairDown`）。 |

### `components/` — ECS 資料

| 檔案 | 職責 |
|------|------|
| `src/core/components/actor_component.h` | `ActorComponent`：空 tag，標記有行動資格的實體。 |
| `src/core/components/controller_component.h` | `ControllerComponent`：Player / Self / Faction 操控者類型。 |
| `src/core/components/act_point_component.h` | `ActPointComponent`：行動點數（每回合重設 1000）。 |
| `src/core/components/hero_component.h` | `HeroComponent`：空 tag，正向標記英雄。 |
| `src/core/components/spatial_component.h` | `SpatialComponent`：座標 `x,y` + `parent`。 |
| `src/core/components/health_component.h` | `HealthComponent`：`hp`/`max_hp`。 |
| `src/core/components/combat_stats_component.h` | `CombatStatsComponent`：`attack`、`move_chance`。 |
| `src/core/components/npc_ai_component.h` | `NpcAiComponent`：`alerted`/`alert_turns`/`is_caster`（`is_caster` 為技能移除後的歷史殘留欄位）。 |
| `src/core/components/item_component.h` | `ItemComponent`：`type`（目前只有 `health_potion`）、`value`。 |
| `src/core/components/world_state_component.h` | `WorldStateComponent`：`turn_count`、`current_floor`，掛在地圖實體上並隨存檔保存。 |

### `systems/` — ECS 規則

| 檔案 | 職責 |
|------|------|
| `src/core/systems/fov_system.h` / `.cpp` | `compute_fov()`（Bresenham 視野計算，更新 visible/explored）、`los()`（兩點視線檢查）。 |
| `src/core/systems/npc_ai_system.h` / `.cpp` | 群體 NPC AI（警覺/追蹤/徘徊，固定種子 RNG）；與 `decide_chase` 平行存在的另一套，是否仍接在主迴圈上待確認。 |

### `maps/` — 地圖

| 檔案 | 職責 |
|------|------|
| `src/core/maps/tile.h` | `Tile`：單格 terrain id + flags（可走/擋視線/下樓梯）。 |
| `src/core/maps/map_data.h` | `MapData`：整張地圖的稠密 tile 網格（一張地圖＝一個元件，非每格一個實體）。 |
| `src/core/maps/map_gen.h` / `.cpp` | `generate_bsp_dungeon()`：BSP 地下城生成，回傳房間清單（第 0 間＝英雄起始房）。 |

### `serialize/` — 存讀檔（cereal）

| 檔案 | 職責 |
|------|------|
| `src/core/serialize/all_components.h` | `AllComponents`：**存檔元件唯一真相來源**，新增/刪除可序列化 component 必改這裡。 |
| `src/core/serialize/save_load.h` | `save()`/`load()`：registry ↔ bytes/檔案/SaveStore 三層 API。 |
| `src/core/serialize/entt_cereal_archive.h` | cereal 與 EnTT snapshot 的轉接層。 |
| `src/core/serialize/save_store.h` | 儲存後端介面（如 folder-based save slot）。 |

### `ecs/` / `util/` / 其他

| 檔案 | 職責 |
|------|------|
| `src/core/ecs/entity_manager.h` / `.cpp` | `EntityManager`：`entt::registry` 薄封裝（create/destroy/valid、元件存取、`add_system`/`tick`）。 |
| `src/core/ecs/system_ctx.h` | `SystemCtx`：傳給系統的依賴包（目前空佔位）。 |
| `src/core/util/vector2i.h` | 整數 2D 向量工具型別。 |
| `src/core/util/resource_path.h` | 資源路徑相關工具。 |
| `src/core/version.h` / `.cpp` | 版本字串。 |

## Godot 前端（`src/gbind/` 橋樑 + `godot_zone/` 場景）

`src/core/` 完全不依賴 Godot；只有 `src/gbind/` 認識 Godot（`#include <godot_cpp/...>` 只能出現在這裡）。

| 檔案 | 職責 |
|------|------|
| `src/gbind/zone_world_gd.h` / `.cpp` | `ZoneWorld`：唯一橋樑類別，持有 `EntityManager` + `TurnEngine`；對 GDScript 暴露 `player_move`/`player_wait`/`next_round`/`is_waiting_player`/`round_in_progress`、地圖與狀態查詢、`save_game`/`load_game`、debug/trace 介面。內部 `pump()` 驅動 `step()`、`drain_events()` 把核心事件翻成 Godot signal。 |
| `src/gbind/zone_core_gd.h` / `.cpp` | `ZoneCore`（`RefCounted`）：小型輔助類別，目前僅暴露 `version()`。 |
| `src/gbind/register_types.h` / `.cpp` | GDExtension 模組進出點：`initialize_zone_module`/`uninitialize_zone_module`，註冊 `ZoneWorld`/`ZoneCore` 等類別給 Godot。 |
| `godot_zone/map_view.tscn` + `map_view.gd` | 主場景：收按鍵 → 呼叫 `ZoneWorld` 方法；接 signal → 更新畫面/HUD。 |
| `godot_zone/verify.gd` | headless 端到端驗證腳本，跑通會輸出 `VERIFY PASSED`。 |
| `godot_zone/smoke_test.gd` | 前端 smoke test 腳本。 |
| `godot_zone/zone.gdextension` | GDExtension descriptor，指向 `res://bin/libzone_gd.so`（等平台變體）。 |
| `godot_zone/project.godot` | Godot 專案設定。 |
| `godot_zone/bin/` | 編譯產物（`.so`/`.dll`，gitignore），由 `cmake --build build --target zone_gd` 建置後自動複製過來，不手動維護。 |

## Tests（`tests/src/`，doctest）

| 檔案 | 覆蓋對象 |
|------|----------|
| `tests/src/main.cpp` | doctest 入口（`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`），不含測試案例。 |
| `tests/src/smoke_test.cpp` | 版本字串等最基本 smoke test。 |
| `tests/src/test_ecs.cpp` | `EntityManager`：create/destroy/valid、元件操作、`add_system`+`tick`、系統執行序、`ActorComponent` 作為 actor 標記。 |
| `tests/src/test_turn_engine.cpp` | `TurnEngine`：加入序＝回合序、每回合 act_point 重設、玩家 actor 阻塞 `submit()` 後續跑、撞擊致死發 `ActorDied`。 |
| `tests/src/test_npc_combat.cpp` | NPC AI 戰鬥邏輯：警覺鄰接攻擊、實體建立順序回歸、`move_chance=0` 仍必定攻擊、`HeroComponent` 序列化後仍可辨識。 |
| `tests/src/test_phase4.cpp` | `MapData`（resize/in_bounds/at/tile 旗標/序列化 round-trip）、`WorldStateComponent` 序列化、移動系統撞牆阻擋、地圖+英雄+NPC 多實體序列化。 |
| `tests/src/test_serialize.cpp` | 存讀檔：`SpatialComponent`（含 `parent` entity）stream round-trip、空 registry、只有 `HeroComponent`、檔案路徑 save/load、`FolderSaveStore` 存取與不存在 slot 的容錯。 |
| `tests/CMakeLists.txt` | `zone_test` 執行檔定義：連結 `zone_core` + `doctest::doctest`，`add_test(NAME zone_test ...)`。 |

（29 test case / 93 assertion，見 [../../../docs/架構與函數說明.md](../../../docs/架構與函數說明.md) 第 6 節。）
