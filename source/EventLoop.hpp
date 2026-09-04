#pragma once
#include <functional>
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include "Poller.hpp"
#include "TimerWheel.hpp"

class Channel;
class TimerWheel;

class EventLoop
{
    using Functor = std::function<void()>;

public:
    // 执行任务池中的所有任务
    void RunAllTask()
    {
    }

    void RemoveEvent(Channel *)
    {
    }

    void UpdateEvent(Channel *)
    {
    }

    void RunInLoop(const Functor &fun)
    {
    }

private:
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