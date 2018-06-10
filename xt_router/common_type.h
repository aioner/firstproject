/*
*@FileName:common_type.h
*@Func:公共类型头文件
*@Date：2015-1-12
*@Author:songlei
*/
#ifndef COMMON_TYPE_H_INCLUDE__
#define COMMON_TYPE_H_INCLUDE__
#include <string>
#include <list>
#include <vector>
#include <stdint.h>
#include <stdio.h>
//目前支持的链路类型
///////////////////////////////////////////////////////////////////////////////////////////
#define 	LINK_TCP_TCP_PRI 0             // 私有tcp会话以及私有tcp流
#define 	LINK_TCP_UDP_PRI 1             // 私有tcp会话以及私有udp流
#define		LINK_TCP_UDP_MULTI 2           // 私有tcp会话以及私有udp流多播
#define		LINK_TCP_RTP_PRI 3             // 私有tcp会话以及私有rtp混合流
#define		LINK_TCP_RTP_MULTI 4           // 私有tcp会话以及私有rtp混合流多播
#define	 	LINK_TCP_RTP_DEMUX_PRI 5       // 私有tcp会话以及私有rtp混合端口复用流
#define  	LINK_RTSP_RTP_STD 6            // 标准rtsp会话以及标准rtp流
#define  	LINK_RTSP_RTP_DEMUX  7         // 标准rtsp会话以及复用rtp流
#define  	LINK_RTSP_RTP_PRI 8            // 标准rtsp会话以及私有rtp流
#define  	LINK_TCP_RTP_STD  9            // 私有tcp会话以及标准rtp流
#define  	LINK_Jinggle_RTP_STD 10         // 私有XMPP会话以及标准rtp流
#define  	LINK_Jinggle_RTP_PRI  11        // 私有XMPP会话以及私有rtp流
#define  	LINK_Jinggle_RTP_DEMUX_PRI 12    // 私有XMPP会话以及私有rtp混合端口复用流
#define		LINK_UDP_RTP_PRI   13        // 私有udp会话以及私有rtp混合流
///////////////////////////////////////////////////////////////////////////////////////////

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

//sip对接相关定义
#define USE_SIP_JOIN
#ifdef USE_SIP_JOIN
//sip对接信令结构
typedef struct _struct_sip_play_msg_
{
    std::string		sender_ids;			// 发送者ids
    long			cmdid;				// 920001-命令ID唯一
	std::string		guid;				// 命令序号(反馈携带)
    int				type;				// 0-接收端口(公司系统-外部)，1-发送端口(外部系统-公司系统)，2-发送/接收端口(呼叫-双向)
    long			sessionid;			// 会话ID
    std::string		recvids;			// 外部设备ids，与接收机制对应
    int				recvmdiaChannel;	// 外部设备点播通道，与接收机制对应,默认为0
    std::string		sendids;			// 公司系统设备/用户ids，与发送机制对应
    int				sendmediaChannel;	// 公司系统设备/用户点播通道,与发送机制对应,默认0
    int				streamtype;			// 0/1/2--主码流/子码流/音视频
    int				transmitchannel;	//转发服务通道
}sip_play_msg,*psip_play_msg;

//会话属性
#define MSG_OPEN_RECV 0				//接收端口(公司系统-外部)
#define MSG_OPEN_SEND 1				//发送端口(外部系统-公司系统)
#define MSG_OPEN_CALL 2				//发送/接收端口(呼叫-双向)
#define MSG_SIP_2_SIP_PLAY 3		//sip-sip 通知交换建立发送 此时中心告知转发通道 抛出sdp
#define MSG_SIP_2_SIP_OPEN_CALL 4	//sip-sip，通知交换建立发送/接收

//码流类型
#define STREAM_MAIN  0				//主码流
#define STREAM_SUB   1				//子码流
#define STREAM_AUDIO 2				//音频流

//sdp信令结构
typedef struct _struct_sip_sdp_msg
{	
    long cmdid;
    long sdp_len;
    std::string sender_ids;
    long sessionid;	
    std::string sdp;
}sip_sdp_msg,psip_sdp_msg;

//sip对接信令id定义
#define GW_JOIN_SIP_PLAY_CMD			920001
#define GW_JOIN_SIP_ROUTER_SDP_CMD		920002
#define GW_JOIN_SIP_SDP_CMD				920003
#define GW_JOIN_SIP_CLOSE_RECV_CMD		920004
#define GW_JOIN_SIP_CLOSE_SEND_CMD		920005
#define GW_JOIN_SIP_ADD_SEND_CMD		920006
#define GW_JOIN_SIP_CLEAR_SESSION_CMD	920007
#define GW_JOIN_SIP_OPERATE_FAIL		920008
#define GW_JOIN_SIP_OPERATE_SUCCESS		920009
#define GW_JOIN_SIP_CLEAR_TRANS_CH		920010

//流相关
#define MAX_TRACK_NUM					9
#define MAX_SDP_LEN						4096

#endif    //#ifdef USE_SIP_JOIN

#define ERR_OUT std::cerr<<"ERROR: "
#define INFO_OUT std::cerr<<"INFO: "

#include <boost/lexical_cast.hpp>
template <typename num_type>
num_type str_to_num(const char* instr)
{
    if ((NULL == instr) || (0 == instr[0]))
    {
        return -1;
    }

    num_type num = 0;
    try
    {
        num = boost::lexical_cast<num_type>(instr);
    }
    catch (...)
    {
        num = -1;
    }

    return num;
}

template <typename num_type>
inline std::string num_to_str(num_type innum)
{
    std::ostringstream os;
    os<<innum;
    return os.str();
}

#include <boost/date_time/posix_time/posix_time.hpp>
//%04d-%02d-%02d %02d:%02d:%02d:%d
inline std::string get_cur_time_microsec()
{
    boost::posix_time::ptime t = boost::posix_time::microsec_clock::local_time();
    std::string strtime = to_iso_extended_string(t);
    std::string::size_type uiPos = strtime.find("T");
    if (uiPos != std::string::npos)
    {
        strtime.replace(uiPos,1,std::string(" "));
    }
    uiPos = strtime.find(".");
    if (uiPos != std::string::npos)
    {
        strtime.replace(uiPos,1,std::string(":"));
    }
    return strtime;
}

#define USE_STD_SIP_SRV_SC__
#ifdef USE_STD_SIP_SRV_SC__
//设备接入的设备操作句柄
typedef long dev_handle_t;

//rtp id 类型声明
typedef uint32_t rtp_id_t;
#define RTP_ID_VALUE_NA -1
#define INT_VALUE_NA   -1

//sdp方向
typedef enum 
{
    dir_na=-1,
    dir_sendonly=0,
    dir_recvonly=1,
    dir_sendrecv=2,
}sdp_direction_t;

#endif //#ifdef USE_STD_SIP_SRV_SC__

#define recv_center_cmd_pro_thread 1
#define response_center_cmd_pro_thread 2
#define arbitrarily_free_work_thread 0
#define center_cmd_xml_parse_pro_thread 3

typedef struct _struct_send_rtp_stream_dst_info_type_
{
    std::string dst_ip;
    int trackid;
    unsigned short dst_port;
    bool dst_demux;
    unsigned int dst_demuxid;
    typedef _struct_send_rtp_stream_dst_info_type_ my_t;
    my_t& operator=(const my_t& rf)
    {
        this->dst_ip = rf.dst_ip;
        this->trackid = rf.trackid;
        this->dst_port = rf.dst_port;
        this->dst_demux = rf.dst_demux;
        this->dst_demuxid = rf.dst_demuxid;
        return *this;
    }
}rtp_dst_info_t,*ptr_rtp_dst_info_t;

//交换错误码 10000 - 99999
#define OPEN_REVC_M_FAIL 10000 //打开接收机制失败
#define OPEN_SEND_M_FAIL 10001 //打开发送机制失败
#define SAVE_SDP_TO_ACEESS_FAIL 10002 //保存sdp到接入库失败
#define SAVE_SDP_TO_SRV_FAIL 10003 //保存sdp到转发库失败
#define ADD_SEND_STREAM_FAIL 10004 //向远端推关媒体流失败
#define PARSE_SDP_FAIL 10005 //解析sdp出错
#define SEINIDS_IS_EMPTY 10006 //sendids为空
#define RECVIDS_IS_EMPTY 10007 //recvids为空
#define TRANS_CH_IS_USED 10008 //转发服务通道被占用
#define CPTURE_DATA_FAIL 10009 //交换捕获数据失败
#define RECV_ILLEGAL_CMD 10010 //收到非法指令 
#define PRO_SDP_FAIL     10011 //处理sdp失败

#endif//COMMON_TYPE_H_INCLUDE__
