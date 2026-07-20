#pragma once
#include <list>
#include <unordered_map>
#include <functional>
#include <string>
#include <entt/entt.hpp>
#include "core/turn/action.h"

namespace zone {

// 一次 step() 的結果。
enum class StepResult {
    Acted,         // 一個 actor 行動完畢，可再 step()
    WaitingPlayer, // 卡在玩家操控的 actor，等 submit() 後再 step()
    RoundDone,     // 本回合 list 已全部跑完
};

// step() 所需的外界依賴。registry 不由引擎持有，保持引擎純粹、可測。
struct EngineCtx {
    entt::registry& reg;
    std::function<Action(entt::entity)>              decide;   // Self/Faction：取得其決策
    std::function<void(entt::entity, const Action&)> resolve;  // 落地一個動作（act 內容）
    std::function<void(const std::string&)>          trace = nullptr;
};

// 最傳統的回合制引擎。
//
// 核心就是一條 actor 的 linked list（順序＝加入序）：新 actor 出現就 append。
// 「下一回合」＝ begin_round() 後反覆 step() 直到 RoundDone。
// 每個 actor 首次輪到時，行動點數 act_point 重設為 1000，再依「操控者」決定行為：
//   Player          → 回 WaitingPlayer 暫停，等 submit() 指令後續跑（即「act() 阻塞等玩家」）
//   Self / Faction  → 立即用 ctx.decide() 取得動作並 resolve()
class TurnEngine {
public:
    // ---- list 維護 ----
    void add_actor(entt::entity e);     // 新 actor 出現 → append（重複則略過）
    void remove_actor(entt::entity e);  // 死亡 / 離場 → 移除（含 cursor 安全處理）
    bool contains(entt::entity e) const;
    void clear();
    const std::list<entt::entity>& order() const { return order_; }
    std::size_t size() const { return order_.size(); }

    // ---- 回合推進 ----
    void begin_round();                                  // 從 head 重新開始一輪
    bool round_in_progress() const { return in_round_; }
    StepResult step(EngineCtx& ctx);

    // ---- 玩家指令 ----
    void submit(entt::entity e, const Action& a) { pending_[e] = a; }
    entt::entity waiting_actor() const { return waiting_; }

    // 行動點數初值（每回合每 actor 重設為此值）。
    static constexpr int kActPointFull = 1000;

private:
    bool take_pending(entt::entity e, Action& out);

    std::list<entt::entity>                     order_;
    std::list<entt::entity>::iterator           cursor_{ order_.end() };
    std::unordered_map<entt::entity, Action>    pending_;
    bool          in_round_{ false };
    bool          cursor_started_{ false };  // 目前 cursor 的 actor 是否已重設 act_point
    entt::entity  waiting_{ entt::null };
};

} // namespace zone
