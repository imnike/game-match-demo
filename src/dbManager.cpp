// @file  : dbManager.cpp
// @brief : ��Ʈw�޲z
// @author: August
// @date  : 2025-05-15
#include "dbManager.h"
#include "playerManager.h"
#include "../sqlite/sqlite3.h"
#include <../../utils/utils.h>
#include <iostream>
#include <chrono>

std::unordered_map<std::string, std::string> MAP_CREATE_TABLE_SQL = {
    {"player_battles", "CREATE TABLE IF NOT EXISTS player_battles (id INTEGER PRIMARY KEY, score INTEGER, wins INTEGER, updated_time INTEGER)"},
};

DbManager& DbManager::instance()
{
    static DbManager instance;
    return instance;
}

DbManager::DbManager()
    : pDbHandler(nullptr), dbName("gameMatch.db")
{
}

DbManager::~DbManager()
{
    if (pDbHandler)
    {
        sqlite3_close(pDbHandler);
        pDbHandler = nullptr;
    }
}

bool DbManager::initialize()
{
	mapFuncSyncData.clear();
    mapFuncSyncData["player_battles"] = [this]() { this->syncAllPlayerBattles(); };
    if (pDbHandler)
    {
        sqlite3_close(pDbHandler);
        pDbHandler = nullptr;
    }

    int rc = sqlite3_open(dbName.c_str(), &pDbHandler);
    if (rc != SQLITE_OK)
    {
        std::cerr << "DbManager::initialize: Cannot open database: " << sqlite3_errmsg(pDbHandler) << std::endl;
        pDbHandler = nullptr;
        return false;
    }
    return true;
}

void DbManager::release()
{
    if (pDbHandler)
    {
        sqlite3_close(pDbHandler);
        pDbHandler = nullptr;
	}
	mapFuncSyncData.clear();
}

void DbManager::ensureTableSchema()
{
    for (auto& itTable : MAP_CREATE_TABLE_SQL)
    {
        const std::string tableName = itTable.first;
        if (!isTableExists(tableName))
        {
			// table is not exists, create it
            std::cout << "DbManager::initialize: Table '" << tableName << "' does not exist. Creating..." << std::endl;
            if (!createTable(tableName))
            {
				// table creation failed
                std::cerr << "DbManager::initialize: Failed to create table '" << tableName << "'." << std::endl;
                return;
            }
        }
    }
}

void DbManager::loadTableData()
{
    for (auto& itFunc : mapFuncSyncData)
    {
		const std::string tableName = itFunc.first;
        // ����itFunc
        itFunc.second();
    }
}

// ��l�Ʈɨ��X�Ҧ����a��ƦP�B��playerManage
void DbManager::syncAllPlayerBattles()
{
    const char* sql = "SELECT id, score, wins, updated_time FROM player_battles;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(pDbHandler, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(pDbHandler) << std::endl;
        return;
    }
    uint64_t id = 0;
    uint32_t score = 0;
    uint32_t wins = 0;
    uint64_t updatedTime = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        id = sqlite3_column_int64(stmt, 0);
        score = sqlite3_column_int(stmt, 1);
        wins = sqlite3_column_int(stmt, 2);
        updatedTime = sqlite3_column_int64(stmt, 3);
        if (id == 0)
        {
            continue;
        }
        PlayerManager::instance().syncPlayerFromDbNoLock(id, score, wins, updatedTime);
    }
    sqlite3_finalize(stmt);
}

bool DbManager::isTableExists(const std::string tableName)
{
    if (!pDbHandler) 
    {
        std::cerr << "DbManager::tableExists: Database not open." << std::endl;
        return false;
    }

    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;

    int rc = sqlite3_prepare_v2(pDbHandler, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "DbManager::tableExists: Failed to prepare statement: " << sqlite3_errmsg(pDbHandler) << std::endl;
        // �Y�� prepare ���ѡA�]�T�O stmt �Q�M�z
        if (stmt) sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        exists = true; // ���F�ǰt����A��ܪ��s�b
    }

    sqlite3_finalize(stmt); // ���� stmt �귽
    return exists;
}

bool DbManager::createTable(const std::string tableName)
{
    auto itSql = MAP_CREATE_TABLE_SQL.find(tableName);
    if (itSql == MAP_CREATE_TABLE_SQL.end())
    {
        std::cerr << "DbManager::createTable: Table name not found in map." << std::endl;
		return false;
    }
    char* errMsg = nullptr;
    int rc = sqlite3_exec(pDbHandler, itSql->second.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

uint64_t DbManager::insertPlayerBattles()
{
    // SQL �y�y�����w id�A�� SQLite �۰ʥͦ� (�]�� id �O INTEGER PRIMARY KEY AUTOINCREMENT)
    const char* sql =
        "INSERT INTO player_battles (score, wins, updated_time) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(pDbHandler, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare insertNewPlayer statement: " << sqlite3_errmsg(pDbHandler) << std::endl;
        return 0;
    }
    const uint32_t score = 0;
	const uint32_t wins = 0;
	const uint64_t updatedTime = time_utils::getTimestampMS(); // �����e�ɶ��W

    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, wins);
    sqlite3_bind_int64(stmt, 3, updatedTime);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt); // ���槹���ᥲ���M�z�y�y

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to insert new player: " << sqlite3_errmsg(pDbHandler) << std::endl;
        return 0;
    }

    // ���\���J��A����s�ͦ��� ID
    uint64_t id = sqlite3_last_insert_rowid(pDbHandler);
    if (id == 0)
    {
        return 0;
    }
	PlayerManager::instance().syncPlayerFromDbNoLock(id, score, wins, updatedTime); // �P�B�� PlayerManager
    return id;
}
bool DbManager::updatePlayerBattles(uint64_t id, uint32_t score, uint32_t wins)
{
    // SQL �y�y�G�`�N ? �����ǥ����P�j�w���Ǥ@�P
    const char* sql = "UPDATE player_battles SET score = ?, wins = ?, updated_time = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(pDbHandler, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "DbManager::updatePlayerBattles: Failed to prepare statement: " << sqlite3_errmsg(pDbHandler) << std::endl;
        return false;
    }

    const uint64_t updatedTime = time_utils::getTimestampMS();

    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, wins);
    sqlite3_bind_int64(stmt, 3, updatedTime);
    sqlite3_bind_int64(stmt, 4, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt); // ���槹���ᥲ���M�z�y�y

    if (rc != SQLITE_DONE)
    {
        std::cerr << "DbManager::updatePlayerBattles: Failed to save player: " << sqlite3_errmsg(pDbHandler) << std::endl;
        return false;
    }

    return true;
}
bool DbManager::queryPlayerBattles(uint64_t id, uint32_t& score, uint32_t& wins, uint64_t& updateTime)
{
    const char* sql = "SELECT score, wins, updated_time FROM player_battles WHERE id = ?;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(pDbHandler, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(pDbHandler) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        score = sqlite3_column_int(stmt, 0);
        wins = sqlite3_column_int(stmt, 1);
        updateTime = sqlite3_column_int64(stmt, 2);
        sqlite3_finalize(stmt);
        return true;
    }
    return false;
}