#include "MysqlThrd.h"

#include <memory>
#include "../base/AsyncLog.h"

CMysqlThrd::CMysqlThrd() {
    m_bTerminate = false;

    m_bStart = false;
    m_poConn = nullptr;
}

void CMysqlThrd::Run() {
    mainLoop();
    uninit();

    if (m_pThread) {
        m_pThread->join();
    }
}

bool CMysqlThrd::start(const std::string &host,
                       const std::string &user,
                       const std::string &pwd,
                       const std::string &dbname) {
    m_poConn = new CDatabaseMysql();

    if (!m_poConn->initialize(host, user, pwd, dbname)) {
        return false;
    }
    init();
    return true;
}

void CMysqlThrd::stop() {
    if (m_bTerminate) {
        return;
    }
    m_bTerminate = true;
    m_pThread->join();
}

void CMysqlThrd::init() {
    if (m_bStart) {
        return;
    }

    m_pThread = std::make_unique<std::thread>([this] { mainLoop(); });

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!m_bStart) {
            cond_.wait(lock);
        }
    }
}

void CMysqlThrd::uninit() {
    //m_poConn->Close();
}

void CMysqlThrd::mainLoop() {
    m_bStart = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.notify_all();
    }

    IMysqlTask *poTask;

    while (!m_bTerminate) {
        if (nullptr != (poTask = m_oTask.pop())) {
            poTask->execute(m_poConn);
            m_oReplyTask.push(poTask);
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
