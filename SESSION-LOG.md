# SESSION-LOG — 進度日誌 hub

只放還沒完成的活狀態。完成的不留在這裡；完成後濃縮到對應工作流的 landed/archive、release note、或 git log。

待使用者親自做/驗證的事放 [WAIT_USER.md](WAIT_USER.md)。

建議單一 `session-log.md` 保持短小，只為「下一個 session 接得上」。若超過 50 行，刪舊留新，或按工作流/主題拆檔。

## 最新進度

- 回合系統：現行 `TurnEngine`（`src/core/turn/`）是佔位性質的最簡實作，使用者計劃整套重寫。
- NPC AI 雙軌並存：`src/core/systems/npc_ai_system.{h,cpp}`（警覺/失憶/徘徊，較完整）與 `apply_action.cpp` 內 `decide_chase` 平行存在，主迴圈目前只走 `decide_chase`——待決定整併或移除。
- 死檔待清：`data/actions.json`（技能資料驅動系統已移除）與 `tests/CMakeLists.txt` 的 `ZONE_TEST_DATA_DIR` 編譯常數皆無人引用——待清理。

## 各工作流 session-log

| 工作流 | session-log | open 摘要 |
|--------|-------------|----------|
| feature-dev | [workflows/feature-dev/session-log.md](workflows/feature-dev/session-log.md) | 無 |
| refactor | [workflows/refactor/session-log.md](workflows/refactor/session-log.md) | 回合系統整套重寫（待啟動） |
| investigation | [workflows/investigation/session-log.md](workflows/investigation/session-log.md) | NPC AI 雙軌整併決策 |

## 不屬任何工作流的進度

- 無。
