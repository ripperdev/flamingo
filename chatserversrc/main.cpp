/**
 *  聊天服务程序入口函数
 *  zhangyl 2017.03.09
 **/
#include <iostream>
#include <cstdlib>

#include "../base/Platform.h"
#include "../base/Singleton.h"
#include "../base/ConfigFileReader.h"
#include "../base/AsyncLog.h"
#include "../net/EventLoop.h"
#include "../net/EventLoopThreadPool.h"
#include "../mysqlmgr/MysqlManager.h"

#include <cstring>
#include "../utils/DaemonRun.h"

#include "UserManager.h"
#include "ChatServer.h"
#include "MonitorServer.h"
#include "HttpServer.h"

using namespace net;

#ifdef WIN32
//初始化Windows socket库
NetworkInitializer windowsNetworkInitializer;
#endif

EventLoop g_mainLoop;

void prog_exit(int signo) {
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    Singleton<MonitorServer>::Instance().uninit();
    Singleton<HttpServer>::Instance().uninit();
    Singleton<ChatServer>::Instance().uninit();
    g_mainLoop.quit();

    CAsyncLog::uninit();
}

int main(int argc, char *argv[]) {
    //设置信号处理
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, prog_exit);
    signal(SIGTERM, prog_exit);

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

    CConfigFileReader config("etc/chatserver.conf");

    const char *logbinarypackage = config.getConfigName("logbinarypackage");
    if (logbinarypackage != nullptr) {
        int logbinarypackageint = atoi(logbinarypackage);
        if (logbinarypackageint != 0)
            Singleton<ChatServer>::Instance().enableLogPackageBinary(true);
        else
            Singleton<ChatServer>::Instance().enableLogPackageBinary(false);
    }

    std::string logFileFullPath;

    const char *logfilepath = config.getConfigName("logfiledir");
    if (logfilepath == nullptr) {
        LOGF("logdir is not set in config file");
        return 1;
    }

    //如果log目录不存在则创建之
    DIR *dp = opendir(logfilepath);
    if (dp == nullptr) {
        if (mkdir(logfilepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
            LOGF("create base dir error, %s , errno: %d, %s", logfilepath, errno, strerror(errno));
            return 1;
        }
    }
    closedir(dp);

    logFileFullPath = logfilepath;

    const char *logfilename = config.getConfigName("logfilename");
    logFileFullPath += logfilename;

    CAsyncLog::init(logFileFullPath.c_str());

    //初始化数据库配置
    const char *dbserver = config.getConfigName("dbserver");
    const char *dbuser = config.getConfigName("dbuser");
    const char *dbpassword = config.getConfigName("dbpassword");
    const char *dbname = config.getConfigName("dbname");
    if (!Singleton<CMysqlManager>::Instance().init(dbserver, dbuser, dbpassword, dbname)) {
        LOGF("Init mysql failed, please check your database config..............");
    }

    if (!Singleton<UserManager>::Instance().init(dbserver, dbuser, dbpassword, dbname)) {
        LOGF("Init UserManager failed, please check your database config..............");
    }

    const char *listenip = config.getConfigName("listenip");
    auto listenport = (short) atol(config.getConfigName("listenport"));
    Singleton<ChatServer>::Instance().init(listenip, listenport, &g_mainLoop);

    const char *monitorlistenip = config.getConfigName("monitorlistenip");
    auto monitorlistenport = (short) atol(config.getConfigName("monitorlistenport"));
    const char *monitortoken = config.getConfigName("monitortoken");
    Singleton<MonitorServer>::Instance().init(monitorlistenip, monitorlistenport, &g_mainLoop, monitortoken);

    const char *httplistenip = config.getConfigName("monitorlistenip");
    auto httplistenport = (short) atol(config.getConfigName("httplistenport"));
    Singleton<HttpServer>::Instance().init(httplistenip, httplistenport, &g_mainLoop);

    LOGI("chatserver initialization completed, now you can use client to connect it.");

    g_mainLoop.loop();

    LOGI("exit chatserver.");

    return 0;
}
