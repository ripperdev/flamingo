#include "TcpServer.h"

#include <memory>

#include "../base/AsyncLog.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

using namespace net;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     std::string nameArg,
                     Option option)
        : loop_(loop),
          hostport_(listenAddr.toIpPort()),
          name_(std::move(nameArg)),
          acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
        //threadPool_(new EventLoopThreadPool(loop, name_)),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          started_(0),
          nextConnId_(1) {
    acceptor_->setNewConnectionCallback([this](int fd, InetAddress address) {
        newConnection(fd, address);
    });
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    LOGD("TcpServer::~TcpServer [%s] destructing", name_.c_str());

    stop();
}

//void TcpServer::setThreadNum(int numThreads)
//{
//  assert(0 <= numThreads);
//  threadPool_->setThreadNum(numThreads);
//}

void TcpServer::start(int workerThreadCount/* = 4*/) {
    if (started_ == 0) {
        eventLoopThreadPool_ = std::make_unique<EventLoopThreadPool>();
        eventLoopThreadPool_->init(loop_, workerThreadCount);
        eventLoopThreadPool_->start();

        //threadPool_->start(threadInitCallback_);
        //assert(!acceptor_->listenning());
        loop_->runInLoop([acceptor = acceptor_.get()] { acceptor->listen(); });
        started_ = 1;
    }
}

void TcpServer::stop() {
    if (started_ == 0)
        return;

    for (auto &connection : connections_) {
        TcpConnectionPtr conn = connection.second;
        connection.second.reset();
        conn->getLoop()->runInLoop([conn] { conn->connectDestroyed(); });
        conn.reset();
    }

    eventLoopThreadPool_->stop();

    started_ = 0;
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    loop_->assertInLoopThread();
    EventLoop *ioLoop = eventLoopThreadPool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOGD("TcpServer::newConnection [%s] - new connection [%s] from %s", name_.c_str(), connName.c_str(),
         peerAddr.toIpPort().c_str());

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr &connection) { removeConnection(connection); });
    //该线程分离完io事件后，立即调用TcpConnection::connectEstablished
    ioLoop->runInLoop([conn] { conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    // FIXME: unsafe
    loop_->runInLoop([this, conn] { removeConnectionInLoop(conn); });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    LOGD("TcpServer::removeConnectionInLoop [%s] - connection %s", name_.c_str(), conn->name().c_str());
    size_t n = connections_.erase(conn->name());
    //(void)n;
    //assert(n == 1);
    if (n != 1) {
        //出现这种情况，是TcpConneaction对象在创建过程中，对方就断开连接了。
        LOGD("TcpServer::removeConnectionInLoop [%s] - connection %s, connection does not exist.", name_.c_str(),
             conn->name().c_str());
        return;
    }

    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop([conn] { conn->connectDestroyed(); });
}
