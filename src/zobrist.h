//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <cstdint>
#include "position.h"
//-------------------------------------------//
class zobrist
{
public:
	uint64_t active_side;
	uint64_t castle[16]{};
	uint64_t ep_squares[64]{};
	uint64_t get_hash(const Position* pos) const;
	uint64_t get_pawn_hash(const Position* pos) const;
	uint64_t piece_keys[64 * 16]{};
	zobrist();
};
//-------------------------------------------//
inline zobrist zb;
