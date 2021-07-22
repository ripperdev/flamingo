#pragma once

#include <vector>
#include <map>

#include "base/Timestamp.h"
#include "Poller.h"

struct epoll_event;

namespace net {
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
