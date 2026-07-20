class_name TileAtlas
extends RefCounted

# 圖集座標的唯一真相來源，地城與戰棋兩個場景共用。
#
# Kenney 1-Bit Pack（CC0）：monochrome-transparent_packed.png
# 16×16 圖塊、49 欄、無間距。單色透明 → 任何圖塊都能用 modulate 上色。

const SHEET := preload("res://assets/tilesheet.png")
const TILE := 16

# 地形
const FLOOR := Vector2i(6, 0)
const WALL  := Vector2i(10, 17)
const STAIR := Vector2i(3, 10)

# 地城
const HERO   := Vector2i(25, 0)
const RAT    := Vector2i(31, 7)
const POTION := Vector2i(34, 13)

# 戰棋單位
const WARRIOR := Vector2i(30, 1)
const ARCHER  := Vector2i(26, 1)
const SCOUT   := Vector2i(25, 0)

static func src(t: Vector2i) -> Rect2:
	return Rect2(t.x * TILE, t.y * TILE, TILE, TILE)

static func cell(x: int, y: int) -> Rect2:
	return Rect2(x * TILE, y * TILE, TILE, TILE)

# 在 canvas item 的 _draw() 裡把某個圖塊畫到格子 (x,y)，以 c 上色。
static func blit(ci: CanvasItem, x: int, y: int, t: Vector2i, c: Color) -> void:
	ci.draw_texture_rect_region(SHEET, cell(x, y), src(t), c)
