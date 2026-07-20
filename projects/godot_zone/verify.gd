extends SceneTree

# Headless 驗證腳本：godot --headless -s res://verify.gd
# 實際載入 libzone_gd.so，驗證 ZoneCore + ZoneWorld（最傳統回合制）端到端可用。

var world  # 在 _initialize 建立、加入 root；_ready 要到第一幀才派發，故檢查放 _process

# 「機器人玩到結局」用的固定種子（見下方 bot 段落）。
# 換 AI / 地圖生成後若失敗，先確認不是真的壞了，再挑一顆新種子。
const BOT_SEED := 12345
const BOT_MAX_ROUNDS := 4000

# get_tile_flags() 的位元
const TF_WALKABLE := 1
const TF_STAIR    := 2

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

	# --- get_tile_flags：整張地圖旗標，row-major，長度 = w*h ---
	var flags: PackedByteArray = world.get_tile_flags()
	if flags.size() != w * h:
		push_error("get_tile_flags 長度 %d != %d" % [flags.size(), w * h]); failed += 1
	else:
		var walkable := 0
		var explored := 0
		for f in flags:
			if f & 1: walkable += 1
			if f & 8: explored += 1
		print("tile flags: %d bytes, walkable=%d explored=%d" % [flags.size(), walkable, explored])
		if walkable == 0:
			push_error("地圖沒有任何可走格"); failed += 1
		if explored == 0:
			push_error("初始 FOV 未標記任何已探索格"); failed += 1

	# --- get_visible_entities：至少含英雄，且座標與 get_hero_x/y 一致 ---
	var ents: Array = world.get_visible_entities()
	var hero_ent := {}
	for e in ents:
		if e["kind"] == "hero":
			hero_ent = e
	if hero_ent.is_empty():
		push_error("get_visible_entities 不含英雄"); failed += 1
	elif hero_ent["x"] != world.get_hero_x() or hero_ent["y"] != world.get_hero_y():
		push_error("英雄座標不一致"); failed += 1
	else:
		print("visible entities: %d（英雄在 (%d,%d)）" % [ents.size(), hero_ent["x"], hero_ent["y"]])

	# --- 勝利樓層常數 ---
	if world.get_win_floor() <= 0:
		push_error("get_win_floor 異常"); failed += 1
	if world.is_game_over() or world.is_game_won():
		push_error("開場就已結束"); failed += 1

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

	# --- 機器人實跑：一路尋路殺向樓梯，必須走到某個結局（通關或戰死）---
	# 這是「這遊戲能不能被玩完」的端到端斷言。NPC 只追看得見的英雄，
	# 站著不動未必會被找到，所以驗證要主動探索而不是原地等。
	# 固定種子 → 可重現，不會隨機失敗。
	failed += _run_bot()

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

# ---- 機器人 ------------------------------------------------------------------

# 從所有下樓梯格反向 BFS，得到每個可走格到最近樓梯的距離。
# 回傳 PackedInt32Array（row-major，-1 = 不可達）。
func _stair_distance_field(w: int, h: int, flags: PackedByteArray) -> PackedInt32Array:
	var dist := PackedInt32Array()
	dist.resize(w * h)
	dist.fill(-1)

	var queue := PackedInt32Array()
	for i in w * h:
		if flags[i] & TF_STAIR:
			dist[i] = 0
			queue.append(i)

	var head := 0
	while head < queue.size():
		var cur := queue[head]
		head += 1
		var cx := cur % w
		var cy := cur / w
		for d: Vector2i in [Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1), Vector2i(0, -1)]:
			var nx := cx + d.x
			var ny := cy + d.y
			if nx < 0 or ny < 0 or nx >= w or ny >= h:
				continue
			var ni := ny * w + nx
			if dist[ni] != -1 or (flags[ni] & TF_WALKABLE) == 0:
				continue
			dist[ni] = dist[cur] + 1
			queue.append(ni)
	return dist

# 一路殺向樓梯，直到通關 / 戰死 / 撞上回合上限。回傳 failed 計數。
func _run_bot() -> int:
	world.set_seed(BOT_SEED)
	world.restart()

	var w: int = world.get_map_width()
	var h: int = world.get_map_height()
	var hp_prev: int = world.get_hero_hp()
	var damage_taken := 0
	var deepest: int = world.get_current_floor()
	var rounds := 0

	while rounds < BOT_MAX_ROUNDS and not world.is_game_over() and not world.is_game_won():
		var flags: PackedByteArray = world.get_tile_flags()
		var dist := _stair_distance_field(w, h, flags)
		var hx: int = world.get_hero_x()
		var hy: int = world.get_hero_y()
		var here := dist[hy * w + hx]

		# 朝距離更小的鄰居走一步。路上有 NPC 擋著 → 撞上去就是攻擊，
		# 打死了下一輪自然通過，所以不必特別處理。
		var stepped := false
		if here > 0:
			for d: Vector2i in [Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1), Vector2i(0, -1)]:
				var nx := hx + d.x
				var ny := hy + d.y
				if nx < 0 or ny < 0 or nx >= w or ny >= h:
					continue
				var nd := dist[ny * w + nx]
				if nd != -1 and nd < here:
					world.player_move(d.x, d.y)
					stepped = true
					break
		if not stepped:
			world.player_wait()   # 不可達（理論上不會發生）：至少別空轉當機

		var hp_now: int = world.get_hero_hp()
		if hp_now < hp_prev:
			damage_taken += hp_prev - hp_now
		hp_prev = hp_now   # 喝藥水回血也走這條，故只累加「掉的」
		deepest = maxi(deepest, world.get_current_floor())
		rounds += 1

	var ending := "通關" if world.is_game_won() else ("戰死" if world.is_game_over() else "未結束")
	print("bot (seed=%d): %s，%d 輪，最深第 %d 層，累計受傷 %d，HP %d/%d" % [
		BOT_SEED, ending, rounds, deepest, damage_taken,
		world.get_hero_hp(), world.get_hero_max_hp()])

	var failed := 0
	if not world.is_game_won() and not world.is_game_over():
		push_error("機器人跑滿 %d 輪仍未走到結局（最深第 %d 層）" % [BOT_MAX_ROUNDS, deepest])
		failed += 1
	if world.is_game_won() and world.get_current_floor() != world.get_win_floor():
		push_error("通關時樓層 %d != 勝利樓層 %d" % [world.get_current_floor(), world.get_win_floor()])
		failed += 1
	if deepest < 2:
		push_error("機器人連第 2 層都沒到，樓梯/下樓流程可能壞了")
		failed += 1
	if damage_taken == 0:
		push_error("整場沒被打到一次，NPC 追擊或反擊可能壞了")
		failed += 1
	return failed
