#pragma once

#include <condition_variable>  // for condition_variable
#include <functional>          // for function
#include <memory>              // for unique_ptr
#include <mutex>               // for mutex
#include <string>              // for allocator, string
#include <thread>              // for thread

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
