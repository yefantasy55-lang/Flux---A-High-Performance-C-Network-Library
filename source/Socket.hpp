#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <cerrno>
#include "Log.hpp"

#define MAX_LISTEN 1024

#define SOCKET_UDP SOCK_DGRAM  // 数据报
#define SOCKET_TCP SOCK_STREAM // 字节流

class Socket
{
public:
    Socket()
        : _sockfd(-1)
    {
    }

    Socket(int fd) : _sockfd(fd)
    {
    }

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    int Fd()
    {
        return _sockfd;
    }

    bool Create(int type)
    {
        _sockfd = socket(AF_INET, type, 0);
        if (_sockfd < 0)
        {
            LOG(LOGLEVEL::ERR, "Create Socket Failed!!");
            return false;
        };
        return true;
    }

    bool Bind(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        // addr.sin_addr.s_addr = inet_addr(ip.c_str());
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        socklen_t len = sizeof(addr);
        int n = bind(_sockfd, (struct sockaddr *)&addr, len);
        if (n < 0)
        {
            LOG(LOGLEVEL::ERR, "Bind Socket Failed!!!");
            return false;
        }
        return true;
    }

    bool Listen(int backlog = MAX_LISTEN)
    {
        int n = listen(_sockfd, backlog);
        if (n < 0)
        {
            LOG(LOGLEVEL::ERR, "Listen Socket Failed!!!");
            return false;
        }
        return true;
    }

    bool Connect(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        socklen_t len = sizeof(addr);
        int n = connect(_sockfd, (struct sockaddr *)&addr, len);
        if (n < 0)
        {
            LOG(LOGLEVEL::ERR, "Connect Socket Failed!!!");
            return false;
        }
        return true;
    }

    int Accept()
    {
        socklen_t len = sizeof(_client);
        int sockfd = accept(_sockfd, (struct sockaddr *)&_client, &len);
        if (sockfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                return -1;
            }
            LOG(LOGLEVEL::ERR, "Accept Socket Failed!!!");
            return -1;
        }
        return sockfd;
    }

    std::string GetPeerString() const
    {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &_client.sin_addr, ip, sizeof(ip));
        return std::string(ip) + ":" + std::to_string(ntohs(_client.sin_port));
    }

    ssize_t Recv(void *buf, size_t len, int flag = 0)
    {
        ssize_t n = recv(_sockfd, buf, len, flag);
        if (n <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                return 0;
            }
            else
            {
                LOG(LOGLEVEL::ERR, "Recv Socket Failed!!!");
                return -1;
            }
        }
        return n;
    }

    ssize_t NonBlockRecv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT是本次不阻塞
    }

    ssize_t Send(const void *buf, size_t len, int flag = 0)
    {
        int n = send(_sockfd, buf, len, flag);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                return 0;
            }
            else
            {
                LOG(LOGLEVEL::ERR, "Send Socket Failed!!!");
                return -1;
            }
        }
        return n;
    }

    ssize_t NonBlockSend(void *buf, size_t len)
    {
        return Send(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT 表示当前发送为非阻塞。
    }

    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }

    bool CreateServer(const std::string &ip = "0.0.0.0", uint16_t port = 8080, bool block_flag = false)
    {
        if (!Create(SOCKET_TCP))
            return false;
        ReuseAddress();
        if (block_flag)
            NonBlock(); // 设置非阻塞
        if (!Bind(ip, port))
            return false;
        if (!Listen(MAX_LISTEN))
            return false;
        return true;
    }

    bool CreateClient(const std::string &ip, uint16_t port)
    {
        if (!Create(SOCKET_TCP))
            return false;
        if (!Connect(ip, port))
            return false;
        return true;
    }

    // 设置套接字选项---开启地址端口重用
    void ReuseAddress()
    {
        // val = 1 表示开启SO_REUSEADDR
        int val = 1;
        int n = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val)); // 重新使用ip
        if (n < 0)
        {
            LOG(LOGLEVEL::WARNING, "Reuse Address Failed!!!");
        }
        val = 1;
        n = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(val)); // 重新使用port
        if (n < 0)
        {
            LOG(LOGLEVEL::WARNING, "Reuse Port Failed!!!");
        }
    }

    void NonBlock()
    {
        int f1 = fcntl(_sockfd, F_GETFL);
        if (f1 < 0)
        {
            LOG(LOGLEVEL::WARNING, "NonBlock Socket Failed");
            return;
        }
        fcntl(_sockfd, F_SETFL, f1 | O_NONBLOCK);
    }

    ~Socket()
    {
        Close();
    }

private:
    int _sockfd;
    struct sockaddr_in _client;
};