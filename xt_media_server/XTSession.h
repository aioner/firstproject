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
    //�������ƻص�
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

    // ��ʼ��tcp
    int init_msg(std::string ip, // ����IP
        unsigned short msg_listen_port, // msg�����˿�
        unsigned short udp_listen_port,       //udp�����˿�
        unsigned short send_start_port, // ������ʼ�˿�
        const char *mul_start_addr, // �鲥������ʼ��ַ
        unsigned short mul_port, // �鲥���Ͷ˿�
        xt_print_cb func); // ��־���

    void register_common_cb();

    // ��ʼ��rtsp��(listen_port ��������˿�)
    int init_rtsp(std::string ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func);
    int set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

    // ����ʼ��
    int uninit_msg();
    int uninit_rtsp();

    // ���ûỰ�ص�
    int set_sessionmsg_cb(session_msg_cb func);

    // ����ϵͳͷ
    int set_key_data(unsigned long nDataChID, char *pHeadData, long nHeadSize, long nDataType);

    // ���÷��Ͷ˿�
    int set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid);
    int set_snd_port(unsigned long nDataChID, unsigned short sndport, bool demux,unsigned int demuxid);

    // ���ϵͳͷ
    int  clear_key_data(unsigned long nDataChID);

    // ����/ɾ��Դ
    int add_src(int srcno);
    int del_src(int srcno);
    // ����SDP
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
