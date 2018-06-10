//#include "config.h"
#include "h_rtspserver.h"
#include "XTRtspServer.h"
#include "XTEngine.h"
#include <boost/thread.hpp>

boost::thread *g_mainThread = NULL;
bool g_run = true;
xt_print_cb rtsp_svr_print_	= NULL;
xt_rtsp_query_real_ch_by_request_ch_callback_t g_query_real_ch_func=NULL;
void rtsp_query_real_ch(const int request_ch,const long stream_type,int* real_ch)
{
    if (NULL != g_query_real_ch_func)
    {
        g_query_real_ch_func(request_ch,stream_type,real_ch);
    }
}

void RouterRTSPServer(void)
{
    initRTSPServer();

    while(g_run && RTSPServerMainLoop(5))
    {
    }

    termRTSPServer();
    return;
}

// 启动/停止rtsp server
int xt_start_server(const char ip[32], unsigned short listen_port, unsigned int max_session, xt_print_cb print_cb)
{
    g_run = true;
    rtsp_svr_print_ = print_cb;

    XTEngine::instance()->set_listen_addr(ip, listen_port);
    XTEngine::instance()->set_max_session(max_session);

    g_mainThread = new boost::thread(RouterRTSPServer);
    if (!g_mainThread)
    {
        return -1;
    }

    return 0;
}
int xt_stop_server()
{
    g_run = false;
    if (g_mainThread)
    {
        g_mainThread->join();
        delete g_mainThread;
        g_mainThread = NULL;
    }

    return 0;
}

// 设置回调
int set_add_send_cb(add_send_cb func)
{
    return XTEngine::instance()->set_add_send_cb(func);
}

int set_del_send_cb(del_send_cb func)
{
    return XTEngine::instance()->set_del_send_cb(func);
}

int set_rtsp_play_cb(rtsp_play_cb func)
{
    return XTEngine::instance()->set_rtsp_play_cb(func);
}

int set_rtsp_pause_cb(rtsp_pause_cb func)
{
    return XTEngine::instance()->set_rtsp_pause_cb(func);
}
// 创建/删除session
int xt_add_src(int sessionno)
{
    return XTEngine::instance()->add_src(sessionno);
}
int xt_del_src(int sessionno)
{
    return XTEngine::instance()->del_src(sessionno);
}

// 设置SDP
int xt_set_sdp(int sessionno, const char *sdp, int len)
{
    return XTEngine::instance()->set_sdp(sessionno, sdp, len);
}

// 设置track发送端口
int xt_set_snd_port(int sessionno, int trackid, unsigned short port, bool demux,unsigned int demuxid)
{
    return XTEngine::instance()->set_snd_port(sessionno, trackid, port, demux, demuxid);
}

int xt_set_rstp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return  XTEngine::instance()->set_heartbit_time(check_timer_interval,time_out_interval);
}

void xt_register_rtsp_query_real_ch_by_request_ch_callback(xt_rtsp_query_real_ch_by_request_ch_callback_t cb)
{
    g_query_real_ch_func = cb;
}
