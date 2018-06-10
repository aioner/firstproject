#ifndef _MTS_FRAMEWORK_DETAIL_TASK_SCHEDULER_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_TASK_SCHEDULER_H_INCLUDED

#include "../task.h"
#include "utility/mutex.h"
#include <deque>
#include <algorithm>

namespace framework
{
    namespace detail
    {
        template<class T>
        class deque_scheduler
        {
        public:
            void push(const T& c)
            {
                container_.push_back(c);
            }

            bool empty() const
            {
                return container_.empty();
            }

            bool erase(const T& c)
            {
				typename std::deque<T>::iterator iter = std::find(container_.begin(), container_.end(), c);
                if (container_.end() == iter)
                {
                    return false;
                }

                container_.erase(iter);
                return true;
            }

            std::size_t size() const
            {
                return container_.size();
            }
        protected:
            std::deque<T> container_;
        };

        template<typename T>
        class fifo_scheduler : public deque_scheduler<T>
        {
        public:
            const T& top()
            {
                return deque_scheduler<T>::container_.front();
            }

            void pop()
            {
                deque_scheduler<T>::container_.pop_front();
            }
        };

        template<typename T>
        class lifo_scheduler : public deque_scheduler<T>
        {
        public:
            const T& top()
            {
                return deque_scheduler<T>::container_.back();
            }

            void pop()
            {
                deque_scheduler<T>::container_.pop_back();
            }
        };

        class task_scheduler
        {
        public:
            typedef task_base *task_base_ptr;
            typedef fifo_scheduler<task_base_ptr> deferred_tasks_type;
            typedef lifo_scheduler<task_base_ptr> immediate_tasks_type;

            void add_deferred_task(task_base_ptr task);
            void add_immediate_task(task_base_ptr task);
            bool erase_deferred_task(task_base_ptr task);
            bool erase_immediate_task(task_base_ptr task);
            bool erase_task(task_base_ptr task);

            std::size_t get_deferred_task_num() const
            {
                return deferred_tasks_.size();
            }
            std::size_t get_immediate_task_num() const
            {
                return immediate_tasks_.size();
            }
        protected:
            task_base_ptr wait_for_task();

            deferred_tasks_type deferred_tasks_;
            immediate_tasks_type immediate_tasks_;
        };
    }
}

#endif //_MTS_FRAMEWORK_DETAIL_TASK_SCHEDULER_H_INCLUDED