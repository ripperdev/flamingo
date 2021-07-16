#include "PollPoller.h"

#include <errno.h>          // for errno, EINTR
#include <poll.h>           // for pollfd, poll
#include <stddef.h>         // for size_t
#include <memory>           // for allocator_traits<>::value_type
#include <utility>          // for pair

#include "Channel.h"        // for Channel
#include "EventLoop.h"      // for EventLoop
#include "base/Logger.h"    // for LOG_DEBUG, LOG_ERROR
#include "net/Callbacks.h"  // for implicit_cast
#include "net/Poller.h"     // for Poller::ChannelList, net

using namespace net;

PollPoller::PollPoller(EventLoop *loop) : ownerLoop_(loop) {
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    // XXX pollfds_ shouldn't change
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        LOG_DEBUG("{}  events happended", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        LOG_DEBUG("nothing happended");
    } else {
        if (savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("PollPoller::poll()");
        }
    }
    return now;
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    for (auto pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents > 0; ++pfd) {
        if (pfd->revents > 0) {
            --numEvents;
            auto ch = channels_.find(pfd->fd);
            //assert(ch != channels_.end());
            if (ch == channels_.end())
                continue;

            Channel *channel = ch->second;
            //assert(channel->fd() == pfd->fd);
            if (channel->fd() != pfd->fd)
                continue;

            channel->set_revents(pfd->revents);
            // pfd->revents = 0;
            activeChannels->push_back(channel);
        }
    }
}

bool PollPoller::updateChannel(Channel *channel) {
    assertInLoopThread();
    LOG_DEBUG("fd = {} events = {}", channel->fd(), channel->events());
    if (channel->index() < 0) {
        // a new one, add to pollfds_
        //assert(channels_.find(channel->fd()) == channels_.end());
        if (channels_.find(channel->fd()) != channels_.end())
            return false;

        struct pollfd pfd{};
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    } else {
        // update existing one
        //assert(channels_.find(channel->fd()) != channels_.end());
        //assert(channels_[channel->fd()] == channel);
        if (channels_.find(channel->fd()) == channels_.end() || channels_[channel->fd()] != channel)
            return false;

        int idx = channel->index();
        //assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        if (0 > idx || idx >= static_cast<int>(pollfds_.size()))
            return false;

        struct pollfd &pfd = pollfds_[idx];
        //TODO: 为什么是 -channel->fd() ？
        //assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            // ignore this pollfd
            pfd.fd = -channel->fd() - 1;
        }
    }

    return true;
}

void PollPoller::removeChannel(Channel *channel) {
    assertInLoopThread();
    LOG_DEBUG("fd:{}", channel->fd());

    //assert(channels_.find(channel->fd()) != channels_.end());
    //assert(channels_[channel->fd()] == channel);
    //assert(channel->isNoneEvent());

    if (channels_.find(channel->fd()) == channels_.end() || channels_[channel->fd()] != channel ||
        !channel->isNoneEvent())
        return;

    int idx = channel->index();
    //assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    if (0 > idx && idx >= static_cast<int>(pollfds_.size()))
        return;

    const struct pollfd &pfd = pollfds_[idx];
    (void) pfd;
    //TODO: 为什么是 -channel->fd()？
    //assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());

    size_t n = channels_.erase(channel->fd());
    //assert(n == 1); (void)n;
    if (n != 1)
        return;

    if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
        pollfds_.pop_back();
    } else {
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if (channelAtEnd < 0) {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}

void PollPoller::assertInLoopThread() const {
    ownerLoop_->assertInLoopThread();
}
