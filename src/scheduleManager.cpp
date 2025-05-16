// 檔案名稱: ScheduleManager.cpp
// 建立日期: 2024-05-16
// 作者: Gemini AI
// 描述: ScheduleManager 類別的實作。負責管理和執行定時任務。
//      核心邏輯在獨立執行緒中運行，確保主應用程式的響應性。

#include "ScheduleManager.h"
#include "PlayerManager.h" // 假設 PlayerManager 已經存在並包含 saveDirtyPlayers()

// 初始化排程器，註冊所有預設的週期性任務並啟動工作執行緒
bool ScheduleManager::initialize()
{
    // 註冊玩家數據存檔任務 (每 5 秒)
    scheduleTask(
        []()
        {
            PlayerManager::instance().saveDirtyPlayers();
            std::cout << "[ScheduleManager] Player data save triggered.\n";
        },
        5 // 間隔：5 秒
    );

    // 註冊遊戲心跳任務 (每 10 秒)
    scheduleTask(
        []()
        {
            std::cout << "[ScheduleManager] Game heartbeat triggered.\n";
        },
        10 // 間隔：10 秒
    );

    // 註冊一個一次性啟動檢查任務 (3 秒後執行一次)
    scheduleTask(
        []()
        {
            std::cout << "[ScheduleManager] One-time startup check completed!\n";
        },
        3,      // 間隔：3 秒
        false   // 只執行一次
    );

    std::cout << "ScheduleManager initialized and tasks scheduled.\n";

    // 啟動獨立的工作執行緒
    m_running = true; // 設定運行標誌
    m_workerThread = std::thread(&ScheduleManager::workerLoop, this); // 啟動執行緒，執行 workerLoop
    return true;
}

// 釋放排程器資源，並安全停止工作執行緒
void ScheduleManager::release()
{
    if (m_running)
    {
        m_running = false; // 設定標誌停止執行緒
        if (m_workerThread.joinable())
        {
            m_workerThread.join(); // 等待執行緒完成其工作並結束
            std::cout << "[ScheduleManager] Worker thread joined.\n";
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex); // 鎖定互斥量以安全地清空任務列表
    m_tasks.clear(); // 清空所有排程的任務
    std::cout << "ScheduleManager released.\n";
}

// 註冊一個新任務
void ScheduleManager::scheduleTask(std::function<void()> callback, int intervalSeconds, bool isRepeating)
{
    std::lock_guard<std::mutex> lock(m_mutex); // 保護 m_tasks 向量

    // 直接使用 int 構建 std::chrono::seconds，無需複雜轉換
    m_tasks.emplace_back(callback, std::chrono::seconds(intervalSeconds), isRepeating);
}

// 私有構造函式實作
ScheduleManager::ScheduleManager()
    : m_running(false) // 初始化 m_running
{
    // 構造時通常不做太多事，主要初始化在 initialize() 中
}

// 私有解構函式實作
ScheduleManager::~ScheduleManager()
{
    // 解構時清理資源
    release();
}

// 執行緒會執行的核心迴圈函式
void ScheduleManager::workerLoop()
{
    std::cout << "[ScheduleManager Thread] Worker loop started.\n";
    while (m_running) // 迴圈會一直運行直到 m_running 變為 false
    {
        std::lock_guard<std::mutex> lock(m_mutex); // 鎖定互斥量以安全地訪問 m_tasks

        auto now = std::chrono::steady_clock::now(); // 獲取當前高精度時間點

        // 遍歷所有排程的任務
        // 使用迭代器 `it` 方便在迴圈中安全地移除一次性任務
        for (auto it = m_tasks.begin(); it != m_tasks.end(); )
        {
            // 檢查任務是否到期 (當前時間 - 上次執行時間 >= 任務間隔)
            if ((now - it->lastExecutionTime) >= it->interval)
            {
                it->callback(); // 執行任務的回調函式

                // 更新上次執行時間為當前時間點
                it->lastExecutionTime = now;

                if (!it->isRepeating)
                {
                    // 如果是只執行一次的任務，執行後就從列表中移除
                    it = m_tasks.erase(it);
                }
                else
                {
                    // 如果是重複執行的任務，則移動到下一個任務
                    ++it;
                }
            }
            else
            {
                // 任務未到期，移動到下一個任務
                ++it;
            }
        }
        // 控制執行緒的頻率，避免 CPU 空轉，同時確保能捕捉到精確的秒數觸發
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 每 1 毫秒檢查一次
    }
    std::cout << "[ScheduleManager Thread] Worker loop stopped.\n";
}