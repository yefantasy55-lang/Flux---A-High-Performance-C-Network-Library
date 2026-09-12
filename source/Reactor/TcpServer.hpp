#pragma once
#include <memory>
#include <functional>
#include "Buffer.hpp"
#include "EventLoop.hpp"
#include "LoopThread.hpp"
#include "LoopThreadPool.hpp"
#include "Connection.hpp"
#include "Acceptor.hpp"
#include "TimerWheel.hpp"

class TcpServer
{
    using ConnectedCallBack = std::function<void(const PtrConnection &)>;
    using MessageCallBack = std::function<void(const PtrConnection &, Buffer *)>;
    using CloseCallBack = std::function<void(const PtrConnection &)>;
    using AnyEventCallBack = std::function<void(const PtrConnection &)>;
    using Functor = std::function<void()>;

    void AddTimerTaskInLoop(const Functor &task, uint32_t delay)
    {
        _next_id++; // 自增
        _baseloop.TimerAdd(_next_id, delay, task);
    }

    void NewConnection(int fd)
    {
        _next_id++;
        PtrConnection conn = std::make_shared<Connection>(_pool.NextLoop(), _next_id, fd);
        conn->SetConnectedCallBack(_connected_callback);
        conn->SetMessageCallBack(_message_callback);
        conn->SetClosedCallBack(_close_callback);
        conn->SetAnyEventCallBack(_any_event_callback);
    }

public:
    TcpServer()
        : _next_id()
    {
    }

    ~TcpServer()
    {
    }

private:
    uint64_t _next_id;                                  // 定时任务的任务号
    int _port;                                          // 连接端口
    bool _enable_interactive_release;                   // 是否启动非活跃超时销毁
    EventLoop _baseloop;                                // 监听套接字的事件管理
    Acceptor _acceptor;                                 // 接收事件
    LoopThreadPool _pool;                               // 线程池
    std::unordered_map<uint64_t, PtrConnection> _conns; // 管理所有Connection

    ConnectedCallBack _connected_callback;
    MessageCallBack _message_callback;
    CloseCallBack _close_callback;
    AnyEventCallBack _any_event_callback;
};