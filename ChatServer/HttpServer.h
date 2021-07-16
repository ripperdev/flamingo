/**
 * 聊天服务支持http请求, HttpServer.h
 * zhangyl 2018.05.16
 */
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <list>              // for list
#include <memory>            // for shared_ptr, unique_ptr
#include <mutex>             // for mutex

#include "base/Singleton.h"  // for Singleton
#include "net/Callbacks.h"   // for net

namespace net {
class EventLoop;
class TcpConnection;
class TcpServer;
}  // namespace net

using namespace net;

class HttpSession;

class HttpServer final : public Singleton<HttpServer> {
public:
    HttpServer() = default;

    ~HttpServer() = default;

    HttpServer(const HttpServer &rhs) = delete;

    HttpServer &operator=(const HttpServer &rhs) = delete;

public:
    bool init(const char *ip, short port, EventLoop *loop);

    void uninit();

    //新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
    void onConnected(std::shared_ptr<TcpConnection> conn);

    //连接断开
    void onDisconnected(const std::shared_ptr<TcpConnection> &conn);

private:
    std::unique_ptr<TcpServer> m_server;
    std::list<std::shared_ptr<HttpSession>> m_sessions;
    std::mutex m_sessionMutex;      //多线程之间保护m_sessions
};

#endif //!__HTTP_SERVER_H__
