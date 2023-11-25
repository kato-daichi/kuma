//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include "position.h"
#include "thread.h"
#if !defined(_MSC_VER)
#define nnue_embedded
#define nnue_eval_file "kuma.nnue"
#endif
int evaluate(const Position& p);
//-------------------------------------------//
inline materialhashentry* get_material_entry(const Position& p)
{
	return &p.my_thread->material_table[p.materialhash & material_hash_size_mask];
}
//-------------------------------------------//
inline constexpr int pawn_eg = 150;
inline constexpr int pawn_mg = 100;
inline constexpr int knight_eg = 440;
inline constexpr int knight_mg = 370;
inline constexpr int bishop_eg = 530;
inline constexpr int bishop_mg = 430;
inline constexpr int bishop_pair = 80;
inline constexpr int rook_eg = 950;
inline constexpr int rook_mg = 730;
inline constexpr int queen_eg = 1660;
inline constexpr int queen_mg = 1240;
inline constexpr int king_eg = 0;
inline constexpr int king_mg = 0;
inline constexpr int move_tempo = 30;
//-------------------------------------------//
inline int piece_values[2][14] = {
	{0, 0, pawn_mg, pawn_mg, knight_mg, knight_mg, bishop_mg, bishop_mg, rook_mg, rook_mg, queen_mg, queen_mg, 0, 0},
	{0, 0, pawn_eg, pawn_eg, knight_eg, knight_eg, bishop_eg, bishop_eg, rook_eg, rook_eg, queen_eg, queen_eg, 0, 0}
};
//-------------------------------------------//
inline int non_pawn_value[14] = { 0, 0, 0, 0, knight_mg, knight_mg, bishop_mg, bishop_mg, rook_mg, rook_mg, queen_mg, queen_mg, 0, 0 };
//-------------------------------------------//
