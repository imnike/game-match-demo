// globalDefine.h
#ifndef GLOBAL_DEFINE_H
#define GLOBAL_DEFINE_H

#include <cstdint> 

// 定義戰鬥結果的常量
namespace battle_constant
{
    const uint32_t WINNER_SCORE = 50;
    const uint32_t LOSER_SCORE = 50; // 可以根據需求調整為負數

    enum TeamColor : uint8_t
    {
        Red = 0,    // team_0
        Blue = 1,   // team_1
        Max
    };
}

#endif // GLOBAL_DEFINE_H