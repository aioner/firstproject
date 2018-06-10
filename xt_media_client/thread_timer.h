#ifndef _THREAD_TIMER_H_INCLUDED
#define _THREAD_TIMER_H_INCLUDED

#include "thread.h"

#include <boost/date_time.hpp>
#include <stdint.h>

namespace xt_media_client
{
    class base_timer 
    {
    public:
        typedef boost::posix_time::microsec_clock clock_type;
        typedef boost::posix_time::ptime time_point_type;

        base_timer();

        void set_interval(uint32_t interval);
        void cancel();

    protected:
        virtual void on_timer() = 0;
        void do_timer();

    private:
        uint32_t interval_;
        time_point_type expires_;
    };

    class thread_timer : private repeat_thread, public base_timer
    {
    public:
        void set_interval(uint32_t interval);
        void close();
    protected:
        using base_timer::on_timer;
        void on_repeat();
    };
}

#endif //_THREAD_TIMER_H_INCLUDED
