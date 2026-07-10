#pragma once
#include <vector>
#include <entt/entt.hpp>
#include "core/tactics/action_bar.h"
#include "core/tactics/tactics_action.h"
#include "core/tactics/tactics_event.h"

namespace zone {
struct MapData;
}

namespace zone::tactics {

// Battle — 一場戰棋戰鬥的狀態機。核心層，godot-free、可單元測試。
//
// 一個單位的回合是「移動一次 ＋ 行動一次」，攻擊後不能再移動：
//
//   輪到此單位 ──MoveTo──> 已移動 ──Attack──> 結束（攻擊即結束回合）
//        │                    └───Wait────> 結束
//        ├──Attack──> 結束
//        └──Wait────> 結束
//
// 結束時 ActionBar::consume()（ct -= 1000，保留溢出），再 begin_next_turn()。
class Battle {
public:
    static constexpr int kNoWinner = -1;

    void reset();
    void add_unit(entt::registry& reg, entt::entity e);   // 順帶補上 TurnGaugeComponent
    const ActionBar& bar() const { return bar_; }

    // 推進行動條到下一個出手的單位。戰鬥已分勝負則回 entt::null。
    entt::entity begin_next_turn(entt::registry& reg);

    entt::entity current() const { return current_; }
    bool turn_active() const { return turn_active_; }
    bool moved() const { return moved_; }
    bool acted() const { return acted_; }

    // 下一個指令給 current()。回傳 true ＝ 被接受。
    // 非法指令（不可達 / 超射程 / 已經移動過又要移動）不消耗回合，回 false。
    bool command(entt::registry& reg, const MapData& map,
                 const TacticsAction& a, std::vector<TacticsEvent>* events);

    // 讓 AI 打完 current() 的整個回合（會反覆決策直到回合結束）。
    void run_ai_turn(entt::registry& reg, const MapData& map, std::vector<TacticsEvent>* events);

    // 存活單位被清空的隊伍 → 對手獲勝。未分勝負回 kNoWinner。
    int winner(const entt::registry& reg) const;

private:
    void end_turn(entt::registry& reg);
    void purge_dead(entt::registry& reg, std::vector<TacticsEvent>* events, std::size_t from);

    ActionBar    bar_;
    entt::entity current_{ entt::null };
    bool         turn_active_{ false };
    bool         moved_{ false };
    bool         acted_{ false };
};

} // namespace zone::tactics
