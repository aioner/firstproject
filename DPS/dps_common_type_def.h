//
//create by songlei 20160316
//
#ifndef DPS_COMMON_TYPE_DEF_H__
#define DPS_COMMON_TYPE_DEF_H__
#include <stdint.h>

#ifndef _WIN32
#ifndef __stdcall
#define __stdcall
#endif//#ifndef __stdcall
#endif // end #ifdef WIN32

#define DPS_MAX_IP_LEN 32
#define DPS_MAX_USR_NAME 512
#define DPS_MAX_PASSWORD 512
#define DPS_MAX_SDP_LEN 4096
#define DPS_MAX_FRAME_SIZE   1024*1024
#define DPS_MAX_TRACK_NAME_LEN 64

typedef enum 
{
    MEDIA_TYPE_NA = -1,
    MEDIA_TYPE_AUDIO = 1, //音频
    MEDIA_TYPE_VIDEO = 0, //视频
}media_type_t;

//目前支持的链路类型
typedef enum
{
    LT_NA=-1,
    LT_TCP_TCP_PRI = 0,          //私有tcp会话以及私有tcp流
    LT_TCP_UDP_PRI = 1,          //私有tcp会话以及私有udp流
    LT_TCP_UDP_MULTI = 2,        //私有tcp会话以及私有udp流多播
    LT_TCP_RTP_PRI = 3,          //私有tcp会话以及私有rtp混合流
    LT_TCP_RTP_MULTI = 4,        //私有tcp会话以及私有rtp混合流多播
    LT_TCP_RTP_DEMUX_PRI = 5,    //私有tcp会话以及私有rtp混合端口复用流
    LT_RTSP_RTP_STD = 6,         //标准rtsp会话以及标准rtp流
    LT_RTSP_RTP_DEMUX = 7,       //标准rtsp会话以及复用rtp流
    LT_RTSP_RTP_PRI = 8,         //标准rtsp会话以及私有rtp流
    LT_TCP_RTP_STD = 9,          //私有tcp会话以及标准rtp流
    LT_XMPP_RTP_STD =10,         //私有XMPP会话以及标准rtp流
    LT_XMPP_RTP_PRI = 11,        //私有XMPP会话以及私有rtp流
    LT_XMPP_RTP_DEMUX_PRI =12,   //私有XMPP会话以及私有rtp混合端口复用流
    LT_UDP_RTP_PRI  = 13,        //私有udp会话以及私有rtp混合流
    LT_UDP_RTP_DEMUX_PRI = 14,    //私有UDP会话 私有RTP流
    LT_UDP_RTP_MULTIPRI  = 15,    //私有有UDP协商 RTP多播
    LT_UDP_RTP_STD = 16,          //私有有UDP协商 标准RTP流
    LT_UDP_RTP_DEMUX_STD = 17,     //私有有UDP协商 标准RTP 复用流
    LT_UDP_RTP_MULTI_STD = 18,     //私有有UDP协商 标准RTP流 多播
}dps_link_type_t;

template<typename T>
dps_link_type_t link_type_cast(T link_type)
{
    dps_link_type_t cast_ret = LT_NA;
    switch(link_type)
    {
    case 0:
        {
            cast_ret = LT_TCP_TCP_PRI;
            break;
        }
    case 1:
        {
            cast_ret = LT_TCP_UDP_PRI;
            break;
        }
    case 2:
        {
            cast_ret = LT_TCP_UDP_MULTI;
            break;
        }
    case 3:
        {
            cast_ret = LT_TCP_RTP_PRI;
            break;
        }
    case 4:
        {
            cast_ret = LT_TCP_RTP_MULTI;
            break;
        }
    case 5:
        {
            cast_ret = LT_TCP_RTP_DEMUX_PRI;
            break;
        }
    case 6:
        {
            cast_ret = LT_RTSP_RTP_STD;
            break;
        }
    case 7:
        {
            cast_ret = LT_RTSP_RTP_DEMUX;
            break;
        }
    case 8:
        {
            cast_ret = LT_RTSP_RTP_PRI;
            break;
        }
    case 9:
        {
            cast_ret = LT_TCP_RTP_STD;
            break;
        }
    case 10:
        {
            cast_ret = LT_XMPP_RTP_STD;
            break;
        }
    case 11:
        {
            cast_ret = LT_XMPP_RTP_PRI;
            break;
        }
    case 12:
        {
            cast_ret = LT_XMPP_RTP_DEMUX_PRI;
            break;
        }
    case 13:
        {
            cast_ret = LT_UDP_RTP_PRI;
            break;
        }
    case 14:
        {
            cast_ret = LT_UDP_RTP_DEMUX_PRI;
            break;
        }
    case 15:
        {
            cast_ret = LT_UDP_RTP_MULTIPRI;
            break;
        }
    case 16:
        {
            cast_ret = LT_UDP_RTP_STD;
            break;
        }
    case 17:
        {
            cast_ret = LT_UDP_RTP_DEMUX_STD;
            break;
        }
    case 18:
        {
            cast_ret = LT_UDP_RTP_MULTI_STD;
            break;
        }
    default:
        {
            break;
        }
    }
    return cast_ret;
}

typedef int srcno_t;
#define SRCNO_NA -1

typedef int track_id_t;

typedef uint32_t dps_ch_t;
#define DPS_CH_NA 65535/*_UI32_MAX*/

typedef struct __struct_device_type
{
    __struct_device_type(const char* in_ip,const uint16_t in_port,const long in_dev_ch,const long in_dev_type,
        const long in_stream_type,const dps_link_type_t in_link_type,const char* in_usr,const char* in_password)
        :port(in_port),dev_ch(in_dev_ch),dev_type(in_dev_type),stream_type(in_stream_type),link_type(in_link_type),transmit_ch(DPS_CH_NA)
    {
        std::strncpy(ip,in_ip,DPS_MAX_IP_LEN);
        std::strncpy(usr,in_usr,DPS_MAX_USR_NAME);
        std::strncpy(password,in_password,DPS_MAX_PASSWORD);
    }
    __struct_device_type()
        :port(0),dev_ch(-1),dev_type(-1),stream_type(-1),link_type(LT_TCP_RTP_PRI),transmit_ch(DPS_CH_NA)
    {
        std::memset(ip,0,DPS_MAX_IP_LEN);
        std::memset(usr,0,DPS_MAX_USR_NAME);
        std::memset(password,0,DPS_MAX_PASSWORD);
    }

    __struct_device_type(const __struct_device_type& rf)
    {
        *this = rf;
    }

    __struct_device_type& operator=(const __struct_device_type& rf)
    {
        this->dev_ch = rf.dev_ch;
        this->port = rf.port;
        this->dev_type = rf.dev_type;
        this->stream_type = rf.stream_type;
        this->link_type = rf.link_type;
        this->transmit_ch = rf.transmit_ch;
        std::strncpy(this->ip,rf.ip,DPS_MAX_IP_LEN);
        std::strncpy(this->usr,rf.usr,DPS_MAX_USR_NAME);
        std::strncpy(this->password,rf.password,DPS_MAX_PASSWORD);
        return *this;
    }

    bool operator==(const __struct_device_type& rf) const
    {
        return (this->dev_type == rf.dev_type && this->dev_ch == rf.dev_ch && this->port == rf.port && 0 == std::strcmp(this->ip,rf.ip)
            && this->stream_type == rf.stream_type && 0 == std::strcmp(this->usr,rf.usr) && 0 == std::strcmp(this->password,rf.password)
            && this->link_type == rf.link_type && this->transmit_ch == rf.transmit_ch);
    }

    char ip[DPS_MAX_IP_LEN];
    char usr[DPS_MAX_USR_NAME];
    char password[DPS_MAX_PASSWORD];
    uint16_t port;
    long dev_type;
    long stream_type;
    long dev_ch;
    dps_link_type_t link_type;
    dps_ch_t transmit_ch;
}device_t,*ptr_device_t;

#define recv_center_cmd_pro_thread 1
#define response_center_cmd_pro_thread 2
#define center_cmd_xml_parse_pro_thread 3
#define arbitrarily_free_work_thread 0
#define dev_access_operator_thread 4

//设备类型定义
////////////////////////////////////////////////////////////////////////////////////////////
#define DEV_ROUTER         9
#define DEV_STREAM         170
#define DEV_USB            174
#define DEV_HC             2
#define DEV_DH             5
#define DEV_ONVIF          227
#define DEV_MOBELE_PHONE    313
#define DEV_XTVGA          172
#define DEV_SIP             1
#define CODEC_MAIN          0
#define CODEC_SUB           1
#define CODEC_VIDEO         2
////////////////////////////////////////////////////////////////////////////////////////////

#endif // end #ifndef DPS_COMMON_TYPE_DEF_H__
