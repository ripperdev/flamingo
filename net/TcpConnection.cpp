#include "TcpConnection.h"

#include <thread>
#include <sstream>
#include <utility>

#include "base/Logger.h"

#include "Channel.h"
#include "EventLoop.h"

using namespace net;

void net::defaultConnectionCallback(const TcpConnectionPtr &conn) {
    LOG_DEBUG("{} -> is {}",
              conn->localAddress().toIpPort().c_str(),
              conn->peerAddress().toIpPort().c_str(),
              (conn->connected() ? "UP" : "DOWN"));
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void net::defaultMessageCallback(const TcpConnectionPtr &, Buffer *buf, Timestamp) {
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, string nameArg, int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
        : loop_(loop),
          name_(std::move(nameArg)),
          state_(kConnecting),
          socket_(new Socket(sockfd)),
          channel_(new Channel(loop, sockfd)),
          localAddr_(localAddr),
          peerAddr_(peerAddr),
          highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback([this](Timestamp timestamp) { handleRead(timestamp); });
    channel_->setWriteCallback([this] { handleWrite(); });
    channel_->setCloseCallback([this] { handleClose(); });
    channel_->setErrorCallback([this] { handleError(); });
    LOG_DEBUG("TcpConnection::ctor[{}] at {} fd={}", name_.c_str(), (void*)this, sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection::dtor[{}] at {} fd={} state={}",
         name_.c_str(), (void*)this, channel_->fd(), stateToString());
    //assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) {
    //return socket_->getTcpInfo(tcpi);
    return false;
}

string TcpConnection::getTcpInfoString() const {
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

void TcpConnection::send(const void *data, int len) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(data, len);
        } else {
            string message(static_cast<const char *>(data), len);
            loop_->runInLoop([this, message] { sendInLoop(message); });
        }
    }
}

void TcpConnection::send(const string &message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop([this, &message] { sendInLoop(message); });
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::send(Buffer *buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        } else {
            loop_->runInLoop([this, &buf] { sendInLoop(buf->retrieveAllAsString()); });
        }
    }
}

void TcpConnection::sendInLoop(const string &message) {
    sendInLoop(message.c_str(), message.size());
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
    loop_->assertInLoopThread();
    int32_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected) {
        LOG_WARN("disconnected, give up writing");
        return;
    }
    // if no thing in output queue, try writing directly
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        //TODO: ??????threadid???????????????????????????
        //std::stringstream ss;
        //ss << std::this_thread::get_id();
        //LOGI << "send data in threadID = " << ss;

        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop([this] { writeCompleteCallback_(shared_from_this()); });
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    //assert(remaining <= len);
    if (remaining > len)
        return;

    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_) {
            loop_->runInLoop([this, oldLen, remaining] {
                highWaterMarkCallback_(shared_from_this(), oldLen + remaining);
            });
        }
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    // FIXME: use compare and swap
    if (state_ == kConnected) {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop([this] { shutdownInLoop(); });
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        // we are not writing
        socket_->shutdownWrite();
    }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected)
//   {
//     setState(kDisconnecting);
//     loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting())
//   {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::forceClose() {
    // FIXME: use compare and swap
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop([this] { forceCloseInLoop(); });
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char *TcpConnection::stateToString() const {
    switch (state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    if (state_ != kConnecting) {
        //???????????????????????????
        return;
    }

    setState(kConnected);
    channel_->tie(shared_from_this());

    //?????????????????????????????????????????????????????????
    if (!channel_->enableReading()) {
        LOG_ERROR("enableReading failed.");
        //setState(kDisconnected);
        handleClose();
        return;
    }

    //connectionCallback_??????void XXServer::OnConnection(const std::shared_ptr<TcpConnection>& conn)
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    int32_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        //messageCallback_??????CTcpSession::OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receiveTime)
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        int32_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop([this] { writeCompleteCallback_(shared_from_this()); });
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite");
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
            //added by zhangyl 2019.05.06
            handleClose();
        }
    } else {
        LOG_DEBUG("Connection fd = {}  is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose() {
    //???Linux????????????????????????????????????????????????handleError???handleClose
    //????????????????????????????????????????????????????????????
    //??????????????????????????????
    if (state_ == kDisconnected)
        return;

    loop_->assertInLoopThread();
    LOG_DEBUG("fd = {}  state = {}", channel_->fd(), stateToString());
    //assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    // must be the last line
    closeCallback_(guardThis);

    //???????????????????????????????????????socket fd???TcpConnection?????????????????????
    //if (socket_)
    //{
    //    sockets::close(socket_->fd());
    //}
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR("TcpConnection::{} handleError [{}] - SO_ERROR = {}", name_.c_str(), err, strerror(err));

    //??????handleClose()?????????????????????Channel???fd
    handleClose();
}
