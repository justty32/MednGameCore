# 結構化 HUD：狀態、訊息與操作面板

> 日期：2026-06-11
> 定位：把單一 debug Label 演進成可玩的 roguelike HUD，同時保留開發期可觀測性。

---

## 1. 現況

`map_view.gd` 目前把 `ZoneWorld.get_debug_text()` 塞進一個 Label。這很適合早期驗證，但玩家需要的是可掃描的資訊：

- 自己 HP / energy / 施法狀態。
- 周圍敵人的簡要狀態。
- 最近發生的事件。
- 可用技能與 cooldown。

## 2. HUD 區塊提案（優先序：高）

先做四塊：

| 區塊 | 資料來源 | 內容 |
|---|---|---|
| Hero panel | snapshot actor | HP、energy、ongoing action、effects |
| Skill bar | ActionLibrary + cooldown | 1-4 技能、cd、是否可用 |
| Message log | drain_events / trace | 最近 8-12 筆事件 |
| Inspect strip | 游標或最近目標 | enemy HP、狀態、距離 |

視覺上不追求華麗，先追求掃描效率：小字、固定位置、數值對齊、狀態 icon 可後補。

## 3. 互動原則

- HUD 是顯示快取，不能成為遊戲狀態真相。
- 技能按鈕灰掉只是提示；core resolve 仍要檢查 cd / 資源。
- Message log 顯示結構化事件，trace 只當 debug fallback。

## 4. Godot 落地順序

1. `get_snapshot()` 曝 hero HP / energy / effects。
2. `map_view.tscn` 加固定 HUD 容器，取代單一大 Label。
3. Message log 改吃 `drain_events()`，debug dump 收到摺疊面板。
4. 技能列先硬接現有 fireball / heal / venom，之後改從 ActionLibrary export。

## 5. 不做的事

- 不先做大型 inventory / character sheet。
- 不在 HUD 裡放操作教學文字；按鍵提示可放 tooltip 或 debug 面板。
- 不把所有 actor 狀態都常駐顯示，避免資訊噪音。
