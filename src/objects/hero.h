// @file: Hero.h
#ifndef HERO_H
#define HERO_H

#include <cstdint>

class Hero
{
public:
    uint64_t m_playerId;    // playerId
    uint32_t m_id;          // �^���ҥN�����aID (����)
    uint32_t m_hp;          // ��e�ͩR��
    uint32_t m_maxHp;       // �̤j�ͩR��
    uint32_t m_mp;          // ��e�]�k��/��q
    uint32_t m_maxMp;       // �̤j�]�k��/��q
    uint16_t m_atk;         // �����O
    uint16_t m_def;         // ���m�O
    uint16_t m_spd;         // ���ʳt��
    uint8_t m_lv;           // �԰���������
    uint32_t m_exp;         // ��e�g���

    Hero(const uint64_t playerId);
    ~Hero();

    //void heal(uint32_t hp);       // �^���v��
    //void gainExp(uint32_t exp);   // ��o�g���
    //void levelUp();               // �ɯ�

    uint64_t getPlayerId() const { return m_playerId; }
    uint32_t getId() const { return m_id; }

private:

};

#endif // HERO_H