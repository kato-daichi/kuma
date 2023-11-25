//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "position.h"
#include "move.h"
#include "search.h"
#include "chrono.h"
#include "uci.h"
#include "util.h"
#include "thread.h"
//-------------------------------------------//
std::vector<std::string> args;
Position* root_position = &main_thread.position;
Position position;
//-------------------------------------------//
#pragma warning(disable: 4127)
#pragma warning(disable: 4554)
//-------------------------------------------//
void cmd_fen()
{
	const std::string fen = args[2] + " " + args[3] + " " + args[4] + " " + args[5] + " " + args[6] + " " + args[7];
	position.read_fen(fen.c_str());
	if (token_equals(8, "moves"))
	{
		for (unsigned i = 9; i < args.size(); i++)
		{
			move m = move_none;
			if (args[i].length() == 4 || args[i].length() == 5)
			{
				m = uci2_move(&position, args[i]);
				if (args[i].length() == 5)
				{
					const piece_type promote = get_piece_type(args[i][4]);
					m = static_cast<move>(m | (promote - knight) << 12);
				}
			}
			if (m != move_none && position.is_pseudo_legal(m))
			{
				position.do_move(m);
				if (position.halfmove_clock == 0)
					position.history_index = 0;
			}
		}
	}
}
//-------------------------------------------//
void cmd_position()
{
	if (args[1] == "fen")
		cmd_fen();
	if (args[1] == "startpos")
		startpos();
}
//-------------------------------------------//
void go()
{
	prepare_threads();
	depth_limit = max_ply;
	memset(&limits, 0, sizeof(time_info));
	if (args.size() <= 1)
	{
		limits.moves_to_go = 0;
		limits.time_left = 10000;
		limits.increment = 0;
		limits.move_time = 0;
		limits.depth_limit = 0;
		limits.time_is_limited = true;
		limits.depth_is_limited = false;
		limits.infinite = false;
	}
	else
	{
		int movestogo = 0;
		int binc = 0;
		int winc = 0;
		int movetime = 0;
		int btime = 0;
		bool depthlimited = false;
		int wtime = 0;
		bool timelimited = false;
		bool infinite = false;
		int depth = 0;
		for (unsigned int i = 1; i < args.size(); ++i)
		{
			if (args[i] == "wtime")
			{
				wtime = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]);
			}
			else if (args[i] == "btime")
			{
				btime = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]);
			}
			else if (args[i] == "winc")
			{
				winc = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]);
			}
			else if (args[i] == "binc")
			{
				binc = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]);
			}
			else if (args[i] == "movestogo")
			{
				movestogo = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]);
			}
			else if (args[i] == "depth")
			{
				depthlimited = true;
				depth = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]);
			}
			else if (args[i] == "ponder")
			{
				is_pondering = true;
			}
			else if (args[i] == "infinite")
			{
				infinite = true;
			}
			else if (args[i] == "movetime")
			{
				movetime = stoi(args[static_cast<std::vector<std::string>::size_type>(i) + 1]) * 99 / 100;
				timelimited = true;
			}
		}
		limits.moves_to_go = movestogo;
		limits.time_left = root_position->active_side == white ? wtime : btime;
		limits.increment = root_position->active_side == white ? winc : binc;
		limits.move_time = movetime;
		limits.depth_limit = depth;
		limits.time_is_limited = timelimited;
		limits.depth_is_limited = depthlimited;
		limits.infinite = infinite;
	}
	std::thread think_thread(think, root_position);
	think_thread.detach();
}
//-------------------------------------------//
void isready()
{
	std::cout << "readyok" << std::endl;
}
//-------------------------------------------//
void loop()
{
	std::string input;
	position.read_fen(start_fen);
	prepare_threads();
	while (true)
	{
		getline(std::cin, input);
		args = split_words(input);
		if (!args.empty())
		{
			run(args[0]);
		}
	}
}
//-------------------------------------------//
void option(const std::string& name, const std::string& value)
{
	if (name == "Hash")
	{
		const int mb = stoi(value);
		if (!mb || MORE_THAN_ONE(uint64_t(mb)))
		{
		}
		reset_hash(mb);
	}
	else if (name == "Threads")
	{
		reset_threads(std::min(max_threads, std::max(1, stoi(value))));
	}
	else if (name == "MoveOverhead")
	{
		move_overhead = stoi(value);
	}
}
//-------------------------------------------//
template <bool Root>
uint64_t perft(Position& pos, const int depth)
{
	uint64_t cnt, nodes = 0;
	const bool leaf = depth == 2;
	for (const auto& m : move_list<all>(pos))
	{
		if (Root && depth <= 1)
			cnt = 1, nodes++;
		else
		{
			pos.do_move(m);
			cnt = leaf ? move_list<all>(pos).size() : perft<false>(pos, depth - 1);
			nodes += cnt;
			pos.undo_move(m);
		}
		if (Root)
			std::cout << m.to_string() << " " << cnt << std::endl;
	}
	return nodes;
}
//-------------------------------------------//
void perft()
{
	prepare_threads();
	const int start = time_now();
	const uint64_t nodes = perft<true>(*root_position, stoi(args[1]));
	const int elapsed = time_now() - start;
	std::cout << "nodes " << nodes << std::endl;
	std::cout << "time  " << elapsed << " ms" << std::endl;
	std::cout << "speed " << nodes / elapsed * 1000 << " nps" << std::endl;
}
//-------------------------------------------//
void ponderhit()
{
	is_pondering = false;
}
//-------------------------------------------//
void prepare_threads()
{
	memcpy(&main_thread.position, &position, sizeof(Position));
	main_thread.position.my_thread = &main_thread;
	get_ready();
}
//-------------------------------------------//
void run(const std::string& s)
{
	if (s == "ucinewgame")
		ucinewgame();
	if (s == "position")
		cmd_position();
	if (s == "go")
		go();
	if (s == "setoption")
		setoption();
	if (s == "isready")
		isready();
	if (s == "uci")
		uci();
	if (s == "ponderhit")
		ponderhit();
	if (s == "stop")
		stop();
	if (s == "bench")
		bench();
	if (s == "perft")
		perft();
	if (s == "quit")
		exit(0);
}
//-------------------------------------------//
void setoption()
{
	if (args[1] != "name" || args[3] != "value")
		return;
	const std::string name = args[2];
	const std::string value = args[4];
	option(name, value);
}
//-------------------------------------------//
std::vector<std::string> split_words(const std::string& s)
{
	std::vector<std::string> tmp;
	unsigned l_index = 0;
	for (unsigned i = 0; i < s.length(); i++)
	{
		if (s[i] == ' ')
		{
			tmp.push_back(s.substr(l_index, i - l_index));
			l_index = i + 1;
		}
		if (i == s.length() - 1)
		{
			tmp.push_back(s.substr(l_index));
		}
	}
	return tmp;
}
//-------------------------------------------//
void startpos()
{
	position.read_fen(start_fen);
	if (token_equals(2, "moves"))
	{
		for (unsigned i = 3; i < args.size(); i++)
		{
			move m = move_none;
			if (args[i].length() == 4 || args[i].length() == 5)
			{
				m = uci2_move(&position, args[i]);
				if (args[i].length() == 5)
				{
					const piece_type promote = get_piece_type(args[i][4]);
					m = static_cast<move>(m | promote - knight << 12);
				}
			}
			if (m != move_none && position.is_pseudo_legal(m))
			{
				position.do_move(m);
				if (position.halfmove_clock == 0)
					position.history_index = 0;
			}
		}
	}
}
//-------------------------------------------//
void stop()
{
	is_timeout = true;
	is_pondering = false;
}
//-------------------------------------------//
bool token_equals(const int index, const std::string& comparison_str)
{
	if (args.size() > static_cast<unsigned>(index))
		return args[index] == comparison_str;
	return false;
}
//-------------------------------------------//
void uci()
{
	std::cout << "id name " << engine_name << " " << version << std::endl << "id author " << author << std::endl;
	std::cout << "option name Hash type spin default " << hash_mb << " min 1 max 65536" << std::endl;
	std::cout << "option name Threads type spin default 1 min 1 max " << max_threads << std::endl;
	std::cout << "uciok" << std::endl;
}
//-------------------------------------------//
move uci2_move(const Position* pos, const std::string& s)
{
	const int from = s[0] - 'a' + 8 * (s[1] - '1');
	int to = s[2] - 'a' + 8 * (s[3] - '1');
	const piece_code pc = pos->mailbox[from];
	special_type type = normal;
	if (pc == w_pawn || pc == b_pawn)
	{
		if (RANK(to) == 0 || RANK(to) == 7)
			type = promotion;
		else if (to == pos->ep_square)
			type = enpassant;
	}
	else if (pc >= w_king && (abs(from - to) == 2 || SET_BIT(to) & pos->piece_bb[make_piece(pos->active_side, rook)]))
	{
		if (abs(from - to) == 2)
			to = to > from ? to + 1 : to - 2;
		type = castling;
	}
	return make_move(from, to, type);
}
//-------------------------------------------//
void ucinewgame()
{
	clear_threads();
	clear_hash();
}
//-------------------------------------------//