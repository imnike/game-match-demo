// battleManager.h
#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H
#include "objects/player.h"
#include "objects/hero.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic> // 確保包含 <atomic>

class BattleRoom
{
public:
    BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue);
    ~BattleRoom();
    void startBattle();
    void finishBattle(); // finishBattle 依然存在，用於清理

    uint64_t getRoomId() const { return m_roomId; } // 暴露 roomId

private:
    uint64_t m_roomId; // 新增 roomId
    std::vector<std::unique_ptr<Hero>> m_vecTeamRed;
    std::vector<std::unique_ptr<Hero>> m_vecTeamBlue;
};

// 3位玩家組成一個隊伍，並且每個隊伍有一個等級 (tier)，這個等級用來匹配對手
class TeamMatchQueue
{
    friend class BattleManager;
public:
    TeamMatchQueue();
    ~TeamMatchQueue();

    void addMember(Player* pPlayer);
    bool hasEnoughMemberForTeam(uint32_t tier);
    std::vector<Player*> getPlayersForTeam(uint32_t tier);
    const std::map<uint32_t, std::vector<Player*>> getTierQueue() const;
    void clear();

    mutable std::mutex mutex;

private:
    void _clearNoLock();
    std::map<uint32_t, std::vector<Player*>> m_mapTierQueues;
};

// 2個隊伍組成一場戰鬥，並且每個隊伍有一個等級 (tier)，這個等級用來匹配對手
class BattleMatchQueue
{
    friend class BattleManager;
public:
    BattleMatchQueue();
    ~BattleMatchQueue();

    void addTeam(std::vector<Player*> team);
    bool hasEnoughTeamsForBattle(uint32_t tier);
    std::vector<std::vector<Player*>> getTeamsForBattle(uint32_t tier);
    const std::map<uint32_t, std::vector<std::vector<Player*>>> getTierQueue() const;
    void clear();

    mutable std::mutex mutex;

private:
    void _clearNoLock();
    std::map<uint32_t, std::vector<std::vector<Player*>>> m_mapTierQueues;
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

    void PlayerWin(uint64_t playerId);
    void PlayerLose(uint64_t playerId);

    // 獲取下一個自動增長的房間 ID
    uint64_t getNextRoomId();
    // 讓 BattleRoom 調用這個函數來請求管理器移除自己
    void removeBattleRoom(uint64_t roomId);

    const TeamMatchQueue* getTeamMatchQueue() const { return &m_teamMatchQueue; }
    const BattleMatchQueue* getBattleMatchQueue() const { return &m_battleMatchQueue; }

private:
    BattleManager();
    ~BattleManager();

    BattleManager(const BattleManager&) = delete;
    BattleManager& operator=(const BattleManager&) = delete;
    BattleManager(BattleManager&&) = delete;
    BattleManager& operator=(BattleManager&&) = delete;

    void matchmakingThread();

    std::atomic<bool> m_isRunning = false;
    std::thread m_matchmakingThreadHandle;

    TeamMatchQueue m_teamMatchQueue{};
    BattleMatchQueue m_battleMatchQueue{};

    // 用於管理所有活躍的戰鬥房間
    std::map<uint64_t, std::unique_ptr<BattleRoom>> m_battleRooms{};
    // 下一個可用的房間 ID，使用 std::atomic 保證原子性
    std::atomic<uint64_t> m_nextRoomId = 1;
    std::mutex m_playerAddQueueMutex; // 新增一個互斥鎖來保護玩家入隊邏輯

    std::mutex m_battleRoomsMutex;
};

#endif // BATTLE_MANAGER_H