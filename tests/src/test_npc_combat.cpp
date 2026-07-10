#include <doctest/doctest.h>
#include <core/ecs/entity_manager.h>
#include <core/components/actor_component.h>
#include <core/components/spatial_component.h>
#include <core/components/health_component.h>
#include <core/components/npc_ai_component.h>
#include <core/components/combat_stats_component.h>
#include <core/components/hero_component.h>
#include <core/maps/map_data.h>
#include <core/maps/tile.h>
#include <core/turn/npc_ai.h>
#include <core/turn/apply_action.h>
#include <core/turn/action.h>
#include <core/turn/move_dir.h>
#include <core/serialize/save_load.h>
#include <random>
#include <sstream>

// ==============================================================
// NPC AI 決策管線：decide_npc（純決策）+ apply_action（結算）
// 取代舊版直接改世界的 npc_ai_system；decide_npc 只回傳 Action，
// 世界的改動一律交給 apply_action。
// ==============================================================

// 建一張全可走的小地圖實體
static entt::entity make_open_map(zone::EntityManager& em, int w, int h) {
    auto map_e = em.create();
    auto& map = em.emplace<zone::MapData>(map_e, w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            map.at(x, y).flags = zone::TILE_WALKABLE;
    return map_e;
}

TEST_CASE("NPC AI — 警覺且鄰接時攻擊英雄（核心戰鬥邏輯）") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 5, 5);
    auto& map = reg.get<zone::MapData>(map_e);

    // 英雄：ActorComponent + Spatial + Health
    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{2, 2, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    // NPC：鄰接 (2,3)，已警覺，move_chance=100（確定行動），attack=5
    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{2, 3, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{true, 6});
    em.emplace<zone::HealthComponent>(npc, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc, zone::CombatStatsComponent{5, 100});

    zone::Action a = zone::decide_npc(reg, &map, npc, hero, rng);
    CHECK(a.kind == zone::ActionKind::Move);  // 攻擊＝朝英雄所在格 Move

    zone::apply_action(reg, &map, npc, a, nullptr);

    CHECK(reg.get<zone::HealthComponent>(hero).hp == 15);  // 20 - 5
}

// 真機回歸：舊版用「排除法」在 storage 尾端往前掃找英雄，物品晚建立會誤選成英雄。
// 新版 decide_npc 不再自己找英雄——target 由呼叫端明確傳入，因此本測試改成
// 驗證：不論 NPC 是在英雄「之前」還是「之後」建立，只要呼叫端把 target 傳對，
// decide_npc 都能正確決策（攻擊正確的目標）。
TEST_CASE("NPC AI — target 由呼叫端明確傳入，不受實體建立順序影響（真機順序回歸）") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 5, 5);
    auto& map = reg.get<zone::MapData>(map_e);

    // 1) 兩個 NPC 在英雄「之前」建立
    auto npc_west = em.create();
    em.emplace<zone::ActorComponent>(npc_west);
    em.emplace<zone::SpatialComponent>(npc_west, zone::SpatialComponent{1, 2, entt::null});
    em.emplace<zone::NpcAiComponent>(npc_west, zone::NpcAiComponent{true, 6});
    em.emplace<zone::HealthComponent>(npc_west, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc_west, zone::CombatStatsComponent{5, 100});

    auto npc_north = em.create();
    em.emplace<zone::ActorComponent>(npc_north);
    em.emplace<zone::SpatialComponent>(npc_north, zone::SpatialComponent{2, 1, entt::null});
    em.emplace<zone::NpcAiComponent>(npc_north, zone::NpcAiComponent{true, 6});
    em.emplace<zone::HealthComponent>(npc_north, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc_north, zone::CombatStatsComponent{3, 100});

    // 2) 英雄
    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{2, 2, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    // 3) 兩個 NPC 在英雄「之後」建立
    auto npc_south = em.create();
    em.emplace<zone::ActorComponent>(npc_south);
    em.emplace<zone::SpatialComponent>(npc_south, zone::SpatialComponent{2, 3, entt::null});
    em.emplace<zone::NpcAiComponent>(npc_south, zone::NpcAiComponent{true, 6});
    em.emplace<zone::HealthComponent>(npc_south, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc_south, zone::CombatStatsComponent{4, 100});

    auto npc_east = em.create();
    em.emplace<zone::ActorComponent>(npc_east);
    em.emplace<zone::SpatialComponent>(npc_east, zone::SpatialComponent{3, 2, entt::null});
    em.emplace<zone::NpcAiComponent>(npc_east, zone::NpcAiComponent{true, 6});
    em.emplace<zone::HealthComponent>(npc_east, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc_east, zone::CombatStatsComponent{2, 100});

    // 每個 NPC 都明確把 hero 當 target 傳入，與建立順序無關
    for (auto npc : { npc_west, npc_north, npc_south, npc_east }) {
        zone::Action a = zone::decide_npc(reg, &map, npc, hero, rng);
        CHECK(a.kind == zone::ActionKind::Move);
        zone::apply_action(reg, &map, npc, a, nullptr);
    }

    CHECK(reg.get<zone::HealthComponent>(hero).hp == 6);  // 20 - (5+3+4+2)
}

// 回歸測試：重現前端「撞 NPC 能傷敵但自己不掉血」的根因。
// move_chance=0 的 NPC（從不移動）若已警覺且貼身，仍必須攻擊英雄——
// 攻擊不該被行動機率閘門吃掉（decide_npc 的攻擊分支在 move_chance 閘門之前）。
TEST_CASE("NPC AI — move_chance=0 的鄰接警覺 NPC 仍必定攻擊") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 5, 5);
    auto& map = reg.get<zone::MapData>(map_e);

    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{2, 2, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{2, 3, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{true, 6});
    em.emplace<zone::HealthComponent>(npc, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc, zone::CombatStatsComponent{3, 0});  // move_chance=0

    for (int i = 0; i < 3; ++i) {
        zone::Action a = zone::decide_npc(reg, &map, npc, hero, rng);
        CHECK(a.kind == zone::ActionKind::Move);
        zone::apply_action(reg, &map, npc, a, nullptr);
    }

    CHECK(reg.get<zone::HealthComponent>(hero).hp == 11);  // 20 - 3*3
}

// HeroComponent 是空 tag：驗證它能通過 cereal/EnTT snapshot round-trip，
// 讀檔後 view<HeroComponent> 仍唯一指回英雄（gbind 載入路徑依賴此）。
TEST_CASE("HeroComponent — 空 tag 序列化 round-trip 後仍可辨識英雄") {
    zone::EntityManager em;

    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{4, 7, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    // 另建一個非英雄實體，確保不是「碰巧只有一個」
    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{1, 1, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{});

    std::ostringstream oss;
    zone::serialize::save(em.registry(), oss);
    em = zone::EntityManager{};
    std::istringstream iss{oss.str()};
    zone::serialize::load(em.registry(), iss);

    auto& reg = em.registry();
    int hero_count = 0;
    entt::entity found = entt::null;
    for (auto e : reg.view<zone::HeroComponent>()) { ++hero_count; found = e; }
    bool found_valid = (found != entt::null);  // 先求值，避免 doctest 分解 operator!= 歧義
    CHECK(hero_count == 1);
    CHECK(found_valid);
    CHECK(reg.get<zone::SpatialComponent>(found).x == 4);
    CHECK(reg.get<zone::SpatialComponent>(found).y == 7);
}

// 從未看見英雄（距離遠超 sight_radius）的 NPC 不會憑空「瞬間鎖定」追擊：
// alerted 應該持續維持 false。（舊系統沒有視野概念，這是新版才有的保證）
TEST_CASE("NPC AI — 從未看見英雄時不會憑空警覺") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 40, 40);
    auto& map = reg.get<zone::MapData>(map_e);

    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{2, 2, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    // Chebyshev 距離 33，遠超預設 sight_radius=10
    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{35, 35, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{false, 0});
    em.emplace<zone::HealthComponent>(npc, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc, zone::CombatStatsComponent{2, 50});

    // 位置固定不變（不套用回傳的 Action），只反覆呼叫決策本身；
    // 只要距離恆遠超視野，alerted 就不該被扳成 true。
    for (int i = 0; i < 20; ++i) {
        zone::decide_npc(reg, &map, npc, hero, rng);
        CHECK(reg.get<zone::NpcAiComponent>(npc).alerted == false);
    }
}

// 警覺記憶：NPC 看見英雄後即使英雄離開視野，仍應記得追擊 alert_memory 回合，
// 之後才解除警覺。
TEST_CASE("NPC AI — 警覺記憶：失去視線後維持 alert_memory 回合才解除") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 30, 30);
    auto& map = reg.get<zone::MapData>(map_e);

    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{15, 17, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{15, 15, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{false, 0});
    em.emplace<zone::HealthComponent>(npc, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc, zone::CombatStatsComponent{2, 50});

    zone::NpcAiConfig cfg{ 5, 3 };  // sight_radius=5, alert_memory=3

    // 英雄在視野內（距離 2 <= 5）→ 建立警覺，alert_turns 重置為 3
    zone::decide_npc(reg, &map, npc, hero, rng, cfg);
    CHECK(reg.get<zone::NpcAiComponent>(npc).alerted == true);

    // 英雄移到視野外（距離 13 > 5）
    reg.get<zone::SpatialComponent>(hero).x = 2;
    reg.get<zone::SpatialComponent>(hero).y = 2;

    // 前 alert_memory - 1 = 2 次呼叫仍應維持警覺（記憶尚未耗盡）
    zone::decide_npc(reg, &map, npc, hero, rng, cfg);
    CHECK(reg.get<zone::NpcAiComponent>(npc).alerted == true);
    zone::decide_npc(reg, &map, npc, hero, rng, cfg);
    CHECK(reg.get<zone::NpcAiComponent>(npc).alerted == true);

    // 第 alert_memory 次呼叫：記憶耗盡，警覺解除
    zone::decide_npc(reg, &map, npc, hero, rng, cfg);
    CHECK(reg.get<zone::NpcAiComponent>(npc).alerted == false);
}

// 牆擋住直線追擊軸時不應卡死：優先軸受阻就換另一軸，
// 而不是對著牆原地重複撞擊（Wait 或無限卡住）。
TEST_CASE("NPC AI — 直線追擊被牆擋住時改走另一軸，不卡死") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 5, 5);
    auto& map = reg.get<zone::MapData>(map_e);

    // NPC 在 (1,3)，英雄在 (4,2)：hdx=3, hdy=-1 → x 軸（East）優先。
    // 把 (2,3)（NPC 東邊那一步）設成牆，逼決策改走 y 軸（North）。
    map.at(2, 3).flags = 0;  // 不可走

    auto hero = em.create();
    em.emplace<zone::ActorComponent>(hero);
    em.emplace<zone::SpatialComponent>(hero, zone::SpatialComponent{4, 2, entt::null});
    em.emplace<zone::HealthComponent>(hero, 20, 20);
    em.emplace<zone::HeroComponent>(hero);

    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{1, 3, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{true, 6});  // 已警覺，免驗視線
    em.emplace<zone::HealthComponent>(npc, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc, zone::CombatStatsComponent{2, 100});  // move_chance=100

    zone::Action a = zone::decide_npc(reg, &map, npc, hero, rng);
    REQUIRE(a.kind == zone::ActionKind::Move);

    int dx = 0, dy = 0;
    zone::decode_dir(a.param, dx, dy);
    CHECK(dx == 0);
    CHECK(dy == -1);  // 改走 North（y 軸），而不是被牆擋住的 East
}

// 徘徊中的 NPC 不會誤傷同類：四周可走格全被其他 actor 佔滿時，
// 應該回傳 Wait，而不是硬擠進被佔的格子（等於誤攻擊）。
TEST_CASE("NPC AI — 徘徊時四周皆被其他 actor 佔滿則等待，不誤傷同類") {
    zone::EntityManager em;
    auto& reg = em.registry();
    std::mt19937 rng{ 1234 };

    auto map_e = make_open_map(em, 5, 5);
    auto& map = reg.get<zone::MapData>(map_e);

    // 未警覺的 NPC，四個方向都被其他 actor 佔住
    auto npc = em.create();
    em.emplace<zone::ActorComponent>(npc);
    em.emplace<zone::SpatialComponent>(npc, zone::SpatialComponent{2, 2, entt::null});
    em.emplace<zone::NpcAiComponent>(npc, zone::NpcAiComponent{false, 0});
    em.emplace<zone::HealthComponent>(npc, 10, 10);
    em.emplace<zone::CombatStatsComponent>(npc, zone::CombatStatsComponent{2, 100});  // move_chance=100

    for (auto [x, y] : { std::pair{2, 1}, std::pair{2, 3}, std::pair{1, 2}, std::pair{3, 2} }) {
        auto blocker = em.create();
        em.emplace<zone::ActorComponent>(blocker);
        em.emplace<zone::SpatialComponent>(blocker, zone::SpatialComponent{x, y, entt::null});
    }

    // 無英雄可追（未警覺也沒有 target），純測徘徊避讓
    zone::Action a = zone::decide_npc(reg, &map, npc, entt::null, rng);
    CHECK(a.kind == zone::ActionKind::Wait);
}
