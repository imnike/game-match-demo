#include "..\include\playerManager.h"
#include "..\include\dbManager.h"
#include "..\utils\timeUtils.h"
#include <iostream>
#include <chrono>
#include <vector>

Player::Player(uint64_t id, uint32_t score, uint32_t wins, time_t updateTime)
    : m_id(id), m_score(score), m_wins(wins), m_updatedTime(updateTime)
{
}

Player::~Player()
{
}

uint32_t Player::getTier() const
{
	return (m_score / 200);   // hidden tier
}

void Player::addScore(uint32_t scoreChange)
{
    m_score += scoreChange;
    updateStats();
}

void Player::addWin()
{
    m_wins++;
    updateStats();
}

void Player::updateStats()
{
	m_updatedTime = DemoTimeUtils::getTimestampMS();
}

PlayerManager& PlayerManager::instance()
{
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
    mapPlayers.clear();
	setOnlinePlayerIds.clear();
    return true;
}

void PlayerManager::release()
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& pair : mapPlayers)
    {
        delete pair.second;
        pair.second = nullptr;
    }

    mapPlayers.clear();
    setOnlinePlayerIds.clear();
}

void PlayerManager::syncPlayer(uint64_t id, uint32_t score, uint32_t wins, time_t updatedTime)
{
    if (mapPlayers.find(id) != mapPlayers.end())
    {
		std::cerr << "Player " << id << " already exists." << std::endl;
        return;
    }
    Player* player = new Player(id, score, wins, updatedTime);
	mapPlayers[id] = player;
}

Player* PlayerManager::playerLogin(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (id == 0)
    {
        // 新增玩家資料到db
        id = DbManager::instance().insertPlayerBattles();
        if (id == 0)
        {
            std::cerr << "Failed to create player in database for ID: " << id << std::endl;
            return nullptr;
        }
        std::cout << "New player created with ID: " << id << std::endl;
    }
    Player* pPlayer = getPlayer(id);    // 新創成功後, 取得玩家資料
    if (pPlayer == nullptr)
    {
        std::cout << "Player " << id << " not found." << std::endl;
        return nullptr;
    }
	std::cout << "Player " << id << " login." << std::endl;
	setPlayerOnline(id, true);
    return pPlayer;
}

bool PlayerManager::playerLogout(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex);

	Player* pPlayer = getPlayer(id);
    if (pPlayer == nullptr)
    {
        std::cout << "Player " << id << " is not logged in." << std::endl;
        return false;
    }

    std::cout << "Player " << id << " logout." << std::endl;
	setPlayerOnline(id, false);
    return true;
}

Player* PlayerManager::getPlayer(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex);
    
    if (mapPlayers.empty())
    {
        return nullptr;
    }
    auto it = mapPlayers.find(id);
    if (it != mapPlayers.end())
    {
        return it->second;
    }

    return nullptr;
}

void PlayerManager::setPlayerOnline(uint64_t id, bool isOnline)
{
    std::lock_guard<std::mutex> lock(mutex);
	if (isOnline == true)
    {
        setOnlinePlayerIds.emplace(id);
    }
    else
    {
        setOnlinePlayerIds.erase(id);
    }
}

bool PlayerManager::isPlayerOnline(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mutex);
	return (setOnlinePlayerIds.find(id) != setOnlinePlayerIds.end());
}

void PlayerManager::getOnlinePlayers(std::vector<Player*>& refVecPlayers)
{
    std::lock_guard<std::mutex> lock(mutex);
    refVecPlayers.clear();
    for (auto& it : setOnlinePlayerIds)
    {
		Player* pPlayer = getPlayer(it);
        if (pPlayer != nullptr)
        {
            refVecPlayers.push_back(pPlayer);
		}
    }
}