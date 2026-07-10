#pragma once
#include <vector>
#include <entt/entt.hpp>

namespace zone::tactics {

// ActionBar — 行動值排程（ToME4 能量模型）。
//
// 每個 tick，所有單位的 `TurnGaugeComponent::ct` 累加自己的
// `TacticsUnitComponent::speed`；第一個達到 kCtToAct 的單位出手。
//
// **出手後 ct -= kCtToAct，不歸零**：溢出的行動值必須保留，否則速度差異會被吃掉
// ——速度 150 的單位就永遠不會「偶爾在慢角之間連動兩次」。這是這類系統最常見的
// 實作錯誤，`consume()` 的註解與 test_tactics.cpp 都盯著它。
//
// 語意與常數沿用 workflows/specs/archive/2026-06-07-action-and-turn-scheduler-design.md
// §4–5 的「A. EnergyInstant」。刻意獨立於地城的 TurnEngine，不共用。
class ActionBar {
public:
    // 行動值達此值才能出手。速度 100 的單位 → 每 10 tick 出手一次。
    static constexpr int kCtToAct = 1000;

    void add(entt::entity e);
    void remove(entt::entity e);
    bool contains(entt::entity e) const;
    void clear();
    std::size_t size() const { return units_.size(); }
    const std::vector<entt::entity>& units() const { return units_; }

    // 推進時間直到某個單位可以出手，回傳該單位。
    // 多個同時達標 → ct 高者優先；ct 平手 → 加入序在前者優先（確定性，無隨機）。
    // 沒有單位、或所有單位速度 <= 0（永遠推不動）→ 回 entt::null。
    entt::entity advance(entt::registry& reg);

    // 出手完畢。ct -= kCtToAct（保留溢出）。
    void consume(entt::registry& reg, entt::entity e) const;

    // 出手序預測（行動條 UI 用）：在複本上模擬，不動真實 ct。
    std::vector<entt::entity> forecast(const entt::registry& reg, int n) const;

private:
    std::vector<entt::entity> units_;
};

} // namespace zone::tactics
