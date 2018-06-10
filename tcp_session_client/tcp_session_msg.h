#ifndef _TCP_SESSION_MSG_H_INCLUDED
#define _TCP_SESSION_MSG_H_INCLUDED

#include<stdint.h>

//tcp协议的操作码列表
#define TCP_SESSION_OPID_LOGOUT                         0
#define TCP_SESSION_OPID_LOGIN                          1
#define TCP_SESSION_OPID_PLAY                           2
#define TCP_SESSION_OPID_STOP                           3
#define TCP_SESSION_OPID_MULTICAST_INFO                 4
#define TCP_SESSION_OPID_KEY_INFO                       5
#define TCP_SESSION_OPID_PORT_INFO                      6       //端口信息(接收端使用的端口信息从发送端获取)
#define TCP_SESSION_OPID_CLEAR                          7       //清除与此握手相关的所有操作

#define TCP_SESSION_OPID_EXT_FLAG                       16      //扩展操作（用来扩展操作指令）
#define TCP_SESSION_OPID_EXT2_FLAG                      17      //扩展操作（RTP端口复用）
#define TCP_SESSION_OPID_KEY_INFO2                      18

#define TCP_SESSION_OPID_CTRL_FLAG_SL                   21
#define TCP_SESSION_OPID_CTRL_PLAY_SL                   30
#define TCP_SESSION_OPID_CTRL_STOP_SL                   32


#define TCP_SESSION_MULTICAST_ADDR_BYTES                40      //组播地址的最大长度（参考了IPV6地址字符串结构占用的最大值）
#define TCP_SESSION_KEY_LEN                             2048

#pragma pack(push, 8)

namespace tcp_session
{
    struct msg_header_t
    {
        uint16_t flag;           //标识值
        uint16_t op_id;          //操作标识

        uint32_t data_size;      //数据长度(有效载荷+填充数据(不包含头长度))
        uint32_t fill_size;      //填充数据长度
        uint32_t data_type;      //数据类型

        uint32_t length() const
        {
            return data_size - fill_size;
        }
    };

    struct login_msg_t
    {
        uint16_t op_id;
        uint16_t rtp_port;          //接收数据用端口
        uint16_t channel;           //要接收的数据通道号
        uint16_t mode;              //接收数据模式(0,1: 单播，2：组播)  //在RTP的点播操作中此参数表示RctpSsrc
    };

    struct multicast_info_t
    {
        uint16_t port;
        char     addr[TCP_SESSION_MULTICAST_ADDR_BYTES];
        uint16_t rtp_start_port;                     //服务端rtp发送的起始端口
    };

    struct play_msg_t
    {
        uint16_t op_id;
        uint16_t op_id_ext;
        uint16_t audio_rtp_port;
        uint16_t rtp_port;          //接收数据用端口
        uint16_t channel;           //要接收的数据通道号
        uint16_t mode;              //接收数据模式(0,1: 单播，2：组播)  //在RTP的点播操作中此参数表示RctpSsrc
        uint32_t ssrc;
    };

    struct demux_play_msg_t : play_msg_t
    {
        uint32_t multiplex;
        uint32_t multiplexID;
    };

    struct sdp_info_t
    {
        uint32_t length;
        char sdp[TCP_SESSION_KEY_LEN];

        uint32_t multiplex;
        uint32_t multiplexID;

        uint32_t data_type;

        uint16_t stop_flag;
        uint16_t rtp_port;          //非复用方式下的服务端的rtp发送端口
    };

    typedef login_msg_t stop_msg_t;

    typedef demux_play_msg_t demux_stop_msg_t;

    typedef login_msg_t stop_response_msg_t;

    struct ctrl_msg_t
    {
        uint16_t op_id;
        uint16_t op_id_ext;
        double npt;
        float scale;
        uint16_t rtp_port;          //接收数据用端口
        uint16_t channel;           //要接收的数据通道号
        uint16_t mode;              //接收数据模式(0,1: 单播，2：组播)  //在RTP的点播操作中此参数表示RctpSsrc
    };
}

#pragma pack(pop)

#endif //_TCP_SESSION_MSG_H_INCLUDED

