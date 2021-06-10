#pragma once

#include "MysqlTask.h"
#include "MysqlThrd.h"

class CMysqlThrdMgr {
public:
    CMysqlThrdMgr() = default;

    virtual ~CMysqlThrdMgr() = default;

    bool init(const std::string &host, const std::string &user, const std::string &pwd, const std::string &dbname);

    bool addTask(uint32_t dwHashID, IMysqlTask *poTask);

    bool addTask(IMysqlTask *poTask) {
        return m_aoMysqlThreads[m_dwThreadsCount].addTask(poTask);
    }

    [[nodiscard]] static inline uint32_t getTableHashID(uint32_t dwHashID) {
        return dwHashID % m_dwThreadsCount;
    }

    bool processReplyTask(int32_t nCount);

    static uint32_t getThreadsCount() {
        return m_dwThreadsCount;
    }

protected:
    static const uint32_t m_dwThreadsCount = 9;
    CMysqlThrd m_aoMysqlThreads[m_dwThreadsCount + 1];
};
