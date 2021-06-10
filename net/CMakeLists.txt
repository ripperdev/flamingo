set(NET_SRC Acceptor.cpp
        Buffer.cpp
        Channel.cpp
        Connector.cpp
        EpollPoller.cpp
        EventLoop.cpp
        EventLoopThread.cpp
        EventLoopThreadPool.cpp
        InetAddress.cpp
        PollPoller.cpp
        ProtocolStream.cpp
        SelectPoller.cpp
        Sockets.cpp
        TcpClient.cpp
        TcpConnection.cpp
        TcpServer.cpp
        Timer.cpp
        TimerQueue.cpp)

add_library(net ${NET_SRC})
target_link_libraries(net base)
