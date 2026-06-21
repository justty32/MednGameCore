# 實體原型（prototype）資料化

> 日期：2026-06-11（內容於 2026-06-21 回合系統重構後校正）
> 定位：把怪物種類/屬性/AI 參數從 spawn 程式碼搬進 prototypes.json 的計劃；舊 PrototypeManager 以「窄而新」的形態回歸。**這仍是有效的未來方向。**

依賴：`src/gbind/zone_world_gd.cpp` 的 `setup_map()`（現行 spawn 硬編處）、
`src/core/components/`（npc_ai_component.h、controller_component.h、
act_point_component.h、health_component.h、combat_stats_component.h）。
session_log.md（PrototypeManager 已於先前重構刪除的記載）。

---

## 1. 現況與痛點

spawn 全部寫死在 `zone_world_gd.cpp` 的 `setup_map()`，每個實體手動 `emplace` 元件：

- 怪物只有一種「匿名怪」，全部 `ControllerKind::Self` + `decide_chase` 追擊
  （2026-06-21 重構後**已無施法者**——舊的 is_caster/nova/`npc_flame` 已隨技能系統移除）；
- 數值是常數＋樓層線性縮放（NPC_CAP/HP/ATK 等常數在 `setup_map()`）；
- 英雄是 `ControllerKind::Player`、最先加入 actor list；
- 道具只有 `health_potion`（隨機灑點）。

> 註：`NpcAiComponent` 仍殘留一個 `is_caster` 欄位，但**已無對應行為**
> （沒有施法者 AI），資料化時可一併清掉或忽略。

痛點：加一種怪 = 改 GDExtension 接線層的 C++ 並重編譯；core 測試（doctest）
無法生成「與遊戲一致的標準怪」，只能在測試裡手動 emplace 元件。

舊 PrototypeManager（opennefia-cpp 遺產）連同 spawn 已在先前重構刪除——
它揹著 OpenNefia 的 def 體系，太重。回歸版應該只做一件事：
**「id → 一組元件初始值」的查表 + 套用**。

## 2. 資料格式草案：data/prototypes.json（優先序：高）

```json
{
  "schema_version": 1,
  "prototypes": [
    { "id": "grunt", "hp": 10, "atk": 3,
      "controller": "self", "ai": "chase" },
    { "id": "brute", "hp": 18, "atk": 5,
      "controller": "self", "ai": "chase" },
    { "id": "hero",  "hp": 20, "atk": 5,
      "controller": "player" }
  ]
}
```

設計要點：

- `controller` 對應 `ControllerKind`（`player`/`self`/`faction`）——決定該 actor
  由誰操控、是否在輪到時暫停等玩家。`ai`（如 `chase`）只在 `controller: self`
  時有意義，對應 `decide_chase`。
- **暫不放 `caster_skill`/技能引用**：2026-06-21 重構後沒有技能系統了，等 01
  的資料驅動動作重新引入後，才在這裡加「技能名跨檔引用 actions.json + 引用驗證」。
- 不放 `speed`：目前是傳統回合制（每回合每 actor 行動一次），沒有能量/速度差。
  若未來引入 act_point 成本制（見 ActPointComponent 的預留），再加 `speed`/成本欄位。
- 樓層縮放不寫進 prototype（保持 def 純粹），先留在 spawn 端參數；
  03 的樓層主題落地後，縮放公式搬到 floor theme 那層。
- 英雄也是一筆 prototype（對稱 actor 模型，僅靠 `controller: player` 區分）。

## 3. C++ 端：PrototypeLibrary + instantiate（優先序：高）

三件套放 `src/core/proto/`（新目錄）：

```cpp
struct PrototypeDef {
    std::string id;
    int hp = 1, atk = 0;
    std::string controller = "self";  // "player" / "self" / "faction"
    std::string ai;                   // "" = 無 AI；"chase" = decide_chase
    // 未來再加：std::string skill（待 01 動作系統重新引入）、speed/cost（待成本制）
};

class PrototypeLibrary {  // load_json / load_defaults / find(id) / at(i)
};

// 套用：建 entity + emplace 各元件
// （Actor/Spatial/Health/CombatStats/ControllerComponent/ActPointComponent/NpcAi...）
entt::entity instantiate(entt::registry&, const PrototypeLibrary&,
                         const std::string& id, int x, int y);
```

- `instantiate` 放 core → doctest 可直接「按表生怪」測 AI/戰鬥，
  解掉測試手刻元件的重複碼。
- `zone_world_gd.cpp` 的 `setup_map()` spawn 迴圈縮成：選 id（暫按樓層輪替）
  → `instantiate`（會 emplace `ControllerComponent` + `ActPointComponent` 等）。
- 對應現有 `decide_chase`：`controller: self` + `ai: chase` 的怪，
  NPC 決策時就走 `decide_chase` 追英雄（與現況等價）。

## 4. spawn 表與樓層掛勾（優先序：中）

第二步才做：每樓層「出什麼怪、出幾隻、權重」也資料化。

```json
"spawn_tables": [
  { "floors": [1, 3], "cap_base": 5, "cap_per_floor": 1,
    "entries": [ { "proto": "grunt", "weight": 3 },
                 { "proto": "brute", "weight": 1 } ] }
]
```

放 prototypes.json 同檔或 `data/spawn.json` 皆可；建議同檔起步（檔少好管），
modding（06）要覆寫時再拆。與 03 的樓層主題天然同層——
屆時 spawn table 併入 floor theme 定義。

## 5. 與存檔的關係（重要規約，優先序：高——規約先立）

- **存檔存元件現值，不存 prototype 內容**：HealthComponent 等已在
  AllComponents 序列化清單，instantiate 之後 prototype 即功成身退。
- 但建議加 `ProtoIdComponent { std::string id; }` 入存檔：
  - debug/trace 顯示怪名（取代裸 eid）；
  - 未來「def 改版 + 舊存檔」要做 re-sync（如改了某怪 max_hp）時有依據；
  - 本地化（07）顯示名稱查表的 key。
- 風險：def 在兩次遊戲版本間改動 → 舊存檔的怪維持舊數值。**接受**——
  roguelike 單局短，不值得做存檔內 def 同步；規約寫進 05。

## 6. 分階段驗收

1. 第一波：PrototypeLibrary + instantiate + grunt/brute/hero 三筆 + doctest
   （載入/後備/instantiate 元件齊全，含 ControllerComponent/ActPointComponent）；
   遊戲行為與現況等價（怪物仍全是追擊近戰，無施法者）。
2. 第二波：spawn table + 樓層 cap 資料化；`zone_world_gd.cpp` 刪掉 NPC_* 常數。
3. 第三波（隨 03）：縮放公式進 floor theme。
