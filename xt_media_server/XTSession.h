#ifndef XTSESSION_H__INCLUDE_
#define XTSESSION_H__INCLUDE_

#ifndef CLOSE_SESSION

#include "h_xtmediaserver.h"
#include <string>
#include "h_xtsessionserver.h"
extern int xt_rtsp_ctrl(Msg_Rtsp_Ctrl *msgData);

class XTSession
{
public:
    //流化控制回调
    //RTSP
    static  int SESSIONSERVER_STDCALL  rtsp_play_cb(void *hSession, int sessionno, int trackid, double npt,float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);
    static  int SESSIONSERVER_STDCALL  rtsp_pause_cb(void *hSession, int sessionno, int trackid);

    //TCP
    static  int SESSIONSERVER_STDCALL tcp_play_cb(void* p_context,uint32_t data_chid,double npt,float scale,uint32_t *rtp_pkt_timestamp);
    static  int SESSIONSERVER_STDCALL tcp_pause_cb(void* p_context,uint32_t data_chid);
    static void SESSIONSERVER_STDCALL xt_session_query_real_ch_by_request_ch_cb(const int request_ch,const long stream_type,int* real_ch);
    static ms_code_t ms_stream_code_type_cast(const long stream_type);

private:
    XTSession(void);
    ~XTSession(void);
    static XTSession self;

public:
    static XTSession* instance(){return &self;}

    // 初始化tcp
    int init_msg(std::string ip, // 本地IP
        unsigned short msg_listen_port, // msg监听端口
        unsigned short udp_listen_port,       //udp侦听端口
        unsigned short send_start_port, // 发送起始端口
        const char *mul_start_addr, // 组播发送起始地址
        unsigned short mul_port, // 组播发送端口
        xt_print_cb func); // 日志输出

    void register_common_cb();

    // 初始化rtsp库(listen_port 服务监听端口)
    int init_rtsp(std::string ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func);
    int set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

    // 反初始化
    int uninit_msg();
    int uninit_rtsp();

    // 设置会话回调
    int set_sessionmsg_cb(session_msg_cb func);

    // 设置系统头
    int set_key_data(unsigned long nDataChID, char *pHeadData, long nHeadSize, long nDataType);

    // 设置发送端口
    int set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid);
    int set_snd_port(unsigned long nDataChID, unsigned short sndport, bool demux,unsigned int demuxid);

    // 清除系统头
    int  clear_key_data(unsigned long nDataChID);

    // 增加/删除源
    int add_src(int srcno);
    int del_src(int srcno);
    // 设置SDP
    int set_sdp(int srcno, const char *sdp, int len);
    void set_rtsp_play_cb_func(xt_rtsp_play_cb rtsp_play)
    {
        rtsp_play_ = rtsp_play;	

    }
    void set_rtsp_pause_cb_func(xt_rtsp_pause_cb rtsp_pause)
    {
        rtsp_pause_ = rtsp_pause;
    }

    void set_tcp_play_cb_func(xt_tcp_play_cb_type tcp_play)
    {
        tcp_play_ = tcp_play;

    }
    void set_tcp_pause_cb_func(xt_tcp_pause_cb_type tcp_pause)
    {
        tcp_pause_ = tcp_pause;
    }

private:
    xt_rtsp_play_cb rtsp_play_;
    xt_rtsp_pause_cb rtsp_pause_;
    xt_tcp_play_cb_type tcp_play_;
    xt_tcp_pause_cb_type tcp_pause_;
};

#endif//CLOSE_SESSION

#endif//XTSESSION_H__INCLUDE_
