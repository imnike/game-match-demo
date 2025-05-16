#include "timeUtils.h"
#include <chrono>

namespace DemoTimeUtils
{
    uint64_t getTimestampMS()
    {
        auto now = std::chrono::system_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

        return static_cast<uint64_t>(duration.count());
    }
}