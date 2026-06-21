# Debug / Developer UI 路線

> 日期：2026-06-21
> 定位：保留早期 debug dump 的效率，同時讓正式 HUD 不被開發資訊淹沒。

> 狀態更新（2026-06-21）：回合系統重構後，trace 管線**仍在**（`get_debug_log` / `set_trace_enabled` / `get_trace_enabled` / `clear_debug_log` / `get_debug_text`）。但 debug 內容已改變：能量/詠唱/狀態效果/排程器 dump **全部移除**，現況 `get_debug_text()` 改為**逐 actor 的 HP / act_point / 位置** dump。本文已據此修正。

---

## 1. 目標

開發者需要快速看：

- trace 最近事件（`get_debug_log()`）。
- 逐 actor 摘要：HP、act_point、座標（現況 `get_debug_text()` 已提供）。
- 回合數、樓層、actor/item 數量（`get_turn_count` / `get_current_floor` / `get_npc_count`）。
- save/load、restart 等操作。

玩家 HUD 不應承擔這些資訊。

## 2. 面板切分（優先序：低～中）

做一個可摺疊 debug drawer：

| Tab | 內容 |
|---|---|
| World | floor、turn count、actor count、item count |
| Actors | 逐 actor：HP、act_point、座標、controller 類型（Player/Self） |
| Trace | 可過濾最近 N 筆 trace / ZoneEvent |
| Save | save/load、has_save_game、last error |

（已移除：Scheduler tab（mode/waiting_actor/ongoing/energy）、Data tab（actions loaded / reload_data）——
排程器與 JSON 技能在重構中已不存在。）

## 3. Trace 過濾

trace 建議附 category：

```text
combat / turn / save / ai / debug
```

短期可在字串前綴 `[combat]`，中期改 structured trace event。
（原 `scheduler` / `data` 分類已隨對應系統移除。）

## 4. Headless 配合

Debug UI 不只給人看，也應暴露可測 API。現況 verify.gd 已可用：

- `get_debug_log()` / `get_trace_enabled()`（trace 開關 round-trip）
- `get_hero_hp()` / `get_turn_count()` / `get_npc_count()`（斷言世界狀態）
- `has_save_game()` / `save_game()` / `load_game()`（存讀檔 round-trip）

未來可補：

- `get_trace_count(category)`
- `get_debug_snapshot()`（結構化逐 actor 狀態）

verify.gd 用這些 API 斷言，不必 parse Label 文字。

## 5. 不做的事

- 不把 debug panel 做成正式遊戲 UI。
- 不讓 debug button 繞過 core invariants；例如 kill actor 也應走測試 helper 或明確 dev-only path。
- 不規劃 scheduler / 能量 / JSON 技能相關 debug 視圖——這些已不存在。
- 不追求第一版漂亮，追求資訊分層與可驗證。
