# 三排程器收斂策略

> 日期：2026-06-11
> 一句話定位：實驗期（A/B/C 可切換比較）過後，決定誰是正史、切換機制去留、以及用什麼準則拍板。

---

## 現況

`src/core/turn/` 三個實作共用 `TurnScheduler` 介面（`turn_scheduler.h`）+
`SchedulerBase`（pending 收件匣、`waiting_actor()`、`actors_sorted()` 決定性迭代）：

| | 檔案 | 行數(.cpp) | 模型 |
|---|---|---|---|
| A | `energy_instant_scheduler.cpp` | 52 | ToME4 能量瞬發，無 channel |
| B | `energy_channel_scheduler.cpp` | 95 | 能量 + 可打斷 channel（spec 標注「最完整」）|
| C | `tick_remaining_scheduler.cpp` | 57 | 純 remaining_ticks，無能量 |

切換點：`make_scheduler.cpp` 工廠 + `ZoneWorld::set_scheduler_mode(int)`
（`src/gbind/zone_world_gd.h:47`），前端 TAB 鍵 flip。預設 B（`scheduler_mode_{1}`）。

## 提議：收斂到 B 為唯一正史（優先序：高）

理由（依 spec 與 session_log 實證）：

1. **B 是 A 的嚴格超集**：A = B 在所有 action `weight ≤ 1` 時的行為（B 的
   `step_actor` 對 weight≤1 即刻 resolve）。A 沒有獨立存在價值，留著只是
   「無 channel 的退化模式」，用資料（所有 ActionDef weight=1）就能重現。
2. **C 缺速度表達**：spec §5.C 自己承認「無 ToME 風加速率 buff 表達」。
   session_log 2026-06-11 的 NPC 速度差實驗（caster 80/近戰 130）正是
   energy 模型才看得到的差異——玩法已實質依賴能量制。
3. **DoT/hook 已三份維護**：`on_actor_turn` hook 與 `reg.valid` 防護
   （防 DoT 自殺越界）在三個 `advance()` 各複製一份。每加一個回合開始/結束
   時點的機制，成本 ×3。

## 切換機制去留

- **介面留、工廠收斂**：`TurnScheduler` 抽象介面 + `make_scheduler` 保留
  （測試與未來 Region 層排程器可能再用），但 `SchedulerMode` 枚舉縮成一項或
  移到 tests-only。
- **`set_scheduler_mode` 從 GDExtension 綁定移除**：前端不該有玩法語意上的
  排程器選擇；TAB 鍵實驗功能移除。spec §9 開放項「切換時狀態處理」直接消滅
  ——不再有切換，就沒有能量/channel 狀態跨版語意問題。
- **A/C 程式碼處置**：移到 `tests/` 旁或直接刪除（git 歷史可考古）。建議
  **刪**：保留會誘使「再比一下」，且 02（order 表）改介面時又得改三份。

## 評估準則（拍板前最後驗證清單）

收斂前用現有切換機制做一輪結構化比較，記錄到本目錄：

| 準則 | 量法 | 預期勝者 |
|---|---|---|
| 玩法表達力 | 速度 buff、channel、打斷、DoT 是否原生支援 | B |
| 決定性 | 同 seed 同指令序列 → 同世界狀態（serialize 後 hash 比對） | 三者皆須通過 |
| 推進成本 | `advance()` 每次喚醒掃描量（A/B 每 tick 掃全 actor 累能 vs C 的 min-jump） | C（但 actor 數 <100 無感）|
| 心智模型 | 向前端解釋 waiting/blocking 語意所需的文字量 | C 最簡、B 可接受 |
| 跨層相容 | 能量 tick 能否映射到 Region 層分鐘級時間（見 05） | B/C 皆可，B 需定義 tick↔秒 |

## 風險

- **中**：B 的能量制是 ToME4 借來的；若未來玩法走向「同時行動/計畫制」
  （如 Frozen Synapse 式），能量序列模型要大改。緩解：`TurnScheduler`
  介面保留，正史排程器仍是可替換單元。
- **低**：刪 A/C 後測試覆蓋下降（`test_turn_scheduler.cpp` 五情境 ×3 變 ×1）。
  緩解：情境測試本來就是介面級，保留全部情境只跑 B。
- **低**：C 的 min-jump（整步推進）想法在 actor 很多、行動很長時有效能價值。
  緩解：B 內部可加「無人就緒時直接跳到下一個就緒 tick」的快轉，等價吸收。
