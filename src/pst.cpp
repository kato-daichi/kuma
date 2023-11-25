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
	piece_values[mg][0] = piece_values[mg][1] = 0;
	piece_values[mg][w_pawn] = piece_values[mg][b_pawn] = pawn_mg;
	piece_values[mg][w_knight] = piece_values[mg][b_knight] = knight_mg;
	piece_values[mg][w_bishop] = piece_values[mg][b_bishop] = bishop_mg;
	piece_values[mg][w_rook] = piece_values[mg][b_rook] = rook_mg;
	piece_values[mg][w_queen] = piece_values[mg][b_queen] = queen_mg;
	piece_values[mg][w_king] = piece_values[mg][b_king] = 0;
	piece_values[eg][0] = piece_values[eg][1] = 0;
	piece_values[eg][w_pawn] = piece_values[eg][b_pawn] = pawn_eg;
	piece_values[eg][w_knight] = piece_values[eg][b_knight] = knight_eg;
	piece_values[eg][w_bishop] = piece_values[eg][b_bishop] = bishop_eg;
	piece_values[eg][w_rook] = piece_values[eg][b_rook] = rook_eg;
	piece_values[eg][w_queen] = piece_values[eg][b_queen] = queen_eg;
	piece_values[eg][w_king] = piece_values[eg][b_king] = 0;
	non_pawn_value[0] = non_pawn_value[1] = 0;
	non_pawn_value[w_pawn] = non_pawn_value[b_pawn] = 0;
	non_pawn_value[w_knight] = non_pawn_value[b_knight] = knight_mg;
	non_pawn_value[w_bishop] = non_pawn_value[b_bishop] = bishop_mg;
	non_pawn_value[w_rook] = non_pawn_value[b_rook] = rook_mg;
	non_pawn_value[w_queen] = non_pawn_value[b_queen] = queen_mg;
	non_pawn_value[w_king] = non_pawn_value[b_king] = 0;
	for (int victim = no_piece; victim <= b_king; victim++)
		for (int attacker = 0; attacker <= b_king; attacker++)
		{
			mvvlva[victim][attacker] = piece_values[mg][victim] - attacker;
		}
}
//-------------------------------------------//
