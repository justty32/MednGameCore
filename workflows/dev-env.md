# dev-env — 開發環境入口

記錄不同機器/環境能做什麼、不能做什麼，以及 fresh clone 後要做的事。

開始前寫 `Done when: <環境能力/缺口/下一步已記錄，必要時 WAIT_USER 已更新>`。

## 環境矩陣

| 環境 | 有什麼 | 能做 | 不能做 |
|------|--------|------|--------|
| local-full | cmake、C++20 編譯器（含網路，FetchContent 可拉依賴）、godot-mono、`../../projects/godot-cpp` checkout | build zone_core/zone_test/zone_gd、跑 `zone_test`/ctest、headless `verify.gd`、Godot 內 GUI 手動驗證 | - |
| local-light（無 godot-mono / 無 godot-cpp checkout） | cmake、C++20 編譯器 | build+跑 `zone_test`（`ZONE_BUILD_GDEXTENSION` 預設 OFF，不需要 godot-cpp） | build `zone_gd`、headless `verify.gd`、GUI 驗證 |
| CI（假設，本 repo 目前未見 CI 設定） | 同 local-light，視是否安裝 godot-mono 而定 | 至少 `zone_test` | GUI 驗證一定不能（無頭環境） |

## Fresh Clone

```bash
# 0) 前置工具：cmake >= 3.14、支援 C++20 的編譯器（GCC/Clang/MSVC）
#    若要建 GDExtension 前端，另需 godot-mono（Godot 4.6 mono 版執行檔）

# 1) 只跑核心測試（不需要 godot-cpp）
cmake -S . -B build
cmake --build build --target zone_test -j4   # 鐵律：一律 -j4，勿無上限並行
./build/zone_test

# 2)（可選）要跑 Godot 前端才需要：取得 godot-cpp
#    預設路徑是 ../../projects/godot-cpp（相對本 repo 根目錄往上兩層的 projects/godot-cpp）；
#    也可以放別的地方，用 -DGODOT_CPP_DIR=<path> 覆寫。
#    取得方式（依 godot-cpp 版本/分支自行對齊 Godot 4.6，本 repo 未內附取得腳本）：
#    git clone --recursive <godot-cpp repo> ../../projects/godot-cpp

cmake -S . -B build -DZONE_BUILD_GDEXTENSION=ON \
    [-DGODOT_CPP_DIR=/path/to/godot-cpp]   # 若不是預設路徑才需要
cmake --build build --target zone_gd -j4
# 建置後 CMake 會自動把 libzone_gd.so/.dll 複製到 godot_zone/bin/（gitignore，勿手動維護）

# 3) 第一次在 Godot 開這個專案，必須先 import 一次
#    （建立 godot_zone/.godot/extension_list.cfg 等快取，否則 GDExtension 類別載不到）
godot-mono --headless --path godot_zone --import

# 4) 驗證前端整條鏈路
godot-mono --headless --path godot_zone -s res://verify.gd   # 預期 "VERIFY PASSED"
```

## 只能本機/使用者做的事

- **GUI 手動驗證**：`godot-mono --path godot_zone` 開遊戲視窗、實際用鍵盤操作、肉眼確認畫面/HUD——agent 沒有互動式 GUI，這件事寫進 [../WAIT_USER.md](../WAIT_USER.md) 交給使用者。
- **godot-cpp checkout 的取得/更新**：`ZONE_BUILD_GDEXTENSION` 依賴外部路徑 `../../projects/godot-cpp`，agent 不應假設它一定存在；若不存在，`zone_gd` 相關步驟只能略過並記錄環境缺口，不要嘗試網路下載大型第三方 repo 除非使用者明確要求。

## 哪些測試會因環境缺失失敗（非 regression）

- 環境沒有 `godot-mono`：`godot-mono --headless --path godot_zone -s res://verify.gd` 和開遊戲都無法執行；**`zone_test`（純 C++ doctest，不連結 godot-cpp）完全不受影響**，應照常 build/run 並視為主要驗證依據。
- 環境沒有 `../../projects/godot-cpp`（或未用 `-DGODOT_CPP_DIR` 指到有效路徑）：`-DZONE_BUILD_GDEXTENSION=ON` 的 configure/build 會失敗於 `add_subdirectory(godot-cpp)`；預設 `ZONE_BUILD_GDEXTENSION=OFF` 時不受影響，`zone_core`/`zone_test` 正常。
- 沒先跑過 `--import`：`verify.gd`/GUI 可能因 `.godot/` 快取缺失而載不到 `ZoneWorld` 等 GDExtension 類別；補跑一次 import 即可，不是程式碼問題。

## 常用環境變數 / CMake 變數

| 變數 | 用途 | 預設/範例 |
|------|------|-----------|
| `ZONE_BUILD_GDEXTENSION`（CMake option） | 是否建置 `src/gbind` GDExtension（`zone_gd`），需要 godot-cpp | 預設 `OFF`；`-DZONE_BUILD_GDEXTENSION=ON` |
| `GODOT_CPP_DIR`（CMake cache path） | godot-cpp checkout 路徑 | 預設 `${CMAKE_SOURCE_DIR}/../../projects/godot-cpp`；可用 `-DGODOT_CPP_DIR=...` 覆寫 |
| `CMAKE_POLICY_VERSION_MINIMUM`（CMake cache） | 相容 FetchContent 依賴（如 doctest v2.4.11）仍宣告舊版 `cmake_minimum_required` 的問題（CMake 4.0+ 不再接受 < 3.5） | 預設 `3.5`，一般不需要動 |

## 出貨/打包

- 本 repo 目前是原型階段，尚無正式 package/release 流程；「打包」目前等同「build `zone_gd` 並確認 `godot_zone/bin/` 有最新 `.so`」，見上方 Fresh Clone 步驟 2。

## 何時不用

- 只是一次性跑 build/test，走 testing 或原工作流。
- 是外部工具細節，走 tooling。
