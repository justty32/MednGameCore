# NPC AI 資料化光譜

> 日期：2026-06-11
> 定位：從現有 `decide_chase` 追擊腦出發，規劃 NPC AI 如何逐步參數化，而不是一步跳到行為樹。

---

## 1. 現況

`npc_brain.cpp` 已有純函式決策：追玩家、相鄰攻擊、caster 可放 `npc_flame`。這很適合測試，但行為差異主要寫死在 C++ 與 spawn 邏輯。

## 2. 第一階段：ChaseParams（優先序：中高）

PrototypeDef 加：

```json
{
  "ai": {
    "kind": "chase",
    "aggro_range": 8,
    "preferred_range": 1,
    "skill": "npc_flame",
    "cast_chance": 35,
    "flee_hp_percent": 0
  }
}
```

C++ 仍是同一個 `decide_chase(params, world)`，只是參數由 prototype 來。

## 3. 第二階段：Utility 權重表（中低）

當技能、狀態、資源變多後，caster 不該只靠固定 chance：

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
