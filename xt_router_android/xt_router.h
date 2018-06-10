#ifndef _XT_ROUTER_H_INCLUDED
#define _XT_ROUTER_H_INCLUDED

#include "h_xt_router.h"
#include "h_xtmediaserver.h"
#include "xt_media_client.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/detail/lightweight_thread.hpp>
#include <stdint.h>
#include <vector>

#ifdef _WIN32
#include <wtypes.h>
#else
#include <sys/time.h>
#include <sched.h>
#include <unistd.h>
#endif

#define MAX_TRACK_NUM					8

namespace xt_router
{
#ifdef _WIN32
    typedef DWORD msec_t;
    inline msec_t get_msec()
    {
        return ::GetTickCount();
    }

    inline void sched_yield()
    {
        if(!::SwitchToThread())
        {
            ::Sleep(0);
        }
    }

    inline void msec_sleep(int msec)
    {
        ::Sleep(msec);
    }
#else

    typedef long msec_t;
    inline msec_t get_msec()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec * 1000 + tv.tv_usec * 0.001);
    }

    inline void msec_sleep(int msec)
    {
        ::usleep(msec * 1000);
    }
#endif

    class repeat_thread_t
    {
    public:
        repeat_thread_t()
            :handle_(NULL),
            quit_flag_(false)
        {}

        int start_thread()
        {
            quit_flag_ = false;
            return pthread_create(&handle_, NULL, &repeat_thread_t::thread_worker, this);
        }

        int close_thread()
        {
            quit_flag_ = true;
            return pthread_join(handle_, NULL);
        }

    protected:
        virtual void on_repeat() = 0;
    private:

#ifdef BOOST_HAS_PTHREADS
        static void *thread_worker(void *pv)
#else
        static unsigned __stdcall thread_worker(void *pv)
#endif
        {
            repeat_thread_t *pt = reinterpret_cast<repeat_thread_t *>(pv);
            if (NULL != pt)
            {
                while (!pt->quit_flag_)
                {
                    pt->on_repeat();
                    msec_sleep(1);
                    sched_yield();
                }
            }
            return 0;
        }

        pthread_t handle_;
        bool quit_flag_;
    };

    class base_timer_t
    {
    public:
        base_timer_t()
            :interval_(0),
            expires_()
        {}

        void set_interval(uint32_t interval)
        {
            interval_ = interval;
            expires_  = get_msec() + interval;
        }

        void cancel()
        {}

    protected:
        virtual void on_timer() = 0;
        void do_timer()
        {
            if (interval_ > 0)
            {
                msec_t now = get_msec();
                if (now >= expires_)
                {
                    on_timer();
                    expires_ = now + interval_;
                }
            }
        }

    private:
        uint32_t interval_;
        msec_t expires_;
    };

    class thread_timer_t : private repeat_thread_t, public base_timer_t
    {
    public:
        void set_interval(uint32_t interval)
        {
            base_timer_t::set_interval(interval);
            repeat_thread_t::start_thread();
        }

        void close()
        {
            repeat_thread_t::close_thread();
            base_timer_t::cancel();
        }

    protected:
        using base_timer_t::on_timer;
        void on_repeat()
        {
            base_timer_t::do_timer();
        }
    };

    class item_t
    {
    protected:
        item_t()
            :index_(-1)
        {}
    protected:
        template<typename T, typename U>
        friend class array_t;

        int32_t index_;
    };

    template<typename T, typename = typename boost::enable_if<boost::is_base_of<item_t, T> >::type >
    class array_t : public boost::ptr_vector<T>
    {
    public:
        typedef boost::ptr_vector<T> impl_t;
        using typename impl_t::value_type;
        using typename impl_t::auto_type;

        void push_back(value_type x)
        {
            x->index_ = this->size();
            impl_t::push_back(x);
        }

        bool erase_if(value_type x)
        {
            if (!x || (-1 == x->index_) || (x->index_ >= static_cast<int32_t>(this->size())))
            {
                return false;
            }

            assert(x == this->base()[x->index_]);

            auto_type item = this->pop_back();
            if (item->index_ != x->index_)
            {
                item->index_ = x->index_;
                this->replace(x->index_, item.release());
            }
            return true;
        }
    };

    struct route_info_t : public item_t
    {
        int srcno;
        int chan;
        xt_media_link_handle_t link;

        media_cb cb;
        void *ctx;

        msec_t deadline;

        int frame_type_to_trackids[MAX_TRACK_NUM];

        uint8_t priority;

        explicit route_info_t()
            :srcno(-1),
            chan(-1),
            link(NULL),
            cb(NULL),
            ctx(NULL),
            deadline(0),
            priority(0)
        {
            std::fill_n(frame_type_to_trackids, MAX_TRACK_NUM, -1);
        }

        void update_deadline(uint32_t timeout)
        {
            deadline = get_msec() + timeout;
        }

        bool is_expired(msec_t now)
        {
            return (0 != deadline) && (now >= deadline);
        }
    };

    class route_info_mgr_t : private thread_timer_t
    {
    public:
        void register_data_break_callback(uint32_t timeout, rt_data_break_callback_t cb, void *ctx)
        {
            {
                scoped_lock _lock(mutex_);
                timeout_ = timeout;
                cb_ = cb;
                ctx_ = ctx;
            }

            thread_timer_t::set_interval(timeout * 0.1);
        }

        void add_route_info(route_info_t *x, bool update_deadline = true)
        {
            scoped_lock _lock(mutex_);
            rts_.push_back(x);

            if (update_deadline)
            {
                update_rt_deadline(x);
            }
        }

        void del_route_info(route_info_t *x)
        {
            scoped_lock _lock(mutex_);
            rts_.erase_if(x);
        }

        uint32_t get_timeout() const
        {
            return timeout_;
        }

        void update_rt_deadline(route_info_t *rt)
        {
            if (rt && cb_ && timeout_ > 0)
            {
                rt->update_deadline(timeout_);
            }
        }

        static route_info_mgr_t& instance()
        {
            return s_inst_;
        }
    private:
        static route_info_mgr_t s_inst_;

        route_info_mgr_t()
            :rts_(),
            mutex_(),
            timeout_(0),
            cb_(NULL),
            ctx_(NULL)
        {}

        typedef std::vector<route_info_t *> route_info_list_t;
        void on_timer()
        {
            route_info_list_t del_rt_list;

            get_deadline_rt(del_rt_list);

            while (!del_rt_list.empty())
            {
                route_info_t *rt = del_rt_list.back();
                if (0 != cb_(ctx_, rt))
                {
                    update_rt_deadline(rt);
                }
                del_rt_list.pop_back();
            }
        }

        void get_deadline_rt(route_info_list_t& rts)
        {
            msec_t now = get_msec();

            scoped_lock _lock(mutex_);
            if (cb_)
            {
                for (std::size_t ii = 0; ii < rts_.size(); ++ii)
                {
                    route_info_t *rt = reinterpret_cast<route_info_t *>(rts_.base()[ii]);
                    if (rt && rt->is_expired(now))
                    {
                        rts.push_back(rt);
                    }
                }
            }
        }

        array_t<route_info_t> rts_;

        boost::detail::spinlock mutex_;
        typedef boost::detail::spinlock::scoped_lock scoped_lock;

        uint32_t timeout_;
        rt_data_break_callback_t cb_;
        void *ctx_;
    };
}

#endif //_XT_ROUTER_H_INCLUDED
