extends Control

# 兩個平行場景的入口。地城與戰棋共用 ECS／地圖／圖集，但回合驅動各自獨立。

func _ready() -> void:
	$Center/Box/DungeonButton.pressed.connect(
		func(): get_tree().change_scene_to_file("res://map_view.tscn"))
	$Center/Box/TacticsButton.pressed.connect(
		func(): get_tree().change_scene_to_file("res://tactics_view.tscn"))
	$Center/Box/QuitButton.pressed.connect(func(): get_tree().quit())
	$Center/Box/DungeonButton.grab_focus()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
		get_tree().quit()
