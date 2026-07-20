#pragma once

namespace zone {

// 行動條的行動值（ToME4 能量模型）。ct >= ActionBar::kCtToAct 即可出手。
// 出手後 ct -= kCtToAct（**不歸零**）：溢出的行動值必須保留，
// 否則速度差異會被吃掉，快角永遠不會「偶爾連動兩次」。
struct TurnGaugeComponent {
    int ct{ 0 };

    template<class Archive>
    void serialize(Archive& ar) { ar(ct); }
};

} // namespace zone
