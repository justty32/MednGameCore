# Build 與依賴策略

> 日期：2026-06-11
> 定位：盤點 gamecore-zone 走向長期維護時，CMake、FetchContent、godot-cpp、測試與建置時間的取捨。

---

## 1. 現況

目前 core 測試透過 CMake / doctest 跑，GDExtension 綁 Godot 4.6 與 godot-cpp。這個組合已能支撐快速實驗，但依賴策略還偏「能跑即可」。

長期風險：

- godot-cpp 版本升級時 ABI / binding 變動會一次影響整個 extension。
- FetchContent 若每次 clean build 都拉遠端，CI 與新機器重建時間不穩。
- 測試資料目錄、Godot 執行檔路徑、mono/headless 參數容易因機器而異。

## 2. godot-cpp 版本策略（優先序：中）

建議：

1. 以 Git submodule 或固定 commit 的 FetchContent 鎖版。
2. 在 `docs/` 記錄 Godot editor/headless 版本與 godot-cpp commit。
3. binding API 變更集中在 `src/gbind/`，core 不 include Godot header。

升級流程：

```text
開新 branch -> 升 godot-cpp -> build extension -> headless verify -> 手動開主場景 -> commit
```

## 3. FetchContent 與離線性

短期可保留 FetchContent，但 CI 應加快取：

- CMake build dir cache：依 `CMakeLists.txt`、compiler、godot-cpp commit key。
- doctest 這類小依賴可 vendor 或固定 tag。
- 若新增 nlohmann/json，優先 header-only 固定 tag，不引入大型套件。

## 4. 建置目標分層

建議 CMake 目標保持三層：

| target | 含義 |
|---|---|
| `zone_core` | 純 C++，無 Godot，所有邏輯測試依賴它 |
| `zone_tests` | doctest，吃 `zone_core` 與 `data/` |
| `zone_gd` | GDExtension binding，唯一可 include Godot header 的層 |

任何新功能若必須先碰 `zone_gd` 才能測，就代表邊界可能走歪。

## 5. 開發體驗工具

可補幾個薄 script 或 CMake preset：

- `cmake --preset dev`
- `cmake --build --preset dev`
- `ctest --preset dev`
- `godot --headless -s res://verify.gd`

重點是把本機命令固定下來，避免 session log 裡反覆手打不同參數。

## 6. 不做的事

- 不引入重量級 package manager，只為少量 header-only 依賴增加心智成本。
- 不讓 Godot 專案反向驅動 core build；core 必須能單獨建置與測試。
- 不追最新版 Godot。遊戲專案需要穩定版，而不是每次升級都重付 binding 成本。
