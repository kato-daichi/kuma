//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <cstring>
#include "position.h"
#include "search.h"
#include "thread.h"
//-------------------------------------------//
void clear_threads()
{
	for (int i = 0; i < num_threads; ++i)
	{
		const auto search_thread = static_cast<searchthread*>(get_thread(i));
		std::memset(&search_thread->history_table, 0, sizeof search_thread->history_table);
		for (int j = 0; j < 14; ++j)
		{
			for (int k = 0; k < 64; ++k)
			{
				for (int l = 0; l < 14; ++l)
				{
					for (int m = 0; m < 64; ++m)
					{
						search_thread->counter_move_history[j][k][l][m] = j < 2 ? -1 : 0;
					}
				}
			}
		}
		for (auto& j : search_thread->counter_move_table)
		{
			for (auto& k : j)
			{
				k = move_none;
			}
		}
	}
}
//-------------------------------------------//
void get_ready()
{
	main_thread.root_height = main_thread.position.history_index;
	for (int i = 1; i < num_threads; ++i)
	{
		const auto t = static_cast<searchthread*>(get_thread(i));
		t->root_height = main_thread.root_height;
		memcpy(&t->position, &main_thread.position, sizeof(Position));
		t->position.my_thread = t;
	}
	for (int i = 0; i < num_threads; i++)
	{
		const auto t = static_cast<searchthread*>(get_thread(i));
		t->do_nmp = true;
		for (auto& s : t->ss)
		{
			searchinfo* info = &s;
			info->pv[0] = move_none;
			info->pv_len = 0;
			info->ply = 0;
			info->singular_extension = false;
			info->chosen_move = move_none;
			info->excluded_move = move_none;
			info->static_eval = undefined;
			info->killers[0] = info->killers[1] = move_none;
			info->counter_move_history = &t->counter_move_history[no_piece][0];
		}
		memset(&t->pawn_table, 0, sizeof t->pawn_table);
	}
}
//-------------------------------------------//
void* get_thread(const int thread_id) { return thread_id == 0 ? &main_thread : &search_threads[thread_id - 1]; }
//-------------------------------------------//
void init_threads()
{
	search_threads = new searchthread[num_threads - 1];
	for (int i = 0; i < num_threads; ++i)
	{
		static_cast<searchthread*>(get_thread(i))->thread_id = static_cast<uint16_t>(i);
	}
	clear_threads();
}
//-------------------------------------------//
void reset_threads(const int thread_num)
{
	num_threads = thread_num;
	delete[] search_threads;
	search_threads = new searchthread[num_threads - 1];
	for (int i = 1; i < thread_num; ++i)
	{
		static_cast<searchthread*>(get_thread(i))->thread_id = static_cast<uint16_t>(i);
	}
	clear_threads();
	get_ready();
}
//-------------------------------------------//
