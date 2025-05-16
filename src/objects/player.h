// player.h
#ifndef PLAYER_H
#define PLAYER_H
#include <cstdint>

class Player
{
public:
    Player(uint64_t id, uint32_t score, uint32_t wins, uint64_t updateTime);
    ~Player();

    uint64_t getId() const { return m_id; };
    uint32_t getScore() const { return m_score; };
    uint32_t getWins() const { return m_wins; };
	uint32_t getTier() const;
    uint64_t getUpdatedTime() const { return m_updatedTime; };

    void addScore(uint32_t scoreDelta);
    void subScore(uint32_t scoreDelta);
    void addWins();
    void save() const;

private:
    uint64_t m_id = 0;
    uint32_t m_score = 0;
    uint32_t m_wins = 0;
    uint64_t m_updatedTime = 0;
};

#endif // !PLAYER_H