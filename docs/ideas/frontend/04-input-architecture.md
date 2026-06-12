# 輸入架構：Input Map 與多角色控制

> 日期：2026-06-11
> 定位：把 `_unhandled_input` 直接 match keycode 的測試台輸入，整理成可擴充到技能、游標、多角色 order 的輸入架構。

---

## 1. 現況問題

直接比對 keycode 的好處是快，壞處是：

- 無法讓玩家重綁鍵。
- 技能、等待、防禦、切換角色、游標模式會讓 match 分支膨脹。
- 多角色 order 表落地後，輸入不再只是「控制 hero 移動」。

## 2. Input Map 分層（優先序：中）

Godot Input Map 建議 action：

```text
move_n / move_ne / move_e / move_se / move_s / move_sw / move_w / move_nw
wait
skill_1..skill_8
defend
cancel
confirm
next_actor / prev_actor
toggle_debug
```

`map_view.gd` 不直接處理「按鍵」，而是處理「意圖」：

```text
Input action -> FrontendCommand -> ZoneWorld submit / UI state transition
```

## 3. 模式狀態機

最少三個模式：

| 模式 | 意義 |
|---|---|
| `normal` | 移動、等待、直接施放 self/nova 技能 |
| `direction_targeting` | bolt/cone 這類方向技能等待方向 |
| `cursor_targeting` | 指定格技能、檢視、未來遠程互動 |

`cancel` 永遠回上一層或 normal；`confirm` 在游標模式送出 action。

## 4. 多角色 order 表接線

當 scheduler 回報 `waiting_actor()` 時：

- 若 actor 是玩家陣營，前端選中該 actor。
- `next_actor/prev_actor` 只在「多名玩家 actor 都可下令」的 order 模型中啟用。
- UI 顯示目前 active actor，技能列改吃該 actor 的 known skills / cooldown。

## 5. 不做的事

- 不把輸入映射塞進 C++ core；core 只收 Action。
- 不在第一版做滑鼠完整操作；鍵盤 roguelike 路徑先穩。
- 不用輸入模式驅動 scheduler 結果；模式只決定如何組出 Action。
