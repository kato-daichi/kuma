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
inline constexpr int diagonal_offset[] = { south_east, south_west, north_west, north_east };
inline constexpr int kingside_castle_masks[2] = { 0x02, 0x08 };
inline constexpr int knight_offset[] = { south + east + east, south + west + west, south + south + east,
	south + south + west, north + west + west, north + east + east, north + north + west, north + north + east };
inline constexpr int orthogonal_and_diagonal_offset[] = { south, west, east, north, south_east, south_west, north_west, north_east };
inline constexpr int orthogonal_offset[] = { south, west, east, north };
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
		SET_BIT(b2), SET_BIT(c2), 0, 0, 0, 0, SET_BIT(f2), SET_BIT(g2),
		SET_BIT(b3), 0, 0, 0, 0, 0, 0, SET_BIT(g3),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		SET_BIT(b5), 0, 0, 0, 0, 0, 0, SET_BIT(g5),
		SET_BIT(b6), 0, 0, 0, 0, 0, 0, SET_BIT(g6),
		SET_BIT(b7), SET_BIT(c7), 0, 0, 0, 0, SET_BIT(f7), SET_BIT(g7)
	},
	{
		SET_BIT(b2), SET_BIT(c2), 0, 0, 0, 0, SET_BIT(f2), SET_BIT(g2),
		SET_BIT(b3), 0, 0, 0, 0, 0, 0, SET_BIT(g3),
		SET_BIT(b4), 0, 0, 0, 0, 0, 0, SET_BIT(g4),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		SET_BIT(b6), 0, 0, 0, 0, 0, 0, SET_BIT(g6),
		SET_BIT(b7), SET_BIT(c7), 0, 0, 0, 0, SET_BIT(f7), SET_BIT(g7)
	}
};
//-------------------------------------------//
inline constexpr uint64_t well_trapped_bishop[64] =
{
	SET_BIT(b3), SET_BIT(b3), 0, 0, 0, 0, SET_BIT(g3), SET_BIT(g3),
	SET_BIT(b3), 0, 0, 0, 0, 0, 0, SET_BIT(f2),
	SET_BIT(b4), 0, 0, 0, 0, 0, 0, SET_BIT(f3),
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	SET_BIT(c6), 0, 0, 0, 0, 0, 0, SET_BIT(f6),
	SET_BIT(c7), 0, 0, 0, 0, 0, 0, SET_BIT(f7),
	SET_BIT(b6), SET_BIT(b6), 0, 0, 0, 0, SET_BIT(g6), SET_BIT(g6)
};
//-------------------------------------------//
inline constexpr uint64_t knight_opposing_bishop[2][64] =
{
	{
		0, 0, 0, 0, 0, 0, 0, 0,
		SET_BIT(a5), SET_BIT(b5), SET_BIT(c5), SET_BIT(d5), SET_BIT(e5), SET_BIT(f5), SET_BIT(g5), SET_BIT(h5),
		SET_BIT(a6), SET_BIT(b6), SET_BIT(c6), SET_BIT(d6), SET_BIT(e6), SET_BIT(f6), SET_BIT(g6), SET_BIT(h6),
		SET_BIT(a7), SET_BIT(b7), SET_BIT(c7), SET_BIT(d7), SET_BIT(e7), SET_BIT(f7), SET_BIT(g7), SET_BIT(h7),
		SET_BIT(a8), SET_BIT(b8), SET_BIT(c8), SET_BIT(d8), SET_BIT(e8), SET_BIT(f8), SET_BIT(g8), SET_BIT(h8),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	},
	{
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		SET_BIT(a1), SET_BIT(b1), SET_BIT(c1), SET_BIT(d1), SET_BIT(e1), SET_BIT(f1), SET_BIT(g1), SET_BIT(h1),
		SET_BIT(a2), SET_BIT(b2), SET_BIT(c2), SET_BIT(d2), SET_BIT(e2), SET_BIT(f2), SET_BIT(g2), SET_BIT(h2),
		SET_BIT(a3), SET_BIT(b3), SET_BIT(c3), SET_BIT(d3), SET_BIT(e3), SET_BIT(f3), SET_BIT(g3), SET_BIT(h3),
		SET_BIT(a4), SET_BIT(b4), SET_BIT(c4), SET_BIT(d4), SET_BIT(e4), SET_BIT(f4), SET_BIT(g4), SET_BIT(h4),
		0, 0, 0, 0, 0, 0, 0, 0
	}
};
//-------------------------------------------//
inline constexpr uint64_t knight_opposing_bishop_left_right[64] =
{
	0, 0, 0, SET_BIT(a1), SET_BIT(h1), 0, 0, 0,
	0, 0, 0, SET_BIT(a2), SET_BIT(h2), 0, 0, 0,
	0, 0, 0, SET_BIT(a3), SET_BIT(h3), 0, 0, 0,
	0, 0, 0, SET_BIT(a4), SET_BIT(h4), 0, 0, 0,
	0, 0, 0, SET_BIT(a5), SET_BIT(h5), 0, 0, 0,
	0, 0, 0, SET_BIT(a6), SET_BIT(h6), 0, 0, 0,
	0, 0, 0, SET_BIT(a7), SET_BIT(h7), 0, 0, 0,
	0, 0, 0, SET_BIT(a8), SET_BIT(h8), 0, 0, 0
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
constexpr score make_score(const int mg, const int eg)
{
	return static_cast<score>(static_cast<int>(static_cast<unsigned>(eg) << 16) + mg);
}
//-------------------------------------------//
constexpr score operator+(const score d1, const score d2) { return static_cast<score>(static_cast<int>(d1) + static_cast<int>(d2)); }
//-------------------------------------------//
constexpr score operator-(const score d1, const score d2) { return static_cast<score>(static_cast<int>(d1) - static_cast<int>(d2)); }
//-------------------------------------------//
constexpr score operator-(const score d) { return static_cast<score>(-static_cast<int>(d)); }
//-------------------------------------------//
constexpr score& operator+=(score& d1, const score d2) { return d1 = d1 + d2; }
//-------------------------------------------//
constexpr score& operator-=(score& d1, const score d2) { return d1 = d1 - d2; }
//-------------------------------------------//
