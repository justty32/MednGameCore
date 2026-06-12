# 效能基準

> 日期：2026-06-11
> 定位：在大量 actor、trace、三 scheduler 模式下建立可重複的效能量測。

---

## 1. 量測目標

- scheduler advance 成本。
- NPC decide 成本。
- ZoneEvent / trace 開銷。
- snapshot export 成本。
- 大地圖 / 多 actor 下的 map query 成本。

## 2. 場景

| 場景 | actor | 特徵 |
|---|---|---|
| tiny | 8 | 日常 smoke |
| medium | 64 | 一般樓層上限 |
| crowd | 256 | 壓力測試 |
| caster | 64 | 大量 channel / nova |
| dot | 64 | 大量 timed effects |

每個場景固定 seed，A/B/C 都跑。

## 3. 指標

- ticks/sec 或 turns/sec。
- events generated。
- trace on/off 差異。
- snapshot export ms。
- max frame budget 估算。

## 4. 工具策略

第一版用 C++ benchmark helper + `std::chrono` 即可，輸出 CSV。等瓶頸明確後再考慮 Google Benchmark。

## 5. 不做的事

- 不把 benchmark 結果作為精確科學數字；看趨勢與回歸。
- 不優化沒有測出來的瓶頸。
- 不在 gameplay 未穩時過早微調資料結構。
