# 多角色戰術：編隊、待命與協同技

> 日期：2026-06-11（設計內容）
> 狀態更新：2026-06-21
> 定位：讓玩家操控多名 actor 產生真正的戰術差異，而不是多個 hero 輪流走路。

---

## 1. 前提與落地點

**落地點（現況）**：`ControllerComponent`（`src/core/components/controller_component.h`）
已有 `kind` 欄位與 `ControllerKind::Faction`（含 `faction` entity 欄位）——這是
**多角色/隊伍操控的天然落地點**。目前 `Faction` 暫等同 `Self`（TurnEngine 對它走
AI 立即決策路徑），未來把「玩家陣營多名 actor 各自等待指令」做成：把隊員設為
`Player`（逐一等指令），或擴充 `Faction` 語意（由玩家統一下令一個陣營）。

TurnEngine 的 actor 順序串列（加入序 = 回合序）已能容納多名 actor；
`is_waiting_player()` 已能回報「現在輪到玩家操控的 actor」。其餘為**未來方向**。

## 2. 待命與延後（未來方向）

多角色戰術最先需要的是「不動也是選擇」：

- `Wait`：消耗本回合（現況已有 `ActionKind::Wait`），未來可附帶維持防禦。
- `Delay`：不消耗完整行動，把 actor 在順序串列中排到稍後再決定。
- `Guard`：待命到敵人進入鄰接時自動攻擊或防禦。

Delay 需要 TurnEngine 支援「重排 actor 順序」，Guard 可先做成 1 回合 buff。

## 3. 編隊（未來方向）

隊伍移動不是每格都下四個命令。非戰鬥或低威脅狀態可用 formation：

- leader 移動。
- followers 根據相對 offset 追隨。
- 進入敵視範圍或戰鬥事件後切回逐 actor 操控。

這比較像前端/Session 層功能，不應塞進 core TurnEngine。

## 4. 協同技（未來方向）

先從資料化條件開始：

```text
if actor A uses "mark" and actor B uses "smite" before mark expires -> bonus
```

需要：

- 狀態效果能標記 source actor。
- ZoneEvent / trace 能顯示觸發原因。
- NPC 也能吃同一套 mark / exploit 機制。

## 5. 不做的事

- 不做即時暫停式 party AI；仍以回合制（TurnEngine actor 順序）為核心。
- 不讓 followers 穿牆或忽略碰撞；形成移動失敗要可見。
- 不在單 hero 手感穩定前投入大型協同技內容。
