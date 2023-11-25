//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <algorithm>
#include "board.h"
using piece_to_history = std::array<std::array<int16_t, 64>, 14>;
//-------------------------------------------//
struct search_info
{
	bool singular_extension;
	int static_eval;
	int8_t ply;
	move chosen_move;
	move excluded_move;
	move killers[2];
	move pv[max_ply + 1];
	piece_to_history* counter_move_history;
	uint8_t pv_len;
};
//-------------------------------------------//
inline constexpr int aspiration_init = 15;
inline constexpr int futility_history_limit[2] = { 16000, 9000 };
inline constexpr int improvement_value = 20;
inline constexpr int probcut_margin = 80;
inline constexpr int razor_margin[3] = { 0, 180, 350 };
inline constexpr int singular_depth = 8;
inline constexpr int strong_history = 13000;
inline constexpr int timer_granularity = 2047;
//-------------------------------------------//
const int futility_move_counts[2][9] = {
	{0, 3, 4, 5, 8, 13, 17, 23, 29},
	{0, 5, 7, 10, 16, 24, 33, 44, 58},
};
//-------------------------------------------//
static const int skip_size[16] = { 1, 1, 1, 2, 2, 2, 1, 3, 2, 2, 1, 3, 3, 2, 2, 1 };
//-------------------------------------------//
static const int skip_depths[16] = { 1, 2, 2, 4, 4, 3, 2, 5, 4, 3, 2, 6, 5, 4, 3, 2 };
//-------------------------------------------//
inline int lmr(const bool improving, const int depth, const int num_moves)
{
	return reductions[improving][std::min(depth, 63)][std::min(num_moves, 63)];
}
//-------------------------------------------//
inline bool is_depth = false;
inline bool is_infinite = false;
inline bool is_movetime = false;
inline int global_state = 0;
inline int pv_length = 0;
inline move main_pv[max_ply + 1];
inline move ponder_move;
inline volatile bool is_timeout = false, is_pondering = false;
//-------------------------------------------//
bool is_draw(Position* pos);
inline bool is_valid(move m);
inline uint64_t sum_nodes();
inline void get_time_limit();
inline void history_scores(const Position* pos, const searchinfo* info, move m, int16_t* history, int16_t* counter_move_history, int16_t* follow_up_history);
inline void initialize_nodes();
int alpha_beta(searchthread* thread, searchinfo* info, int depth, int alpha, int beta);
int q_search(searchthread* thread, searchinfo* info, int depth, int alpha, int beta);
int q_search_delta(const Position* pos);
int16_t history_bonus(int depth);
void add_history_bonus(int16_t* history, int16_t bonus);
void check_time(const searchthread* thread);
int print_info(const Position* pos, const searchinfo* info, int depth, int score, int alpha, int beta);
void save_killer(const Position* pos, searchinfo* info, move m, int16_t bonus);
void update_countermove_histories(const searchinfo* info, piece_code pc, int to, int16_t bonus);
void update_heuristics(const Position* pos, searchinfo* info, int best_score, int beta, int depth, move m, const move* quiets, int quiets_count);
void update_time(bool failed_low, bool same_pv, int init_time);
void* aspiration_thread(void* t);
void* think(void* p);