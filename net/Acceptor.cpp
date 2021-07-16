#include "Acceptor.h"

#include <errno.h>           // for EMFILE, errno
#include <fcntl.h>           // for open, O_CLOEXEC, O_RDONLY
#include <sys/socket.h>      // for accept
#include <unistd.h>          // for close
#include <string>            // for string

#include "EventLoop.h"       // for EventLoop
#include "InetAddress.h"     // for InetAddress
#include "base/Logger.h"     // for LOG_DEBUG, LOG_ERROR
#include "base/Timestamp.h"  // for Timestamp
#include "net/Channel.h"     // for Channel, net
#include "net/Sockets.h"     // for Socket, close, createNonblockingOrDie

using namespace net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
        : loop_(loop),
          acceptSocket_(sockets::createNonblockingOrDie()),
          acceptChannel_(loop, acceptSocket_.fd()),
          listenning_(false) {
    idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);

    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback([this](Timestamp) { handleRead(); });
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    //FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        string hostport = peerAddr.toIpPort();
        LOG_DEBUG("Accepts of {}", hostport.c_str());
        //newConnectionCallback_实际指向TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            sockets::close(connfd);
        }
    } else {
        LOG_ERROR("in Acceptor::handleRead");
        /*
        The special problem of accept()ing when you can't

        Many implementations of the POSIX accept function (for example, found in post-2004 Linux) 
        have the peculiar behaviour of not removing a connection from the pending queue in all error cases.

        For example, larger servers often run out of file descriptors (because of resource limits),
        causing accept to fail with ENFILE but not rejecting the connection, leading to libev signalling
        readiness on the next iteration again (the connection still exists after all), and typically 
        causing the program to loop at 100% CPU usage.

        Unfortunately, the set of errors that cause this issue differs between operating systems,
        there is usually little the app can do to remedy the situation, and no known thread-safe
        method of removing the connection to cope with overload is known (to me).

        One of the easiest ways to handle this situation is to just ignore it - when the program encounters
        an overload, it will just loop until the situation is over. While this is a form of busy waiting,
        no OS offers an event-based way to handle this situation, so it's the best one can do.

        A better way to handle the situation is to log any errors other than EAGAIN and EWOULDBLOCK, 
        making sure not to flood the log with such messages, and continue as usual, which at least gives
        the user an idea of what could be wrong ("raise the ulimit!"). For extra points one could 
        stop the ev_io watcher on the listening fd "for a while", which reduces CPU usage.

        If your program is single-threaded, then you could also keep a dummy file descriptor for overload
        situations (e.g. by opening /dev/null), and when you run into ENFILE or EMFILE, close it, 
        run accept, close that fd, and create a new dummy fd. This will gracefully refuse clients under
        typical overload conditions.

        The last way to handle it is to simply log the error and exit, as is often done with malloc
        failures, but this results in an easy opportunity for a DoS attack.
        */
        if (errno == EMFILE) {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
