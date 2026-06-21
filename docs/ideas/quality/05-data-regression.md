# 資料回歸防護

> 日期：2026-06-21
> 定位：避免資料檔（地圖主題、存檔、未來資料驅動內容）的 typo 或 schema 演進在執行期才爆。
>
> **狀態更新（2026-06-21）**：本檔原針對 `data/actions.json`（fireball/heal/meteor/venom 等 6 技能 + ActionLibrary）。
> 回合系統重構**移除了技能資料與 ActionLibrary**，`data/actions.json` 與相關 fallback 路徑已不存在。
> 本檔重定位為：（a）**現存可做**的地圖生成/存檔回歸；（b）**未來**若重新引入資料驅動內容時的 schema 護欄方法論（保留作設計參考）。

---

## 1. 現況

目前 core 沒有外部資料驅動的玩法內容——動作只有 `Idle/Move/Wait`（攻擊＝撞擊），
道具只有寫死的 health_potion，地圖為 BSP 程序生成（60×40）。
因此「壞 JSON 在執行期才爆」這個原始風險暫時不存在，但有兩個仍真實的回歸面向：

- **地圖生成**：mapgen 參數/演算法改動可能讓地圖變不可玩（無連通、無下樓梯、英雄被困）。
- **存檔格式**：cereal 序列化的欄位/元件清單演進可能讓舊存檔讀不回（或靜默讀錯）。

## 2. 地圖生成回歸（優先序：中）

固定 seed 跑 mapgen，斷言**結構不變量**而非逐格像素：

- 地圖尺寸如預期（60×40）。
- 英雄起點 walkable。
- 存在至少一個下樓梯，且與英雄起點連通（flood fill 可達）。
- walkable 格佔比在合理區間（避免退化成全牆/全空）。
- 固定 seed → 固定地圖 digest（釘成 golden，演算法無意改動即紅）。

這些可在 `zone_core` 純 C++ 測，不需 Godot。

## 3. 存檔回歸（優先序：中）

- **round-trip**：建構含全部 `AllComponents` 元件的世界，save → load → digest 比對（與 01 §3 共用）。
- **golden 存檔**：把一個已知世界的存檔位元組（或其 digest）釘住，讀回後世界 digest 應一致；
  cereal 版本/欄位變動造成不相容時測試紅，迫使有意識地處理遷移。
- **缺欄位/壞檔**：餵截斷或欄位缺失的存檔，斷言讀取**大聲失敗**（回傳 false / trace warning），
  而非靜默產生半初始化世界。

## 4. 未來資料驅動內容的 schema 護欄（保留作方法論）

若日後重新引入資料驅動內容（技能、道具原型、地圖主題），建議每種資料檔具備：

- `schema_version`、parse errors、semantic warnings/errors、loaded count（可程式化檢查，而非只印 log）。
- 測試直接餵字串：valid minimal / missing required / duplicate id / invalid enum / negative number。
- golden data test：載入 repo 內資料，斷言數量、必要 id 存在、欄位範圍合理。
- fallback 規則：開發期大聲（trace warning）、測試期 strict（任何 warning 即 fail）、
  正式遊戲 base data 壞掉 fail fast、mod data 壞掉可跳過該 mod。

> 註：上述為設計參考，**目前無對應程式**。重新引入資料前不需先建這套，但引入時應同步建立。

## 5. 不做的事

- 不為已不存在的 `actions.json` 維護 schema 驗證。
- 不讓 `catch(...)` 吞掉存檔/讀檔的欄位資訊。
- 不在資料驅動內容尚未重新引入前，預先實作完整 JSON Schema 工具。
