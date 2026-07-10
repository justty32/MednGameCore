# specs — 設計方案入口

一個 idea/roadmap 項認真討論後產出的設計方案：目標、架構、資料流、權衡、取捨。

規劃階梯：

```text
idea → roadmap → spec → plan → feature-dev
```

## 規則

- 開始前寫 `Done when: <設計問題被回答，取捨/不做範圍清楚，可進 plan>`。
- 本夾 `*.md` = 各功能的設計方案。
- 建議命名：`<feature>-design.md`。
- 設計涉及 code 結構時參考 [common/conventions](../common/conventions.md)。
- 落地或被取代後移到 `archive/`。

## 現役設計方案

| 設計方案 | 討論日期 | 對應 idea/roadmap | 狀態 |
|----------|----------|-------------------|------|
| 無 | - | - | - |

## archive

以下皆為 2026-06-21 回合系統重構（三排程器/技能/DoT 整套移除，改為最精簡傳統回合制 `TurnEngine`）**之前**的舊設計，已標記 superseded，僅保留作歷史記錄：

| 檔案 | 說明 |
|------|------|
| [archive/2026-06-03-gamecore-zone-design.md](archive/2026-06-03-gamecore-zone-design.md) | gamecore-zone 設計文件初稿；§7 時間驅動 Actor Poll 構想已被取代 |
| [archive/2026-06-03-turn-loop-sketch.cpp](archive/2026-06-03-turn-loop-sketch.cpp) | 回合迴圈草稿（單 hero + 時間 dt 模型），已被取代 |
| [archive/turn-loop-example.cpp](archive/turn-loop-example.cpp) | 回合迴圈 example（order 表模型，多角色操控版），已被取代 |
| [archive/2026-06-07-action-and-turn-scheduler-design.md](archive/2026-06-07-action-and-turn-scheduler-design.md) | Action 型別與可切換回合排程器（A/B/C 三排程器）設計，整套已被取代 |

## 何時不用

- 小功能已清楚可直接做，走 feature-dev。
- 已決定設計，只缺實作步驟，走 plans。
- 還只是靈感，走 idea 或 roadmap。
