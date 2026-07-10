# WAIT_USER — 等待使用者的事

只列需要使用者親自做/驗證才能繼續的 open 項。完成即移除，不留完成清單。

常見類型：

- 實機或 UI 手動驗證
- 外部帳號、權限、下載、授權
- 本機環境變數或工具安裝
- 不能由 agent 代跑的指令
- 高風險操作的確認

## Open

- **Godot GUI 實機驗證**：`godot-mono --path godot_zone` 開視窗實玩只能使用者親自做；agent 只能跑 headless `verify.gd`（→ VERIFY PASSED）。這次「可玩化」slice 換了整套渲染與 HUD，headless 驗證測不到畫面觀感，需要使用者實際開視窗確認：
  - 圖塊渲染正確：地形/牆/樓梯/英雄/NPC/道具用 Kenney 1-Bit Pack 圖塊畫出來（`godot_zone/dungeon_view.gd`），霧中戰爭（探索過但目前看不見）有正確調暗，沒有破圖/缺圖/圖塊對錯格。
  - HUD／訊息列讀起來順：`Stats`／`HPBar`／`MessageLog`（右下訊息列）顯示是否清楚，死亡與勝利 overlay 文字/排版是否正常。
  - `Tab` 鍵能開關除錯面板（`DebugPanel`，按鍵已從舊版的 `L` 改為 `Tab`）。
  - 圖塊來源：`godot_zone/assets/tilesheet.png` 取自 `~/Downloads/1bitpack_kenney_1.2.zip`（Kenney 1-Bit Pack，CC0），授權見 `godot_zone/assets/LICENSE-kenney-1bit.txt`。
- **新機器首次環境**：需先手動 clone godot-cpp 到 `../../projects/godot-cpp`，並首次執行 `godot-mono --headless --path godot_zone --import`（建立 `.godot/extension_list.cfg`）後 agent 才能建置/驗證。

