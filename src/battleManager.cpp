#include "battleManager.h"
#include "..\include\globalDefine.h"
#include <iostream>
#include <algorithm> 
#include <random>
#include <thread>
#include <chrono>

// --- BattleRoom Implementation ---
BattleRoom::BattleRoom(std::vector<Player*> blackTeam, std::vector<Player*> whiteTeam)
    : blackTeam(blackTeam), whiteTeam(whiteTeam)
{
}

BattleRoom::~BattleRoom()
{
}

void BattleRoom::startBattle()
{
    // Display initial battle state
    std::cout << "\n----- BATTLE STARTS -----" << std::endl;
    std::cout << "Black Team (Tier " << (blackTeam.empty() ? 0 : blackTeam[0]->getTier()) << "):" << std::endl;
    for (const auto& player : blackTeam)
    {
        std::cout << "  Player " << player->getId()
            << " (Score: " << player->getScore()
            << ", Tier: " << player->getTier() << ")" << std::endl;
    }

    std::cout << "White Team (Tier " << (whiteTeam.empty() ? 0 : whiteTeam[0]->getTier()) << "):" << std::endl;
    for (const auto& pPlayer : whiteTeam)
    {
        std::cout << "  Player " << pPlayer->getId()
            << " (Score: " << pPlayer->getScore()
            << ", Tier: " << pPlayer->getTier() << ")" << std::endl;
    }

    // Randomly decide the winning team (Black or White)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    bool blackTeamWins = dis(gen) == 1;

    std::vector<Player*>& winningTeam = blackTeamWins ? blackTeam : whiteTeam;
    std::vector<Player*>& losingTeam = blackTeamWins ? whiteTeam : blackTeam;

    std::cout << "\n" << (blackTeamWins ? "Black" : "White") << " Team wins!" << std::endl;

    // Update scores for winning and losing teams
    for (auto& pPlayer : winningTeam)
    {
        if (pPlayer == nullptr)
        {
			continue;
        }
		BattleManager::instance().PlayerWin(pPlayer);

        std::cout << "  Player " << pPlayer->getId()
            << " gets +50 points, new score: " << pPlayer->getScore()
            << ", new tier: " << pPlayer->getTier() << std::endl;
        PlayerManager::instance().playerLogout(pPlayer->getId());
    }

    for (auto& pPlayer : losingTeam)
    {
        if (pPlayer == nullptr)
        {
            continue;
        }
        BattleManager::instance().PlayerLose(pPlayer);

        std::cout << "  Player " << pPlayer->getId()
            << " loses 50 points, new score: " << pPlayer->getScore()
            << ", new tier: " << pPlayer->getTier() << std::endl;
        PlayerManager::instance().playerLogout(pPlayer->getId());
    }
    std::cout << "----- BATTLE ENDS -----\n" << std::endl;
}

// --- MatchQueue Implementation ---
MatchQueue::MatchQueue()
{
}

MatchQueue::~MatchQueue()
{
}

void MatchQueue::addPlayer(Player* pPlayer)
{
    std::lock_guard<std::mutex> lock(mutex);

    uint32_t tier = pPlayer->getTier(); // Get player's tier
    tierQueues[tier].push_back(pPlayer);

    std::cout << "Player " << pPlayer->getId() << " added to match queue for tier " << tier << std::endl;
}

bool MatchQueue::hasEnoughPlayersForTier(uint32_t tier) // Changed to tier
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = tierQueues.find(tier);
    // We need 6 players for a 3v3 match (3 per team)
    return it != tierQueues.end() && it->second.size() >= 6;
}

std::vector<Player*> MatchQueue::getPlayersForMatch(uint32_t tier) // Changed to tier
{
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Player*> matchedPlayers;
    auto it = tierQueues.find(tier);

    if (it != tierQueues.end() && it->second.size() >= 6)
    {
        // Take the first 6 players for the match
        matchedPlayers.assign(it->second.begin(), it->second.begin() + 6);

        // Remove these players from the queue
        it->second.erase(it->second.begin(), it->second.begin() + 6);

        // If the queue for this tier becomes empty, remove it from the map
        if (it->second.empty())
        {
            tierQueues.erase(it);
        }
    }

    return matchedPlayers;
}

// --- BattleManager Implementation (Singleton) ---
BattleManager& BattleManager::instance()
{
    static BattleManager instance;
    return instance;
}

BattleManager::BattleManager()
    : isRunning(false)
{
}

BattleManager::~BattleManager()
{
    stopMatchmaking();
}

bool BattleManager::initialize()
{
    startMatchmaking();
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
            matchmakingThreadHandle.join(); // Wait for the matchmaking thread to finish
        }
    }
}

void BattleManager::addPlayerToQueue(Player* pPlayer)
{
    matchQueue.addPlayer(pPlayer);
}

void BattleManager::PlayerWin(Player* pPlayer)
{
    if (pPlayer == nullptr)
    {
		return;
    }
	pPlayer->addWins();
    pPlayer->addScore(BattleResult::WINNER_SCORE);
    pPlayer->save();
}

void BattleManager::PlayerLose(Player* pPlayer)
{
    if (pPlayer == nullptr)
    {
        return;
    }
    pPlayer->subScore(BattleResult::LOSER_SCORE);
	pPlayer->save();
}

void BattleManager::matchmakingThread()
{
    std::cout << "Matchmaking thread started" << std::endl;

    while (isRunning)
    {
        // Get all tiers that currently have players in queues
        std::vector<uint32_t> tiersToCheck;
        {
            std::lock_guard<std::mutex> lock(matchQueue.mutex);
            for (const auto& queuePair : matchQueue.tierQueues)
            {
                tiersToCheck.push_back(queuePair.first);
            }
        }

        // Check each tier for enough players to form a match
        for (uint32_t tier : tiersToCheck)
        {
            if (matchQueue.hasEnoughPlayersForTier(tier))
            {
                std::vector<Player*> matchedPlayers = matchQueue.getPlayersForMatch(tier);

                if (matchedPlayers.size() >= 6)
                {
                    std::cout << "\nMatched " << matchedPlayers.size() << " players for tier " << tier << std::endl;

                    // Randomly shuffle players to ensure fair team distribution
                    std::random_device rd;
                    std::mt19937 g(rd());
                    std::shuffle(matchedPlayers.begin(), matchedPlayers.end(), g);

                    // Form two teams of 3 players each
                    std::vector<Player*> blackTeam(matchedPlayers.begin(), matchedPlayers.begin() + 3);
                    std::vector<Player*> whiteTeam(matchedPlayers.begin() + 3, matchedPlayers.end());

                    // Create a battle room and start the battle
                    BattleRoom room(blackTeam, whiteTeam);
                    room.startBattle();
                }
            }
        }

        // Sleep for a short period to prevent busy-waiting and reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Matchmaking thread stopped" << std::endl;
}