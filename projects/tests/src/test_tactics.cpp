#include <doctest/doctest.h>
#include <core/ecs/entity_manager.h>
#include <core/components/spatial_component.h>
#include <core/components/health_component.h>
#include <core/components/combat_stats_component.h>
#include <core/components/team_component.h>
#include <core/components/tactics_unit_component.h>
#include <core/components/turn_gauge_component.h>
#include <core/maps/map_data.h>
#include <core/maps/tile.h>
#include <core/tactics/action_bar.h>
#include <core/tactics/move_range.h>
#include <core/tactics/apply_tactics_action.h>
#include <core/tactics/tactics_ai.h>
#include <core/tactics/battle.h>

#include <algorithm>

using namespace zone;
using namespace zone::tactics;

// ==============================================================
// 戰棋核心：行動條、可達集、指令結算、AI、整場戰鬥
// ==============================================================

// 全可走的小地圖
static MapData make_open_map(int w, int h) {
    MapData map(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            map.at(x, y).flags = TILE_WALKABLE;
    return map;
}

// 造一個戰棋單位
static entt::entity make_unit(entt::registry& reg, int x, int y, uint8_t team,
                              int hp = 10, int attack = 3,
                              int move_range = 4, int attack_range = 1, int speed = 100) {
    auto e = reg.create();
    reg.emplace<SpatialComponent>(e, x, y);
    reg.emplace<HealthComponent>(e, hp, hp);
    reg.emplace<CombatStatsComponent>(e, CombatStatsComponent{ attack, 100 });
    reg.emplace<TeamComponent>(e, TeamComponent{ team });
    reg.emplace<TacticsUnitComponent>(e, TacticsUnitComponent{ move_range, attack_range, speed });
    reg.emplace<TurnGaugeComponent>(e);
    return e;
}

// ---- ActionBar --------------------------------------------------------------

TEST_CASE("ActionBar — 出手後保留溢出的行動值，不歸零") {
    entt::registry reg;
    // 速度 150：兩次 tick 後 ct=300... 直到 ct=1050 才出手，溢出 50 必須留下
    auto fast = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, 4, 1, 150);

    ActionBar bar;
    bar.add(fast);

    CHECK(bar.advance(reg) == fast);
    CHECK(reg.get<TurnGaugeComponent>(fast).ct == 1050);  // 7 tick × 150

    bar.consume(reg, fast);
    CHECK(reg.get<TurnGaugeComponent>(fast).ct == 50);    // 1050 - 1000，不是 0
}

TEST_CASE("ActionBar — 速度差異靠溢出累積，快角會連動兩次") {
    entt::registry reg;
    auto fast = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, 4, 1, 200);  // 兩倍速
    auto slow = make_unit(reg, 5, 0, TEAM_ENEMY,  10, 3, 4, 1, 100);

    ActionBar bar;
    bar.add(fast);
    bar.add(slow);

    // 跑 6 次出手，快角應該出手 4 次、慢角 2 次（2:1）
    int fast_turns = 0, slow_turns = 0;
    for (int i = 0; i < 6; ++i) {
        auto e = bar.advance(reg);
        REQUIRE((e != entt::null));
        if (e == fast) ++fast_turns; else ++slow_turns;
        bar.consume(reg, e);
    }
    CHECK(fast_turns == 4);
    CHECK(slow_turns == 2);
}

TEST_CASE("ActionBar — ct 平手時依加入序，結果確定") {
    entt::registry reg;
    auto first  = make_unit(reg, 0, 0, TEAM_PLAYER);
    auto second = make_unit(reg, 1, 0, TEAM_PLAYER);

    ActionBar bar;
    bar.add(first);
    bar.add(second);

    CHECK(bar.advance(reg) == first);
    bar.consume(reg, first);
    CHECK(bar.advance(reg) == second);
}

TEST_CASE("ActionBar — forecast 與實際出手序一致，且不改動真實 ct") {
    entt::registry reg;
    auto fast = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, 4, 1, 170);
    auto slow = make_unit(reg, 5, 0, TEAM_ENEMY,  10, 3, 4, 1, 100);

    ActionBar bar;
    bar.add(fast);
    bar.add(slow);

    const int ct_fast_before = reg.get<TurnGaugeComponent>(fast).ct;
    const int ct_slow_before = reg.get<TurnGaugeComponent>(slow).ct;

    auto predicted = bar.forecast(reg, 5);
    REQUIRE(predicted.size() == 5);

    // 預測不能有副作用
    CHECK(reg.get<TurnGaugeComponent>(fast).ct == ct_fast_before);
    CHECK(reg.get<TurnGaugeComponent>(slow).ct == ct_slow_before);

    // 真的跑一遍，逐一比對
    for (std::size_t i = 0; i < predicted.size(); ++i) {
        auto actual = bar.advance(reg);
        CHECK(actual == predicted[i]);
        bar.consume(reg, actual);
    }
}

TEST_CASE("ActionBar — 全員速度 0 不會無限迴圈") {
    entt::registry reg;
    make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, 4, 1, 0);
    ActionBar bar;
    for (auto e : reg.view<TacticsUnitComponent>()) bar.add(e);

    CHECK((bar.advance(reg) == entt::null));
    CHECK(bar.forecast(reg, 3).empty());
}

// ---- MoveField --------------------------------------------------------------

TEST_CASE("MoveField — 可達集是四方向的步數上限，斜角要繞") {
    entt::registry reg;
    MapData map = make_open_map(9, 9);
    auto u = make_unit(reg, 4, 4, TEAM_PLAYER, 10, 3, /*move_range*/2);

    auto f = compute_move_field(reg, map, u);
    CHECK(f.at(4, 4) == 0);
    CHECK(f.at(6, 4) == 2);   // 正東兩格
    CHECK(f.at(5, 5) == 2);   // 斜角＝走兩步
    CHECK(f.at(6, 5) == -1);  // 需要三步，超出 move_range
    CHECK_FALSE(f.reachable(7, 4));
}

TEST_CASE("MoveField — 牆擋住可達集，繞路的步數要正確") {
    entt::registry reg;
    MapData map = make_open_map(7, 3);
    // 在 x=1 砌一道牆（除了 y=2 留一個缺口）
    map.at(1, 0).flags = 0;
    map.at(1, 1).flags = 0;

    auto u = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, /*move_range*/6);
    auto f = compute_move_field(reg, map, u);

    CHECK(f.at(1, 0) == -1);           // 牆本身不可達
    CHECK(f.at(0, 2) == 2);            // 往下兩格到缺口
    // (2,0) 明明只隔一道牆，卻得整個繞過去：
    //   (0,0)→(0,1)→(0,2)→(1,2)→(2,2)→(2,1)→(2,0) ＝ 6 步
    CHECK(f.at(2, 0) == 6);
    CHECK(f.at(6, 0) == -1);           // move_range=6 用完了，再遠就到不了
}

TEST_CASE("MoveField — 其他單位佔著的格子不能穿也不能停") {
    entt::registry reg;
    MapData map = make_open_map(5, 1);   // 一條 1 格寬的走廊
    auto self  = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, /*move_range*/4);
    make_unit(reg, 2, 0, TEAM_ENEMY);    // 擋在中間

    auto f = compute_move_field(reg, map, self);
    CHECK(f.at(1, 0) == 1);
    CHECK(f.at(2, 0) == -1);   // 被佔：不能停
    CHECK(f.at(3, 0) == -1);   // 也不能穿過去
}

TEST_CASE("MoveField — path_to 沿 dist 回溯出連續路徑") {
    entt::registry reg;
    MapData map = make_open_map(5, 5);
    auto u = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, /*move_range*/4);

    auto f = compute_move_field(reg, map, u);
    auto path = path_to(f, 2, 2);

    REQUIRE(path.size() == 5);                       // 起點 + 4 步
    CHECK(path.front() == Vector2i{ 0, 0 });
    CHECK(path.back()  == Vector2i{ 2, 2 });
    for (std::size_t i = 1; i < path.size(); ++i) {  // 每步只走一格，四方向
        const int dx = std::abs(path[i].x - path[i-1].x);
        const int dy = std::abs(path[i].y - path[i-1].y);
        CHECK(dx + dy == 1);
    }
    CHECK(path_to(f, 4, 4).empty());                 // 不可達 → 空路徑
}

// ---- apply_tactics_action ---------------------------------------------------

TEST_CASE("結算 — 移動到可達格生效，超出可達集被拒且世界不變") {
    entt::registry reg;
    MapData map = make_open_map(9, 9);
    auto u = make_unit(reg, 4, 4, TEAM_PLAYER, 10, 3, /*move_range*/2);

    std::vector<TacticsEvent> ev;
    CHECK(apply_tactics_action(reg, map, u, TacticsAction::move_to(6, 4), &ev));
    CHECK(reg.get<SpatialComponent>(u).x == 6);
    REQUIRE(ev.size() == 1);
    CHECK(ev[0].kind == TacticsEventKind::Moved);
    CHECK(ev[0].from_x == 4);

    ev.clear();
    CHECK_FALSE(apply_tactics_action(reg, map, u, TacticsAction::move_to(0, 0), &ev));
    CHECK(reg.get<SpatialComponent>(u).x == 6);   // 沒動
    REQUIRE(ev.size() == 1);
    CHECK(ev[0].kind == TacticsEventKind::Rejected);
}

TEST_CASE("結算 — 攻擊受射程限制，且不能打自己人") {
    entt::registry reg;
    MapData map = make_open_map(9, 1);
    auto me   = make_unit(reg, 0, 0, TEAM_PLAYER, 10, /*attack*/4, 4, /*attack_range*/2);
    auto ally = make_unit(reg, 1, 0, TEAM_PLAYER);
    auto foe  = make_unit(reg, 2, 0, TEAM_ENEMY, 10);
    auto far  = make_unit(reg, 5, 0, TEAM_ENEMY, 10);

    std::vector<TacticsEvent> ev;

    // 射程 2、距離 2 → 命中
    CHECK(apply_tactics_action(reg, map, me, TacticsAction::attack(foe), &ev));
    CHECK(reg.get<HealthComponent>(foe).hp == 6);
    REQUIRE(ev.size() == 1);
    CHECK(ev[0].kind == TacticsEventKind::Attacked);
    CHECK(ev[0].amount == 4);

    // 距離 5 → 超出射程
    ev.clear();
    CHECK_FALSE(apply_tactics_action(reg, map, me, TacticsAction::attack(far), &ev));
    CHECK(reg.get<HealthComponent>(far).hp == 10);
    CHECK(ev[0].kind == TacticsEventKind::Rejected);

    // 同隊 → 拒絕，不誤傷
    ev.clear();
    CHECK_FALSE(apply_tactics_action(reg, map, me, TacticsAction::attack(ally), &ev));
    CHECK(reg.get<HealthComponent>(ally).hp == 10);
    CHECK(ev[0].kind == TacticsEventKind::Rejected);
}

TEST_CASE("結算 — 致死發 UnitDied，且不在此 destroy") {
    entt::registry reg;
    MapData map = make_open_map(3, 1);
    auto me  = make_unit(reg, 0, 0, TEAM_PLAYER, 10, /*attack*/99);
    auto foe = make_unit(reg, 1, 0, TEAM_ENEMY, 5);

    std::vector<TacticsEvent> ev;
    CHECK(apply_tactics_action(reg, map, me, TacticsAction::attack(foe), &ev));
    REQUIRE(ev.size() == 1);
    CHECK(ev[0].kind == TacticsEventKind::UnitDied);
    CHECK(ev[0].b == foe);
    CHECK(reg.valid(foe));   // 由 Battle 統一移除，不在結算裡 destroy
}

// ---- AI ---------------------------------------------------------------------

TEST_CASE("AI — 射程內必攻擊，且挑 HP 最低的目標") {
    entt::registry reg;
    MapData map = make_open_map(5, 5);
    auto me   = make_unit(reg, 2, 2, TEAM_ENEMY, 10, 3, 4, /*attack_range*/1);
    make_unit(reg, 1, 2, TEAM_PLAYER, /*hp*/9);
    auto weak = make_unit(reg, 3, 2, TEAM_PLAYER, /*hp*/2);

    auto a = decide_tactics(reg, map, me, /*moved*/false, /*acted*/false);
    CHECK(a.kind == TacticsActionKind::Attack);
    CHECK(a.target == weak);
}

TEST_CASE("AI — 射程外則靠近；已行動過就不再攻擊") {
    entt::registry reg;
    MapData map = make_open_map(11, 1);
    auto me = make_unit(reg, 0, 0, TEAM_ENEMY, 10, 3, /*move_range*/3, /*attack_range*/1);
    make_unit(reg, 10, 0, TEAM_PLAYER);

    auto a = decide_tactics(reg, map, me, false, false);
    CHECK(a.kind == TacticsActionKind::MoveTo);
    CHECK(a.x == 3);   // 走滿 3 步，離敵人最近

    // 已移動且已行動 → 只能待機
    auto b = decide_tactics(reg, map, me, /*moved*/true, /*acted*/true);
    CHECK(b.kind == TacticsActionKind::Wait);
}

TEST_CASE("AI — 遠程單位走進射程就停，不會貼到敵人臉上") {
    entt::registry reg;
    MapData map = make_open_map(11, 1);
    // 射程 3、移動 8：從 x=0 出發，敵人在 x=10（移動力足以走到 x=8 貼臉）。
    // 只求「最近」的話會走到 x=8（距離 2）；正確行為是停在 x=7，剛好卡在射程 3。
    auto archer = make_unit(reg, 0, 0, TEAM_ENEMY, 10, 3, /*move*/8, /*attack_range*/3);
    make_unit(reg, 10, 0, TEAM_PLAYER);

    auto a = decide_tactics(reg, map, archer, false, false);
    REQUIRE(a.kind == TacticsActionKind::MoveTo);
    CHECK(a.x == 7);   // 進得了射程就別再往前
}

TEST_CASE("AI — 近戰單位仍會貼上去（射程 1 時第二鍵不該擋路）") {
    entt::registry reg;
    MapData map = make_open_map(11, 1);
    auto knight = make_unit(reg, 0, 0, TEAM_ENEMY, 10, 3, /*move*/5, /*attack_range*/1);
    make_unit(reg, 10, 0, TEAM_PLAYER);

    auto a = decide_tactics(reg, map, knight, false, false);
    REQUIRE(a.kind == TacticsActionKind::MoveTo);
    CHECK(a.x == 5);   // 走滿移動力，還進不了射程
}

TEST_CASE("AI — 沒有敵人時待機，不會亂走") {
    entt::registry reg;
    MapData map = make_open_map(5, 5);
    auto me = make_unit(reg, 2, 2, TEAM_ENEMY);
    auto a = decide_tactics(reg, map, me, false, false);
    CHECK(a.kind == TacticsActionKind::Wait);
}

// ---- Battle -----------------------------------------------------------------

TEST_CASE("Battle — 一回合可移動一次再攻擊一次；攻擊後回合結束") {
    entt::registry reg;
    MapData map = make_open_map(9, 1);
    Battle b;
    auto me  = make_unit(reg, 0, 0, TEAM_PLAYER, 10, /*attack*/3, /*move*/4, /*range*/1);
    auto foe = make_unit(reg, 5, 0, TEAM_ENEMY, 10, 3, 4, 1, /*speed*/1);  // 慢，不會插隊
    b.add_unit(reg, me);
    b.add_unit(reg, foe);

    REQUIRE(b.begin_next_turn(reg) == me);
    std::vector<TacticsEvent> ev;

    CHECK(b.command(reg, map, TacticsAction::move_to(4, 0), &ev));
    CHECK(b.moved());
    CHECK(b.turn_active());                    // 移動後回合還沒結束

    CHECK_FALSE(b.command(reg, map, TacticsAction::move_to(3, 0), &ev));  // 第二次移動被拒
    CHECK(reg.get<SpatialComponent>(me).x == 4);

    CHECK(b.command(reg, map, TacticsAction::attack(foe), &ev));
    CHECK(b.acted());
    CHECK_FALSE(b.turn_active());              // 攻擊即結束回合
    CHECK(reg.get<HealthComponent>(foe).hp == 7);
}

TEST_CASE("Battle — 攻擊後不能再移動") {
    entt::registry reg;
    MapData map = make_open_map(5, 1);
    Battle b;
    auto me  = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 3, 4, /*attack_range*/2);
    auto foe = make_unit(reg, 2, 0, TEAM_ENEMY, 10, 3, 4, 1, /*speed*/1);
    b.add_unit(reg, me);
    b.add_unit(reg, foe);
    REQUIRE(b.begin_next_turn(reg) == me);

    std::vector<TacticsEvent> ev;
    CHECK(b.command(reg, map, TacticsAction::attack(foe), &ev));
    CHECK_FALSE(b.turn_active());
    CHECK_FALSE(b.command(reg, map, TacticsAction::move_to(1, 0), &ev));
    CHECK(reg.get<SpatialComponent>(me).x == 0);
}

TEST_CASE("Battle — 死亡單位從行動條移除並 destroy") {
    entt::registry reg;
    MapData map = make_open_map(5, 1);
    Battle b;
    auto me  = make_unit(reg, 0, 0, TEAM_PLAYER, 10, /*attack*/99, 4, /*range*/1);
    auto foe = make_unit(reg, 1, 0, TEAM_ENEMY, 5, 3, 4, 1, /*speed*/1);
    b.add_unit(reg, me);
    b.add_unit(reg, foe);
    REQUIRE(b.begin_next_turn(reg) == me);

    std::vector<TacticsEvent> ev;
    CHECK(b.command(reg, map, TacticsAction::attack(foe), &ev));
    CHECK_FALSE(reg.valid(foe));
    CHECK_FALSE(b.bar().contains(foe));
    CHECK(b.winner(reg) == TEAM_PLAYER);
}

TEST_CASE("Battle — 殲滅一方即分勝負，戰鬥結束後不再有回合") {
    entt::registry reg;
    MapData map = make_open_map(5, 1);
    Battle b;
    auto me  = make_unit(reg, 0, 0, TEAM_PLAYER, 10, 99, 4, 1);
    auto foe = make_unit(reg, 1, 0, TEAM_ENEMY, 5, 3, 4, 1, 1);
    b.add_unit(reg, me);
    b.add_unit(reg, foe);

    CHECK(b.winner(reg) == Battle::kNoWinner);
    REQUIRE(b.begin_next_turn(reg) == me);

    std::vector<TacticsEvent> ev;
    b.command(reg, map, TacticsAction::attack(foe), &ev);

    CHECK(b.winner(reg) == TEAM_PLAYER);
    CHECK((b.begin_next_turn(reg) == entt::null));   // 已分勝負
}

TEST_CASE("Battle — AI 自動打完整場，必定分出勝負") {
    entt::registry reg;
    MapData map = make_open_map(12, 6);
    Battle b;

    // 兩隊全交給 AI：一場能自己跑完的戰鬥
    for (int i = 0; i < 3; ++i) make_unit(reg, 0, i * 2, TEAM_PLAYER, 12, 4, 3, 1, 100);
    for (int i = 0; i < 3; ++i) make_unit(reg, 11, i * 2, TEAM_ENEMY, 10, 3, 3, 1, 110);
    for (auto e : reg.view<TacticsUnitComponent>()) b.add_unit(reg, e);

    std::vector<TacticsEvent> ev;
    int turns = 0;
    while (b.winner(reg) == Battle::kNoWinner && turns < 500) {
        auto cur = b.begin_next_turn(reg);
        if (cur == entt::null) break;
        b.run_ai_turn(reg, map, &ev);
        ++turns;
    }

    CHECK(turns < 500);                       // 沒有卡死
    CHECK(b.winner(reg) != Battle::kNoWinner);
    // 事件流裡至少要有移動、攻擊、死亡三種，證明整條路徑都跑過
    auto has = [&](TacticsEventKind k) {
        return std::any_of(ev.begin(), ev.end(),
                           [k](const TacticsEvent& e) { return e.kind == k; });
    };
    CHECK(has(TacticsEventKind::Moved));
    CHECK(has(TacticsEventKind::Attacked));
    CHECK(has(TacticsEventKind::UnitDied));
}
