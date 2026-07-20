#pragma once

namespace zone {

// 戰棋單位屬性。只給 src/core/tactics/ 用；地城那側的 actor 不掛這個。
//   move_range   一回合最多移動幾格（Dijkstra 步數，斜角不可走）
//   attack_range 攻擊的 Chebyshev 射程（1 ＝ 只能打相鄰）
//   speed        行動值累積速率；100 ＝ 每 tick +100，10 tick 出手一次
struct TacticsUnitComponent {
    int move_range{ 4 };
    int attack_range{ 1 };
    int speed{ 100 };

    template<class Archive>
    void serialize(Archive& ar) { ar(move_range, attack_range, speed); }
};

} // namespace zone
