#include "core/tactics/action_bar.h"

#include "core/components/tactics_unit_component.h"
#include "core/components/turn_gauge_component.h"

#include <algorithm>

namespace zone::tactics {

namespace {

// 一個單位在排程裡的最小狀態。forecast() 在這上面模擬，不碰 registry。
struct Slot {
    entt::entity e;
    int ct;
    int speed;
    int order;   // 加入序，用於平手時的確定性排序
};

// 目前誰該出手？ct 最高者；平手取加入序在前者。都沒達標則回 -1。
int ready_index(const std::vector<Slot>& slots) {
    int best = -1;
    for (int i = 0; i < (int)slots.size(); ++i) {
        if (slots[i].ct < ActionBar::kCtToAct) continue;
        if (best < 0 || slots[i].ct > slots[best].ct) best = i;
        // ct 相同時不換人：slots 已按加入序排列，先到先出手
    }
    return best;
}

} // namespace

void ActionBar::add(entt::entity e) {
    if (contains(e)) return;
    units_.push_back(e);
}

void ActionBar::remove(entt::entity e) {
    units_.erase(std::remove(units_.begin(), units_.end(), e), units_.end());
}

bool ActionBar::contains(entt::entity e) const {
    return std::find(units_.begin(), units_.end(), e) != units_.end();
}

void ActionBar::clear() { units_.clear(); }

entt::entity ActionBar::advance(entt::registry& reg) {
    if (units_.empty()) return entt::null;

    for (;;) {
        // 已經有人達標就直接出手（例如上一次出手後溢出的 ct 仍 >= kCtToAct）。
        // 嚴格大於 → ct 平手時保留先掃到的，也就是加入序在前者。
        entt::entity best = entt::null;
        int best_ct = 0;
        for (auto e : units_) {
            if (!reg.valid(e)) continue;
            const auto* g = reg.try_get<TurnGaugeComponent>(e);
            if (!g || g->ct < kCtToAct) continue;
            if (best == entt::null || g->ct > best_ct) {
                best = e;
                best_ct = g->ct;
            }
        }
        if (best != entt::null) return best;

        // 沒人達標 → tick 一次
        bool any_moving = false;
        for (auto e : units_) {
            if (!reg.valid(e)) continue;
            auto* g = reg.try_get<TurnGaugeComponent>(e);
            const auto* u = reg.try_get<TacticsUnitComponent>(e);
            if (!g || !u) continue;
            if (u->speed > 0) {
                g->ct += u->speed;
                any_moving = true;
            }
        }
        if (!any_moving) return entt::null;  // 全員速度 <= 0：永遠推不動，別無限迴圈
    }
}

void ActionBar::consume(entt::registry& reg, entt::entity e) const {
    if (!reg.valid(e)) return;
    if (auto* g = reg.try_get<TurnGaugeComponent>(e))
        g->ct -= kCtToAct;   // 保留溢出，不是 g->ct = 0
}

std::vector<entt::entity> ActionBar::forecast(const entt::registry& reg, int n) const {
    std::vector<entt::entity> out;
    if (n <= 0) return out;

    std::vector<Slot> slots;
    slots.reserve(units_.size());
    int order = 0;
    for (auto e : units_) {
        if (!reg.valid(e)) continue;
        const auto* g = reg.try_get<TurnGaugeComponent>(e);
        const auto* u = reg.try_get<TacticsUnitComponent>(e);
        if (!g || !u) continue;
        slots.push_back({ e, g->ct, u->speed, order++ });
    }
    if (slots.empty()) return out;

    const bool any_moving = std::any_of(slots.begin(), slots.end(),
                                        [](const Slot& s) { return s.speed > 0; });

    while ((int)out.size() < n) {
        int i = ready_index(slots);
        if (i < 0) {
            if (!any_moving) break;   // 推不動，預測就到此為止
            for (auto& s : slots) if (s.speed > 0) s.ct += s.speed;
            continue;
        }
        out.push_back(slots[i].e);
        slots[i].ct -= kCtToAct;      // 與 consume() 同一條規則
    }
    return out;
}

} // namespace zone::tactics
