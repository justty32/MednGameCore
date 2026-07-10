# GDExtension 介面演進：snapshot 與事件流

> 日期：2026-06-11（原稿）／狀態更新：2026-06-21
> 定位：把 `ZoneWorld` 對 Godot 的介面，從 debug 字串與整圖重繪，演進成可給前端、測試、跨層
> 系統共用的結構化資料介面。

> [!IMPORTANT]
> **2026-06-21 重構後校正**：本文主張的「snapshot + 事件流取代 get_debug_text 字串 dump」
> **路線仍完全有效**。需更新的只是「現有介面清單」——重構後 ZoneWorld 改為最傳統回合制 API
> （`next_round`/`player_move`/`player_wait`/`is_waiting_player`/`round_in_progress`），移除了
> `move`/`wait_turn`/`set_scheduler_mode`/`submit_hero_*`/`step_scheduler`/`hero_is_waiting`/
> `get_world_clock`/`get_hero_effects` 等舊綁定。snapshot 草案中的 `energy`/`ongoing`/`effects`
> 欄位現在沒有對應資料，重新標為「若引入能量／狀態擴充時才有」。

---

## 1. 現況與痛點（2026-06-21）

`src/gbind/zone_world_gd.{h,cpp}` 目前的對 Godot 介面：

- **回合制入口**：`next_round()`（開新一輪並 pump 到玩家等待／回合結束）、`player_move(dx,dy)`、
  `player_wait()`、`is_waiting_player()`、`round_in_progress()`。
- **顯示**：`generate_map_image(cell_px)` 一次產整張 `ImageTexture`；簡單可靠，但難做 actor
  動畫、FOV、局部更新。
- **debug**：`get_debug_text()` 把世界狀態 dump 成字串（逐 actor）；`set/get_trace_enabled`、
  `get_debug_log`、`clear_debug_log`。
- **狀態 getter**：`get_map_width/height`、`is_walkable`、`get_hero_x/y`、`get_turn_count`、
  `get_hero_hp/max_hp`、`get_npc_count`、`get_current_floor`、`restart`。
- **存讀檔**：`save_game`/`load_game`/`has_save_game`。
- **signals**：`world_changed`、`round_finished`、`floor_changed(floor_num)`、`hero_bumped_wall`、
  `hero_bumped_npc(npc_id)`、`npc_died(npc_id)`、`item_picked_up(item_name, heal_amount)`、`game_over`。

問題不是「不能玩」，而是**資料沒有邊界**：Godot 端若想知道某個 actor 的 HP、位置，只能從
`get_hero_*` 單一英雄 getter 或 debug 字串旁敲側擊，多角色（見 02）一來就不夠用，且會繼續把
binding 擴成 god class。

## 2. Snapshot 介面（優先序：中高）

新增只讀查詢：

```gdscript
var snap := zone_world.get_snapshot()
```

回傳 `Dictionary`：

```text
{
  "floor": int,
  "turn": int,
  "map": { "w": int, "h": int, "tiles": PackedByteArray },
  "actors": [
    { "id": int, "kind": String, "x": int, "y": int,
      "hp": int, "max_hp": int,
      "controller": String,        # "Player" / "Self" / "Faction"
      "act_point": int             # ActPointComponent.value
      # "energy"/"ongoing"/"effects"：若未來引入能量／狀態擴充時才加（見 01）
    }
  ],
  "items": [ { "id": int, "x": int, "y": int, "kind": String } ]
}
```

設計原則：
- **只讀、可重建**：snapshot 是顯示快照，不是存檔格式。
- **穩定欄位優先**：先曝位置、HP、controller kind、act_point；能量／狀態效果等隨擴充再加。
- **Godot 型別友好**：`Dictionary` / `Array` / `PackedByteArray`，避免前端理解 C++ 結構。
- **多角色就緒**：`actors[].id` 與 `controller` 是 02 去 hero 化的前提（前端按 controller=Player
  highlight 受控角色）。

## 3. 事件流介面（優先序：高，frontend 02 前置）

新增：

```gdscript
var events := zone_world.drain_events()
```

每筆事件至少含：

```text
{ "type": String, "actor": int, "target": int, "from": Vector2i, "to": Vector2i,
  "amount": int, "text": String }
```

事件由 core `ZoneEvent`（`apply_action` 產生：BumpedWall/BumpedActor/ActorDied/ItemPickedUp/
ReachedStairDown 等）轉型而來，GDExtension 只轉型、不在 binding 層補推理。`text` 是 debug 後備，
前端正式顯示應優先吃結構化欄位。當前 `ZoneWorld::drain_events()` 已存在（內部用，回傳「英雄踩到
下樓梯」並轉 signal）——對外曝露結構化 `Array[Dictionary]` 是下一步。

## 4. 漸進落地順序

1. `ZoneEvent` 補座標與數值 payload（部分已有），維持現有 trace 字串。
2. `ZoneWorld::drain_events()` 對外匯出 `Array[Dictionary]`，headless verify 斷言事件數。
3. `get_snapshot()` 先只回 actor（id/位置/HP/controller）+ map 尺寸，前端 HUD 改吃 snapshot。
4. `generate_map_image()` 保留給 smoke test 與 fallback，直到 TileMapLayer 路徑穩定。

## 5. 不做的事

- 不把 snapshot 當 save/load；存檔仍在 core serialize 層。
- 不讓 Godot 端直接改 C++ registry；操作仍走 `player_*` / 未來 `submit_actor` 命令。
- 不一次移除 `get_debug_text()`；它仍是 debug 面板與回歸排查的便宜工具。
