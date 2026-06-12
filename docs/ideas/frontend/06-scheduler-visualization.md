# 排程器視覺化：能量、詠唱與時間線

> 日期：2026-06-11
> 定位：把 A/B/C 三種 scheduler 的不可見時間狀態，轉成玩家能理解的 UI 回饋。

---

## 1. 現況問題

排程器是 gamecore-zone 的核心賣點，但目前玩家只能看 debug dump。若不視覺化，B 模型的 channel / interrupt、A 模型的速度差、C 模型的 tick 剩餘都會變成黑箱。

## 2. 最小可用視覺化（優先序：中）

每個 actor 顯示：

- HP 條。
- energy / next action meter。
- ongoing action 名稱與剩餘進度。
- 若等待玩家輸入，外框閃爍。

HUD 側邊加一個「即將行動」列表：

```text
Hero      ready
Goblin A  74%
Caster B  casting fireball 2/3
```

## 3. 模式差異

| Scheduler | UI 重點 |
|---|---|
| A energy instant | 下一個 ready actor、速度差 |
| B energy channel | 詠唱進度、可打斷窗口 |
| C tick remaining | 剩餘 ticks、固定倒數感 |

不需要三套 UI。共用欄位為 `readiness`、`ongoing`、`waiting`，由 binding 層把各 scheduler 內部狀態正規化。

## 4. 需要的 snapshot 欄位

```text
actor.scheduler = {
  "readiness": float,      // 0..1
  "is_waiting": bool,
  "ongoing_name": String,
  "ongoing_remaining": int,
  "ongoing_total": int
}
```

如果某 scheduler 沒有某欄，填合理預設，不讓前端知道太多 C++ 型別。

## 5. 不做的事

- 不做華麗時間軸動畫第一版；先讓玩家知道誰快動、誰在詠唱。
- 不把 scheduler 內部 pending map 原樣曝給 Godot。
- 不為 debug 視覺化破壞 scheduler 可替換性。
