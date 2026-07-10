# INIT-QUESTIONS — 新 repo 導入問答

把工作流導入新 repo 前，先回答這些問題。回答不完整也可以開始，但缺口要記到 `WAIT_USER.md` 或 `dev-env.md`。

## 專案基本資訊

- 專案一句話是什麼？
- 主要語言/框架是什麼？
- repo 的主要入口檔在哪？
- 使用者最常要 agent 做哪三類事？

## 指令

- build 指令是什麼？
- test 指令是什麼？
- lint/format 指令是什麼？
- full verification 指令是什麼？
- 哪些指令很慢，不適合每次跑？

## 環境

- fresh clone 後必做哪些步驟？
- 是否需要外部服務、帳號、授權、遊戲、硬體、資料集？
- 哪些測試會因環境缺失而失敗？
- CI 能跑什麼？本機才能跑什麼？

## Truth Layers

本 repo 的 source-of-truth 優先序是什麼？

預設：

```text
code/tests > schema/examples/fixtures > CODE_MAP > docs > generated/html
```

若本 repo 不同，請改寫 `PRINCIPLES.md` 和 `workflows/common/conventions.md`。

## CODE_MAP

- 是否需要 CODE_MAP？
- 若需要，最少分幾個領域？
- 哪些檔案是修改前最容易讀錯/漏讀的？
- 測試檔如何對應到 production 檔？

## 工作流啟用

- 是否只需要 minimal adoption？
- 是否會跨 session 工作？
- 是否需要 WAIT_USER？
- 是否會分析外部 repo？
- 是否會讀 paper/長文並建索引？
- 是否需要 patch 包交給別的 agent 套用？
- 是否需要 HTML 導覽層？

## 風險與禁令

- 哪些檔案不能自動改？
- 哪些命令不能自動跑？
- 哪些輸出不能 commit？
- push/release/deploy 是否一定要使用者確認？

