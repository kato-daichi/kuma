//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <algorithm>
#include "position.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "thread.h"
#include "movegen.h"
//-------------------------------------------//
move_gen::move_gen(Position* p, const search_type type, const move hshm,
                   const int t, const int d) {
  pos_ = p;
  depth = d;
  threshold = t;
  if (p->check_bb) {
    state = evasions_init;
    hashmove = move_none;
  } else {
    if (type == normal_search) {
      hashmove = hshm != move_none && pos_->is_pseudo_legal(hshm) &&
                         pos_->is_legal(hshm)
                     ? hshm
                     : move_none;
      state = hashmove_state;
    } else {
      hashmove = hshm != move_none && pos_->is_pseudo_legal(hshm) &&
                         pos_->is_legal(hshm) && pos_->is_tactical(hshm)
                     ? hshm
                     : move_none;
      state = type == quiescence_search ? q_hashmove : probcut_hashmove;
    }
    if (hashmove == move_none) {
      state++;
    }
  }
  const int prev_to = to_sq(p->move_stack[p->history_index]);
  counter_move = p->my_thread->counter_move_table[p->mailbox[prev_to]][prev_to];
}
//-------------------------------------------//
inline int quiet_score(const Position* pos, const searchinfo* info,
                       const move m) {
  const int from = from_sq(m);
  const int to = to_sq(m);
  const piece_code pc = pos->mailbox[from];
  return pos->my_thread->history_table[pos->active_side][from][to] +
         (*(info - 1)->counter_move_history)[pc][to] +
         (*(info - 2)->counter_move_history)[pc][to];
}
//-------------------------------------------//
inline int mvvlva_score(const Position* pos, const move m) {
  return mvvlva[pos->mailbox[to_sq(m)]][pos->mailbox[from_sq(m)]];
}
//-------------------------------------------//
void move_gen::score_moves(const searchinfo* info,
                           const score_type type) const {
  for (auto& [code, value] : *this) {
    if (type == score_capture) {
      value = mvvlva_score(pos_, code);
    } else if (type == score_quiet) {
      value = quiet_score(pos_, info, code);
    } else {
      if (pos_->is_capture(code))
        value = mvvlva_score(pos_, code);
      else
        value = quiet_score(pos_, info, code) - (1 << 20);
    }
    if (type_of(code) == promotion)
      value += mvvlva[make_piece(pos_->active_side, promotion_type(code))]
                     [pos_->mailbox[from_sq(code)]];
  }
}
//-------------------------------------------//
template <pick_type Type>
move move_gen::select_move() {
  if constexpr (Type == best)
    std::swap(*curr, *std::max_element(curr, end_moves));
  return *curr++;
}
//-------------------------------------------//
void insertion_sort(s_move* head, const s_move* tail) {
  for (s_move* i = head + 1; i < tail; i++) {
    const s_move tmp = *i;
    s_move* j;
    for (j = i; j != head && *(j - 1) < tmp; --j) {
      *j = *(j - 1);
    }
    *j = tmp;
  }
}
//-------------------------------------------//
move move_gen::next_move(const searchinfo* info, const bool skip_quiets) {
  move m;
  switch (state) {
    case hashmove_state:
      ++state;
      return hashmove;
    case tactical_init:
      curr = end_bad_captures = move_list;
      end_moves = generate_all<tactical>(*pos_, curr);
      score_moves(info, score_capture);
      ++state;
      [[fallthrough]];
    case tactical_state:
      while (curr < end_moves) {
        m = select_move<best>();
        if (m == hashmove) continue;
        if (pos_->see(m, 0)) return m;
        *end_bad_captures++ = m;
      }
      ++state;
      m = info->killers[0];
      if (m != move_none && m != hashmove && !pos_->is_tactical(m) &&
          pos_->is_pseudo_legal(m) && pos_->is_legal(m)) {
        return m;
      }
      [[fallthrough]];
    case killer_move_2:
      ++state;
      m = info->killers[1];
      if (m != move_none && m != hashmove && !pos_->is_tactical(m) &&
          pos_->is_pseudo_legal(m) && pos_->is_legal(m)) {
        return m;
      }
      [[fallthrough]];
    case countermove:
      ++state;
      m = counter_move;
      if (m != move_none && m != hashmove && m != info->killers[0] &&
          m != info->killers[1] && !pos_->is_tactical(m) &&
          pos_->is_pseudo_legal(m) && pos_->is_legal(m)) {
        return m;
      }
      [[fallthrough]];
    case quiets_init:
      if (!skip_quiets) {
        curr = end_bad_captures;
        end_moves = generate_all<quiet>(*pos_, curr);
        score_moves(info, score_quiet);
        insertion_sort(curr, end_moves);
      }
      ++state;
      [[fallthrough]];
    case quiet_state:
      if (!skip_quiets) {
        while (curr < end_moves) {
          m = select_move<next>();
          if (m != hashmove && m != info->killers[0] && m != info->killers[1] &&
              m != counter_move) {
            return m;
          }
        }
      }
      ++state;
      curr = move_list;
      end_moves = end_bad_captures;
      [[fallthrough]];
    case bad_tactical_state:
      while (curr < end_moves) {
        m = select_move<next>();
        if (m != hashmove) return m;
      }
      break;
    case evasions_init:
      curr = move_list;
      end_moves = generate_all<evasion>(*pos_, curr);
      score_moves(info, score_evasion);
      ++state;
      [[fallthrough]];
    case evasions_state:
      while (curr < end_moves) {
        return select_move<best>();
      }
      break;
    case q_hashmove:
      ++state;
      return hashmove;
    case q_captures_init:
      curr = end_bad_captures = move_list;
      end_moves = generate_all<tactical>(*pos_, curr);
      score_moves(info, score_capture);
      ++state;
      [[fallthrough]];
    case q_captures:
      while (curr < end_moves) {
        m = select_move<best>();
        if (m != hashmove) {
          return m;
        }
      }
      if (depth != 0) break;
      ++state;
      [[fallthrough]];
    case q_checks_init:
      curr = move_list;
      end_moves = generate_all<quiet_check>(*pos_, curr);
      ++state;
      [[fallthrough]];
    case q_checks:
      while (curr < end_moves) {
        m = select_move<next>();
        if (m != hashmove) return m;
      }
      break;
    case probcut_hashmove:
      ++state;
      return hashmove;
    case probcut_captures_init:
      curr = end_bad_captures = move_list;
      end_moves = generate_all<tactical>(*pos_, curr);
      score_moves(info, score_capture);
      ++state;
      [[fallthrough]];
    case probcut_captures:
      while (curr < end_moves) {
        m = select_move<best>();
        if (m != hashmove && pos_->see(m, threshold)) return m;
      }
      break;
    default:;
  }
  return move_none;
}
//-------------------------------------------//