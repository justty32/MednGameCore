# Headless Verify 制度化

> 日期：2026-06-11
> 定位：把 `godot_zone/verify.gd` 從「跑完沒錯」提升為「斷言接線真的有效」。

---

## 1. 教訓

verify.gd 曾出現假通過：流程跑完、印出訊息，但世界狀態其實沒按預期改變。這類測試只能證明沒有 crash，不能證明功能有效。

## 2. 斷言準則（優先序：高）

每個 case 至少要有一個狀態斷言：

- clock / turn count 改變。
- actor 位置改變。
- HP 改變。
- event count 增加。
- save/load 後欄位一致。

禁止只有 `print()` 沒有 `_assert()` 的 case。

## 3. Helper 草案

```gdscript
func _expect_changed(label: String, before, after) -> void
func _expect_event(type: String) -> void
func _expect_actor_hp(actor_id: int, hp: int) -> void
func _expect_snapshot_has_actor(actor_id: int) -> void
```

這些 helper 依賴 snapshot / drain_events，避免 parse debug 字串。

## 4. CI 接線

headless verify 比 ctest 重，放第二階段 CI：

- 先 build extension。
- 啟動 Godot headless。
- 跑 `res://verify.gd`。
- 上傳 log。

## 5. 不做的事

- 不在 verify.gd 重測所有 core 邏輯；core 邏輯交給 ctest。
- 不把 UI 動畫細節放 headless verify；只驗接線與狀態。
- 不接受 silent skip，缺 Godot binary 應明確標示 skipped / failed。
