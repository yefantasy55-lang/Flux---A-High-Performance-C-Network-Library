#pragma once
#include <atomic>
#include <functional>
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <sys/eventfd.h>
#include <pthread.h>
#include <cassert>
#include "Poller.hpp"
#include "TimerWheel.hpp"
#include "Log.hpp"

class Channel;
class TimerWheel;

enum EVENTFD_ERR
{
    CREATE_EVENTFD_ERR = 0,
    READ_EVENTFD_ERR,
    WEAK_EVENTFD_ERR
};

class EventLoop
{
    using Functor = std::function<void()>;

    static int CreateEventFd()
    {
        int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK); // EFD_CLOEXEC表示该描述符只能被当前线程持有
        if (efd < 0)
        {
            LOG(LOGLEVEL::ERR, "CREATE EVENTFD FAILED!!!");
            exit(CREATE_EVENTFD_ERR);
        }
        return efd;
    }

    void ReadEventfd()
    {
        uint64_t ret = 0;
        int n = read(_event_fd, &ret, sizeof(ret)); // 防止一直通知
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return;
            }
            LOG(LOGLEVEL::ERR, "READ EVENTFD FAILED!!!");
            exit(READ_EVENTFD_ERR);
        }
        return;
    }

    void WakeUpEventFd()
    {
        uint64_t val = 1; // 内部计数一次增长1
        int n = write(_event_fd, &val, sizeof(val));
        if (n < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            LOG(LOGLEVEL::ERR, "WEAK EVENTFD FAILED!!!");
            exit(WEAK_EVENTFD_ERR);
        }
        return;
    }

    // 执行任务池中的所有任务
    void RunAllTask()
    {
        std::vector<Functor> functor;
        // 1.交换需加锁保证线程安全,因为在交换时可能会有其他任务压入
        // 2.降低锁的持有时间，如果没有funtor来交换指针，那就需要对原_tasks挨个遍历执行完任务才能继续压入任务，
        // 非常耗时
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _tasks.swap(functor);
        }
        for (auto &f : functor)
        {
            f();
        }
        return;
    }

public:
    EventLoop()
        : _isrunning(false), _thread_id(std::this_thread::get_id()), _event_fd(CreateEventFd()),
          _event_channel(std::make_unique<Channel>(this, _event_fd)), _timer_wheel(this)
    {
        _event_channel->SetReadCallback(std::bind(&ReadEventfd, this));
        _event_channel->EnableRead();
    }

    ~EventLoop()
    {
        if (_event_fd > 0)
        {
            close(_event_fd);
        }
    }

    void Start()
    {
        _isrunning = true;
        while (_isrunning)
        {
            std::vector<Channel *> actives;
            _poller.Poll(actives, -1); // 积极连接(有事件需处理的channel)
            for (auto &channel : actives)
            {
                channel->HandleEvent(); // 处理事件
            }
            RunAllTask(); // 执行回调
        }
    }

    void Stop()
    {
        _isrunning = false;
        WakeUpEventFd(); // 唤醒监控
    }

    // 用于判断当前线程是否是EventLoop对应的线程；
    bool IsInLoop()
    {
        return _thread_id == std::this_thread::get_id();
    }

    void AssertInLoop()
    {
        assert(_thread_id == std::this_thread::get_id());
    }

    // 判断将要执行的任务是否处于当前线程中，如果是则执行，不是则压入队列。
    void RunInLoop(const Functor &cb)
    {
        if (IsInLoop())
        {
            cb();
            return;
        }
        else
        {
            QueueInLoop(cb);
            return;
        }
    }

    // 将操作压入任务池
    void QueueInLoop(const Functor &cb)
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _tasks.push_back(cb);
        }
        WakeUpEventFd(); // 有数据了，就可以唤醒线程
    }

    void UpdateEvent(Channel *channel) { _poller.UpdateEvent(channel); }

    void RemoveEvent(Channel *channel) { _poller.RemoveEvent(channel); }

    void TimerRefresh(uint64_t id) { _timer_wheel.TimerRefresh(id); }

    void TimerAdd(uint64_t id, uint32_t delay, const TaskFun &cb) { return _timer_wheel.TimerAdd(id, delay, cb); }

    void TimerCancel(uint64_t id) { _timer_wheel.TimerCancel(id); }

    bool HasTimer(uint64_t id) { _timer_wheel.HasTimer(id); }

private:
    std::atomic<bool> _isrunning;            // 是否运行,用atomic是为了防止编译器激进优化导致布尔值一直不改变
    std::thread::id _thread_id;              // 线程id，一个线程只有一个eventloop
    int _event_fd;                           // 通过eventfd来唤醒消费任务
    std::unique_ptr<Channel> _event_channel; // 将eventfd作为channel管理起来
    Poller _poller;                          // 用于监听事件
    std::vector<Functor> _tasks;             // 任务池
    std::mutex _mutex;                       // 保证线程安全
    TimerWheel _timer_wheel;                 // 定时器
};

// 把timerwheel的部分类成员函数只做声明，放到EventLoop模块实现，防止互包
inline void TimerWheel::TimerAdd(uint64_t id, uint32_t timeout, const TaskFun &task_cb)
{
    _loop->RunInLoop(std::bind(&TimerAddInLoop, this, id, timeout, task_cb));
}

inline void TimerWheel::TimerRefresh(uint64_t id)
{
    _loop->RunInLoop(std::bind(&TimerRefreshInLoop, this, id));
}

inline void TimerWheel::TimerCancel(uint64_t id)
{
    _loop->RunInLoop(std::bind(&TimerCancelInLoop, this, id));
}