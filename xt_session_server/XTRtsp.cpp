#ifdef _USE_XT_RTSP_SESSION_SERVER
#include "XTRtsp.h"
#include "h_rtspserver.h"
#include "h_xtsessionserver.h"
#include <stdio.h>
#include <string.h>

extern session_msg_cb g_fnSessionMsg;
extern void query_real_ch(const int request_ch,const long stream_type,int* real_ch);

XTRtsp XTRtsp::self;
XTRtsp::XTRtsp(void)
{
}

XTRtsp::~XTRtsp(void)
{
}

void RTSPSERVER_STDCALL xt_rtsp_query_real_ch_by_request_ch_cb(const int request_ch,const long stream_type,int* real_ch)
{
    query_real_ch(request_ch,stream_type,real_ch);
}

int XTRtsp::init(const string& ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func)
{
    int ret = -1;
    ret = xt_start_server(ip.c_str(), listen_port, max_session, func);
    set_add_send_cb(add_send_proc);
    set_del_send_cb(del_send_proc);
    set_rtsp_play_cb(rtsp_play_cb_func);
    set_rtsp_pause_cb(rtsp_pause_cb_func);
    xt_register_rtsp_query_real_ch_by_request_ch_callback(xt_rtsp_query_real_ch_by_request_ch_cb);
    return ret;
}
int XTRtsp::uninit(){ return xt_stop_server();}

// 建立转发
int SESSIONSERVER_STDCALL XTRtsp::add_send_proc(void *hsession, int srcno, int trackid, const char *pstrRAddr,unsigned short nRPort, bool multiplex,unsigned int multiplexID)
{
    Msg_Rtsp msg;
    msg.ctrl = RTSP_STARTPLAY;
    msg.session = hsession;
    msg.srcno = srcno;
    msg.trackid = trackid;
    (void)strncpy(msg.pstrRAddr, pstrRAddr, SESSION_SERVER_IP_LEN);
    msg.nRPort = nRPort;
    msg.multiplex = multiplex;
    msg.multiplexID = multiplexID;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_RTSP, &msg);
    }

    return 0;
}

// 删除转发
int SESSIONSERVER_STDCALL XTRtsp::del_send_proc(void *hsession,			//session
                                                int srcno,			//sessionid
                                                int trackid,  				//track id
                                                const char *pstrRAddr, 	//IP
                                                unsigned short nRPort, 	//port
                                                bool multiplex,			//multiplex
                                                unsigned int multiplexID)	//multiplexID
{
    Msg_Rtsp msg;
    msg.ctrl = RTSP_STOPPLAY;
    msg.session = hsession;
    msg.srcno = srcno;
    msg.trackid = trackid;
    (void)strncpy(msg.pstrRAddr, pstrRAddr, SESSION_SERVER_IP_LEN);
    msg.nRPort = nRPort;
    msg.multiplex = multiplex;
    msg.multiplexID = multiplexID;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_RTSP, &msg);
    }

    return 0;
}
int XTRtsp::rtsp_play_cb_func(void *hSession, int sessionno, int trackid, 
                              double npt, float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp)
{
    int ret_code = 0;

    Msg_Rtsp_Ctrl msg;
    msg.ctrl = RTSP_PLAY;
    msg.session = hSession;
    msg.srcno = sessionno;
    msg.trackid = trackid;
    msg.npt = npt;
    msg.scale = scale;
    msg.rtp_pkt_seq = rtp_pkt_seq;
    msg.rtp_pkt_timestamp = rtp_pkt_timestamp;

    if (g_fnSessionMsg)
    {
        ret_code = g_fnSessionMsg(PT_CTRL_RTSP,static_cast<void*>(&msg));
    }
    else
    {
        ret_code = -1;
    }

    return ret_code;
}
int XTRtsp::rtsp_pause_cb_func(void *hSession, int sessionno, int trackid)
{
    int ret_code = 0;

    Msg_Rtsp_Ctrl msg;
    msg.ctrl = RTSP_PAUSE;
    msg.session = hSession;
    msg.srcno = sessionno;
    msg.trackid = trackid;
    msg.npt = 0;
    msg.scale = 0;
    msg.rtp_pkt_seq = NULL;
    msg.rtp_pkt_timestamp = NULL;

    if (g_fnSessionMsg)
    {
        ret_code = g_fnSessionMsg(PT_CTRL_RTSP,static_cast<void*>(&msg));
    }
    else
    {
        ret_code = -1;
    }

    return ret_code;
}

// 增加/删除源
int XTRtsp::add_src(int srcno)
{
    return xt_add_src(srcno);
}

int XTRtsp::del_src(int srcno)
{
    return xt_del_src(srcno);
}

// 设置SDP
int XTRtsp::set_sdp(int srcno, const char *sdp, int len)
{
    return xt_set_sdp(srcno, sdp, len);
}

// 设置发送端口
int XTRtsp::set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid)
{
    return xt_set_snd_port(srcno, trackid, port, demux, demuxid);
}
int XTRtsp::set_play_cb(rtsp_play_cb func)
{
    return set_rtsp_play_cb(func);
}
int XTRtsp::set_pause_cb(rtsp_pause_cb func)
{
    return ::set_rtsp_pause_cb(func);
}

int XTRtsp::set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return ::xt_set_rstp_heartbit_time(check_timer_interval,time_out_interval);
}

#endif //_USE_XT_RTSP_SESSION_SERVER
