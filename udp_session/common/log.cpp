#include "log.h"

#include <stdarg.h>
#include <string>
#include <fstream>

#if defined(WIN32) || defined(WIN64)
#include <wtypes.h>
#else
#include <time.h>
#endif


std::ofstream g_log;

namespace udp_session
{
    void _debug_log(const char *prefix, const char *fmt, va_list args)
    {
		/*
        char logxx[1024];

#if defined(WIN32) || defined(WIN64)
        SYSTEMTIME st;
        ::GetLocalTime(&st);
        int prefix_len = sprintf_s(logxx, 1024, "[%s][%02d-%02d-%02d %02d:%02d:%02d:%03d]", 
            prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        vsprintf_s(logxx + prefix_len, 1024 - prefix_len, fmt, args);
#else
    time_t now = time(NULL);
    struct tm tm_now;
    ::localtime_r(&now, &tm_now);

    int prefix_len = snprintf(logxx, 1024, "[%s][%02d-%02d-%02d %02d:%02d:%02d]", 
        prefix, tm_now.tm_year, tm_now.tm_mon, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

    vsnprintf(logxx + prefix_len, 1024 - prefix_len, fmt, args);
#endif
        logxx[1023] = 0;
        g_log << logxx << std::endl;
		*/
    }

    void logger::debug(const char *fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        _debug_log("debug", fmt, args);
        va_end(args);
    }

    void logger::info(const char *fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        _debug_log("info", fmt, args);
        va_end(args);
    }

    void logger::error(const char *fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        _debug_log("error", fmt, args);
        va_end(args);
    }

    void logger::set_file(const char *path)
    {
        g_log.open(path);
    }
}
