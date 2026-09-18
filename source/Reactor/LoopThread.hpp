#pragma once
#include <memory>
#include <condition_variable> // 条件变量
#include <thread>
#include "EventLoop.hpp"

class LoopThread
{

    /*实例化 EventLoop 对象，唤醒_cond上有可能阻塞的线程，并且开始运行EventLoop模块的功能*/
    void ThreadEntry()
    {
        EventLoop loop;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loop = &loop;
            _cond.notify_all();
        }
        loop.Start(); // 这里开始阻塞运行
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _loop = nullptr; // 标记失效
        }
    }

public:
    LoopThread()
        : _loop(nullptr), _thread(std::thread(&LoopThread::ThreadEntry, this))
    {
    }

    EventLoop *GetLoop()
    {
        {

            std::unique_lock<std::mutex> lock(_mutex);
            _cond.wait(lock, [&]()
                       { return _loop != nullptr; });
        }
        return _loop;
    }

    ~LoopThread()
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_loop)
            {
                _loop->Stop(); // 持有锁时调用 Stop，确保 _loop 不会在调用中途被置空
            }
        }
        if (_thread.joinable())
        {
            _thread.join(); // 等待线程真正结束
        }
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    EventLoop *_loop;
    std::thread _thread;
};