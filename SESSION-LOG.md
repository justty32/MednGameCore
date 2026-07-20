# SESSION-LOG — 進度日誌 hub

只放還沒完成的活狀態。完成的不留在這裡；完成後濃縮到對應工作流的 landed/archive、release note、或 git log。

待使用者親自做/驗證的事放 [WAIT_USER.md](WAIT_USER.md)。

建議單一 `session-log.md` 保持短小，只為「下一個 session 接得上」。若超過 50 行，刪舊留新，或按工作流/主題拆檔。

## 最新進度

- **2026-07-20 資料夾整理（對齊 medps）**：頂層改為 `docs/` + `projects/` + `workflows/` + md kernel。三個專案搬進 `projects/`：`projects/core/`（原 `src/` + 根 `CMakeLists.txt`）、`projects/tests/`（原 `tests/`）、`projects/godot_zone/`（原 `godot_zone/`）；全程 `git mv` 保歷史。CMake 由單一根檔拆成**兩個獨立專案**——`projects/core` 建 `zone_core`(+選用 `zone_gd`)，`projects/tests` 自行 FetchContent + find/link 已建好的 `zone_core`。已用本機 g++ 16.1 兩步建置實測 **55 case / 236 assertion 全綠**。同步更新 `.gitignore`／AGENTS／INDEX／README／docs／testing／dev-env／conventions／CODE_MAP 的路徑與建置指令。`html/` 導覽站（5 檔）路徑與建置指令已一併同步。**GDExtension/Godot 前端未在此環境驗證**（缺 godot-cpp/godot-mono，見 WAIT_USER）。刪除 `PROJECT.md`（內容早已分流到 docs）。未 commit。
- **本地 main 有 10 個未 push 的 commit**（回合系統重構、地城可玩化、SRPG 戰棋三個切片＋文檔對齊）。要不要 push 未定；本次重構亦尚未 commit。
- **戰棋數值未調過**：單位原型（戰士 18/5、弓手 11/4、斥候 13/3）、`WALL_PCT=12`、競技場 20×14，全在 `src/gbind/tactics_battle_gd.cpp` 開頭，是隨手定的初值。使用者已試玩過，說「跑起來還不錯」，但沒有針對平衡做調整。
- 戰棋後續主題（職業／裝備／地形效果／命中率／反擊／技能／增援／戰鬥中存讀檔）明確不在已落地的切片內，見 [workflows/specs/2026-07-10-srpg-tactics-battle-design.md](workflows/specs/2026-07-10-srpg-tactics-battle-design.md) §9。

> 註：`TurnEngine` **不是**佔位品。2026-06-21 `e9ed694` 已完成整套重寫，收斂成最傳統回合制是**設計目標**。戰棋的行動條刻意另開 `src/core/tactics/`，不得加回 `TurnEngine`。

## 各工作流 session-log

| 工作流 | session-log | open 摘要 |
|--------|-------------|----------|
| feature-dev | [workflows/feature-dev/session-log.md](workflows/feature-dev/session-log.md) | 無 |
| refactor | [workflows/refactor/session-log.md](workflows/refactor/session-log.md) | 無 |
| investigation | [workflows/investigation/session-log.md](workflows/investigation/session-log.md) | 無 |

## 不屬任何工作流的進度

- 無。
