#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <memory.h>
#include "Log.hpp"

#define BUFFER_DEFAULT_SIZE 1024

class Buffer
{
public:
    Buffer()
        : _buffer(BUFFER_DEFAULT_SIZE), _reader_idx(0), _writer_idx(0)
    {
    }

    char *Begin()
    {
        return &(*_buffer.begin());
    }

    char *ReaderPosition()
    {
        return Begin() + _reader_idx;
    }

    char *WritePosition()
    {
        return Begin() + _writer_idx;
    }

    // 缓冲区空闲头部大小
    uint64_t HeadIdleSize()
    {
        return _reader_idx;
    }

    // 缓冲区空闲尾部大小
    uint64_t TailIdleSize()
    {
        return _buffer.size() - _writer_idx;
    }

    // 缓冲区可读数据大小
    uint64_t ReadAbleSize()
    {
        return _writer_idx - _reader_idx;
    }

    // 移动读偏移
    void MoveReadOffset(uint64_t len)
    {
        if (len == 0)
            return;
        assert(len <= ReadAbleSize());
        _reader_idx += len;
    }

    // 移动写偏移
    void MoveWriteOffset(uint64_t len)
    {
        if (len == 0)
            return;
        assert(len <= TailIdleSize());
        _writer_idx += len;
    }

    // 确保有足够的空间写入
    void EnsureWriteSpace(uint64_t len)
    {
        if (len <= TailIdleSize()) // 尾部空间有足够空间插入
        {
            return;
        }
        else if (len <= HeadIdleSize() + TailIdleSize()) // 总体空间足够移动数据
        {
            uint64_t rsz = ReadAbleSize();
            memmove(Begin(), ReaderPosition(), rsz);
            _reader_idx = 0;
            _writer_idx = rsz;
        }
        else // 扩容
        {
            // 翻倍扩容，避免频繁的 memcpy
            uint64_t new_size = std::max(_buffer.size() * 2, _writer_idx + len);
            // 限制单个缓冲区最大 1MB，防止恶意请求撑爆内存
            if (new_size > 1024 * 1024)
                new_size = 1024 * 1024;
            LOG(LOGLEVEL::DEBUG, "RESIZE %ld", new_size);
            _buffer.resize(new_size);
        }
    }

    void Read(void *buf, uint64_t len)
    {
        if (len == 0)
            return;
        assert(len <= ReadAbleSize());
        std::copy(ReaderPosition(), ReaderPosition() + len, (char *)buf);
    }

    void ReadAndPop(void *buf, uint64_t len)
    {
        Read(buf, len);
        MoveReadOffset(len);
    }

    std::string ReadAsString(uint64_t len)
    {
        if (len == 0)
            return std::string();
        assert(len <= ReadAbleSize());
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }

    std::string ReadAsStringAndPop(uint64_t len)
    {
        std::string str = ReadAsString(len);
        MoveReadOffset(len);
        return str;
    }

    void Write(const void *data, uint64_t len)
    {
        if (len == 0)
            return;
        EnsureWriteSpace(len);
        const char *d = (const char *)data;
        std::copy(d, d + len, WritePosition());
    }

    void WriteAndPush(const void *data, uint64_t len)
    {
        Write(data, len);
        MoveWriteOffset(len);
    }

    void WriteString(const std::string &data)
    {
        Write(data.data(), data.size());
    }

    void WriteStringAndPush(const std::string &data)
    {
        WriteString(data);
        MoveWriteOffset(data.size());
    }

    void WriteBuffer(Buffer &data)
    {
        if (this == &data)
        {
            std::string temp = ReadAsString(ReadAbleSize());
            WriteString(temp);
        }
        else
        {
            Write(data.ReaderPosition(), data.ReadAbleSize());
        }
    }

    void WriteBufferAndPush(Buffer &data)
    {
        WriteBuffer(data);
        MoveWriteOffset(data.ReadAbleSize());
    }

    // 找换行符
    const char *FindCRLF()
    {
        char *res = (char *)memchr(ReaderPosition(), '\n', ReadAbleSize());
        return res;
    }

    // 获取一行数据
    std::string GetLine()
    {
        const char *pos = FindCRLF();
        if (pos == nullptr)
            return std::string();
        return ReadAsString(pos - ReaderPosition() + 1); // 带出换行符
    }

    std::string GetLineAndPop()
    {
        std::string str = GetLine();
        MoveReadOffset(str.size());
        return str;
    }

    // 清空缓冲区
    void Clear()
    {
        _reader_idx = 0;
        _writer_idx = 0;
        // 如果缓冲区过大且当前没有数据，缩容回初始大小
        if (_buffer.capacity() > BUFFER_DEFAULT_SIZE && ReadAbleSize() == 0)
        {
            std::vector<char> tmp(BUFFER_DEFAULT_SIZE);
            _buffer.swap(tmp); // 真正释放内存
        }
    }

    ~Buffer() {}

private:
    std::vector<char> _buffer;
    uint64_t _reader_idx; // 读偏移
    uint64_t _writer_idx; // 写偏移
};