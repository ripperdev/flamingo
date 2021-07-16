#pragma once

#include <stdint.h>             // for int64_t
#include <functional>           // for function
#include <memory>               // for unique_ptr
#include <mutex>                // for mutex
#include <thread>               // for get_id, operator==, thread::id, thread
#include <vector>               // for vector

#include "../base/Platform.h"   // for SOCKET
#include "../base/Timestamp.h"  // for Timestamp
#include "Callbacks.h"          // for TimerCallback
#include "TimerId.h"            // for TimerId

namespace net {
    class Channel;
    class Poller;
class TimerQueue;

    ///
    /// Reactor, at most one per thread.
    ///
    /// This is an interface class, so don't expose too much details.
    class EventLoop {
    public:
        typedef std::function<void()> Functor;

        EventLoop();

        ~EventLoop();  // force out-line dtor, for scoped_ptr members.

        ///
        /// Loops forever.
        ///
        /// Must be called in the same thread as creation of the object.
        ///
        void loop();

        /// Quits loop.
        ///
        /// This is not 100% thread safe, if you call through a raw pointer,
        /// better to call through shared_ptr<EventLoop> for 100% safety.
        void quit();

        ///
        /// Time when poll returns, usually means data arrival.
        ///
        [[nodiscard]] Timestamp pollReturnTime() const { return pollReturnTime_; }

        [[nodiscard]] int64_t iteration() const { return iteration_; }

        /// Runs callback immediately in the loop thread.
        /// It wakes up the loop, and run the cb.
        /// If in the same loop thread, cb is run within the function.
        /// Safe to call from other threads.
        void runInLoop(const Functor &cb);

        /// Queues callback in the loop thread.
        /// Runs after finish pooling.
        /// Safe to call from other threads.
        void queueInLoop(const Functor &cb);

        // timers，时间单位均是微秒
        ///
        /// Runs callback at 'time'.
        /// Safe to call from other threads.
        ///
        TimerId runAt(const Timestamp &time, const TimerCallback &cb);

        ///
        /// Runs callback after @c delay seconds.
        /// Safe to call from other threads.
        ///
        TimerId runAfter(int64_t delay, const TimerCallback &cb);

        ///
        /// Runs callback every @c interval seconds.
        /// Safe to call from other threads.
        ///
        TimerId runEvery(int64_t interval, const TimerCallback &cb);

        ///
        /// Cancels the timer.
        /// Safe to call from other threads.
        ///
        void cancel(TimerId timerId, bool off);

        void remove(TimerId timerId);


        TimerId runAt(const Timestamp &time, TimerCallback &&cb);

        TimerId runAfter(int64_t delay, TimerCallback &&cb);

        TimerId runEvery(int64_t interval, TimerCallback &&cb);

        void setFrameFunctor(const Functor &cb);

        // internal usage
        bool updateChannel(Channel *channel);

        void removeChannel(Channel *channel);

        bool hasChannel(Channel *channel);

        // pid_t threadId() const { return threadId_; }
        void assertInLoopThread() {
            if (!isInLoopThread()) {
                abortNotInLoopThread();
            }
        }

        [[nodiscard]] bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }

        // bool callingPendingFunctors() const { return callingPendingFunctors_; }
        [[nodiscard]] bool eventHandling() const { return eventHandling_; }

        [[nodiscard]] std::thread::id getThreadID() const {
            return threadId_;
        }

    private:
        bool createWakeupfd();

        bool wakeup() const;

        void abortNotInLoopThread();

        bool handleRead() const;  // waked up handler
        void doPendingFunctors();

        void printActiveChannels() const; // DEBUG

    private:
        typedef std::vector<Channel *> ChannelList;

        bool looping_; /* atomic */
        bool quit_; /* atomic and shared between threads, okay on x86, I guess. */
        bool eventHandling_; /* atomic */
        bool callingPendingFunctors_; /* atomic */
        const std::thread::id threadId_;
        Timestamp pollReturnTime_;
        std::unique_ptr<Poller> poller_;
        std::unique_ptr<TimerQueue> timerQueue_;
        int64_t iteration_;
        SOCKET wakeupFd_{};          //TODO: 这个fd什么时候释放？
        // unlike in TimerQueue, which is an internal class,
        // we don't expose Channel to client.
        std::unique_ptr<Channel> wakeupChannel_;

        // scratch variables
        ChannelList activeChannels_;
        Channel *currentActiveChannel_;

        std::mutex mutex_;
        std::vector<Functor> pendingFunctors_; // Guarded by mutex_

        Functor frameFunctor_;
    };
}
