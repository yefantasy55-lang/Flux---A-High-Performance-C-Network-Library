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

    void AddTimerTaskInLoop(const Functor &task, uint32_t timeout)
    {
        _next_id++; // 自增
        _baseloop.TimerAdd(_next_id, timeout, task);
    }

    void NewConnection(int fd)
    {
        _next_id++;
        PtrConnection conn = std::make_shared<Connection>(_pool.NextLoop(), _next_id, fd);

        // 为新来的connection设置回调
        conn->SetConnectedCallBack(_connected_callback);
        conn->SetMessageCallBack(_message_callback);
        conn->SetClosedCallBack(_close_callback);
        conn->SetAnyEventCallBack(_any_event_callback);

        // 是否启动超时非活跃连接销毁
        if (_enable_interactive_release)
            conn->EnableInactiveRelease(_timeout);

        conn->Established();                           // 建立后的回调(算是conn的初始化)
        _conns.insert(std::make_pair(_next_id, conn)); // 添加到_conns中统一管理
    }

    void RemoveConnectionInLoop(const PtrConnection &conn)
    {
        uint64_t id = conn->Id();
        auto it = _conns.find(id);
        if (it == _conns.end())
        {
            return;
        }
        _conns.erase(it); // 移除对conn的管理
    }

    void RemoveConnection(const PtrConnection &conn)
    {
        // _conns的管理操作交由baseloop执行
        _baseloop.RunInLoop(std::bind(&RemoveConnectionInLoop, this, conn));
    }

public:
    TcpServer(int port)
        : _next_id(0), _port(port), _enable_interactive_release(false),
          _acceptor(&_baseloop, _port), _pool(&_baseloop)
    {
        _acceptor.SetAcceptCallback(std::bind(&NewConnection, this, std::placeholders::_1));
        _acceptor.Listen();
    }

    // 设置线程数量
    void SetThreadCount(int count)
    {
        _pool.SetThreadCount(count);
    }

    void SetConnectedCallBack(const ConnectedCallBack &cb)
    {
        _connected_callback = cb;
    }

    void SetMessageCallBack(const MessageCallBack &cb)
    {
        _message_callback = cb;
    }

    void SetCloseCallBack(const CloseCallBack &cb)
    {
        _close_callback = cb;
    }

    void SetAnyEventCallBack(const AnyEventCallBack &cb)
    {
        _any_event_callback = cb;
    }

    // 启动超时非活跃连接销毁
    void EnableInactiveRelease(int timeout)
    {
        _timeout = timeout;
        _enable_interactive_release = true;
    }

    // 添加定时任务
    void AddTimerTask(const Functor &task, uint32_t timeout)
    {
        _baseloop.RunInLoop(std::bind(&AddTimerTaskInLoop, this, task, timeout));
    }

    // 启动服务器
    void Start()
    {
        _pool.Create();
        _baseloop.Start();
    }

    ~TcpServer()
    {
    }

private:
    uint64_t _next_id;                                  // 定时任务的任务号
    int _port;                                          // 连接端口
    int _timeout;                                       // 这是非活跃连接的统计时间
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