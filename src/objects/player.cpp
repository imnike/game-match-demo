#include "player.h"
#include "..\..\include\globalDefine.h"
#include "..\..\utils\timeUtils.h"
#include "..\dbManager.h"

Player::Player(uint64_t id, uint32_t score, uint32_t wins, uint64_t updateTime)
    : m_id(id), m_score(score), m_wins(wins), m_updatedTime(updateTime)
{
}

Player::~Player()
{
}

uint32_t Player::getTier() const
{
    return (m_score / 200) + 1; // hidden tier
}

void Player::addScore(uint32_t scoreDelta)
{
    m_score += scoreDelta;
}

void Player::subScore(uint32_t scoreDelta)
{
    if (m_score >= scoreDelta)
    {
        m_score -= scoreDelta;
    }
    else
    {
        m_score = 0;
	}
}

void Player::addWins()
{
    m_wins++;
}

void Player::save() const
{
    DbManager::instance().updatePlayerBattles(m_id, m_score, m_wins);
}
