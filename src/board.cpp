//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <cmath>
#include "position.h"
#include "board.h"
#include "magic.h"
//-------------------------------------------//
uint64_t bishop_attacks(const uint64_t occ, const int sq)
{
	const s_magic m = bishop_tbl[sq];
	return m.ptr[m.index(occ)];
}
//-------------------------------------------//
uint64_t rook_attacks(const uint64_t occ, const int sq)
{
	const s_magic m = rook_tbl[sq];
	return m.ptr[m.index(occ)];
}
//-------------------------------------------//
void init_boards()
{
	for (int i = 1; i < 64; i++)
		for (int j = 1; j < 64; j++)
		{
			reductions[0][i][j] = static_cast<int>(1 + round(log(1.5 * i) * log(j) * 0.55));
			reductions[1][i][j] = static_cast<int>(round(log(1.5 * i) * log(2 * j) * 0.4));
		}
	for (int i = 0; i < 64; i++)
	{
		castling_rights[i] = static_cast<uint64_t>(-1);
		const int rank = RANK(i);
		const int file = FILE(i);
		if (rank == 0 && (file == 0 || file == 4))
			castling_rights[i] &= ~wqc_mask;
		if (rank == 0 && (file == 7 || file == 4))
			castling_rights[i] &= ~wkc_mask;
		if (rank == 7 && (file == 0 || file == 4))
			castling_rights[i] &= ~bqc_mask;
		if (rank == 7 && (file == 7 || file == 4))
			castling_rights[i] &= ~bkc_mask;
	}
	memset(&distance_rings, 0, sizeof distance_rings);
	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 64; j++)
		{
			if (i != j)
			{
				int r_distance = abs(RANK(i) - RANK(j));
				int f_distance = abs(FILE(i) - FILE(j));
				const int distance = std::max(r_distance, f_distance);
				distance_rings[i][distance - 1] |= SET_BIT(j);
			}
		}
	}
	uint64_t passed_pawn_temp[2][64]{};
	uint64_t king_ring_temp[64]{};
	for (int i = 0; i < 64; i++)
	{
		pseudo_attacks[king][i] = 0ULL;
		pseudo_attacks[knight][i] = 0ULL;
		rank_masks[i] = 0ULL;
		file_masks[i] = 0ULL;
		diag_masks[i] = 0ULL;
		antidiag_masks[i] = 0ULL;
		neighbor_masks[i] = 0ULL;
		passed_pawn_masks[0][i] = passed_pawn_masks[1][i] = file_bb[FILE(i)];
		passed_pawn_temp[0][i] = passed_pawn_temp[1][i] = 0ULL;
		pawn_blocker_masks[0][i] = pawn_blocker_masks[1][i] = file_bb[FILE(i)];
		phalanx_masks[i] = 0ULL;
		pawn_attacks[0][i] = pawn_attacks[1][i] = 0ULL;
		pawn_pushes[0][i] = pawn_pushes[1][i] = 0ULL;
		pawn_2_pushes[0][i] = pawn_2_pushes[1][i] = 0ULL;
		pawn_2_pushesfrom[0][i] = pawn_2_pushesfrom[1][i] = 0ULL;
		pawn_pushesfrom[0][i] = pawn_pushesfrom[1][i] = 0ULL;
		pawn_attacks_from[0][i] = pawn_attacks_from[1][i] = 0ULL;
		king_ring_temp[i] = SET_BIT(i);
		square_masks[i] = SET_BIT(i);
		color_masks[i] = i % 2 == 0 ? dark_squares : ~dark_squares;
		for (int j = 0; j < 64; j++)
		{
			square_distance[i][j] = std::min(abs(FILE(i) - FILE(j)), abs(RANK(i) - RANK(j)));
			between_masks[i][j] = 0ULL;
			ray_masks[i][j] = 0ULL;
			if (FILE(i) == FILE(j) && i != j)
			{
				file_masks[i] |= SET_BIT(j);
				for (int k = std::min(RANK(i), RANK(j)) + 1; k < std::max(RANK(i), RANK(j)); k++)
					between_masks[i][j] |= SET_BIT(INDEX(k, FILE(i)));
			}
			if (RANK(i) == RANK(j) && i != j)
			{
				rank_masks[i] |= SET_BIT(j);
				for (int k = std::min(FILE(i), FILE(j)) + 1; k < std::max(FILE(i), FILE(j)); k++)
					between_masks[i][j] |= SET_BIT(INDEX(RANK(i), k));
			}
			if (abs(RANK(i) - RANK(j)) == abs(FILE(i) - FILE(j)) && i != j)
			{
				const int dx = FILE(i) < FILE(j) ? 1 : -1;
				const int dy = RANK(i) < RANK(j) ? 1 : -1;
				for (int k = 1; FILE(i) + k * dx != FILE(j); k++)
				{
					between_masks[i][j] |= SET_BIT(INDEX((RANK(i) + k * dy), (FILE(i) + k * dx)));
				}
				if (abs(dx + dy) == 2)
				{
					diag_masks[i] |= SET_BIT(j);
				}
				else
				{
					antidiag_masks[i] |= SET_BIT(j);
				}
			}
		}
		rook_masks[i] = (rank_masks[i] & ~(filea_bb | fileh_bb)) | (file_masks[i] & ~(rank1_bb | rank8_bb));
		bishop_masks[i] = (diag_masks[i] | antidiag_masks[i]) & ~(filea_bb | fileh_bb | rank1_bb | rank8_bb);
		int to;
		for (int j = 0; j < 8; j++)
		{
			to = i + orthogonal_and_diagonal_offset[j];
			if (to >= 0 && to < 64 && abs(FILE(i) - FILE(to)) <= 1)
				pseudo_attacks[king][i] |= SET_BIT(to);
			to = i + knight_offset[j];
			if (to >= 0 && to < 64 && abs(FILE(i) - FILE(to)) <= 2)
				pseudo_attacks[knight][i] |= SET_BIT(to);
		}
		king_ring_temp[i] |= pseudo_attacks[king][i];
		for (int s = 0; s < 2; s++)
		{
			if (R_RANK(i, s) < 7)
				pawn_pushes[s][i] |= SET_BIT(i + SIDE2_M_SIGN(s) * 8);
			if (R_RANK(i, s) > 0)
				pawn_pushesfrom[s][i] |= SET_BIT(i - SIDE2_M_SIGN(s) * 8);
			if (R_RANK(i, s) == 1)
				pawn_2_pushes[s][i] |= SET_BIT(i + SIDE2_M_SIGN(s) * 16);
			if (R_RANK(i, s) == 3)
				pawn_2_pushesfrom[s][i] |= SET_BIT(i - SIDE2_M_SIGN(s) * 16);
			for (int j = i + SIDE2_M_SIGN(s) * 8; 0 <= j && j <= 63; j += SIDE2_M_SIGN(s) * 8)
			{
				passed_pawn_temp[s][i] |= rank_bb[RANK(j)];
			}
			if (FILE(i) != 0)
			{
				neighbor_masks[i] |= file_bb[FILE(i - 1)];
				passed_pawn_masks[s][i] |= file_bb[FILE(i - 1)];
				phalanx_masks[i] |= SET_BIT(i - 1);
			}
			if (FILE(i) != 7)
			{
				neighbor_masks[i] |= file_bb[FILE(i + 1)];
				passed_pawn_masks[s][i] |= file_bb[FILE(i + 1)];
				phalanx_masks[i] |= SET_BIT(i + 1);
			}
			for (int d = -1; d <= 1; d++)
			{
				to = i + SIDE2_M_SIGN(s) * 8 + d;
				if (d && abs(FILE(i) - FILE(to)) <= 1 && to >= 0 && to < 64)
					pawn_attacks[s][i] |= SET_BIT(to);
				to = i - SIDE2_M_SIGN(s) * 8 + d;
				if (d && abs(FILE(i) - FILE(to)) <= 1 && to >= 0 && to < 64)
					pawn_attacks_from[s][i] |= SET_BIT(to);
			}
			passed_pawn_masks[s][i] &= passed_pawn_temp[s][i];
			pawn_blocker_masks[s][i] &= passed_pawn_temp[s][i];
		}
		ept_helper[i] = 0ULL;
		if (RANK(i) == 3 || RANK(i) == 4)
		{
			if (RANK(i - 1) == RANK(i))
				ept_helper[i] |= SET_BIT(i - 1);
			if (RANK(i + 1) == RANK(i))
				ept_helper[i] |= SET_BIT(i + 1);
		}
	}
	for (int i = 0; i < 64; i++)
	{
		const int r = std::max(1, std::min(RANK(i), 6));
		const int f = std::max(1, std::min(FILE(i), 6));
		king_ring[i] = king_ring_temp[INDEX(r, f)];
	}
	init_magics(r_attacks, rook_tbl, rook_masks, rook_attacks_slow, rook_magics);
	init_magics(battacks, bishop_tbl, bishop_masks, bishop_attacks_slow, bishop_magics);
	for (int i = 0; i < 64; i++)
	{
		pseudo_attacks[queen][i] = pseudo_attacks[bishop][i] = bishop_attacks(0, i);
		pseudo_attacks[queen][i] |= pseudo_attacks[rook][i] = rook_attacks(0, i);
		for (int j = 0; j < 64; j++)
		{
			if (pseudo_attacks[rook][i] & SET_BIT(j))
			{
				ray_masks[i][j] |= (pseudo_attacks[rook][i] & rook_attacks(0, j)) | SET_BIT(i) | SET_BIT(j);
			}
			if (pseudo_attacks[bishop][i] & SET_BIT(j))
			{
				ray_masks[i][j] |= (pseudo_attacks[bishop][i] & bishop_attacks(0, j)) | SET_BIT(i) | SET_BIT(j);
			}
		}
	}
	for (int i = 0; i < 4; i++)
	{
		const int side = i / 2;
		castle_king_walk[i] = between_masks[4 + 56 * side][castle_king_to[i]] | SET_BIT(castle_king_to[i]);
	}
}
