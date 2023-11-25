//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include <array>
#include <iostream>
#include <thread>
#include "position.h"
#include "board.h"
#include "eval.h"
#include "move.h"
#include "chrono.h"
#include "thread.h"
#include "search.h"
//-------------------------------------------//
#ifdef _MSC_VER
#pragma warning(disable: 4100)
#pragma warning(disable: 4244)
#pragma warning(disable: 4611)
#pragma warning(disable: 5054)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma GCC diagnostic ignored "-Wclobbered"
#endif
//-------------------------------------------//
void add_history_bonus(int16_t* history, const int16_t bonus)
{
	*history += bonus - *history * abs(bonus) / 16384;
}
//-------------------------------------------//
inline bool is_valid(const move m)
{
	return from_sq(m) != to_sq(m);
}
//-------------------------------------------//
int alpha_beta(searchthread* thread, searchinfo* info, int depth, int alpha, int beta)
{
	if (depth < 1)
	{
		return q_search(thread, info, 0, alpha, beta);
	}
	int ply = static_cast<uint8_t>(info->ply);
	bool is_pv = beta - alpha > 1;
	if (is_pv)
	{
		info->pv_len = 0;
	}
	bool is_root = ply == 0;
	Position* pos = &thread->position;
	bool in_check = static_cast<bool>(pos->check_bb);
	if (thread->thread_id == 0)
		check_time(thread);
	if (!is_root)
	{
		if (is_timeout)
		{
			longjmp(thread->jbuffer, 1);
		}
		if (ply >= max_ply)
		{
			return in_check ? 0 : evaluate(*pos);
		}
		if (is_draw(pos))
		{
			return 1 - (thread->nodes & 2);
		}
		alpha = std::max(value_mated + ply, alpha);
		beta = std::min(value_mate - ply, beta);
		if (alpha >= beta)
		{
			return alpha;
		}
	}
	if (is_pv && ply > thread->seldepth)
		thread->seldepth = static_cast<int16_t>(ply);
	info->chosen_move = move_none;
	move excluded_move = info->excluded_move;
	uint64_t new_hash;
	new_hash = pos->key ^ static_cast<uint64_t>(excluded_move << 16);
	(info + 1)->killers[0] = (info + 1)->killers[1] = move_none;
	(info + 1)->ply = static_cast<int8_t>(ply + 1);
	move hash_move = move_none;
	int hash_score = undefined;
	bool tt_hit;
	hash_entry* tte = probe_hash(new_hash, tt_hit);
	uint8_t flag = hashentry_flag(tte);
	if (tt_hit)
	{
		hash_move = tte->movecode;
		if (tte->depth >= depth)
		{
			hash_score = hash_to_score(tte->value, static_cast<uint16_t>(ply));
			if (!is_pv &&
				hash_score != undefined &&
				(flag == flag_exact ||
					flag == flag_beta && hash_score >= beta ||
					flag == flag_alpha && hash_score <= alpha))
			{
				if (hash_move && !pos->is_tactical(hash_move))
				{
					if (hash_score >= beta)
					{
						save_killer(pos, info, hash_move, history_bonus(depth));
					}
					else
					{
						int16_t penalty = -history_bonus(static_cast<int16_t>(depth));
						add_history_bonus(
							&thread->history_table[pos->active_side][from_sq(hash_move)][to_sq(hash_move)], penalty);
						update_countermove_histories(info, pos->mailbox[from_sq(hash_move)], to_sq(hash_move), penalty);
					}
				}
				return hash_score;
			}
		}
	}
	bool is_null = (info - 1)->chosen_move == move_null;
	if (!in_check)
	{
		if (tt_hit && tte->static_eval != undefined)
		{
			info->static_eval = tte->static_eval;
		}
		else if (is_null)
		{
			info->static_eval = move_tempo * 2 - (info - 1)->static_eval;
		}
		else
		{
			info->static_eval = evaluate(*pos);
		}
		if (pos->game_cycle)
			info->static_eval *= std::max(0, 100 - pos->halfmove_clock) / 100;
	}
	else
	{
		info->static_eval = undefined;
	}
	if (!in_check && !is_pv && depth < 3 && info->static_eval <= alpha - razor_margin[depth])
	{
		return q_search(thread, info, 0, alpha, beta);
	}
	bool improving = !in_check && ply > 1 && (info->static_eval >= (info - 2)->static_eval || (info - 2)->static_eval ==
		undefined);
	if (!in_check && !is_pv && depth < 9 && info->static_eval - 80 * depth >= beta - improvement_value * improving &&
		pos->non_pawn[pos->active_side])
	{
		return info->static_eval;
	}
	if (!in_check && !is_pv && thread->do_nmp && !is_null &&
		excluded_move == move_none && info->static_eval >= beta - improvement_value * improving && depth > 2 && pos->
		non_pawn[pos->active_side] &&
		(!tt_hit || !(flag & flag_alpha) || hash_score >= beta))
	{
		int R = 3 + depth / 4 + std::min((info->static_eval - beta) / pawn_mg, 3);
		pos->do_null_move();
		info->chosen_move = move_null;
		info->counter_move_history = &thread->counter_move_history[no_piece][0];
		int null_score = -alpha_beta(thread, info + 1, depth - R, -beta, -beta + 1);
		pos->undo_null_move();
		if (null_score >= beta)
		{
			if (null_score >= mate_in_max_ply)
				null_score = beta;
			if (depth < 13)
			{
				return null_score;
			}
			thread->do_nmp = false;
			int v = alpha_beta(thread, info, depth - R, beta - 1, beta);
			thread->do_nmp = true;
			if (v >= beta)
			{
				return v;
			}
		}
	}
	move m;
	if (!in_check && !is_pv && depth > 4 && abs(beta) < mate_in_max_ply && info->static_eval + q_search_delta(pos) >=
		beta + probcut_margin)
	{
		int rbeta = std::min(beta + probcut_margin, static_cast<int>(value_mate));
		auto movegen = move_gen(pos, probcut_search, hash_move, rbeta - info->static_eval, depth);
		while ((m = movegen.next_move(info, depth)) != move_none)
		{
			if (m != excluded_move)
			{
				pos->do_move(m);
				info->chosen_move = m;
				int to = to_sq(m);
				piece_code pc = pos->mailbox[to];
				info->counter_move_history = &thread->counter_move_history[pc][to];
				int value = -q_search(thread, info + 1, 0, -rbeta, -rbeta + 1);
				if (value >= rbeta)
					value = -alpha_beta(thread, info + 1, depth - 4, -rbeta, -rbeta + 1);
				pos->undo_move(m);
				if (value >= rbeta)
					return value;
			}
		}
	}
	auto movegen = move_gen(pos, normal_search, hash_move, 0, depth);
	move best_move = move_none;
	int best_score = -value_inf;
	move quiets[64]{};
	int quiets_count = 0;
	int num_moves = 0;
	bool skip_quiets = false;
	info->singular_extension = false;
	while ((m = movegen.next_move(info, skip_quiets)) != move_none)
	{
		if (m == excluded_move)
			continue;
		num_moves++;
		bool gives_check = pos->gives_check(m);
		bool is_tactical = pos->is_tactical(m);
		int extension = 0;
		int16_t history, counter, followup;
		history_scores(pos, info, m, &history, &counter, &followup);
		if (!is_root && pos->non_pawn[pos->active_side] && best_score > mated_in_max_ply)
		{
			if (!gives_check && !is_tactical)
			{
				if (!is_root && depth <= 8 && num_moves >= futility_move_counts[improving][depth])
					skip_quiets = true;
				if (!is_root && depth <= 8 && !in_check && info->static_eval + pawn_mg * depth <= alpha &&
					history + counter + followup < futility_history_limit[improving])
					skip_quiets = true;
				if (!is_root && depth <= 5 && counter < 0 && followup < 0)
					continue;
				if (depth < 9 && !pos->see(m, -10 * depth * depth))
					continue;
			}
			else if (movegen.state > tactical_state && !pos->see(m, -pawn_eg * depth))
				continue;
		}
		if (depth >= singular_depth && m == hash_move && !is_root && excluded_move == move_none
			&& abs(hash_score) < won_endgame && flag & flag_beta && tte->depth >= depth - 2 && !pos->game_cycle)
		{
			int singular_beta = hash_score - 2 * depth;
			int half_depth = depth / 2;
			info->excluded_move = m;
			int singular_value = alpha_beta(thread, info, half_depth, singular_beta - 1, singular_beta);
			info->excluded_move = move_none;
			if (singular_value < singular_beta)
			{
				extension = 1;
				info->singular_extension = true;
			}
			else if (singular_beta >= beta)
				return singular_beta;
		}
		else
		{
			if (gives_check && pos->see(m, 0) || depth < 3 && pos->advanced_pawn_push(m))
				extension = 1;
		}
		pos->do_move(m);
		if (is_root && time_elapsed() > 3000)
			std::cout << "info depth " << depth << " currmove " << move_to_str(m) << " currmovenumber " << num_moves <<
				std::endl;
		thread->nodes++;
		info->chosen_move = m;
		int to = to_sq(m);
		piece_code pc = pos->mailbox[to];
		info->counter_move_history = &thread->counter_move_history[pc][to];
		int new_depth = depth - 1 + extension;
		int score;
		if (is_pv && num_moves == 1)
		{
			score = -alpha_beta(thread, info + 1, new_depth, -beta, -alpha);
		}
		else
		{
			int reduction = 0;
			if (depth >= 3 && num_moves > 1 + is_root && (!is_tactical || piece_values[eg][pos->captured_piece] + info->
				static_eval <= alpha))
			{
				reduction = lmr(improving, depth, num_moves);
				reduction -= 2 * is_pv;
				if (!is_tactical)
				{
					if (m == info->killers[0] || m == info->killers[1] || m == movegen.counter_move)
						reduction--;
					if (hash_move && pos->is_capture(hash_move))
						reduction += 1 + info->singular_extension;
				}
				int quiet_score = history + counter + followup;
				reduction -= std::max(-2, std::min(quiet_score / 8000, 2));
				reduction = std::min(new_depth - 1, std::max(reduction, 0));
			}
			score = -alpha_beta(thread, info + 1, new_depth - reduction, -alpha - 1, -alpha);
			if (reduction > 0 && score > alpha)
				score = -alpha_beta(thread, info + 1, new_depth, -alpha - 1, -alpha);
			if (is_pv && score > alpha && score < beta)
				score = -alpha_beta(thread, info + 1, new_depth, -beta, -alpha);
		}
		pos->undo_move(m);
		if (score > best_score)
		{
			best_score = score;
			if (score > alpha)
			{
				if (is_pv && is_main_thread(pos))
				{
					info->pv[0] = m;
					info->pv_len = (info + 1)->pv_len + 1;
					memcpy(info->pv + 1, (info + 1)->pv, sizeof(move) * (info + 1)->pv_len);
				}
				best_move = m;
				if (is_pv && score < beta)
				{
					alpha = score;
				}
				else
				{
					break;
				}
			}
		}
		if (m != best_move && !is_tactical && quiets_count < 64)
		{
			quiets[quiets_count++] = m;
		}
	}
	if (num_moves == 0)
	{
		best_score = excluded_move != move_none ? alpha : in_check ? value_mated + ply : 0;
	}
	else if (best_move)
	{
		update_heuristics(pos, info, best_score, beta, depth, best_move, quiets, quiets_count);
	}
	if (excluded_move == move_none)
	{
		save_entry(tte, pos->key, best_move, depth, score_to_hash(best_score, static_cast<int16_t>(ply)),
		           info->static_eval, is_pv && best_move ? flag_exact : flag_alpha);
	}
	return best_score;
}
//-------------------------------------------//
void* aspiration_thread(void* t)
{
	const auto thread = static_cast<searchthread*>(t);
	const Position* pos = &thread->position;
	searchinfo* info = &thread->ss[2];
	const bool is_main = is_main_thread(pos);
	move last_pv = move_none;
	int previous = value_mated;
	int score;
	const int init_ideal_usage = ideal_usage;
	int depth = 0;
	while (++depth <= depth_limit)
	{
		thread->seldepth = 0;
		int aspiration = aspiration_init;
		int alpha = value_mated;
		int beta = value_mate;
		int actual_search_depth = depth;
		if (actual_search_depth >= 5)
		{
			alpha = std::max(previous - aspiration, static_cast<int>(value_mated));
			beta = std::min(previous + aspiration, static_cast<int>(value_mate));
		}
		bool failed_low = false;
		if (thread->thread_id != 0)
		{
			if (const int cycle = thread->thread_id % 16; (actual_search_depth + cycle) % skip_depths[cycle] == 0)
				actual_search_depth += skip_size[cycle];
		}
		if (setjmp(thread->jbuffer)) break;
		while (true)
		{
			score = alpha_beta(thread, info, actual_search_depth, alpha, beta);
			if (is_timeout)
			{
				break;
			}
			if (is_main && (score <= alpha || score >= beta) && depth > 12)
			{
				print_info(pos, info, depth, score, alpha, beta);
			}
			if (score <= alpha)
			{
				alpha = std::max(score - aspiration, static_cast<int>(value_mated));
				failed_low = true;
				actual_search_depth = depth;
			}
			else if (score >= beta)
			{
				beta = std::min(score + aspiration, static_cast<int>(value_mate));
				actual_search_depth--;
			}
			else
			{
				if (is_main)
				{
					pv_length = info->pv_len;
					memcpy(main_pv, info->pv, sizeof(move) * pv_length);
					print_info(pos, info, depth, score, alpha, beta);
				}
				break;
			}
			aspiration += aspiration == aspiration_init ? aspiration * 2 / 3 : aspiration / 2;
		}
		previous = score;
		if (is_timeout)
		{
			break;
		}
		if (!is_main)
		{
			continue;
		}
		if (time_elapsed() > ideal_usage && !is_pondering && !is_depth && !is_infinite)
		{
			is_timeout = true;
			break;
		}
		if (depth >= 6 && !is_movetime)
		{
			update_time(failed_low, last_pv == main_pv[0], init_ideal_usage);
		}
		last_pv = main_pv[0];
	}
	return nullptr;
}
//-------------------------------------------//
void check_time(const searchthread* thread)
{
	if ((thread->nodes & timer_granularity) == timer_granularity)
	{
		if (time_elapsed() >= max_usage && !is_pondering && !is_depth && !is_infinite)
		{
			is_timeout = true;
		}
	}
}
//-------------------------------------------//
inline void get_time_limit()
{
	is_movetime = limits.time_is_limited;
	is_depth = limits.depth_is_limited;
	is_infinite = limits.infinite;
	is_timeout = false;
	depth_limit = limits.depth_is_limited ? limits.depth_limit : max_ply;
	if (depth_limit == max_ply)
	{
		const auto [optimum_time, maximum_time] = calculate_time();
		ideal_usage = optimum_time;
		max_usage = maximum_time;
	}
	else
	{
		ideal_usage = 10000;
		max_usage = 10000;
	}
}
//-------------------------------------------//
inline void history_scores(const Position* pos, const searchinfo* info, const move m, int16_t* history,
                           int16_t* counter_move_history, int16_t* follow_up_history)
{
	const int from = from_sq(m);
	const int to = to_sq(m);
	const piece_code pc = pos->mailbox[from];
	*history = pos->my_thread->history_table[pos->active_side][from][to];
	*counter_move_history = (*(info - 1)->counter_move_history)[pc][to];
	*follow_up_history = (*(info - 2)->counter_move_history)[pc][to];
}
//-------------------------------------------//
int16_t history_bonus(const int depth)
{
	return static_cast<int16_t>(depth > 15 ? 0 : 32 * depth * depth);
}
//-------------------------------------------//
inline void initialize_nodes()
{
	for (int i = 0; i < num_threads; ++i)
	{
		const auto t = static_cast<searchthread*>(get_thread(i));
		t->nodes = 0;
	}
}
//-------------------------------------------//
bool is_draw(Position* pos)
{
	const int halfmove_clock = pos->halfmove_clock;
	pos->game_cycle = false;
	if (halfmove_clock > 3)
	{
		if (halfmove_clock > 99)
		{
			return true;
		}
		int repetitions = 0;
		const int min_index = std::max(pos->history_index - halfmove_clock, 0);
		const int rootheight = pos->my_thread->root_height;
		for (int i = pos->history_index - 4; i >= min_index; i -= 2)
		{
			if (pos->key == pos->history_stack[i].key)
			{
				pos->game_cycle = true;
				if (i >= rootheight)
				{
					return true;
				}
				repetitions++;
				if (repetitions == 2)
				{
					return true;
				}
			}
		}
	}
	if (get_material_entry(*pos)->is_drawn)
	{
		return true;
	}
	return false;
}
//-------------------------------------------//
int print_info(const Position* pos, const searchinfo* info, const int depth, int score, const int alpha, const int beta)
{
	const int time_taken = time_elapsed();
	const int seldepth = pos->my_thread->seldepth + 1;
	const uint64_t nodes = sum_nodes() * 1000;
	const uint64_t nps = nodes / (static_cast<unsigned long long>(time_taken) + 1);
	const bool print_pv = score > alpha && score < beta;
	const char* bound = score >= beta ? " lowerbound" : score <= alpha ? " upperbound" : "";
	const char* score_type = abs(score) >= mate_in_max_ply ? "mate" : "cp";
	score = score <= mated_in_max_ply
		        ? (value_mated - score) / 2
		        : score >= mate_in_max_ply
		        ? (value_mate - score + 1) / 2
		        : score * 100 / pawn_eg;
	std::cout << "info depth " << depth << " seldepth " << seldepth << " multipv 1" << " time " << time_taken << " nodes " << nodes << " nps " << nps
		<< " hashfull " << hashfull() << " score " << score_type << " " << score << bound << " pv ";
	if (print_pv)
	{
		for (int i = 0; i < info->pv_len; i++)
			std::cout << move_to_str(info->pv[i]) << " ";
	}
	else
	{
		std::cout << move_to_str(main_pv[0]);
	}
	std::cout << std::endl;
	return fflush(stdout);
}
//-------------------------------------------//
int q_search(searchthread* thread, searchinfo* info, const int depth, int alpha, const int beta)
{
	Position* pos = &thread->position;
	thread->nodes++;
	const int ply =static_cast<uint8_t>(info->ply);
	const bool is_pv = beta - alpha > 1;
	if (is_pv)
		info->pv_len = 0;
	if (thread->thread_id == 0)
		check_time(thread);
	if (is_timeout)
	{
		longjmp(thread->jbuffer, 1);
	}
	if (is_draw(pos))
		return 1 - (thread->nodes & 2);
	const bool in_check = static_cast<bool>(pos->check_bb);
	if (ply >= max_ply)
	{
		return !in_check ? evaluate(*pos) : 0;
	}
	move hash_move = move_none;
	bool tt_hit;
	hash_entry* tte = probe_hash(pos->key, tt_hit);
	const uint8_t flag = hashentry_flag(tte);
	info->chosen_move = move_none;
	(info + 1)->ply = static_cast<int8_t>(ply + 1);
	if (tt_hit)
	{
		hash_move = tte->movecode;
		if (const int hash_score = hash_to_score(tte->value, static_cast<uint16_t>(ply));
			!is_pv
			&& (flag == flag_exact
				|| flag == flag_beta && hash_score >= beta
				|| flag == flag_alpha && hash_score <= alpha))
		{
			return hash_score;
		}
	}
	int best_score;
	if (!in_check)
	{
		const bool is_null = (info - 1)->chosen_move == move_null;
		if (tt_hit && tte->static_eval != undefined)
			info->static_eval = best_score = tte->static_eval;
		else if (is_null)
			info->static_eval = best_score = move_tempo * 2 - (info - 1)->static_eval;
		else
			info->static_eval = best_score = evaluate(*pos);
		if (best_score >= beta)
		{
			return best_score;
		}
		if (best_score + q_search_delta(pos) < alpha)
			return best_score;
		if (is_pv && best_score > alpha)
			alpha = best_score;
	}
	else
	{
		best_score = -value_inf;
		info->static_eval = undefined;
	}
	auto movegen = move_gen(pos, quiescence_search, hash_move, 0, depth);
	move best_move = move_none;
	int move_count = 0;
	move m;
	while ((m = movegen.next_move(info, depth)) != move_none)
	{
		move_count++;
		if (!in_check && !pos->see(m, 0))
			continue;
		int16_t history, counter, followup;
		history_scores(pos, info, m, &history, &counter, &followup);
		if (const bool is_tactical = pos->is_tactical(m); move_count > 1 && !is_tactical && counter < 0 && followup < 0)
			continue;
		pos->do_move(m);
		thread->nodes++;
		info->chosen_move = m;
		const int to = to_sq(m);
		const piece_code pc = pos->mailbox[to];
		info->counter_move_history = &thread->counter_move_history[pc][to];
		const int score = -q_search(thread, info + 1, depth - 1, -beta, -alpha);
		pos->undo_move(m);
		if (score > best_score)
		{
			best_score = score;
			if (score > alpha)
			{
				if (is_pv && is_main_thread(pos))
				{
					info->pv[0] = m;
					info->pv_len = (info + 1)->pv_len + 1;
					memcpy(info->pv + 1, (info + 1)->pv, sizeof(move) * (info + 1)->pv_len);
				}
				best_move = m;
				if (is_pv && score < beta)
				{
					alpha = score;
				}
				else
				{
					save_entry(tte, pos->key, m, 0, score_to_hash(score, static_cast<uint16_t>(ply)), info->static_eval,
					           flag_beta);
					return score;
				}
			}
		}
	}
	if (move_count == 0 && in_check)
		return value_mated + ply;
	const uint8_t return_flag = is_pv && best_move ? flag_exact : flag_alpha;
	save_entry(tte, pos->key, best_move, 0, score_to_hash(best_score, static_cast<uint16_t>(ply)), info->static_eval,
	           return_flag);
	return best_score;
}
//-------------------------------------------//
int q_search_delta(const Position* pos)
{
	const int delta = pos->pawn_on7_th() ? queen_mg : pawn_mg;
	const color enemy = ~pos->active_side;
	return delta + (pos->piece_bb[w_queen | enemy]
		                ? queen_mg
		                : pos->piece_bb[w_rook | enemy]
		                ? rook_mg
		                : pos->piece_bb[w_bishop | enemy]
		                ? bishop_mg
		                : pos->piece_bb[w_knight | enemy]
		                ? knight_mg
		                : pawn_mg);
}
//-------------------------------------------//
void save_killer(const Position* pos, searchinfo* info, const move m, const int16_t bonus)
{
	searchthread* thread = pos->my_thread;
	if (m != info->killers[0])
	{
		info->killers[1] = info->killers[0];
		info->killers[0] = m;
	}
	const color side = pos->active_side;
	const int from = from_sq(m);
	const int to = to_sq(m);
	const piece_code pc = pos->mailbox[from];
	add_history_bonus(&thread->history_table[side][from][to], bonus);
	update_countermove_histories(info, pc, to, bonus);
	if (is_valid((info - 1)->chosen_move))
	{
		const int prev_to = to_sq((info - 1)->chosen_move);
		thread->counter_move_table[pos->mailbox[prev_to]][prev_to] = m;
	}
}
//-------------------------------------------//
inline uint64_t sum_nodes()
{
	uint64_t s = 0;
	for (int i = 0; i < num_threads; ++i)
	{
		const auto t = static_cast<searchthread*>(get_thread(i));
		s += t->nodes;
	}
	return s;
}
//-------------------------------------------//
void update_heuristics(const Position* pos, searchinfo* info, const int best_score, const int beta, const int depth,
	const move m, const move* quiets, const int quiets_count)
{
	const int16_t big_bonus = history_bonus(depth + 1);
	const int16_t bonus = best_score > beta + pawn_mg ? big_bonus : history_bonus(depth);
	if (!pos->is_tactical(m))
	{
		save_killer(pos, info, m, bonus);
		for (int i = 0; i < quiets_count; i++)
		{
			const int from = from_sq(quiets[i]);
			const int to = to_sq(quiets[i]);
			add_history_bonus(&pos->my_thread->history_table[pos->active_side][from][to], -bonus);
			update_countermove_histories(info, pos->mailbox[from], to, -bonus);
		}
	}
}
//-------------------------------------------//
void* think(void* p)
{
	get_time_limit();
	start_search();
	start_time = time_now();
	pv_length = 0;
	const auto threads = new std::thread[num_threads];
	initialize_nodes();
	for (int i = 0; i < num_threads; i++)
	{
		threads[i] = std::thread(aspiration_thread, get_thread(i));
	}
	for (int i = 0; i < num_threads; i++)
	{
		threads[i].join();
	}
	delete[] threads;
	while (is_pondering)
	{
	}
	ponder_move = pv_length > 1 ? main_pv[1] : move_none;
	std::cout << "bestmove " << move_to_str(main_pv[0]);
	if (to_sq(ponder_move) != from_sq(ponder_move))
	{
		std::cout << " ponder " << move_to_str(ponder_move);
	}
	std::cout << std::endl;
	fflush(stdout);
	return nullptr;
}
//-------------------------------------------//
void update_countermove_histories(const searchinfo* info, const piece_code pc, const int to, const int16_t bonus)
{
	for (int i = 1; i <= 2; i++)
	{
		if (is_valid((info - i)->chosen_move))
		{
			add_history_bonus(&(*(info - i)->counter_move_history)[pc][to], bonus);
		}
	}
}
//-------------------------------------------//
void update_time(const bool failed_low, const bool same_pv, const int init_time)
{
	if (failed_low)
	{
		ideal_usage *= 105 / 100;
	}
	if (same_pv)
	{
		ideal_usage = std::max(95 * ideal_usage / 100, init_time / 2);
		max_usage = std::min(ideal_usage * 6, max_usage);
	}
}
//-------------------------------------------//
