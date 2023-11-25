# kuma
クマのチェスエンジン
uci chess engine

![alt tag](https://raw.githubusercontent.com/kato-daichi/kuma/master/src/japan-bear.jpg)

## feature
- c++20
- 64-bit
- windows
- uci
- alphabeta
- threads
- hash
- ponder
- multiPV
- zobrist
- bench
- perft
- magics
- NNUE

## options
- UCI Hash
- UCI Threads

## nnue

Kuma uses a HalfKP_256x2-32-32 NNUE net trained from self-play with reinforcement learning.

HalfKP-256-32-32-1 is the original size introduced for Shogi in 2018 by Yu Nasu, and integrated into Shogi engine YaneuraOu by Motohiro Isozaki.

https://yaneuraou.yaneu.com/2020/06/19/stockfish-nnue-the-complete-guide/

https://github.com/yaneurao/YaneuraOu

HalfKP_256x2-32-32 NNUE was implemented in Stockfish by Hisayori Noda.

https://www.chessprogramming.org/Stockfish_NNUE

https://www.chessprogramming.org/Hisayori_Noda

https://github.com/nodchip/Stockfish

I believe HalfKP_256x2-32-32 is the ideal size ~20MB, large enough to hold sufficient data but not too large, which could negatively affect engine performance.
It is simple with features that are fast to calculate.
