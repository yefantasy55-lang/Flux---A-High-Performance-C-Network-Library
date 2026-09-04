#pragma once
#include <sys/epoll.h>
#include <unordered_map>
#include <stdlib.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "Log.hpp"
#include "Channel.hpp"

#define MAX_EPOLLEVENTS 1024

enum Error
{
    EPOLL_CREATE_ERR,
    EPOLL_WAIT_ERR,
};

class Poller
{
private:
    bool Update(Channel *channel, int op)
    {
        struct epoll_event ev;
        ev.data.fd = channel->Fd();
        ev.events = channel->Events();
        int n = epoll_ctl(_epfd, op, channel->Fd(), &ev);
        if (n < 0)
        {
            LOG(LOGLEVEL::ERR, "Socket:%d EpollCTL Failed!!!", channel->Fd());
            return false;
        }
        return true;
    }

    // 有返回true,无false
    bool HasChannel(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it == _channels.end())
            return false;
        else
            return true;
    }

public:
    Poller()
    {
        _epfd = epoll_create(256);
        if (_epfd < 0)
        {
            LOG(LOGLEVEL::ERR, "Epoll Create Failed!!!");
            exit(EPOLL_CREATE_ERR);
        }
    }

    // 添加或修改监控事件
    bool UpdateEvent(Channel *channel)
    {
        bool ret = HasChannel(channel);

        // 没有该Channel
        if (ret == false)
        {
            int fd = channel->Fd();
            _channels.insert(std::make_pair(fd, channel));
            if (!Update(channel, EPOLL_CTL_ADD))
            {
                _channels.erase(fd); // 回滚
                return false;
            }
        }
        else
        {
            if (!Update(channel, EPOLL_CTL_MOD))
                return false;
        }
        return true;
    }

    // 移除事件监控
    bool RemoveEvent(Channel *channel)
    {
        bool ret = HasChannel(channel);
        int fd = channel->Fd();
        if (ret == false)
        {
            LOG(LOGLEVEL::WARNING, "Doesn't exist channel(sockfd:%d)", fd);
            return false;
        }
        else
        {
            _channels.erase(fd);
            Update(channel, EPOLL_CTL_DEL);
        }
        return true;
    }

    // 监控Channel,返回活跃连接
    void Poll(std::vector<Channel *> &actives, int timeout = -1)
    {
        int nfds = epoll_wait(_epfd, _evs, MAX_EPOLLEVENTS, timeout);
        if (nfds < 0)
        {
            if (errno == EINTR)
            {
                LOG(LOGLEVEL::WARNING, "EPOLL WAIT EINTR:%s\n", strerror(errno));
                return;
            }
            LOG(LOGLEVEL::ERR, "EPOLL WAIT ERROR:%s\n", strerror(errno));
            exit(EPOLL_WAIT_ERR);
        }

        for (int i = 0; i < nfds; i++)
        {
            int fd = _evs[i].data.fd;
            uint32_t events = _evs[i].events;
            auto it = _channels.find(fd);
            if (it != _channels.end())
            {
                it->second->SetREvents(events);
                actives.push_back(it->second);
            }
            else
            {
                LOG(LOGLEVEL::ERR, "Poll: fd %d not found in channels, event dropped!", fd);
            }
        }
    }

    void Close()
    {
        close(_epfd);
    }

    ~Poller()
    {
        Close();
    }

private:
    int _epfd;
    struct epoll_event _evs[MAX_EPOLLEVENTS];
    std::unordered_map<int, Channel *> _channels; // 上层的connection可以根据这个找到对应的Channel处理事件
};