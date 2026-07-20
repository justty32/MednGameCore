#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/array.hpp>

#include "core/maps/map_data.h"
#include "core/tactics/battle.h"
#include "core/tactics/tactics_event.h"

#include <entt/entt.hpp>
#include <cstdint>
#include <random>
#include <vector>

namespace zone_gd {

// TacticsBattle — SRPG 戰棋戰鬥的橋樑層。與 ZoneWorld 平行、互不依賴。
//
// 規則全在 core（zone::tactics::Battle）；這裡只負責：
//   registry/地圖的所有權、推進迴圈 pump()、事件 → Godot signal、資料查詢。
// 一如 ZoneWorld，**只吐資料不做圖形決策**：tile 與顏色由 tactics_view.gd 決定。
//
// 單位 id ＝ entt::to_integral(entity)（含 version 位元，可安全轉回）。
class TacticsBattle : public godot::Node {
    GDCLASS(TacticsBattle, godot::Node)

protected:
    static void _bind_methods();

public:
    TacticsBattle();
    ~TacticsBattle() override = default;

    // ---- 開局 ----
    // 刻意**不在 _ready() 自動開局**：開局的 pump() 會跑敵方 AI 並發 signal，
    // 前端必須先接好信號再呼叫，否則第一批事件會憑空消失。
    void start_battle(int seed);
    void set_seed(int seed);
    int  get_seed() const;

    // ---- 地圖 ----
    int get_map_width()  const;
    int get_map_height() const;
    // row-major（index = y*width + x），bit0 可走。戰棋沒有戰爭迷霧。
    godot::PackedByteArray get_tile_flags() const;

    // ---- 單位 ----
    // Array[Dictionary]：id / x / y / team / hp / max_hp / attack / speed
    //                  / move_range / attack_range / ct
    godot::Array get_units() const;
    godot::Array get_turn_order(int n) const;   // 接下來 n 個出手者的 id（行動條 UI）

    // ---- 當前回合 ----
    int  current_unit() const;        // -1 ＝ 沒有
    bool is_waiting_player() const;   // 輪到我方單位、且回合尚未結束
    bool can_move() const;            // 本回合還沒移動過
    bool can_act()  const;            // 本回合還沒行動過

    godot::PackedVector2Array get_move_tiles() const;   // 當前單位可移動到的格
    godot::Array get_attack_targets() const;            // 當前單位可攻擊的 id
    godot::PackedVector2Array get_path_to(int x, int y) const;  // 走路動畫用

    // ---- 玩家指令（回傳是否被接受）----
    bool command_move(int x, int y);
    bool command_attack(int unit_id);
    bool command_wait();

    // ---- 勝負 ----
    int  get_winner() const;          // -1 未分勝負；0 我方；1 敵方
    bool is_battle_over() const;

private:
    void build_arena();
    void spawn_units();
    void pump();                      // 推進到「輪到玩家」或「戰鬥結束」
    void drain_events();              // 事件 → signal
    void after_player_command();

    entt::entity to_entity(int id) const;
    static int   to_id(entt::entity e);

    entt::registry reg_;
    zone::MapData  map_;
    zone::tactics::Battle battle_;
    std::vector<zone::tactics::TacticsEvent> events_;

    std::mt19937 rng_;
    uint32_t     seed_{ 0 };
    bool         battle_over_emitted_{ false };
};

} // namespace zone_gd
