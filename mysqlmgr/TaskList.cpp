#include "TaskList.h"
#include <cstring>
#include "../base/AsyncLog.h"

CTaskList::CTaskList() : m_uReadIndex(0), m_uWriteIndex(0) {
    memset(m_pTaskNode, 0, sizeof(m_pTaskNode));
}

CTaskList::~CTaskList() {
    for (auto & i : m_pTaskNode) {
        delete i;
    }
}

bool CTaskList::push(IMysqlTask *poTask) {

    auto usNextIndex = static_cast<uint16_t>((m_uWriteIndex + 1) % MAX_TASK_NUM);
    if (usNextIndex == m_uReadIndex) {
        // getchar();
        //LOGE << "mysql task list full (read : " << m_uReadIndex << ", write : " << m_uWriteIndex << ")";
        return false;
    }

    m_pTaskNode[m_uWriteIndex] = poTask;
    m_uWriteIndex = usNextIndex;

    return true;
}

IMysqlTask *CTaskList::pop() {
    if (m_uWriteIndex == m_uReadIndex) {
        return nullptr;
    }
    IMysqlTask *pTask = m_pTaskNode[m_uReadIndex];
    m_uReadIndex = static_cast<uint16_t>((m_uReadIndex + 1) % MAX_TASK_NUM);

    return pTask;
}
