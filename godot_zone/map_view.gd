extends Node2D

# F3 地圖場景：輸入 → 核心回合推進 → Signal 刷新顯示。
# 最傳統回合制：按一鍵 = 英雄行動一次 + 所有 NPC 行動一輪（player_move/player_wait 內部跑完整輪）。
# F4 音效框架：hero_bumped_wall / hero_bumped_npc 信號 → AudioStreamPlayer。
# 戰鬥：hero 移動撞 NPC 即攻擊；NPC 鄰接攻擊 hero；game_over 偵測。
# 存讀檔：F5 存檔（user://zone_save.bin），F9 讀檔。
#   兩者皆可在 game_over 後操作（不受 _dead 限制）。
#
# 建議場景結構：
#   MapView (Node2D)          ← 此腳本
#   ├── Sprite2D              ← 地圖圖片
#   ├── Camera2D
#   └── CanvasLayer (layer=1)
#       └── InfoLabel (Label) ← 左上角 UI（hero 座標 + 回合數 + HP）

@onready var sprite: Sprite2D   = $Sprite2D
@onready var camera: Camera2D   = $Camera2D
@onready var info_label: Label  = $CanvasLayer/InfoLabel

var world: ZoneWorld
const CELL_PX := 16
const SAVE_PATH := "user://zone_save.bin"

var _dead := false  # game over 後鎖定移動輸入

# F4 音效播放器（load 實際 .ogg/.wav 後取消 stream 行的注釋即可）
var sfx_step:     AudioStreamPlayer
var sfx_wall:     AudioStreamPlayer
var sfx_bump_npc: AudioStreamPlayer

func _ready() -> void:
	world = ZoneWorld.new()
	world.name = "ZoneWorld"
	add_child(world)

	# 核心事件信號
	world.world_changed.connect(_on_world_changed)
	world.round_finished.connect(_on_round_finished)
	world.hero_bumped_wall.connect(_on_hero_bumped_wall)
	world.hero_bumped_npc.connect(_on_hero_bumped_npc)
	world.npc_died.connect(_on_npc_died)
	world.game_over.connect(_on_game_over)
	world.floor_changed.connect(_on_floor_changed)
	world.item_picked_up.connect(_on_item_picked_up)

	_setup_audio()

	_refresh_display()
	_refresh_ui()

	print("ready — WASD/方向鍵/數字鍵移動 .等待 R重開 F5存/F9讀 L開關trace K清log")

func _setup_audio() -> void:
	sfx_step     = AudioStreamPlayer.new()
	sfx_wall     = AudioStreamPlayer.new()
	sfx_bump_npc = AudioStreamPlayer.new()
	add_child(sfx_step)
	add_child(sfx_wall)
	add_child(sfx_bump_npc)
	# 換成真實音效時，取消以下注釋：
	# sfx_step.stream     = load("res://sounds/step.ogg")
	# sfx_wall.stream     = load("res://sounds/bump.ogg")
	# sfx_bump_npc.stream = load("res://sounds/hit.ogg")

func _unhandled_input(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed and not event.echo):
		return

	# 存讀檔 / debug 不受 _dead 限制（game over 後仍可存 / 讀 / 重開）
	match event.keycode:
		KEY_F5: _do_save(); return
		KEY_F9: _do_load(); return
		KEY_R:  _do_restart(); return
		KEY_L:
			var en := not world.get_trace_enabled()
			world.set_trace_enabled(en)
			print("— debug trace %s —" % ("ON" if en else "OFF")); return
		KEY_K:
			world.clear_debug_log(); return

	if _dead: return

	# 移動 / 等待：每一鍵推進完整一輪（英雄 + 所有 NPC）。
	match event.keycode:
		KEY_UP,    KEY_W, KEY_KP_8: _do_move( 0, -1)
		KEY_DOWN,  KEY_S, KEY_KP_2: _do_move( 0,  1)
		KEY_RIGHT, KEY_D, KEY_KP_6: _do_move( 1,  0)
		KEY_LEFT,  KEY_A, KEY_KP_4: _do_move(-1,  0)
		KEY_KP_7:                   _do_move(-1, -1)
		KEY_KP_9:                   _do_move( 1, -1)
		KEY_KP_1:                   _do_move(-1,  1)
		KEY_KP_3:                   _do_move( 1,  1)
		KEY_PERIOD, KEY_KP_5:       _do_wait()

# 玩家下指令：核心會落地英雄動作、跑完本輪所有 NPC，再經 signal 回來刷新。
func _do_move(dx: int, dy: int) -> void:
	world.player_move(dx, dy)

func _do_wait() -> void:
	world.player_wait()

func _on_world_changed() -> void:
	_refresh_display()
	_refresh_ui()
	if sfx_step.stream:
		sfx_step.play()

func _on_round_finished() -> void:
	# 本輪（英雄 + 全部 NPC）結束。回合制下無須自動續跑，等玩家下一個指令。
	_refresh_ui()

func _on_hero_bumped_wall() -> void:
	if sfx_wall.stream:
		sfx_wall.play()
	else:
		print("* 碰牆 *")

func _on_hero_bumped_npc(npc_id: String) -> void:
	if sfx_bump_npc.stream:
		sfx_bump_npc.play()
	else:
		print("* 攻擊 %s（未致死）*" % npc_id)

func _on_npc_died(npc_id: String) -> void:
	print("* %s 倒下了！*" % npc_id)

func _on_game_over() -> void:
	_dead = true
	print("★ GAME OVER ★  你被 NPC 擊倒了。")
	info_label.text += "\n[GAME OVER]"

func _on_floor_changed(floor_num: int) -> void:
	print("★ 下降至第 %d 層 ★" % floor_num)

func _on_item_picked_up(item_name: String, heal_amount: int) -> void:
	print("★ 拾取 %s（回復 %d HP）" % [item_name, heal_amount])

func _do_restart() -> void:
	_dead = false
	world.restart()
	print("— 重新開始 —")

func _do_save() -> void:
	var real_path := ProjectSettings.globalize_path(SAVE_PATH)
	if world.save_game(real_path):
		print("★ 存檔成功（F%d，第 %d 回合）" % [world.get_current_floor(), world.get_turn_count()])
		_refresh_ui()
	else:
		print("★ 存檔失敗（地圖或英雄未初始化）")

func _do_load() -> void:
	var real_path := ProjectSettings.globalize_path(SAVE_PATH)
	if not world.has_save_game(real_path):
		print("★ 找不到存檔，請先按 F5 存檔")
		return
	if world.load_game(real_path):
		_dead = false
		print("★ 讀檔成功（F%d，第 %d 回合）" % [world.get_current_floor(), world.get_turn_count()])
	else:
		print("★ 讀檔失敗")

func _refresh_display() -> void:
	var img := world.generate_map_image(CELL_PX)
	sprite.centered = false
	sprite.position = Vector2.ZERO
	sprite.texture  = ImageTexture.create_from_image(img)
	camera.position = Vector2(world.get_hero_x() + 0.5, world.get_hero_y() + 0.5) * CELL_PX

func _refresh_ui() -> void:
	# 詳細診斷 dump（世界 + 逐 actor 全狀態）+ 最近 trace log，供測試系統機制時觀察
	var t := world.get_debug_text()
	var log: String = world.get_debug_log()
	if log != "":
		t += "\n—— trace ——\n" + log
	info_label.text = t
