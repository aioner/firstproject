#ifndef XT_ENGINE_H__
#define XT_ENGINE_H__
#include <string>
#include <boost/thread/shared_mutex.hpp>
#include <boost/noncopyable.hpp>

#include "XTSession.h"
#include "XTSrc.h"
#include "h_rtspserver.h"

using namespace std;
using namespace XT_RTSP;

class XTEngine:boost::noncopyable
{
private:
    XTEngine(void);
    ~XTEngine(void);

    static XTEngine self;

public:
    static XTEngine* instance()
    {
        return &self;
    }

    int set_listen_addr(const char ip[32], unsigned short port){ m_ipListen.assign(ip); m_portListen = port; return 0;}
    int get_listen_addr(string &ip, unsigned short &port) { ip = m_ipListen; port = m_portListen; return 0;}

    // ����/ɾ��session
    int add_src(int sessionno);
    int del_src(int sessionno);

    // ����SDP
    int set_sdp(int sessionno, const char *sdp, int len);

    // ���SDP
    int get_sdp(int sessionno, char *sdp, int &len);

    // ����track���Ͷ˿�
    int set_snd_port(int sessionno, int trackid, unsigned short port, bool demux,unsigned int demuxid);

    // ���track���Ͷ˿�
    int get_snd_port(int sessionno, int trackid, unsigned short &port, bool &demux,unsigned int &demuxid);

    // ���ûص�
    int set_add_send_cb(add_send_cb func);
    int set_del_send_cb(del_send_cb func);
    int set_rtsp_play_cb(rtsp_play_cb func);
    int set_rtsp_pause_cb(rtsp_pause_cb func);

    // ����/ɾ��ת��
    int add_send(void *hSession,int session, int trackid, const char *pstrRAddr,  unsigned short nRPort, bool multiplex, unsigned int multiplexID);
    int del_send(void *hSession,int session, int trackid, const char *pstrRAddr,  unsigned short nRPort, bool multiplex, unsigned int multiplexID);
    int del_send_session(void *session); 
    int del_send_connection(void *connection);

    //��������
    int do_play(void *hSession, int sessionno, int trackid, double npt, float scale, unsigned int *rtp_pkt_seq, unsigned int *rtp_pkt_timestamp);
    int do_pause(void *hSession, int sessionno, int trackid);

    int set_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
    {
        check_timer_interval_ = check_timer_interval;
        time_out_interval_ = time_out_interval;
        return 0;
    }
    int get_heartbit_time(unsigned int & check_timer_interval,unsigned int &time_out_interval)
    {
        check_timer_interval = check_timer_interval_;
        time_out_interval = time_out_interval_;
        return 0;
    }

    void set_max_session(unsigned int max_session){max_session_ = max_session;}
    unsigned int get_max_session(){return max_session_;}

private:
    boost::shared_mutex	  m_mutex;	//mutex(ȫ��)
    boost::shared_mutex	   m_mtxSrc;	//mutex(m_srcs)
    string m_ipListen;					//����IP
    unsigned short m_portListen;		//�����˿�
    add_send_cb m_fnAddSend;			//����ת��
    del_send_cb m_fnDelSend;			//ɾ��ת��
    rtsp_play_cb m_fnRtspPlay;
    rtsp_pause_cb m_fnRtspPause;
    map<int, rtsp_src*> m_srcs;			//Դ
    unsigned int check_timer_interval_;  //��ʱˢ��ʱ����
    unsigned int time_out_interval_;     //��ʱʱ����

    unsigned int max_session_;

};
#endif //#ifdef XT_ENGINE_H__
