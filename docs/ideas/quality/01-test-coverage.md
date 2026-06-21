# 測試覆蓋盤點與缺口

> 日期：2026-06-21
> 定位：現有 doctest 29 cases / 93 assertions 覆蓋什麼、缺什麼——以可立即動工的測試案清單呈現。
>
> **狀態更新（2026-06-21）**：原盤點以三排程器 + 技能 + DoT 的測試檔為對象。回合系統重構後，`test_turn_scheduler.cpp` / `test_npc_brain.cpp` / `test_zone_effects.cpp` 已刪除，新增 `test_turn_engine.cpp`。本檔依新測試樹重新盤點，並重估缺口。

---

## 1. 現有覆蓋盤點（依檔案）

| 檔案 | 覆蓋 | 評估 |
|---|---|---|
| `test_turn_engine.cpp` | 加入序＝回合序（linked list 維護）、每 actor 每回合行動點數重設 1000 且全 Self 一輪跑完、玩家 actor 阻塞 submit 後續跑、撞 actor 即攻擊致死發 ActorDied | 新主力檔；引擎核心行為覆蓋良好；缺整合與決定性回放（見 02、03） |
| `test_npc_combat.cpp` | 鄰接攻擊（含實體建立順序回歸案） | 戰鬥路徑回歸；可逐步併入 `apply_action` / `decide_chase` 觀點 |
| `test_serialize.cpp` | Spatial round-trip、parent entity 引用、空 registry、純 Hero tag、檔案 API、FolderSaveStore | **只測了清單內元件的一小部分**（見 §3） |
| `test_phase4.cpp` | MapData resize/旗標/越界、Map+WorldState 序列化、牆阻擋、多實體序列化 | 地圖層 OK |
| `test_ecs.cpp` | EntityManager create/destroy/component/系統執行序 | 基礎 OK |

> 已刪除（重構移除）：`test_turn_scheduler.cpp`、`test_npc_brain.cpp`、`test_zone_effects.cpp`。
> 這三檔對應的能量排程器、施法者 brain、effect/DoT 系統皆已不存在；不需補回，相關覆蓋觀念已合併到 `test_turn_engine.cpp` 與 `apply_action`。

結構性觀察：測試金字塔仍只有兩層——doctest 單元測試與 verify.gd 端到端，
中間缺「core 層整合測試」（完整 ZoneWorld 等價物：map + TurnEngine + apply_action + NPC 跑多回合）。
目前最接近的是 `test_turn_engine` 的「全 Self 一輪跑完」案，值得擴充成一個共用 arena fixture。

## 2. 缺口總表

| # | 缺口 | 優先序 | 工作量 |
|---|---|---|---|
| G1 | 序列化 round-trip 不完整（含新元件 ControllerComponent / ActPointComponent）（§3） | **高** | 中 |
| G2 | 決定性回放不變量（→ 02 檔） | **高** | 中 |
| G3 | `apply_action` 結算分支覆蓋（撞牆/撞 actor/移動拾取/踩樓梯/Wait）（§4） | 高 | 小 |
| G4 | TurnEngine 邊界：空 list、單 actor、行動中 remove_actor（cursor 安全）、清空後 begin_round | 中 | 小 |
| G5 | `decide_chase` 在無目標 / 已相鄰 / 對角 各分支 | 中 | 小 |
| G6 | 多 NPC + 玩家混合的整合場景（追擊至死、同回合互殺、撞擊致死後 list 移除） | 中 | 中 |
| G7 | 邊界：地圖 1×1、actor 無 ActPointComponent、move 方向編碼 0..8 全列舉 | 中 | 小 |
| G8 | ZoneEvent 流的完整性斷言（事件數量/順序，而非「至少一個」） | 低 | 小 |
| G9 | `core/util/`、FOV 系統（fov_system.cpp）無任何測試 | 低 | 中 |

## 3. G1：序列化 round-trip 的真實缺口（優先序：高、工作量：中）

`src/core/serialize/all_components.h` 的 `AllComponents` 清單在重構後已更新：
**加入** `ControllerComponent`、`ActPointComponent`；**移除** `EnergyComponent`、`OngoingActionComponent`、`PlayerControlledComponent`（後三者連同所屬系統已刪除）。

待測的真實缺口（皆為「清單已含、但 round-trip 未斷言」）：
- `ControllerComponent` —— 存讀檔後 `kind`（Player/Self/Faction）與 `faction` 引用是否一致；讀回後 hero 是否仍被引擎判為玩家阻塞（否則 hero 會變 NPC 行為）。
- `ActPointComponent` —— 行動點數 `value` 是否原值還原（雖然每回合首次輪到會重設 1000，但回合中存檔應保留當前值，需明確定義並斷言）。

另兩個不在 registry 的狀態也需檢視：`TurnEngine` 的內部 `order_` linked list 與 `pending_` map。
- `order_`（actor 順序）：存檔後讀回需能重建（目前由 setup 依實體重新 `add_actor`，順序＝實體遍歷序，須確認與存檔前一致）。
- `pending_`（玩家待提交指令）：讀檔後重新等指令即可，可接受不持久化，但要寫成明確斷言/註解。

建議測試（先寫測試讓它紅，再補實作）：
1. 「全元件 round-trip」：建一個掛滿所有清單元件（含 Controller/ActPoint）的 actor，存→讀→逐欄位比對。
2. 「回合中存讀檔」：開一輪、英雄行動到一半（NPC 已動部分）時 save、load、續跑 → actor 順序與行動點語意一致。
3. **守門測試**：以手寫 expected component count + 註解指向 `all_components.h`；新增 component 檔案而未進 `AllComponents` 時測試紅（最簡做法即可，不必上 type_list 反射）。

注意：verify.gd 現有 save/load 案主要比對 turn_count 與 floor——
測不到上述元件級缺口，這正是 04 檔「弱斷言」問題的另一例。

## 4. G3：apply_action 結算分支覆蓋（優先序：高、工作量：小）

`apply_action(reg, map, self, action, events)` 是世界唯一的 Action 落地點，分支明確、純函式、易測：
- Move 撞牆/出界 → 發 `BumpedWall`，座標不變。
- Move 撞 actor → 扣 `CombatStatsComponent.attack` 傷害；致死發 `ActorDied`（**不在此 destroy**，由呼叫端移除），未死發 `BumpedActor`。
- Move 進空格 → 座標改變；踩到 health_potion 自動拾取回血發 `ItemPickedUp`；踩到下樓梯發 `ReachedStairDown`。
- Wait / Idle → 消耗回合、不改世界。

想法：每個分支一個 `SUBCASE`，斷言（a）座標/HP 的精確變化、（b）事件型別與數量，而非「至少一個事件」。`events` 可傳 null，需另有一案驗證 null 路徑不崩。

## 5. G6/G7 速寫（中優先）

- 整合 fixture：`make_arena(w, h, heroes, npcs)` 回傳 registry + TurnEngine + events 包。
  在其上寫：圍殺英雄（已有 verify.gd 版，搬進 doctest）、同回合互殺的 valid 防護、
  撞擊致死後 `remove_actor` 與 cursor 安全（死者正好是當前 cursor 時不應崩）。
- 邊界值用 doctest 的 `SUBCASE` 批量列舉；重點不是行為「正確」而是「定義過且不崩」。
