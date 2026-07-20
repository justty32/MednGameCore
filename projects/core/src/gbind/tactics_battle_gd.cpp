#include "tactics_battle_gd.h"

#include "core/components/spatial_component.h"
#include "core/components/health_component.h"
#include "core/components/combat_stats_component.h"
#include "core/components/team_component.h"
#include "core/components/tactics_unit_component.h"
#include "core/components/turn_gauge_component.h"
#include "core/tactics/move_range.h"
#include "core/tactics/tactics_action.h"
#include "core/maps/tile.h"
#include "core/util/vector2i.h"

#include <algorithm>
#include <deque>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;
namespace tac = zone::tactics;

// 競技場常數
static constexpr int ARENA_W    = 20;
static constexpr int ARENA_H    = 14;
static constexpr int WALL_PCT   = 12;   // 內部格變成障礙的機率
static constexpr int SPAWN_COLS = 3;    // 左右各留幾欄淨空，避免出生點被封死

// 單位原型：{ hp, attack, move_range, attack_range, speed }
struct Archetype { int hp, attack, move_range, attack_range, speed; };
static constexpr Archetype PLAYER_UNITS[] = {
    { 18, 5, 3, 1,  90 },   // 戰士：硬、慢、近戰
    { 11, 4, 3, 3, 100 },   // 弓手：脆、遠程
    { 13, 3, 5, 1, 130 },   // 斥候：快、機動
};
static constexpr Archetype ENEMY_UNITS[] = {
    { 15, 4, 3, 1,  90 },
    { 10, 4, 3, 3, 100 },
    { 12, 3, 5, 1, 120 },
};

static constexpr uint8_t TF_WALKABLE = 1 << 0;

// ---- binding ---------------------------------------------------------------

void zone_gd::TacticsBattle::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_battle", "seed"), &TacticsBattle::start_battle);
    ClassDB::bind_method(D_METHOD("set_seed", "seed"), &TacticsBattle::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &TacticsBattle::get_seed);

    ClassDB::bind_method(D_METHOD("get_map_width"),  &TacticsBattle::get_map_width);
    ClassDB::bind_method(D_METHOD("get_map_height"), &TacticsBattle::get_map_height);
    ClassDB::bind_method(D_METHOD("get_tile_flags"), &TacticsBattle::get_tile_flags);

    ClassDB::bind_method(D_METHOD("get_units"), &TacticsBattle::get_units);
    ClassDB::bind_method(D_METHOD("get_turn_order", "n"), &TacticsBattle::get_turn_order);

    ClassDB::bind_method(D_METHOD("current_unit"), &TacticsBattle::current_unit);
    ClassDB::bind_method(D_METHOD("is_waiting_player"), &TacticsBattle::is_waiting_player);
    ClassDB::bind_method(D_METHOD("can_move"), &TacticsBattle::can_move);
    ClassDB::bind_method(D_METHOD("can_act"),  &TacticsBattle::can_act);

    ClassDB::bind_method(D_METHOD("get_move_tiles"), &TacticsBattle::get_move_tiles);
    ClassDB::bind_method(D_METHOD("get_attack_targets"), &TacticsBattle::get_attack_targets);
    ClassDB::bind_method(D_METHOD("get_path_to", "x", "y"), &TacticsBattle::get_path_to);

    ClassDB::bind_method(D_METHOD("command_move", "x", "y"), &TacticsBattle::command_move);
    ClassDB::bind_method(D_METHOD("command_attack", "unit_id"), &TacticsBattle::command_attack);
    ClassDB::bind_method(D_METHOD("command_wait"), &TacticsBattle::command_wait);

    ClassDB::bind_method(D_METHOD("get_winner"), &TacticsBattle::get_winner);
    ClassDB::bind_method(D_METHOD("is_battle_over"), &TacticsBattle::is_battle_over);

    ADD_SIGNAL(MethodInfo("turn_started", PropertyInfo(Variant::INT, "unit_id")));
    ADD_SIGNAL(MethodInfo("unit_moved",
        PropertyInfo(Variant::INT, "unit_id"),
        PropertyInfo(Variant::INT, "from_x"), PropertyInfo(Variant::INT, "from_y")));
    ADD_SIGNAL(MethodInfo("unit_attacked",
        PropertyInfo(Variant::INT, "attacker_id"),
        PropertyInfo(Variant::INT, "target_id"),
        PropertyInfo(Variant::INT, "damage")));
    ADD_SIGNAL(MethodInfo("unit_died", PropertyInfo(Variant::INT, "unit_id")));
    ADD_SIGNAL(MethodInfo("battle_over", PropertyInfo(Variant::INT, "winning_team")));
}

// ---- lifecycle -------------------------------------------------------------

zone_gd::TacticsBattle::TacticsBattle() {
    seed_ = std::random_device{}();
    rng_.seed(seed_);
}

void zone_gd::TacticsBattle::set_seed(int seed) {
    seed_ = static_cast<uint32_t>(seed);
    rng_.seed(seed_);
}

int zone_gd::TacticsBattle::get_seed() const { return static_cast<int>(seed_); }

int zone_gd::TacticsBattle::to_id(entt::entity e) {
    return static_cast<int>(entt::to_integral(e));
}

entt::entity zone_gd::TacticsBattle::to_entity(int id) const {
    if (id < 0) return entt::null;
    auto e = static_cast<entt::entity>(static_cast<entt::id_type>(id));
    return reg_.valid(e) ? e : entt::null;
}

// ---- 開局 ------------------------------------------------------------------

void zone_gd::TacticsBattle::start_battle(int seed) {
    set_seed(seed);
    reg_.clear();
    battle_.reset();
    events_.clear();
    battle_over_emitted_ = false;

    build_arena();
    spawn_units();
    pump();
}

// 開闊競技場 + 散落障礙。左右各留 SPAWN_COLS 欄淨空當出生區，
// 並用 flood fill 確認整張圖連通——否則某一隊會被牆困死，戰鬥永遠打不完。
void zone_gd::TacticsBattle::build_arena() {
    std::uniform_int_distribution<int> pct(0, 99);

    for (int attempt = 0; attempt < 32; ++attempt) {
        map_.resize(ARENA_W, ARENA_H);
        for (int y = 0; y < ARENA_H; ++y)
            for (int x = 0; x < ARENA_W; ++x)
                map_.at(x, y).flags = zone::TILE_WALKABLE;

        // 外框牆
        for (int x = 0; x < ARENA_W; ++x) {
            map_.at(x, 0).flags = 0;
            map_.at(x, ARENA_H - 1).flags = 0;
        }
        for (int y = 0; y < ARENA_H; ++y) {
            map_.at(0, y).flags = 0;
            map_.at(ARENA_W - 1, y).flags = 0;
        }

        // 內部障礙（跳過左右出生欄）
        for (int y = 1; y < ARENA_H - 1; ++y)
            for (int x = 1 + SPAWN_COLS; x < ARENA_W - 1 - SPAWN_COLS; ++x)
                if (pct(rng_) < WALL_PCT)
                    map_.at(x, y).flags = 0;

        // 連通性檢查：從左出生區某格 flood fill，必須能碰到右出生區
        std::vector<uint8_t> seen((std::size_t)ARENA_W * ARENA_H, 0);
        std::deque<zone::Vector2i> q;
        q.push_back({ 1, 1 });
        seen[(std::size_t)1 * ARENA_W + 1] = 1;
        int reached_right = 0;
        while (!q.empty()) {
            auto c = q.front(); q.pop_front();
            if (c.x == ARENA_W - 2) ++reached_right;
            constexpr int DX[4] = { 0, 0, 1, -1 };
            constexpr int DY[4] = { -1, 1, 0, 0 };
            for (int i = 0; i < 4; ++i) {
                const int nx = c.x + DX[i], ny = c.y + DY[i];
                if (!map_.in_bounds(nx, ny)) continue;
                if (seen[(std::size_t)ny * ARENA_W + nx]) continue;
                if (!map_.at(nx, ny).is_walkable()) continue;
                seen[(std::size_t)ny * ARENA_W + nx] = 1;
                q.push_back({ nx, ny });
            }
        }
        if (reached_right >= 3) return;   // 夠通了
    }
    // 32 次都失敗（機率極低）：退回全開闊，寧可無趣也不要打不完
    for (int y = 1; y < ARENA_H - 1; ++y)
        for (int x = 1; x < ARENA_W - 1; ++x)
            map_.at(x, y).flags = zone::TILE_WALKABLE;
}

void zone_gd::TacticsBattle::spawn_units() {
    auto make = [&](int x, int y, uint8_t team, const Archetype& a) {
        auto e = reg_.create();
        reg_.emplace<zone::SpatialComponent>(e, x, y);
        reg_.emplace<zone::HealthComponent>(e, a.hp, a.hp);
        reg_.emplace<zone::CombatStatsComponent>(e,
            zone::CombatStatsComponent{ a.attack, 100 });
        reg_.emplace<zone::TeamComponent>(e, zone::TeamComponent{ team });
        reg_.emplace<zone::TacticsUnitComponent>(e,
            zone::TacticsUnitComponent{ a.move_range, a.attack_range, a.speed });
        reg_.emplace<zone::TurnGaugeComponent>(e);
        battle_.add_unit(reg_, e);
    };

    const int n = (int)(sizeof(PLAYER_UNITS) / sizeof(PLAYER_UNITS[0]));
    const int y0 = ARENA_H / 2 - n;   // 直向排開、置中
    for (int i = 0; i < n; ++i) {
        make(2, y0 + i * 2, zone::TEAM_PLAYER, PLAYER_UNITS[i]);
        make(ARENA_W - 3, y0 + i * 2, zone::TEAM_ENEMY, ENEMY_UNITS[i]);
    }
}

// ---- 推進 ------------------------------------------------------------------

void zone_gd::TacticsBattle::pump() {
    for (;;) {
        if (battle_.winner(reg_) != tac::Battle::kNoWinner) {
            if (!battle_over_emitted_) {
                battle_over_emitted_ = true;
                emit_signal("battle_over", battle_.winner(reg_));
            }
            return;
        }

        if (!battle_.turn_active()) {
            entt::entity e = battle_.begin_next_turn(reg_);
            if (e == entt::null) return;          // 沒人能動：交給下一次 winner 檢查
            emit_signal("turn_started", to_id(e));
        }

        entt::entity cur = battle_.current();
        if (cur == entt::null) return;

        const auto* team = reg_.try_get<zone::TeamComponent>(cur);
        if (team && team->team == zone::TEAM_PLAYER) return;   // 等玩家下指令

        // 敵方：AI 自己打完一回合
        events_.clear();
        battle_.run_ai_turn(reg_, map_, &events_);
        drain_events();
    }
}

void zone_gd::TacticsBattle::drain_events() {
    for (const auto& ev : events_) {
        switch (ev.kind) {
            case tac::TacticsEventKind::Moved:
                emit_signal("unit_moved", to_id(ev.a), ev.from_x, ev.from_y);
                break;
            case tac::TacticsEventKind::Attacked:
                emit_signal("unit_attacked", to_id(ev.a), to_id(ev.b), ev.amount);
                break;
            case tac::TacticsEventKind::UnitDied:
                emit_signal("unit_attacked", to_id(ev.a), to_id(ev.b), ev.amount);
                emit_signal("unit_died", to_id(ev.b));
                break;
            case tac::TacticsEventKind::Rejected:
                break;   // 前端本來就該先用 get_move_tiles/get_attack_targets 過濾
        }
    }
    events_.clear();
}

void zone_gd::TacticsBattle::after_player_command() {
    drain_events();
    pump();   // 玩家回合結束 → 繼續跑 AI，直到再次輪到玩家或分出勝負
}

// ---- 玩家指令 --------------------------------------------------------------

bool zone_gd::TacticsBattle::command_move(int x, int y) {
    if (!is_waiting_player()) return false;
    events_.clear();
    const bool ok = battle_.command(reg_, map_, tac::TacticsAction::move_to(x, y), &events_);
    if (ok) after_player_command();
    return ok;
}

bool zone_gd::TacticsBattle::command_attack(int unit_id) {
    if (!is_waiting_player()) return false;
    entt::entity t = to_entity(unit_id);
    if (t == entt::null) return false;
    events_.clear();
    const bool ok = battle_.command(reg_, map_, tac::TacticsAction::attack(t), &events_);
    if (ok) after_player_command();
    return ok;
}

bool zone_gd::TacticsBattle::command_wait() {
    if (!is_waiting_player()) return false;
    events_.clear();
    const bool ok = battle_.command(reg_, map_, tac::TacticsAction::wait(), &events_);
    if (ok) after_player_command();
    return ok;
}

// ---- 查詢 ------------------------------------------------------------------

int zone_gd::TacticsBattle::get_map_width()  const { return map_.width; }
int zone_gd::TacticsBattle::get_map_height() const { return map_.height; }

godot::PackedByteArray zone_gd::TacticsBattle::get_tile_flags() const {
    PackedByteArray out;
    if (map_.empty()) return out;
    out.resize(map_.width * map_.height);
    uint8_t* w = out.ptrw();
    for (int y = 0; y < map_.height; ++y)
        for (int x = 0; x < map_.width; ++x)
            w[y * map_.width + x] = map_.at(x, y).is_walkable() ? TF_WALKABLE : 0;
    return out;
}

godot::Array zone_gd::TacticsBattle::get_units() const {
    Array out;
    for (auto e : reg_.view<const zone::TacticsUnitComponent, const zone::SpatialComponent,
                            const zone::TeamComponent, const zone::HealthComponent>()) {
        const auto& sp = reg_.get<const zone::SpatialComponent>(e);
        const auto& hp = reg_.get<const zone::HealthComponent>(e);
        const auto& u  = reg_.get<const zone::TacticsUnitComponent>(e);
        const auto& tm = reg_.get<const zone::TeamComponent>(e);

        Dictionary d;
        d["id"] = to_id(e);
        d["x"] = sp.x;
        d["y"] = sp.y;
        d["team"] = (int)tm.team;
        d["hp"] = std::max(0, hp.hp);
        d["max_hp"] = hp.max_hp;
        d["move_range"] = u.move_range;
        d["attack_range"] = u.attack_range;
        d["speed"] = u.speed;
        d["attack"] = reg_.all_of<zone::CombatStatsComponent>(e)
            ? reg_.get<const zone::CombatStatsComponent>(e).attack : 0;
        d["ct"] = reg_.all_of<zone::TurnGaugeComponent>(e)
            ? reg_.get<const zone::TurnGaugeComponent>(e).ct : 0;
        out.append(d);
    }
    return out;
}

godot::Array zone_gd::TacticsBattle::get_turn_order(int n) const {
    Array out;
    for (auto e : battle_.bar().forecast(reg_, n)) out.append(to_id(e));
    return out;
}

int zone_gd::TacticsBattle::current_unit() const {
    entt::entity e = battle_.current();
    return (e == entt::null) ? -1 : to_id(e);
}

bool zone_gd::TacticsBattle::is_waiting_player() const {
    if (!battle_.turn_active()) return false;
    entt::entity e = battle_.current();
    if (e == entt::null || !reg_.valid(e)) return false;
    const auto* tm = reg_.try_get<const zone::TeamComponent>(e);
    return tm && tm->team == zone::TEAM_PLAYER;
}

bool zone_gd::TacticsBattle::can_move() const { return battle_.turn_active() && !battle_.moved() && !battle_.acted(); }
bool zone_gd::TacticsBattle::can_act()  const { return battle_.turn_active() && !battle_.acted(); }

godot::PackedVector2Array zone_gd::TacticsBattle::get_move_tiles() const {
    PackedVector2Array out;
    if (!battle_.turn_active() || !can_move()) return out;
    entt::entity e = battle_.current();
    if (e == entt::null) return out;

    const tac::MoveField f = tac::compute_move_field(reg_, map_, e);
    for (const auto& t : tac::reachable_tiles(f))
        out.append(Vector2(t.x, t.y));
    return out;
}

godot::Array zone_gd::TacticsBattle::get_attack_targets() const {
    Array out;
    if (!can_act()) return out;
    entt::entity self = battle_.current();
    if (self == entt::null || !reg_.valid(self)) return out;

    const auto* sp = reg_.try_get<const zone::SpatialComponent>(self);
    const auto* u  = reg_.try_get<const zone::TacticsUnitComponent>(self);
    const auto* tm = reg_.try_get<const zone::TeamComponent>(self);
    if (!sp || !u || !tm) return out;

    for (auto e : reg_.view<const zone::TacticsUnitComponent, const zone::SpatialComponent,
                            const zone::TeamComponent>()) {
        if (e == self) continue;
        if (reg_.get<const zone::TeamComponent>(e).team == tm->team) continue;
        const auto& tp = reg_.get<const zone::SpatialComponent>(e);
        const int d = std::max(std::abs(tp.x - sp->x), std::abs(tp.y - sp->y));
        if (d <= u->attack_range) out.append(to_id(e));
    }
    return out;
}

godot::PackedVector2Array zone_gd::TacticsBattle::get_path_to(int x, int y) const {
    PackedVector2Array out;
    entt::entity e = battle_.current();
    if (e == entt::null || !reg_.valid(e)) return out;
    const tac::MoveField f = tac::compute_move_field(reg_, map_, e);
    for (const auto& p : tac::path_to(f, x, y)) out.append(Vector2(p.x, p.y));
    return out;
}

int  zone_gd::TacticsBattle::get_winner() const { return battle_.winner(reg_); }
bool zone_gd::TacticsBattle::is_battle_over() const { return get_winner() != tac::Battle::kNoWinner; }
