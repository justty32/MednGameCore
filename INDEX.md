# INDEX — gamecore-zone 專案地圖

整個專案的頂層導航。gamecore-zone = **C++（EnTT ECS）核心 + Godot 4.6（GDExtension, mono）前端的最傳統回合制地城 roguelike 原型**（World/Region/Area 三層中的 Area 層）。[AGENTS.md](AGENTS.md) 只做路由 + 鐵律；細節從這裡分流。

---

## Repo 佈局

| 路徑 | 內容 |
|------|------|
| `projects/` | 各獨立、平級的子專案（見下三列）|
| `projects/core/` | C++ 核心專案：`src/core/`（ECS 邏輯，**完全 godot-free、可單元測試**：turn/、tactics/、systems/、serialize/…）、`src/gbind/`（`ZoneWorld`／`TacticsBattle` 橋樑層，**唯一認識 Godot**）、`CMakeLists.txt`、`build/`（產出，不 commit）；導航見 [CODE_MAP](workflows/common/code-map/CODE_MAP.md) |
| `projects/tests/` | doctest 測試（獨立專案，自行 find/link 已建好的 `zone_core`）；跑法見 [testing](workflows/testing.md) |
| `projects/godot_zone/` | Godot 4.6 前端專案（`main_menu.tscn`、`verify.gd`…、`bin/` 為 gitignored 編譯產物）|
| `workflows/` | 開發工作流（入口見 [WORKFLOWS.md](WORKFLOWS.md)）；跨工作流共用在 `workflows/common/` |
| `docs/` | 給人讀的深入說明（[架構與函數說明](docs/架構與函數說明.md)、進度說明）|
| `html/` | 由 `.md` 生成的瀏覽導覽站（衍生層，非真相層；工作流 `workflows/html-guide/`）|

## 工作流

「做某件事該用哪個工作流」的派發表在 **[WORKFLOWS.md](WORKFLOWS.md)**。每個工作流的 durable 知識歸在 `workflows/<該工作流>/`（或單檔 `workflows/<名>.md`），具體流程在各自入口檔。

[DEV-GUIDE.md](DEV-GUIDE.md) 是**被動的結構整理參考**（結構原則 + 四級成長軌跡 + 跳流程/Done when 細則）——只在重構/整理結構時取用。always-on 鐵律在 [AGENTS.md](AGENTS.md)；碰原始碼的程式碼慣例 + CODE_MAP 維護鏈在 [workflows/common/conventions.md](workflows/common/conventions.md)。

## 活狀態（只列還沒完成的）

| 檔案 | 用途 |
|------|------|
| [SESSION-LOG.md](SESSION-LOG.md) | 進度 hub（repo 根）→ 各工作流 `session-log.md`（open-only）|
| [WAIT_USER.md](WAIT_USER.md) | 待**使用者**親自做/驗證的事（實機、外部環境、權限）|
