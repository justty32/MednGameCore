# 回合事件動畫化：事件佇列消化模型

日期：2026-06-21
定位：讓 ZoneEvent 驅動移動補間、受擊閃爍、傷害數字——在「core 結算、渲染獨立」的鐵律下，設計事件佇列 + 動畫播放器的消化模型。

> 狀態更新（2026-06-21）：回合系統重構後，已無 Timer/排程器。回合模型改為**一鍵一輪**：`player_move()/player_wait()` 同步把英雄＋本輪所有 NPC 結算完才返回，並在過程中發出 `world_changed` / `round_finished`。本文已改寫消化模型以配合這個同步推進模型；事件清單也更新為現存的 ZoneEvent / signals（移除已不存在的能量/詠唱/DoT/nova 事件）。

---

## 1. 核心矛盾與解法

矛盾：一次 `player_move()` 內部會把**整輪**（英雄 + 所有 NPC）瞬間結算完，
過程可能產生多個 actor 的移動、攻擊、死亡事件。若動畫直接掛在 signal callback 上，
畫面會同幀疊爆——所有 NPC 同時瞬移、傷害數字同一幀冒出。

解法：**事件入佇列、動畫慢慢消化**——core 一次把整輪結算完並把事件交給前端；
動畫播放器照自己的節奏消化佇列。兩條時鐘，中間只有一條單向管道。

```
[一鍵輸入] → player_move()/player_wait()
              → TurnEngine 跑完整輪 → 事件全部進佇列（前端）→ round_finished
                                                      ↓
[AnimationDirector] ← 依序/分組取出 → Tween 播放 → 播完取下一批
```

§7 鐵律不破：動畫**永不**影響結算結果。整輪結算在 `player_move()` 返回時就已完成且不可變，
動畫只是把這段「已發生的歷史」重放出來。下一次輸入要等動畫播完（或被跳播）才接受，
避免玩家在動畫中途下一步指令打亂視覺因果。

## 2. 前提：ZoneEvent 結構化匯出（優先序：高，core 配合）

依賴：src/core/turn/zone_event.h、src/core/turn/apply_action.cpp、src/gbind/zone_world_gd.cpp `drain_events`。

現況事件種類（`zone_event.h`，重構後）：

| EventKind | 語意 |
|---|---|
| `BumpedWall` | 撞牆/出界（未移動） |
| `BumpedActor` | 攻擊命中但未殺死（a 攻擊 b，amount = 傷害） |
| `ActorDied` | b 被 a 殺死（**b 已 destroy**） |
| `ItemPickedUp` | a 拾取道具 b（amount = 實際治療量） |
| `ReachedStairDown` | a 走到下樓梯 |

對應現有 signals（`zone_world_gd.cpp` `drain_events`）：
`world_changed`、`round_finished`、`floor_changed(floor_num)`、`hero_bumped_wall`、
`hero_bumped_npc(npc_id)`、`npc_died(npc_id)`、`item_picked_up(item_name, heal_amount)`、`game_over`。

現況兩個動畫化缺陷：
1. `ZoneEvent` 只有 `entt::entity a/b + amount`，**沒有座標**；
   `ActorDied` 時 b 已 destroy，drain 時查不到死在哪格——動畫無從播起。
2. `drain_events` 把事件壓成幾個粗粒度 signal（`hero_bumped_npc` 連是哪隻 NPC 都只回傳字串 `"npc"`，丟了 eid 與座標）。

改法：
- `ZoneEvent` 在**發生當下**就把座標/數值塞滿：加 `int ax, ay, bx, by;`
  （`apply_action.cpp` 結算 Move/撞擊時本來就在摸 SpatialComponent 座標）。
  順手補一個目前缺的事件：`Moved`（**移動成功現在根本沒有事件**，動畫補間要靠它）。
- ZoneWorld 新增 `Array drain_events_v2()`：回傳
  `[{kind:"moved", eid:5, from:Vector2i(3,4), to:Vector2i(4,4)}, {kind:"damaged", eid:7, at:Vector2i(...), amount:3}, ...]`，
  GDScript 直接吃。既有 signals 保留（音效 callback 還在用），additive。

## 3. AnimationDirector（前端新模組，優先序：高）

依賴：§2 的 drain_events_v2；01 的 actor Sprite2D 池（補間要有東西可補）。

map_view.gd 旁新增 `animation_director.gd`（RefCounted 或 Node）。
與新回合模型接線：每次 `player_move/player_wait` 後（或在 `round_finished` callback 裡）
把該輪 drain 出的事件交給 director：

```gdscript
var _queue: Array = []          # 事件批次的佇列
var _playing := false

func enqueue_batch(events: Array) -> void:
    _queue.push_back(events); _try_play_next()

func _try_play_next() -> void:
    if _playing or _queue.is_empty(): return
    _playing = true
    var tw := get_tree().create_tween()
    _build_batch_tween(tw, _queue.pop_front())
    tw.finished.connect(func(): _playing = false; _try_play_next())
```

關鍵設計：**批的切分對應因果**。一輪內可細分為「英雄這一步」與「各 NPC 這一步」，
也可整輪當一批。建議切法：
- 英雄行動的事件為一批先播（玩家剛按的鍵，要立即回饋）。
- 接著各 NPC 行動依 actor 順序分批播，批間串行，因果順序可讀。
- 同一 actor 同一步的多個事件（如移動 + 拾取）可在批內平行。

> 設計取捨：TurnEngine 是順序結算（list 順序），沒有「同 tick 多 actor 同時就緒」的並行語意。
> 因此預設「批間串行」即可正確表達回合制的逐一行動感；若覺得 NPC 一隻一隻動太慢，
> 可把整輪 NPC 設為同一平行批（`tw.set_parallel(true)`）一次滑動。

### 3.1 各事件的動畫配方（全用 Tween，不用 AnimationPlayer）

| 事件 | 配方 | 時長基準 |
|---|---|---|
| Moved | `tween_property(sprite, "position", to*CELL_PX, D)`，`TRANS_QUAD/EASE_OUT` | D=0.12s |
| BumpedWall | 朝牆半格往返：`position` 去 0.05s 回 0.05s（`tween_property` 兩段 chain） | 0.1s |
| BumpedActor（Damaged） | 受擊閃爍：`sprite.modulate` → 白(2,2,2) 0.05s → 原色 0.1s；+ 傷害數字（§3.2） | 0.15s |
| ActorDied | `modulate:a` → 0 + `scale` → 0.5，0.25s，**播完才把 sprite 回收進池** | 0.25s |
| ItemPickedUp | 綠色 `+N` 數字上飄 + sprite 短暫泛綠 | 0.15s |
| ReachedStairDown | 樓梯格高亮一閃，接 `floor_changed` 後重鋪地圖 | 0.2s |

Tween 而非 AnimationPlayer 的理由：事件是程序性、參數化的（座標每次不同），
`create_tween()` 即拋即用，無需預製動畫資源。

### 3.2 傷害數字（優先序：高，視覺回報率最高）

- FxLayer 下生成 `Label`（預載 LabelSettings：描邊、粗體），文字 `-3`/`+5`，
  `tween_property("position:y", -16, 0.4)` + `tween_property("modulate:a", 0, 0.4).set_delay(0.2)`，
  `finished` 後 `queue_free()`。量小不必池化，之後再說。
- 顏色約定：物理傷害白、治療亮綠——與 03 訊息卷軸、07 trace 著色共用一張色票表。
  （火/毒等元素色俟未來引入元素傷害擴充再加。）

## 4. 佇列背壓：動畫趕不上輸入怎麼辦（優先序：高，模型核心）

新模型下沒有自動 Timer 連發，但玩家可能**快速連按**移動鍵，動畫尚未播完下一輪事件又進來。
策略：

1. **輸入閘**（預設、最簡）：`director.is_playing()` 為真時，`_unhandled_input` 暫存或忽略新移動鍵，
   待動畫播完再接受。語意上等同「動畫期間鎖輸入」，最不易出錯。
2. **加速消化**（連按手感更好）：佇列長度 > 2 批時，本批 tween `tw.set_speed_scale(queue_len)`，
   越積越快，自然追平；佇列 > 8 批直接**跳播**（瞬間套用終態、只播最後一批）。
   這允許玩家壓著方向鍵時畫面快速追上，而 core 真相早已結算完。
3. **零動畫模式**：`director.instant = true` 全部瞬間套用——保住高速測試體驗，
   也是 headless verify 的預設（verify.gd 根本不建立 director）。

終態對帳：無論哪一檔，每批播完後以 `get_actors()`（01 §4）校正 sprite 位置，
飄移就 snap——顯示快取永遠向 C++ 真相收斂。

## 5. 未來：帶選目標動作的事件（優先序：低）

現況動作只有 Idle/Move/Wait，無詠唱、無多回合動作，因此**沒有「打斷」這類時序問題**。
若未來在 TurnEngine 之上引入需選目標或跨回合的動作擴充（見 05、06），
再為其新增對應事件種類與動畫配方即可。事件佇列模型的優點在此依然成立：
**過去不可變，動畫只是重放歷史**——新動作只是多一種要重放的歷史。
