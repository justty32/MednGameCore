# NPC AI 資料化光譜

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：從現有 `decide_chase` 追擊腦出發，規劃 NPC AI 如何逐步參數化，而不是一步跳到行為樹。**AI 光譜仍為有效未來方向。**

---

## 1. 現況

`apply_action.cpp` 的 `decide_chase(reg, self, target)` 是**唯一**的 NPC 決策：
朝 target 移動一格，相鄰即撞擊攻擊（攻擊＝移動進入有 actor 的格子），無目標則
`Action::wait()`。2026-06-21 重構**移除了施法者 AI**（舊的 nova/`npc_flame`/
is_caster 已隨技能系統一併刪除）——現在只剩單純追擊。這很適合測試，但所有行為
差異仍寫死在 C++ 與 spawn 邏輯。

> **落地點**：「AI／操控由誰決定」現在統一收斂到 `ControllerComponent.kind`
> （`Player`/`Self`/`Faction`）。TurnEngine 依此分派：`Player` 暫停等玩家、
> `Self`/`Faction` 走 `ctx.decide()`（目前即 `decide_chase`）。要擴充 AI 種類時，
> 這個 kind ＋ prototype 的 `ai` 欄位（見 02）就是分派的接口。

## 2. 第一階段：ChaseParams（優先序：中高）

PrototypeDef 的 `ai` 由字串擴成物件，加可調參數：

```json
{
  "ai": {
    "kind": "chase",
    "aggro_range": 8,
    "preferred_range": 1,
    "flee_hp_percent": 0
  }
}
```

C++ 仍是同一個 `decide_chase`，只是改吃 prototype 來的參數。
（`skill`/`cast_chance` 這類施法欄位等 01 的資料驅動動作重新引入後再加，
目前沒有技能可放。）

## 3. 第二階段：Utility 權重表（中低）

當技能、狀態、資源變多後（前提：01 的資料驅動動作已重新引入），caster 不該只靠
固定 chance。下例的 `heal`/`fireball` 是**假想的未來 action id**（目前不存在）：

```json
"utility": [
  { "action": "heal", "when": "self_hp_below", "value": 35, "weight": 100 },
  { "action": "fireball", "when": "enemy_count_in_radius", "value": 2, "weight": 80 }
]
```

表達式能力要極窄，避免變成腳本引擎。

## 4. 第三階段：行為樹（暫不做）

行為樹只有在需要巡邏、呼援、互動物件、任務腳本時才值得。現在 roguelike 戰鬥 AI 用 utility 已足夠。

## 5. 不做的事

- 不引入 Lua/quickjs 只為 NPC 行為。
- 不讓 AI 資料直接指定 entity id；只引用 prototype、tag、action id。
- 不讓 NPC 作弊無視 cooldown / MP / line of sight。
