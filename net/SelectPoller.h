#pragma once

#include <sys/epoll.h>       // for epoll_event
#include <sys/select.h>      // for fd_set
#include <algorithm>         // for max
#include <map>               // for map, map<>::value_compare
#include <vector>            // for vector

#include "Poller.h"          // for Poller::ChannelList, Poller
#include "base/Timestamp.h"  // for Timestamp

namespace net {
    class Channel;
    class EventLoop;

    class SelectPoller : public Poller {
    public:
        explicit SelectPoller(EventLoop *loop);

        virtual ~SelectPoller() = default;

        Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

        bool updateChannel(Channel *channel) override;

        void removeChannel(Channel *channel) override;

        bool hasChannel(Channel *channel) const override;

        //static EPollPoller* newDefaultPoller(EventLoop* loop);

        void assertInLoopThread() const;

    private:
        static const int kInitEventListSize = 16;

        void fillActiveChannels(int numEvents, ChannelList *activeChannels, fd_set &readfds, fd_set &writefds) const;

        bool update(int operation, Channel *channel);

        typedef std::vector<struct epoll_event> EventList;

        int epollfd_{};
        EventList events_;

        typedef std::map<int, Channel *> ChannelMap;

        ChannelMap channels_;
        EventLoop *ownerLoop_;
    };
}
