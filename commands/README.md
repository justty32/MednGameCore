# commands — 可選 agent command 模板

這些是可選入口，用於支援 slash command 或 prompt command 的 agent 環境。它們不是核心工作流；沒有 command 系統的 repo 可以忽略。

| Command | 對應工作流 |
|---------|------------|
| `analysis.md` | `workflows/analysis.md` |
| `save.md` | `SESSION-LOG.md` / 各工作流 `session-log.md` |
| `html-guide.md` | `workflows/html-guide/README.md` |
| `patch.md` | `workflows/patch/README.md` |

使用方式：把需要的檔案複製到目標 agent 的 command 目錄，並依該 agent 語法調整 frontmatter。

