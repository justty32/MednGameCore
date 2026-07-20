#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/array.hpp>

#include "core/ecs/entity_manager.h"
#include "core/turn/turn_engine.h"
#include "core/turn/zone_event.h"

#include <entt/entt.hpp>
#include <cstdint>
#include <random>
#include <vector>
#include <string>

namespace zone_gd {

// ZoneWorld — gamecore 區域層核心 Node。
//
// 最傳統的回合制：TurnEngine 維護一條 actor linked list（加入序＝回合序）。
//   next_round()         ＝「下一回合」按鈕：從 head 起，每個 actor act_point=1000 後 act()
//   遇到玩家 actor 會暫停（is_waiting_player()），等 player_move()/player_wait() 投遞指令後續跑
//   一輪跑完發出 round_finished signal
//
// 呈現層（圖塊、顏色、字型）不在這裡：本類別只吐「地形旗標」與「實體種類＋座標」，
// 由 godot_zone/map_view.gd 決定畫成什麼樣子。
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

    // 整張地圖的旗標，一次過河（row-major，index = y * width + x）：
    //   bit0 可走 / bit1 下樓梯 / bit2 本回合可見 / bit3 曾探索
    godot::PackedByteArray get_tile_flags() const;

    // 目前該畫出來的實體：Array[Dictionary]，鍵 x / y / kind / hp / max_hp。
    // kind ∈ {"hero", "npc", "item"}；只含視野內的（英雄恆含）。繪製序＝陣列序。
    godot::Array get_visible_entities() const;

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
    int  get_hero_x()      const;
    int  get_hero_y()      const;
    int  get_turn_count()  const;
    int  get_hero_hp()     const;
    int  get_hero_max_hp() const;
    int  get_hero_attack() const;
    int  get_npc_count()   const;
    int  get_current_floor() const;
    int  get_win_floor()   const;   // 走下這一層的樓梯即獲勝
    bool is_game_over()    const;
    bool is_game_won()     const;
    void restart();

    // ---- 隨機種子（可重現的跑法；headless 驗證仰賴此）----
    // set_seed() 後 restart() 一律以該種子重建；未設過則每次 restart 抽新種子。
    void set_seed(int seed);
    int  get_seed() const;

    // ---- 存讀檔 ----
    bool save_game(const godot::String& path);
    bool load_game(const godot::String& path);
    bool has_save_game(const godot::String& path) const;

private:
    void setup_world();
    void setup_map();
    void next_floor();
    void recompute_fov();
    bool finished() const;   // 死亡或通關：玩家指令一律無效

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

    bool trace_enabled_{ false };   // 預設關；由前端的除錯面板開關（Tab）打開
    std::vector<std::string> trace_log_;

    entt::entity map_entity_{ entt::null };
    entt::entity hero_entity_{ entt::null };
    int  turn_count_{ 0 };
    bool game_over_{ false };
    bool game_won_{ false };
    int  current_floor_{ 1 };

    std::mt19937 rng_;         // 地圖生成 + NPC AI 共用（種子固定 → 整場可重現）
    uint32_t     seed_{ 0 };
    bool         seed_fixed_{ false };
};

} // namespace zone_gd
