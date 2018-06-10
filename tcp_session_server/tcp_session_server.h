#ifndef _TCP_SESSION_SERVER_API_H_INCLUDED
#define _TCP_SESSION_SERVER_API_H_INCLUDED

#ifdef _WIN32
	#ifdef TCP_SESSION_SERVER_EXPORTS
		#define TCP_SESSION_SERVER_API __declspec(dllexport)
	#else
		#define TCP_SESSION_SERVER_API __declspec(dllimport)
	#endif

	#define TCP_SESSION_SERVER_STDCALL __stdcall
#else
	#ifdef TCP_SESSION_SERVER_EXPORTS
		#define TCP_SESSION_SERVER_API __attribute__((visibility("default")))
	#else
		#define TCP_SESSION_SERVER_API
	#endif
	
	#define TCP_SESSION_SERVER_STDCALL
#endif

#include<stdint.h>
#include "xt_log_def.h"

#define  TCP_SESSION_SERVER_IP_LEN			32

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (TCP_SESSION_SERVER_STDCALL *close_connect_callback)(void* pContext,	//���ӹ�����
																char   *remote_ip,			//Զ��IP
																uint16_t remote_port,			//Զ�˶˿�(��������ʱ���ɵ�)
																uint32_t nReason);		//ԭ����


	typedef void (TCP_SESSION_SERVER_STDCALL *stop_rtp_link_callback)(void* pContext,					//���ӹ�����	
															char   *local_ip,					//����IP										 
															char   *remote_ip,					//Զ��IP
															uint16_t remote_port,				//Զ�˶˿�(��������ʱ���ɵ�)			
															uint16_t port,
															uint16_t channel,
															uint16_t mode,
															uint32_t multiplex,						//�Ƿ�˿ڸ���
															uint32_t multiplex_id);				//����ID


	typedef void (TCP_SESSION_SERVER_STDCALL *start_rtp_link_callback)(void* pContext,		//���ӹ�����
																char   *local_ip,				//����IP										 
																char   *remote_ip,				//Զ��IP
																uint16_t remote_port,				//Զ�˶˿�(��������ʱ���ɵ�)			
																uint16_t port,
																uint16_t channel,
																uint16_t mode,
																uint32_t ssrc,					//RTP�����շ�SSRCֵ
																uint32_t multiplex,				//�Ƿ�˿ڸ���
																uint32_t multiplex_id);			//����ID



	typedef void (TCP_SESSION_SERVER_STDCALL *get_sdp_callback)(void* pContext,				//���ӹ�����
															char   *remote_ip,
															uint16_t remote_port,
															uint16_t channel,		//ͨ����
															char *sdp_info,				//��ȡϵͳͷ������Ϣ
															uint32_t *data_type,
															uint32_t *data_size,
															uint32_t *sdp_len,
															uint32_t *multiplex,
															uint32_t *multiplex_id,
															uint32_t *res);	

	typedef void (TCP_SESSION_SERVER_STDCALL *get_mul_info_callback)(void *pContext,
															char*multi_info,	
															uint16_t *multi_len	);

	typedef struct _tcp_session_server_param_t
	{
			close_connect_callback		close_connect_cb;
			stop_rtp_link_callback		stop_link_cb;
			start_rtp_link_callback		start_link_cb;
			get_mul_info_callback		get_mul_cb;
			get_sdp_callback			get_sdp_cb;
			xt_print_cb					print_cb;


			char					ip[TCP_SESSION_SERVER_IP_LEN];					//����IP
			uint16_t				listen_port;								//�����˿�
			char					multicast_ip[TCP_SESSION_SERVER_IP_LEN];		//�鲥��ʼ��ַ
			uint16_t				multicast_port;								//�鲥��ʼ�˿�
	} tcp_session_server_param_t;

typedef int tcp_session_server_status_t;
typedef void *tcp_server_handle;

TCP_SESSION_SERVER_API  void tcp_session_server_start(tcp_session_server_param_t *pServer_Param);

TCP_SESSION_SERVER_API void tcp_session_server_stop();

#ifdef __cplusplus
};
#endif




#endif



