#pragma once
#include <cstdint>

namespace zone {

// 最傳統回合制的動作種類。攻擊以「移動進入有 actor 的格子」表達（撞擊攻擊），
// 故不另設 Attack；技能/詠唱等先全部拿掉（見 turn 重建）。
enum class ActionKind : uint8_t {
    Idle,  // 無動作
    Move,  // 移動（param = encode_dir 方向；撞牆/撞 actor 由 apply_action 結算）
    Wait,  // 原地等待，消耗本回合
};

// 「指令」值型別：玩家投遞 / NPC 決策共用。
struct Action {
    ActionKind kind{ ActionKind::Idle };
    int        param{ 0 };  // Move：方向編碼

    static Action idle()        { return { ActionKind::Idle, 0 }; }
    static Action wait()        { return { ActionKind::Wait, 0 }; }
    static Action move(int dir) { return { ActionKind::Move, dir }; }
};

} // namespace zone
