# GDExtension 介面演進：snapshot 與事件流

> 日期：2026-06-11
> 定位：把 `ZoneWorld` 對 Godot 的介面，從 debug 字串與整圖重繪，演進成可給前端、測試、跨層系統共用的結構化資料介面。

---

## 1. 現況與痛點

`src/gbind/zone_world_gd.cpp` 目前提供兩條主要前端路徑：

- `generate_map_image()`：一次產生整張 `ImageTexture` 所需像素，簡單可靠，但很難做 actor 動畫、FOV、局部更新。
- `get_debug_text()`：把世界狀態 dump 成字串，適合早期驗證，不適合作正式 HUD 或事件面板。

問題不是「不能玩」，而是**資料沒有邊界**：Godot 端若想知道某個 actor 的 HP、位置、效果、施法狀態，只能從 debug 字串旁敲側擊，或繼續把 Godot binding 擴成 god class。

## 2. Snapshot 介面（優先序：中高）

新增只讀查詢：

```gdscript
var snap := zone_world.get_snapshot()
```

回傳 `Dictionary`：

```text
{
  "clock": int,
  "floor": int,
  "map": { "w": int, "h": int, "tiles": PackedByteArray },
  "actors": [
    { "id": int, "kind": String, "x": int, "y": int,
      "hp": int, "max_hp": int, "energy": int,
      "is_player": bool, "ongoing": Dictionary, "effects": Array }
  ],
  "items": [ { "id": int, "x": int, "y": int, "kind": String } ]
}
```

設計原則：

- **只讀、可重建**：snapshot 是顯示快照，不是存檔格式。
- **穩定欄位優先**：先曝位置、HP、能量、ongoing/effects 摘要；背包、技能、抗性等後續再加。
- **Godot 型別友好**：`Dictionary` / `Array` / `PackedByteArray`，避免前端理解 C++ 結構。

## 3. 事件流介面（優先序：高，frontend 02 前置）

新增：

```gdscript
var events := zone_world.drain_events()
```

每筆事件至少含：

```text
{ "type": String, "actor": int, "target": int, "from": Vector2i, "to": Vector2i,
  "amount": int, "damage_type": String, "text": String }
```

`text` 是 debug 後備，前端正式顯示應優先吃結構化欄位。事件由 core `ZoneEvent` 產生，GDExtension 只轉型，不在 binding 層補推理。

## 4. 漸進落地順序

1. `ZoneEvent` 補座標與數值 payload，維持現有 trace 字串。
2. `ZoneWorld::drain_events()` 匯出 `Array[Dictionary]`，headless verify 斷言事件數。
3. `get_snapshot()` 先只回 actor + HP + map 尺寸，前端 HUD 改吃 snapshot。
4. `generate_map_image()` 保留給 smoke test 與 fallback，直到 TileMapLayer 路徑穩定。

## 5. 不做的事

- 不把 snapshot 當 save/load；存檔仍在 core serialize 層。
- 不讓 Godot 端直接改 C++ registry；操作仍走 `submit_*` 命令。
- 不一次移除 `get_debug_text()`；它仍是 debug 面板與回歸排查的便宜工具。
