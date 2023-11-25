//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <cstdint>
//-------------------------------------------//
#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#  include <sys/time.h>
#endif
//-------------------------------------------//
#if defined(_MSC_VER)
#include <bit>
inline int popcnt(const uint64_t bb) { return std::popcount(bb); }
inline int lsb(const uint64_t bb) { return std::countr_zero(bb); }
#elif defined(__GNUC__)
inline int popcnt(uint64_t bb) { return __builtin_popcountll(bb); }
inline int lsb(uint64_t bb) { return __builtin_ctzll(bb); }
#endif
//-------------------------------------------//
inline int pop_lsb(uint64_t& bb)
{
	const uint64_t x = bb;
	bb &= bb - 1;
	return lsb(x);
}
//-------------------------------------------//