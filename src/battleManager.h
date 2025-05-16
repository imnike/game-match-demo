#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H

#include "playerManager.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <cstdint> 

// --- BattleRoom Class ---
class BattleRoom
{
public:
    BattleRoom(std::vector<Player*> blackTeam, std::vector<Player*> whiteTeam);
    ~BattleRoom();

    void startBattle();

private:
    std::vector<Player*> blackTeam;
    std::vector<Player*> whiteTeam;
};

// --- MatchQueue Class ---
class MatchQueue
{
public:
    MatchQueue();
    ~MatchQueue();

    void addPlayer(Player* pPlayer);
    // Renamed from hasEnoughPlayersForLevel to reflect tier-based matchmaking
    bool hasEnoughPlayersForTier(uint32_t tier);
    // Renamed from getPlayersForMatch (level) to getPlayersForMatch (tier)
    std::vector<Player*> getPlayersForMatch(uint32_t tier);

    std::mutex mutex; // Mutex to protect access to tierQueues
    // Changed from levelQueues to tierQueues
    std::map<uint32_t, std::vector<Player*>> tierQueues;
};

// --- BattleManager Class (Singleton) ---
class BattleManager
{
public:
    // Singleton instance access
    static BattleManager& instance();

    // Prevent copying and assignment
    BattleManager(const BattleManager&) = delete;
    BattleManager& operator=(const BattleManager&) = delete;

    bool initialize();
    void release();

    void startMatchmaking();
    void stopMatchmaking();
    void addPlayerToQueue(Player* pPlayer);
	void PlayerWin(Player* pPlayer);
	void PlayerLose(Player* pPlayer);

private:
    BattleManager(); // Private constructor for singleton
    ~BattleManager();

    void matchmakingThread();

    MatchQueue matchQueue;
    std::thread matchmakingThreadHandle;
    std::atomic<bool> isRunning; // Use atomic for thread-safe flag
};

#endif // BATTLE_MANAGER_H