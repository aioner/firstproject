#ifndef _MTS_FRAMEWORK_TASK_H_INCLUDED
#define _MTS_FRAMEWORK_TASK_H_INCLUDED

#include "utility/noncopyable.h"
#include "utility/thread.h"
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include <memory>
#include <ctime>

namespace framework
{
    namespace detail
    {
        class task_thread;
        class task_timeout_timer;
    }

    enum task_thread_status
    {
        status_task_valid = 0,
        status_task_new,
        status_task_run_begin,
        status_task_waiting,
        status_task_begin,
        status_task_end,
        status_task_run_end,
        status_task_destroy
    };

    struct task_thread_info_type
    {
        utility::thread::id tid;
        uint32_t deferred_task_num;
        uint32_t immediate_task_num;
        task_thread_status status;
        std::time_t point;
    };

    struct task_sched_info_type
    {
        typedef std::vector<task_thread_info_type> task_thread_info_container_type;
        task_thread_info_container_type threads_info;
    };

    class task_base : private utility::noncopyable
    {
    public:
        enum signal_type
        {
            deferred,       //可能有延迟
            immediate       //立刻调用
        };

        virtual void signal(signal_type type = deferred, uint32_t sequence = 0);
        bool cancel();

        static bool query_task_sched_info(task_sched_info_type& task_sched_info);
    protected:
        friend class detail::task_thread;
        friend class detail::task_timeout_timer;

        task_base();
		virtual ~task_base() {};
        virtual uint32_t run() = 0;
    private:
        void do_run_result(uint32_t);
        class impl_type;
#ifdef _WIN32
        std::tr1::shared_ptr<impl_type> impl_;
#else
		boost::shared_ptr<impl_type> impl_;
#endif
    };
}
#endif//_MTS_FRAMEWORK_TASK_H_INCLUDED

