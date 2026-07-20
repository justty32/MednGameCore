# MednGameCore — gamecore-zone（區域層 Area Layer）

gamecore 三層架構（World / Region / Area）中的第三層——**區域層（Area Layer）**：
以 **C++/EnTT ECS 核心** 搭配 **Godot 4.6 GDExtension（mono）前端**，實作**兩個平行獨立的場景**：
最傳統回合制地城探索原型，以及 SRPG 戰棋戰鬥原型。兩者共用 ECS／地圖／存讀檔基建，
回合驅動各自獨立、互不干涉。開啟遊戲後由主選單（`main_menu.tscn`）選擇進入哪一個。

> 2026-06-21 回合系統重構後，地城探索這條線刻意收斂為「教科書式回合制」（`TurnEngine`），
> 作為乾淨的基礎地基。能量制／先攻／詠唱／技能／狀態效果等已從地城側移除。
> 2026-07-10 新增的 SRPG 戰棋戰鬥（`tactics::Battle`）則是 ToME4 行動值模型的獨立實作，
> 活在自己的 `src/core/tactics/` 模組——**不會**加回 `TurnEngine`。
> 設計記錄：[workflows/specs/2026-07-10-srpg-tactics-battle-design.md](workflows/specs/2026-07-10-srpg-tactics-battle-design.md)。

## 現有功能

### 地城探索

- **TurnEngine 回合制**（`src/core/turn/turn_engine.{h,cpp}`）：維護一條 actor 清單（順序＝加入序，hero 最先行動），`begin_round()` 後反覆 `step()` 直到回合結束；玩家 actor 阻塞等 `submit()`，NPC 立即用 `decide()` 結算。
- **apply_action**（`src/core/turn/apply_action.{h,cpp}`）：把單一 `Action`（Idle / Move / Wait）結算到世界——撞牆、**撞擊攻擊**（移動進入有 actor 的格子即攻擊，無獨立 Attack 動作）、自動拾取道具（health_potion 回血）、踩到下樓梯換層。
- **NPC AI**：`decide_npc`（`src/core/turn/npc_ai.*`）——LOS 視野警覺、`alert_memory` 記憶、相鄰即撞擊攻擊、追擊/徘徊皆避開牆與其他 actor。
- **FOV**（`src/core/systems/fov_system.*`）視野計算。
- **勝利條件與成長**：走下第 5 層（`get_win_floor()`）樓梯即通關；每下一層英雄成長最大 HP／治療／攻擊，否則怪物逐層變強會導致遊戲數學上打不贏。

### SRPG 戰棋戰鬥

- **行動條 `ActionBar`**（`src/core/tactics/action_bar.{h,cpp}`）：ToME4 行動值模型，
  `kCtToAct=1000`；出手後 `ct -= kCtToAct`（不歸零，溢出保留，快角偶爾連動兩次）。
- **可達集**（`src/core/tactics/move_range.{h,cpp}`）：四方向 BFS，牆與其他單位佔格皆阻擋。
- **單位回合子狀態機 `Battle`**（`src/core/tactics/battle.{h,cpp}`）：移動一次＋行動一次，
  攻擊即結束回合；殲滅一方即分勝負。
- **敵方 AI `decide_tactics`**（`src/core/tactics/tactics_ai.{h,cpp}`）：射程內攻擊最脆
  目標，否則移動到「進得了射程但不必貼身」的格子。

### 共用

- **存讀檔 save/load**：以 cereal 序列化整個 registry（`src/core/serialize/`），支援 round-trip。

## 目錄結構速覽

| 路徑 | 內容 |
|------|------|
| `projects/core/src/core/` | 純 C++/EnTT ECS 核心（godot-free）：`turn/`（地城回合引擎、動作、`npc_ai.*` NPC 決策）、`tactics/`（戰棋行動條、可達集、AI、回合子狀態機）、`components/`、`systems/`（FOV）、`maps/`（BSP 地圖生成）、`serialize/`、`ecs/`、`util/` |
| `projects/core/src/gbind/` | Godot GDExtension 綁定：`ZoneWorld`（地城，`zone_world_gd.{h,cpp}`）、`TacticsBattle`（戰棋，`tactics_battle_gd.{h,cpp}`），兩者平行、互不依賴，只吐資料不做渲染決策 |
| `projects/godot_zone/` | Godot 專案：`main_menu.tscn`（main scene，選地城／戰棋）、`map_view.tscn`+`map_view.gd`（地城 HUD/輸入）、`dungeon_view.gd`（地城圖塊繪製）、`tactics_view.tscn`+`tactics_view.gd`（戰棋操作）、`arena_view.gd`（戰棋圖塊繪製）、`tile_atlas.gd`（兩場景共用的圖塊座標）、`assets/`（Kenney 1-Bit Pack 圖塊）、`verify.gd`／`verify_tactics.gd`、`bin/`（編譯產物 `.so`，gitignore） |
| `projects/tests/` | doctest 測試（獨立專案，`projects/tests/src/`，find/link 已建好的 `zone_core`） |
| `docs/` | 現役文檔（架構與函數說明、進度說明）、`archive/`（舊 session log） |
| `workflows/` | `idea/`（後續設計主題）、`specs/`（含本次戰棋設計記錄）、`specs/archive/`、`plans/archive/`（歷史設計記錄／規格） |
| `html/` | 專案導覽網站：`onboarding.html`（接手儀表板／一頁掃完全貌）＋ `index.html` 總覽／`architecture.html` 架構／`mechanics.html` 機制／`testing.html` 測試驗證 |

## 建置與測試

> **建置鐵則：務必限制並行（如 `-j4`）。** 曾因無上限 `-j` 同時啟動約 541 個編譯程序耗盡 60GiB RAM 觸發 OOM 強制重啟。

兩個獨立 C++ 專案，先建核心、再建測試（測試會 find/link 上一步產出的 `zone_core`）：

```bash
# 1) 核心庫 zone_core（godot-free STATIC）
cmake -S projects/core -B projects/core/build
cmake --build projects/core/build -j4

# 2) 測試（doctest：55 test case / 236 assertion 全綠）
cmake -S projects/tests -B projects/tests/build
cmake --build projects/tests/build -j4
./projects/tests/build/bin/zone_test      # 或 ctest --test-dir projects/tests/build

# 3) 選用：GDExtension（建置後自動把 libzone_gd.so 複製到 projects/godot_zone/bin/）
cmake -S projects/core -B projects/core/build -DZONE_BUILD_GDEXTENSION=ON
cmake --build projects/core/build --target zone_gd -j4
```

CMake 目標分屬兩個獨立專案：`projects/core` 產出 `zone_core`（STATIC，godot-free）與選用的 `zone_gd`（SHARED，GDExtension，連結 zone_core + godot-cpp）；`projects/tests` 產出 `zone_test`（doctest，連結已建好的 zone_core）。

## 在 Godot 跑

```bash
# 首次需先 import 一次（建立 .godot/extension_list.cfg）
godot-mono --headless --path projects/godot_zone --import

# 開遊戲（main scene 是 main_menu.tscn：選地城探索或戰棋對戰）
godot-mono --path projects/godot_zone

# headless 端到端驗證 → VERIFY PASSED（地城）
godot-mono --headless --path projects/godot_zone -s res://verify.gd

# headless 端到端驗證 → TACTICS VERIFY PASSED（戰棋）
godot-mono --headless --path projects/godot_zone -s res://verify_tactics.gd
```

## 操作按鍵

### 地城探索

最傳統回合制：按一鍵 ＝ 英雄行動一次 ＋ 本輪所有 NPC 行動一輪。

| 按鍵 | 動作 |
|------|------|
| WASD／方向鍵／數字鍵（KP1-9） | 八方向移動（撞 actor＝攻擊） |
| 按住方向鍵 | 超過 0.5 秒後每 0.08 秒自動走一格；撞牆／攻擊／被攻擊／拾取／下樓／新怪進入視野都會中斷，放開按鍵才能再跑 |
| `.` / KP5 | 原地等待 |
| R | 重開 |
| F5 / F9 | 存檔 / 讀檔（`user://zone_save.bin`） |
| Tab | 開關除錯面板（同時開關 trace；trace 預設關閉，只進緩衝不印 stdout） |
| K | 清除 trace log |
| Esc | 回主選單 |

### 戰棋對戰

一個單位的回合是「移動一次＋行動一次」，攻擊後不能再移動。

| 操作 | 動作 |
|------|------|
| 滑鼠點藍色格 | 移動到該格（僅輪到我方單位、且本回合還沒移動過時可用） |
| 滑鼠點紅框敵人 | 攻擊該單位（攻擊後回合立即結束） |
| 空白鍵 | 待機／結束回合 |
| R | 重開一場 |
| Esc | 回主選單 |
