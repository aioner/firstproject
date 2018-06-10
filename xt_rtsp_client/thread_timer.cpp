#include "thread_timer.h"

namespace xt_rtsp_client
{
    base_timer::base_timer()
        :interval_(0),
        expires_()
    {}

    void base_timer::set_interval(uint32_t interval)
    {
        interval_ = interval;
        expires_ = clock_type::local_time() + boost::posix_time::milliseconds(interval_);
    }

    void base_timer::cancel()
    {}

    void base_timer::do_timer()
    {
        if (interval_ > 0)
        {
            time_point_type now = clock_type::local_time();
            if (now >= expires_)
            {
                on_timer();
                expires_ = now + boost::posix_time::milliseconds(interval_);
            }
        }
    }

    void thread_timer::close()
    {
        repeat_thread::close_thread();
        base_timer::cancel();
    }

    void thread_timer::on_repeat()
    {
        base_timer::do_timer();
    }

    void thread_timer::set_interval(uint32_t interval)
    {
        base_timer::set_interval(interval);
        start_thread();
    }
}

