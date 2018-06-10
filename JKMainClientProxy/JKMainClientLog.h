#include "logger.h"
#define JK_MIAN_CLIENT_INFO  "JKMainClient_log"

#ifdef _WIN32
#define LOG_PATH "d:\\log\\xt_router\\"
#else
#define LOG_PATH "/var/log/xtlog/xt_router/"
#endif //#ifdef _OS_WINDOWS

#define WRITE_LOG(logger_name, loglevel, format, ...) xt_log_write(std::string(LOG_PATH).append(logger_name).c_str(), loglevel, format, ##__VA_ARGS__)

class CJKMainClientLog
{
protected:
    CJKMainClientLog(void);
    ~CJKMainClientLog(void);
public:
    static CJKMainClientLog* _()
    {
        static CJKMainClientLog m_obj;
        return &m_obj;
    }
public:
    void regist_log_sys();
    void un_log_sys();
};

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
