//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "position.h"
#include "board.h"
#include "move.h"
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
		uint64_t diag_snipers = pseudo_attacks[bishop][sq] & bishops & occupied_bb[opponent];
		uint64_t straight_snipers = pseudo_attacks[rook][sq] & rooks & occupied_bb[opponent];
		int sniper;
		uint64_t pinned;
		while (diag_snipers)
		{
			sniper = pop_lsb(diag_snipers);
			pinned = between_masks[sq][sniper] & occ;
			if (!MORE_THAN_ONE(pinned))
			{
				pinned_pieces |= pinned;
			}
		}
		kingblockers[us][0] = pinned_pieces;
		pinned_pieces = 0ULL;
		while (straight_snipers)
		{
			sniper = pop_lsb(straight_snipers);
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
	uint64_t non_pinned_fossils = fossils & not_pinned;
	uint64_t pinned_fossils = fossils ^ non_pinned_fossils;
	while (non_pinned_fossils)
	{
		from = pop_lsb(non_pinned_fossils);
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

