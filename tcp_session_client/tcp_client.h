#ifndef _TCP_CLIENT_H_INCLUDED
#define _TCP_CLIENT_H_INCLUDED

#include "xt_tcp/xt_tcp_wrapper.h"
#include "tcp_session_client_api.h"
#include "tcp_session_serialization.h"

#include <boost/thread/condition.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace tcp_session
{
    class client_t : public xt_tcp::socket_t
    {
        boost::condition cv_;
        mutable boost::recursive_mutex mutex_;
        typedef boost::unique_lock<boost::recursive_mutex> scoped_lock;
    public:
        enum {  receive_buffer_max_size = 2048 };
        typedef xt_tcp::socket_t base_t;

        client_t(xt_tcp_service_t service, int native_socket, tcp_session_client_close_callback_t cb, void *ctx);
        client_t(xt_tcp_service_t service, const char *ip, uint16_t port, tcp_session_client_close_callback_t cb, void *ctx);

        void destruct();

        bool connect(const char *ip, uint16_t port, uint32_t timeout);
        bool is_connected() const;

        template<typename RequestT, typename ResponseT>
        bool session_op(const RequestT &request, ResponseT& response, uint32_t timeout)
        {
            scoped_lock _lock(mutex_);
            return do_session_op(_lock, request, response, timeout);
        }

    private:
        bool start_read_header(scoped_lock &);
        bool start_read_body(scoped_lock &);
        bool start_read(scoped_lock &);
        bool start_connect(scoped_lock &lock, const char *ip, uint16_t port, uint32_t timeout);

        template<typename RequestT>
        bool start_request(scoped_lock &lock, const RequestT& request)
        {
            if (!connected_)
            {
                return false;
            }

            std::pair<const void *, uint32_t> buf;
            if (!tcp_session::serialize_request(request, buf))
            {
                return false;
            }

            return this->send(buf.first, buf.second);
        }

        template<typename ResponseT>
        bool wait_response(scoped_lock &lock, ResponseT& response, uint32_t timeout)
        {
            if (!cv_.timed_wait(lock, boost::posix_time::milliseconds(timeout)) || !receive_success_)
            {
                return false;
            }

            return tcp_session::deserialize_response(header_, body_, response);
        }

        template<typename RequestT, typename ResponseT>
        bool do_session_op(scoped_lock &lock, const RequestT& request, ResponseT& response, uint32_t timeout)
        {
            return (start_request(lock, request) && wait_response(lock, response, timeout));
        }

        void on_read_completion(scoped_lock &, bool success);
        void on_close(scoped_lock &, bool close_if_error = false);
        void on_connect(xt_tcp_status_t stat);
        void on_send(xt_tcp_status_t stat, const void* buf, size_t bytes_transferred) {}
        void on_receive(xt_tcp_status_t stat, void *buf, size_t bytes_transferred);

        bool connected_;
        bool receive_success_;

        enum read_state_t
        {
            read_init,
            read_header,
            read_body
        } read_state_;

        tcp_session::msg_header_t header_;
        char body_[receive_buffer_max_size];

        tcp_session_client_close_callback_t cb_;
        void *ctx_;
    };
}

#endif //_TCP_CLIENT_H_INCLUDED
