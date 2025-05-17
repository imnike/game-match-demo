// @file  : utils.cpp
// @brief : 工具函式
// @author: August
// @date  : 2025-05-15
#include "utils.h"
#include <chrono>    // for std::chrono::system_clock, milliseconds, time_point
#include <sstream>
#include <iomanip>   // for std::put_time

namespace time_utils
{
    uint64_t getTimestampMS()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return static_cast<uint64_t>(duration.count());
    }

    uint64_t getTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
		return static_cast<uint64_t>(duration.count());
    }
    std::string formatTimestampMs(uint64_t timestamp_ms)
    {
        // 將毫秒轉換為秒 (std::time_t 通常是秒級別)
        std::time_t timeT_in_seconds = static_cast<std::time_t>(timestamp_ms / 1000);

        std::tm local_tm_struct; // 在棧上聲明一個 std::tm 結構體

        // 使用 localtime_s 進行轉換
        // localtime_s 在成功時返回 0，失敗時返回非零值
        // 第一個參數是目標 tm 結構體的位址，第二個參數是源 time_t 的位址
        if (localtime_s(&local_tm_struct, &timeT_in_seconds) != 0)
        {
            return "Invalid Time (conversion failed)"; // 轉換失敗
        }

        std::ostringstream oss;
        // 使用 std::put_time 格式化時間，它接受一個指向 tm 結構的指針
        oss << std::put_time(&local_tm_struct, "%Y-%m-%d %H:%M:%S");

        // 計算並添加毫秒部分
        int remaining_ms = timestamp_ms % 1000;
        oss << "." << std::setw(3) << std::setfill('0') << remaining_ms;

        return oss.str();
    }
}
