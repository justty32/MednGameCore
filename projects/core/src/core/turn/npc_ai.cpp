#include "core/turn/npc_ai.h"

#include "core/turn/apply_action.h"
#include "core/turn/move_dir.h"
#include "core/components/spatial_component.h"
#include "core/components/npc_ai_component.h"
#include "core/components/combat_stats_component.h"
#include "core/maps/map_data.h"
#include "core/systems/fov_system.h"

#include <algorithm>

namespace zone {

namespace {

// 四方向徘徊/追擊：N / S / E / W
constexpr int DX[4] = { 0,  0, 1, -1 };
constexpr int DY[4] = {-1,  1, 0,  0 };

int chebyshev(int ax, int ay, int bx, int by) {
    return std::max(std::abs(ax - bx), std::abs(ay - by));
}

// 該格能不能站？（在界內、可走、且沒有別的 actor 佔著）
// allow_occupant：目標所在格允許踏入——踏進去就是撞擊攻擊。
bool can_step(const entt::registry& reg, const MapData& map, entt::entity self,
              int x, int y, entt::entity allow_occupant) {
    if (!map.in_bounds(x, y) || !map.at(x, y).is_walkable()) return false;
    entt::entity occ = actor_at(reg, x, y, self);
    return occ == entt::null || occ == allow_occupant;
}

} // namespace

Action decide_npc(entt::registry& reg, const MapData* map, entt::entity self,
                  entt::entity target, std::mt19937& rng, const NpcAiConfig& cfg) {
    if (!reg.valid(self)) return Action::wait();
    const auto* sp = reg.try_get<SpatialComponent>(self);
    auto* ai = reg.try_get<NpcAiComponent>(self);
    if (!sp || !map) return Action::wait();

    // 目標可能已死亡 / 不存在（例如英雄已被移除）
    const SpatialComponent* tsp = nullptr;
    if (target != entt::null && reg.valid(target))
        tsp = reg.try_get<SpatialComponent>(target);

    int move_chance = 50;
    if (const auto* cs = reg.try_get<CombatStatsComponent>(self))
        move_chance = cs->move_chance;

    // ---- 1. 更新警覺（每回合必做，不受 move_chance 閘門）----
    // 否則 move_chance 偏低的 NPC 連「發現英雄」都會被機率閘門吃掉。
    if (ai && tsp) {
        const bool can_see = chebyshev(sp->x, sp->y, tsp->x, tsp->y) <= cfg.sight_radius
                          && los(*map, sp->x, sp->y, tsp->x, tsp->y);
        if (can_see) {
            ai->alerted     = true;
            ai->alert_turns = cfg.alert_memory;
        } else if (ai->alerted) {
            if (--ai->alert_turns <= 0) ai->alerted = false;
        }
    }
    const bool alerted = ai && ai->alerted && tsp != nullptr;

    // ---- 2. 已警覺且相鄰 → 撞擊攻擊（不受 move_chance 閘門）----
    if (alerted && chebyshev(sp->x, sp->y, tsp->x, tsp->y) == 1) {
        const int dx = (tsp->x > sp->x) - (tsp->x < sp->x);
        const int dy = (tsp->y > sp->y) - (tsp->y < sp->y);
        return Action::move(encode_dir(dx, dy));
    }

    // ---- 3. move_chance 閘門：以下「移動」（追擊 / 徘徊）才受限 ----
    std::uniform_int_distribution<int> pct(0, 99);
    if (pct(rng) >= move_chance) return Action::wait();

    // ---- 4. 追擊：大 delta 軸優先，受阻換另一軸 ----
    if (alerted) {
        const int hdx = tsp->x - sp->x;
        const int hdy = tsp->y - sp->y;
        const int cx = (hdx > 0) - (hdx < 0);
        const int cy = (hdy > 0) - (hdy < 0);

        int try_x[2], try_y[2];
        if (std::abs(hdx) >= std::abs(hdy)) {
            try_x[0] = cx; try_y[0] = 0;
            try_x[1] = 0;  try_y[1] = cy;
        } else {
            try_x[0] = 0;  try_y[0] = cy;
            try_x[1] = cx; try_y[1] = 0;
        }
        for (int i = 0; i < 2; ++i) {
            if (try_x[i] == 0 && try_y[i] == 0) continue;
            if (can_step(reg, *map, self, sp->x + try_x[i], sp->y + try_y[i], target))
                return Action::move(encode_dir(try_x[i], try_y[i]));
        }
        // 兩軸皆受阻 → 落到徘徊，至少不會貼著牆空轉一輩子
    }

    // ---- 5. 徘徊：隨機起點掃四方向，避開牆與其他 actor ----
    std::uniform_int_distribution<int> dir_dist(0, 3);
    const int start = dir_dist(rng);
    for (int i = 0; i < 4; ++i) {
        const int d = (start + i) % 4;
        if (can_step(reg, *map, self, sp->x + DX[d], sp->y + DY[d], entt::null))
            return Action::move(encode_dir(DX[d], DY[d]));
    }
    return Action::wait();
}

} // namespace zone
