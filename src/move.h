//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include "enum.h"
#include "position.h"
#include "util.h"
//-------------------------------------------//
struct s_move
{
	move code;
	int value;
	operator move() const { return code; }
	void operator=(const move m) { code = m; }
	bool operator<(const s_move m) const { return value < m.value; }
	bool operator>(const s_move m) const { return value > m.value; }
	[[nodiscard]] std::string to_string() const
	{
		return move_to_str(code);
	}
};
//-------------------------------------------//
template <piece_type Pt>s_move* generate_piece_moves(const Position& pos, s_move* movelist, uint64_t occ, uint64_t to_squares, uint64_t from_mask = ~0);
//-------------------------------------------//
template <color Side, move_type Mt>s_move* generate_pawn_moves(const Position& pos, s_move* movelist, uint64_t from_mask = ~0, uint64_t to_mask = ~0);
//-------------------------------------------//
template <color Side>s_move* generate_castles(const Position& pos, s_move* movelist);
//-------------------------------------------//
template <color Side, move_type Mt>s_move* generate_pseudo_legal_moves(const Position& pos, s_move* movelist, uint64_t from_mask);
//-------------------------------------------//
template <color Side>s_move* generate_evasions(const Position& pos, s_move* movelist);
//-------------------------------------------//
template <color Side, move_type Mt>s_move* generate_pinned_moves(const Position& pos, s_move* movelist);
//-------------------------------------------//
template <color Side, move_type Mt>s_move* generate_legal_moves(const Position& pos, s_move* movelist);
//-------------------------------------------//
template <move_type Mt>s_move* generate_all(const Position& pos, s_move* movelist);
//-------------------------------------------//
template <color Side>s_move* generate_quiet_checks(const Position& pos, s_move* movelist);
//-------------------------------------------//
template <move_type T>
struct move_list
{
	explicit move_list(const Position& pos) : move_list_{}, last_(generate_all<T>(pos, move_list_))
	{
	}
	[[nodiscard]] const s_move* begin() const { return move_list_; }
	[[nodiscard]] const s_move* end() const { return last_; }
	[[nodiscard]] size_t size() const { return last_ - move_list_; }
	[[nodiscard]] bool contains(const move mv) const
	{
		return std::find(begin(), end(), mv) != end();
	}
private:
	s_move move_list_[256], * last_;
};
//-------------------------------------------//
class move_gen
{
public:
	int depth;
	int state;
	int threshold;
	move counter_move;
	move hashmove;
	move next_move(const searchinfo* info, bool skip_quiets = false);
	move_gen(Position* p, search_type type, move hshm, int t, int d);
	s_move move_list[256]{};
	s_move* curr{}, * end_moves{}, * end_bad_captures{};
	template <pick_type Type> move select_move();
	void score_moves(const searchinfo* info, score_type type) const;
private:
	Position* pos_;
	[[nodiscard]] s_move* begin() const { return curr; }
	[[nodiscard]] s_move* end() const { return end_moves; }
};
//-------------------------------------------//