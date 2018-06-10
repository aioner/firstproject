#ifndef _LOG_H_INCLUDED
#define _LOG_H_INCLUDED

namespace udp_session
{
    struct logger
    {
        static void debug(const char *fmt, ...);
        static void info(const char *fmt, ...);
        static void error(const char *fmt, ...);
        static void set_file(const char *path);
    };
}

#endif //_LOG_H_INCLUDED
