//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <iostream>
#include <sstream>
#include <vector>
#include "position.h"
#include "search.h"
#include "chrono.h"
#include "thread.h"
#include "util.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/mman.h>
#endif
//-------------------------------------------//
void* alloc_aligned_mem(const size_t alloc_size, void*& mem)
{
	constexpr size_t alignment = 64;
	const size_t size = alloc_size + alignment - 1;
	mem = malloc(size);
	const auto ret = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mem) + alignment - 1 & ~(alignment - 1));
	return ret;
}
//-------------------------------------------//
std::vector<std::string> split_string(const char* c)
{
	const std::string ss(c);
	std::vector<std::string> out;
	std::istringstream iss(ss);
	for (std::string s; iss >> s;)
		out.push_back(s);
	return out;
}
//-------------------------------------------//
unsigned char algebraic_to_index(const std::string& s)
{
	const char file = s[0] - 'a';
	const char rank = s[1] - '1';
	return rank << 3 | file;
}
//-------------------------------------------//
piece_type get_piece_type(const char c)
{
	switch (c)
	{
	case 'n':
	case 'N':
		return knight;
	case 'b':
	case 'B':
		return bishop;
	case 'r':
	case 'R':
		return rook;
	case 'q':
	case 'Q':
		return queen;
	case 'k':
	case 'K':
		return king;
	default:
		break;
	}
	return blanktype;
}
//-------------------------------------------//
char piece_char(const piece_code c, const bool lower)
{
	const auto p = static_cast<piece_type>(c >> 1);
	const int col = c & 1;
	char o{};
	switch (p)
	{
	case pawn:
		o = 'p';
		break;
	case knight:
		o = 'n';
		break;
	case bishop:
		o = 'b';
		break;
	case rook:
		o = 'r';
		break;
	case queen:
		o = 'q';
		break;
	case king:
		o = 'k';
		break;
	case blanktype:
		break;
	default:
		o = ' ';
		break;
	}
	if (!lower && !col)
		o = static_cast<char>(o + ('A' - 'a'));
	return o;
}
//-------------------------------------------//
std::string move_to_str(const move code)
{
	char s[100];
	if (code == 65 || code == 0)
		return "none";
	const int from = from_sq(code);
	int to = to_sq(code);
	char prom_char = 0;
	if (type_of(code) == promotion)
		prom_char = piece_char(static_cast<piece_code>(promotion_type(code) * 2), true);
	if (type_of(code) == castling)
	{
		const int side = from > 56 ? 1 : 0;
		const int castle_index = 2 * side + (to > from ? 1 : 0);
		to = castle_king_to[castle_index];
	}
	sprintf(s, "%c%d%c%d%c", (from & 0x7) + 'a', (from >> 3 & 0x7) + 1, (to & 0x7) + 'a', (to >> 3 & 0x7) + 1, prom_char);
	return s;
}
//-------------------------------------------//
void bench()
{
	uint64_t nodes = 0;
	const int bench_start = time_now();
	is_timeout = false;
	limits.moves_to_go = 0;
	limits.time_left = 0;
	limits.increment = 0;
	limits.move_time = 0;
	limits.depth_limit = 16;
	limits.time_is_limited = false;
	limits.depth_is_limited = true;
	limits.infinite = false;
	for (int i = 0; i < 41; i++)
	{
		std::cout << "\nPosition " << i + 1 << "/41" << std::endl;
		clear_threads();
		clear_hash();
		Position* p = import_fen(fen_positions[i].c_str(), 0);
		get_ready();
		think(p);
		nodes += main_thread.nodes;
	}
	const int time_taken = time_now() - bench_start;
	std::cout << std::endl;
	std::cout << "Time  " << time_taken << std::endl;
	std::cout << "Nodes " << nodes << std::endl;
	std::cout << "NPS   " << nodes * 1000 / (static_cast<unsigned long long>(time_taken) + 1) << std::endl;
}
//-------------------------------------------//