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

    // �� operator< �D�n�O���F�b `std::sort` �ɨϥΡA���O�� `std::set` �ϥ�
    // �ƦC�W�h�G���ư����u���A���ƬۦP�h�ӳ��h���u���A��̬ҦP�h ID ���p���u���C
    bool operator<(const PlayerRankInfo& other) const {
        if (m_score != other.m_score) {
            return m_score > other.m_score; // ���ǡG�����b�e
        }
        if (m_wins != other.m_wins) {
            return m_wins > other.m_wins;   // ���ǡG�h�ӳ��b�e
        }
        return m_id < other.m_id;           // �ɧǡGID �p���b�e (�Ω�í�w�Ƨ�)
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

    // --- �Ʀ�]�����s�W��k ---
    // �ҰʱƦ�]��s�����
    void startLeaderboardUpdateThread(std::chrono::seconds intervalSeconds);
    // ����Ʀ�]��s�����
    void stopLeaderboardUpdateThread();
    // ���o�w�s���Ʀ�]��� (�Ȥ�ݩI�s�o�Ө�����Ʀ�])
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

    // �������ڰ��檺�禡
    void leaderboardUpdateLoop(std::chrono::seconds interval);
    // ������k�A�Ω��ڭp��Ʀ�]�ç�s�w�s
    void calculateAndCacheLeaderboard();

    std::unordered_map<uint64_t, std::unique_ptr<Player>> mapPlayers{};
	std::set<uint64_t> setOnlinePlayerIds{};
	std::mutex mapPlayersMutex;

    std::set<uint64_t> setDirtyPlayerIds{};
    std::mutex setdirtyPlayerIdsMutex;

    // --- �Ʀ�]�M�Φ��� ---
    std::vector<PlayerRankInfo> m_vecCachedLeaderboard; // �x�s�w�p��B�ƧǦn���Ʀ�]�ƾ�
    std::mutex m_cachedLeaderboardMutex;             // �O�@ m_cachedLeaderboard ��������

    std::thread m_leaderboardThread;                 // �Ʀ�]��s���W�߰����
    std::atomic_bool m_running;                      // ���������B�檬�A����l�X��

};

#endif // !PLAYER_MANAGER_H