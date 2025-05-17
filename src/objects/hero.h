// @file: Hero.h
#ifndef HERO_H
#define HERO_H

#include <cstdint>

class Hero
{
public:
    uint64_t m_playerId;    // playerId
    uint32_t m_id;          // 英雄所代表的玩家ID (不變)
    uint32_t m_hp;          // 當前生命值
    uint32_t m_maxHp;       // 最大生命值
    uint32_t m_mp;          // 當前魔法值/能量
    uint32_t m_maxMp;       // 最大魔法值/能量
    uint16_t m_atk;         // 攻擊力
    uint16_t m_def;         // 防禦力
    uint16_t m_spd;         // 移動速度
    uint8_t m_lv;           // 戰鬥中的等級
    uint32_t m_exp;         // 當前經驗值

    Hero(const uint64_t playerId);
    ~Hero();

    //void heal(uint32_t hp);       // 英雄治療
    //void gainExp(uint32_t exp);   // 獲得經驗值
    //void levelUp();               // 升級

    uint64_t getPlayerId() const { return m_playerId; }
    uint32_t getId() const { return m_id; }

private:

};

#endif // HERO_H