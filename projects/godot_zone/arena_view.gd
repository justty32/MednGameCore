extends Node2D

# 戰棋競技場的繪製層。與 dungeon_view.gd 平行：
# 唯一決定「畫哪個圖塊、什麼顏色」的地方，資料全部來自 TacticsBattle。
#
# 疊法（後畫的蓋在上面）：地形 → 可移動格 → 可攻擊目標框 → 單位 → 血條

const TILE := TileAtlas.TILE
const TF_WALKABLE := 1

const C_FLOOR_BG := Color(0.15, 0.15, 0.17)
const C_FLOOR_FG := Color(0.32, 0.32, 0.35)
const C_WALL_BG  := Color(0.24, 0.22, 0.20)
const C_WALL_FG  := Color(0.48, 0.45, 0.41)

const C_MOVE     := Color(0.30, 0.60, 1.00, 0.28)   # 可移動格：半透明藍
const C_TARGET   := Color(1.00, 0.30, 0.25, 0.95)   # 可攻擊目標：紅框
const C_CURRENT  := Color(1.00, 0.90, 0.35, 0.95)   # 當前單位：黃框

const C_PLAYER := Color(0.62, 0.86, 1.00)
const C_ENEMY  := Color(0.92, 0.42, 0.38)

var battle: TacticsBattle

func _ready() -> void:
	texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST

func _unit_tile(u: Dictionary) -> Vector2i:
	if int(u["attack_range"]) > 1:
		return TileAtlas.ARCHER
	if int(u["speed"]) >= 120:
		return TileAtlas.SCOUT
	return TileAtlas.WARRIOR

func _outline(x: int, y: int, c: Color, w: float = 1.5) -> void:
	draw_rect(TileAtlas.cell(x, y), c, false, w)

func _health_bar(u: Dictionary) -> void:
	var hp := int(u["hp"])
	var max_hp: int = max(int(u["max_hp"]), 1)
	if hp >= max_hp:
		return   # 滿血不畫，畫面才不會被血條淹沒
	var x := int(u["x"]) * TILE
	var y := int(u["y"]) * TILE + TILE - 2
	var frac := float(hp) / float(max_hp)
	draw_rect(Rect2(x + 1, y, TILE - 2, 2), Color(0.1, 0.1, 0.1, 0.9))
	var col := Color(0.35, 0.8, 0.4) if frac > 0.34 else Color(0.85, 0.25, 0.2)
	draw_rect(Rect2(x + 1, y, (TILE - 2) * frac, 2), col)

func _draw() -> void:
	if battle == null:
		return
	var w: int = battle.get_map_width()
	var h: int = battle.get_map_height()
	if w <= 0 or h <= 0:
		return

	# ---- 地形 ----
	var flags: PackedByteArray = battle.get_tile_flags()
	for y in h:
		for x in w:
			var walkable := (flags[y * w + x] & TF_WALKABLE) != 0
			draw_rect(TileAtlas.cell(x, y), C_FLOOR_BG if walkable else C_WALL_BG)
			TileAtlas.blit(self, x, y, TileAtlas.FLOOR if walkable else TileAtlas.WALL,
						   C_FLOOR_FG if walkable else C_WALL_FG)

	# ---- 可移動格（只有輪到玩家、且還沒移動時才有）----
	for t in battle.get_move_tiles():
		draw_rect(TileAtlas.cell(int(t.x), int(t.y)), C_MOVE)

	# ---- 單位 ----
	var cur := battle.current_unit()
	var targets: Array = battle.get_attack_targets()
	for u in battle.get_units():
		var ux := int(u["x"])
		var uy := int(u["y"])
		var col: Color = C_PLAYER if int(u["team"]) == 0 else C_ENEMY
		TileAtlas.blit(self, ux, uy, _unit_tile(u), col)
		_health_bar(u)
		if int(u["id"]) == cur:
			_outline(ux, uy, C_CURRENT)
		elif targets.has(int(u["id"])):
			_outline(ux, uy, C_TARGET)
