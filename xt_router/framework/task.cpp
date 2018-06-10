#include "task.h"
#include "../framework/deadline_timer.h"
#include "detail/task_scheduler_service.h"
#include "utility/mutex.h"
#include "utility/memory_pool.h"

namespace framework
{
    namespace detail
    {
        class task_timeout_timer : public deadline_timer
        {
        public:
            explicit task_timeout_timer(task_base::impl_type *task_impl)
                :task_impl_(task_impl)
            {}

            void on_time_expires(bool cancel_flag);

        private:
            task_base::impl_type *task_impl_;
        };
    }

    typedef utility::singleton_object_pool<detail::task_timeout_timer> task_timeout_timer_pool;

    class task_base::impl_type
    {
    public:
        typedef detail::task_scheduler_service::scheduler_type scheduler_type;
        impl_type()
            :scheduler_(NULL),
            task_(NULL),
            timeout_timer_(NULL)
        {}

        void set_task(task_base *task)
        {
            task_ = task;
        }

        task_base *get_task()
        {
            return task_;
        }

        void signal(signal_type type, uint32_t sequence = 0)
        {
            switch (type)
            {
            case deferred:
                get_scheduler(sequence)->add_deferred_task(get_task());
                break;
            case immediate:
                get_scheduler(sequence)->add_immediate_task(get_task());
                break;
            }
        }

        bool cancel()
        {
            {
                utility::lock_guard<utility::recursive_mutex> lock(mtx_);
                if (NULL != timeout_timer_)
                {
                    timeout_timer_->cancel();
                    timeout_timer_ = NULL;
                }
            }
            return get_scheduler()->erase_task(get_task());
        }

        scheduler_type *get_scheduler(uint32_t sequence = 0)
        {
            if ((NULL == scheduler_) || (0 == sequence))
            {
                scheduler_ = detail::task_scheduler_service::instance()->get_scheduler(sequence);
            }
            return scheduler_;
        }

        void do_run_result(uint32_t result)
        {
            utility::lock_guard<utility::recursive_mutex> lock(mtx_);
            timeout_timer_ = new detail::task_timeout_timer(this);
            //task_timeout_timer_pool::construct(this);
            assert(NULL != timeout_timer_);
            timeout_timer_->expires_from_now(result);
            timeout_timer_->start();
        }

        void clear_run_result()
        {
            utility::lock_guard<utility::recursive_mutex> lock(mtx_);
            timeout_timer_ = NULL;
        }

    private:
        utility::recursive_mutex mtx_;
        scheduler_type *scheduler_;
        task_base *task_;
        detail::task_timeout_timer *timeout_timer_;
    };

    task_base::task_base()
        :impl_(new impl_type)
    {
        impl_->set_task(this);
    }

    void task_base::signal(signal_type type, uint32_t sequence)
    {
        impl_->signal(type, sequence);
    }

    bool task_base::cancel()
    {
        return impl_->cancel();
    }

    void task_base::do_run_result(uint32_t result)
    {
        impl_->do_run_result(result);
    }

    //static
    bool task_base::query_task_sched_info(task_sched_info_type& task_sched_info)
    {
        return detail::task_scheduler_service::instance()->query_task_sched_info(task_sched_info);
    }

    namespace detail
    {
        void task_timeout_timer::on_time_expires(bool cancel_flag)
        {
            ASSERT(NULL != task_impl_);
            if (!cancel_flag)
            {
                task_impl_->signal(task_base::immediate);
                task_impl_->clear_run_result();
            }
            //task_timeout_timer_pool::destroy(this);
            delete this;
        }
    }
}
