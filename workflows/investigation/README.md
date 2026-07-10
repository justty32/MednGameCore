# investigation — 調查/解碼入口

用於研究外部專案、既有系統、bug 真因、可行性。目標是把不確定變成可行/不可行/待補缺口。

## 流程

開始前寫一句：

```text
Done when: <可行/不可行/缺口/下一步已明確，finding 已落檔>
```

```text
收集事實
  → 對照本專案現有能力
  → 分類：可直接做 / 有缺口 / 不值得做 / 需使用者驗證
  → 產出 finding
  → 缺口進 roadmap，踩坑進 gotchas
```

規則：

- 優先保留可驗證來源、命令、檔案路徑、版本。
- 不把未驗證猜測寫成結論。
- 調查結果若會導致功能開發，先進 specs 或 plans，不直接散落在聊天紀錄。

## 何時不用

- 已經知道要改哪裡且能直接實作，走 feature-dev。
- 是系統性閱讀 paper/長文並建索引，走 research。
- 是完整陌生 repo 初始分析，走 analysis。

## 內容

| 路徑 | 內容 |
|------|------|
| `findings/` | 調查結果，按需建立 |
| `gotchas.md` | 調查踩坑，按需建立 |
| `session-log.md` | 本工作流 open/in-flight 調查 |
| `archive/` | 過時調查文檔 |
