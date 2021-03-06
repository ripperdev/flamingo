#include "TimerQueue.h"

#include "EventLoop.h"       // for EventLoop
#include "Timer.h"           // for Timer
#include "base/Timestamp.h"  // for Timestamp, operator<=
#include "net/Callbacks.h"   // for TimerCallback, net
#include "net/TimerId.h"     // for TimerId

using namespace net;
//using namespace net::detail;

TimerQueue::TimerQueue(EventLoop *loop)
        : loop_(loop),
        /*timerfd_(createTimerfd()),
        timerfdChannel_(loop, timerfd_),*/
          timers_()
//callingExpiredTimers_(false)
{
    //timerfdChannel_.setReadCallback(
    //    std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    //½«timerfd¹Òµ½epollfdÉÏ
    //timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    //timerfdChannel_.disableAll();
    //timerfdChannel_.remove();
    //::close(timerfd_);
    // do not remove channel, since we're in EventLoop::dtor();
    for (const auto & timer : timers_) {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback &cb, Timestamp when, int64_t interval, int64_t repeatCount) {
    auto *timer = new Timer(cb, when, interval);
    loop_->runInLoop([this, timer] { addTimerInLoop(timer); });
    return TimerId(timer, timer->sequence());
}

TimerId TimerQueue::addTimer(TimerCallback &&cb, Timestamp when, int64_t interval, int64_t repeatCount) {
    auto *timer = new Timer(std::move(cb), when, interval, repeatCount);
    loop_->runInLoop([this, timer] { addTimerInLoop(timer); });
    return TimerId(timer, timer->sequence());
}

void TimerQueue::removeTimer(TimerId timerId) {
    loop_->runInLoop([this, timerId] { removeTimerInLoop(timerId); });
}

void TimerQueue::cancel(TimerId timerId, bool off) {
    loop_->runInLoop([this, timerId, off] { cancelTimerInLoop(timerId, off); });
}

void TimerQueue::doTimer() {
    loop_->assertInLoopThread();

    Timestamp now(Timestamp::now());

    for (auto iter = timers_.begin(); iter != timers_.end();) {
        //if (iter->first <= now)
        if (iter->second->expiration() <= now) {
            //LOGD("time: %lld", iter->second->expiration().microSecondsSinceEpoch());
            iter->second->run();
            if (iter->second->getRepeatCount() == 0) {
                iter = timers_.erase(iter);
            } else {
                ++iter;
            }
        } else {
            break;
        }
    }


    //readTimerfd(timerfd_, now);

    //std::vector<Entry> expired = getExpired(now);

    //callingExpiredTimers_ = true;
    //cancelingTimers_.clear();
    //// safe to callback outside critical section
    //for (std::vector<Entry>::iterator it = expired.begin();
    //    it != expired.end(); ++it)
    //{
    //    it->second->run();
    //}
    //callingExpiredTimers_ = false;

    //reset(expired, now);
}

void TimerQueue::addTimerInLoop(Timer *timer) {
    loop_->assertInLoopThread();
    /*bool earliestChanged = */insert(timer);

    //if (earliestChanged)
    //{
    //    resetTimerfd(timerfd_, timer->expiration());
    //}
}

void TimerQueue::removeTimerInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    //assert(timers_.size() == activeTimers_.size());
    //ActiveTimer timer(timerId.timer_, timerId.sequence_);
    //ActiveTimerSet::iterator it = activeTimers_.find(timer);

    Timer *timer = timerId.timer_;
    for (auto iter = timers_.begin(); iter != timers_.end(); ++iter) {
        if (iter->second == timer) {
            timers_.erase(iter);
            break;
        }
    }
}

void TimerQueue::cancelTimerInLoop(TimerId timerId, bool off) {
    loop_->assertInLoopThread();

    Timer *timer = timerId.timer_;
    for (const auto & iter : timers_) {
        if (iter.second == timer) {
            iter.second->cancel(off);
            break;
        }
    }

    ////assert(timers_.size() == activeTimers_.size());
    //ActiveTimer timer(timerId.timer_, timerId.sequence_);
    //ActiveTimerSet::iterator it = activeTimers_.find(timer);
    //if (it != activeTimers_.end())
    //{
    //    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    //    //assert(n == 1); (void)n;
    //    delete it->first; // FIXME: no delete please
    //    activeTimers_.erase(it);
    //}
    //else if (callingExpiredTimers_)
    //{
    //    cancelingTimers_.insert(timer);
    //}
    ////assert(timers_.size() == activeTimers_.size());
}

//std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
//{
//    assert(timers_.size() == activeTimers_.size());
//    std::vector<Entry> expired;
//    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
//    TimerList::iterator end = timers_.lower_bound(sentry);
//    assert(end == timers_.end() || now < end->first);
//    std::copy(timers_.begin(), end, back_inserter(expired));
//    timers_.erase(timers_.begin(), end);
//
//    for (std::vector<Entry>::iterator it = expired.begin();
//        it != expired.end(); ++it)
//    {
//        ActiveTimer timer(it->second, it->second->sequence());
//        size_t n = activeTimers_.erase(timer);
//        assert(n == 1); (void)n;
//    }
//
//    assert(timers_.size() == activeTimers_.size());
//    return expired;
//}

//void TimerQueue::reset(const std::vector<Entry> & expired, Timestamp now)
//{
//    Timestamp nextExpire;
//
//    for (std::vector<Entry>::const_iterator it = expired.begin();
//        it != expired.end(); ++it)
//    {
//        ActiveTimer timer(it->second, it->second->sequence());
//        if (it->second->getRepeatCount()
//            && cancelingTimers_.find(timer) == cancelingTimers_.end())
//        {
//            it->second->restart(now);
//            insert(it->second);
//        }
//        else
//        {
//            // FIXME move to a free list
//            delete it->second; // FIXME: no delete please
//        }
//    }
//
//    if (!timers_.empty())
//    {
//        nextExpire = timers_.begin()->second->expiration();
//    }
//
//    if (nextExpire.valid())
//    {
//        resetTimerfd(timerfd_, nextExpire);
//    }
//}

void TimerQueue::insert(Timer *timer) {
    loop_->assertInLoopThread();
    //assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    //TimerList::iterator it = timers_.begin();
    //if (it == timers_.end() || when < it->first)
    //{
    //    earliestChanged = true;
    //}
    //{
    /*std::pair<TimerList::iterator, bool> result = */timers_.insert(Entry(when, timer));
    //assert(result.second); (void)result;
    //}
    //{
    //    std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    //assert(result.second); (void)result;
    //}

    //assert(timers_.size() == activeTimers_.size());
    //return earliestChanged;
}
