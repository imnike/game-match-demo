// @file :  battleManager.h
// @brief:  管理戰鬥, 匹配隊伍, 處理戰鬥結果等功能
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
    // 遍歷原始 Player 指標，為每個 Player 創建一個全新的副本
    for (Player* pPlayer : refVecTeamRed) 
    {
        if (pPlayer)
        {
            // 使用 std::make_unique<Player>(*p) 來調用 Player 的拷貝建構子，
            // 在堆上創建一個新的 Player 物件副本，並將其包裝在 std::unique_ptr 中。
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
    // 顯示戰鬥開始狀態
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

    // 隨機決定獲勝隊伍 (紅隊或藍隊)
    // random_utils::getRandom(2) 會返回 0 或 1。
    // 如果返回 0 則紅隊贏，返回 1 則藍隊贏
    uint8_t dice = 2;
    bool isRedWin = (random_utils::getRandom(dice) == 0);

    std::vector<std::unique_ptr<Player>>& vecWinningTeam = isRedWin ? m_vecTeamRed : m_vecTeamBlue;
    std::vector<std::unique_ptr<Player>>& vecLosingTeam = isRedWin ? m_vecTeamBlue : m_vecTeamRed;

    std::cout << "\n" << (isRedWin ? "Red" : "Blue") << " Team wins!" << std::endl;

    // 更新獲勝隊伍和落敗隊伍的分數
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
        PlayerManager::instance().playerLogout(pPlayer->getId()); // 戰鬥結束，玩家登出
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
        PlayerManager::instance().playerLogout(pPlayer->getId()); // 戰鬥結束，玩家登出
    }
    std::cout << "----- BATTLE ENDS -----\n" << std::endl;
}

void BattleRoom::finishBattle()
{
    // 在這裡可以添加結束戰鬥的邏輯，例如釋放資源、更新玩家狀態等
    std::cout << "Battle finished." << std::endl;

	// 回存玩家資料??
   
    // 釋放隊伍成員的記憶體
    m_vecTeamRed.clear();
	m_vecTeamBlue.clear();
}

// --- TeamMatchQueue Implementation (保持不變) ---
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

// --- BattleMatchQueue Implementation (保持不變) ---
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
        // --- 階段一：玩家到隊伍 (Player to Team) ---
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

        // --- 階段二：隊伍到戰鬥 (Team to Battle) ---
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

                    // 將第一支隊伍設為紅隊，第二支為藍隊
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