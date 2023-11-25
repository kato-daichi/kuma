//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <algorithm>
#include "position.h"
#include "chrono.h"
//-------------------------------------------//
int time_now()
{
#if defined(_WIN64)
	return static_cast<int>(GetTickCount64());
#else
	struct timeval tv;
	int secsInMilli, usecsInMilli;
	gettimeofday(&tv, NULL);
	secsInMilli = (tv.tv_sec) * 1000;
	usecsInMilli = tv.tv_usec / 1000;
	return secsInMilli + usecsInMilli;
#endif
}
//-------------------------------------------//
time_tuple calculate_time()
{
	int optimaltime;
	int maxtime;
	if (limits.time_is_limited)
	{
		return {limits.move_time, limits.move_time};
	}
	if (limits.moves_to_go == 1)
	{
		return {limits.time_left - move_overhead, limits.time_left - move_overhead};
	}
	if (limits.moves_to_go == 0)
	{
		optimaltime = limits.time_left / 50 + limits.increment;
		maxtime = std::min(6 * optimaltime, limits.time_left / 4);
	}
	else
	{
		const int movestogo = limits.moves_to_go;
		optimaltime = limits.time_left / (movestogo + 5) + limits.increment;
		maxtime = std::min(6 * optimaltime, limits.time_left / 4);
	}
	optimaltime = std::min(optimaltime, limits.time_left - move_overhead);
	maxtime = std::min(maxtime, limits.time_left - move_overhead);
	return {optimaltime, maxtime};
}
//-------------------------------------------//