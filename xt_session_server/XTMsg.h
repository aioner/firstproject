#ifdef _USE_XT_TCP_SESSION_SERVER
#ifndef XTMSG_H__
#define XTMSG_H__
#include <boost/thread/shared_mutex.hpp>
#include <string>
#include <stdint.h>
#include "xt_log_def.h"

#include "tcp_session_server.h"

using namespace std;

#define TCP_SESSION_MULTICAST_IP  40
#define TCP_SESSION_SERVER_IP 32

struct channel_info 
{
    uint32_t	chid;
    uint16_t	rtp_port;
    uint16_t	rtcp_port;
    bool		demux;
    uint32_t	demux_id;
    char		sdp[2048];
    uint32_t	sdp_len; 
    uint32_t	data_type;

    //added by lichao, 20151127 增加构造函数
    channel_info()
        :chid(0)
        ,rtp_port(0)
        ,rtcp_port(0)
        ,demux(false)
        ,demux_id(0)
        ,sdp()
        ,sdp_len(0)
        ,data_type(0)
    {}
};

class XTMsg
{
private:
    XTMsg(void);
    ~XTMsg(void);

    static XTMsg self;

public:
    static XTMsg* instance(){return &self;}

    // 初始化
    int init(string ip,
        unsigned short listen_port,
        unsigned short send_start_port,
        const char *mul_start_addr,
        unsigned short mul_port,
		xt_print_cb func);

    // 反初始化
    int uninit();

    // 设置系统头

    int set_key_data(uint32_t chid, char *pHeadData,uint32_t nHeadSize, uint32_t nDataType);



    int set_snd_port(uint32_t chid, unsigned short sndport, bool multiplex,unsigned int multid);

    // 清除系统头
    int  clear_key_data(uint32_t nDataChID);

    int add_socket_client(void *sock);
    /*set_play_cb();*/

    int get_ch_info(const uint16_t channel,uint32_t *sdp_len,char * sdp_info,uint32_t *multiplex,uint32_t *multiplex_id,uint32_t *data_type,uint32_t *data_size,uint32_t *rtp_port);

    int clear_ch_info(const uint32_t chid);

    int upate_ch_info(const uint32_t chid,const uint16_t rtp_port,const uint16_t rtcp_port,const bool multiplex,const unsigned int multid);
    int save_sdp_to_ch_info(const uint32_t chid, char *sdp, const uint32_t sdp_len, const uint32_t data_type);

public:

    static void TCP_SESSION_SERVER_STDCALL close_connect_callback(void* pContext,
        char   *remote_ip,
        uint16_t remote_port,
        uint32_t nReason);


    static void TCP_SESSION_SERVER_STDCALL stop_rtp_link_callback(void* pContext,
        char   *local_ip, 
        char   *remote_ip,
        uint16_t remote_port,
        uint16_t port,
        uint16_t channel,
        uint16_t mode,
        uint32_t multiplex,
        uint32_t multiplex_id);


    static void TCP_SESSION_SERVER_STDCALL start_rtp_link_callback (void* pContext,
        char   *local_ip,
        char   *remote_ip,
        uint16_t remote_port,
        uint16_t port,
        uint16_t channel,
        uint16_t mode,
        uint32_t ssrc,
        uint32_t multiplex,
        uint32_t multiplex_id);



    static void TCP_SESSION_SERVER_STDCALL  get_sdp_callback(void* pContext,
        char   *remote_ip,
        uint16_t remote_port,
        uint16_t channel,
        char *sdp_info,
        uint32_t *data_type,
        uint32_t *data_size,
        uint32_t *sdp_len,
        uint32_t *multiplex,
        uint32_t *multiplex_id,
        uint32_t *rtp_port);	

    static void TCP_SESSION_SERVER_STDCALL get_mul_info_callback(void *pContext,
        char*multi_info,
        uint16_t *multi_len	);

private:
    boost::shared_mutex m_mChInf_mutex_; //m_mChInf 的资源锁
    std::map<uint32_t,channel_info> m_mChInf;
    char m_multicast_ip[TCP_SESSION_MULTICAST_IP];
    uint16_t m_multicast_port;
    uint16_t m_snd_start_port;
};
#endif //#ifndef XTMSG_H__
#endif //_USE_XT_TCP_SESSION_SERVER

