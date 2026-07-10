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
- **改前端操作**（按鍵、HUD、訊息列）→ `godot_zone/map_view.gd` + `src/gbind/zone_world_gd.{h,cpp}`；**改畫面上畫什麼 tile/顏色**→ `godot_zone/dungeon_view.gd`（唯一決定渲染的地方）。
- **改地圖生成**→ `src/core/maps/map_gen.cpp`（BSP）+ `src/core/maps/map_data.h`。
- **改 NPC 行為**→ `src/core/turn/npc_ai.{h,cpp}` 的 `decide_npc()`（回合引擎唯一使用的 NPC 決策函式：純函式，只回傳 `Action`，警覺/追擊/徘徊都在這裡；世界的改動仍交給 `apply_action()`）。

## Core（`src/core/` — godot-free，純 C++/EnTT）

### `turn/` — 回合引擎與動作結算

| 檔案 | 職責 |
|------|------|
| `src/core/turn/turn_engine.h` / `.cpp` | `TurnEngine`：actor 清單（加入序＝行動序）、`begin_round()`/`step()` 推進回合、玩家 `submit()` 阻塞續跑。 |
| `src/core/turn/apply_action.h` / `.cpp` | `apply_action()` 把單一 `Action` 結算到世界（撞牆/撞擊攻擊/拾取/下樓梯）；`actor_at()` 查某格站著哪個 actor（撞擊判定、NPC 尋路避讓共用，公開在標頭）。 |
| `src/core/turn/npc_ai.h` / `.cpp` | `decide_npc(reg, map, self, target, rng, cfg)`：回合引擎唯一使用的 NPC 決策函式（純決策，唯一副作用是更新自己的 `NpcAiComponent` 警覺狀態）。決策順序：LOS+`sight_radius` 更新警覺／`alert_memory` 逐回合遞減 → 已警覺且相鄰即撞擊攻擊（不受 `move_chance` 閘門）→ `move_chance` 擲骰 → 已警覺則大 delta 軸優先追擊、受阻換另一軸 → 未警覺則四方向隨機徘徊；全程避開牆與其他 actor。`NpcAiConfig{sight_radius=10, alert_memory=6}`。 |
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
| `src/core/systems/fov_system.h` / `.cpp` | `compute_fov()`（Bresenham 視野計算，更新 visible/explored）、`los()`（兩點視線檢查，`decide_npc()` 判斷能否看見目標用）。 |

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
| `src/gbind/zone_world_gd.h` / `.cpp` | `ZoneWorld`：唯一橋樑類別，持有 `EntityManager` + `TurnEngine` + 一個共用 `std::mt19937 rng_`（地圖生成與 NPC AI 共用種子，`set_seed`/`get_seed` 可重現整場）。對 GDScript 暴露：回合制介面 `player_move`/`player_wait`/`next_round`/`is_waiting_player`/`round_in_progress`；呈現層資料來源 `get_tile_flags()`（PackedByteArray，row-major 地形旗標）與 `get_visible_entities()`（Array[Dictionary]，視野內實體，繪製序＝陣列序）；狀態查詢含 `get_hero_attack()`、`get_win_floor()`、`is_game_won()`；`save_game`/`load_game`；debug/trace 介面。**沒有 `generate_map_image()`**——渲染完全交給 `godot_zone/dungeon_view.gd`。內部 `pump()` 驅動 `step()`、`drain_events()` 把核心事件翻成 Godot signal（`hero_attacked`/`hero_hurt`/`npc_died`/`game_won`/`hero_grew` 等）。 |
| `src/gbind/zone_core_gd.h` / `.cpp` | `ZoneCore`（`RefCounted`）：小型輔助類別，目前僅暴露 `version()`。 |
| `src/gbind/register_types.h` / `.cpp` | GDExtension 模組進出點：`initialize_zone_module`/`uninitialize_zone_module`，註冊 `ZoneWorld`/`ZoneCore` 等類別給 Godot。 |
| `godot_zone/map_view.tscn` + `map_view.gd` | 主場景：收按鍵（WASD/方向鍵/數字鍵移動、`.`/KP5 等待、R 重開、F5/F9 存讀檔、Tab 開關除錯面板）→ 呼叫 `ZoneWorld` 方法；接 signal → 更新 HUD／訊息列／死亡與勝利 overlay。**不決定 tile 或顏色**，只管 HUD/訊息/輸入。 |
| `godot_zone/dungeon_view.gd` | 地城繪製層：唯一決定「畫哪個 tile、什麼顏色」的地方。`_draw()` 逐格 `draw_texture_rect_region` 從 `assets/tilesheet.png` 取圖塊；霧中戰爭＝同一圖塊調暗/偏冷色，不用 shader。 |
| `godot_zone/assets/tilesheet.png` + `LICENSE-kenney-1bit.txt` | Kenney 1-Bit Pack（CC0）`monochrome-transparent_packed.png`，16×16 圖塊、49 欄、無間距，單色可任意 modulate 上色。 |
| `godot_zone/verify.gd` | headless 端到端驗證腳本：基本欄位檢查外，`_run_bot()` 用固定種子（`BOT_SEED`）跑 BFS 尋路機器人一路殺向樓梯，斷言遊戲必達結局（通關或戰死）、至少到第 2 層、且過程中英雄有掉血；跑通輸出 `VERIFY PASSED`。 |
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
| `tests/src/test_npc_combat.cpp` | `decide_npc()` + `apply_action()` 的戰鬥/AI 邏輯：警覺鄰接攻擊、target 由呼叫端傳入不受實體建立順序影響、`move_chance=0` 仍必定攻擊、`HeroComponent` 序列化後仍可辨識、從未看見英雄不會憑空警覺、警覺記憶（`alert_memory`）到期解除、直線追擊被牆擋住時改走另一軸不卡死、徘徊時四周被佔滿則等待不誤傷同類（共 8 個 test case）。 |
| `tests/src/test_phase4.cpp` | `MapData`（resize/in_bounds/at/tile 旗標/序列化 round-trip）、`WorldStateComponent` 序列化、移動系統撞牆阻擋、地圖+英雄+NPC 多實體序列化。 |
| `tests/src/test_serialize.cpp` | 存讀檔：`SpatialComponent`（含 `parent` entity）stream round-trip、空 registry、只有 `HeroComponent`、檔案路徑 save/load、`FolderSaveStore` 存取與不存在 slot 的容錯。 |
| `tests/CMakeLists.txt` | `zone_test` 執行檔定義：連結 `zone_core` + `doctest::doctest`，`add_test(NAME zone_test ...)`。 |

（33 test case / 129 assertion，見 [../../../docs/架構與函數說明.md](../../../docs/架構與函數說明.md) 第 6 節。）
