# html-guide — Markdown 導覽層入口

當某個工作單位的 `.md` 文件太多、難以快速綜覽時，建立 HTML 導覽層。HTML 只負責索引與呈現，不取代 `.md`。

## 鐵則

- 開始前寫 `Done when: <導覽入口可瀏覽、連回來源 md、沒有明顯斷鏈>`。
- `.md` 是唯一真相層；內容更新一律先改 `.md`，再更新 HTML。
- HTML 放在該工作單位下的 `html/`，以相對路徑連回來源 `.md`。
- 不把 HTML 當作主要維護面，除非使用者明確要求。

## 建議結構

```text
<work-unit>/
  topic-a.md
  topic-b.md
  html/
    index.html
    topic-a.html
    topic-b.html
    _shared.css
```

## index.html 內容

- 工作單位總覽。
- 主要分類卡片或表格。
- 指向主題頁與原始 `.md` 的連結。
- 最近更新或狀態摘要，若來源 `.md` 有明確狀態。

## 圖表

- Markdown 真相層：用 Mermaid、表格、列點。
- HTML 呈現層：可用 CSS 卡片、grid、section、或內嵌 Mermaid。
- 避免 ASCII art 框線圖，因中英文寬度與字元對齊不穩。

## 何時不用

- `.md` 數量少，README/INDEX 已足夠。
- 內容仍快速變動，HTML 同步成本高。
- 使用者只需要原始 Markdown，不需要瀏覽頁。
