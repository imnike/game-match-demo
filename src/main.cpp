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

    std::cout << "Game Server initialized. Main thread ready for commands.\n";
    std::cout << "Type 'quit' or 'exit' to shut down the server.\n";

    std::thread cmdThread(commandThread);   // �ҰʥD���x�R�O�B�z�����

    // main �禡�{�b�|���� cmdThread �����C
    // �o�N���ۥu�n cmdThread �S�����Amain �禡�N�|�O�����D�A��ӵ{���]�|�B��C
    // ��A�b commandThread ����J "exit" �ɡAisRunning �|�ܬ� false�AcmdThread �����A
    // ���� main �禡�N�~�����M�z�N�X�C
    cmdThread.join();

    std::cout << "--- Game Match Demo Shutting Down ---\n";

    // *** �`�N!�Ҧ��޲z���� release() �I�s���b exitGame() �禡���Τ@�B�z ***

    return 0;
}

// �������a�n�J�äǰt
void simulatePlayers(uint32_t counts)
{
    std::cout << "--- Starting player simulation batch ---\n";

    std::vector<uint64_t> vecLoggedInIds;

    // ����Ҧ����aob
    auto pMapAllPlayers = PlayerManager::instance().getAllPlayers();
    // ����b�u���aid
    std::set<uint64_t>* pSetOnlinePlayerIds = PlayerManager::instance().getOnlinePlayerIds();

    // ��@�H�����o�@�Ӥ��b�u�����aid
    uint64_t maxId = static_cast<uint64_t>(pMapAllPlayers->size());
    for (uint32_t i = 0; i < counts; i++)
    {
        // ���o�@���H��(1~maxID)
        uint64_t playerId = random_utils::getRandom(maxId);
        auto itRes = pSetOnlinePlayerIds->find(playerId);
        if (itRes != pSetOnlinePlayerIds->end())
        {
            playerId = 0;
        }

        Player* pPlayer = PlayerManager::instance().playerLogin(playerId);
        if (pPlayer != nullptr)
        {
            playerId = pPlayer->getId(); // ���] playerLogin �|��^���Ī����a ID
            vecLoggedInIds.push_back(playerId);
            // �N���a�[�J�ǰt���C
            BattleManager::instance().addPlayerToQueue(pPlayer);
        }
        else
        {
            std::cerr << "Error: Failed to login player " << playerId << "\n";
        }
    }

    std::cout << "Waiting 5 seconds for players to potentially match...\n";
    // ���ݤ@�q�ɶ��A�����a�����|�ǰt�M�԰�
    std::this_thread::sleep_for(std::chrono::seconds(5));

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
//
//void commandThread()
//{
//    std::cout << "Command thread started. Available commands: <list>, <start>, <exit>\n";
//
//    std::string command; // ����Ū���R�O�r��
//    while (isRunning)
//    {
//        // �T�O�b�C�����ݨϥΪ̿�J�e�A�M���i�઺�w�İϴݯd
//        // �o�O�B�z `std::cin` �ǲ��欰������B�J�A�Y�Ϧb²��Ҧ��U�]��ĳ�O�d�C
//        //std::cin.clear();
//        //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//
//        std::cout << "> "; // ���ܥΤ��J
//        std::getline(std::cin, command); // ���릡Ū���@���J
//
//        // �N�R�O�W���ഫ���p�g�A�H�K���Ϥ��j�p�g
//        //std::transform(command.begin(), command.end(), command.begin(), ::tolower);
//
//        if (command == "list")
//        {
//            listAllPlayers();
//        }
//        else if (command == "start") // �������a���ʪ��R�O
//        {
//            simulatePlayers(10); // �`�O�����@����Ӫ��a
//        }
//        else if (command == "exit" || command == "quit") // �h�X�R�O
//        {
//            exitGame(); // �I�s exitGame �|�]�w isRunning = false
//        }
//        else if (command.empty()) // �B�z�Ŧ� (�u�� Enter)
//        {
//            // ���򳣤����A�u�O���ݤU�@����J
//        }
//        else // �����R�O
//        {
//            std::cout << "Unknown command '" << command << "'. Available commands: <list>, <start>, <exit>\n";
//        }
//    }
//
//    std::cout << "Command thread ended.\n";
//}
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
        << std::setw(10) << "Level" // ���] Player::getTier() ���� Level
        << std::setw(10) << "Wins"
        << std::setw(25) << "Updated Time" << "\n"; // �s�W Updated Time ���
    std::cout << "--------------------------------------------------------\n"; // �վ���j�u����

    for (const auto& itPlayer : *pMapPlayers)
    {
        Player* pPlayer = itPlayer.second.get();
        if (pPlayer == nullptr)
        {
            continue;
        }
        std::cout << std::left << std::setw(10) << pPlayer->getId()
            << std::setw(10) << pPlayer->getScore()
            << std::setw(10) << pPlayer->getTier()
            << std::setw(10) << pPlayer->getWins();

        // �I�s Player �� getUpdatedTimestamp() �î榡��
        std::cout << std::setw(25) << time_utils::formatTimestampMs(pPlayer->getUpdatedTime()) << "\n";
    }

    std::cout << "--------------------------------------------------------\n"; // �վ���j�u����
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