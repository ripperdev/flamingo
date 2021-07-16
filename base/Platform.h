#pragma once

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

#ifdef WIN32

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

//remove warning C4996 on Windows
//#define _CRT_SECURE_NO_WARNINGS

typedef int          socklen_t;
//typedef uint64_t     ssize_t;
typedef unsigned int in_addr_t;

//Windows 上没有这些结构的定义，为了移植方便，手动定义这些结构
#define  XPOLLIN         1
#define  XPOLLPRI        2
#define  XPOLLOUT        4
#define  XPOLLERR        8 
#define  XPOLLHUP        16
#define  XPOLLNVAL       32
#define  XPOLLRDHUP      8192

#define  XEPOLL_CTL_ADD  1
#define  XEPOLL_CTL_DEL  2
#define  XEPOLL_CTL_MOD  3

#pragma pack(push, 1)
typedef union epoll_data {
    void*    ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;    /* Epoll events */
    epoll_data_t data;      /* User data variable */
};
#pragma pack(pop)

#include <Windows.h>
#include <Ws2ipdef.h>
#include <fcntl.h>  //for O_BINARY
#include <io.h>     //_pipe
#include <shlwapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>

class NetworkInitializer
{
public:
    NetworkInitializer();
    ~NetworkInitializer();
};

#else

typedef int SOCKET;

#define SOCKET_ERROR -1

#define closesocket(s) close(s)


#include <endian.h>     // for be64toh, htobe64
#include <poll.h>       // for POLLERR, POLLHUP, POLLIN, POLLNVAL, POLLOUT
#include <sys/epoll.h>  // for EPOLL_CTL_ADD, EPOLL_CTL_DEL, EPOLL_CTL_MOD

//for ubuntu readv not found
#ifdef __UBUNTU
#include <sys/uio.h>
#endif


#define  XPOLLIN         POLLIN
#define  XPOLLPRI        POLLPRI
#define  XPOLLOUT        POLLOUT
#define  XPOLLERR        POLLERR
#define  XPOLLHUP        POLLHUP
#define  XPOLLNVAL       POLLNVAL
#define  XPOLLRDHUP      POLLRDHUP

#define  XEPOLL_CTL_ADD  EPOLL_CTL_ADD
#define  XEPOLL_CTL_DEL  EPOLL_CTL_DEL
#define  XEPOLL_CTL_MOD  EPOLL_CTL_MOD

//Linux下没有这两个函数，定义之
#define ntohll(x) be64toh(x)
#define htonll(x) htobe64(x)

#endif
