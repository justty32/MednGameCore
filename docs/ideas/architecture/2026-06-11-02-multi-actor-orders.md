# order 表（多角色操控）落地計劃

> 日期：2026-06-11
> 一句話定位：把 `docs/superpowers/specs/turn-loop-example.cpp` 的多角色 order 表模型，改造進現有 scheduler 與 ZoneWorld/前端，淘汰「單一 hero」殘餘假設。

---

## 差距盤點：spec 已定案 vs 程式碼現況

spec（design doc §7 callout + turn-loop-example.cpp）定案：玩家操控**多個**角色、
無單一 hero、阻塞 = 任一玩家角色 idle 且無指令、下令順序可選
（Initiative/PlayerFirst）、打斷非即時、消費指令。

但現況程式碼處處單 hero：

| 位置 | 單 hero 殘餘 |
|---|---|
| `src/gbind/zone_world_gd.h:49-54` | `submit_hero_move/wait/cast/skill`、`hero_is_waiting()` 全綁 `hero_entity_` |
| `src/gbind/zone_world_gd.h:115-116` | `hero_entity_` 成員、`get_hero_x/y/hp` 系列 |
| `src/core/turn/npc_brain.h` | `decide_chase` 朝「英雄」追擊（單目標假設） |
| `godot_zone/map_view.gd` | WASD 直接餵 hero、`_advance_until_wait` 只認 hero |
| 排程器本身 | **已經是多角色乾淨的**：`PlayerControlledComponent` 任意多個、`waiting_actor()` 回 entity、`test_turn_scheduler.cpp` 有「兩個 PlayerControlled 依序阻塞」案 |

好消息：核心層幾乎不用動，**改造重心在 gbind 與前端**。

## 改造計劃（建議在 01 收斂後做，少改兩份排程器）

### 第 1 步：排程器補 order 語意（核心層，小改）

- 現在 `SchedulerBase::actors_sorted()` 用 entity id 排序＝隱性 Initiative 都沒有。
  引入 `OrderPolicy`（`rebuild_orders()` 對應物）：`Initiative`（速度/能量序——
  能量制下「誰先達 1000」本身就是先攻序，B 天然滿足）與 `PlayerFirst`
  （`stable_partition` 把 `PlayerControlledComponent` 排前段）。
- 落點：`SchedulerBase` 加 `order_mode_` + `build_orders(reg)`，B 的 `advance()`
  在「同 tick 多人就緒」時依 orders 取人。example 的 `last_actor_order_i` 游標
  對應 B 現有的「推進到下一個出手即 return」語意，不需額外狀態。

### 第 2 步：gbind 介面去 hero 化（破壞性，集中一次做）

- `submit_hero_*` → `submit_actor(int actor_id, ...)`；`hero_is_waiting()` →
  `get_waiting_actor() -> int`（-1 = 不卡）。`waiting_actor` 即 example 的
  聚焦訊號，前端拿它 highlight 該角色。
- `hero_entity_` 降級為「相機跟隨對象」（spec §3.3：HeroComponent 保留作
  主視角 tag），不再是指令路由依據。
- 與 04（snapshot 介面）合做：actor_id 的對外穩定編號問題（eid 遮 version
  hack，見 07）必須在這裡一併解，否則前端拿到的 id 不可靠。

### 第 3 步：NPC 腦升級（可後做）

- `decide_chase`（`src/core/turn/npc_brain.cpp`）從「追英雄」改「追最近的
  PlayerControlled actor」；簽名已吃 registry，改 view 即可，ctest 可測。

### 第 4 步：前端多角色操控

- `map_view.gd`：Tab/數字鍵切換「目前下令角色」，輸入寫入
  `player_instructions[waiting_actor]` 對應的 submit；推進計時器照舊
  （`_on_advance_tick` 已是解耦時鐘，符合 example 的 on_advance_timer）。

## 驗收情境（加進 ctest + headless verify）

1. 兩玩家角色 + 兩 NPC：advance 依序卡在兩個玩家角色，各 submit 後續推一輪。
2. PlayerFirst 模式：我方兩角色都先於 NPC 出手（檢查事件序）。
3. 角色 A channel 詠唱中、角色 B 等指令：B 收指令後世界續推、A 的 channel
   不被打斷（多角色 + channel 疊加，spec §9 開放項的主情境）。
4. 玩家角色死亡（die() 保護下降級為 game-over 標記）：orders 重建不含死者、
   不卡死迴圈。

## 優先序與風險

- **優先序：高**（這是 design doc 真相層已定案但未落地的最大缺口）。
- **風險·中**：gbind API 破壞性改名，前端 map_view.gd/verify.gd 同步重寫；
  verify.gd 曾出過「假通過」bug（session_log 2026-06-10），重寫時要先寫
  會失敗的驗證再接通。
- **風險·中**：「同 tick 多人就緒」在能量制下的順序定義要明寫（目前靠
  entity id 排序的隱性決定性），否則 PlayerFirst 切換會破壞 save 重播決定性。
- **風險·低**：`pending_` 收件匣在 scheduler 內部 map，多角色下每角色一格
  語意不變；但 save/load 不含它（見 07 P0），多角色化後更痛。
