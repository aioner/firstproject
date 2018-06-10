#ifndef _TCP_SESSION_CLIENT_API_H_INCLUDED
#define _TCP_SESSION_CLIENT_API_H_INCLUDED

#ifdef _WIN32
    #ifdef TCP_SESSION_CLIENT_EXPORTS
        #define TCP_SESSION_CLIENT_API __declspec(dllexport)
    #else
        #define TCP_SESSION_CLIENT_API __declspec(dllimport)
    #endif

    #define TCP_SESSION_CLIENT_STDCALL __stdcall
#else
    #ifdef TCP_SESSION_CLIENT_EXPORTS
        #define TCP_SESSION_CLIENT_API __attribute__((visibility("default")))
    #else
        #define TCP_SESSION_CLIENT_API 
    #endif

    #define TCP_SESSION_CLIENT_STDCALL
#endif

#include<stdint.h>

#define TCP_SESSION_CLIENT_IP_LEN                               32
#define TCP_SESSION_CLIENT_SDP_LEN                              2048
#define TCP_SESSION_CLIENT_STATUS_OK                            0
#define MODLE_LOG_NAME "tcp_session_client"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum 
    {
        tcp_sc_log_info=0,
        tcp_sc_log_warn,
        tcp_sc_log_error,
        tcp_sc_log_debug
    }tcp_sc_level_t;
    typedef void (TCP_SESSION_CLIENT_STDCALL * tcp_session_client_log_cb_t)(const char* log_name,const tcp_sc_level_t log_level,const char* log_ctx);
    typedef void *tcp_session_client_service_t;
    typedef void *tcp_session_client_handle_t;
    typedef int tcp_session_client_status_t;

    typedef void (TCP_SESSION_CLIENT_STDCALL *tcp_session_client_done_callback_t)(void *ctx, tcp_session_client_status_t stat);
    typedef tcp_session_client_done_callback_t tcp_session_client_close_callback_t;

    typedef struct _tcp_session_client_connect_callbacks_t
    {
        tcp_session_client_close_callback_t close_callback;
        void *ctx;
    } tcp_session_client_connect_callbacks_t;

    typedef struct _tcp_session_client_login_request_t
    {
        char reserve;
    } tcp_session_client_login_request_t;

    typedef struct _tcp_session_client_login_response_t
    {
        uint16_t multicast_port;
        uint16_t rtp_start_port;
        char  multicast_addr[TCP_SESSION_CLIENT_IP_LEN];

    } tcp_session_client_login_response_t;

    typedef struct _tcp_session_client_play_request_t
    {
        uint16_t audio_rtp_port;
        uint16_t rtp_port;
        uint16_t channel;
        uint16_t mode;
        uint32_t ssrc;
    } tcp_session_client_play_request_t;

    typedef struct _tcp_session_client_play_response_t
    {
        uint32_t data_type;

        uint16_t stop_flag;
        uint16_t rtp_port;          //非复用方式下的服务端的rtp发送端口

        uint32_t length;
        char sdp[TCP_SESSION_CLIENT_SDP_LEN];
    } tcp_session_client_play_response_t;

    typedef struct _tcp_session_client_demux_play_request_t
    {
        uint16_t audio_rtp_port;
        uint16_t rtp_port;
        uint16_t channel;
        uint16_t mode;
        uint32_t ssrc;
        uint32_t multiplex;
        uint32_t multiplexID;
    } tcp_session_client_demux_play_request_t;

    typedef struct _tcp_session_client_demux_play_response_t
    {
        uint32_t data_type;

        uint16_t stop_flag;
        uint16_t rtp_port;          //非复用方式下的服务端的rtp发送端口

        uint32_t multiplex;
        uint32_t multiplexID;

        uint32_t length;
        char sdp[TCP_SESSION_CLIENT_SDP_LEN];
    } tcp_session_client_demux_play_response_t;

    typedef struct _tcp_session_client_stop_request_t
    {
        uint16_t rtp_port;
        uint16_t channel;
        uint16_t mode;
    } tcp_session_client_stop_request_t; 

    typedef tcp_session_client_stop_request_t tcp_session_client_stop_response_t;

    typedef tcp_session_client_demux_play_request_t tcp_session_client_demux_stop_request_t;
    typedef tcp_session_client_stop_response_t tcp_session_client_demux_stop_response_t;

    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_create_service(tcp_session_client_service_t *pservice);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_destroy_service(tcp_session_client_service_t service);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_service_stop(tcp_session_client_service_t service);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_service_run(tcp_session_client_service_t service);

    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_new(const char *local_ip, uint16_t local_port, tcp_session_client_service_t service, const tcp_session_client_connect_callbacks_t *callbacks, tcp_session_client_handle_t *phandle);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_native(int native_socket, tcp_session_client_service_t service, const tcp_session_client_connect_callbacks_t *callbacks, tcp_session_client_handle_t *phandle);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_connect(tcp_session_client_handle_t handle, const char *remote_ip, uint16_t remote_port, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_is_connected(tcp_session_client_handle_t handle);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_close(tcp_session_client_handle_t handle);

    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_login(tcp_session_client_handle_t handle, const tcp_session_client_login_request_t *request, tcp_session_client_login_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout);

    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_play(tcp_session_client_handle_t handle, const tcp_session_client_play_request_t *request, tcp_session_client_play_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_stop(tcp_session_client_handle_t handle, const tcp_session_client_stop_request_t *request, tcp_session_client_stop_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout);

    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_demux_play(tcp_session_client_handle_t handle, const tcp_session_client_demux_play_request_t *request, tcp_session_client_demux_play_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout);
    TCP_SESSION_CLIENT_API tcp_session_client_status_t tcp_session_client_demux_stop(tcp_session_client_handle_t handle, const tcp_session_client_demux_stop_request_t *request, tcp_session_client_demux_stop_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout);

    TCP_SESSION_CLIENT_API tcp_session_client_status_t xt_tcp_client_register_log( tcp_session_client_log_cb_t log_cb);
#ifdef __cplusplus
}
#endif

#endif //_TCP_SESSION_CLIENT_API_H_INCLUDED
