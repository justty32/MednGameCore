extends Node2D

# 主場景：輸入 → 核心回合推進 → signal → 刷新畫面與 HUD。
#
# 最傳統回合制：按一鍵 = 英雄行動一次 + 所有 NPC 行動一輪
# （player_move/player_wait 內部跑完整輪，跑完才回來）。
#
# 呈現分工：
#   dungeon_view.gd  地形與實體的圖塊繪製（唯一決定 tile / 顏色的地方）
#   本檔             HUD、訊息列、結局 overlay、按鍵
#
# 按鍵：WASD / 方向鍵 / 數字鍵移動，. 或 KP5 等待，R 重開，
#       F5 存檔，F9 讀檔，Tab 開關除錯面板，K 清 trace。

@onready var view:      Node2D          = $DungeonView
@onready var camera:    Camera2D        = $Camera2D
@onready var stats:     Label           = $HUD/Stats
@onready var hp_bar:    ProgressBar     = $HUD/HPBar
@onready var hp_text:   Label           = $HUD/HPBar/HPText
@onready var msg_log:   RichTextLabel   = $HUD/MessageLog
@onready var debug_box: Label           = $HUD/DebugPanel
@onready var overlay:   ColorRect       = $HUD/Overlay
@onready var over_title:Label           = $HUD/Overlay/Center/Box/Title
@onready var over_hint: Label           = $HUD/Overlay/Center/Box/Hint

const CELL_PX := 16
const SAVE_PATH := "user://zone_save.bin"
const MAX_MESSAGES := 7

# 長按連續移動：按住超過 REPEAT_DELAY 後，每 REPEAT_RATE 秒自動走一格。
# 一格＝一整輪（英雄＋所有 NPC），所以速率別調太快，否則來不及看訊息。
const REPEAT_DELAY := 0.5
const REPEAT_RATE  := 0.08

# 按鍵 → 方向。數字鍵盤含斜角；WASD / 方向鍵只有四方。
const MOVE_KEYS := {
	KEY_W: Vector2i(0, -1), KEY_UP:    Vector2i(0, -1), KEY_KP_8: Vector2i(0, -1),
	KEY_S: Vector2i(0,  1), KEY_DOWN:  Vector2i(0,  1), KEY_KP_2: Vector2i(0,  1),
	KEY_D: Vector2i(1,  0), KEY_RIGHT: Vector2i(1,  0), KEY_KP_6: Vector2i(1,  0),
	KEY_A: Vector2i(-1, 0), KEY_LEFT:  Vector2i(-1, 0), KEY_KP_4: Vector2i(-1, 0),
	KEY_KP_7: Vector2i(-1, -1), KEY_KP_9: Vector2i(1, -1),
	KEY_KP_1: Vector2i(-1,  1), KEY_KP_3: Vector2i(1,  1),
}

var world: ZoneWorld
var _messages: Array[String] = []
var _hp_fill := StyleBoxFlat.new()

# 連續移動狀態
var _held := Vector2i.ZERO   # 目前按住的方向
var _hold_t := 0.0
var _repeating := false
var _blocked := false        # 被事件打斷；放開按鍵才解除
var _seen_npcs := 0          # 上一幀視野內的怪數：變多就停止連跑

func _ready() -> void:
	world = ZoneWorld.new()
	world.name = "ZoneWorld"
	add_child(world)
	view.world = world

	_hp_fill.corner_radius_top_left = 2
	_hp_fill.corner_radius_top_right = 2
	_hp_fill.corner_radius_bottom_left = 2
	_hp_fill.corner_radius_bottom_right = 2
	hp_bar.add_theme_stylebox_override("fill", _hp_fill)

	world.world_changed.connect(_on_world_changed)
	world.hero_bumped_wall.connect(_on_hero_bumped_wall)
	world.hero_attacked.connect(_on_hero_attacked)
	world.hero_hurt.connect(_on_hero_hurt)
	world.npc_died.connect(_on_npc_died)
	world.item_picked_up.connect(_on_item_picked_up)
	world.floor_changed.connect(_on_floor_changed)
	world.hero_grew.connect(_on_hero_grew)
	world.game_over.connect(_on_game_over)
	world.game_won.connect(_on_game_won)

	debug_box.visible = false
	overlay.visible = false
	_say("你走進了地城。目標：抵達第 %d 層的樓梯。" % world.get_win_floor(), "#c8c8c8")
	_refresh()

# ---- 輸入 -------------------------------------------------------------------

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed and not event.echo):
		return

	# 存讀檔 / 重開 / 除錯：結局後仍可用
	match event.keycode:
		KEY_ESCAPE: get_tree().change_scene_to_file("res://main_menu.tscn"); return
		KEY_F5:  _do_save();    return
		KEY_F9:  _do_load();    return
		KEY_R:   _do_restart(); return
		KEY_TAB: _toggle_debug(); return
		KEY_K:   world.clear_debug_log(); _refresh(); return

	if world.is_game_over() or world.is_game_won():
		return

	if MOVE_KEYS.has(event.keycode):
		var d: Vector2i = MOVE_KEYS[event.keycode]
		world.player_move(d.x, d.y)   # 第一步立刻走；後續由 _process 的長按邏輯接手
		_held = d
		_hold_t = 0.0
		_repeating = false
	elif event.keycode == KEY_PERIOD or event.keycode == KEY_KP_5:
		world.player_wait()

# ---- 長按連續移動 ------------------------------------------------------------

func _pressed_dir() -> Vector2i:
	for k in MOVE_KEYS:
		if Input.is_key_pressed(k):
			return MOVE_KEYS[k]
	return Vector2i.ZERO

# 任何值得看一眼的事發生 → 停止連跑。放開按鍵才會重新允許。
# 沒有這道閘門，按住方向鍵會直接跑進怪堆裡送死。
func _interrupt_repeat() -> void:
	if _repeating or _hold_t > 0.0:
		_blocked = true
	_repeating = false
	_hold_t = 0.0

func _process(delta: float) -> void:
	var dir := _pressed_dir()

	if dir == Vector2i.ZERO:
		_held = Vector2i.ZERO
		_hold_t = 0.0
		_repeating = false
		_blocked = false      # 放開按鍵：解除打斷
		return

	if _blocked or world.is_game_over() or world.is_game_won():
		return

	if dir != _held:          # 換方向：重新計時，這一格由 _unhandled_input 走
		_held = dir
		_hold_t = 0.0
		_repeating = false
		return

	_hold_t += delta
	if not _repeating:
		if _hold_t >= REPEAT_DELAY:
			_repeating = true
			_hold_t = 0.0
	elif _hold_t >= REPEAT_RATE:
		_hold_t -= REPEAT_RATE
		world.player_move(dir.x, dir.y)

# ---- 核心事件 → 訊息列 ------------------------------------------------------

func _on_world_changed() -> void:
	_refresh()

func _on_hero_bumped_wall() -> void:
	_interrupt_repeat()
	_say("前面是牆。", "#8a8a8a")

func _on_hero_attacked(damage: int, target_hp_left: int) -> void:
	_interrupt_repeat()
	_say("你擊中老鼠，造成 %d 傷害（剩 %d HP）。" % [damage, target_hp_left], "#ffd479")

func _on_hero_hurt(damage: int, hp_left: int) -> void:
	_interrupt_repeat()
	_say("老鼠咬了你，受到 %d 傷害（剩 %d HP）！" % [damage, hp_left], "#ff6b6b")

func _on_npc_died() -> void:
	_say("老鼠倒下了。", "#9ae66e")

func _on_item_picked_up(item_name: String, heal_amount: int) -> void:
	_interrupt_repeat()
	if heal_amount > 0:
		_say("你喝下%s，回復 %d HP。" % [_item_label(item_name), heal_amount], "#5fd3a0")
	else:
		_say("你撿起%s，但已經是滿血了。" % _item_label(item_name), "#5fd3a0")

func _on_hero_grew(max_hp: int, attack: int) -> void:
	_say("你感覺更強壯了（最大 HP %d，攻擊 %d）。" % [max_hp, attack], "#7fb4ff")

func _on_floor_changed(floor_num: int) -> void:
	_interrupt_repeat()
	_say("你走下樓梯，來到第 %d 層。" % floor_num, "#ffd479")

func _on_game_over() -> void:
	_interrupt_repeat()
	_say("你倒下了……", "#ff6b6b")
	_show_overlay("你死了", "在第 %d 層撐了 %d 回合。\n按 R 重新開始" %
		[world.get_current_floor(), world.get_turn_count()], Color(0.75, 0.15, 0.15))

func _on_game_won(floor_num: int) -> void:
	_say("你走下第 %d 層的樓梯，逃出了地城！" % floor_num, "#ffd479")
	_show_overlay("你逃出來了", "花了 %d 回合，帶著 %d/%d HP 生還。\n按 R 再來一次" %
		[world.get_turn_count(), world.get_hero_hp(), world.get_hero_max_hp()],
		Color(0.15, 0.55, 0.30))

func _item_label(item_name: String) -> String:
	return "治療藥水" if item_name == "health_potion" else item_name

# ---- 動作 -------------------------------------------------------------------

func _toggle_debug() -> void:
	var on := not debug_box.visible
	debug_box.visible = on
	world.set_trace_enabled(on)   # trace 只有面板開著時才有意義，關著就別浪費
	if not on:
		world.clear_debug_log()
	_refresh()

func _do_restart() -> void:
	world.restart()
	_messages.clear()
	_seen_npcs = 0
	_interrupt_repeat()
	overlay.visible = false
	_say("新的一場（種子 %d）。" % world.get_seed(), "#c8c8c8")
	_refresh()

func _do_save() -> void:
	var real_path := ProjectSettings.globalize_path(SAVE_PATH)
	if world.save_game(real_path):
		_say("已存檔（第 %d 層，第 %d 回合）。" % [world.get_current_floor(), world.get_turn_count()], "#c8c8c8")
	else:
		_say("存檔失敗。", "#ff6b6b")
	_refresh()

func _do_load() -> void:
	var real_path := ProjectSettings.globalize_path(SAVE_PATH)
	if not world.has_save_game(real_path):
		_say("找不到存檔，先按 F5。", "#ff6b6b")
		_refresh()
		return
	if world.load_game(real_path):
		overlay.visible = false
		_seen_npcs = 0
		_interrupt_repeat()
		_say("已讀檔（第 %d 層，第 %d 回合）。" % [world.get_current_floor(), world.get_turn_count()], "#c8c8c8")
	else:
		_say("讀檔失敗。", "#ff6b6b")
	_refresh()

# ---- 呈現 -------------------------------------------------------------------

func _say(text: String, color: String) -> void:
	_messages.append("[color=%s]%s[/color]" % [color, text])
	if _messages.size() > MAX_MESSAGES:
		_messages = _messages.slice(_messages.size() - MAX_MESSAGES)
	msg_log.text = "\n".join(_messages)

func _show_overlay(title: String, hint: String, tint: Color) -> void:
	over_title.text = title
	over_title.add_theme_color_override("font_color", tint.lightened(0.45))
	over_hint.text = hint
	overlay.visible = true

func _refresh() -> void:
	view.queue_redraw()
	camera.position = Vector2(world.get_hero_x() + 0.5, world.get_hero_y() + 0.5) * CELL_PX

	# 有新的怪進入視野 → 停止連跑（傳統 roguelike 的 "disturb"）
	var npcs := 0
	for e in world.get_visible_entities():
		if e["kind"] == "npc":
			npcs += 1
	if npcs > _seen_npcs:
		_interrupt_repeat()
	_seen_npcs = npcs

	var hp := world.get_hero_hp()
	var max_hp := world.get_hero_max_hp()
	hp_bar.max_value = max(max_hp, 1)
	hp_bar.value = hp
	hp_text.text = "HP %d / %d" % [hp, max_hp]

	# 血量低於三分之一轉紅，這是玩家最需要一眼看到的資訊
	var frac := float(hp) / float(max(max_hp, 1))
	_hp_fill.bg_color = Color(0.30, 0.70, 0.35) if frac > 0.34 else Color(0.80, 0.22, 0.20)

	stats.text = "第 %d / %d 層     攻擊 %d     回合 %d     敵人 %d" % [
		world.get_current_floor(), world.get_win_floor(), world.get_hero_attack(),
		world.get_turn_count(), world.get_npc_count()]

	if debug_box.visible:
		var t := world.get_debug_text()
		var trace: String = world.get_debug_log()
		if trace != "":
			t += "\n—— trace ——\n" + trace
		debug_box.text = t
