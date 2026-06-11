# 前端升級總覽與路線圖

日期：2026-06-11
定位：gamecore-zone Godot 前端（godot_zone/）從「診斷用測試台」演進為「可玩的 roguelike 介面」的整體路線索引。

---

## 1. 現況一句話

`map_view.gd` 是一個**功能完備但全靠文字與整圖重繪的測試台**：
- 渲染 = `ZoneWorld.generate_map_image()` 每次 `world_changed` 整張重畫成 `ImageTexture`（src/gbind/zone_world_gd.cpp `generate_map_image`）。
- UI = `get_debug_text()` 一整坨 dump 塞進單一 `Label`。
- 輸入 = `_unhandled_input` 直接 match `keycode`。
- 推進 = `Timer`（`_tick_sec` 0.2s）驅動 `step_scheduler()`，與渲染解耦（這點是對的，**要保住**）。

核心架構（事件流、計時器推進、阻塞=英雄 idle）已經健康，前端升級是**在這個骨架上換皮**，不是重構骨架。

## 2. 文件索引

| 檔案 | 主題 | 優先序 |
|---|---|---|
| 01-rendering-tilemap-camera-fov.md | 渲染升級：TileMapLayer、actor Sprite、攝影機、FOV | 高 |
| 02-event-animation-pipeline.md | ZoneEvent 動畫化：事件佇列消化模型、補間/閃爍/傷害數字 | 高 |
| 03-hud-structured-ui.md | 結構化 HUD：HP/能量條、效果 icon、施法進度、訊息卷軸 | 高 |
| 04-input-architecture.md | Input Map actions、多角色切換操控 UX | 中 |
| 05-targeting-cursor-mode.md | 目標選擇：游標模式、方向技能、範圍預覽 | 中 |
| 06-scheduler-visualization.md | 排程器三模型的視覺回饋：行動值面板、時間軸 | 中 |
| 07-debug-developer-ui.md | debug/開發者介面演進：可摺疊面板、trace 過濾 | 低～中 |

## 3. 建議實施順序（依依賴關係）

```
階段 0（core 配合，先做）
  └─ ZoneEvent 結構化匯出：get_events() 帶座標/數值的 Array[Dictionary]
     （02 的前提；03、06 也受益）— 見 02 §2

階段 1（純前端，可並行）
  ├─ 01：TileMapLayer + actor Sprite2D 池 + Camera2D 平滑跟隨
  └─ 03：HUD 骨架（HP/能量條、訊息卷軸），先吃現有 getters

階段 2（建立在階段 0+1 上）
  ├─ 02：事件→動畫管線（補間移動、受擊閃爍、傷害數字）
  └─ 06：排程器視覺面板（吃結構化 actor 狀態）

階段 3（玩法擴張時再做）
  ├─ 04：Input Map 化 + 多角色 order 表 UX
  ├─ 05：目標選擇游標（需要 core 支援帶目標的 Action）
  └─ 07：debug 面板進化
```

## 4. 跨文件共通原則

1. **core 計時器推進、渲染獨立**不可破壞：所有動畫都是「事後演繹」，
   不得反向影響 `step_scheduler()` 的結果；動畫最多影響「下一次 tick 何時被消化」
   （見 02 的 gate 模型）。
2. **狀態真相永遠在 C++**（specs/2026-06-03-gamecore-zone-design.md §7 決策定案）。
   前端任何快取（actor sprite 位置、HP 條數值）都是顯示快取，
   每次事件消化完要能與 `ZoneWorld` 查詢結果對帳。
3. **逐步替換，不大爆炸**：每一項都設計成 additive（如同 core 的
   scheduler 路徑不動 `move()`/verify 的做法，session_log.md 已驗證此節奏可行）。
   舊的 `generate_map_image` 路徑保留給 verify.gd / smoke_test.gd。
4. **headless 可驗**：每階段結束 `godot-mono --headless -s res://verify.gd`
   必須仍 PASSED；新增的前端模組盡量提供無頭可檢查的 API（如事件佇列長度）。

## 5. 主要依賴一覽（實際檔案）

- godot_zone/map_view.gd、godot_zone/map_view.tscn — 改造對象
- src/gbind/zone_world_gd.h / .cpp — 需新增的查詢/匯出介面都長在這
- src/core/turn/zone_event.h — 事件 payload 擴充（座標、傷害值、效果種類）
- src/core/turn/turn_scheduler.h — `waiting_actor()`；多角色 UX 的核心訊號
- docs/superpowers/specs/2026-06-03-gamecore-zone-design.md §7 — 推進/渲染解耦鐵律
- docs/superpowers/specs/2026-06-07-action-and-turn-scheduler-design.md — 三排程器語意
