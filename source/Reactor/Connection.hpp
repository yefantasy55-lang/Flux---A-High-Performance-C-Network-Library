#pragma once
#include <functional>
#include <memory>
#include "Any.hpp"
#include "EventLoop.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "Buffer.hpp"

class Connection;

typedef enum
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
} ConnStatu;

using PtrConnection = std::shared_ptr<Connection>;

static const int buffer_size = 65536;

class Connection : public std::enable_shared_from_this<Connection>
{
    using ConnectedCallBack = std::function<void(const PtrConnection &)>;         // 连接建立后的回调
    using MessageCallBack = std::function<void(const PtrConnection &, Buffer *)>; // 有消息后的回调
    using ClosedCallBack = std::function<void(const PtrConnection &)>;            // 关闭连接的回调
    using AnyEventCallBack = std::function<void(const PtrConnection &)>;          // 任意事件的回调

    // 描述符可读事件触发后调用的函数，接收socket数据放到接收缓冲区中，然后调用_message_callback
    void HandleRead()
    {
        char buffer[buffer_size];
        ssize_t n = _socket.NonBlockRecv(buffer, buffer_size - 1);
        if (n == 0)
        {
            return; // 返回0表示EAGAIN，没有数据了，直接返回，不要关连接！
        }
        if (n < 0)
        {
            // -1是错误，-2是对端关闭，都走关闭逻辑
            ShutdownInLoop();
            return;
        }
        _in_buffer.WriteAndPush(buffer, n);
        if (_in_buffer.ReadAbleSize() > 0)
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }
    }

    // 描述符可写事件触发后调用的函数，将发送缓冲区中的数据进行发送
    void HandleWrite()
    {
        ssize_t n = _socket.NonBlockSend(_out_buffer.ReaderPosition(), _out_buffer.ReadAbleSize());
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            // 因为如果小于0了，就说明套接字出现了严重问题，数据已经不可能发出去了，但是已经收到的数据还是
            // 要接收处理完毕
            if (_in_buffer.ReadAbleSize() > 0) // 输入缓冲区还有数据
            {
                _message_callback(shared_from_this(), &_in_buffer);
            }
            Release();
            return;
        }
        _out_buffer.MoveReadOffset(n);

        if (_out_buffer.ReadAbleSize() == 0) // 刚好发完
        {
            _channel.DisableWrite();     // 关闭对写事件的关系
            if (_statu == DISCONNECTING) // 如果当前是连接待关闭状态，则有数据，发送完数据释放连接，没有数据则直接释放
            {
                Release();
                return;
            }
        }
        return;
    }

    void HandleClose()
    {
        if (_in_buffer.ReadAbleSize() > 0) // 输入缓冲区还有数据
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }
        Release();
    }

    void HandleError()
    {
        HandleClose();
    }

    void HandleAnyEvent()
    {
        if (_enable_inactive_release) // 启动超时销毁的情况下，一有动静就刷新超时任务
        {
            _loop->TimerRefresh(_conn_id);
        }
        if (_anyevent_callback)
        {
            _anyevent_callback(shared_from_this());
        }
    }

    // 连接获取之后，所处的状态下要进行各种设置（启动读监控,调用回调函数）
    void EstablishedInLoop()
    {
        if (_statu != CONNECTING) // 确保连接为半连接状态
            return;
        _statu = CONNECTED;

        _channel.EnableRead();
        if (_connected_callback) // 调用连接建立后的回调
            _connected_callback(shared_from_this());
    }

    // 这个接口才是实际的释放接口
    void ReleaseInLoop()
    {
        _statu = DISCONNECTED;
        _channel.Remove();
        _socket.Close();
        if (_loop->HasTimer(_conn_id)) // 有定时任务的话取消
        {
            CancelInactiveReleaseInLoop();
        }
        if (_closed_callback)
        {
            _closed_callback(shared_from_this());
        }
        if (_server_closed_callback)
        {
            _server_closed_callback(shared_from_this());
        }
    }

    void SendInLoop(Buffer buf)
    {
        if (_statu == DISCONNECTED)
            return;
        _out_buffer.WriteAndPush(buf.ReaderPosition(), buf.ReadAbleSize());
        if (_channel.WriteAble() == false)
        {
            _channel.EnableWrite(); // 启动写事件监控
        }
    }

    void ShutdownInLoop()
    {
        _statu = DISCONNECTING;            // 半关闭状态
        if (_in_buffer.ReadAbleSize() > 0) // 对方接收出错就处理完已经发过来的数据即可
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }
        if (_out_buffer.ReadAbleSize() > 0) // 对方读取时出错就继续发送没发完的数据
        {
            if (_channel.WriteAble() == false)
            {
                _channel.EnableWrite();
            }
        }
        if (_out_buffer.ReadAbleSize() == 0) // 没数据可读可发了，直接释放
        {
            Release();
        }
    }

    // 启动超时销毁
    void EnableInactiveReleaseInLoop(int sec)
    {
        _enable_inactive_release = true;
        if (_loop->HasTimer(_conn_id))
        {
            _loop->TimerRefresh(_conn_id);
            return;
        }
        _loop->TimerAdd(_conn_id, sec, std::bind(&Connection::Release, shared_from_this())); // 没有就添加
    }

    void CancelInactiveReleaseInLoop()
    {
        _enable_inactive_release = false;
        if (_loop->HasTimer(_conn_id))
        {
            _loop->TimerCancel(_conn_id);
        }
    }

    void UpgradeInLoop(const Any &context,
                       const ConnectedCallBack &conn,
                       const MessageCallBack &msg,
                       const ClosedCallBack &closed,
                       const AnyEventCallBack &event)
    {
        _context = context;
        _connected_callback = conn;
        _message_callback = msg;
        _closed_callback = closed;
        _anyevent_callback = event;
    }

public:
    Connection(EventLoop *loop, uint64_t conn_id, int sockfd)
        : _conn_id(conn_id), _sockfd(sockfd), _enable_inactive_release(false), _loop(loop),
          _statu(CONNECTING), _socket(sockfd), _channel(_loop, _sockfd)
    {
        _channel.SetCloseCallback(std::bind(&Connection::HandleClose, this));
        _channel.SetErrorCallback(std::bind(&Connection::HandleError, this));
        _channel.SetEventCallback(std::bind(&Connection::HandleAnyEvent, this));
        _channel.SetReadCallback(std::bind(&Connection::HandleRead, this));
        _channel.SetWriteCallback(std::bind(&Connection::HandleWrite, this));
    }

    ~Connection()
    {
        LOG(LOGLEVEL::DEBUG, "RELEASE CONNECTION:%p", this);
    }

    int Fd()
    {
        return _sockfd;
    }

    uint64_t Id()
    {
        return _conn_id;
    }

    bool Connected()
    {
        return _statu == CONNECTED;
    }

    void SetContext(const Any &context)
    {
        _context = context;
    }

    Any *GetContext()
    {
        return &_context;
    }

    void SetConnectedCallBack(const ConnectedCallBack &cb)
    {
        _connected_callback = cb;
    }

    void SetMessageCallBack(const MessageCallBack &cb)
    {
        _message_callback = cb;
    }

    void SetClosedCallBack(const ClosedCallBack &cb)
    {
        _closed_callback = cb;
    }

    void SetAnyEventCallBack(const AnyEventCallBack &cb)
    {
        _anyevent_callback = cb;
    }

    void SetSrvClosedCallBack(const ClosedCallBack &cb)
    {
        _server_closed_callback = cb;
    }

    void Established()
    {
        _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, shared_from_this()));
    }

    void Send(const char *data, size_t len)
    {
        Buffer buf; // 为了防止Send回去后data已经释放，所以需临时拷贝一份
        buf.WriteAndPush(data, len);
        _loop->RunInLoop(std::bind(&Connection::SendInLoop, shared_from_this(), std::move(buf)));
    }

    void Shutdown()
    {
        _loop->RunInLoop(std::bind(&Connection::ShutdownInLoop, shared_from_this()));
    }

    void Release()
    {
        _loop->RunInLoop(std::bind(&Connection::ReleaseInLoop, shared_from_this()));
    }

    void EnableInactiveRelease(int sec)
    {
        _loop->RunInLoop(std::bind(&Connection::EnableInactiveReleaseInLoop, shared_from_this(), sec));
    }

    void CancelInactiveRelease()
    {
        _loop->RunInLoop(std::bind(&Connection::CancelInactiveReleaseInLoop, shared_from_this()));
    }

    void Upgrade(const Any &context, const ConnectedCallBack &conn, const MessageCallBack &msg,
                 const ClosedCallBack &closed, const AnyEventCallBack &event)
    {
        _loop->AssertInLoop();
        _loop->RunInLoop(std::bind(&Connection::UpgradeInLoop, shared_from_this(), context, conn, msg, closed, event));
    }

private:
    uint64_t _conn_id;             // 连接的唯一值ID(与定时任务id保持一致),方便查找
    int _sockfd;                   // 一个connection对应一个channel的fd
    bool _enable_inactive_release; // 是否启动非活跃销毁
    EventLoop *_loop;              // 连接所属的EventLoop
    ConnStatu _statu;              // 连接状态
    Socket _socket;                // 套接字管理操作
    Channel _channel;              // 连接的事件管理
    Buffer _in_buffer;             // 输入缓冲区
    Buffer _out_buffer;            // 输出缓冲区
    Any _context;                  // 请求的上下文

    ConnectedCallBack _connected_callback;
    MessageCallBack _message_callback;
    ClosedCallBack _closed_callback;
    AnyEventCallBack _anyevent_callback;
    ClosedCallBack _server_closed_callback; // 组件内关闭的回调
};