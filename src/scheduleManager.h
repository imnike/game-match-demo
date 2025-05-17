// scheduleManager.h
#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <vector>
#include <functional>
#include <chrono>        // For std::chrono::steady_clock, std::chrono::seconds, std::chrono::time_point
#include <mutex>         // For std::mutex, std::lock_guard
#include <thread>        // For std::thread, std::this_thread
#include <atomic>        // For std::atomic<bool>
#include <iostream>      // For std::cout (�ܽd�γ~�A������Ϋ�ĳ�ϥΤ�x�w)

// �N��@�ӭn�Ƶ{���檺����
struct ScheduledTask
{
    std::function<void()> callback;                 // ���Ȫ��^�ը禡
    std::chrono::seconds interval;                  // ���Ȫ����涡�j (�H�����)
    std::chrono::steady_clock::time_point lastExecutionTime; // �W��������Ȫ��ɶ��I
    bool isRepeating;                               // ���ȬO�_���ư���

    // �c�y�禡
    ScheduledTask(std::function<void()> cb, std::chrono::seconds iv, bool repeat = true)
        : callback(std::move(cb)), interval(iv), lastExecutionTime(std::chrono::steady_clock::now()), isRepeating(repeat)
    {
    }
};

// ScheduleManager ���O (��ҼҦ��A�޲z�Ҧ��Ƶ{���Ȩæb�W�߰�������B��)
class ScheduleManager
{
public:
    // �����ҹ��
    static ScheduleManager& instance()
    {
        static ScheduleManager instance;
        return instance;
    }

    // ��l�ƱƵ{���A���U�Ҧ��w�]���g���ʥ��ȨñҰʤu�@�����
    bool initialize();

    // ����Ƶ{���귽�A�æw������u�@�����
    void release();

    // ���U�@�ӷs���ȡC
    // callback: ���Ȱ���ɽեΪ��禡�C
    // intervalSeconds: ���Ȫ����涡�j (��Ƭ�)�C
    // isRepeating: ���ȬO�_���ư��� (�w�]�� true)�C
    void scheduleTask(std::function<void()> callback, int intervalSeconds, bool isRepeating = true);

private:
    ScheduleManager();
    ~ScheduleManager();

    ScheduleManager(const ScheduleManager&) = delete;
    ScheduleManager& operator=(const ScheduleManager&) = delete;
    ScheduleManager(ScheduleManager&&) = delete;
    ScheduleManager& operator=(ScheduleManager&&) = delete;

    std::vector<ScheduledTask> m_tasks{};               // �x�s�Ҧ��Ƶ{���Ȫ��C��
    std::mutex m_mutex;                                 // �O�@ m_tasks �V�q��������

    std::thread m_workerThread;                         // �ΨӰ���Ƶ{���֤��޿誺�W�߰����
    std::atomic<bool> m_running;                        // �лx������O�_�����~��B��

    // ������|���檺�֤߰j��禡
    void workerLoop();
};

#endif // SCHEDULE_MANAGER_H