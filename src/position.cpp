//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <iomanip>
#include <string>
#include <vector>
#include "board.h"
#include "eval.h"
#include "move.h"
#include "position.h"
#include <iostream>
#include "pst.h"
#include "zobrist.h"
#include "thread.h"
//-------------------------------------------//
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#pragma warning(disable: 4244)
#pragma warning(disable: 5054)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
//-------------------------------------------//
uint64_t Position::bad_squares() const
{
	const int side = active_side;
	const int opponent = side ^ side_switch;
	const uint64_t occ = (occupied_bb[0] | occupied_bb[1]) ^ SET_BIT(kingpos[side]);
	int from;
	uint64_t out = PAWN_ATTACKS(opponent, piece_bb[w_pawn | opponent]);
	uint64_t knights = piece_bb[(w_knight | opponent)];
	while (knights)
	{
		from = pop_lsb(knights);
		out |= pseudo_attacks[knight][from];
	}
	out |= pseudo_attacks[king][kingpos[opponent]];
	uint64_t bishops_queens = piece_bb[w_bishop | opponent] | piece_bb[w_queen | opponent];
	while (bishops_queens)
	{
		from = pop_lsb(bishops_queens);
		out |= bishop_attacks(occ, from);
	}
	uint64_t rooks_queens = piece_bb[w_rook | opponent] | piece_bb[w_queen | opponent];
	while (rooks_queens)
	{
		from = pop_lsb(rooks_queens);
		out |= rook_attacks(occ, from);
	}
	return out;
}
//-------------------------------------------//
Position* import_fen(const char* fen, const int thread_id)
{
	const auto t = static_cast<searchthread*>(get_thread(thread_id));
	Position* p = &t->position;
	p->read_fen(fen);
	p->my_thread = t;
	return p;
}
//-------------------------------------------//
bool Position::advanced_pawn_push(const move m) const
{
	const int to = to_sq(m);
	return mailbox[from_sq(m)] <= b_pawn &&
		(R_RANK(to, active_side) == 6 ||
			R_RANK(to, active_side) == 5 && mailbox[PAWN_PUSH_INDEX(active_side, to)] == no_piece
			);
}
//-------------------------------------------//
uint64_t Position::all_attackers_to(const int sq, const uint64_t occ) const
{
	return (pawn_attacks_from[white][sq] & (piece_bb[w_pawn])) | (pawn_attacks_from[black][sq] & (piece_bb[b_pawn]))
		| (pseudo_attacks[knight][sq] & (piece_bb[w_knight] | piece_bb[b_knight]))
		| (bishop_attacks(occ, sq) & (piece_bb[w_bishop] | piece_bb[w_queen] | piece_bb[b_bishop] | piece_bb[b_queen]))
		| (rook_attacks(occ, sq) & (piece_bb[w_rook] | piece_bb[w_queen] | piece_bb[b_rook] | piece_bb[b_queen]));
}
//-------------------------------------------//
uint64_t Position::attackers_to(const int sq, const int side, const bool free) const
{
	const uint64_t occ = occupied_bb[0] | occupied_bb[1];
	return ((free ? (pawn_pushesfrom[side][sq] | (pawn_2_pushesfrom[side][sq]
		& PAWN_PUSH(side ^ side_switch, ~occ))) : (pawn_attacks_from[side][sq])) & piece_bb[w_pawn | side])
		| (pseudo_attacks[knight][sq] & piece_bb[w_knight | side])
		| (bishop_attacks(occ, sq) & (piece_bb[w_bishop | side] | piece_bb[w_queen | side]))
		| (rook_attacks(occ, sq) & (piece_bb[w_rook | side] | piece_bb[w_queen | side]));
}
//-------------------------------------------//
uint64_t Position::attackers_to(const int sq, const int side, const uint64_t occ) const
{
	return (pawn_attacks_from[side][sq] & piece_bb[w_pawn | side])
		| (pseudo_attacks[knight][sq] & piece_bb[w_knight | side])
		| (bishop_attacks(occ, sq) & (piece_bb[w_bishop | side] | piece_bb[w_queen | side]))
		| (rook_attacks(occ, sq) & (piece_bb[w_rook | side] | piece_bb[w_queen | side]));
}
//-------------------------------------------//
void Position::do_move(const move m)
{
	memcpy(&history_stack[history_index++], &key, sizeof(state_history));
	move_stack[history_index] = m;
	halfmove_clock++;
	const special_type type = type_of(m);
	captured_piece = no_piece;
	int eptnew = 0;
	const uint8_t old_castle = castle_rights;
	const color side = active_side;
	const color opponent = ~side;
	if (type == castling)
	{
		const int king_from = from_sq(m);
		const int rook_from = to_sq(m);
		const int castle_type = 2 * side + (rook_from > king_from ? 1 : 0);
		const int king_to = castle_king_to[castle_type];
		const int rook_to = castle_rook_to[castle_type];
		const auto kingpc = static_cast<piece_code>(w_king | side);
		const auto rookpc = static_cast<piece_code>(w_rook | side);
		kingpos[side] = king_to;
		move_piece(king_from, king_to, kingpc);
		move_piece(rook_from, rook_to, rookpc);
		pawnhash ^= zb.piece_keys[king_from << 4 | kingpc] ^ zb.piece_keys[king_to << 4 | kingpc];
		castle_rights &= side ? ~(bqc_mask | bkc_mask) : ~(wqc_mask | wkc_mask);
	}
	else
	{
		const int from = from_sq(m);
		const int to = to_sq(m);
		const piece_code pc = mailbox[from];
		const auto pt = static_cast<piece_type>(pc >> 1);
		captured_piece = type == enpassant ? make_piece(opponent, pawn) : mailbox[to];
		if (type == promotion)
		{
			const piece_code promote = make_piece(side, promotion_type(m));
			set_piece_at(to, promote);
			remove_piece_at(from, pc);
			pawnhash ^= zb.piece_keys[from << 4 | pc];
			materialhash ^= zb.piece_keys[(piece_count[pc] << 4) | pc] ^ zb.piece_keys[((piece_count[promote] - 1) << 4) | promote];
		}
		else
		{
			move_piece(from, to, pc);
			if (pt == pawn)
			{
				if ((to ^ from) == 16 && ept_helper[to] & piece_bb[w_pawn | opponent])
					eptnew = (to + from) / 2;
				pawnhash ^= zb.piece_keys[from << 4 | pc] ^ zb.piece_keys[to << 4 | pc];
				halfmove_clock = 0;
			}
			else if (pt == king)
			{
				kingpos[side] = to;
				pawnhash ^= zb.piece_keys[from << 4 | pc] ^ zb.piece_keys[to << 4 | pc];
			}
		}
		if (captured_piece != no_piece)
		{
			if (captured_piece >> 1 == pawn)
			{
				if (type == enpassant)
				{
					const int capturesquare = (from & 0x38) | (to & 0x07);
					remove_piece_at(capturesquare, captured_piece);
					pawnhash ^= zb.piece_keys[capturesquare << 4 | captured_piece];
					pawnhash ^= zb.piece_keys[from << 4 | pc] ^ zb.piece_keys[to << 4 | pc];
				}
				else
					pawnhash ^= zb.piece_keys[to << 4 | captured_piece];
			}
			halfmove_clock = 0;
			materialhash ^= zb.piece_keys[piece_count[captured_piece] << 4 | captured_piece];
		}
		castle_rights &= castling_rights[from] & castling_rights[to];
	}
	active_side = ~active_side;
	if (!active_side)
		fullmove_clock++;
	key ^= zb.active_side;
	key ^= zb.ep_squares[ep_square];
	ep_square = eptnew;
	key ^= zb.ep_squares[ep_square];
	key ^= zb.castle[old_castle] ^ zb.castle[castle_rights];
	check_bb = attackers_to(kingpos[active_side], active_side ^ side_switch);
	update_blockers();
}
//-------------------------------------------//
void Position::undo_move(const move m)
{
	const special_type type = type_of(m);
	active_side = ~active_side;
	const color side = active_side;
	const color opponent = ~side;
	if (type == castling)
	{
		const int king_from = from_sq(m);
		const int rook_from = to_sq(m);
		const int castle_type = 2 * side + (rook_from > king_from ? 1 : 0);
		const int king_to = castle_king_to[castle_type];
		const int rook_to = castle_rook_to[castle_type];
		const auto kingpc = static_cast<piece_code>(w_king | side);
		const auto rookpc = static_cast<piece_code>(w_rook | side);
		move_piece(king_to, king_from, kingpc);
		move_piece(rook_to, rook_from, rookpc);
		kingpos[side] = king_from;
	}
	else
	{
		const int from = from_sq(m);
		const int to = to_sq(m);
		const piece_code pc = mailbox[to];
		const auto pt = static_cast<piece_type>(pc >> 1);
		const piece_code capture = captured_piece;
		if (type == promotion)
		{
			remove_piece_at(to, static_cast<piece_code>(promotion_type(m) << 1 | side));
			set_piece_at(from, static_cast<piece_code>(w_pawn | side));
		}
		else
		{
			move_piece(to, from, pc);
			if (pt == king)
				kingpos[side] = from;
		}
		if (capture != no_piece)
		{
			if (type == enpassant)
			{
				const int capturesquare = (from & 0x38) | (to & 0x07);
				set_piece_at(capturesquare, make_piece(opponent, pawn));
			}
			else
			{
				set_piece_at(to, capture);
			}
		}
	}
	history_index--;
	memcpy(&key, &history_stack[history_index], sizeof(state_history));
}
//-------------------------------------------//
void Position::do_null_move()
{
	memcpy(&history_stack[history_index++], &key, sizeof(state_history));
	move_stack[history_index] = move_null;
	halfmove_clock++;
	constexpr int eptnew = 0;
	key ^= zb.ep_squares[ep_square];
	ep_square = eptnew;
	key ^= zb.ep_squares[ep_square];
	active_side = ~active_side;
	key ^= zb.active_side;
}
//-------------------------------------------//
void Position::undo_null_move()
{
	history_index--;
	memcpy(&key, &history_stack[history_index], sizeof(state_history));
	active_side = ~active_side;
}
//-------------------------------------------//
uint64_t Position::get_attack_set(const int sq, const uint64_t occ) const
{
	switch (mailbox[sq] >> 1)
	{
	case knight:
		return pseudo_attacks[knight][sq];
	case king:
		return pseudo_attacks[king][sq];
	case bishop:
		return bishop_attacks(occ, sq);
	case rook:
		return rook_attacks(occ, sq);
	case queen:
		return bishop_attacks(occ, sq) | rook_attacks(occ, sq);
	default:;
	}
	return 0;
}
//-------------------------------------------//
bool Position::gives_check(const move m)
{
	const color opponent = ~active_side;
	const int opponent_king_square = kingpos[opponent];
	const uint64_t opponent_king = SET_BIT(opponent_king_square);
	const special_type mtype = type_of(m);
	const int from = from_sq(m);
	const int to = to_sq(m);
	const piece_code pc = mailbox[from];
	const auto type = static_cast<piece_type>(pc >> 1);
	const uint64_t occ = occupied_bb[0] | occupied_bb[1];
	switch (type)
	{
	case pawn:
		if (pawn_attacks[active_side][to] & opponent_king)
			return true;
		if (!(opponent_king & ray_masks[from][to]) && (kingblockers[opponent][0] | kingblockers[opponent][1]) & SET_BIT(from))
			return true;
		if (mtype == promotion)
		{
			switch (promotion_type(m))
			{
			case queen:
				return opponent_king & (rook_attacks(occ, to) | bishop_attacks(occ, to));
			case rook:
				return opponent_king & rook_attacks(occ, to);
			case bishop:
				return opponent_king & bishop_attacks(occ, to);
			case knight:
				return opponent_king & pseudo_attacks[knight][to];
			case blanktype: break;
			case pawn: break;
			case king: break;
			default:
				return false;
			}
		}
		if (mtype == enpassant)
		{
			do_move(m);
			if (check_bb)
			{
				undo_move(m);
				return true;
			}
			undo_move(m);
		}
		break;
	case king:
		if (!(opponent_king & ray_masks[from][to]) && (kingblockers[opponent][0] | kingblockers[opponent][1]) & SET_BIT(from))
			return true;
		if (mtype == castling)
		{
			do_move(m);
			if (check_bb)
			{
				undo_move(m);
				return true;
			}
			undo_move(m);
		}
		break;
	case queen:
		if (opponent_king & (rook_attacks(occ, to) | bishop_attacks(occ, to)))
			return true;
		if (!(opponent_king & ray_masks[from][to]) && (kingblockers[opponent][0] | kingblockers[opponent][1]) & SET_BIT(from))
			return true;
		break;
	case rook:
		if (opponent_king & rook_attacks(occ, to))
			return true;
		if (!(opponent_king & ray_masks[from][to]) && (kingblockers[opponent][0] | kingblockers[opponent][1]) & SET_BIT(from))
			return true;
		break;
	case bishop:
		if (opponent_king & bishop_attacks(occ, to))
			return true;
		if (!(opponent_king & ray_masks[from][to]) && (kingblockers[opponent][0] | kingblockers[opponent][1]) & SET_BIT(from))
			return true;
		break;
	case knight:
		if (opponent_king & pseudo_attacks[knight][to])
			return true;
		if (!(opponent_king & ray_masks[from][to]) && (kingblockers[opponent][0] | kingblockers[opponent][1]) & SET_BIT(from))
			return true;
		break;
	case blanktype: break;
	default:
		return false;
	}
	return false;
}
//-------------------------------------------//
void Position::init()
{
	std::cout << engine_name << " " << version << " by " << author << std::endl;
	init_boards();
	init_threads();
	init_values();
}
//-------------------------------------------//
bool Position::is_attacked(const int sq, const int side) const
{
	const uint64_t occ = occupied_bb[0] | occupied_bb[1];
	return pawn_attacks_from[side][sq] & piece_bb[w_pawn | side]
		|| pseudo_attacks[knight][sq] & piece_bb[w_knight | side]
		|| pseudo_attacks[king][sq] & piece_bb[w_king | side]
		|| bishop_attacks(occ, sq) & (piece_bb[w_bishop | side] | piece_bb[w_queen | side])
		|| rook_attacks(occ, sq) & (piece_bb[w_rook | side] | piece_bb[w_queen | side]);
}
//-------------------------------------------//
bool Position::is_capture(const move m) const
{
	return mailbox[to_sq(m)];
}
//-------------------------------------------//
bool Position::is_tactical(const move m) const
{
	return mailbox[to_sq(m)] || type_of(m) == promotion;
}
//-------------------------------------------//
bool Position::is_pseudo_legal(const move m) const
{
	const color side = active_side;
	const int from = from_sq(m);
	const int to = to_sq(m);
	const piece_code pc = mailbox[from_sq(m)];
	if (type_of(m) != normal)
		return move_list<all>(*this).contains(m);
	if (promotion_type(m) - knight != blanktype)
		return false;
	if (pc < w_pawn || (pc & side_switch) != side)
		return false;
	if (occupied_bb[side] & SET_BIT(to))
		return false;
	if (pc >> 1 == pawn)
	{
		if ((rank8_bb | rank1_bb) & SET_BIT(to))
			return false;
		if (!(pawn_attacks[side][from] & occupied_bb[~side] & SET_BIT(to))
			&& !(from + pawn_push(side) == to && !mailbox[to])
			&& !(from + 2 * pawn_push(side) == to
				&& R_RANK(from, side) == 1
				&& !mailbox[to]
				&& !mailbox[(to - pawn_push(side))]))
			return false;
	}
	else if (!(get_attack_set(from, occupied_bb[0] | occupied_bb[1]) & SET_BIT(to)))
		return false;
	if (check_bb)
	{
		if (pc >> 1 != king)
		{
			if (!ONE_OR_ZERO(check_bb))
				return false;
			if (!((between_masks[lsb(check_bb)][kingpos[side]] | check_bb) & SET_BIT(to)))
				return false;
		}
		else if (attackers_to(to, ~side, (occupied_bb[0] | occupied_bb[1]) ^ SET_BIT(from)))
			return false;
	}
	return true;
}
//-------------------------------------------//
bool Position::is_legal(const move m) const
{
	const color side = active_side;
	const int from = from_sq(m);
	int to = to_sq(m);
	const int king_sq = kingpos[side];
	if (type_of(m) == enpassant)
	{
		const int captured = to - pawn_push(side);
		const uint64_t occ = (occupied_bb[0] | occupied_bb[1]) ^ SET_BIT(from) ^ SET_BIT(to) ^ SET_BIT(captured);
		const uint64_t bishops = piece_bb[make_piece(~side, bishop)] | piece_bb[make_piece(~side, queen)];
		const uint64_t rooks = piece_bb[make_piece(~side, rook)] | piece_bb[make_piece(~side, queen)];
		return !(rook_attacks(occ, king_sq) & rooks) && !(bishop_attacks(occ, king_sq) & bishops);
	}
	if (type_of(m) == castling)
	{
		const int castle_type = 2 * side + (to > from ? 1 : 0);
		to = castle_king_to[castle_type];
		uint64_t attack_targets = castle_king_walk[castle_type];
		while (attack_targets)
		{
			if (is_attacked(pop_lsb(attack_targets), ~side))
				return false;
		}
	}
	if (mailbox[from] >= w_king)
	{
		return !is_attacked(to, ~side);
	}
	return !(SET_BIT(from) & (kingblockers[side][0] | kingblockers[side][1]))
		|| SET_BIT(from) & ray_masks[to][king_sq];
}
//-------------------------------------------//
void Position::move_piece(const int from, const int to, const piece_code pc)
{
	remove_piece_at(from, pc);
	set_piece_at(to, pc);
}
//-------------------------------------------//
bool Position::pawn_on7_th() const
{
	const uint64_t pawns = piece_bb[w_pawn | active_side];
	return pawns & (active_side ? rank2_bb : rank7_bb);
}
//-------------------------------------------//
void Position::read_fen(const char* fen)
{
	const std::vector<std::string> parts = split_string(fen);
	for (int i = 0; i < 14; i++)
	{
		piece_bb[i] = 0ULL;
		piece_count[i] = 0;
	}
	for (auto& i : mailbox)
		i = no_piece;
	for (unsigned long long& i : occupied_bb)
		i = 0ULL;
	std::string temp = parts[0];
	int rank = 7;
	int file = 0;
	non_pawn[0] = non_pawn[1] = 0;
	halfmove_clock = 0;
	fullmove_clock = 0;
	for (const char c : temp)
	{
		piece_code p = {};
		int num = 1;
		const int index = INDEX(rank, file);
		switch (c)
		{
		case 'k':
			p = b_king;
			kingpos[1] = index;
			break;
		case 'q':
			p = b_queen;
			break;
		case 'r':
			p = b_rook;
			break;
		case 'b':
			p = b_bishop;
			break;
		case 'n':
			p = b_knight;
			break;
		case 'p':
			p = b_pawn;
			break;
		case 'K':
			p = w_king;
			kingpos[0] = index;
			break;
		case 'Q':
			p = w_queen;
			break;
		case 'R':
			p = w_rook;
			break;
		case 'B':
			p = w_bishop;
			break;
		case 'N':
			p = w_knight;
			break;
		case 'P':
			p = w_pawn;
			break;
		case '/':
			rank--;
			num = 0;
			file = 0;
			break;
		default:
			num = 0;
			file += c - '0';
			break;
		}
		if (num)
		{
			set_piece_at(index, p);
			file++;
		}
	}
	castle_rights = 0;
	active_side = white;
	if (parts[1] == "b")
		active_side = black;
	temp = parts[2];
	const std::string castles = "QKqk";
	for (const char c : temp)
	{
		if (size_t castleindex; (castleindex = castles.find(c)) != std::string::npos)
			castle_rights |= wqc_mask << castleindex;
	}
	ep_square = 0;
	temp = parts[3];
	if (temp.length() == 2)
		ep_square = algebraic_to_index(temp);
	if (parts.size() > 4)
		halfmove_clock = stoi(parts[4]);
	if (parts.size() > 5)
		fullmove_clock = stoi(parts[5]);
	key = zb.get_hash(this);
	pawnhash = zb.get_pawn_hash(this);
	materialhash = 0ULL;
	for (int i = w_pawn; i <= b_king; i++)
		for (int j = 0; j < piece_count[i]; j++)
			materialhash ^= zb.piece_keys[j << 4 | i];
	history_index = 0;
	captured_piece = no_piece;
	check_bb = attackers_to(kingpos[active_side], active_side ^ side_switch);
	update_blockers();
}
//-------------------------------------------//
void Position::set_piece_at(const int sq, const piece_code pc)
{
	const int side = pc & 0x01;
	if (const piece_code current = mailbox[sq])
		remove_piece_at(sq, current);
	mailbox[sq] = pc;
	piece_bb[pc] |= square_masks[sq];
	occupied_bb[side] |= square_masks[sq];
	key ^= zb.piece_keys[sq << 4 | pc];
	non_pawn[side] += non_pawn_value[pc];
	piece_count[pc]++;
}
//-------------------------------------------//
void Position::remove_piece_at(const int sq, const piece_code pc)
{
	const int side = pc & 0x01;
	mailbox[sq] = no_piece;
	piece_bb[pc] ^= square_masks[sq];
	occupied_bb[side] ^= square_masks[sq];
	key ^= zb.piece_keys[sq << 4 | pc];
	non_pawn[side] -= non_pawn_value[pc];
	piece_count[pc]--;
}
//-------------------------------------------//
bool Position::see(const move m, const int threshold) const
{
	const int from = from_sq(m);
	const int to = to_sq(m);
	int value = piece_values[mg][mailbox[to]] + (type_of(m) == promotion ? piece_values[mg][promotion_type(m) << 1] - pawn_mg : 0) - threshold;
	if (value < 0)
		return false;
	const int next_piece = type_of(m) == promotion ? promotion_type(m) << 1 : mailbox[from];
	value -= piece_values[mg][next_piece];
	if (value >= 0)
		return true;
	uint64_t occ = (occupied_bb[0] | occupied_bb[1]) ^ (SET_BIT(from) | SET_BIT(to));
	const uint64_t rooks = piece_bb[w_rook] | piece_bb[b_rook] | piece_bb[w_queen] | piece_bb[b_queen];
	const uint64_t bishops = piece_bb[w_bishop] | piece_bb[b_bishop] | piece_bb[w_queen] | piece_bb[b_queen];
	uint64_t attackers = all_attackers_to(to, occ) & occ;
	color active_side_temp = ~active_side;
	while (true)
	{
		const uint64_t my_attackers = attackers & occupied_bb[active_side_temp];
		if (!my_attackers)
			break;
		const int sq = smallest_attacker(my_attackers, active_side_temp);
		occ ^= SET_BIT(sq);
		const int attacker_type = mailbox[sq] >> 1;
		if (attacker_type == pawn || attacker_type == bishop || attacker_type == queen)
		{
			attackers |= bishop_attacks(occ, to) & bishops;
		}
		if (attacker_type == rook || attacker_type == queen)
		{
			attackers |= rook_attacks(occ, to) & rooks;
		}
		attackers &= occ;
		active_side_temp = ~active_side_temp;
		value = -value - 1 - piece_values[mg][mailbox[sq]];
		if (value >= 0)
			break;
	}
	return active_side_temp ^ active_side;
}
//-------------------------------------------//
int Position::smallest_attacker(const uint64_t attackers, const color col) const
{
	for (int piece = w_pawn + col; piece <= b_king; piece += 2)
	{
		if (const uint64_t intersection = piece_bb[piece] & attackers)
			return lsb(intersection);
	}
	return 0;
}
//-------------------------------------------//
Position* start_position()
{
	Position* p = &main_thread.position;
	p->read_fen(start_fen);
	p->my_thread = &main_thread;
	return p;
}
//-------------------------------------------//