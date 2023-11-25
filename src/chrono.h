//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
//-------------------------------------------//
struct time_info
{
	bool depth_is_limited;
	bool infinite;
	bool time_is_limited;
	int depth_limit;
	int increment;
	int moves_to_go;
	int move_time;
	int time_left;
};
inline int ideal_usage;
inline int max_usage;
inline int move_overhead = 100;
inline int start_time;
inline int depth_limit;
inline int timer_count;
inline time_info limits;
inline timeval curr_time;
inline timeval start_ts;
int time_now();
//-------------------------------------------//
inline int time_elapsed()
{
	return time_now() - start_time;
}
//-------------------------------------------//
using time_tuple = struct timetuple
{
	int optimum_time;
	int maximum_time;
};
//-------------------------------------------//
time_tuple calculate_time();
//-------------------------------------------//