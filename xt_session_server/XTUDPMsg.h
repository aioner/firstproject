#ifndef XTUDPMSG_H__
#define XTUDPMSG_H__
#include "h_xtsessionserver.h"
#include "xt_udp_session_server.h"
#include <string>
#include <map>
#include <boost/thread.hpp>

struct ch_inf 
{
    uint16_t rtp_port;
    uint16_t rtcp_port;
    bool		demux;
    uint32_t	demux_id;
    char		sdp[2048];
    uint32_t	sdp_len; 
    uint32_t	data_type;
};

class XTUDPMsg
{
private:
    XTUDPMsg(void);
    ~XTUDPMsg(void);

public:
    static XTUDPMsg* inst()
    {
        static XTUDPMsg msg;
        return &msg;
    }

    static void udp_work_thread(void *param);

    static int32_t UDP_SESSION_SERVER_CALLBACK query_sdp_callback(uint32_t channel, uint8_t code, uint8_t *sdp, uint32_t *sdp_len);

    static int32_t UDP_SESSION_SERVER_CALLBACK add_sink_callback(uint32_t channel, uint8_t code, uint16_t trackid, const char *ip, uint16_t *rtp_port, uint16_t *rtcp_port, uint8_t *demux_flag, uint32_t *demux_id);

    static int32_t UDP_SESSION_SERVER_CALLBACK del_sink_callback(uint32_t channel, uint8_t code, uint16_t trackid, const char *ip, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id);

    static int32_t UDP_SESSION_SERVER_CALLBACK send_regist_calllback(const char *ip, uint16_t port,uint32_t code);

    // 初始化
    int init(std::string ip, unsigned short listen_port, uint16_t snd_port, xt_print_cb func);

    // 反初始化
    int uninit();

    // 设置系统头
    int set_key_data(unsigned long chid, char *pHeadData, long nHeadSize, long nDataType);

    // 设置复用参数
    int  set_mult_data(unsigned long chid, unsigned short port, bool demux,unsigned int demux_id);

    //????????
    void set_regist_response_callback(regist_response_callback_t cb);

    int xt_regist(const std::string &regist_ids,const std::string &server_ip, unsigned short server_port,uint32_t millisec);

    int xt_stop_regist( const std::string &server_ip, unsigned short server_port,uint32_t millisec);

private:
    bool m_run;

    boost::thread	*m_udp_workT;

    udp_service_handle m_udp_service;

    udp_session_handle m_udp_session;

    std::map<uint32_t,ch_inf> m_mChInf;

    uint16_t m_snd_port;

    regist_response_callback_t regist_response_cb;
};
#endif //#ifndef XTUDPMSG_H__
