#include "task_scheduler.h"

namespace framework
{
    namespace detail
    {
        void task_scheduler::add_deferred_task(task_base_ptr task)
        {
            deferred_tasks_.push(task);
        }

        void task_scheduler::add_immediate_task(task_base_ptr task)
        {
            immediate_tasks_.push(task);
        }

        bool task_scheduler::erase_deferred_task(task_base_ptr task)
        {
            return deferred_tasks_.erase(task);
        }

        bool task_scheduler::erase_immediate_task(task_base_ptr task)
        {
            return immediate_tasks_.erase(task);
        }

        bool task_scheduler::erase_task(task_base_ptr task)
        {
            if (erase_deferred_task(task))
            {
                return true;
            }

            if (erase_immediate_task(task))
            {
                return true;
            }
            return false;
        }

        task_scheduler::task_base_ptr task_scheduler::wait_for_task()
        {
            task_base_ptr task = NULL;

            do
            {
                //优先从堆从取任务
                if (!immediate_tasks_.empty())
                {
                    task = immediate_tasks_.top();
                    immediate_tasks_.pop();
                    break;
                }

                if (!deferred_tasks_.empty())
                {
                    task = deferred_tasks_.top();
                    deferred_tasks_.pop();
                    break;
                }
            }
            while (false);

            return task;
        }
    }
}