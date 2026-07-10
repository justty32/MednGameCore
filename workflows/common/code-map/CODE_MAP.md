# CODE_MAP — 程式碼導航 index

gamecore-zone：C++（EnTT ECS）核心 + Godot 4.6 GDExtension（godot-mono）前端，**兩個平行場景**——
最傳統回合制地城探索 roguelike（`TurnEngine`）＋ SRPG 戰棋戰鬥（`tactics::Battle`）。
兩者共用 ECS／地圖／存讀檔基建，回合驅動各自獨立、互不干涉。
repo 不大，全部放單檔，不拆子 index。細節敘事（資料流、函數逐一說明）見
[../../../docs/架構與函數說明.md](../../../docs/架構與函數說明.md)；本檔只負責「東西在哪」。

## 修改前規則

1. 先判斷要改的功能屬於哪個領域（core / godot 前端 / tests）。
2. 只讀對應章節列出的檔案；需要更深入的函數說明才去查 `docs/架構與函數說明.md`。
3. 若本檔缺資料或與程式碼衝突，以程式碼為準，立即修正本檔。

## 常用起點

- **改回合邏輯（地城）**（誰先動、動作怎麼結算）→ `src/core/turn/`（`turn_engine.*`、`apply_action.*`）。
- **改存檔**→ `src/core/serialize/all_components.h` 必看（可序列化 component 的唯一真相來源，新增/刪除元件務必同步）。
- **改前端操作（地城）**（按鍵、HUD、訊息列）→ `godot_zone/map_view.gd` + `src/gbind/zone_world_gd.{h,cpp}`；**改畫面上畫什麼 tile/顏色**→ `godot_zone/dungeon_view.gd`（唯一決定渲染的地方）。
- **改地圖生成（地城）**→ `src/core/maps/map_gen.cpp`（BSP）+ `src/core/maps/map_data.h`。
- **改 NPC 行為（地城）**→ `src/core/turn/npc_ai.{h,cpp}` 的 `decide_npc()`（回合引擎唯一使用的 NPC 決策函式：純函式，只回傳 `Action`，警覺/追擊/徘徊都在這裡；世界的改動仍交給 `apply_action()`）。
- **改戰棋規則**（行動條、可達集、攻擊判定、AI、回合子狀態）→ `src/core/tactics/`（見下方「Core / tactics」表）。**`TurnEngine`（地城）與 `ActionBar`（戰棋）是刻意分離的兩套系統，不共用、不互相依賴**——地城側「最傳統回合制」保持精簡，戰棋側才是 ToME4 行動值模型；改一邊前先確認自己在哪個模組。

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

### `tactics/` — SRPG 戰棋核心（godot-free，與地城 `turn/` 平行、不共用）

設計記錄：[workflows/specs/2026-07-10-srpg-tactics-battle-design.md](../../specs/2026-07-10-srpg-tactics-battle-design.md)。

| 檔案 | 職責 |
|------|------|
| `src/core/tactics/action_bar.h` / `.cpp` | `ActionBar`：ToME4 行動值排程。`kCtToAct=1000`。`advance()` 每 tick 對所有單位 `TurnGaugeComponent::ct += TacticsUnitComponent::speed`，第一個達標者出手，平手取 ct 高者、再平手取加入序在前者。`consume()` 是 `ct -= kCtToAct`（**不歸零**，溢出保留，快角偶爾連動兩次）。`forecast(reg, n)` 在複本上模擬，不動真實 ct（行動條 UI 用）。 |
| `src/core/tactics/move_range.h` / `.cpp` | `unit_at()` 查某格站著哪個戰棋單位；`compute_move_field()` 四方向 BFS（每步花費固定為 1，等價 Dijkstra），牆與**其他單位佔格**皆不可通行（不能穿人也不能停在人身上）；`reachable_tiles()`、`path_to()`（沿 dist 場回溯，只給前端做走路動畫，核心結算是瞬移）。 |
| `src/core/tactics/tactics_action.h` | `TacticsAction{Wait, MoveTo, Attack}` 值型別，玩家與 AI 共用。 |
| `src/core/tactics/tactics_event.h` | `TacticsEvent{Moved, Attacked, UnitDied, Rejected}`，橋樑層讀取後翻成 Godot signal。 |
| `src/core/tactics/apply_tactics_action.h` / `.cpp` | `apply_tactics_action()`：結算一個 `TacticsAction`，**只驗證幾何合法性**（可達／射程／是否敵隊），不管「這回合是否已移動過」——那是 `Battle` 的子狀態職責。回傳 bool；被殺死的單位只發 `UnitDied`，不在此 destroy（避免迭代失效，由 `Battle` 統一移除）。 |
| `src/core/tactics/tactics_ai.h` / `.cpp` | `decide_tactics(reg, map, self, moved, acted)`：純決策函式，回傳 `TacticsAction`，與地城 `decide_npc()` 同一個模式。選格排序鍵（依序）：`deficit = max(最近敵人距離 - attack_range, 0)` 越小越好 → `nearest` **越大越好**（進得了射程就別再靠近，第二鍵是關鍵，少了它遠程單位會走進近戰送死）→ `steps` 越小越好。 |
| `src/core/tactics/battle.h` / `.cpp` | `Battle`：一場戰鬥的子狀態機。一個單位的回合是「移動一次＋行動一次」，攻擊即結束回合（可移動後攻擊，不可攻擊後再移動）。`begin_next_turn()`、`command()`（含子狀態閘門）、`run_ai_turn()`（AI 打完 `current()` 整個回合，最多 3 步防卡死）、`winner()`（殲滅一方即分勝負，兩隊同歸於盡不判）。死亡單位從行動條移除並 `destroy()`。 |

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
| `src/core/components/team_component.h` | `TeamComponent`：`uint8_t team`（`TEAM_PLAYER=0`／`TEAM_ENEMY=1`）。只給戰棋用，勝負＝某隊清空；與 `ControllerComponent`（誰下指令）無關。 |
| `src/core/components/tactics_unit_component.h` | `TacticsUnitComponent`：`move_range`（Dijkstra 步數，預設 4）、`attack_range`（Chebyshev 射程，預設 1）、`speed`（行動值累積速率，預設 100）。只給戰棋單位用。 |
| `src/core/components/turn_gauge_component.h` | `TurnGaugeComponent`：`int ct`，`>= ActionBar::kCtToAct` 即可出手。 |

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
| `src/gbind/tactics_battle_gd.h` / `.cpp` | `TacticsBattle : Node`：SRPG 戰棋的橋樑層，與 `ZoneWorld` 平行、互不依賴、不共用任何狀態。持有自己的 `entt::registry`＋`MapData`＋`zone::tactics::Battle`。內部生成 20×14 開闊競技場（含障礙與 flood-fill 連通性檢查，重試 32 次仍失敗就退回全開闊），雙方各 3 個原型單位（戰士/弓手/斥候）。對 GDScript 暴露：`start_battle(seed)`（**刻意不在 `_ready()` 自動開局**——開局的 `pump()` 會跑敵方 AI 並發 signal，前端須先接信號再呼叫）、地圖查詢（`get_tile_flags()` 只有 bit0 可走，戰棋無戰爭迷霧）、`get_units()`/`get_turn_order(n)`、當前回合查詢（`current_unit()`/`is_waiting_player()`/`can_move()`/`can_act()`）、`get_move_tiles()`/`get_attack_targets()`/`get_path_to()`、玩家指令 `command_move/attack/wait()`、`get_winner()`/`is_battle_over()`。signal：`turn_started`/`unit_moved`/`unit_attacked`/`unit_died`/`battle_over`。單位 id＝`entt::to_integral(entity)`。 |
| `src/gbind/register_types.h` / `.cpp` | GDExtension 模組進出點：`initialize_zone_module`/`uninitialize_zone_module`，註冊 `ZoneWorld`/`ZoneCore`/`TacticsBattle` 等類別給 Godot。 |
| `godot_zone/map_view.tscn` + `map_view.gd` | 地城主場景：收按鍵（WASD/方向鍵/數字鍵移動、`.`/KP5 等待、R 重開、F5/F9 存讀檔、Tab 開關除錯面板、**Esc 回主選單**）→ 呼叫 `ZoneWorld` 方法；接 signal → 更新 HUD／訊息列／死亡與勝利 overlay。**不決定 tile 或顏色**，只管 HUD/訊息/輸入。 |
| `godot_zone/dungeon_view.gd` | 地城繪製層：唯一決定「畫哪個 tile、什麼顏色」的地方。`_draw()` 逐格從 `assets/tilesheet.png` 取圖塊；霧中戰爭＝同一圖塊調暗/偏冷色，不用 shader。**已改用 `tile_atlas.gd` 的 `TileAtlas`**——不再自己定義 `T_*` 常數或 preload 圖集。 |
| `godot_zone/tile_atlas.gd` | `class_name TileAtlas`：圖集座標（`Vector2i`）與 `blit()` 繪製工具的唯一真相來源，**地城與戰棋兩個場景共用**。Kenney 1-Bit Pack，16×16 圖塊。 |
| `godot_zone/arena_view.gd` | 戰棋繪製層：與 `dungeon_view.gd` 平行，唯一決定戰棋畫面「畫哪個圖塊、什麼顏色」。疊法：地形 → 可移動格（半透明藍）→ 可攻擊目標框（紅）／當前單位框（黃）→ 單位 → 血條（滿血不畫）。資料全來自 `TacticsBattle`。 |
| `godot_zone/tactics_view.tscn` + `tactics_view.gd` | 戰棋場景：滑鼠操作（點藍格移動、點紅框敵人攻擊，攻擊後回合結束）、空白鍵待機、R 重開、Esc 回主選單。維護 `_seen_units` 快照——單位被致命一擊時已被 core 從 registry 移除，`get_units()` 查不到，訊息列靠快照仍能叫出名字。 |
| `godot_zone/main_menu.tscn` + `main_menu.gd` | **現為專案 main scene**（見 `project.godot`）。兩個入口：地城探索 → `map_view.tscn`；戰棋對戰 → `tactics_view.tscn`；Esc 退出遊戲。 |
| `godot_zone/assets/tilesheet.png` + `LICENSE-kenney-1bit.txt` | Kenney 1-Bit Pack（CC0）`monochrome-transparent_packed.png`，16×16 圖塊、49 欄、無間距，單色可任意 modulate 上色。 |
| `godot_zone/verify.gd` | 地城 headless 端到端驗證腳本：基本欄位檢查外，`_run_bot()` 用固定種子（`BOT_SEED`）跑 BFS 尋路機器人一路殺向樓梯，斷言遊戲必達結局（通關或戰死）、至少到第 2 層、且過程中英雄有掉血；跑通輸出 `VERIFY PASSED`。 |
| `godot_zone/verify_tactics.gd` | 戰棋 headless 端到端驗證腳本：固定種子（`BATTLE_SEED`）機器人（能打就打、否則走向最近敵人）打完一整場，斷言必達勝負、`battle_over` signal 與 `get_winner()` 一致、移動/攻擊/死亡事件都至少發生一次、戰鬥結束後不再接受指令；跑通輸出 `TACTICS VERIFY PASSED`。 |
| `godot_zone/smoke_test.gd` | 前端 smoke test 腳本。 |
| `godot_zone/zone.gdextension` | GDExtension descriptor，指向 `res://bin/libzone_gd.so`（等平台變體）。 |
| `godot_zone/project.godot` | Godot 專案設定；`run/main_scene` 為 `main_menu.tscn`。 |
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
| `tests/src/test_tactics.cpp` | 戰棋核心（22 case）：`ActionBar` 出手序＋溢出保留＋平手確定性＋`forecast` 一致且無副作用＋全員速度 0 不無限迴圈；`MoveField` 四方向步數上限＋牆繞路＋其他單位佔格不可穿不可停＋`path_to` 回溯；`apply_tactics_action` 移動可達性＋射程與敵我判定＋致死發 `UnitDied` 不 destroy；`decide_tactics` 射程內攻擊最脆目標＋射程外靠近＋遠程停在射程邊緣（第二鍵）＋近戰貼身＋無敵人待機；`Battle` 移動+攻擊子狀態＋攻擊後不能再移動＋死亡單位移除＋殲滅分勝負＋AI 打完整場不卡死。 |
| `tests/CMakeLists.txt` | `zone_test` 執行檔定義：連結 `zone_core` + `doctest::doctest`，`add_test(NAME zone_test ...)`。 |

（55 test case / 236 assertion，`./build/bin/zone_test` 實跑確認；見 [../../../docs/架構與函數說明.md](../../../docs/架構與函數說明.md) 第 6 節。）
