# 渲染升級：TileMapLayer / Sprite2D / 攝影機 / FOV

日期：2026-06-21
定位：把「`generate_map_image()` 整圖重繪」升級為節點化渲染——地形 TileMapLayer、actor 獨立 Sprite2D、Camera2D 平滑跟隨、FOV 用調變層表現。

> 狀態更新（2026-06-21）：本主題在回合系統重構後**大致仍有效**——`generate_map_image()` 與 FOV（`map.is_visible/is_explored`）皆保留於現況。本次僅修正過時引用（移除已不存在的 `is_caster`／能量欄位、`step_scheduler` 觸發路徑等），渲染升級設計本身不受重構影響。

---

## 1. 為什麼要換（現況問題）

現況（src/gbind/zone_world_gd.cpp `generate_map_image` + map_view.gd `_refresh_display`）：
- 每次 `world_changed` 在 C++ 端 `fill_rect` 整張 60×40×16px 圖、跨 GDExtension 邊界傳 `Image`、再 `ImageTexture.create_from_image`——**每回合一次完整 CPU 紋理上傳**。
- actor 是圖裡的色塊，**無法獨立動畫**（補間移動、閃爍都做不到）→ 這是 02 事件動畫化的硬前提。
- 攝影機 `camera.position` 直接 snap 到英雄格，無平滑。

## 2. 目標場景結構（map_view.tscn 演進）

```
MapView (Node2D)
├── ZoneWorld（程式碼 add_child，現況不變）
├── TerrainLayer  (TileMapLayer)   ← 地形：地板/牆/樓梯
├── FogLayer      (TileMapLayer)   ← FOV：黑/半暗 遮罩 tile
├── Actors        (Node2D)         ← 每 actor 一個 Sprite2D（池化）
├── Items         (Node2D)         ← 道具 Sprite2D
├── FxLayer       (Node2D)         ← 傷害數字/特效（見 02）
├── Camera2D
└── CanvasLayer (HUD，見 03)
```

Godot 4.3+ 用 `TileMapLayer`（單層節點，舊 `TileMap` 已 deprecated）。每層一個節點，z 序即樹序，乾淨。

## 3. 想法清單

### 3.1 地形 TileMapLayer（優先序：高）
依賴：src/gbind/zone_world_gd.h 需新增 tile 查詢（見 §4）；godot_zone/map_view.tscn。

- 美術未到位前，先用 **程式生成 TileSet**：`Image` 畫 4 個 16×16 色塊
  （floor/wall/stair/unknown）→ `ImageTexture` → `TileSetAtlasSource.create_tile()`，
  與現有色票一致，零美術成本即可切換架構。
- 載入樓層時一次 `set_cell(Vector2i(x,y), source_id, atlas_coords)` 鋪滿；
  之後**只在 `floor_changed` 時重鋪**（地形不會變）。
- 牆面之後可升級 terrain set（autotile 接邊），TileSet 內建支援，前端程式不用改。

### 3.2 Actor / Item 改 Sprite2D 池（優先序：高）
依賴：需要穩定 actor id 與座標的結構化查詢（§4 `get_actors()`）；02 的補間建立在此之上。

- `Actors` 節點下維護 `Dictionary[int eid → Sprite2D]`。每次同步：
  新 eid → 從池取出/新建；消失的 eid → 回收（死亡動畫播完再收，見 02）。
- sprite 位置 = `Vector2(x, y) * CELL_PX`（與現有 CELL_PX=16 一致）。
- 純色階段用 `Sprite2D` + 16×16 白圖 + `modulate`（hero 黃/NPC 紅/item 綠），
  之後換貼圖只動 texture。
- 外觀區分：現況只有 hero（ControllerKind::Player）與追擊 NPC（ControllerKind::Self）。
  若未來引入施法者等 actor 類型，`get_actors()` 再補對應欄位即可
  （現已無 caster/NpcAiComponent，那是重構前的概念）。

### 3.3 Camera2D 平滑跟隨（優先序：高，便宜）
依賴：godot_zone/map_view.gd `_refresh_display`。

- `camera.position_smoothing_enabled = true`、`position_smoothing_speed = 8.0`，
  目標仍設英雄格中心——一行屬性就把 snap 變平滑，**先做這個**。
- `limit_left/top/right/bottom` 設為地圖像素邊界（`get_map_width()*CELL_PX`），
  邊緣不再露黑底。
- 現況單一玩家角色：攝影機目標固定為英雄格（`get_hero_x/y()`）。
  若未來引入「多名可操控角色」擴充（04），攝影機目標改為「目前等待輸入的角色」，
  切換時用 `create_tween().tween_property(camera, "position", target, 0.25)`
  做一次性飛行，平時恢復 smoothing 跟隨。
- 之後加 `camera.zoom` 滾輪縮放（clamp 0.5～2.0）。

### 3.4 FOV 視覺化（優先序：中）
依賴：core 已有 `map.is_visible/is_explored`（generate_map_image 已在用）；需匯出（§4）。

- **FogLayer 方案**（推薦起步）：第二層 TileMapLayer，三態 tile——
  不可見=全黑、已探索未見=半透明黑（`Color(0,0,0,0.6)` 的 tile）、可見=不鋪。
  每次 FOV 變更只更新「狀態有變的格子」：core 端 `recompute_fov` 後
  把 dirty 格清單塞進事件或提供 `get_fov_grid()`（PackedByteArray，0/1/2 三態）
  讓 GDScript diff。60×40=2400 格全量 diff 在 GDScript 也只是微秒級，先全量再說。
- 進階（低優先）：FogLayer 換成一張低解析 `ImageTexture`（每格 1px）+
  `canvas_item` shader 做柔邊光照漸層，roguelike 質感大增，但等貼圖時代再說。
- actor/item 可見性：沿用現有規則（不可見不畫），由 `get_actors()` 只回傳可見者，
  或回傳全部 + `visible` 欄位由前端裁決（後者利於「殘影：最後目擊位置」這種進階表現）。

## 4. 需要 core 配合的新介面（zone_world_gd.h）

優先序：高（階段 0/1 的地基）。

```
// 結構化匯出（取代逐 getter 拼湊）
Array get_actors() const;      // [{eid,x,y,hp,max_hp,is_hero}]（未來擴充再補欄位）
Array get_items()  const;      // [{eid,x,y,kind}]
PackedByteArray get_fov_grid() const;   // w*h，0=未知 1=已探索 2=可見
int get_tile(int x, int y) const;       // 0=wall 1=floor 2=stair（鋪 TileMap 用）
```

- Dictionary/Array 跨界有成本，但每回合一次、數十 actor，毫無壓力；
  比每回合傳一張 38KB Image 還便宜。
- `get_debug_text()` 保留不動（07 的 debug 面板還要用）。

## 5. 風險與保留

- `generate_map_image()` **不刪**：verify.gd（res://verify.gd 第 44 行起）靠它驗證，
  且 minimap（03）之後可直接拿它當小地圖紋理——意外的二次利用。
- 切換期間建議 map_view.gd 加 `const USE_TILEMAP := true` 開關，兩條渲染路徑並存一陣子。
