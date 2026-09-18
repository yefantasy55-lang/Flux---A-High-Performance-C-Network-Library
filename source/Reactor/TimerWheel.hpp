#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <unistd.h>
#include <sys/timerfd.h>
#include "Log.hpp"
#include "Channel.hpp"

class EventLoop; // 前置声明,防止与EventLoop互包

using TaskFun = std::function<void()>;
using ReleaseFun = std::function<void()>;

enum TIMER_ERR
{
    TIMERFD_ERR = 0,
    READ_TIMEFD_ERR
};

class TimerTask
{
public:
    TimerTask(uint64_t id, uint32_t timeout, const TaskFun &task_cb)
        : _id(id), _timeout(timeout), _canceled(false), _task_cb(task_cb)
    {
    }

    void Cancel()
    {
        _canceled = true;
    }

    uint32_t DelayTime()
    {
        return _timeout;
    }

    void SetRelease(const ReleaseFun &cb)
    {
        _release = cb;
    }

    ~TimerTask()
    {
        if (_canceled == false)
            _task_cb();
        _release();
    }

private:
    uint64_t _id;        // 定时器任务的id
    uint32_t _timeout;   // 定时任务的超时时间
    bool _canceled;      // 是否取消定时任务
    TaskFun _task_cb;    // 定时器对象要执行的定时任务
    ReleaseFun _release; // 删除TimerWheel保存的定时器对象信息
};

class TimerWheel
{
    using WeakTask = std::weak_ptr<TimerTask>;  // 虚指针对象
    using PtrTask = std::shared_ptr<TimerTask>; // 共享指针对象

    bool IsExistTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it != _timers.end())
            return true;
        else
            return false;
    }

    void RemoveTimer(uint64_t id)
    {
        if (IsExistTimer(id))
            _timers.erase(id);
        else
            return;
    }

    // timerfd需要在_timer_channel前完成初始化，所以用了静态函数辅助创建
    static int CreateTimerfd()
    {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0)
        {
            LOG(LOGLEVEL::ERR, "TIMERFD CREATE FAILED!!!");
            exit(TIMERFD_ERR);
        }
        struct itimerspec itime;      // 通过这个结构体把定时时间设置进去
        itime.it_value.tv_sec = 1;    // 设置超时时间为一秒后
        itime.it_value.tv_nsec = 0;   // 这是纳秒的超时时间，为了防止错误，也要设置一下
        itime.it_interval.tv_sec = 1; // 设置超时后的时间间隔为1秒
        itime.it_interval.tv_nsec = 0;
        timerfd_settime(timerfd, 0, &itime, nullptr); // 第二个参数0表示用相对时间，即创建timerfd的时间开始
        return timerfd;
    }

    int ReadTimerfd()
    {
        uint64_t times; // timerfd需固定用uint64_t类型接收
        int ret = read(_timerfd, &times, sizeof(times));
        if (ret < 0)
        {
            LOG(LOGLEVEL::ERR, "READ TIMEFD FAILED!!!");
            exit(READ_TIMEFD_ERR);
        }
        return times;
    }

    void RunTimerTask()
    {
        _wheel[_tick].clear(); // 清空元素，即时间轮中的智能指针走向析构
        _tick = (_tick + 1) % _capacity;
    }

    void OnTime()
    {
        int times = ReadTimerfd();
        for (int i = 0; i < times; i++) // 根据超时次数执行对应次数的任务
        {
            RunTimerTask();
        }
    }

    void TimerAddInLoop(uint64_t id, uint32_t timeout, const TaskFun &task_cb)
    {
        PtrTask pt = std::make_shared<TimerTask>(id, timeout, task_cb);
        pt->SetRelease(std::bind(&TimerWheel::RemoveTimer, this, id));
        int pos = (_tick + timeout) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = WeakTask(pt);
    }

    void TimerRefreshInLoop(uint64_t id)
    {
        if (!IsExistTimer(id))
            return;
        PtrTask pt = _timers[id].lock();
        if (!pt)
        {
            _timers.erase(id);
            return;
        }
        int pos = (_tick + pt->DelayTime()) % _capacity;
        _wheel[pos].push_back(pt);
    }

    void TimerCancelInLoop(uint64_t id)
    {
        if (!IsExistTimer(id))
            return;
        auto pt = _timers[id].lock();
        if (pt)
            pt->Cancel();
    }

public:
    TimerWheel(EventLoop *loop)
        : _tick(0), _capacity(60), _timerfd(CreateTimerfd()),
          _loop(loop), _timer_channel(std::make_unique<Channel>(_loop, _timerfd)),
          _wheel(_capacity)
    {
        _timer_channel->SetReadCallback(std::bind(&TimerWheel::OnTime, this));
        _timer_channel->EnableRead(); // 启动读事件监控
    }

    void TimerAdd(uint64_t id, uint32_t timeout, const TaskFun &task_cb);

    void TimerRefresh(uint64_t id);

    void TimerCancel(uint64_t id);

    // 这个接口存在线程安全问题--这个接口实际上不能被外界使用者调用，只能在模块内，在对应的EventLoop线程内执行
    bool HasTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return false;
        }
        return true;
    }

    ~TimerWheel()
    {
        if (_timerfd > 0)
        {
            close(_timerfd);
        }
    }

private:
    int _tick;                                      // 时间轮的时针
    int _capacity;                                  // 表盘的最大数量，即时针走完一圈的长度
    int _timerfd;                                   // 定时器文件描述符
    EventLoop *_loop;                               // 用于定时任务串行执行
    std::unique_ptr<Channel> _timer_channel;        // 放进Channel统一管理
    std::vector<std::vector<PtrTask>> _wheel;       // 时间轮
    std::unordered_map<uint64_t, WeakTask> _timers; // 用于找出特定任务id的任务
};