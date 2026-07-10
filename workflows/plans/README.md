# plans — 實作規劃入口

真的要動工前的詳細實作規劃：精確到檔案、步驟、測試、驗證。

規劃階梯：

```text
idea → roadmap → spec → plan → feature-dev
```

## 規則

- 開始前寫 `Done when: <每個 task、檔案、測試與驗證都足以直接動工>`。
- 本夾 `*.md` = 各功能的逐步實作計畫。
- 建議命名：`<feature>.md`。
- 對應設計方案：`specs/<feature>-design.md`。
- 計畫要切成 bite-sized task，每步都有驗證。
- 落地或被取代後移到 `archive/`。

## 現役計畫

| 計畫 | 出計畫日期 | 對應 spec | 狀態 |
|------|------------|-----------|------|
| 無 | - | - | - |

## archive

以下皆為 2026-06-21 回合系統重構（三排程器/技能/DoT 整套移除，改為最精簡傳統回合制 `TurnEngine`）**之前**的舊計畫，已標記 superseded，僅保留作歷史記錄：

| 檔案 | 說明 |
|------|------|
| [archive/2026-06-03-gamecore-zone-refactor.md](archive/2026-06-03-gamecore-zone-refactor.md) | gamecore-zone 重構實作計劃；Task 1–8/10–13 主幹已落實，但回合制細節部分已被取代 |
| [archive/2026-06-07-action-and-turn-scheduler.md](archive/2026-06-07-action-and-turn-scheduler.md) | Action 與可切換回合排程器實作計畫（A/B/C 三排程器），整套已被取代並刪除 |

## 何時不用

- 小改動已能直接安全實作，走 feature-dev。
- 設計還沒定，走 specs。
- 只是排優先順序，走 roadmap。
