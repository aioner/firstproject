#ifndef _ASYNC_TASK_H_INCLUDED
#define _ASYNC_TASK_H_INCLUDED

#include "spinlock.h"

#include <map>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace xt_rtsp_client
{
    namespace detail
    {
        template<typename T, bool>
        struct param_helper_t
        {
            typedef const T& type;
        };

        template<typename T>
        struct param_helper_t<T, true>
        {
            typedef T type;
        };

        template<typename T>
        struct param_t : param_helper_t<T, sizeof(T) <= sizeof(void *)>{};
    }

    struct boost_ptime_traits
    {
        typedef boost::posix_time::ptime time_point;
        typedef boost::posix_time::milliseconds milliseconds;

        static time_point local_time()
        {
            return boost::posix_time::microsec_clock::local_time();
        }
    };

    class async_task_t
    {
    public:
        typedef boost_ptime_traits time_traits;

        template<typename DuringT>
        async_task_t(const DuringT &d)
            :deadline_(time_traits::local_time() + d)
        {}

        virtual void done(void *response) = 0;
        virtual void release() = 0;

        bool is_overtime_task(const time_traits::time_point& now) const
        {
            return (now >= deadline_);
        }
    protected:
        virtual ~async_task_t() {}
        time_traits::time_point deadline_;
    };

    template<typename KeyT, typename MutexT = spinlock_t>
    class async_task_mgr_t
    {
    public:
        typedef KeyT key_type;
        typedef async_task_t *task_type;
        typedef std::map<key_type, task_type> tasks_map_type;
        typedef async_task_t::time_traits time_traits;
        typedef MutexT mutex_type;
        typedef typename detail::param_t<key_type>::type key_param_t;

        bool request_task(key_param_t key, task_type task)
        {
            scoped_lock _lock(mutex_);

            typename tasks_map_type::iterator it = tasks_.find(key);
            if (tasks_.end() != it)
            {
                return false;
            }

            return tasks_.insert(typename tasks_map_type::value_type(key, task)).second;
        }

        bool response_task(key_param_t key, void *response)
        {
            scoped_lock _lock(mutex_);

            typename tasks_map_type::iterator it = tasks_.find(key);
            if (tasks_.end() == it)
            {
                return false;
            }

            task_done(it->second, response);
            tasks_.erase(it);

            return true;
        }

        bool cancel_task(key_param_t key)
        {
            scoped_lock _lock(mutex_);

            typename tasks_map_type::iterator it = tasks_.find(key);
            if (tasks_.end() == it)
            {
                return false;
            }

            task_done(it->second, NULL);
            tasks_.erase(it);

            return true;
        }

        std::size_t check_overtime_tasks()
        {
			std::size_t overtime_task_count = 0;

            scoped_lock _lock(mutex_);
            typename time_traits::time_point now = time_traits::local_time();
            for (typename tasks_map_type::iterator it = tasks_.begin(); tasks_.end() != it; )
            {
                assert(NULL != it->second);
                if (it->second->is_overtime_task(now))
                {
                    task_done(it->second, NULL);
                    tasks_.erase(it++);

                    overtime_task_count++;
                }
                else
                {
                    ++it;
                }
            }

            return overtime_task_count;
        }

    private:
        void task_done(task_type& task, void *response)
        {
            task->done(response);
            task->release();
        }

        tasks_map_type tasks_;
        mutex_type mutex_;
        typedef typename mutex_type::scoped_lock scoped_lock;
    };
}

#endif //_ASYNC_TASK_H_INCLUDED
