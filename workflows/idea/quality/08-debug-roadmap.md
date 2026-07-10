# Debug 工具路線：種子、重播、雜湊

> 日期：2026-06-21
> 定位：讓隨機 bug、決定性回放、長跑模擬可以重現、縮小、比對。
>
> **狀態更新（2026-06-21）**：trace 管線仍在（`set/get_trace_enabled`、`get_debug_log`、`clear_debug_log`、`get_debug_text`）。
> 重構移除了能量/效果系統，故 debug dump 不再含能量/effect/scheduler 欄位；
> 改為逐 actor 的 HP / act_point / 位置 / 操控者。其餘（種子統一、重播、雜湊）大致不變。

---

## 1. RNG 種子統一（優先序：高）

現有種子分散在 mapgen、NPC AI、turn RNG。建議引入：

```cpp
struct WorldSeed {
    uint64_t root;
    uint64_t map_seed;
    uint64_t ai_seed;
    uint64_t combat_seed;
};
```

由 root 派生子種子。snapshot / debug UI 顯示 root。
這是 02（決定性回放）與 03（長跑模擬）可重現的共同前置。

## 2. 指令重播（優先序：中）

記錄玩家提交的 command：

```text
round, actor_id/display_id, action_kind(Idle/Move/Wait), param(方向編碼 0..8)
```

搭配 seed 可重播大部分 bug（攻擊＝撞擊移動，故無獨立 attack 指令；無技能/詠唱，無 def_id）。
entity id 若不穩，需 display id 或 actor label。

## 3. 狀態雜湊（優先序：中）

`WorldDigest` 產生穩定 hash（與 02/03 共用同一結構）：

- map digest
- actor components digest（位置 / HP / act_point / controller kind）
- item digest
- turn engine digest（actor 順序 `order_`、當前 cursor / waiting）

用於決定性回放比對與重播驗證。hash 不取代可讀 dump；失敗時兩者都輸出。

## 4. 結構化 debug dump

`get_debug_text()` 保留，但底層先產生 `DebugSnapshot`，文字只是 renderer。
這樣測試可以吃結構化資料，人類仍可看文字。重構後的 dump 內容：

- 逐 actor：label / 位置 (x,y) / HP（current/max）/ `ActPointComponent.value` / `ControllerComponent.kind`。
- 樓層、turn count、是否等待玩家、回合是否進行中。
- **移除**：能量值、effect / DoT 清單、排程器 pending / world clock（皆已不存在）。

trace 行（`get_debug_log` ring）則是 `apply_action` / `TurnEngine` 經由 `EngineCtx.trace`
吐出的時序事件，與 snapshot 互補。

## 5. 不做的事

- 不重播 wall-clock input，只重播 core command。
- 不把雜湊當存檔相容保證。
- 不要求第一版能縮小 failing seed；先可重現。
- 不在 dump 中保留已移除系統（能量/效果/排程器）的欄位。
