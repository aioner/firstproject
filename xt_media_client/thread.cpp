#include "thread.h"

namespace xt_media_client
{
    repeat_thread::repeat_thread()
        :base_t(),
        quit_flag_(false)
    {}

    repeat_thread::~repeat_thread()
    {
        close_thread();
    }

    void repeat_thread::start_thread()
    {
        quit_flag_ = false;
        base_t::start_thread();
    }

    void repeat_thread::close_thread()
    {
        quit_flag_ = true;
        base_t::close_thread();
    }

    void repeat_thread::on_thread_run()
    {
        while (!quit_flag_)
        {
            on_repeat();
            sleep_yield(1);
        }
    }
}
