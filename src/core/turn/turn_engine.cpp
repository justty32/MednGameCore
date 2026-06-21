#include "core/turn/turn_engine.h"
#include "core/components/actor_component.h"
#include "core/components/act_point_component.h"
#include "core/components/controller_component.h"

#include <algorithm>

namespace zone {

void TurnEngine::add_actor(entt::entity e) {
    if (contains(e)) return;
    order_.push_back(e);
}

void TurnEngine::remove_actor(entt::entity e) {
    auto it = std::find(order_.begin(), order_.end(), e);
    if (it == order_.end()) return;
    if (in_round_ && it == cursor_) {
        // 正要輪到它就被移除：cursor 前移到下一個，避免懸空
        cursor_ = order_.erase(it);
        cursor_started_ = false;
    } else {
        order_.erase(it);
    }
    if (waiting_ == e) waiting_ = entt::null;
    pending_.erase(e);
}

bool TurnEngine::contains(entt::entity e) const {
    return std::find(order_.begin(), order_.end(), e) != order_.end();
}

void TurnEngine::clear() {
    order_.clear();
    pending_.clear();
    cursor_ = order_.end();
    in_round_ = false;
    cursor_started_ = false;
    waiting_ = entt::null;
}

void TurnEngine::begin_round() {
    cursor_ = order_.begin();
    cursor_started_ = false;
    in_round_ = true;
    waiting_ = entt::null;
}

bool TurnEngine::take_pending(entt::entity e, Action& out) {
    auto it = pending_.find(e);
    if (it == pending_.end()) return false;
    out = it->second;
    pending_.erase(it);
    return true;
}

StepResult TurnEngine::step(EngineCtx& ctx) {
    if (!in_round_) return StepResult::RoundDone;

    while (cursor_ != order_.end()) {
        entt::entity e = *cursor_;

        // 失效 / 非 actor：跳過並從 list 移除
        if (!ctx.reg.valid(e) || !ctx.reg.all_of<ActorComponent>(e)) {
            cursor_ = order_.erase(cursor_);
            cursor_started_ = false;
            continue;
        }

        // 本回合首次輪到此 actor：行動點數設為 1000
        if (!cursor_started_) {
            ctx.reg.get_or_emplace<ActPointComponent>(e).value = kActPointFull;
            cursor_started_ = true;
            if (ctx.trace)
                ctx.trace("turn #" + std::to_string((unsigned)entt::to_integral(e))
                          + " act_point=" + std::to_string(kActPointFull));
        }

        // 操控者分派
        ControllerKind kind = ControllerKind::Self;
        if (const auto* c = ctx.reg.try_get<ControllerComponent>(e)) kind = c->kind;

        if (kind == ControllerKind::Player) {
            Action cmd;
            if (!take_pending(e, cmd)) {
                waiting_ = e;
                return StepResult::WaitingPlayer;  // act() 阻塞：等玩家指令
            }
            waiting_ = entt::null;
            if (ctx.resolve) ctx.resolve(e, cmd);
        } else {
            // Self / Faction（暫等同 Self）
            Action a = ctx.decide ? ctx.decide(e) : Action::wait();
            if (ctx.resolve) ctx.resolve(e, a);
        }

        // 此 actor 本回合結束
        if (auto* ap = ctx.reg.try_get<ActPointComponent>(e)) ap->value = 0;
        ++cursor_;
        cursor_started_ = false;
        return StepResult::Acted;
    }

    in_round_ = false;
    waiting_ = entt::null;
    return StepResult::RoundDone;
}

} // namespace zone
