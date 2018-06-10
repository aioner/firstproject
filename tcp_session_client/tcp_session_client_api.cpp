#include "tcp_session_client_api.h"
#include "tcp_client.h"
#include <stdarg.h>
#include <stdio.h>

const char* XT_TCP_CLIENT_LIB_INFO = "XT_Lib_Version: V_XT_Tcp_Client_1.00.0118.0";
tcp_session_client_log_cb_t g_log_cb = NULL;
void TCP_SC_LOG(const tcp_sc_level_t log_level,const char* fmt,...)
{
    if (g_log_cb)
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
        g_log_cb("tcp_client",log_level,log_buf);
    }
}

tcp_session_client_status_t tcp_session_client_create_service(tcp_session_client_service_t *pservice)
{
    return xt_tcp_create_service(pservice);
}

tcp_session_client_status_t tcp_session_client_destroy_service(tcp_session_client_service_t service)
{
    return xt_tcp_destroy_service(service);
}

tcp_session_client_status_t tcp_session_client_service_stop(tcp_session_client_service_t service)
{
    return xt_tcp_service_stop(service);
}

tcp_session_client_status_t tcp_session_client_service_run(tcp_session_client_service_t service)
{
    return xt_tcp_service_run(service);
}

tcp_session_client_status_t tcp_session_client_new(const char *local_ip, uint16_t local_port, tcp_session_client_service_t service, const tcp_session_client_connect_callbacks_t *callbacks, tcp_session_client_handle_t *phandle)
{
    try
    {
        boost::intrusive_ptr<tcp_session::client_t> sp(new (std::nothrow) tcp_session::client_t(service, local_ip, local_port, callbacks->close_callback, callbacks->ctx));
        if (NULL == sp.get())
        {
            return -1;
        }

        boost::sp_adl_block::intrusive_ptr_add_ref(sp.get());
        *phandle = sp.get();
    }
    catch(...)
    {
        return -1;
    }

    return 0;
}

tcp_session_client_status_t tcp_session_client_native(int native_socket, tcp_session_client_service_t service, const tcp_session_client_connect_callbacks_t *callbacks, tcp_session_client_handle_t *phandle)
{
    try
    {
        boost::intrusive_ptr<tcp_session::client_t> sp(new (std::nothrow) tcp_session::client_t(service, native_socket, callbacks->close_callback, callbacks->ctx));
        if (NULL == sp.get())
        {
            return -1;
        }

        boost::sp_adl_block::intrusive_ptr_add_ref(sp.get());
        *phandle = sp.get();
    }
    catch(...)
    {
        return -1;
    }

    return 0;
}

tcp_session_client_status_t tcp_session_client_connect(tcp_session_client_handle_t handle, const char *remote_ip, uint16_t remote_port, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout)
{
    if (NULL != done)
    {
        return -1;
    }

    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    if (!impl->connect(remote_ip, remote_port, timeout))
    {
         TCP_SC_LOG(tcp_sc_log_info, "tcp_session_client_connect fail remote_ip[%s],remote_port[%d],timeout[%d]",remote_ip, remote_port, timeout);
        return -1;
    }

    TCP_SC_LOG(tcp_sc_log_info, "tcp_session_client_connect success remote_ip[%s],remote_port[%d],timeout[%d]",remote_ip, remote_port, timeout);
    return 0;
}

tcp_session_client_status_t tcp_session_client_is_connected(tcp_session_client_handle_t handle)
{
    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    if (!impl->is_connected())
    {
        return -1;
    }

    return 0;
}

tcp_session_client_status_t tcp_session_client_close(tcp_session_client_handle_t handle)
{
    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    impl->destruct();

    boost::intrusive_ptr<tcp_session::client_t> delete_sp(impl, false);

    TCP_SC_LOG(tcp_sc_log_info, "tcp_session_client_close");
    return 0;
}


tcp_session_client_status_t tcp_session_client_login(tcp_session_client_handle_t handle, const tcp_session_client_login_request_t *request, tcp_session_client_login_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout)
{
    if (NULL != done)
    {
        return -1;
    }

    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    tcp_session::login_msg_t login_info;
    login_info.op_id = TCP_SESSION_OPID_LOGIN;
    login_info.rtp_port = 0;
    login_info.channel = 0;
    login_info.mode = 0;

    tcp_session::multicast_info_t multicast_info;
    if (!impl->session_op(login_info, multicast_info, timeout))
    {
        TCP_SC_LOG(tcp_sc_log_error, "session_op impl op_id[%d],rtp_port[%d],channel[%d],mode[%d],multicast_info:port[%d],addr[%s],rtp_start_port[%d],timeout[%d]",
           login_info.op_id,login_info.rtp_port,login_info.channel,login_info.mode,multicast_info.port,multicast_info.addr,multicast_info.rtp_start_port,timeout);
        return -1;
    }

    memcpy(response->multicast_addr, multicast_info.addr, TCP_SESSION_CLIENT_IP_LEN);
    response->multicast_port = multicast_info.port;
    response->rtp_start_port = multicast_info.rtp_start_port;

    TCP_SC_LOG(tcp_sc_log_info, "multicast_addr[%s],multicast_port[%d],rtp_start_port[%d]",response->multicast_addr,response->multicast_port,response->rtp_start_port);

    return 0;
}

tcp_session_client_status_t tcp_session_client_play(tcp_session_client_handle_t handle, const tcp_session_client_play_request_t *request, tcp_session_client_play_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout)
{
    if (NULL != done)
    {
        return -1;
    }

    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    tcp_session::play_msg_t play_info={0};
    play_info.op_id = TCP_SESSION_OPID_EXT_FLAG;
    play_info.op_id_ext = TCP_SESSION_OPID_PLAY;
    play_info.channel = request->channel;
    play_info.audio_rtp_port = request->audio_rtp_port;
    play_info.rtp_port = request->rtp_port;
    play_info.mode = request->mode;
    play_info.ssrc = request->ssrc;

    tcp_session::sdp_info_t sdp_info = {0};
    if (!impl->session_op(play_info, sdp_info, timeout))
    {
        TCP_SC_LOG(tcp_sc_log_error, "tcp_session_client_play  fail op_id[%d],op_id_ext[%d],channel[%d],rtp_port[%d],mode[%d],ssrc[%d] sdp_info::length[%d],multiplex[%d],multiplexID[%d],data_type[%d],stop_flag[%d],rtp_port[%d]",
             play_info.op_id,play_info.op_id_ext, play_info.channel,play_info.rtp_port,play_info.mode,play_info.ssrc,sdp_info.length,sdp_info.multiplex,sdp_info.multiplexID,sdp_info.data_type,sdp_info.stop_flag,sdp_info.rtp_port);
        return -1;
    }

    response->data_type = sdp_info.data_type;
    response->stop_flag = sdp_info.stop_flag;
    response->rtp_port = sdp_info.rtp_port;
    response->length = sdp_info.length;
    memcpy(response->sdp, sdp_info.sdp, sdp_info.length);

    TCP_SC_LOG(tcp_sc_log_info, "data_type[%d],stop_flag[%d],rtp_port[%d],length[%d],sdp[%s]",response->data_type,response->stop_flag,response->rtp_port,response->length,response->sdp);
    return 0;
}

tcp_session_client_status_t tcp_session_client_stop(tcp_session_client_handle_t handle, const tcp_session_client_stop_request_t *request, tcp_session_client_stop_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout)
{
    if (NULL != done)
    {
        return -1;
    }

    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    tcp_session::stop_msg_t stop_info;
    stop_info.op_id = TCP_SESSION_OPID_STOP;
    stop_info.channel = request->channel;
    stop_info.mode = request->mode;
    stop_info.rtp_port = request->rtp_port;

    tcp_session::stop_response_msg_t result;
    if (!impl->session_op(stop_info, result, timeout))
    {
        TCP_SC_LOG(tcp_sc_log_error, "tcp_session_client_stop fail  op_id[%d],channel[%d],mode[%d],rtp_port[%d],result.op_id[%d],result.channel[%d],result.mode[%d],result.rtp_port[%d],timeout[%d]",
         stop_info.op_id,stop_info.channel,stop_info.mode,stop_info.rtp_port,result.op_id,result.channel,result.mode,result.rtp_port,timeout);
        return -1;
    }

    response->channel = result.channel;
    response->mode = result.mode;
    response->rtp_port = result.rtp_port;

    TCP_SC_LOG(tcp_sc_log_info, "channel[%d],mode[%d],rtp_port[%d]", response->channel,response->mode,response->rtp_port);

    return 0;
}

tcp_session_client_status_t tcp_session_client_demux_play(tcp_session_client_handle_t handle, const _tcp_session_client_demux_play_request_t *request, tcp_session_client_demux_play_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout)
{
    if (NULL != done)
    {
        return -1;
    }

    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    tcp_session::demux_play_msg_t play_info;
    play_info.op_id = TCP_SESSION_OPID_EXT2_FLAG;
    play_info.op_id_ext = TCP_SESSION_OPID_PLAY;
    play_info.channel = request->channel;
    play_info.audio_rtp_port = request->audio_rtp_port;
    play_info.rtp_port = request->rtp_port;
    play_info.mode = request->mode;
    play_info.ssrc = request->ssrc;
    play_info.multiplex = request->multiplex;
    play_info.multiplexID = request->multiplexID;

    tcp_session::sdp_info_t sdp_info;
    if (!impl->session_op(play_info, sdp_info, timeout))
    {
        TCP_SC_LOG(tcp_sc_log_error, "tcp_session_client_demux_play fail op_id[%d],op_id_ext[%d],channel[%d],audio_rtp_port[%d],rtp_port[%d],mode[%d],ssrc[%d],multiplex[%d],multiplexID[%d],sdp_info::length[%d],sdp[%s],multiplex[%d],multiplexID[%d],data_type[%d],stop_flag[%d],rtp_port[%d],timeout[%d]",
            play_info.op_id,play_info.op_id_ext,play_info.channel,play_info.audio_rtp_port,play_info.rtp_port,play_info.mode,play_info.ssrc,play_info.multiplex,play_info.multiplexID,sdp_info.length,sdp_info.sdp,sdp_info.multiplex,sdp_info.multiplexID,sdp_info.data_type,sdp_info.stop_flag,sdp_info.rtp_port,timeout);

        return -1;
    }

    response->data_type = sdp_info.data_type;
    response->stop_flag = sdp_info.stop_flag;
    response->rtp_port = sdp_info.rtp_port;
    response->multiplex = sdp_info.multiplex;
    response->multiplexID = sdp_info.multiplexID;
    response->length = sdp_info.length;
    memcpy(response->sdp, sdp_info.sdp, sdp_info.length);

    TCP_SC_LOG(tcp_sc_log_info, "tcp_session_client_demux_play  data_type[%d],stop_flag[%d],rtp_port[%d],multiplex[%d],multiplexID[%d],length[%d],sdp[%s]",
        response->data_type,response->stop_flag,response->rtp_port,response->multiplex,response->multiplexID, response->length,response->sdp);
    return 0;
}

tcp_session_client_status_t tcp_session_client_demux_stop(tcp_session_client_handle_t handle, const tcp_session_client_demux_stop_request_t *request, tcp_session_client_demux_stop_response_t *response, tcp_session_client_done_callback_t done, void *ctx, uint32_t timeout)
{
    if (NULL != done)
    {
        return -1;
    }

    tcp_session::client_t *impl = static_cast<tcp_session::client_t *>(handle);
    if (NULL == impl)
    {
        return -1;
    }

    tcp_session::demux_play_msg_t stop_info;
    stop_info.op_id = TCP_SESSION_OPID_EXT2_FLAG;
    stop_info.op_id_ext = TCP_SESSION_OPID_STOP;
    stop_info.channel = request->channel;
    stop_info.audio_rtp_port = request->audio_rtp_port;
    stop_info.rtp_port = request->rtp_port;
    stop_info.mode = request->mode;
    stop_info.ssrc = request->ssrc;
    stop_info.multiplex = request->multiplex;
    stop_info.multiplexID = request->multiplexID;

    tcp_session::stop_response_msg_t result;
    if (!impl->session_op(stop_info, result, timeout))
    {
        TCP_SC_LOG(tcp_sc_log_error, "tcp_session_client_demux_stop  op_id[%d],op_id_ext[%d],channel[%d],audio_rtp_port[%d],rtp_port[%d],mode[%d],ssrc[%d],multiplex[%d],multiplexID[%d],result::op_id[%d],channel[%d],mode[%d],rtp_port[%d],timeout[%d]",
            stop_info.op_id,stop_info.op_id_ext,stop_info.channel,stop_info.audio_rtp_port,stop_info.rtp_port,stop_info.mode,stop_info.ssrc,stop_info.multiplex,stop_info.multiplexID,result.op_id,result.channel,result.mode,result.rtp_port,timeout);
            return -1;
    }

    response->channel = result.channel;
    response->mode = result.mode;
    response->rtp_port = result.rtp_port;

    TCP_SC_LOG(tcp_sc_log_info, "channel[%d],mode[%d],rtp_port[%d]", response->channel,response->mode,response->rtp_port);

    return 0;
}

tcp_session_client_status_t xt_tcp_client_register_log(tcp_session_client_log_cb_t log_cb)
{
    g_log_cb = log_cb;
    return 0;
}