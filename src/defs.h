//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
constexpr auto engine_name = "kuma";
constexpr auto version = "1.2";
constexpr auto author = "kato daichi";
using searchthread = struct search_thread;
using pawnhashtable = struct pawnhash_table;
using searchinfo = struct search_info;
using materialhashentry = struct materialhash_entry;
inline constexpr auto bkc_mask = 0x08;
inline constexpr auto black_castle = 0x0c;
inline constexpr auto bqc_mask = 0x04;
inline constexpr auto promotion_rank_bb = 0xff000000000000ff;
inline constexpr auto side_switch = 0x01;
inline constexpr auto start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
inline constexpr auto white_castle = 0x03;
inline constexpr auto wkc_mask = 0x02;
inline constexpr auto wqc_mask = 0x01;
#define DOUBLE_PUSH_INDEX(s, x) ((s) ? ((x) - 16) : ((x) + 16))
#define FILE(x) ((x) & 0x7)
#define FLIP_SQUARE(s, x) ((s) ? (x) ^ 56 : (x))
#define INDEX(r,f) (((r) << 3) | (f))
#define MORE_THAN_ONE(x) ((x) & ((x) - 1))
#define ONE_OR_ZERO(x) (!MORE_THAN_ONE(x))
#define ONLY_ONE(x) (!MORE_THAN_ONE(x) && (x))
#define PAWN_ATTACKS(s, x) ((s) ? ((shift<south_east>(x)) | (shift < south_west> (x))) : ((shift <north_east> (x)) | (shift <north_west> (x))))
#define PAWN_PUSH(s, x) ((s) ? ((x) >> 8) : ((x) << 8))
#define PAWN_PUSH_INDEX(s, x) ((s) ? ((x) - 8) : ((x) + 8))
#define PROMOTION_RANK(x) (RANK(x) == 0 || RANK(x) == 7)
#define RANK(x) ((x) >> 3)
#define R_RANK(x,s) ((s) ? ((x) >> 3) ^ 7 : ((x) >> 3))
#define S(m, e) make_score(m, e)
#define SET_BIT(x) (1ULL << (x))
#define SIDE2_M_SIGN(s) ((s) ? -1 : 1)
