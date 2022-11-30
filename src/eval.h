//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include "position.h"
#include "thread.h"

class Eval
{
public:
	explicit Eval(const Position& p) : pos_(p), king_attackers_count_{}, king_attackers_weight_{}, king_attacks_count_{},
		pawnhashentry_(nullptr), double_targets_{}, king_rings_{}, mobility_area_{}
	{
		memset(&attacked_squares_, 0, sizeof attacked_squares_);
	}
	int value();
private:
	const Position& pos_;
	int king_attackers_count_[2];
	int king_attackers_weight_[2];
	int king_attacks_count_[2];
	pawnhash_entry* pawnhashentry_;
	Score mobility_[2] = { S(0, 0), S(0, 0) };
	template <color Side, piece_type Type>Score evaluate_piece();
	template <color Side>[[nodiscard]] int pawn_shelter_score(int sq) const;
	template <color Side>[[nodiscard]] Score evaluate_passers() const;
	template <color Side>[[nodiscard]] Score evaluate_threats() const;
	template <color Side>[[nodiscard]] Score king_safety() const;
	template <color Side>void evaluate_pawn_structure() const;
	template <color Side>void pawn_shelter_castling() const;
	uint64_t attacked_squares_[14]{};
	uint64_t double_targets_[2];
	uint64_t king_rings_[2];
	uint64_t mobility_area_[2];
	void evaluate_pawns() const;
	void pre_eval();
};
//-------------------------------------------//
inline int eg_value(const Score s)
{
	return static_cast<int16_t>(static_cast<uint16_t>(static_cast<unsigned>(s + 0x8000) >> 16));
}
//-------------------------------------------//
inline int mg_value(const Score s)
{
	return static_cast<int16_t>(static_cast<uint16_t>(static_cast<unsigned>(s)));
}
//-------------------------------------------//
inline Score operator/(const Score s, const int i)
{
	return make_score(mg_value(s) / i, eg_value(s) / i);
}
//-------------------------------------------//
inline Score operator*(const Score s, const int i)
{
	const auto result = static_cast<Score>(static_cast<int>(s) * i);
	return result;
}
//-------------------------------------------//
inline pawnhash_entry* get_pawntte(const Position& pos)
{
	return &pos.my_thread->pawntable[pos.pawnhash & pawn_hash_size_mask];
}
//-------------------------------------------//
inline materialhashentry* get_material_entry(const Position& p)
{
	return &p.my_thread->material_table[p.materialhash & material_hash_size_mask];
}
//-------------------------------------------//
inline constexpr int bishop_eg = 534;
inline constexpr int bishop_mg = 426;
inline constexpr int bishop_pair = 70;
inline constexpr int king_danger_base = -10;
inline constexpr int king_eg = 0;
inline constexpr int king_mg = 0;
inline constexpr int king_shield_bonus = 14;
inline constexpr int kingpinned_penalty = 37;
inline constexpr int kingring_attack = 52;
inline constexpr int kingweak_penalty = 96;
inline constexpr int knight_eg = 440;
inline constexpr int knight_mg = 367;
inline constexpr int move_tempo = 30;
inline constexpr int no_queen = 584;
inline constexpr int pawn_eg = 165;
inline constexpr int pawn_mg = 100;
inline constexpr int queen_contact_check = 186;
inline constexpr int queen_eg = 1667;
inline constexpr int queen_mg = 1302;
inline constexpr int rook_eg = 962;
inline constexpr int rook_mg = 750;
inline constexpr int scale_hardtowin = 3;
inline constexpr int scale_nopawns = 0;
inline constexpr int scale_normal = 32;
inline constexpr int scale_ocb = 16;
inline constexpr int scale_ocb_pieces = 27;
inline constexpr int scale_onepawn = 10;
inline constexpr Score backward_penalty[2] = { S(-5, -5), S(-2, 3) };
inline constexpr Score battery = S(11, -19);
inline constexpr Score bishop_opposer_bonus = S(-1, 7);
inline constexpr Score bishop_pawns = S(-1, 1);
inline constexpr Score defended_rook_file = S(1, -33);
inline constexpr Score doubled_penalty[2] = { S(42, 22), S(0, 117) };
inline constexpr Score doubled_penalty_undefended[2] = { S(-10, 16), S(-1, 19) };
inline constexpr Score hanging_piece = S(7, 17);
inline constexpr Score isolated_doubled_penalty[2] = { S(12, 31), S(2, -6) };
inline constexpr Score isolated_doubled_penalty_ah[2] = { S(-4, 60), S(4, 39) };
inline constexpr Score isolated_penalty[2] = { S(7, 26), S(12, 27) };
inline constexpr Score isolated_penalty_ah[2] = { S(-8, 27), S(8, 22) };
inline constexpr Score king_multiple_threat = S(-32, 98);
inline constexpr Score king_protector = S(4, -1);
inline constexpr Score king_threat = S(-12, 53);
inline constexpr Score kingflank_attack = S(0, 0);
inline constexpr Score pawn_distance_penalty = S(0, 10);
inline constexpr Score pawn_push_threat = S(20, 21);
inline constexpr Score rank7_rook = S(5, 39);
inline constexpr Score rook_file[2] = { S(29, -7), S(40, 6) };
inline constexpr Score safe_pawn_threat = S(65, 9);
inline constexpr Score trapped_bishop_penalty = S(53, 30);
inline constexpr Score very_trapped_bishop_penalty = S(74, 121);
//-------------------------------------------//
inline constexpr Score connected_bonus[2][2][8] = {
	{
		{S(0, 0), S(0, 0), S(14, -6), S(18, 0), S(31, 21), S(58, 47), S(274, -2), S(0, 0)},
		{S(0, 0), S(13, -22), S(14, 0), S(37, 31), S(91, 89), S(101, 271), S(-164, 1043), S(0, 0)}
	},
	{
		{S(0, 0), S(0, 0), S(0, -5), S(-3, -9), S(-1, -4), S(-8, 31), S(0, 0), S(0, 0)},
		{S(0, 0), S(-1, -22), S(3, -3), S(19, 0), S(39, 35), S(113, -57), S(0, 0), S(0, 0)}
	}
};
//-------------------------------------------//
inline constexpr Score passed_rank_bonus[8] = { S(0, 0), S(0, 0), S(9, 13), S(-12, 12), S(13, 72), S(-2, 98), S(67, 149), S(0, 0) };
//-------------------------------------------//
inline constexpr Score passed_unsafe_bonus[2][8] = {
	{S(0, 0), S(0, 0), S(-13, -8), S(-8, 54), S(-50, 45), S(8, 97), S(-25, 191), S(0, 0)},
	{S(0, 0), S(0, 0), S(-12, -12), S(-1, 26), S(-22, -37), S(57, -82), S(83, -95), S(0, 0)}
};
//-------------------------------------------//
inline constexpr Score passed_blocked_bonus[2][8] = {
	{S(0, 0), S(0, 0), S(1, 3), S(-2, 1), S(6, 20), S(11, 72), S(73, 242), S(0, 0)},
	{S(0, 0), S(0, 0), S(-11, 1), S(-10, -23), S(7, -38), S(23, -83), S(145, -121), S(0, 0)}
};
//-------------------------------------------//
inline constexpr Score knight_mobility_bonus[9] = {
	S(-71, -23), S(-70, -31), S(-50, 8), S(-40, 15), S(-30, 22), S(-23, 32), S(-14, 33),
	S(-4, 35), S(2, 22)
};
//-------------------------------------------//
inline constexpr Score bishop_mobility_bonus[14] = {
	S(-75, -49), S(-43, -50), S(-25, -37), S(-17, -9), S(-2, -4), S(7, 3), S(13, 10),
	S(16, 10), S(21, 13), S(22, 20), S(32, 9), S(53, 11), S(53, 20), S(68, 14)
};
//-------------------------------------------//
inline constexpr Score rook_mobility_bonus[15] = {
	S(-145, -167), S(-120, -101), S(-95, -83), S(-93, -64), S(-87, -61), S(-92, -34), S(-86, -37),
	S(-82, -32), S(-77, -29), S(-73, -22), S(-73, -18), S(-71, -11), S(-72, -2), S(-60, -4),
	S(-53, -9)
};
//-------------------------------------------//
inline constexpr Score queen_mobility_bonus[28] = {
	S(1477, -1234), S(17, 108), S(1, 114), S(8, 7), S(19, -6), S(28, -23), S(29, 25),
	S(33, 38), S(34, 71), S(39, 75), S(42, 86), S(44, 93), S(45, 105), S(50, 103),
	S(52, 114), S(54, 115), S(52, 123), S(55, 125), S(51, 129), S(67, 113), S(66, 105),
	S(108, 76), S(113, 54), S(98, 47), S(142, 8), S(176, 44), S(213, -48), S(264, 16)
};
//-------------------------------------------//
inline const int attacker_weights[7] = { 0, 0, 37, 19, 14, 0, 0 };
inline const int check_penalty[7] = { 0, 0, 366, 326, 487, 326, 0 };
inline const int unsafe_check_penalty[7] = { 0, 0, 79, 160, 132, 104, 0 };
inline constexpr Score minor_threat[7] = { S(0, 0), S(3, 22), S(33, 33), S(39, 34), S(68, 9), S(65, -17), S(287, 932) };
inline constexpr Score outpost_bonus[2][2] = { {S(21, 23), S(30, 22)}, {S(70, 21), S(71, 39)} };
inline constexpr Score passed_enemy_distance[8] = { S(0, 0), S(-3, 4), S(1, 6), S(-4, 10), S(-11, 32), S(-31, 58), S(15, 8), S(0, 0) };
inline constexpr Score passed_friendly_distance[8] = { S(0, 0), S(5, -2), S(-1, -4), S(10, -12), S(15, -14), S(20, -15), S(1, -9), S(0, 0) };
inline constexpr Score reachable_outpost[2] = { S(0, 21), S(18, 25) };
inline constexpr Score rook_threat[7] = { S(0, 0), S(-1, 25), S(19, 32), S(35, 40), S(7, 16), S(87, -40), S(471, 590) };
inline constexpr Score tarrasch_rule_enemy = S(-28, 99);
inline constexpr Score tarrasch_rule_friendly[8] = { S(0, 0), S(0, 0), S(0, 0), S(21, -6), S(21, 8), S(-13, 40), S(-32, 62), S(0, 0) };
//-------------------------------------------//
inline constexpr int my_pieces[5][5] = {
	{16},
	{188, 10},
	{47, 177, -44},
	{31, 195, 163, -85},
	{60, 3, -14, -459, -72}
};
//-------------------------------------------//
inline constexpr int opponent_pieces[5][5] = {
	{0},
	{59, 0},
	{43, -107, 0},
	{93, -26, -128, 0},
	{287, 9, 232, 266, 0}
};
//-------------------------------------------//
inline const int king_shield[4][8] = {
	{1, 57, 60, 39, 62, 135, 103, 0},
	{-26, 42, 28, -2, -10, 132, 64, 0},
	{25, 76, 48, 41, 60, 135, 97, 0},
	{5, 25, 15, 7, 17, 50, 63, 0}
};
//-------------------------------------------//
inline const int pawn_storm_blocked[4][8] = {
	{0, 0, 50, -8, 6, 59, 128, 0},
	{0, 0, 71, 10, 7, 0, 94, 0},
	{0, 0, 98, 37, 33, 50, 154, 0},
	{0, 0, 84, 42, 19, 28, 16, 0}
};
//-------------------------------------------//
inline constexpr int pawn_storm_free[4][8] = {
	{38, -275, -120, 14, 25, 27, 18, 0},
	{32, -158, -24, 27, 11, 6, 7, 0},
	{21, -125, 9, 29, 20, 20, 18, 0},
	{26, -32, 48, 58, 32, 17, 15, 0}
};
//-------------------------------------------//
inline int piece_values[2][14] = {
	{0, 0, pawn_mg, pawn_mg, knight_mg, knight_mg, bishop_mg, bishop_mg, rook_mg, rook_mg, queen_mg, queen_mg, 0, 0},
	{0, 0, pawn_eg, pawn_eg, knight_eg, knight_eg, bishop_eg, bishop_eg, rook_eg, rook_eg, queen_eg, queen_eg, 0, 0}
};
//-------------------------------------------//
inline int non_pawn_value[14] = { 0, 0, 0, 0, knight_mg, knight_mg, bishop_mg, bishop_mg, rook_mg, rook_mg, queen_mg, queen_mg, 0, 0 };
//-------------------------------------------//
inline constexpr Score psq_bonus[7][64] = {
	{
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)
	},
	{
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(-24, 28), S(12, 12), S(-23, 17), S(7, 2), S(13, 20), S(-2, 4), S(21, -1), S(-33, -6),
		S(-17, 13), S(-1, 9), S(-2, -5), S(11, -7), S(27, -2), S(-1, -3), S(28, -10), S(-36, -4),
		S(-27, 30), S(-12, 19), S(1, -9), S(22, -20), S(29, -26), S(16, -17), S(7, -6), S(-37, 3),
		S(-4, 47), S(19, 19), S(14, 0), S(39, -34), S(41, -23), S(24, -11), S(34, 12), S(-3, 16),
		S(-6, 72), S(-10, 53), S(-1, 22), S(1, -22), S(54, -50), S(69, -19), S(26, 16), S(13, 29),
		S(10, 52), S(9, 24), S(-45, 15), S(12, -49), S(-63, -14), S(54, -47), S(0, 21), S(24, 35),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)
	},
	{
		S(-105, 11), S(4, -15), S(-26, 1), S(-14, 33), S(27, 14), S(-4, 23), S(1, -22), S(3, -41),
		S(-14, -13), S(-32, 4), S(4, 7), S(20, 18), S(27, 19), S(41, 0), S(22, 8), S(9, -15),
		S(1, -11), S(1, 16), S(29, 6), S(31, 39), S(44, 39), S(41, 11), S(50, -9), S(11, 0),
		S(-2, -2), S(24, 1), S(24, 36), S(27, 53), S(29, 49), S(35, 39), S(28, 36), S(6, 1),
		S(-13, 4), S(40, 15), S(18, 47), S(51, 54), S(47, 52), S(55, 42), S(38, 25), S(10, 6),
		S(-100, -7), S(7, 10), S(-5, 40), S(6, 43), S(22, 38), S(67, 19), S(39, -1), S(4, -39),
		S(-108, -14), S(-77, 20), S(0, 6), S(-8, 44), S(-58, 45), S(20, 1), S(-5, 11), S(-55, -51),
		S(-237, -23), S(-178, -5), S(-136, 40), S(-101, -4), S(-4, 2), S(-123, -8), S(-85, -30), S(-200, -101)
	},
	{
		S(-10, 5), S(30, 18), S(23, 19), S(23, 27), S(35, 18), S(15, 29), S(-1, 19), S(-1, 11),
		S(34, 10), S(43, -1), S(37, 11), S(23, 24), S(34, 33), S(46, 22), S(71, 3), S(41, -16),
		S(17, 17), S(36, 19), S(34, 33), S(31, 34), S(38, 44), S(59, 25), S(44, 21), S(29, 18),
		S(1, 15), S(31, 10), S(26, 24), S(29, 40), S(40, 23), S(16, 34), S(22, 19), S(28, -2),
		S(3, 16), S(14, 21), S(12, 28), S(31, 35), S(18, 42), S(19, 40), S(8, 18), S(-3, 23),
		S(18, 11), S(25, 12), S(0, 32), S(9, 15), S(-15, 30), S(43, 28), S(17, 27), S(25, 17),
		S(-8, 23), S(-2, 30), S(-13, 31), S(-93, 42), S(1, 39), S(-25, 52), S(-35, 45), S(-71, 38),
		S(11, 2), S(25, -4), S(-156, 37), S(-85, 36), S(-95, 49), S(-58, 27), S(-4, 24), S(31, -26)
	},
	{
		S(30, 30), S(27, 40), S(27, 42), S(46, 27), S(53, 24), S(50, 26), S(34, 23), S(21, 16),
		S(3, 42), S(29, 34), S(14, 45), S(45, 33), S(62, 18), S(57, 21), S(40, 18), S(-8, 32),
		S(-2, 47), S(22, 46), S(32, 35), S(27, 39), S(46, 30), S(41, 26), S(42, 32), S(25, 14),
		S(-2, 56), S(18, 57), S(11, 63), S(40, 43), S(42, 43), S(19, 46), S(48, 44), S(4, 51),
		S(-3, 63), S(23, 52), S(41, 65), S(49, 46), S(50, 53), S(46, 63), S(4, 66), S(0, 66),
		S(3, 57), S(5, 68), S(19, 55), S(30, 47), S(5, 55), S(24, 52), S(86, 34), S(10, 45),
		S(1, 56), S(10, 51), S(40, 41), S(32, 46), S(44, 35), S(30, 59), S(-41, 84), S(18, 56),
		S(-10, 73), S(18, 70), S(-41, 83), S(26, 63), S(2, 79), S(-35, 86), S(8, 76), S(0, 72)
	},
	{
		S(35, -43), S(21, -43), S(27, -32), S(49, -54), S(22, 6), S(25, -41), S(28, -39), S(-30, -22),
		S(-5, -5), S(22, -29), S(47, -43), S(34, -12), S(40, -1), S(57, -44), S(27, -41), S(45, -23),
		S(2, 32), S(36, -37), S(5, 23), S(25, -5), S(12, 27), S(25, 16), S(42, 14), S(23, 46),
		S(9, 13), S(-18, 60), S(6, 33), S(-11, 59), S(5, 40), S(16, 16), S(17, 48), S(11, 36),
		S(-26, 75), S(-15, 62), S(-22, 53), S(-26, 47), S(-28, 62), S(14, -6), S(-10, 70), S(9, 42),
		S(25, -16), S(7, 9), S(-1, 7), S(-18, 58), S(-39, 47), S(57, -1), S(37, -22), S(65, -26),
		S(-17, -4), S(-43, 25), S(-10, 37), S(-15, 75), S(-98, 93), S(-43, 12), S(-33, 51), S(76, -20),
		S(4, -43), S(13, 11), S(14, 18), S(-2, 14), S(110, -50), S(6, 8), S(52, -25), S(30, 44)
	},
	{
		S(5, -102), S(63, -70), S(86, -26), S(15, -7), S(25, -17), S(29, -7), S(55, -45), S(37, -81),
		S(23, -60), S(22, -17), S(38, 13), S(2, 27), S(23, 23), S(36, 9), S(30, -15), S(34, -45),
		S(0, -43), S(15, -16), S(22, 27), S(-9, 43), S(-9, 41), S(17, 22), S(7, 0), S(-18, -26),
		S(-74, -26), S(20, -11), S(6, 44), S(-64, 64), S(-101, 73), S(-15, 42), S(-35, 7), S(-61, -28),
		S(81, -35), S(16, 34), S(45, 48), S(-42, 69), S(-1, 59), S(49, 59), S(27, 40), S(-60, 4),
		S(101, -1), S(73, 27), S(146, 19), S(-22, 62), S(-4, 69), S(140, 76), S(168, 52), S(24, 20),
		S(132, -34), S(47, 36), S(50, 50), S(86, 47), S(-20, 73), S(74, 72), S(93, 21), S(-73, 73),
		S(-154, -65), S(46, -7), S(97, -23), S(58, -19), S(-110, 58), S(41, 88), S(114, 22), S(55, -27)
	}
};
//-------------------------------------------//
int evaluate(const Position& p);
int kbn_vs_k(const Position& pos);