#ifndef _H_RTSP_SERVER_H
#define _H_RTSP_SERVER_H

#include "xt_log_def.h"

#ifdef _WIN32
	#ifdef  RTSPSERVER_EXPORTS
		#define RTSPSERVER_API __declspec(dllexport)
	#else
		#define RTSPSERVER_API __declspec(dllimport)
	#endif

	#define RTSPSERVER_STDCALL __stdcall
#else
	#define RTSPSERVER_API __attribute__((visibility("default")))
	#define RTSPSERVER_STDCALL
#endif

// 建立转发
typedef int (RTSPSERVER_STDCALL *add_send_cb)(void * hSession,				//session
									int sessionno,    //sessionid
									 int trackid,  				//track id 
									const char *pstrRAddr, 		//IP
									unsigned short nRPort,		//port
									bool multiplex,				//multiplex
									unsigned int multiplexID);	//multiplexID

// 删除转发
typedef int (RTSPSERVER_STDCALL *del_send_cb)(void * hSession,				//session
									int sessionno, //sessionid
									 int trackid,  				//track id
									const char *pstrRAddr, 		//IP
									unsigned short nRPort,		//port
									bool multiplex,				//multiplex
									unsigned int multiplexID);	//multiplexID

typedef int (RTSPSERVER_STDCALL *rtsp_play_cb)(void *hSession, int sessionno, int trackid, double npt, float scale, unsigned int *rtp_pkt_seq, unsigned int *rtp_pkt_timestamp);
typedef int (RTSPSERVER_STDCALL *rtsp_pause_cb)(void *hSession, int sessionno, int trackid);
// 设置回调
RTSPSERVER_API int set_add_send_cb(add_send_cb func);
RTSPSERVER_API int set_del_send_cb(del_send_cb func);
RTSPSERVER_API int set_rtsp_play_cb(rtsp_play_cb func);
RTSPSERVER_API int set_rtsp_pause_cb(rtsp_pause_cb func);

// 启动/停止rtsp server
RTSPSERVER_API int xt_start_server(const char ip[32], unsigned short listen_port, unsigned int max_session, xt_print_cb print_cb);
RTSPSERVER_API int xt_stop_server();

// 创建/删除src
RTSPSERVER_API int xt_add_src(int sessionno);
RTSPSERVER_API int xt_del_src(int sessionno);

// 设置SDP
RTSPSERVER_API int xt_set_sdp(int sessionno, const char *sdp, int len);

// 设置track发送端口
RTSPSERVER_API int xt_set_snd_port(int sessionno, int trackid, unsigned short port, bool demux,unsigned int demuxid);

// 设置RTSP心跳时间
RTSPSERVER_API int xt_set_rstp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);

typedef void (RTSPSERVER_STDCALL *xt_rtsp_query_real_ch_by_request_ch_callback_t)(const int request_ch,const long stream_type,int* real_ch);
RTSPSERVER_API void xt_register_rtsp_query_real_ch_by_request_ch_callback(xt_rtsp_query_real_ch_by_request_ch_callback_t cb);

#endif //end #ifndef _H_RTSP_SERVER_H
