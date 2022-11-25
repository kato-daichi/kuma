//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <iomanip>
#include <sstream>
#include "position.h"
#include "board.h"
#include "eval.h"
#include "zobrist.h"
//-------------------------------------------//
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#pragma warning(disable: 4244)
#pragma warning(disable: 5054)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
//-------------------------------------------//
constexpr int lazy_threshold = 1300;
//-------------------------------------------//
int evaluate(const Position& p)
{
	return Eval(p).value();
}
//-------------------------------------------//
template <color Side>
Score Eval::evaluate_passers() const
{
	constexpr color opponent = ~Side;
	constexpr int up = Side == white ? north : south;
	Score out = S(0, 0);
	uint64_t passers = pawnhashe_->passed_pawns[Side];
	const uint64_t my_rooks = pos_.piece_bb[make_piece(Side, rook)];
	const uint64_t opponent_rooks = pos_.piece_bb[make_piece(opponent, rook)];
	while (passers)
	{
		const int sq = pop_lsb(passers);
		const int r = RRANK(sq, Side);
		out += passed_rank_bonus[r];
		const bool blocked = square_masks[PAWNPUSHINDEX(Side, sq)] & (pos_.occupied_bb[0] | pos_.occupied_bb[1]);
		const bool unsafe = square_masks[PAWNPUSHINDEX(Side, sq)] & attacked_squares_[opponent];
		out += passed_unsafe_bonus[unsafe][r];
		out += passed_blocked_bonus[blocked][r];

		out += passed_friendly_distance[r] * square_distance[pos_.kingpos[Side]][sq + up];
		out += passed_enemy_distance[r] * square_distance[pos_.kingpos[opponent]][sq + up];

		if (r >= 3)
		{
			if (my_rooks & pawn_blocker_masks[opponent][sq])
			{
				out += tarrasch_rule_friendly[r];
			}

			if (opponent_rooks & pawn_blocker_masks[opponent][sq])
			{
				out -= tarrasch_rule_enemy;
			}
		}
	}
	return out;
}
//-------------------------------------------//
void Eval::evaluate_pawns() const
{
	if (pawnhashe_->pawn_hash != pos_.pawnhash)
	{
		pawnhashe_->pawn_hash = pos_.pawnhash;
		pawnhashe_->kingpos[white] = pos_.kingpos[white];
		pawnhashe_->kingpos[black] = pos_.kingpos[black];
		pawnhashe_->castling = pos_.castle_rights;

		evaluate_pawn_structure<white>();
		evaluate_pawn_structure<black>();
		pawn_shelter_castling<white>();
		pawn_shelter_castling<black>();
	}
	else
	{
		if (pos_.kingpos[white] != pawnhashe_->kingpos[white]
			|| (pos_.castle_rights & whitecastle) != (pawnhashe_->castling & whitecastle))
			pawn_shelter_castling<white>();

		if (pos_.kingpos[black] != pawnhashe_->kingpos[black]
			|| (pos_.castle_rights & blackcastle) != (pawnhashe_->castling & blackcastle))
			pawn_shelter_castling<black>();

		pawnhashe_->kingpos[white] = pos_.kingpos[white];
		pawnhashe_->kingpos[black] = pos_.kingpos[black];
		pawnhashe_->castling = pos_.castle_rights;
	}
}
//-------------------------------------------//
template <color Side>
void Eval::evaluate_pawn_structure() const
{
	Score pawn_structure = S(0, 0);
	const color us = Side ? black : white;
	const color them = ~us;
	uint64_t my_pawns = pos_.piece_bb[make_piece(us, pawn)];
	const uint64_t their_pawns = pos_.piece_bb[make_piece(them, pawn)];
	pawnhashe_->passed_pawns[us] = 0ULL;
	pawnhashe_->attack_spans[us] = 0ULL;
	pawnhashe_->semiopen_files[us] = 0xFF;
	while (my_pawns)
	{
		const int sq = pop_lsb(my_pawns);
		pawnhashe_->attack_spans[us] |= passed_pawn_masks[us][sq] ^ pawn_blocker_masks[us][sq];

		const int fwd1 = PAWNPUSHINDEX(us, sq);
		const int fwd2 = PAWNPUSHINDEX(us, fwd1);
		const bool opposed = (pawn_blocker_masks[us][sq] & their_pawns) != 0ULL;
		const bool isolated = (neighbor_masks[sq] & my_pawns) == 0ULL;
		const bool doubled = (pawn_blocker_masks[us][sq] & my_pawns) != 0ULL;
		const uint64_t forward_threats = pawn_attacks[us][fwd1] & their_pawns;
		const bool backward = !(passed_pawn_masks[them][sq] & my_pawns) && forward_threats;
		const uint64_t phalanx = phalanx_masks[sq] & my_pawns;
		const uint64_t supported = pawn_attacks[them][sq] & my_pawns;

		if (isolated)
		{
			pawn_structure -= BITSET(sq) & file_ah ? isolated_penalty_ah[opposed] : isolated_penalty[opposed];
		}

		if (backward)
		{
			pawn_structure -= backward_penalty[opposed];
		}

		if (doubled)
		{
			pawn_structure -= BITSET(sq) & attacked_squares_[make_piece(Side, pawn)] ? doubled_penalty[opposed] : doubled_penalty_undefended[opposed];
		}

		if (isolated && doubled)
		{
			pawn_structure -= BITSET(sq) & file_ah ? isolated_doubled_penalty_ah[opposed] : isolated_doubled_penalty[opposed];
		}

		if (phalanx | supported)
		{
			pawn_structure += connected_bonus[opposed][static_cast<bool>(phalanx)][RRANK(sq, us)];
		}

		if (!opposed &&
			(!(passed_pawn_masks[us][fwd1] & their_pawns)
				|| passed_pawn_masks[us][fwd2] & their_pawns
				&& popcnt(phalanx) >= popcnt(forward_threats)
				))
			pawnhashe_->passed_pawns[us] |= BITSET(sq);
		pawnhashe_->semiopen_files[us] &= ~(1 << FILE(sq));
	}

	pawnhashe_->scores[us] = pawn_structure;
}
//-------------------------------------------//
template <color Side, piece_type Type>
Score Eval::evaluate_piece()
{
	Score out = S(0, 0);
	constexpr piece_code pc = make_piece(Side, Type);
	uint64_t pieces = pos_.piece_bb[pc];
	constexpr uint64_t outpostranks = outpost_ranks[Side];
	constexpr color opponent = ~Side;
	const int king_square = pos_.kingpos[Side];

	while (pieces)
	{
		const int sq = pop_lsb(pieces);
		uint64_t attacks = Type == bishop
			? pos_.get_attack_set(sq, (pos_.occupied_bb[0] | pos_.occupied_bb[1]) ^ pos_.piece_bb[wqueen] ^ pos_.piece_bb[bqueen])
			: Type == rook
			? pos_.get_attack_set(sq, (pos_.occupied_bb[0] | pos_.occupied_bb[1]) ^ pos_.piece_bb[wqueen] ^ pos_.piece_bb[bqueen] ^ pos_.piece_bb[pc])
			: pos_.get_attack_set(sq, pos_.occupied_bb[0] | pos_.occupied_bb[1]);
		if ((pos_.kingblockers[Side][0] | pos_.kingblockers[Side][1]) & BITSET(sq))
			attacks &= ray_masks[sq][pos_.kingpos[Side]];

		if (const uint64_t king_attacks = attacks & king_rings_[opponent])
		{
			++king_attackers_count_[opponent];
			king_attackers_weight_[opponent] += attacker_weights[pc >> 1];
			king_attacks_count_[opponent] += popcnt(king_attacks);
		}

		double_targets_[Side] |= attacked_squares_[Side] & attacks;
		attacked_squares_[Side] |= attacked_squares_[pc] |= attacks;

		if (Type == bishop || Type == knight)
		{
			if (const uint64_t outpost_squares = outpostranks & ~pawnhashe_->attack_spans[opponent]; outpost_squares & BITSET(sq))
			{
				out += outpost_bonus[static_cast<bool>(attacked_squares_[make_piece(Side, pawn)] & BITSET(sq))][Type == knight];
			}
			else if (outpost_squares & attacks & ~pos_.occupied_bb[Side])
			{
				out += reachable_outpost[Type == knight];
			}

			if (Type == bishop)
			{
				const int same_color_pawns = popcnt(pos_.piece_bb[make_piece(Side, pawn)] & color_masks[sq]);
				out -= bishop_pawns * same_color_pawns;

				if (const uint64_t pawns = pos_.piece_bb[wpawn] | pos_.piece_bb[bpawn]; pawns & trapped_bishop[Side][sq])
				{
					out -= pawns & very_trapped_bishop[sq] ? very_trapped_bishop_penalty : trapped_bishop_penalty;
				}

				if (pos_.piece_bb[make_piece(opponent, knight)] & knight_opposing_bishop[Side][sq])
				{
					out += bishop_opposer_bonus;
				}

				mobility_[Side] += bishop_mobility_bonus[popcnt(attacks & mobility_area_[Side])];
			}
			else
			{
				mobility_[Side] += knight_mobility_bonus[popcnt(attacks & mobility_area_[Side])];
			}

			out -= king_protector * square_distance[sq][king_square];
		}

		if (Type == rook)
		{
			if (pawnhashe_->semiopen_files[Side] & 1 << FILE(sq))
			{
				if (pawnhashe_->semiopen_files[opponent] & 1 << FILE(sq))
				{
					out += rook_file[1];
				}

				else
				{
					out += attacked_squares_[make_piece(opponent, pawn)] & pos_.piece_bb[make_piece(opponent, pawn)] & file_bb[FILE(sq)] ? defended_rook_file : rook_file[0];
				}
			}

			if (file_bb[FILE(sq)] & (pos_.piece_bb[wqueen] | pos_.piece_bb[bqueen]))
			{
				out += battery;
			}

			if (RRANK(sq, Side) == 6 && RRANK(pos_.kingpos[opponent], Side) == 7)
			{
				out += rank7_rook;
			}

			mobility_[Side] += rook_mobility_bonus[popcnt(attacks & mobility_area_[Side])];
		}

		if (Type == queen)
		{
			mobility_[Side] += queen_mobility_bonus[popcnt(attacks & mobility_area_[Side])];
		}
	}
	return out;
}
//-------------------------------------------//
template <color Side>
Score Eval::evaluate_threats() const
{
	const color opponent = ~Side;
	Score out = S(0, 0);
	const uint64_t non_pawns = pos_.occupied_bb[opponent] ^ pos_.piece_bb[make_piece(opponent, pawn)];
	const uint64_t supported = (double_targets_[opponent] | attacked_squares_[make_piece(opponent, pawn)]) & ~double_targets_[Side];
	const uint64_t weak = attacked_squares_[Side] & ~supported & pos_.occupied_bb[opponent];
	const uint64_t occ = pos_.occupied_bb[0] | pos_.occupied_bb[1];
	const uint64_t r_rank3 = Side ? rank6_bb : rank3_bb;

	uint64_t attacked;
	if (non_pawns | weak)
	{
		int sq;
		attacked = (non_pawns | weak) & (attacked_squares_[make_piece(Side, knight)] | attacked_squares_[make_piece(Side, bishop)]);
		while (attacked)
		{
			sq = pop_lsb(attacked);
			out += minor_threat[pos_.mailbox[sq] >> 1];
		}

		attacked = (pos_.piece_bb[make_piece(opponent, queen)] | weak) & attacked_squares_[make_piece(Side, rook)];
		while (attacked)
		{
			sq = pop_lsb(attacked);
			out += rook_threat[pos_.mailbox[sq] >> 1];
		}

		attacked = weak & attacked_squares_[make_piece(Side, king)];
		if (attacked)
		{
			out += MORETHANONE(attacked) ? king_multiple_threat : king_threat;
		}

		out += hanging_piece * popcnt(weak & (~attacked_squares_[opponent] | (non_pawns & double_targets_[Side])));
	}

	const uint64_t safe = ~attacked_squares_[opponent] | attacked_squares_[Side];
	const uint64_t safe_pawns = safe & pos_.piece_bb[make_piece(Side, pawn)];
	attacked = PAWNATTACKS(Side, safe_pawns) & non_pawns;
	out += safe_pawn_threat * popcnt(attacked);

	uint64_t pawn_push = PAWNPUSH(Side, pos_.piece_bb[make_piece(Side, pawn)]) & ~occ;
	pawn_push |= PAWNPUSH(Side, pawn_push & r_rank3) & ~occ;

	pawn_push &= ~attacked_squares_[opponent] & safe;
	out += pawn_push_threat * popcnt(PAWNATTACKS(Side, pawn_push) & non_pawns);

	return out;
}
//-------------------------------------------//
int kbn_vs_k(const Position& pos)
{
	const color stronger_side = pos.psqt_score > 0 ? white : black;
	const bool dark_bishop = pos.piece_bb[wbishop + stronger_side] & dark_squares;
	const int sk = pos.kingpos[stronger_side] ^ (dark_bishop ? 0 : 56);
	const int wk = pos.kingpos[~stronger_side] ^ (dark_bishop ? 0 : 56);
	const int king_distance = 7 - square_distance[wk][sk];
	const int corner_distance = abs(7 - RANK(wk) - FILE(wk));

	return S2_M_SIGN(pos.active_side) * (won_endgame + 420 * corner_distance + 20 * king_distance);
}
//-------------------------------------------//
template <color Side>
Score Eval::king_safety() const
{
	const color opponent = Side == white ? black : white;
	int out_mg = 0;
	int out_eg = 0;
	const int king_square = pos_.kingpos[Side];
	const int pawn_shelter = pawnhashe_->pawn_shelter[Side];
	out_mg += pawn_shelter;

	if (king_attackers_count_[Side] > 1 - pos_.piece_count[make_piece(opponent, queen)])
	{
		const uint64_t weak = attacked_squares_[opponent]
			& ~double_targets_[Side]
			& (~attacked_squares_[Side] | attacked_squares_[make_piece(Side, king)] | attacked_squares_[make_piece(Side, queen)]);

		int king_danger = king_danger_base
			- pawn_shelter * king_shield_bonus / 10
			- !pos_.piece_count[make_piece(opponent, queen)] * no_queen
			+ king_attackers_count_[Side] * king_attackers_weight_[Side]
			+ king_attacks_count_[Side] * kingring_attack
			+ static_cast<bool>(pos_.kingblockers[Side]) * kingpinned_penalty
			+ popcnt(weak & king_rings_[Side]) * kingweak_penalty;

		const uint64_t safe = ~pos_.occupied_bb[opponent] & (~attacked_squares_[Side] | (weak & double_targets_[opponent]));

		const uint64_t occ = pos_.occupied_bb[0] | pos_.occupied_bb[1];
		const uint64_t rook_squares = rook_attacks(occ, king_square);
		const uint64_t bishop_squares = bishop_attacks(occ, king_square);
		const uint64_t knight_squares = pseudo_attacks[knight][king_square];

		const uint64_t rook_checks = rook_squares & attacked_squares_[make_piece(opponent, rook)];
		if (rook_checks)
		{
			king_danger += rook_checks & safe ? check_penalty[rook] : unsafe_check_penalty[rook];
		}

		const uint64_t queen_checks = (rook_squares | bishop_squares)
			& attacked_squares_[make_piece(opponent, queen)]
			& ~attacked_squares_[make_piece(Side, queen)]
			& ~rook_checks;

		if (queen_checks)
		{
			if (queen_checks & ~attacked_squares_[make_piece(Side, king)])
			{
				king_danger += queen_checks & safe ? check_penalty[queen] : unsafe_check_penalty[queen];
			}

			if (queen_checks & attacked_squares_[make_piece(Side, king)] & double_targets_[opponent] & weak)
			{
				king_danger += queen_contact_check;
			}
		}

		if (const uint64_t bishop_checks = bishop_squares
			& attacked_squares_[make_piece(opponent, bishop)]
			& ~queen_checks)
		{
			king_danger += bishop_checks & safe ? check_penalty[bishop] : unsafe_check_penalty[bishop];
		}

		if (const uint64_t knight_checks = knight_squares
			& attacked_squares_[make_piece(opponent, knight)])
		{
			king_danger += knight_checks & safe ? check_penalty[knight] : unsafe_check_penalty[knight];
		}

		if (king_danger > 0)
		{
			out_mg -= king_danger * king_danger / 4096;
			out_eg -= king_danger / 20;
		}
	}

	Score out = S(out_mg, out_eg);

	if (pos_.piece_bb[make_piece(Side, pawn)])
	{
		int distance = 0;
		while (!(distance_rings[king_square][distance++] & pos_.piece_bb[make_piece(Side, pawn)]))
		{
		}
		out -= pawn_distance_penalty * distance;
	}

	const uint64_t flank_attacks = attacked_squares_[opponent] & flank_ranks[Side] & flank_files[FILE(king_square)];
	const uint64_t double_flank_attacks = flank_attacks & double_targets_[opponent];
	out -= kingflank_attack * (popcnt(flank_attacks) + popcnt(double_flank_attacks));

	return out;
}
//-------------------------------------------//
int mat_imbalance(const Position& pos, const color side)
{
	int bonus = 0;
	for (int pt1 = pawn; pt1 < king; ++pt1)
	{
		if (!pos.piece_count[make_piece(side, static_cast<piece_type>(pt1))])
		{
			continue;
		}

		for (int pt2 = pawn; pt2 <= pt1; ++pt2)
		{
			bonus += pos.piece_count[make_piece(side, static_cast<piece_type>(pt1))] * (my_pieces[pt1 - 1][pt2 - 1] * pos.piece_count[make_piece(side, static_cast<piece_type>(pt2))] +
				opponent_pieces[pt1 - 1][pt2 - 1] * pos.piece_count[make_piece(~side, static_cast<piece_type>(pt2))]);
		}
	}

	return bonus;
}
//-------------------------------------------//
uint64_t material_key(const Position& pos)
{
	uint64_t materialhash = 0ULL;

	for (int i = wpawn; i <= bking; i++)
		for (int j = 0; j < pos.piece_count[i]; j++)
			materialhash ^= zb.piece_keys[j << 4 | i];

	return materialhash;
}
//-------------------------------------------//
template <color Side>
int Eval::pawn_shelter_score(const int sq) const
{
	int out = 0;
	const int middle_file = std::max(1, std::min(6, FILE(sq)));
	const uint64_t my_pawns = pos_.piece_bb[make_piece(Side, pawn)];
	const uint64_t opponent_pawns = pos_.piece_bb[make_piece(~Side, pawn)];

	for (int file = middle_file - 1; file <= middle_file + 1; file++)
	{
		uint64_t pawns = my_pawns & file_bb[file];
		const int defending_rank = pawns ? RRANK((Side ? msb(pawns) : lsb(pawns)), Side) : 0;
		pawns = opponent_pawns & file_bb[file];
		const int storming_rank = pawns ? RRANK((Side ? msb(pawns) : lsb(pawns)), Side) : 0;

		const int f = std::min(file, 7 - file);
		out += king_shield[f][defending_rank];
		const bool blocked = defending_rank != 0 && defending_rank == storming_rank - 1;
		out -= blocked ? pawn_storm_blocked[f][storming_rank] : pawn_storm_free[f][storming_rank];
	}

	return out;
}
//-------------------------------------------//
template <color Side>
void Eval::pawn_shelter_castling() const
{
	pawnhashe_->pawn_shelter[Side] = pawn_shelter_score<Side>(pos_.kingpos[Side]);
	if (pos_.castle_rights & kingside_castle_masks[Side])
		pawnhashe_->pawn_shelter[Side] = std::max(pawnhashe_->pawn_shelter[Side], pawn_shelter_score<Side>(castle_king_to[2 * Side + kingside]));

	if (pos_.castle_rights & queenside_castle_masks[Side])
		pawnhashe_->pawn_shelter[Side] = std::max(pawnhashe_->pawn_shelter[Side], pawn_shelter_score<Side>(castle_king_to[2 * Side + queenside]));
}
//-------------------------------------------//
void Eval::pre_eval()
{
	double_targets_[0] = double_targets_[1] = 0ULL;
	king_attackers_count_[0] = king_attackers_count_[1] = king_attacks_count_[0] = king_attacks_count_[1] = king_attackers_weight_[0] = king_attackers_weight_[1] = 0;
	king_rings_[0] = king_ring[pos_.kingpos[0]];
	king_rings_[1] = king_ring[pos_.kingpos[1]];

	const uint64_t real_occ = pos_.occupied_bb[0] | pos_.occupied_bb[1];

	attacked_squares_[wpawn] = PAWNATTACKS(white, pos_.piece_bb[wpawn]);
	attacked_squares_[bpawn] = PAWNATTACKS(black, pos_.piece_bb[bpawn]);
	attacked_squares_[wking] = pseudo_attacks[king][pos_.kingpos[white]];
	attacked_squares_[bking] = pseudo_attacks[king][pos_.kingpos[black]];

	double_targets_[white] = (pos_.piece_bb[wpawn] & ~filea_bb) << 7 & (pos_.piece_bb[wpawn] & ~fileh_bb) << 9;
	double_targets_[black] = (pos_.piece_bb[bpawn] & ~fileh_bb) >> 7 & (pos_.piece_bb[bpawn] & ~filea_bb) >> 9;
	double_targets_[white] |= attacked_squares_[wking] & attacked_squares_[wpawn];
	double_targets_[black] |= attacked_squares_[bking] & attacked_squares_[bpawn];

	attacked_squares_[white] = attacked_squares_[wking] | attacked_squares_[wpawn];
	attacked_squares_[black] = attacked_squares_[bking] | attacked_squares_[bpawn];

	uint64_t low_ranks = rank2_bb | rank3_bb;
	uint64_t blocked_pawns = pos_.piece_bb[wpawn] & (PAWNPUSH(black, real_occ) | low_ranks);
	mobility_area_[white] = ~(blocked_pawns | pos_.piece_bb[wking] | attacked_squares_[bpawn]);

	low_ranks = rank6_bb | rank7_bb;
	blocked_pawns = pos_.piece_bb[bpawn] & (PAWNPUSH(white, real_occ) | low_ranks);
	mobility_area_[black] = ~(blocked_pawns | pos_.piece_bb[bking] | attacked_squares_[wpawn]);
}
//-------------------------------------------//
int Position::scale_factor() const
{
	if (ONLYONE(piece_bb[wbishop]) && ONLYONE(piece_bb[bbishop]) && ONLYONE((piece_bb[wbishop] | piece_bb[bbishop]) & dark_squares))
	{
		if (non_pawn[0] == bishop_mg && non_pawn[1] == bishop_mg)
			return scale_ocb;
		return scale_ocb_pieces;
	}
	const color stronger_side = eg_value(psqt_score) > 0 ? white : black;
	if (!piece_bb[make_piece(stronger_side, pawn)] && non_pawn[stronger_side] <= non_pawn[~stronger_side] + bishop_mg)
	{
		return non_pawn[stronger_side] < rook_mg ? scale_nopawns : scale_hardtowin;
	}
	if (ONLYONE(piece_bb[make_piece(stronger_side, pawn)]) && non_pawn[stronger_side] <= non_pawn[~stronger_side] + bishop_mg)
	{
		return scale_onepawn;
	}
	return scale_normal;
}
//-------------------------------------------//
materialhashentry* probe_material(const Position& pos)
{
	materialhashentry* material = get_material_entry(pos);

	if (material->key == pos.materialhash)
	{
		return material;
	}

	material->key = pos.materialhash;
	material->phase = ((24 - (pos.piece_count[wbishop] + pos.piece_count[bbishop] + pos.piece_count[wknight] + pos.piece_count[bknight])
		- 2 * (pos.piece_count[wrook] + pos.piece_count[brook])
		- 4 * (pos.piece_count[wqueen] + pos.piece_count[bqueen])) * 255 + 12) / 24;

	int value = (mat_imbalance(pos, white) - mat_imbalance(pos, black)) / 16;

	if (pos.piece_count[wbishop] > 1)
	{
		value += bishop_pair;
	}
	if (pos.piece_count[bbishop] > 1)
	{
		value -= bishop_pair;
	}

	material->score = S(value, value);

	const int white_minor = pos.piece_count[wbishop] + pos.piece_count[wknight];
	const int white_major = pos.piece_count[wrook] + pos.piece_count[wqueen];
	const int black_minor = pos.piece_count[bbishop] + pos.piece_count[bknight];
	const int black_major = pos.piece_count[brook] + pos.piece_count[bqueen];
	const int all_minor = white_minor + black_minor;
	const int all_major = white_major + black_major;
	const bool no_pawns = pos.piece_count[wpawn] == 0 && pos.piece_count[bpawn] == 0;

	material->is_drawn = false;

	if (no_pawns && all_minor + all_major == 0)
	{
		material->is_drawn = true;
	}
	else if (no_pawns && all_major == 0 && white_minor < 2 && black_minor < 2)
	{
		material->is_drawn = true;
	}
	else if (no_pawns && all_major == 0 && all_minor == 2 && (pos.piece_count[wknight] == 2 || pos.piece_count[bknight] == 2))
	{
		material->is_drawn = true;
	}

	material->has_special_endgame = false;
	material->evaluation = nullptr;

	if (pos.materialhash == 0xa088eeb4f991b4ea || pos.materialhash == 0x52f8aa4b980286be)
	{
		material->has_special_endgame = true;
		material->evaluation = &kbn_vs_k;
	}

	return material;
}
//-------------------------------------------//
int Eval::value()
{
	const materialhashentry* material = probe_material(pos_);

	if (material->has_special_endgame)
	{
		return material->evaluation(pos_);
	}

	if (material->is_drawn)
	{
		return 1 - static_cast<int>(pos_.my_thread->nodes & 2);
	}

	pawnhashe_ = get_pawntte(pos_);
	evaluate_pawns();

	Score out = material->score + pos_.psqt_score;

	out += pawnhashe_->scores[white] - pawnhashe_->scores[black];

	if (const int lazy_value = (mg_value(out) + eg_value(out)) / 2; abs(lazy_value) >= lazy_threshold + (pos_.non_pawn[0] + pos_.non_pawn[1]) / 64)
		goto return_flag;

	pre_eval();

	out += evaluate_piece<white, knight>() - evaluate_piece<black, knight>() +
		evaluate_piece<white, bishop>() - evaluate_piece<black, bishop>() +
		evaluate_piece<white, rook>() - evaluate_piece<black, rook>() +
		evaluate_piece<white, queen>() - evaluate_piece<black, queen>();

	out += evaluate_passers<white>() - evaluate_passers<black>() +
		mobility_[white] - mobility_[black] +
		evaluate_threats<white>() - evaluate_threats<black>() +
		king_safety<white>() - king_safety<black>();
return_flag:

	const int phase = material->phase;
	int v = (mg_value(out) * (256 - phase) + eg_value(out) * phase * pos_.scale_factor() / scale_normal) / 256;
	v = v * S2_M_SIGN(pos_.active_side) + move_tempo;
	return v;
}
//-------------------------------------------//
double to_cp(const int v) { return static_cast<double>(v) / pawn_eg; }
//-------------------------------------------//
std::ostream& operator<<(std::ostream& os, const Score s)
{
	os << std::setw(5) << to_cp(mg_value(s)) << " "
		<< std::setw(5) << to_cp(eg_value(s));
	return os;
}
//-------------------------------------------//