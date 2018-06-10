#ifndef _XT_MEDIA_CLIENT_COMPAT_H_INCLUEDED
#define _XT_MEDIA_CLIENT_COMPAT_H_INCLUEDED

#include <string.h>
#include <stdio.h>

#ifdef _WIN32
    #define snprintf_s _snprintf_s
    #define strcasecmp _stricmp

    #include <wtypes.h>
    inline void sched_yield()
    {
        if(!::SwitchToThread())
        {
            ::Sleep(0);
        }
    }

    inline void msec_sleep(int ms)
    {
        ::Sleep(ms);
    }

    inline unsigned long getpid()
    {
        return ::GetCurrentProcessId();
    }
#else
    inline int strncpy_s(char *dst, size_t bytes, const char *src, size_t max_size)
    {
        strncpy(dst, src, std::min(bytes, max_size));
        return 0;
    }

    template<size_t N>
    int strncpy_s(char (&dst)[N], const char *src, size_t max_size)
    {
        return strncpy_s(dst, N, src, max_size);
    }

    #define snprintf_s(s, bytes, max_size, fmt, ...) snprintf(s, std::min(bytes, max_size), fmt, ##__VA_ARGS__)

    #define sscanf_s sscanf

    #include <sched.h>
    #include <unistd.h>

    inline void msec_sleep(int ms)
    {
        ::usleep(ms * 1000);
    }
#endif

    template<size_t N, size_t M>
    int strncpy_s(char (&dst)[N], char (&src)[M])
    {
        return strncpy_s(dst, N, src, M);
    }


#endif //_XT_MEDIA_CLIENT_COMPAT_H_INCLUEDED
