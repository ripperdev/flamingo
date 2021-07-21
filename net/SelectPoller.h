#pragma once

#include "Poller.h"
#include <map>
#include "../base/Platform.h"

namespace net {
    class EventLoop;

    class Channel;

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
