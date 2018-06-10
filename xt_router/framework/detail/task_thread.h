#ifndef _MTS_FRAMEWORK_TASK_THREAD_H_INCLUDED
#define _MTS_FRAMEWORK_TASK_THREAD_H_INCLUDED

#include "../../framework/config.h"
#include "thread_creator.h"
#include "task_scheduler.h"
#include "utility/atomic.h"
#include "utility/condition_variable.h"
#include "utility/common_macro.h"
#include "utility/chrono.h"
#include <ctime>

namespace framework
{
    namespace detail
    {
        class task_thread : 
            public detail::task_scheduler,
            public detail::thread_creator<detail::threadgroup_create_policy>::static_<task_thread>
        {
        public:
            typedef utility::thread_group thread_group_type;

            explicit task_thread(thread_group_type *tg)
                :
#ifdef _QUERY_TASK_THREAD_STATUS
                task_thread_status_(status_task_new),
                task_thread_time_point_(0),
                task_thread_id_(),
#endif
                quit_flag_(false)
            {
                set_threadgroup(tg);
            }

            ~task_thread()
            {
                shutdown();
            }

            void shutdown()
            {
                try_stop_thread();
                task_add_notifier_.notify_all();
#ifdef _QUERY_TASK_THREAD_STATUS
                task_thread_status_ = status_task_destroy;
#endif
            }

            class set_task_thread_status_at_exit
            {
            public:
                set_task_thread_status_at_exit(task_thread *ptt, task_thread_status status)
                    :task_thread_(ptt),
                    status_at_exit_(status)
                {}

                ~set_task_thread_status_at_exit()
                {
                    task_thread_->set_task_thread_status(status_at_exit_);
                }

            private:
                task_thread *task_thread_;
                task_thread_status status_at_exit_;
            };

            void on_run_thread()
            {
#ifdef _QUERY_TASK_THREAD_STATUS
                task_thread_id_ = utility::this_thread::get_id();
#endif
                //1.run begin
                set_task_thread_status(status_task_run_begin);

                set_task_thread_status_at_exit _at_exit(this, status_task_run_end);

                while (!quit_flag_)
                {
                    //2.waiting for task
                    set_task_thread_status(status_task_waiting);
                    task_base_ptr task = NULL;
                    {
                        utility::mutex::scoped_lock lock(task_access_mtx_);

                        while (!(task = wait_for_task()))
                        {
                            task_add_notifier_.wait(task_access_mtx_);
                            if (quit_flag_)
                            {
                                return ;
                            }
                        }
                    }

                    //3.do task begin
                    set_task_thread_status(status_task_begin);
                    uint32_t ret = task->run();
                    if (ret > 0)
                    {
                        task->do_run_result(ret);
                    }
                    else
                    {
                        //todo:
                    }

                    //4.do task end
                    set_task_thread_status(status_task_end);

                    utility::this_thread::sleep_for(utility::chrono::milliseconds(1));
                }
                //5.run end
            }

            void try_stop_thread()
            {
                quit_flag_ = true;
            }

            void add_deferred_task(task_base_ptr task)
            {
                utility::mutex::scoped_lock lock(task_access_mtx_);
                detail::task_scheduler::add_deferred_task(task);
                task_add_notifier_.notify_all();
            }

            void add_immediate_task(task_base_ptr task)
            {
                utility::mutex::scoped_lock lock(task_access_mtx_);
                detail::task_scheduler::add_immediate_task(task);
                task_add_notifier_.notify_all();
            }

            bool erase_task(task_base_ptr task)
            {
                utility::mutex::scoped_lock lock(task_access_mtx_);
                return detail::task_scheduler::erase_task(task);
            }

            task_thread_status get_task_thread_status() const
            {
#ifdef _QUERY_TASK_THREAD_STATUS
                return task_thread_status_;
#else
                return status_task_valid;
#endif
            }

            std::time_t get_task_thread_time_point() const
            {
#ifdef _QUERY_TASK_THREAD_STATUS
                return task_thread_time_point_;
#else
                return 0;
#endif
            }

            utility::thread::id get_task_thread_id() const
            {
#ifdef _QUERY_TASK_THREAD_STATUS
                return task_thread_id_;
#else
                return utility::thread::id();
#endif
            }

            std::size_t get_deferred_task_num() const
            {
                utility::mutex::scoped_lock lock(task_access_mtx_);
                return detail::task_scheduler::get_deferred_task_num();
            }

            std::size_t get_immediate_task_num() const
            {
                utility::mutex::scoped_lock lock(task_access_mtx_);
                return detail::task_scheduler::get_immediate_task_num();
            }
        private:
            void set_task_thread_status(task_thread_status status)
            {
#ifdef _QUERY_TASK_THREAD_STATUS
                task_thread_status_ = status;
                task_thread_time_point_ = std::time(NULL);
#endif
            }

            mutable utility::mutex task_access_mtx_;
            utility::condition_variable_any task_add_notifier_;

#ifdef _QUERY_TASK_THREAD_STATUS
            utility::atomic<task_thread_status> task_thread_status_;
            utility::atomic<std::time_t> task_thread_time_point_;   //步骤发生时间点（秒）
            utility::thread::id task_thread_id_;
#endif

            utility::atomic_bool quit_flag_;

            using detail::task_scheduler::erase_deferred_task;
            using detail::task_scheduler::erase_immediate_task;
        };
    }
}

#endif //_MTS_FRAMEWORK_TASK_THREAD_H_INCLUDED