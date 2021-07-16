#include "FileServer.h"

#include "FileServer/FileSession.h"  // for FileSession
#include "base/Logger.h"             // for LOG_INFO, LOG_ERROR
#include "base/Timestamp.h"          // for Timestamp
#include "net/InetAddress.h"         // for InetAddress
#include "net/TcpConnection.h"       // for TcpConnection, TcpConnectionPtr

namespace net {
class Buffer;
class EventLoop;
}  // namespace net

bool FileServer::init(const char *ip, short port, EventLoop *loop, const char *fileBaseDir/* = "filecache/"*/) {
    m_strFileBaseDir = fileBaseDir;

    InetAddress addr(ip, port);
    m_server = std::make_unique<TcpServer>(loop, addr, "ZYL-MYImgAndFileServer", TcpServer::kReusePort);
    m_server->setConnectionCallback([this](const TcpConnectionPtr &conn) { onConnected(conn); });
    //启动侦听
    m_server->start(6);

    return true;
}

void FileServer::uninit() {
    if (m_server)
        m_server->stop();
}

void FileServer::onConnected(const std::shared_ptr<TcpConnection> &conn) {
    if (conn->connected()) {
        LOG_INFO("client connected: {}", conn->peerAddress().toIpPort().c_str());
        std::shared_ptr<FileSession> spSession(new FileSession(conn, m_strFileBaseDir.c_str()));
        conn->setMessageCallback([spSession](const TcpConnectionPtr &connPtr, Buffer *buffer, Timestamp timestamp) {
            spSession->onRead(connPtr, buffer, timestamp);
        });

        std::lock_guard<std::mutex> guard(m_sessionMutex);
        m_sessions.push_back(spSession);
    } else {
        onDisconnected(conn);
    }
}

void FileServer::onDisconnected(const std::shared_ptr<TcpConnection> &conn) {
    //TODO: 这样的代码逻辑太混乱，需要优化
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter) {
        if ((*iter)->getConnectionPtr() == nullptr) {
            LOG_ERROR("connection is NULL");
            break;
        }

        //用户下线
        m_sessions.erase(iter);
        //bUserOffline = true;
        LOG_INFO("client disconnected: {}", conn->peerAddress().toIpPort().c_str());
        break;
    }
}
