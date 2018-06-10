#include "xt_regist_client.h"
#include <stdio.h>

#ifndef CLOSE_SESSION

xt_regist_client xt_regist_client::m_regist_client;
xt_regist_client::regist_info xt_regist_client::rinfo;
//static
xt_regist_client::xt_regist_client()
{

}
int xt_regist_client::regist(const char* regist_ids,const char* server_ip, unsigned short server_port,uint32_t millisec)
{
    rinfo.m_regist_ids.assign(regist_ids);
    rinfo.m_server_ip.assign(server_ip);
    rinfo.m_server_port = server_port;
    rinfo.m_millisec = millisec;
    m_brun = true;
    xt_regist_thread = new boost::thread(xt_regist_work_thread, &rinfo);
    if (xt_regist_thread == NULL)
    {
        return -1;
    }

    return  0;
}

void xt_regist_client::xt_regist_work_thread(void *param)
{
    regist_info rinfo = *((regist_info *)param);
    while (instance()->m_brun)
    {
        xtr_regist(rinfo.m_regist_ids.c_str(),rinfo.m_server_ip.c_str(),rinfo.m_server_port, rinfo.m_millisec);

        boost::this_thread::sleep_for(boost::chrono::milliseconds(3000));
    }

}

int xt_regist_client::stop_regist(const char* server_ip, unsigned short server_port,uint32_t millisec)
{ 
    m_brun = false;
    if (xt_regist_thread)
    {
        xt_regist_thread->join();
    }

    return xtr_stop_regist(server_ip,server_port,millisec);
}

void xt_regist_client::set_regist_response_callback_t( regist_response_callback_t func )
{
    ::regist_response_callback(func);

}

#endif


