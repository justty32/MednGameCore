#pragma once
#include <entt/entt.hpp>
#include "core/tactics/tactics_action.h"

namespace zone {
struct MapData;
}

namespace zone::tactics {

// 敵方單位的決策（最小版）。純決策：只讀世界、回傳一個 TacticsAction，
// 世界改動交給 apply_tactics_action()——與地城的 decide_npc() 同一個模式。
//
// moved / acted 是 self 在**本回合**已經用掉的子動作，由 Battle 傳入：
//   1. 還沒行動且射程內有敵人 → 攻擊 HP 最低者（平手取 entity 序，確定性）
//   2. 還沒移動 → 走到可達集中「離最近敵人 Chebyshev 距離最小」的格子
//                （平手取步數少者，再平手取 row-major 序）
//   3. 其餘 → Wait
//
// Battle 會反覆呼叫直到回合結束，所以「移動後進入射程 → 攻擊」自然由第 1 條接手。
TacticsAction decide_tactics(const entt::registry& reg, const MapData& map,
                             entt::entity self, bool moved, bool acted);

} // namespace zone::tactics
