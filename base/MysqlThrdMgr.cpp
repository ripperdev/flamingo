#include "MysqlThrdMgr.h"

#include "MysqlTask.h"  // for IMysqlTask

bool CMysqlThrdMgr::addTask(uint32_t dwHashID, IMysqlTask *poTask) {
    //LOG_DEBUG << "CMysqlThrdMgr::AddTask, HashID = " << dwHashID;
    uint32_t btIndex = getTableHashID(dwHashID);

    if (btIndex >= m_dwThreadsCount) {
        return false;
    }

    return m_aoMysqlThreads[btIndex].addTask(poTask);
}

bool CMysqlThrdMgr::init(const std::string &host, const std::string &user, const std::string &pwd,
                         const std::string &dbname) {
    for (auto &m_aoMysqlThread : m_aoMysqlThreads) {
        if (!m_aoMysqlThread.start(host, user, pwd, dbname)) {
            return false;
        }
    }

    return true;
}

bool CMysqlThrdMgr::processReplyTask(int32_t nCount) {
    bool bResult = false;

    for (auto & m_aoMysqlThread : m_aoMysqlThreads) {
        IMysqlTask *poTask = m_aoMysqlThread.getReplyTask();
        int32_t dwProcessedNbr = 0;

        while (((nCount == -1) || (dwProcessedNbr < nCount)) && (nullptr != poTask)) {
            poTask->reply();
            poTask->release();
            poTask = m_aoMysqlThread.getReplyTask();
            ++dwProcessedNbr;
        }

        if (dwProcessedNbr == nCount) {
            bResult = true;
        }
    }
    return bResult;
}
