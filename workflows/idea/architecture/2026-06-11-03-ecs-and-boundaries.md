# ECS 演化與 TurnEngine / ZoneWorld 職責邊界

> 日期：2026-06-11（原稿）／狀態更新：2026-06-21
> 一句話定位：壓低「新增一個 component」的邊際成本，並把 ZoneWorld god class 拆回「薄綁定層」。

> [!IMPORTANT]
> **2026-06-21 重構後校正**：原稿的職責邊界畫在「`TurnWorld`（純 core 排程器作業環境）vs
> ZoneWorld」之間。`TurnWorld` 已**整個移除**。新邊界是 **`TurnEngine`（純 core，**不持有
> registry**，只在 `step(ctx)` 時用 `EngineCtx` 借用）vs `ZoneWorld`（gbind，仍是 god class
> 待拆）**。元件序列化清單也已換血：`ControllerComponent` / `ActPointComponent` 取代了
> `EnergyComponent` / `OngoingActionComponent` / `PlayerControlledComponent` / `TimedEffectsComponent`。

---

## 問題一：component 註冊/序列化的擴充成本（優先序：高）

新增一個 component 目前要碰的點：

1. `src/core/components/*.h` 寫 struct + `serialize()`（或 save/load 對，如 `ControllerComponent`
   因 entt::entity 需手動轉底層整數而用 save/load 分離寫法）——成本合理。
2. `src/core/serialize/all_components.h` 的 type_list 手動加一行。
3. gbind 若要曝露 → `zone_world_gd.cpp` 手寫 getter/dump 段落。

### 現況（2026-06-21）

重構已順手清掉原稿抱怨的「type_list 漏登記」歷史包袱：舊的 Energy/OngoingAction/PlayerControlled/
TimedEffects 連同其產生它們的系統一起被移除，`all_components.h` 已更新為納入
**`ControllerComponent` + `ActPointComponent`**。當前序列化清單與實際使用的 component 一致。

### 提議（仍有效）

- **round-trip 測試常設化**：加一個「save→load→再 save，bytes 相等」的 round-trip 測試，
  讓「新增 component 忘記登記 type_list」這類缺口由測試結構性抓出，而不是靠 code review 撞到。
  這是上一輪「漏登記」事故的根因防護，與本次重構無關，仍應補。
- **static_assert 防呆（中期）**：用 `entt::type_list_size` + 手動維護的
  `kExpectedComponentCount`，新增 component 忘記登記時至少在改 count 時被迫想起 type_list。
  更強的巨集自動收集對這個規模屬過度工程，不建議。
- **版本化**：save 格式目前無 schema 版本欄位。在 header 寫入 `format_version` 整數 + 載入時
  不符即拒絕，是三層架構（World 層存檔引用 Area 層存檔）前的必要地基。成本一天內。

## 問題二：TurnEngine 與 ZoneWorld 的職責邊界（優先序：高）

### 現況邊界

`TurnEngine`（`src/core/turn/turn_engine.{h,cpp}`）是純 core 的回合推進引擎：維護 actor list、
`begin_round()`/`step()`/分派、`add_actor`/`remove_actor`/`contains`/`order()`。它**不持有
registry**——每次 `step(ctx)` 由呼叫端傳入 `EngineCtx { registry&; decide; resolve; trace; }`，
引擎只借用。這個「引擎純粹、可單獨 doctest」的性質是重構刻意保留的紀律（`test_turn_engine.cpp`
正是因此能脫離 Godot 測加入序、act_point 重設、玩家阻塞、撞死發 ActorDied）。

`ZoneWorld`（`src/gbind/zone_world_gd.{h,cpp}`）則同時是：
(a) Godot Node 綁定、(b) 世界生命週期（setup_world/setup_map/next_floor/restart）、
(c) 引擎宿主（持有 `EntityManager em_` 與 `TurnEngine engine_`、組 `make_ctx()`、`pump()`）、
(d) 事件→signal 轉譯（`drain_events()`）、(e) trace ring buffer、
(f) 診斷字串工廠（`get_debug_text` 逐 actor dump）、(g) NPC 決策橋（`npc_decide` → `decide_chase`）。
**god class 是這裡，不是 core。** 重構移除了原稿提到的「舊同步 move() 路徑」，但 ZoneWorld
仍肥。

### 提議拆分：抽出 godot-free 的 `ZoneSession`

```
src/core/session/zone_session.h   ← 新增（zone_core 內，可 doctest）
  持有：EntityManager、TurnEngine、events_、turn_count_、current_floor_、game_over_
  方法：setup/restart/next_floor、submit(actor, Action)、step()、
        make_ctx()、drain_events() → std::vector<ZoneEvent>、npc_decide()
src/gbind/zone_world_gd.*         ← 萎縮成薄殼
  持有：ZoneSession 一顆 + trace ring + String 轉譯（utf8 集中於此）+ Node 生命週期
```

收益：
- 世界生命週期可 doctest（如 restart／next_floor 的實體清理早一步在 core 抓到，而不是在
  Godot 端撞出來）。
- 04 的 snapshot 介面有乾淨落點（ZoneSession 產 struct，gbind 轉 Dictionary）。
- 05 的 Region/World 層將來掛的是 ZoneSession（純 C++ 可組合），不是 Godot Node。

### 邊界守則（寫進 PROJECT.md 的一句話版本）

- core 層禁 include godot_cpp（現有 CMake 已用目錄隔離保證，維持）。
- gbind 層禁遊戲規則：只准「型別轉譯 + signal + Node 生命週期」。檢查法：gbind 出現
  `if (hp <= 0)` 之類規則判斷即違規（規則歸 `apply_action`）。
- `EngineCtx` 維持「每次 make_ctx() 現組的借用窗口」性質，不要讓它長成持久物件——持久狀態
  歸 ZoneSession，`EngineCtx` 只是借據。TurnEngine 不持 registry 的紀律必須守住。

## 風險

- **中**：ZoneSession 抽取是一次大搬家，期間 headless verify 與 doctest 都要保持綠。緩解：
  分兩刀——先搬 (b)(c)(d)，診斷字串 (f) 留在 gbind 等 04 一起處理。
- **低**：format_version 與 round-trip 測試補上後，舊存檔不相容。現階段無玩家存檔資產，正是
  做格式破壞的最便宜時刻。
