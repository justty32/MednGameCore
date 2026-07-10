# SYNC — 模板更新同步策略

當中央 `workflow` 模板更新後，不要無腦覆蓋已導入 repo。目標是保留本地客製化，只同步仍像模板的部分。

## 檔案分類

永遠手動 merge：

- `AGENTS.md`
- `WORKFLOWS.md`
- `PRINCIPLES.md`
- `workflows/common/conventions.md`
- `workflows/common/code-map/CODE_MAP.md`
- `workflows/dev-env.md`
- `workflows/testing.md`

通常可覆蓋，若 repo 沒客製化：

- `DEV-GUIDE.md`
- `ADOPTION.md`
- `MAINTENANCE.md`
- `SYNC.md`
- `INIT-QUESTIONS.md`
- `commands/`
- `examples/`
- `scripts/`

不從模板同步：

- `SESSION-LOG.md`
- `WAIT_USER.md`
- 各工作流 `session-log.md`
- repo 內現役 specs/plans/research/findings

## 同步流程

1. 在目標 repo 先確認 git 狀態乾淨，或至少知道哪些變更是使用者的。
2. 跑健康檢查：`scripts/check-workflow.sh <repo>`。
3. 先更新可覆蓋類文件。
4. 對手動 merge 類文件逐個比較，不覆蓋本地規則。
5. 確認 `WORKFLOWS.md` 連結存在。
6. 確認 `AGENTS.md` 仍然薄。
7. 提交時在 commit message 說明同步來源版本。

## 版本記錄

導入或同步後，可在目標 repo 的 `AGENTS.md` 或 `MAINTENANCE.md` 記一行：

```text
Workflow template version: 0.1.0
```

若 repo 已高度客製化，版本只代表最後一次人工參照，不代表可以自動覆蓋。

