#pragma once
#include <functional>
#include <sys/epoll.h>
#include "EventLoop.hpp"

class Channel
{

    using EventCallBack = std::function<void()>;

public:
    Channel(EventLoop *loop, int sockfd)
        : _sockfd(sockfd), _loop(loop), _events(0), _revents(0)
    {
    }

    int Fd()
    {
        return _sockfd;
    }

    uint32_t Events()
    {
        return _events;
    }

    // 事件好了就通过这个接口设置
    void SetREvents(uint32_t events)
    {
        _revents = events;
    }

    void SetReadCallback(const EventCallBack &cb)
    {
        _read_callback = cb;
    }

    void SetWriteCallback(const EventCallBack &cb)
    {
        _write_callback = cb;
    }

    void SetErrorCallback(const EventCallBack &cb)
    {
        _error_callback = cb;
    }

    void SetCloseCallback(const EventCallBack &cb)
    {
        _close_callback = cb;
    }

    void SetEventCallback(const EventCallBack &cb)
    {
        _event_callback = cb;
    }

    // 是否监控可读
    bool ReadAble()
    {
        if (_events & EPOLLIN)
            return true;
        else
            return false;
    }

    // 是否监控可写
    bool WriteAble()
    {
        if (_events & EPOLLOUT)
            return true;
        else
            return false;
    }

    void EnableRead()
    {
        _events |= (EPOLLIN | EPOLLRDHUP | EPOLLPRI);
        Update();
    }

    void EnableWrite()
    {
        _events |= EPOLLOUT;
        Update();
    }

    void DisableRead()
    {
        _events &= ~(EPOLLIN | EPOLLRDHUP | EPOLLPRI);
        Update();
    }

    void DisableWrite()
    {
        _events &= ~EPOLLOUT;
        Update();
    }

    void DisableAll()
    {
        _events = 0;
        Update();
    }

    // 移除监控
    void Remove()
    {
        _loop->RemoveEvent(this);
    }

    void Update()
    {
        _loop->UpdateEvent(this);
    }

    // 根据触发的事假去调用对应的回调
    void HandleEvent()
    {
        if (_event_callback) // 什么事假触发都得调用任意事件回调
            _event_callback();

        // 可能同时触发读写，所以读写要设置成两个独立的if块,EPOLLPRI为紧急事件
        // 判断_revents & EPOLLRDHUP 和 _revents & EPOLLPRI是为了防止极端情况下没有返回EPOLLIN
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI))
        {
            if (_read_callback)
                _read_callback();
        }

        if (_revents & EPOLLOUT)
        {
            if (_write_callback)
                _write_callback();
        }
        else if (_revents & EPOLLERR)
        {
            if (_error_callback)
                _error_callback();
        }
        else if (_revents & EPOLLHUP) // 对端关闭
        {
            if (_close_callback)
                _close_callback();
        }
    }

    ~Channel() = default;

private:
    int _sockfd;
    EventLoop *_loop;
    uint32_t _events;
    uint32_t _revents;
    EventCallBack _read_callback;
    EventCallBack _write_callback;
    EventCallBack _error_callback;
    EventCallBack _close_callback;
    EventCallBack _event_callback;
};