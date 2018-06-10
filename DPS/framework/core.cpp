#include "framework/core.h"

namespace framework
{
    class core_objects_t
    {
    public:
        enum { serial_executor_num = 8 };
        enum { once_timer_basic_period = 1 };

        core_objects_t()
            :thread_pool_(),
            once_timer_(),
            serial_executors_()
        {
        }

        ~core_objects_t()
        {}

        void start()
        {
            once_timer_.start(once_timer_basic_period);
            for (size_t ii = 0; ii < serial_executor_num; ++ii)
            {
                serial_executors_[ii].reset(new serial_executor_t(thread_pool_));
            }
        }

        void trem()
        {
            once_timer_.close();
            thread_pool_.trem();
            for (size_t ii = 0; ii < serial_executor_num; ++ii)
            {
                serial_executors_[ii].get()->close();
            }
        }

        thread_pool_t thread_pool_;
        once_timer_t once_timer_;
        std::auto_ptr<serial_executor_t> serial_executors_[serial_executor_num];
    } g_core_objects;

    namespace core
    {
        thread_pool_t& get_thread_pool()
        {
            return g_core_objects.thread_pool_;
        }

        once_timer_t& get_once_timer()
        {
            return g_core_objects.once_timer_;
        }

        serial_executor_t& get_serial_executor(uint32_t seq)
        {
            return *g_core_objects.serial_executors_[seq % core_objects_t::serial_executor_num];
        }

        void start()
        {
            g_core_objects.start();
        }

        void trem()
        {
            g_core_objects.trem();
        }
    }
}
