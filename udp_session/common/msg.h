#ifndef _COMMON_MSG_H_INCLUDED
#define _COMMON_MSG_H_INCLUDED

#include<stdint.h>

#pragma pack(push, 1)

#define UDP_SESSION_MSG_VERSION   3
#define MAX_SDP_LENGTH  2048

namespace udp_session
{
    enum msg_code
    {
        get_sdp = 1,
        play = 2,
        stop = 3,
        heartbit = 4,
        get_sdp_and_play = 5,
        send_data = 6,
        send_regist=7,
        send_stop_regist=8,
        heartbit2 = 9,
        get_sdp_and_play_v1 = 10
    };

    struct msg_header_t
    {
        uint16_t code;             // 指令号
        uint16_t version;          // 版本号
        uint32_t sequence;         // 序号
    };

    struct msg_request_t : msg_header_t
    {};

    struct msg_response_t : msg_header_t
    {
        int16_t error_code;
    };

    /*
    1.  获取头部信息(sdp)
    c->s :
    指令号          2
    版本号          2
    序号            4
    点播通道        2

    s-c:
    指令号          2
    版本号          2
    序号            4

    错误码          2  (0成功， else错误类型)
    头部信息长度    2
    内容            n
    */

    struct get_sdp_request_msg_t : msg_request_t
    {
        uint16_t channel;
    };

    struct get_sdp_response_msg_t : msg_response_t
    {
        uint16_t length;
        uint8_t sdp[0];
    };

    /*
    2.  开启点播
    c->s:
    指令号          2
    版本号          2
    序号            4
    点播通道        2
    传输协议        1    (1标准流， 2混合流)
    复用标志        1
    复用id          4
    RTP接收端口     2
    RTCP接收端口    2 

    s->c:
    指令号          2
    版本号          2
    序号            4
    错误码          2 (0成功， else错误类型)
    传输协议        1    (1标准流， 2混合流)
    复用标志        1
    复用id          4
    RTP接收端口     2
    RTCP接收端口    2
    */
    struct play_request_msg_t : msg_request_t
    {
        uint16_t channel;
        uint8_t proto;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t client_rtp_port;
        uint16_t client_rtcp_port;
    };

    struct play_response_msg_t : msg_response_t
    {
        uint8_t proto;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t server_rtp_port;
        uint16_t server_rtcp_port;
    };

    /*
    3.  停止点播
    c->s:
    指令号          2
    版本号          2
    序号            4
    点播通道        2

    传输协议        1
    复用标志        1
    复用id          4
    RTP接收端口     2
    RTCP接收端口    2

    s->c:
    指令号          2
    版本号          2
    序号            4
    错误码          2(0成功， else错误类型)
    */
    struct stop_request_msg_t : msg_request_t
    {
        uint16_t channel;
        uint8_t proto;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t client_rtp_port;
        uint16_t client_rtcp_port;
    };

    struct stop_response_msg_t : msg_response_t
    {};

    /*
    4.  心跳
    c->s:
    指令号          2
    版本号          2
    序号            4
    通道            2
    */
    struct heartbit_request_msg_t : msg_request_t
    {
        uint16_t channel;
    };

    /*
    5.  快速点播
        c->s:  (与2指令同)
        指令号          2
        版本号          2
        序号            4

        点播通道        2
        传输协议        1    (1标准流， 2混合流)
        复用标志        1
        复用id          4
        RTP接收端口     2
        RTCP接收端口    2

        s->c:
        指令号          2
        版本号          2
        序号            4
        错误码          2 (0成功， else错误类型)
        传输协议        1 (1标准流，2混合流)
        复用标志        1
        复用id          4
        RTP接收端口     2
        RTCP接收端口    2
        头部信息长度    2
        内容            n
     */

    struct get_sdp_and_play_request_msg_t : play_request_msg_t
    {};

    /*
    快速点播，功能同命令5 get_sdp_and_play_request_msg_t，
    在原基础上扩展code码流类型字段 1个字节，支持多码流方案。
    */
    struct get_sdp_and_play_request_msg_v1_t : play_request_msg_t
    {
        uint8_t code;
    };

    struct get_sdp_and_play_response_msg_t : play_response_msg_t
    {
        uint16_t length;
        uint8_t sdp[0];
    };

    /*
    6.	透传数据
    c->s:
    指令号          2
    版本号          2
    序号            4
    点播通道        2
    数据长度        2
    数据            n

    s->c:
    指令号          2
    版本号          2
    序号            4
    错误码          2(0成功， else错误类型)
    长度            2
    数据            n
    */

    struct send_data_request_msg_t : msg_request_t
    {
        uint16_t channel;
        uint16_t length;
        uint8_t content[0];
    };

    struct send_data_response_msg_t : msg_response_t
    {
        uint16_t length;
        uint8_t data[0];
    };


    struct send_regist_request_msg_t:msg_request_t
    {
        uint16_t length;
        uint8_t ids[0];
    };
    struct send_regist_reponse_msg_t:msg_response_t
    {

    };

    /*
    9.  带转发地址的心跳
    c->s:
    指令号          2
    版本号          2
    序号            4
    ip地址          4
    端口            2
    通道            2
    */
    struct heartbit2_request_msg_t : msg_request_t
    {
        uint32_t ip;
        uint16_t port;
        uint16_t channel;
    };
}


#pragma pack(pop)

#endif //_COMMON_MSG_H_INCLUDED
