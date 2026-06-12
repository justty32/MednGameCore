# Debug 工具路線：種子、重播、雜湊

> 日期：2026-06-11
> 定位：讓隨機 bug、scheduler 差分、長跑模擬可以重現、縮小、比對。

---

## 1. RNG 種子統一（優先序：高）

現有種子分散在 mapgen、NPC AI、turn RNG。建議引入：

```cpp
struct WorldSeed {
    uint64_t root;
    uint64_t map_seed;
    uint64_t ai_seed;
    uint64_t combat_seed;
};
```

由 root 派生子種子。snapshot / debug UI 顯示 root。

## 2. 指令重播（優先序：中）

記錄玩家提交的 command：

```text
turn, actor_id/display_id, action_kind, param, def_id
```

搭配 seed 可重播大部分 bug。entity id 若不穩，需 display id 或 actor label。

## 3. 狀態雜湊（優先序：中）

`WorldDigest` 產生穩定 hash：

- map digest
- actor components digest
- item digest
- scheduler digest

用於差分測試與重播驗證。hash 不取代可讀 dump；失敗時兩者都輸出。

## 4. 結構化 debug dump

`get_debug_text()` 保留，但底層先產生 `DebugSnapshot`，文字只是 renderer。這樣測試可以吃結構化資料，人類仍可看文字。

## 5. 不做的事

- 不重播 wall-clock input，只重播 core command。
- 不把雜湊當存檔相容保證。
- 不要求第一版能縮小 failing seed；先可重現。
