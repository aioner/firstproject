#ifndef _H_XTSESSIONSERVER_H
#define _H_XTSESSIONSERVER_H

#ifdef _WIN32
#define USE_MSG_SERVER_
#ifdef XTSESSIONSERVER_EXPORTS
#define SESSIONSERVER_API __declspec(dllexport)
#else
#define SESSIONSERVER_API __declspec(dllimport)
#endif

#define SESSIONSERVER_STDCALL __stdcall
#else
#define SESSIONSERVER_API __attribute__((visibility("default")))
#define SESSIONSERVER_STDCALL 
#endif

#include "stdint.h"
#include "xt_log_def.h"

#define SESSION_SERVER_IP_LEN   32
enum PROTOL_TYPE
{
	PT_MSG       = 1,		//XTMsg
	PT_RTSP      = 2,	   //RTSP
	PT_CTRL_MSG   = 3,    //TCP�������
	PT_CTRL_RTSP  = 4     //RTSP�������
};

enum XT_MSG_TYPE
{
	MSG_STARTPLAY        = 1,		//�����㲥��Ϣ
	MSG_STOPPLAY         = 2,		//ֹͣ�㲥��Ϣ
	MSG_CLOSEMSG         = 3,		//�Ͽ�����������Ϣ
	MSG_GETSNDINFO       = 4,		//��÷��Ͷ���Ϣ
	MSG_TRANS_CTRL_PLAY  = 5,		//��λ����
	MSG_TRANS_CTRL_PAUSE = 6,		//��ͣ

	RTSP_STARTPLAY      = 101,		//�����㲥��Ϣ
	RTSP_STOPPLAY       = 102,		//ֹͣ�㲥��Ϣ
	RTSP_GETSNDINFO     = 103,		//��÷��Ͷ���Ϣ
	RTSP_PLAY           = 104,		//��λ����
	RTSP_PAUSE          = 105,		//��ͣ

	STD_ADD_SEND		= 201,		//���ת��
	STD_DEL_SEND		= 202		//ɾ��ת��
};

struct STR_MsgData{
	XT_MSG_TYPE			ctrl;				//���Ʊ��
	void*				pMsgContext;		//���ӹ�����
	char				LAddr[SESSION_SERVER_IP_LEN];				//����IP
	char				Addr[SESSION_SERVER_IP_LEN];				//Զ��IP
	unsigned short		nMsgPort;			//Զ�˶˿�(��������ʱ���ɵ�)
	
	uint16_t nRecvPort;			//���������ö˿�					
	uint16_t nDataChannelID;		//Ҫ���յ�����ͨ����
	uint16_t nMode;				//��������ģʽ(0,1: ������2���鲥)	//��RTP�ĵ㲥�����д˲�����ʾRctpSsrc

	unsigned int		nSSRC;				//RTP��������ʱ
	bool				multiplex;			//�Ƿ�˿ڸ���
	unsigned int		multiplexID;		//����ID

};

struct STR_MsgCtrlData
{
	XT_MSG_TYPE			ctrl;			    // ���Ʊ��
	void*				pMsgContext;		// ���Ӿ��
	uint32_t          data_chid;          // ����ͨ��
	double            npt;               // �ͻ���seek֮���nptʱ��  
	float             scale;             // �ͻ���seek֮��Ĳ��ű���
	uint32_t          *rtp_pkt_timestamp;   // seek֮���rtp����ʱ��
};

struct Msg_Rtsp 
{
	XT_MSG_TYPE			ctrl;			//���Ʊ��	
	void				*session;		//���Ӿ��
	int					srcno;			//srcno
	int					trackid;  		//track id 
	char				pstrRAddr[SESSION_SERVER_IP_LEN]; 		//IP
	unsigned short		nRPort;		 	//port
	bool				multiplex;		//�Ƿ�˿ڸ���
	unsigned int		multiplexID;	//����ID
};

struct std_send_task
{
	XT_MSG_TYPE			ctrl;			//���Ʊ��	
	void				*session;		//���Ӿ��
	int					srcno;			//srcno
	int					trackid;  		//track id 
	char				ip[SESSION_SERVER_IP_LEN]; 		//IP
	unsigned short		port;		 	//port
	bool				demux;		//�Ƿ�˿ڸ���
	unsigned int		demux_id;	//����ID
};

struct Msg_Rtsp_Ctrl
{
	XT_MSG_TYPE			ctrl;			//���Ʊ��	
	void				*session;		//���Ӿ��
	int					srcno;			//srcno
	int					trackid;  		//track id 
	double            npt;
	float             scale;
	uint32_t          *rtp_pkt_seq;
	uint32_t          *rtp_pkt_timestamp;
};

typedef int (SESSIONSERVER_STDCALL *session_msg_cb)(PROTOL_TYPE type, void* msg); 

// ���ûỰ�ص�
SESSIONSERVER_API int xt_set_sessionmsg_cb(session_msg_cb func);

//��������
////////////////////////////////////////////////////////////////////////////////////////////////////////
//RTSP
typedef int (SESSIONSERVER_STDCALL *rtsp_play_cb)(void *hSession, int sessionno, int trackid, double npt, 
												  float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);
typedef int (SESSIONSERVER_STDCALL *rtsp_pause_cb)(void *hSession, int sessionno, int trackid);
SESSIONSERVER_API int xt_set_rtsp_play_cb(rtsp_play_cb func);
SESSIONSERVER_API int xt_set_rtsp_pause_cb(rtsp_pause_cb func);

//TCP
typedef int (SESSIONSERVER_STDCALL *tcp_play_cb_type)(void *p_context,   // ���ӹ�����
													  uint32_t data_chid,           // ����ͨ��
													  double npt,                  // �ͻ���seek֮���nptʱ��  
													  float scale,                 // �ͻ���seek֮��Ĳ��ű���
													  uint32_t *rtp_pkt_timestamp   // seek֮���rtp����ʱ��
													  );
typedef int( SESSIONSERVER_STDCALL *tcp_pause_cb_type)(void *p_context, // ���ӹ�����
													   uint32_t data_chid          // ����ͨ��
													   );
//����ע���ȡ����ص�
typedef void(SESSIONSERVER_STDCALL *regist_response_callback_t)(const char *ip, uint16_t port, uint32_t code);

SESSIONSERVER_API int xt_set_tcp_play_cb(tcp_play_cb_type func);
SESSIONSERVER_API int xt_set_tcp_pause_cb(tcp_pause_cb_type func);
////////////////////////////////////////////////////////////////////////////////////////////////////////

// ��ʼ��˽�п�
SESSIONSERVER_API int xt_init_msg(const char *ip,					// ����IP
								  unsigned short msg_listen_port,	// ��������˿� msg�����˿�
								  unsigned short udp_listen_port,	//udp�����˿�
								  unsigned short send_start_port,	// ������ʼ�˿�
								  const char *mul_start_addr,		// �鲥������ʼ��ַ
								  unsigned short mul_port,			// �鲥���Ͷ˿�
								  xt_print_cb func);				// ��־���

// ��ʼ��rtsp��(listen_port ��������˿�)
SESSIONSERVER_API int xt_init_rtsp(const char *ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func);

SESSIONSERVER_API int xt_set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

// ����ʼ��
SESSIONSERVER_API int xt_uninit_msg();
SESSIONSERVER_API int xt_uninit_rtsp();


SESSIONSERVER_API int xtr_regist(const char *regist_ids,const char *server_ip, unsigned short server_port,uint32_t millisec);



SESSIONSERVER_API int xtr_stop_regist( const char* server_ip, unsigned short server_port,uint32_t millisec );

// XTMsg
//////////////////////////////////////////////////////////////////////////
// ����ϵͳͷ
SESSIONSERVER_API int xt_set_key_data(unsigned long nDataChID, char *pHeadData, long nHeadSize, long nDataType);

// ���ø��ò���
SESSIONSERVER_API int  xtm_set_snd_port(unsigned long nDataChID, unsigned short sndport, bool multiplex,unsigned int multid);

// ���ϵͳͷ
SESSIONSERVER_API int  xt_clear_key_data(unsigned long nDataChID);

//��ӻỰ�ͻ���
SESSIONSERVER_API int xt_msg_add_socket_client(void *sock);
//////////////////////////////////////////////////////////////////////////

// XTRtsp
//////////////////////////////////////////////////////////////////////////
// ����/ɾ��Դ
SESSIONSERVER_API int xtr_add_src(int srcno);
SESSIONSERVER_API int xtr_del_src(int srcno);

// ����SDP
SESSIONSERVER_API int xtr_set_sdp(int srcno, const char *sdp, int len);

// ���÷��Ͷ˿�
SESSIONSERVER_API int xtr_set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid);


SESSIONSERVER_API void regist_response_callback(regist_response_callback_t func);

typedef void (SESSIONSERVER_STDCALL *xt_query_real_ch_by_request_ch_callback_t)(const int request_ch,long stream_type,int* real_ch);
SESSIONSERVER_API void xt_regist_query_real_ch_request_callback_func(xt_query_real_ch_by_request_ch_callback_t cb);

//////////////////////////////////////////////////////////////////////////
#endif   //_H_XTSESSIONSERVER_H
