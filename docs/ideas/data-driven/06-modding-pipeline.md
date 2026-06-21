# Modding 與資料覆寫管線

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：遠期構想：讓 prototypes、map themes、（未來重新引入的）actions 等外部資料包覆寫，同時保持可驗證與可除錯。

> **狀態（2026-06-21）**：本文多為遠期方向，受重構影響小。唯一需注意的是：
> `actions.json` / ActionLibrary 目前**不存在**（隨能量排程器移除），下文凡提到
> 覆寫 actions 或 reload ActionLibrary 的部分，皆以「01 的資料驅動動作重新引入後」
> 為前提；可先從 prototypes/terrains 等已可資料化的內容著手。

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

`ZoneWorld.reload_data()`（GDExtension 方法）：

- 重新載入已資料化的內容檔（先做 prototypes/terrains；待 01 動作系統重新引入後
  再含 actions）。
- 回報 errors/warnings 進 debug log。
- 不重建正在進行的世界，除非引用的 id 缺失。

這對調數值（先是怪物屬性，未來是技能數值）立即有價值。

## 5. 不做的事

- 不允許 mod 執行原生程式碼。
- 不保證大型 total conversion。
- 不在資料 id 穩定前公開 mod API。
