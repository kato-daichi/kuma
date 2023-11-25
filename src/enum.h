//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <cstdint>
//-------------------------------------------//
enum
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};
//-------------------------------------------//
inline constexpr int max_ply = 128;
inline constexpr int max_threads = 32;
enum { value_draw = 0, value_mate = 32000, value_inf = 32001, undefined = 32002, timeout = 32003, won_endgame = 10000, mate_in_max_ply = value_mate - max_ply,
	value_mated = -32000, mated_in_max_ply = value_mated + max_ply };
enum castle_sides { queenside, kingside };
enum color { white, black };
enum direction : int { north = 8, east = 1, south = -8, west = -1, north_east = north + east, south_east = south + east, south_west = south + west, north_west = north + west };
enum eval_terms { material, mobility, knights, bishops, rooks, queens, imbalance, pawns, passers, king_safety, threat, total, phase, scale, tempo, term_nb };
enum game_phase { mg, eg };
enum move : uint16_t { move_none, move_null = 65 };
enum move_type { quiet = 1, capture = 2, promote = 4, tactical = 6, all = 7, evasion = 8, quiet_check = 16 };
enum pick_type { next, best };
enum piece_code { no_piece, w_pawn = 2, b_pawn, w_knight, b_knight, w_bishop, b_bishop, w_rook, b_rook, w_queen, b_queen, w_king, b_king };
enum piece_type { blanktype, pawn, knight, bishop, rook, queen, king };
enum score : int { score_draw };
enum score_type { score_quiet, score_capture, score_evasion };
enum search_type { normal_search, quiescence_search, probcut_search};
enum special_type { normal, promotion = 1, enpassant = 2, castling = 3 };
enum stages { hashmove_state = 0, tactical_init, tactical_state, killer_move_2, countermove, quiets_init, quiet_state, bad_tactical_state, evasions_init,
	evasions_state, q_hashmove, q_captures_init, q_captures, q_checks_init, q_checks, probcut_hashmove, probcut_captures_init, probcut_captures };
