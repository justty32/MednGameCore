# order 表（多角色操控）落地計劃

> 日期：2026-06-11（原稿）／狀態更新：2026-06-21
> 一句話定位：把 `workflows/specs/archive/turn-loop-example.cpp` 的多角色 order 表模型，落地到
> **TurnEngine 的 actor list + ControllerComponent(kind=Faction)** 之上，淘汰「單一 hero」殘餘假設。

> [!IMPORTANT]
> **2026-06-21 重構後校正**：原稿假設 order 表蓋在「scheduler 與 `SchedulerBase::actors_sorted()`」
> 之上、由 `PlayerControlledComponent` 標記玩家角色。這些都已不存在——排程器整套移除、
> `PlayerControlledComponent` 被 `ControllerComponent` 取代。
>
> 新落點：order 表蓋在 **`TurnEngine` 的 `std::list<entt::entity>`（加入序）** 與
> **`ControllerComponent { kind; faction; }`** 上。`ControllerKind::Faction`（暫等同 Self）與
> `faction` 欄位**已存在於程式碼**，正是「多角色／多陣營操控」的預留落地點。本提議的玩法目標
> （多玩家角色、無單一 hero、可選下令順序、消費指令）**仍然有效**，只是技術接點換了。

---

## 差距盤點：spec 已定案 vs 程式碼現況（2026-06-21）

spec（design doc §7 callout + turn-loop-example.cpp）定案：玩家操控**多個**角色、
無單一 hero、阻塞 = 任一玩家角色待指令、下令順序可選（Initiative/PlayerFirst）、消費指令。

現況程式碼仍處處單 hero：

| 位置 | 單 hero 殘餘 |
|---|---|
| `src/gbind/zone_world_gd.h` | `player_move/player_wait`、`is_waiting_player()` 隱含「唯一玩家 actor」；`hero_entity_` 成員、`get_hero_x/y/hp` 系列 |
| `src/gbind/zone_world_gd.cpp` | `npc_decide` → `decide_chase` 追「英雄」（單目標假設）；setup 只造一個 `ControllerKind::Player` |
| `godot_zone/map_view.gd` | WASD 直接呼叫 `player_move()`，預設前端只有一個受控角色 |
| `TurnEngine` 本體 | **已是多角色乾淨的**：actor list 可含任意多個 `ControllerKind::Player`，`step()` 會在每個 Player actor 各回一次 `WaitingPlayer`，無單 hero 假設 |

好消息依舊成立：**引擎核心幾乎不用動，改造重心在 gbind 與前端**，以及把 `ControllerComponent`
的多陣營語意接起來。

## 改造計劃

### 第 1 步：actor 取人順序補 order 語意（核心層，小改）

- 現在 `TurnEngine` 取人＝list 加入序（hero 先）＝隱性「PlayerFirst 且固定」。引入可選的
  **取人策略**：`JoinOrder`（現狀）、`PlayerFirst`（`stable_partition` 把
  `ControllerKind::Player` 排前段）、`Initiative`（未來若引入能量／速度擴充才有意義，見 01）。
- 落點：在 `TurnEngine::begin_round()` 建立本回合的「行動順序視圖」時套用策略；游標仍是現有的
  list iterator 語意，無需新增持久狀態。

### 第 2 步：gbind 介面去 hero 化（破壞性，集中一次做）

- `player_move/player_wait`（隱含唯一 Player）→ `submit_actor(int actor_id, ...)`；
  `is_waiting_player()` → `get_waiting_actor() -> int`（-1 = 不卡），即 example 的聚焦訊號，
  前端拿它 highlight 該角色。
- `hero_entity_` 降級為「相機跟隨對象」（主視角 tag），不再是指令路由依據；路由改看
  「目前 `WaitingPlayer` 的是哪個 actor」。
- 與 04（snapshot 介面）合做：actor_id 的對外穩定編號問題（eid 遮 version hack，見 07）必須
  在這裡一併解，否則前端拿到的 id 不可靠。

### 第 3 步：多陣營與 NPC 腦（接 ControllerComponent.faction）

- 把 `ControllerKind::Faction` 從「暫等同 Self」升級為真正的陣營分派：同 `faction` 的 actor
  共享一個決策來源（軍團 AI），敵對 `faction` 互為攻擊目標。`faction` 欄位**已在
  ControllerComponent 內**，這是落地點。
- `decide_chase`（`src/core/turn/apply_action.cpp`）從「追英雄」改「追最近的敵對陣營 actor」；
  簽名已吃 registry，改 view 篩 `ControllerComponent.faction` 即可，doctest 可測。

### 第 4 步：前端多角色操控

- `map_view.gd`：Tab/數字鍵切換「目前下令角色」，輸入寫入對應 `submit_actor`。前端已是
  「按一鍵推進一輪」模型，多角色化只是把單一投遞點換成「依 `get_waiting_actor()` 路由」。

## 驗收情境（加進 doctest + headless verify）

1. 兩玩家角色 + 兩 NPC：`step()` 依序在兩個玩家角色各回 `WaitingPlayer`，各 submit 後續推一輪。
2. PlayerFirst 取人策略：我方兩角色都先於 NPC 行動（檢查事件序）。
3. 兩陣營對抗：`ControllerKind::Faction` 的兩軍互相以最近敵方為目標、不誤擊同陣營（檢查 ActorDied 目標）。
4. 玩家角色死亡：actor list 即時 `remove_actor`（cursor 安全），order 重建不含死者、不卡死迴圈。

## 優先序與風險

- **優先序：高**（這是 design doc 真相層已定案但未落地的最大缺口，且落點已備齊）。
- **風險·中**：gbind API 破壞性改名，前端 map_view.gd/verify.gd 同步重寫；verify.gd 曾出過
  「假通過」bug（見 07），重寫時要先寫會失敗的驗證再接通。
- **風險·低**：取人策略切換（PlayerFirst vs JoinOrder）必須明寫決定性規則，否則 save 重播失準。
- **風險·低**：`Faction` 升級後，同陣營多 actor 的決策共享要避免重複計算；先做「每 actor 各自
  decide、只是共享目標選擇」即可。
