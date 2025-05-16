// 檔案名稱: ScheduleManager.h
// 建立日期: 2024-05-16
// 作者: Gemini AI
// 描述: 定義 ScheduleManager 類別，用於管理遊戲伺服器中的定時任務。
//      任務在獨立執行緒中運行，確保主邏輯的響應性。
//      任務間隔現在只支援整數秒。

#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <vector>
#include <functional>
#include <chrono>        // For std::chrono::steady_clock, std::chrono::seconds, std::chrono::time_point
#include <mutex>         // For std::mutex, std::lock_guard
#include <thread>        // For std::thread, std::this_thread
#include <atomic>        // For std::atomic<bool>
#include <iostream>      // For std::cout (示範用途，實際應用建議使用日誌庫)

// 代表一個要排程執行的任務
struct ScheduledTask
{
    std::function<void()> callback;                 // 任務的回調函式
    std::chrono::seconds interval;                  // 任務的執行間隔 (以秒為單位)
    std::chrono::steady_clock::time_point lastExecutionTime; // 上次執行任務的時間點
    bool isRepeating;                               // 任務是否重複執行

    // 構造函式
    ScheduledTask(std::function<void()> cb, std::chrono::seconds iv, bool repeat = true)
        : callback(std::move(cb)), interval(iv), lastExecutionTime(std::chrono::steady_clock::now()), isRepeating(repeat)
    {
    }
};

// ScheduleManager 類別 (單例模式，管理所有排程任務並在獨立執行緒中運行)
class ScheduleManager
{
public:
    // 獲取單例實例
    static ScheduleManager& instance()
    {
        static ScheduleManager instance;
        return instance;
    }

    // 初始化排程器，註冊所有預設的週期性任務並啟動工作執行緒
    bool initialize();

    // 釋放排程器資源，並安全停止工作執行緒
    void release();

    // 註冊一個新任務。
    // callback: 任務執行時調用的函式。
    // intervalSeconds: 任務的執行間隔 (整數秒)。
    // isRepeating: 任務是否重複執行 (預設為 true)。
    void scheduleTask(std::function<void()> callback, int intervalSeconds, bool isRepeating = true);

private:
    // 私有構造函式，確保單例模式
    ScheduleManager();

    // 私有解構函式
    ~ScheduleManager();

    // 禁用拷貝構造函式和拷貝賦值運算子，確保單例的唯一性
    ScheduleManager(const ScheduleManager&) = delete;
    ScheduleManager& operator=(const ScheduleManager&) = delete;

    std::vector<ScheduledTask> m_tasks;                 // 儲存所有排程任務的列表
    std::mutex m_mutex;                                 // 保護 m_tasks 向量的互斥鎖

    std::thread m_workerThread;                         // 用來執行排程器核心邏輯的獨立執行緒
    std::atomic<bool> m_running;                        // 標誌執行緒是否應該繼續運行

    // 執行緒會執行的核心迴圈函式
    void workerLoop();
};

#endif // SCHEDULE_MANAGER_H