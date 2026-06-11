# 回合事件動畫化：事件佇列消化模型

日期：2026-06-11
定位：讓 ZoneEvent 驅動移動補間、受擊閃爍、傷害數字——在「core 計時器推進、渲染獨立」的鐵律下，設計事件佇列 + 動畫播放器的消化模型。

---

## 1. 核心矛盾與解法

矛盾：core 一個 `step_scheduler()` 可能瞬間結算多個 actor 的行動
（src/core/turn/*_scheduler.cpp 的 advance 一次推到下一批就緒），
但動畫需要**時間**播。若動畫直接掛在 signal callback 上，畫面會同幀疊爆。

解法：**雙佇列解耦**——core 照自己的計時器跑，事件進佇列；
動畫播放器照自己的節奏消化佇列。兩條時鐘，中間只有一條單向管道。

```
[core Timer 0.2s] → step_scheduler() → 事件批次 → EventQueue（前端）
                                                      ↓
[AnimationDirector] ← 依序/分組取出 → Tween 播放 → 播完取下一批
```

§7 鐵律不破：動畫**永不**影響結算結果；唯一允許的反向影響是
「佇列太長時 gate 計時器」（§4），那是節流，不是邏輯。

## 2. 前提：ZoneEvent 結構化匯出（優先序：高，core 配合）

依賴：src/core/turn/zone_event.h、src/gbind/zone_world_gd.cpp `drain_events`。

現況兩個致命缺陷：
1. `ZoneEvent` 只有 `entt::entity a/b + amount`，**沒有座標**；
   `ActorDied` 時 b 已 destroy，drain 時查不到死在哪格——動畫無從播起。
2. `drain_events` 把事件壓成幾個粗粒度 signal（`hero_bumped_npc` 連是哪隻 NPC 都丟了）。

改法：
- `ZoneEvent` 在**發生當下**就把座標/數值塞滿：加 `int ax, ay, bx, by;`
  （效果結算點都拿得到 SpatialComponent，zone_effects.cpp 的 resolve_move/resolve_nova
  本來就在摸座標）。順手補事件種類：`Moved`（現在移動成功根本沒有事件！）、
  `Damaged`（取代 BumpedActor 只報 hero 的限制）、`EffectApplied`、`HealApplied`、
  `CastProgress`。
- ZoneWorld 新增 `Array drain_events_v2()`：回傳
  `[{kind:"moved", eid:5, from:Vector2i(3,4), to:Vector2i(4,4)}, {kind:"damaged", eid:7, at:Vector2i(...), amount:3}, ...]`，
  GDScript 直接吃。既有 signals 保留（音效 callback 還在用），additive。

## 3. AnimationDirector（前端新模組，優先序：高）

依賴：§2 的 drain_events_v2；01 的 actor Sprite2D 池（補間要有東西可補）。

map_view.gd 旁新增 `animation_director.gd`（RefCounted 或 Node）：

```gdscript
var _queue: Array = []          # 事件批次的佇列（每 step 一批）
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

關鍵設計：**同一批（同一次 step）內的事件平行播，批與批之間串行**。
- 同批平行：`tw.set_parallel(true)`——一次 step 裡 3 隻 NPC 各走一步，
  視覺上同時滑動，正確表達「這些事同時發生」（能量排程器同 tick 就緒的語意）。
- 批間串行：保持因果順序可讀。

### 3.1 各事件的動畫配方（全用 Tween，不用 AnimationPlayer）

| 事件 | 配方 | 時長基準 |
|---|---|---|
| Moved | `tween_property(sprite, "position", to*CELL_PX, D)`，`TRANS_QUAD/EASE_OUT` | D=0.12s |
| BumpedWall | 朝牆半格往返：`position` 去 0.05s 回 0.05s（`tween_property` 兩段 chain） | 0.1s |
| Damaged | 受擊閃爍：`sprite.modulate` → 白(2,2,2) 0.05s → 原色 0.1s；+ 傷害數字（§3.2） | 0.15s |
| ActorDied | `modulate:a` → 0 + `scale` → 0.5，0.25s，**播完才把 sprite 回收進池** | 0.25s |
| HealApplied | 綠色數字上飄 + sprite 短暫泛綠 | 0.15s |
| nova/技能 | FxLayer 生成範圍格高亮（半透明色塊閃一下），radius 直接畫 chebyshev 方框 | 0.2s |

Tween 而非 AnimationPlayer 的理由：事件是程序性、參數化的（座標每次不同），
`create_tween()` 即拋即用，無需預製動畫資源。

### 3.2 傷害數字（優先序：高，視覺回報率最高）

- FxLayer 下生成 `Label`（預載 LabelSettings：描邊、粗體），文字 `-3`/`+5`，
  `tween_property("position:y", -16, 0.4)` + `tween_property("modulate:a", 0, 0.4).set_delay(0.2)`，
  `finished` 後 `queue_free()`。量小不必池化，之後再說。
- 顏色約定：物傷白、火橘、毒綠、治療亮綠——與 03 效果 icon、07 trace 著色共用一張色票表。

## 4. 佇列背壓：動畫趕不上 core 怎麼辦（優先序：高，模型核心）

`_tick_sec` 可調到 0.03s（map_view.gd `_set_tick` clamp 下限），動畫必然堆積。三檔策略：

1. **加速消化**（預設）：佇列長度 > 2 批時，本批 tween `tw.set_speed_scale(queue_len)`，
   越積越快，自然追平。佇列 > 8 批直接**跳播**（瞬間套用終態、只播最後一批）。
2. **gate 模式**（可選旋鈕）：`_on_advance_tick` 加一條
   `if director.is_playing() and gate_enabled: return`——核心照舊由計時器驅動，
   只是這個 tick 跳過不消化。語意上等同玩家把 `_tick_sec` 調慢，**不違反解耦**
   （計時器仍是唯一推進源，動畫只是讓某些 tick 變 no-op）。
   適合「觀賞模式」；測試排程器時關掉。
3. **零動畫模式**：`director.instant = true` 全部瞬間套用——保住現在這種
   高速測試體驗，也是 headless verify 的預設。

終態對帳：無論哪一檔，每批播完後以 `get_actors()`（01 §4）校正 sprite 位置，
飄移就 snap——顯示快取永遠向 C++ 真相收斂。

## 5. 與打斷的互動（優先序：中）

詠唱中按移動=即時打斷（map_view.gd 現有行為，core 端覆寫 pending）。
動畫側：打斷發生時佇列裡可能還有舊詠唱的 `CastProgress` 批——不用特殊處理，
照播即可（那是「已發生的過去」）；新增 `CastInterrupted` 事件配紅色 "打斷!" 飄字，
玩家就能讀懂時序。這正是事件佇列模型的優點：**過去不可變，動畫只是重放歷史**。
