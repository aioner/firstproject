#include "timer.h"
#include <boost/bind.hpp>
#include <boost/bind/placeholders.hpp>

namespace udp_session
{
    static void async_wait_handler(timer_t* pt, const boost::system::error_code& e)
    {
        pt->on_timer(boost::asio::error::operation_aborted == e);
        pt->async();
    }

    timer_t::timer_t()
        :timer_impl_(NULL)
    {}

    timer_t::~timer_t()
    {
        if (timer_impl_)
        {
            delete timer_impl_;
            timer_impl_ = NULL;
        }
    }

    void timer_t::init(boost::asio::io_service&service, uint32_t repeat_millisec)
    {
        if (NULL == timer_impl_)
        {
            timer_impl_ = new boost::asio::deadline_timer(service);
            repeat_millisec_ = repeat_millisec;

            async();
        }
    }

    void timer_t::cancel()
    {
        if (timer_impl_)
        {
            timer_impl_->cancel();
        }
    }

    void timer_t::async()
    {
        if (timer_impl_)
        {
            timer_impl_->expires_from_now(boost::posix_time::millisec(repeat_millisec_));
            timer_impl_->async_wait(boost::bind(&async_wait_handler, this, _1));
        }
    }
}
