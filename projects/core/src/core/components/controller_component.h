#pragma once
#include <cstdint>
#include <type_traits>
#include <entt/entt.hpp>

namespace zone {

// 操控者種類（actor 的行動由誰決定）。
//   Player  : 玩家操控 — act() 時暫停回合，等玩家指令（TurnEngine 回 WaitingPlayer）。
//   Self    : NPC 自己操控 — 由 AI 立即決策。
//   Faction : 由勢力（軍團）操控 — 尚未實作，暫時等同 Self。
enum class ControllerKind : uint8_t { Player, Self, Faction };

struct ControllerComponent {
    ControllerKind kind{ ControllerKind::Self };
    entt::entity   faction{ entt::null };  // kind==Faction 時指向勢力實體

    // entt::entity 是強型別 enum，需手動轉底層整數（仿 spatial_component 的做法）。
    template<class Archive>
    void save(Archive& ar) const {
        ar(static_cast<uint8_t>(kind));
        using raw_t = std::underlying_type_t<entt::entity>;
        ar(static_cast<raw_t>(faction));
    }
    template<class Archive>
    void load(Archive& ar) {
        uint8_t k{}; ar(k); kind = static_cast<ControllerKind>(k);
        using raw_t = std::underlying_type_t<entt::entity>;
        raw_t raw{}; ar(raw); faction = static_cast<entt::entity>(raw);
    }
};

} // namespace zone
