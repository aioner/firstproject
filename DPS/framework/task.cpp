#include "framework/task.h"
#include "framework/core.h"

namespace framework
{

    void task_base_t::s_task_work(framework::task_base_t *task)
    {
        do 
        {
            if (NULL == task) break;
            if (task->cancel_flag_)
            {
                delete task;
                task = NULL;
                break;
            }
            uint32_t expires_from_now = task->run();
            if (task->cancel_flag_ || 0 == expires_from_now)
            {
                delete task;
                task = NULL;
                break;
            }
            if (expires_from_now > 0)
            {
                boost::function0<void> f = boost::bind(s_task_work, task);
                core::once_timer_submit_on(f, expires_from_now);
            }
        } while (0);
    }

    task_base_t::task_base_t()
        :cancel_flag_(false)
    {}

    void task_base_t::signal(signal_t type, uint32_t seq)
    {
        boost::function0<void> f = boost::bind(s_task_work, this);
        if (0 == seq)
        {
            core::thread_pool_submit(f, (deferred == type));
        }
        else
        {
            core::serial_executor_submit(f, seq);
        }
    }

    bool task_base_t::cancel()
    {
        cancel_flag_ = true;
        return true;
    }

    //static 
    bool task_base_t::query_task_sched_info(task_sched_info_t& task_sched_info)
    {
        return false;
    }
}
