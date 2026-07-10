#pragma once
#include <vector>
#include <entt/entt.hpp>
#include "core/tactics/tactics_action.h"
#include "core/tactics/tactics_event.h"

namespace zone {
struct MapData;
}

namespace zone::tactics {

// 把一個 TacticsAction 結算到世界上。
//
// 只驗證**幾何合法性**（可達 / 射程 / 是敵人），不管「這回合是不是已經移動過了」
// ——那是單位回合的子狀態，屬於 Battle 的職責。
//
// 回傳 true ＝ 指令被接受並已生效；false ＝ 非法，世界未改變（並發 Rejected 事件）。
// 被殺死的目標只發 UnitDied 事件，不在此 destroy（避免迭代失效），由 Battle 移除。
bool apply_tactics_action(entt::registry& reg, const MapData& map, entt::entity self,
                          const TacticsAction& a, std::vector<TacticsEvent>* events);

} // namespace zone::tactics
