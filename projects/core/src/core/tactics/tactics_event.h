#pragma once
#include <entt/entt.hpp>

namespace zone::tactics {

// 結算戰棋指令時產生的事件；橋樑層讀取後翻成 Godot signal。
// 核心層不認識 Godot，只把事件丟進 apply_tactics_action 的 events 向量。
enum class TacticsEventKind {
    Moved,      // a 從 (from_x,from_y) 移動到現在的位置
    Attacked,   // a 攻擊 b，amount = 傷害（b 未死）
    UnitDied,   // b 被 a 殺死（b 尚未 destroy，由呼叫端統一移除）
    Rejected,   // 指令非法（不可達 / 超出射程 / 打到自己人）；a = 下令者
};

struct TacticsEvent {
    TacticsEventKind kind;
    entt::entity a{ entt::null };
    entt::entity b{ entt::null };
    int amount{ 0 };
    int from_x{ 0 };
    int from_y{ 0 };
};

} // namespace zone::tactics
