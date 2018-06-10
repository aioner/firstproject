#include "h_xtmediaserver.h"
#include "xt_session_sip.h"
#include "XTTcp.h"
#include "XTRtp.h"
#include "XTSrc.h"
#include "XTChan.h"
#include "RunInfoMgr.h"
#include "XTSingleSrc.h"
#include "xt_regist_client.h"
#include "h_xtsessionserver.h"
#include "stdarg.h"

#ifndef CLOSE_SESSION
#include "XTSession.h"
#endif

const char* XT_MEDIA_SERVER_LIB_INFO = "XT_Lib_Version: V_XT_MEDIA_SERVER_4.27.0226.0";

#define KEY_FRAME_TYPE 128 // 系统头帧类型

// 服务配置
MS_CFG ms_cfg;

int xt_rtsp_process(Msg_Rtsp *msgData);
int std_sendtask_process(std_send_task *task);

void XT_STDCALL MEDIA_SVR_PRINT(const xt_log_level ll,const char* format,...)
{
    if (ms_cfg.xt_media_server_log_cb)
    {
        char context[4096] = {0};

        va_list arg;
        va_start(arg, format);
        vsnprintf(context, sizeof(context)-1, format, arg);
        va_end(arg);

        log_level_type level;
        if (ll==level_error)
        {
            level = log_err;
        }
        else if (ll==level_warn)
        {
            level = log_warn;
        }
        else if (ll>=level_info)
        {
            level = log_info;
        }
        else
        {
            return;
        }

        ms_cfg.xt_media_server_log_cb("media_svr", level, context, strlen(context));
    }
}
void XT_STDCALL MEDIA_SVR_PRINT2(const char* logger_name,const xt_log_level ll,const char* context)
{
    if (ms_cfg.xt_media_server_log_cb)
    {
        log_level_type level;
        if (ll==level_error)
        {
            level = log_err;
        }
        else if (ll==level_warn)
        {
            level = log_warn;
        }
        else if (ll>=level_info)
        {
            level = log_info;
        }
        else
        {
            return;
        }
        ms_cfg.xt_media_server_log_cb((char*)logger_name, level, (char*)context, strlen(context));
    }
}

//初始化库的基础配置
void init_media_server_cfg(MS_CFG &cfg)
{
    ms_cfg.num_chan= cfg.num_chan;
    ::strncpy(ms_cfg.ip,cfg.ip,MEDIA_SERVER_IP_LEN);
    ms_cfg.snd_start_port= cfg.snd_start_port;
    ms_cfg.demux= cfg.demux; 
    ::strncpy(ms_cfg.mul_start_ip,cfg.mul_start_ip,MEDIA_SERVER_IP_LEN);
    ms_cfg.msg_liten_port= cfg.msg_liten_port;
    ms_cfg.rtsp_listen_port= cfg.rtsp_listen_port;
    ms_cfg.tcp_listen_port= cfg.tcp_listen_port;
    ms_cfg.udp_listen_port= cfg.udp_listen_port;
    ms_cfg.snd_std_rtp= cfg.snd_std_rtp;
    ms_cfg.sink_single= cfg.sink_single;
    ms_cfg.rtsp_play_cb= cfg.rtsp_play_cb;
    ms_cfg.rtsp_pause_cb= cfg.rtsp_pause_cb;
    ms_cfg.tcp_play_cb= cfg.tcp_play_cb;
    ms_cfg.tcp_pause_cb= cfg.tcp_pause_cb;
    ms_cfg.xt_link_state_event= cfg.xt_link_state_event;
    ms_cfg.xt_media_server_log_cb= cfg.xt_media_server_log_cb;
    ms_cfg.rtcp_force_iframe_cb = cfg.rtcp_force_iframe_cb;
    ms_cfg.use_traffic_shaping = cfg.use_traffic_shaping;
}

extern int _xt_create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid);
extern int xt_create_singlesrc(int srcno_prime, int &srcno);

#ifndef CLOSE_SESSION
// 私有会话协商处理
int xt_msg_process(STR_MsgData *msgData)
{
    if ( NULL == msgData) return -1;
    int trans_ch = -1;
    //if (multi_code_mgr::_()->multi_code_query(msgData->nDataChannelID,ms_code_na,&trans_ch) < 0)
    {
        trans_ch = msgData->nDataChannelID;
    }

    switch (msgData->ctrl)
    {
    case MSG_STARTPLAY://启动点播消息
        { 
            MEDIA_SVR_PRINT(level_info, "MSG_STARTPLAY: nDataChannelID[%d]Addr[%s]nRecvPort[%d]nMode[%d]nSSRC[%d]multiplex[%d]multiplexID[%d]",
                trans_ch, msgData->Addr,msgData->nRecvPort,msgData->nMode,msgData->nSSRC,msgData->multiplex,msgData->multiplexID);
            do
            {
                unsigned long chid = trans_ch;
                int srcno_prime = chid;
                int srcno = chid;

                if (ms_cfg.sink_single)
                {
                    //获得空闲转发源
                    int ret = XTSingleSrc::inst()->get_freesrc(srcno_prime, srcno);
                    if (ret < 0)
                    {
                        //创建转发源
                        ret = xt_create_singlesrc(srcno_prime, srcno);
                        XTSingleSrc::inst()->use_src(srcno, true);
                    }
                    if (ret >= 0)
                    {
                        xt_track_t pri_track;
                        ret = XTSrc::_()->get_track(srcno, PRI_TRACK_ID, pri_track);
                        chid = pri_track.chanid;
                    }
                    else
                    {
                        MEDIA_SVR_PRINT(level_error, "xt_create_singlesrc fail:src[%d] ret[%d]", srcno_prime, ret);
                        break;
                    }
                    MEDIA_SVR_PRINT(level_info, "xt_create_singlesrc success!prime_srcno[%d] srcno[%d]", srcno_prime, srcno);
                }

                xt_sink_t sink;
                sink.session = msgData->pMsgContext;
                sink.srcno_prime = srcno_prime;
                sink.chanid = chid;
                sink.addr = msgData->Addr;
                sink.port = msgData->nRecvPort;
                sink.demux = msgData->multiplex;
                sink.demuxid = msgData->multiplexID;

                int ret = XTSrc::_()->add_sink(srcno, sink);
                ret = XTRtp::_()->add_send(chid, msgData->Addr, msgData->nRecvPort, msgData->pMsgContext, msgData->nMode, msgData->nSSRC, msgData->multiplex, msgData->multiplexID);

                if (NULL != ms_cfg.xt_link_state_event) ms_cfg.xt_link_state_event(EVENT_TCP_PLAY,srcno);
            }while (false);

            break;
        }
    case MSG_STOPPLAY://停止点播消息
        {
            MEDIA_SVR_PRINT(level_info, "MSG_STOPPLAY: nDataChannelID[%d]Addr[%s]nRecvPort[%d]nMode[%d]multiplex[%d]multiplexID[%d]",
                trans_ch,msgData->Addr,msgData->nRecvPort,msgData->nMode, msgData->multiplex, msgData->multiplexID);
            int srcno = trans_ch;
            xt_sink_t sink;
            //modified by lichao, 20141231 需要加通道
            sink.srcno_prime = trans_ch;
            sink.session = msgData->pMsgContext;
            sink.addr = msgData->Addr;
            sink.port = msgData->nRecvPort;
            sink.demux = msgData->multiplex;
            sink.demuxid = msgData->multiplexID;
            if (ms_cfg.sink_single)
            {
                //销毁对应转发源
                int del_srcno = -1;
                if (XTSrc::_()->get_srcno_by_sink(del_srcno, sink) == 0)
                {
                    int ret = XTSingleSrc::inst()->del_src2(del_srcno);
                    if (ret < 0)
                    {
                        MEDIA_SVR_PRINT(level_error, "XTSingleSrc::inst()->del_src2 fail: src[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                            del_srcno, sink.addr.c_str(), sink.port, sink.demux, sink.demuxid);
                    }
                    ret = xt_destroy_src(del_srcno);
                    if (ret < 0)
                    {
                        MEDIA_SVR_PRINT(level_error, "xt_destroy_src fail: src[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                            del_srcno, sink.addr.c_str(), sink.port, sink.demux, sink.demuxid);
                    }
                }
                else
                {
                    MEDIA_SVR_PRINT(level_error, "destroy src fail(not find): ip[%s] port[%d] demux[%d] demuxid[%d]",
                        sink.addr.c_str(), sink.port, sink.demux, sink.demuxid);
                }
            }
            XTSrc::_()->del_sink(srcno, sink);
            XTRtp::_()->del_send(trans_ch, msgData->Addr, msgData->nRecvPort, msgData->nMode, msgData->multiplex, msgData->multiplexID);
            if (NULL != ms_cfg.xt_link_state_event) ms_cfg.xt_link_state_event(EVENT_TCP_STOP,srcno);
            break;
        }
    case MSG_CLOSEMSG://断开握手连接消息
        {
            MEDIA_SVR_PRINT(level_info, "MSG_CLOSEMSG: nDataChannelID[%d]Addr[%s]nRecvPort[%d]nMode[%d]nSSRC[%d]multiplex[%d]multiplexID[%d]",
                trans_ch, msgData->Addr,msgData->nRecvPort,msgData->nMode,msgData->nSSRC,msgData->multiplex,msgData->multiplexID);
            if (ms_cfg.sink_single)
            {
                std::vector<int> srcs;
                XTSrc::_()->find_src(msgData->pMsgContext, srcs);
                for (std::size_t i=0;i<srcs.size();++i)
                {
                    int srcno = srcs[i];
                    int ret = XTSingleSrc::_()->del_src2(srcno);
                    if (ret == 0)
                    {
                        xt_destroy_src(srcno);
                    }
                }
            }
            XTSrc::_()->del_sink(msgData->pMsgContext);
            XTRtp::_()->del_send(msgData->pMsgContext);

            if (NULL != ms_cfg.xt_link_state_event) ms_cfg.xt_link_state_event(EVENT_TCP_CLOSE,trans_ch);
            break;
        }
    default:
        {
            MEDIA_SVR_PRINT(level_info,"MSG ERR:nDataChannelID[%d]Addr[%s]nRecvPort[%d]nMode[%d]nSSRC[%d]multiplex[%d]multiplexID[%d]",
                trans_ch, msgData->Addr,msgData->nRecvPort,msgData->nMode,msgData->nSSRC,msgData->multiplex,msgData->multiplexID);
            break;
        }
    }
    return 0;
}

int xt_rtsp_ctrl(Msg_Rtsp_Ctrl *msgData)
{
    switch (msgData->ctrl)
    {
    case RTSP_PLAY://启动点播消息
        {
            if (NULL != ms_cfg.xt_link_state_event)
            {
                ms_cfg.xt_link_state_event(EVENT_RTSP_CTRLPLAY, msgData->srcno);
            }
            break;
        }
    default:
        {
            break;
        }
    }

    return 0;
}

// 会话协商回调
int MEDIASERVER_STDCALL xt_session_msg_cb(PROTOL_TYPE type, void* msg)
{
    if (!msg)
    {
        return -1;
    }

    switch (type)
    {
    case PT_MSG:
        {
            STR_MsgData *msgData = (STR_MsgData*)msg;
            xt_msg_process(msgData);
            break;
        }
    case PT_RTSP:
        {
            Msg_Rtsp *msgData = (Msg_Rtsp*)msg;
            xt_rtsp_process(msgData);
            break;
        }
    case PT_CTRL_RTSP:
        {
            Msg_Rtsp_Ctrl *msgData = (Msg_Rtsp_Ctrl*)msg;
            xt_rtsp_ctrl(msgData);
            break;
        }
    default:
        {
            break;
        }
    }

    return 0;
}
#endif

// rtsp会话协商处理
int xt_rtsp_process(Msg_Rtsp *msgData)
{
    if (!msgData) return -1;

    int trans_ch = -1;
    //if (multi_code_mgr::_()->multi_code_query(msgData->srcno,ms_code_na,&trans_ch) < 0)
    {
        trans_ch = msgData->srcno;
    }
    xt_track_t std_track;
    switch (msgData->ctrl)
    {
    case RTSP_STARTPLAY://启动点播消息
        {
            int srcno = -1;
            unsigned long chid = -1;
            do
            {
                int ret = XTSrc::instance()->get_track(trans_ch, msgData->trackid, std_track);
                if (ret < 0)
                {
                    MEDIA_SVR_PRINT(level_info, "rtsp_add_send fail: srcno[%d] trackid[%d] ", trans_ch, msgData->trackid);
                    break;
                }

                int srcno_prime = trans_ch;
                srcno = trans_ch;
                chid = std_track.chanid;
                if (msgData->session)
                {
                    XTRtp::instance()->reset_payload(chid);
                }
                xt_sink_t sink;
                sink.session = msgData->session;
                sink.chanid = chid;
                sink.addr = msgData->pstrRAddr;
                sink.port = msgData->nRPort;
                sink.demux = msgData->multiplex;
                sink.demuxid = msgData->multiplexID;
                XTSrc::instance()->add_sink(srcno, sink);
                XTRtp::instance()->add_send(chid, msgData->pstrRAddr, msgData->nRPort, (void*)-1,6, 0, msgData->multiplex, msgData->multiplexID);
                if (NULL != ms_cfg.xt_link_state_event)
                {
                    ms_cfg.xt_link_state_event(EVENT_RTSP_PLAY,srcno);
                }
            }while (false);
            MEDIA_SVR_PRINT(level_info, "rtsp_add_send: srcno[%d] chanid[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                srcno, chid, msgData->pstrRAddr,msgData->nRPort,msgData->multiplex, msgData->multiplexID);
            break;
        }
    case RTSP_STOPPLAY://停止点播消息
        {
            do 
            {
                int ret = XTSrc::instance()->get_track(trans_ch, msgData->trackid, std_track);
                if (ret < 0)
                {
                    break;
                }
                xt_sink_t sink;
                sink.session = msgData->session;
                sink.chanid = std_track.chanid;
                sink.addr = msgData->pstrRAddr;
                sink.port = msgData->nRPort;
                sink.demux = msgData->multiplex;
                sink.demuxid = msgData->multiplexID;
               int res= XTSrc::instance()->del_sink(trans_ch, sink);
				if(res <= 0)
				{
                	XTRtp::instance()->del_send(std_track.chanid, msgData->pstrRAddr, msgData->nRPort, 6, msgData->multiplex, msgData->multiplexID);
				}
                if (NULL != ms_cfg.xt_link_state_event)
                {
                    ms_cfg.xt_link_state_event(EVENT_RTSP_STOP,trans_ch);
                }
            } while (false);

            MEDIA_SVR_PRINT(level_info, "rtsp_del_send: srcno[%d] chanid[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                trans_ch, std_track.chanid,msgData->pstrRAddr,msgData->nRPort,msgData->multiplex, msgData->multiplexID);
            break;
        }
    default:
        {
            MEDIA_SVR_PRINT(level_info, "RTSP MSG ERR: srcno[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                trans_ch, msgData->pstrRAddr,msgData->nRPort,msgData->multiplex, msgData->multiplexID);
            break;
        }
    }
    return 0;
}

// rtsp会话协商处理
int std_sendtask_process(std_send_task *task)
{
    if (!task) return -1;
    xt_track_t std_track;

    switch (task->ctrl)
    {
    case STD_ADD_SEND://启动点播消息
        {
            int srcno = -1;
            unsigned long chid = -1;
            do
            {
                int ret = XTSrc::instance()->get_track(task->srcno, task->trackid, std_track);
                if (ret < 0)
                {
                    MEDIA_SVR_PRINT(level_info, "std_add_send fail: srcno[%d] trackid[%d] ", task->srcno, task->trackid);
                    break;
                }
                srcno = task->srcno;
                chid = std_track.chanid;
                if (task->session)
                {
                    XTRtp::instance()->reset_payload(chid);
                }
                xt_sink_t sink;
                sink.session = task->session;
                sink.chanid = chid;
                sink.addr = task->ip;
                sink.port = task->port;
                sink.demux = task->demux;
                sink.demuxid = task->demux_id;
                XTSrc::instance()->add_sink(srcno, sink);
                XTRtp::instance()->add_send(chid, task->ip, task->port, (void*)-1,6, 0, task->demux, task->demux_id);
            }while (false);
            MEDIA_SVR_PRINT(level_info, "std_add_send: srcno[%d] chanid[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                srcno, chid, task->ip,task->port,task->demux, task->demux_id);
            break;
        }
    case STD_DEL_SEND://停止点播消息
        {
            do 
            {
                int ret = XTSrc::instance()->get_track(task->srcno, task->trackid, std_track);
                if (ret < 0) break;
                xt_sink_t sink;
                sink.session = task->session;
                sink.chanid = std_track.chanid;
                sink.addr = task->ip;
                sink.port = task->port;
                sink.demux = task->demux;
                sink.demuxid = task->demux_id;
                XTSrc::instance()->del_sink(task->srcno, sink);
                XTRtp::instance()->del_send(std_track.chanid, task->ip, task->port, 6, task->demux, task->demux_id);
            } while (false);
            MEDIA_SVR_PRINT(level_info, "std_del_send: srcno[%d] chanid[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                task->srcno, std_track.chanid,task->ip,task->port,task->demux, task->demux_id);
            break;
        }
    default:
        {
            MEDIA_SVR_PRINT(level_info, "std send error: srcno[%d] ip[%s] port[%d] demux[%d] demuxid[%d]",
                task->srcno, task->ip,task->port,task->demux, task->demux_id);
            break;
        }
    }

    return 0;
}

int xt_init_tcp(unsigned long num_chan, string ip, unsigned short server_port)
{
    int ret = 0;
#ifdef USE_TCP_SERVER_
    ret = XTTcp::instance()->init(num_chan, ip, server_port);
    if (ret < 0)
    {
        return -1;
    }
#endif //#ifdef USE_TCP_SERVER_
    return ret;
}

int xt_uninit_tcp()
{
    int ret = 0;
#ifdef USE_TCP_SERVER_
    ret = XTTcp::instance()->uninit();
#endif //#ifdef USE_TCP_SERVER_
    return ret;
}

int xt_init_rtp(unsigned long num_chan,string ip, unsigned short start_port, bool multiplex)
{
    int num = XTRtp::instance()->init(num_chan, ip, start_port, multiplex, ms_cfg.sink_single, ms_cfg.use_traffic_shaping);
    if (num <= 0)
    {
        return -1;
    }
    XTChan::instance()->add_chan(0, num);
    return 0;
}

int xt_uninit_rtp()
{
    return XTRtp::instance()->uninit();
} 

int xt_init_server(MS_CFG &cfg)
{
    MEDIA_SVR_PRINT(level_info, "xt_init_server strat:num_chan[%d] ip[%s] snd_start_port[%d] demux[%d] msg_liten_port[%d] rtsp_listen_port[%d] tcp_listen_port[%d] udp_listen_port[%d] snd_std_rtp[%d]\n",
        cfg.num_chan,cfg.ip,cfg.snd_start_port,cfg.demux,cfg.msg_liten_port,cfg.rtsp_listen_port,cfg.tcp_listen_port,cfg.udp_listen_port,cfg.snd_std_rtp);

    init_media_server_cfg(cfg);
    unsigned int num_ch2 = 0;
    unsigned int num_std = 0;
    if (cfg.sink_single)
    {
        num_ch2 = 2*cfg.num_chan;
    }
    if (cfg.snd_std_rtp)
    {
        num_std = 2*cfg.num_chan;
    }
    unsigned long rtp_chan_num = cfg.num_chan+num_ch2+num_std;

    int ret = xt_init_rtp(rtp_chan_num, cfg.ip, cfg.snd_start_port, cfg.demux);
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "%s", "rtp init fail");
        return -1;
    }

    for (unsigned long cid = cfg.num_chan;cid < cfg.num_chan+num_ch2;++cid)
    {
        XTChan::instance()->set_candchan(cid);
    }
    for (unsigned long cid = cfg.num_chan+num_ch2;cid < cfg.num_chan+num_ch2+num_std;++cid)
    {
        XTChan::instance()->set_stdchan(cid);
    }

#ifndef CLOSE_SESSION
    if (0<cfg.msg_liten_port || 0<cfg.udp_listen_port)
    {
        ret = XTSession::instance()->init_msg(cfg.ip, cfg.msg_liten_port,cfg.udp_listen_port,cfg.snd_start_port, 
            cfg.mul_start_ip, cfg.snd_start_port, MEDIA_SVR_PRINT2);
        if (ret < 0)
        {
            MEDIA_SVR_PRINT(level_info, "%s", "tcp session init fail");
            return -2;
        }

        XTSession::instance()->set_tcp_play_cb_func(cfg.tcp_play_cb);
        XTSession::instance()->set_tcp_pause_cb_func(cfg.tcp_pause_cb);
    }

    if (0 < cfg.rtsp_listen_port)
    {
        ret = XTSession::instance()->init_rtsp(cfg.ip, cfg.rtsp_listen_port, cfg.num_chan, MEDIA_SVR_PRINT2);
        if (ret < 0)
        {
            MEDIA_SVR_PRINT(level_info, "%s", "rtsp session init fail");
            return -3;
        }

        XTSession::instance()->set_rtsp_play_cb_func(cfg.rtsp_play_cb);
        XTSession::instance()->set_rtsp_pause_cb_func(cfg.rtsp_pause_cb);
    }

    XTSession::instance()->set_sessionmsg_cb(xt_session_msg_cb);
    XTSession::instance()->register_common_cb();
#endif
    MEDIA_SVR_PRINT(level_info, "%s", "xt_init_server end");
    return ret;
}

int xt_uninit_server()
{
    int ret = xt_uninit_rtp();
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "%s", "xt_uninit_rtp fail");
    }
#ifndef CLOSE_SESSION
    if (0<ms_cfg.msg_liten_port || 0<ms_cfg.udp_listen_port)
    {
        ret = XTSession::instance()->uninit_msg();
        if (ret < 0)
        {
            MEDIA_SVR_PRINT(level_info, "%s", "uninit_msg fail");
        }
    }
    if (0 < ms_cfg.rtsp_listen_port)
    {
        ret = XTSession::instance()->uninit_rtsp();
        if (ret < 0)
        {
            MEDIA_SVR_PRINT(level_info, "%s", "uninit_rtsp fail");
        }
    }
#endif
    MEDIA_SVR_PRINT(level_info, "%s", "xt_uninit_server end");
    //反初始化
#ifdef _USE_LOG
    CLog::instance()->UnInit();
#endif//#ifdef _USE_LOG
    return ret;
}

int xt_get_tcplink_num()
{
    int ret = 0;
#ifdef USE_TCP_SERVER_
    // 获得tcp连接数
    ret = XTTcp::instance()->get_link_num();
#endif //#ifdef USE_TCP_SERVER_
    return ret;
}

long add_src_to_session(const int srcno)
{
    long ret_code = 0;

#ifndef CLOSE_SESSION
    do 
    {
        ret_code = XTSession::instance()->add_src(srcno);
        if (ret_code < 0)
        {
            XTSrc::instance()->destroy_src(srcno);
            break;
        }
        // 设置会话协商源转发端口
        std::list<xt_track_t> tracks;
        XTSrc::instance()->get_tracks(srcno, tracks);
        std::list<xt_track_t>::iterator itr = tracks.begin();
        for (;itr != tracks.end();++itr)
        {
            xt_track_t &t = *itr;
            Rtp_Handle rtp;
            XTRtp::instance()->get_rtp(t.chanid, rtp);
            XTSession::instance()->set_snd_port(srcno, t.trackid, rtp.port, rtp.multiplex, rtp.multid);
        }
        ret_code = 1;
    } while (0);
#endif

    return ret_code;
}

int xt_create_src_sdp(int* srcno,xt_track_t* trackinfos,int* tracknum,const char* sdp,const long sdp_len,const long chanid)
{
    int ret_code = -1;
    do 
    {
        ret_code = XTSrc::instance()->create_src_sdp(srcno,trackinfos,tracknum,sdp,sdp_len,chanid);
        if (ret_code < 0)
        {
            MEDIA_SVR_PRINT(level_info, "create_src_sdp fail:ret_code[%d] sdp[%s]", ret_code, sdp);
            break;
        }

        ret_code = add_src_to_session(*srcno);
        if (ret_code < 0)
        {
            break;
        }
        ret_code = 0;
    } while (0);
    return  ret_code;
}

int xt_create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip)
{
    int ret_code = -1;
    do 
    {
        ret_code = XTSrc::instance()->create_src_defult(srcno,sdp,sdp_len,chanid,local_bind_ip);
        if (ret_code < 0)
        {
            MEDIA_SVR_PRINT(level_info, "create_src_defult fail:ret_code[%d]", ret_code);
            break;
        }

        // 会话协商增加源
        int _srcno = *srcno;
        ret_code = add_src_to_session(_srcno);
        if (ret_code < 0)
        {
            break;
        }
        ret_code = 0;
    } while (0);
    return ret_code;
}

int xt_create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid)
{
    int ret = _xt_create_src(tracknum, trackids, tracknames, srcno, chanid);
	if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "xt_create_src:ret[%d] tracknum[%d] chanid[%d]",ret,tracknum, chanid);
        return -1;
    }
    MEDIA_SVR_PRINT(level_info, "xt_create_src:tracknum[%d] chanid[%d]", tracknum, chanid);
    return 0;
}

int xt_create_src(src_track_info_t &tracks,int &srcno,long chanid)
{
    int tracknum = tracks.tracknum;
    if (tracknum<0 || tracknum>MAX_TRACK) return -1;

    char *pname[MAX_TRACK]={0};
    for (int i=0;i<MAX_TRACK;++i)
    {
        pname[i] = tracks.tracknames[i];
    }
    int ret = _xt_create_src(tracknum, tracks.trackids, pname, srcno, chanid);
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "xt_create_src:ret[-1] tracknum[%d] chanid[%d]", tracknum, chanid);
        return -1;
    }
    MEDIA_SVR_PRINT(level_info, "xt_create_src:tracknum[%d] chanid[%d]", tracknum, chanid);
    return 0;
}

int _xt_create_src(int tracknum,int *trackids,char *tracknames[],int &srcno,long chanid)
{
    // 创建转发源
    int ret = XTSrc::_()->create_src(tracknum, trackids, tracknames, srcno, chanid);
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "_xt_create_src:create_src fail srcno[%d] ret[%d]\n",srcno,ret);
        return ret;
    }

#ifndef CLOSE_SESSION
    // 会话协商增加源
    ret = XTSession::instance()->add_src(srcno);
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "_xt_create_src:add_src fail srcno[%d] ret[%d]\n",srcno,ret);
        XTSrc::_()->destroy_src(srcno);
        return ret;
    }

    // 设置会话协商源转发端口
    std::list<xt_track_t> tracks;
    XTSrc::_()->get_tracks(srcno, tracks);
    std::list<xt_track_t>::iterator itr = tracks.begin();
    for (;itr != tracks.end();++itr)
    {
        xt_track_t &t = *itr;
        Rtp_Handle rtp;
        XTRtp::instance()->get_rtp(t.chanid, rtp);
        XTSession::instance()->set_snd_port(srcno, t.trackid, rtp.port, rtp.multiplex, rtp.multid);
    }
#endif
    return ret;
}

int xt_create_singlesrc(int srcno_prime, int &srcno)
{
    unsigned long chid;
    int ret = XTChan::instance()->get_freecandchan(chid);
    if (ret < 0)
    {
        return -1;
    }

    MEDIA_SVR_PRINT(level_info, "xt_create_singlesrc: srcno_prime[%d] main_chan[%d]", srcno_prime, chid);

    int trk_ids[16];
    int trk_num = XTSrc::_()->get_tracknum(srcno_prime);
    XTSrc::_()->get_trackids(srcno_prime, trk_ids);

    MEDIA_SVR_PRINT(level_info, "xt_create_singlesrc: srcno_prime[%d] trk_num[%d]", srcno_prime, trk_num);

    ret = _xt_create_src(trk_num, trk_ids, NULL, srcno, chid);
    if (ret < 0)
    {
        XTChan::instance()->active_chan(chid, false);
        return -2;
    }

    XTSingleSrc::inst()->add_src(srcno, srcno_prime);

#ifndef CLOSE_SESSION
    // 设置会话协商源转发端口
    std::list<xt_track_t> tracks1,tracks2;
    XTSrc::instance()->get_tracks(srcno_prime, tracks1);
    XTSrc::instance()->get_tracks(srcno, tracks2);
    std::list<xt_track_t>::iterator itr1 = tracks1.begin();
    std::list<xt_track_t>::iterator itr2 = tracks2.begin();
    for (;itr1!=tracks1.end() && itr2!=tracks2.end();++itr1,++itr2)
    {
        xt_track_t &t1 = *itr1;
        xt_track_t &t2 = *itr2;

        Rtp_Handle rtp;
        XTRtp::instance()->get_rtp(t2.chanid, rtp);

        XTSession::instance()->set_snd_port(srcno_prime, t1.trackid, rtp.port, rtp.multiplex, rtp.multid);

        XTSession::instance()->set_snd_port(t1.chanid, rtp.port, rtp.multiplex, rtp.multid);
        MEDIA_SVR_PRINT(level_info, "xt_create_singlesrc set_snd_port: srcno_prime[%d] chanid[%d] port[%d] multiplex[%d] multid[%d]", 
            srcno_prime, t1.chanid, rtp.port, rtp.multiplex, rtp.multid);
    }
#endif

    return 0;
}

int _xt_destroy_src(int srcno)
{
    int ret = 0;
    if (srcno < 0)
    {
        MEDIA_SVR_PRINT(level_info, "_xt_destroy_src:ret[-1] srcno[%d]", srcno);
        return -1;
    }

#ifndef CLOSE_SESSION
    // 清除系统头
    xt_track_t track;
    ret = XTSrc::instance()->get_main_track(srcno, track);
    if (ret >= 0)
    {
        XTSession::instance()->clear_key_data(track.chanid);
    }

    // 会话协商删除源
    XTSession::instance()->del_src(srcno);
#endif

    // 销毁转发源
    ret = XTSrc::instance()->destroy_src(srcno);
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info, "_xt_destroy_src:ret[-1] srcno[%d]", srcno);
        return ret;
    }

    return 0;
}

int xt_destroy_src(int srcno)
{
    int ret = 0;
    if (srcno < 0)
    {
        MEDIA_SVR_PRINT(level_info, "xt_destroy_src:ret[-1] srcno[%d]", srcno);
        return -1;
    }

#ifndef CLOSE_SESSION
    // 清除系统头
    xt_track_t track;
    ret = XTSrc::instance()->get_main_track(srcno, track);
    if (ret >= 0)
    {
        XTSession::instance()->clear_key_data(track.chanid);
    }

    // 会话协商删除源
    XTSession::instance()->del_src(srcno);
#endif

    // 销毁转发源
    ret = XTSrc::instance()->destroy_src(srcno);
    if (ret < 0) return ret;
    if (ms_cfg.sink_single)
    {
        XTSingleSrc::inst()->del_src(srcno);
    }

    MEDIA_SVR_PRINT(level_info, "xt_destroy_src:srcno[%d]", srcno);

    return 0;
}

int update_sdp_addr_port_by_rtsp(int srcno, const char *sdp, long len, std::string &sdp_back)
{
    if (!sdp || len >= LEN_SDP) return -1;
    std::ostringstream oss;
    xt_sdp::parse_buffer_t pb(sdp, len); 
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -1;
    }

    std::list<xt_track_t> tracks;
    XTSrc::instance()->get_tracks(srcno, tracks);
    if (tracks.size()==0)
    {
        return -1;
    }

    std::string ip("0.0.0.0");
    xsdp.origin_.set_address(ip);
    xsdp.connection_.set_address(ip);

    sdp_session_t::medium_container_t &media = xsdp.media_;
    sdp_session_t::medium_container_t::iterator itr = media.begin();
    for (;itr!=media.end();++itr)
    {
        sdp_session_t::medium_t &medium = *itr;
        medium.session_->origin_.set_address(ip);
        medium.session_->connection_.set_address(ip);

        std::list<xt_sdp::sdp_session_t::connection_t> &conns = medium.get_medium_connections();
        std::list<xt_sdp::sdp_session_t::connection_t>::iterator itr_c = conns.begin();
        for (;itr_c!=conns.end();++itr_c)
        {
            xt_sdp::sdp_session_t::connection_t &c = *itr_c;
            c.set_address(ip);
        }
        medium.port_ = 0;
    }

    try
    {
        xsdp.encode(oss);
        sdp_back = oss.str();
    }
    catch(...)
    {
        return -1;
    }

    return 1;
}

int update_sdp_addr(int srcno, const char *sdp, long len, std::string &sdp_back)
{
    if (!sdp || len >= LEN_SDP) return -1;
    std::ostringstream oss;
    xt_sdp::parse_buffer_t pb(sdp, len); 
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(const std::exception& e)
    {
        printf("sdp parsed failed.%s\n", e.what());
        return -1;
    }

    std::list<xt_track_t> tracks;
    XTSrc::instance()->get_tracks(srcno, tracks);
    if (tracks.size()==0)
    {
        return -1;
    }

    xsdp.origin_.set_address(ms_cfg.ip);
    xsdp.connection_.set_address(ms_cfg.ip);
    xsdp.attribute_helper_.clear_attribute("rtpport-mux");

    sdp_session_t::medium_container_t &media = xsdp.media_;
    sdp_session_t::medium_container_t::iterator itr = media.begin();
    for (;itr!=media.end();++itr)
    {
        sdp_session_t::medium_t &medium = *itr;
        medium.session_->origin_.set_address(ms_cfg.ip);
        medium.session_->connection_.set_address(ms_cfg.ip);

        std::list<xt_sdp::sdp_session_t::connection_t> &conns = medium.get_medium_connections();
        std::list<xt_sdp::sdp_session_t::connection_t>::iterator itr_c = conns.begin();
        for (;itr_c!=conns.end();++itr_c)
        {
            xt_sdp::sdp_session_t::connection_t &c = *itr_c;
            c.set_address(ms_cfg.ip);
        }

        std::string name = medium.name();

        std::list<xt_track_t>::iterator itr2 = tracks.begin();
        for (;itr2!=tracks.end();++itr2)
        {
            xt_track_t &track = *itr2;
            std::string name2 = track.trackname;

            if (name == name2)
            {
                Rtp_Handle rtp;
                int ret = XTRtp::instance()->get_rtp(track.chanid, rtp);
                if (ret < 0)
                {
                    continue;
                }

                medium.port_ = rtp.port;

                medium.attribute_helper_.clear_attribute("muxid");
                medium.attribute_helper_.clear_attribute("rtpport-mux");
                if (rtp.multiplex)
                {
                    if (!xsdp.attribute_helper_.exists("rtpport-mux"))
                    {
                        xsdp.attribute_helper_.add_attribute("rtpport-mux");
                    }
                    stringstream ss;
                    ss << rtp.multid;
                    medium.attribute_helper_.add_attribute("muxid", ss.str());
                }

                if (!medium.attribute_helper_.exists("sendonly"))
                {
                    medium.attribute_helper_.add_attribute("sendonly");
                }
                break;
            }
        }
    }
    try
    {
        xsdp.encode(oss);
        sdp_back = oss.str();
    }
    catch(...)
    {
        return -1;
    }

    return 0;
}
int set_payload_sdp(int srcno, const char *sdp, long len)
{
    if (!sdp || len >= LEN_SDP)  return -1;
    xt_sdp::parse_buffer_t pb(sdp, len); 
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -1;
    }

    sdp_session_t::medium_container_t &media = xsdp.media_;
    sdp_session_t::medium_container_t::iterator itr = media.begin();
    for (;itr!=media.end();++itr)
    {
        sdp_session_t::medium_t &medium = *itr;	
        std::list<std::string> &fs = medium.formats_;
        if (fs.size()>0)
        {
            std::string name = medium.name();
            std::string f = *(fs.begin());
            int ret = xt_set_payload(srcno, name.c_str(), ::atoi(f.c_str()), false);
            if (ret < 0)
            {
                const std::list<std::string>& tracks = medium.attribute_helper_.get_values("control");
                if (tracks.size() > 0)
                {
                    std::string track = *tracks.begin();
                    if (track.length() > 0)
                    {
                        track = track.substr(track.length()-1, 1);
                        int trackid = ::atoi(track.c_str());
                        xt_set_payload(srcno, trackid, ::atoi(f.c_str()), false);
                    }
                }
            }
        }
    }
    return 0;
}
int xt_set_key(int srcno, char *keydata, long len, long datatype)
{
    int ret_code = -1;
    do 
    {
        if (NULL == keydata || len<=0 || LEN_SDP <= len)
        {
            MEDIA_SVR_PRINT(level_info, "xt_set_key start:fail srcno[%d] len[%d] datatype[%d] sdp[%s]",srcno,len,datatype,keydata);
            ret_code = -1;
            break;
        }

        MEDIA_SVR_PRINT(level_info, "xt_set_key start:srcno[%d] len[%d] datatype[%d] sdp[%s]",srcno,len,datatype,keydata);

#ifndef CLOSE_SESSION
        // 设置系统头(tcp协商)
        xt_track_t track;
        ret_code = XTSrc::instance()->get_main_track(srcno, track);
        if (ret_code >= 0)
        {
            ret_code = XTSession::instance()->set_key_data(track.chanid, keydata, len, datatype);
            if (ret_code < 0)
            {
                MEDIA_SVR_PRINT(level_error,
                    "pri tcp session set_key_data fail:srcno[%d] ret_code[%d] chanid[%d] len[%d] datatype[%d] keydata[%s]",srcno,ret_code,track.chanid,len,datatype,keydata);
                break;
            }
        }
        else
        {
            MEDIA_SVR_PRINT(level_info,"设置系统头(tcp协商)-get_main_track fail  srcno[%d] ret_code[%d]",srcno,ret_code);
            break;
        }
#endif // end #ifndef CLOSE_SESSION

        ::set_payload_sdp(srcno, keydata, len);
        std::string sdp_back;
        //保存 sip sdp
        if(xt_session_sip::inst()->is_init())
        {
            ::update_sdp_addr(srcno, keydata, len, sdp_back);
            xt_session_sip::inst()->set_sdp(srcno, sdp_back.c_str(), sdp_back.length());
        }

#ifndef CLOSE_SESSION
        // rtsp协商设置sdp，失败不返回 支持只支持私有流转的流
        if (ms_cfg.rtsp_listen_port > 0)
        {
            //rtsp 协商 sdp c 为0.0.0.0
            ret_code = ::update_sdp_addr_port_by_rtsp(srcno, keydata, len, sdp_back);
            if (!sdp_back.empty() && 0 <= ret_code)
            {
                ret_code = XTSrc::_()->set_std_track_frametype(srcno,sdp_back.c_str(),sdp_back.length());
                if (ret_code < 0)
                {
                    MEDIA_SVR_PRINT(level_error,"set_std_track_frametype fail  srcno[%d] ret_code[%d]",srcno,ret_code);
                }
                ret_code = XTSession::instance()->set_sdp(srcno, sdp_back.c_str(), sdp_back.length());
                if (ret_code < 0)
                {
                    MEDIA_SVR_PRINT(level_error,"XTSession::instance()->set_sdp fail  srcno[%d] ret_code[%d]",srcno,ret_code);
                }
            }
            else
            {
                MEDIA_SVR_PRINT(level_info, "update_sdp_addr_port_by_rtsp:set_sdp no save to rtsp session libs  srcno[%d]",srcno);
            }
        }
#endif //end #ifndef CLOSE_SESSION
        ret_code = 1;
    } while (0);

    return ret_code;
}

int xt_set_sipsdp_full(int srcno, char *keydata, long len)
{
    if (NULL == keydata || len<=0 || LEN_SDP < len)
    {
        return -1;
    }

    //保存 sip sdp
    if(xt_session_sip::inst()->is_init())
    {
        std::string sdp_back;
        ::update_sdp_addr(srcno, keydata, len, sdp_back);
        xt_session_sip::inst()->set_sdp_full(srcno, sdp_back.c_str(), sdp_back.length());
    }

    return 0;
}

int xt_send_data_single(int src_prime,          // 源id
                        int trackid,            // trackid
                        char *buff,             // 发送数据
                        unsigned long len,      // 数据长度
                        int frame_type,         // 帧类型
                        long device_type)       // 设备类型
{
    std::vector<int> vsrc;
    int ret = XTSingleSrc::inst()->get_src(src_prime, vsrc);
    if (ret < 0)
    {
        return -1;
    }

    std::vector<int>::iterator itr = vsrc.begin();
    for (;itr!=vsrc.end();++itr)
    {
        int srcno = *itr;

        // 混合流
        //////////////////////////////////////////////////////////////////////////
        xt_track_t pri_track;
        int ret = XTSrc::instance()->get_main_track(srcno, pri_track);
        if (ret < 0)
        {
            continue;
        }
#ifdef USE_TCP_SERVER_
        // tcp发送
        XTTcp::instance()->send_data(track.chanid, buff, len, frame_type, device_type);
#endif //#ifdef USE_TCP_SERVER_

        // rtp发送
        //XTRtp::instance()->send_data(track.chanid, buff, len, frame_type, device_type, false);
        pri_track.frametype = frame_type;
       XTRtp::_()->send_data(pri_track, buff, len,device_type, false);
        //////////////////////////////////////////////////////////////////////////

        // 标准流
        //////////////////////////////////////////////////////////////////////////		
        if (ms_cfg.snd_std_rtp)
        {
            xt_track_t std_trk;
            ret = XTSrc::instance()->get_track(srcno, trackid, std_trk);
            if (ret>=0 && trackid>=0)
            {
                //XTRtp::instance()->send_data(std_trk.chanid, buff, len, frame_type, device_type, true);
                XTRtp::_()->send_data(std_trk, buff, len, device_type, true);
            }
        }
        //////////////////////////////////////////////////////////////////////////
    }

    return 0;
}

int xt_send_data(int srcno,				// 源id
                 int trackid,			// trackid
                 char *buff,			// 发送数据
                 unsigned long len,		// 数据长度
                 int frame_type,		// 帧类型
                 long device_type)		// 设备类型
{
    int ret = 0;
    // 混合流
    //////////////////////////////////////////////////////////////////////////
    if (0 < ms_cfg.msg_liten_port)
    {
        xt_track_t pri_track;
        ret = XTSrc::instance()->get_main_track(srcno, pri_track);
        if (ret < 0)
        {
            return -1;
        }

        pri_track.frametype = frame_type;
#ifdef USE_TCP_SERVER_
        // tcp发送
        XTTcp::instance()->send_data(pri_track.chanid, buff, len, frame_type, device_type);
#endif //#ifdef USE_TCP_SERVER_

        // rtp发送
        //XTRtp::instance()->send_data(pri_track.chanid, buff, len, frame_type, device_type, false);
        XTRtp::_()->send_data(pri_track, buff, len, device_type, false);
        //////////////////////////////////////////////////////////////////////////
    }

    // 标准流
    //////////////////////////////////////////////////////////////////////////	
    if (ms_cfg.snd_std_rtp)
    {
        xt_track_t std_trk;
        ret = XTSrc::instance()->get_track(srcno, trackid, std_trk);
        if (ret>=0 && trackid>=0)
        {
            //XTRtp::instance()->send_data(std_trk.chanid, buff, len, frame_type, device_type, true);
            XTRtp::_()->send_data(std_trk, buff, len, device_type, true);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    if (ms_cfg.sink_single)
    {
        xt_send_data_single(srcno, trackid, buff, len, frame_type, device_type);
    }

    return 0;
}

int xt_send_data_in_stamp(int srcno,	        // 源id
                          int trackid,		    // trackid
                          char *buff,		    // 发送数据
                          unsigned long len,    // 数据长度
                          int frame_type,		 // 帧类型
                          long device_type,     // 设备类型
                          bool frame_ts_flg,    // 是否外部传入时戳
                          uint32_t in_time_stamp,
						  bool use_ssrc,
						  uint32_t ssrc)// 外部输入时戳

{
    int ret = 0;
    // 混合流
    //////////////////////////////////////////////////////////////////////////
    if (0<ms_cfg.msg_liten_port || 0<ms_cfg.udp_listen_port)
    { 
        xt_track_t pri_track;//for loop 512x(512+1)/2=13w 次
        ret = XTSrc::_()->get_main_track(srcno, pri_track);
        if (ret < 0)
        {
            MEDIA_SVR_PRINT(level_info, "xt_send_data_in_stamp:get_main_track fail ret[%d] srcno[%d]", ret,srcno);
            return -1;
        }
        pri_track.frametype = frame_type;

#ifdef USE_TCP_SERVER_
        // tcp发送
        XTTcp::instance()->send_data(pri_track.chanid, buff, len, frame_type, device_type);
#endif //#ifdef USE_TCP_SERVER_ 

        // rtp发送
        //ret = XTRtp::instance()->send_data_in_stamp(track.chanid, buff, len, frame_type, device_type,frame_ts_flg,in_time_stamp,0,false);
        ret = XTRtp::_()->send_data_in_stamp(pri_track, buff, len, device_type,frame_ts_flg,in_time_stamp,0,false,use_ssrc,ssrc);
    }
    //////////////////////////////////////////////////////////////////////////

    // 标准流
    //////////////////////////////////////////////////////////////////////////
    if (ms_cfg.snd_std_rtp)
    {
        xt_track_t std_trk;
        ret = XTSrc::_()->get_track(srcno, trackid, std_trk);
        if (ret>=0 && trackid>=0)
        {
            //ret = XTRtp::instance()->send_data_in_stamp(std_trk.chanid, buff, len, frame_type, device_type,frame_ts_flg,in_time_stamp,0,true);
            XTRtp::_()->send_data_in_stamp(std_trk, buff, len, device_type,frame_ts_flg,in_time_stamp,0,true,use_ssrc,ssrc);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    if (ms_cfg.sink_single)
    {
        ret = xt_send_data_single(srcno, trackid, buff, len, frame_type, device_type);
        if (ret < 0)
        {
            MEDIA_SVR_PRINT(level_info,"xt_send_data_in_stamp:send single data fail ret[%d] srcno[%d] trackid[%d]\n",ret,srcno,trackid);
        }
    }
    return 0;
}

int xt_send_rtp_in_stamp(int srcno,	        // 源id
						  int trackid,		    // trackid
						  char *buff,		    // 发送数据
						  unsigned long len,    // 数据长度
						  int frame_type,		 // 帧类型
						  long device_type,     // 设备类型
						  bool frame_ts_flg,    // 是否外部传入时戳
						  uint32_t in_time_stamp,
						  bool use_ssrc,
						  uint32_t ssrc)// 外部输入时戳

{
	int ret = 0;
	// 混合流
	//////////////////////////////////////////////////////////////////////////
	if (0<ms_cfg.msg_liten_port || 0<ms_cfg.udp_listen_port)
	{ 
		xt_track_t pri_track;//for loop 512x(512+1)/2=13w 次
		ret = XTSrc::_()->get_main_track(srcno, pri_track);
		if (ret < 0)
		{
			MEDIA_SVR_PRINT(level_info, "xt_send_data_in_stamp:get_main_track fail ret[%d] srcno[%d]", ret,srcno);
			return -1;
		}
		pri_track.frametype = frame_type;

#ifdef USE_TCP_SERVER_
		// tcp发送
		XTTcp::instance()->send_data(pri_track.chanid, buff, len, frame_type, device_type);
#endif //#ifdef USE_TCP_SERVER_ 

		// rtp发送
		//ret = XTRtp::instance()->send_data_in_stamp(track.chanid, buff, len, frame_type, device_type,frame_ts_flg,in_time_stamp,0,false);
		ret = XTRtp::_()->send_rtp_in_stamp(pri_track, buff, len, device_type,frame_ts_flg,in_time_stamp,0,false,use_ssrc,ssrc);
	}
	//////////////////////////////////////////////////////////////////////////

	// 标准流
	//////////////////////////////////////////////////////////////////////////
	if (ms_cfg.snd_std_rtp)
	{
		xt_track_t std_trk;
		ret = XTSrc::_()->get_track(srcno, trackid, std_trk);
		if (ret>=0 && trackid>=0)
		{
			//ret = XTRtp::instance()->send_data_in_stamp(std_trk.chanid, buff, len, frame_type, device_type,frame_ts_flg,in_time_stamp,0,true);
			XTRtp::_()->send_rtp_in_stamp(std_trk, buff, len, device_type,frame_ts_flg,in_time_stamp,0,true,use_ssrc,ssrc);
		}
	}
	//////////////////////////////////////////////////////////////////////////

	/*if (ms_cfg.sink_single)
	{
		ret = xt_send_rtp_single(srcno, trackid, buff, len, frame_type, device_type);
		if (ret < 0)
		{
			MEDIA_SVR_PRINT(level_info,"xt_send_data_in_stamp:send single data fail ret[%d] srcno[%d] trackid[%d]\n",ret,srcno,trackid);
		}
	}*/
	return 0;
}

int xt_send_data_in_stamp_p(int srcno,	        // 源id
                            int trackid,		    // trackid
                            char *buff,		    // 发送数据
                            unsigned long len,    // 数据长度
                            int frame_type,		 // 帧类型
                            long device_type,     // 设备类型
                            bool frame_ts_flg,    // 是否外部传入时戳
                            uint32_t in_time_stamp,// 外部输入时戳
                            uint8_t priority)   //发送数据优先级

{
    // 混合流
    //////////////////////////////////////////////////////////////////////////
    xt_track_t pri_track;
    int ret = XTSrc::_()->get_main_track(srcno, pri_track);
    if (ret < 0)
    {
        MEDIA_SVR_PRINT(level_info,"get_main_track fail ret[%d] srcno[%d]\n",ret,srcno);
        return -1;
    }

#ifdef USE_TCP_SERVER_
    // tcp发送
    XTTcp::instance()->send_data(pri_track.chanid, buff, len, frame_type, device_type);
#endif //#ifdef USE_TCP_SERVER_

    // rtp发送
    //ret = XTRtp::instance()->send_data_in_stamp(track.chanid, buff, len, frame_type, device_type,frame_ts_flg,in_time_stamp, priority);

    pri_track.frametype = frame_type;
    ret = XTRtp::_()->send_data_in_stamp(pri_track, buff, len, device_type,frame_ts_flg,in_time_stamp, priority);
    //////////////////////////////////////////////////////////////////////////

    // 标准流
    //////////////////////////////////////////////////////////////////////////
    if (ms_cfg.snd_std_rtp)
    {
        xt_track_t std_trk;
        ret = XTSrc::instance()->get_track(srcno, trackid, std_trk);
        if (ret>=0 && trackid>=0)
        {
            //ret = XTRtp::instance()->send_data_in_stamp(std_trk.chanid, buff, len, frame_type, device_type,frame_ts_flg,in_time_stamp,priority,true);
            ret = XTRtp::_()->send_data_in_stamp(std_trk, buff, len, device_type,frame_ts_flg,in_time_stamp,priority,true);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    if (ms_cfg.sink_single)
    {
        ret = xt_send_data_single(srcno, trackid, buff, len, frame_type, device_type);
    }

    return ret;

}


int xt_send_data_in_stamp_ps(int srcno,	        // 源id
                            int trackid,		    // trackid
                            char *buff,		    // 发送数据
                            unsigned long len,    // 数据长度
                            int frame_type,		 // 帧类型
                            long device_type,     // 设备类型
                            bool frame_ts_flg,    // 是否外部传入时戳
                            uint32_t in_time_stamp,// 外部输入时戳
                            uint8_t priority,   //发送数据优先级
                            bool use_ssrc,
                            uint32_t ssrc)

{
    int ret=0;
    xt_track_t std_trk;

    ret = XTSrc::_()->get_track(srcno, trackid, std_trk);
    if (ret>=0 && trackid>=0)
    {
        std_trk.frametype = frame_type;
        //发送国标PS流接口
        XTRtp::_()->send_data_in_stamp_ps(std_trk, buff, len, device_type,frame_ts_flg,in_time_stamp,0,true, use_ssrc,ssrc);
    }

    return 0;
}

int xt_get_chanid(int srcno, int trackid, long &chanid)
{
    xt_track_t track;
    int ret = XTSrc::instance()->get_track(srcno, trackid, track);
    if (ret < 0)
    {
        return -1;
    }
    chanid = track.chanid;
    return 0;
}

int xt_get_rtp_sn(const int srcno, const int trackid, unsigned short *sn)
{
	long chanid = 0;
	int ret = xt_get_chanid(srcno, trackid, chanid);
	if (ret < 0)
	{
		return -1;
	}

	ret = XTRtp::instance()->get_sink_sn(chanid, sn);
	if (ret < 0)
	{
		return -1;
	}
	return 0;
}

uint32_t xt_get_cur_connect_num()
{
    return CRunInfoMgr::instance()->get_cur_connect_num();
}

int xt_get_connect_info(connect_info_t out_cinfo[],uint32_t& connect_num)
{
    return CRunInfoMgr::instance()->get_connect_info(out_cinfo,connect_num);
}

int xt_regist(const char* sz_ids, const char* sz_server_ip, unsigned short server_port,uint32_t millisec)
{
#ifndef CLOSE_SESSION
    return xt_regist_client::instance()->regist(sz_ids,sz_server_ip,server_port,millisec);
#else
    return 0;
#endif
}

int xt_stop_regist(const char* sz_server_ip,unsigned short server_port,uint32_t millisec)
{
#ifndef CLOSE_SESSION
    return  xt_regist_client::instance()->stop_regist(sz_server_ip,server_port,millisec);
#else
    return 0;
#endif
}

void xt_regist_response_callback(regist_response_callback_t func)
{
#ifndef CLOSE_SESSION
    xt_regist_client::instance()->set_regist_response_callback_t(func);
#endif
}

bool xt_isregist()
{
    return false;
}

// 增加转发
int xt_add_send(int srcno, int trackid, const char *ip, unsigned short port, bool demux, unsigned int demuxid)
{
    std_send_task task;
    task.ctrl = STD_ADD_SEND;
    task.session = NULL;
    task.srcno = srcno;
    task.trackid = trackid;
    ::strncpy(task.ip, ip, SESSION_SERVER_IP_LEN);
    task.port = port;
    task.demux = demux;
    task.demux_id = demuxid;

    return std_sendtask_process(&task);
}
int xt_add_send(int srcno, const char *track, const char *ip, unsigned short port, bool demux, unsigned int demuxid)
{
    int trackid = -1;
    bool find = false;

    std::list<xt_track_t> tracks;
    int ret = XTSrc::instance()->get_tracks(srcno, tracks);
    if (ret<0 || tracks.size()==0)
    {
        return -1;
    }

    std::list<xt_track_t>::iterator itr = tracks.begin();
    for (;itr!=tracks.end();++itr)
    {
        xt_track_t &t = *itr;
        if (::strcmp(t.trackname, track)==0)
        {
            trackid = t.trackid;
            find = true;
            break;
        }
    }

    if (!find)
    {
        return -1;
    }

    return xt_add_send(srcno, trackid, ip, port, demux, demuxid);
}

int xt_set_payload(int srcno, int trackid, int payload, bool update)
{
    xt_track_t track;
    int ret = XTSrc::instance()->get_track(srcno, trackid, track);
    if (ret < 0)
    {
        return -1;
    }

    unsigned long chid = track.chanid; 
    XTRtp::instance()->set_payload(chid, payload, update);

    return 0;
}
int xt_set_payload(int srcno, const char *track, int payload, bool update)
{
    int trackid = -1;
    bool find = false;

    std::list<xt_track_t> tracks;
    int ret = XTSrc::instance()->get_tracks(srcno, tracks);
    if (ret<0 || tracks.size()==0)
    {
        return -1;
    }

    std::list<xt_track_t>::iterator itr = tracks.begin();
    for (;itr!=tracks.end();++itr)
    {
        xt_track_t &t = *itr;
        if (::strcmp(t.trackname, track)==0)
        {
            trackid = t.trackid;
            find = true;
            break;
        }
    }

    if (!find)
    {
        return -1;
    }

    return xt_set_payload(srcno, trackid, payload, update);
}

int xt_del_send(int srcno, int trackid, const char *ip, unsigned short port, bool demux, unsigned int demuxid)
{
    std_send_task task;

    task.ctrl = STD_DEL_SEND;
    task.session = NULL;
    task.srcno = srcno;
    task.trackid = trackid;
    ::strncpy(task.ip, ip, SESSION_SERVER_IP_LEN);
    task.port = port;
    task.demux = demux;
    task.demux_id = demuxid;

    return std_sendtask_process(&task);
}
int xt_del_send(int srcno, const char *track, const char *ip, unsigned short port, bool demux, unsigned int demuxid)
{
    int trackid = 1;

    std::list<xt_track_t> tracks;
    int ret = XTSrc::instance()->get_tracks(srcno, tracks);
    if (ret<0 || tracks.size()==0)
    {
        return -1;
    }

    std::list<xt_track_t>::iterator itr = tracks.begin();
    for (;itr!=tracks.end();++itr)
    {
        xt_track_t &t = *itr;
        if (::strcmp(t.trackname, track)==0)
        {
            trackid = t.trackid;
            break;
        }
    }

    return xt_del_send(srcno, trackid, ip, port, demux, demuxid);
}

// 删除转发(srcno 源id)
int xt_del_send_src(int srcno)
{
    return XTSrc::instance()->clear_send(srcno);
}

// 删除所有转发
int xt_del_send_all()
{
    return XTSrc::instance()->clear_allsend();
}

//JigaleRTP支持
////////////////////////////////////////////////////
int add_trans_sever(xmpp_cfg_t& cfg)
{
    int ret_code = 0;
    switch(cfg.ctrl)
    {
    case XMPP_START_PLAY:
        {
            do{
                unsigned long chid = cfg.Channle;
                int srcno = chid;

                xt_sink_t sink;
                sink.session = 0;
                sink.chanid = chid;
                sink.addr = cfg.Addr;
                sink.port = cfg.nRptPort_r;
                sink.demux = cfg.multiplex;
                sink.demuxid = cfg.multiplexID;

                ret_code = XTSrc::instance()->add_sink(srcno, sink);
                if (ret_code < 0)
                {
                    ret_code = -1;
                    break;
                }
                ret_code = XTRtp::instance()->add_send(chid, cfg.Addr, cfg.nRptPort_r,NULL, cfg.link_type,0, cfg.multiplex, cfg.multiplexID);
                if (ret_code < 0)
                {
                    ret_code = -2;
                    break;
                }
            }while (false);

            break;
        }

    case XMPP_STOP_PLAY:
        {
            int srcno =  cfg.Channle;

            xt_sink_t sink;
            sink.session = 0;
            sink.chanid = cfg.Channle;
            sink.addr = cfg.Addr;
            sink.port = cfg.nRptPort_r;
            sink.demux = cfg.multiplex;
            sink.demuxid = cfg.multiplexID;
            XTSrc::instance()->del_sink(srcno, sink);
            XTRtp::instance()->del_send(cfg.Channle, cfg.Addr, cfg.nRptPort_r, cfg.link_type, cfg.multiplex, cfg.multiplexID);
            break;
        }

    default:
        {
            break;
        }
    } 
    return ret_code;
}

int xt_get_svr_info(svr_info info[],int& tracknum,const int srcno)
{
    int ret_code =-1;
    do 
    {
        tracknum = 0;
        std::list<xt_track_t> lsttrack;
        ret_code =XTSrc::instance()->get_tracks(srcno,lsttrack);
        if (ret_code < 0)
        {
            break;
        }

        std::list<xt_track_t>::iterator itr = lsttrack.begin();
        for (int index = 0; lsttrack.end() != itr; ++itr)
        {
            if (-1 == itr->trackid)
            {
                continue;
            }

            info[index].trackid = itr->trackid;

            Rtp_Handle rtp;
            XTRtp::instance()->get_rtp(itr->chanid, rtp);

            ::memcpy(info[index].trackname,itr->trackname,64);
            info[index].multid_s = rtp.multid;
            info[index].multiplex_s = rtp.multiplex;
            info[index].rtp_send_port = rtp.port;
            info[index].rtcp_send_port = rtp.port+1;

            ++tracknum;
            ++index;
        }

    } while (false);

    return ret_code;
}

int xt_start_sip(const start_sip_port_t& sip_port, const start_timer_t& timer, const char *domain, const char *username, const char *password)
{
    int ret = 0;
    ret = xt_session_sip::inst()->start(sip_port,timer,domain,username,password);
    if (ret < 0)
    {
        return -1;
    }

    return 0;
}

void xt_stop_sip()
{
    xt_session_sip::inst()->stop();
}

int xt_regist_sip(const regist_timer_t& timer, const sip_channel_t& chtype,const char *target,const char* sdp, uint32_t sdp_len)
{
    if (NULL == target)
    {
        return -1;

    }
    xt_session_sip::inst()->regist(timer,chtype,target,sdp,sdp_len);

    return 0;
}

void xt_unregist_sip()
{
    xt_session_sip::inst()->unregist();
}

void xt_sipmsg_ptz_cb(sipmsg_ptz_cb cb)
{
    xt_session_sip::inst()->set_sipmsg_ptz_cb(cb);
}

void xt_sipmsg_picupdate_cb(sipmsg_picupdate_cb cb)
{
    xt_session_sip::inst()->set_sipmsg_picupdate_cb(cb);
}

void xt_sipmsg_cb(sipmsg_cb cb)
{
    xt_session_sip::inst()->set_sipmsg_cb(cb);
}

void xt_sipinfo_cb(sipinfo_cb cb)
{
    xt_session_sip::inst()->set_sipinfo_cb(cb);
}

void xt_sipmsg_ifrmconfreq_cb(sipmsg_ifrmconfreq_cb cb)
{
    xt_session_sip::inst()->set_sipmsg_ifrmconfreq_cb(cb);
}

void xt_regist_sip_video_adjust_cb(sip_video_adjust_cb cb)
{
    xt_session_sip::inst()->set_sip_video_adjust_cb(cb);
}

void xt_get_dev_info_cb(sip_get_dev_info_cb cb)
{
    xt_session_sip::inst()->set_sip_get_dev_info_cb(cb);
}

void xt_point_index_operation_cb(sip_point_index_operation_cb cb)
{
    xt_session_sip::inst()->set_sip_point_index_operation_cb(cb);
}

void xt_sip_register_srv_ret_info_cb(sip_register_srv_ret_info_cb cb)
{
    xt_session_sip::inst()->set_sip_register_srv_ret_info_cb(cb);
}

void xt_fir_cb(unsigned long chanid)
{
    int srcno = XTSrc::instance()->find_src(chanid);
    if (srcno >= 0)
    {
        if (xt_session_sip::inst()->picupdate_cb_)
        {
            int oprcode = 0;
            char fail_case[2048]={0};
            xt_session_sip::inst()->picupdate_cb_(srcno,oprcode,fail_case);
        }
        if (ms_cfg.rtcp_force_iframe_cb)
        {
            ms_cfg.rtcp_force_iframe_cb(srcno);
        }
    }
}

int xt_update_resend_flag(const int resend_flag)
{
    int ret_code = 0;
    do 
    {
        (void)XTRtp::instance()->update_resend_flag(resend_flag);
    } while (0);
    return ret_code;
}

// 设置文件保存
void xt_set_file_path(const char * file)
{
    do 
    {
        (void)XTRtp::instance()->set_file_patha(file);
    } while (0);
}
////////////////////////////////////////////////////


void xt_sipmsg_bandwidth_cb(sipmsg_bandwidth_cb cb)
{
    xt_session_sip::inst()->set_sipmsg_bandwidth_cb(cb);
}

void xt_sipmsg_profile_cb(sipmsg_profile_cb cb)
{
    xt_session_sip::inst()->set_sipmsg_profile_cb(cb);
}

#ifdef _USE_RTP_SEND_CONTROLLER
int xt_register_network_changed_callback(int srcno, int trackid, xt_network_changed_callback_t cb, void *ctx)
{
    int ret = -1;
    list<xt_track_t> tracks;
    if (0 == XTSrc::instance()->get_tracks(srcno, tracks))
    {
        for (list<xt_track_t>::const_iterator it = tracks.begin(); tracks.end() != it; ++it)
        {
            if (it->trackid == trackid)
            {
                ret = 0;
                XTRtp::instance()->register_network_changed_callback(it->chanid, cb, ctx);
            }
        }
    }

    return ret;
}
#endif // end #ifdef _USE_RTP_SEND_CONTROLLER

int xt_ms_set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return XTSession::instance()->set_rtsp_heartbit_time(check_timer_interval,time_out_interval);
}

void xt_register_multi_code_query_callback(multi_code_query_callback_t cb)
{
    multi_code_mgr::_()->register_callback(cb);
}
