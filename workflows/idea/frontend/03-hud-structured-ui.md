# 結構化 HUD：狀態、訊息與操作面板

> 日期：2026-06-21
> 定位：把單一 debug Label 演進成可玩的 roguelike HUD，同時保留開發期可觀測性。

> 狀態更新（2026-06-21）：回合系統重構後，`get_hero_status()` / `get_hero_effects()` **已不存在**；技能/能量/詠唱/狀態效果亦全部移除。現況可用資料源為 `get_debug_text()` 字串 dump，加上 getters（`get_hero_hp/get_hero_max_hp`、`get_turn_count`、`get_current_floor`、`get_npc_count`、`get_hero_x/y`）。本文已改寫為以這些現存來源為起點；結構化 HUD 仍是有效的未來方向。

---

## 1. 現況

`map_view.gd` 目前把 `ZoneWorld.get_debug_text()` 塞進一個 Label。這很適合早期驗證，但玩家需要的是可掃描的資訊：

- 自己 HP（`get_hero_hp` / `get_hero_max_hp`）。
- 當前回合數、樓層（`get_turn_count` / `get_current_floor`）。
- 周圍敵人數量與簡要狀態（現況只有 `get_npc_count` 總數）。
- 最近發生的事件（目前只能從 trace log / signal callback 拼）。

注意：能量、施法狀態、技能 cooldown 等欄位在重構後**已不存在**，HUD 不應再規劃這些區塊（俟未來擴充再說）。

## 2. HUD 區塊提案（優先序：高）

先做三塊（皆吃現存資料源）：

| 區塊 | 資料來源 | 內容 |
|---|---|---|
| Hero panel | `get_hero_hp/max_hp`、`get_turn_count`、`get_current_floor` | HP 條、回合數、樓層 |
| Message log | signal callbacks（`hero_bumped_*`/`npc_died`/`item_picked_up`/`game_over`）或 02 的 drain_events_v2 | 最近 8-12 筆事件 |
| Inspect strip | 游標或最近目標（俟結構化 `get_actors()` 後） | enemy HP、距離 |

視覺上不追求華麗，先追求掃描效率：小字、固定位置、數值對齊、狀態 icon 可後補。

## 3. 互動原則

- HUD 是顯示快取，不能成為遊戲狀態真相。
- Message log 顯示結構化事件；`get_debug_text()` dump 只當 debug fallback（收進 07 的摺疊面板）。
- HP 條等顯示值每輪結束（`round_finished`）後與 getters 對帳，避免飄移。

## 4. Godot 落地順序

1. 先用現有 getters 拼出 Hero panel（HP 條 = `get_hero_hp/get_hero_max_hp`），取代單一大 Label 的核心區。
2. `map_view.tscn` 加固定 HUD 容器（`CanvasLayer`），把 `get_debug_text()` Label 移進可摺疊面板（07）。
3. Message log 接 signal callbacks；之後改吃 02 的 `drain_events_v2()` 取得帶座標/數值的結構化事件。
4. 待 core 提供結構化 `get_actors()`（01 §4）後，補 Inspect strip 顯示敵人 HP。

## 5. 不做的事

- 不先做大型 inventory / character sheet。
- 不規劃技能列 / 能量條 / 施法進度——這些在重構後不存在，屬未來擴充才考慮。
- 不在 HUD 裡放操作教學文字；按鍵提示可放 tooltip 或 debug 面板。
- 不把所有 actor 狀態都常駐顯示，避免資訊噪音。
