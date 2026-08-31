#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <unistd.h>

using TaskFun = std::function<void()>;
using ReleaseFun = std::function<void()>;

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

public:
    TimerWheel()
        : _tick(0), _capacity(60), _wheel(_capacity)
    {
    }

    void TimerAdd(uint64_t id, uint32_t timeout, const TaskFun &task_cb)
    {
        PtrTask pt = std::make_shared<TimerTask>(id, timeout, task_cb);
        pt->SetRelease(std::bind(&TimerWheel::TimerRefresh, this, id));
        int pos = (_tick + timeout) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = WeakTask(pt);
    }

    void TimerRefresh(uint64_t id)
    {
        if (!IsExistTimer(id))
            return;
        PtrTask pt = _timers[id].lock();
        int pos = (_tick + pt->DelayTime()) % _capacity;
        _wheel[pos].push_back(pt);
    }

    void TimerCancel(uint64_t id)
    {
        if (!IsExistTimer(id))
            return;
        _timers[id].lock()->Cancel();
    }

    void RunTimerTask()
    {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear(); // 清空元素，即时间轮中的智能指针走向析构
    }

    ~TimerWheel()
    {
    }

private:
    int _tick;                                      // 时间轮的时针
    int _capacity;                                  // 表盘的最大数量，即时针走完一圈的长度
    std::vector<std::vector<PtrTask>> _wheel;       // 时间轮
    std::unordered_map<uint64_t, WeakTask> _timers; // 用于找出特定任务id的任务
};
