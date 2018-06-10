#include "repeat_thread.h"
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#ifdef _WIN32
#include <wtypes.h>
#endif

namespace xt_rtsp_client
{
    repeat_thread::repeat_thread()
        :thread_quit_flag_(false),
        thread_()
    {}

    repeat_thread::~repeat_thread()
    {
        close_thread();
    }

    void repeat_thread::start_thread()
    {
        thread_quit_flag_ = false;
        thread_.reset(new boost::thread(&repeat_thread::thread_worker, this));
    }

    void repeat_thread::close_thread()
    {
        thread_quit_flag_ = true;
        if (thread_)
        {
            thread_->join();
        }
    }

    void repeat_thread::thread_worker()
    {
        while (!thread_quit_flag_)
        {
            on_repeat();
            sleep_yield(1);
        }
    }
}
