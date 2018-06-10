#ifndef _PACKAGED_TASK_H_INCLUDED
#define _PACKAGED_TASK_H_INCLUDED

#include <boost/thread/future.hpp>
#include <boost/move/move.hpp>
#include <queue>

#include "spinlock.h"

namespace xt_rtsp_client
{
    template<typename T>
    class queue_t : public std::queue<T>
    {};

    template<typename Result, typename Task = boost::packaged_task<Result>, template<typename> class Queue = queue_t, typename Mutex = spinlock_t>
    class packaged_task_queue_t : private Queue<Task *>
    {
    public:
        template<typename F>
        boost::unique_future<Result> add_task(const F& f)
        {
            scoped_lock _lock(mutex_);

            std::auto_ptr<Task> task(new (std::nothrow) Task(f));
            boost::unique_future<Result> fut = task->get_future();
            this->push(task.release());
            return boost::move(fut);
        }

        bool try_excuting_one()
        {
            scoped_lock _lock(mutex_);

            if (this->empty())
            {
                return false;
            }

            std::auto_ptr<Task> task(this->front());
            this->pop();
            (*task)();

            return true;
        }

    private:
        typedef typename Mutex::scoped_lock scoped_lock;
        mutable Mutex mutex_;
    };
}

#endif //_PACKAGED_TASK_H_INCLUDED
