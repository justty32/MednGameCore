#pragma once
#include <vector>
#include <entt/entt.hpp>
#include "core/util/vector2i.h"

namespace zone {
struct MapData;
}

namespace zone::tactics {

// 站在 (x,y) 的戰棋單位（忽略 except）；沒有則回 entt::null。
entt::entity unit_at(const entt::registry& reg, int x, int y, entt::entity except);

// MoveField — 某單位本回合的可達集。
//
// 每格記錄「從起點走過來的步數」，-1 ＝ 不可達。四方向、每步花費 1；
// 牆與**其他單位佔著的格子**皆不可通行（不能穿人，也不能停在人身上）。
struct MoveField {
    int width{ 0 };
    int height{ 0 };
    Vector2i origin{ 0, 0 };
    std::vector<int> dist;   // row-major，index = y * width + x；-1 = 不可達

    bool in_bounds(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
    int at(int x, int y) const {
        return in_bounds(x, y) ? dist[(std::size_t)y * width + x] : -1;
    }
    bool reachable(int x, int y) const { return at(x, y) >= 0; }
};

// 計算 self 的可達集。步數上限取自 self 的 TacticsUnitComponent::move_range。
MoveField compute_move_field(const entt::registry& reg, const MapData& map, entt::entity self);

// 可達集內的所有格（含起點）。
std::vector<Vector2i> reachable_tiles(const MoveField& f);

// 從起點到 (tx,ty) 的路徑（含頭尾）。不可達則回空。
// 沿 dist 場回溯即可，不需要再看地圖或 registry。
// 核心結算是瞬移到目標格；這支只給前端做走路動畫。
std::vector<Vector2i> path_to(const MoveField& f, int tx, int ty);

} // namespace zone::tactics
