#include <cstdlib>
#include <iostream>

#include "base/ConfigFileReader.h"
#include "base/Logger.h"
#include "base/MysqlManager.h"
#include "base/Platform.h"
#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "utils/DaemonRun.h"

#include "UserManager.h"
#include "ChatServer.h"
#include "MonitorServer.h"
#include "HttpServer.h"

using namespace net;


void prog_exit(int signo) {
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    MonitorServer::getMe().uninit();
    HttpServer::getMe().uninit();
    ChatServer::getMe().uninit();
}

int main(int argc, char *argv[]) {
    //设置信号处理
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, prog_exit);
    signal(SIGTERM, prog_exit);

    if (!Logger::getMe().init("ChatServer", "log/ChatServer")) {
        std::cout << "ChatServer Logger init failed" << std::endl;
        return EXIT_FAILURE;
    }

    EventLoop mainLoop;

    CConfigFileReader config("etc/chatserver.conf");

    int ch;
    bool bdaemon = false;
    while ((ch = getopt(argc, argv, "d")) != -1) {
        if (ch == 'd') {
            bdaemon = true;
            break;
        }
    }

    if (bdaemon)
        daemon_run();

    //初始化数据库配置
    const char *dbserver = config.getConfigName("dbserver");
    const char *dbuser = config.getConfigName("dbuser");
    const char *dbpassword = config.getConfigName("dbpassword");
    const char *dbname = config.getConfigName("dbname");
    if (!CMysqlManager::getMe().init(dbserver, dbuser, dbpassword, dbname)) {
        LOG_ERROR("Init mysql failed, please check your database config");
        return EXIT_FAILURE;
    }

    if (!UserManager::getMe().init(dbserver, dbuser, dbpassword, dbname)) {
        LOG_ERROR("Init UserManager failed, please check your database config");
        return EXIT_FAILURE;
    }

    const char *listenip = config.getConfigName("listenip");
    auto listenport = (short) atol(config.getConfigName("listenport"));
    ChatServer::getMe().init(listenip, listenport, &mainLoop);

    const char *monitorlistenip = config.getConfigName("monitorlistenip");
    auto monitorlistenport = (short) atol(config.getConfigName("monitorlistenport"));
    const char *monitortoken = config.getConfigName("monitortoken");
    MonitorServer::getMe().init(monitorlistenip, monitorlistenport, &mainLoop, monitortoken);

    const char *httplistenip = config.getConfigName("monitorlistenip");
    auto httplistenport = (short) atol(config.getConfigName("httplistenport"));
    HttpServer::getMe().init(httplistenip, httplistenport, &mainLoop);

    LOG_INFO("chatserver initialization completed, now you can use client to connect it.");

    mainLoop.loop();

    LOG_INFO("exit chatserver.");

    return 0;
}
