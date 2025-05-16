// playerManager.h
#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H
#include <unordered_map>
#include <set>
#include <mutex>
#include <cstdint>

class Player
{
public:
    Player(uint64_t id, uint32_t score, uint32_t wins, time_t updateTime);
    ~Player();

    uint64_t getId() const { return m_id; };
    uint32_t getScore() const { return m_score; };
    uint32_t getWins() const { return m_wins; };
	uint32_t getTier() const;
    time_t getUpdatedTime() const { return m_updatedTime; };

    void addScore(uint32_t scoreChange);
    void addWin();
    void updateStats();

private:
    uint64_t m_id = 0;
    uint32_t m_score = 0;
    uint32_t m_wins = 0;
    uint64_t m_updatedTime = 0;
};

class PlayerManager
{
public:

    static PlayerManager& instance();

    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;

    ~PlayerManager();

    bool initialize();
    void release();
    void syncPlayer(uint64_t id, uint32_t score, uint32_t wins, time_t updatedTime);
    Player* playerLogin(uint64_t id);
    bool playerLogout(uint64_t id);
    Player* getPlayer(uint64_t id);
    bool isPlayerOnline(uint64_t id);
    void getOnlinePlayers(std::vector<Player*>& refVecPlayers);
    void setPlayerOnline(uint64_t id, bool isOnline);

private:

    PlayerManager();

    std::unordered_map<uint64_t, Player*> mapPlayers = {};
	std::set<uint64_t> setOnlinePlayerIds = {};
	std::mutex mutex;

};

#endif // !PLAYER_MANAGER_H