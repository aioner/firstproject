#ifdef _USE_XT_RTSP_SESSION_SERVER

#ifdef _OS_WINDOWS
#pragma once
#endif
#ifndef _XTRTSP_H__
#define _XTRTSP_H__
#include <string>
#include "h_xtsessionserver.h"

using namespace std;

class XTRtsp
{
private:
	XTRtsp(void);
	~XTRtsp(void);

	static XTRtsp self;

public:
	static XTRtsp* instance(){return &self;}

	// 初始化
	int init(const string& ip, unsigned short listen_port, unsigned int max_session, xt_print_cb func);
    int set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval);
	
	// 反初始化
	int uninit();

	// 建立转发
	static int SESSIONSERVER_STDCALL add_send_proc(void *hsession,
										int srcno,					//srcno
										int trackid,  				//track id 
										const char *pstrRAddr, 		//IP
										unsigned short nRPort,		//port
										bool multiplex,				//multiplex
										unsigned int multiplexID);	//multiplexID

	// 删除转发
	static int SESSIONSERVER_STDCALL del_send_proc(void *hsession,
										int srcno,					//srcno
										int trackid,  				//track id
										const char *pstrRAddr, 		//IP
										unsigned short nRPort,		//port
										bool multiplex,				//multiplex
										unsigned int multiplexID);	//multiplexID

	static int SESSIONSERVER_STDCALL rtsp_play_cb_func(void *hSession, int sessionno, int trackid, 
		double npt, float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp);
	static int SESSIONSERVER_STDCALL rtsp_pause_cb_func(void *hSession, int sessionno, int trackid);
	// 增加/删除源
	int add_src(int srcno);
	int del_src(int srcno);

	// 设置SDP
	int set_sdp(int srcno, const char *sdp, int len);

	// 设置发送端口
	int set_snd_port(int srcno, int trackid, unsigned short port, bool demux,unsigned int demuxid);
	int set_play_cb(rtsp_play_cb func);
	int set_pause_cb(rtsp_pause_cb func);
};
#endif //#ifndef _XTRTSP_H__
#endif //_USE_XT_RTSP_SESSION_SERVER
