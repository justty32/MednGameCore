#include "core/tactics/battle.h"

#include "core/tactics/apply_tactics_action.h"
#include "core/tactics/tactics_ai.h"
#include "core/components/health_component.h"
#include "core/components/team_component.h"
#include "core/components/tactics_unit_component.h"
#include "core/components/turn_gauge_component.h"

#include <array>

namespace zone::tactics {

void Battle::reset() {
    bar_.clear();
    current_ = entt::null;
    turn_active_ = false;
    moved_ = acted_ = false;
}

void Battle::add_unit(entt::registry& reg, entt::entity e) {
    if (!reg.valid(e)) return;
    reg.get_or_emplace<TurnGaugeComponent>(e);
    bar_.add(e);
}

int Battle::winner(const entt::registry& reg) const {
    std::array<int, 2> alive{ 0, 0 };
    for (auto e : reg.view<const TacticsUnitComponent, const TeamComponent, const HealthComponent>()) {
        if (reg.get<const HealthComponent>(e).hp <= 0) continue;
        const uint8_t t = reg.get<const TeamComponent>(e).team;
        if (t < alive.size()) ++alive[t];
    }
    if (alive[TEAM_PLAYER] == 0 && alive[TEAM_ENEMY] == 0) return kNoWinner;  // 同歸於盡：不判
    if (alive[TEAM_ENEMY]  == 0) return TEAM_PLAYER;
    if (alive[TEAM_PLAYER] == 0) return TEAM_ENEMY;
    return kNoWinner;
}

entt::entity Battle::begin_next_turn(entt::registry& reg) {
    if (winner(reg) != kNoWinner) {
        turn_active_ = false;
        current_ = entt::null;
        return entt::null;
    }
    current_ = bar_.advance(reg);
    turn_active_ = (current_ != entt::null);
    moved_ = acted_ = false;
    return current_;
}

void Battle::end_turn(entt::registry& reg) {
    if (current_ != entt::null) bar_.consume(reg, current_);
    turn_active_ = false;
}

// 只看 [from, end) 這段——也就是本次 command 新產生的事件。
// 掃整個向量的話，events 跨回合累積時會一再重掃舊的死亡事件。
void Battle::purge_dead(entt::registry& reg, std::vector<TacticsEvent>* events, std::size_t from) {
    if (!events) return;
    for (std::size_t i = from; i < events->size(); ++i) {
        const auto& ev = (*events)[i];
        if (ev.kind != TacticsEventKind::UnitDied) continue;
        bar_.remove(ev.b);
        if (ev.b == current_) {
            // 理論上打不到自己，但別讓 current_ 懸空
            current_ = entt::null;
            turn_active_ = false;
        }
        if (reg.valid(ev.b)) reg.destroy(ev.b);
    }
}

bool Battle::command(entt::registry& reg, const MapData& map,
                     const TacticsAction& a, std::vector<TacticsEvent>* events) {
    if (!turn_active_ || current_ == entt::null) return false;

    // 子狀態閘門：移動只有一次；攻擊只有一次；攻擊過就不能再移動。
    switch (a.kind) {
        case TacticsActionKind::MoveTo:
            if (moved_ || acted_) return false;
            break;
        case TacticsActionKind::Attack:
            if (acted_) return false;
            break;
        case TacticsActionKind::Wait:
            break;
    }

    const std::size_t before = events ? events->size() : 0;
    if (!apply_tactics_action(reg, map, current_, a, events)) return false;

    switch (a.kind) {
        case TacticsActionKind::MoveTo:
            moved_ = true;
            break;
        case TacticsActionKind::Attack:
            acted_ = true;
            purge_dead(reg, events, before);
            if (turn_active_) end_turn(reg);   // 攻擊即結束回合
            break;
        case TacticsActionKind::Wait:
            end_turn(reg);
            break;
    }
    return true;
}

void Battle::run_ai_turn(entt::registry& reg, const MapData& map, std::vector<TacticsEvent>* events) {
    // 最多三步：移動 → 攻擊 → Wait。有上限才不會因為 AI 一直回傳被拒的指令而卡死。
    for (int i = 0; i < 3 && turn_active_; ++i) {
        const TacticsAction a = decide_tactics(reg, map, current_, moved_, acted_);
        if (!command(reg, map, a, events)) {
            // AI 給了非法指令：直接結束它的回合，別讓戰鬥停住
            command(reg, map, TacticsAction::wait(), events);
            return;
        }
        if (a.kind == TacticsActionKind::Wait) return;
    }
    if (turn_active_) command(reg, map, TacticsAction::wait(), events);
}

} // namespace zone::tactics
