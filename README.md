# kuma

  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]
  ![Downloads][downloads-badge]
  [![License][license-badge]][license-link]
  
![alt tag](https://raw.githubusercontent.com/kato-daichi/kuma/master/src/japan-bear.jpg)

クマのチェスエンジン
uci chess engine

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

Kuma uses an original HalfKP_256x2-32-32 NNUE net trained from self-play with reinforcement learning.

HalfKP-256-32-32-1 is the original size introduced for Shogi in 2018 by Yu Nasu, and integrated into Shogi engine YaneuraOu by Motohiro Isozaki.

https://yaneuraou.yaneu.com/2020/06/19/stockfish-nnue-the-complete-guide/

https://github.com/yaneurao/YaneuraOu

HalfKP_256x2-32-32 NNUE was implemented in Stockfish by Hisayori Noda.

https://www.chessprogramming.org/Stockfish_NNUE

https://www.chessprogramming.org/Hisayori_Noda

https://github.com/nodchip/Stockfish

HalfKP_256x2-32-32 is the ideal size ~20MB, large enough to hold sufficient data but not too large, which could negatively affect engine performance.
It is simple with features that are fast to calculate.

## credits

Kuma borrows ideas and code from the following excellent open-source projects:

- Beef https://github.com/jtseng20/Beef (Jonathan Tseng)
- Defenchess https://github.com/cetincan0/Defenchess (Can Cetin & Dogac Eldenkà
- Fire https://github.com/FireFather/fire (Norman Schmidt)
- NNUE probe: https://github.com/dshawul/nnue-probe (Daniel Shawul)
- NNUE-gui https://github.com/FireFather/nnue-gui (Norman Schmidt)


[license-badge]:https://img.shields.io/github/license/kato-daichi/kuma?style=for-the-badge&label=license&color=success
[license-link]:https://github.com/kato-daichi/kuma/blob/main/LICENSE
[release-badge]:https://img.shields.io/github/v/release/kato-daichi/kuma?style=for-the-badge&label=official%20release
[release-link]:https://github.com/kato-daichi/kuma/releases/latest
[commits-badge]:https://img.shields.io/github/commits-since/kato-daichi/kuma/latest?style=for-the-badge
[commits-link]:https://github.com/kato-daichi/kuma/commits/main
[downloads-badge]:https://img.shields.io/github/downloads/kato-daichi/kuma/total?color=success&style=for-the-badge
