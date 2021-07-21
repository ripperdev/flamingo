#pragma once

#include "Poller.h"

#include <vector>
#include <map>

struct pollfd;

namespace net {
    class Channel;

    class EventLoop;

    ///
    /// IO Multiplexing with poll(2).
    ///
    class PollPoller : public Poller {
    public:

        explicit PollPoller(EventLoop *loop);

        Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

        bool updateChannel(Channel *channel) override;

        void removeChannel(Channel *channel) override;

        void assertInLoopThread() const;

    private:
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

        typedef std::vector<struct pollfd> PollFdList;
        typedef std::map<int, Channel *> ChannelMap;

        ChannelMap channels_;
        PollFdList pollfds_;
        EventLoop *ownerLoop_;
    };

}
