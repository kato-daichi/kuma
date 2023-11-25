//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "position.h"
#include "hash.h"
#include "uci.h"
#include "util.h"
#include "nnue.h"
//-------------------------------------------//
int main(const int argc, char** argv)
{
	Position::init();
	init_hash();
	nnue_init(nnue_file);
	if (argc > 1 && strstr(argv[1], "bench"))
		bench();
	else
		loop();
	return 0;
}
//-------------------------------------------//