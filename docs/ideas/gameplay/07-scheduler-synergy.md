# Scheduler × Gameplay 機制相性

> 日期：2026-06-11
> 定位：盤點哪些玩法在 A/B/C 三種 scheduler 下最有趣，避免新增機制只服務單一時間模型。

---

## 1. 三模型簡述

| 模型 | 體感 |
|---|---|
| A 能量瞬發 | 快慢差、誰先動、行動立即結算 |
| B 能量 + channel | 詠唱、打斷、風險窗口 |
| C tick remaining | 固定倒數、DoT 與延遲效果清楚 |

## 2. 機制相性矩陣

| 機制 | A | B | C | 備註 |
|---|---|---|---|---|
| 命中/暴擊 | 高 | 高 | 高 | 與 scheduler 幾乎獨立 |
| cooldown | 高 | 高 | 中 | B 下 cd + 詠唱形成雙節奏 |
| channel spell | 低 | 最高 | 高 | A 可視為 weight 動作，不是主舞台 |
| interrupt | 中 | 最高 | 高 | B 最有戲 |
| DoT | 中 | 高 | 最高 | C 的 tick 感最直觀 |
| 防禦/Guard | 高 | 高 | 中 | B 可用來中斷施法改防禦 |
| 速度 buff | 最高 | 高 | 高 | A 最容易感知 |

## 3. 設計原則

- 新機制可以偏愛某 scheduler，但不能讓另外兩個壞掉。
- 若機制強依賴 B（如 channel interrupt），A/C 至少要有退化語意。
- trace 要標明 scheduler 相關原因，例如 cooldown、channel interrupted、not ready。

## 4. 內容建議

- A 主打：速度、連擊、短 cd、閃避。
- B 主打：高風險法術、打斷、防禦反應。
- C 主打：陷阱、DoT、倒數爆炸、延遲召喚。

同一技能可依 scheduler 語意有不同手感，但資料定義應盡量共用。

## 5. 不做的事

- 不為每個 scheduler 寫一套完全不同技能表。
- 不把 scheduler mode 當難度選項；它們是時間語法，不只是快慢。
- 不在未收斂前大量製作內容，避免三套語意都要重平衡。
