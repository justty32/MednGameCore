#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "core/ecs/entity_manager.h"
#include "core/turn/turn_engine.h"
#include "core/turn/zone_event.h"

#include <entt/entt.hpp>
#include <vector>
#include <string>

namespace zone_gd {

// ZoneWorld — gamecore 區域層核心 Node。
//
// 最傳統的回合制：TurnEngine 維護一條 actor linked list（加入序＝回合序）。
//   next_round()         ＝「下一回合」按鈕：從 head 起，每個 actor act_point=1000 後 act()
//   遇到玩家 actor 會暫停（is_waiting_player()），等 player_move()/player_wait() 投遞指令後續跑
//   一輪跑完發出 round_finished signal（前端可自動接續下一回合）
class ZoneWorld : public godot::Node {
    GDCLASS(ZoneWorld, godot::Node)

protected:
    static void _bind_methods();

public:
    ZoneWorld();
    ~ZoneWorld() override = default;

    void _ready() override;

    // ---- 地圖查詢 ----
    int  get_map_width()  const;
    int  get_map_height() const;
    bool is_walkable(int x, int y) const;
    godot::Ref<godot::Image> generate_map_image(int cell_px) const;

    // ---- 回合制介面 ----
    void next_round();                  // 「下一回合」：開新一輪並推進到玩家等待 / 回合結束
    void player_move(int dx, int dy);   // 玩家指令：移動（卡在玩家時投遞並續跑）
    void player_wait();                 // 玩家指令：原地等待
    bool is_waiting_player() const;     // 目前是否卡在玩家 actor 等指令
    bool round_in_progress() const;

    // ---- UI / debug ----
    godot::String get_debug_text() const;    // 逐 actor 全狀態 dump
    void set_trace_enabled(bool on);
    bool get_trace_enabled() const;
    godot::String get_debug_log() const;     // 最近數十行 trace
    void clear_debug_log();

    // ---- 狀態查詢 ----
    int get_hero_x()      const;
    int get_hero_y()      const;
    int get_turn_count()  const;
    int get_hero_hp()     const;
    int get_hero_max_hp() const;
    int get_npc_count()   const;
    int get_current_floor() const;
    void restart();

    // ---- 存讀檔 ----
    bool save_game(const godot::String& path);
    bool load_game(const godot::String& path);
    bool has_save_game(const godot::String& path) const;

private:
    void setup_world();
    void setup_map();
    void next_floor();
    void recompute_fov();

    // 回合推進內部
    zone::EngineCtx make_ctx();
    void  pump();                 // 反覆 step()，於玩家等待 / 回合結束 / 樓層變更時停下
    bool  drain_events();         // 處理 act() 事件 → signal；回傳「英雄踩到下樓梯」
    void  on_trace(const std::string& line);
    zone::Action npc_decide(entt::entity e);
    void  add_actor_to_engine(entt::entity e);

    zone::EntityManager em_;
    zone::TurnEngine    engine_;
    std::vector<zone::ZoneEvent> events_;

    bool trace_enabled_{ true };
    std::vector<std::string> trace_log_;

    entt::entity map_entity_{ entt::null };
    entt::entity hero_entity_{ entt::null };
    int  turn_count_{ 0 };
    bool game_over_{ false };
    int  current_floor_{ 1 };
};

} // namespace zone_gd
