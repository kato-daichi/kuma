#include "hash.h"
#include "zobrist.h"
//-------------------------------------------//
zobrist::zobrist()
{
	int i;
	uint64_t c[4]{};
	uint64_t ep[8]{};
	for (i = 0; i < 4; i++)
		c[i] = prng::rand();
	for (i = 0; i < 8; i++)
		ep[i] = prng::rand();
	for (i = 0; i < 16; i++)
	{
		castle[i] = 0ULL;
		for (int j = 0; j < 4; j++)
		{
			if (i & 1 << j)
				castle[i] ^= c[j];
		}
	}
	for (i = 0; i < 64; i++)
	{
		ep_squares[i] = 0ULL;
		if (RANK(i) == 2 || RANK(i) == 5)
			ep_squares[i] = ep[FILE(i)];
	}
	for (i = 0; i < 64 * 16; i++)
		piece_keys[i] = prng::rand();
	active_side = prng::rand();
}
//-------------------------------------------//
uint64_t zobrist::get_hash(const Position* pos) const
{
	uint64_t out = 0ULL;
	for (int i = w_pawn; i <= b_king; i++)
	{
		uint64_t pieces = pos->piece_bb[i];
		while (pieces)
		{
			const unsigned int index = pop_lsb(pieces);
			out ^= piece_keys[index << 4 | i];
		}
	}
	if (pos->active_side)
		out ^= active_side;
	out ^= castle[pos->castle_rights];
	out ^= ep_squares[pos->ep_square];
	return out;
}
//-------------------------------------------//
uint64_t zobrist::get_pawn_hash(const Position* pos) const
{
	uint64_t out = 0ULL;
	for (int i = w_pawn; i <= b_pawn; i++)
	{
		uint64_t pawns = pos->piece_bb[i];
		while (pawns)
		{
			const unsigned int index = pop_lsb(pawns);
			out ^= piece_keys[index << 4 | i];
		}
	}
	out ^= piece_keys[pos->kingpos[0] << 4 | w_king] ^ piece_keys[pos->kingpos[1] << 4 | b_king];
	return out;
}
//-------------------------------------------//