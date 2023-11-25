//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <string>
#include "bitop.h"
#include "const.h"
#include "enum.h"
//-------------------------------------------//
struct state_history
{
	uint64_t key;
	uint64_t pawnhash;
	uint64_t materialhash;
	uint64_t king_blockers[2][2];
	uint64_t check_bb;
	uint8_t castle_rights;
	int ep_square;
	int kingpos[2];
	int halfmove_clock;
	int fullmove_clock;
	piece_code captured_piece;
};
//-------------------------------------------//
class Position
{
public:
	uint64_t key;
	uint64_t pawnhash;
	uint64_t materialhash;
	uint64_t kingblockers[2][2];
	uint64_t check_bb;
	uint8_t castle_rights;
	int ep_square;
	int kingpos[2];
	int halfmove_clock;
	int fullmove_clock;
	piece_code captured_piece;
	uint64_t piece_bb[14];
	uint64_t occupied_bb[2];
	piece_code mailbox[64];
	int piece_count[14];
	color active_side;
	state_history history_stack[512];
	int history_index;
	move move_stack[512];
	searchthread* my_thread;
	int non_pawn[2];
	bool game_cycle;
	static void init();
	[[nodiscard]] bool is_attacked(int sq, int side) const;
	[[nodiscard]] bool is_capture(move m) const;
	[[nodiscard]] bool is_tactical(move m) const;
	[[nodiscard]] bool is_pseudo_legal(move m) const;
	[[nodiscard]] bool is_legal(move m) const;
	[[nodiscard]] bool pawn_on7_th() const;
	[[nodiscard]] int smallest_attacker(uint64_t attackers, color col) const;
	[[nodiscard]] bool see(move m, int threshold) const;
	[[nodiscard]] uint64_t attackers_to(int sq, int side, bool free = false) const;
	[[nodiscard]] uint64_t attackers_to(int sq, int side, uint64_t occ) const;
	[[nodiscard]] uint64_t all_attackers_to(int sq, uint64_t occ) const;
	[[nodiscard]] uint64_t get_attack_set(int sq, uint64_t occ) const;
	bool gives_check(move m);
	[[nodiscard]] bool gives_discovered_check(move m) const;
	[[nodiscard]] bool advanced_pawn_push(move m) const;
	void do_move(move m);
	void undo_move(move m);
	void do_null_move();
	void undo_null_move();
	void set_piece_at(int sq, piece_code pc);
	void remove_piece_at(int sq, piece_code pc);
	void move_piece(int from, int to, piece_code pc);
	[[nodiscard]] uint64_t bad_squares() const;
	[[nodiscard]] bool horizontal_check(uint64_t occ, int sq) const;
	void update_blockers();
	void read_fen(const char* fen);
};
//-------------------------------------------//
inline bool Position::gives_discovered_check(const move m) const
{
	return (kingblockers[~active_side][0] | kingblockers[~active_side][1]) & SET_BIT(from_sq(m));
}
//-------------------------------------------//
Position* start_position();
Position* import_fen(const char* fen, int thread_id);