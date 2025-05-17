// @file :  battleManager.h
// @brief:  �޲z�԰�, �ǰt����, �B�z�԰����G���\��
// author:  August
// date  :  2025-05-14
#include "battleManager.h"
#include "playerManager.h"
#include "../include/globalDefine.h"
#include "../utils/utils.h"
#include <iostream>
#include <algorithm>
#include <memory>
#include <random>
#include <thread>
#include <chrono>

// --- BattleRoom Implementation ---
BattleRoom::BattleRoom(const std::vector<Player*>& refVecTeamRed, const std::vector<Player*>& refVecTeamBlue)
{
    // �M����l Player ���СA���C�� Player �Ыؤ@�ӥ��s���ƥ�
    for (Player* pPlayer : refVecTeamRed) 
    {
        if (pPlayer)
        {
            // �ϥ� std::make_unique<Player>(*p) �ӽե� Player �������غc�l�A
            // �b��W�Ыؤ@�ӷs�� Player ����ƥ��A�ñN��]�˦b std::unique_ptr ���C
            m_vecTeamRed.push_back(std::make_unique<Player>(*pPlayer));
        }
    }

    for (Player* pPlayer : refVecTeamBlue) 
    {
        if (pPlayer) 
        {
            m_vecTeamBlue.push_back(std::make_unique<Player>(*pPlayer));
        }
    }
}

BattleRoom::~BattleRoom()
{
}

void BattleRoom::startBattle()
{
    // ��ܾ԰��}�l���A
    std::cout << "\n----- BATTLE STARTS -----" << std::endl;
    std::cout << "Red Team (Tier " << (m_vecTeamRed.empty() ? 0 : m_vecTeamRed[0]->getTier()) << ", Players: ";
    for (const auto& player : m_vecTeamRed)
    {
        std::cout << player->getId() << " ";
    }
    std::cout << "):" << std::endl;
    for (const auto& pPlayer : m_vecTeamRed)
    {
        std::cout << "  Player " << pPlayer->getId()
            << " (Score: " << pPlayer->getScore()
            << ", Tier: " << pPlayer->getTier() << ")" << std::endl;
    }

    std::cout << "Blue Team (Tier " << (m_vecTeamBlue.empty() ? 0 : m_vecTeamBlue[0]->getTier()) << ", Players: ";
    for (const auto& pPlayer : m_vecTeamBlue)
    {
        std::cout << pPlayer->getId() << " ";
    }
    std::cout << "):" << std::endl;
    for (const auto& pPlayer : m_vecTeamBlue)
    {
        std::cout << "  Player " << pPlayer->getId()
            << " (Score: " << pPlayer->getScore()
            << ", Tier: " << pPlayer->getTier() << ")" << std::endl;
    }

    // �H���M�w��Ӷ��� (�������Ŷ�)
    // random_utils::getRandom(2) �|��^ 0 �� 1�C
    // �p�G��^ 0 �h����Ĺ�A��^ 1 �h�Ŷ�Ĺ
    uint8_t dice = 2;
    bool isRedWin = (random_utils::getRandom(dice) == 0);

    std::vector<std::unique_ptr<Player>>& vecWinningTeam = isRedWin ? m_vecTeamRed : m_vecTeamBlue;
    std::vector<std::unique_ptr<Player>>& vecLosingTeam = isRedWin ? m_vecTeamBlue : m_vecTeamRed;

    std::cout << "\n" << (isRedWin ? "Red" : "Blue") << " Team wins!" << std::endl;

    // ��s��Ӷ���M���Ѷ������
    for (auto& pPlayer : vecWinningTeam)
    {
        if (pPlayer == nullptr)
        {
            continue;
        }
        BattleManager::instance().PlayerWin(pPlayer.get());

        std::cout << "  Player " << pPlayer->getId()
            << " gets +" << battle_constant::WINNER_SCORE
            << " points, new score: " << pPlayer->getScore()
            << ", new tier: " << pPlayer->getTier() << std::endl;
        PlayerManager::instance().playerLogout(pPlayer->getId()); // �԰������A���a�n�X
    }

    for (auto& pPlayer : vecLosingTeam)
    {
        if (pPlayer == nullptr)
        {
            continue;
        }
        BattleManager::instance().PlayerLose(pPlayer.get());

        std::cout << "  Player " << pPlayer->getId()
            << " loses " << battle_constant::LOSER_SCORE
            << " points, new score: " << pPlayer->getScore()
            << ", new tier: " << pPlayer->getTier() << std::endl;
        PlayerManager::instance().playerLogout(pPlayer->getId()); // �԰������A���a�n�X
    }
    std::cout << "----- BATTLE ENDS -----\n" << std::endl;
}

void BattleRoom::finishBattle()
{
    // �b�o�̥i�H�K�[�����԰����޿�A�Ҧp����귽�B��s���a���A��
    std::cout << "Battle finished." << std::endl;

	// �^�s���a���??
   
    // ���񶤥�����O����
    m_vecTeamRed.clear();
	m_vecTeamBlue.clear();
}

// --- TeamMatchQueue Implementation (�O������) ---
TeamMatchQueue::TeamMatchQueue() {}
TeamMatchQueue::~TeamMatchQueue() {}

void TeamMatchQueue::addMember(Player* pPlayer)
{
    std::lock_guard<std::mutex> lock(mutex);

    uint32_t tier = pPlayer->getTier();
    mapTierQueues[tier].push_back(pPlayer);

    std::cout << "Player " << pPlayer->getId() << " added to TEAM match queue for tier " << tier << std::endl;
}

bool TeamMatchQueue::hasEnoughMemberForTeam(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = mapTierQueues.find(tier);
    return it != mapTierQueues.end() && it->second.size() >= 3;
}

std::vector<Player*> TeamMatchQueue::getPlayersForTeam(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Player*> teamPlayers;
    auto it = mapTierQueues.find(tier);

    if (it != mapTierQueues.end() && it->second.size() >= 3)
    {
        teamPlayers.assign(it->second.begin(), it->second.begin() + 3);
        it->second.erase(it->second.begin(), it->second.begin() + 3);

        if (it->second.empty())
        {
            mapTierQueues.erase(it);
        }
    }
    return teamPlayers;
}

// --- BattleMatchQueue Implementation (�O������) ---
BattleMatchQueue::BattleMatchQueue() {}
BattleMatchQueue::~BattleMatchQueue() {}

void BattleMatchQueue::addTeam(std::vector<Player*> team)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (team.empty()) {
        return;
    }
    uint32_t tier = team[battle_constant::TeamColor::Red]->getTier();
    mapTierQueues[tier].push_back(team);

    std::cout << "Team (Tier " << tier << ") added to BATTLE match queue. Players: ";
    for (Player* p : team) {
        std::cout << p->getId() << " ";
    }
    std::cout << std::endl;
}

bool BattleMatchQueue::hasEnoughTeamsForBattle(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = mapTierQueues.find(tier);
    return it != mapTierQueues.end() && it->second.size() >= battle_constant::TeamColor::Max;
}

std::vector<std::vector<Player*>> BattleMatchQueue::getTeamsForBattle(uint32_t tier)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::vector<Player*>> battleTeams;
    auto it = mapTierQueues.find(tier);

    if (it != mapTierQueues.end() && it->second.size() >= battle_constant::TeamColor::Max)
    {
        battleTeams.assign(it->second.begin(), it->second.begin() + 2);
        it->second.erase(it->second.begin(), it->second.begin() + 2);

        if (it->second.empty())
        {
            mapTierQueues.erase(it);
        }
    }
    return battleTeams;
}

// --- BattleManager Implementation (Singleton) ---
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
}

bool BattleManager::initialize()
{
    //startMatchmaking();
    isRunning = false;
    return true;
}

void BattleManager::release()
{
    stopMatchmaking();
}

void BattleManager::startMatchmaking()
{
    if (!isRunning)
    {
        isRunning = true;
        matchmakingThreadHandle = std::thread(&BattleManager::matchmakingThread, this);
    }
}

void BattleManager::stopMatchmaking()
{
    if (isRunning)
    {
        isRunning = false;
        if (matchmakingThreadHandle.joinable())
        {
            matchmakingThreadHandle.join();
        }
    }
}

void BattleManager::addPlayerToQueue(Player* pPlayer)
{
    teamMatchQueue.addMember(pPlayer);
}

void BattleManager::PlayerWin(Player* pPlayer)
{
    if (pPlayer == nullptr)
    {
        return;
    }
	PlayerManager::instance().updatePlayerBattleResult(pPlayer->getId(), battle_constant::WINNER_SCORE, true);
}

void BattleManager::PlayerLose(Player* pPlayer)
{
    if (pPlayer == nullptr)
    {
        return;
    }
    PlayerManager::instance().updatePlayerBattleResult(pPlayer->getId(), battle_constant::LOSER_SCORE, false);
}

void BattleManager::matchmakingThread()
{
    std::cout << "Matchmaking thread started" << std::endl;

    while (isRunning)
    {
        // --- ���q�@�G���a�춤�� (Player to Team) ---
        std::vector<uint32_t> tiersToFormTeams;
        {
            std::lock_guard<std::mutex> lock(teamMatchQueue.mutex);
            for (const auto& queuePair : teamMatchQueue.mapTierQueues)
            {
                tiersToFormTeams.push_back(queuePair.first);
            }
        }

        for (uint32_t tier : tiersToFormTeams)
        {
            if (teamMatchQueue.hasEnoughMemberForTeam(tier))
            {
                std::vector<Player*> newTeam = teamMatchQueue.getPlayersForTeam(tier);

                if (newTeam.size() == 3)
                {
                    std::cout << "  Formed a 3-player team for tier " << tier << ". Players: ";
                    for (Player* p : newTeam) {
                        std::cout << p->getId() << " ";
                    }
                    std::cout << std::endl;

                    battleMatchQueue.addTeam(newTeam);
                }
                else {
                    std::cerr << "Warning: getPlayersForTeam returned incorrect number of players for tier " << tier << std::endl;
                }
            }
        }

        // --- ���q�G�G�����԰� (Team to Battle) ---
        std::vector<uint32_t> tiersToStartBattles;
        {
            std::lock_guard<std::mutex> lock(battleMatchQueue.mutex);
            for (const auto& queuePair : battleMatchQueue.mapTierQueues)
            {
                tiersToStartBattles.push_back(queuePair.first);
            }
        }

        for (uint32_t tier : tiersToStartBattles)
        {
            if (battleMatchQueue.hasEnoughTeamsForBattle(tier))
            {
                std::vector<std::vector<Player*>> battleTeams = battleMatchQueue.getTeamsForBattle(tier);

                if (battleTeams.size() == battle_constant::TeamColor::Max)
                {
                    std::cout << "\n  Matched 2 teams for tier " << tier << ". Initiating battle!\n";

                    // �N�Ĥ@�䶤��]�������A�ĤG�䬰�Ŷ�
                    BattleRoom room(battleTeams[battle_constant::TeamColor::Red], battleTeams[battle_constant::TeamColor::Blue]);
                    room.startBattle();
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