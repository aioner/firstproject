#ifndef _XT_TCP_WRAPPER_H_INCLUDED
#define _XT_TCP_WRAPPER_H_INCLUDED

#include "xt_tcp.h"
#include <stdexcept>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>

#ifndef __cplusplus
    #error "xt_tcp_wrapper is a cplusplus tool file."
#endif


#define XT_TCP_STR2(x)              #x
#define XT_TCP_STR(x)               XT_TCP_STR2(x)

#ifdef _MSC_VER
    #define XT_TCP_THROW()              throw std::runtime_error(__FUNCTION__ "-" XT_TCP_STR(__LINE__))
#else
    #define XT_TCP_THROW()              throw std::runtime_error(__FILE__ "-" XT_TCP_STR(__LINE__))
#endif


namespace xt_tcp
{
    class service_t : private boost::noncopyable
    {
    public:
        service_t()
        {
            if (!init())
            {
                XT_TCP_THROW();
            }
        }

        ~service_t()
        {
            destroy();
        }

        bool init()
        {
            return (XT_TCP_STATUS_OK == xt_tcp_create_service(&impl_));
        }

        void destroy()
        {
            if (XT_TCP_INVALID_HANDLE != impl_)
            {
                xt_tcp_destroy_service(impl_);
                impl_ = XT_TCP_INVALID_HANDLE;
            }
        }

        bool run()
        {
            return (XT_TCP_STATUS_OK == xt_tcp_service_run(impl_));
        }

        bool poll()
        {
            return (XT_TCP_STATUS_OK == xt_tcp_service_poll(impl_));
        }

        bool stop()
        {
            return (XT_TCP_STATUS_OK == xt_tcp_service_stop(impl_));
        }

        operator xt_tcp_service_t()
        {
            return impl_;
        }

        operator const xt_tcp_service_t() const
        {
            return impl_;
        }

    private:
        xt_tcp_service_t impl_;
    };

    inline xt_tcp_socket_t create_socket(xt_tcp_service_t service)
    {
        xt_tcp_socket_t socket = XT_TCP_INVALID_HANDLE;
        xt_tcp_create_socket(service, 0, &socket);
        return socket;
    }

    inline void destroy_socket(xt_tcp_socket_t socket)
    {
        xt_tcp_destroy_socket(socket);
    }

    class socket_t : public boost::intrusive_ref_counter<socket_t>, private boost::noncopyable
    {
    public:
        socket_t()
            :impl_(XT_TCP_INVALID_HANDLE)
        {}

        explicit socket_t(xt_tcp_service_t service)
        {
            if (!init(service))
            {
                XT_TCP_THROW();
            }
        }

        socket_t(xt_tcp_service_t service, int native_socket)
        {
            if (XT_TCP_STATUS_OK != xt_tcp_attach_socket(service, native_socket, &impl_))
            {
                XT_TCP_THROW();
            }
        }

        socket_t(xt_tcp_service_t service, const char *ip, uint16_t port, bool reuse_addr = true)
        {
            if (!init(service, true))
            {
                XT_TCP_THROW();
            }

            if (!set_option(XT_TCP_OPT_REUSE_ADDR, reuse_addr))
            {
                XT_TCP_THROW();
            }

            if (!bind(ip, port))
            {
                XT_TCP_THROW();
            }

            xt_tcp_linger_t opt;
            opt.onoff = 1;
            opt.linger = 0;
            if (!set_option(XT_TCP_OPT_LINGER, opt))
            {
                XT_TCP_THROW();
            }
        }

        bool set_keepalive_opt(bool enabled, uint32_t idle, uint32_t interval, uint32_t count = 1)
        {
            xt_tcp_keepalive_t opt = { 0 };
            opt.onoff = enabled;
            opt.idle = idle;
            opt.interval = interval;
            opt.count = count;
            return (0 == xt_tcp_socket_set_keepalive(impl_, &opt));
        }

        ~socket_t()
        {
            term();
        }

        bool init(xt_tcp_service_t service, bool open = false)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_create_socket(service, (uint8_t)open, &impl_));
        }

        void term()
        {
            //!!!notify: shutdown会使setsockopt lingger强制退出方式失效
            close();
            destroy();
        }

        void destroy()
        {
            if (XT_TCP_INVALID_HANDLE != impl_)
            {
                xt_tcp_destroy_socket(impl_);
                impl_ = XT_TCP_INVALID_HANDLE;
            }
        }

        void attach(xt_tcp_socket_t socket)
        {
            impl_ = socket;
        }

        xt_tcp_socket_t detach()
        {
            xt_tcp_socket_t socket = impl_;
            impl_ = XT_TCP_INVALID_HANDLE;
            return socket;
        }

        template<typename T>
        bool set_option(xt_tcp_socket_opt opt, T val)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_set_opt(impl_, opt, &val, sizeof(val)));
        }

        template<typename T>
        T get_option(xt_tcp_socket_opt opt)
        {
            T val;
            if (XT_TCP_STATUS_OK != xt_tcp_socket_get_opt(impl_, opt, &val, sizeof(val)))
            {
                throw std::runtime_error("xt_tcp_socket_get_opt");
            }
            return val;
        }

        bool bind(const char *ip, uint16_t port)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_bind(impl_, ip, port));
        }

        void cancel()
        {
            if (XT_TCP_INVALID_HANDLE != impl_)
            {
                xt_tcp_socket_cancel(impl_);
            }
        }

        void close()
        {
            if (XT_TCP_INVALID_HANDLE != impl_)
            {
                xt_tcp_socket_close(impl_);
            }
        }

        void shutdown(xt_tcp_shutdown_type what = XT_TCP_SHUT_BOTH)
        {
            if (XT_TCP_INVALID_HANDLE != impl_)
            {
                xt_tcp_socket_shutdown(impl_, what);
            }
        }

        bool local_endpoint(char *ip, uint16_t& port)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_get_local(impl_, ip, &port));
        }

        bool remote_endpoint(char *ip, uint16_t& port)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_get_remote(impl_, ip, &port));
        }

        bool connect(const char *ip, uint16_t port)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_connect(impl_, ip, port, NULL, NULL));
        }

        bool async_connect(const char *ip, uint16_t port)
        {
            boost::sp_adl_block::intrusive_ptr_add_ref(this);
            return (XT_TCP_STATUS_OK == xt_tcp_socket_connect(impl_, ip, port, &socket_t::connect_handler, this));
        }

        bool send(const void *buf, size_t len, int flags = 0)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_send(impl_, buf, len, flags, NULL, NULL));
        }

        bool async_send(const void *buf, size_t len, int flags = 0)
        {
            boost::sp_adl_block::intrusive_ptr_add_ref(this);
            return (XT_TCP_STATUS_OK == xt_tcp_socket_send(impl_, buf, len, flags, &socket_t::send_handler, this));
        }

        bool receivce(void *buf, size_t len, int flags = 0)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_socket_receive(impl_, buf, len, flags, NULL, NULL));
        }
        bool async_receice(void *buf, size_t len, int flags = 0)
        {
            boost::sp_adl_block::intrusive_ptr_add_ref(this);
            return (XT_TCP_STATUS_OK == xt_tcp_socket_receive(impl_, buf, len, flags, &socket_t::receice_handler, this));
        }

        xt_tcp_socket_t& get_impl()
        {
            return impl_;
        }

    protected:
        virtual void on_connect(xt_tcp_status_t stat) = 0;
        virtual void on_send(xt_tcp_status_t stat, const void* buf, size_t bytes_transferred) = 0;
        virtual void on_receive(xt_tcp_status_t stat, void *buf, size_t bytes_transferred) = 0;

    private:
        static void XT_TCP_STDCALL connect_handler(void *ctx, xt_tcp_status_t stat)
        {
            boost::intrusive_ptr<socket_t> sp(static_cast<socket_t *>(ctx), false);
            sp->on_connect(stat);
        }

        static void XT_TCP_STDCALL send_handler(void *ctx, xt_tcp_status_t stat, const void *buf, size_t bytes_transferred)
        {
            boost::intrusive_ptr<socket_t> sp(static_cast<socket_t *>(ctx), false);
            sp->on_send(stat, buf, bytes_transferred);
        }

        static void XT_TCP_STDCALL receice_handler(void *ctx, xt_tcp_status_t stat, void *buf, size_t bytes_transferred)
        {
            boost::intrusive_ptr<socket_t> sp(static_cast<socket_t *>(ctx), false);
            sp->on_receive(stat, buf, bytes_transferred);
        }

        xt_tcp_socket_t impl_;
    };

    class acceptor_t : public boost::intrusive_ref_counter<acceptor_t>, private boost::noncopyable
    {
    public:
        acceptor_t()
            :service_(XT_TCP_INVALID_HANDLE),
            impl_(XT_TCP_INVALID_HANDLE)
        {}

        acceptor_t(xt_tcp_service_t service, const char *ip, uint16_t port)
        {
            if (!init(service, ip, port))
            {
                XT_TCP_THROW();
            }
        }

        ~acceptor_t()
        {
            destroy();
        }

        bool init(xt_tcp_service_t service, const char *ip, uint16_t port)
        {
            service_ = service;
            return (XT_TCP_STATUS_OK == xt_tcp_create_acceptor(service, ip, port, 1, &impl_));
        }

        void destroy()
        {
            if (XT_TCP_INVALID_HANDLE != impl_)
            {
                xt_tcp_destroy_acceptor(impl_);
                impl_ = XT_TCP_INVALID_HANDLE;
            }

            service_ = XT_TCP_INVALID_HANDLE;
        }

        bool accept(xt_tcp_socket_t socket)
        {
            return (XT_TCP_STATUS_OK == xt_tcp_acceptor_accept(impl_, socket, NULL, NULL));
        }

        bool async_accept(xt_tcp_socket_t socket)
        {
            boost::sp_adl_block::intrusive_ptr_add_ref(this);
            return (XT_TCP_STATUS_OK == xt_tcp_acceptor_accept(impl_, socket, &acceptor_t::accept_handler, this));
        }

        xt_tcp_service_t &get_service()
        {
            return service_;
        }

    protected:
        virtual void on_accept(xt_tcp_status_t stat, xt_tcp_socket_t socket) {}

    private:
        static void XT_TCP_STDCALL accept_handler(void *ctx, xt_tcp_status_t stat, xt_tcp_socket_t socket)
        {
            boost::intrusive_ptr<acceptor_t> sp(static_cast<acceptor_t *>(ctx), false);
            sp->on_accept(stat, socket);
        }

        xt_tcp_service_t service_;
        xt_tcp_acceptor_t impl_;
    };
}

#endif //_XT_TCP_WRAPPER_H_INCLUDED
