# gamecore-zone — Agent 專案備忘

> 最頂層路由器：只放 always-on 規則與入口連結；細節放到各工作流。

## 專案摘要

- 專案一句話：C++（EnTT ECS）核心 + Godot 4.6（GDExtension, mono）前端的最傳統回合制地城 roguelike 原型；World/Region/Area 三層架構中的 Area（區域）層。
- 主要語言/框架：C++17（EnTT ECS、doctest、cereal）＋ Godot 4.6 GDExtension（godot-cpp, mono）。
- 主要 build 指令：`cmake -S . -B build -DZONE_BUILD_GDEXTENSION=ON` 後 `cmake --build build --target zone_test -j4`（核心測試）／`--target zone_gd -j4`（GDExtension）。
- 主要 test 指令：`./build/zone_test`（doctest，29 case/93 assertion）或 `ctest --test-dir build`；端到端 `godot-mono --headless --path godot_zone -s res://verify.gd` → VERIFY PASSED。

## 先讀哪裡

- 使用者要你動手做某件事 → [WORKFLOWS.md](WORKFLOWS.md)：依意圖派發到對應工作流。
- 想看 repo 頂層結構 → [INDEX.md](INDEX.md)：頂層目錄地圖；深入現況與架構 → [README.md](README.md)、[PROJECT.md](PROJECT.md)、[docs/架構與函數說明.md](docs/架構與函數說明.md)。
- 碰原始碼 → 先讀 [workflows/common/conventions.md](workflows/common/conventions.md)，再讀 [CODE_MAP](workflows/common/code-map/CODE_MAP.md)。

## Always-on 鐵律

- **建置一律加並行上限 `-j4`（或更低）。** 曾因無上限 `-j` 同時起約 541 個編譯程序吃光 60GiB RAM 觸發 OOM 強制重啟——這是本專案第一鐵律。
- 重構/整理必須 behavior-preserving；改完跑對應測試（`zone_test` 全綠、必要時 headless `verify.gd`）。
- 未經使用者確認，不 push、不開新大型工作。
- 不 revert 使用者或其他 agent 的未確認變更；遇到衝突先停下說明。
- 各工作流的具體流程在自己的 README，不在本檔重複。
- 小事可以跳流程；跳流程規則與 Done when 細則見 [DEV-GUIDE.md](DEV-GUIDE.md)。
- 非微小工作先定義 `Done when:`。
- 需要使用者親自驗證、外部環境、權限、實機、帳號或手動操作時，記到 [WAIT_USER.md](WAIT_USER.md)。
- 跨 session 的 open 狀態記到 [SESSION-LOG.md](SESSION-LOG.md) 或對應工作流的 `session-log.md`。
- 引用外部專案程式碼或技術結論時，盡量附來源位置：`path/to/file:line`、函式名、URL、paper id、或命令輸出摘要。
- 架構圖/流程圖優先用 Mermaid、表格、列點；不要用需要字元對齊的 ASCII 框線圖。

## 分層思想

整個 repo 是分層樹，每一層只指向下一層：

```text
AGENTS.md → WORKFLOWS.md / INDEX.md → 各工作流入口 → 工作流內容 → 子工作流
```

- `README.md` = 初入一個資料夾先讀的入口/導引。
- `INDEX.md` = 描述該資料夾頂層結構的索引。
- 小資料夾可以 README 兼 index；變大後才拆出獨立 INDEX。
- durable 知識歸到它所屬的工作流，不堆在頂層。

## 本地專案規則

- **分層鐵律**：`src/core/` 完全 godot-free、可單元測試；`src/gbind/`（`ZoneWorld`）是唯一認識 Godot 的橋樑層。核心邏輯不得 include 任何 Godot header。
- commit 訊息慣例：conventional commits + 中文描述（`refactor(turn): …`、`feat(godot): …`，見 git log）。
- 生成檔/二進位不 commit：`build/`、`godot_zone/bin/`、`.godot/`、`user://` 皆已 gitignore。
- 外部依賴：godot-cpp 在 repo 外 `../../projects/godot-cpp`（可用 `-DGODOT_CPP_DIR` 覆寫）；`ZONE_BUILD_GDEXTENSION` 預設 OFF，要建 GDExtension 才開。
