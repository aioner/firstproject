#ifndef _MTS_FRAMEWORK_DEADLINE_TIMER_H_INCLUDED
#define _MTS_FRAMEWORK_DEADLINE_TIMER_H_INCLUDED

#include "utility/noncopyable.h"
#include "utility/function.h"

#ifdef _WIN32
#include <memory>
#else
#include <boost/shared_ptr.hpp>
#endif
#include <stdint.h>

namespace framework
{
    class deadline_timer_impl : private utility::noncopyable
    {
    public:
        typedef utility::function<void (bool)> wait_handler;

        deadline_timer_impl();
        //设置截止时间
        void expires_from_now(uint32_t millisec);
        //取消
        void cancel();
        //异步
        void async_wait(const wait_handler& handler);
        //同步
        bool wait();
    private:
        class impl_type;
#ifdef _WIN32
        std::tr1::shared_ptr<impl_type> impl_;
#else
		boost::shared_ptr<impl_type> impl_;
#endif
    };

    class deadline_timer : public deadline_timer_impl
    {
    public:
        void start();
    protected:
        virtual void on_time_expires(bool cancel_flag) = 0;
        virtual ~deadline_timer() {}
    };
}

#endif //_MTS_FRAMEWORK_DEADLINE_TIME_H_INCLUDED
