# 存檔相容性策略

> 日期：2026-06-11
> 定位：趁存檔尚未成為玩家資產前，先定義 header、版本與 migration 原則。

---

## 1. 為什麼現在要做

目前存檔能 round-trip 部分世界狀態，但缺 header。未來 AllComponents、ActionDef、PrototypeDef 都會演進，若沒有版本資訊，讀舊檔只能猜。

## 2. Save Header（優先序：高）

```cpp
struct SaveHeader {
    uint32_t magic;          // 'MNGC'
    uint16_t save_version;   // 存檔格式版本
    uint16_t game_version;   // 可選，或字串 hash
    uint32_t data_schema;    // actions/prototypes schema 摘要
};
```

先寫在 binary archive 最前面。讀檔先檢查 header，不對就回明確錯誤。

## 3. Migration 原則

- 加 component 且有預設值：可讀舊檔，補預設。
- 改 component 欄位語意：升 save_version，寫 migration。
- 刪 component：至少保留讀舊欄位並丟棄一版。
- Action / Prototype 定義不進存檔，只存穩定 id。

## 4. 測試

建立 `tests/data/saves/` 放小型 golden save：

- v1 minimal world。
- v1 actor with timed effects。
- v2 active channel。

每次改 save schema 必須能讀前一版，或明確在文件宣告破壞。

## 5. 不做的事

- 不承諾 alpha 期間永久相容，但要能明確失敗。
- 不把 JSON def 複製進存檔；存 id + runtime state。
- 不 silent fallback 成新遊戲。
