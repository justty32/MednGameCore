extends Node2D

# 戰棋場景：滑鼠操作 → TacticsBattle 指令 → signal → 刷新。
#
# 一個單位的回合是「移動一次 ＋ 攻擊一次」，攻擊後結束回合。
# 規則全在 C++ core（zone::tactics::Battle）；本檔只負責畫面與輸入。
#
# 操作：
#   點自己的單位所在格以外的藍色格 → 移動
#   點紅框敵人 → 攻擊（攻擊後回合結束）
#   空白鍵 → 待機／結束回合
#   R 重開一場、Esc 回主選單

@onready var view:       Node2D        = $ArenaView
@onready var camera:     Camera2D      = $Camera2D
@onready var info:       Label         = $HUD/UnitInfo
@onready var order_box:  Label         = $HUD/TurnOrder
@onready var msg_log:    RichTextLabel = $HUD/MessageLog
@onready var overlay:    ColorRect     = $HUD/Overlay
@onready var over_title: Label         = $HUD/Overlay/Center/Box/Title
@onready var over_hint:  Label         = $HUD/Overlay/Center/Box/Hint

const TILE := TileAtlas.TILE
const ZOOM := 2.0
const MAX_MESSAGES := 6

var battle: TacticsBattle
var _messages: Array[String] = []

# id → 單位快照。致命一擊時目標已被 core 從 registry 移除，
# get_units() 查不到它，訊息就會變成「單位」。留一份快照才叫得出名字。
var _seen_units := {}

func _ready() -> void:
	battle = TacticsBattle.new()
	battle.name = "TacticsBattle"
	add_child(battle)
	view.battle = battle

	# 先接信號再開局：開局的 pump() 會跑敵方 AI 並發事件
	battle.turn_started.connect(_on_turn_started)
	battle.unit_moved.connect(_on_unit_moved)
	battle.unit_attacked.connect(_on_unit_attacked)
	battle.unit_died.connect(_on_unit_died)
	battle.battle_over.connect(_on_battle_over)

	overlay.visible = false
	_start(randi())

func _start(seed_val: int) -> void:
	_messages.clear()
	overlay.visible = false
	_say("戰鬥開始。點藍色格移動，點紅框敵人攻擊，空白鍵待機。", "#c8c8c8")
	_seen_units.clear()
	battle.start_battle(seed_val)
	camera.position = Vector2(battle.get_map_width(), battle.get_map_height()) * TILE * 0.5
	_refresh()

# ---- 輸入 -------------------------------------------------------------------

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_ESCAPE:
				get_tree().change_scene_to_file("res://main_menu.tscn")
			KEY_R:
				_start(randi())
			KEY_SPACE:
				if battle.is_waiting_player():
					battle.command_wait()
					_refresh()
		return

	if event is InputEventMouseButton and event.pressed \
			and event.button_index == MOUSE_BUTTON_LEFT:
		_click(view.get_local_mouse_position())

func _click(local: Vector2) -> void:
	if not battle.is_waiting_player():
		return
	var cell := Vector2i(int(floor(local.x / TILE)), int(floor(local.y / TILE)))

	# 先看是不是可攻擊的敵人（攻擊優先於移動：同一格不會兩者皆是）
	for uid in battle.get_attack_targets():
		var u := _unit(uid)
		if not u.is_empty() and int(u["x"]) == cell.x and int(u["y"]) == cell.y:
			battle.command_attack(uid)
			_refresh()
			return

	# 再看是不是可移動的格
	for t in battle.get_move_tiles():
		if int(t.x) == cell.x and int(t.y) == cell.y:
			if battle.command_move(cell.x, cell.y):
				_refresh()
			return

func _unit(uid: int) -> Dictionary:
	for u in battle.get_units():
		if int(u["id"]) == uid:
			return u
	return _seen_units.get(uid, {})   # 已陣亡：退回快照

# ---- 事件 → 訊息 ------------------------------------------------------------

func _name_of(uid: int) -> String:
	var u := _unit(uid)
	if u.is_empty():
		return "單位"
	var side: String = "我方" if int(u["team"]) == 0 else "敵方"
	return side + _role_name(u)

func _role_name(u: Dictionary) -> String:
	if int(u["attack_range"]) > 1:
		return "弓手"
	if int(u["speed"]) >= 120:
		return "斥候"
	return "戰士"

func _on_turn_started(uid: int) -> void:
	var u := _unit(uid)
	if not u.is_empty() and int(u["team"]) == 0:
		_say("輪到 %s 行動。" % _name_of(uid), "#8fd0ff")

func _on_unit_moved(_uid: int, _fx: int, _fy: int) -> void:
	pass   # 移動不需要文字訊息，畫面已經說明一切

func _on_unit_attacked(a: int, b: int, damage: int) -> void:
	var color := "#ffd479" if _team_of(a) == 0 else "#ff6b6b"
	_say("%s 攻擊 %s，造成 %d 傷害。" % [_name_of(a), _name_of(b), damage], color)

func _on_unit_died(uid: int) -> void:
	_say("%s 倒下了。" % _name_of(uid), "#9ae66e")
	_refresh()

func _on_battle_over(winning_team: int) -> void:
	if winning_team == 0:
		_show_overlay("勝利", "敵方全滅。\n按 R 再打一場，Esc 回選單", Color(0.2, 0.6, 0.35))
	else:
		_show_overlay("敗北", "我方全滅。\n按 R 再打一場，Esc 回選單", Color(0.7, 0.15, 0.15))

func _team_of(uid: int) -> int:
	var u := _unit(uid)
	return int(u["team"]) if not u.is_empty() else -1

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
	for u in battle.get_units():
		_seen_units[int(u["id"])] = u

	var cur := battle.current_unit()
	var u := _unit(cur)
	if u.is_empty():
		info.text = ""
	else:
		var state := ""
		if battle.is_waiting_player():
			state = "（可移動）" if battle.can_move() else ("（可攻擊）" if battle.can_act() else "")
		info.text = "%s  HP %d/%d   攻擊 %d   移動 %d   射程 %d   速度 %d %s" % [
			_name_of(cur), int(u["hp"]), int(u["max_hp"]), int(u["attack"]),
			int(u["move_range"]), int(u["attack_range"]), int(u["speed"]), state]

	var names: Array[String] = []
	for uid in battle.get_turn_order(6):
		names.append(_name_of(uid))
	order_box.text = "出手順序：" + " → ".join(names)
