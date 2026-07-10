#pragma once
#include <cstdint>

namespace zone {

// 戰棋單位的陣營。勝負判定＝某一隊的單位被清空。
// 與 ControllerComponent 無關：那個管「誰下指令」，這個管「誰跟誰是敵人」。
inline constexpr uint8_t TEAM_PLAYER = 0;
inline constexpr uint8_t TEAM_ENEMY  = 1;

struct TeamComponent {
    uint8_t team{ TEAM_PLAYER };

    template<class Archive>
    void serialize(Archive& ar) { ar(team); }
};

} // namespace zone
