# 物品與裝備：從「踩到即用」到 inventory / 裝備欄 / 消耗品

> 日期：2026-06-11
> 定位：拾取雛形（resolve_move 內踩格觸發）演進為背包、裝備欄、消耗品三層的路線圖。

---

## 1. 現狀

依賴：`src/core/components/item_component.h`（`ItemComponent{type, value}`，
只有 health_potion）、`src/core/turn/zone_effects.cpp`（`resolve_move` 內
踩到物品 → 立即生效 → destroy entity）。

現狀是「地上的增益格」，不是物品系統。三步演進，每步獨立可玩。

## 2. 第一步：InventoryComponent——拾取進背包（優先序：高）

```cpp
struct ItemDef {           // 仿 ActionDef：JSON 資料化（data/items.json）
    std::string name;
    int use_action = -1;   // 使用時觸發的 ActionDef index（-1 = 不可使用）
    int stack_max = 1;     // 可堆疊上限
    // 裝備欄位見 §4
};

struct InventoryComponent {
    struct Slot { int def; int count; };
    std::vector<Slot> slots;   // 上限先定 10，逼出取捨
};
```

關鍵設計：**「使用物品」= 觸發一個 ActionDef**。
- 治療藥水 = `use_action` 指向 heal 類定義——`LibraryEffects`
  （`src/core/turn/zone_effects.h`）原語全部複用，物品系統**不需要自己的效果框架**。
- 投擲炸彈 = `use_action` 指向 smite_target 形技能（02 §1）——消耗品自動獲得
  瞄準、傷害類型、DoT 的全部能力。
- 使用 = 一個 Action（`ActionKind::UseItem`，param 帶 slot index，weight 1）——
  自然消耗回合、走排程器、可被 NPC 用（見 §6）。

牽動：`resolve_move` 拾取分支改為「塞進 InventoryComponent，滿則不撿」；
`zone_world_gd.cpp` 加 `submit_hero_use_item(slot)` + debug_text dump 背包。

## 3. 第二步：消耗品內容（優先序：高，隨 §2）

依賴：`data/actions.json` 既有原語即可全部表達——

| 物品 | use_action 內容 | 戰術定位 |
|---|---|---|
| 治療藥水 | self_heal 8 | 既有，搬家 |
| 再生藥水 | self_regen 5 回合 | 戰前準備 vs 急救的對比 |
| 炸彈 | smite_target r1 傷害 6 | 無 MP/cd 的爆發，數量即資源 |
| 毒刃油 | 給自己掛「攻擊附毒」buff（03 框架） | 戰前 buff 流 |
| 加速藥劑 | Haste 5 回合（03） | B/C 排程器下「偷回合」神器 |

roguelike 鐵律：**消耗品是「捨不得用」的決策**——掉落率調低、效果調強，
寧可玩家囤著死掉，不要變成例行公事。

## 4. 第三步：裝備欄（優先序：中，依賴 05 屬性模型）

```cpp
enum class EquipSlot : uint8_t { Weapon, Armor, Trinket };  // 三格起步，夠了

struct EquipmentComponent {
    int equipped[3]{-1,-1,-1};   // ItemDef index
};
// ItemDef 加：equip_slot（-1=不可裝備）、stat 修正欄位、proc 欄位
```

- **裝備加成走 03 的 effective_* 查詢層**：`effective_attack` 疊加順序 =
  base + 裝備 + buff。裝備=「永不過期的 stat-mod」，零新機制。
- **proc 裝備是趣味主力**：「攻擊 20% 附燃燒」「受擊時 10% 回 1 HP」——
  proc = 條件 + 觸發一個 ActionDef/TimedEffect，依舊複用原語。
  比「+2 攻擊」有趣一個數量級，內容投資優先放這。
- 換裝消耗回合（`ActionKind::Equip`，weight 1）——戰鬥中換裝是決策不是免費。

## 5. 掉落與生成（優先序：中）

依賴：`src/gbind/zone_world_gd.cpp`（spawn 邏輯）、`src/core/maps/`（BSP 生成）。

- NPC 死亡掉落：`die()` 保護路徑（session_log 修過的雙重 destroy bug 處）
  加掉落擲骰——在原格生成 item entity。掉落表先全域一張（JSON），
  之後按 NpcAi 類型分表。
- 地圖生成擺放：BSP 房間角落擺 0~2 件——已有踩格拾取，只差生成時機。
- **鑑定/詛咒系統明確不做**（低）：經典 roguelike 包袱，在多角色操控
  （06）下管理成本 ×N 倍，樂趣不成比例。

## 6. NPC 也用物品（優先序：低，但便宜）

對稱 actor 模型的紅利：`decide_chase`（`src/core/turn/npc_brain.cpp`）加一條
「HP < 30% 且背包有藥水 → UseItem」。NPC 喝藥的瞬間，玩家學會「斬殺要快」——
一條 if 換一層戰術，等 §2 落地後順手做。

## 7. 序列化提醒

`InventoryComponent`/`EquipmentComponent` 都要進 cereal snapshot
（`src/core/serialize/`，save/load round-trip 已是 verify 項目）。
ItemDef 與 ActionDef 同款 index 引用——存檔存 index，**JSON 改序會壞檔**，
與 ActionLibrary 相同的已知限制，記在 def 檔案頭注釋即可（正式版再做 name 引用）。
