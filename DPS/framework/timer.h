#ifndef _TIMER_H_INCLUDED
#define _TIMER_H_INCLUDED

#include <stdint.h>
#include <queue>
#include "boost/function.hpp"

#ifdef _WIN32
#include <wtypes.h>
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")
#else
#include <signal.h>
#include <time.h>
#endif

#include "boost/thread/synchronized_value.hpp"

namespace framework
{
    typedef int64_t tick_count_t;

    inline tick_count_t get_tick_count()
    {
        int64_t ticks = 0;
#ifdef _WIN32
        LARGE_INTEGER qpcnt;
        QueryPerformanceCounter(&qpcnt);
        ticks = qpcnt.QuadPart;

#elif defined(_LINUX)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        ticks = 1000000000LL * static_cast<int64_t>(ts.tv_sec) + static_cast<int64_t>(ts.tv_nsec);
#elif defined(_MAC)
        static mach_timebase_info_data_t timebase;
        if (timebase.denom == 0) 
        {
            // Get the timebase if this is the first time we run.
            // Recommended by Apple's QA1398.
            kern_return_t retval = mach_timebase_info(&timebase);
            if (retval != KERN_SUCCESS) 
            {
                // TODO(wu): Implement CHECK similar to chrome for all the platforms.
                // Then replace this with a CHECK(retval == KERN_SUCCESS);
#ifndef _IOS
                asm("int3");
#else
                __builtin_trap();
#endif
            }
        }
        // Use timebase to convert absolute time tick units into nanoseconds.
        ticks = mach_absolute_time() * timebase.numer / timebase.denom;

#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ticks = 1000000LL * static_cast<int64_t>(tv.tv_sec) + static_cast<int64_t>(tv.tv_usec);
#endif

#ifdef _WIN32
        LARGE_INTEGER qpfreq;
        QueryPerformanceFrequency(&qpfreq);
        return (ticks * 1000) / qpfreq.QuadPart;
#elif defined(_LINUX) || defined(_MAC)
        return ticks / 1000000LL;
#else
        return ticks / 1000LL;
#endif
    }

    class timer_callback_t
    {
    public:
        virtual void on_timer() = 0;
    protected:
        virtual ~timer_callback_t() {}
    };

#ifdef _WIN32
    class winmm_timer_t
    {
    public:
        typedef MMRESULT id_t;

        winmm_timer_t()
            :id_(0)
        {}

        bool start(uint32_t ms, timer_callback_t *cb)
        {
            id_ = ::timeSetEvent(ms, 1, &winmm_timer_t::s_timer_func, reinterpret_cast<DWORD_PTR>(cb), TIME_PERIODIC);
            return (0 != id_);
        }

        void stop()
        {
            if (0 != id_)
            {
                ::timeKillEvent(id_);
                id_ = 0;
            }
        }

        bool stoped() const
        {
            return (0 == id_);
        }

    private:
        static void CALLBACK s_timer_func(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
        {
            reinterpret_cast<timer_callback_t *>(dwUser)->on_timer();
        }

        id_t id_;
    };

    typedef winmm_timer_t timer_t;
#elif defined(_MAC)
#include <dispatch/dispatch.h>
    class dispatch_source_timer_t
    {
    public:
        typedef dispatch_source_t id_t;

        dispatch_source_timer_t()
            :timer_(dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)))
        {
            dispatch_source_set_event_handler_f(timer_, &dispatch_source_timer_t::timer_func);
        }

        ~dispatch_source_timer_t()
        {
            dispatch_release(timer_);
        }

        bool start(uint32_t ms, timer_callback_t * cb)
        {
            dispatch_set_context(timer_, cb);
            dispatch_source_set_timer(timer_, DISPATCH_TIME_NOW, 1000000 * ms, 1000);
            dispatch_resume(timer_);
            return true;
        }

        void stop()
        {
            dispatch_source_cancel(timer_);
        }
    private:
        static void timer_func(void *p)
        {
            reinterpret_cast<timer_callback_t *>(p)->on_timer();
        }

        id_t timer_;
    };

    typedef dispatch_source_timer_t timer_t;
#else
    class posix_timer_t
    {
    public:
        typedef timer_t id_t;

        posix_timer_t()
            :id_(0)
        {}

        bool start(uint32_t ms, timer_callback_t *cb)
        {
            struct sigevent sige = { 0 };
            sige.sigev_notify          = SIGEV_THREAD;
            sige.sigev_notify_function = &posix_timer_t::s_timer_func;
            sige.sigev_value.sival_ptr = cb;

            ::timer_create(CLOCK_REALTIME, &sige, &id_);

            struct itimerspec its = { 0 };

            its.it_value.tv_sec  = (ms / 1000);
            its.it_value.tv_nsec = (ms % 1000 ) * 1000 * 1000;
            its.it_interval = its.it_value;

            return (0 == ::timer_settime(id_, 0, &its, 0));
        }

        void stop()
        {
            if (0 != id_)
            {
                struct itimerspec its = { 0 };
                ::timer_settime(id_, 0, &its, 0);
                ::timer_delete(id_);
                id_ = 0;
            }
        }

    private:
        static void s_timer_func(sigval_t t)
        {
            reinterpret_cast<timer_callback_t *>(t.sival_ptr)->on_timer();
        }

        id_t id_;
    };

    typedef posix_timer_t timer_t;
#endif

    class once_timer_t : public timer_callback_t, private timer_t
    {
    public:
        typedef boost::function0<void> work_t;

        struct with_deadline_work_t
        {
            work_t work_;
            tick_count_t deadline_;

            template<typename F>
            with_deadline_work_t(F f, tick_count_t t)
                :work_(f),
                deadline_(t)
            {}

            bool operator < (const with_deadline_work_t&rhs) const
            {
                return deadline_ > rhs.deadline_;
            }
        };

        once_timer_t()
        {}

        ~once_timer_t()
        {
            close();
        }

        bool start(uint32_t basic_ms_period)
        {
            return timer_t::start(basic_ms_period, this);
        }

        void close()
        {
            timer_t::stop();
        }

        template<typename F>
        void submit(F f, uint32_t ms)
        {
            return works_->push(with_deadline_work_t(f, get_tick_count() + ms));
        }

        template<typename F, typename E>
        void submit_on(F f, uint32_t ms, E& exec)
        {
            return works_->push(with_deadline_work_t(boost::bind(&E::template submit<F>, &exec, f, false), get_tick_count() + ms));
        }

    private:
        void on_timer()
        {
            tick_count_t now = get_tick_count();
			boost::strict_lock_ptr<std::priority_queue<framework::once_timer_t::with_deadline_work_t>, boost::recursive_mutex> slp = works_.synchronize();
            while (!slp->empty() && (now >= slp->top().deadline_))
            {
                slp->top().work_();
                slp->pop();
            }
        }

        boost::synchronized_value<std::priority_queue<with_deadline_work_t>, boost::recursive_mutex> works_;
    };
}

#endif //_TIMER_H_INCLUDED
