#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <functional>

namespace net {

    class EventLoop;

    class EventLoopThread {
    public:
        typedef std::function<void(EventLoop *)> ThreadInitCallback;

        explicit EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string &name = "");

        ~EventLoopThread();

        EventLoop *startLoop();

        void stopLoop();

    private:
        void threadFunc();

        EventLoop *loop_;
        bool exiting_;
        std::unique_ptr<std::thread> thread_;
        std::mutex mutex_;
        std::condition_variable cond_;
        ThreadInitCallback callback_;
    };
}
