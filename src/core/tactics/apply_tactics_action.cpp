#include "core/tactics/apply_tactics_action.h"

#include "core/tactics/move_range.h"
#include "core/components/spatial_component.h"
#include "core/components/health_component.h"
#include "core/components/combat_stats_component.h"
#include "core/components/tactics_unit_component.h"
#include "core/components/team_component.h"
#include "core/maps/map_data.h"

#include <algorithm>

namespace zone::tactics {

namespace {

void push(std::vector<TacticsEvent>* ev, const TacticsEvent& e) {
    if (ev) ev->push_back(e);
}

bool reject(std::vector<TacticsEvent>* ev, entt::entity self) {
    push(ev, { TacticsEventKind::Rejected, self });
    return false;
}

int chebyshev(int ax, int ay, int bx, int by) {
    return std::max(std::abs(ax - bx), std::abs(ay - by));
}

} // namespace

bool apply_tactics_action(entt::registry& reg, const MapData& map, entt::entity self,
                          const TacticsAction& a, std::vector<TacticsEvent>* ev) {
    if (!reg.valid(self)) return false;

    switch (a.kind) {
        case TacticsActionKind::Wait:
            return true;

        case TacticsActionKind::MoveTo: {
            auto* sp = reg.try_get<SpatialComponent>(self);
            if (!sp) return reject(ev, self);

            const MoveField f = compute_move_field(reg, map, self);
            if (!f.reachable(a.x, a.y)) return reject(ev, self);

            const int fx = sp->x, fy = sp->y;
            sp->x = a.x;
            sp->y = a.y;
            push(ev, { TacticsEventKind::Moved, self, entt::null, 0, fx, fy });
            return true;
        }

        case TacticsActionKind::Attack: {
            if (!reg.valid(a.target) || a.target == self) return reject(ev, self);

            const auto* sp = reg.try_get<SpatialComponent>(self);
            const auto* tp = reg.try_get<SpatialComponent>(a.target);
            const auto* u  = reg.try_get<TacticsUnitComponent>(self);
            if (!sp || !tp || !u) return reject(ev, self);

            // 只能打敵隊：沒有 TeamComponent 一律視為不可攻擊，寧可拒絕也不誤傷
            const auto* my_team = reg.try_get<TeamComponent>(self);
            const auto* tg_team = reg.try_get<TeamComponent>(a.target);
            if (!my_team || !tg_team || my_team->team == tg_team->team)
                return reject(ev, self);

            if (chebyshev(sp->x, sp->y, tp->x, tp->y) > u->attack_range)
                return reject(ev, self);

            auto* hp = reg.try_get<HealthComponent>(a.target);
            if (!hp) return reject(ev, self);

            int dmg = 2;
            if (const auto* cs = reg.try_get<CombatStatsComponent>(self)) dmg = cs->attack;
            hp->hp -= dmg;

            if (hp->hp <= 0)
                push(ev, { TacticsEventKind::UnitDied, self, a.target, dmg });
            else
                push(ev, { TacticsEventKind::Attacked, self, a.target, dmg });
            return true;
        }
    }
    return reject(ev, self);
}

} // namespace zone::tactics
