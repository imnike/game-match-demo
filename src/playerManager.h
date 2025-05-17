// playerManager.h
#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H
#include "objects/player.h"
#include <unordered_map>
#include <set>
#include <mutex>
#include <cstdint>

struct PlayerRankInfo
{
    uint64_t m_id;
    uint32_t m_score;
    uint32_t m_wins;

    // 此 operator< 主要是為了在 `std::sort` 時使用，不是給 `std::set` 使用
    // 排列規則：分數高者優先，分數相同則勝場多者優先，兩者皆同則 ID 較小者優先。
    bool operator<(const PlayerRankInfo& other) const {
        if (m_score != other.m_score) {
            return m_score > other.m_score; // 降序：高分在前
        }
        if (m_wins != other.m_wins) {
            return m_wins > other.m_wins;   // 降序：多勝場在前
        }
        return m_id < other.m_id;           // 升序：ID 小的在前 (用於穩定排序)
    }
};

class PlayerManager
{
public:

    static PlayerManager& instance();

    bool initialize();
    void release();
    Player* playerLogin(uint64_t id);
    bool playerLogout(uint64_t id);
    bool isPlayerOnline(uint64_t id);
    std::vector<Player*> getOnlinePlayers();
	std::unordered_map<uint64_t, std::unique_ptr<Player>>* getAllPlayers() { return &mapPlayers; }
    std::set<uint64_t>* getOnlinePlayerIds() { return &setOnlinePlayerIds; }
    void syncPlayerFromDbNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime);

    void handlePlayerBattleResult(uint64_t playerId, uint32_t scoreDelta, bool isWin);

    void enqueuePlayerSave(uint64_t playerId);
    void saveDirtyPlayers();

    //void getTopPlayers(std::vector<PlayerRank>& refVecTops);

    // --- 排行榜相關新增方法 ---
    // 啟動排行榜更新執行緒
    void startLeaderboardUpdateThread(std::chrono::seconds intervalSeconds);
    // 停止排行榜更新執行緒
    void stopLeaderboardUpdateThread();
    // 取得緩存的排行榜資料 (客戶端呼叫這個來獲取排行榜)
    std::vector<PlayerRankInfo> getLeaderboard(size_t topN = 100);

private:

    PlayerManager();
    ~PlayerManager();

    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;
    PlayerManager(PlayerManager&&) = delete;
    PlayerManager& operator=(PlayerManager&&) = delete;

    Player* _getPlayerNoLock(uint64_t id);
    void _setPlayerOnlineNoLock(uint64_t id, bool isOnline);
    void _syncPlayerNoLock(uint64_t id, uint32_t score, uint32_t wins, uint64_t updatedTime);

    // 執行緒實際執行的函式
    void leaderboardUpdateLoop(std::chrono::seconds interval);
    // 內部方法，用於實際計算排行榜並更新緩存
    void calculateAndCacheLeaderboard();

    std::unordered_map<uint64_t, std::unique_ptr<Player>> mapPlayers{};
	std::set<uint64_t> setOnlinePlayerIds{};
	std::mutex mapPlayersMutex;

    std::set<uint64_t> setDirtyPlayerIds{};
    std::mutex setdirtyPlayerIdsMutex;

    // --- 排行榜專用成員 ---
    std::vector<PlayerRankInfo> m_vecCachedLeaderboard; // 儲存預計算且排序好的排行榜數據
    std::mutex m_cachedLeaderboardMutex;             // 保護 m_cachedLeaderboard 的互斥鎖

    std::thread m_leaderboardThread;                 // 排行榜更新的獨立執行緒
    std::atomic_bool m_running;                      // 控制執行緒運行狀態的原子旗標

};

#endif // !PLAYER_MANAGER_H