#include "JKMainClientLog.h"
#include "config.h"

#define LOG_SIZE 500*1024*1024
void CJKMainClientLog ::regist_log_sys()
{
    int level = config::instance()->is_write_JKMainClientLog(0);

    init_log_target(LOG_PATH"JKMainClient_log");

    if (level <= 0)
    {
        set_log_on_off(false);
    }
    else
    {
        set_log_on_off(true);
    }
}

void CJKMainClientLog::un_log_sys()
{
}



CJKMainClientLog::CJKMainClientLog( void )
{

}

CJKMainClientLog::~CJKMainClientLog( void )
{

}
