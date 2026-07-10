# feature-dev — 功能開發入口

新增或修改功能時使用。碰原始碼前先讀 [common/conventions](../common/conventions.md)。

## 流程

開始前寫一句：

```text
Done when: <功能完成、測試/驗證通過、文檔/CODE_MAP 對齊的條件>
```

```text
修改程式碼（增量）
  → 跑本地可跑的測試
  → 需要時交使用者手動/實機驗證
  → 回報問題 → 修程式碼 → 重複
  → 全數通過後：補齊 CODE_MAP → 補文檔 → commit
```

規則：

- agent 自己能跑的 test/lint/build 要自己跑。
- agent 不能代做的外部驗證記到 [../../WAIT_USER.md](../../WAIT_USER.md)。
- 測試迭代期間，CODE_MAP/文檔可暫時落後。
- commit 前，CODE_MAP + 文檔必須對齊。
- 跨 session 時在 `session-log.md` 補 open 狀態。

## 何時不用

- 單行修正、小 typo、小型內部調整，直接改並跑必要檢查即可。
- 還在純討論設計，未決定動 code，走 specs 或 plans。
- 只是研究外部材料，走 research 或 investigation。

## 內容

| 檔案 | 內容 |
|------|------|
| `session-log.md` | 本工作流 open/in-flight 進度 |
| `gotchas.md` | 功能開發踩坑，按需建立 |
| `landed/` | 已落地功能目錄，按需建立 |
| `archive/` | 過時或被取代的功能開發文檔 |
