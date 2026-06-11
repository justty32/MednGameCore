# 測試覆蓋盤點與缺口

> 日期：2026-06-11
> 定位：現有 ctest 49 cases / 168 assertions 覆蓋什麼、缺什麼——以可立即動工的測試案清單呈現。

---

## 1. 現有覆蓋盤點（依檔案）

| 檔案 | 覆蓋 | 評估 |
|---|---|---|
| `test_turn_scheduler.cpp` | §8 五情境：速度差(A/B)、channel 完成(B/C)、打斷(B/C)、玩家阻塞(A/B/C)、多角色阻塞(A/B/C) | 行為面良好；缺等價性與邊界（見 02） |
| `test_zone_effects.cpp` | Move/撞牆/攻擊擊殺/拾取、Cast nova+打斷、DoT 三案、玩家不滅、JSON 載入/後備、LibraryEffects fireball/heal、radius/dot_kind、trace 有輸出 | 主力檔；單一 effect 粒度完整 |
| `test_npc_brain.cpp` | decide_chase 方向、相鄰撞擊、caster 放 npc_flame、遠距仍移動 | 純函式測試，乾淨 |
| `test_npc_combat.cpp` | 舊 NpcAiSystem 鄰接攻擊（含實體建立順序回歸案） | 屬舊系統路徑，保留作回歸 |
| `test_serialize.cpp` | Spatial round-trip、parent entity 引用、空 registry、純 Hero tag、檔案 API、FolderSaveStore | **只測了清單內元件的一小部分**（見 §3） |
| `test_phase4.cpp` | MapData resize/旗標/越界、Map+WorldState 序列化、牆阻擋、多實體序列化 | 地圖層 OK |
| `test_ecs.cpp` | EntityManager create/destroy/component/系統執行序 | 基礎 OK |

結構性觀察：測試金字塔只有兩層——doctest 單元測試與 verify.gd 端到端，
中間缺「core 層整合測試」（完整 ZoneWorld 等價物：map+scheduler+effects+NPC 跑多回合）。
目前最接近的是 test_zone_effects 的 DoT-via-scheduler 案，值得擴充成一個共用 fixture。

## 2. 缺口總表

| # | 缺口 | 優先序 | 工作量 |
|---|---|---|---|
| G1 | 序列化元件清單不完整（§3） | **高** | 中 |
| G2 | 排程器等價性不變量（→ 02 檔） | **高** | 中 |
| G3 | `Action.def` 索引跨存檔/跨 JSON 改版的穩定性（§4） | 高 | 小 |
| G4 | scheduler safety guard 靜默返回無測試（§5） | 中 | 小 |
| G5 | TurnConfig 非預設值（energy_to_act≠1000 等）下的行為 | 中 | 小 |
| G6 | 多 NPC + 玩家混合的整合場景（追擊至死、nova 連殺、同回合互殺） | 中 | 中 |
| G7 | 邊界：speed_mod=0 / 負數、weight=0、地圖 1×1、actor 無 Energy 元件 | 中 | 小 |
| G8 | ZoneEvent 流的完整性斷言（事件數量/順序，而非「至少一個」） | 低 | 小 |
| G9 | `core/util/`、FOV 系統（fov_system.cpp）無任何測試 | 低 | 中 |

## 3. G1：序列化 round-trip 的真實缺口（優先序：高、工作量：中）

`src/core/serialize/all_components.h` 的 `AllComponents` 清單目前為：
Actor、Spatial、MapData、NpcAi、Health、Item、CombatStats、WorldState、Hero。

**缺**（皆為排程器時代新增）：
- `EnergyComponent` —— 存檔再讀，所有 actor 能量歸零，行動順序重排
- `OngoingActionComponent` —— **channel 詠唱到一半存檔，讀回後詠唱消失**
- `TimedEffectsComponent` —— 燃燒/中毒/回復 buff 存檔即消失
- `PlayerControlledComponent` —— 讀回後玩家阻塞判定失效（hero 變 NPC 行為）

另外兩個不在 registry 的狀態也會遺失：scheduler 內部 `pending_` map、
`TurnWorld.clock` / `ZoneWorld.world_clock_`。前者可接受（讀檔後重新等指令），
後者應併入 WorldStateComponent。

建議測試（先寫測試讓它紅，再補清單）：
1. 「全元件 round-trip」：建一個掛滿所有元件的 actor，存→讀→逐欄位比對。
2. 「channel 中存讀檔」：submit cast3、advance 兩步、save、load、繼續 advance
   → 詠唱應在原進度上完成（或明確定義為取消，但要寫成斷言）。
3. 「DoT 中存讀檔」：燃燒 3 回合過 1 回合存檔，讀回後應再燒 2 回合。
4. **守門測試**：用 static_assert 或編譯期 type_list 長度比對「元件目錄」，
   新增 component 檔案而未進 `AllComponents` 時測試失敗（最簡做法：一個
   手寫 expected count + 註解指向 all_components.h，加元件忘記更新就紅）。

注意：verify.gd 現有 save/load 案只比對 turn_count 與 floor 兩個整數，
完全測不到上述缺口——這正是 04 檔「弱斷言」問題的另一例。

## 4. G3：Action.def 索引穩定性（優先序：高、工作量：小）

`Action.def` 是 `ActionLibrary` 的**整數索引**。若未來 OngoingActionComponent
進入存檔（§3），而 actions.json 在版本間調整順序，舊存檔的 def 索引會指錯技能。

想法：
- 短期：測試鎖住現有 JSON 順序（fireball=0…），改順序就紅，迫使有意識決策。
- 長期：存檔層以 name 字串保存、載入時 `find()` 重解析；或 ActionDef 加穩定 id 欄位。

## 5. G4：safety guard 的可觀測性（優先序：中、工作量：小）

`energy_channel_scheduler.cpp` 的 `advance()` 有 `guard < threshold * 64` 護欄，
耗盡時**靜默** `waiting_ = null` 返回——若哪天能量增量被設成 0（TurnConfig 誤設、
speed_mod 全 0），世界會「看似正常推進但什麼都沒發生」，正是 verify.gd 假通過的
core 層翻版。三個排程器都應檢查同類路徑。

想法：
- guard 耗盡時若有 `w.trace` 吐一行 `"scheduler stalled"`；或在 TurnWorld 加
  `stall_count` 統計欄位。
- 測試：`speed_mod=0` 的單 NPC 世界 advance 一次，斷言 stall 被回報而非無事返回。

## 6. G6/G7 速寫（中優先）

- 整合 fixture：`make_arena(w, h, heroes, melee_npcs, caster_npcs, mode)` 回傳
  registry+scheduler+events 包，三模式參數化。在其上寫：圍殺英雄（已有 verify.gd
  版，搬進 ctest）、meteor r2 多殺、venom 疊加、互殺同回合的 valid 防護。
- 邊界值用 doctest 的 `SUBCASE` 批量列舉；重點不是行為「正確」而是「定義過且不崩」。
