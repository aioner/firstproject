#include "XTUDPMsg.h"
#include <string.h>
extern session_msg_cb g_fnSessionMsg;
extern void query_real_ch(const int request_ch,long stream_type,int* real_ch);

XTUDPMsg::XTUDPMsg(void)
:m_udp_workT(0)
,regist_response_cb(NULL)
{
}

XTUDPMsg::~XTUDPMsg(void)
{
}

int XTUDPMsg::init(std::string ip, unsigned short listen_port, uint16_t snd_port, xt_print_cb func)
{
    m_run = true;

    m_snd_port = snd_port;

#ifdef _ANDROID
    m_udp_service = xt_udp_server_create_service();
#else
    m_udp_service = xt_udp_create_service();
#endif

    xt_udp_session_config_t cfg;
    strcpy(cfg.ip, ip.c_str());
    cfg.port = listen_port;
    cfg.service = m_udp_service;
    cfg.heartbit_check_millsec = 1000;
    cfg.client_overtime_millsec = 20000;
    cfg.query_sdp_callback = query_sdp_callback;
    cfg.add_sink_callback = add_sink_callback;
    cfg.del_sink_callback = del_sink_callback;
    cfg.send_regist_data_callback = send_regist_calllback;
#ifdef _ANDROID
    m_udp_session = xt_udp_server_session_init(&cfg);
#else
    m_udp_session = xt_udp_session_init(&cfg, func);
#endif

    m_udp_workT = new boost::thread(udp_work_thread, this);

    return 0;
}

int XTUDPMsg::uninit()
{
    m_run = false;

#ifdef _ANDROID
    xt_udp_server_session_term(m_udp_session);
    xt_udp_server_destroy_service(m_udp_service);
#else
    xt_udp_session_term(m_udp_session);
    xt_udp_destroy_service(m_udp_service);
#endif

    if (m_udp_workT)
    {
        m_udp_workT->join();

        delete	 m_udp_workT;
        m_udp_workT = 0;
    }

    return 0;
}

int XTUDPMsg::set_key_data(unsigned long chid, char *pHeadData, long nHeadSize, long nDataType)
{
    m_mChInf[chid].sdp_len = nHeadSize;
    m_mChInf[chid].data_type = nDataType;
    ::memcpy(m_mChInf[chid].sdp,pHeadData,nHeadSize);
    return 0;
}

int  XTUDPMsg::set_mult_data(unsigned long chid, unsigned short port, bool demux,unsigned int demux_id)
{
    uint16_t rtp = m_snd_port;
    uint16_t rtcp = m_snd_port;
    if (!demux)
    {
        rtp = m_snd_port + 2*chid;
        rtcp = rtp+1;
    }

    if (port > 0)
    {
        rtp = port;
        rtcp = port + 1;
    }

    m_mChInf[chid].demux = demux;
    m_mChInf[chid].demux_id = demux_id;
    m_mChInf[chid].rtp_port = rtp;
    m_mChInf[chid].rtcp_port = rtcp;
    return 0;
}

void XTUDPMsg::udp_work_thread(void *param)
{
    while (inst()->m_run)
    {
#ifdef _ANDROID
        xt_udp_server_run_service(inst()->m_udp_service);
#else
        xt_udp_run_service(inst()->m_udp_service);
#endif

        boost::this_thread::sleep(boost::posix_time::millisec(10));
    }
}

int32_t XTUDPMsg::query_sdp_callback(uint32_t channel, uint8_t code, uint8_t *sdp, uint32_t *sdp_len)
{
    int real_ch = -1;
    query_real_ch(channel, code,&real_ch);
    if (real_ch < 0)
    {
        real_ch = channel;
    }

    if (inst()->m_mChInf.find(real_ch)!=inst()->m_mChInf.end())
    {
        *sdp_len = inst()->m_mChInf[real_ch].sdp_len;
        ::memcpy(sdp, inst()->m_mChInf[real_ch].sdp, *sdp_len);
    }

    return 0;
}
int32_t XTUDPMsg::add_sink_callback(uint32_t channel, uint8_t code, uint16_t trackid, const char *ip, uint16_t *rtp_port, uint16_t *rtcp_port, uint8_t *demux_flag, uint32_t *demux_id)
{
    STR_MsgData MsgData;
    MsgData.ctrl = MSG_STARTPLAY;
    MsgData.pMsgContext = NULL;
    strncpy(MsgData.LAddr, "0.0.0.0", SESSION_SERVER_IP_LEN);
    strncpy(MsgData.Addr, ip, SESSION_SERVER_IP_LEN);
    MsgData.nMsgPort = 0;

    int real_ch = -1;
    query_real_ch(channel,code,&real_ch);
    if (real_ch < 0)
    {
        real_ch = channel;
    }

    MsgData.nDataChannelID = real_ch;
    MsgData.nMode = 13;
    MsgData.nRecvPort = *rtp_port;
    MsgData.nSSRC = 0;
    MsgData.multiplex = *demux_flag;
    MsgData.multiplexID = *demux_id;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_MSG, &MsgData);
    }

    if (inst()->m_mChInf.find(real_ch)!=inst()->m_mChInf.end())
    {
        *rtp_port = inst()->m_mChInf[real_ch].rtp_port;
        *rtcp_port = inst()->m_mChInf[real_ch].rtcp_port;
        *demux_flag= inst()->m_mChInf[real_ch].demux;
        *demux_id = inst()->m_mChInf[real_ch].demux_id;
    }
    return 0;
}
int32_t XTUDPMsg::del_sink_callback(uint32_t channel, uint8_t code, uint16_t trackid, const char *ip, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id)
{
    STR_MsgData MsgData;
    MsgData.ctrl = MSG_STOPPLAY;
    MsgData.pMsgContext = NULL;
    strncpy(MsgData.LAddr, "0.0.0.0", SESSION_SERVER_IP_LEN);
    strncpy(MsgData.Addr, ip, SESSION_SERVER_IP_LEN);

    MsgData.nMsgPort = 0;

    int real_ch = -1;
    query_real_ch(channel,code,&real_ch);
    if (real_ch < 0)
    {
        real_ch = channel;
    }

    MsgData.nDataChannelID = real_ch;
    MsgData.nMode = 13;
    MsgData.nRecvPort = rtp_port;

    MsgData.nSSRC = 0;
    MsgData.multiplex = demux_flag;
    MsgData.multiplexID = demux_id;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_MSG, &MsgData);
    }
    return 0;
}

int32_t  XTUDPMsg::send_regist_calllback( const char *ip, uint16_t port, uint32_t code )
{
    if (inst()->regist_response_cb != NULL )
    {
        inst()->regist_response_cb(ip,port,code);
        return 0;
    }
    else
        return -1;
}

int  XTUDPMsg::xt_regist( const std::string &regist_ids,const std::string &server_ip, unsigned short server_port,uint32_t millisec )
{

    xt_send_regist_request_t request;
    memset(&request,0,sizeof(xt_send_regist_request_t));
    xt_send_regist_response_t response;
    memset(&response,0,sizeof(xt_send_regist_response_t));
    memcpy(request.ids,regist_ids.c_str(),regist_ids.length());
    request.length = regist_ids.length();

    int ret = ::xt_udp_server_session_send_regist(m_udp_session,server_ip.c_str(),server_port,&request,&response,NULL,NULL,millisec);
    return ret;
}

int XTUDPMsg::xt_stop_regist( const std::string &server_ip, unsigned short server_port,uint32_t millisec )
{
    xt_send_regist_request_t request;
    xt_send_regist_response_t response;
    memset(&request,0,sizeof(xt_send_regist_request_t));
    memset(&response,0,sizeof(xt_send_regist_response_t));
    int ret = ::xt_udp_server_session_send_stop_regist(m_udp_session,server_ip.c_str(),server_port,&request,&response,NULL,NULL,millisec);
    return ret;
}

void XTUDPMsg::set_regist_response_callback( regist_response_callback_t cb )
{
    inst()->regist_response_cb = cb;
}

