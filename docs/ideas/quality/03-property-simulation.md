# Property / 模擬測試

> 日期：2026-06-11
> 定位：用隨機但可重現的長跑場景，捕捉單元測試很難列舉的交互 bug。

---

## 1. 目標

模擬測試不是驗證某一步精確輸出，而是驗證長時間內不變量：

- 不 crash。
- 不產生 invalid entity access。
- HP、energy、turn count 在合理範圍。
- dead actor 不再行動。
- scheduler 不 stall。

## 2. 最小模擬器（優先序：高）

```cpp
simulate(seed, mode, turns, actor_count, command_policy)
```

command_policy 可以很笨：

- 玩家若等待輸入，隨機 move / wait / skill。
- NPC 用既有 `decide_chase`。

重點是固定 seed 後可重現。

## 3. 不變量清單

- registry 中有 Health 的 actor：`0 <= hp <= max_hp`，除非死者已移除 Spatial。
- map 內最多一個 actor 佔同格。
- TimedEffect turns 不為負。
- OngoingAction remaining 不為負。
- event target 若非 0，應在事件產生當下有效或標記為 dead target。

## 4. 失敗輸出

失敗時印：

- seed
- mode
- turn index
- command history 最近 32 筆
- world digest

這比單純 assertion line 更重要，否則 fuzz 找到 bug 也很難重現。

## 5. 不做的事

- 不追求外部 property testing 框架第一版；doctest loop 足夠。
- 不讓模擬依賴 Godot。
- 不把隨機測試設成 flaky；seed 固定，另有本機 stress 模式可跑更多 seed。
