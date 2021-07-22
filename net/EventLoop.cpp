#include "EventLoop.h"

#include <sstream>
#include <cstring>

#include "base/Logger.h"

#include "Channel.h"
#include "EpollPoller.h"

using namespace net;

//内部侦听唤醒fd的侦听端口，因此外部可以再使用这个端口
//#define INNER_WAKEUP_LISTEN_PORT 10000

thread_local EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 1;

EventLoop *getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

// 在线程函数中创建eventloop
EventLoop::EventLoop()
        : looping_(false),
          quit_(false),
          eventHandling_(false),
          callingPendingFunctors_(false),
          threadId_(std::this_thread::get_id()),
          timerQueue_(new TimerQueue(this)),
          iteration_(0L),
          currentActiveChannel_(nullptr) {
    createWakeupfd();

    wakeupChannel_ = std::make_unique<Channel>(this, wakeupFd_);
    poller_ = std::make_unique<EPollPoller>(this);

    if (t_loopInThisThread) {
        LOG_ERROR("Another EventLoop  exists in this thread ");
    } else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback([this](Timestamp) { handleRead(); });
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();

    //std::stringstream ss;	
    //ss << "eventloop create threadid = " << threadId_;
    //std::cout << ss.str() << std::endl;
}

EventLoop::~EventLoop() {
    assertInLoopThread();
    LOG_DEBUG("EventLoop {} destructs.", (void *) this);

    //std::stringstream ss;
    //ss << "eventloop destructs threadid = " << threadId_;
    //std::cout << ss.str() << std::endl;

    wakeupChannel_->disableAll();
    wakeupChannel_->remove();

    sockets::close(wakeupFd_);

    //_close(fdpipe_[0]);
    //_close(fdpipe_[1]);

    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    //assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
    LOG_DEBUG("EventLoop {}  start looping", (void *) this);

    while (!quit_) {
        timerQueue_->doTimer();

        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        //if (Logger::logLevel() <= Logger::TRACE)
        //{
        printActiveChannels();
        //}
        ++iteration_;
        // TODO sort channel by priority
        eventHandling_ = true;
        for (const auto &it : activeChannels_) {
            currentActiveChannel_ = it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        doPendingFunctors();

        if (frameFunctor_) {
            frameFunctor_();
        }
    }

    LOG_DEBUG("EventLoop {} stop looping", (void *) this);
    looping_ = false;


    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string stid = oss.str();
    LOG_INFO("Exiting loop, EventLoop object: {} , threadID: {}", (void *) this, stid.c_str());
}

void EventLoop::quit() {
    quit_ = true;
    // There is a chance that loop() just executes while(!quit_) and exists,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor &cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::setFrameFunctor(const Functor &cb) {
    frameFunctor_ = cb;
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
    //只执行一次
    return timerQueue_->addTimer(cb, time, 0, 1);
}

TimerId EventLoop::runAfter(int64_t delay, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(int64_t interval, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    //-1表示一直重复下去
    return timerQueue_->addTimer(cb, time, interval, -1);
}

TimerId EventLoop::runAt(const Timestamp &time, TimerCallback &&cb) {
    return timerQueue_->addTimer(std::move(cb), time, 0, 1);
}

TimerId EventLoop::runAfter(int64_t delay, TimerCallback &&cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(int64_t interval, TimerCallback &&cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval, -1);
}

void EventLoop::cancel(TimerId timerId, bool off) {
    return timerQueue_->cancel(timerId, off);
}

void EventLoop::remove(TimerId timerId) {
    return timerQueue_->removeTimer(timerId);
}

bool EventLoop::updateChannel(Channel *channel) {
    //assert(channel->ownerLoop() == this);
    if (channel->ownerLoop() != this)
        return false;

    assertInLoopThread();

    return poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    //assert(channel->ownerLoop() == this);
    if (channel->ownerLoop() != this)
        return;

    assertInLoopThread();
    if (eventHandling_) {
        //assert(currentActiveChannel_ == channel || std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }

    LOG_DEBUG("Remove channel, channel = {}, fd = {}", (void *) channel, channel->fd());
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    //assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

bool EventLoop::createWakeupfd() {
    //Linux上一个eventfd就够了，可以实现读写
    wakeupFd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd_ < 0) {
        //让程序挂掉
        LOG_CRITICAL("Unable to create wakeup eventfd, EventLoop: {}", (void*)this);
        return false;
    }
    return true;
}

void EventLoop::abortNotInLoopThread() {
    std::stringstream ss;
    ss << "threadid_ = " << threadId_ << " this_thread::get_id() = " << std::this_thread::get_id();
    LOG_CRITICAL("EventLoop::abortNotInLoopThread - EventLoop {}", ss.str().c_str());
}

bool EventLoop::wakeup() const {
    uint64_t one = 1;
    int32_t n = sockets::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof one) {
        int error = errno;
        LOG_ERROR("EventLoop::wakeup() writes {} bytes instead of 8, fd: {}, error: {}, errorinfo: {}", n, wakeupFd_,
                  error, strerror(error));
        return false;
    }
    return true;
}

bool EventLoop::handleRead() const {
    uint64_t one = 1;
    int32_t n = sockets::read(wakeupFd_, &one, sizeof(one));

    if (n != sizeof one) {
        int error = errno;
        LOG_ERROR("EventLoop::wakeup() read {} bytes instead of 8, fd: {}, error: {}, errorinfo: {}", n, wakeupFd_,
                  error, strerror(error));
        return false;
    }
    return true;
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (auto &functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const {
    for (auto ch : activeChannels_) {
        LOG_DEBUG("{}", ch->reventsToString().c_str());
    }
}
