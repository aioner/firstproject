#ifndef XTROUTERLOG_H
#define XTROUTERLOG_H
/*
*日志相关定义文件
*/
#include "logger.h"
//点播信息日志记录
#define  DBLOGINFO  "db_info"

//XmppGlooxApply.dll日志
#define XmppGlooxApplyLog "xmpp"

#define XTROUTER_ZL "zl"

#include "common_type.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <stdarg.h>
#endif //#ifdef _WIN32

#include "web_srv_mgr.h"

#define MAX_LOG_BUF_SIZE 4096
#define LOG_HEAD_FLAG "XTRouter| "

#ifdef _WIN32
#define LOG_PATH "d:\\log\\xt_router\\"
#else
#define LOG_PATH "/var/log/xtlog/xt_router/"
#endif

#define WRITE_LOG(logger_name, loglevel, format, ...) xt_log_write(std::string(LOG_PATH).append(logger_name).c_str(), loglevel, format, __VA_ARGS__)
#define DEBUG_LOG(logger_name, loglevel,fmt, ...) put_log_impl(logger_name, loglevel,fmt,##__VA_ARGS__)

inline void put_log_impl(const char* logger_name, const severity_level log_leve,const char *fmt, ...)
{
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

    if (logger_name != NULL)
    {
        WRITE_LOG(logger_name,log_leve,"%s",log_buf);
    }
    
    std::string t = get_cur_time_microsec();
    if (t.empty())
    {
        return;
    }

    std::string strlog;
    strlog.append("[");
    strlog.append(t.c_str());
    strlog.append("]");
    strlog.append(LOG_HEAD_FLAG);
    strlog.append(log_buf);
    strlog.append("\n");

    if (NULL == logger_name)
    {
        ::printf(strlog.c_str());
    }

#ifdef _USE_WEB_SRV_
    web_srv_mgr::_()->write_log(strlog);
#endif//_USE_WEB_SRV_
}

#endif // XTROUTERLOG_H