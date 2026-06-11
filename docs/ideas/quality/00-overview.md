# quality ideas 總覽與路線圖

> 日期：2026-06-11
> 定位：docs/ideas/quality/ 目錄的索引——測試、品質與開發工具各主題檔的關係、優先序總表、建議推進順序。

---

## 1. 這個目錄是什麼

gamecore-zone 目前的品質基線：ctest 49 cases / 168 assertions 全綠（doctest）、
godot-mono headless `verify.gd` VERIFY PASSED（三排程器模式 + cast + skill）、主場景 headless 可跑通。
但「全綠」不等於「夠用」——本目錄從**測試策略、回歸防護、CI、效能基準、debug 工具**
五個面向盤點缺口並記錄前瞻想法，作為日後 brainstorming → spec → plan 的素材庫。

這些是**想法，不是定案 spec**。動工前仍應走 brainstorming 收斂成
`docs/superpowers/specs/` 的正式設計文件。

## 2. 檔案地圖

| 檔案 | 主題 | 一句話 |
|---|---|---|
| `01-test-coverage.md` | 測試覆蓋盤點 | 現有 49 案覆蓋什麼、缺什麼——含序列化元件清單的真實缺口 |
| `02-scheduler-equivalence.md` | 排程器等價性 | 同輸入下 A/B/C 應守的不變量，跨模式差分測試 |
| `03-property-simulation.md` | property / 模擬測試 | N 回合隨機指令長跑、守恆類不變量、fuzz 思路 |
| `04-headless-verify.md` | headless 驗證制度化 | verify.gd 假通過教訓——「斷言世界真的變了」的寫法準則 |
| `05-data-regression.md` | 資料回歸防護 | actions.json schema 驗證、load_defaults 後備、存檔版本演進 |
| `06-ci.md` | CI 構想 | GitHub Actions 跑 ctest + headless godot 的可行性與快取策略 |
| `07-benchmark.md` | 效能基準 | 大量 actor 下 A/B/C 排程器成本、trace 開銷量測 |
| `08-debug-roadmap.md` | debug 工具路線 | 種子統一、指令重播、狀態雜湊、結構化 debug dump |

## 3. 優先序總表（高優先項彙整）

| 想法 | 出處 | 為什麼先做 |
|---|---|---|
| 補 `AllComponents` 序列化缺口 + 完整 round-trip 測試 | 01 §3 | **已知真缺口**：存檔不含 Energy/OngoingAction/TimedEffects，channel 中存檔必然遺失狀態 |
| verify.gd 斷言準則（消滅殘存 print-only 檢查） | 04 §2 | 假通過已抓到一次真 bug；目前仍有四處只印不驗 |
| 排程器跨模式差分測試（weight=1 時 A≡B） | 02 §2 | 三排程器是核心賣點，等價性沒測過就只是「三份各自全綠」 |
| 隨機指令長跑模擬（不崩潰 + 基本不變量） | 03 §2 | restart 雙重 destroy、DoT 自殺越界都是這類 bug；用模擬提前撈 |
| GitHub Actions：ctest job | 06 §2 | 成本最低的回歸護欄，先擋住 core 層 |
| RNG 種子統一（WorldSeed） | 08 §2 | 重播/差分測試/長跑重現的共同前置；現在三處種子策略不一致 |

## 4. 建議推進順序（三波）

**第一波（堵已知漏洞，彼此獨立可並行）**
1. 序列化缺口：`AllComponents` 補元件 + 全狀態 round-trip 測試（01 §3）
2. verify.gd 斷言化改寫 + `_expect_changed` helper（04 §2-3）
3. CI 第一階段：ubuntu ctest（06 §2）

**第二波（把「三排程器」變成可驗證的承諾）**
4. RNG 種子統一（08 §2）——差分/模擬測試的前置
5. 排程器等價性差分測試（02）
6. 隨機指令長跑模擬 + 不變量檢查（03）

**第三波（工具與量化）**
7. CI 第二階段：headless godot job（06 §3）
8. scheduler benchmark + trace 開銷量測（07）
9. 指令重播 + 狀態雜湊（08 §3-4）

## 5. 共用依賴速查（實際檔案路徑）

- 測試：`tests/src/`（doctest v2.4.11，FetchContent）、`tests/CMakeLists.txt`
  （`ZONE_TEST_DATA_DIR` 指向源碼 `data/`）
- 排程器：`src/core/turn/turn_scheduler.h` + `energy_instant / energy_channel /
  tick_remaining_scheduler.cpp`、`make_scheduler.cpp`
- 序列化：`src/core/serialize/all_components.h`（**元件清單真相層**）、`save_load.h`、`save_store.h`
- 世界與 hook：`src/core/turn/turn_world.h`（`TurnConfig`、`on_actor_turn`、`trace`）
- 資料：`data/actions.json`、`src/core/turn/action_def.h/.cpp`（`load_json` / `load_defaults`）
- 前端：`src/gbind/zone_world_gd.cpp`（mapgen RNG、`turn_rng_`、trace ring）、
  `godot_zone/verify.gd`、`godot_zone/map_view.gd`
- 既有 RNG 三處：`zone_world_gd.cpp:145`（`std::random_device`）、
  `npc_ai_system.cpp:17`（static 種子 42）、`zone_world_gd.h:110`（`turn_rng_{0xC0FFEE}`）

## 6. 不變原則（所有想法都必須遵守）

1. **core 先行**：能在 `zone_core` 純 C++ 測的就不要依賴 godot headless；
   headless 只驗「接線」，不驗「邏輯」。
2. **斷言世界狀態，不斷言流程跑完**：「沒報錯」與「印出來了」都不是通過條件，
   通過條件是「狀態如預期地改變」（verify.gd 假通過教訓的核心）。
3. **決定性優先**：所有測試與模擬必須可用固定種子重現；新增隨機性一律走可注入的 RNG。
4. **失敗要大聲**：silent fallback（safety guard 靜默返回、load 靜默忽略）至少要可觀測
   （trace / event / 回傳值），測試才能抓到。
5. **三排程器同權**：新測試基礎設施（模擬器、差分、benchmark）一律以
   `SchedulerMode` 參數化，三模式同跑。
