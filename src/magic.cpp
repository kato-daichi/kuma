//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "board.h"
#include "position.h"
#include "magic.h"
//-------------------------------------------//
uint64_t bishop_attacks_slow(const uint64_t occ, const int sq)
{
	return sliding_attacks_slow(occ, sq, diag_masks, flip_vertical) | sliding_attacks_slow(occ, sq, antidiag_masks, flip_vertical);
}
//-------------------------------------------//
uint64_t flip_vertical(uint64_t x)
{
	constexpr uint64_t y = 0x00FF00FF00FF00FF;
	constexpr uint64_t z = 0x0000FFFF0000FFFF;
	x = ((x >> 8) & y) | ((x & y) << 8);
	x = ((x >> 16) & z) | ((x & z) << 16);
	x = (x >> 32) | (x << 32);
	return x;
}
//-------------------------------------------//
void init_magics(uint64_t attack_table[], s_magic magics[], const uint64_t* masks, uint64_t(*func)(uint64_t, int), const uint64_t* magic_numbers)
{
	int size = 0;
	for (int i = 0; i < 64; i++)
	{
		s_magic& m = magics[i];
		m.mask = masks[i];
		m.shift = 64 - popcnt(m.mask);
		m.ptr = i == 0 ? attack_table : magics[i - 1].ptr + size;
		m.magic = magic_numbers[i];
		uint64_t n = size = 0;
		do
		{
			m.ptr[m.index(n)] = func(n, i);
			size++;
			n = n - m.mask & m.mask;
		} while (n);
	}
}
//-------------------------------------------//
uint64_t rankattacks(const uint64_t occ, const int sq)
{
	const unsigned int file = sq & 7;
	const unsigned int rkx8 = sq & 56;
	const unsigned int rank_occ_x2 = occ >> rkx8 & static_cast<unsigned long long>(2) * 63;       
	const uint64_t attacks = rank_attacks[4 * rank_occ_x2 + file];         
	return attacks << rkx8;
}
//-------------------------------------------//
uint64_t rook_attacks_slow(const uint64_t occ, const int sq)
{
	return rankattacks(occ, sq) | sliding_attacks_slow(occ, sq, file_masks, flip_vertical);
}
//-------------------------------------------//
uint64_t sliding_attacks_slow(const uint64_t occ, const int sq, const uint64_t* masks, uint64_t(*func)(uint64_t))
{
	uint64_t f = occ & masks[sq];
	uint64_t r = func(f);
	f -= square_masks[sq];
	r -= square_masks[sq ^ 56];
	const uint64_t r2 = func(r);
	f ^= r2;
	f &= masks[sq];
	return f;
}
//-------------------------------------------//