// �ɮצW��: ScheduleManager.cpp
// �إߤ��: 2024-05-16
// �@��: Gemini AI
// �y�z: ScheduleManager ���O����@�C�t�d�޲z�M����w�ɥ��ȡC
//      �֤��޿�b�W�߰�������B��A�T�O�D���ε{�����T���ʡC

#include "ScheduleManager.h"
#include "PlayerManager.h" // ���] PlayerManager �w�g�s�b�å]�t saveDirtyPlayers()

// ��l�ƱƵ{���A���U�Ҧ��w�]���g���ʥ��ȨñҰʤu�@�����
bool ScheduleManager::initialize()
{
    // ���U���a�ƾڦs�ɥ��� (�C 5 ��)
    scheduleTask(
        []()
        {
            PlayerManager::instance().saveDirtyPlayers();
            std::cout << "[ScheduleManager] Player data save triggered.\n";
        },
        5 // ���j�G5 ��
    );

    // ���U�C���߸����� (�C 10 ��)
    scheduleTask(
        []()
        {
            std::cout << "[ScheduleManager] Game heartbeat triggered.\n";
        },
        10 // ���j�G10 ��
    );

    // ���U�@�Ӥ@���ʱҰ��ˬd���� (3 ������@��)
    scheduleTask(
        []()
        {
            std::cout << "[ScheduleManager] One-time startup check completed!\n";
        },
        3,      // ���j�G3 ��
        false   // �u����@��
    );

    std::cout << "ScheduleManager initialized and tasks scheduled.\n";

    // �ҰʿW�ߪ��u�@�����
    m_running = true; // �]�w�B��лx
    m_workerThread = std::thread(&ScheduleManager::workerLoop, this); // �Ұʰ�����A���� workerLoop
    return true;
}

// ����Ƶ{���귽�A�æw������u�@�����
void ScheduleManager::release()
{
    if (m_running)
    {
        m_running = false; // �]�w�лx��������
        if (m_workerThread.joinable())
        {
            m_workerThread.join(); // ���ݰ����������u�@�õ���
            std::cout << "[ScheduleManager] Worker thread joined.\n";
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex); // ��w�����q�H�w���a�M�ť��ȦC��
    m_tasks.clear(); // �M�ũҦ��Ƶ{������
    std::cout << "ScheduleManager released.\n";
}

// ���U�@�ӷs����
void ScheduleManager::scheduleTask(std::function<void()> callback, int intervalSeconds, bool isRepeating)
{
    std::lock_guard<std::mutex> lock(m_mutex); // �O�@ m_tasks �V�q

    // �����ϥ� int �c�� std::chrono::seconds�A�L�ݽ����ഫ
    m_tasks.emplace_back(callback, std::chrono::seconds(intervalSeconds), isRepeating);
}

// �p���c�y�禡��@
ScheduleManager::ScheduleManager()
    : m_running(false) // ��l�� m_running
{
    // �c�y�ɳq�`�����Ӧh�ơA�D�n��l�Ʀb initialize() ��
}

// �p���Ѻc�禡��@
ScheduleManager::~ScheduleManager()
{
    // �Ѻc�ɲM�z�귽
    release();
}

// ������|���檺�֤߰j��禡
void ScheduleManager::workerLoop()
{
    std::cout << "[ScheduleManager Thread] Worker loop started.\n";
    while (m_running) // �j��|�@���B�檽�� m_running �ܬ� false
    {
        std::lock_guard<std::mutex> lock(m_mutex); // ��w�����q�H�w���a�X�� m_tasks

        auto now = std::chrono::steady_clock::now(); // �����e����׮ɶ��I

        // �M���Ҧ��Ƶ{������
        // �ϥέ��N�� `it` ��K�b�j�餤�w���a�����@���ʥ���
        for (auto it = m_tasks.begin(); it != m_tasks.end(); )
        {
            // �ˬd���ȬO�_��� (��e�ɶ� - �W������ɶ� >= ���ȶ��j)
            if ((now - it->lastExecutionTime) >= it->interval)
            {
                it->callback(); // ������Ȫ��^�ը禡

                // ��s�W������ɶ�����e�ɶ��I
                it->lastExecutionTime = now;

                if (!it->isRepeating)
                {
                    // �p�G�O�u����@�������ȡA�����N�q�C������
                    it = m_tasks.erase(it);
                }
                else
                {
                    // �p�G�O���ư��檺���ȡA�h���ʨ�U�@�ӥ���
                    ++it;
                }
            }
            else
            {
                // ���ȥ�����A���ʨ�U�@�ӥ���
                ++it;
            }
        }
        // �����������W�v�A�קK CPU ����A�P�ɽT�O�ஷ�����T�����Ĳ�o
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // �C 1 �@���ˬd�@��
    }
    std::cout << "[ScheduleManager Thread] Worker loop stopped.\n";
}