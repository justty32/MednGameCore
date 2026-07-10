# 輸入架構：現況模型與 Input Map 演進

> 日期：2026-06-21
> 定位：描述重構後最簡的輸入模型（`_unhandled_input` → `player_move/player_wait`），並規劃未來整理成 Input Map 與意圖層的擴充路線。

> 狀態更新（2026-06-21）：回合系統重構後，舊的「submit/step 模式、Timer 推進、技能鍵(C/H/M/V)、排程器 TAB 切換、調速 [ ]、`_pending_input`」**全部移除**。輸入現在直接同步推進整輪。本文已改寫為描述新模型；Input Map / 意圖層 / 多角色等仍保留為未來擴充。

---

## 1. 現況模型

`map_view.gd` 的 `_unhandled_input` 直接 match `keycode`，每個移動鍵**同步呼叫一次** `ZoneWorld`：

```text
_unhandled_input(event)
  └─ match keycode:
       移動鍵 → world.player_move(dx, dy)   # 同步：英雄走一步 + 本輪所有 NPC 走一輪
       等待鍵 → world.player_wait()          # 同步：英雄原地等 + 本輪所有 NPC 走一輪
       其餘   → 存讀檔 / trace / 重開等工具鍵
```

關鍵：**按一鍵＝一整輪**。`player_move/player_wait` 內部自動開新一輪（若未在回合中）、
推進到英雄、投遞指令、把整輪 NPC 跑完才返回，並沿途發出 signals。前端不需要也不應該
自己驅動推進——沒有 Timer、沒有 `step`、沒有 `submit` 兩段式。

### 1.1 現況按鍵表（`map_view.gd`）

| 按鍵 | 動作 | 呼叫 |
|---|---|---|
| `W` / `↑` / KP8 | 上移 | `player_move(0, -1)` |
| `S` / `↓` / KP2 | 下移 | `player_move(0, 1)` |
| `D` / `→` / KP6 | 右移 | `player_move(1, 0)` |
| `A` / `←` / KP4 | 左移 | `player_move(-1, 0)` |
| KP7 / KP9 / KP1 / KP3 | 四斜向移動 | `player_move(±1, ±1)` |
| `.` / KP5 | 原地等待 | `player_wait()` |
| `R` | 重開遊戲 | `restart()` |
| `F5` / `F9` | 存檔 / 讀檔 | `save_game` / `load_game` |
| `L` | 開關 trace | `set_trace_enabled(!...)` |
| `K` | 清 trace log | `clear_debug_log()` |

（已移除：技能 C/H/M/V、排程器切換 TAB、調速 `[` `]`、submit/step 相關鍵。）

## 2. Input Map 分層（優先序：中，未來整理）

直接比對 keycode 的好處是快，壞處是無法重綁鍵、且未來加游標/多角色時 match 分支會膨脹。
俟玩法擴張時，建議改用 Godot Input Map action：

```text
move_n / move_ne / move_e / move_se / move_s / move_sw / move_w / move_nw
wait
cancel
confirm
toggle_debug
(未來) skill_1..n / next_actor / prev_actor / defend
```

`map_view.gd` 不直接處理「按鍵」，而是處理「意圖」：

```text
Input action -> FrontendCommand -> ZoneWorld player_move/player_wait / UI state transition
```

現況意圖層只有兩個終點（`player_move`、`player_wait`）加幾個工具操作，過度抽象沒必要；
這層在引入需選目標的動作（05）或多角色控制前可暫緩。

## 3. 模式狀態機（未來擴充）

現況**只有一個模式**：normal（移動 / 等待 / 工具鍵）。不需要狀態機。

若未來在 TurnEngine 之上引入需要「先選方向 / 先選格子」的動作擴充（見 05），
再加入模式：

| 模式 | 意義 |
|---|---|
| `normal` | 移動、等待（現況唯一模式） |
| `direction_targeting` | 方向型動作等待方向（未來） |
| `cursor_targeting` | 指定格動作 / 檢視（未來） |

`cancel` 永遠回上一層或 normal；`confirm` 在游標模式送出動作。

## 4. 多角色控制接線（未來擴充）

現況只有單一玩家英雄（ControllerKind::Player，list 開頭）。
若未來引入「多名可操控角色」擴充，當引擎在某個玩家陣營 actor 暫停等輸入時
（`is_waiting_player()` 為真）：

- 前端選中該 actor，攝影機飛向它（01 §3.3）。
- `next_actor/prev_actor` 只在「多名玩家 actor 都可下令」的模型中啟用。
- UI 顯示目前 active actor。

注意：這是擴充，非現況——現況一輪內只有英雄一個玩家停點。

## 5. 不做的事

- 不把輸入映射塞進 C++ core；core 只收 `player_move/player_wait`（最終轉成 Action）。
- 不在第一版做滑鼠完整操作；鍵盤 roguelike 路徑先穩。
- 不自行驅動回合推進；推進是 `player_move/player_wait` 的同步副作用。
