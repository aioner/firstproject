#ifndef _TASK_H_INCLUDED
#define _TASK_H_INCLUDED

#include "boost/thread.hpp"
#include "boost/atomic.hpp"
#include <stdint.h>

namespace framework
{
    enum task_thread_status_t
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

    struct task_thread_info_t
    {
        boost::thread::id tid;
        uint32_t deferred_task_num;
        uint32_t immediate_task_num;
        task_thread_status_t status;
        std::time_t point;
    };

    struct task_sched_info_t
    {
        typedef std::vector<task_thread_info_t> task_thread_info_container_t;
        task_thread_info_container_t threads_info;
    };

    class task_base_t
    {
    public:
        enum signal_t
        {
            deferred,       //可能有延迟
            immediate       //立刻调用
        };

        task_base_t();

        virtual void signal(signal_t type = deferred, uint32_t seq = 0);
        virtual uint32_t run() = 0;
        bool cancel();

        static bool query_task_sched_info(task_sched_info_t& task_sched_info);
    protected:


        virtual ~task_base_t() {}
    private:
        static void s_task_work(framework::task_base_t *task);

        boost::atomic_bool cancel_flag_;
    };
}

#endif //_TASK_H_INCLUDED
