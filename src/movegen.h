#pragma once
#include "move.h"
//-------------------------------------------//
class move_gen {
 public:
  int depth;
  int state;
  int threshold;
  move counter_move;
  move hashmove;
  move next_move(const searchinfo* info, bool skip_quiets = false);
  move_gen(Position* p, search_type type, move hshm, int t, int d);
  s_move move_list[256]{};
  s_move *curr{}, *end_moves{}, *end_bad_captures{};
  template <pick_type Type>
  move select_move();
  void score_moves(const searchinfo* info, score_type type) const;

 private:
  Position* pos_;
  [[nodiscard]] s_move* begin() const { return curr; }
  [[nodiscard]] s_move* end() const { return end_moves; }
};
//-------------------------------------------//