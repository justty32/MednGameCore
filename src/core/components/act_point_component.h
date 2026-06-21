#pragma once

namespace zone {

// 行動點數（最傳統回合制）。每回合開始由 TurnEngine 重設為 1000；act() 消耗。
// 目前一個 actor 每回合行動一次，act_point 為未來「依成本行動多次」預留。
struct ActPointComponent {
    int value{ 0 };

    template<class Archive>
    void serialize(Archive& ar) { ar(value); }
};

} // namespace zone
