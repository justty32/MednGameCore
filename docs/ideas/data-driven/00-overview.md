# data-driven ideas 總覽與路線圖

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：docs/ideas/data-driven/ 目錄的索引——資料驅動、內容管線與模組化各主題檔的關係、優先序總表、建議推進順序。

> **狀態更新（2026-06-21）**：回合系統已整體重構為最精簡的傳統回合制引擎
> `TurnEngine`。原本的「ActionDef/ActionLibrary 可組合原語 ＋ `data/actions.json`
> JSON 技能 ＋ DoT 持續效果 ＋ EffectRegistry/nova」整套**已移除**。
> 因此本目錄談的「資料驅動技能/動作」目前**並不存在於程式**，全部重新定位為
> 「建立在最簡 TurnEngine／apply_action 之上的**未來可選擴充**」。下文已校正現況描述。

---

## 1. 這個目錄是什麼

gamecore-zone 的資料化現況**很低**：2026-06-21 重構後，Action 退回到極簡
enum（`Idle`/`Move`/`Wait`，攻擊＝撞擊），不再有 ActionDef/ActionLibrary，也不再有
`data/actions.json` JSON 技能（fireball/heal/meteor/venom 皆已刪除）。目前唯一仍帶
「程序生成」色彩的是**地圖**（`map_gen.cpp` 的 BSP 地洞）與一種道具
`health_potion`；怪物屬性、生成參數、NPC 決策、訊息字串全部寫死在 C++。

本目錄記錄**「資料驅動化」**的前瞻設計想法——目標是讓「加內容」逐步從
「改 C++ 重編譯」變成「改 JSON 重載」，並為日後 modding 鋪路。
其中「資料驅動動作/技能」要在現有 TurnEngine／apply_action 之上**重新引入**，
不是恢復舊系統（舊系統綁在已移除的能量排程器上）。
與 `docs/ideas/gameplay/` 是姊妹目錄：gameplay 談「做什麼機制」，
本目錄談「機制的參數與內容放哪裡、怎麼載入、怎麼演進」。

這些是**想法，不是定案 spec**。動工前仍應走 brainstorming 收斂成
`docs/superpowers/specs/` 的正式設計文件。

## 2. 檔案地圖

| 檔案 | 主題 | 一句話 |
|---|---|---|
| `01-actions-schema.md` | actions.json schema 設計（未實作） | 若要**重新引入**資料驅動動作/技能，schema 該長怎樣：版本欄位、驗證與錯誤訊息、原語擴充 vs 表達式/腳本的取捨 |
| `02-entity-prototypes.md` | 實體原型資料化 | 怪物種類/屬性/AI 參數搬進 prototypes.json；PrototypeManager 以新形態回歸 |
| `03-map-procgen-data.md` | 地圖與 procgen 資料化 | terrain def 表、BSP 生成參數、樓層主題 |
| `04-npc-ai-spectrum.md` | NPC AI 資料化光譜 | 參數化 decide_chase → utility 權重表 → 行為樹的成本效益 |
| `05-save-compat.md` | 存檔相容性策略 | save header 版本標記、AllComponents 變動規約、migration |
| `06-modding-pipeline.md` | modding 願景 | 資料目錄結構、載入順序/覆寫規則、開發期熱重載 |
| `07-localization.md` | 本地化考量 | trace/事件字串從程式碼抽出的時機與分層 |

## 3. 優先序總表（高優先項彙整）

> 注意：下表中與「動作/技能」相關的 ActionLibrary/actions.json 項目，現在是
> **從零重新引入**（舊系統已隨能量排程器一併移除），不是修補既有程式。

| 想法 | 出處 | 為什麼值得做 |
|---|---|---|
| PrototypeDef + prototypes.json（怪物 hp/atk/speed/ai/controller） | 02 | spawn 寫死在 `zone_world_gd.cpp` 的 `setup_map()`，是目前最大塊的硬編內容；且解開 core 測試無法產生「標準怪」的問題 |
| save header（magic + 格式版本 + 遊戲版本） | 05 | 趁存檔還沒人持有，現在加一行，未來省一個 migration 地獄 |
| terrain def 表 + BSP 生成參數 JSON 化 | 03 | 地圖已是程序生成，把常數抽成資料即可加速樓層節奏調整 |
| （未來）重新引入資料驅動動作：schema_version + 載入錯誤回報 | 01 | 動作系統若再資料化，這是地基；目前 Action 還是 enum，尚無 actions.json |

## 4. 建議推進順序（三波）

> 此順序以「**最簡 TurnEngine 為地基**」重排：先做與動作系統解耦的內容資料化
> （原型、地圖、存檔），動作/技能資料化是更後面、需要先擴充 apply_action 的工作。

**第一波（地基，小而硬，與動作系統無關）**
1. save header 版本標記（05 §2）
2. PrototypeDef + prototypes.json，spawn 改查表（02）——內含 ControllerKind / AI 種類
3. terrain def 表 + BSP 生成參數 JSON 化（03 §2-3）

**第二波（內容搬家）**
4. spawn table + 樓層 cap 資料化（02 §4）
5. NPC AI 參數化（04 §2，ChaseParams 進 prototype）

**第三波（架構升級，按需）**
6. **重新引入資料驅動動作/技能**：先在 apply_action 上加可組合效果原語，
   再加 actions.json + schema_version + 驗證（01）——這是最大塊、最晚做的
7. utility 權重表 AI（04 §3）——等技能/狀態夠多才有意義
8. mod 目錄 + 覆寫規則（06 §2-3）——等資料檔種類穩定
9. 訊息字串抽出（07）——等 UI 方向確定（trace 是 debug 用，不急）

## 5. 共用依賴速查（實際檔案路徑）

- 動作/結算現況：`src/core/turn/action.h`（Action = enum Idle/Move/Wait）、
  `src/core/turn/apply_action.h`/`.cpp`（apply_action 結算、decide_chase）、
  `src/core/turn/turn_engine.h`/`.cpp`（TurnEngine、EngineCtx、StepResult）。
  **（已移除：`action_def.*`、`data/actions.json`、`zone_effects.*`、各排程器）**
- spawn 硬編處：`src/gbind/zone_world_gd.cpp` 的 `setup_map()`
  （NPC_CAP/HP/ATK 常數、ControllerKind、物品機率；目前無施法者）
- 地圖：`src/core/maps/tile.h`（flags）、`map_data.h`、`map_gen.cpp`（BSP 常數）
- NPC AI：`src/core/turn/apply_action.h`/`.cpp`（`decide_chase`，最精簡追擊）、
  `src/core/components/npc_ai_component.h`、`controller_component.h`（操控分派）
- 序列化：`src/core/serialize/save_load.h`（AllComponents fold）、`save_store.h`
  （FolderSaveStore）、`all_components.h`（含 ControllerComponent + ActPointComponent）
- 事件：`src/core/turn/zone_event.h`（ZoneEvent，apply_action 產出）；
  trace 經 `EngineCtx.trace` sink
- 設計真相層：`docs/superpowers/specs/2026-06-03-gamecore-zone-design.md`
  （注意：`2026-06-07-action-and-turn-scheduler-design.md` 所設計的能量排程器
  已於 2026-06-21 重構整體移除，僅作歷史記錄）

## 6. 不變原則（所有想法都必須遵守）

1. **資料挑原語，C++ 寫行為**：JSON 只描述「用哪個原語、什麼參數」；行為邏輯
   保持 C++ 可測試原語。腳本語言是最後手段（01 §5）。
2. **core 先行**：載入器/驗證器放 `src/core/`，doctest 直接喂字串測；GDExtension 只接線。
3. **後備不沉默**：每個 `load_defaults()` 式 fallback 都必須吐 warning（經 trace 或回傳
   錯誤字串），不准 `catch(...)` 靜默吞掉。
4. **版本欄位先於內容增容**：任何資料檔在欄位變多之前，先加 `schema_version`。
5. **存檔只存狀態，不存定義**：實體存 prototype id + 差異，不把 def 內容塞進存檔
   （02 §5、05 §4）。
