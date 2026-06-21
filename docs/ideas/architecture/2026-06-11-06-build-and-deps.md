# Build 與依賴策略

> 日期：2026-06-11（原稿）／狀態更新：2026-06-21
> 定位：盤點 gamecore-zone 走向長期維護時，CMake、FetchContent、godot-cpp、測試與建置時間的取捨。

> [!IMPORTANT]
> **2026-06-21 重構後校正**：建置現況數字已更新——doctest **29 個 test case / 93 個 assertion**
> 全綠（舊文常引用的「JSON 技能 / 三排程器」相關測試已隨重構移除）。CMake 三 target 命名為
> `zone_core` / `zone_test` / `zone_gd`。新增兩條對維護至關重要的事實：**`zone_gd` 建置後
> POST_BUILD 自動把 `.so` 複製到 `godot_zone/bin/`**，以及**建置必須限制並行（OOM 鐵則）**。

---

## 1. 現況（2026-06-21）

core 測試透過 CMake / doctest 跑（**29 test case / 93 assertion 全綠**），GDExtension 綁
Godot 4.6（mono）與 godot-cpp。這個組合已能支撐快速實驗，但依賴策略仍偏「能跑即可」。

長期風險：

- godot-cpp 版本升級時 ABI / binding 變動會一次影響整個 extension。
- FetchContent 若每次 clean build 都拉遠端，CI 與新機器重建時間不穩。
- 測試資料目錄、Godot 執行檔路徑、mono/headless 參數容易因機器而異。

## 2. 建置鐵則（必讀，血淚教訓）

> [!CAUTION]
> **建置務必限制並行度。** 例如 `cmake --build build --target zone_test -j4`。曾因無上限 `-j`
> 同時啟動約 541 個編譯程序，耗盡 60GiB RAM 觸發 **OOM 強制重啟**。godot-cpp 的 binding
> 生成檔數量龐大，預設無限並行會直接打爆記憶體。任何 build 指令都要顯式帶 `-jN`（建議 ≤4）。

> [!NOTE]
> **`zone_gd` 建置後自動同步 `.so` 到 `godot_zone/bin/`。** `zone_gd` target 設有 POST_BUILD
> `copy_if_different`，把 `libzone_gd.so`（及對應平台的 `.dll`）複製到 `godot_zone/bin/`。
> 該目錄已 gitignore，故此自動複製是「改完 C++ 重 build 後 Godot 端立即拿到新二進位」的關鍵，
> 免去手動 copy（手動同步遺漏曾是除錯陷阱，見 07）。

## 3. godot-cpp 版本策略（優先序：中）

建議：

1. 以 Git submodule 或固定 commit 的 FetchContent 鎖版。
2. 在 `docs/` 記錄 Godot editor/headless 版本（現為 Godot 4.6 mono）與 godot-cpp commit。
3. binding API 變更集中在 `src/gbind/`，core 不 include Godot header。

升級流程：

```text
開新 branch -> 升 godot-cpp -> build extension(-jN) -> headless verify -> 手動開主場景 -> commit
```

## 4. FetchContent 與離線性

短期可保留 FetchContent，但 CI 應加快取：

- CMake build dir cache：依 `CMakeLists.txt`、compiler、godot-cpp commit key。
- doctest 這類小依賴可 vendor 或固定 tag。

## 5. 建置目標分層

CMake 目標保持三層：

| target | 含義 |
|---|---|
| `zone_core` | 純 C++ STATIC，無 Godot，所有邏輯測試依賴它 |
| `zone_test` | doctest（吃 `zone_core`），29 case / 93 assertion |
| `zone_gd` | GDExtension binding SHARED（連結 zone_core + godot-cpp），唯一可 include Godot header 的層；POST_BUILD 複製 .so 到 godot_zone/bin/ |

任何新功能若必須先碰 `zone_gd` 才能測，就代表邊界可能走歪（見 03 的純 core 紀律）。

## 6. 開發體驗工具

可補幾個薄 script 或 CMake preset，把本機命令固定下來：

- `cmake --build build --target zone_test -j4`（**務必帶 `-jN`**）
- `ctest`（或直接跑 zone_test 執行檔）
- `godot-mono --headless --import`（首次建立 `.godot/extension_list.cfg`）
- `godot-mono --headless -s res://verify.gd`（→ VERIFY PASSED）
- `godot-mono --path godot_zone`（開主場景 `res://map_view.tscn`）

## 7. 不做的事

- 不引入重量級 package manager，只為少量依賴增加心智成本。
- 不讓 Godot 專案反向驅動 core build；core 必須能單獨建置與測試。
- 不追最新版 Godot。遊戲專案需要穩定版，而不是每次升級都重付 binding 成本。
- **不用無上限 `-j`。** 見 §2。
