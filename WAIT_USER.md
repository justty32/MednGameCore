# WAIT_USER — 等待使用者的事

只列需要使用者親自做/驗證才能繼續的 open 項。完成即移除，不留完成清單。

常見類型：

- 實機或 UI 手動驗證
- 外部帳號、權限、下載、授權
- 本機環境變數或工具安裝
- 不能由 agent 代跑的指令
- 高風險操作的確認

## Open

- **新機器首次環境**：需先手動 clone godot-cpp 到 `../../projects/godot-cpp`，並首次執行 `godot-mono --headless --path godot_zone --import`（建立 `.godot/extension_list.cfg`）後 agent 才能建置/驗證。

