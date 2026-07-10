# Headless Verify 制度化

> 日期：2026-06-21
> 定位：把 `godot_zone/verify.gd` 從「跑完沒錯」提升為「斷言接線真的有效」。
>
> **狀態更新（2026-06-21）**：重構後 verify.gd 改用最傳統回合制的新 API
> （`player_move` / `player_wait` / `next_round` / `is_waiting_player` / `round_in_progress`）；
> 舊的排程器模式切換、cast、skill 介面已移除。本檔已更新流程、API 與實務細節；
> 斷言準則的方法論不變。

---

## 1. 教訓

verify.gd 曾出現假通過：流程跑完、印出訊息，但世界狀態其實沒按預期改變。
這類測試只能證明沒有 crash，不能證明功能有效。重構後 API 變了，但教訓不變。

## 2. 現行流程與 API

執行（兩步，缺第一步會找不到 GDExtension）：

```bash
# 1) 先建立 .godot/extension_list.cfg（首次或 .so 更新後）
godot-mono --headless --path godot_zone --import
# 2) 跑驗證腳本
godot-mono --headless --path godot_zone -s res://verify.gd
# → 結尾應印 VERIFY PASSED
```

實務細節（必記）：
- **`.so` 在 `godot_zone/bin/`（gitignore）**，由 `zone_gd` 建置時 POST_BUILD 自動 `copy_if_different`；
  若驗證前剛重建，要先確認新 `.so` 已就位。
- **首次或換機需先 `--headless --import`** 建立 `.godot/extension_list.cfg`，
  否則 `ZoneCore` / `ZoneWorld` 類別載入不到、verify.gd 直接失敗。
- **腳本結尾出現 editor core dump 是已知無害現象**：判定通過看的是有沒有印出 `VERIFY PASSED`
  與 `push_error` 計數為 0，不是看 process exit cleanly。

verify.gd 目前驗：版本字串非空、地圖尺寸與英雄 HP 初始化、`generate_map_image` 尺寸、
`player_wait` 推進整輪（turn_count 遞增且 `round_in_progress()` 收尾為 false）、
`player_move` 四方至少一次合法移動改座標、NPC 追擊致 HP 下降、trace、save/load round-trip。

## 3. 斷言準則（優先序：高）

每個 case 至少要有一個狀態斷言（用回傳值，不要 parse debug 字串）：

- turn count 改變（`get_turn_count`）。
- 英雄位置改變（`get_hero_x` / `get_hero_y`）。
- HP 改變（`get_hero_hp`）。
- 回合狀態收尾（`round_in_progress()` 為 false、或 `is_waiting_player()` 如預期）。
- save/load 後欄位一致。
- event 經由 signal 觀察到（`npc_died` / `item_picked_up` / `hero_bumped_wall` / `floor_changed` 等）。

禁止只有 `print()` 沒有狀態斷言的 case。

## 4. Helper 草案

```gdscript
func _expect_changed(label: String, before, after) -> void
func _expect_unchanged(label: String, before, after) -> void
func _expect_actor_hp(hp_before: int, hp_after: int) -> void
func _expect_signal(signal_name: String) -> void   # 連 signal 計數，斷言被觸發
```

這些 helper 依賴回傳值與 signal，避免 parse debug 字串。

## 5. CI 接線

headless verify 比 doctest 重，放第二階段 CI：

- 先 build extension（**建置帶並行限制 `-j4`，曾 OOM**）。
- `--headless --import` 建立 extension_list.cfg。
- 跑 `res://verify.gd`，以 `VERIFY PASSED` 字串判定。
- 上傳 log（含已知無害的結尾 core dump，避免被誤判為失敗）。

## 6. 不做的事

- 不在 verify.gd 重測所有 core 邏輯；core 邏輯交給 doctest。
- 不把 UI 動畫細節放 headless verify；只驗接線與狀態。
- 不接受 silent skip，缺 Godot binary 應明確標示 skipped / failed。
- 不以 process exit code 當唯一判據（結尾 core dump 已知無害）；以 `VERIFY PASSED` 為準。
