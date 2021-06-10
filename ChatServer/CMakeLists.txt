include_directories(${PROJECT_SOURCE_DIR})

set(CHAT_SRC
        BussinessLogic.cpp
        ChatServer.cpp
        ChatSession.cpp
        HttpServer.cpp
        HttpSession.cpp
        main.cpp
        MonitorServer.cpp
        MonitorSession.cpp
        MsgCacheManager.cpp
        TcpSession.cpp
        UserManager.cpp)

add_executable(ChatServer ${CHAT_SRC})
target_link_libraries(ChatServer net util jsoncpp)
