# ADOPTION — 導入既有 repo

不要一開始把整套全打開。按 repo 複雜度逐步導入，讓流程跟著需要長出來。

## 最小導入

適合小 repo、個人專案、剛開始使用 agent 的專案。

放入：

- `AGENTS.md`
- `WORKFLOWS.md`
- `PRINCIPLES.md`
- `workflows/testing.md`
- `workflows/dev-env.md`
- `workflows/` 中各入口檔

說明：minimal 仍會放入 `WORKFLOWS.md` 連到的入口檔，避免產生斷鏈；只是先不放 examples、commands 實作、CI、smoke tests 等外圍資產。

填好：

- 專案一句話
- build/test/lint 指令
- fresh clone 步驟
- 使用者必須親自做的限制

## 標準導入

適合已有多個模組、會跨 session 維護的 repo。

在最小導入外加：

- `README.md`
- `VERSION`
- `CHANGELOG.md`
- `TEMPLATE-MANIFEST.md`
- `UNINSTALL.md`
- `LICENSE`
- `.editorconfig`

填好 CODE_MAP 的第一版，只列會幫助修改前定位的檔案，不寫百科。

## 研究/外部材料導入

只有在專案常需要讀外部 repo、paper、規格、或輸出 patch 時再加：

- `commands/`
- `examples/`
- `scripts/`
- `tests/`
- `.github/`

## 導入順序

1. 先放最小導入。
2. 跑一次真實小任務，觀察缺什麼。
3. 若 agent 常讀錯檔，補 CODE_MAP。
4. 若常跨 session，啟用 SESSION-LOG。
5. 若常等使用者驗證，啟用 WAIT_USER。
6. 若外部材料開始堆積，啟用 analysis/research。

## 不要做

- 不要為了模板完整而建立空資料夾森林。
- 不要把舊文檔一次全遷移；先只連結現役入口。
- 不要把 `AGENTS.md` 寫成手冊。
- 不要把 generated/html 納入日常必同步，除非使用者真的用它。
