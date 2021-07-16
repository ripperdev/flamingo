#include <functional>           // for _Bind_helper<>::type, _Placeholder, bind

#include "MonitorServer.h"
#include "MonitorSession.h"     // for MonitorSession
#include "base/Logger.h"        // for LOG_ERROR, LOG_INFO
#include "base/Timestamp.h"     // for Timestamp
#include "net/InetAddress.h"    // for InetAddress
#include "net/TcpConnection.h"  // for TcpConnection
#include "net/TcpServer.h"      // for TcpServer, TcpServer::kReusePort

namespace net {
class EventLoop;
}  // namespace net

bool MonitorServer::init(const char *ip, short port, EventLoop *loop, const char *token) {
    m_token = token;

    InetAddress addr(ip, port);
    m_server.reset(new TcpServer(loop, addr, "ZYL-MYIMMONITORSERVER", TcpServer::kReusePort));
    m_server->setConnectionCallback(std::bind(&MonitorServer::onConnected, this, std::placeholders::_1));
    //启动侦听
    m_server->start(1);

    return true;
}

void MonitorServer::uninit() {
    if (m_server)
        m_server->stop();
}

//新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
void MonitorServer::onConnected(std::shared_ptr<TcpConnection> conn) {
    if (conn->connected()) {
        std::shared_ptr<MonitorSession> spSession(new MonitorSession(conn));
        conn->setMessageCallback(
                std::bind(&MonitorSession::onRead, spSession.get(), std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3));

        {
            std::lock_guard<std::mutex> guard(m_sessionMutex);
            m_sessions.push_back(spSession);
        }

        spSession->showHelp();
    } else {
        onDisconnected(conn);
    }
}

//连接断开
void MonitorServer::onDisconnected(const std::shared_ptr<TcpConnection> &conn) {
    //TODO: 这样的代码逻辑太混乱，需要优化
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter) {
        if ((*iter)->getConnectionPtr() == nullptr) {
            LOG_ERROR("connection is NULL");
            break;
        }

        //通过比对connection对象找到对应的session
        if ((*iter)->getConnectionPtr() == conn) {
            m_sessions.erase(iter);
            LOG_INFO("monitor client disconnected:{}", conn->peerAddress().toIpPort());
            break;
        }
    }
}

bool MonitorServer::isMonitorTokenValid(const char *token) {
    if (token == nullptr || token[0] == '\0')
        return false;

    return m_token == token;
}
