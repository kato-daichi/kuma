//-------------------------------------------//
// クマのチェスエンジン
// (c) 2022 kato daichi
//-------------------------------------------//
#include "position.h"
#include "board.h"
#include "eval.h"
#include "nnue.h"
//-------------------------------------------//
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#pragma warning(disable: 4244)
#pragma warning(disable: 5054)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
//-------------------------------------------//
int evaluate(const Position& p)
{
    int pieces[33]{};
    int squares[33]{};
    int index = 2;
    for (uint8_t i = 0; i < 64; i++)
    {
	    if (const piece_code pc = p.mailbox[i]; pc == 12)
        {
            pieces[0] = 1;
            squares[0] = i;
        }
        else if (pc == 13)
        {
            pieces[1] = 7;
            squares[1] = i;
        }
        else if (pc == 2)
        {
            pieces[index] = 6;
            squares[index] = i;
            index++;
        }
        else if (pc == 3)
        {
            pieces[index] = 12;
            squares[index] = i;
            index++;
        }
        else if (pc == 4)
        {
            pieces[index] = 5;
            squares[index] = i;
            index++;
        }
        else if (pc == 5)
        {
            pieces[index] = 11;
            squares[index] = i;
            index++;
        }
        else if (pc == 6)
        {
            pieces[index] = 4;
            squares[index] = i;
            index++;
        }
        else if (pc == 7)
        {
            pieces[index] = 10;
            squares[index] = i;
            index++;
        }
        else if (pc == 8)
        {
            pieces[index] = 3;
            squares[index] = i;
            index++;
        }
        else if (pc == 9)
        {
            pieces[index] = 9;
            squares[index] = i;
            index++;
        }
        else if (pc == 10)
        {
            pieces[index] = 2;
            squares[index] = i;
            index++;
        }
        else if (pc == 11)
        {
            pieces[index] = 8;
            squares[index] = i;
            index++;
        }
    }
    const int nnue_score = nnue_evaluate(p.active_side, pieces, squares);
    return nnue_score;
}
