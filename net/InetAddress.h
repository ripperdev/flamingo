#pragma once

#include <netinet/in.h>  // for sockaddr_in, in_addr
#include <stdint.h>      // for uint16_t, uint32_t
#include <string>        // for string

namespace net {
    ///
    /// Wrapper of sockaddr_in.
    ///
    /// This is an POD interface class.
    class InetAddress {
    public:
        /// Constructs an endpoint with given port number.
        /// Mostly used in TcpServer listening.
        explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);

        /// Constructs an endpoint with given ip and port.
        /// @c ip should be "1.2.3.4"
        InetAddress(const std::string &ip, uint16_t port);

        /// Constructs an endpoint with given struct @c sockaddr_in
        /// Mostly used when accepting new connections
        explicit InetAddress(const struct sockaddr_in &addr)
                : addr_(addr) {}

        [[nodiscard]] std::string toIp() const;

        [[nodiscard]] std::string toIpPort() const;

        [[nodiscard]] uint16_t toPort() const;

        // default copy/assignment are Okay

        [[nodiscard]] const struct sockaddr_in &getSockAddrInet() const { return addr_; }

        void setSockAddrInet(const struct sockaddr_in &addr) { addr_ = addr; }

        [[nodiscard]] uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }

        [[nodiscard]] uint16_t portNetEndian() const { return addr_.sin_port; }

        // resolve hostname to IP address, not changing port or sin_family
        // return true on success.
        // thread safe
        static bool resolve(const std::string &hostname, InetAddress *result);
        // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

    private:
        struct sockaddr_in addr_{};
    };
}
