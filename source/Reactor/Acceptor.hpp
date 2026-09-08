#pragma once
#include <functional>
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Log.hpp"

class Acceptor
{
    using AcceptCallBack = std::function<void(int)>;

    int CreateServer(int port)
    {
        bool ret = _listensockfd.CreateServer(port);
        if (ret == false)
        {
            LOG(LOGLEVEL::ERR, "listensock create failed!!!");
            exit(-1);
        }
        return _listensockfd.Fd();
    }

    void HandleRead()
    {
        int newfd = _listensockfd.Accept();
        if (newfd < 0)
        {
            LOG(LOGLEVEL::WARNING, "listensock accept failed!!!");
            return;
        }
        if (_accept_callback)
        {
            _accept_callback(newfd);
        }
    }

public:
    Acceptor(EventLoop *loop, int port)
        : _listensockfd(CreateServer(port)), _loop(loop), _channel(_loop, _listensockfd.Fd())
    {
        _channel.SetReadCallback(std::bind(&HandleRead, this));
    }

    Acceptor(const Acceptor &) = delete;
    Acceptor &operator=(const Acceptor &) = delete;

    void SetAcceptCallback(const AcceptCallBack &cb)
    {
        _accept_callback = cb;
    }

    void Listen()
    {
        _channel.EnableRead();
    }

    void Close()
    {
        _channel.Remove();
        _listensockfd.Close();
    }

    ~Acceptor()
    {
        Close();
    }

private:
    Socket _listensockfd;
    EventLoop *_loop;
    Channel _channel;
    AcceptCallBack _accept_callback; // 用于接收新连接后的处理
};