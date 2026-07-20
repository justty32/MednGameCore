extends SceneTree

# Headless 驗證：godot --headless --path godot_zone -s res://verify_tactics.gd
# 實際載入 libzone_gd.so，驗證 TacticsBattle 端到端可用：
# 一個機器人操作我方單位，必須把整場戰鬥打到分出勝負。

const BATTLE_SEED := 20260710
const MAX_TURNS := 400
const TF_WALKABLE := 1

var battle
var seen := { "moved": 0, "attacked": 0, "died": 0, "turns": 0, "over": -99 }

func _initialize() -> void:
	battle = TacticsBattle.new()
	battle.name = "TacticsBattle"
	root.add_child(battle)

	# 一定要先接好 signal 再 start_battle()：開局的 pump() 會跑敵方 AI 並發事件
	battle.turn_started.connect(func(_id): seen["turns"] += 1)
	battle.unit_moved.connect(func(_id, _fx, _fy): seen["moved"] += 1)
	battle.unit_attacked.connect(func(_a, _b, _d): seen["attacked"] += 1)
	battle.unit_died.connect(func(_id): seen["died"] += 1)
	battle.battle_over.connect(func(team): seen["over"] = team)

func _cheby(ax: int, ay: int, bx: int, by: int) -> int:
	return max(abs(ax - bx), abs(ay - by))

# 我方單位的極簡策略：能打就打，否則走向最近的敵人，再不行就待機。
func _play_player_turn() -> void:
	var targets: Array = battle.get_attack_targets()
	if not targets.is_empty():
		battle.command_attack(targets[0])
		return

	var units: Array = battle.get_units()
	var cur: int = battle.current_unit()
	var me := {}
	var foes := []
	for u in units:
		if u["id"] == cur:
			me = u
		elif u["team"] != 0:
			foes.append(u)

	if me.is_empty() or foes.is_empty():
		battle.command_wait()
		return

	var tiles: PackedVector2Array = battle.get_move_tiles()
	var best := Vector2i(me["x"], me["y"])
	var best_d := 1 << 30
	for t in tiles:
		var tx := int(t.x)
		var ty := int(t.y)
		var d := 1 << 30
		for f in foes:
			d = min(d, _cheby(tx, ty, f["x"], f["y"]))
		if d < best_d:
			best_d = d
			best = Vector2i(tx, ty)

	if best.x != int(me["x"]) or best.y != int(me["y"]):
		if battle.command_move(best.x, best.y):
			# 移動後可能進了射程
			var t2: Array = battle.get_attack_targets()
			if not t2.is_empty():
				battle.command_attack(t2[0])
				return
			battle.command_wait()
			return
	battle.command_wait()

func _process(_delta: float) -> bool:
	var failed := 0
	battle.start_battle(BATTLE_SEED)

	var w: int = battle.get_map_width()
	var h: int = battle.get_map_height()
	print("arena: %dx%d  units=%d" % [w, h, battle.get_units().size()])
	if w <= 0 or h <= 0:
		push_error("競技場尺寸異常"); failed += 1

	# --- tile flags ---
	var flags: PackedByteArray = battle.get_tile_flags()
	if flags.size() != w * h:
		push_error("get_tile_flags 長度 %d != %d" % [flags.size(), w * h]); failed += 1
	var walkable := 0
	for f in flags:
		if f & TF_WALKABLE:
			walkable += 1
	print("walkable tiles: %d / %d" % [walkable, w * h])
	if walkable < w * h / 4:
		push_error("可走格太少，競技場可能被牆塞滿"); failed += 1

	# --- 雙方都有單位 ---
	var team_count := [0, 0]
	for u in battle.get_units():
		team_count[u["team"]] += 1
	print("units: player=%d enemy=%d" % [team_count[0], team_count[1]])
	if team_count[0] == 0 or team_count[1] == 0:
		push_error("開局就有一隊是空的"); failed += 1

	# --- 行動條預測 ---
	var order: Array = battle.get_turn_order(6)
	print("turn order (6): ", order)
	if order.size() != 6:
		push_error("get_turn_order 長度 %d != 6" % order.size()); failed += 1

	# --- 開局應該輪到玩家，且有可移動的格 ---
	if not battle.is_waiting_player():
		push_error("開局未停在玩家單位（current=%d）" % battle.current_unit()); failed += 1
	elif battle.get_move_tiles().is_empty():
		push_error("玩家單位沒有任何可移動的格"); failed += 1

	# --- 機器人把整場打完 ---
	var turns := 0
	while turns < MAX_TURNS and not battle.is_battle_over():
		if battle.is_waiting_player():
			_play_player_turn()
		else:
			# pump() 應該已經自動跑完 AI；停在這裡代表推進卡住了
			push_error("既非玩家回合、戰鬥也沒結束（current=%d）" % battle.current_unit())
			failed += 1
			break
		turns += 1

	print("battle: %s 於 %d 個玩家回合後結束（winner=%d）" % [
		"結束" if battle.is_battle_over() else "未結束", turns, battle.get_winner()])
	print("events: turn_started=%d moved=%d attacked=%d died=%d battle_over=%d" % [
		seen["turns"], seen["moved"], seen["attacked"], seen["died"], seen["over"]])

	if not battle.is_battle_over():
		push_error("跑滿 %d 回合仍未分出勝負" % MAX_TURNS); failed += 1
	if battle.get_winner() != 0 and battle.get_winner() != 1:
		push_error("勝利者異常：%d" % battle.get_winner()); failed += 1
	if seen["over"] != battle.get_winner():
		push_error("battle_over signal 的隊伍 %d 與 get_winner() %d 不符" % [
			seen["over"], battle.get_winner()]); failed += 1
	if seen["moved"] == 0 or seen["attacked"] == 0 or seen["died"] == 0:
		push_error("事件流不完整：移動/攻擊/死亡至少各要有一次"); failed += 1

	# --- 結束後不該再接受指令 ---
	if battle.command_wait():
		push_error("戰鬥結束後仍接受指令"); failed += 1

	if failed == 0:
		print("TACTICS VERIFY PASSED")
	else:
		print("TACTICS VERIFY FAILED: %d 項" % failed)
	quit(failed)
	return true
