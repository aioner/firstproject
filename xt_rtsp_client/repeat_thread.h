#ifndef _REPEAT_THREAD_H_INCLUDED
#define _REPEAT_THREAD_H_INCLUDED

#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#ifdef _WIN32
#include <wtypes.h>
inline void sched_yield()
{
    if(!::SwitchToThread())
    {
        ::Sleep(0);
    }
}
inline void msec_sleep(int ms)
{
    ::Sleep(ms);
}
#else
#include <sched.h>
#include <unistd.h>
inline void msec_sleep(int ms)
{
    ::usleep(ms * 1000);
}
#endif

namespace xt_rtsp_client
{
    class repeat_thread
    {
    public:
        repeat_thread();
        ~repeat_thread();

        void start_thread();
        void close_thread();

    protected:
        virtual void on_repeat() = 0;
    private:
        void thread_worker();

        boost::atomic_bool thread_quit_flag_;
        boost::shared_ptr<boost::thread> thread_;
    };

    inline void sleep_yield(int ms)
    {
        ::msec_sleep(ms);
        ::sched_yield();
    }
}

#endif //_REPEAT_THREAD_H_INCLUDED
