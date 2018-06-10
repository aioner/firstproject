#ifndef _SERIAL_EXECUTOR_H_INCLUDED
#define _SERIAL_EXECUTOR_H_INCLUDED

#include "framework/executor.h"
#include "framework/thread_pool.h"

#include "boost/thread/scoped_thread.hpp"
#include "boost/thread/future.hpp"

namespace framework { namespace executors {
    class serial_executor_t : public deque_executor_t
    {
    public:
        serial_executor_t(thread_pool_t& tp)
            :deque_executor_t(),
            tp_(tp),
            thread_(&serial_executor_t::worker_thread, this)
        {}

        ~serial_executor_t()
        {
            //外面手动close
            //this->close();
        }
    private:
        void worker_thread()
        {
            work_t work;
            while (this->wait_work(work))
            {
                boost::packaged_task<void> task(work);
                boost::unique_future<void> future = task.get_future();
                tp_.submit(boost::bind(&boost::packaged_task<void>::operator(), &task));
                future.wait();
            }
        }

        thread_pool_t& tp_;
        boost::scoped_thread<> thread_;
    };
} }

#endif //_SERIAL_EXECUTOR_H_INCLUDED
