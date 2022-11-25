//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "position.h"
#include "eval.h"
#include "board.h"
#include "pst.h"
//-------------------------------------------//
void init_values()
{
	for (int i = 0; i < 64; i++)
	{
		psq.psqt[wpawn][i] = (piece_bonus[pawn][FLIP_SQUARE(white, i)] + S(pawn_mg, pawn_eg)) * S2_M_SIGN(white);
		psq.psqt[bpawn][i] = (piece_bonus[pawn][FLIP_SQUARE(black, i)] + S(pawn_mg, pawn_eg)) * S2_M_SIGN(black);
		psq.psqt[wknight][i] = (piece_bonus[knight][FLIP_SQUARE(white, i)] + S(knight_mg, knight_eg)) * S2_M_SIGN(white);
		psq.psqt[bknight][i] = (piece_bonus[knight][FLIP_SQUARE(black, i)] + S(knight_mg, knight_eg)) * S2_M_SIGN(black);
		psq.psqt[wbishop][i] = (piece_bonus[bishop][FLIP_SQUARE(white, i)] + S(bishop_mg, bishop_eg)) * S2_M_SIGN(white);
		psq.psqt[bbishop][i] = (piece_bonus[bishop][FLIP_SQUARE(black, i)] + S(bishop_mg, bishop_eg)) * S2_M_SIGN(black);
		psq.psqt[wrook][i] = (piece_bonus[rook][FLIP_SQUARE(white, i)] + S(rook_mg, rook_eg)) * S2_M_SIGN(white);
		psq.psqt[brook][i] = (piece_bonus[rook][FLIP_SQUARE(black, i)] + S(rook_mg, rook_eg)) * S2_M_SIGN(black);
		psq.psqt[wqueen][i] = (piece_bonus[queen][FLIP_SQUARE(white, i)] + S(queen_mg, queen_eg)) * S2_M_SIGN(white);
		psq.psqt[bqueen][i] = (piece_bonus[queen][FLIP_SQUARE(black, i)] + S(queen_mg, queen_eg)) * S2_M_SIGN(black);
		psq.psqt[wking][i] = (piece_bonus[king][FLIP_SQUARE(white, i)] + S(king_mg, king_eg)) * S2_M_SIGN(white);
		psq.psqt[bking][i] = (piece_bonus[king][FLIP_SQUARE(black, i)] + S(king_mg, king_eg)) * S2_M_SIGN(black);
	}

	piece_values[mg][0] = piece_values[mg][1] = 0;
	piece_values[mg][wpawn] = piece_values[mg][bpawn] = pawn_mg;
	piece_values[mg][wknight] = piece_values[mg][bknight] = knight_mg;
	piece_values[mg][wbishop] = piece_values[mg][bbishop] = bishop_mg;
	piece_values[mg][wrook] = piece_values[mg][brook] = rook_mg;
	piece_values[mg][wqueen] = piece_values[mg][bqueen] = queen_mg;
	piece_values[mg][wking] = piece_values[mg][bking] = 0;

	piece_values[eg][0] = piece_values[eg][1] = 0;
	piece_values[eg][wpawn] = piece_values[eg][bpawn] = pawn_eg;
	piece_values[eg][wknight] = piece_values[eg][bknight] = knight_eg;
	piece_values[eg][wbishop] = piece_values[eg][bbishop] = bishop_eg;
	piece_values[eg][wrook] = piece_values[eg][brook] = rook_eg;
	piece_values[eg][wqueen] = piece_values[eg][bqueen] = queen_eg;
	piece_values[eg][wking] = piece_values[eg][bking] = 0;

	non_pawn_value[0] = non_pawn_value[1] = 0;
	non_pawn_value[wpawn] = non_pawn_value[bpawn] = 0;
	non_pawn_value[wknight] = non_pawn_value[bknight] = knight_mg;
	non_pawn_value[wbishop] = non_pawn_value[bbishop] = bishop_mg;
	non_pawn_value[wrook] = non_pawn_value[brook] = rook_mg;
	non_pawn_value[wqueen] = non_pawn_value[bqueen] = queen_mg;
	non_pawn_value[wking] = non_pawn_value[bking] = 0;

	for (int victim = blank; victim <= bking; victim++)
		for (int attacker = 0; attacker <= bking; attacker++)
		{
			mvvlva[victim][attacker] = piece_values[mg][victim] - attacker;
		}
}
//-------------------------------------------//
