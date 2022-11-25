//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#pragma once
#include <string>
#include <vector>
//-------------------------------------------//
const std::string fen_positions[41] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
	"rnb2rk1/pp2np1p/2p2q1b/8/2BPPN2/2P2Q2/PP4PP/R1B2RK1 w - -",
	"rnb2rk1/p4ppp/1p2pn2/q1p5/2BP4/P1P1PN2/1B3PPP/R2QK2R w KQ -",
	"rnb2rk1/1pq1bppp/p3pn2/3p4/3NPP2/2N1B3/PPP1B1PP/R3QRK1 w - -",
	"rn3rk1/pbppqpp1/1p2p2p/8/2PP4/2Q1PN2/PP3PPP/R3KB1R w KQ -",
	"rn3rk1/1p2ppbp/1pp3p1/3n4/3P1Bb1/2N1PN2/PP3PPP/2R1KB1R w K -",
	"r3rbk1/1pq2ppp/2ppbnn1/p3p3/P1PPN3/BP1BPN1P/2Q2PP1/R2R2K1 w - -",
	"r3kbnr/1bqp1ppp/p3p3/1p2P3/5P2/2N2B2/PPP3PP/R1BQK2R w KQkq -",
	"r3kbnr/1bpq2pp/p2p1p2/1p2p3/3PP2N/1PN5/1PP2PPP/R1BQ1RK1 w kq -",
	"r3kb1r/pp2pppp/3q4/3Pn3/6b1/2N1BN2/PP3PPP/R2QKB1R w KQkq -",
	"r2q1rk1/pp2ppbp/2n1bnp1/3p4/4PPP1/1NN1B3/PPP1B2P/R2QK2R w KQ -",
	"r2q1rk1/pb1n1ppp/1p1ppn2/2p3B1/2PP4/P1Q2P2/1P1NP1PP/R3KB1R w KQ -",
	"r2q1rk1/p1p2ppp/2p1pb2/3n1b2/3P4/P4N1P/1PP2PP1/RNBQ1RK1 w - -",
	"r2q1rk1/3nbppp/bpp1pn2/p1Pp4/1P1P1B2/P1N1PN1P/5PP1/R2QKB1R w KQ -",
	"r2q1rk1/1p1bbppp/p1nppn2/8/3NPP2/2N1B3/PPPQB1PP/R4RK1 w - -",
	"r2q1rk1/1bppbppp/p4n2/n2Np3/Pp2P3/1B1P1N2/1PP2PPP/R1BQ1RK1 w - -",
	"r2q1rk1/1b2ppbp/ppnp1np1/2p5/P3P3/2PP1NP1/1P1N1PBP/R1BQR1K1 w - -",
	"r1bq1rk1/pp1n1ppp/4p3/2bpP3/3n1P2/2N1B3/PPPQ2PP/2KR1B1R w - -",
	"r1bq1rk1/bpp2ppp/p2p1nn1/4p3/4P3/1BPP1NN1/PP3PPP/R1BQ1RK1 w - -",
	"r1bq1rk1/3nbppp/p1p1pn2/1p4B1/3P4/2NBPN2/PP3PPP/2RQK2R w K -",
	"r1bn1rk1/ppp1qppp/3pp3/3P4/2P1n3/2B2NP1/PP2PPBP/2RQK2R w K -",
	"r1b1r1k1/pp1nqppp/2pbpn2/8/2pP4/2N1PN1P/PPQ1BPP1/R1BR2K1 w - -",
	"r1b1r1k1/p1p3pp/2p2n2/2bp4/5P2/3BBQPq/PPPK3P/R4N1R b - -",
	"r1b1k3/5p1p/p1p5/3np3/1b2N3/4B3/PPP1BPrP/2KR3R w q -",
	"r1b1k2r/pp1nqp1p/2p3p1/3p3n/3P4/2NBP3/PPQ2PPP/2KR2NR w kq -",
	"b7/2q2kp1/p3pbr1/1pPpP2Q/1P1N3P/6P1/P7/5RK1 w - -",
	"7r/1p2k3/2bpp3/p3np2/P1PR4/2N2PP1/1P4K1/3B4 b - -",
	"6k1/p1qb1p1p/1p3np1/2b2p2/2B5/2P3N1/PP2QPPP/4N1K1 b - -",
	"5rk1/7p/p1N5/3pNp2/2bPnqpQ/P7/1P3PPP/4R1K1 w - -",
	"4k3/p1P3p1/2q1np1p/3N4/8/1Q3PP1/6KP/8 w - -",
	"3rr1k1/pb3pp1/1p1q1b1p/1P2NQ2/3P4/P1NB4/3K1P1P/2R3R1 w - -",
	"3r4/1b2k3/1pq1pp2/p3n1pr/2P5/5PPN/PP1N1QP1/R2R2K1 b - -",
	"3q4/pp3pkp/5npN/2bpr1B1/4r3/2P2Q2/PP3PPP/R4RK1 w - -",
	"2rr2k1/1b3p1p/p4qpb/2R1n3/3p4/BP2P3/P3QPPP/3R1BKN b - -",
	"2rq1rk1/p3bppp/bpn1pn2/2pp4/3P4/1P2PNP1/PBPN1PBP/R2QR1K1 w - -",
	"2r4k/pB4bp/6p1/6q1/1P1n4/2N5/P4PPP/2R1Q1K1 b - -",
	"2r1b1k1/R4pp1/4pb1p/1pBr4/1Pq2P2/3N4/2PQ2PP/5RK1 b - -",
	"2q1r1k1/1ppb4/r2p1Pp1/p4n1p/2P1n3/5NPP/PP3Q1K/2BRRB2 w - -",
	"2k4r/1pp2ppp/p1p1bn2/4N3/1q1rP3/2N1Q3/PPP2PPP/R4RK1 w - -",
	"1rr1nbk1/5ppp/3p4/1q1PpN2/np2P3/5Q1P/P1BB1PP1/2R1R1K1 w - -",
	"1r5r/3b1pk1/3p1np1/p1qPp3/p1N1PbP1/2P2PN1/1PB1Q1K1/R3R3 b - -",
};
//-------------------------------------------//
char piece_char(piece_code c, bool lower = false);
std::string move_to_str(move code);
std::vector<std::string> split_string(const char* c);
unsigned char algebraic_to_index(const std::string& s);
void bench();
void* alloc_aligned_mem(size_t alloc_size, void*& mem);
