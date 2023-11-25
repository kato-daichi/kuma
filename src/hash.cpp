//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "position.h"
#include "hash.h"
#include "util.h"
//-------------------------------------------//
#pragma warning(disable: 4244)
//-------------------------------------------//
hash_table hash;
//-------------------------------------------//
int age_diff(const hash_entry* entry)
{
	return hash.generation - hashentry_age(entry) & 0x3F;
}
//-------------------------------------------//
void clear_hash()
{
	memset(hash.table, 0, hash.table_size);
	hash.generation = 0;
}
//-------------------------------------------//
int hashfull()
{
	int cnt = 0;
	for (int i = 0; i < 1000 / 3; i++) {
		for (const hash_bucket* bucket = &hash.table[i]; const auto & entry : bucket->entries)
		{
			if (hashentry_age(&entry) == hash.generation) {
				++cnt;
			}
		}
	}
	return cnt * 1000 / (3 * (1000 / 3));
}
//-------------------------------------------//
int hash_to_score(const int score, const uint16_t ply)
{
	if (score >= mate_in_max_ply)
	{
		return score - ply;
	}
	if (score <= mated_in_max_ply)
	{
		return score + ply;
	}
	return score;
}
//-------------------------------------------//
void init_hash()
{
	constexpr size_t mb = hash_mb;
	hash.table_size = mb * 1024 * 1024;
	hash.size_mask = hash.table_size / sizeof(hash_bucket) - 1;
	hash.table = static_cast<hash_bucket*>(alloc_aligned_mem(hash.table_size, hash.mem));
	clear_hash();
}
//-------------------------------------------//
hash_entry* probe_hash(const uint64_t key, bool& hash_hit)
{
	const uint64_t index = key & hash.size_mask;
	hash_bucket* bucket = &hash.table[index];
	const auto upper = static_cast<uint16_t>(key >> 48);
	for (auto& entry : bucket->entries)
	{
		if (entry.hashupper == upper)
		{
			hash_hit = true;
			return &entry;
		}
		if (!entry.hashupper)
		{
			hash_hit = false;
			return &entry;
		}
	}
	hash_entry* cheapest = &bucket->entries[0];
	for (int i = 1; i < 3; i++)
	{
		if (bucket->entries[i].depth - age_diff(&bucket->entries[i]) * 16 < cheapest->depth - age_diff(cheapest) * 16)
			cheapest = &bucket->entries[i];
	}
	hash_hit = false;
	return cheapest;
}
//-------------------------------------------//
void reset_hash(const int mb)
{
	free(hash.mem);
	hash.table_size = static_cast<uint64_t>(mb) * 1024 * 1024;
	hash.size_mask = hash.table_size / sizeof(hash_bucket) - 1;
	hash.table = static_cast<hash_bucket*>(alloc_aligned_mem(hash.table_size, hash.mem));
	if (!hash.mem)
	{
		exit(EXIT_FAILURE);
	}
	clear_hash();
}
//-------------------------------------------//
void save_entry(hash_entry* entry, const uint64_t key, const move m, const int depth, const int score, const int static_eval, const uint8_t flag)
{
	const auto upper = static_cast<uint16_t>(key >> 48);
	if (m || upper != entry->hashupper)
		entry->movecode = m;
	if (upper != entry->hashupper || depth > entry->depth - 4)
	{
		entry->hashupper = upper;
		entry->depth = static_cast<int8_t>(depth);
		entry->flags = static_cast<uint8_t>(hash.generation << 2 | flag);
		entry->static_eval = static_cast<int16_t>(static_eval);
		entry->value = static_cast<int16_t>(score);
	}
}
//-------------------------------------------//
int score_to_hash(const int score, const uint16_t ply)
{
	if (score >= mate_in_max_ply)
	{
		return score + ply;
	}
	if (score <= mated_in_max_ply)
	{
		return score - ply;
	}
	return score;
}
//-------------------------------------------//
void start_search()
{
	hash.generation = (hash.generation + 1) % 64;
}
//-------------------------------------------//
