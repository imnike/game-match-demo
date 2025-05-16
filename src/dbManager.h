// dbManager.h
#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

struct sqlite3;

class DbManager
{
public:
    static DbManager& instance();

    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    ~DbManager();

    bool initialize();
    void release();
    void loadTableData();
    void ensureTableSchema();
    bool isTableExists(const std::string tableName);
    bool createTable(const std::string tableName);

    void syncAllPlayerBattles();
    uint64_t insertPlayerBattles();
    bool updatePlayerBattles(uint64_t id, uint32_t score, uint32_t wins);
    bool queryPlayerBattles(uint64_t id, uint32_t& score, uint32_t& wins, uint64_t& updateTime);


private:
    DbManager();

    sqlite3* pDbHandler = nullptr;
    std::string dbName = "";
    std::unordered_map<std::string, std::function<void()>> mapFuncSyncData{};
};

#endif // DB_MANAGER_H