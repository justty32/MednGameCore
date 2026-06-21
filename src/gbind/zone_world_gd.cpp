#include "zone_world_gd.h"

#include "core/components/actor_component.h"
#include "core/components/spatial_component.h"
#include "core/components/npc_ai_component.h"
#include "core/components/health_component.h"
#include "core/components/item_component.h"
#include "core/components/combat_stats_component.h"
#include "core/components/hero_component.h"
#include "core/components/world_state_component.h"
#include "core/components/controller_component.h"
#include "core/components/act_point_component.h"
#include "core/turn/action.h"
#include "core/turn/move_dir.h"
#include "core/turn/apply_action.h"
#include "core/maps/map_data.h"
#include "core/maps/map_gen.h"
#include "core/systems/fov_system.h"
#include "core/serialize/save_load.h"

#include <algorithm>
#include <random>
#include <filesystem>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// 遊戲常數（取代 CVar）
static constexpr int MAP_W         = 60;
static constexpr int MAP_H         = 40;
static constexpr int NPC_CAP_BASE  = 4;
static constexpr int NPC_CAP_MAX   = 8;
static constexpr int FOV_RADIUS    = 8;
static constexpr int ITEM_PCT      = 60;
static constexpr int HERO_HP       = 20;
static constexpr int HERO_ATK      = 3;
static constexpr int NPC_BASE_HP   = 5;
static constexpr int NPC_HP_SCALE  = 2;
static constexpr int NPC_ATK_BASE  = 2;
static constexpr int NPC_ATK_SCALE = 1;

// ---- static binding --------------------------------------------------------

void zone_gd::ZoneWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_map_width"),  &ZoneWorld::get_map_width);
    ClassDB::bind_method(D_METHOD("get_map_height"), &ZoneWorld::get_map_height);
    ClassDB::bind_method(D_METHOD("is_walkable", "x", "y"), &ZoneWorld::is_walkable);
    ClassDB::bind_method(D_METHOD("generate_map_image", "cell_px"), &ZoneWorld::generate_map_image);

    ClassDB::bind_method(D_METHOD("next_round"), &ZoneWorld::next_round);
    ClassDB::bind_method(D_METHOD("player_move", "dx", "dy"), &ZoneWorld::player_move);
    ClassDB::bind_method(D_METHOD("player_wait"), &ZoneWorld::player_wait);
    ClassDB::bind_method(D_METHOD("is_waiting_player"), &ZoneWorld::is_waiting_player);
    ClassDB::bind_method(D_METHOD("round_in_progress"), &ZoneWorld::round_in_progress);

    ClassDB::bind_method(D_METHOD("get_debug_text"),   &ZoneWorld::get_debug_text);
    ClassDB::bind_method(D_METHOD("set_trace_enabled", "on"), &ZoneWorld::set_trace_enabled);
    ClassDB::bind_method(D_METHOD("get_trace_enabled"), &ZoneWorld::get_trace_enabled);
    ClassDB::bind_method(D_METHOD("get_debug_log"),    &ZoneWorld::get_debug_log);
    ClassDB::bind_method(D_METHOD("clear_debug_log"),  &ZoneWorld::clear_debug_log);

    ClassDB::bind_method(D_METHOD("get_hero_x"),      &ZoneWorld::get_hero_x);
    ClassDB::bind_method(D_METHOD("get_hero_y"),      &ZoneWorld::get_hero_y);
    ClassDB::bind_method(D_METHOD("get_turn_count"),  &ZoneWorld::get_turn_count);
    ClassDB::bind_method(D_METHOD("get_hero_hp"),     &ZoneWorld::get_hero_hp);
    ClassDB::bind_method(D_METHOD("get_hero_max_hp"), &ZoneWorld::get_hero_max_hp);
    ClassDB::bind_method(D_METHOD("get_npc_count"),   &ZoneWorld::get_npc_count);
    ClassDB::bind_method(D_METHOD("get_current_floor"), &ZoneWorld::get_current_floor);
    ClassDB::bind_method(D_METHOD("restart"),           &ZoneWorld::restart);
    ClassDB::bind_method(D_METHOD("save_game", "path"),     &ZoneWorld::save_game);
    ClassDB::bind_method(D_METHOD("load_game", "path"),     &ZoneWorld::load_game);
    ClassDB::bind_method(D_METHOD("has_save_game", "path"), &ZoneWorld::has_save_game);

    ADD_SIGNAL(MethodInfo("world_changed"));
    ADD_SIGNAL(MethodInfo("round_finished"));
    ADD_SIGNAL(MethodInfo("floor_changed", PropertyInfo(Variant::INT, "floor_num")));
    ADD_SIGNAL(MethodInfo("hero_bumped_wall"));
    ADD_SIGNAL(MethodInfo("hero_bumped_npc", PropertyInfo(Variant::STRING, "npc_id")));
    ADD_SIGNAL(MethodInfo("npc_died", PropertyInfo(Variant::STRING, "npc_id")));
    ADD_SIGNAL(MethodInfo("item_picked_up",
        PropertyInfo(Variant::STRING, "item_name"),
        PropertyInfo(Variant::INT,    "heal_amount")));
    ADD_SIGNAL(MethodInfo("game_over"));
}

// ---- ctor / lifecycle -------------------------------------------------------

zone_gd::ZoneWorld::ZoneWorld() = default;

void zone_gd::ZoneWorld::_ready() {
    setup_world();
    recompute_fov();
}

void zone_gd::ZoneWorld::setup_world() {
    setup_map();
}

// ---- world / map setup ------------------------------------------------------

void zone_gd::ZoneWorld::add_actor_to_engine(entt::entity e) {
    engine_.add_actor(e);
}

void zone_gd::ZoneWorld::setup_map() {
    auto& reg = em_.registry();

    // 清空回合 list（NPC 將重建；hero 之後重新加入）
    engine_.clear();

    // 銷毀舊 NPC / 物品 / 地圖
    auto destroy_all = [&](auto view) {
        std::vector<entt::entity> v(view.begin(), view.end());
        for (auto e : v) reg.destroy(e);
    };
    destroy_all(reg.view<zone::NpcAiComponent>());
    destroy_all(reg.view<zone::ItemComponent>());
    if (map_entity_ != entt::null) { reg.destroy(map_entity_); map_entity_ = entt::null; }

    // 建立新地圖
    map_entity_ = em_.create();
    auto& map = em_.emplace<zone::MapData>(map_entity_, MAP_W, MAP_H);
    em_.emplace<zone::WorldStateComponent>(map_entity_,
        zone::WorldStateComponent{ turn_count_, current_floor_ });

    std::mt19937 rng(std::random_device{}());
    auto rooms = zone::generate_bsp_dungeon(map, rng);

    // 英雄（操控者＝玩家）。第一次建立，之後只更新位置（保留 HP）。
    int hx = rooms.empty() ? MAP_W / 2 : rooms[0].cx();
    int hy = rooms.empty() ? MAP_H / 2 : rooms[0].cy();
    if (hero_entity_ == entt::null) {
        hero_entity_ = em_.create();
        reg.emplace<zone::HeroComponent>(hero_entity_);
        reg.emplace<zone::ActorComponent>(hero_entity_);
        reg.emplace<zone::SpatialComponent>(hero_entity_, hx, hy);
        reg.emplace<zone::HealthComponent>(hero_entity_, HERO_HP, HERO_HP);
        reg.emplace<zone::CombatStatsComponent>(hero_entity_,
            zone::CombatStatsComponent{ HERO_ATK, 100 });
        reg.emplace<zone::ControllerComponent>(hero_entity_,
            zone::ControllerComponent{ zone::ControllerKind::Player, entt::null });
        reg.emplace<zone::ActPointComponent>(hero_entity_);
    } else if (auto* sp = reg.try_get<zone::SpatialComponent>(hero_entity_)) {
        sp->x = hx; sp->y = hy;
    }
    // 英雄是 list 的開頭（每回合最先行動）
    add_actor_to_engine(hero_entity_);

    // 樓梯
    if (rooms.size() >= 2) {
        auto& tile = map.at(rooms.back().cx(), rooms.back().cy());
        tile.flags |= zone::TILE_STAIR_DOWN;
    }

    // NPC（操控者＝自己；最精簡 AI 追擊）
    int npc_cap = std::min(NPC_CAP_BASE + current_floor_, NPC_CAP_MAX);
    int npc_count = 0;
    for (int r = 1; r < (int)rooms.size() && npc_count < npc_cap; ++r, ++npc_count) {
        int hp  = NPC_BASE_HP + (current_floor_ - 1) * NPC_HP_SCALE;
        int atk = NPC_ATK_BASE + (current_floor_ - 1) * NPC_ATK_SCALE;
        auto e = em_.create();
        reg.emplace<zone::NpcAiComponent>(e);
        reg.emplace<zone::ActorComponent>(e);
        reg.emplace<zone::SpatialComponent>(e, rooms[r].cx(), rooms[r].cy());
        reg.emplace<zone::HealthComponent>(e, hp, hp);
        reg.emplace<zone::CombatStatsComponent>(e, zone::CombatStatsComponent{ atk, 50 });
        reg.emplace<zone::ControllerComponent>(e,
            zone::ControllerComponent{ zone::ControllerKind::Self, entt::null });
        reg.emplace<zone::ActPointComponent>(e);
        add_actor_to_engine(e);
    }

    // 物品
    {
        std::uniform_int_distribution<int> pct(0, 99);
        for (int r = 1; r < (int)rooms.size() - 1; ++r) {
            if (pct(rng) >= ITEM_PCT) continue;
            int val = 5 + (current_floor_ - 1) * 2;
            auto e = em_.create();
            reg.emplace<zone::ItemComponent>(e,
                zone::ItemComponent{ zone::ItemType::health_potion, val });
            reg.emplace<zone::SpatialComponent>(e, rooms[r].x + 1, rooms[r].y + 1);
        }
    }
}

// ---- floor / restart --------------------------------------------------------

void zone_gd::ZoneWorld::next_floor() {
    ++current_floor_;
    setup_map();
    recompute_fov();
    emit_signal("floor_changed", current_floor_);
    emit_signal("world_changed");
}

void zone_gd::ZoneWorld::restart() {
    if (hero_entity_ != entt::null && em_.registry().valid(hero_entity_))
        em_.registry().destroy(hero_entity_);
    hero_entity_   = entt::null;
    game_over_     = false;
    turn_count_    = 0;
    current_floor_ = 1;
    engine_.clear();
    trace_log_.clear();
    setup_map();
    recompute_fov();
    emit_signal("world_changed");
}

// ---- FOV -------------------------------------------------------------------

void zone_gd::ZoneWorld::recompute_fov() {
    if (hero_entity_ == entt::null || map_entity_ == entt::null) return;
    const auto* sp = em_.registry().try_get<zone::SpatialComponent>(hero_entity_);
    if (!sp) return;
    auto& map = em_.get<zone::MapData>(map_entity_);
    zone::compute_fov(map, sp->x, sp->y, FOV_RADIUS);
}

// ---- 回合推進 --------------------------------------------------------------

zone::Action zone_gd::ZoneWorld::npc_decide(entt::entity e) {
    return zone::decide_chase(em_.registry(), e, hero_entity_);
}

zone::EngineCtx zone_gd::ZoneWorld::make_ctx() {
    zone::MapData* map = (map_entity_ != entt::null)
        ? &em_.get<zone::MapData>(map_entity_) : nullptr;
    zone::EngineCtx ctx{
        em_.registry(),
        [this](entt::entity e) { return npc_decide(e); },
        [this, map](entt::entity e, const zone::Action& a) {
            zone::apply_action(em_.registry(), map, e, a, &events_);
        }
    };
    if (trace_enabled_)
        ctx.trace = [this](const std::string& s) { on_trace(s); };
    return ctx;
}

void zone_gd::ZoneWorld::next_round() {
    if (game_over_) return;
    engine_.begin_round();
    pump();
}

void zone_gd::ZoneWorld::player_move(int dx, int dy) {
    if (game_over_) return;
    if (!engine_.round_in_progress()) next_round();   // 沒在回合中就先開一輪（推進到英雄）
    if (engine_.waiting_actor() != hero_entity_) return;  // 還沒輪到玩家
    engine_.submit(hero_entity_, zone::Action::move(zone::encode_dir(dx, dy)));
    pump();
}

void zone_gd::ZoneWorld::player_wait() {
    if (game_over_) return;
    if (!engine_.round_in_progress()) next_round();
    if (engine_.waiting_actor() != hero_entity_) return;
    engine_.submit(hero_entity_, zone::Action::wait());
    pump();
}

bool zone_gd::ZoneWorld::is_waiting_player() const {
    return engine_.waiting_actor() == hero_entity_ && hero_entity_ != entt::null;
}

bool zone_gd::ZoneWorld::round_in_progress() const {
    return engine_.round_in_progress();
}

void zone_gd::ZoneWorld::pump() {
    auto ctx = make_ctx();
    for (;;) {
        if (game_over_) { emit_signal("world_changed"); return; }

        events_.clear();
        zone::StepResult r = engine_.step(ctx);
        bool reached_stair = drain_events();   // 處理本步事件 → signal

        if (game_over_) { emit_signal("world_changed"); return; }
        if (reached_stair) {                   // 樓層變更：結束本回合
            next_floor();
            emit_signal("round_finished");
            return;
        }
        if (r == zone::StepResult::WaitingPlayer) {
            emit_signal("world_changed");       // 等玩家：刷新畫面
            return;
        }
        if (r == zone::StepResult::RoundDone) {
            ++turn_count_;
            if (map_entity_ != entt::null)
                em_.registry().get<zone::WorldStateComponent>(map_entity_).turn_count = turn_count_;
            emit_signal("world_changed");
            emit_signal("round_finished");
            return;
        }
        // Acted → 繼續推進
    }
}

bool zone_gd::ZoneWorld::drain_events() {
    auto& reg = em_.registry();
    bool reached_stair = false;
    for (auto& ev : events_) {
        switch (ev.kind) {
            case zone::EventKind::BumpedWall:
                if (ev.a == hero_entity_) emit_signal("hero_bumped_wall");
                break;
            case zone::EventKind::BumpedActor:
                if (ev.a == hero_entity_) emit_signal("hero_bumped_npc", String("npc"));
                break;
            case zone::EventKind::ActorDied:
                if (ev.b != hero_entity_) emit_signal("npc_died", String("npc"));
                engine_.remove_actor(ev.b);
                if (reg.valid(ev.b)) reg.destroy(ev.b);
                if (ev.b == hero_entity_ && !game_over_) {
                    game_over_ = true;
                    emit_signal("game_over");
                }
                break;
            case zone::EventKind::ItemPickedUp:
                if (ev.a == hero_entity_)
                    emit_signal("item_picked_up", String("health_potion"), ev.amount);
                break;
            case zone::EventKind::ReachedStairDown:
                if (ev.a == hero_entity_) reached_stair = true;
                break;
        }
    }
    events_.clear();
    recompute_fov();

    // 防禦性英雄死亡判定
    if (hero_entity_ != entt::null && reg.valid(hero_entity_)) {
        if (const auto* hp = reg.try_get<zone::HealthComponent>(hero_entity_))
            if (hp->hp <= 0 && !game_over_) { game_over_ = true; emit_signal("game_over"); }
    }
    return reached_stair;
}

// ---- 地圖查詢 ---------------------------------------------------------------

int zone_gd::ZoneWorld::get_map_width() const {
    if (map_entity_ == entt::null) return 0;
    return em_.get<zone::MapData>(map_entity_).width;
}
int zone_gd::ZoneWorld::get_map_height() const {
    if (map_entity_ == entt::null) return 0;
    return em_.get<zone::MapData>(map_entity_).height;
}
bool zone_gd::ZoneWorld::is_walkable(int x, int y) const {
    if (map_entity_ == entt::null) return false;
    const auto& map = em_.get<zone::MapData>(map_entity_);
    return map.in_bounds(x, y) && map.at(x, y).is_walkable();
}

// ---- 狀態查詢 ---------------------------------------------------------------

int zone_gd::ZoneWorld::get_hero_x() const {
    if (!em_.registry().valid(hero_entity_)) return 0;
    if (const auto* sp = em_.registry().try_get<zone::SpatialComponent>(hero_entity_)) return sp->x;
    return 0;
}
int zone_gd::ZoneWorld::get_hero_y() const {
    if (!em_.registry().valid(hero_entity_)) return 0;
    if (const auto* sp = em_.registry().try_get<zone::SpatialComponent>(hero_entity_)) return sp->y;
    return 0;
}
int zone_gd::ZoneWorld::get_turn_count()  const { return turn_count_; }
int zone_gd::ZoneWorld::get_hero_hp()     const {
    if (!em_.registry().valid(hero_entity_)) return 0;
    if (const auto* hp = em_.registry().try_get<zone::HealthComponent>(hero_entity_)) return hp->hp;
    return 0;
}
int zone_gd::ZoneWorld::get_hero_max_hp() const {
    if (!em_.registry().valid(hero_entity_)) return 0;
    if (const auto* hp = em_.registry().try_get<zone::HealthComponent>(hero_entity_)) return hp->max_hp;
    return 0;
}
int zone_gd::ZoneWorld::get_npc_count() const {
    return (int)em_.registry().view<zone::NpcAiComponent>().size();
}
int zone_gd::ZoneWorld::get_current_floor() const { return current_floor_; }

// ---- generate_map_image ----------------------------------------------------

godot::Ref<godot::Image> zone_gd::ZoneWorld::generate_map_image(int cell_px) const {
    if (map_entity_ == entt::null) return {};
    const auto& map = em_.get<zone::MapData>(map_entity_);
    auto& reg = em_.registry();
    Ref<Image> img = Image::create(map.width * cell_px, map.height * cell_px,
                                   false, Image::FORMAT_RGB8);

    const Color floor_c(0.40f, 0.35f, 0.25f), wall_c(0.12f, 0.10f, 0.08f);
    const Color hero_c (1.00f, 0.90f, 0.20f), npc_c (0.90f, 0.20f, 0.20f);
    const Color item_c (0.20f, 0.85f, 0.40f), black (0.00f, 0.00f, 0.00f);

    for (int x = 0; x < map.width; ++x)
        for (int y = 0; y < map.height; ++y) {
            Color c;
            if (map.is_visible(x, y))
                c = map.at(x, y).is_stair_down()
                    ? Color(0.80f, 0.65f, 0.10f)
                    : (map.at(x, y).is_walkable() ? floor_c : wall_c);
            else if (map.is_explored(x, y)) {
                Color b = map.at(x, y).is_walkable() ? floor_c : wall_c;
                c = Color(b.r * 0.4f, b.g * 0.4f, b.b * 0.4f);
            } else {
                c = black;
            }
            img->fill_rect(Rect2i(x*cell_px, y*cell_px, cell_px, cell_px), c);
        }

    for (auto e : reg.view<zone::ItemComponent, zone::SpatialComponent>()) {
        const auto& sp = reg.get<zone::SpatialComponent>(e);
        if (map.is_visible(sp.x, sp.y))
            img->fill_rect(Rect2i(sp.x*cell_px, sp.y*cell_px, cell_px, cell_px), item_c);
    }
    for (auto e : reg.view<zone::NpcAiComponent, zone::SpatialComponent>()) {
        const auto& sp = reg.get<zone::SpatialComponent>(e);
        if (map.is_visible(sp.x, sp.y))
            img->fill_rect(Rect2i(sp.x*cell_px, sp.y*cell_px, cell_px, cell_px), npc_c);
    }
    if (reg.valid(hero_entity_)) {
        if (const auto* sp = reg.try_get<zone::SpatialComponent>(hero_entity_))
            if (map.in_bounds(sp->x, sp->y))
                img->fill_rect(Rect2i(sp->x*cell_px, sp->y*cell_px, cell_px, cell_px), hero_c);
    }
    return img;
}

// ---- 存讀檔 ----------------------------------------------------------------

bool zone_gd::ZoneWorld::save_game(const godot::String& path) {
    try {
        if (map_entity_ != entt::null) {
            if (auto* ws = em_.registry().try_get<zone::WorldStateComponent>(map_entity_)) {
                ws->turn_count    = turn_count_;
                ws->current_floor = current_floor_;
            }
        }
        std::filesystem::path fpath{ std::string(path.utf8().get_data()) };
        zone::serialize::save(em_.registry(), fpath);
        return true;
    } catch (...) { return false; }
}

bool zone_gd::ZoneWorld::load_game(const godot::String& path) {
    try {
        std::filesystem::path fpath{ std::string(path.utf8().get_data()) };
        if (!std::filesystem::exists(fpath)) return false;

        em_.registry().clear();
        engine_.clear();
        hero_entity_ = entt::null;
        map_entity_  = entt::null;

        zone::serialize::load(em_.registry(), fpath);

        for (auto e : em_.registry().view<zone::HeroComponent>()) { hero_entity_ = e; break; }
        for (auto e : em_.registry().view<zone::MapData>())       { map_entity_ = e; break; }
        if (map_entity_ != entt::null) {
            if (const auto* ws = em_.registry().try_get<zone::WorldStateComponent>(map_entity_)) {
                turn_count_    = ws->turn_count;
                current_floor_ = ws->current_floor;
            }
        }
        // 回合 list 不持久化：由現存 actor 重建（英雄優先，再依 entity 順序補 NPC）
        if (hero_entity_ != entt::null) engine_.add_actor(hero_entity_);
        for (auto e : em_.registry().view<zone::ActorComponent>()) {
            if (e == hero_entity_) continue;
            engine_.add_actor(e);
        }
        game_over_ = false;
        recompute_fov();
        return true;
    } catch (...) { return false; }
}

bool zone_gd::ZoneWorld::has_save_game(const godot::String& path) const {
    return std::filesystem::exists(std::string(path.utf8().get_data()));
}

// ---- trace / debug ---------------------------------------------------------

void zone_gd::ZoneWorld::set_trace_enabled(bool on) { trace_enabled_ = on; }
bool zone_gd::ZoneWorld::get_trace_enabled() const { return trace_enabled_; }
void zone_gd::ZoneWorld::clear_debug_log() { trace_log_.clear(); }

void zone_gd::ZoneWorld::on_trace(const std::string& line) {
    trace_log_.push_back(line);
    if (trace_log_.size() > 300)
        trace_log_.erase(trace_log_.begin(),
                         trace_log_.begin() + (trace_log_.size() - 300));
    godot::UtilityFunctions::print(godot::String::utf8(line.c_str()));
}

godot::String zone_gd::ZoneWorld::get_debug_log() const {
    using godot::String;
    String s;
    const std::size_t n = trace_log_.size();
    const std::size_t start = n > 24 ? n - 24 : 0;
    for (std::size_t i = start; i < n; ++i)
        s += String::utf8(trace_log_[i].c_str()) + "\n";
    return s;
}

godot::String zone_gd::ZoneWorld::get_debug_text() const {
    using godot::String;
    const auto& reg = em_.registry();

    String waiting = String::utf8("無");
    entt::entity wa = engine_.waiting_actor();
    if (wa != entt::null)
        waiting = String("#") + String::num_int64((int)(entt::to_integral(wa) & 0xFFFFFu))
                + (wa == hero_entity_ ? String::utf8("(英雄)") : String());

    String out = String::utf8("回合 ") + String::num_int64(turn_count_)
        + String::utf8("  樓層 ") + String::num_int64(current_floor_)
        + String::utf8("  actor數 ") + String::num_int64((int)engine_.size())
        + String::utf8("  等待 ") + waiting
        + (game_over_ ? String::utf8("  [GAME OVER]") : String()) + "\n";

    auto kind_label = [](zone::ControllerKind k) {
        switch (k) {
            case zone::ControllerKind::Player:  return String::utf8("玩家");
            case zone::ControllerKind::Self:    return String::utf8("自己");
            case zone::ControllerKind::Faction: return String::utf8("勢力");
        }
        return String::utf8("?");
    };

    // 依 list 順序 dump（回合序）
    for (auto e : engine_.order()) {
        if (!reg.valid(e)) continue;
        const bool is_hero = (e == hero_entity_);
        String line = (is_hero ? String::utf8("◆英雄 #") : String::utf8("·NPC  #"))
            + String::num_int64((int)(entt::to_integral(e) & 0xFFFFFu));
        if (const auto* sp = reg.try_get<zone::SpatialComponent>(e))
            line += String::utf8(" (") + String::num_int64(sp->x) + ","
                  + String::num_int64(sp->y) + ")";
        if (const auto* hp = reg.try_get<zone::HealthComponent>(e))
            line += String::utf8("  HP ") + String::num_int64(hp->hp) + "/" + String::num_int64(hp->max_hp);
        if (const auto* ap = reg.try_get<zone::ActPointComponent>(e))
            line += String::utf8("  AP ") + String::num_int64(ap->value);
        if (const auto* c = reg.try_get<zone::ControllerComponent>(e))
            line += String::utf8("  操控:") + kind_label(c->kind);
        out += line + "\n";
    }
    return out;
}
