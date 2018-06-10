#ifndef _CORE_H_INCLUDED
#define _CORE_H_INCLUDED

#include "framework/thread_pool.h"
#include "framework/timer.h"
#include "framework/serial_executor.h"

namespace framework
{
    using executors::thread_pool_t;
    using executors::serial_executor_t;

    namespace core
    {
        void start();
        void trem();
        thread_pool_t& get_thread_pool();
        once_timer_t& get_once_timer();
        serial_executor_t& get_serial_executor(uint32_t seq);

        template<typename F>
        void thread_pool_submit(F f, bool deferred = false);

        template<typename F>
        void once_timer_submit(F f, uint32_t ms);

        template<typename F>
        void once_timer_submit_on(F f, uint32_t ms, thread_pool_t& exec = get_thread_pool());

        template<typename F>
        void serial_executor_submit(F f, uint32_t seq = 0);
    }

    template<typename F>
    void core::thread_pool_submit(F f, bool deferred)
    {
        core::get_thread_pool().submit(f, deferred);
    }

    template<typename F>
    void core::once_timer_submit(F f, uint32_t ms)
    {
        core::get_once_timer().submit(f, ms);
    }

    template<typename F>
    void core::once_timer_submit_on(F f, uint32_t ms, thread_pool_t& exec)
    {
        core::get_once_timer().submit_on(f, ms, exec);
    }

    template<typename F>
    void core::serial_executor_submit(F f, uint32_t seq)
    {
        core::get_serial_executor(seq).submit(f);
    }
}

#endif //_CORE_H_INCLUDED
