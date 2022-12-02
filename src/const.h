//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include "defs.h"
#include "enum.h"
//-------------------------------------------//
piece_type get_piece_type(char c);
inline constexpr uint64_t rank1_bb = 0xFF;
inline constexpr uint64_t rank2_bb = rank1_bb << 8 * 1;
inline constexpr uint64_t rank3_bb = rank1_bb << 8 * 2;
inline constexpr uint64_t rank4_bb = rank1_bb << 8 * 3;
inline constexpr uint64_t rank5_bb = rank1_bb << 8 * 4;
inline constexpr uint64_t rank6_bb = rank1_bb << 8 * 5;
inline constexpr uint64_t rank7_bb = rank1_bb << 8 * 6;
inline constexpr uint64_t rank8_bb = rank1_bb << 8 * 7;

inline constexpr uint64_t filea_bb = 0x0101010101010101ULL;
inline constexpr uint64_t fileb_bb = filea_bb << 1;
inline constexpr uint64_t filec_bb = filea_bb << 2;
inline constexpr uint64_t filed_bb = filea_bb << 3;
inline constexpr uint64_t filee_bb = filea_bb << 4;
inline constexpr uint64_t filef_bb = filea_bb << 5;
inline constexpr uint64_t fileg_bb = filea_bb << 6;
inline constexpr uint64_t fileh_bb = filea_bb << 7;

inline constexpr uint64_t file_ah = filea_bb | fileh_bb;
inline constexpr uint64_t file_bb[8] = { filea_bb, fileb_bb, filec_bb, filed_bb, filee_bb, filef_bb, fileg_bb, fileh_bb };

inline constexpr int castle_king_to[4] = { c1, g1, c8, g8 };
inline constexpr int castle_rook_from[4] = { a1, h1, a8, h8 };
inline constexpr int castle_rook_to[4] = { d1, f1, d8, f8 };
inline constexpr int diagonaloffset[] = { south_east, south_west, north_west, north_east };
inline constexpr int kingside_castle_masks[2] = { 0x02, 0x08 };
inline constexpr int knightoffset[] = { south + east + east, south + west + west, south + south + east,
	south + south + west, north + west + west, north + east + east, north + north + west, north + north + east };
inline constexpr int orthogonalanddiagonaloffset[] = { south, west, east, north, south_east, south_west, north_west, north_east };
inline constexpr int orthogonaloffset[] = { south, west, east, north };
inline constexpr int queenside_castle_masks[2] = { 0x01, 0x04 };

inline constexpr uint64_t center_files = filec_bb | filed_bb | filee_bb | filef_bb;
inline constexpr uint64_t dark_squares = 0xAA55AA55AA55AA55ULL;
inline constexpr uint64_t edges = file_ah | rank1_bb | rank2_bb | rank7_bb | rank8_bb;
inline constexpr uint64_t center = ~edges;
inline constexpr uint64_t king_side = filee_bb | filef_bb | fileg_bb | fileh_bb;
inline constexpr uint64_t queen_side = filea_bb | fileb_bb | filec_bb | filed_bb;
inline constexpr uint64_t flank_files[8] = { queen_side, queen_side, queen_side, center_files, center_files, king_side, king_side, king_side};
inline constexpr uint64_t flank_ranks[2] = { rank1_bb | rank2_bb | rank3_bb | rank4_bb, rank5_bb | rank6_bb | rank7_bb | rank8_bb };

inline constexpr uint64_t outpost_ranks[2] = {	rank4_bb | rank5_bb | rank6_bb, rank3_bb | rank4_bb | rank5_bb };
inline constexpr uint64_t rank_bb[8] = { rank1_bb, rank2_bb, rank3_bb, rank4_bb, rank5_bb, rank6_bb, rank7_bb, rank8_bb };
//-------------------------------------------//
constexpr color operator~(const color c)
{
	return static_cast<color>(c ^ black);
}
//-------------------------------------------//
template <direction D>
constexpr uint64_t shift(const uint64_t b)
{
	return  D == north ? b << 8 : D == south ? b >> 8
		: D == north + north ? b << 16 : D == south + south ? b >> 16
		: D == east ? (b & ~fileh_bb) << 1 : D == west ? (b & ~filea_bb) >> 1
		: D == north_east ? (b & ~fileh_bb) << 9 : D == north_west ? (b & ~filea_bb) << 7
		: D == south_east ? (b & ~fileh_bb) >> 7 : D == south_west ? (b & ~filea_bb) >> 9
		: 0;
}
//-------------------------------------------//
inline constexpr uint64_t trapped_bishop[2][64] =
{
	{
		BITSET(b2), BITSET(c2), 0, 0, 0, 0, BITSET(f2), BITSET(g2),
		BITSET(b3), 0, 0, 0, 0, 0, 0, BITSET(g3),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		BITSET(b5), 0, 0, 0, 0, 0, 0, BITSET(g5),
		BITSET(b6), 0, 0, 0, 0, 0, 0, BITSET(g6),
		BITSET(b7), BITSET(c7), 0, 0, 0, 0, BITSET(f7), BITSET(g7)
	},
	{
		BITSET(b2), BITSET(c2), 0, 0, 0, 0, BITSET(f2), BITSET(g2),
		BITSET(b3), 0, 0, 0, 0, 0, 0, BITSET(g3),
		BITSET(b4), 0, 0, 0, 0, 0, 0, BITSET(g4),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		BITSET(b6), 0, 0, 0, 0, 0, 0, BITSET(g6),
		BITSET(b7), BITSET(c7), 0, 0, 0, 0, BITSET(f7), BITSET(g7)
	}
};
//-------------------------------------------//
inline constexpr uint64_t very_trapped_bishop[64] =
{
	BITSET(b3), BITSET(b3), 0, 0, 0, 0, BITSET(g3), BITSET(g3),
	BITSET(b3), 0, 0, 0, 0, 0, 0, BITSET(f2),
	BITSET(b4), 0, 0, 0, 0, 0, 0, BITSET(f3),
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	BITSET(c6), 0, 0, 0, 0, 0, 0, BITSET(f6),
	BITSET(c7), 0, 0, 0, 0, 0, 0, BITSET(f7),
	BITSET(b6), BITSET(b6), 0, 0, 0, 0, BITSET(g6), BITSET(g6)
};
//-------------------------------------------//
inline constexpr uint64_t knight_opposing_bishop[2][64] =
{
	{
		0, 0, 0, 0, 0, 0, 0, 0,
		BITSET(a5), BITSET(b5), BITSET(c5), BITSET(d5), BITSET(e5), BITSET(f5), BITSET(g5), BITSET(h5),
		BITSET(a6), BITSET(b6), BITSET(c6), BITSET(d6), BITSET(e6), BITSET(f6), BITSET(g6), BITSET(h6),
		BITSET(a7), BITSET(b7), BITSET(c7), BITSET(d7), BITSET(e7), BITSET(f7), BITSET(g7), BITSET(h7),
		BITSET(a8), BITSET(b8), BITSET(c8), BITSET(d8), BITSET(e8), BITSET(f8), BITSET(g8), BITSET(h8),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	},
	{
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		BITSET(a1), BITSET(b1), BITSET(c1), BITSET(d1), BITSET(e1), BITSET(f1), BITSET(g1), BITSET(h1),
		BITSET(a2), BITSET(b2), BITSET(c2), BITSET(d2), BITSET(e2), BITSET(f2), BITSET(g2), BITSET(h2),
		BITSET(a3), BITSET(b3), BITSET(c3), BITSET(d3), BITSET(e3), BITSET(f3), BITSET(g3), BITSET(h3),
		BITSET(a4), BITSET(b4), BITSET(c4), BITSET(d4), BITSET(e4), BITSET(f4), BITSET(g4), BITSET(h4),
		0, 0, 0, 0, 0, 0, 0, 0
	}
};
//-------------------------------------------//
inline constexpr uint64_t knight_opposing_bishop_left_right[64] =
{
	0, 0, 0, BITSET(a1), BITSET(h1), 0, 0, 0,
	0, 0, 0, BITSET(a2), BITSET(h2), 0, 0, 0,
	0, 0, 0, BITSET(a3), BITSET(h3), 0, 0, 0,
	0, 0, 0, BITSET(a4), BITSET(h4), 0, 0, 0,
	0, 0, 0, BITSET(a5), BITSET(h5), 0, 0, 0,
	0, 0, 0, BITSET(a6), BITSET(h6), 0, 0, 0,
	0, 0, 0, BITSET(a7), BITSET(h7), 0, 0, 0,
	0, 0, 0, BITSET(a8), BITSET(h8), 0, 0, 0
};
//-------------------------------------------//
template <color C>
constexpr uint64_t pawn_attacks_bb(const uint64_t b)
{
	return C == white
		? shift<north_west>(b) | shift<north_east>(b)
		: shift<south_west>(b) | shift<south_east>(b);
}
//-------------------------------------------//
constexpr direction pawn_push(const color c)
{
	return c == white ? north : south;
}
//-------------------------------------------//

constexpr special_type type_of(const uint16_t m)
{
	return static_cast<special_type>(m >> 14 & 3);
}
//-------------------------------------------//
constexpr int from_sq(const uint16_t m)
{
	return m >> 6 & 0x3F;
}
//-------------------------------------------//
constexpr int to_sq(const uint16_t m)
{
	return m & 0x3F;
}
//-------------------------------------------//
constexpr int from_to(const uint16_t m)
{
	return m & 0xFFF;
}
//-------------------------------------------//
constexpr piece_type promotion_type(const uint16_t m)
{
	return static_cast<piece_type>((m >> 12 & 3) + knight);
}
//-------------------------------------------//
constexpr move make_move(const int from, const int to, const special_type type)
{
	return static_cast<move>(type << 14 | from << 6 | to);
}
//-------------------------------------------//
constexpr move make_promotion_move(const int from, const int to, const piece_type promote, const special_type type)
{
	return static_cast<move>(type << 14 | (promote - knight) << 12 | from << 6 | to);
}
//-------------------------------------------//
constexpr piece_code make_piece(const color c, const piece_type pt)
{
	return static_cast<piece_code>((pt << 1) + c);
}
//-------------------------------------------//
constexpr Score make_score(const int mg, const int eg)
{
	return static_cast<Score>(static_cast<int>(static_cast<unsigned>(eg) << 16) + mg);
}
//-------------------------------------------//
constexpr Score operator+(const Score d1, const Score d2) { return static_cast<Score>(static_cast<int>(d1) + static_cast<int>(d2)); }
//-------------------------------------------//
constexpr Score operator-(const Score d1, const Score d2) { return static_cast<Score>(static_cast<int>(d1) - static_cast<int>(d2)); }
//-------------------------------------------//
constexpr Score operator-(const Score d) { return static_cast<Score>(-static_cast<int>(d)); }
//-------------------------------------------//
constexpr Score& operator+=(Score& d1, const Score d2) { return d1 = d1 + d2; }
//-------------------------------------------//
constexpr Score& operator-=(Score& d1, const Score d2) { return d1 = d1 - d2; }
//-------------------------------------------//
