# quality ideas 總覽與路線圖

> 日期：2026-06-21
> 定位：docs/ideas/quality/ 目錄的索引——測試、品質與開發工具各主題檔的關係、優先序總表、建議推進順序。
>
> **狀態更新（2026-06-21）**：本目錄原以「三排程器（A 能量瞬發 / B 能量＋詠唱 / C 純 tick）＋資料驅動 JSON 技能＋DoT 效果」為前提撰寫。該套系統已於回合系統重構中**整體移除**，重建為最傳統、最精簡的 `TurnEngine`。本檔已改正所有「現況」描述；各主題檔的前瞻設計價值保留，但前提改為「建立在最簡 TurnEngine 之上的未來可選擴充」。

---

## 1. 這個目錄是什麼

gamecore-zone 目前的品質基線：

- **doctest 29 個 test case / 93 個 assertion 全綠**（`zone_test`）。
- godot-mono headless `verify.gd` → **VERIFY PASSED**（最傳統回合制：`player_move` / `player_wait` 推進、NPC 追擊致 HP 下降、trace、存讀檔 round-trip）。
- 主場景 `res://map_view.tscn` headless 可跑通。

但「全綠」不等於「夠用」——本目錄從**測試策略、回歸防護、CI、效能基準、debug 工具**
五個面向盤點缺口並記錄前瞻想法，作為日後 brainstorming → spec → plan 的素材庫。

這些是**想法，不是定案 spec**。動工前仍應走 brainstorming 收斂成
`docs/superpowers/specs/` 的正式設計文件。

## 2. 檔案地圖

| 檔案 | 主題 | 一句話 |
|---|---|---|
| `01-test-coverage.md` | 測試覆蓋盤點 | 現有 29 案覆蓋什麼、缺什麼——含序列化元件清單的真實缺口 |
| `02-scheduler-equivalence.md` | 決定性回放測試 | 原「排程器等價性」前提消失；重定位為單一 TurnEngine 的「同 seed 同指令序列 → 同世界 hash」 |
| `03-property-simulation.md` | property / 模擬測試 | N 回合隨機指令長跑、守恆類不變量、fuzz 思路 |
| `04-headless-verify.md` | headless 驗證制度化 | verify.gd 假通過教訓——「斷言世界真的變了」的寫法準則與新 API 流程 |
| `05-data-regression.md` | 資料回歸防護 | 技能 JSON 已移除；重定位為地圖生成/存檔回歸與未來資料驅動內容的護欄 |
| `06-ci.md` | CI 構想 | GitHub Actions 跑 doctest + headless godot 的可行性、快取策略與並行限制鐵則 |
| `07-benchmark.md` | 效能基準 | 大量 actor 下 `TurnEngine` step / 推進一輪的成本、trace 開銷量測 |
| `08-debug-roadmap.md` | debug 工具路線 | 種子統一、指令重播、狀態雜湊、結構化 debug dump |

## 3. 優先序總表（高優先項彙整）

| 想法 | 出處 | 為什麼先做 |
|---|---|---|
| `AllComponents` 序列化 round-trip 完整測試 + 守門測試 | 01 §3 | 新增 `ControllerComponent` / `ActPointComponent` 後，存讀檔是否完整保留操控者與行動點未被斷言 |
| verify.gd 斷言準則（消滅殘存 print-only 檢查） | 04 §2 | 假通過已抓到一次真 bug；新流程仍應堅持「斷言世界真的變了」 |
| 決定性回放測試（同 seed 同指令序列 → 同世界 hash） | 02 | 單一 TurnEngine 的決定性是長跑/重播/差分的共同地基 |
| 隨機指令長跑模擬（不崩潰 + 基本不變量） | 03 | restart 雙重 destroy、移動越界都是這類 bug；用模擬提前撈 |
| GitHub Actions：doctest job（含並行限制） | 06 §2 | 成本最低的回歸護欄，先擋住 core 層 |
| RNG 種子統一（WorldSeed） | 08 §2 | 重播/差分測試/長跑重現的共同前置；現在多處種子策略不一致 |

## 4. 建議推進順序（三波）

**第一波（堵已知漏洞，彼此獨立可並行）**
1. 序列化 round-trip：`AllComponents` 全狀態 round-trip + 守門測試（01 §3）
2. verify.gd 斷言化改寫 + `_expect_changed` helper（04 §2-3）
3. CI 第一階段：ubuntu doctest（06 §2，務必帶並行限制）

**第二波（把決定性變成可驗證的承諾）**
4. RNG 種子統一（08 §2）——差分/模擬測試的前置
5. 決定性回放測試（02）
6. 隨機指令長跑模擬 + 不變量檢查（03）

**第三波（工具與量化）**
7. CI 第二階段：headless godot job（06 §3）
8. TurnEngine benchmark + trace 開銷量測（07）
9. 指令重播 + 狀態雜湊（08 §3-4）

## 5. 共用依賴速查（實際檔案路徑）

- 測試：`tests/src/`（doctest v2.4.11，FetchContent）、`tests/CMakeLists.txt`
- 回合引擎：`src/core/turn/turn_engine.{h,cpp}`（`TurnEngine`、`EngineCtx`、`StepResult`）
- 動作結算：`src/core/turn/apply_action.{h,cpp}`（`apply_action`、`decide_chase`）、`action.h`、`move_dir.h`
- 序列化：`src/core/serialize/all_components.h`（**元件清單真相層**）、`save_load.h`、`save_store.h`
- 元件：`src/core/components/controller_component.h`、`act_point_component.h`
- 前端：`src/gbind/zone_world_gd.{h,cpp}`（mapgen RNG、`turn_rng_`、trace ring）、
  `godot_zone/verify.gd`、`godot_zone/map_view.gd`

## 6. 不變原則（所有想法都必須遵守）

1. **core 先行**：能在 `zone_core` 純 C++ 測的就不要依賴 godot headless；
   headless 只驗「接線」，不驗「邏輯」。
2. **斷言世界狀態，不斷言流程跑完**：「沒報錯」與「印出來了」都不是通過條件，
   通過條件是「狀態如預期地改變」（verify.gd 假通過教訓的核心）。
3. **決定性優先**：所有測試與模擬必須可用固定種子重現；新增隨機性一律走可注入的 RNG。
   `TurnEngine` 本身已是純粹可測（registry 由 `EngineCtx` 外部注入）。
4. **失敗要大聲**：silent fallback（safety guard 靜默返回、load 靜默忽略）至少要可觀測
   （trace / event / 回傳值），測試才能抓到。
5. **建置務必限制並行**：`zone_test` / `zone_gd` 一律帶上限（如 `-j4`）。
   曾因無上限 `-j` 同時數百個編譯程序耗盡 RAM 觸發 OOM 強制重啟——這是測試/CI 基礎設施的鐵則。
