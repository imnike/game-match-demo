// @file  : playerManager.cpp
// @brief : 管理玩家資料
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

// --- 排行榜相關方法實作 ---

// 啟動排行榜更新執行緒
void PlayerManager::startLeaderboardUpdateThread(std::chrono::seconds intervalSeconds)
{
    if (m_running.load()) 
    {
        // 使用 load() 檢查原子變數
        std::cout << "排行榜更新執行緒已在運行中。\n";
        return;
    }
    m_running.store(true); // 使用 store() 設定原子變數
    // 創建並啟動執行緒，傳遞成員函數指針和對象指針
    m_leaderboardThread = std::thread(&PlayerManager::leaderboardUpdateLoop, this, intervalSeconds);
    std::cout << "排行榜更新執行緒已啟動，更新間隔為 " << intervalSeconds.count() << " 秒。\n";
}

// 停止排行榜更新執行緒
void PlayerManager::stopLeaderboardUpdateThread()
{
    if (m_running.load()) 
    {
        m_running.store(false); // 通知執行緒停止運行
        if (m_leaderboardThread.joinable())
        {
            m_leaderboardThread.join(); // 等待執行緒安全地結束
        }
        std::cout << "排行榜更新執行緒已停止。\n";
    }
}

// 排行榜更新執行緒的迴圈函式
void PlayerManager::leaderboardUpdateLoop(std::chrono::seconds interval)
{
    while (m_running.load()) 
    { // 只要 m_running 為 true 就一直運行
        // 執行排行榜的計算和緩存更新
        calculateAndCacheLeaderboard();
        // 睡眠指定的時間間隔
        std::this_thread::sleep_for(interval);
    }
}

// 實際計算並緩存排行榜數據
void PlayerManager::calculateAndCacheLeaderboard()
{
    std::vector<PlayerRankInfo> currentRanks;

    // 鎖定 mapPlayersMutex 以安全地讀取所有玩家數據
    // 這確保了在讀取 mapPlayers 時，沒有其他執行緒在修改它
    {
        std::lock_guard<std::mutex> lock(m_mapPlayersMutex);
        currentRanks.reserve(m_mapPlayers.size()); // 預留空間以提高效能
        for (const auto& pair : m_mapPlayers) 
        {
            Player* pPlayer = pair.second.get();
            if (pPlayer) 
            {
                // 將 Player 物件的數據複製到 PlayerRankInfo
                currentRanks.push_back({ pPlayer->getId(), pPlayer->getScore(), pPlayer->getWins() });
            }
        }
    } // mapPlayersMutex 在這裡自動解鎖

    // 對複製出來的數據進行排序
    // 這裡不需要鎖定，因為 currentRanks 是這個函式的局部變數
    std::sort(currentRanks.begin(), currentRanks.end(),
        [](const PlayerRankInfo& a, const PlayerRankInfo& b) 
        {
            if (a.m_score != b.m_score) 
            {
                return a.m_score > b.m_score; // 分數高者優先 (降序)
            }
            if (a.m_wins != b.m_wins) 
            {
                return a.m_wins > b.m_wins;   // 勝場多者優先 (降序)
            }
            return a.m_id < b.m_id;           // ID 小的優先 (升序，作為穩定排序)
        });

    // 鎖定排行榜緩存 mutex，更新緩存數據
    // 這確保了在更新緩存時，沒有其他執行緒在讀取它
    {
        std::lock_guard<std::mutex> lock(m_cachedLeaderboardMutex);
        m_vecCachedLeaderboard = std::move(currentRanks); // 使用 std::move 高效地轉移數據
    } // m_cachedLeaderboardMutex 在這裡自動解鎖
    // std::cout << "排行榜已更新。總計 " << m_cachedLeaderboard.size() << " 位玩家。\n"; // 可以打開用於調試
}

// 取得緩存的排行榜資料 (客戶端呼叫這個來獲取排行榜)
std::vector<PlayerRankInfo> PlayerManager::getLeaderboard(size_t topN)
{
    std::lock_guard<std::mutex> lock(m_cachedLeaderboardMutex); // 鎖定以讀取緩存

    std::vector<PlayerRankInfo> result;
    result.reserve(std::min(topN, m_vecCachedLeaderboard.size())); // 預留空間以提高效能

    size_t count = 0;
    for (const auto& rank : m_vecCachedLeaderboard)
    {
        if (count >= topN) 
        {
            break; // 達到指定數量就停止
        }
        result.push_back(rank); // 複製數據到結果向量
        count++;
    }
    return result;
}

void PlayerManager::handlePlayerBattleResult(uint64_t playerId, uint32_t scoreDelta, bool isWin)
{
    {
        std::lock_guard<std::mutex> lock(m_mapPlayersMutex); // 保護對 m_mapPlayers 的訪問

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
        // 僅鎖定 m_dirtyPlayerIdsMutex，並迅速交換數據
        std::lock_guard<std::mutex> lock(m_setdirtyPlayerIdsMutex);
        setSaveIds.swap(m_setDirtyPlayerIds); // 將 m_dirtyPlayerIds 的內容移動到 setSaveIds
        // 這樣，m_dirtyPlayerIds 就可以立即接受新的 ID 了
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