/**
 *  文件服务程序入口函数
 *  zhangyl 2017.03.09
 **/
#include <bits/getopt_core.h>       // for getopt
#include <signal.h>                 // for signal, SIGCHLD, SIGINT, SIGPIPE
#include <cstdlib>                  // for atol, EXIT_FAILURE
#include <iostream>                 // for operator<<, endl, basic_ostream
#include <memory>                   // for allocator

#include "FileManager.h"            // for FileManager
#include "FileServer.h"             // for FileServer
#include "base/ConfigFileReader.h"  // for CConfigFileReader
#include "base/Logger.h"            // for LOG_INFO, Logger
#include "net/Callbacks.h"          // for net
#include "net/EventLoop.h"          // for EventLoop
#include "utils/DaemonRun.h"        // for daemon_run

using namespace net;


void prog_exit(int signo) {
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    FileServer::getMe().uninit();
}

int main(int argc, char *argv[]) {
    //设置信号处理
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, prog_exit);
    signal(SIGTERM, prog_exit);

    if (!Logger::getMe().init("FileServer", "log/FileServer")) {
        std::cout << "FileServer Logger init failed" << std::endl;
        return EXIT_FAILURE;
    }

    EventLoop mainLoop;

    CConfigFileReader config("etc/fileserver.conf");

    int ch;
    bool bdaemon = false;
    while ((ch = getopt(argc, argv, "d")) != -1) {
        switch (ch) {
            case 'd':
                bdaemon = true;
                break;
        }
    }

    if (bdaemon)
        daemon_run();

    const char *filecachedir = config.getConfigName("filecachedir");
    FileManager::getMe().init(filecachedir);

    const char *listenip = config.getConfigName("listenip");
    auto listenport = (short) atol(config.getConfigName("listenport"));
    FileServer::getMe().init(listenip, listenport, &mainLoop, filecachedir);

    LOG_INFO("fileserver initialization completed, now you can use client to connect it.");

    mainLoop.loop();

    LOG_INFO("exit fileserver.");

    return 0;
}
