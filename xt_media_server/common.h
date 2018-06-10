#ifndef COMMON_H__INCLUDE
#define COMMON_H__INCLUDE
#include <boost/date_time/posix_time/posix_time.hpp>

#if (defined(WIN32) || defined(WIN64))  && !defined(_OS_WINDOWS)
#define _OS_WINDOWS
#endif//#if (defined(WIN32) || defined(WIN64))  && !defined(OS_WINDOWS)

namespace common{

    typedef boost::posix_time::ptime Time_Type;
    typedef boost::posix_time::time_duration time_duration_t;//时间长度类型
    //%04d-%02d-%02d %02d:%02d:%02d:%d
    inline std::string ToStrMicrosecByPtime(const Time_Type& t)
    {
        std::string strTime = to_iso_extended_string(t);
        std::string::size_type uiPos = strTime.find("T");	
        if (uiPos != std::string::npos)
        {
            strTime.replace(uiPos,1,std::string(" "));
        }

        uiPos = strTime.find(".");
        if (uiPos != std::string::npos)
        {
            strTime.replace(uiPos,1,std::string(":"));
        }

        return strTime;
    }

    //%04d-%02d-%02d %02d:%02d:%02d
    inline std::string ToStrSecondByPtime(const Time_Type& t)
    {
        std::string strTime = to_iso_extended_string(t);
        std::string::size_type uiPos = strTime.find("T");
        if (uiPos != std::string::npos)
        {
            strTime.replace(uiPos,1,std::string(" "));
        }

        return strTime;
    }

    //获取当时时间microsec_级
    inline Time_Type GetCurTimeMicrosecValue()
    {
        return boost::posix_time::microsec_clock::local_time();
    }

    //获取当时时间Second_级
    inline Time_Type GetCurTimeSecondValue()
    {
        return boost::posix_time::second_clock::local_time();
    }

    //获取当前时间字符串
    inline std::string GetCurTime()
    {
        //获取创创时间
        return ToStrMicrosecByPtime(GetCurTimeMicrosecValue());
    }

    //任意类型转string
    template <typename T>
    inline std::string Type2Str(T& Src)
    {
        std::ostringstream os;
        os<<Src;
        return os.str();
    }

}//namespace common

#define LIB_LOG_KEY "xt_media_sever |"
#if defined(_WIN32) || defined(_OS_WINDOWS) ||	defined(_WIN64) 
#include <Windows.h>
#define MAX_LOG_BUF_SIZE 2048

inline void put_log_impl(const char *fmt, ...)
{

    va_list args;
    va_start(args, fmt);
    char log_buf[MAX_LOG_BUF_SIZE];
    ::vsnprintf_s(log_buf, MAX_LOG_BUF_SIZE, MAX_LOG_BUF_SIZE, fmt, args);
    ::OutputDebugStringA(log_buf);
    va_end(args);

}

#define DEBUG_LOG(fmt, ...) put_log_impl(LIB_LOG_KEY fmt,##__VA_ARGS__)

#else
//#include <stdarg.h>
#define DEBUG_LOG(fmt, ...)  ::printf(LIB_LOG_KEY fmt,##__VA_ARGS__)
#endif //#ifdef _WIN32


#endif//COMMON_H__INCLUDE