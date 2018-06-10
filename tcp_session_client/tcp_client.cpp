#include "tcp_client.h"

namespace tcp_session
{
    client_t::client_t(xt_tcp_service_t service, int native_socket, tcp_session_client_close_callback_t cb, void *ctx)
        :base_t(service, native_socket),
        cv_(),
        mutex_(),
        connected_(false),
        receive_success_(false),
        read_state_(read_init),
        header_(),
        cb_(cb),
        ctx_(ctx)
    {}

    client_t::client_t(xt_tcp_service_t service, const char *ip, uint16_t port, tcp_session_client_close_callback_t cb, void *ctx)
        :base_t(service, ip, port),
        cv_(),
        mutex_(),
        connected_(false),
        receive_success_(false),
        read_state_(read_init),
        header_(),
        cb_(cb),
        ctx_(ctx)
    {}

    bool client_t::connect(const char *ip, uint16_t port, uint32_t timeout)
    {
        scoped_lock _lock(mutex_);
        return start_connect(_lock, ip, port, timeout);
    }

    bool client_t::is_connected() const
    {
        scoped_lock _lock(mutex_);
        return connected_;
    }

    void client_t::destruct()
    {
        this->term();

        scoped_lock _lock(mutex_);
        cb_ = NULL;
        ctx_ = NULL;
        connected_ = false;
    }

    bool client_t::start_read_header(scoped_lock &)
    {
        return async_receice(&header_, sizeof(tcp_session::msg_header_t));
    }

    bool client_t::start_read_body(scoped_lock &)
    {
        if (header_.data_size > receive_buffer_max_size)
        {
            return false;
        }
        return async_receice(body_, header_.data_size);
    }

    bool client_t::start_read(scoped_lock &lock)
    {
        read_state_ = read_header;
        return start_read_header(lock);
    }

    bool client_t::start_connect(scoped_lock &lock, const char *ip, uint16_t port, uint32_t timeout)
    {
        if (connected_)
        {
            return true;
        }

        if (!this->async_connect(ip, port))
        {
            return false;
        }

        if (!cv_.timed_wait(lock, boost::posix_time::milliseconds(timeout)) || !connected_)
        {
            this->cancel();
            return false;
        }

        return start_read(lock);
    }

    void client_t::on_read_completion(scoped_lock&, bool success)
    {
        receive_success_ = success;
        cv_.notify_one();
    }

    void client_t::on_connect(xt_tcp_status_t stat)
    {
        scoped_lock _lock(mutex_);
        connected_ = (XT_TCP_STATUS_OK == stat);
        cv_.notify_one();
    }

    void client_t::on_receive(xt_tcp_status_t stat, void *buf, size_t bytes_transferred)
    {
        bool will_close = false;
        bool close_if_error = false;

        if ((XT_TCP_STATUS_OK == stat) && (bytes_transferred > 0))
        {
            scoped_lock _lock(mutex_);
            if (read_header == read_state_)
            {
                if ((bytes_transferred != sizeof(tcp_session::msg_header_t)) 
                    || (0 == header_.data_size)
                    || !start_read_body(_lock))
                {
                    will_close = true;
                    close_if_error = true;
                }
                else
                {
                    read_state_ = read_body;
                }
            }
            else if (read_body == read_state_)
            {
                if (bytes_transferred != header_.data_size)
                {
                    will_close = true;
                    close_if_error = true;
                }
                else
                {
                    on_read_completion(_lock, true);
                    start_read(_lock);
                }
            }
            else
            {
                will_close = true;
                close_if_error = true;
            }
        }
        else
        {
            if (0 == bytes_transferred)
            {
                will_close = true;
                close_if_error = false;
            }
            else
            {
                will_close = true;
                close_if_error = true;
            }
        }

        if (will_close)
        {
            scoped_lock _lock(mutex_);
            on_read_completion(_lock, false);
            on_close(_lock, close_if_error);
        }
    }

    void client_t::on_close(scoped_lock &, bool close_if_error)
    {
        connected_ = false;

        if (cb_)
        {
            cb_(ctx_, close_if_error);
        }
    }
}

