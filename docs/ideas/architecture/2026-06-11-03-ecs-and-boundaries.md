# ECS 演化與 TurnWorld / ZoneWorld 職責邊界

> 日期：2026-06-11
> 一句話定位：壓低「新增一個 component」的邊際成本，並把 763 行的 ZoneWorld god class 拆回「薄綁定層」。

---

## 問題一：component 註冊/序列化的擴充成本（優先序：高）

新增一個 component 目前要碰的點：

1. `src/core/components/*.h` 寫 struct + `serialize()`（成本合理）
2. `src/core/serialize/all_components.h` 的 `AllComponents` type_list 手動加一行
   ——**而且現在已經漏了**：`EnergyComponent`、`OngoingActionComponent`、
   `PlayerControlledComponent`、`TimedEffectsComponent` 四個都有 `serialize()`
   方法卻不在 type_list 內 → save/load round-trip「全綠」其實只覆蓋舊世界狀態，
   排程器相關狀態全部默默丟失（詳見 07 P0）。
3. gbind 若要曝露 → `zone_world_gd.cpp` 手寫 getter/dump 段落。

### 提議

- **短期（必做）**：把四個遺漏 component 補進 `AllComponents`，並加一個
  「save→load→再 save，bytes 相等」的 round-trip 測試到 `test_serialize.cpp`
  ——這類缺口應該由測試結構性抓出，而不是 code review 撞到。
- **中期**：加 static_assert 防呆。例如在 all_components.h 用
  `entt::type_list_size` + 一個手動維護的 `kExpectedComponentCount`，新增
  component 忘記登記時至少在改 count 時被迫想起 type_list。更強的方案
  （巨集註冊器自動收集）對這個規模是過度工程，不建議。
- **版本化**：save 格式目前無 schema 版本欄位。`save_load.h` 的 fold
  expression 模型下，type_list 增刪即格式破壞。在 header 寫入
  `format_version` 整數 + 載入時不符即拒絕，是三層架構（World 層存檔會
  引用 Area 層存檔）前的必要地基。成本一天內。

## 問題二：TurnWorld 與 ZoneWorld 的職責邊界（優先序：高）

### 現況邊界（其實畫得不錯）

`TurnWorld`（`src/core/turn/turn_world.h`）是純 core 的「排程器作業環境」：
reg/effects/npc_decide/cfg/clock + map/events 指標 + `on_actor_turn` hook +
`trace` sink。scheduler 不碰 map、effect 才碰——註解寫明，紀律有守住。

`ZoneWorld`（`src/gbind/zone_world_gd.cpp`，763 行）則同時是：
(a) Godot Node 綁定、(b) 世界生命週期（setup_world/next_floor/restart）、
(c) effect 實例與 ActionLibrary 持有者、(d) 事件→signal 轉譯、(e) trace ring
buffer、(f) 診斷字串工廠（get_debug_text 等百餘行 String 組裝）、
(g) 舊同步路徑 move()/advance_turn()。**god class 是這裡，不是 core。**

### 提議拆分：抽出 godot-free 的 `ZoneSession`

```
src/core/session/zone_session.h   ← 新增（zone_core 內，可 ctest）
  持有：EntityManager、scheduler_、effects_/各 effect 實例、ActionLibrary、
        events_、world_clock_、turn_count_、current_floor_、game_over_
  方法：setup/restart/next_floor、submit(actor, Action)、step()、
        make_turn_world()、drain_events() → std::vector<ZoneEvent>
src/gbind/zone_world_gd.*         ← 萎縮成薄殼
  持有：ZoneSession 一顆 + trace ring + String 轉譯（utf8 集中於此）
```

收益：
- 世界生命週期可 ctest（現在 restart 的雙重 destroy 崩潰是在 Godot 端撞出來
  的，session_log 2026-06-10——若 restart 在 core 可測，早就被抓到）。
- 04 的 snapshot 介面有乾淨落點（ZoneSession 產 struct，gbind 轉 Dictionary）。
- 05 的 Region/World 層將來掛的是 ZoneSession（純 C++ 可組合），不是 Godot Node。

### 邊界守則（寫進 PROJECT.md 的一句話版本）

- core 層禁 include godot_cpp（現有 CMake 已用目錄隔離保證，維持）。
- gbind 層禁遊戲規則：只准「型別轉譯 + signal + Node 生命週期」。
  檢查法：gbind 出現 `if (hp <= 0)` 之類規則判斷即違規（目前舊 move()
  路徑就有，隨 07 的雙路徑清理一併移除）。
- TurnWorld 維持「每次 make_turn_world() 現組的窗口 struct」性質，不要讓它
  長成持久物件——持久狀態歸 ZoneSession，TurnWorld 只是借據。

## 風險

- **中**：ZoneSession 抽取是一次大搬家（763 行裡約 6 成要移動），期間
  headless verify 與 ctest 都要保持綠。緩解：分兩刀——先搬 (b)(c)(d)，
  診斷字串 (f) 留在 gbind 等 04 一起處理。
- **低**：AllComponents 補登記後舊存檔不相容。現階段無玩家存檔資產，
  正是做格式破壞的最便宜時刻，越晚越貴。
