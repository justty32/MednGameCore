# ideas — 想法入口

放不確定要不要做、還沒承諾的想法。這裡可以粗糙，但要能回頭理解。

## 規則

- 開始前寫 `Done when: <想法被保存到可回頭理解的程度>`；brainstorm 不需要完整驗證。
- 一個想法成熟後，若確定會做但不知何時，移到 [roadmap](../roadmap/README.md)。
- 需要認真討論架構時，升到 [specs](../specs/README.md)。
- 已放棄的想法可以移到 `archive/` 或標記為 dropped。

## 校正說明

以下按分類列出的檔案（`architecture/`、`data-driven/`、`frontend/`、`gameplay/`、`quality/`，共 38 篇），多數寫於 2026-06-11 前後的「三種可切換回合排程器（A/B/C）＋ 資料驅動技能／DoT／能量制」實驗期。2026-06-21 該整套系統被移除、回合系統重構為最精簡的傳統回合制 `TurnEngine` 後，已逐篇加註校正說明。**閱讀時請以各檔開頭的 `[!IMPORTANT]`／狀態更新區塊為準**，內文其餘部分可能仍描述已被取代的舊設計。

## Ideas

### architecture/

| 檔案 | 一句話主題 |
|---|---|
| [architecture/2026-06-11-00-overview.md](architecture/2026-06-11-00-overview.md) | gamecore-zone 從「實驗沙盒」走向「三層架構正式區域層」的工程演進路線圖索引 |
| [architecture/2026-06-11-01-scheduler-convergence.md](architecture/2026-06-11-01-scheduler-convergence.md) | 原為三排程器收斂到 B 的提議，現重定位為「若未來重新引入先攻／能量制」的設計參考 |
| [architecture/2026-06-11-02-multi-actor-orders.md](architecture/2026-06-11-02-multi-actor-orders.md) | 多角色 order 表模型，落地到 TurnEngine 的 actor list + ControllerComponent(Faction) 之上 |
| [architecture/2026-06-11-03-ecs-and-boundaries.md](architecture/2026-06-11-03-ecs-and-boundaries.md) | 壓低新增 component 的邊際成本，並把 ZoneWorld god class 拆回薄綁定層 |
| [architecture/2026-06-11-04-gdextension-interface.md](architecture/2026-06-11-04-gdextension-interface.md) | 把 ZoneWorld 對 Godot 的介面從 debug 字串與整圖重繪，演進成結構化資料介面 |
| [architecture/2026-06-11-05-layer-integration.md](architecture/2026-06-11-05-layer-integration.md) | gamecore-zone 如何從單一區域沙盒，接回更大尺度的 Region/World 層 |
| [architecture/2026-06-11-06-build-and-deps.md](architecture/2026-06-11-06-build-and-deps.md) | 盤點長期維護時 CMake、FetchContent、godot-cpp、測試與建置時間的取捨 |
| [architecture/2026-06-11-07-tech-debt.md](architecture/2026-06-11-07-tech-debt.md) | 把目前 gamecore-zone 的已知債務列成可排序清單 |

### data-driven/

| 檔案 | 一句話主題 |
|---|---|
| [data-driven/00-overview.md](data-driven/00-overview.md) | data-driven ideas 目錄索引——資料驅動、內容管線與模組化各主題的關係與優先序 |
| [data-driven/01-actions-schema.md](data-driven/01-actions-schema.md) | 若未來重新引入資料驅動動作/技能，schema 該長怎樣——版本化、驗證、原語擴充路線 |
| [data-driven/02-entity-prototypes.md](data-driven/02-entity-prototypes.md) | 把怪物種類/屬性/AI 參數從 spawn 程式碼搬進 prototypes.json（仍為有效未來方向） |
| [data-driven/03-map-procgen-data.md](data-driven/03-map-procgen-data.md) | 把 terrain、BSP 生成參數、樓層主題從 C++ 常數逐步搬到資料檔（仍為有效未來方向） |
| [data-driven/04-npc-ai-spectrum.md](data-driven/04-npc-ai-spectrum.md) | 從現有 decide_chase 出發，規劃 NPC AI 如何逐步參數化（仍為有效未來方向） |
| [data-driven/05-save-compat.md](data-driven/05-save-compat.md) | 趁存檔尚未成為玩家資產前，先定義 header、版本與 migration 原則 |
| [data-driven/06-modding-pipeline.md](data-driven/06-modding-pipeline.md) | 遠期構想：讓 prototypes、map themes 等外部資料包覆寫，同時保持可驗證與可除錯 |
| [data-driven/07-localization.md](data-driven/07-localization.md) | 記錄何時、如何把 trace / combat log / UI 字串從程式碼抽出（遠期方向，受重構影響小） |

### frontend/

| 檔案 | 一句話主題 |
|---|---|
| [frontend/00-overview-roadmap.md](frontend/00-overview-roadmap.md) | godot_zone/ 前端從「最簡回合制輸入台」演進為「可玩的 roguelike 介面」的路線索引 |
| [frontend/01-rendering-tilemap-camera-fov.md](frontend/01-rendering-tilemap-camera-fov.md) | 把整圖重繪升級為節點化渲染——地形 TileMapLayer、actor Sprite2D、Camera2D、FOV |
| [frontend/02-event-animation-pipeline.md](frontend/02-event-animation-pipeline.md) | 讓 ZoneEvent 驅動移動補間、受擊閃爍、傷害數字的事件佇列 + 動畫播放器模型 |
| [frontend/03-hud-structured-ui.md](frontend/03-hud-structured-ui.md) | 把單一 debug Label 演進成可玩的 roguelike HUD，同時保留開發期可觀測性 |
| [frontend/04-input-architecture.md](frontend/04-input-architecture.md) | 現況輸入模型（`_unhandled_input` → player_move/player_wait）與 Input Map 演進路線 |
| [frontend/05-targeting-cursor-mode.md](frontend/05-targeting-cursor-mode.md) | 為未來引入需選目標的動作，設計前端選目標流程（方向/游標/範圍預覽） |
| [frontend/06-scheduler-visualization.md](frontend/06-scheduler-visualization.md) | 原為三排程器時間狀態轉 UI 回饋的設計，現況不適用（排程器已移除） |
| [frontend/07-debug-developer-ui.md](frontend/07-debug-developer-ui.md) | 保留早期 debug dump 的效率，同時讓正式 HUD 不被開發資訊淹沒 |

### gameplay/

| 檔案 | 一句話主題 |
|---|---|
| [gameplay/00-overview.md](gameplay/00-overview.md) | gameplay ideas 目錄索引——各主題檔的關係、優先序總表、建議推進順序 |
| [gameplay/01-combat-depth.md](gameplay/01-combat-depth.md) | 把「attack 直減 hp」的扁平戰鬥，升級成有變異性與 build 空間的數值層 |
| [gameplay/02-skill-targeting-resources.md](gameplay/02-skill-targeting-resources.md) | 技能系統如何蓋在最簡 TurnEngine 的 Action/apply_action 之上——形狀、代價、成長 |
| [gameplay/03-status-effects.md](gameplay/03-status-effects.md) | 若重新引入狀態效果，設計成任何 buff/debuff 都是一筆 TimedEffect 的通用框架 |
| [gameplay/04-items-equipment.md](gameplay/04-items-equipment.md) | 拾取雛形（移動踩格觸發）演進為背包、裝備欄、消耗品三層的路線圖 |
| [gameplay/05-character-growth.md](gameplay/05-character-growth.md) | 為戰鬥公式、技能升級、裝備欄建立角色長期成長模型，避免太早導入等級膨脹 |
| [gameplay/06-party-tactics.md](gameplay/06-party-tactics.md) | 讓玩家操控多名 actor 產生真正的戰術差異，而不是多個 hero 輪流走路 |
| [gameplay/07-scheduler-synergy.md](gameplay/07-scheduler-synergy.md) | 盤點若未來引入能量/速度/詠唱時間擴充，哪些玩法會因此更有趣 |

### quality/

| 檔案 | 一句話主題 |
|---|---|
| [quality/00-overview.md](quality/00-overview.md) | quality ideas 目錄索引——測試、品質與開發工具各主題檔的關係與優先序 |
| [quality/01-test-coverage.md](quality/01-test-coverage.md) | 現有 doctest 29 cases / 93 assertions 覆蓋什麼、缺什麼的測試案清單 |
| [quality/02-scheduler-equivalence.md](quality/02-scheduler-equivalence.md) | 決定性回放測試（原「排程器等價性」）：同 seed 同指令序列 → 同世界 hash |
| [quality/03-property-simulation.md](quality/03-property-simulation.md) | 用隨機但可重現的長跑場景，捕捉單元測試很難列舉的交互 bug |
| [quality/04-headless-verify.md](quality/04-headless-verify.md) | 把 godot_zone/verify.gd 從「跑完沒錯」提升為「斷言接線真的有效」 |
| [quality/05-data-regression.md](quality/05-data-regression.md) | 避免資料檔（地圖主題、存檔、未來資料驅動內容）的 typo 或 schema 演進在執行期才爆 |
| [quality/06-ci.md](quality/06-ci.md) | 用最小成本建立回歸護欄，先保 core，再接 Godot headless |
| [quality/07-benchmark.md](quality/07-benchmark.md) | 在大量 actor、trace 開關下，對 TurnEngine 推進建立可重複的效能量測 |
| [quality/08-debug-roadmap.md](quality/08-debug-roadmap.md) | 讓隨機 bug、決定性回放、長跑模擬可以重現、縮小、比對 |

## 何時不用

- 已確定要做，走 roadmap/spec/plan。
- 是外部資料整理，走 research。
- 是立即可做的小修，直接做。
