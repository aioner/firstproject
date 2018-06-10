#ifndef _XT_UDP_SESSION_CLIENT_H_INCLUDED
#define _XT_UDP_SESSION_CLIENT_H_INCLUDED


#if defined(WIN32) || defined(WIN64)
#ifdef UDP_SESSION_CLIENT_EXPORTS
#define UDP_SESSION_CLIENT_API __declspec(dllexport)
#else
#define UDP_SESSION_CLIENT_API __declspec(dllimport)
#endif
#else
#define UDP_SESSION_CLIENT_API __attribute__((visibility("default")))
#endif

#ifdef _WIN32
#define UDP_SESSION_CLIENT_CALLBACK __stdcall
#else
#define UDP_SESSION_CLIENT_CALLBACK
#endif

#include<stdint.h>

#pragma pack(push, 8)

#define XT_UDP_SESSION_SEND_DATA_MAX  				1024
#define XT_UDP_SESSION_RECV_DATA_MAX  				1024
#define XT_UDP_SESSION_CLIENT_SDP_LEN		  		2048

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void *udp_service_handle;
    typedef void *udp_session_handle;

    typedef void (UDP_SESSION_CLIENT_CALLBACK *response_done_callback_t)(void *ctx, uint32_t);

    typedef void (UDP_SESSION_CLIENT_CALLBACK *udp_session_regist_call_back_t)(const char *ip, uint16_t port, const uint8_t *data, uint32_t length);

    typedef struct _xt_get_sdp_request_t
    {
        uint16_t channel;
    } xt_get_sdp_request_t;

    typedef struct _xt_get_sdp_response_t
    {
        uint8_t sdp[XT_UDP_SESSION_CLIENT_SDP_LEN];
        uint16_t length;
    } xt_get_sdp_response_t;

    typedef struct _xt_play_request_t
    {
        uint16_t channel;
        uint8_t mode;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t rtp_port;
        uint16_t rtcp_port;
    } xt_play_request_t;

    typedef struct _xt_play_request_v1_t
    {
        uint16_t channel;
        uint8_t code;
        uint8_t mode;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t rtp_port;
        uint16_t rtcp_port;
    } xt_play_request_v1_t;

    typedef struct _xt_play_response_t
    {
        uint32_t demux_id;
        uint16_t rtp_port;
        uint16_t rtcp_port;
        uint8_t mode;
        uint8_t demux_flag;
    } xt_play_response_t;

    typedef struct _xt_stop_request_t
    {
        uint16_t channel;
        uint8_t mode;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t rtp_port;
        uint16_t rtcp_port;
    } xt_stop_request_t;

    typedef struct _xt_stop_response_t
    {} xt_stop_response_t;

    typedef xt_play_request_t xt_get_sdp_and_play_request_t;
    typedef xt_play_request_v1_t xt_get_sdp_and_play_request_v1_t;

    typedef struct _xt_get_sdp_and_play_response_t
    {
        uint32_t demux_id;
        uint16_t rtp_port;
        uint16_t rtcp_port;
        uint8_t mode;
        uint8_t demux_flag;
        uint16_t length;
        uint8_t sdp[2048];
    } xt_get_sdp_and_play_response_t;

    typedef struct _xt_send_data_request_t
    {
        uint16_t channel;
        uint16_t length;
        uint8_t content[XT_UDP_SESSION_SEND_DATA_MAX];
    } xt_send_data_request_t;

    typedef struct _xt_send_data_response_t
    {
        uint16_t length;
        uint8_t data[XT_UDP_SESSION_RECV_DATA_MAX];
    } xt_send_data_response_t;

    UDP_SESSION_CLIENT_API udp_service_handle xt_udp_client_create_service();
    UDP_SESSION_CLIENT_API void xt_udp_client_run_service(udp_service_handle service);
    UDP_SESSION_CLIENT_API void xt_udp_client_destroy_service(udp_service_handle service);

    UDP_SESSION_CLIENT_API udp_session_handle xt_udp_client_session_init(const char *ip, uint16_t port, udp_service_handle service);
    UDP_SESSION_CLIENT_API void xt_udp_client_session_term(udp_session_handle session);

    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_get_sdp(udp_session_handle session, const char *ip, uint16_t port, xt_get_sdp_request_t *request, xt_get_sdp_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_play(udp_session_handle session, const char *ip, uint16_t port, xt_play_request_t *request, xt_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_stop(udp_session_handle session, const char *ip, uint16_t port, xt_stop_request_t *request, xt_stop_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    UDP_SESSION_CLIENT_API void xt_udp_client_session_heartbit(udp_session_handle session, const char *ip, uint16_t port, uint16_t channel);
    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_fast_play(udp_session_handle session, const char *ip, uint16_t port, xt_get_sdp_and_play_request_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_fast_play_v1(udp_session_handle session, const char *ip, uint16_t port, xt_get_sdp_and_play_request_v1_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);


    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_send_data(udp_session_handle session, const char *ip, uint16_t port, xt_send_data_request_t *request, xt_send_data_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);
    
    UDP_SESSION_CLIENT_API int32_t xt_udp_client_session_regist_callback(udp_session_handle session, udp_session_regist_call_back_t cb);

    UDP_SESSION_CLIENT_API void xt_udp_client_session_heartbit2(udp_session_handle session, const char *ip, uint16_t port, uint32_t sink_ip, uint16_t sink_port, uint16_t channel);
#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif //_XT_UDP_SESSION_SERVER_H_INCLUDED
