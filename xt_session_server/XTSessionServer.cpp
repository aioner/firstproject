#include "h_xtsessionserver.h"
#include "XTMsg.h"
#include "XTRtsp.h"
#include "XTUDPMsg.h"
#include <stdio.h>
#include <stdarg.h>

session_msg_cb g_fnSessionMsg = NULL;
xt_print_cb session_svr_print_ = NULL;
xt_query_real_ch_by_request_ch_callback_t g_query_real_ch_cb = NULL;

void query_real_ch(const int request_ch,long stream_type,int* real_ch)
{
    if (NULL != g_query_real_ch_cb)
    {
        g_query_real_ch_cb(request_ch,stream_type,real_ch);
    }
}

void SESSION_SVR_PRINT(const xt_log_level ll,const char* format,...)
{
    if (session_svr_print_)
    {
        char context[4096] = {0};

        va_list arg;
        va_start(arg, format);
        vsnprintf(context, sizeof(context)-1, format, arg);
        va_end(arg);

        session_svr_print_("session_svr", ll, context);
    }	
}

int xt_set_sessionmsg_cb(session_msg_cb func)
{
    g_fnSessionMsg = func;

    return 0;
}

int xt_init_msg(const char *ip,					
                unsigned short msg_listen_port,	
                unsigned short udp_listen_port,	
                unsigned short send_start_port,	
                const char *mul_start_addr,		
                unsigned short mul_port,
                xt_print_cb func)
{
    int ret = 0;
#ifdef _CLOSE_UDP_TCP_SESSION
    return 0;
#else

    session_svr_print_ = func;

    if (udp_listen_port > 0)
    {
        ret = XTUDPMsg::inst()->init(ip, udp_listen_port, send_start_port, func);
    }

#ifdef _USE_XT_TCP_SESSION_SERVER
    if (msg_listen_port > 0)
    {
        ret = XTMsg::instance()->init(ip, msg_listen_port, send_start_port, mul_start_addr, mul_port, func);
    }
#endif //_USE_XT_TCP_SESSION_SERVER

#endif

    return ret;
}

#ifdef _USE_XT_TCP_SESSION_SERVER
int xt_init_rtsp(const char ip[SESSION_SERVER_IP_LEN], unsigned short listen_port, unsigned int max_session, xt_print_cb func)
{
    session_svr_print_ = func;
    return XTRtsp::instance()->init(ip, listen_port, max_session, func);
}
#endif

int xt_uninit_msg()
{
#ifdef _CLOSE_UDP_TCP_SESSION
    return 0;
#else
    XTUDPMsg::inst()->uninit();

#ifdef _USE_XT_TCP_SESSION_SERVER
    XTMsg::instance()->uninit();
#endif

    return 0;
#endif
}

#ifdef _USE_XT_RTSP_SESSION_SERVER
int xt_uninit_rtsp()
{
    return XTRtsp::instance()->uninit();;
}
#endif

int xt_set_key_data(unsigned long nDataChID, char *pHeadData, long nHeadSize, long nDataType)
{
#ifdef _CLOSE_UDP_TCP_SESSION
    return 0;
#else
    XTUDPMsg::inst()->set_key_data(nDataChID, pHeadData, nHeadSize, nDataType);

#ifdef _USE_XT_TCP_SESSION_SERVER
    XTMsg::instance()->set_key_data(nDataChID, pHeadData, nHeadSize, nDataType);
#endif

    return 0;
#endif
}

int xtm_set_snd_port(unsigned long nDataChID, unsigned short sndport, bool multiplex,unsigned int multid)
{
#ifdef _CLOSE_UDP_TCP_SESSION
    return 0;
#else
    XTUDPMsg::inst()->set_mult_data(nDataChID, sndport, multiplex, multid);

#ifdef _USE_XT_TCP_SESSION_SERVER
    XTMsg::instance()->set_snd_port(nDataChID, sndport, multiplex, multid);
#endif

    return 0;
#endif
}

int  xt_clear_key_data(unsigned long nDataChID)
{
#ifdef _CLOSE_UDP_TCP_SESSION
    return 0;
#else

#if _USE_XT_TCP_SESSION_SERVER
    XTMsg::instance()->clear_key_data(nDataChID);
#endif
    return 0;
#endif
}

#ifdef _USE_XT_RTSP_SESSION_SERVER
int xtr_add_src(int srcno)
{
    return XTRtsp::instance()->add_src(srcno);
}

int xtr_del_src(int srcno)
{
    return XTRtsp::instance()->del_src(srcno);
}

int xtr_set_sdp(int srcno, const char *sdp, int len)
{
    return XTRtsp::instance()->set_sdp(srcno, sdp, len);
}

int xtr_set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid)
{
    return XTRtsp::instance()->set_snd_port(srcno, trackid, port, demux, demuxid);
}
#endif // _USE_XT_RTSP_SESSION_SERVER

int xt_msg_add_socket_client(void *sock)
{
#ifdef _CLOSE_UDP_TCP_SESSION
    return 0;
#else

#ifdef _USE_XT_TCP_SESSION_SERVER
    return XTMsg::instance()->add_socket_client(sock);
#endif
    return 0;
#endif
}

#ifdef _USE_XT_RTSP_SESSION_SERVER
int xt_set_rtsp_play_cb(rtsp_play_cb func)
{
    return XTRtsp::instance()->set_play_cb(func);

}
int xt_set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return XTRtsp::instance()->set_rtsp_heartbit_time(check_timer_interval,time_out_interval);
}
int xt_set_rtsp_pause_cb(rtsp_pause_cb func)
{
    return XTRtsp::instance()->set_pause_cb(func);
}
#endif //_USE_XT_RTSP_SESSION_SERVER

#ifdef _USE_XT_TCP_SESSION_SERVER
int xt_set_tcp_play_cb(tcp_play_cb_type func)
{
    //return  XTMsg::instance()->set_play_cb(func);
    return 0;

}
int xt_set_tcp_pause_cb(tcp_pause_cb_type func)
{
    //return  XTMsg::instance()->set_pause_cb(func);
    return 0;
}
#endif

int xtr_regist(const char *regist_ids,const char *server_ip, unsigned short server_port,uint32_t millisec)
{
    return XTUDPMsg::inst()->xt_regist(regist_ids,server_ip,server_port,millisec);
}
int xtr_stop_regist( const char* server_ip, unsigned short server_port,uint32_t millisec )
{
    return XTUDPMsg::inst()->xt_stop_regist(server_ip,server_port,millisec);
}


void  regist_response_callback(regist_response_callback_t func)
{
    XTUDPMsg::inst()->set_regist_response_callback(func);
}

void xt_regist_query_real_ch_request_callback_func(xt_query_real_ch_by_request_ch_callback_t cb)
{
    g_query_real_ch_cb = cb;
}
