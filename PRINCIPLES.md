# PRINCIPLES — 輕量使用原則

這套工作流的目的，是讓重要上下文可延續，不是讓每件小事都進流程。

## 核心原則

- 小改直接做；只有跨 session、大型設計、外部研究、重構、交接風險高時才啟用完整工作流。
- `AGENTS.md` 保持薄，只放 always-on 鐵律、路由入口、本地特殊規則。
- 每個工作單位一開始都要有 `Done when:`，避免工作無限膨脹。
- `session-log.md` 只服務接手，不做歷史；歷史交給 git log、archive、landed 或 release notes。
- CODE_MAP 只服務修改前導航，不寫成百科。
- analysis/research 產物要有出口：`discard`、`roadmap`、`spec`、`create`、`patch`。
- patch 工作流只在跨 repo、無 git、交付給冷啟動 agent、或不能直接改原 repo 時使用。
- HTML 導覽層是衍生呈現，不是真相層；除非明確需要，不把它放進日常同步要求。

## Truth Layers

專案可以改自己的優先序，但必須明確。預設優先序：

```text
code/tests > schema/examples/fixtures > CODE_MAP > docs > generated/html
```

規則：

- 上層與下層衝突時，以上層為準並修正下層。
- generated/html 永遠不是唯一真相。
- research/analysis 的原始來源與摘要衝突時，以原始來源為準。
- CODE_MAP 是導航，不是規格；行為以 code/tests 為準。

## Done When

每個非微小工作都先寫一句完成定義：

```text
Done when: <可觀察、可驗證的完成條件>
```

好的完成定義：

- 有可驗證結果，例如測試、文件、索引、可執行 demo、使用者驗收。
- 說清楚哪些事不包含在本次工作。
- 能讓下一個 agent 判斷「該停了」。

## 跳流程規則

可以跳過完整 workflow 的情況：

- 單行或小範圍修正，風險低且不跨 session。
- 純查詢或一次性回答，不會留下 durable 知識。
- 使用者明確要求快速處理，不需要文檔化。
- 既有工作流會增加同步成本，而不會降低風險。

跳流程不代表跳過基本工程規矩：仍要讀必要上下文、避免破壞使用者改動、能測就測。

## 多 Agent 並行

只有在工作能切成互斥範圍時才並行。

並行前：

- 每個 agent 有明確 `Done when:`。
- 每個 agent 分配互斥檔案、領域或資料集。
- 共享狀態寫入 `SESSION-LOG.md` 或工作流 `session-log.md`，不要只留在聊天裡。

整合時：

- 由一個整合者合併。
- 先讀每個 agent 的產物與 open 狀態。
- 遇到同檔衝突，手動理解後合併，不盲目覆蓋。
- 整合後跑對應測試，更新 CODE_MAP/文檔。

不適合並行：

- 多人會改同一小檔。
- 設計尚未定，子任務邊界不清。
- 測試/驗證只能在單一環境序列執行。
