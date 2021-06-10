set(UTIL_SRC
        DaemonRun.cpp
        MD5.cpp
        StringUtil.cpp
        URLEncodeUtil.cpp
        UUIDGenerator.cpp
        ZlibUtil.cpp)

add_library(util ${UTIL_SRC})
target_link_libraries(util z)
