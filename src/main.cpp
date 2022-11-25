//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "position.h"
#include "hash.h"
#include "uci.h"
#include "util.h"
//-------------------------------------------//
int main(const int argc, char** argv)
{
	Position::init();
	init_hash();

	if (argc > 1 && strstr(argv[1], "bench"))
		bench();
	else
		loop();
	return 0;
}
//-------------------------------------------//