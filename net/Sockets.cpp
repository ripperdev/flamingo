#include "Sockets.h"

#include <cstdio>
#include <cstring>

#include "base/Logger.h"
#include "InetAddress.h"
#include "Endian.h"
#include "Callbacks.h"

using namespace net;

Socket::~Socket() {
    sockets::close(sockfd_);
}

//bool Socket::getTcpInfo(struct tcp_info* tcpi) const
//{
//	//socklen_t len = sizeof(*tcpi);
//	//memset(tcpi, 0, len);
//	//return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
//    return false;
//}

bool Socket::getTcpInfoString(char *buf, int len) {
    //struct tcp_info tcpi;
    //bool ok = getTcpInfo(&tcpi);
    //if (ok)
    //{
    //	snprintf(buf, len, "unrecovered=%u "
    //		"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
    //		"lost=%u retrans=%u rtt=%u rttvar=%u "
    //		"sshthresh=%u cwnd=%u total_retrans=%u",
    //		tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
    //		tcpi.tcpi_rto,          // Retransmit timeout in usec
    //		tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
    //		tcpi.tcpi_snd_mss,
    //		tcpi.tcpi_rcv_mss,
    //		tcpi.tcpi_lost,         // Lost packets
    //		tcpi.tcpi_retrans,      // Retransmitted packets out
    //		tcpi.tcpi_rtt,          // Smoothed round trip time in usec
    //		tcpi.tcpi_rttvar,       // Medium deviation
    //		tcpi.tcpi_snd_ssthresh,
    //		tcpi.tcpi_snd_cwnd,
    //		tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    //}
    //return ok;
    return false;
}

void Socket::bindAddress(const InetAddress &addr) const {
    sockets::bindOrDie(sockfd_, addr.getSockAddrInet());
}

void Socket::listen() const {
    sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress *peeraddr) const {
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof addr);
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0) {
        peeraddr->setSockAddrInet(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() const {
    sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on) const {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void Socket::setReuseAddr(bool on) const {
    sockets::setReuseAddr(sockfd_, on);
}

void Socket::setReusePort(bool on) const {
    sockets::setReusePort(sockfd_, on);
}

void Socket::setKeepAlive(bool on) const {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

//namespace
//{
//	//typedef struct sockaddr SA;
//
//	
//
//}

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in *addr) {
    return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr));
}

struct sockaddr *sockets::sockaddr_cast(struct sockaddr_in *addr) {
    return static_cast<struct sockaddr *>(implicit_cast<void *>(addr));
}

const struct sockaddr_in *sockets::sockaddr_in_cast(const struct sockaddr *addr) {
    return static_cast<const struct sockaddr_in *>(implicit_cast<const void *>(addr));
}

struct sockaddr_in *sockets::sockaddr_in_cast(struct sockaddr *addr) {
    return static_cast<struct sockaddr_in *>(implicit_cast<void *>(addr));
}

SOCKET sockets::createOrDie() {
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_CRITICAL("sockets::createNonblockingOrDie");
    }
    return sockfd;
}

SOCKET sockets::createNonblockingOrDie() {
    SOCKET sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_CRITICAL("sockets::createNonblockingOrDie");
    }

    setNonBlockAndCloseOnExec(sockfd);
    return sockfd;
}

void sockets::setNonBlockAndCloseOnExec(SOCKET sockfd) {
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    // FIXME check

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    // FIXME check

    (void) ret;
}

void sockets::bindOrDie(SOCKET sockfd, const struct sockaddr_in &addr) {
    int ret = ::bind(sockfd, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
    if (ret < 0) {
        LOG_CRITICAL("sockets::bindOrDie");
    }
}

void sockets::listenOrDie(SOCKET sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_CRITICAL("sockets::listenOrDie");
    }
}

SOCKET sockets::accept(SOCKET sockfd, struct sockaddr_in *addr) {
    auto addrlen = static_cast<socklen_t>(sizeof *addr);
    SOCKET connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        int savedErrno = errno;
        LOG_ERROR("Socket::accept");
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG_CRITICAL("unexpected error of ::accept %d", savedErrno);
                break;
            default:
                LOG_CRITICAL("unknown error of ::accept %d", savedErrno);
                break;
        }
    }
    return connfd;
}

void sockets::setReuseAddr(SOCKET sockfd, bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void sockets::setReusePort(SOCKET sockfd, bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on) {
        LOG_ERROR("SO_REUSEPORT failed.");
    }
}

SOCKET sockets::connect(SOCKET sockfd, const struct sockaddr_in &addr) {
    return ::connect(sockfd, sockaddr_cast(&addr), static_cast<socklen_t>(sizeof addr));
}

int32_t sockets::read(SOCKET sockfd, void *buf, int32_t count) {
    return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(SOCKET sockfd, const struct iovec *iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

int32_t sockets::write(SOCKET sockfd, const void *buf, int32_t count) {
    return ::write(sockfd, buf, count);
}

void sockets::close(SOCKET sockfd) {
    if (::close(sockfd) < 0) {
        LOG_ERROR("sockets::close, fd=%d, errno=%d, errorinfo=%s", sockfd, errno, strerror(errno));
    }
}

void sockets::shutdownWrite(SOCKET sockfd) {
    if (::shutdown(sockfd, SHUT_WR) < 0) {
        LOG_ERROR("sockets::shutdownWrite");
    }
}

void sockets::toIpPort(char *buf, size_t size, const struct sockaddr_in &addr) {
    //if (size >= sizeof(struct sockaddr_in))
    //    return;

    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
    size_t end = ::strlen(buf);
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    //if (size > end)
    //    return;

    snprintf(buf + end, size - end, ":%u", port);
}

void sockets::toIp(char *buf, size_t size, const struct sockaddr_in &addr) {
    if (size >= sizeof(struct sockaddr_in))
        return;

    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

void sockets::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr) {
    addr->sin_family = AF_INET;
    //TODO: 校验下写的对不对
    addr->sin_port = htobe16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        LOG_ERROR("sockets::fromIpPort");
    }
}

int sockets::getSocketError(SOCKET sockfd) {
    int optval;
    auto optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        return errno;
    return optval;
}

struct sockaddr_in sockets::getLocalAddr(SOCKET sockfd) {
    struct sockaddr_in localaddr = {0};
    memset(&localaddr, 0, sizeof localaddr);
    auto addrlen = static_cast<socklen_t>(sizeof localaddr);
    ::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen);
    //if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    //{
    //	LOG_SYSERR << "sockets::getLocalAddr";
    //  return 
    //}
    return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(SOCKET sockfd) {
    struct sockaddr_in peeraddr = {0};
    memset(&peeraddr, 0, sizeof peeraddr);
    auto addrlen = static_cast<socklen_t>(sizeof peeraddr);
    ::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen);
    //if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
    //{
    //	LOG_SYSERR << "sockets::getPeerAddr";
    //}
    return peeraddr;
}

bool sockets::isSelfConnect(SOCKET sockfd) {
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_port == peeraddr.sin_port && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}
