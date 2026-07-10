# testing — 測試入口

把本專案所有常用驗證指令放這裡。agent 能跑的測試應自己跑；不能跑的記到 [../WAIT_USER.md](../WAIT_USER.md)。

開始前寫 `Done when: <指定測試/驗證命令跑完，結果已回報>`。

> **鐵律：所有 `cmake --build` 一律加 `-j4`（或更低）。** 曾因無上限 `-j` 同時啟動約 541 個編譯程序，耗盡 60GiB RAM 觸發強制重啟。不要用裸 `-j`、`--parallel` 或 `make -j$(nproc)`。

## 常用指令

```bash
# 1) configure（首次或 CMakeLists 變更後才需要；含 GDExtension）
cmake -S . -B build -DZONE_BUILD_GDEXTENSION=ON

# 2) 單元測試：build + run（doctest，29 test case / 93 assertion 全綠）
cmake --build build --target zone_test -j4
./build/zone_test
# 或用 ctest（同一個目標，輸出格式不同）
ctest --test-dir build

# 3) GDExtension build（改了 src/gbind/ 或要跑 Godot 前端才需要）
cmake --build build --target zone_gd -j4
# 建置後會自動把 libzone_gd.so 複製到 godot_zone/bin/（見 CMakeLists.txt 的
# POST_BUILD copy_if_different），不用手動複製。

# 4) headless 端到端驗證（改了 gbind/ZoneWorld 或前端行為後才需要）
godot-mono --headless --path godot_zone --import   # 只需跑一次（fresh clone 後）
godot-mono --headless --path godot_zone -s res://verify.gd
# 預期輸出結尾包含 "VERIFY PASSED"
```

## 測試分類

- `fast`：只改 `src/core/` 邏輯 → 跑 `zone_test`（build 通常數秒~數十秒，除非 configure 剛清過快取）。
- `full`：改了 `src/gbind/` 或前端行為 → `zone_test` + `zone_gd` + headless `verify.gd` 三項都跑。
- `external`：GUI 手動驗證（見下）—— agent 做不到，記到 WAIT_USER。

## 慢的步驟（先預期，別誤判成卡住）

- **首次 `cmake -S . -B build`**：會用 FetchContent 拉 EnTT、cereal、spdlog、doctest 原始碼並 configure，第一次可能要數分鐘（含網路）。之後只要 `CMakeLists.txt` 沒變就不會重跑。
- **`zone_gd` 全量編譯**：`ZONE_BUILD_GDEXTENSION=ON` 時要另外 `add_subdirectory(godot-cpp)`，godot-cpp 本身是大型 C++ 綁定層，第一次連結 `zone_gd` 目標會明顯比 `zone_test` 慢。
- 兩者都受 `-j4` 鐵律限制，不要為了「跑快一點」調高並行度。

## 已知環境性失敗

- 沒有 `godot-mono` 執行檔時：`godot-mono --headless ... verify.gd`、GUI 跑遊戲都無法執行；`zone_test`（純 C++ doctest）完全不受影響，仍可正常 build/run。
- 沒有先跑過 `godot-mono --headless --path godot_zone --import`（建立 `.godot/extension_list.cfg` 等）時，`verify.gd` 或開遊戲可能找不到 GDExtension 類別；先補跑一次 import。
- 若 `godot_zone/bin/` 下沒有 `libzone_gd.so`（例如只 build 了 `zone_test` 沒 build `zone_gd`），Godot 端會缺 `ZoneWorld` 類別；先跑 `cmake --build build --target zone_gd -j4`。

## 何時不用

- 只是查測試指令，直接讀本檔回答。
- 測試是 feature/refactor 的一部分，不需要另開測試工作流；在原工作流內執行即可。
