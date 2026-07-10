# refactor — 重構整理入口

behavior-preserving 的拆分、模組化、結構整理使用。結構原則見 [../../DEV-GUIDE.md](../../DEV-GUIDE.md)。

## 流程

開始前寫一句：

```text
Done when: <行為不變的整理完成、測試通過、導航/文檔同步的條件>
```

維護鏈中一次只動一個面向，做完再看下一個：

```text
Step 1  程式碼重構 → 立即更新 CODE_MAP 與相關文檔 → 跑測試 → commit
Step 2  CODE_MAP 若臃腫 → 單獨重構 → 同步連結到的文檔段落 → commit
Step 3  文檔若臃腫 → 單獨重構 → 同步 CODE_MAP 指向 → commit
Step 4  examples/assets/fixtures 若需整理 → 單獨處理 → commit
```

規則：

- 同一 session 不同時重構超過一個面向。
- 每個 step 完成前不啟動下一個。
- 行為不變；若測試不足，要說明殘餘風險。

## 何時不用

- 修改會改變行為或新增能力，走 feature-dev。
- 只是補文檔或整理研究資料，不碰 code 結構，走對應文檔/研究工作流。
- 小範圍命名或格式修正，直接做並跑必要檢查即可。

## 內容

| 檔案 | 內容 |
|------|------|
| `session-log.md` | 本工作流 open/in-flight 重構項 |
| `archive/` | 過時重構筆記/計畫 |
