#include "xt_rtsp_client.h"
#include "rtsp_global_mgr.h"
#include "rv_rtsp_client_adapter.h"
#include "RvRtspClientConnection.h"
#include <string.h>
#include <stdarg.h>

/*
#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
static void LOG_PUBLIC_STDCALL win_default_log_cb(void *ctx, xt_log_level_t level, const char *file, const char *function, unsigned int line, const char *log)
{
    static const char *log_level_name[] = { "warning", "debug", "info", "error", "fatal" };

    char buf[1024];
    //snprintf(buf, sizeof(buf), "[%s][%s][%d][%s@%u]:%s\n", log_level_name[level], XT_RTSP_CLIENT_LOG_MODULE, ::GetTickCount(), function, line, log);
    ::sprintf_s(buf,sizeof(buf), "[%s][%s][%d][%s@%u]:%s\n", log_level_name[level], XT_RTSP_CLIENT_LOG_MODULE, ::GetTickCount(), function, line, log);
    ::OutputDebugStringA(buf);
}
#else
#include <fstream>
static void LOG_PUBLIC_STDCALL default_log_cb(void *ctx, xt_log_level_t level, const char *file, const char *function, unsigned int line, const char *log)
{
    static const char *log_level_name[] = { "warning", "debug", "info", "error", "fatal" };

    struct timeval tv = { 0 };
    gettimeofday(&tv, NULL);

    uint32_t tick = tv.tv_sec * 1000UL + (uint32_t)(tv.tv_usec * 0.0000001);

    char buf[1024];
    int n = snprintf(buf, sizeof(buf), "[%s][%s][%d][%s@%u]:%s\n", log_level_name[level], XT_RTSP_CLIENT_LOG_MODULE, tick, function, line, log);
    static_cast<std::ofstream *>(ctx)->write(buf, n);
    static_cast<std::ofstream *>(ctx)->flush();
}
std::ofstream *g_log = NULL;
#endif
*/


using namespace xt_rtsp_client;
#define RTSP_CLIENT_REQUEST_TIMEOUT_MS 200

const char* XT_RTSP_CLIENT_LIB_INFO = "XT_Lib_Version: V_XT_Rtsp_Client_1.00.0118.0";

xt_rtsp_client_log_cb_t g_rtsp_log_cb = NULL;
void RTSP_C_LOG(const rtsp_client_level_t log_level,const char* fmt,...)
{
    if (g_rtsp_log_cb)
    {
        va_list args;
        va_start(args, fmt);
        char log_buf[2048]={0};
#ifdef _WIN32
        ::vsnprintf_s(log_buf, 2048, 2048-1, fmt, args);
#else
        ::vsnprintf(log_buf, 2048, fmt, args);
#endif
        va_end(args);
        g_rtsp_log_cb("rtsp_client",log_level,log_buf);
    }
}

extern "C"
{
    xt_rtsp_client_status_t xt_rtsp_client_init()
    {
        rtsp_global_mgr::new_instance();
        rtsp_global_mgr::instance()->init(RTSP_CLIENT_REQUEST_TIMEOUT_MS);

        return RTSP_CLIENT_STATUS_OK;
    }

    void xt_rtsp_client_term()
    {
        rtsp_global_mgr::delete_instance();
    }

    xt_rtsp_client_status_t xt_rtsp_create_client(const rtsp_client_config_t *config, rtsp_client_handle_t *pclient)
    {
        if ((NULL == config) || (NULL == pclient))
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        return rtsp_global_mgr::instance()->create_client(config, pclient);
    }

    void xt_rtsp_destroy_client(rtsp_client_handle_t client)
    {
        rtsp_global_mgr::instance()->destroy_client(client);
    }

    xt_rtsp_client_status_t xt_rtsp_client_create_connection(rtsp_client_handle_t client, const char *uri, const char *local_ip, uint16_t local_port, const rtsp_client_connection_config_t *config, int8_t *connected, rtsp_connection_handle_t *pconnection)
    {
        if ((NULL == config) || (NULL == pconnection))
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        xt_rtsp_client_status_t status = rtsp_global_mgr::instance()->create_connection(client, uri, local_ip, local_port, config, connected, pconnection);
        RTSP_C_LOG(rtsp_c_log_info, "xt_rtsp_client_create_connection. uri(%s) connection(%p)", uri, *pconnection);
        return	status;
    }

    void xt_rtsp_client_destroy_connection(rtsp_connection_handle_t connection)
    {
        if (NULL != connection)
        {
            RTSP_C_LOG(rtsp_c_log_info, "xt_rtsp_client_destroy_connection. connection(%p)", connection);
            rtsp_global_mgr::instance()->destroy_connection(connection);
        }
    }

    xt_rtsp_client_status_t xt_rtsp_client_authenticate(rtsp_connection_handle_t connection, const char *username, const char *pwd)
    {
        return RTSP_CLIENT_STATUS_OK;
    }

    xt_rtsp_client_status_t xt_rtsp_client_connect(rtsp_connection_handle_t connection, const rtsp_client_connect_request_t *request, rtsp_client_connect_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        if (!rtsp_global_mgr::instance()->all_op(&rtsp_global_mgr::async_connect, connection, request, response, done, ctx, timeout))
        {
            return RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    void xt_rtsp_client_disconnect(rtsp_connection_handle_t connection)
    {
        RTSP_C_LOG(rtsp_c_log_info, "xt_rtsp_client_disconnect. connection(%p)", connection);
        rtsp_global_mgr::instance()->disconnect(connection);
    }

    xt_rtsp_client_status_t xt_rtsp_client_get_addr(rtsp_connection_handle_t connection, char ip[RTSP_CLIENT_IP_LEN], uint16_t *port)
    {
        return rtsp_global_mgr::instance()->get_addr(connection, ip, port) ? RTSP_CLIENT_STATUS_OK : RTSP_CLIENT_STATUS_BAD_ADDR;
    }

    xt_rtsp_client_status_t xt_rtsp_client_describe(rtsp_connection_handle_t connection, const rtsp_client_describe_request_t *request, rtsp_client_describe_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        if (!rtsp_global_mgr::instance()->all_op(&rtsp_global_mgr::async_describe_request, connection, request, response, done, ctx,  timeout))
        {
            return RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    xt_rtsp_client_status_t xt_rtsp_client_create_session(rtsp_connection_handle_t connection, const rtsp_client_session_config_t *config, rtsp_session_handle_t *psession)
    {
        xt_rtsp_client_status_t status = rtsp_global_mgr::instance()->create_session(connection, config, psession);
        RTSP_C_LOG(rtsp_c_log_info, "xt_rtsp_client_create_session. connection(%p) session(%p)", connection, *psession);
        return status;
    }

    void xt_rtsp_client_destroy_session(rtsp_session_handle_t session)
    {
        RTSP_C_LOG(rtsp_c_log_info, "xt_rtsp_client_destroy_session. session(%p)", session);
        rtsp_global_mgr::instance()->destroy_session(session);
    }

    xt_rtsp_client_status_t xt_rtsp_client_setup(rtsp_session_handle_t session, const rtsp_client_setup_request_t *request, rtsp_client_setup_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        if (!rtsp_global_mgr::instance()->all_op(&rtsp_global_mgr::async_setup_request, session, request, response, done, ctx, timeout))
        {
            return RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    xt_rtsp_client_status_t xt_rtsp_client_play(rtsp_session_handle_t session, const rtsp_client_play_request_t *request, rtsp_client_play_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {

        if (!rtsp_global_mgr::instance()->all_op(&rtsp_global_mgr::async_play_request, session, request, response, done, ctx, timeout))
        {
            return RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    xt_rtsp_client_status_t xt_rtsp_client_pause(rtsp_session_handle_t session, const rtsp_client_pause_request_t *request, rtsp_client_pause_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        if (!rtsp_global_mgr::instance()->all_op(&rtsp_global_mgr::async_pause_request, session, request, response, done, ctx, timeout))
        {
            return RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    xt_rtsp_client_status_t xt_rtsp_client_teardown(rtsp_session_handle_t session, const rtsp_client_teardown_request_t *request, rtsp_client_teardown_response_t *response, xt_rtsp_client_callback_t done, void *ctx, uint32_t timeout)
    {
        RTSP_C_LOG(rtsp_c_log_info, "xt_rtsp_client_teardown. session(%p)", session);
        if (!rtsp_global_mgr::instance()->all_op(&rtsp_global_mgr::async_teardown_request, session, request, response, done, ctx, timeout))
        {
            return RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    void xt_rtsp_client_register_log(xt_rtsp_client_log_cb_t log_cb)
    {
        g_rtsp_log_cb = log_cb;
    }
};
