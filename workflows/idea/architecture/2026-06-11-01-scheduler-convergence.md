# 三排程器收斂策略（結論已被超越）→ 先攻／能量制的未來擴充取捨

> 日期：2026-06-11（原稿）／狀態更新：2026-06-21
> 一句話定位：原為「實驗期 A/B/C 過後決定誰是正史」的收斂提議；現重定位為「若未來要重新引入
> 先攻／能量制，該做什麼取捨」的設計參考。

> [!IMPORTANT]
> **結論已執行，且比本文更激進。** 本文原提議「三排程器收斂到 B 為唯一正史」。實際決定不是
> 收斂——而是**連 B 也整個移除**，回到最簡傳統回合制 `TurnEngine`（順序＝加入序、一 actor 每
> 回合行動一次、無能量／無 channel）。因此「現況」「提議：收斂到 B」「切換機制去留」三節描述的
> 程式已不存在（`turn_scheduler.h`、三個 scheduler.cpp、`make_scheduler.cpp`、`SchedulerBase`、
> `SchedulerMode`、`set_scheduler_mode`、TAB 切換全已刪除）。
>
> 下文保留的價值在於**取捨分析**（能量 vs tick vs 傳統）。把它讀作：「若未來要在 TurnEngine 上
> 重新長出先攻／能量制，下列權衡仍然成立。」實作落點不再是『選哪個 scheduler』，而是『在
> `TurnEngine` 的 `decide`/`resolve` 與 `ActPointComponent` 上做可替換擴充』。

---

## 歷史現況（2026-06-11，已移除）

> 以下描述的程式已於 2026-06-21 重構整套刪除，僅留作背景。

`src/core/turn/` 曾有三個實作共用 `TurnScheduler` 介面 + `SchedulerBase`（pending 收件匣、
`waiting_actor()`、`actors_sorted()` 決定性迭代）：

| | 檔案 | 行數(.cpp) | 模型 |
|---|---|---|---|
| A | `energy_instant_scheduler.cpp` | 52 | ToME4 能量瞬發，無 channel |
| B | `energy_channel_scheduler.cpp` | 95 | 能量 + 可打斷 channel（spec 標注「最完整」）|
| C | `tick_remaining_scheduler.cpp` | 57 | 純 remaining_ticks，無能量 |

切換點曾是 `make_scheduler.cpp` 工廠 + `ZoneWorld::set_scheduler_mode(int)`，前端 TAB 鍵 flip。

## 實際決定：全移除，回到最簡 TurnEngine（已執行）

不留任何 scheduler。`TurnEngine` 取代之：

- 維護 `std::list<entt::entity>`，**順序＝加入序**（無 Initiative、無能量序、無 weight）。
- `begin_round()` 後 `step(ctx)` 到 `RoundDone`；每 actor 本回合首次輪到時 `ActPointComponent.value`
  重設 `kActPointFull = 1000`，行動後歸 0；**目前一 actor 每回合行動一次**。
- 分派依 `ControllerComponent.kind`：`Player` → `WaitingPlayer` 暫停等 `submit`；`Self`/`Faction`
  → 立即 `ctx.decide(e)` 再 `ctx.resolve()`。

換言之：原本「能量 vs tick」的兩難被「都不要，先把回合制做到最乾淨」一刀切過。能量制不再是
現存程式的退化選項，而是一個全新的、未來才考慮的擴充主題。

## 若未來要重新引入先攻／能量制（重定位後的提議）

下列分析原本用來「選正史 scheduler」，現在用來「評估要不要、以及怎麼在 TurnEngine 上加回」：

- **落點不是新 class，而是擴充三個既有接點**：
  1. `ActPointComponent.value`：已預留「依成本一回合行動多次」。能量制＝把 act_point 從
     「回合首次輪到重設 1000、行動歸 0」改為「每 tick 累積、達門檻才行動、行動依 weight 扣減」。
  2. `EngineCtx.decide`/`resolve`：行動成本（weight）由 `resolve` 回報、由引擎扣 act_point；
     channel／詠唱＝`decide` 回一個「需多次 step 才完成」的動作，引擎在中途仍可被打斷。
  3. actor list 取人順序：先攻＝把「加入序」換成「依 act_point/速度排序」的取人策略。
- **介面保留可替換性**：`TurnEngine` 本身不該硬編能量；速度／先攻屬於「取人策略」與「成本結算」，
  應做成可注入的策略物件，讓「傳統回合制」與「能量回合制」是同一引擎的兩種配置。

## 取捨分析（保留：能量 vs tick vs 傳統，作為擴充前的評估）

| 準則 | 能量制（舊 B） | 純 tick（舊 C） | 傳統回合（現 TurnEngine） |
|---|---|---|---|
| 玩法表達力 | 速度 buff／channel／打斷／DoT 原生 | 速度差需另表達 | 最弱：人人等速、一回合一動 |
| 心智模型成本 | 高：要向前端解釋 waiting/blocking/累能 | 中 | 最低：「按一鍵＝我動一次＋NPC 動一輪」 |
| 決定性 | 同 seed 同序列須能 serialize 後 hash 相等 | 同上 | 最易保證（無浮點、無累能順序歧義）|
| 推進成本 | 每 tick 掃全 actor 累能 | min-jump 跳到下一就緒 | O(actor)/回合，最低 |
| 跨層相容（見 05）| 能量 tick → Region 分鐘需定義 tick↔秒 | 同上 | 回合 → Region 分鐘需定義回合↔時間 |

結論方向：先攻／能量制的玩法價值真實存在（速度差、詠唱、打斷），但其複雜度只在玩法確實需要時
才值得。當前選擇是先把傳統回合做穩，把能量制留作「需要時再在 act_point/decide/resolve 上長出來」
的可選擴充。

## 風險

- **中**：未來若玩法走向「同時行動／計畫制」（如 Frozen Synapse 式），無論能量序或回合序模型
  都要大改。緩解：把「取人策略 + 成本結算」做成可替換單元，TurnEngine 本體只管 step/分派。
- **低**：能量制的決定性比傳統回合難保證（同 tick 多人就緒的順序定義）。引入前須先寫
  serialize 後 hash 比對的重播測試。
- **已消除**：原「刪 A/C 後測試覆蓋下降」「切換時狀態跨版語意」等風險，因排程器整套移除而不再存在。
