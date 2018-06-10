#ifndef _TIMER_H_INCLUDED
#define _TIMER_H_INCLUDED

#include <boost/asio/deadline_timer.hpp>
#include<stdint.h>

namespace udp_session
{
    class timer_t
    {
    public:
        timer_t();
        ~timer_t();

        virtual void on_timer(bool operation_aborted) = 0;
        void init(boost::asio::io_service&service, uint32_t repeat_millisec);
        void cancel();

        void async();
    private:
        boost::asio::deadline_timer *timer_impl_;
        uint32_t repeat_millisec_;
    };
}

#endif //_TIMER_H_INCLUDED
