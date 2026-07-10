# 地圖與 Procgen 資料化

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：把 terrain、BSP 生成參數、樓層主題從 C++ 常數逐步搬到資料檔。**仍為有效未來方向**——地圖程序生成（BSP）在重構後完整保留。

---

## 1. 現況

地圖仍是**程序生成**：`map_gen.cpp` 用 BSP 切割房間（60×40 地洞），
`zone_world_gd.cpp` 的 `setup_map()` 在其上灑怪與道具（`health_potion`）、放下樓梯。
這些生成常數（地圖大小、房間切割、NPC cap、物品分布）仍寫死在 C++，
調整樓層節奏需要重編譯。
（2026-06-21 重構只動回合/動作系統，**未動地圖生成**；此文涉及的內容大致不受影響。）

## 2. TerrainDef（優先序：中）

```json
{
  "schema_version": 1,
  "terrains": [
    { "id": "floor", "walkable": true, "transparent": true },
    { "id": "wall", "walkable": false, "transparent": false },
    { "id": "water", "walkable": false, "transparent": true }
  ]
}
```

短期只映射到 `TileFlags`，不做複雜材質。

## 3. ProcgenParams（優先序：中）

```json
{
  "id": "default_dungeon",
  "width": 60,
  "height": 40,
  "room_min": 4,
  "room_max": 10,
  "split_iterations": 5,
  "monster_budget": 12,
  "item_budget": 4
}
```

由 `ZoneLaunchRequest` 指定 theme id，`map_gen` 只吃已驗證過的 params。

## 4. 樓層主題

樓層 theme 可引用：

- terrain set
- procgen params
- monster prototype table
- loot table
- ambient tags

這會成為 Region → Zone 的主要內容接口。

## 5. 不做的事

- 不做完整 map scripting。
- 不把 BSP 演算法本身資料化；先資料化參數。
- 不在 FOV / rendering 穩定前投入大量 tile 美術 metadata。
