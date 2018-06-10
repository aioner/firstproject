//#include "config.h"
#include "XTEngine.h"
#include <stdio.h>

extern void rtsp_query_real_ch(const int request_ch,const long stream_type,int* real_ch);

XTEngine XTEngine::self;
XTEngine::XTEngine(void)
:m_ipListen("0.0.0.0")
,m_portListen(1554)
,max_session_(128)
,m_fnAddSend(NULL)
,m_fnDelSend(NULL)
,m_fnRtspPlay(NULL)
,m_fnRtspPause(NULL)
{}
XTEngine::~XTEngine(void){}

int XTEngine::add_src(int session_no)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mtxSrc);
    if (m_srcs.find(session_no) != m_srcs.end())
    {
        return -1;
    }
    rtsp_src *session = new rtsp_src;
    if (!session)
    {
        return -1;
    }
    m_srcs[session_no] = session;
    return 0;
}

int XTEngine::del_src(int session_no)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mtxSrc);
    if (m_srcs.find(session_no) == m_srcs.end())
    {
        return -1;
    }

    rtsp_src *session = m_srcs[session_no];
    if (session)
    {
        delete session;
    }

    m_srcs.erase(session_no);

    return 0;
}

// 设置SDP
int XTEngine::set_sdp(int sessionno, const char *sdp, int len)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mtxSrc);

    if (m_srcs.find(sessionno) == m_srcs.end())
    {
        return -1;
    }

    return m_srcs[sessionno]->set_sdp(sdp, len);
}

// 设置track发送端口
int XTEngine::set_snd_port(int sessionno, int trackid, unsigned short port, bool demux,unsigned int demuxid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mtxSrc);
    if (m_srcs.find(sessionno) == m_srcs.end())
    {
        return -1;
    }

    return m_srcs[sessionno]->set_snd_port(trackid, port, demux, demuxid);
}

// 获得SDP
int XTEngine::get_sdp(int sessionno, char *sdp, int &len)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mtxSrc);
    int real_ch = -1;
    rtsp_query_real_ch(sessionno,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = sessionno;
    }
    if (m_srcs.find(real_ch) == m_srcs.end())
    {
        return -1;
    }

    return m_srcs[real_ch]->get_sdp(sdp, len);
}

// 获得track发送端口
int XTEngine::get_snd_port(int sessionno, int trackid, unsigned short &port, bool &demux,unsigned int &demuxid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mtxSrc);
    int real_ch = -1;
    rtsp_query_real_ch(sessionno,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = sessionno;
    }
    if (m_srcs.find(real_ch) == m_srcs.end())
    {
        return -1;
    }
    return m_srcs[real_ch]->get_snd_port(trackid, port, demux, demuxid);
}

// 设置回调
int XTEngine::set_add_send_cb(add_send_cb func)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_fnAddSend = func;
    return 0;
}

int XTEngine::set_del_send_cb(del_send_cb func)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_fnDelSend = func;
    return 0;
}

int XTEngine::set_rtsp_play_cb(rtsp_play_cb func)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_fnRtspPlay = func;
    return 0;
}
int XTEngine::set_rtsp_pause_cb(rtsp_pause_cb func)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_fnRtspPause = func;
    return 0;
}
// 建立/删除转发
int XTEngine::add_send(void *hSession, int sessionno, int trackid, const char *pstrRAddr,  unsigned short nRPort, bool multiplex, unsigned int multiplexID)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    if (!m_fnAddSend && !pstrRAddr)
    {
        return -1;
    }
    rtsp_sink sink;
    sink.trackid = trackid;
    sink.ip.assign(pstrRAddr);
    sink.portA = nRPort;
    sink.portB = nRPort+1;
    sink.multiplex = multiplex;
    sink.multiplexID = multiplexID;
    rtsp_session::inst()->add_sink(hSession, sink);

    int real_ch = -1;
    rtsp_query_real_ch(sessionno,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = sessionno;
    }
    return m_fnAddSend(hSession, real_ch, trackid, pstrRAddr, nRPort, multiplex, multiplexID);
}
int XTEngine::del_send(void *hSession, int sessionno, int trackid, const char *pstrRAddr,  unsigned short nRPort, bool multiplex, unsigned int multiplexID)
{
    if (NULL == m_fnDelSend) return -1;
    rtsp_sink sink;
    sink.trackid = trackid;
    sink.ip = pstrRAddr;
    sink.portA = nRPort;
    sink.portB = nRPort+1;
    sink.multiplex = multiplex;
    sink.multiplexID = multiplexID;
    rtsp_session::inst()->del_sink(hSession, sink);

    int real_ch = -1;
    rtsp_query_real_ch(sessionno,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = sessionno;
    }

    return m_fnDelSend(hSession, real_ch, trackid, pstrRAddr, nRPort, multiplex, multiplexID);
}

int XTEngine::del_send_session(void *session)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    xt_session session_;
    if (rtsp_session::inst()->get_session(session, session_) == 0)
    {
        for (int nI = 0;nI < MAX_TRACK;++nI)
        {
            rtsp_sink &s = session_.sink[nI];
            if (s.ip.length() != 0)
            {
                del_send(session,session_.session_no, s.trackid, s.ip.c_str(), s.portA, s.multiplex, s.multiplexID);
            }
        }
    }
    return 0;
}

int XTEngine::del_send_connection(void *connection)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    std::map<void*, xt_session> conn_;
    if (rtsp_session::inst()->get_connection(connection, conn_) == 0)
    {
        std::map<void*, xt_session>::iterator itr = conn_.begin();
        for (;itr != conn_.end();++itr)
        {
            xt_session &session = itr->second;
            for (int nI = 0;nI < MAX_TRACK;++nI)
            {
                rtsp_sink &s = session.sink[nI];
                if (s.ip.length() != 0)
                {
                    del_send(session.session, session.session_no, s.trackid, s.ip.c_str(), s.portA, s.multiplex, s.multiplexID);
                }
            }
        }
    }
    return 0;
}
int XTEngine::do_play(void *hSession, int sessionno, int trackid, double npt, float scale, unsigned int *rtp_pkt_seq, unsigned int *rtp_pkt_timestamp)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    if (m_fnRtspPlay)
    {
        int real_ch = -1;
        rtsp_query_real_ch(sessionno,-1,&real_ch);
        if (real_ch < 0)
        {
            real_ch = sessionno;
        }
        m_fnRtspPlay(hSession, real_ch, trackid, npt, scale, rtp_pkt_seq, rtp_pkt_timestamp);
    }
    return 0;
}

int XTEngine::do_pause(void *hSession, int sessionno, int trackid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    if (m_fnRtspPause)
    {
        int real_ch = -1;
        rtsp_query_real_ch(sessionno,-1,&real_ch);
        if (real_ch < 0)
        {
            real_ch = sessionno;
        }

        m_fnRtspPause(hSession, real_ch, trackid);
    }
    return 0;
}
