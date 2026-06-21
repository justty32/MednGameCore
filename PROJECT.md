# PROJECT — gamecore-zone

gamecore 三層架構（World/Region/Area）的第三層——區域層（Area Layer）。
Hard fork 自 derived/opennefia-cpp/，保留 ECS/FOV/戰鬥，刪除 OpenNefia 特有機制。
設計文件：docs/superpowers/specs/2026-06-03-gamecore-zone-design.md

---

## 進度快照（2026-06-21 回合系統重構）

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

**後續可選擴充**：能量制／先攻／詠唱／技能／狀態效果／多角色 order 等，重新定位為「建立在最簡 `TurnEngine` 之上的未來主題」，設計探索保留於 `docs/ideas/`。

---

## 歷史設計記錄（已被 2026-06-21 重構取代）

> ⚠️ **狀態更新（2026-06-21）**：以下段落描述的是重構**前**的舊設計
> （對稱 `tick_actor` 模型、`OngoingActionComponent` 打斷、能量制／三排程器 A/B/C 等）。
> 這套實驗已於回合系統重構中**整體移除**，改為最簡 `TurnEngine`。
> 以下保留作歷史設計記錄，所引用的設計文件（`docs/superpowers/specs/`、`plans/`、`turn-loop-sketch.cpp` 等）若仍存在亦屬歷史記錄，已被重構取代。

**（歷史）回合迴圈設計定案（五條）**：
1. 對稱模型：玩家/NPC 共用 tick_actor，唯一差別在 provide_next_action()（玩家回 nullopt→idle 等輸入；NPC 回 AI 決策）
2. 介面 step() / step(cmd)：繼續 vs 換行動；dt 由 C++ 算（Godot 不傳 dt）
3. 玩家恆先手：先 tick hero，再迭代 view<ActorComponent>(exclude<HeroComponent>)
4. display_chunk_ 動態可調（Godot 設）；C++ 擁規則 dt = min(行動剩餘, display_chunk_)
5. 阻塞 = snapshot.hero.idle，C++ 永不 spin；「打斷」= 覆寫 OngoingActionComponent（無佇列）；「繼續」不是 Action，是 step() 不帶 cmd

**（歷史）核心上下文**：設計真相層曾在 `docs/superpowers/specs/2026-06-03-gamecore-zone-design.md` §7 + 同目錄 turn-loop-sketch.cpp，以及後續 `2026-06-07-action-and-turn-scheduler-design.md`（能量制／三排程器）。上述設計皆已被 2026-06-21 重構取代。
