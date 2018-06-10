/*
*@FileName:common_type.h
*@Func:��������ͷ�ļ�
*@Date��2015-1-12
*@Author:songlei
*/
#ifndef COMMON_TYPE_H_INCLUDE__
#define COMMON_TYPE_H_INCLUDE__
#include <string>
#include <list>
#include <vector>
#include <stdint.h>
#include <stdio.h>
//Ŀǰ֧�ֵ���·����
///////////////////////////////////////////////////////////////////////////////////////////
#define 	LINK_TCP_TCP_PRI 0             // ˽��tcp�Ự�Լ�˽��tcp��
#define 	LINK_TCP_UDP_PRI 1             // ˽��tcp�Ự�Լ�˽��udp��
#define		LINK_TCP_UDP_MULTI 2           // ˽��tcp�Ự�Լ�˽��udp���ಥ
#define		LINK_TCP_RTP_PRI 3             // ˽��tcp�Ự�Լ�˽��rtp�����
#define		LINK_TCP_RTP_MULTI 4           // ˽��tcp�Ự�Լ�˽��rtp������ಥ
#define	 	LINK_TCP_RTP_DEMUX_PRI 5       // ˽��tcp�Ự�Լ�˽��rtp��϶˿ڸ�����
#define  	LINK_RTSP_RTP_STD 6            // ��׼rtsp�Ự�Լ���׼rtp��
#define  	LINK_RTSP_RTP_DEMUX  7         // ��׼rtsp�Ự�Լ�����rtp��
#define  	LINK_RTSP_RTP_PRI 8            // ��׼rtsp�Ự�Լ�˽��rtp��
#define  	LINK_TCP_RTP_STD  9            // ˽��tcp�Ự�Լ���׼rtp��
#define  	LINK_Jinggle_RTP_STD 10         // ˽��XMPP�Ự�Լ���׼rtp��
#define  	LINK_Jinggle_RTP_PRI  11        // ˽��XMPP�Ự�Լ�˽��rtp��
#define  	LINK_Jinggle_RTP_DEMUX_PRI 12    // ˽��XMPP�Ự�Լ�˽��rtp��϶˿ڸ�����
#define		LINK_UDP_RTP_PRI   13        // ˽��udp�Ự�Լ�˽��rtp�����
///////////////////////////////////////////////////////////////////////////////////////////

//�豸���Ͷ���
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

//sip�Խ���ض���
#define USE_SIP_JOIN
#ifdef USE_SIP_JOIN
//sip�Խ�����ṹ
typedef struct _struct_sip_play_msg_
{
    std::string		sender_ids;			// ������ids
    long			cmdid;				// 920001-����IDΨһ
	std::string		guid;				// �������(����Я��)
    int				type;				// 0-���ն˿�(��˾ϵͳ-�ⲿ)��1-���Ͷ˿�(�ⲿϵͳ-��˾ϵͳ)��2-����/���ն˿�(����-˫��)
    long			sessionid;			// �ỰID
    std::string		recvids;			// �ⲿ�豸ids������ջ��ƶ�Ӧ
    int				recvmdiaChannel;	// �ⲿ�豸�㲥ͨ��������ջ��ƶ�Ӧ,Ĭ��Ϊ0
    std::string		sendids;			// ��˾ϵͳ�豸/�û�ids���뷢�ͻ��ƶ�Ӧ
    int				sendmediaChannel;	// ��˾ϵͳ�豸/�û��㲥ͨ��,�뷢�ͻ��ƶ�Ӧ,Ĭ��0
    int				streamtype;			// 0/1/2--������/������/����Ƶ
    int				transmitchannel;	//ת������ͨ��
}sip_play_msg,*psip_play_msg;

//�Ự����
#define MSG_OPEN_RECV 0				//���ն˿�(��˾ϵͳ-�ⲿ)
#define MSG_OPEN_SEND 1				//���Ͷ˿�(�ⲿϵͳ-��˾ϵͳ)
#define MSG_OPEN_CALL 2				//����/���ն˿�(����-˫��)
#define MSG_SIP_2_SIP_PLAY 3		//sip-sip ֪ͨ������������ ��ʱ���ĸ�֪ת��ͨ�� �׳�sdp
#define MSG_SIP_2_SIP_OPEN_CALL 4	//sip-sip��֪ͨ������������/����

//��������
#define STREAM_MAIN  0				//������
#define STREAM_SUB   1				//������
#define STREAM_AUDIO 2				//��Ƶ��

//sdp����ṹ
typedef struct _struct_sip_sdp_msg
{	
    long cmdid;
    long sdp_len;
    std::string sender_ids;
    long sessionid;	
    std::string sdp;
}sip_sdp_msg,psip_sdp_msg;

//sip�Խ�����id����
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

//�����
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
//�豸������豸�������
typedef long dev_handle_t;

//rtp id ��������
typedef uint32_t rtp_id_t;
#define RTP_ID_VALUE_NA -1
#define INT_VALUE_NA   -1

//sdp����
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

//���������� 10000 - 99999
#define OPEN_REVC_M_FAIL 10000 //�򿪽��ջ���ʧ��
#define OPEN_SEND_M_FAIL 10001 //�򿪷��ͻ���ʧ��
#define SAVE_SDP_TO_ACEESS_FAIL 10002 //����sdp�������ʧ��
#define SAVE_SDP_TO_SRV_FAIL 10003 //����sdp��ת����ʧ��
#define ADD_SEND_STREAM_FAIL 10004 //��Զ���ƹ�ý����ʧ��
#define PARSE_SDP_FAIL 10005 //����sdp����
#define SEINIDS_IS_EMPTY 10006 //sendidsΪ��
#define RECVIDS_IS_EMPTY 10007 //recvidsΪ��
#define TRANS_CH_IS_USED 10008 //ת������ͨ����ռ��
#define CPTURE_DATA_FAIL 10009 //������������ʧ��
#define RECV_ILLEGAL_CMD 10010 //�յ��Ƿ�ָ�� 
#define PRO_SDP_FAIL     10011 //����sdpʧ��

#endif//COMMON_TYPE_H_INCLUDE__
