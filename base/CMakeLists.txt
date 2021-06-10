set(BASE_SRC
        AsyncLog.cpp
        ConfigFileReader.cpp
        CountDownLatch.cpp
        DatabaseMysql.cpp
        Field.cpp
        MysqlManager.cpp
        MysqlThrd.cpp
        MysqlThrdMgr.cpp
        Platform.cpp
        QueryResult.cpp
        TaskList.cpp
        Timestamp.cpp)

add_library(base ${BASE_SRC})
target_link_libraries(base mysqlclient)
