# SESSION-LOG — 進度日誌 hub

只放還沒完成的活狀態。完成的不留在這裡；完成後濃縮到對應工作流的 landed/archive、release note、或 git log。

待使用者親自做/驗證的事放 [WAIT_USER.md](WAIT_USER.md)。

建議單一 `session-log.md` 保持短小，只為「下一個 session 接得上」。若超過 50 行，刪舊留新，或按工作流/主題拆檔。

## 最新進度

- **本地 main 有 8 個未 push 的 commit**（2026-07-10 兩個切片：地城可玩化、SRPG 戰棋）。要不要 push 未定。
- **戰棋數值未調過**：單位原型（戰士 18/5、弓手 11/4、斥候 13/3）、`WALL_PCT=12`、競技場 20×14，全在 `src/gbind/tactics_battle_gd.cpp` 開頭，是隨手定的初值。使用者已試玩過，說「跑起來還不錯」，但沒有針對平衡做調整。
- 戰棋後續主題（職業／裝備／地形效果／命中率／反擊／技能／增援／戰鬥中存讀檔）明確不在已落地的切片內，見 [workflows/specs/2026-07-10-srpg-tactics-battle-design.md](workflows/specs/2026-07-10-srpg-tactics-battle-design.md) §9。
- 死檔待清：`data/actions.json`（技能資料驅動系統已移除）與 `tests/CMakeLists.txt` 的 `ZONE_TEST_DATA_DIR` 編譯常數皆無人引用——待清理。

> 註：`TurnEngine` **不是**佔位品。2026-06-21 `e9ed694` 已完成整套重寫，收斂成最傳統回合制是**設計目標**。戰棋的行動條刻意另開 `src/core/tactics/`，不得加回 `TurnEngine`。

## 各工作流 session-log

| 工作流 | session-log | open 摘要 |
|--------|-------------|----------|
| feature-dev | [workflows/feature-dev/session-log.md](workflows/feature-dev/session-log.md) | 無 |
| refactor | [workflows/refactor/session-log.md](workflows/refactor/session-log.md) | 無 |
| investigation | [workflows/investigation/session-log.md](workflows/investigation/session-log.md) | 無 |

## 不屬任何工作流的進度

- 無。
