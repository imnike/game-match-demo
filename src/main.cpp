#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <string>
#include "..\include\battleManager.h"
#include "..\include\playerManager.h"
#include "..\include\dbManager.h"

bool isRunning = true;

void commandThread();
void listPlayers();
void showTopPlayers();
void simulatePlayersOnce();
void exitGame();

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "Game Match Demo Starting..." << std::endl;

    if (!DbManager::instance().initialize())
    {
        std::cerr << "Failed to initialize DbManager." << std::endl;
        return 1;
    }

    if (!PlayerManager::instance().initialize())
    {
        std::cerr << "Failed to initialize PlayerManager." << std::endl;
        return 1;
    }

    if (!BattleManager::instance().initialize())
    {
        std::cerr << "Failed to initialize BattleManager." << std::endl;
        return 1;
    }
	DbManager::instance().ensureTableSchema();
	DbManager::instance().loadTableData();

	std::thread cmdThread(commandThread);   // make a thread for command input

    cmdThread.join();

    std::cout << "Game Match Demo Ended." << std::endl;

    return 0;
}

void simulatePlayersOnce()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> playerIdDis(1, 50); // 隨機生成玩家ID範圍

    std::cout << "--- Starting player simulation batch ---" << std::endl;

    std::vector<uint64_t> vecLoggedInIds;

    for (int i = 0; i < 6; ++i) // 每次模擬6位玩家登入
    {
        uint64_t playerId;
        bool foundUniqueId = false;
        // 嘗試找到一個未登入且本次批次未選擇的玩家
        for (int attempt = 0; attempt < 100; ++attempt)
        { // 設定嘗試次數限制，避免無限循環
            playerId = playerIdDis(gen);
            if (!PlayerManager::instance().isPlayerOnline(playerId) &&
                std::find(vecLoggedInIds.begin(), vecLoggedInIds.end(), playerId) == vecLoggedInIds.end())
            {
                foundUniqueId = true;
                break;
            }
        }

        if (!foundUniqueId)
        {
            std::cerr << "Warning: Could not find a unique, offline player ID after multiple attempts. Skipping player." << std::endl;
            continue;
        }

        // 登入玩家 (playerLogin 應改名為 loginPlayer，與之前討論一致)
        Player* pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer != nullptr)
        {
            vecLoggedInIds.push_back(playerId);
            std::cout << "Player " << playerId << " logged in." << std::endl;
            // 將玩家加入匹配隊列
            BattleManager::instance().addPlayerToQueue(pPlayer);
        }
        else {
            std::cerr << "Error: Failed to login player " << playerId << std::endl;
        }
    }

    std::cout << "Waiting 5 seconds for players to potentially match..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待一段時間，讓玩家有機會匹配和戰鬥

    // 使這些玩家登出
    for (uint64_t id : vecLoggedInIds)
    {
        // playerLogout 應改名為 logoutPlayer
        if (PlayerManager::instance().playerLogout(id)) 
        {
            std::cout << "Player " << id << " logged out." << std::endl;
        }
        else {
            std::cerr << "Error: Failed to logout player " << id << std::endl;
        }
    }

    std::cout << "--- Player simulation batch finished ---" << std::endl;
}


void commandThread()
{
    std::cout << "Command thread started. Available commands: <simulate>, <list>, <top>, <exit>" << std::endl;

    std::string command = "";
    while (isRunning == true)
    {
        std::cout << "> "; // 提示用戶輸入
        std::getline(std::cin, command);

        if (command == "list")
        {
            listPlayers();
        }
        else if (command == "top")
        {
            showTopPlayers();
        }
        else if (command == "simulate") // 新增的模擬命令
        {
            simulatePlayersOnce();
        }
        else if (command == "exit")
        {
			exitGame();
        }
        else if (!command.empty()) // 避免處理空行
        {
            std::cout << "Unknown command. Available commands: <list>, <top>, <simulate>, <exit>" << std::endl;
        }
    }

    std::cout << "Command thread ended" << std::endl;
}

// listPlayers() 和 showTopPlayers() 保持不變，因為它們的邏輯依然有效。
void listPlayers()
{
    std::vector<Player*> vecPlayers; // 注意：這裡應該是 Player*
    PlayerManager::instance().getOnlinePlayers(vecPlayers);
    if (vecPlayers.empty())
    {
        std::cout << "No players currently logged in." << std::endl;
        return;
    }

    std::cout << "\n----- LOGGED IN PLAYERS -----" << std::endl;
    std::cout << std::left << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Level"
        << std::setw(10) << "Wins" << std::endl;
    std::cout << "-------------------------------" << std::endl;

    for (const auto& player : vecPlayers)
    {
        if (player) 
        { // 增加空指針檢查
            std::cout << std::left << std::setw(10) << player->getId()
                << std::setw(10) << player->getScore()
                << std::setw(10) << player->getTier()
                << std::setw(10) << player->getWins() << std::endl;
        }
    }

    std::cout << "-------------------------------" << std::endl;
}

void showTopPlayers()
{
    // 假設 DbManager::instance().getTopPlayers() 返回 PlayerRawData 的 vector
    auto rankings = DbManager::instance().getTopPlayers();

    if (rankings.empty())
    {
        std::cout << "No players in database." << std::endl;
        return;
    }

    std::cout << "\n----- TOP PLAYERS -----" << std::endl;
    std::cout << std::left << std::setw(5) << "Rank"
        << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Level"
        << std::setw(10) << "Wins" << std::endl;
    std::cout << "------------------------------" << std::endl;

    int rank = 1;
    for (const auto& player : rankings)
    {
        uint32_t level = player.score / 200; // 計算玩家等級

        std::cout << std::left << std::setw(5) << rank++
            << std::setw(10) << player.id
            << std::setw(10) << player.score
            << std::setw(10) << level
            << std::setw(10) << player.wins << std::endl;

        // 只顯示前20名玩家
        if (rank > 20)
        {
            break;
        }
    }

    std::cout << "------------------------------" << std::endl;
}

void exitGame()
{
    isRunning = false;
    std::cout << "Exiting game..." << std::endl;

    BattleManager::instance().release();
    PlayerManager::instance().release();
    DbManager::instance().release();

    // 這裡可以添加任何需要的清理代碼
}