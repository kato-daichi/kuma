//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <vector>
bool token_equals(int index, const std::string& comparison_str);
move uci2_move(const Position* pos, const std::string& s);
std::vector<std::string> split_words(const std::string& s);
template <bool Root>
uint64_t perft(Position& pos, int depth);
void cmd_fen();
void cmd_position();
void go();
void isready();
void loop();
void option(const std::string& name, const std::string& value);
void perft();
void ponderhit();
void prepare_threads();
void run(const std::string& s);
void setoption();
void startpos();
void stop();
void uci();
void ucinewgame();
