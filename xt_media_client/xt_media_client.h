#ifndef _XT_MEDIA_CLIENT_H_INCLUDED
#define _XT_MEDIA_CLIENT_H_INCLUDED

#include "xt_media_client_types.h"

#ifdef COMPILE_XP_VER 
#define _WIN32_WINNT 0x0501     // 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#pragma pack(push, 8)

#ifdef __cplusplus
extern "C"
{
#endif
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_init(const xt_media_client_cfg_t *config);
    XT_MEDIA_CLIENT_API void xt_media_client_term();

    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_rtsp_link(const char *uri, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle,const char* localip="0.0.0.0");
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_priv_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_socket_link(void *sock, uint32_t channel, uint32_t link_type, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_custom_link(const xt_custom_session_callback_t *session, void *ctx, xt_media_client_link_mode mode, uint32_t media_type, bool demux, xt_media_link_handle_t *phandle);

    XT_MEDIA_CLIENT_API void xt_media_client_close_link(xt_media_link_handle_t handle);

    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_query_prof(xt_media_link_handle_t handle, xt_rtp_prof_info_t *prof);

    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_get_header(xt_media_link_handle_t handle, uint8_t *data, uint32_t *length);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_play(xt_media_link_handle_t handle, xt_media_client_frame_callback_t cb, void *ctx);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_pause(xt_media_link_handle_t handle);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_seek(xt_media_link_handle_t handle, double npt, float scale, uint32_t *seq, uint32_t *timestamp);

    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_create_recv(int track_num, bool demux, xt_media_link_handle_t *phandle);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_create_recv_2(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports,xt_media_link_handle_t *phandle);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_setsdp(xt_media_link_handle_t handle, const char *sdp, uint32_t len_sdp);
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_getport(xt_media_link_handle_t handle, xt_sink_info_t *sinks, int &num);

	XT_MEDIA_CLIENT_API int xt_media_client_setresend(int resend,int wait_resend, int max_resend, int vga_order);

    //请求I帧 add by songlei 20150505
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_request_iframe(xt_media_link_handle_t handle);

    XT_MEDIA_CLIENT_API void  xt_media_client_regist_callback(regist_call_back_t func);

	/*************************************
	函数功能：设置SSRC报告
	参数说明：
			handle，设备连接句柄
			cb，      回调函数
			ctx，     用户上下文
	返回值：xt_media_client_status_t */
    XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_register_rtcp_callback(xt_media_link_handle_t handle, xt_media_client_rtcp_report_callback_t cb, void *ctx);
    
	XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_priv_multicast_link(const char *ip, uint16_t port, uint32_t channel, uint32_t link_type, uint32_t media_type, const char *multicast_ip, uint16_t multicast_port, xt_media_link_handle_t *phandle);
	XT_MEDIA_CLIENT_API xt_media_client_status_t xt_media_client_rtsp_multicat_link(const char *uri, uint32_t media_type, bool demux,  const char *multicast_ip, uint16_t multicast_port,xt_media_link_handle_t *phandle);
#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif //_XT_MEDIA_CLIENT_H_INCLUDED
