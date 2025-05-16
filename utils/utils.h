// timeUtils.h
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <cstdint> // For uint64_t
#include <random>
#include <limits>

namespace time_utils
{
    uint64_t getTimestampMS();
    uint64_t getTimestamp();
    std::string formatTimestampMs(uint64_t timestamp);
}
namespace random_utils
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // 這是核心的亂數生成函式：從 min 到 max (閉區間)
    template <typename T>
    T getRandomRange(T min, T max)
    {
        if (min > max)
        {
            throw std::invalid_argument("random_utils::getRandomRange: min cannot be greater than max.");
        }

        // 修正點：使用一個 std::uniform_int_distribution 支援的類型 (例如 int)
        // 並將 min 和 max 轉換為該類型以匹配分布器
        // 確保轉換不會導致溢出（例如 uint64_t 轉 int 需要小心，但對 uint8_t 沒問題）
        std::uniform_int_distribution<int> distrib(static_cast<int>(min), static_cast<int>(max));

        // 產生亂數，然後將結果轉換回原始的 T 型別
        return static_cast<T>(distrib(gen));
    }

    // 這是方便使用的亂數生成函式：從 0 到 max - 1
    template <typename T>
    T getRandom(T max)
    {
        if (max <= 0)
        {
            return 0; // 或者根據你的需求，可以拋出異常
        }
        // 修正點：確保參數類型正確且無歧義
        return getRandomRange(static_cast<T>(0), static_cast<T>(max - 1));
    }
}
#endif // TIME_UTILS_H