# 資料回歸防護

> 日期：2026-06-11
> 定位：避免 actions.json、未來 prototypes/map themes 的 typo 或 schema 演進在執行期才爆。

---

## 1. 現況

ActionLibrary 載入失敗會 fallback 到 defaults，容易掩蓋壞資料。資料檔越多，這種 silent fallback 越危險。

## 2. Schema + 語意驗證（優先序：高）

每種資料檔都應有：

- `schema_version`
- parse errors
- semantic warnings/errors
- loaded count

測試直接餵字串：

- valid minimal。
- missing required。
- duplicate id。
- invalid enum。
- negative number。

## 3. Golden data test

ctest 應載入 repo 內 `data/actions.json`，斷言：

- action count。
- 必要 id 存在。
- 每個 action 至少有一個效果。
- weight / radius / cooldown 範圍合理。

這能抓到「資料改壞但程式沒改」。

## 4. Fallback 規則

- 開發期：資料錯誤應大聲，至少 trace warning。
- 測試期：可設定 strict mode，任何 warning 都 fail。
- 正式遊戲：若 base data 壞掉，應 fail fast；mod data 壞掉可跳過該 mod。

## 5. 不做的事

- 不靠手動打開 Godot 才知道 JSON 壞了。
- 不讓 `catch(...)` 吞掉欄位資訊。
- 不在資料尚未穩定前追求完整 JSON Schema 標準工具。
