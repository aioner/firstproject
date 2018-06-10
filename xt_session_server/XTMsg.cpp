#ifdef _USE_XT_TCP_SESSION_SERVER

#include "XTMsg.h"
#include "h_xtsessionserver.h"
#include <stdio.h>
#include <boost/thread/lock_types.hpp>

extern session_msg_cb g_fnSessionMsg;
extern xt_print_cb session_svr_print_;
extern void query_real_ch(const int request_ch,long stream_type,int* real_ch);

XTMsg XTMsg::self;
XTMsg::XTMsg(void)
{}

XTMsg::~XTMsg(void)
{}
int XTMsg::init(string ip, unsigned short listen_port, unsigned short send_start_port, const char *mul_start_addr, unsigned short mul_port, xt_print_cb func)
{ 
    //////////////////////////////////////////////////////////////////////////
    tcp_session_server_param_t server_param;
    memset(&server_param, 0, sizeof(tcp_session_server_param_t));

    server_param.listen_port = listen_port;
    server_param.multicast_port = mul_port;
#ifdef _OS_WINDOWS
    _snprintf(server_param.ip, TCP_SESSION_SERVER_IP_LEN, ip.c_str());
    _snprintf(server_param.multicast_ip, TCP_SESSION_SERVER_IP_LEN, mul_start_addr);
#else
    snprintf(server_param.ip, TCP_SESSION_SERVER_IP_LEN, ip.c_str());
    snprintf(server_param.multicast_ip, TCP_SESSION_SERVER_IP_LEN, mul_start_addr);
#endif//#ifdef _OS_WINDOWS	
    server_param.close_connect_cb = close_connect_callback;
    server_param.stop_link_cb = stop_rtp_link_callback;
    server_param.start_link_cb = start_rtp_link_callback;
    server_param.get_mul_cb = get_mul_info_callback;
    server_param.get_sdp_cb = get_sdp_callback;
    server_param.print_cb = func;
    //////////////////////////////////////////////////////////////////////////
    (void)strncpy(m_multicast_ip,server_param.multicast_ip,TCP_SESSION_MULTICAST_IP);
    m_multicast_port = mul_port;
    m_snd_start_port = send_start_port;

    tcp_session_server_start(&server_param);
    return 0;
}

int XTMsg::uninit()
{
    tcp_session_server_stop();
    return 0;
}

int XTMsg::set_key_data(uint32_t chid, 
                        char *pHeadData, 
                        uint32_t nHeadSize, 
                        uint32_t nDataType)
{
    return save_sdp_to_ch_info(chid,pHeadData,nHeadSize,nDataType);
}

int  XTMsg::set_snd_port(uint32_t chid, unsigned short sndport, bool multiplex,unsigned int multid)
{
    uint16_t rtp = sndport;
    uint16_t rtcp = rtp + 1;
    if (!multiplex)
    {
        rtp = sndport + 2*chid;
        rtcp = rtp+1;
    }
    return upate_ch_info(chid,rtp,rtcp,multiplex,multid);
}

int  XTMsg::clear_key_data(uint32_t  chid)
{
    return clear_ch_info(chid);
}

int XTMsg::add_socket_client(void *sock)
{
    return 0;
}

void XTMsg::start_rtp_link_callback(void* pContext,
                                    char   *local_ip,
                                    char   *remote_ip,
                                    uint16_t remote_port,
                                    uint16_t port,
                                    uint16_t channel,
                                    uint16_t mode,
                                    uint32_t ssrc,
                                    uint32_t multiplex,
                                    uint32_t multiplex_id)
{

    STR_MsgData MsgData;
    memset(&MsgData, 0, sizeof(MsgData));
    MsgData.ctrl = MSG_STARTPLAY;
    MsgData.pMsgContext = pContext;
    (void)strncpy(MsgData.LAddr,local_ip,SESSION_SERVER_IP_LEN);
    (void)strncpy(MsgData.Addr,remote_ip,SESSION_SERVER_IP_LEN);

    MsgData.nMsgPort = remote_port;


    MsgData.nRecvPort = port;
    int real_ch = -1;
    query_real_ch(channel,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = channel;
    }
    MsgData.nDataChannelID = real_ch;
    MsgData.nMode = mode;

    MsgData.nSSRC = ssrc;
    MsgData.multiplex = multiplex;
    MsgData.multiplexID = multiplex_id;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_MSG, &MsgData);
    }
}

void XTMsg::close_connect_callback(void* pContext, char   *remote_ip, uint16_t remote_port, uint32_t nReason)
{
    STR_MsgData MsgData;
    memset(&MsgData, 0, sizeof(MsgData));

    MsgData.ctrl = MSG_CLOSEMSG;
    MsgData.pMsgContext = pContext;
    (void)strncpy(MsgData.Addr,remote_ip,SESSION_SERVER_IP_LEN);
    MsgData.nMsgPort = remote_port;

    MsgData.nSSRC = 0;
    MsgData.multiplex = 0;
    MsgData.multiplexID = 0;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_MSG, &MsgData);
    }
}

void XTMsg::stop_rtp_link_callback(void* pContext,
                                   char   *local_ip, 
                                   char   *remote_ip,
                                   uint16_t remote_port,
                                   uint16_t port,
                                   uint16_t channel,
                                   uint16_t mode,
                                   uint32_t multiplex,
                                   uint32_t multiplex_id)
{
    STR_MsgData MsgData;
    memset(&MsgData, 0, sizeof(MsgData));

    MsgData.ctrl = MSG_STOPPLAY;
    MsgData.pMsgContext = pContext;
    (void)strncpy(MsgData.LAddr,local_ip,SESSION_SERVER_IP_LEN);
    (void)strncpy(MsgData.Addr,remote_ip,SESSION_SERVER_IP_LEN);

    MsgData.nMsgPort = remote_port;

    int real_ch = -1;
    query_real_ch(channel,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = channel;
    }
    MsgData.nDataChannelID = real_ch;
    MsgData.nRecvPort = port;
    MsgData.nMode = mode;

    MsgData.nSSRC = 0;
    MsgData.multiplex = multiplex;
    MsgData.multiplexID = multiplex_id;

    if (g_fnSessionMsg)
    {
        g_fnSessionMsg(PT_MSG, &MsgData);
    }
}

int XTMsg::get_ch_info(const uint16_t channel,uint32_t *sdp_len,char * sdp_info,uint32_t *multiplex,uint32_t *multiplex_id,uint32_t *data_type,uint32_t *data_size,uint32_t *rtp_port)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mChInf_mutex_);
    int real_ch = -1;
    query_real_ch(channel,-1,&real_ch);
    if (real_ch < 0)
    {
        real_ch = channel;
    }
    std::map<uint32_t,channel_info>::iterator it = m_mChInf.find(real_ch);
    if(it != m_mChInf.end())
    {
        *sdp_len = it->second.sdp_len;
        ::memcpy(sdp_info, it->second.sdp, *sdp_len);
        *multiplex = it->second.demux;
        *multiplex_id = it->second.demux_id;
        *data_type = it->second.data_type;
        *data_size = *sdp_len;
        *rtp_port = it->second.rtp_port;
        return 0;
    }
    return -1;
}
int XTMsg::clear_ch_info(const uint32_t chid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mChInf_mutex_);
    std::map<uint32_t,channel_info>::iterator it = m_mChInf.find(chid);
    if (it != m_mChInf.end())
    {
        m_mChInf.erase(it);
        return 0;
    }
    return -1;
}
int XTMsg::upate_ch_info(const uint32_t chid,const uint16_t rtp_port,const uint16_t rtcp_port,const bool multiplex,const unsigned int multid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mChInf_mutex_);
    std::map<uint32_t,channel_info>::iterator it = m_mChInf.find(chid);
    if (m_mChInf.end() == it)
    {
        it = m_mChInf.insert(std::make_pair(chid, channel_info())).first;
    }

    it->second.demux = multiplex;
    it->second.demux_id = multid;
    it->second.rtp_port = rtp_port;
    it->second.rtcp_port = rtcp_port;
    return 0;
}
int XTMsg::save_sdp_to_ch_info(const uint32_t chid, char *sdp, const uint32_t sdp_len, const uint32_t data_type)
{
    if (sdp_len > 2048)
    {
        return -1;
    }

    boost::unique_lock<boost::shared_mutex> lock(m_mChInf_mutex_);
    std::map<uint32_t,channel_info>::iterator it = m_mChInf.find(chid);
    if (m_mChInf.end() == it)
    {
        it = m_mChInf.insert(std::make_pair(chid, channel_info())).first;
    }

    it->second.chid = chid;
    it->second.sdp_len = sdp_len;
    it->second.data_type = data_type;
    ::memcpy(it->second.sdp,sdp,sdp_len);
    return 0;
}


void XTMsg::get_sdp_callback(void* pContext,
                             char   *remote_ip,
                             uint16_t remote_port,
                             uint16_t channel,
                             char *    sdp_info,
                             uint32_t *data_type,
                             uint32_t *data_size,
                             uint32_t *sdp_len,
                             uint32_t *multiplex,
                             uint32_t *multiplex_id,
                             uint32_t *rtp_port )
{
    instance()->get_ch_info(channel,sdp_len,sdp_info,multiplex,multiplex_id,data_type,data_size,rtp_port);
}

void XTMsg::get_mul_info_callback( void *pContext, char*multi_info, uint16_t *multi_len )
{
    *multi_len = 0;
    memcpy(multi_info+(*multi_len),&(instance()->m_multicast_port),sizeof(instance()->m_multicast_port));
    *multi_len+=sizeof(instance()->m_multicast_port);
    memcpy(multi_info+(*multi_len),instance()->m_multicast_ip,TCP_SESSION_MULTICAST_IP);
    *multi_len+=TCP_SESSION_MULTICAST_IP;
    memcpy(multi_info+(*multi_len),&(instance()->m_snd_start_port),sizeof(instance()->m_snd_start_port));
    *multi_len+=sizeof(instance()->m_snd_start_port);
}
#endif //_USE_XT_TCP_SESSION_SERVER
