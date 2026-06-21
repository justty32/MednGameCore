# Property / 模擬測試

> 日期：2026-06-21
> 定位：用隨機但可重現的長跑場景，捕捉單元測試很難列舉的交互 bug。
>
> **狀態更新（2026-06-21）**：本檔大致仍有效。回合系統重構後不再有能量/技能，範例改用移動/撞擊/拾取；
> 模擬器簽名去掉 `mode`（只剩一個 `TurnEngine`）。

---

## 1. 目標

模擬測試不是驗證某一步精確輸出，而是驗證長時間內不變量：

- 不 crash。
- 不產生 invalid entity access。
- HP、turn count 在合理範圍。
- dead actor 已從 `TurnEngine.order_` 移除、不再行動。
- `begin_round()` → 反覆 `step()` 必在有限步內到達 `RoundDone`（不永久卡住）。

## 2. 最小模擬器（優先序：高）

```cpp
simulate(seed, rounds, actor_count, command_policy)
```

command_policy 可以很笨：

- 玩家若 `WaitingPlayer`，隨機 `Action::move(dir)` / `Action::wait()`。
- NPC 用既有 `decide_chase`（朝英雄移動一格，相鄰即撞擊攻擊）。

重點是固定 seed 後可重現（與 02 的決定性回放共用同一可注入 RNG）。

## 3. 不變量清單

- registry 中有 Health 的 actor：`0 <= hp <= max_hp`，除非死者已被移除。
- map 內最多一個 actor 佔同格（移動/撞擊結算後）。
- `ActPointComponent.value` 在合理範圍（首次輪到重設 1000、行動後歸 0）。
- 撞擊致死的目標只發一次 `ActorDied`，且隨後不再出現在 `order_`。
- event target 若非 0，應在事件產生當下有效或標記為 dead target。

## 4. 失敗輸出

失敗時印：

- seed
- round index
- command history 最近 32 筆（玩家指令 + NPC 推斷的動作）
- world digest（與 02/08 共用結構）

這比單純 assertion line 更重要，否則 fuzz 找到 bug 也很難重現。

## 5. 不做的事

- 不追求外部 property testing 框架第一版；doctest loop 足夠。
- 不讓模擬依賴 Godot。
- 不把隨機測試設成 flaky；seed 固定，另有本機 stress 模式可跑更多 seed。
