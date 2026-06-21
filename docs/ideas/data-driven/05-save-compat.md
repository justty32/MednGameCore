# 存檔相容性策略

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：趁存檔尚未成為玩家資產前，先定義 header、版本與 migration 原則。

---

## 1. 為什麼現在要做

目前存檔能 round-trip 世界狀態（headless `verify.gd` 的 save/load round-trip 為綠）。
序列化清單 `src/core/serialize/all_components.h` 在 2026-06-21 重構後已更新為：
`ActorComponent`、`SpatialComponent`、`MapData`、`NpcAiComponent`、`HealthComponent`、
`ItemComponent`、`CombatStatsComponent`、`WorldStateComponent`、`HeroComponent`、
**新增的 `ControllerComponent` 與 `ActPointComponent`**（取代已移除的
`EnergyComponent`/`OngoingActionComponent`/`PlayerControlledComponent`）。

> **舊問題已消失**：先前「save/load 不含排程器狀態（actors_sorted / waiting_actor /
> world clock / 進行中 channel）」這個相容性破口，已隨**整套能量排程器被移除**而不復
> 存在。TurnEngine 的 actor list 順序＝加入序、act_point 每回合重設，皆可由
> setup（按 prototype 重建）＋元件現值還原，無額外排程器狀態需序列化。

但存檔仍缺 header。未來 AllComponents、（未來重新引入的）ActionDef、PrototypeDef
都會演進，若沒有版本資訊，讀舊檔只能猜。

## 2. Save Header（優先序：高）

```cpp
struct SaveHeader {
    uint32_t magic;          // 'MNGC'
    uint16_t save_version;   // 存檔格式版本
    uint16_t game_version;   // 可選，或字串 hash
    uint32_t data_schema;    // prototypes（及未來 actions）schema 摘要
};
```

先寫在 binary archive 最前面。讀檔先檢查 header，不對就回明確錯誤。

## 3. Migration 原則

- 加 component 且有預設值：可讀舊檔，補預設。
- 改 component 欄位語意：升 save_version，寫 migration。
- 刪 component：至少保留讀舊欄位並丟棄一版。
- Action / Prototype 定義不進存檔，只存穩定 id。

## 4. 測試

建立 `tests/data/saves/` 放小型 golden save：

- v1 minimal world（hero + 牆/地板）。
- v1 world with NPC（含 ControllerComponent/ActPointComponent，驗證新元件 round-trip）。
- v1 world after pickup（道具消失、HP 上升的狀態）。

> 註：舊草案的「actor with timed effects」「active channel」golden save 已不適用——
> DoT/狀態效果與多回合詠唱（channel）皆在 2026-06-21 重構移除。

每次改 save schema 必須能讀前一版，或明確在文件宣告破壞。

## 5. 不做的事

- 不承諾 alpha 期間永久相容，但要能明確失敗。
- 不把 JSON def 複製進存檔；存 id + runtime state。
- 不 silent fallback 成新遊戲。
