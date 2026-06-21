#pragma once
#include <vector>
#include <entt/entt.hpp>
#include "core/turn/action.h"
#include "core/turn/zone_event.h"

namespace zone {

struct MapData;  // 前置宣告；.cpp 才 include map_data.h

// act() 的落地：把一個 Action 結算到世界上（最精簡版）。
//   Move  → 撞牆 / 撞 actor(攻擊) / 移動(順帶拾取道具、踩到下樓梯)
//   Wait/Idle → 不做事，消耗本回合
// 被殺死的目標只發 ActorDied 事件，不在此 destroy（避免迭代/順序失效），
// 由呼叫端（TurnEngine 使用者）統一移除。events 可為 null。
void apply_action(entt::registry& reg, MapData* map, entt::entity self,
                  const Action& a, std::vector<ZoneEvent>* events);

// 最精簡 NPC 決策：朝 target 移動一格（相鄰時即為撞擊攻擊）；無目標則等待。
Action decide_chase(entt::registry& reg, entt::entity self, entt::entity target);

} // namespace zone
