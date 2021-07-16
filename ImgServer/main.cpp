#include <cstdlib>
#include <iostream>

#include "base/ConfigFileReader.h"
#include "base/Logger.h"
#include "base/Platform.h"
#include "net/EventLoop.h"
#include "utils/DaemonRun.h"

#include "FileServer/FileManager.h"
#include "FileServer/FileServer.h"

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

    if (!Logger::getMe().init("ImgServer", "log/ImgServer")) {
        std::cout << "ImgServer Logger init failed" << std::endl;
        return EXIT_FAILURE;
    }

    EventLoop mainLoop;

    CConfigFileReader config("etc/imgserver.conf");

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

    const char *filecachedir = config.getConfigName("imgcachedir");
    FileManager::getMe().init(filecachedir);

    const char *listenip = config.getConfigName("listenip");
    auto listenport = (short) atol(config.getConfigName("listenport"));
    FileServer::getMe().init(listenip, listenport, &mainLoop, filecachedir);

    LOG_INFO("imgserver initialization complete, now you can use client to connect it.");

    mainLoop.loop();

    LOG_INFO("exit imgserver.");

    return 0;
}
