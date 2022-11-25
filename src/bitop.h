//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#  include <sys/time.h>
#endif

constexpr uint64_t k1 = 0x5555555555555555;
constexpr uint64_t k2 = 0x3333333333333333;
constexpr uint64_t k4 = 0x0f0f0f0f0f0f0f0f;
constexpr uint64_t kf = 0x0101010101010101;
//-------------------------------------------//
inline int popcnt(uint64_t bb)
{
	bb = bb - (bb >> 1 & k1);
	bb = (bb & k2) + (bb >> 2 & k2);
	bb = bb + (bb >> 4) & k4;
	bb = bb * kf >> 56;
	return static_cast<int>(bb);
}
//-------------------------------------------//
const int index64[64] = {
	0, 47, 1, 56, 48, 27, 2, 60,
	57, 49, 41, 37, 28, 16, 3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11, 4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30, 9, 24,
	13, 18, 8, 12, 7, 6, 5, 63
};
//-------------------------------------------//
inline int lsb(const uint64_t bb)
{
	return index64[0x03f79d71b4cb0a89 * (bb ^ bb - 1) >> 58];
}
//-------------------------------------------//
inline int msb(uint64_t bb)
{
	constexpr uint64_t debruijn64 = 0x03f79d71b4cb0a89UL;
	bb |= bb >> 1;
	bb |= bb >> 2;
	bb |= bb >> 4;
	bb |= bb >> 8;
	bb |= bb >> 16;
	bb |= bb >> 32;
	return index64[bb * debruijn64 >> 58];
}
//-------------------------------------------//
inline int pop_lsb(uint64_t& bb)
{
	const uint64_t x = bb;
	bb &= bb - 1;
	return lsb(x);
}
//-------------------------------------------//