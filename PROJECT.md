# PROJECT — gamecore-zone

gamecore 三層架構（World/Region/Area）的第三層——區域層（Area Layer）。
Hard fork 自 derived/opennefia-cpp/，保留 ECS/FOV/戰鬥，刪除 OpenNefia 特有機制。
設計文件（歷史記錄，已被 2026-06-21 重構取代）：workflows/specs/archive/2026-06-03-gamecore-zone-design.md

---

## 進度快照（2026-07-10 可玩化 slice）

**當前理解（一句話）**：這一批把「回合引擎能跑」推進到「一場遊戲可以真的被玩完」——NPC AI 雙軌並存問題已解決（只留 `decide_npc`）、遊戲有明確勝利條件（第 5 層樓梯）與下樓成長、Godot 前端改用圖塊渲染＋HUD／訊息列，並用 headless BFS 機器人驗證整場可達結局。

**已落地（現況）**：
- **NPC AI 統一為 `decide_npc`**（`src/core/turn/npc_ai.{h,cpp}`）：純決策函式，回傳 `Action`，唯一副作用是更新自己的 `NpcAiComponent` 警覺狀態（LOS + `sight_radius=10`、`alert_memory=6` 回合記憶）。已警覺且相鄰＝撞擊攻擊（不受 `move_chance` 閘門）；否則移動受 `move_chance` 閘門；追擊採大 delta 軸優先、受阻換另一軸；未警覺則隨機徘徊；避開牆與其他 actor。**舊的兩套 AI 都已刪除**：`src/core/systems/npc_ai_system.*`（世界改動型群體 AI）與 `apply_action.cpp` 內的 `decide_chase`（回合引擎原本實際用的最簡追擊 AI）。`apply_action.h` 現在導出 `actor_at()`（原本是檔案內 static，撞擊判定與 NPC 尋路避讓共用）。
- **勝利條件與下樓成長**（`src/gbind/zone_world_gd.cpp`）：新增 `WIN_FLOOR=5`——走下第 5 層樓梯即通關（`is_game_won()`）。若沒有下樓成長，怪物逐層變強而英雄不變，第 4 層起打死一隻怪要挨滿 20 點傷、第 5 層打不贏——這是數學上不可能過關，因此每下一層樓 `HeroComponent`/`CombatStatsComponent` 會 `+5` 最大 HP、`+5` 治療、`+1` 攻擊（`HERO_MAXHP_PER_FLOOR`/`HERO_HEAL_PER_FLOOR`/`HERO_ATK_PER_FLOOR`），發 `hero_grew(max_hp, attack)` signal。新增 `get_hero_attack()`。英雄死亡不再 `destroy()`（死亡畫面仍要讀 HP/座標），`get_hero_hp()` 對外夾到 0；`restart()` 才真正清掉。
- **渲染責任移出核心**：`ZoneWorld` 移除 `generate_map_image(cell_px)`；新增 `get_tile_flags()`（PackedByteArray，row-major，bit0 可走/bit1 下樓梯/bit2 本回合可見/bit3 曾探索）與 `get_visible_entities()`（Array[Dictionary]，鍵 `x,y,kind,hp,max_hp`，只含視野內實體，繪製序＝陣列序，英雄最後）。`godot_zone/dungeon_view.gd` 是唯一決定「畫哪個 tile、什麼顏色」的地方，用 Kenney 1-Bit Pack（CC0，`godot_zone/assets/tilesheet.png`，16×16、49 欄）逐格 `draw_texture_rect_region`，霧中戰爭＝同圖塊調暗，不用 shader。
- **可重現的隨機性**：新增 `set_seed(int)`/`get_seed()`，一個 `std::mt19937 rng_` 同時驅動地圖生成與 NPC AI，固定種子可重現整場；`restart()` 若曾 `set_seed()` 則沿用該種子，否則抽新種子。
- **前端 HUD 化**（`godot_zone/map_view.gd` + `map_view.tscn`）：場景改為 `DungeonView` + `Camera2D(zoom 2)` + `HUD`（`Stats`／`HPBar+HPText`／`MessageLog` bbcode RichTextLabel／`DebugPanel`（Tab 切換，原本是 `L`）／死亡/勝利 `Overlay`）。所有遊戲回饋改走訊息列，不再 `print()`。移動按鍵不變；`L` 不再切 trace。
- **signal 變動**：`hero_bumped_npc(String)` → `hero_attacked(damage, target_hp_left)`；`npc_died(String)` → `npc_died()`（無參數）；新增 `hero_hurt(damage, hp_left)`（修掉一個真 bug——過去玩家被 NPC 打中時完全沒有回饋）；新增 `game_won(floor_num)`、`hero_grew(max_hp, attack)`。`world_changed`/`round_finished`/`floor_changed`/`hero_bumped_wall`/`item_picked_up`/`game_over` 不變。
- **`verify.gd` 重寫驗證方式**：舊版「英雄站著不動、NPC 必須找到並打中他」的斷言已移除——LOS-based AI 下，徘徊中的 NPC 未必找得到靜止的英雄，該假設不再成立。改用 `_run_bot()`：固定種子（`BOT_SEED=12345`）的 BFS 尋路機器人一路殺向樓梯，斷言遊戲必達結局（通關或戰死）、至少到第 2 層、且過程中英雄有掉血；另加 `get_tile_flags()`/`get_visible_entities()` 的欄位斷言。

**測試／驗證（現況數字）**：
- doctest **33 test case / 129 assertion 全綠**（`./build/bin/zone_test` 實跑確認）。`test_npc_combat.cpp` 已改測 `decide_npc` + `apply_action`（8 case：鄰接攻擊、target 傳入不受建立順序影響、`move_chance=0` 仍必攻擊、`HeroComponent` 序列化辨識、未見英雄不憑空警覺、警覺記憶到期、牆擋追擊換軸、徘徊避讓同類）。
- headless `verify.gd` → **VERIFY PASSED**（含 BFS 機器人端到端跑完整場）。

**建置鐵則**：建置務必限制並行（如 `-j4`）。曾因無上限 `-j` 觸發 OOM 強制重啟。

**待使用者驗證**：Godot GUI 實機開視窗確認圖塊渲染、HUD/訊息列可讀性、Tab 除錯面板——見 `WAIT_USER.md`。

---

## 進度快照（2026-06-21 回合系統重構）

> ⚠️ **部分內容已被 2026-07-10 slice 取代**：下方「`decide_chase` 最精簡 NPC 追擊 AI」已刪除（見上一節，改為 `decide_npc`）；`ZoneWorld` 新增的 `get_tile_flags`/`get_visible_entities`/`set_seed` 等介面、`hero_bumped_npc`/`npc_died(String)` 等舊 signal 簽名、測試數字（29/93）皆已變動，以上一節為準。以下段落其餘部分（`TurnEngine`、`Action`、元件、建置鐵則）仍然正確。

**當前理解（一句話）**：回合系統已**重構為最傳統、最精簡的 `TurnEngine`**，作為乾淨的基礎地基；
先前的能量排程器／JSON 技能／詠唱／DoT 整套實驗已**整體移除**（淨刪約 1300 行）。

**已落地（現況）**：
- `TurnEngine`（`src/core/turn/turn_engine.{h,cpp}`）：actor `std::list` 順序＝加入序（hero 最先），`begin_round()` → `step()` 直到 `RoundDone`；玩家 actor 回 `WaitingPlayer` 阻塞等 `submit()`，Self/Faction 立即 `decide()`＋`resolve()`。每 actor 本回合首次輪到時 `ActPointComponent` 重設 1000，行動後歸 0。
- `Action`（`src/core/turn/action.h`）：`{ kind; param }`，僅 `Idle / Move / Wait`，**無 Attack／技能／詠唱**（攻擊＝移動進入有 actor 的格子）。
- `apply_action`（`src/core/turn/apply_action.{h,cpp}`）：結算 Move（撞牆／撞擊攻擊／自動拾取／下樓梯）與 Wait/Idle，發 `ZoneEvent`；`decide_chase` 最精簡 NPC 追擊 AI。
- 元件：**新增** `ControllerComponent`（Player/Self/Faction）、`ActPointComponent`；**移除** `EnergyComponent`、`OngoingActionComponent`、`PlayerControlledComponent`；`serialize/all_components.h` 已同步。
- GDExtension `ZoneWorld`：新介面 `next_round()`／`player_move(dx,dy)`／`player_wait()`／`is_waiting_player()`／`round_in_progress()`；移除 `move`／`wait_turn`／`set/get_scheduler_mode`／`submit_hero_*`／`step_scheduler` 等舊 API。前端 `map_view.gd` 直接呼叫 `player_move`／`player_wait`。
- 建置：`zone_gd` POST_BUILD 自動複製 `libzone_gd.so` 到 `godot_zone/bin/`（該目錄 gitignore）。

**測試／驗證（現況數字）**：
- doctest **29 test case / 93 assertion 全綠**（`test_turn_engine.cpp` 涵蓋加入序＝回合序、act_point 重設 1000、玩家阻塞 submit 後續跑、撞 actor 致死發 `ActorDied`）。
- headless `verify.gd` → **VERIFY PASSED**（需先 `--headless --import`）；Godot 4.6 mono、zone core 0.0.1-alpha、60×40 地圖、save/load round-trip 綠。

**本次重構 commit（本地 main，尚未 push）**：
1. `e9ed694` refactor(turn)：重建為最傳統回合制引擎，移除能量排程系統。
2. `b7712f2` feat(godot)：前端改用新回合制介面（next_round/player_move/player_wait）。
3. `b54faef` build(gbind)：zone_gd 建置後自動複製 .so/.dll 到 godot_zone/bin/。

**建置鐵則**：建置務必限制並行（如 `-j4`）。曾因無上限 `-j` 觸發 OOM 強制重啟。

**後續可選擴充**：能量制／先攻／詠唱／技能／狀態效果／多角色 order 等，重新定位為「建立在最簡 `TurnEngine` 之上的未來主題」，設計探索保留於 `workflows/idea/`。

---

## 歷史設計記錄（已被 2026-06-21 重構取代）

> ⚠️ **狀態更新（2026-06-21）**：以下段落描述的是重構**前**的舊設計
> （對稱 `tick_actor` 模型、`OngoingActionComponent` 打斷、能量制／三排程器 A/B/C 等）。
> 這套實驗已於回合系統重構中**整體移除**，改為最簡 `TurnEngine`。
> 以下保留作歷史設計記錄，所引用的設計文件（現位於 `workflows/specs/archive/`、`workflows/plans/archive/`）若仍存在亦屬歷史記錄，已被重構取代。

**（歷史）回合迴圈設計定案（五條）**：
1. 對稱模型：玩家/NPC 共用 tick_actor，唯一差別在 provide_next_action()（玩家回 nullopt→idle 等輸入；NPC 回 AI 決策）
2. 介面 step() / step(cmd)：繼續 vs 換行動；dt 由 C++ 算（Godot 不傳 dt）
3. 玩家恆先手：先 tick hero，再迭代 view<ActorComponent>(exclude<HeroComponent>)
4. display_chunk_ 動態可調（Godot 設）；C++ 擁規則 dt = min(行動剩餘, display_chunk_)
5. 阻塞 = snapshot.hero.idle，C++ 永不 spin；「打斷」= 覆寫 OngoingActionComponent（無佇列）；「繼續」不是 Action，是 step() 不帶 cmd

**（歷史）核心上下文**：設計真相層曾在 `workflows/specs/archive/2026-06-03-gamecore-zone-design.md` §7 + 同目錄 `turn-loop-sketch.cpp`，以及後續 `workflows/specs/archive/2026-06-07-action-and-turn-scheduler-design.md`（能量制／三排程器）。上述設計皆已被 2026-06-21 重構取代。
