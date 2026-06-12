# Modding 與資料覆寫管線

> 日期：2026-06-11
> 定位：遠期構想：讓 actions、prototypes、map themes 能被外部資料包覆寫，同時保持可驗證與可除錯。

---

## 1. 基本方向

Modding 不是近期核心，但資料驅動化每一步都應避免把路堵死。目標是「資料包覆寫」，不是「任意腳本」。

## 2. 目錄結構草案

```text
data/
  base/
    actions.json
    prototypes.json
    terrains.json
  mods/
    my_mod/
      mod.json
      actions.patch.json
      prototypes.json
```

`mod.json` 宣告 id、版本、依賴、載入順序。

## 3. 覆寫規則

- 同 id 項目後載入覆蓋前載入。
- array 不靠順序引用，全部用穩定 id。
- load report 列出每個 id 來源檔，debug UI 可查。
- 衝突預設 warning，不 silent。

## 4. 開發期 reload（優先序：高，哪怕不做 mod 也有用）

`ZoneWorld.reload_data()`：

- 重新載入 ActionLibrary。
- 回報 errors/warnings。
- 不重建正在進行的世界，除非 action id 缺失。

這對調技能數值立即有價值。

## 5. 不做的事

- 不允許 mod 執行原生程式碼。
- 不保證大型 total conversion。
- 不在資料 id 穩定前公開 mod API。
