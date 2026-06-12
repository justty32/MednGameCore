# 多角色戰術：編隊、待命與協同技

> 日期：2026-06-11
> 定位：在 order 表模型落地後，讓玩家操控多名 actor 產生真正的戰術差異，而不是多個 hero 輪流走路。

---

## 1. 前提

依賴 architecture 02 的 order 表：玩家陣營可有多名 actor 等待指令，scheduler 需要清楚回報哪個 actor 正在等待。

## 2. 待命與延後（優先序：高，order 落地後）

多角色戰術最先需要的是「不動也是選擇」：

- `Wait`：消耗本回合，回復少量 energy 或維持防禦。
- `Delay`：不消耗完整行動，把 actor 排到稍後再決定。
- `Guard`：待命到敵人進入鄰接時自動攻擊或防禦。

Delay 需要 scheduler 支援，Guard 可先做成 1 回合 buff。

## 3. 編隊（優先序：中）

隊伍移動不是每格都下四個命令。非戰鬥或低威脅狀態可用 formation：

- leader 移動。
- followers 根據相對 offset 追隨。
- 進入敵視範圍或戰鬥事件後切回逐 actor order。

這比較像前端/Session 層功能，不應塞進 core scheduler。

## 4. 協同技（優先序：中低）

先從資料化條件開始：

```text
if actor A uses "mark" and actor B uses "smite" before mark expires -> bonus
```

需要：

- 狀態效果能標記 source actor。
- ZoneEvent / trace 能顯示觸發原因。
- NPC 也能吃同一套 mark / exploit 機制。

## 5. 不做的事

- 不做即時暫停式 party AI；仍以回合制 order 為核心。
- 不讓 followers 穿牆或忽略碰撞；形成移動失敗要可見。
- 不在單 hero 手感穩定前投入大型協同技內容。
