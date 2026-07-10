# 效能基準

> 日期：2026-06-21
> 定位：在大量 actor、trace 開關下，對 `TurnEngine` 推進建立可重複的效能量測。
>
> **狀態更新（2026-06-21）**：原以「三 scheduler 模式的 advance 成本」「caster channel / nova / DoT 場景」為對象。
> 這些系統已於回合系統重構移除。本檔改以單一 `TurnEngine` 的 `step()` / 推進一輪為量測對象，
> 並移除施法者/效果相關場景。

---

## 1. 量測目標

- `TurnEngine::step()` 單步成本。
- 推進一整輪（`begin_round()` → 反覆 `step()` 到 `RoundDone`）的成本。
- NPC `decide_chase` 成本（隨 actor 數的擴張）。
- `apply_action` 結算成本（移動/撞擊/拾取分支）。
- `ZoneEvent` 蒐集與 trace 開銷。
- snapshot / `generate_map_image` export 成本。
- 大地圖 / 多 actor 下的 map query（walkable / 佔格查詢）成本。

## 2. 場景

| 場景 | actor | 特徵 |
|---|---|---|
| tiny | 8 | 日常 smoke |
| medium | 64 | 一般樓層上限 |
| crowd | 256 | 壓力測試（多 NPC 追擊、撞擊互殺） |
| chase | 64 | 全 NPC 朝英雄收斂，撞擊頻繁 |

每個場景固定 seed；trace on/off 各跑一次比較。

## 3. 指標

- rounds/sec（推進一輪）與 steps/sec（單 actor 行動）。
- events generated。
- trace on/off 差異。
- snapshot / map image export ms。
- max frame budget 估算（前端按一鍵 = 英雄一動 + 全 NPC 一輪，須落在一幀預算內）。

## 4. 工具策略

第一版用 C++ benchmark helper + `std::chrono` 即可，輸出 CSV。
benchmark 屬獨立執行檔或 `zone_test` 的 stress 子集，**建置一律帶並行限制**（見 06 §0）。
等瓶頸明確後再考慮 Google Benchmark。

## 5. 不做的事

- 不把 benchmark 結果作為精確科學數字；看趨勢與回歸。
- 不優化沒有測出來的瓶頸。
- 不在 gameplay 未穩時過早微調資料結構。
- 不為已移除的排程器/效果系統保留場景。
