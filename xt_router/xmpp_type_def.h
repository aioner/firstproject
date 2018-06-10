/*@
 *@FileNmae:  xmpp_type_def.h
 *@Describe:  XMPP�������Ͷ����ļ�
 *@Author:    songlei
 *@Date:      2014-12-24 15:27
**/
#ifndef XMPP_TYPE_DEF_H__
#define XMPP_TYPE_DEF_H__
#include <stdio.h>
#include "common_type.h"
#define STR_NULL ""

#define PLAY_REQUEST        "playRequest"    //�㲥����
#define PLAY_REPLAY         "playReplay"     //�㲥����
#define PLAY_STOP_REQUEST   "playStopRequest" //ͣ������
#define PLAY_STOP_REPLY     "playStopReply"  //ͣ�㷴��
#define PLAY_INFORM_REQUST  "playInformRequest" //TCP����ע�� 
#define PLAY_INFORM_REPLAY    "playInformReplay"
#define	PLAY_STOP_INFORM_REQUEST  "playStopInformRequest"
#define PLAY_STOP_INFORM_REPLAY    "playStopInformReplay"

#define TYPE_RESULT "result"  //����
#define TYPE_ERROR   "error"  


#define JINGGLE_RTP       "Jinggle_RTP"                // Jingleý��Ự����׼RTP����
#define JINGGLE_XTRTP     "Jinggle_XTRTP"              // jingleý��Ự��XT˽��RTP����
#define RTSP_RTP          "RTSP_RTP"                  // RTSPý��Ự����׼RTP����
#define RSTP_XTRTP        "RTSP_XTRTP"                // RTSPý��Ự��XT˽��RTP����
#define TCP_TCP           "TCP_TCP"                  // TCPý��Ự��TCP����
#define NAT_TCP_RTP       "NAT_TCP_RTP"              // ��NAT��͸�ģ�TCPý��Ự��RTP����
#define NAT_UDP_RTP       "NAT_UDP_RTP"             // ��NAT��͸�ģ�UDPý��Ự��RTP����
#define TCP_XTRTP         "TCP_XTRTP"               // TCPý��Ự��XT˽��TCP����
#define UDP_XTRTP         "UDP_XTRTP"               // UDPý��Ự��XT˽��RTP����
#define UDP_RTP           "UDP_RTP"                // UDPý��Ự����׼RTP����

//0:tcp+tcp/1:xtmsg+rtp/2:xtmsg+rtp mul/3:xtmsg+rtp/4:xtmsg+rtp mul/5:xtmsg+rtp demux/6:rtsp+rtp std/7:rtsp+rtp/8:rtsp+rtp demux

#ifndef DATA_NA
#define DATA_NA -1 //��Ч����
#endif//DATA_NA

//play �ڵ�
struct play_type
{
	std::string action_;       // ��������
	std::string sessionType_;  // ת�����Ľ���Э��,��һ�ε㲥��������ʱ��CCS��¼��
	std::string sid_;         // Ψһ��ʶ�ỰB
};

//query�ڵ�
template<typename bodytype>
struct query_type 
{
	std::string  xmlns_;	
	bodytype    playbody_;
};

//iq�ڵ�
template<typename bodytype>
struct iq_type
{
	std::string id_;//�Զ����ɣ���ʶ��ϢΨһ��
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

//original�ڵ�
struct original_type
{
	std::string requestor_;      // ԭʼ�ĵ㲥������
	std::string	responder_;      // ԭʼ�ı��㲥��һֱ��Դ���ֲ���
	long	   mediaSrcChannel_; // ý��Դ��ͨ��
	long	   streamType_;     // ������0��������1����Ƶ��2

};

//�㲥�����
struct play_session_arg_type
{
	std::string token_; //���㲥�ߵ�JID
	std::string	loginName_; //"��¼�û���
	std::string	loginPassword_; //"��¼����	
	std::string playip_; //�㲥ԴIP

	long	transmitChannel_; //ָ���ı��ؽ���ת��ͨ��
	long    playChannel_; //�㲥Դ��ͨ��
	long	playPort_;  //�㲥Դ�Ķ˿ڣ��Ự�˿�
	long	loginport_; //"��¼�˿�

	int     playType_; //�豸����

	bool	reuseFlag_s_; //���͸��ñ�ʶ
	bool	reuseFlag_r_; //���ո��ñ�ʶ
};
struct play_requst_body
{
	typedef transmitInfo_type<
		play_session_arg_type> arg_type;

	play_type     play_;
	arg_type      transmitInfo_;
	original_type  original_;
};

//ͣ������
struct stop_session_arg_type
{
	std::string token_;            // ���㲥�ߵ�JID
	long	    transmitChannel_;   // ָ���ı��ؽ���ת��ͨ��
};
struct stop_requst_body
{
	typedef transmitInfo_type<
		stop_session_arg_type> arg_type;	
	play_type     play_;
	arg_type      transmitInfo_;
};

//TCP����ע��
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