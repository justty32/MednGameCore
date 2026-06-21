# 決定性回放測試（原「排程器等價性」）

> 日期：2026-06-21
> 定位：以「同 seed 同指令序列 → 同世界 hash」作為單一 `TurnEngine` 的決定性承諾，並可被測試。
>
> **狀態更新（2026-06-21）**：本檔原為「三種 scheduler（A 能量瞬發 / B 能量＋詠唱 / C 純 tick）的跨模式等價性差分測試」。
> 回合系統重構**移除了全部三個排程器**，現在只剩一個 `TurnEngine`——
> **「A/B/C 同 seed 同狀態 hash」這個比較對象的前提整個消失，該測試策略已不適用。**
> 但其中真正有價值的方法論——**世界摘要（digest）+ 固定種子 + 決定性比對**——對單一引擎一樣成立，
> 改寫為「決定性回放測試」保留。

---

## 1. 問題（重定位後）

過去的問題是「某機制可能只在 A 正確、在 B/C 悄悄偏掉」。
現在只有一個引擎，新問題是：**`TurnEngine` 是否真的決定性？**

只要 `(seed, 指令序列)` 相同，跑兩次（或不同建置、不同時間）應得到**完全相同的世界**。
這是長跑模擬（03）、指令重播與差分比對（08）的共同地基；引擎決定性一旦破裂，
上層所有「可重現」承諾都失效。

引擎本身已具備有利條件：`TurnEngine` 不持有 registry（由 `EngineCtx` 注入），
actor 順序＝加入序的 linked list，沒有 wall-clock 依賴；唯一隨機性來源在
NPC 決策與 mapgen，須走可注入的 RNG（見 08）。

## 2. 第一組回放不變量（優先序：高）

固定 `seed` 與固定指令序列，跑兩次：

```text
run(seed, commands) == run(seed, commands)   # 逐次重現
```

可比較（兩次結果應完全一致）：

- 所有 actor 的最終位置。
- HP。
- alive / dead 集合。
- item pickup 結果與當前樓層。
- `ZoneEvent` 型別序列（含順序）。

因為只有單一引擎、無模式差異，這裡可以要求**事件序列完全同序**，
不需像舊版那樣容忍 scheduler-specific 差異。

## 3. 回放 fixture

建立 helper：

```cpp
run_scenario(seed, commands) -> WorldDigest
```

- `commands`：一串 `Action`（或玩家指令 + NPC 由 `decide_chase` 決定）。
- `WorldDigest` 不含 entity id 細節，改用 actor label / position / components 摘要，
  避免實體建立順序造成假差異（與 08 §3 的 `WorldDigest` 共用同一結構）。

斷言：`run_scenario(s, cmds) == run_scenario(s, cmds)`，以及（跨重構防回歸）
把一組已知 digest 釘成 golden，引擎行為若無意改變則 digest 變動 → 測試紅。

## 4. 應守的不變量

即使位置/HP 因設計調整而變，下列仍應恆成立：

- HP 不為負。
- dead actor 已從 `TurnEngine.order_` 移除，不再行動。
- 若當前等待的是玩家（`waiting_actor()` 為 hero），`step()` 不應吞掉玩家回合。
- `begin_round()` → 反覆 `step()` 必在有限步內到達 `RoundDone`（不永久卡住）。

## 5. 不做的事

- 不再比較「多模式」——只有一個引擎，沒有 A/B/C。
- 不用 `get_debug_text()` 字串做比較（用結構化 digest）。
- 不在回放測試中依賴 wall-clock 或未固定種子的 AI。
