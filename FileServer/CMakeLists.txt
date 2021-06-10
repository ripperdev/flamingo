set(FILE_SRC FileManager.cpp
        FileServer.cpp
        FileSession.cpp
        main.cpp
        TcpSession.cpp)

add_executable(FileServer ${FILE_SRC})
target_link_libraries(FileServer net util)
