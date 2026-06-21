# Localization：訊息字串與事件文字分層

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：記錄何時、如何把 trace / combat log / UI 字串從程式碼抽出，避免太早為 debug 字串背包袱。**遠期方向，受重構影響小。**

> **狀態（2026-06-21）**：本文為遠期方向。下文的「火球/火傷」僅為**結構化事件
> 的示意**，非現有內容——目前的 `ZoneEvent`（`apply_action` 產出）只有撞牆/撞 actor/
> actor 死亡/拾取道具/抵達下樓梯等，沒有技能或傷害類型。技能類事件要等 01 的
> 資料驅動動作重新引入後才會出現。

---

## 1. 字串類型

| 類型 | 是否需要 localization |
|---|---|
| Debug trace | 低，開發者看得懂即可 |
| Combat log | 中高，玩家會看到 |
| UI label | 高 |
| Data id | 不翻譯，穩定機器 id |

## 2. 事件先結構化，文字後生成

不要讓 core 直接吐最終玩家文字（`ZoneEvent` 已是結構化的，沿用此原則）。
以一個未來的傷害事件為例：

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
    "proto.grunt.name": "雜兵",
    "item.health_potion.name": "治療藥水",
    "event.damage": "{actor} 對 {target} 造成 {amount} 點{damage_type}傷害"
  }
}
```

PrototypeDef（及未來的 ActionDef）只存 `name_key`，不直接存顯示文字。

## 4. 何時做

等以下兩件事完成再做：

1. ZoneEvent 結構化欄位穩定。
2. HUD / combat log 已經開始顯示玩家文字。

debug trace 可晚一點，甚至永遠只維持英文/中文開發語。

## 5. 不做的事

- 不把 localization 放進 core combat 原語。
- 不翻譯 id。
- 不為尚未定型的 debug 字串建立龐大字典。
