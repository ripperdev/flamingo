#pragma once

#include <poll.h>            // for pollfd
#include <algorithm>         // for max
#include <map>               // for map, map<>::value_compare
#include <vector>            // for vector

#include "Poller.h"          // for Poller::ChannelList, Poller
#include "base/Timestamp.h"  // for Timestamp

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
