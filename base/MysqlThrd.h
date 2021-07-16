#pragma once

#include <condition_variable>  // for condition_variable
#include <memory>              // for unique_ptr
#include <mutex>               // for mutex
#include <string>              // for string
#include <thread>              // for thread

#include "TaskList.h"          // for CTaskList

class CDatabaseMysql;
class IMysqlTask;

class CMysqlThrd {
public:
    CMysqlThrd();

    void Run();

    bool start(const std::string &host, const std::string &user, const std::string &pwd, const std::string &dbname);

    void stop();

    bool addTask(IMysqlTask *poTask) {
        return m_oTask.push(poTask);
    }

    IMysqlTask *getReplyTask() {
        return m_oReplyTask.pop();
    }

protected:
    void init();

    void mainLoop();

    void uninit();

private:
    bool m_bTerminate;
    std::unique_ptr<std::thread> m_pThread;
    bool m_bStart;
    CDatabaseMysql *m_poConn;
    CTaskList m_oTask;
    CTaskList m_oReplyTask;

    std::mutex mutex_;
    std::condition_variable cond_;
};
