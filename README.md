# MednGameCore — gamecore-zone（區域層 Area Layer）

gamecore 三層架構（World / Region / Area）中的第三層——**區域層（Area Layer）**：
以 **C++/EnTT ECS 核心** 搭配 **Godot 4.6 GDExtension（mono）前端**，實作一個**最傳統的回合制**地城探索原型。

> 2026-06-21 回合系統重構後，本專案刻意收斂為「教科書式回合制」（`TurnEngine`），
> 作為乾淨的基礎地基。能量制／先攻／詠唱／技能／狀態效果等已移除，重新定位為未來可選的擴充主題（見 `workflows/idea/`）。

## 現有功能

- **TurnEngine 回合制**（`src/core/turn/turn_engine.{h,cpp}`）：維護一條 actor 清單（順序＝加入序，hero 最先行動），`begin_round()` 後反覆 `step()` 直到回合結束；玩家 actor 阻塞等 `submit()`，NPC 立即用 `decide()` 結算。
- **apply_action**（`src/core/turn/apply_action.{h,cpp}`）：把單一 `Action`（Idle / Move / Wait）結算到世界——撞牆、**撞擊攻擊**（移動進入有 actor 的格子即攻擊，無獨立 Attack 動作）、自動拾取道具（health_potion 回血）、踩到下樓梯換層。
- **NPC AI**：`decide_npc`（`src/core/turn/npc_ai.*`）——LOS 視野警覺、`alert_memory` 記憶、相鄰即撞擊攻擊、追擊/徘徊皆避開牆與其他 actor。
- **FOV**（`src/core/systems/fov_system.*`）視野計算。
- **勝利條件與成長**：走下第 5 層（`get_win_floor()`）樓梯即通關；每下一層英雄成長最大 HP／治療／攻擊，否則怪物逐層變強會導致遊戲數學上打不贏。
- **存讀檔 save/load**：以 cereal 序列化整個 registry（`src/core/serialize/`），支援 round-trip。

## 目錄結構速覽

| 路徑 | 內容 |
|------|------|
| `src/core/` | 純 C++/EnTT ECS 核心（godot-free）：`turn/`（回合引擎、動作、`npc_ai.*` NPC 決策）、`components/`、`systems/`（FOV）、`maps/`（BSP 地圖生成）、`serialize/`、`ecs/`、`util/` |
| `src/gbind/` | Godot GDExtension 綁定：`ZoneWorld`（`zone_world_gd.{h,cpp}`），只吐地形旗標與實體資料，不做渲染決策 |
| `godot_zone/` | Godot 專案：主場景 `map_view.tscn`、`map_view.gd`（HUD/輸入）、`dungeon_view.gd`（圖塊繪製）、`assets/`（Kenney 1-Bit Pack 圖塊）、`verify.gd`、`bin/`（編譯產物 `.so`，gitignore） |
| `tests/` | doctest 測試（`tests/src/`） |
| `docs/` | 現役文檔（架構與函數說明、進度說明）、`archive/`（舊 session log） |
| `workflows/` | `idea/`（後續設計主題）、`specs/archive/`、`plans/archive/`（歷史設計記錄／規格） |
| `html/` | 專案導覽網站（總覽／架構／機制／測試驗證） |

## 建置與測試

> **建置鐵則：務必限制並行（如 `-j4`）。** 曾因無上限 `-j` 同時啟動約 541 個編譯程序耗盡 60GiB RAM 觸發 OOM 強制重啟。

```bash
# 設定（含 GDExtension）
cmake -S . -B build -DZONE_BUILD_GDEXTENSION=ON

# 跑測試（doctest：33 test case / 129 assertion 全綠）
cmake --build build --target zone_test -j4
./build/zone_test          # 或 ctest --test-dir build

# 建置 GDExtension（建置後自動把 libzone_gd.so 複製到 godot_zone/bin/）
cmake --build build --target zone_gd -j4
```

CMake 目標：`zone_core`（STATIC，godot-free）／`zone_test`（doctest）／`zone_gd`（SHARED，GDExtension，連結 zone_core + godot-cpp）。

## 在 Godot 跑

```bash
# 首次需先 import 一次（建立 .godot/extension_list.cfg）
godot-mono --headless --path godot_zone --import

# 開遊戲（主場景 map_view.tscn）
godot-mono --path godot_zone

# headless 端到端驗證 → VERIFY PASSED
godot-mono --headless --path godot_zone -s res://verify.gd
```

## 操作按鍵

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
