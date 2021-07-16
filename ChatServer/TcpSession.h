/**
 * TcpSession.h
 * zhangyl 2017.03.09
 **/

#pragma once

#include <stdint.h>         // for int32_t
#include <memory>           // for weak_ptr, shared_ptr
#include <string>           // for string

#include "net/Callbacks.h"  // for net

namespace net {
class TcpConnection;
}  // namespace net

using namespace net;

//为了让业务与逻辑分开，实际应该新增一个子类继承自TcpSession，让TcpSession中只有逻辑代码，其子类存放业务代码
class TcpSession {
public:
    explicit TcpSession(std::weak_ptr<TcpConnection> tmpconn);

    virtual ~TcpSession() = default;

    TcpSession(const TcpSession &rhs) = delete;

    TcpSession &operator=(const TcpSession &rhs) = delete;

    std::shared_ptr<TcpConnection> getConnectionPtr() {
        if (tmpConn_.expired())
            return nullptr;

        return tmpConn_.lock();
    }

    void send(int32_t cmd, int32_t seq, const std::string &data);

    void send(int32_t cmd, int32_t seq, const char *data, int32_t dataLength);

    void send(const std::string &p);

    void send(const char *p, int32_t length);

private:
    void sendPackage(const char *p, int32_t length);

protected:
    //TcpSession引用TcpConnection类必须是弱指针，因为TcpConnection可能会因网络出错自己销毁，此时TcpSession应该也要销毁
    std::weak_ptr<TcpConnection> tmpConn_;
    //std::shared_ptr<TcpConnection>    tmpConn_;
};
