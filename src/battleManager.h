// battleManager.h
#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H
#include "objects/player.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

// �e�V�n�� PlayerManager (�p�G�S���b player.h ���]�t)
class PlayerManager;
class Player;

// --- BattleRoom ���O (�԰��ж�) ---
class BattleRoom
{
public:
    // �N�¶��ն��אּ�����Ŷ�
    BattleRoom(std::vector<Player*> vecTeamRed, std::vector<Player*> vecTeamBlue);
    ~BattleRoom();
    void startBattle();

private:
    std::vector<Player*> vecTeamRed;
    std::vector<Player*> vecTeamBlue;
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

    void matchmakingThread();

    std::atomic<bool> isRunning;
    std::thread matchmakingThreadHandle;

    TeamMatchQueue teamMatchQueue;
    BattleMatchQueue battleMatchQueue;
};

#endif // BATTLE_MANAGER_H