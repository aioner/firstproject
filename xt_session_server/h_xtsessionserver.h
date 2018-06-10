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
	PT_CTRL_MSG   = 3,    //TCP传输控制
	PT_CTRL_RTSP  = 4     //RTSP传输控制
};

enum XT_MSG_TYPE
{
	MSG_STARTPLAY        = 1,		//启动点播消息
	MSG_STOPPLAY         = 2,		//停止点播消息
	MSG_CLOSEMSG         = 3,		//断开握手连接消息
	MSG_GETSNDINFO       = 4,		//获得发送端信息
	MSG_TRANS_CTRL_PLAY  = 5,		//定位播放
	MSG_TRANS_CTRL_PAUSE = 6,		//暂停

	RTSP_STARTPLAY      = 101,		//启动点播消息
	RTSP_STOPPLAY       = 102,		//停止点播消息
	RTSP_GETSNDINFO     = 103,		//获得发送端信息
	RTSP_PLAY           = 104,		//定位播放
	RTSP_PAUSE          = 105,		//暂停

	STD_ADD_SEND		= 201,		//添加转发
	STD_DEL_SEND		= 202		//删除转发
};

struct STR_MsgData{
	XT_MSG_TYPE			ctrl;				//控制编号
	void*				pMsgContext;		//连接管理编号
	char				LAddr[SESSION_SERVER_IP_LEN];				//本地IP
	char				Addr[SESSION_SERVER_IP_LEN];				//远端IP
	unsigned short		nMsgPort;			//远端端口(建立链接时生成的)
	
	uint16_t nRecvPort;			//接收数据用端口					
	uint16_t nDataChannelID;		//要接收的数据通道号
	uint16_t nMode;				//接收数据模式(0,1: 单播，2：组播)	//在RTP的点播操作中此参数表示RctpSsrc

	unsigned int		nSSRC;				//RTP建立链接时
	bool				multiplex;			//是否端口复用
	unsigned int		multiplexID;		//复用ID

};

struct STR_MsgCtrlData
{
	XT_MSG_TYPE			ctrl;			    // 控制编号
	void*				pMsgContext;		// 连接句柄
	uint32_t          data_chid;          // 操作通道
	double            npt;               // 客户端seek之后的npt时间  
	float             scale;             // 客户端seek之后的播放倍率
	uint32_t          *rtp_pkt_timestamp;   // seek之后的rtp包的时戳
};

struct Msg_Rtsp 
{
	XT_MSG_TYPE			ctrl;			//控制编号	
	void				*session;		//连接句柄
	int					srcno;			//srcno
	int					trackid;  		//track id 
	char				pstrRAddr[SESSION_SERVER_IP_LEN]; 		//IP
	unsigned short		nRPort;		 	//port
	bool				multiplex;		//是否端口复用
	unsigned int		multiplexID;	//复用ID
};

struct std_send_task
{
	XT_MSG_TYPE			ctrl;			//控制编号	
	void				*session;		//连接句柄
	int					srcno;			//srcno
	int					trackid;  		//track id 
	char				ip[SESSION_SERVER_IP_LEN]; 		//IP
	unsigned short		port;		 	//port
	bool				demux;		//是否端口复用
	unsigned int		demux_id;	//复用ID
};

struct Msg_Rtsp_Ctrl
{
	XT_MSG_TYPE			ctrl;			//控制编号	
	void				*session;		//连接句柄
	int					srcno;			//srcno
	int					trackid;  		//track id 
	double            npt;
	float             scale;
	uint32_t          *rtp_pkt_seq;
	uint32_t          *rtp_pkt_timestamp;
};

typedef int (SESSIONSERVER_STDCALL *session_msg_cb)(PROTOL_TYPE type, void* msg); 

// 设置会话回调
SESSIONSERVER_API int xt_set_sessionmsg_cb(session_msg_cb func);

//流化控制
////////////////////////////////////////////////////////////////////////////////////////////////////////
//RTSP
typedef int (SESSIONSERVER_STDCALL *rtsp_play_cb)(void *hSession, int sessionno, int trackid, double npt, 
												  float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);
typedef int (SESSIONSERVER_STDCALL *rtsp_pause_cb)(void *hSession, int sessionno, int trackid);
SESSIONSERVER_API int xt_set_rtsp_play_cb(rtsp_play_cb func);
SESSIONSERVER_API int xt_set_rtsp_pause_cb(rtsp_pause_cb func);

//TCP
typedef int (SESSIONSERVER_STDCALL *tcp_play_cb_type)(void *p_context,   // 连接管理编号
													  uint32_t data_chid,           // 操作通道
													  double npt,                  // 客户端seek之后的npt时间  
													  float scale,                 // 客户端seek之后的播放倍率
													  uint32_t *rtp_pkt_timestamp   // seek之后的rtp包的时戳
													  );
typedef int( SESSIONSERVER_STDCALL *tcp_pause_cb_type)(void *p_context, // 连接管理编号
													   uint32_t data_chid          // 操作通道
													   );
//反向注册获取结果回调
typedef void(SESSIONSERVER_STDCALL *regist_response_callback_t)(const char *ip, uint16_t port, uint32_t code);

SESSIONSERVER_API int xt_set_tcp_play_cb(tcp_play_cb_type func);
SESSIONSERVER_API int xt_set_tcp_pause_cb(tcp_pause_cb_type func);
////////////////////////////////////////////////////////////////////////////////////////////////////////

// 初始化私有库
SESSIONSERVER_API int xt_init_msg(const char *ip,					// 本地IP
								  unsigned short msg_listen_port,	// 服务监听端口 msg侦听端口
								  unsigned short udp_listen_port,	//udp侦听端口
								  unsigned short send_start_port,	// 发送起始端口
								  const char *mul_start_addr,		// 组播发送起始地址
								  unsigned short mul_port,			// 组播发送端口
								  xt_print_cb func);				// 日志输出

// 初始化rtsp库(listen_port 服务监听端口)
SESSIONSERVER_API int xt_init_rtsp(const char *ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func);

SESSIONSERVER_API int xt_set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

// 反初始化
SESSIONSERVER_API int xt_uninit_msg();
SESSIONSERVER_API int xt_uninit_rtsp();


SESSIONSERVER_API int xtr_regist(const char *regist_ids,const char *server_ip, unsigned short server_port,uint32_t millisec);



SESSIONSERVER_API int xtr_stop_regist( const char* server_ip, unsigned short server_port,uint32_t millisec );

// XTMsg
//////////////////////////////////////////////////////////////////////////
// 设置系统头
SESSIONSERVER_API int xt_set_key_data(unsigned long nDataChID, char *pHeadData, long nHeadSize, long nDataType);

// 设置复用参数
SESSIONSERVER_API int  xtm_set_snd_port(unsigned long nDataChID, unsigned short sndport, bool multiplex,unsigned int multid);

// 清除系统头
SESSIONSERVER_API int  xt_clear_key_data(unsigned long nDataChID);

//添加会话客户端
SESSIONSERVER_API int xt_msg_add_socket_client(void *sock);
//////////////////////////////////////////////////////////////////////////

// XTRtsp
//////////////////////////////////////////////////////////////////////////
// 增加/删除源
SESSIONSERVER_API int xtr_add_src(int srcno);
SESSIONSERVER_API int xtr_del_src(int srcno);

// 设置SDP
SESSIONSERVER_API int xtr_set_sdp(int srcno, const char *sdp, int len);

// 设置发送端口
SESSIONSERVER_API int xtr_set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid);


SESSIONSERVER_API void regist_response_callback(regist_response_callback_t func);

typedef void (SESSIONSERVER_STDCALL *xt_query_real_ch_by_request_ch_callback_t)(const int request_ch,long stream_type,int* real_ch);
SESSIONSERVER_API void xt_regist_query_real_ch_request_callback_func(xt_query_real_ch_by_request_ch_callback_t cb);

//////////////////////////////////////////////////////////////////////////
#endif   //_H_XTSESSIONSERVER_H
