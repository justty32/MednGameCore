extends Node2D

# 地城繪製層。
#
# 一次 _draw() 畫完整張地圖：地形逐格、實體疊在上面。
# 每格自己決定 modulate，霧中戰爭＝同一張圖塊調暗，不需要 shader。
# 資料全部來自 ZoneWorld（get_tile_flags / get_visible_entities），
# 本檔是唯一決定「哪個 tile、什麼顏色」的地方。
#
# 圖集：Kenney 1-Bit Pack（CC0），monochrome-transparent_packed.png
#       16×16 圖塊、49 欄、無間距。單色 → 可用 modulate 任意上色。

const TILE := TileAtlas.TILE

# get_tile_flags() 的位元（與 zone_world_gd.cpp 的 TF_* 對應）
const TILE_WALKABLE := 1
const TILE_STAIR    := 2
const TILE_VISIBLE  := 4
const TILE_EXPLORED := 8

# 地形每格先鋪一層實心底色，再把圖塊當「紋理細節」疊上去。
# 圖集是單色透明的，直接畫圖塊的話地板只剩幾顆碎石點，讀不出哪裡能走。
const C_FLOOR_BG := Color(0.14, 0.12, 0.10)
const C_FLOOR_FG := Color(0.34, 0.29, 0.22)
const C_WALL_BG  := Color(0.22, 0.21, 0.20)
const C_WALL_FG  := Color(0.46, 0.44, 0.41)
const C_STAIR := Color(1.00, 0.84, 0.30)
const C_HERO  := Color(1.00, 0.96, 0.78)
const C_NPC   := Color(0.86, 0.30, 0.27)
const C_ITEM  := Color(0.36, 0.85, 0.50)

# 記憶中（探索過但目前看不見）的格子：調暗並偏冷色，和眼前的視野拉開對比。
# 別調太暗——霧中戰爭的重點是「記得地圖長怎樣」，不是把它抹黑。
const MEMORY_DIM := 0.45
const MEMORY_TINT := Color(0.62, 0.68, 0.95)

var world: ZoneWorld

func _ready() -> void:
	texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST

func _cell(x: int, y: int) -> Rect2:
	return TileAtlas.cell(x, y)

func _blit(x: int, y: int, t: Vector2i, c: Color) -> void:
	TileAtlas.blit(self, x, y, t, c)

func _remembered(c: Color) -> Color:
	return Color(c.r * MEMORY_TINT.r, c.g * MEMORY_TINT.g, c.b * MEMORY_TINT.b) * MEMORY_DIM

func _draw() -> void:
	if world == null:
		return
	var w: int = world.get_map_width()
	var h: int = world.get_map_height()
	if w <= 0 or h <= 0:
		return

	# ---- 地形 ----
	var flags: PackedByteArray = world.get_tile_flags()
	for y in h:
		for x in w:
			var f := flags[y * w + x]
			if f & TILE_EXPLORED == 0:
				continue  # 從未探索：留黑
			var visible := (f & TILE_VISIBLE) != 0
			var walkable := (f & TILE_WALKABLE) != 0

			var bg := C_FLOOR_BG if walkable else C_WALL_BG
			var fg := C_FLOOR_FG if walkable else C_WALL_FG
			if not visible:
				bg = _remembered(bg)
				fg = _remembered(fg)
			draw_rect(_cell(x, y), bg)
			_blit(x, y, TileAtlas.FLOOR if walkable else TileAtlas.WALL, fg)

			# 樓梯疊在地板上；沒看見時也記得它在哪，這是玩家的目標
			if f & TILE_STAIR:
				_blit(x, y, TileAtlas.STAIR, C_STAIR if visible else _remembered(C_STAIR))

	# ---- 實體（只有視野內的；繪製序由核心決定，英雄最後）----
	for e in world.get_visible_entities():
		var ex: int = e["x"]
		var ey: int = e["y"]
		match e["kind"]:
			"item": _blit(ex, ey, TileAtlas.POTION, C_ITEM)
			"npc":  _blit(ex, ey, TileAtlas.RAT, C_NPC)
			"hero": _blit(ex, ey, TileAtlas.HERO, C_HERO)
