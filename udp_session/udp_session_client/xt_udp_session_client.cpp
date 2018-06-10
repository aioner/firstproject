#include "xt_udp_session_client.h"
#include "session_client.h"
#include "error.h"

#include <memory>

const char* XT_UDP_CLIENT_LIB_INFO = "XT_Lib_Version: V_XT_Udp_Client_1.00.0120.0";

extern "C"
{
    udp_service_handle xt_udp_client_create_service()
    {
        return udp_session_client::session_client_impl::create_service();
    }

    void xt_udp_client_run_service(udp_service_handle service)
    {
        udp_session_client::session_client_impl::run_service(service);
    }

    void xt_udp_client_destroy_service(udp_service_handle service)
    {
        udp_session_client::session_client_impl::destroy_service(service);
    }

    udp_session_handle xt_udp_client_session_init(const char *ip, uint16_t port, udp_service_handle service)
    {
        std::auto_ptr<udp_session_client::session_client_impl>impl(new udp_session_client::session_client_impl);
        if (!impl->init(ip, port, service))
        {
            return NULL;
        }
        return impl.release();
    }

    void xt_udp_client_session_term(udp_session_handle session)
    {
        delete (udp_session_client::session_client_impl *)session;
    }

    int32_t xt_udp_client_session_get_sdp(udp_session_handle session, const char *ip, uint16_t port, xt_get_sdp_request_t *request, xt_get_sdp_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }

        return impl->get_sdp_method(ip, port, request, response, done, ctx, timeout);
    }

    int32_t xt_udp_client_session_play(udp_session_handle session, const char *ip, uint16_t port, xt_play_request_t *request, xt_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }

        return impl->play_method(ip, port, request, response, done, ctx, timeout);
    }

    int32_t xt_udp_client_session_stop(udp_session_handle session, const char *ip, uint16_t port, xt_stop_request_t *request, xt_stop_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }

        return impl->stop_method(ip, port, request, response, done, ctx, timeout);
    }

    void xt_udp_client_session_heartbit(udp_session_handle session, const char *ip, uint16_t port, uint16_t channel)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL != impl)
        {
            impl->heartbit_method(ip, port, channel);
        }
    }

    int32_t xt_udp_client_session_fast_play(udp_session_handle session, const char *ip, uint16_t port, xt_get_sdp_and_play_request_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }
        return impl->get_sdp_and_play_method(ip, port, request, response, done, ctx, timeout);
    }

    /*支持多码流点播接口*/
    int32_t xt_udp_client_session_fast_play_v1(udp_session_handle session, const char *ip, uint16_t port, xt_get_sdp_and_play_request_v1_t *request, xt_get_sdp_and_play_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }

        return impl->get_sdp_and_play_method_v1(ip, port, request, response, done, ctx, timeout);
    }

    int32_t xt_udp_client_session_send_data(udp_session_handle session, const char *ip, uint16_t port, xt_send_data_request_t *request, xt_send_data_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }

        return impl->send_data_method(ip, port, request, response, done, ctx, timeout);
    }

    int32_t xt_udp_client_session_regist_callback( udp_session_handle session, udp_session_regist_call_back_t cb )
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }

        return impl->set_regist_callback(cb);

    }

    void xt_udp_client_session_heartbit2(udp_session_handle session, const char *ip, uint16_t port, uint32_t sink_ip, uint16_t sink_port, uint16_t channel)
    {
        udp_session_client::session_client_impl *impl = (udp_session_client::session_client_impl *)session;
        if (NULL != impl)
        {
            impl->heartbit2_method(ip, port, sink_ip, sink_port, channel);
        }
    }
};
