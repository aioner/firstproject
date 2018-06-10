#ifndef _XT_RTSP_CLIENT_H_INCLUDED
#define _XT_RTSP_CLIENT_H_INCLUDED

#include "xt_rtsp_client_types.h"

#define XT_RTSP_CLIENT_LOG_MODULE "xt_rtsp_client"

#pragma pack(push, 8)

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum 
    {
        rtsp_c_log_info=0,
        rtsp_c_log_warn,
        rtsp_c_log_error,
        rtsp_c_log_debug
    }rtsp_client_level_t;

    typedef void (XT_RTSP_CLIENT_STDCALL *xt_rtsp_client_log_cb_t)(const char* log_name,const rtsp_client_level_t log_level,const char* log_ctx);
    typedef void (XT_RTSP_CLIENT_STDCALL *xt_rtsp_client_callback_t)(int32_t stat, void *ctx);

    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_init();
    XT_RTSP_CLIENT_API void xt_rtsp_client_term();

    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_create_client(const rtsp_client_config_t *config, rtsp_client_handle_t *pclient);
    XT_RTSP_CLIENT_API void xt_rtsp_destroy_client(rtsp_client_handle_t client);

    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_create_connection(rtsp_client_handle_t client, const char *uri, const char *local_ip, uint16_t local_port, const rtsp_client_connection_config_t *config, int8_t *connected, rtsp_connection_handle_t *pconnection);
    XT_RTSP_CLIENT_API void xt_rtsp_client_destroy_connection(rtsp_connection_handle_t connection);

    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_authenticate(rtsp_connection_handle_t connection, const char *username, const char *pwd);
    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_connect(rtsp_connection_handle_t connection, const rtsp_client_connect_request_t *request, rtsp_client_connect_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
    XT_RTSP_CLIENT_API void xt_rtsp_client_disconnect(rtsp_connection_handle_t connection);
    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_get_addr(rtsp_connection_handle_t connection, char ip[RTSP_CLIENT_IP_LEN], uint16_t *port);
    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_describe(rtsp_connection_handle_t connection, const rtsp_client_describe_request_t *request, rtsp_client_describe_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);

    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_create_session(rtsp_connection_handle_t connection, const rtsp_client_session_config_t *config, rtsp_session_handle_t *psession);
    XT_RTSP_CLIENT_API void xt_rtsp_client_destroy_session(rtsp_session_handle_t session);

    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_setup(rtsp_session_handle_t session, const rtsp_client_setup_request_t *request, rtsp_client_setup_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_play(rtsp_session_handle_t session, const rtsp_client_play_request_t *request, rtsp_client_play_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_pause(rtsp_session_handle_t session, const rtsp_client_pause_request_t *request, rtsp_client_pause_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);
    XT_RTSP_CLIENT_API xt_rtsp_client_status_t xt_rtsp_client_teardown(rtsp_session_handle_t session, const rtsp_client_teardown_request_t *request, rtsp_client_teardown_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout);

    XT_RTSP_CLIENT_API void xt_rtsp_client_register_log(xt_rtsp_client_log_cb_t log_cb);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif //_XT_RTSP_CLIENT_H_INCLUDED
