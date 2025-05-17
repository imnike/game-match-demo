// battleManager.h
#ifndef BATTLE_MANAGER_H
#define BATTLE_MANAGER_H
#include "objects/player.h"
#include "objects/hero.h"
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic> // �T�O�]�t <atomic>

class BattleRoom
{
public:
    BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue);
    ~BattleRoom();
    void startBattle();
    void finishBattle(); // finishBattle �̵M�s�b�A�Ω�M�z

    uint64_t getRoomId() const { return m_roomId; } // ���S roomId

private:
    uint64_t m_roomId; // �s�W roomId
    std::vector<std::unique_ptr<Hero>> m_vecTeamRed;
    std::vector<std::unique_ptr<Hero>> m_vecTeamBlue;
};

// 3�쪱�a�զ��@�Ӷ���A�åB�C�Ӷ���@�ӵ��� (tier)�A�o�ӵ��ťΨӤǰt���
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

// 2�Ӷ���զ��@���԰��A�åB�C�Ӷ���@�ӵ��� (tier)�A�o�ӵ��ťΨӤǰt���
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

    void PlayerWin(uint64_t playerId);
    void PlayerLose(uint64_t playerId);

    // ����U�@�Ӧ۰ʼW�����ж� ID
    uint64_t getNextRoomId();
    // �� BattleRoom �եγo�Ө�ƨӽШD�޲z�������ۤv
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

    // �Ω�޲z�Ҧ����D���԰��ж�
    std::map<uint64_t, std::unique_ptr<BattleRoom>> m_battleRooms{};
    // �U�@�ӥi�Ϊ��ж� ID�A�ϥ� std::atomic �O�ҭ�l��
    std::atomic<uint64_t> m_nextRoomId = 1;
    std::mutex m_playerAddQueueMutex; // �s�W�@�Ӥ�����ӫO�@���a�J���޿�

    std::mutex m_battleRoomsMutex;
};

#endif // BATTLE_MANAGER_H