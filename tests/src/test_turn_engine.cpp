#include <doctest/doctest.h>
#include <vector>
#include <entt/entt.hpp>

#include <core/turn/turn_engine.h>
#include <core/turn/apply_action.h>
#include <core/turn/move_dir.h>
#include <core/components/actor_component.h>
#include <core/components/act_point_component.h>
#include <core/components/controller_component.h>
#include <core/components/spatial_component.h>
#include <core/components/health_component.h>
#include <core/components/combat_stats_component.h>
#include <core/maps/map_data.h>
#include <core/maps/tile.h>

using namespace zone;

// 全可走的小地圖
static MapData open_map(int w, int h) {
    MapData m(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            m.at(x, y).flags = TILE_WALKABLE;
    return m;
}

static entt::entity make_actor(entt::registry& reg, int x, int y, ControllerKind k) {
    auto e = reg.create();
    reg.emplace<ActorComponent>(e);
    reg.emplace<SpatialComponent>(e, x, y);
    reg.emplace<ControllerComponent>(e, ControllerComponent{ k, entt::null });
    return e;
}

// 把一輪跑到底（會在玩家處停下）；回傳停下的原因。
static StepResult run(TurnEngine& eng, EngineCtx& ctx) {
    for (;;) {
        StepResult r = eng.step(ctx);
        if (r != StepResult::Acted) return r;
    }
}

TEST_CASE("TurnEngine — 加入序即回合序，linked list 維護") {
    entt::registry reg;
    TurnEngine eng;
    auto a = make_actor(reg, 0, 0, ControllerKind::Self);
    auto b = make_actor(reg, 1, 0, ControllerKind::Self);
    auto c = make_actor(reg, 2, 0, ControllerKind::Self);
    eng.add_actor(a); eng.add_actor(b); eng.add_actor(c);
    eng.add_actor(a);  // 重複略過

    CHECK(eng.size() == 3);
    std::vector<entt::entity> got(eng.order().begin(), eng.order().end());
    CHECK(got == std::vector<entt::entity>{ a, b, c });

    eng.remove_actor(b);
    CHECK(eng.size() == 2);
    CHECK_FALSE(eng.contains(b));
}

TEST_CASE("TurnEngine — 每回合每 actor 行動點數重設為 1000，全 Self 一輪跑完") {
    entt::registry reg;
    TurnEngine eng;
    auto a = make_actor(reg, 0, 0, ControllerKind::Self);
    auto b = make_actor(reg, 5, 5, ControllerKind::Self);
    eng.add_actor(a); eng.add_actor(b);

    std::vector<entt::entity> acted;
    EngineCtx ctx{ reg,
        [](entt::entity) { return Action::wait(); },
        [&](entt::entity e, const Action&) { acted.push_back(e); } };

    eng.begin_round();
    StepResult r = run(eng, ctx);
    CHECK(r == StepResult::RoundDone);
    CHECK(acted == std::vector<entt::entity>{ a, b });
    // act_point 行動時設 1000、結束歸 0
    CHECK(reg.get<ActPointComponent>(a).value == 0);
    CHECK_FALSE(eng.round_in_progress());
}

TEST_CASE("TurnEngine — 玩家 actor 阻塞，submit 後續跑") {
    entt::registry reg;
    TurnEngine eng;
    auto hero = make_actor(reg, 0, 0, ControllerKind::Player);
    auto npc  = make_actor(reg, 9, 9, ControllerKind::Self);
    eng.add_actor(hero); eng.add_actor(npc);

    auto map = open_map(12, 12);
    std::vector<ZoneEvent> ev;
    int npc_acts = 0;
    EngineCtx ctx{ reg,
        [&](entt::entity) { ++npc_acts; return Action::wait(); },
        [&](entt::entity e, const Action& a) { apply_action(reg, &map, e, a, &ev); } };

    eng.begin_round();
    // hero 是 head：立刻卡住等指令
    StepResult r = run(eng, ctx);
    CHECK(r == StepResult::WaitingPlayer);
    CHECK(eng.waiting_actor() == hero);
    CHECK(npc_acts == 0);              // 還沒輪到 NPC

    // 投遞向右移動
    eng.submit(hero, Action::move(encode_dir(1, 0)));
    r = run(eng, ctx);
    CHECK(r == StepResult::RoundDone);
    CHECK(reg.get<SpatialComponent>(hero).x == 1);  // 真的動了
    CHECK(npc_acts == 1);             // NPC 在 hero 之後行動
}

TEST_CASE("TurnEngine — 撞 actor 即攻擊，致死發 ActorDied 事件") {
    entt::registry reg;
    auto map = open_map(8, 8);

    auto attacker = reg.create();
    reg.emplace<ActorComponent>(attacker);
    reg.emplace<SpatialComponent>(attacker, 1, 1);
    reg.emplace<CombatStatsComponent>(attacker, CombatStatsComponent{ 100, 50 });

    auto victim = reg.create();
    reg.emplace<ActorComponent>(victim);
    reg.emplace<SpatialComponent>(victim, 2, 1);
    reg.emplace<HealthComponent>(victim, 5, 5);

    std::vector<ZoneEvent> ev;
    apply_action(reg, &map, attacker, Action::move(encode_dir(1, 0)), &ev);

    REQUIRE(ev.size() == 1);
    CHECK(ev[0].kind == EventKind::ActorDied);
    CHECK(ev[0].b == victim);
    // 攻擊者沒有移動到目標格（被擋下）
    CHECK(reg.get<SpatialComponent>(attacker).x == 1);
}
