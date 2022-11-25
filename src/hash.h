//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <cstdint>
#include "enum.h"

inline constexpr int hash_mb = 128;
inline constexpr int material_entries = 65536;
inline constexpr int material_hash_size_mask = material_entries - 1;
inline constexpr int pawn_entries = 16384;
inline constexpr int pawn_hash_size_mask = pawn_entries - 1;
inline constexpr uint8_t flag_alpha = 1;
inline constexpr uint8_t flag_beta = 2;
inline constexpr uint8_t flag_exact = 3;
//-------------------------------------------//
struct hash_entry
{
	int16_t static_eval;
	int16_t value;
	move movecode;
	uint16_t hashupper;
	uint8_t depth;
	uint8_t flags;
};
//-------------------------------------------//
inline uint8_t hashentry_flag(const hash_entry* tte)
{
	return static_cast<uint8_t>(tte->flags & 0x3);
}
//-------------------------------------------//
inline uint8_t hashentry_age(const hash_entry* tte)
{
	return static_cast<uint8_t>(tte->flags >> 2);
}
//-------------------------------------------//
struct hash_bucket
{
	hash_entry entries[3];
	char padding[2];
};
//-------------------------------------------//
struct hash_table
{
	hash_bucket* table;
	uint64_t size_mask;
	uint64_t table_size;
	uint8_t generation;
	void* mem;
};
//-------------------------------------------//
extern hash_table hash;
//-------------------------------------------//
class prng
{
	uint64_t s_;

	uint64_t rand64()
	{
		s_ ^= s_ >> 12;
		s_ ^= s_ << 25;
		s_ ^= s_ >> 27;
		return s_ * 2685821657736338717LL;
	}

public:
	explicit prng(const uint64_t seed) : s_(seed)
	{
	}
	uint64_t rand() { return rand64(); }
	uint64_t sparse_rand()
	{
		return rand64() & rand64() & rand64();
	}
};
//-------------------------------------------//
hash_entry* probe_hash(uint64_t key, bool& hash_hit);
int age_diff(const hash_entry* entry);
int hash_to_score(int score, uint16_t ply);
int hashfull();
int score_to_hash(int score, uint16_t ply);
void clear_hash();
void init_hash();
void reset_hash(int mb);
void save_entry(hash_entry* entry, uint64_t key, move m, int depth, int score, int static_eval, uint8_t flag);
void start_search();
