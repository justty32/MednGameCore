# 實體原型（prototype）資料化

> 日期：2026-06-11
> 定位：把怪物種類/屬性/AI 參數從 spawn 程式碼搬進 prototypes.json 的計劃；舊 PrototypeManager 以「窄而新」的形態回歸。

依賴：`src/gbind/zone_world_gd.cpp`（現行 spawn 硬編處）、
`src/core/components/`（npc_ai_component.h、energy_component.h、health_component.h、
combat_stats_component.h）、`src/core/turn/action_def.h`（ActionLibrary 載入模式可複製）、
session_log.md（PrototypeManager 已於重構時刪除的記載）。

---

## 1. 現況與痛點

spawn 全部寫死在 `zone_world_gd.cpp` 的地圖生成段落：

- 怪物只有一種「匿名怪」，以 `npc_count % 2 == 0` 決定 is_caster；
- 數值是常數＋樓層線性縮放（NPC_BASE_HP + floor*HP_SCALE、ATK 同理）；
- 速度寫死 caster=80 / 近戰=130（當初只為了讓能量面板看得出差異）；
- caster 的技能寫死查 `"npc_flame"`（在 `npc_brain.cpp`）。

痛點：加一種怪 = 改 GDExtension 接線層的 C++ 並重編譯；core 測試（ctest）
無法生成「與遊戲一致的標準怪」，只能手動 emplace 元件。

舊 PrototypeManager（opennefia-cpp 遺產）連同 spawn 已在重構時刪除——
它揹著 OpenNefia 的 def 體系，太重。回歸版應該只做一件事：
**「id → 一組元件初始值」的查表 + 套用**。

## 2. 資料格式草案：data/prototypes.json（優先序：高）

```json
{
  "schema_version": 1,
  "prototypes": [
    { "id": "grunt",  "hp": 10, "atk": 3, "speed": 130,
      "ai": "chase" },
    { "id": "imp",    "hp": 6,  "atk": 2, "speed": 80,
      "ai": "chase", "caster_skill": "npc_flame" },
    { "id": "hero",   "hp": 20, "atk": 5, "speed": 100,
      "player": true }
  ]
}
```

設計要點：

- `caster_skill` 直接存技能名（跨檔引用 actions.json）——取代 `is_caster` bool。
  載入後做**引用驗證**：skill 名在 ActionLibrary 找不到 → error（沿用 01 §3 的
  LoadResult 模式）。
- 樓層縮放不寫進 prototype（保持 def 純粹），先留在 spawn 端參數；
  03 的樓層主題落地後，縮放公式搬到 floor theme 那層。
- 英雄也是一筆 prototype（對稱 actor 模型，無玩家特判）。

## 3. C++ 端：PrototypeLibrary + instantiate（優先序：高）

仿 ActionLibrary 的成功三件套，放 `src/core/proto/`（新目錄）：

```cpp
struct PrototypeDef {
    std::string id;
    int hp = 1, atk = 0, speed = 100;
    std::string ai;            // "" = 無 AI；"chase" = decide_chase
    std::string caster_skill;  // "" = 非施法者
    bool player = false;
};

class PrototypeLibrary {  // load_json / load_defaults / find(id) / at(i)
};

// 套用：建 entity + emplace 各元件（Health/CombatStats/Energy/NpcAi/...）
entt::entity instantiate(entt::registry&, const PrototypeLibrary&,
                         const std::string& id, int x, int y);
```

- `instantiate` 放 core → ctest 可直接「按表生怪」測 AI/戰鬥，
  解掉測試手刻元件的重複碼。
- `zone_world_gd.cpp` 的 spawn 迴圈縮成：選 id（暫按樓層輪替）→ `instantiate`。
- `npc_brain.cpp` 的 `decide_chase` 改讀 `NpcAiComponent::caster_skill`
  （新欄位，從 prototype 填入）取代寫死的 `"npc_flame"` + is_caster。

## 4. spawn 表與樓層掛勾（優先序：中）

第二步才做：每樓層「出什麼怪、出幾隻、權重」也資料化。

```json
"spawn_tables": [
  { "floors": [1, 3], "cap_base": 5, "cap_per_floor": 1,
    "entries": [ { "proto": "grunt", "weight": 3 },
                 { "proto": "imp",   "weight": 1 } ] }
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

1. 第一波：PrototypeLibrary + instantiate + grunt/imp/hero 三筆 + ctest
   （載入/後備/instantiate 元件齊全/引用驗證抓壞 skill 名）；
   遊戲行為與現況等價（半數 caster 改為 grunt/imp 輪替，速度 130/80 不變）。
2. 第二波：spawn table + 樓層 cap 資料化；`zone_world_gd.cpp` 刪掉 NPC_* 常數。
3. 第三波（隨 03）：縮放公式進 floor theme。
