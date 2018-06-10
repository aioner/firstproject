#include "XTSession.h"
#include "XTSrc.h"

#ifndef CLOSE_SESSION

XTSession XTSession::self;
XTSession::XTSession()
:rtsp_play_(NULL),rtsp_pause_(NULL),
tcp_play_(NULL),tcp_pause_(NULL)
{
}

XTSession::~XTSession(void)
{

}

//rtsp
int XTSession::rtsp_play_cb(void *hSession, int sessionno, int trackid, double npt,float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp)
{
    int ret = 0;

    // edit by zhouzx 2015/07/16
    //////////////////////////////////////////////////////////////////////////
    Msg_Rtsp_Ctrl msg;
    msg.ctrl = RTSP_PLAY;
    msg.session = hSession;
    msg.srcno = sessionno;
    msg.trackid = trackid;
    msg.npt = npt;
    msg.scale = scale;
    msg.rtp_pkt_seq = rtp_pkt_seq;
    msg.rtp_pkt_timestamp = rtp_pkt_timestamp;
    xt_rtsp_ctrl(&msg);
    //////////////////////////////////////////////////////////////////////////

    do 
    {
        if ( NULL == instance()->rtsp_play_)
        {
            ret=-1;
            break;
        }

        xt_track_t track;
        ret = XTSrc::instance()->get_track(sessionno,trackid, track);
        if (ret < 0)
        {
            break;
        }

        ret = instance()->rtsp_play_(sessionno,trackid,track.chanid,npt,scale,rtp_pkt_seq,rtp_pkt_timestamp);

    } while (false);


    return ret;

}
int XTSession::rtsp_pause_cb(void *hSession, int sessionno, int trackid)
{
    int ret = 0;

    do 
    {
        if (NULL == instance()->rtsp_pause_)
        {
            ret=-1;
            break;
        }

        xt_track_t track;
        ret = XTSrc::instance()->get_track(sessionno,trackid, track);
        if (ret < 0)
        {
            break;
        }

        ret = instance()->rtsp_pause_(sessionno,trackid,track.chanid);

    } while (false);

    return ret;

}

//TCP
int XTSession::tcp_play_cb(void* p_context,uint32_t data_chid,double npt,float scale,uint32_t *rtp_pkt_timestamp)
{

    int ret = 0;
    do 
    {
        if ( NULL == instance()->tcp_play_)
        {
            ret = -1;
            break;
        }

        int srcno = data_chid;
        xt_track_t track;
        ret = XTSrc::instance()->get_track(srcno, -1, track);

        if (ret < 0)
        {
            break;
        }

        ret = instance()->tcp_play_(srcno,track.chanid,npt,scale,rtp_pkt_timestamp);


    } while (false);

    return ret;

}
int XTSession::tcp_pause_cb(void* p_context,uint32_t data_chid)
{
    int ret = 0;
    do 
    {
        if ( NULL== instance()->tcp_pause_)
        {
            ret = -1;
            break;
        }
        int srcno = data_chid;
        xt_track_t track;
        ret = XTSrc::instance()->get_track(srcno, -1, track);

        if (ret < 0)
        {
            break;
        }
        ret = instance()->tcp_pause_(srcno,track.chanid);
    } while (false);
    return ret;
}

int XTSession::set_sessionmsg_cb(session_msg_cb func)
{
    return ::xt_set_sessionmsg_cb(func);
}

ms_code_t XTSession::ms_stream_code_type_cast(const long stream_type)
{
    ms_code_t code = ms_code_na;
    switch (stream_type)
    {
    case 0:
        {
            code = ms_code_main;
            break;
        }
    case 1:
        {
            code = ms_code_sub;
            break;
        }
    case 2:
        {
            code = ms_code_audio;
            break;
        }
    default:
        {
            code = ms_code_na;
            break;
        }
    }
    return code;
}

void XTSession::xt_session_query_real_ch_by_request_ch_cb(const int request_ch,const long stream_type,int* real_ch)
{
    ms_code_t code = ms_stream_code_type_cast(stream_type);
    multi_code_mgr::_()->multi_code_query(request_ch,code,real_ch);
}

void XTSession::register_common_cb()
{
    ::xt_regist_query_real_ch_request_callback_func(xt_session_query_real_ch_by_request_ch_cb);
}

int XTSession::init_msg(std::string ip, unsigned short msg_listen_port, unsigned short udp_listen_port,
                        unsigned short send_start_port, const char *mul_start_addr, unsigned short mul_port, xt_print_cb func)
{
    int ret = ::xt_init_msg(ip.c_str(), msg_listen_port,udp_listen_port,send_start_port, mul_start_addr, mul_port, func);
    ::xt_set_tcp_play_cb(tcp_play_cb);
    ::xt_set_tcp_pause_cb(tcp_pause_cb);

    return ret;
}

int XTSession::init_rtsp(std::string ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func)
{
    int ret = ::xt_init_rtsp(ip.c_str(), listen_port, max_session, func);
    ::xt_set_rtsp_play_cb(rtsp_play_cb);
    ::xt_set_rtsp_pause_cb(rtsp_pause_cb);
    return ret;
}

int XTSession::uninit_msg()
{
    return ::xt_uninit_msg();
}

int XTSession::uninit_rtsp()
{
    return ::xt_uninit_rtsp();
}

int XTSession::set_key_data(unsigned long nDataChID, char *pHeadData, long nHeadSize, long nDataType)
{
    return ::xt_set_key_data(nDataChID, pHeadData, nHeadSize, nDataType);
}

int XTSession::set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid)
{
    return ::xtr_set_snd_port(srcno, trackid, port, demux, demuxid);
}

int XTSession::set_snd_port(unsigned long nDataChID, unsigned short sndport, bool demux,unsigned int demuxid)
{
    return ::xtm_set_snd_port(nDataChID, sndport, demux, demuxid);
}

int  XTSession::clear_key_data(unsigned long nDataChID)
{
    return ::xt_clear_key_data(nDataChID);
}

int XTSession::add_src(int srcno)
{
    return ::xtr_add_src(srcno);
}

int XTSession::del_src(int srcno)
{
    return ::xtr_del_src(srcno);
}

int XTSession::set_sdp(int srcno, const char *sdp, int len)
{
    return ::xtr_set_sdp(srcno, sdp, len);
}

int XTSession::set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return ::xt_set_rtsp_heartbit_time(check_timer_interval,time_out_interval);
}
#endif// CLOSE_SESSION