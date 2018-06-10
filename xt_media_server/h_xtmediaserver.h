#ifndef _H_XTMEDIASERVER_H
#define _H_XTMEDIASERVER_H

#if (defined(WIN32) || defined(_WIN32) ||defined(WIN64) || defined(_WIN64)) && !defined(_OS_WINDOWS)
#define _OS_WINDOWS
#endif

#ifdef _OS_WINDOWS
#ifdef XTMEDIASERVER_EXPORTS
#define MEDIASERVER_API __declspec(dllexport)
#else
#define MEDIASERVER_API __declspec(dllimport)
#endif

#define MEDIASERVER_STDCALL __stdcall
#else
#define MEDIASERVER_API __attribute__((visibility("default")))
#define MEDIASERVER_STDCALL 
#endif

#include<stdint.h>
#include "xt_log_def.h"
#include "stddef.h"

#ifndef MEDIA_SERVER_IP_LEN
#define MEDIA_SERVER_IP_LEN 32
#endif//

#define MAX_TRACK 9

#ifndef MAX_STR_SIZE
#define  MAX_STR_SIZE 512
#endif//MAX_STR_SIZE

//������Ϣ���
typedef struct _struct_connect_info_
{
    char m_pszCreateTime[MAX_STR_SIZE];//����ʱ��
    char m_pszSrcIDS[MAX_STR_SIZE];    //ԴIDS
    char m_pszSrcIP[MAX_STR_SIZE];     //ԴIP
    char m_pszDestIP[MAX_STR_SIZE];    //Ŀ��IP 
    long m_lChID;                     //����ͨ��
    long m_lDestPort;                 //Ŀ����ն˿�
    long m_lSendPort;                  //���Ͷ˿�
    unsigned short m_usProtocol;    //Э��  
    unsigned int   m_uiSsrc;            //SSRC
    int srcno;
    //RR_Info
    /////////////////////////////////////
    unsigned int    m_uiFractionLost;     //������
    unsigned int    m_uiCumulativeLost;   //�ۼƶ���
    unsigned int    m_uiSequenceNumber;   //Sequence number that was received from the source specified by SSRC.
    unsigned int    m_uiJitter;           //���綶�� 
    unsigned int    m_uiLSR;       //The middle 32 bits of the NTP timestamp received. 
    unsigned int    m_uiDlSR;      //Delay since the last SR.
	unsigned int    m_urtt;        //round trip time
    /////////////////////////////////// 
    //SR_Info
    ///////////////////////////////////
    unsigned int    m_uiMNTPtimestamp;  //Most significant 32bit of NTP timestamp 
    unsigned int    m_uiLNTPtimestamp;  //Least significant 32bit of NTP timestamp
    unsigned int    m_uiTimestamp;  //RTP timestamp
    unsigned int    m_uiPackets;         //�ۼƷ����� 
    unsigned int    m_uiOctets;          //�ۼƷ����ֽ���
    ////////////////////////////////
    bool   m_bSendMultiplex;       // ���͸��ñ�־
    bool           m_bDestMultiplex;       //Զ�˽��ո��ñ�־
    unsigned int m_uiSendMultid;         // ���Ͷ˸���id
    unsigned int    m_uiDestMultid;         //Զ�˽��ո���id 
}connect_info_t,*pconnect_info_t;

/**
*@ function: RTCPý���ǿ��I֡����
*@ param[int] const int srcno ת��Դ��ʶ
**/
typedef void (MEDIASERVER_STDCALL * xt_rtcp_force_iframe_cb_t)(const int srcno);


//�������ƻص���������
////////////////////////////////////////////////////////////////////////////////////
/**
*@function:rtsp�������Ʋ��Żص�����
*@param[in]:int srcno  Դ��ʶ
*@param[in]:int trackid  ��ID
*@param[in]:long chid  ͨ��
*@param[in]:double npt  �ͻ���seek֮���nptʱ��
*@param[in]:float scale  �ͻ���seek֮��Ĳ��ű���
*@param[out]:uint32_t *rtp_pkt_seq
*@param[out]:uint32_t *rtp_pkt_timestamp  seek֮���rtp����ʱ��
**/
typedef int (MEDIASERVER_STDCALL *xt_rtsp_play_cb)(int srcno, int trackid, long chid, double npt, float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);

/**
*@function:rtsp����������ͣ�ص�����
*@param[in]:int srcno,  Դ��ʶ
*@param[in]:int trackid,  ��ID
*@param[in]:long chid  ͨ��
**/
typedef int (MEDIASERVER_STDCALL *xt_rtsp_pause_cb)(int srcno, int trackid, long chid);

/**
*@function:rtcpЭ���������Ʋ���
*@param[in]:int srcno  Դ��ʶ
*@param[in]:long chid  ͨ��
*@param[in]:double npt  �ͻ���seek֮���nptʱ��
*@param[in]:float scale  �ͻ���seek֮��Ĳ��ű���
*@param[out]:uint32_t *rtp_pkt_timestamp  seek֮���rtp����ʱ��
**/
typedef int (MEDIASERVER_STDCALL *xt_tcp_play_cb_type)(int srcno, long chid, double npt, float scale, uint32_t *rtp_pkt_timestamp);

/**
*@function:tcpЭ������������ͣ�ص�����
*@param[in]:int srcno  Դ��ʶ
*@param[in]:long chid  ͨ��
**/
typedef int( MEDIASERVER_STDCALL *xt_tcp_pause_cb_type)(int srcno, long chid);

//��·������ֹͣ�¼�
enum LINK_STATE_EVENT
{
    EVENT_TCP_PLAY=0, 
    EVENT_TCP_STOP,
    EVENT_TCP_CLOSE,
    EVENT_UDP_PLAY,
    EVENT_UDP_STOP,
    EVENT_RTSP_PLAY,
	EVENT_RTSP_CTRLPLAY,
	EVENT_RTSP_CTRLPAUSE,
    EVENT_RTSP_STOP,
}; 
typedef int(MEDIASERVER_STDCALL *xt_link_state_event_type)(const LINK_STATE_EVENT evnt,const int srno);

//��־����ص���
enum log_level_type
{
    log_info=0,
    log_err,
    log_warn,
};
typedef void(MEDIASERVER_STDCALL *xt_media_server_log_cb_type)(char* logname,log_level_type level,char* log_ctx,uint32_t log_ctx_len);

//ע��ʱ�����ص�
typedef void(MEDIASERVER_STDCALL *regist_response_callback_t)(const char *ip, uint16_t port, uint32_t code);

// mediaserver config
struct MS_CFG 
{
    int num_chan;                        // ͨ������
    char ip[MEDIA_SERVER_IP_LEN];                  // ip
    unsigned short snd_start_port;                 // ��ʼ�˿�
    bool         demux;                           // ����
    char mul_start_ip[MEDIA_SERVER_IP_LEN];          // �鲥��ʼ��ַ
    unsigned short msg_liten_port;                  // ˽�м����˿�   0��ʾ������
    unsigned short rtsp_listen_port;                // rtsp�����˿�     0��ʾ������
    unsigned short tcp_listen_port;                 // tcp����������Ͷ˿�  0��ʾ������
    unsigned short udp_listen_port;                 // udp�����˿�         0��ʾ������

    bool          snd_std_rtp;                     // �Ƿ��ͱ�׼��
    bool         sink_single;                      // �Ƿ�һת��
    bool        use_traffic_shaping;                //�Ƿ�����������
    xt_rtsp_play_cb      rtsp_play_cb;               // ����RTSP����������
    xt_rtsp_pause_cb     rtsp_pause_cb;
    xt_tcp_play_cb_type   tcp_play_cb;                // ����TCP�Ự����������
    xt_tcp_pause_cb_type  tcp_pause_cb;
    xt_link_state_event_type xt_link_state_event;       // ��·״̬�¼�
    xt_media_server_log_cb_type xt_media_server_log_cb;  // ��־����ص���
    xt_rtcp_force_iframe_cb_t  rtcp_force_iframe_cb;     // RTCPý���ǿ��I֡�ص�
    MS_CFG():
        num_chan(0),
        snd_start_port(0),
        demux(false),
        msg_liten_port(0),
        rtsp_listen_port(0),
        tcp_listen_port(0),
        udp_listen_port(0),
        snd_std_rtp(false),
        sink_single(false),
        use_traffic_shaping(false),
        rtsp_play_cb(NULL),
        rtsp_pause_cb(NULL),
        tcp_play_cb(NULL),
        tcp_pause_cb(NULL),
        xt_link_state_event(NULL),
        xt_media_server_log_cb(NULL),
        rtcp_force_iframe_cb(NULL){}
};
struct src_track_info_t 
{
    int tracknum;
    int trackids[MAX_TRACK];
    char tracknames[MAX_TRACK][128];
};

//SIP�ص�
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SIPMSG_NOEDE_LEN 128

//�����Ʋ����ṹ
struct sipmsg_ptz 
{
    char commandname[SIPMSG_NOEDE_LEN];
    char operation[SIPMSG_NOEDE_LEN];
    char seq[SIPMSG_NOEDE_LEN];
    char devname[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char zoomctrl[SIPMSG_NOEDE_LEN];
    char focusctrl[SIPMSG_NOEDE_LEN];
    char aperturectrl[SIPMSG_NOEDE_LEN];
    char direction[SIPMSG_NOEDE_LEN];
    char speed[SIPMSG_NOEDE_LEN];
    char pointindexvalue[SIPMSG_NOEDE_LEN];
};
/**
*@ function:�ƾ�����
*@ param[in]:const sipmsg_ptz *ptz:������� �����Ʋ���
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_ptz_cb)(const sipmsg_ptz *ptz);


/**
*@ function: ǿ��I֡
*@ param[in]:const sipmsg_ifrmconfreq *ifrmconfreq: ������� �豸I֡�����ز���
*@ param[out]:int& out_oprcode: ������� 0:Ϊ�����ɹ� С��0��Ϊ����ʧ��
*@ param[out]:char* out_fail_case: ������� �˲�����out_oprcode��� ��out_oprcodeΪС��0ʱҪд��ʧ��ԭ�򳤶Ȳ��ܳ���2048������ֽض�
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_picupdate_cb)(int streamid,int& out_oprcode,char* out_fail_case);

/**
*@ function: �Ự��� message��Ϣ
*@ param[in]:const char *xml: ������� ��Ϣ��
*@ param[in]:unsigned int len: ������� ��Ϣ�峤��
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_cb)(const char *xml, unsigned int len);

/**
*@ function:�Ự��info �ص���
*@ param[in]:const char *xml: ������� ��Ϣ��
*@ param[in]:unsigned int len: ������� ��Ϣ�峤��
**/
typedef void(MEDIASERVER_STDCALL *sipinfo_cb)(int streamid, const char *xml, unsigned int len);

//�豸I֡���
struct sipmsg_ifrmconfreq 
{
    int seq;
    char devname[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char groupid[SIPMSG_NOEDE_LEN];
    int iframedelta;
};
/**
*@ function:�豸I֡���
*@ param[in]:const sipmsg_ifrmconfreq *ifrmconfreq: ������� �豸I֡�����ز���
*@ param[out]:int& out_oprcode: ������� 0:Ϊ�����ɹ� С��0��Ϊ����ʧ��
*@ param[out]:char* out_fail_case: ������� �˲�����out_oprcode��� ��out_oprcodeΪС��0ʱҪд��ʧ��ԭ�򳤶Ȳ��ܳ���2048������ֽض�
**/
typedef void(MEDIASERVER_STDCALL *sipmsg_ifrmconfreq_cb)(const sipmsg_ifrmconfreq *ifrmconfreq,int& out_oprcode,char* out_fail_case);


//��Ƶ�����ص�
typedef struct _struct_video_adjust_param_t 
{
    char framerate[64];  //֡��
    char max_br[64];    //����
    char resolution[64]; //�ֱ���
}video_adjust_param_t,*pvideo_adjust_param_t;
typedef void(MEDIASERVER_STDCALL *sip_video_adjust_cb)(const video_adjust_param_t&adjust_params,const int streamid);

//��ȡ�豸��Ϣ
typedef struct _struct_device_info_type
{
    char devname[SIPMSG_NOEDE_LEN];         // �豸����
    char vendername[SIPMSG_NOEDE_LEN];      // �豸����
    char deviceversion[SIPMSG_NOEDE_LEN];   // �豸�ͺ�
    char bandwidth[SIPMSG_NOEDE_LEN];       // ��Ƶ�������ʵ�λkbps
}dev_info_t,*pdev_info_t;

/*
*@param[in]:dev_info_t& out_devinfo: Ϊ����豸��Ϣ
*@return int :0<=����ȡ�ɹ� 0>:��ȡʧ��
*/
typedef int (MEDIASERVER_STDCALL *sip_get_dev_info_cb)(dev_info_t& out_devinfo);

//Ԥ�õ����
typedef struct _struct_point_index_operation_type_
{
    char operation[SIPMSG_NOEDE_LEN];//add|remove|set
    char devname[SIPMSG_NOEDE_LEN];
    char devid[SIPMSG_NOEDE_LEN];
    char  seq[SIPMSG_NOEDE_LEN];
    char pointindexvalue[SIPMSG_NOEDE_LEN];
}point_index_operation_t,*ppoint_index_operation_t;

typedef int (MEDIASERVER_STDCALL *sip_point_index_operation_cb)(point_index_operation_t& pio);

/*
*@ function:����ע��sip��������ע����
*@param[in]:const char* target ע��Ŀ��
*@param[in]:const uint8_t success success=0��ʾʧ��else�ɹ�
*/
typedef void (MEDIASERVER_STDCALL* sip_register_srv_ret_info_cb)(const char* target,const uint8_t success);

/*
*@ function:������SDP���b�ֶΣ�����b�ֶδ�����������
*@param[in]:uint32_t bandwidth ������ֵ
*@return[]:�ɹ�����0��ʧ�ܷ��ظ���
*/
typedef int (MEDIASERVER_STDCALL* sipmsg_bandwidth_cb)(uint32_t bandwidth);


/*
*@ function:������SDP���a�ֶε�fmtp������fmtp�ַ�������������
*@param[in]:const char* profile  fmtp�ַ���
*@return[]:�ɹ�����0��ʧ�ܷ��ظ���
*/
typedef int (MEDIASERVER_STDCALL* sipmsg_profile_cb)(const char* profile);
MEDIASERVER_API void xt_sipmsg_ptz_cb(sipmsg_ptz_cb cb);
MEDIASERVER_API void xt_sipmsg_picupdate_cb(sipmsg_picupdate_cb cb);
MEDIASERVER_API void xt_sipmsg_cb(sipmsg_cb cb);
MEDIASERVER_API void xt_sipinfo_cb(sipinfo_cb cb);
MEDIASERVER_API void xt_sipmsg_ifrmconfreq_cb(sipmsg_ifrmconfreq_cb cb);
MEDIASERVER_API void xt_regist_sip_video_adjust_cb(sip_video_adjust_cb cb);
MEDIASERVER_API void xt_get_dev_info_cb(sip_get_dev_info_cb cb);
MEDIASERVER_API void xt_point_index_operation_cb(sip_point_index_operation_cb cb);
MEDIASERVER_API void xt_sip_register_srv_ret_info_cb(sip_register_srv_ret_info_cb cb);
MEDIASERVER_API void xt_sipmsg_bandwidth_cb(sipmsg_bandwidth_cb cb);
MEDIASERVER_API void xt_sipmsg_profile_cb(sipmsg_profile_cb cb);
///////////////////////////////////////////////////////////////////////////////////////////////////

//����ӿ�
//////////////////////////////////////////////////////////////////////////////////////////  
/**
*@name:xt_init_server
*@funcion:��ʼ�� xt_media_server��
*@param[int] MS_CFG &cfg ������Ϣ
**/
MEDIASERVER_API int xt_init_server(MS_CFG &cfg);

/**
*@name:xt_uninit_server
*@funcion:����ʼ�� xt_media_server��
**/
MEDIASERVER_API int xt_uninit_server();

/**
*@name:xt_create_src
*@function:����ת��Դ
*@param[in]:int tracknum  ������
*@param[in]:int *trackids  ��ids
*@param[in]:char *tracknames[] ��������
*@param[out]:int &srcno Դ��ʶ
*@param[in]:long chanid = -1  ָ��ͨ��
**/
MEDIASERVER_API int xt_create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid = -1);

// track��Ϣ
#define PRI_TRACK_ID -1
#define MAX_TRACKNAME_LEN 64
typedef struct _struct_xt_track_type__
{
    char trackname[MAX_TRACKNAME_LEN];    // ��name
    int trackid;                          // ��id
    int tracktype;                        // �������͡�0��video��1��audio����1������
    int frametype;                        // ����֡���� �ο�share_type_def.h ov_frame_type
    unsigned long chanid;                 // ͨ��id
}xt_track_t,*pxt_track_t;
/**
*@name:xt_create_src_sdp
*@function:����ת��Դ
*@param[out]:int* srcno  Դ��ʶ
*@param[out]:xt_track_t* trackinfos
*@param[out]:int* tracknum
*@param[in]:int *sdp  sdp
*@param[in]:char sdp_len 
*@param[in]:long chanid ָ��ͨ�� -1��ʾ�ڲ��Զ�����
**/
MEDIASERVER_API int xt_create_src_sdp(int* srcno,xt_track_t* trackinfos,int* tracknum,const char* sdp,const long sdp_len,const long chanid);

/**
*@name:xt_create_src_defult
*@function:����ת��Դ
*@param[out]:int* srcno  Դ��ʶ
*@param[out]:char sdp[]
*@param[out]:int* sdp_len
*@param[in]:long chanid ָ��ͨ�� -1��ʾ�ڲ��Զ�����
*@param[in]:const char* local_bind_ip ���ذ�IP
**/
MEDIASERVER_API int xt_create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip);

/**
*@name:xt_create_src
*@function:����ת��Դ
*@param[in]:src_track_info_t &tracks  ����Ϣ
*@param[out]:int &srcno  Դ��ʶ
*@param[in]:long chanid = -1  ָ��ͨ�� -1��ʾ�ڲ��Զ�����
**/
MEDIASERVER_API int xt_create_src(src_track_info_t &tracks, int &srcno, const long chanid = -1);

/**
*@name:xt_destroy_src
*@function:ɾ��ת��Դ
*@param[in]:int srcno Դ��ʶ
**/
MEDIASERVER_API int xt_destroy_src(int srcno);

/**
*@name:xt_set_key
*@function:����ϵͳͷ
*@param[in]:int srcno
*@param[in]:char *keydata
*@param[in]:long len
*@param[in]:long datatype
**/
MEDIASERVER_API int xt_set_key(int srcno, char *keydata, long len, long datatype);

/**
*@name:xt_set_sipsdp_full
*@function:����sipȫ������ϵͳͷ
*@param[in]:int srcno
*@param[in]:char *keydata
*@param[in]:long len
**/
int xt_set_sipsdp_full(int srcno, char *keydata, long len);

/**
*@name:xt_send_data
*@function:���ݷ���
*@param[in]:int srcno  Դid
*@param[in]:int trackid  trackid
*@param[in]:char *buff  ��������
*@param[in]:unsigned long len  ���ݳ���
*@param[in]:int frame_type  ֡����
*@param[in]:long device_type  �豸����
**/
MEDIASERVER_API int xt_send_data(int srcno, int trackid, char *buff, unsigned long len, int frame_type, long device_type);

/**
*@name:xt_send_data_in_stamp
*@function:���ݷ������ⲿ����ʱ��������
*@param[in]:int srcno  Դid
*@param[in]:int trackid  trackid
*@param[in]:char *buff  ��������
*@param[in]:unsigned long len  ���ݳ���
*@param[in]:int frame_type  ֡����
*@param[in]:long device_type  �豸����
*@param[in]:bool frame_ts_flg  �Ƿ��ⲿ����ʱ��
*@param[in]:uint32_t in_time_stamp  �ⲿ����ʱ��
**/
MEDIASERVER_API int xt_send_data_in_stamp(int srcno, int trackid, char *buff, unsigned long len, 
										  int frame_type, long device_type, bool frame_ts_flg, uint32_t in_time_stamp,
										  bool use_ssrc,uint32_t ssrc);

MEDIASERVER_API int xt_send_rtp_in_stamp(int srcno, int trackid, char *buff, unsigned long len, 
										  int frame_type, long device_type, bool frame_ts_flg, uint32_t in_time_stamp,
										  bool use_ssrc,uint32_t ssrc);

/**
*@name:xt_send_data_in_stamp_p
*@function:���ݷ������ⲿ�����ش����ȼ�
*@param[in]:int srcno  Դid
*@param[in]:int trackid  trackid
*@param[in]:char *buff  ��������
*@param[in]:unsigned long len  ���ݳ���
*@param[in]:int frame_type  ֡����
*@param[in]:long device_type  �豸����
*@param[in]:bool frame_ts_flg  �Ƿ��ⲿ����ʱ��
*@param[in]:uint32_t in_time_stamp  �ⲿ����ʱ��
*@param[in]:uint8_t priority  �����������ȼ�
**/
MEDIASERVER_API int xt_send_data_in_stamp_p(int srcno,int trackid,char *buff,unsigned long len,int frame_type,long device_type,bool frame_ts_flg,uint32_t in_time_stamp,uint8_t priority);

/**
*@name:xt_send_data_in_stamp_ps
*@function:ps�������ݷ��ͽӿ�
*@param[in]:int srcno  Դid
*@param[in]:int trackid  trackid
*@param[in]:char *buff  ��������
*@param[in]:unsigned long len  ���ݳ���
*@param[in]:int frame_type  ֡����
*@param[in]:long device_type  �豸����
*@param[in]:bool frame_ts_flg  �Ƿ��ⲿ����ʱ��
*@param[in]:uint32_t in_time_stamp  �ⲿ����ʱ��
*@param[in]:uint8_t priority  �����������ȼ�
**/
MEDIASERVER_API int xt_send_data_in_stamp_ps(int srcno,int trackid,char *buff,unsigned long len,int frame_type,long device_type,bool frame_ts_flg,uint32_t in_time_stamp,uint8_t priority,bool use_ssrc, uint32_t ssrc);

/**
*@name:xt_send_data_single
*@function:����ϵͳ֧��
*@param[in]:int src_prime  Դid
*@param[in]:int trackid  tracki
*@param[in]:char *buff  ��������
*@param[in]:unsigned long len  ���ݳ���
*@param[in]:int frame_type  ֡����
*@param[in]:long device_type  �豸����
**/
MEDIASERVER_API int xt_send_data_single(int src_prime, int trackid, char *buff, unsigned long len, int frame_type, long device_type);


/**
*@name:xt_get_chanid
*@function:����ת��Դ��Ӧͨ����
*@param[in]:int srcno
*@param[in]:int trackid
*@param[out]:long &chanid
**/
MEDIASERVER_API int xt_get_chanid(int srcno, int trackid, long &chanid);

/**
*@name:xt_get_sinksn
*@function:��ȡһ·rtp��sn
*@param[in]:const int srcno
*@param[in]:const int trackid
*@param[out]:unsigned short *sn
**/
MEDIASERVER_API int xt_get_rtp_sn(const int srcno, const int trackid, unsigned short *sn);

/**
*@name:xt_regist
*@function: �����������ע��
*@param[in]:const char* sz_ids
*@param[in]:const char* sz_local_ip
*@param[in]:unsigned short local_port
*@param[in]:const char* sz_server_ip
*@param[in]:unsigned short server_port
**/
MEDIASERVER_API int xt_regist(const char* sz_ids, const char* sz_server_ip, unsigned short server_port, uint32_t millisec);

/**
*@name:xt_stop_regist
*@function:�رշ���������ע��
*@param[in]:const char* sz_server_ip
*@param[in]:unsigned short server_port)
**/
MEDIASERVER_API int xt_stop_regist(const char* sz_server_ip,unsigned short server_port,uint32_t millisec);

/**
*@name:xt_isregist
*@function:�鿴����ע��״̬
**/
MEDIASERVER_API bool xt_isregist();

//����ע�����ص�
MEDIASERVER_API void xt_regist_response_callback(regist_response_callback_t func);


//xmpp JigaleRTP֧��
///////////////////////////////////////////////////////////////////////////////
enum xmpp_msg_t
{
    XMPP_STOP_PLAY = 0,
    XMPP_START_PLAY,
};
typedef struct _struct_xmpp_cfg
{
    xmpp_msg_t   ctrl;               //���Ʊ�� 
    char  LAddr[MEDIA_SERVER_IP_LEN];  //����IP
    char  Addr[MEDIA_SERVER_IP_LEN];  //Զ��IP
    long  Channle;
    long  link_type;
    unsigned short nRptPort_r;      //Զ�˶˿� 
    unsigned int multiplexID;       //����ID
    bool  multiplex;                //�Ƿ�˿ڸ���

}xmpp_cfg_t,*pxmpp_cfg_t;
/**
*@name:add_trans_sever
*@function:xmpp JigaleRTP ����һ��ת��
*@param[in]:xmpp_cfg_t& cfg
**/
MEDIASERVER_API int add_trans_sever(xmpp_cfg_t& cfg);
///////////////////////////////////////////////////////////////////////////////

/**
*@name:xt_get_cur_connect_num
*@ function:��ȡ��ǰ������
*@ return: int ���ص�ǰ������
*@ note: ���xt_get_connect_info �ӿ�ʹ��
**/
MEDIASERVER_API uint32_t xt_get_cur_connect_num();

/**
*@name:xt_get_connect_info
*@ function:��ȡ��ǰ������Ϣ
*@ param[out]:connect_info_t out_cinfo[]
*@ param[in out]:connect_num  in:������������� out:ʵ�ʻ�ȡ����������
*@ return: int С��0Ϊʧ��
**/
MEDIASERVER_API int xt_get_connect_info(connect_info_t out_cinfo[], uint32_t& connect_num);


//����SIPЭ��ջ�Ķ�ʱ����������
typedef struct _struct_start_sip_timer_type_ 
{
    uint32_t default_session_time_interval; //�Ự����ʱ�䣬��λ���� [90, 180]�� ����UPDATE
    uint32_t default_registration_retry_time_interval; //ע������ʱ��
}start_timer_t,*pstart_timer_t;

typedef enum _enum_sip_trans_protocol_type_
{
    TRANS_PROTOCOL_NONE = 0,
    TRANS_PROTOCOL_TCP = 1<<0,
    TRANS_PROTOCOL_UDP = 1<<1
}sip_trans_protocol_t,*psip_trans_protocol_t;
typedef struct _struct_start_sip_port_type_
{
    uint32_t protocol;   //����Э�� ��sip_trans_protocol_t ����"|"����������п���ֻʹ��tcp��udp�����ͬʱʹ��
    uint16_t transport;  //sip ����˿�
    uint16_t tls_port;  //�����tls�˿ڣ���0��ʾ������
    uint16_t dtls_port;  //�����dtls�˿ڣ���0��ʾ������
}start_sip_port_t,*pstart_sip_port_t;
/**
*@name:xt_start_sip
*@ function: ����SIPЭ��ջ
*@ param[in]:const start_sip_port_t& sip_port:�˿ڼ�Э����Ϣ
*@ param[in]:const char *domain: �û���֤����
*@ param[in]:const char *username: 
*@ param[in]:const char *password: 
*@ param[in]:const start_timer_t& timer:��ʱ��
**/
MEDIASERVER_API int xt_start_sip(const start_sip_port_t& sip_port, const start_timer_t& timer, const char *domain, const char *username, const char *password);

/**
*@name:xt_stop_sip
*@function:ֹͣSIPЭ��ջ
**/
MEDIASERVER_API void xt_stop_sip();

//ע��ʱ�ŵ���������
typedef enum _enum_channel_type
{
    XT_CHAN_NONE            = 0,
    XT_CHAN_SATELLITE       = 1<<0, //���� satellite
    XT_CHAN_MOBILE          = 1<<1, //�ƶ� mobile
    XT_CHAN_MICROWARE       = 1<<2, //΢�� microwave
    XT_CHAN_SHORTWARE       = 1<<3, //���̲� shortwave
    XT_CHAN_FIXED           = 1<<4 //�̶�����ͨ��  fixed
}channel_t;
//�ŵ��������ο��ܲ��淶
typedef struct _struct_sip_channel_type_
{
    uint32_t type;                  //�ŵ����� ��ֵ��channel_t ������õ��п�����һ������Ҳ�����Ƕ�������
    uint32_t delay;                 //��ʱ
    uint32_t packetloss;            //������
    uint32_t mtu;                   //����䵥Ԫ Ĭ�ϣ�1500
    uint32_t bandwidth;             //����
}sip_channel_t,*psip_channel_t;

typedef struct _struct_sip_regist_timer_type_
{
    uint32_t  expires;    //��������˼ 1���������������ʱ�� 2��REGIST ˢ��ʱ�� [60, 3600]��
    uint32_t  link_time_interval; //��·����ʱ�� ��λ����  ��OPTION��ʱ�� [25000, 90000]ms
    uint32_t  regist_retry_time_interval; //REGIST ʧ������ʱ�� [60,3600]s

}regist_timer_t,*pregist_timer_t;
/**
*@name:xt_regist_sip
*@ function:����������ע��
*@ param[in]:const char *target ע��Ŀ������� ����sip:router@11.55.30.11:5060
*@ param[in]:const regist_timer_t& timer ע�ᶨʱ��ʱ�����ò���
*@ param[in]:const char* sdp
*@ param[in]:uint32_t sdp_len
*@ param[in]:const sip_channel_t& chtype �ŵ���Ϣ
*@ return: 0:�ɹ� -1:target Ϊ��ʧ��
**/ 
MEDIASERVER_API int xt_regist_sip(const regist_timer_t& timer, const sip_channel_t& chtype,const char *target,const char* sdp, uint32_t sdp_len);

/**
*@name:xt_unregist_sip
*@function: sip unregister
**/
MEDIASERVER_API void xt_unregist_sip();


/**
*@name:xt_add_send
*@function: ����ת��
*@param[in]:int srcno  Դid
*@param[in]:int trackid  track id
*@param[in]:const char *ip,  Ŀ��ip
*@param[in]:unsigned short port  Ŀ��port
*@param[in]:bool   demux = false  ����
*@param[in]:unsigned int demuxid = 0  ����id
**/
MEDIASERVER_API int xt_add_send(int srcno, int trackid, const char *ip, unsigned short port, bool   demux = false, unsigned int demuxid = 0);

/**
*@name:
*@function:����ת��
*@param[in]:int srcno  Դid
*@param[in]:int trackid  track name
*@param[in]:const char *ip,  Ŀ��ip
*@param[in]:unsigned short port  Ŀ��port
*@param[in]:bool   demux = false  ����
*@param[in]:unsigned int demuxid = 0  ����id
**/
MEDIASERVER_API int xt_add_send(int srcno, const char  *track, const char  *ip, unsigned short port, bool   demux = false, unsigned int demuxid = 0);

/**
*@name:
*@function:����payload
*@param[in]:int srcno  Դid
*@param[in]:int trackid  track id
*@param[in]:int payload  payload
*@param[in]:bool update  update
**/
MEDIASERVER_API int xt_set_payload(int srcno, int trackid, int payload, bool update);

/**
*@name:
*@function:����payload
*@param[in]:int srcno  Դid
*@param[in]:const char  *track  track name
*@param[in]:int payload  payload
*@param[in]:bool update  update
**/
MEDIASERVER_API int xt_set_payload(int srcno, const char  *track, int payload, bool update);

/**
*@name:
*@function:ɾ��ת��
*@param[in]:int srcno,  Դid
*@param[in]:int trackid  track id
*@param[in]:const char  *ip  Ŀ��ip
*@param[in]:unsigned short port / Ŀ��port
*@param[in]:bool demux = false  ����
*@param[in]:unsigned int demuxid = 0  ����id
**/
MEDIASERVER_API int xt_del_send(int srcno, int trackid, const char  *ip, unsigned short port, bool demux = false, unsigned int demuxid = 0);

/**
*@name:xt_del_send
*@function:xt_del_send
*@param[in]:int srcno  Դid
*@param[in]:const char *tracki  track name
*@param[in]:unsigned short port  Ŀ��port
*@param[in]:bool demux = false  ����
*@param[in]:unsigned int demuxid = 0  ����id
**/
MEDIASERVER_API int xt_del_send(int srcno, const char *tracki, const char *ip, unsigned short port, bool demux = false, unsigned int demuxid = 0);

/**
*@name:xt_del_send_src
*@function:  ɾ��ת��(srcno Դid)
*@param[in]:int srcno
**/
MEDIASERVER_API int xt_del_send_src(int srcno);

/**
*@name:xt_del_send_all
*@function:ɾ������ת��
**/
MEDIASERVER_API int xt_del_send_all();

//������Ϣ�ṹ����
typedef struct _struct_server_info
{
    int trackid;
    bool multiplex_s;     // ���Ͷ˸��ñ�ʶ 
    uint32_t rtp_send_port;  // ���˿�
    uint32_t rtcp_send_port;  // ���˿�    
    uint32_t  multid_s;      // ���Ͷ˸���ID
    char trackname[64];      //��name

}svr_info,*psvr_info;
/**
*@name:xt_get_svr_info
*@function:xt_get_svr_info
*@param[out]:svr_info info[]  ת����Ϣ
*@param[in out]:int& tracknum  ������
*@param[in]:const int srcno  Դ��ʶ
**/
MEDIASERVER_API int xt_get_svr_info(svr_info info[],int& tracknum,const int srcno);

/**
*@ function:�޸Ķ����ش�����
*@ param[in] const int resend_flag �ش����ر�־ ����0Ϊ��
*@ return int ������� С��0����ʧ��
**/
MEDIASERVER_API int xt_update_resend_flag(const int resend_flag);

MEDIASERVER_API void xt_set_file_path(const char * file);

/**
*@ function: ����RTSP�Ự������ʱ��
*@ param[in] const unsigned int check_timer_interval ��ⶨʱ���Ĵ���ʱ����ms
*@ param[in] const unsigned int time_out_interval �ж���ʱ��ʱ������ms
*@ return int ������� С��0ʧ�� ����Ϊ�ɹ�
**/
MEDIASERVER_API int xt_ms_set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

#ifdef _USE_RTP_SEND_CONTROLLER
typedef void (MEDIASERVER_STDCALL*xt_network_changed_callback_t)(void *ctx, uint32_t bitrate, uint32_t fraction_lost, uint32_t rtt);
MEDIASERVER_API int xt_register_network_changed_callback(int srcno, int trackid, xt_network_changed_callback_t cb, void *ctx);
#endif

//��˾��������
typedef enum
{
    ms_code_na   = -1,
    ms_code_main  = 0,
    ms_code_sub   = 1,
    ms_code_audio = 2,
}ms_code_t;

/**
*@ function: ������ת����ѯ
*@ param[in] const int main_srcno ��ͨ��
*@ param[in] const ms_code_t code ��������
*@ param[out] int* srcno ʵ��ת����ת��Դ��ʶ��
*@ return int ������� С��0ʧ�� ����Ϊ�ɹ�
**/
typedef int (MEDIASERVER_STDCALL *multi_code_query_callback_t)(const int prime_srcno,const ms_code_t code,int* srcno);
MEDIASERVER_API void xt_register_multi_code_query_callback(multi_code_query_callback_t cb);

#endif  //_H_XTMEDIASERVER_H
