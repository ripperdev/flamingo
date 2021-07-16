#pragma once

#include <netinet/in.h>        // for sockaddr_in
#include <stddef.h>            // for size_t
#include <sys/types.h>         // for ssize_t
#include <cstdint>             // for int32_t, uint16_t

#include "../base/Platform.h"  // for SOCKET

namespace net {
    class InetAddress;

    ///
    /// Wrapper of socket file descriptor.
    ///
    /// It closes the sockfd when desctructs.
    /// It's thread safe, all operations are delagated to OS.
    class Socket {
    public:
        explicit Socket(int sockfd) : sockfd_(sockfd) {}

        // Socket(Socket&&) // move constructor in C++11
        ~Socket();

        [[nodiscard]] SOCKET fd() const { return sockfd_; }

        // return true if success.
        //bool getTcpInfo(struct tcp_info*) const;
        static bool getTcpInfoString(char *buf, int len) ;

        /// abort if address in use
        void bindAddress(const InetAddress &localaddr) const;

        /// abort if address in use
        void listen() const;

        /// On success, returns a non-negative integer that is
        /// a descriptor for the accepted socket, which has been
        /// set to non-blocking and close-on-exec. *peeraddr is assigned.
        /// On error, -1 is returned, and *peeraddr is untouched.
        int accept(InetAddress *peeraddr) const;

        void shutdownWrite() const;

        ///
        /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        ///
        void setTcpNoDelay(bool on) const;

        ///
        /// Enable/disable SO_REUSEADDR
        ///
        void setReuseAddr(bool on) const;

        ///
        /// Enable/disable SO_REUSEPORT
        ///
        void setReusePort(bool on) const;

        ///
        /// Enable/disable SO_KEEPALIVE
        ///
        void setKeepAlive(bool on) const;

    private:
        const SOCKET sockfd_;
    };

    namespace sockets {
        ///
        /// Creates a socket file descriptor,
        /// abort if any error.
        SOCKET createOrDie();

        SOCKET createNonblockingOrDie();

        void setNonBlockAndCloseOnExec(SOCKET sockfd);

        void setReuseAddr(SOCKET sockfd, bool on);

        void setReusePort(SOCKET sockfd, bool on);

        SOCKET connect(SOCKET sockfd, const struct sockaddr_in &addr);

        void bindOrDie(SOCKET sockfd, const struct sockaddr_in &addr);

        void listenOrDie(SOCKET sockfd);

        SOCKET accept(SOCKET sockfd, struct sockaddr_in *addr);

        int32_t read(SOCKET sockfd, void *buf, int32_t count);

        ssize_t readv(SOCKET sockfd, const struct iovec *iov, int iovcnt);

        int32_t write(SOCKET sockfd, const void *buf, int32_t count);

        void close(SOCKET sockfd);

        void shutdownWrite(SOCKET sockfd);

        void toIpPort(char *buf, size_t size, const struct sockaddr_in &addr);

        void toIp(char *buf, size_t size, const struct sockaddr_in &addr);

        void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);

        int getSocketError(SOCKET sockfd);

        const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr);

        struct sockaddr *sockaddr_cast(struct sockaddr_in *addr);

        const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr);

        struct sockaddr_in *sockaddr_in_cast(struct sockaddr *addr);

        struct sockaddr_in getLocalAddr(SOCKET sockfd);

        struct sockaddr_in getPeerAddr(SOCKET sockfd);

        bool isSelfConnect(SOCKET sockfd);
    }
}
