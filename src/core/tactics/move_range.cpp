#include "core/tactics/move_range.h"

#include "core/components/spatial_component.h"
#include "core/components/tactics_unit_component.h"
#include "core/maps/map_data.h"

#include <algorithm>
#include <deque>

namespace zone::tactics {

namespace {
constexpr int DX[4] = { 0,  0, 1, -1 };
constexpr int DY[4] = {-1,  1, 0,  0 };
} // namespace

entt::entity unit_at(const entt::registry& reg, int x, int y, entt::entity except) {
    for (auto e : reg.view<const TacticsUnitComponent, const SpatialComponent>()) {
        if (e == except) continue;
        const auto& sp = reg.get<const SpatialComponent>(e);
        if (sp.x == x && sp.y == y) return e;
    }
    return entt::null;
}

MoveField compute_move_field(const entt::registry& reg, const MapData& map, entt::entity self) {
    MoveField f;
    f.width = map.width;
    f.height = map.height;
    f.dist.assign((std::size_t)map.width * map.height, -1);

    const auto* sp = reg.try_get<const SpatialComponent>(self);
    const auto* u  = reg.try_get<const TacticsUnitComponent>(self);
    if (!sp || !u) return f;

    f.origin = { sp->x, sp->y };
    if (!map.in_bounds(sp->x, sp->y)) return f;

    // 每步花費固定為 1 → BFS 即是 Dijkstra，不必上優先佇列。
    // （之後若加地形移動消耗，這裡換成 Dijkstra，介面不變。）
    const int limit = std::max(0, u->move_range);
    f.dist[(std::size_t)sp->y * f.width + sp->x] = 0;

    std::deque<Vector2i> q;
    q.push_back({ sp->x, sp->y });
    while (!q.empty()) {
        const Vector2i cur = q.front();
        q.pop_front();
        const int d = f.at(cur.x, cur.y);
        if (d >= limit) continue;

        for (int i = 0; i < 4; ++i) {
            const int nx = cur.x + DX[i];
            const int ny = cur.y + DY[i];
            if (!map.in_bounds(nx, ny)) continue;
            if (f.at(nx, ny) != -1) continue;              // 已有更短或等長的路
            if (!map.at(nx, ny).is_walkable()) continue;   // 牆
            if (unit_at(reg, nx, ny, self) != entt::null) continue;  // 有人佔著：不能穿也不能停

            f.dist[(std::size_t)ny * f.width + nx] = d + 1;
            q.push_back({ nx, ny });
        }
    }
    return f;
}

std::vector<Vector2i> reachable_tiles(const MoveField& f) {
    std::vector<Vector2i> out;
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x)
            if (f.reachable(x, y)) out.push_back({ x, y });
    return out;
}

std::vector<Vector2i> path_to(const MoveField& f, int tx, int ty) {
    std::vector<Vector2i> path;
    if (!f.reachable(tx, ty)) return path;

    // 從終點沿著 dist 遞減回溯到起點
    Vector2i cur{ tx, ty };
    path.push_back(cur);
    while (!(cur == f.origin)) {
        const int d = f.at(cur.x, cur.y);
        bool stepped = false;
        for (int i = 0; i < 4 && !stepped; ++i) {
            const int nx = cur.x + DX[i];
            const int ny = cur.y + DY[i];
            if (f.at(nx, ny) != d - 1) continue;
            cur = { nx, ny };
            path.push_back(cur);
            stepped = true;
        }
        if (!stepped) return {};   // dist 場破損；寧可回空也不要無限迴圈
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace zone::tactics
