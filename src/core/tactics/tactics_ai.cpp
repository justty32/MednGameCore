#include "core/tactics/tactics_ai.h"

#include "core/tactics/move_range.h"
#include "core/components/spatial_component.h"
#include "core/components/health_component.h"
#include "core/components/tactics_unit_component.h"
#include "core/components/team_component.h"
#include "core/maps/map_data.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace zone::tactics {

namespace {

int chebyshev(int ax, int ay, int bx, int by) {
    return std::max(std::abs(ax - bx), std::abs(ay - by));
}

struct Foe {
    entt::entity e;
    int x, y, hp;
};

std::vector<Foe> enemies_of(const entt::registry& reg, uint8_t my_team) {
    std::vector<Foe> out;
    for (auto e : reg.view<const TacticsUnitComponent, const SpatialComponent,
                           const TeamComponent, const HealthComponent>()) {
        if (reg.get<const TeamComponent>(e).team == my_team) continue;
        const auto& sp = reg.get<const SpatialComponent>(e);
        const auto& hp = reg.get<const HealthComponent>(e);
        if (hp.hp <= 0) continue;
        out.push_back({ e, sp.x, sp.y, hp.hp });
    }
    // entity 序：讓「HP 平手取誰」這件事在不同平台上都一樣
    std::sort(out.begin(), out.end(), [](const Foe& a, const Foe& b) {
        return entt::to_integral(a.e) < entt::to_integral(b.e);
    });
    return out;
}

} // namespace

TacticsAction decide_tactics(const entt::registry& reg, const MapData& map,
                             entt::entity self, bool moved, bool acted) {
    if (!reg.valid(self)) return TacticsAction::wait();
    const auto* sp = reg.try_get<const SpatialComponent>(self);
    const auto* u  = reg.try_get<const TacticsUnitComponent>(self);
    const auto* tm = reg.try_get<const TeamComponent>(self);
    if (!sp || !u || !tm) return TacticsAction::wait();

    const std::vector<Foe> foes = enemies_of(reg, tm->team);
    if (foes.empty()) return TacticsAction::wait();

    // ---- 1. 射程內有敵人 → 打最脆的那個 ----
    if (!acted) {
        const Foe* best = nullptr;
        for (const auto& f : foes) {
            if (chebyshev(sp->x, sp->y, f.x, f.y) > u->attack_range) continue;
            if (!best || f.hp < best->hp) best = &f;   // 嚴格小於 → 平手取 entity 序在前者
        }
        if (best) return TacticsAction::attack(best->e);
    }

    // ---- 2. 還沒移動 → 走到「進得了射程、但不必貼身」的格子 ----
    //
    // 排序鍵（依序）：
    //   deficit = max(離最近敵人的距離 - 自己的射程, 0)   越小越好（進入射程）
    //   nearest                                          越大越好（進得了射程就別再靠近）
    //   steps                                            越小越好（同樣好就別多跑）
    //
    // 第二鍵是關鍵：少了它，射程 3 的弓手會一路走進近戰把自己送掉。
    if (!moved) {
        const MoveField field = compute_move_field(reg, map, self);

        Vector2i best_tile{ sp->x, sp->y };
        int best_deficit = std::numeric_limits<int>::max();
        int best_nearest = -1;
        int best_steps = 0;

        for (int y = 0; y < field.height; ++y) {
            for (int x = 0; x < field.width; ++x) {
                const int steps = field.at(x, y);
                if (steps < 0) continue;

                int nearest = std::numeric_limits<int>::max();
                for (const auto& f : foes)
                    nearest = std::min(nearest, chebyshev(x, y, f.x, f.y));
                const int deficit = std::max(nearest - u->attack_range, 0);

                const bool better =
                    deficit < best_deficit ||
                    (deficit == best_deficit && nearest > best_nearest) ||
                    (deficit == best_deficit && nearest == best_nearest && steps < best_steps);
                if (better) {
                    best_deficit = deficit;
                    best_nearest = nearest;
                    best_steps = steps;
                    best_tile = { x, y };
                }
            }
        }
        if (!(best_tile == Vector2i{ sp->x, sp->y }))
            return TacticsAction::move_to(best_tile.x, best_tile.y);
    }

    return TacticsAction::wait();
}

} // namespace zone::tactics
