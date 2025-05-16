// battleManager.h
#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H
#include "objects/player.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

// 前向聲明 PlayerManager (如果沒有在 player.h 中包含)
class PlayerManager;
class Player;

// --- BattleRoom 類別 (戰鬥房間) ---
class BattleRoom
{
public:
    // 將黑隊白隊改為紅隊藍隊
    BattleRoom(std::vector<Player*> vecTeamRed, std::vector<Player*> vecTeamBlue);
    ~BattleRoom();
    void startBattle();

private:
    std::vector<Player*> vecTeamRed;
    std::vector<Player*> vecTeamBlue;
};

// --- TeamMatchQueue 類別 (保持不變，因為它處理的是單個玩家) ---
// 用於將單個玩家匹配成隊伍 (例如 3 人一隊)
class TeamMatchQueue
{
public:
    TeamMatchQueue();
    ~TeamMatchQueue();

    void addMember(Player* pPlayer);
    bool hasEnoughMemberForTeam(uint32_t tier);
    std::vector<Player*> getPlayersForTeam(uint32_t tier);

    std::mutex mutex;
    std::map<uint32_t, std::vector<Player*>> mapTierQueues;
};

// --- BattleMatchQueue 類別 (保持不變，因為它處理的是隊伍列表) ---
// 用於將已組成的隊伍匹配成戰鬥 (例如 2 支隊伍一場戰鬥)
class BattleMatchQueue
{
public:
    BattleMatchQueue();
    ~BattleMatchQueue();

    void addTeam(std::vector<Player*> team);
    bool hasEnoughTeamsForBattle(uint32_t tier);
    std::vector<std::vector<Player*>> getTeamsForBattle(uint32_t tier);

    std::mutex mutex;
    std::map<uint32_t, std::vector<std::vector<Player*>>> mapTierQueues;
};

// --- BattleManager 類別 (單例模式) ---
class BattleManager
{
public:
    static BattleManager& instance();

    bool initialize();
    void release();

    void startMatchmaking();
    void stopMatchmaking();

    void addPlayerToQueue(Player* pPlayer);

    void PlayerWin(Player* pPlayer);
    void PlayerLose(Player* pPlayer);

private:
    BattleManager();
    ~BattleManager();
    BattleManager(const BattleManager&) = delete;
    BattleManager& operator=(const BattleManager&) = delete;

    void matchmakingThread();

    std::atomic<bool> isRunning;
    std::thread matchmakingThreadHandle;

    TeamMatchQueue teamMatchQueue;
    BattleMatchQueue battleMatchQueue;
};

#endif // BATTLE_MANAGER_H