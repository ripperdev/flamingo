#include "Connector.h"
#include <sstream>
#include <cstring>
#include "Channel.h"
#include "EventLoop.h"
#include "base/Logger.h"

using namespace net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop),
          serverAddr_(serverAddr),
          connect_(false),
          state_(kDisconnected),
          retryDelayMs_(kInitRetryDelayMs) {}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop([this] { startInLoop(); });
}

void Connector::startInLoop() {
    loop_->assertInLoopThread();
    //assert(state_ == kDisconnected);
    if (state_ != kDisconnected)
        return;

    if (connect_) {
        connect();
    } else {
        LOG_DEBUG("do not connect");
    }
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop([_this = shared_from_this()] { _this->stopInLoop(); });
    // FIXME: cancel timer
}

void Connector::stopInLoop() {
    //std::stringstream ss;	
    //ss << "stopInLoop eventloop threadid = " << std::this_thread::get_id();
    //std::cout << ss.str() << std::endl;

    loop_->assertInLoopThread();
    if (state_ == kConnecting) {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect() {
    int sockfd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());

    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR("connect error in Connector::startInLoop, {}", savedErrno);
            sockets::close(sockfd);
            break;

        default:
            LOG_ERROR("Unexpected error in Connector::startInLoop, {}", savedErrno);
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart() {
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd) {
    setState(kConnecting);
    //assert(!channel_);
    channel_ = std::make_unique<Channel>(loop_, sockfd);
    channel_->setWriteCallback([this] { handleWrite(); }); // FIXME: unsafe
    channel_->setErrorCallback([this] { handleError(); }); // FIXME: unsafe

    // channel_->tie(shared_from_this()); is not working,
    // as channel_ is not managed by shared_ptr
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop([_this = shared_from_this()] { _this->resetChannel(); });
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    LOG_DEBUG("Connector::handleWrite {}", state_);

    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err) {
            LOG_WARN("Connector::handleWrite - SO_ERROR = {} {}", err, strerror(err));
            retry(sockfd);
        } else if (sockets::isSelfConnect(sockfd)) {
            LOG_WARN("Connector::handleWrite - Self connect");
            retry(sockfd);
        } else {
            setState(kConnected);
            if (connect_) {
                //newConnectionCallback_??????TcpClient::newConnection(int sockfd)
                newConnectionCallback_(sockfd);
            } else {
                sockets::close(sockfd);
            }
        }
    } else {
        // what happened?
        //assert(state_ == kDisconnected);
        if (state_ != kDisconnected)
            LOG_ERROR("state_ != kDisconnected");
    }
}

void Connector::handleError() {
    LOG_ERROR("Connector::handleError state={}", state_);
    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_DEBUG("SO_ERROR = {} {}", err, strerror(err));
        LOG_ERROR("Connector::handleError state={}", state_);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_) {
        LOG_INFO("Connector::retry - Retry connecting to {} in {}  milliseconds.", serverAddr_.toIpPort(),
             retryDelayMs_);
        //loop_->runAfter(retryDelayMs_/1000.0,
        //                std::bind(&Connector::startInLoop, shared_from_this()));
        //retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
        //?????????????????? todo
    } else {
        LOG_DEBUG("do not connect");
    }
}
