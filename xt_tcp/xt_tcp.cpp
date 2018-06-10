#include "xt_tcp.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <new>
#include <memory>

#ifdef _WIN32
#include <MSTcpIP.h>
#endif

namespace
{
    void tcp_connect_handler(xt_tcp_connect_handler_t handler, void *ctx, const boost::system::error_code& ec)
    {
        handler(ctx, ec.value());
    }

    void tcp_accept_handler(xt_tcp_accept_handler_t handler, void *ctx, xt_tcp_socket_t socket, const boost::system::error_code& ec)
    {
        handler(ctx, ec.value(), socket);
    }

    void tcp_send_handler(xt_tcp_send_handler_t handler, void *ctx, const void *buf, const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        handler(ctx, ec.value(), buf, bytes_transferred);
    }

    void tcp_receive_handler(xt_tcp_receive_handler_t handler, void *ctx, void *buf, const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        handler(ctx, ec.value(), buf, bytes_transferred);
    }

    struct service_t
    {
        boost::asio::io_service service_;
        boost::asio::io_service::work work_;

        service_t()
            :service_(),
            work_(service_)
        {}
    };

    template<int Level, int Name>
    xt_tcp_status_t socket_set_opt_helper(boost::asio::ip::tcp::socket *socket, void *val, size_t len, boost::asio::detail::socket_option::boolean<Level, Name> *)
    {
        bool value = false;
        if (sizeof(uint8_t) == len)
        {
            value = (0 != *static_cast<uint8_t *>(val));
        }
        else if (sizeof(uint16_t) == len)
        {
            value = (0 != *static_cast<uint16_t *>(val));
        }
        else if (sizeof(uint32_t) == len)
        {
            value = (0 != *static_cast<uint32_t *>(val));
        }
        else
        {
            return -1;
        }

        boost::system::error_code ec;
        socket->set_option(boost::asio::detail::socket_option::boolean<Level, Name>(value), ec);
        return ec.value();
    }

    template<int Level, int Name>
    xt_tcp_status_t socket_set_opt_helper(boost::asio::ip::tcp::socket *socket, void *val, size_t len, boost::asio::detail::socket_option::integer<Level, Name> *)
    {
        int value = 0;
        if (sizeof(uint8_t) == len)
        {
            value = *static_cast<uint8_t *>(val);
        }
        else if (sizeof(uint16_t) == len)
        {
            value = *static_cast<uint16_t *>(val);
        }
        else if (sizeof(uint32_t) == len)
        {
            value = *static_cast<uint32_t *>(val);
        }
        else
        {
            return -1;
        }

        boost::system::error_code ec;
        socket->set_option(boost::asio::detail::socket_option::integer<Level, Name>(value), ec);
        return ec.value();
    }

    template<int Level, int Name>
    xt_tcp_status_t socket_set_opt_helper(boost::asio::ip::tcp::socket *socket, void *val, size_t len, boost::asio::detail::socket_option::linger<Level, Name> *)
    {
        if (len < sizeof(xt_tcp_linger_t))
        {
            return -1;
        }

        boost::system::error_code ec;
        xt_tcp_linger_t *opt = static_cast<xt_tcp_linger_t *>(val);
        socket->set_option(boost::asio::detail::socket_option::linger<Level, Name>(0 != opt->onoff, opt->linger), ec);
        return ec.value();
    }

    template<int Level, int Name>
    xt_tcp_status_t socket_get_opt_helper(boost::asio::ip::tcp::socket *socket, void *val, size_t len, boost::asio::detail::socket_option::boolean<Level, Name> *)
    {
        boost::system::error_code ec;
        boost::asio::detail::socket_option::boolean<Level, Name> opt;
        socket->get_option(opt, ec);
        if (!ec)
        {
            if (sizeof(uint8_t) == len)
            {
                *static_cast<uint8_t *>(val) = opt.value();
            }
            else if (sizeof(uint16_t) == len)
            {
                *static_cast<uint16_t *>(val) = opt.value();
            }
            else if (sizeof(uint32_t) == len)
            {
                *static_cast<uint32_t *>(val) = opt.value();
            }
            else
            {
                return -1;
            }
        }
        return ec.value();
    }

    template<int Level, int Name>
    xt_tcp_status_t socket_get_opt_helper(boost::asio::ip::tcp::socket *socket, void *val, size_t len, boost::asio::detail::socket_option::integer<Level, Name> *)
    {
        boost::system::error_code ec;
        boost::asio::detail::socket_option::integer<Level, Name> opt;
        socket->get_option(opt, ec);
        if (!ec)
        {
            if (sizeof(uint32_t) == len)
            {
                *static_cast<uint32_t *>(val) = opt.value();
            }
            else
            {
                return -1;
            }
        }
        return ec.value();
    }

    template<int Level, int Name>
    xt_tcp_status_t socket_get_opt_helper(boost::asio::ip::tcp::socket *socket, void *val, size_t len, boost::asio::detail::socket_option::linger<Level, Name> *)
    {
        boost::system::error_code ec;
        boost::asio::detail::socket_option::linger<Level, Name> opt;
        socket->get_option(opt, ec);
        if (!ec)
        {
            if (sizeof(xt_tcp_linger_t) == len)
            {
                xt_tcp_linger_t *pl = static_cast<xt_tcp_linger_t *>(val);
                pl->onoff = opt.enabled();
                pl->linger = opt.timeout();
            }
            else
            {
                return -1;
            }
        }
        return ec.value();
    }

    template<typename Opt>
    xt_tcp_status_t socket_set_opt(boost::asio::ip::tcp::socket *socket, void *val, size_t len)
    {
        return socket_set_opt_helper(socket, val, len, (Opt *)NULL);
    }

    template<typename Opt>
    xt_tcp_status_t socket_get_opt(boost::asio::ip::tcp::socket *socket, void *val, size_t len)
    {
        return socket_get_opt_helper(socket, val, len, (Opt *)NULL);
    }
}

xt_tcp_status_t xt_tcp_create_service(xt_tcp_service_t *pservice)
{
    if (NULL == pservice)
    {
        return -1;
    }

    *pservice = new (std::nothrow) service_t;

    return 0;
}

xt_tcp_status_t xt_tcp_destroy_service(xt_tcp_service_t service)
{
    service_t *impl = static_cast<service_t *>(service);
    if (NULL == impl)
    {
        return -1;
    }

    delete impl;
    return 0;
}

xt_tcp_status_t xt_tcp_service_run(xt_tcp_service_t service)
{
    service_t *impl = static_cast<service_t *>(service);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        impl->service_.run();
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }
    return 0;
}

xt_tcp_status_t xt_tcp_service_poll(xt_tcp_service_t service)
{
    service_t *impl = static_cast<service_t *>(service);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        impl->service_.poll();
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }
    return 0;
}

xt_tcp_status_t xt_tcp_service_stop(xt_tcp_service_t service)
{
    service_t *impl = static_cast<service_t *>(service);
    if (NULL == impl)
    {
        return -1;
    }

    impl->service_.stop();
    return 0;
}

xt_tcp_status_t xt_tcp_service_stoped(xt_tcp_service_t service)
{
    service_t *impl = static_cast<service_t *>(service);
    if (NULL == impl)
    {
        return -1;
    }

    return impl->service_.stopped() ? 0 : -1;
}

xt_tcp_status_t xt_tcp_create_acceptor(xt_tcp_service_t service, const char *ip, uint16_t port, uint8_t reuse_addr, xt_tcp_acceptor_t *pacceptor)
{
    if ((NULL == service) || (NULL == pacceptor))
    {
        return -1;
    }

    std::auto_ptr<boost::asio::ip::tcp::acceptor> acceptor;

    try
    {
        acceptor.reset(new (std::nothrow) boost::asio::ip::tcp::acceptor(((service_t *)service)->service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ip), port), 0 != reuse_addr));
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    *pacceptor = acceptor.release();
    return 0;
}

xt_tcp_status_t xt_tcp_destroy_acceptor(xt_tcp_acceptor_t acceptor)
{
    boost::asio::ip::tcp::acceptor *impl = static_cast<boost::asio::ip::tcp::acceptor *>(acceptor);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        impl->close();
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    delete impl;
    return 0;
}

xt_tcp_status_t xt_tcp_acceptor_accept(xt_tcp_acceptor_t acceptor, xt_tcp_socket_t socket, xt_tcp_accept_handler_t handler, void *ctx)
{
    boost::asio::ip::tcp::acceptor *impl = static_cast<boost::asio::ip::tcp::acceptor *>(acceptor);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        if (NULL == handler)
        {
            impl->accept(*(boost::asio::ip::tcp::socket *)socket);
        }
        else
        {
            impl->async_accept(*(boost::asio::ip::tcp::socket *)socket, boost::bind(tcp_accept_handler, handler, ctx, socket, boost::asio::placeholders::error));
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}


xt_tcp_status_t xt_tcp_create_socket(xt_tcp_service_t service, int8_t open, xt_tcp_socket_t *psocket)
{
    if ((NULL == service) || (NULL == psocket))
    {
        return -1;
    }

    try
    {
        if (0 == open)
        {
            *psocket = new (std::nothrow) boost::asio::ip::tcp::socket(((service_t *)service)->service_);
        }
        else
        {
            *psocket = new (std::nothrow) boost::asio::ip::tcp::socket(((service_t *)service)->service_, boost::asio::ip::tcp::v4());
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_attach_socket(xt_tcp_service_t service, int native_socket, xt_tcp_socket_t *psocket)
{
    if ((NULL == service) || (NULL == psocket))
    {
        return -1;
    }

    try
    {
        *psocket = new (std::nothrow) boost::asio::ip::tcp::socket(((service_t *)service)->service_, boost::asio::ip::tcp::v4(), native_socket);
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_destroy_socket(xt_tcp_socket_t socket)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    delete impl;
    return 0;
}

xt_tcp_status_t xt_tcp_socket_bind(xt_tcp_socket_t socket, const char *ip, uint16_t port)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        if (!impl->is_open())
        {
            impl->open(boost::asio::ip::tcp::v4());
        }

        impl->bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ip), port));
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_connect(xt_tcp_socket_t socket, const char *ip, uint16_t port, xt_tcp_connect_handler_t handler, void *ctx)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        if (NULL == handler)
        {
            impl->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ip), port));
        }
        else
        {
            impl->async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ip), port), boost::bind(tcp_connect_handler, handler, ctx, boost::asio::placeholders::error));
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_close(xt_tcp_socket_t socket)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        impl->close();
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_cancel(xt_tcp_socket_t socket)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        impl->cancel();
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

#define XT_TCP_SOCKET_OPT_IMPL(socket, opt, val, len, func) \
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);   \
    if (NULL == impl)   \
    {   \
        return -1;  \
    }   \
    switch (opt)    \
    {   \
    case XT_TCP_OPT_BROADCAST:  \
        return func<boost::asio::socket_base::broadcast>(impl, val, len); \
        break;  \
    case XT_TCP_OPT_DEBUG:  \
        return func<boost::asio::socket_base::debug>(impl, val, len); \
        break;  \
    case XT_TCP_OPT_NOT_ROUTE:  \
        return func<boost::asio::socket_base::do_not_route>(impl, val, len);  \
        break;  \
    case XT_TCP_OPT_KEEP_ALIVE: \
        return func<boost::asio::socket_base::keep_alive>(impl, val, len);    \
        break;  \
    case XT_TCP_OPT_SEND_BUF_SIZE:  \
        return func<boost::asio::socket_base::send_buffer_size>(impl, val, len);  \
        break;  \
    case XT_TCP_OPT_SEND_LOW_WATERMARK: \
        return func<boost::asio::socket_base::send_low_watermark>(impl, val, len);    \
        break;  \
    case XT_TCP_OPT_RECV_BUF_SIZE:  \
        return func<boost::asio::socket_base::receive_buffer_size>(impl, val, len);   \
        break;  \
    case XT_TCP_OPT_RECV_LOW_WATERMARK: \
        return func<boost::asio::socket_base::receive_low_watermark>(impl, val, len); \
        break;  \
    case XT_TCP_OPT_REUSE_ADDR: \
        return func<boost::asio::socket_base::reuse_address>(impl, val, len); \
        break;  \
    case XT_TCP_OPT_LINGER: \
        return func<boost::asio::socket_base::linger>(impl, val, len);    \
        break;  \
    case XT_TCP_OPT_ENABLE_CONNECTION_ABORTED:  \
        return func<boost::asio::socket_base::enable_connection_aborted>(impl, val, len); \
        break;  \
    default:    \
        break;  \
    }   \
    return -1;

xt_tcp_status_t xt_tcp_socket_set_opt(xt_tcp_socket_t socket, xt_tcp_socket_opt opt, void *val, size_t len)
{
    XT_TCP_SOCKET_OPT_IMPL(socket, opt, val, len, socket_set_opt)
}

xt_tcp_status_t xt_tcp_socket_get_opt(xt_tcp_socket_t socket, xt_tcp_socket_opt opt, void *val, size_t len)
{
    XT_TCP_SOCKET_OPT_IMPL(socket, opt, val, len, socket_get_opt)
}

xt_tcp_status_t xt_tcp_socket_shutdown(xt_tcp_socket_t socket, xt_tcp_shutdown_type what)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    boost::asio::socket_base::shutdown_type internal_what = boost::asio::socket_base::shutdown_both;
    switch (what)
    {
    case XT_TCP_SHUT_SEND:
        internal_what = boost::asio::socket_base::shutdown_send;
        break;
    case XT_TCP_SHUT_RECEIVE:
        internal_what = boost::asio::socket_base::shutdown_receive;
        break;
    case XT_TCP_SHUT_BOTH:
        internal_what = boost::asio::socket_base::shutdown_both;
        break;
    }

    try
    {
        impl->shutdown(internal_what);
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_get_remote(xt_tcp_socket_t socket, char *ip, uint16_t *port)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        boost::asio::ip::tcp::endpoint peer = impl->remote_endpoint();
#ifdef _WIN32
        strcpy_s(ip, 32, peer.address().to_string().c_str());
#else
        strcpy(ip, peer.address().to_string().c_str());
#endif
        *port = static_cast<uint16_t>(peer.port());
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_get_local(xt_tcp_socket_t socket, char *ip, uint16_t *port)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        boost::asio::ip::tcp::endpoint peer = impl->local_endpoint();
#ifdef _WIN32
        strcpy_s(ip, 32, peer.address().to_string().c_str());
#else
        strcpy(ip, peer.address().to_string().c_str());
#endif
        *port = static_cast<uint16_t>(peer.port());
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_send(xt_tcp_socket_t socket, const void *buf, size_t len, xt_tcp_message_flag flags, xt_tcp_send_handler_t handler, void *ctx)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    boost::asio::socket_base::message_flags internal_flags = 0;
    switch (flags)
    {
    case XT_TCP_MSG_PEEK:
        internal_flags = boost::asio::socket_base::message_peek;
        break;
    case XT_TCP_MSG_OOB:
        internal_flags = boost::asio::socket_base::message_out_of_band;
        break;
    case XT_TCP_MSG_MSG_DONTROUTE:
        internal_flags = boost::asio::socket_base::message_do_not_route;
        break;
    case XT_TCP_MSG_MSG_MSG_EOR:
        internal_flags = boost::asio::socket_base::message_end_of_record;
        break;
    }

    try
    {
        if (NULL == handler)
        {
            impl->send(boost::asio::buffer(buf, len), internal_flags);
        }
        else
        {
            impl->async_send(boost::asio::buffer(buf, len), internal_flags, boost::bind(tcp_send_handler, handler, ctx, buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_receive(xt_tcp_socket_t socket, void *buf, size_t len, xt_tcp_message_flag flags, xt_tcp_receive_handler_t handler, void *ctx)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    boost::asio::socket_base::message_flags internal_flags = 0;
    switch (flags)
    {
    case XT_TCP_MSG_PEEK:
        internal_flags = boost::asio::socket_base::message_peek;
        break;
    case XT_TCP_MSG_OOB:
        internal_flags = boost::asio::socket_base::message_out_of_band;
        break;
    case XT_TCP_MSG_MSG_DONTROUTE:
        internal_flags = boost::asio::socket_base::message_do_not_route;
        break;
    case XT_TCP_MSG_MSG_MSG_EOR:
        internal_flags = boost::asio::socket_base::message_end_of_record;
        break;
    }

    try
    {
        if (NULL == handler)
        {
            impl->receive(boost::asio::buffer(buf, len), internal_flags);
        }
        else
        {
            impl->async_receive(boost::asio::buffer(buf, len), internal_flags, boost::bind(tcp_receive_handler, handler, ctx, buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_write_some(xt_tcp_socket_t socket, const void *buf, size_t len, xt_tcp_send_handler_t handler, void *ctx)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        if (NULL == handler)
        {
            impl->write_some(boost::asio::buffer(buf, len));
        }
        else
        {
            impl->async_write_some(boost::asio::buffer(buf, len), boost::bind(tcp_send_handler, handler, ctx, buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_read_some(xt_tcp_socket_t socket, void *buf, size_t len, xt_tcp_receive_handler_t handler, void *ctx)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    try
    {
        if (NULL == handler)
        {
            impl->read_some(boost::asio::buffer(buf, len));
        }
        else
        {
            impl->async_read_some(boost::asio::buffer(buf, len), boost::bind(tcp_receive_handler, handler, ctx, buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

    return 0;
}

xt_tcp_status_t xt_tcp_socket_set_keepalive(xt_tcp_socket_t socket, const xt_tcp_keepalive_t *opt)
{
    boost::asio::ip::tcp::socket *impl = static_cast<boost::asio::ip::tcp::socket *>(socket);
    if (NULL == impl)
    {
        return -1;
    }

    if (NULL == opt)
    {
        return -1;
    }

    try
    {
        impl->set_option(boost::asio::socket_base::keep_alive(0 != opt->onoff));
    }
    catch (const boost::system::system_error& e)
    {
        return e.code().value();
    }

#ifdef _WIN32
    tcp_keepalive alive_in = { 0 };
    tcp_keepalive alive_out = { 0 };

    alive_in.onoff = opt->onoff;
    alive_in.keepalivetime = opt->idle * 1000;
    alive_in.keepaliveinterval = opt->interval * 1000;

    DWORD dwByteReturn = 0;
    return ::WSAIoctl(impl->native_handle(), SIO_KEEPALIVE_VALS, &alive_in, sizeof(tcp_keepalive), &alive_out, sizeof(tcp_keepalive), &dwByteReturn, NULL, NULL);
#else
    int ret = setsockopt(impl->native_handle(), SOL_TCP, TCP_KEEPIDLE, &opt->idle, sizeof(uint32_t));
    if (0 != ret)
    {
        return ret;
    }

    ret = setsockopt(impl->native_handle(), SOL_TCP, TCP_KEEPINTVL, &opt->interval, sizeof(uint32_t));
    if (0 != ret)
    {
        return ret;
    }

    ret = setsockopt(impl->native_handle(), SOL_TCP, TCP_KEEPCNT, &opt->count, sizeof(uint32_t));
    if (0 != ret)
    {
        return ret;
    }
    return 0;
#endif
}

