#pragma once
#include <random>
#include <entt/entt.hpp>
#include "core/turn/action.h"

namespace zone {

struct MapData;

// NPC AI 調參。之後要做多種怪物時，把這包搬進 component 即可。
struct NpcAiConfig {
    int sight_radius{ 10 };  // 看得到英雄的最大 Chebyshev 距離（且需通過 los()）
    int alert_memory{ 6 };   // 失去視線後仍記得追擊的回合數
};

// decide_npc — 回合引擎的 NPC 決策函式（純決策，唯一副作用是更新自己的警覺狀態）。
//
// 世界的改動一律交給 apply_action()：本函式只回傳一個 Action。
// 攻擊沒有獨立動作——朝目標所在格 Move 即為撞擊攻擊。
//
// 決策順序：
//   1. 更新警覺：視線內 → alerted，記憶 alert_memory 回合；否則逐回合遞減。
//   2. 已警覺且與目標相鄰 → 朝目標 Move（＝攻擊）。不受 move_chance 閘門，
//      否則低 move_chance 的怪在貼身戰裡常常一次都還不了手。
//   3. move_chance 擲骰失敗 → Wait。（此閘門只管「移動」頻率。）
//   4. 已警覺 → 朝目標追擊：大 delta 軸優先，該軸受阻就換另一軸；
//      兩軸皆不可走 → 退化成徘徊。避開牆與其他 actor（不誤傷同類）。
//   5. 未警覺 → 隨機四方向徘徊；無路可走 → Wait。
Action decide_npc(entt::registry& reg, const MapData* map, entt::entity self,
                  entt::entity target, std::mt19937& rng, const NpcAiConfig& cfg = {});

} // namespace zone
