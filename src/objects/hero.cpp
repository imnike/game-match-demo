// @file  : hero.cpp
// @brief : 戰鬥時英雄物件
// @author: August
// @date  : 2025-05-17
#include "Hero.h"
#include "Player.h" // 包含 Player.h 以便從 Player 數據初始化 Hero
#include <iostream>   // 用於演示輸出
#include <algorithm>  // 用於 std::min, std::max

Hero::Hero(const uint64_t playerId)
    : m_playerId(playerId),
    m_id(1),
    m_hp(100), m_maxHp(100),
    m_mp(50), m_maxMp(50),
    m_atk(10),
    m_def(5),
    m_spd(5),
    m_lv(1),
    m_exp(0)
{
    this->m_hp = this->m_maxHp;
    this->m_mp = this->m_maxMp;
}

Hero::~Hero() 
{
}