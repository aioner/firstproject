#ifndef _EXECUTOR_H_INCLUDED
#define _EXECUTOR_H_INCLUDED

#include "framework/sync_deque.h"
#include "boost/function.hpp"
#include "boost/noncopyable.hpp"

namespace framework { namespace executors {
    typedef boost::function0<void> work_t;

    class executor_t : private boost::noncopyable
    {
    public:
        void close() {}

        bool closed() const { return false; }

        template<typename F>
        void submit(F f, bool deferred = true) { f(); }

        bool wait_work(work_t& work) { return false; } 
    };

    class deque_executor_t : public executor_t
    {
    public:
        void close()
        {
            works_.close();
        }

        bool closed()
        {
            return works_.closed();
        }

        template<typename F>
        void submit(F f, bool deferred = true)
        {
            if (deferred)
            {
                works_.push_back(f);
            }
            else
            {
                works_.push_front(f);
            }
        }

        bool wait_work(work_t& work)
        {
            bool closed = false;
            works_.wait_pull_front(work, closed);
            return !closed;
        }

        std::size_t size(){return works_.size();}
    private:
        sync_deque_t<work_t> works_;
    };
} }

#endif //_EXECUTOR_H_INCLUDED

