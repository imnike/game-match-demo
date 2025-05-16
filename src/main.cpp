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
    std::uniform_int_distribution<> playerIdDis(1, 50); // �H���ͦ����aID�d��

    std::cout << "--- Starting player simulation batch ---" << std::endl;

    std::vector<uint64_t> vecLoggedInIds;

    for (int i = 0; i < 6; ++i) // �C������6�쪱�a�n�J
    {
        uint64_t playerId;
        bool foundUniqueId = false;
        // ���է��@�ӥ��n�J�B�����妸����ܪ����a
        for (int attempt = 0; attempt < 100; ++attempt)
        { // �]�w���զ��ƭ���A�קK�L���`��
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

        // �n�J���a (playerLogin ����W�� loginPlayer�A�P���e�Q�פ@�P)
        Player* pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer != nullptr)
        {
            vecLoggedInIds.push_back(playerId);
            std::cout << "Player " << playerId << " logged in." << std::endl;
            // �N���a�[�J�ǰt���C
            BattleManager::instance().addPlayerToQueue(pPlayer);
        }
        else {
            std::cerr << "Error: Failed to login player " << playerId << std::endl;
        }
    }

    std::cout << "Waiting 5 seconds for players to potentially match..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // ���ݤ@�q�ɶ��A�����a�����|�ǰt�M�԰�

    // �ϳo�Ǫ��a�n�X
    for (uint64_t id : vecLoggedInIds)
    {
        // playerLogout ����W�� logoutPlayer
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
        std::cout << "> "; // ���ܥΤ��J
        std::getline(std::cin, command);

        if (command == "list")
        {
            listPlayers();
        }
        else if (command == "top")
        {
            showTopPlayers();
        }
        else if (command == "simulate") // �s�W�������R�O
        {
            simulatePlayersOnce();
        }
        else if (command == "exit")
        {
			exitGame();
        }
        else if (!command.empty()) // �קK�B�z�Ŧ�
        {
            std::cout << "Unknown command. Available commands: <list>, <top>, <simulate>, <exit>" << std::endl;
        }
    }

    std::cout << "Command thread ended" << std::endl;
}

// listPlayers() �M showTopPlayers() �O�����ܡA�]�����̪��޿�̵M���ġC
void listPlayers()
{
    std::vector<Player*> vecPlayers; // �`�N�G�o�����ӬO Player*
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
        { // �W�[�ū��w�ˬd
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
    // ���] DbManager::instance().getTopPlayers() ��^ PlayerRawData �� vector
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
        uint32_t level = player.score / 200; // �p�⪱�a����

        std::cout << std::left << std::setw(5) << rank++
            << std::setw(10) << player.id
            << std::setw(10) << player.score
            << std::setw(10) << level
            << std::setw(10) << player.wins << std::endl;

        // �u��ܫe20�W���a
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

    // �o�̥i�H�K�[����ݭn���M�z�N�X
}