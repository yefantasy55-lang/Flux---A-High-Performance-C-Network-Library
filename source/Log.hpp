#pragma once
#include <cstdio>
#include <ctime>
#include <pthread.h>

enum class LOGLEVEL
{
    INFO = 0,
    DEBUG,
    WARNING,
    ERR
};

#define LOG(level, format, ...)                                                                                                \
    do                                                                                                                         \
    {                                                                                                                          \
        if (level < LOGLEVEL::DEBUG)                                                                                           \
            break;                                                                                                             \
        time_t t = time(nullptr);                                                                                              \
        struct tm tm_buf;                                                                                                      \
        ::localtime_r(&t, &tm_buf);                                                                                            \
        char tmp[32] = {0};                                                                                                    \
        strftime(tmp, 31, "%H:%M:%S", &tm_buf);                                                                                \
        fprintf(stdout, "[%lu %s %s:%d] " format "\n", (unsigned long)pthread_self(), tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
