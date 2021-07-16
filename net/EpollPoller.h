#pragma once

#include <sys/epoll.h>          // for epoll_event
#include <algorithm>            // for max
#include <map>                  // for map, map<>::value_compare
#include <vector>               // for vector

#include "../base/Timestamp.h"  // for Timestamp
#include "Poller.h"             // for Poller::ChannelList, Poller

namespace net {
class Channel;
    class EventLoop;

    class EPollPoller : public Poller {
    public:
        explicit EPollPoller(EventLoop *loop);

        virtual ~EPollPoller();

        Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

        bool updateChannel(Channel *channel) override;

        void removeChannel(Channel *channel) override;

        bool hasChannel(Channel *channel) const override;

        //static EPollPoller* newDefaultPoller(EventLoop* loop);

        void assertInLoopThread() const;

    private:
        static const int kInitEventListSize = 16;

        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

        bool update(int operation, Channel *channel) const;

    private:
        typedef std::vector<struct epoll_event> EventList;

        int epollfd_;
        EventList events_;

        typedef std::map<int, Channel *> ChannelMap;

        ChannelMap channels_;
        EventLoop *ownerLoop_;
    };

}
