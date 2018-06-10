#ifndef _THREAD_H_INCLUDED
#define _THREAD_H_INCLUDED

#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <memory>

#include "compat.h"

namespace xt_media_client
{
    template<typename Drived>
    class thread_base_t
    {
    public:
        typedef Drived drived_type;
        typedef thread_base_t<Drived> this_type;

        void start_thread()
        {
            thread_.reset(new (std::nothrow) boost::thread(&this_type::thread_worker, this));
        }

        void close_thread()
        {
            if (is_running())
            {
                thread_->join();
                thread_.reset();
            }
        }

        bool is_running() const
        {
            return (NULL != thread_.get());
        }

    protected:
        void on_thread_run();

    private:
        void thread_worker()
        {
            get_drived()->on_thread_run();
        }

        drived_type *get_drived()
        {
            return static_cast<drived_type *>(this);
        }

        std::auto_ptr<boost::thread> thread_;
    };

    class repeat_thread : public thread_base_t<repeat_thread>
    {
    public:
        typedef thread_base_t<repeat_thread> base_t;

        repeat_thread();
        ~repeat_thread();

        void start_thread();
        void close_thread();

        void on_thread_run();
    protected:
        virtual void on_repeat() = 0;
    private:

        boost::atomic_bool quit_flag_;
    };

    inline void sleep_yield(int ms)
    {
        ::msec_sleep(ms);
        ::sched_yield();
    }
}

#endif //_THREAD_H_INCLUDED
