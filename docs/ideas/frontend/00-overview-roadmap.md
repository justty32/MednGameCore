# 前端升級總覽與路線圖

日期：2026-06-21
定位：gamecore-zone Godot 前端（godot_zone/）從「最簡回合制輸入台」演進為「可玩的 roguelike 介面」的整體路線索引。

> 狀態更新（2026-06-21）：回合系統重構後，前端已大幅簡化。原本的「排程器模式 TAB 切換、Timer 驅動推進、技能(C/H/M/V)、調速([ ])、submit/step 模式」**全部移除**。本文已改寫為符合新 `TurnEngine` + `ZoneWorld` 現實的路線圖；技能/能量/詠唱相關主題重新定位為「建立在最簡 TurnEngine 之上的未來可選擴充」。

---

## 1. 現況一句話

`map_view.gd` 是一個**功能精簡、全靠文字與整圖重繪的可玩台**：
- 渲染 = `ZoneWorld.generate_map_image(cell_px)` 每次 `world_changed` 整張重畫成 `ImageTexture`（src/gbind/zone_world_gd.cpp `generate_map_image`）。
- UI = `get_debug_text()` 一整坨 dump 塞進單一 `Label`。
- 輸入 = `_unhandled_input` 直接 match `keycode`。
- 推進 = **無 Timer**。輸入直接同步呼叫 `player_move(dx,dy)` / `player_wait()`——**按一鍵＝英雄行動一次＋本輪所有 NPC 行動一輪**，函式內部把整輪 pump 完才返回。

回合模型已從「三排程器＋計時器驅動」收斂為**最傳統的回合制**（教科書式 `TurnEngine`：actor linked list，加入序＝回合序）。前端升級是**在這個乾淨地基上換皮**，不是重構骨架。

## 2. 文件索引

| 檔案 | 主題 | 優先序 |
|---|---|---|
| 01-rendering-tilemap-camera-fov.md | 渲染升級：TileMapLayer、actor Sprite、攝影機、FOV | 高 |
| 02-event-animation-pipeline.md | ZoneEvent 動畫化：事件→動畫消化模型、補間/閃爍/傷害數字 | 高 |
| 03-hud-structured-ui.md | 結構化 HUD：HP 條、訊息卷軸、檢視條 | 高 |
| 04-input-architecture.md | Input Map actions、意圖層、未來模式狀態機 | 中 |
| 05-targeting-cursor-mode.md | 目標選擇：游標模式、方向選擇、範圍預覽（未來擴充） | 中 |
| 06-scheduler-visualization.md | 行動順序視覺化（**現況不適用**；俟未來能量/先攻擴充再考慮） | 低 |
| 07-debug-developer-ui.md | debug/開發者介面演進：可摺疊面板、trace 過濾 | 低～中 |

## 3. 建議實施順序（依依賴關係）

```
階段 0（core 配合，先做）
  └─ ZoneEvent 結構化匯出：drain 出帶座標/數值的 Array[Dictionary]
     （02 的前提；03 也受益）— 見 02 §2

階段 1（純前端，可並行）
  ├─ 01：TileMapLayer + actor Sprite2D 池 + Camera2D 平滑跟隨
  └─ 03：HUD 骨架（HP 條、訊息卷軸），先吃現有 getters

階段 2（建立在階段 0+1 上）
  └─ 02：事件→動畫管線（補間移動、受擊閃爍、傷害數字），配合
        world_changed / round_finished 排程

階段 3（玩法擴張時再做）
  ├─ 04：Input Map 化 + 意圖層
  ├─ 05：目標選擇游標（需要 core 支援帶目標的 Action）
  └─ 07：debug 面板進化

未定（俟 core 引入時間模型擴充才有意義）
  └─ 06：行動順序/先攻視覺化（現況 TurnEngine 為固定加入序，無可視化價值）
```

## 4. 跨文件共通原則

1. **推進與渲染解耦**不可破壞：`player_move/player_wait` 同步把整輪結算完並發出
   `world_changed` / `round_finished`；所有動畫都是「事後演繹」，
   不得反向影響 `TurnEngine` 的結算結果（見 02 的動畫消化模型）。
2. **狀態真相永遠在 C++**（specs/2026-06-03-gamecore-zone-design.md §7 決策定案）。
   前端任何快取（actor sprite 位置、HP 條數值）都是顯示快取，
   每次事件消化完要能與 `ZoneWorld` 查詢結果對帳。
3. **逐步替換，不大爆炸**：每一項都設計成 additive。
   舊的 `generate_map_image` 路徑保留給 verify.gd / smoke_test.gd。
4. **headless 可驗**：每階段結束 `godot-mono --headless -s res://verify.gd`
   必須仍 PASSED；新增的前端模組盡量提供無頭可檢查的 API。

## 5. 主要依賴一覽（實際檔案）

- godot_zone/map_view.gd、godot_zone/map_view.tscn — 改造對象
- src/gbind/zone_world_gd.h / .cpp — 需新增的查詢/匯出介面都長在這（ZoneWorld）
- src/core/turn/zone_event.h — 事件 payload 擴充（座標、數值）
- src/core/turn/turn_engine.h — `TurnEngine`：actor 順序、回合推進語意
- src/core/turn/apply_action.h — Action 結算與事件產生點
- docs/superpowers/specs/2026-06-03-gamecore-zone-design.md §7 — 推進/渲染解耦鐵律
