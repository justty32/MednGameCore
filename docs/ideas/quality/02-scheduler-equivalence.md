# 排程器等價性與差分測試

> 日期：2026-06-11
> 定位：把「三種 scheduler 都能跑」提升成「三種 scheduler 的共同承諾可被測試」。

---

## 1. 問題

A/B/C 各自有測試，但缺少跨模式不變量。這代表某個新機制可能只在 A 正確，在 B/C 悄悄偏掉。

## 2. 第一組等價性（優先序：高）

在 `weight=1`、無 channel、無 cooldown 的情況：

```text
A energy instant ≈ B energy channel ≈ C tick remaining
```

可比較：

- 最終 actor 位置。
- HP。
- alive/dead。
- item pickup 結果。
- ZoneEvent 類型序列（允許 scheduler-specific trace 不同）。

## 3. 差分 fixture

建立 helper：

```cpp
run_scenario(mode, seed, commands) -> WorldDigest
```

`WorldDigest` 不含 entity id 細節，改用 actor label / position / components 摘要，避免 id 建立順序造成假差異。

## 4. 非等價但應守的不變量

對 channel / tick 差異明顯的情境，比較不變量：

- HP 不為負。
- dead actor 無 Spatial 或不再行動。
- waiting_actor 若為玩家，advance 不應吞掉玩家回合。
- OngoingAction 最終完成或被明確 interrupt，不會永久卡住。

## 5. 不做的事

- 不強求三模式每一步事件完全同序。
- 不用 debug_text 做比較。
- 不在差分測試中依賴 wall-clock 或隨機種子未固定的 AI。
