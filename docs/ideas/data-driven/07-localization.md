# Localization：訊息字串與事件文字分層

> 日期：2026-06-11
> 定位：記錄何時、如何把 trace / combat log / UI 字串從程式碼抽出，避免太早為 debug 字串背包袱。

---

## 1. 字串類型

| 類型 | 是否需要 localization |
|---|---|
| Debug trace | 低，開發者看得懂即可 |
| Combat log | 中高，玩家會看到 |
| UI label | 高 |
| Data id | 不翻譯，穩定機器 id |

## 2. 事件先結構化，文字後生成

不要讓 core 直接吐最終玩家文字：

```text
{ type: "damage", actor: 1, target: 2, amount: 5, damage_type: "fire" }
```

前端或 localization layer 再轉：

```text
"Fireball hits Goblin for 5 fire damage."
```

這樣同一事件可用於 HUD、動畫、測試、不同語言。

## 3. 資料檔草案

```json
{
  "locale": "zh-TW",
  "strings": {
    "action.fireball.name": "火球",
    "event.damage": "{actor} 對 {target} 造成 {amount} 點{damage_type}傷害"
  }
}
```

ActionDef / PrototypeDef 只存 `name_key`，不直接存顯示文字。

## 4. 何時做

等以下兩件事完成再做：

1. ZoneEvent 結構化欄位穩定。
2. HUD / combat log 已經開始顯示玩家文字。

debug trace 可晚一點，甚至永遠只維持英文/中文開發語。

## 5. 不做的事

- 不把 localization 放進 core combat 原語。
- 不翻譯 id。
- 不為尚未定型的 debug 字串建立龐大字典。
