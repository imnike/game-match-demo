// @file  : battleManager.cpp
// @brief : �޲z�԰�, �ǰt����, �B�z�԰����G���\��
// @author: August
// @date  : 2025-05-15
#include "battleManager.h"
#include "playerManager.h"
#include "../include/globalDefine.h"
#include "../utils/utils.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

BattleRoom::BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue)
    : m_roomId(BattleManager::instance().getNextRoomId()) // �b�c�y������ó]�m roomId
{
    for (Player* pPlayer : refVecTeamRed)
    {
        if (pPlayer)
        {
            pPlayer->setStatus(common::PlayerStatus::battle);
            m_vecTeamRed.emplace_back(std::make_unique<Hero>(pPlayer->getId()));
        }
    }

    for (Player* pPlayer : refVecTeamBlue)
    {
        if (pPlayer)
        {
            pPlayer->setStatus(common::PlayerStatus::battle);
            m_vecTeamBlue.emplace_back(std::make_unique<Hero>(pPlayer->getId()));
        }
    }
    std::cout << "Battle Room " << m_roomId << " created." << std::endl;
}

BattleRoom::~BattleRoom()
{
    std::cout << "Battle Room " << m_roomId << " destroyed." << std::endl;
}

void BattleRoom::startBattle()
{
    // �C�X�����P�Ŷ�����ID
    std::cout << "red team members: ";
    for (const auto& pHero : m_vecTeamRed)
    {
        if (pHero)
        {
            std::cout << pHero->getPlayerId() << "(hero:" << pHero->getId() << ") ";
        }
    }
    std::cout << std::endl;

    std::cout << "blue team members: ";
    for (const auto& pHero : m_vecTeamBlue)
    {
        if (pHero)
        {
            std::cout << pHero->getPlayerId() << "(hero:" << pHero->getId() << ") ";
        }
    }
    std::cout << std::endl;

    std::cout << "\n----- BATTLE STARTS (Room " << m_roomId << ") -----" << std::endl;

    // �����԰��L�{
    std::this_thread::sleep_for(std::chrono::seconds(3));

    uint8_t dice = 2;
    bool isRedWin = (random_utils::getRandom(dice) == 0);

    std::vector<std::unique_ptr<Hero>>& vecWinningTeam = isRedWin ? m_vecTeamRed : m_vecTeamBlue;
    std::vector<std::unique_ptr<Hero>>& vecLosingTeam = isRedWin ? m_vecTeamBlue : m_vecTeamRed;

    std::cout << "\n" << (isRedWin ? "Red" : "Blue") << " Team wins in Room " << m_roomId << "!!!" << std::endl;

    for (auto& pHero : vecWinningTeam)
    {
        if (!pHero) continue;
        const uint64_t playerId = pHero->getPlayerId();
        BattleManager::instance().PlayerWin(playerId);
    }

    for (auto& pHero : vecLosingTeam)
    {
        if (!pHero) continue;
        const uint64_t playerId = pHero->getPlayerId();
        BattleManager::instance().PlayerLose(playerId);
    }
    std::cout << "----- BATTLE ENDS (Room " << m_roomId << ") -----\n" << std::endl;

    // �԰������A�q�� BattleManager �����өж�
    finishBattle();
}

void BattleRoom::finishBattle()
{
    std::cout << "Battle finished for Room " << m_roomId << "." << std::endl;
    m_vecTeamRed.clear();
    m_vecTeamBlue.clear();
    // �q�� BattleManager �����o�өж�
    BattleManager::instance().removeBattleRoom(m_roomId);
}

// --- TeamMatchQueue Implementation (�O������) ---
TeamMatchQueue::TeamMatchQueue() {}
TeamMatchQueue::~TeamMatchQueue() {}

void TeamMatchQueue::addMember(Player* pPlayer)
{
    std::lock_guard<std::mutex> lock(mutex);
    uint32_t tier = pPlayer->getTier();
    m_mapTierQueues[tier].push_back(pPlayer);
    std::cout << "Player " << pPlayer->getId() << " added to TEAM match queue for tier " << tier << std::endl;
}

bool TeamMatchQueue::hasEnoughMemberForTeam(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = m_mapTierQueues.find(tier);
    return it != m_mapTierQueues.end() && it->second.size() >= 3;
}

std::vector<Player*> TeamMatchQueue::getPlayersForTeam(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Player*> teamPlayers;
    auto it = m_mapTierQueues.find(tier);

    if (it != m_mapTierQueues.end() && it->second.size() >= 3)
    {
        teamPlayers.assign(it->second.begin(), it->second.begin() + 3);
        it->second.erase(it->second.begin(), it->second.begin() + 3);
        if (it->second.empty())
        {
            m_mapTierQueues.erase(it);
        }
    }
    return teamPlayers;
}

const std::map<uint32_t, std::vector<Player*>> TeamMatchQueue::getTierQueue() const
{
    std::lock_guard<std::mutex> lock(mutex);
	return m_mapTierQueues;
}

void TeamMatchQueue::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    _clearNoLock();
}

void TeamMatchQueue::_clearNoLock()
{
    m_mapTierQueues.clear();
}


// --- BattleMatchQueue Implementation (�O������) ---
BattleMatchQueue::BattleMatchQueue() {}
BattleMatchQueue::~BattleMatchQueue() {}

void BattleMatchQueue::addTeam(std::vector<Player*> team)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (team.empty()) { return; }
    uint32_t tier = team[0]->getTier(); // ���]������� Tier �ۦP
    m_mapTierQueues[tier].emplace_back(team);

    std::cout << "Team (Tier " << tier << ") added to BATTLE match queue. Players: ";
    for (Player* p : team)
    {
        std::cout << p->getId() << " ";
    }
    std::cout << std::endl;
}

bool BattleMatchQueue::hasEnoughTeamsForBattle(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = m_mapTierQueues.find(tier);
    return it != m_mapTierQueues.end() && it->second.size() >= battle_constant::TeamColor::Max;
}

std::vector<std::vector<Player*>> BattleMatchQueue::getTeamsForBattle(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::vector<Player*>> battleTeams;
    auto it = m_mapTierQueues.find(tier);

    if (it != m_mapTierQueues.end() && it->second.size() >= battle_constant::TeamColor::Max)
    {
        battleTeams.assign(it->second.begin(), it->second.begin() + 2);
        it->second.erase(it->second.begin(), it->second.begin() + 2);
        if (it->second.empty())
        {
            m_mapTierQueues.erase(it);
        }
    }
    return battleTeams;
}

const std::map<uint32_t, std::vector<std::vector<Player*>>> BattleMatchQueue::getTierQueue() const
{
    std::lock_guard<std::mutex> lock(mutex);
	return m_mapTierQueues;
}

void BattleMatchQueue::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    _clearNoLock();
}

void BattleMatchQueue::_clearNoLock()
{
    m_mapTierQueues.clear();
}

BattleManager& BattleManager::instance()
{
    static BattleManager instance;
    return instance;
}

BattleManager::BattleManager()
{
}

BattleManager::~BattleManager()
{
    stopMatchmaking(); // �T�O�u�{�w���h�X
    // �M�ũҦ��԰��ж�
    std::lock_guard<std::mutex> lock(m_battleRoomsMutex);
    m_battleRooms.clear();
}

bool BattleManager::initialize()
{
    m_isRunning = false;
    return true;
}

void BattleManager::release()
{
    stopMatchmaking();

    std::unique_lock<std::mutex> lockBattleRooms(m_battleRoomsMutex, std::defer_lock);
    std::unique_lock<std::mutex> lockTeamQueue(m_teamMatchQueue.mutex, std::defer_lock);
    std::unique_lock<std::mutex> lockBattleQueue(m_battleMatchQueue.mutex, std::defer_lock);

    std::lock(lockBattleRooms, lockTeamQueue, lockBattleQueue);

    m_battleRooms.clear();
    m_teamMatchQueue._clearNoLock();
    m_battleMatchQueue._clearNoLock();

    m_nextRoomId.store(0);
}

void BattleManager::startMatchmaking()
{
    if (!m_isRunning)
    {
        m_isRunning = true;
        m_matchmakingThreadHandle = std::thread(&BattleManager::matchmakingThread, this);
    }
}

void BattleManager::stopMatchmaking()
{
    if (m_isRunning)
    {
        m_isRunning = false;
        if (m_matchmakingThreadHandle.joinable())
        {
            m_matchmakingThreadHandle.join();
        }
    }
}

void BattleManager::addPlayerToQueue(Player* pPlayer)
{
    if (!pPlayer)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_playerAddQueueMutex);
    {
        if (pPlayer->isInLobby() == false)
        {
            //std::cerr << "Error: Player " << pPlayer->getId() << " is not in lobby." << std::endl;
            return;
        }
        m_teamMatchQueue.addMember(pPlayer);
        pPlayer->setStatus(common::PlayerStatus::queue);
    }
}

void BattleManager::PlayerWin(uint64_t playerId)
{
    std::cout << "Player " << playerId << " WIN!!! (+ " << battle_constant::WINNER_SCORE << " points)" << std::endl;
    PlayerManager::instance().handlePlayerBattleResult(playerId, battle_constant::WINNER_SCORE, true);
}

void BattleManager::PlayerLose(uint64_t playerId)
{
    std::cout << "Player " << playerId << " LOSE... (" << battle_constant::LOSER_SCORE << " points)" << std::endl;
    PlayerManager::instance().handlePlayerBattleResult(playerId, battle_constant::LOSER_SCORE, false);
}

// ����U�@�Ӧ۰ʼW�����ж� ID
uint64_t BattleManager::getNextRoomId()
{
    // �ϥ� fetch_add ��k��{��l�ʪ��ۼW�ާ@�A��^�­�
    return m_nextRoomId.fetch_add(1);
}

// BattleRoom �եΦ���ƨӽШD�޲z�������ۤv
void BattleManager::removeBattleRoom(uint64_t roomId)
{
    // �O�@ m_battleRooms ���ק�ާ@�A�P getNextRoomId() �ϥΤ��P������A�����|�Ĭ�
    std::lock_guard<std::mutex> lock(m_battleRoomsMutex);
    auto it = m_battleRooms.find(roomId);
    if (it != m_battleRooms.end())
    {
        m_battleRooms.erase(it); // unique_ptr �|�b���B�۰ʽե� BattleRoom ���R�c���
        std::cout << "Removed Battle Room " << roomId << " from manager." << std::endl;
    }
    else
    {
        std::cerr << "Error: Attempted to remove non-existent Battle Room " << roomId << std::endl;
    }
}

void BattleManager::matchmakingThread()
{
    std::cout << "Matchmaking thread started" << std::endl;

    while (m_isRunning)
    {
        // 1. 
        std::vector<uint32_t> tiersToFormTeams;
        {
            std::lock_guard<std::mutex> lock(m_teamMatchQueue.mutex);
            for (const auto& queuePair : m_teamMatchQueue.m_mapTierQueues)
            {
                tiersToFormTeams.emplace_back(queuePair.first);
            }
        }

        for (uint32_t tier : tiersToFormTeams)
        {
            if (m_teamMatchQueue.hasEnoughMemberForTeam(tier))
            {
                std::vector<Player*> newTeam = m_teamMatchQueue.getPlayersForTeam(tier);

                if (newTeam.size() == 3)
                {
                    std::cout << "Formed a 3-player team for tier " << tier << ". Players: ";
                    for (Player* p : newTeam) 
                    {
                        std::cout << p->getId() << " ";
                    }
                    std::cout << std::endl;

                    m_battleMatchQueue.addTeam(newTeam);
                }
                else {
                    std::cerr << "Warning: getPlayersForTeam returned incorrect number of players for tier " << tier << std::endl;
                }
            }
        }

        // --- ���q�G�G�����԰� (Team to Battle) ---
        std::vector<uint32_t> tiersToStartBattles;
        {
            std::lock_guard<std::mutex> lock(m_battleMatchQueue.mutex);
            for (const auto& queuePair : m_battleMatchQueue.m_mapTierQueues)
            {
                tiersToStartBattles.emplace_back(queuePair.first);
            }
        }

        for (uint32_t tier : tiersToStartBattles)
        {
            if (m_battleMatchQueue.hasEnoughTeamsForBattle(tier))
            {
                std::vector<std::vector<Player*>> battleTeams = m_battleMatchQueue.getTeamsForBattle(tier);

                if (battleTeams.size() == battle_constant::TeamColor::Max)
                {
                    std::cout << "\nMatched 2 teams for tier " << tier << ". Initiating battle!\n";

                    std::unique_ptr<BattleRoom> room_ptr;
                    uint64_t roomIdForThread = 0;

                    // 1. �b��w m_battleRoomsMutex �����p�U�Ы� BattleRoom�A�ñN���J map
                    //    �`�N�G�o�� m_battleRoomsMutex �u�O�@ m_battleRooms map ���ק�C
                    //    m_nextRoomId �w�g�� std::atomic �O�@�C
                    {
                        std::lock_guard<std::mutex> lock(m_battleRoomsMutex); // ��w m_battleRooms
                        room_ptr = std::make_unique<BattleRoom>(battleTeams[battle_constant::TeamColor::Red], battleTeams[battle_constant::TeamColor::Blue]);
                        roomIdForThread = room_ptr->getRoomId(); // ����ж� ID (�b BattleRoom �c�y��Ƥ��w��l�ͦ�)
                        m_battleRooms[roomIdForThread] = std::move(room_ptr); // �N unique_ptr ���ʨ� map ��
                    } // ��b���B����

                    // 2. �Ұʤ@�ӷs�� detached �u�{�Ӱ��� BattleRoom �� startBattle()
                    std::thread battle_thread([roomIdForThread]() 
                        {
                        BattleRoom* battleRoomInstance = nullptr;
                        {
                            // �A����w m_battleRoomsMutex �Ӧw���X�� map
                            std::lock_guard<std::mutex> lock(BattleManager::instance().m_battleRoomsMutex);
                            auto it = BattleManager::instance().m_battleRooms.find(roomIdForThread);
                            if (it != BattleManager::instance().m_battleRooms.end()) 
                            {
                                battleRoomInstance = it->second.get(); // ����r���w
                            }
                        } // ��b���B����

                        if (battleRoomInstance) 
                        {
                            battleRoomInstance->startBattle(); // ����԰��A�԰�������|�۰ʽե� finishBattle
                        }
                        else 
                        {
                            std::cerr << "Error: Battle Room " << roomIdForThread << " not found for battle thread execution." << std::endl;
                        }
                        });
                    battle_thread.detach(); // ����������A�����W�߹B��
                }
                else {
                    std::cerr << "Warning: getTeamsForBattle returned incorrect number of teams for tier " << tier << std::endl;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Matchmaking thread stopped" << std::endl;
}