/*@
 *@FileNmae:  xmpp_type_def.h
 *@Describe:  XMPP公共类型定义文件
 *@Author:    songlei
 *@Date:      2014-12-24 15:27
**/
#ifndef XMPP_TYPE_DEF_H__
#define XMPP_TYPE_DEF_H__
#include <stdio.h>
#include "common_type.h"
#define STR_NULL ""

#define PLAY_REQUEST        "playRequest"    //点播请求
#define PLAY_REPLAY         "playReplay"     //点播反馈
#define PLAY_STOP_REQUEST   "playStopRequest" //停点请求
#define PLAY_STOP_REPLY     "playStopReply"  //停点反馈
#define PLAY_INFORM_REQUST  "playInformRequest" //TCP反向注册 
#define PLAY_INFORM_REPLAY    "playInformReplay"
#define	PLAY_STOP_INFORM_REQUEST  "playStopInformRequest"
#define PLAY_STOP_INFORM_REPLAY    "playStopInformReplay"

#define TYPE_RESULT "result"  //反馈
#define TYPE_ERROR   "error"  


#define JINGGLE_RTP       "Jinggle_RTP"                // Jingle媒体会话，标准RTP传输
#define JINGGLE_XTRTP     "Jinggle_XTRTP"              // jingle媒体会话，XT私有RTP传输
#define RTSP_RTP          "RTSP_RTP"                  // RTSP媒体会话，标准RTP传输
#define RSTP_XTRTP        "RTSP_XTRTP"                // RTSP媒体会话，XT私有RTP传输
#define TCP_TCP           "TCP_TCP"                  // TCP媒体会话，TCP传输
#define NAT_TCP_RTP       "NAT_TCP_RTP"              // 带NAT穿透的，TCP媒体会话，RTP传输
#define NAT_UDP_RTP       "NAT_UDP_RTP"             // 带NAT穿透的，UDP媒体会话，RTP传输
#define TCP_XTRTP         "TCP_XTRTP"               // TCP媒体会话，XT私有TCP传输
#define UDP_XTRTP         "UDP_XTRTP"               // UDP媒体会话，XT私有RTP传输
#define UDP_RTP           "UDP_RTP"                // UDP媒体会话，标准RTP传输

//0:tcp+tcp/1:xtmsg+rtp/2:xtmsg+rtp mul/3:xtmsg+rtp/4:xtmsg+rtp mul/5:xtmsg+rtp demux/6:rtsp+rtp std/7:rtsp+rtp/8:rtsp+rtp demux

#ifndef DATA_NA
#define DATA_NA -1 //无效数据
#endif//DATA_NA

//play 节点
struct play_type
{
	std::string action_;       // 操作类型
	std::string sessionType_;  // 转发出的交互协议,第一次点播请求发来的时候，CCS记录的
	std::string sid_;         // 唯一标识会话B
};

//query节点
template<typename bodytype>
struct query_type 
{
	std::string  xmlns_;	
	bodytype    playbody_;
};

//iq节点
template<typename bodytype>
struct iq_type
{
	std::string id_;//自动生成，标识消息唯一性
	std::string from_;
	std::string to_;
	std::string type_;
	query_type<bodytype> query_;
};

template<typename session_arg_type>
struct transmitInfo_type
{
	std::string     sessionType_;
	session_arg_type arg;
};

//original节点
struct original_type
{
	std::string requestor_;      // 原始的点播发起者
	std::string	responder_;      // 原始的被点播者一直是源保持不变
	long	   mediaSrcChannel_; // 媒体源的通道
	long	   streamType_;     // 主码流0或子码流1或纯音频流2

};

//点播请求的
struct play_session_arg_type
{
	std::string token_; //被点播者的JID
	std::string	loginName_; //"登录用户名
	std::string	loginPassword_; //"登录密码	
	std::string playip_; //点播源IP

	long	transmitChannel_; //指定的本地交换转发通道
	long    playChannel_; //点播源的通道
	long	playPort_;  //点播源的端口，会话端口
	long	loginport_; //"登录端口

	int     playType_; //设备类型

	bool	reuseFlag_s_; //发送复用标识
	bool	reuseFlag_r_; //接收复用标识
};
struct play_requst_body
{
	typedef transmitInfo_type<
		play_session_arg_type> arg_type;

	play_type     play_;
	arg_type      transmitInfo_;
	original_type  original_;
};

//停点请求
struct stop_session_arg_type
{
	std::string token_;            // 被点播者的JID
	long	    transmitChannel_;   // 指定的本地交换转发通道
};
struct stop_requst_body
{
	typedef transmitInfo_type<
		stop_session_arg_type> arg_type;	
	play_type     play_;
	arg_type      transmitInfo_;
};

//TCP反向注册
struct play_inform_request_session_arg_type 
{
	std::string routerip_;
	unsigned short routerport_;	
};
struct play_inform_request_body
{
	typedef transmitInfo_type<
		play_inform_request_session_arg_type> arg_type;
	play_type     play_;
	arg_type      transmitInfo_;

};

struct stop_inform_request_session_arg_type 
{
	std::string routerip_;
	unsigned short routerport_;	
};
struct stop_inform_request_body
{
	typedef transmitInfo_type<
		stop_inform_request_session_arg_type> arg_type;
	play_type     play_;
	arg_type      transmitInfo_;

};
#endif //XMPP_TYPE_DEF_H__