#ifndef _MSEC_TIMER_H_INCLUDED
#define _MSEC_TIMER_H_INCLUDED

#include <stdint.h>
#include <map>

#ifdef _WIN32
#include <wtypes.h>
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")
#else
#include <signal.h>
#include <time.h>
#endif

#include "boost/smart_ptr/detail/spinlock.hpp"

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
    typedef MMRESULT id_type;

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

    static uint32_t get_tick_count()
    {
        return ::timeGetTime();
    }

private:
    static void CALLBACK s_timer_func(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
    {
        reinterpret_cast<timer_callback_t *>(dwUser)->on_timer();
    }

    id_type id_;
};

typedef winmm_timer_t msec_timer_t;
#else
class posix_timer_t
{
public:
    typedef timer_t id_type;

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

    static uint32_t get_tick_count()
    {
        struct timeval tv = { 0 };
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000U + tv.tv_usec * 0.001;
    }
private:
    static void s_timer_func(sigval_t t)
    {
        reinterpret_cast<timer_callback_t *>(t.sival_ptr)->on_timer();
    }

    id_type id_;
};

typedef posix_timer_t msec_timer_t;

#endif

class deadline_timer_callback_t
{
public:
    virtual uint32_t on_expires() = 0;

protected:
    virtual ~deadline_timer_callback_t() {}
};

class deadline_timer_mgr_t : public timer_callback_t, private msec_timer_t
{
public:
    typedef std::map<deadline_timer_callback_t *, uint32_t> cb_map_t;

    deadline_timer_mgr_t()
    {}

    bool start(uint32_t basic_priod)
    {
        return msec_timer_t::start(basic_priod, this);
    }

    void stop()
    {
        msec_timer_t::stop();
    }

    void add_timer(deadline_timer_callback_t *cb)
    {
        boost::lock_guard<boost::detail::spinlock> guard(lock_);

        cbs_.insert(cb_map_t::value_type(cb, 0));
    }

    void remove_timer(deadline_timer_callback_t *cb)
    {
        boost::lock_guard<boost::detail::spinlock> guard(lock_);

        cbs_.erase(cb);
    }

private:
    void on_timer()
    {
        boost::lock_guard<boost::detail::spinlock> guard(lock_);

        uint32_t now = get_tick_count();
        for (cb_map_t::iterator it = cbs_.begin(); cbs_.end() != it;)
        {
            bool flag = false;
            if (now >= it->second)
            {
                uint32_t expires = it->first->on_expires();
                if (expires > 0)
                {
                    it->second = now + expires;
                }
                else
                {
                    flag = true;
                }
            }

            if (flag)
            {
                cbs_.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    cb_map_t cbs_;
    boost::detail::spinlock lock_;
};

#endif //_MSEC_TIMER_H_INCLUDED
