#include "core/turn/apply_action.h"
#include "core/turn/move_dir.h"
#include "core/maps/map_data.h"
#include "core/components/actor_component.h"
#include "core/components/spatial_component.h"
#include "core/components/health_component.h"
#include "core/components/combat_stats_component.h"
#include "core/components/item_component.h"

#include <algorithm>

namespace zone {

namespace {

void push(std::vector<ZoneEvent>* ev, const ZoneEvent& e) {
    if (ev) ev->push_back(e);
}

// 站在 (x,y) 的其他 actor（用於撞擊攻擊判定）。
entt::entity actor_at(entt::registry& reg, int x, int y, entt::entity except) {
    for (auto e : reg.view<ActorComponent, SpatialComponent>()) {
        if (e == except) continue;
        const auto& sp = reg.get<SpatialComponent>(e);
        if (sp.x == x && sp.y == y) return e;
    }
    return entt::null;
}

} // namespace

void apply_action(entt::registry& reg, MapData* map, entt::entity self,
                  const Action& a, std::vector<ZoneEvent>* ev) {
    if (!reg.valid(self)) return;
    if (a.kind != ActionKind::Move) return;  // Wait/Idle：消耗回合，不做事

    auto* sp = reg.try_get<SpatialComponent>(self);
    if (!sp) return;

    int dx = 0, dy = 0;
    decode_dir(a.param, dx, dy);
    if (dx == 0 && dy == 0) return;

    const int nx = sp->x + dx, ny = sp->y + dy;

    // 撞牆 / 出界
    if (map && (!map->in_bounds(nx, ny) || !map->at(nx, ny).is_walkable())) {
        push(ev, { EventKind::BumpedWall, self });
        return;
    }

    // 撞 actor → 攻擊
    if (entt::entity tgt = actor_at(reg, nx, ny, self); tgt != entt::null) {
        int dmg = 2;
        if (const auto* cs = reg.try_get<CombatStatsComponent>(self)) dmg = cs->attack;
        if (auto* hp = reg.try_get<HealthComponent>(tgt)) {
            hp->hp -= dmg;
            if (hp->hp <= 0)
                push(ev, { EventKind::ActorDied, self, tgt, dmg });
            else
                push(ev, { EventKind::BumpedActor, self, tgt, dmg });
        }
        return;
    }

    // 移動
    sp->x = nx;
    sp->y = ny;

    // 拾取道具（自動）
    for (auto e : reg.view<ItemComponent, SpatialComponent>()) {
        const auto& isp = reg.get<SpatialComponent>(e);
        if (isp.x != nx || isp.y != ny) continue;
        const auto& item = reg.get<ItemComponent>(e);
        int healed = 0;
        if (item.type == ItemType::health_potion) {
            if (auto* hp = reg.try_get<HealthComponent>(self)) {
                healed = std::min(item.value, hp->max_hp - hp->hp);
                hp->hp += healed;
            }
        }
        reg.destroy(e);
        push(ev, { EventKind::ItemPickedUp, self, entt::null, healed });
        break;
    }

    // 踩到下樓梯
    if (map && map->at(nx, ny).is_stair_down())
        push(ev, { EventKind::ReachedStairDown, self });
}

Action decide_chase(entt::registry& reg, entt::entity self, entt::entity target) {
    if (!reg.valid(self) || !reg.valid(target)) return Action::wait();
    const auto* s = reg.try_get<SpatialComponent>(self);
    const auto* t = reg.try_get<SpatialComponent>(target);
    if (!s || !t) return Action::wait();

    const int dx = (t->x > s->x) - (t->x < s->x);  // -1 / 0 / +1
    const int dy = (t->y > s->y) - (t->y < s->y);
    if (dx == 0 && dy == 0) return Action::wait();
    return Action::move(encode_dir(dx, dy));
}

} // namespace zone
