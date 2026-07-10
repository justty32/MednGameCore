# tooling — 外部工具入口

記錄外部工具、env var、資料依賴、二進位檔、CLI 用法。

開始前寫 `Done when: <工具用途、安裝/設定/驗證方式、限制已記錄>`。

## 內容

| 檔案 | 內容 |
|------|------|
| `binaries.md` | 外部 binary / 安裝位置 / 版本，按需建立 |
| `env-vars.md` | 環境變數，按需建立 |
| `data-assets.md` | 外部資料/大型資產，按需建立 |

## 規則

- 工具版本、來源 URL、安裝路徑要可驗證。
- agent 無法安裝或需要帳號/授權時，記到 [../../WAIT_USER.md](../../WAIT_USER.md)。
- 不把大型二進位或私有資料誤 commit。

## 何時不用

- 工具只是某 feature 的內部實作細節，記在 feature docs/CODE_MAP 即可。
- 是開發環境矩陣或 fresh clone 步驟，走 dev-env。
