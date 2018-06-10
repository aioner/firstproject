#ifndef SINK_COMMON_H__
#define SINK_COMMON_H__
#include <boost/threadpool.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/timer.hpp>
#include <boost/filesystem.hpp>
#include <tghelper/recycle_pool.h>
#include <tghelper/byte_pool.h>
#include <utility/utility.hpp>
#include <xt_mp_def.h>
#include <iostream>
#include <list>
#include <algorithm>
#include <functional>
#include <rv_adapter/rv_def.h>
#include <rv_adapter/rv_api.h>

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <WinBase.h>
#pragma comment(lib, "ws2_32.lib")
#else //linux
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define sprintf_s snprintf

typedef struct _SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME;

#include <sys/time.h>
inline unsigned long  GetTickCount()
{
    struct timeval tv;
    gettimeofday(&tv,0);
    return tv.tv_sec*1000+tv.tv_usec/1000;
}
#endif//#ifdef _WIN32

#include <boost/date_time/posix_time/posix_time.hpp>
//%04d-%02d-%02d %02d:%02d:%02d:%d
inline std::string get_cur_time_microsec()
{
    boost::posix_time::ptime t = boost::posix_time::microsec_clock::local_time();
    std::string strtime = to_iso_extended_string(t);
    std::string::size_type uiPos = strtime.find("T");
    if (uiPos != std::string::npos)
    {
        strtime.replace(uiPos,1,std::string(" "));
    }
    uiPos = strtime.find(".");
    if (uiPos != std::string::npos)
    {
        strtime.replace(uiPos,1,std::string(":"));
    }
    return strtime;
}

#ifdef _WIN32
#include <Windows.h>
#define LOG_PATH_NAME "D:\\log\\"
#else
#include <stdarg.h>
#define LOG_PATH_NAME "/var/log/xtlog/"
#endif //#ifdef _WIN32
#define MAX_LOG_BUF_SIZE 2048

extern int g_log;
inline void put_log_impl(const char* log_name,const int level,const char *fmt, ...)
{
    if (level <= g_log)
    {
        if (NULL == log_name)
        {
            return;
        }

        va_list args;
        va_start(args, fmt);
        char log_buf[MAX_LOG_BUF_SIZE]={0};
#ifdef _WIN32
        ::vsnprintf_s(log_buf, MAX_LOG_BUF_SIZE,MAX_LOG_BUF_SIZE,fmt, args);
#else
        ::vsnprintf(log_buf, MAX_LOG_BUF_SIZE,fmt, args);
#endif //#ifdef _WIN32
        va_end(args);

        log_buf[MAX_LOG_BUF_SIZE - 1] = 0;
        std::string strlog;
        strlog.append("[");
        strlog.append(get_cur_time_microsec());
        strlog.append("]");
        strlog.append(log_buf);

        char sfilename[256] = {0};
        sprintf(sfilename, LOG_PATH_NAME"xt_mp_sink_dll_%s.txt",log_name);

        FILE *fp = ::fopen(sfilename,"a");
        if(NULL != fp)
        {
            ::fprintf(fp,"%s\n",strlog.c_str());
            ::fclose(fp);
        }
    }
}
#define DEBUG_LOG(log_name,level,fmt,...) put_log_impl(log_name,level,fmt,##__VA_ARGS__)

#define SINK_ERROE "sink_error"
#define SINK_CALL "sink_call"
#define SINK_DATA "recv_data_info"
#define LL_ERROE 1
#define LL_WARNING 2
#define LL_INFO    3
#define LL_NORMAL_INFO 9
#endif //#ifndef SINK_COMMON_H__
