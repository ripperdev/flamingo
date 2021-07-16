/**
 * 监控会话类, MonitorSession.h
 * zhangyl 2017.03.09
 */
#ifndef __MONITOR_SESSION_H__
#define __MONITOR_SESSION_H__

#include <stddef.h>       // for size_t
#include <stdint.h>       // for int32_t
#include <memory>         // for shared_ptr, weak_ptr, allocator
#include <string>         // for string

#include "net/Sockets.h"  // for net

class Timestamp;
namespace net {
class Buffer;
class TcpConnection;
}  // namespace net

using namespace net;

class MonitorSession {
public:
    explicit MonitorSession(std::shared_ptr<TcpConnection> &conn);

    ~MonitorSession() = default;

    MonitorSession(const MonitorSession &rhs) = delete;

    MonitorSession &operator=(const MonitorSession &rhs) = delete;

public:
    //有数据可读, 会被多个工作loop调用
    void onRead(const std::shared_ptr<TcpConnection> &conn, Buffer *pBuffer, Timestamp receivTime);

    std::shared_ptr<TcpConnection> getConnectionPtr() {
        if (m_tmpConn.expired())
            return nullptr;

        return m_tmpConn.lock();
    }

    void showHelp();

    void send(const char *data, size_t length);

private:
    bool process(const std::shared_ptr<TcpConnection> &conn, const std::string &inbuf);

    bool showOnlineUserList(const std::string &token = "");

    bool showSpecifiedUserInfoByID(int32_t userid);

private:
    std::weak_ptr<TcpConnection> m_tmpConn;
};

#endif //!__MONITOR_SESSION_H__
