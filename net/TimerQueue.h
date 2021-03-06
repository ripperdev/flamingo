#pragma once

#include <stdint.h>             // for int64_t
#include <set>                  // for set
#include <utility>              // for pair

#include "../base/Timestamp.h"  // for Timestamp, operator<
#include "../net/Callbacks.h"   // for TimerCallback

namespace net {
    class EventLoop;
    class Timer;
    class TimerId;

    ///
    /// A best efforts timer queue.
    /// No guarantee that the callback will be on time.
    ///
    class TimerQueue {
    public:
        explicit TimerQueue(EventLoop *loop);

        ~TimerQueue();

        ///
        /// Schedules the callback to be run at given time,
        /// repeats if @c interval > 0.0.
        ///
        /// Must be thread safe. Usually be called from other threads.
        //intervalµ¥Î»ÊÇÎ¢Ãî
        TimerId addTimer(const TimerCallback &cb, Timestamp when, int64_t interval, int64_t repeatCount);

        TimerId addTimer(TimerCallback &&cb, Timestamp when, int64_t interval, int64_t repeatCount);

        void removeTimer(TimerId timerId);

        void cancel(TimerId timerId, bool off);

        // called when timerfd alarms
        void doTimer();

        //noncopyable
        TimerQueue(const TimerQueue &rhs) = delete;

        TimerQueue &operator=(const TimerQueue &rhs) = delete;

    private:
        // FIXME: use unique_ptr<Timer> instead of raw pointers.
        typedef std::pair<Timestamp, Timer *> Entry;
        typedef std::set<Entry> TimerList;
        typedef std::pair<Timer *, int64_t> ActiveTimer;
        typedef std::set<ActiveTimer> ActiveTimerSet;

        void addTimerInLoop(Timer *timer);

        void removeTimerInLoop(TimerId timerId);

        void cancelTimerInLoop(TimerId timerId, bool off);

        // move out all expired timers
        //std::vector<Entry> getExpired(Timestamp now);
        //void reset(const std::vector<Entry>& expired, Timestamp now);

        void insert(Timer *timer);

        EventLoop *loop_;
        //const int           timerfd_;
        //Channel             timerfdChannel_;
        // Timer list sorted by expiration
        TimerList timers_;

        //for cancel()
        //ActiveTimerSet      activeTimers_;
        //bool                callingExpiredTimers_; /* atomic */
        //ActiveTimerSet      cancelingTimers_;
    };
}
