#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include "EventLoopThread.h"

namespace net {

    class EventLoop;

//    class EventLoopThread;

    class EventLoopThreadPool {
    public:
        typedef std::function<void(EventLoop *)> ThreadInitCallback;

        EventLoopThreadPool();

        ~EventLoopThreadPool() = default;

        void init(EventLoop *baseLoop, int numThreads);

        void start(const ThreadInitCallback &cb = ThreadInitCallback());

        void stop();

        // valid after calling start()
        /// round-robin
        EventLoop *getNextLoop();

        /// with the same hash code, it will always return the same EventLoop
        EventLoop *getLoopForHash(size_t hashCode);

        std::vector<EventLoop *> getAllLoops();

        [[nodiscard]] bool started() const { return started_; }

        [[nodiscard]] const std::string &name() const { return name_; }

        [[nodiscard]] std::string info() const;

    private:

        EventLoop *baseLoop_;
        std::string name_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::unique_ptr<EventLoopThread> > threads_;
        std::vector<EventLoop *> loops_;
    };
}
