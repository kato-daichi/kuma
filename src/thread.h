//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <array>
#include <csetjmp>
#include "search.h"
#include "hash.h"
//-------------------------------------------//
#pragma warning(disable: 4324)
//-------------------------------------------//
struct pawnhash_entry
{
	int king_pos[2];
	int pawn_shelter[2];
	score scores[2];
	uint64_t attack_spans[2];
	uint64_t passed_pawns[2];
	uint64_t pawn_hash;
	uint8_t castling;
	uint8_t semiopen_files[2];
};
//-------------------------------------------//
struct materialhash_entry
{
	bool is_special_endgame;
	bool is_drawn;
	int (*evaluation)(const Position&);
	int phase;
	score score;
	uint64_t key;
};
//-------------------------------------------//
struct search_thread
{
	bool do_nmp;
	int root_height;
	int16_t history_table[2][64][64];
	int16_t seldepth;
	jmp_buf jbuffer;
	materialhashentry material_table[material_entries];
	move counter_move_table[14][64];
	pawnhash_entry pawn_table[pawn_entries];
	piece_to_history counter_move_history[14][64];
	Position position;
	searchinfo ss[max_ply + 2];
	uint16_t thread_id;
	uint64_t nodes;
};
//-------------------------------------------//
inline int num_threads = 1;
//-------------------------------------------//
inline bool is_main_thread(const Position* p) { return p->my_thread->thread_id == 0; }
//-------------------------------------------//
inline searchthread main_thread;
inline searchthread* search_threads;
void clear_threads();
void get_ready();
void init_threads();
void reset_threads(int thread_num);
void* get_thread(int thread_id);
