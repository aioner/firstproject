#ifndef _THREAD_POOL_H_INCLUDED
#define _THREAD_POOL_H_INCLUDED

#include "framework/executor.h"
#include "boost/thread.hpp"

namespace framework { namespace executors {
    class thread_pool_t : public deque_executor_t
    {
    public:
        explicit thread_pool_t(size_t thread_count = boost::thread::hardware_concurrency())
            :deque_executor_t(),
            threads_()
        {
            for (size_t count = 0; count < thread_count; ++count)
            {
                threads_.create_thread(boost::bind(&thread_pool_t::worker_thread, this));
            }
        }

        ~thread_pool_t()
        {
            //外面手动调用
            //trem();
        }

        void trem()
        {
            this->close();
            threads_.join_all();
        }
    private:
        void worker_thread()
        {
            work_t work;
            while (this->wait_work(work))
            {
                work();
            }
        }

        boost::thread_group threads_;
    };
} }

#endif //_THREAD_POOL_H_INCLUDED
