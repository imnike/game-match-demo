// battleManager.h
#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H
#include "objects/player.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

// --- BattleRoom ���O (�԰��ж�) ---
class BattleRoom
{
public:
    // �N�¶��ն��אּ�����Ŷ�
    BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue);
    ~BattleRoom();
    void startBattle();
    void finishBattle();

private:
    std::vector<std::unique_ptr<Player>> m_vecTeamRed;
    std::vector<std::unique_ptr<Player>> m_vecTeamBlue;
};

// --- TeamMatchQueue ���O (�O�����ܡA�]�����B�z���O��Ӫ��a) ---
// �Ω�N��Ӫ��a�ǰt������ (�Ҧp 3 �H�@��)
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

// --- BattleMatchQueue ���O (�O�����ܡA�]�����B�z���O����C��) ---
// �Ω�N�w�զ�������ǰt���԰� (�Ҧp 2 �䶤��@���԰�)
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

// --- BattleManager ���O (��ҼҦ�) ---
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
    BattleManager(BattleManager&&) = delete;
    BattleManager& operator=(BattleManager&&) = delete;

    void matchmakingThread();

    std::atomic<bool> isRunning = false;
    std::thread matchmakingThreadHandle;

    TeamMatchQueue teamMatchQueue;
    BattleMatchQueue battleMatchQueue;
};

#endif // BATTLE_MANAGER_H