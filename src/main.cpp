// main.cpp
#include <iostream>
#include <sstream> // �Ω�r��ѪR
#include <limits>  // �Ω� std::numeric_limits
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <atomic>

#include "battleManager.h"
#include "playerManager.h"
#include "scheduleManager.h"
#include "dbManager.h"
#include "../utils/utils.h"

std::atomic<bool> isRunning = true; // ����R�O�B�z��������B�檬�A

void commandThread();
void listAllPlayers();
void simulatePlayers(uint32_t counts);
void exitGame();

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "--- Game Match Demo Starting (Multithreaded Server) ---\n";

    // 1. ��l�ƩҦ��֤ߺ޲z��
    if (!DbManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize DbManager!\n";
        return 1;
    }

    if (!PlayerManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize PlayerManager!\n";
        return 1;
    }

    if (!BattleManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize BattleManager!\n";
        return 1;
    }

    if (!ScheduleManager::instance().initialize())
    {
        std::cerr << "Error: Failed to initialize ScheduleManager!\n";
        return 1;
    }

    // �T�O��Ʈw���c�M�[�����
    DbManager::instance().ensureTableSchema();
    DbManager::instance().loadTableData();

	BattleManager::instance().startMatchmaking(); // �Ұʤǰt�����

    std::cout << "Game Server initialized. Main thread ready for commands.\n";
    std::cout << "Type 'quit' or 'exit' to shut down the server.\n";

    std::thread cmdThread(commandThread);   // �ҰʥD���x�R�O�B�z�����

    // main �禡�{�b�|���� cmdThread �����C
    // �o�N���ۥu�n cmdThread �S�����Amain �禡�N�|�O�����D�A��ӵ{���]�|�B��C
    // ��A�b commandThread ����J "exit" �ɡAisRunning �|�ܬ� false�AcmdThread �����A
    // ���� main �禡�N�~�����M�z�N�X�C
    cmdThread.join();

    std::cout << "--- Game Match Demo Shutting Down ---\n";

    return 0;
}

// �������a�n�J�äǰt
void simulatePlayers(uint32_t counts)
{
    std::cout << "--- Starting player simulation batch ---\n";

    std::vector<uint64_t> vecLoggedInIds;

    // ����Ҧ����aob
    auto pMapAllPlayers = PlayerManager::instance().getAllPlayers();

    // ��@�H�����o�@�Ӥ��b�u�����aid
    uint64_t maxId = static_cast<uint64_t>(pMapAllPlayers->size());
    for (uint32_t i = 0; i < counts; i++)
    {
        // ���o�@���H��(1~maxID)
        uint64_t playerId = random_utils::getRandom(maxId);
        Player* pPlayer = nullptr;
        pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer)
        {
            if (pPlayer->isInLobby())
            {
                playerId = pPlayer->getId(); // ���] playerLogin �|��^���Ī����a ID
                vecLoggedInIds.push_back(playerId);
                // �N���a�[�J�ǰt���C
                BattleManager::instance().addPlayerToQueue(pPlayer);
                std::cout << " player " << playerId << " join matchQueue.\n";
            }
            else
            {
                std::cout << " player " << playerId << " is not in lobby.\n";
            }
        }
        else
        {
            std::cerr << "Error: Failed to get player " << playerId << "\n";
        }
    }

    //std::cout << "Waiting 3 seconds for players to potentially match...\n";
    // ���ݤ@�q�ɶ��A�����a�����|�ǰt�M�԰�
    //std::this_thread::sleep_for(std::chrono::seconds(3));

    // // �p�G�ݭn�A�i�H�ϳo�Ǫ��a�n�X
    // for (uint64_t id : vecLoggedInIds)
    // {
    //     if (PlayerManager::instance().playerLogout(id))
    //     {
    //         std::cout << "Player " << id << " logged out.\n";
    //     }
    //     else
    //     {
    //         std::cerr << "Error: Failed to logout player " << id << "\n";
    //     }
    // }

    std::cout << "--- Player simulation batch finished ---\n";
}
// �R�O�B�z��������禡
void commandThread()
{
    std::cout << "Command thread started. Available commands: <list [ID|all]>, <query ID>, <start [count]>, <exit>\n";

    std::string line_input; // �Ω�Ū������J
    while (isRunning)
    {
        // ***** ����ץ��G�����ε��P���o�Ǧ� *****
        // std::cin.clear(); // �����o��A�]�� getline ���|�]�m���~���A
        // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // �����o��A�]�� getline �w�g�����F�����
        // ***************************************

        std::cout << "> "; // ���ܥΤ��J
        std::getline(std::cin, line_input); // ���릡Ū���@���J�C���|Ū�����촫��šA�ç⴫��Ų����C

        // �ˬd�O�_Ū�����\�]�קK�b EOF �ο��~���A�U�~��B�z�^
        if (std::cin.fail() || std::cin.eof()) {
            std::cout << "Input stream error or end of file detected. Exiting command thread.\n";
            exitGame(); // �Ϊ̨�L�A�����~�B�z
            break;
        }

        std::istringstream iss(line_input);
        std::string command_name;
        iss >> command_name;

        if (command_name.empty()) {
            continue; // �B�z�Ŧ�
        }

        std::transform(command_name.begin(), command_name.end(), command_name.begin(), ::tolower);

        if (command_name == "list")
        {
            listAllPlayers();
        }
        else if (command_name == "queue")
        {
            auto pTeamTierQueues = BattleManager::instance().getTeamMatchQueue();
            std::cout << "\n--- Team Match Queue ���e ---" << std::endl;
            // ��� TeamMatchQueue ������ map ���w
            if (pTeamTierQueues)
            {
                const auto* pMapQueue = &pTeamTierQueues->mapTierQueues;
                if (pMapQueue->empty())
                {
                    std::cout << "  (Team Match Queue �ثe�S�����a)\n";
                }
                else 
                {
                    for (const auto& pair : *pMapQueue)
                    {
                        uint32_t tier = pair.first;
                        const std::vector<Player*>& playersInTier = pair.second;
                        std::cout << "  Tier " << tier << " (���a��: " << playersInTier.size() << "): ";
                        for (const auto& pPlayer : playersInTier) 
                        {
                            if (pPlayer)
                            {
                                std::cout << pPlayer->getId() << " ";
                            }
                        }
                        std::cout << std::endl;
                    }
                }
            }
            else {
                std::cout << "  �L�k��� Team Match Queue ���e�C\n";
            }

            std::cout << "\n--- Battle Match Queue ���e ---" << std::endl;
            // ��� BattleMatchQueue ������ map ���w
            auto pBattleTierQueues = BattleManager::instance().getBattleMatchQueue();
            if (pBattleTierQueues)
            {
                const auto* pMapQueue = &pBattleTierQueues->mapTierQueues;
                if (pMapQueue->empty())
                {
                    std::cout << "  (Battle Match Queue �ثe�S������)\n";
                }
                else 
                {
                    for (const auto& pair : *pMapQueue) 
                    {
                        uint32_t tier = pair.first;
                        const std::vector<std::vector<Player*>>& teamsInTier = pair.second;
                        std::cout << "  Tier " << tier << " (�����: " << teamsInTier.size() << "): ";
                        for (const auto& team : teamsInTier) 
                        {
                            std::cout << "[";
                            for (const auto& pPlayer : team) 
                            {
                                if (pPlayer) 
                                {
                                    std::cout << pPlayer->getId() << " ";
                                }
                            }
                            std::cout << "] ";
                        }
                        std::cout << std::endl;
                    }
                }
            }
            else 
            {
                std::cout << "  �L�k��� Battle Match Queue ���e�C\n";
            }
        }
        else if (command_name == "query")
        {
            std::string arg;
            if (!(iss >> arg))
            {
                std::cout << "Usage: query <player_id>\n";
                continue;
            }

            try {
                uint64_t playerId = std::stoull(arg);
                uint32_t score = 0;
                uint32_t wins = 0;
                uint64_t updatedTime = 0;

                if (DbManager::instance().queryPlayerBattles(playerId, score, wins, updatedTime))
                {
                    std::cout << "\n----- Player Data (ID: " << playerId << ") -----\n";
                    std::cout << std::left << std::setw(10) << "Score"
                        << std::setw(10) << "Wins"
                        << std::setw(25) << "Updated Time" << "\n";
                    std::cout << "---------------------------------------------------\n";
                    std::cout << std::left << std::setw(10) << score
                        << std::setw(10) << wins
                        << std::setw(25) << time_utils::formatTimestampMs(updatedTime) << "\n";
                    std::cout << "---------------------------------------------------\n";
                }
                else
                {
                    std::cout << "Player ID " << arg << " not found or no data.\n";
                }
            }
            catch (const std::invalid_argument&) {
                std::cout << "Invalid player ID format: '" << arg << "'. Please enter a valid number.\n";
            }
            catch (const std::out_of_range&) {
                std::cout << "Player ID '" << arg << "' is out of range.\n";
            }
        }
        else if (command_name == "start")
        {
            int count = 1;
            std::string arg;
            if (iss >> arg)
            {
                try {
                    count = std::stoi(arg);
                    if (count <= 0) {
                        std::cout << "Count must be a positive number.\n";
                        continue;
                    }
                }
                catch (const std::invalid_argument&) {
                    std::cout << "Invalid count format: '" << arg << "'. Using default (1).\n";
                    count = 1;
                }
                catch (const std::out_of_range&) {
                    std::cout << "Count '" << arg << "' is out of range. Using default (1).\n";
                    count = 1;
                }
            }
            simulatePlayers(count);
        }
        else if (command_name == "exit" || command_name == "quit")
        {
            exitGame();
        }
        else
        {
            std::cout << "Unknown command '" << line_input << "'. Available commands: <list [ID|all]>, <query ID>, <start [count]>, <exit>\n";
        }
    }

    std::cout << "Command thread ended.\n";
}

static const std::string getStatusToString(common::PlayerStatus status)
 {
    switch (status) 
    {
    case common::PlayerStatus::offline:
        return "";
        break;
    case common::PlayerStatus::lobby:
        return "Lobby";
        break;
    case common::PlayerStatus::queue:
        return "Matching";
        break;
    case common::PlayerStatus::battle:
        return "Fighting";
        break;
    default:
        break;
    }
    return "unknown";
}

// �C�X�Ҧ����a��T
void listAllPlayers()
{
    auto pMapPlayers = PlayerManager::instance().getAllPlayers();
    if (pMapPlayers->empty())
    {
        std::cout << "No players currently.\n";
        return;
    }

    std::cout << "\n----- PLAYERS LIST -----\n";
    std::cout << std::left << std::setw(10) << "ID"
        << std::setw(10) << "Score"
        << std::setw(10) << "Tier"
        << std::setw(10) << "Wins"
        << std::setw(15) << "Status" 
        << std::setw(25) << "Updated Time" << "\n"; 
    std::cout << "------------------------------------------------------------------------------\n";

    for (const auto& itPlayer : *pMapPlayers)
    {
        Player* pPlayer = itPlayer.second.get();
        if (pPlayer)
        {
            std::cout << std::left << std::setw(10) << pPlayer->getId()
                << std::setw(10) << pPlayer->getScore()
                << std::setw(10) << pPlayer->getTier()
                << std::setw(10) << pPlayer->getWins()
                << std::setw(15) << getStatusToString(pPlayer->getStatus())
                << std::setw(25) << time_utils::formatTimestampMs(pPlayer->getUpdatedTime()) << "\n";
        }
    }

    std::cout << "------------------------------------------------------------------------------\n";
}

// �h�X�C���A����M�z�ާ@
void exitGame()
{
    isRunning = false; // �]�w�X�СA���� commandThread �����j��
    std::cout << "Exiting game...\n";

    // �T�O�Ҧ��޲z���� release() �禡�Q�I�s�A�ë��ӥ��T����������귽�C
    // �q�`�̿��L�ե󪺥�����A�Ϊ̽T�O������������ե���w������C
    // ScheduleManager.release() �|���ݨ䤺������������C
    ScheduleManager::instance().release(); // �̭��n���A�T�O��x�Ƶ{������w������
    BattleManager::instance().release();   // ����԰��޲z���귽
    PlayerManager::instance().release();   // ���񪱮a�޲z���귽
    DbManager::instance().release();       // �����Ʈw�޲z���귽

    // �o�̥i�H�K�[�����L�ݭn���M�z�N�X
}