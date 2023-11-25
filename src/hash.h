//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <cstdint>
#include <random>
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
	return tte->flags & 0x3;
}
//-------------------------------------------//
inline uint8_t hashentry_age(const hash_entry* tte)
{
	return tte->flags >> 2;
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
	static uint64_t rand64()
	{
		std::random_device rd;
		std::mt19937_64 gen(rd());
		std::uniform_int_distribution<uint64_t> dis;
		return dis(gen);
	}
public:
	uint64_t s;
	explicit prng(const uint64_t seed) : s(seed)
	{
	}
	static uint64_t rand() { return rand64(); }
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
