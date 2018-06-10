#include "xt_udp_session_server.h"
#include "session_server.h"
#include "error.h"
#include <memory>

xt_print_cb udp_svr_print_	= NULL;

extern "C"
{
    udp_service_handle xt_udp_create_service()
    {
        return udp_session_server::session_server_impl::create_service();
    }

    void xt_udp_run_service(udp_service_handle service)
    {
        udp_session_server::session_server_impl::run_service(service);
    }

    void xt_udp_destroy_service(udp_service_handle service)
    {
        udp_session_server::session_server_impl::destroy_service(service);
    }

    udp_session_handle xt_udp_session_init(const xt_udp_session_config_t *config, xt_print_cb print_cb)
    {
		udp_svr_print_ = print_cb;

        std::auto_ptr<udp_session_server::session_server_impl>impl(new udp_session_server::session_server_impl(config));
        if (!impl->init())
        {
            return NULL;
        }
        return impl.release();
    }

    void xt_udp_session_term(udp_session_handle session)
    {
        delete (udp_session_server::session_server_impl *)session;
    }

    int32_t xt_udp_server_session_send_regist( udp_session_handle session, const char *ip, uint16_t port, const xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout )
    {
        udp_session_server::session_server_impl *impl = (udp_session_server::session_server_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }
        return impl->send_regist_method(ip,port,request,response,done,ctx, timeout);
    }

    int32_t xt_udp_server_session_send_stop_regist( udp_session_handle session, const char *ip, uint16_t port, xt_send_regist_request_t *request, xt_send_regist_response_t *response, response_done_callback_t done, void *ctx, uint32_t timeout )
    {
        udp_session_server::session_server_impl *impl = (udp_session_server::session_server_impl *)session;
        if (NULL == impl)
        {
            return udp_session::error::invalid_arg;
        }
        return impl->send_stop_regist_method(ip,port,request,response,done,ctx, timeout);
    }
}
