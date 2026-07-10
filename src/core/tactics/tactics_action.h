#pragma once
#include <cstdint>
#include <entt/entt.hpp>

namespace zone::tactics {

// 一個單位在自己回合內可以下的指令。
//
// 傳統戰棋（FE / FFT）的一回合是「移動一次 ＋ 行動一次」，不是「只做一件事」，
// 而且攻擊後不能再移動。單位回合的子狀態由 BattleState 維護，本型別只是指令。
enum class TacticsActionKind : uint8_t {
    Wait,    // 結束本回合
    MoveTo,  // 移動到 (x,y)：必須在 move_range 可達集內
    Attack,  // 攻擊 target：必須在 attack_range 內、且為敵隊
};

struct TacticsAction {
    TacticsActionKind kind{ TacticsActionKind::Wait };
    int          x{ 0 };
    int          y{ 0 };
    entt::entity target{ entt::null };

    static TacticsAction wait()                 { return { TacticsActionKind::Wait, 0, 0, entt::null }; }
    static TacticsAction move_to(int x, int y)  { return { TacticsActionKind::MoveTo, x, y, entt::null }; }
    static TacticsAction attack(entt::entity t) { return { TacticsActionKind::Attack, 0, 0, t }; }
};

} // namespace zone::tactics
