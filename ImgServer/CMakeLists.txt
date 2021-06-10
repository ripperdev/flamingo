include_directories(${PROJECT_SOURCE_DIR})
set(IMG_SRC main.cpp
        ../FileServer/FileManager.cpp
        ../FileServer/FileServer.cpp
        ../FileServer/FileSession.cpp
        ../FileServer/TcpSession.cpp)

add_executable(ImgServer ${IMG_SRC})
target_link_libraries(ImgServer net util)
