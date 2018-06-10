#ifndef _XT_UDP_SESSION_SERVER_H_INCLUDED
#define _XT_UDP_SESSION_SERVER_H_INCLUDED

#ifdef _OS_WINDOWS
#ifdef UDP_SESSION_SERVER_EXPORTS
#define UDP_SESSION_SERVER_API __declspec(dllexport)
#else
#define UDP_SESSION_SERVER_API __declspec(dllimport)
#endif
#else
#define UDP_SESSION_SERVER_API __attribute__((visibility("default")))
#endif

#ifdef _WIN32
#define UDP_SESSION_SERVER_CALLBACK __stdcall
#else
#define UDP_SESSION_SERVER_CALLBACK 
#endif

#ifdef _MSC_VER
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#pragma pack(push, 8)

#define XT_UDP_SESSION_SEND_DATA_MAX  1024

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void *udp_service_handle;
    typedef void *udp_session_handle;

    typedef int32_t (UDP_SESSION_SERVER_CALLBACK *query_sdp_callback_t)(uint32_t channel, uint8_t code, uint8_t *sdp, uint32_t *sdp_len);
    typedef int32_t (UDP_SESSION_SERVER_CALLBACK *add_sink_callback_t)(uint32_t channel, uint8_t code, uint16_t trackid, const char *ip, uint16_t *rtp_port, uint16_t *rtcp_port, uint8_t *demux_flag, uint32_t *demux_id);
    typedef int32_t (UDP_SESSION_SERVER_CALLBACK *del_sink_callback_t)(uint32_t channel, uint8_t code, uint16_t trackid, const char *ip, uint16_t rtp_port, uint16_t rtcp_port, uint8_t demux_flag, uint32_t demux_id);
    typedef int32_t (UDP_SESSION_SERVER_CALLBACK *send_data_callback_t)(const char *ip, uint16_t port, uint32_t channel, uint8_t code, const uint8_t *data, uint32_t length);

    typedef int32_t (UDP_SESSION_SERVER_CALLBACK *send_regist_calllback_t)(const char *ip, uint16_t port, uint32_t code);

    typedef void (UDP_SESSION_SERVER_CALLBACK *response_done_callback_t)(void *ctx, uint32_t);


    typedef struct _xt_udp_session_config_t
    {
        query_sdp_callback_t query_sdp_callback;
        add_sink_callback_t add_sink_callback;
        del_sink_callback_t del_sink_callback;
        send_data_callback_t send_data_callback;
        send_regist_calllback_t send_regist_data_callback;

        udp_service_handle service;

        uint32_t heartbit_check_millsec;
        uint32_t client_overtime_millsec;

        uint16_t port;
        char ip[32];
    } xt_udp_session_config_t;

    //×¢²áÏûÏ¢Ìå
    typedef struct _xt_send_regist_request_t
    {
        uint8_t ids[XT_UDP_SESSION_SEND_DATA_MAX];
        uint16_t length;
    } xt_send_regist_request_t;


    typedef struct _xt_send_regis_response_t
    {
        uint8_t status;
    } xt_send_regist_response_t;

#include "../../src/include/xt_log_def.h"
    UDP_SESSION_SERVER_API udp_service_handle xt_udp_create_service();
    UDP_SESSION_SERVER_API void xt_udp_run_service(udp_service_handle service);
    UDP_SESSION_SERVER_API void xt_udp_destroy_service(udp_service_handle service);

    UDP_SESSION_SERVER_API udp_session_handle xt_udp_session_init(const xt_udp_session_config_t *config, xt_print_cb print_cb);
    UDP_SESSION_SERVER_API void xt_udp_session_term(udp_session_handle session);


    UDP_SESSION_SERVER_API int32_t xt_udp_server_session_send_regist(udp_session_handle session, const char *ip, uint16_t port, const xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout);

    UDP_SESSION_SERVER_API int32_t xt_udp_server_session_send_stop_regist( udp_session_handle session, const char *ip, uint16_t port, xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout );


#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif //_XT_UDP_SESSION_SERVER_H_INCLUDED
