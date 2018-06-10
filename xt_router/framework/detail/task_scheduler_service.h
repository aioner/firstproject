#ifndef _MTS_FRAMEWORK_TASK_SCHEDULER_SERVICE_H_INCLUDED
#define _MTS_FRAMEWORK_TASK_SCHEDULER_SERVICE_H_INCLUDED

#include "task_thread.h"
#include "singleton.h"
#include "utility/thread.h"
#include "utility/mutex.h"
#ifndef _WIN32
#include <boost/shared_ptr.hpp>
#endif
#include <memory>

namespace framework
{
    namespace detail
    {
        class task_scheduler_service :
            public detail::singleton<task_scheduler_service>
        {
        public:
            typedef utility::thread_group thread_group_type;
            typedef task_thread scheduler_type;
#ifdef _WIN32
            typedef std::tr1::shared_ptr<scheduler_type> scheduler_ptr;
            typedef std::tr1::shared_ptr<thread_group_type> thread_group_ptr;
#else
            typedef boost::shared_ptr<scheduler_type> scheduler_ptr;
            typedef boost::shared_ptr<thread_group_type> thread_group_ptr;
#endif
            typedef std::vector<scheduler_ptr> scheduler_container_type;

            enum {  default_thread_num = 0 };

            scheduler_type *get_scheduler(uint32_t sequence = 0)
            {
                utility::write_lock lock(mtx_);
                if (0 == schedulers_.size())
                {
                    return NULL;
                }
                return schedulers_[get_scheduler_index(sequence)].get();
            }

            void shutdown()
            {
                {
                    utility::write_lock lock(mtx_);
                    for (scheduler_container_type::iterator iter = schedulers_.begin(); schedulers_.end() != iter; ++iter)
                    {
                        (*iter)->shutdown();
                    }
                }
                threadgroup_->join_all();
            }

            bool query_task_sched_info(task_sched_info_type& task_sched_info)
            {
                utility::read_lock lock(mtx_);

                if (!threadgroup_)
                {
                    return false;
                }

                task_sched_info_type::task_thread_info_container_type& threads_info = task_sched_info.threads_info;
                threads_info.resize(schedulers_.size());
                std::size_t index = 0;

                for (scheduler_container_type::iterator iter = schedulers_.begin(); schedulers_.end() != iter; ++iter)
                {
                    threads_info[index].tid = (*iter)->get_task_thread_id();
                    threads_info[index].status = (*iter)->get_task_thread_status();
                    threads_info[index].point = (*iter)->get_task_thread_time_point();
                    threads_info[index].deferred_task_num = (uint32_t)(*iter)->get_deferred_task_num();
                    threads_info[index].immediate_task_num = (uint32_t)(*iter)->get_immediate_task_num();
                    index++;
                }

                return true;
            }
        private:
            friend class singleton<task_scheduler_service>;

            task_scheduler_service(size_t thread_num = default_thread_num)
                :used_index_(0)
            {
                //默认配置
                if (default_thread_num == thread_num)
                {
                    thread_num = utility::thread::hardware_concurrency();
                }
                threadgroup_.reset(new thread_group_type);

                schedulers_.reserve(thread_num);
                for (size_t index = 0; index < thread_num; ++index)
                {
                    scheduler_ptr ptr(new scheduler_type(threadgroup_.get()));
                    if (ptr->start_thread())
                    {
                        schedulers_.push_back(ptr);
                    }
                }
            }

            std::size_t get_scheduler_index(uint32_t sequence)
            {
                std::size_t index = 0;
                if (0 == sequence)
                {
                    if (used_index_ >= schedulers_.size())
                    {
                        used_index_ = 0;
                    }
                    index = used_index_;
                    used_index_++;
                }
                else
                {
                    //modified by lichao, 20150723修正框架线程选择算法的bug
                    index = (sequence % schedulers_.size());
                }
                return index;
            }

            thread_group_ptr threadgroup_;
            scheduler_container_type schedulers_;
            std::size_t used_index_;

            utility::read_write_mutex mtx_;
        };
    }
}

#endif //_MTS_FRAMEWORK_TASK_SCHEDULER_SERVICE_H_INCLUDED
