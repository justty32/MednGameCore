# data-driven ideas 總覽與路線圖

> 日期：2026-06-11
> 定位：docs/ideas/data-driven/ 目錄的索引——資料驅動、內容管線與模組化各主題檔的關係、優先序總表、建議推進順序。

---

## 1. 這個目錄是什麼

gamecore-zone 的資料化現況是「Q1 終點」：ActionDef 可組合原語（do_move/nova_damage/dot/
self_heal + radius/dot_kind/weight）由 `data/actions.json` 經 cereal JSONInputArchive 載入，
失敗走 `load_defaults()` 後備；其餘內容（怪物屬性、地圖 tile、生成參數、NPC 決策、
訊息字串）仍寫死在 C++。

本目錄記錄**下一階段「資料驅動化」**的前瞻設計想法——目標是讓「加內容」逐步從
「改 C++ 重編譯」變成「改 JSON 重載」，並為日後 modding 鋪路。
與 `docs/ideas/gameplay/` 是姊妹目錄：gameplay 談「做什麼機制」，
本目錄談「機制的參數與內容放哪裡、怎麼載入、怎麼演進」。

這些是**想法，不是定案 spec**。動工前仍應走 brainstorming 收斂成
`docs/superpowers/specs/` 的正式設計文件。

## 2. 檔案地圖

| 檔案 | 主題 | 一句話 |
|---|---|---|
| `01-actions-schema.md` | actions.json schema 演進 | 版本欄位、驗證與錯誤訊息、原語擴充 vs 表達式/腳本的取捨 |
| `02-entity-prototypes.md` | 實體原型資料化 | 怪物種類/屬性/AI 參數搬進 prototypes.json；PrototypeManager 以新形態回歸 |
| `03-map-procgen-data.md` | 地圖與 procgen 資料化 | terrain def 表、BSP 生成參數、樓層主題 |
| `04-npc-ai-spectrum.md` | NPC AI 資料化光譜 | 參數化 decide_chase → utility 權重表 → 行為樹的成本效益 |
| `05-save-compat.md` | 存檔相容性策略 | save header 版本標記、AllComponents 變動規約、migration |
| `06-modding-pipeline.md` | modding 願景 | 資料目錄結構、載入順序/覆寫規則、開發期熱重載 |
| `07-localization.md` | 本地化考量 | trace/事件字串從程式碼抽出的時機與分層 |

## 3. 優先序總表（高優先項彙整）

| 想法 | 出處 | 為什麼先做 |
|---|---|---|
| schema_version 欄位 + 載入錯誤訊息（取代 `catch(...)` 吞掉） | 01 | 成本最低、所有後續資料檔的地基；現在 silent fallback 會掩蓋 typo |
| ActionLibrary 載後語意驗證（範圍檢查、重名檢查） | 01 | JSON 增容前必備，否則壞資料只能靠跑起來才發現 |
| PrototypeDef + prototypes.json（怪物 hp/atk/speed/is_caster/skill） | 02 | spawn 寫死在 `zone_world_gd.cpp`，是目前最大塊的硬編內容；且解開 core 測試無法產生「標準怪」的問題 |
| save header（magic + 格式版本 + 遊戲版本） | 05 | 趁存檔還沒人持有，現在加一行，未來省一個 migration 地獄 |
| 開發期 reload：`reload_data()` GDExtension 方法 | 06 | 調技能數值免重編譯免重啟，直接加速所有 gameplay 迭代 |

## 4. 建議推進順序（三波）

**第一波（地基，小而硬）**
1. actions.json 加 `schema_version` + 載入錯誤回報（01 §2-3）
2. save header 版本標記（05 §2）——與 1 同週可完成
3. `reload_data()` 熱重載（06 §4）——只重建 ActionLibrary，立即受益

**第二波（內容搬家）**
4. PrototypeDef + prototypes.json，spawn 改查表（02）
5. terrain def 表 + 生成參數 JSON 化（03 §2-3）
6. NPC AI 參數化（04 §2，ChaseParams 進 prototype）

**第三波（架構升級，按需）**
7. utility 權重表 AI（04 §3）——等技能/狀態夠多才有意義
8. mod 目錄 + 覆寫規則（06 §2-3）——等資料檔種類穩定
9. 訊息字串抽出（07）——等 UI 方向確定（trace 是 debug 用，不急）

## 5. 共用依賴速查（實際檔案路徑）

- 資料載入現況：`src/core/turn/action_def.h`/`.cpp`（ActionLibrary、cereal JSON）、
  `data/actions.json`
- spawn 硬編處：`src/gbind/zone_world_gd.cpp`（NPC_CAP/HP/ATK 常數、is_caster 半數、
  speed_mod 80/130、物品機率）
- 地圖：`src/core/maps/tile.h`（flags）、`map_data.h`、`map_gen.cpp`（BSP 常數）
- NPC AI：`src/core/turn/npc_brain.h`/`.cpp`（decide_chase）、
  `src/core/components/npc_ai_component.h`
- 序列化：`src/core/serialize/save_load.h`（AllComponents fold）、`save_store.h`
  （FolderSaveStore）、`all_components.h`
- trace/事件：`src/core/turn/turn_world.h`（trace sink）、`zone_effects.cpp`（中文字串）、
  `zone_event.h`
- 設計真相層：`docs/superpowers/specs/2026-06-03-gamecore-zone-design.md`、
  `2026-06-07-action-and-turn-scheduler-design.md`

## 6. 不變原則（所有想法都必須遵守）

1. **資料挑原語，C++ 寫行為**：JSON 只描述「用哪個原語、什麼參數」；行為邏輯
   保持 C++ 可測試原語（沿用 ActionDef 的成功模式）。腳本語言是最後手段（01 §5）。
2. **core 先行**：載入器/驗證器放 `src/core/`，ctest 直接喂字串測；GDExtension 只接線。
3. **後備不沉默**：每個 `load_defaults()` 式 fallback 都必須吐 warning（經 trace 或回傳
   錯誤字串），不准 `catch(...)` 靜默吞掉。
4. **版本欄位先於內容增容**：任何資料檔在欄位變多之前，先加 `schema_version`。
5. **存檔只存狀態，不存定義**：實體存 prototype id + 差異，不把 def 內容塞進存檔
   （02 §5、05 §4）。
