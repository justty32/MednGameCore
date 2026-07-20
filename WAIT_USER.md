# WAIT_USER — 等待使用者的事

只列需要使用者親自做/驗證才能繼續的 open 項。完成即移除，不留完成清單。

常見類型：

- 實機或 UI 手動驗證
- 外部帳號、權限、下載、授權
- 本機環境變數或工具安裝
- 不能由 agent 代跑的指令
- 高風險操作的確認

## Open

- **新機器首次環境**：需先手動 clone godot-cpp 到 `../../projects/godot-cpp`（相對 repo 根，即 `/c/code/projects/godot-cpp`），並首次執行 `godot-mono --headless --path projects/godot_zone --import`（建立 `.godot/extension_list.cfg`）後 agent 才能建置/驗證。
- **待驗證：projects/ 重構後的 GDExtension + Godot 前端**——本環境只有 g++（已實測 core+tests **55/236 全綠**），沒有 godot-cpp/godot-mono，`zone_gd` 與 Godot 端無法在此建置/驗證。請在本機依序跑：① `cmake -S projects/core -B projects/core/build -DZONE_BUILD_GDEXTENSION=ON` → `cmake --build projects/core/build --target zone_gd -j4`；② 確認 `.so/.dll` 有自動複製到 `projects/godot_zone/bin/`；③ `--import`；④ `verify.gd`／`verify_tactics.gd` → PASSED；⑤ GUI 試玩兩場景。**看點**：core CMake 找 godot-cpp 的相對路徑因搬進 `projects/core/` 而改為 `../../../../projects/godot-cpp`（指向同一 `/c/code/projects/godot-cpp`）；若路徑不對，用 `-DGODOT_CPP_DIR=<絕對路徑>` 覆寫。

