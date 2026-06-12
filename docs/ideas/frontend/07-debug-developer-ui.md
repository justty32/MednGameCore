# Debug / Developer UI 路線

> 日期：2026-06-11
> 定位：保留早期 debug dump 的效率，同時讓正式 HUD 不被開發資訊淹沒。

---

## 1. 目標

開發者需要快速看：

- trace 最近事件。
- scheduler 狀態。
- entity/component 摘要。
- RNG seed、floor、clock。
- save/load、reload_data、切 scheduler 等操作。

玩家 HUD 不應承擔這些資訊。

## 2. 面板切分（優先序：低～中）

做一個可摺疊 debug drawer：

| Tab | 內容 |
|---|---|
| World | floor、clock、seed、actor count、item count |
| Scheduler | mode、waiting_actor、ongoing actions、energy |
| Trace | 可過濾最近 N 筆 trace / ZoneEvent |
| Data | actions loaded、load warnings、reload_data button |
| Save | save/load slot、last error |

## 3. Trace 過濾

trace 建議附 category：

```text
combat / scheduler / data / save / ai / debug
```

短期可在字串前綴 `[combat]`，中期改 structured trace event。

## 4. Headless 配合

Debug UI 不只給人看，也應暴露可測 API：

- `get_trace_count(category)`
- `get_last_error()`
- `get_loaded_action_count()`
- `get_debug_snapshot()`

verify.gd 可用這些 API 斷言，不必 parse Label 文字。

## 5. 不做的事

- 不把 debug panel 做成正式遊戲 UI。
- 不讓 debug button 繞過 core invariants；例如 kill actor 也應走測試 helper 或明確 dev-only path。
- 不追求第一版漂亮，追求資訊分層與可驗證。
