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
	explicit Eval(const Position& p) : pos_(p), pawnhashe_(nullptr), mobility_area_{}, double_targets_{}, king_rings_{}, king_attackers_count_{},
		king_attacks_count_{}, king_attackers_weight_{}
	{
		memset(&attacked_squares_, 0, sizeof attacked_squares_);
	}
	int value();
private:
	const Position& pos_;
	int king_attackers_count_[2];
	int king_attackers_weight_[2];
	int king_attacks_count_[2];
	pawnhash_entry* pawnhashe_;
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
inline constexpr Score piece_bonus[7][64] = {
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
		S(-20, 23), S(5, 16), S(-13, 8), S(8, -3), S(13, 11), S(-5, 1), S(23, -2), S(-31, -1),
		S(-22, 16), S(2, 6), S(-2, -8), S(11, -11), S(23, -5), S(0, -9), S(27, -10), S(-37, -1),
		S(-23, 25), S(-5, 13), S(-3, -6), S(19, -16), S(23, -24), S(12, -18), S(13, -1), S(-26, -1),
		S(-12, 50), S(17, 29), S(10, 0), S(19, -26), S(43, -23), S(17, -12), S(32, 12), S(2, 16),
		S(-15, 70), S(4, 53), S(9, 11), S(15, -33), S(35, -47), S(51, 0), S(17, 27), S(-6, 46),
		S(36, 43), S(-19, 36), S(-25, 25), S(5, -58), S(-57, -31), S(23, -37), S(-31, 31), S(4, 37),
		S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)
	},
	{
		S(-67, 32), S(-3, -6), S(-16, 22), S(6, 28), S(18, 21), S(13, 19), S(-2, -3), S(-14, -24),
		S(-11, 8), S(-9, 10), S(3, 25), S(23, 22), S(29, 16), S(27, 17), S(32, 22), S(26, 6),
		S(-11, 4), S(5, 20), S(21, 11), S(29, 42), S(43, 38), S(33, 10), S(39, 8), S(21, 10),
		S(1, 1), S(17, 18), S(24, 36), S(34, 47), S(27, 50), S(35, 24), S(19, 22), S(10, 11),
		S(4, 17), S(27, 8), S(38, 32), S(50, 41), S(46, 37), S(55, 43), S(36, 26), S(27, 4),
		S(-63, 13), S(-9, 30), S(-7, 52), S(15, 38), S(35, 26), S(59, 27), S(-9, 10), S(-12, -21),
		S(-64, -12), S(-49, 35), S(-6, 21), S(8, 25), S(-39, 41), S(36, 7), S(-19, 8), S(-53, -31),
		S(-231, -39), S(-128, -8), S(-113, 20), S(-99, 7), S(-51, 4), S(-135, -21), S(-133, -23), S(-176, -110)
	},
	{
		S(11, 24), S(43, 5), S(30, 10), S(21, 25), S(34, 31), S(15, 33), S(20, 28), S(20, 24),
		S(41, 16), S(37, -2), S(42, 5), S(22, 26), S(33, 26), S(53, 7), S(60, 9), S(58, -14),
		S(21, 1), S(50, 30), S(36, 29), S(33, 36), S(41, 41), S(46, 16), S(53, 11), S(33, 13),
		S(4, 25), S(5, 12), S(24, 27), S(36, 26), S(32, 15), S(23, 24), S(11, 22), S(33, -2),
		S(-10, 29), S(7, 32), S(13, 20), S(19, 33), S(32, 23), S(7, 29), S(9, 23), S(-26, 19),
		S(11, 25), S(8, 22), S(-4, 28), S(10, 18), S(-12, 22), S(36, 33), S(-1, 31), S(31, 13),
		S(5, 11), S(1, 14), S(-7, 30), S(-16, 43), S(-22, 28), S(-29, 37), S(-48, 18), S(-6, 3),
		S(-14, 20), S(-19, 16), S(-41, 37), S(-80, 38), S(-105, 42), S(-42, 20), S(-21, 3), S(0, -43)
	},
	{
		S(25, 35), S(26, 35), S(25, 46), S(38, 24), S(46, 19), S(43, 17), S(52, 15), S(23, 24),
		S(19, 44), S(23, 40), S(29, 46), S(41, 34), S(49, 30), S(51, 18), S(60, 27), S(13, 32),
		S(14, 43), S(21, 41), S(26, 49), S(28, 46), S(44, 37), S(38, 29), S(58, 18), S(40, 9),
		S(13, 55), S(20, 48), S(15, 61), S(32, 57), S(31, 49), S(11, 56), S(45, 33), S(19, 44),
		S(12, 75), S(33, 74), S(46, 69), S(46, 59), S(50, 51), S(45, 46), S(25, 71), S(16, 62),
		S(5, 67), S(32, 61), S(31, 72), S(31, 53), S(26, 65), S(45, 54), S(83, 31), S(31, 38),
		S(4, 46), S(0, 62), S(28, 57), S(33, 48), S(23, 42), S(19, 58), S(15, 72), S(37, 50),
		S(6, 74), S(-3, 70), S(-21, 87), S(12, 74), S(-10, 74), S(-3, 90), S(13, 75), S(36, 72)
	},
	{
		S(36, -63), S(20, -41), S(20, -29), S(32, -64), S(28, -31), S(26, -51), S(37, -28), S(-9, 36),
		S(17, -3), S(32, -30), S(41, -44), S(40, -21), S(41, -24), S(45, -52), S(43, -19), S(47, -60),
		S(9, 29), S(35, -27), S(16, 25), S(26, -13), S(27, 15), S(33, 15), S(51, 14), S(31, 15),
		S(15, 11), S(-1, 51), S(13, 22), S(11, 53), S(18, 29), S(19, 14), S(27, 42), S(35, 34),
		S(-10, 58), S(3, 73), S(-2, 58), S(-5, 84), S(-15, 83), S(18, 14), S(2, 74), S(22, 23),
		S(15, -7), S(18, 0), S(7, 55), S(10, 40), S(-22, 61), S(37, 13), S(26, -1), S(58, -35),
		S(-12, 7), S(-12, 37), S(-20, 64), S(-24, 58), S(-63, 104), S(-9, 31), S(-19, 73), S(65, 11),
		S(-1, -49), S(-17, -16), S(2, -4), S(3, 6), S(56, -57), S(30, 14), S(36, 17), S(-1, 13)
	},
	{
		S(26, -69), S(67, -67), S(81, -32), S(10, 0), S(17, -24), S(29, -8), S(52, -53), S(50, -85),
		S(53, -52), S(22, -16), S(32, 12), S(21, 19), S(19, 16), S(26, 8), S(21, -18), S(28, -41),
		S(-4, -37), S(-8, -3), S(-6, 29), S(-11, 42), S(2, 40), S(17, 14), S(0, -6), S(-11, -27),
		S(-47, -10), S(19, 8), S(6, 50), S(-56, 71), S(-60, 64), S(-34, 43), S(-70, 28), S(-99, -7),
		S(35, -18), S(21, 39), S(43, 65), S(-52, 87), S(16, 84), S(5, 72), S(-6, 45), S(-92, 19),
		S(29, -5), S(119, 56), S(210, 40), S(22, 89), S(16, 91), S(72, 86), S(114, 70), S(26, 13),
		S(109, -17), S(86, 52), S(167, 71), S(44, 76), S(4, 110), S(109, 94), S(217, 41), S(-103, 37),
		S(-161, -148), S(66, -64), S(80, -13), S(77, 1), S(-76, 66), S(10, 27), S(84, -43), S(-24, -89)
	}
};
//-------------------------------------------//
int evaluate(const Position& p);
int kbn_vs_k(const Position& pos);