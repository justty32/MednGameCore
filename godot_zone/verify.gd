extends SceneTree

# Headless 驗證腳本：godot --headless -s res://verify.gd
# 實際載入 libzone_gd.so，驗證 ZoneCore + ZoneWorld（最傳統回合制）端到端可用。

var world  # 在 _initialize 建立、加入 root；_ready 要到第一幀才派發，故檢查放 _process

func _initialize() -> void:
	# --- ZoneCore.version()（不依賴 _ready，可即時驗）---
	var core := ZoneCore.new()
	var v: String = core.version()
	print("zone core version: ", v)
	if v == "":
		push_error("version() 為空")

	# --- ZoneWorld：加入 root，_ready→setup_world 於下一幀觸發 ---
	world = ZoneWorld.new()
	world.name = "ZoneWorld"
	root.add_child(world)

func _process(_delta: float) -> bool:
	var failed := 0
	world.set_trace_enabled(false)  # 大量檢查時關閉 trace 噪音，最後再單獨驗

	var w: int = world.get_map_width()
	var h: int = world.get_map_height()
	print("map size: %dx%d  hero=(%d,%d) hp=%d/%d npc=%d floor=%d turn=%d" % [
		w, h, world.get_hero_x(), world.get_hero_y(),
		world.get_hero_hp(), world.get_hero_max_hp(),
		world.get_npc_count(), world.get_current_floor(), world.get_turn_count()])
	if w <= 0 or h <= 0:
		push_error("地圖尺寸異常"); failed += 1
	if world.get_hero_max_hp() <= 0:
		push_error("英雄 HP 未初始化"); failed += 1

	# --- generate_map_image 回傳有效 Image ---
	var img: Image = world.generate_map_image(8)
	if img == null or img.get_width() != w * 8 or img.get_height() != h * 8:
		push_error("generate_map_image 尺寸異常"); failed += 1
	else:
		print("map image: %dx%d px" % [img.get_width(), img.get_height()])

	# --- player_wait 推進完整一輪：turn_count 應遞增、回合不殘留 ---
	var t0: int = world.get_turn_count()
	world.player_wait()
	if world.get_turn_count() <= t0:
		push_error("player_wait 未推進回合"); failed += 1
	else:
		print("turn advanced: %d -> %d" % [t0, world.get_turn_count()])
	if world.round_in_progress():
		push_error("player_wait 後回合仍在進行（應已跑完整輪）"); failed += 1

	# --- player_move：往四方各試一次，至少有一次合法移動改變座標 ---
	var hx0: int = world.get_hero_x()
	var hy0: int = world.get_hero_y()
	var moved := false
	for d in [Vector2i(1,0), Vector2i(-1,0), Vector2i(0,1), Vector2i(0,-1)]:
		var px: int = world.get_hero_x()
		var py: int = world.get_hero_y()
		world.player_move(d.x, d.y)
		if world.get_hero_x() != px or world.get_hero_y() != py:
			moved = true
	print("hero after moves: (%d,%d) -> (%d,%d)" % [hx0, hy0, world.get_hero_x(), world.get_hero_y()])
	if not moved:
		push_error("四方向移動皆未改變英雄座標"); failed += 1

	# --- NPC 追擊：英雄原地等待，NPC 接近並攻擊，HP 應下降 ---
	var hp0: int = world.get_hero_hp()
	for i in range(60):
		world.player_wait()
		if world.get_hero_hp() <= 0:
			break
	print("chase: hero hp %d -> %d" % [hp0, world.get_hero_hp()])
	if world.get_hero_hp() >= hp0:
		push_error("等待多輪後 HP 未因 NPC 攻擊下降"); failed += 1

	# --- debug trace：開啟、動一步、檢查有產生 log ---
	world.restart()
	world.set_trace_enabled(true)
	world.clear_debug_log()
	world.player_move(0, 1)
	var tlog: String = world.get_debug_log()
	if tlog.strip_edges() == "":
		push_error("trace 未產生任何行"); failed += 1
	else:
		print("trace sample (前數行):")
		for ln in tlog.split("\n"):
			if ln.strip_edges() != "":
				print("  ", ln)
	world.set_trace_enabled(false)

	# --- 存讀檔 round-trip ---
	var save_path := "user://verify_save.bin"
	var ok_save: bool = world.save_game(save_path)
	var exists: bool = world.has_save_game(save_path)
	if not ok_save or not exists:
		push_error("save_game 失敗 (ok=%s exists=%s)" % [ok_save, exists]); failed += 1
	else:
		var saved_turn: int = world.get_turn_count()
		var saved_floor: int = world.get_current_floor()
		world.player_wait()  # 改變狀態
		var ok_load: bool = world.load_game(save_path)
		if not ok_load:
			push_error("load_game 失敗"); failed += 1
		elif world.get_turn_count() != saved_turn or world.get_current_floor() != saved_floor:
			push_error("讀檔後狀態不符 (turn %d!=%d floor %d!=%d)" % [
				world.get_turn_count(), saved_turn, world.get_current_floor(), saved_floor]); failed += 1
		else:
			print("save/load round-trip OK (turn=%d floor=%d)" % [saved_turn, saved_floor])

	if failed == 0:
		print("VERIFY PASSED")
	else:
		print("VERIFY FAILED: %d 項" % failed)
	quit(failed)
	return true  # 結束主迴圈（單幀驗證）
