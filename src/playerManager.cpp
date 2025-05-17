// @file  : playerManager.cpp
// @brief : �޲z���a���
// @author: August
// @date  : 2025-05-15
#include "playerManager.h"
#include "dbManager.h"
#include "battleManager.h"
#include "../utils/utils.h"
#include "../include/globalDefine.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm>

PlayerManager& PlayerManager::instance()
{
	// singleton instance
    static PlayerManager instance;
    return instance;
}

PlayerManager::PlayerManager()
{
}

PlayerManager::~PlayerManager()
{
}

bool PlayerManager::initialize()
{
    m_mapPlayers.clear();
    m_setOnlinePlayerIds.clear();
    return true;
}

void PlayerManager::release()
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    m_mapPlayers.clear();
    m_setOnlinePlayerIds.clear();
}

Player* PlayerManager::playerLogin(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    if (id == 0)
    {
		// insert new player
        id = DbManager::instance().insertPlayerBattles();
        if (id == 0)
        {
            std::cerr << "Failed to create player in database for ID: " << id << std::endl;
            return nullptr;
        }
        std::cout << "New player created with ID: " << id << std::endl;
    }

    Player* pPlayer = _getPlayerNoLock(id);
    if (!pPlayer)
    {
        std::cout << "Player " << id << " not found." << std::endl;
        return nullptr;
    }
    //std::cout << "Player " << id << " login." << std::endl;
    _setPlayerOnlineNoLock(id, true);
    pPlayer->setStatus(common::PlayerStatus::lobby);
    return pPlayer;
}

bool PlayerManager::playerLogout(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    Player* pPlayer = _getPlayerNoLock(id);
    if (!pPlayer)
    {
        std::cout << "Player " << id << " is not logged in." << std::endl;
        return false;
    }

    //std::cout << "Player " << id << " logout." << std::endl;
    _setPlayerOnlineNoLock(id, false);
    pPlayer->setStatus(common::PlayerStatus::offline);
    return true;
}

bool PlayerManager::isPlayerOnline(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    return (m_setOnlinePlayerIds.find(id) != m_setOnlinePlayerIds.end());
}

std::vector<Player*> PlayerManager::getOnlinePlayers() 
{
    std::lock_guard<std::mutex> lock(m_mapPlayersMutex);

    std::vector<Player*> tmpVecPlayers;
    for (auto& id : m_setOnlinePlayerIds) 
    {
        Player* pPlayer = _getPlayerNoLock(id);
        if (!pPlayer)
        {
            continue;
        }
        tmpVecPlayers.push_back(pPlayer);
    }
    return tmpVecPlayers;
}

// *** only for dbManager to sync player data from db ***
void PlayerManager::syncPlayerFromDbNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime)
{
	_syncPlayerNoLock(id, score, wins, updatedTime);
}

Player* PlayerManager::_getPlayerNoLock(uint64_t id)
{
    if (m_mapPlayers.empty())
    {
        return nullptr;
    }
    auto it = m_mapPlayers.find(id);
    if (it != m_mapPlayers.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void PlayerManager::_setPlayerOnlineNoLock(uint64_t id, bool isOnline)
{
    if (isOnline)
    {
        m_setOnlinePlayerIds.emplace(id);
    }
    else
    {
        m_setOnlinePlayerIds.erase(id);
    }
}

void PlayerManager::_syncPlayerNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime)
{
    if (m_mapPlayers.find(id) != m_mapPlayers.end())
    {
        std::cerr << "Player " << id << " already exists." << std::endl;
        return;
    }
    std::unique_ptr<Player> uPlayer = std::make_unique<Player>(id, score, wins, updatedTime);
    m_mapPlayers[id] = std::move(uPlayer);
}

// --- �Ʀ�]������k��@ ---

// �ҰʱƦ�]��s�����
void PlayerManager::startLeaderboardUpdateThread(std::chrono::seconds intervalSeconds)
{
    if (m_running.load()) 
    {
        // �ϥ� load() �ˬd��l�ܼ�
        std::cout << "�Ʀ�]��s������w�b�B�椤�C\n";
        return;
    }
    m_running.store(true); // �ϥ� store() �]�w��l�ܼ�
    // �ЫبñҰʰ�����A�ǻ�������ƫ��w�M��H���w
    m_leaderboardThread = std::thread(&PlayerManager::leaderboardUpdateLoop, this, intervalSeconds);
    std::cout << "�Ʀ�]��s������w�ҰʡA��s���j�� " << intervalSeconds.count() << " ��C\n";
}

// ����Ʀ�]��s�����
void PlayerManager::stopLeaderboardUpdateThread()
{
    if (m_running.load()) 
    {
        m_running.store(false); // �q�����������B��
        if (m_leaderboardThread.joinable())
        {
            m_leaderboardThread.join(); // ���ݰ�����w���a����
        }
        std::cout << "�Ʀ�]��s������w����C\n";
    }
}

// �Ʀ�]��s��������j��禡
void PlayerManager::leaderboardUpdateLoop(std::chrono::seconds interval)
{
    while (m_running.load()) 
    { // �u�n m_running �� true �N�@���B��
        // ����Ʀ�]���p��M�w�s��s
        calculateAndCacheLeaderboard();
        // �ίv���w���ɶ����j
        std::this_thread::sleep_for(interval);
    }
}

// ��ڭp��ýw�s�Ʀ�]�ƾ�
void PlayerManager::calculateAndCacheLeaderboard()
{
    std::vector<PlayerRankInfo> currentRanks;

    // ��w mapPlayersMutex �H�w���aŪ���Ҧ����a�ƾ�
    // �o�T�O�F�bŪ�� mapPlayers �ɡA�S����L������b�ק復
    {
        std::lock_guard<std::mutex> lock(m_mapPlayersMutex);
        currentRanks.reserve(m_mapPlayers.size()); // �w�d�Ŷ��H�����į�
        for (const auto& pair : m_mapPlayers) 
        {
            Player* pPlayer = pair.second.get();
            if (pPlayer) 
            {
                // �N Player ���󪺼ƾڽƻs�� PlayerRankInfo
                currentRanks.push_back({ pPlayer->getId(), pPlayer->getScore(), pPlayer->getWins() });
            }
        }
    } // mapPlayersMutex �b�o�̦۰ʸ���

    // ��ƻs�X�Ӫ��ƾڶi��Ƨ�
    // �o�̤��ݭn��w�A�]�� currentRanks �O�o�Ө禡�������ܼ�
    std::sort(currentRanks.begin(), currentRanks.end(),
        [](const PlayerRankInfo& a, const PlayerRankInfo& b) 
        {
            if (a.m_score != b.m_score) 
            {
                return a.m_score > b.m_score; // ���ư����u�� (����)
            }
            if (a.m_wins != b.m_wins) 
            {
                return a.m_wins > b.m_wins;   // �ӳ��h���u�� (����)
            }
            return a.m_id < b.m_id;           // ID �p���u�� (�ɧǡA�@��í�w�Ƨ�)
        });

    // ��w�Ʀ�]�w�s mutex�A��s�w�s�ƾ�
    // �o�T�O�F�b��s�w�s�ɡA�S����L������bŪ����
    {
        std::lock_guard<std::mutex> lock(m_cachedLeaderboardMutex);
        m_vecCachedLeaderboard = std::move(currentRanks); // �ϥ� std::move ���Ħa�ಾ�ƾ�
    } // m_cachedLeaderboardMutex �b�o�̦۰ʸ���
    // std::cout << "�Ʀ�]�w��s�C�`�p " << m_cachedLeaderboard.size() << " �쪱�a�C\n"; // �i�H���}�Ω�ո�
}

// ���o�w�s���Ʀ�]��� (�Ȥ�ݩI�s�o�Ө�����Ʀ�])
std::vector<PlayerRankInfo> PlayerManager::getLeaderboard(size_t topN)
{
    std::lock_guard<std::mutex> lock(m_cachedLeaderboardMutex); // ��w�HŪ���w�s

    std::vector<PlayerRankInfo> result;
    result.reserve(std::min(topN, m_vecCachedLeaderboard.size())); // �w�d�Ŷ��H�����į�

    size_t count = 0;
    for (const auto& rank : m_vecCachedLeaderboard)
    {
        if (count >= topN) 
        {
            break; // �F����w�ƶq�N����
        }
        result.push_back(rank); // �ƻs�ƾڨ쵲�G�V�q
        count++;
    }
    return result;
}

void PlayerManager::handlePlayerBattleResult(uint64_t playerId, uint32_t scoreDelta, bool isWin)
{
    {
        std::lock_guard<std::mutex> lock(m_mapPlayersMutex); // �O�@�� m_mapPlayers ���X��

        Player* pPlayer = _getPlayerNoLock(playerId);
        if (!pPlayer)
        {
            return;
        }
        if (isWin)
        {
            pPlayer->addWins();
            pPlayer->addScore(scoreDelta);
        }
        else
        {
            pPlayer->subScore(scoreDelta);
        }
        pPlayer->setStatus(common::PlayerStatus::lobby);
    }
    enqueuePlayerSave(playerId);
}

void PlayerManager::enqueuePlayerSave(uint64_t playerId)
{
	std::lock_guard<std::mutex> lock(m_setdirtyPlayerIdsMutex);
	m_setDirtyPlayerIds.emplace(playerId);
}

void PlayerManager::saveDirtyPlayers()
{
    if (m_setDirtyPlayerIds.empty())
    {
        return;
    }
    std::set<uint64_t> setSaveIds;
    {
        // ����w m_dirtyPlayerIdsMutex�A�è��t�洫�ƾ�
        std::lock_guard<std::mutex> lock(m_setdirtyPlayerIdsMutex);
        setSaveIds.swap(m_setDirtyPlayerIds); // �N m_dirtyPlayerIds �����e���ʨ� setSaveIds
        // �o�ˡAm_dirtyPlayerIds �N�i�H�ߧY�����s�� ID �F
    }
    for (auto& id : setSaveIds)
    {
		Player* pPlayer = _getPlayerNoLock(id);
        if (!pPlayer)
        {
            continue;
        }
        DbManager::instance().updatePlayerBattles(pPlayer->getId(), pPlayer->getScore(), pPlayer->getWins());
    }
}