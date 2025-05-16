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

    // �o�O�֤ߪ��üƥͦ��禡�G�q min �� max (���϶�)
    template <typename T>
    T getRandomRange(T min, T max)
    {
        if (min > max)
        {
            throw std::invalid_argument("random_utils::getRandomRange: min cannot be greater than max.");
        }

        // �ץ��I�G�ϥΤ@�� std::uniform_int_distribution �䴩������ (�Ҧp int)
        // �ñN min �M max �ഫ���������H�ǰt������
        // �T�O�ഫ���|�ɭP���X�]�Ҧp uint64_t �� int �ݭn�p�ߡA���� uint8_t �S���D�^
        std::uniform_int_distribution<int> distrib(static_cast<int>(min), static_cast<int>(max));

        // ���ͶüơA�M��N���G�ഫ�^��l�� T ���O
        return static_cast<T>(distrib(gen));
    }

    // �o�O��K�ϥΪ��üƥͦ��禡�G�q 0 �� max - 1
    template <typename T>
    T getRandom(T max)
    {
        if (max <= 0)
        {
            return 0; // �Ϊ̮ھڧA���ݨD�A�i�H�ߥX���`
        }
        // �ץ��I�G�T�O�Ѽ��������T�B�L�[�q
        return getRandomRange(static_cast<T>(0), static_cast<T>(max - 1));
    }
}
#endif // TIME_UTILS_H