//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <algorithm>
#include "position.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "thread.h"
//-------------------------------------------//
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#pragma warning(disable: 4244)
#pragma warning(disable: 5054)
#else
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
//-------------------------------------------//
template <piece_type Pt>
s_move* generate_piece_moves(const Position& pos, s_move* movelist, const uint64_t occ, const uint64_t to_squares, const uint64_t from_mask)
{
	const piece_code pc = make_piece(pos.active_side, Pt);
	uint64_t from_squares = pos.piece_bb[pc] & from_mask;
	uint64_t tobits = 0ULL;
	while (from_squares)
	{
		const int from = pop_lsb(from_squares);
		if (Pt == knight)
			tobits = pseudo_attacks[knight][from] & to_squares;
		if (Pt == bishop || Pt == queen)
			tobits |= bishop_attacks(occ, from) & to_squares;
		if (Pt == rook || Pt == queen)
			tobits |= rook_attacks(occ, from) & to_squares;
		if (Pt == king)
			tobits = pseudo_attacks[king][from] & to_squares & ~pos.bad_squares();
		while (tobits)
		{
			const int to = pop_lsb(tobits);
			*movelist++ = make_move(from, to, normal);
		}
	}
	return movelist;
}
//-------------------------------------------//
inline bool Position::horizontal_check(const uint64_t occ, const int sq) const
{
	const int opponent = active_side ^ side_switch;
	const uint64_t rooks_queens = piece_bb[w_rook | opponent] | piece_bb[w_queen | opponent];
	return rooks_queens && rook_attacks(occ, sq) & rooks_queens;
}
//-------------------------------------------//
template <color Side, move_type Mt>
s_move* generate_pawn_moves(const Position& pos, s_move* movelist, const uint64_t from_mask, uint64_t to_mask)
{
	constexpr color opponent = Side == white ? black : white;
	const uint64_t occ = pos.occupied_bb[0] | pos.occupied_bb[1];
	constexpr auto pc = w_pawn | static_cast<piece_code>(Side);
	constexpr uint64_t t_rank7_bb = Side == white ? rank7_bb : rank2_bb;
	constexpr uint64_t t_rank3_bb = Side == white ? rank3_bb : rank6_bb;
	constexpr direction up = pawn_push(Side);
	constexpr direction up_right = Side == white ? north_east : south_west;
	constexpr direction up_left = Side == white ? north_west : south_east;
	const uint64_t pawns_on7 = pos.piece_bb[pc] & t_rank7_bb & from_mask;
	const uint64_t pawns_not_on7 = pos.piece_bb[pc] & ~t_rank7_bb & from_mask;
	uint64_t enemies = pos.occupied_bb[opponent];
	uint64_t empty_squares = ~occ;
	int to;
	if (Mt == quiet_check)
	{
		uint64_t target = pawn_attacks_from[Side][pos.kingpos[opponent]];
		uint64_t b1 = shift<up>(pawns_not_on7) & empty_squares & target;
		uint64_t b2 = shift<up>(b1 & t_rank3_bb) & empty_squares & target;
		while (b1)
		{
			to = pop_lsb(b1);
			*movelist++ = make_move(to - up, to, normal);
		}
		while (b2)
		{
			to = pop_lsb(b2);
			*movelist++ = make_move(to - up - up, to, normal);
		}
		return movelist;
	}
	if (Mt & quiet)
	{
		uint64_t b1 = shift<up>(pawns_not_on7) & empty_squares & to_mask;
		uint64_t b2 = shift<up>(b1 & t_rank3_bb) & empty_squares & to_mask;
		while (b1)
		{
			to = pop_lsb(b1);
			*movelist++ = make_move(to - up, to, normal);
		}
		while (b2)
		{
			to = pop_lsb(b2);
			*movelist++ = make_move(to - up - up, to, normal);
		}
	}
	if (Mt & capture)
	{
		uint64_t b1 = shift<up_right>(pawns_not_on7) & enemies;
		uint64_t b2 = shift<up_left>(pawns_not_on7) & enemies;
		while (b1)
		{
			to = pop_lsb(b1);
			*movelist++ = make_move(to - up_right, to, normal);
		}
		while (b2)
		{
			to = pop_lsb(b2);
			*movelist++ = make_move(to - up_left, to, normal);
		}
		if (pos.ep_square)
		{
			b1 = pawns_not_on7 & pawn_attacks[opponent][pos.ep_square];
			while (b1)
			{
				if (const int from = pop_lsb(b1); RANK(from) != RANK(pos.kingpos[Side]) || !pos.horizontal_check(occ ^ SET_BIT(from) ^ SET_BIT(pos.ep_square + (Side ? 8 : -8)), pos.kingpos[Side]))
					*movelist++ = make_move(from, pos.ep_square, enpassant);
			}
		}
	}
	if (pawns_on7 && Mt & promote)
	{
		uint64_t b1 = shift<up_right>(pawns_on7) & enemies;
		uint64_t b2 = shift<up_left>(pawns_on7) & enemies;
		uint64_t b3 = shift<up>(pawns_on7) & empty_squares;
		while (b1)
		{
			to = pop_lsb(b1);
			*movelist++ = make_promotion_move(to - up_right, to, queen, promotion);
			*movelist++ = make_promotion_move(to - up_right, to, knight, promotion);
			*movelist++ = make_promotion_move(to - up_right, to, rook, promotion);
			*movelist++ = make_promotion_move(to - up_right, to, bishop, promotion);
		}
		while (b2)
		{
			to = pop_lsb(b2);
			*movelist++ = make_promotion_move(to - up_left, to, queen, promotion);
			*movelist++ = make_promotion_move(to - up_left, to, knight, promotion);
			*movelist++ = make_promotion_move(to - up_left, to, rook, promotion);
			*movelist++ = make_promotion_move(to - up_left, to, bishop, promotion);
		}
		while (b3)
		{
			to = pop_lsb(b3);
			*movelist++ = make_promotion_move(to - up, to, queen, promotion);
			*movelist++ = make_promotion_move(to - up, to, knight, promotion);
			*movelist++ = make_promotion_move(to - up, to, rook, promotion);
			*movelist++ = make_promotion_move(to - up, to, bishop, promotion);
		}
	}
	return movelist;
}
//-------------------------------------------//
template <color Side>
s_move* generate_castles(const Position& pos, s_move* movelist)
{
	const uint64_t occupied = pos.occupied_bb[0] | pos.occupied_bb[1];
	for (int index = Side * 2; index < Side * 2 + 2; ++index)
	{
		if ((pos.castle_rights & wqc_mask << index) == 0)
			continue;
		const int king_from = pos.kingpos[Side];
		const int rook_from = castle_rook_from[index];
		if (between_masks[king_from][rook_from] & occupied)
			continue;
		uint64_t attack_targets = castle_king_walk[index];
		bool attacked = false;
		while (!attacked && attack_targets)
		{
			const int to = pop_lsb(attack_targets);
			attacked = pos.is_attacked(to, Side ^ side_switch);
		}
		if (attacked)
			continue;
		*movelist++ = make_move(king_from, rook_from, castling);
	}
	return movelist;
}
//-------------------------------------------//
template <color Side, move_type Mt>
s_move* generate_pinned_moves(const Position& pos, s_move* movelist)
{
	constexpr color opponent = Side == white ? black : white;
	const int sq = pos.kingpos[Side];
	const uint64_t my_pieces = pos.occupied_bb[Side];
	uint64_t diag_pinned = pos.kingblockers[Side][0] & my_pieces;
	uint64_t straight_pinned = pos.kingblockers[Side][1] & my_pieces;
	int from, to;
	uint64_t pinned;
	uint64_t target_mask;
	uint64_t to_bb;
	const uint64_t occ = pos.occupied_bb[0] | pos.occupied_bb[1];
	uint64_t to_squares = 0ULL;
	if (Mt & quiet)
		to_squares |= ~occ;
	if (Mt & capture)
		to_squares |= pos.occupied_bb[opponent];
	while (diag_pinned)
	{
		from = pop_lsb(diag_pinned);
		target_mask = ray_masks[sq][from] & to_squares;
		pinned = SET_BIT(from);
		if (const auto pt = static_cast<piece_type>(pos.mailbox[from] >> 1); pt == bishop)
			movelist = generate_piece_moves<bishop>(pos, movelist, occ, target_mask, pinned);
		else if (pt == queen)
			movelist = generate_piece_moves<queen>(pos, movelist, occ, target_mask, pinned);
		else if (pt == pawn)
		{
			if (R_RANK(from, Side) == 6)
			{
				to_bb = pawn_attacks[Side][from] & pos.occupied_bb[opponent] & target_mask;
				while (to_bb)
				{
					to = pop_lsb(to_bb);
					*movelist++ = make_promotion_move(from, to, queen, promotion);
					*movelist++ = make_promotion_move(from, to, knight, promotion);
					*movelist++ = make_promotion_move(from, to, rook, promotion);
					*movelist++ = make_promotion_move(from, to, bishop, promotion);
				}
			}
			else
			{
				if (SET_BIT(pos.ep_square) & target_mask)
				{
					uint64_t from_bb = pos.piece_bb[w_pawn | static_cast<piece_code>(Side)] & pawn_attacks[opponent][pos.ep_square] & pinned;
					while (from_bb)
					{
						from = pop_lsb(from_bb);
						*movelist++ = make_move(from, pos.ep_square, enpassant);
					}
				}
				to_bb = pawn_attacks[Side][from] & pos.occupied_bb[opponent] & target_mask;
				while (to_bb)
				{
					to = pop_lsb(to_bb);
					*movelist++ = make_move(from, to, normal);
				}
			}
		}
	}
	while (straight_pinned)
	{
		from = pop_lsb(straight_pinned);
		target_mask = ray_masks[sq][from] & to_squares;
		pinned = SET_BIT(from);
		if (const auto pt = static_cast<piece_type>(pos.mailbox[from] >> 1); pt == rook)
			movelist = generate_piece_moves<rook>(pos, movelist, occ, target_mask, pinned);
		else if (pt == queen)
			movelist = generate_piece_moves<queen>(pos, movelist, occ, target_mask, pinned);
		else if (pt == pawn && FILE(sq) == FILE(from))
		{
			if (Mt & quiet)
				movelist = generate_pawn_moves<Side, quiet>(pos, movelist, pinned);
		}
	}
	return movelist;
}
//-------------------------------------------//
void Position::update_blockers()
{
	const uint64_t bishops = piece_bb[w_bishop] | piece_bb[b_bishop] | piece_bb[w_queen] | piece_bb[b_queen];
	const uint64_t rooks = piece_bb[w_rook] | piece_bb[b_rook] | piece_bb[w_queen] | piece_bb[b_queen];
	const uint64_t occ = occupied_bb[0] | occupied_bb[1];
	for (int us = white; us <= black; us++)
	{
		const int opponent = us ^ side_switch;
		const int sq = kingpos[us];
		uint64_t pinned_pieces = 0ULL;
		uint64_t diagsnipers = pseudo_attacks[bishop][sq] & bishops & occupied_bb[opponent];
		uint64_t straightsnipers = pseudo_attacks[rook][sq] & rooks & occupied_bb[opponent];
		int sniper;
		uint64_t pinned;
		while (diagsnipers)
		{
			sniper = pop_lsb(diagsnipers);
			pinned = between_masks[sq][sniper] & occ;
			if (!MORE_THAN_ONE(pinned))
			{
				pinned_pieces |= pinned;
			}
		}
		kingblockers[us][0] = pinned_pieces;
		pinned_pieces = 0ULL;
		while (straightsnipers)
		{
			sniper = pop_lsb(straightsnipers);
			pinned = between_masks[sq][sniper] & occ;
			if (!MORE_THAN_ONE(pinned))
			{
				pinned_pieces |= pinned;
			}
		}
		kingblockers[us][1] = pinned_pieces;
	}
}
//-------------------------------------------//
template <color Side, move_type Mt>
s_move* generate_pseudo_legal_moves(const Position& pos, s_move* movelist, const uint64_t from_mask)
{
	uint64_t to_squares = 0ULL;
	const uint64_t occ = pos.occupied_bb[0] | pos.occupied_bb[1];
	if (Mt & quiet)
		to_squares |= ~occ;
	if (Mt & capture)
		to_squares |= pos.occupied_bb[pos.active_side ^ side_switch];
	movelist = generate_pawn_moves<Side, Mt>(pos, movelist, from_mask);
	movelist = generate_piece_moves<knight>(pos, movelist, occ, to_squares, from_mask);
	movelist = generate_piece_moves<bishop>(pos, movelist, occ, to_squares, from_mask);
	movelist = generate_piece_moves<rook>(pos, movelist, occ, to_squares, from_mask);
	movelist = generate_piece_moves<queen>(pos, movelist, occ, to_squares, from_mask);
	movelist = generate_piece_moves<king>(pos, movelist, occ, to_squares, from_mask);
	if (Mt & quiet)
		movelist = generate_castles<Side>(pos, movelist);
	return movelist;
}
//-------------------------------------------//
template <color Side>
s_move* generate_evasions(const Position& pos, s_move* movelist)
{
	const uint64_t occ = pos.occupied_bb[0] | pos.occupied_bb[1];
	const uint64_t to_squares = ~pos.occupied_bb[Side];
	movelist = generate_piece_moves<king>(pos, movelist, occ, to_squares);
	if (ONE_OR_ZERO(pos.check_bb))
	{
		const int checker = lsb(pos.check_bb);
		uint64_t targets = between_masks[pos.kingpos[Side]][checker];
		const uint64_t no_pin = ~(pos.kingblockers[Side][0] | pos.kingblockers[Side][1]);
		uint64_t from_bb = pos.attackers_to(checker, Side) & no_pin;
		while (from_bb)
		{
			if (const int from = pop_lsb(from_bb); pos.mailbox[from] <= b_pawn && PROMOTION_RANK(checker))
			{
				*movelist++ = make_promotion_move(from, checker, queen, promotion);
				*movelist++ = make_promotion_move(from, checker, knight, promotion);
				*movelist++ = make_promotion_move(from, checker, bishop, promotion);
				*movelist++ = make_promotion_move(from, checker, rook, promotion);
			}
			else
				*movelist++ = make_move(from, checker, normal);
		}
		if (pos.ep_square && pos.ep_square == checker + SIDE2_M_SIGN(Side) * 8)
		{
			from_bb = pawn_attacks_from[Side][pos.ep_square] & pos.piece_bb[w_pawn | static_cast<piece_code>(Side)] & no_pin;
			while (from_bb)
			{
				const int from = pop_lsb(from_bb);
				*movelist++ = make_move(from, checker + SIDE2_M_SIGN(Side) * 8, enpassant);
			}
		}
		while (targets)
		{
			const int to = pop_lsb(targets);
			from_bb = pos.attackers_to(to, Side, true) & no_pin;
			while (from_bb)
			{
				if (const int from = pop_lsb(from_bb); pos.mailbox[from] <= b_pawn && PROMOTION_RANK(to))
				{
					*movelist++ = make_promotion_move(from, to, queen, promotion);
					*movelist++ = make_promotion_move(from, to, knight, promotion);
					*movelist++ = make_promotion_move(from, to, bishop, promotion);
					*movelist++ = make_promotion_move(from, to, rook, promotion);
				}
				else
					*movelist++ = make_move(from, to, normal);
			}
		}
	}
	return movelist;
}
//-------------------------------------------//
template <color Side>
s_move* generate_quiet_checks(const Position& pos, s_move* movelist)
{
	constexpr color opponent = Side == white ? black : white;
	const int my_king = pos.kingpos[Side];
	const int king_square = pos.kingpos[~Side];
	const uint64_t occ = pos.occupied_bb[0] | pos.occupied_bb[1];
	const uint64_t my_pieces = pos.occupied_bb[Side];
	uint64_t diag_pinned = pos.kingblockers[Side][0] & my_pieces;
	uint64_t straight_pinned = pos.kingblockers[Side][1] & my_pieces;
	const uint64_t not_pinned = ~(diag_pinned | straight_pinned);
	const uint64_t empty_squares = ~occ;
	const uint64_t knight_squares = pseudo_attacks[knight][king_square] & empty_squares;
	const uint64_t bishop_squares = bishop_attacks(occ, king_square) & empty_squares;
	const uint64_t rook_squares = rook_attacks(occ, king_square) & empty_squares;
	const uint64_t queen_squares = bishop_squares | rook_squares;
	movelist = generate_piece_moves<knight>(pos, movelist, occ, knight_squares, not_pinned);
	movelist = generate_piece_moves<bishop>(pos, movelist, occ, bishop_squares, not_pinned);
	movelist = generate_piece_moves<rook>(pos, movelist, occ, rook_squares, not_pinned);
	movelist = generate_piece_moves<queen>(pos, movelist, occ, queen_squares, not_pinned);
	movelist = generate_pawn_moves<Side, quiet_check>(pos, movelist, not_pinned);
	int from;
	uint64_t target_mask;
	while (diag_pinned)
	{
		from = pop_lsb(diag_pinned);
		target_mask = ray_masks[my_king][from] & empty_squares;
		if (const auto pt = static_cast<piece_type>(pos.mailbox[from] >> 1); pt == bishop)
			movelist = generate_piece_moves<bishop>(pos, movelist, occ, target_mask & bishop_squares, SET_BIT(from));
		else if (pt == queen)
			movelist = generate_piece_moves<queen>(pos, movelist, occ, target_mask & queen_squares, SET_BIT(from));
	}
	while (straight_pinned)
	{
		from = pop_lsb(straight_pinned);
		target_mask = ray_masks[my_king][from] & empty_squares;
		if (const auto pt = static_cast<piece_type>(pos.mailbox[from] >> 1); pt == rook)
			movelist = generate_piece_moves<rook>(pos, movelist, occ, target_mask & rook_squares, SET_BIT(from));
		else if (pt == queen)
			movelist = generate_piece_moves<queen>(pos, movelist, occ, target_mask & queen_squares, SET_BIT(from));
		else if (pt == pawn && FILE(my_king) == FILE(from))
		{
			movelist = generate_pawn_moves<Side, quiet_check>(pos, movelist, SET_BIT(from));
		}
	}
	const uint64_t fossils = (pos.kingblockers[opponent][0] | pos.kingblockers[opponent][1]) & my_pieces;
	uint64_t nonpinned_fossils = fossils & not_pinned;
	uint64_t pinned_fossils = fossils ^ nonpinned_fossils;
	while (nonpinned_fossils)
	{
		from = pop_lsb(nonpinned_fossils);
		const int pt = pos.mailbox[from] >> 1;
		target_mask = ~ray_masks[king_square][from] & empty_squares;
		switch (pt)
		{
		case knight:
			movelist = generate_piece_moves<knight>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case bishop:
			movelist = generate_piece_moves<bishop>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case rook:
			movelist = generate_piece_moves<rook>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case queen:
			movelist = generate_piece_moves<queen>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case king:
			movelist = generate_piece_moves<king>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case pawn:
			movelist = generate_pawn_moves<Side, quiet>(pos, movelist, SET_BIT(from), target_mask);
			break;
		default: ;
		}
	}
	while (pinned_fossils)
	{
		from = pop_lsb(pinned_fossils);
		const int pt = pos.mailbox[from] >> 1;
		target_mask = ~ray_masks[king_square][from] & empty_squares & ray_masks[my_king][from];
		switch (pt)
		{
		case knight:
			movelist = generate_piece_moves<knight>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case bishop:
			movelist = generate_piece_moves<bishop>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case rook:
			movelist = generate_piece_moves<rook>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case queen:
			movelist = generate_piece_moves<queen>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case king:
			movelist = generate_piece_moves<king>(pos, movelist, occ, target_mask, SET_BIT(from));
			break;
		case pawn:
			movelist = generate_pawn_moves<Side, quiet>(pos, movelist, SET_BIT(from), target_mask);
			break;
		default: ;
		}
	}
	return movelist;
}
//-------------------------------------------//
template <color Side, move_type Mt>
s_move* generate_legal_moves(const Position& pos, s_move* movelist)
{
	movelist = generate_pinned_moves<Side, Mt>(pos, movelist);
	const uint64_t pinned = (pos.kingblockers[Side][0] | pos.kingblockers[Side][1]) & pos.occupied_bb[Side];
	movelist = generate_pseudo_legal_moves<Side, Mt>(pos, movelist, ~pinned);
	return movelist;
}
//-------------------------------------------//
template <move_type Mt>
s_move* generate_all(const Position& pos, s_move* movelist)
{
	return pos.active_side ? generate_legal_moves<black, Mt>(pos, movelist) : generate_legal_moves<white, Mt>(pos, movelist);
}
//-------------------------------------------//
template s_move* generate_all<quiet>(const Position&, s_move*);
template s_move* generate_all<capture>(const Position&, s_move*);
template s_move* generate_all<promote>(const Position&, s_move*);
template s_move* generate_all<tactical>(const Position&, s_move*);
//-------------------------------------------//
template <>
s_move* generate_all<evasion>(const Position& pos, s_move* movelist)
{
	return pos.active_side ? generate_evasions<black>(pos, movelist) : generate_evasions<white>(pos, movelist);
}
//-------------------------------------------//
template <>
s_move* generate_all<quiet_check>(const Position& pos, s_move* movelist)
{
	return pos.active_side ? generate_quiet_checks<black>(pos, movelist) : generate_quiet_checks<white>(pos, movelist);
}
//-------------------------------------------//
template <>
s_move* generate_all<all>(const Position& pos, s_move* movelist)
{
	if (pos.check_bb)
		return generate_all<evasion>(pos, movelist);
	return pos.active_side ? generate_legal_moves<black, all>(pos, movelist) : generate_legal_moves<white, all>(pos, movelist);
}
//-------------------------------------------//
move_gen::move_gen(Position* p, const search_type type, const move hshm, const int t, const int d)
{
	pos_ = p;
	depth = d;
	threshold = t;
	if (p->check_bb)
	{
		state = evasions_init;
		hashmove = move_none;
	}
	else
	{
		if (type == normal_search)
		{
			hashmove = hshm != move_none && pos_->is_pseudo_legal(hshm) && pos_->is_legal(hshm) ? hshm : move_none;
			state = hashmove_state;
		}
		else
		{
			hashmove = hshm != move_none && pos_->is_pseudo_legal(hshm) && pos_->is_legal(hshm) && pos_->is_tactical(hshm) ? hshm : move_none;
			state = type == quiescence_search ? q_hashmove : probcut_hashmove;
		}
		if (hashmove == move_none)
		{
			state++;
		}
	}
	const int prev_to = to_sq(p->move_stack[p->history_index]);
	counter_move = p->my_thread->counter_move_table[p->mailbox[prev_to]][prev_to];
}
//-------------------------------------------//
inline int quiet_score(const Position* pos, const searchinfo* info, const move m)
{
	const int from = from_sq(m);
	const int to = to_sq(m);
	const piece_code pc = pos->mailbox[from];
	return pos->my_thread->history_table[pos->active_side][from][to] +
		(*(info - 1)->counter_move_history)[pc][to] +
		(*(info - 2)->counter_move_history)[pc][to];
}
//-------------------------------------------//
inline int mvvlva_score(const Position* pos, const move m)
{
	return mvvlva[pos->mailbox[to_sq(m)]][pos->mailbox[from_sq(m)]];
}
//-------------------------------------------//
void move_gen::score_moves(const searchinfo* info, const score_type type) const
{
	for (auto& [code, value] : *this)
	{
		if (type == score_capture)
		{
			value = mvvlva_score(pos_, code);
		}
		else if (type == score_quiet)
		{
			value = quiet_score(pos_, info, code);
		}
		else
		{
			if (pos_->is_capture(code))
				value = mvvlva_score(pos_, code);
			else
				value = quiet_score(pos_, info, code) - (1 << 20);
		}
		if (type_of(code) == promotion)
			value += mvvlva[make_piece(pos_->active_side, promotion_type(code))][pos_->mailbox[from_sq(code)]];
	}
}
//-------------------------------------------//
template <pick_type Type>
move move_gen::select_move()
{
	if (Type == best)
		std::swap(*curr, *std::max_element(curr, end_moves));
	return *curr++;
}
//-------------------------------------------//
void insertion_sort(s_move* head, const s_move* tail)
{
	for (s_move* i = head + 1; i < tail; i++)
	{
		const s_move tmp = *i;
		s_move* j;
		for (j = i; j != head && *(j - 1) < tmp; --j)
		{
			*j = *(j - 1);
		}
		*j = tmp;
	}
}
//-------------------------------------------//
move move_gen::next_move(const searchinfo* info, const bool skip_quiets)
{
	move m;
	switch (state)
	{
	case hashmove_state:
		++state;
		return hashmove;
	case tactical_init:
		curr = end_bad_captures = move_list;
		end_moves = generate_all<tactical>(*pos_, curr);
		score_moves(info, score_capture);
		++state;
		[[fallthrough]];
	case tactical_state:
		while (curr < end_moves)
		{
			m = select_move<best>();
			if (m == hashmove)
				continue;
			if (pos_->see(m, 0))
				return m;
			*end_bad_captures++ = m;
		}
		++state;
		m = info->killers[0];
		if (m != move_none && m != hashmove && !pos_->is_tactical(m) && pos_->is_pseudo_legal(m) && pos_->is_legal(m))
		{
			return m;
		}
		[[fallthrough]];
	case killer_move_2:
		++state;
		m = info->killers[1];
		if (m != move_none && m != hashmove && !pos_->is_tactical(m) && pos_->is_pseudo_legal(m) && pos_->is_legal(m))
		{
			return m;
		}
		[[fallthrough]];
	case countermove:
		++state;
		m = counter_move;
		if (m != move_none
			&& m != hashmove
			&& m != info->killers[0]
			&& m != info->killers[1]
			&& !pos_->is_tactical(m)
			&& pos_->is_pseudo_legal(m)
			&& pos_->is_legal(m))
		{
			return m;
		}
		[[fallthrough]];
	case quiets_init:
		if (!skip_quiets)
		{
			curr = end_bad_captures;
			end_moves = generate_all<quiet>(*pos_, curr);
			score_moves(info, score_quiet);
			insertion_sort(curr, end_moves);
		}
		++state;
		[[fallthrough]];
	case quiet_state:
		if (!skip_quiets)
		{
			while (curr < end_moves)
			{
				m = select_move<next>();
				if (m != hashmove
					&& m != info->killers[0]
					&& m != info->killers[1]
					&& m != counter_move)
				{
					return m;
				}
			}
		}
		++state;
		curr = move_list;
		end_moves = end_bad_captures;
		[[fallthrough]];
	case bad_tactical_state:
		while (curr < end_moves)
		{
			m = select_move<next>();
			if (m != hashmove)
				return m;
		}
		break;
	case evasions_init:
		curr = move_list;
		end_moves = generate_all<evasion>(*pos_, curr);
		score_moves(info, score_evasion);
		++state;
		[[fallthrough]];
	case evasions_state:
		while (curr < end_moves)
		{
			return select_move<best>();
		}
		break;
	case q_hashmove:
		++state;
		return hashmove;
	case q_captures_init:
		curr = end_bad_captures = move_list;
		end_moves = generate_all<tactical>(*pos_, curr);
		score_moves(info, score_capture);
		++state;
		[[fallthrough]];
	case q_captures:
		while (curr < end_moves)
		{
			m = select_move<best>();
			if (m != hashmove)
			{
				return m;
			}
		}
		if (depth != 0)
			break;
		++state;
		[[fallthrough]];
	case q_checks_init:
		curr = move_list;
		end_moves = generate_all<quiet_check>(*pos_, curr);
		++state;
		[[fallthrough]];
	case q_checks:
		while (curr < end_moves)
		{
			m = select_move<next>();
			if (m != hashmove)
				return m;
		}
		break;
	case probcut_hashmove:
		++state;
		return hashmove;
	case probcut_captures_init:
		curr = end_bad_captures = move_list;
		end_moves = generate_all<tactical>(*pos_, curr);
		score_moves(info, score_capture);
		++state;
		[[fallthrough]];
	case probcut_captures:
		while (curr < end_moves)
		{
			m = select_move<best>();
			if (m != hashmove && pos_->see(m, threshold))
				return m;
		}
		break;
	default: ;
	}
	return move_none;
}
//-------------------------------------------//