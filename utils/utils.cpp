// @file  : utils.cpp
// @brief : �u��禡
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
        // �N�@���ഫ���� (std::time_t �q�`�O��ŧO)
        std::time_t timeT_in_seconds = static_cast<std::time_t>(timestamp_ms / 1000);

        std::tm local_tm_struct; // �b�̤W�n���@�� std::tm ���c��

        // �ϥ� localtime_s �i���ഫ
        // localtime_s �b���\�ɪ�^ 0�A���Ѯɪ�^�D�s��
        // �Ĥ@�ӰѼƬO�ؼ� tm ���c�骺��}�A�ĤG�ӰѼƬO�� time_t ����}
        if (localtime_s(&local_tm_struct, &timeT_in_seconds) != 0)
        {
            return "Invalid Time (conversion failed)"; // �ഫ����
        }

        std::ostringstream oss;
        // �ϥ� std::put_time �榡�Ʈɶ��A�������@�ӫ��V tm ���c�����w
        oss << std::put_time(&local_tm_struct, "%Y-%m-%d %H:%M:%S");

        // �p��òK�[�@����
        int remaining_ms = timestamp_ms % 1000;
        oss << "." << std::setw(3) << std::setfill('0') << remaining_ms;

        return oss.str();
    }
}
