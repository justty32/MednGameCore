# Layer Integration：Region / World 與 Zone 的接線

> 日期：2026-06-11
> 定位：思考 gamecore-zone 如何從單一區域沙盒，接回更大尺度的 Region/World 層，尤其是時間、事件與狀態邊界。

---

## 1. 核心問題

Zone 層現在是「局部戰術場」：地圖、actor、scheduler、Action 全在一個 `TurnWorld` 裡推進。
Region/World 層未來會處理更大尺度的事：地點生成、隊伍旅行、派系狀態、長期事件、存檔索引。

要避免兩個極端：

- Zone 完全自閉，打完才吐一個勝負結果，失去跨層連動。
- World 直接操控 Zone registry，邊界破掉，測試與存檔都難維護。

## 2. 建議邊界：Zone 是短生命戰術實例

Region/World 只負責建立與回收 Zone：

```text
World/Region
  -> ZoneLaunchRequest { seed, biome, threat_level, party_snapshot, encounter_id }
  <- ZoneResult { survivors, loot_delta, time_spent, event_tags, world_mutations }
```

Zone 內部仍保有完整真相：scheduler、map、actor、items。跨層只交換「啟動參數」與「結算結果」。

## 3. 時間轉換（優先序：設計先行）

三種時間概念需要明確換算：

| 層 | 單位 | 用途 |
|---|---|---|
| Zone | tick / actor turn | 戰鬥、施法、DoT、能量 |
| Region | minute / watch | 旅行、遭遇、短期事件 |
| World | day / season | 經濟、派系、長期狀態 |

建議先採簡單規則：Zone 結束時由 `ZoneResult.time_spent_ticks` 換算 Region 分鐘，例如
`1000 zone ticks = 1 region minute`。換算係數放在 Region config，不放在 core combat。

## 4. 事件上拋

ZoneEvent 不應全部上拋 World。只把有長期意義的事件轉成 tag：

- `hero_down`
- `boss_killed`
- `artifact_found`
- `faction_unit_killed`
- `zone_abandoned`

轉換點在 GDExtension 或未來的 `ZoneSession` wrapper，不在 `zone_core` 原語內。core 不認識派系與劇情。

## 5. 存檔策略

- 短期：只支援「Zone 內存檔」與「Zone 結束回 World」兩種狀態。
- 中期：World save 包一個 optional active-zone blob。
- 長期：active-zone save header 記錄 zone schema version、world encounter id、party binding id。

## 6. 不做的事

- 不讓 Region scheduler 與 Zone scheduler 共用同一套 tick；尺度不同，硬合會拖累兩邊。
- 不在 Zone actor 上直接掛 World faction/component；改用啟動時注入 prototype/tag。
- 不急著做無縫大地圖。先把「進入戰術區域 → 結算回大地圖」做好。
