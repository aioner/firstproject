#include "deadline_timer.h"
#include "detail/deadline_timer_service.h"

#ifdef _USE_BOOST
#include "detail/boost_deadline_timer_impl.h"
typedef framework::detail::boost_deadline_timer_impl deadline_timer_impl_base;
#endif

namespace framework
{
    class deadline_timer_impl::impl_type : public deadline_timer_impl_base
    {
    public:
        impl_type()
            :deadline_timer_impl_base(*detail::deadline_timer_service::instance())
        {}
    };

    deadline_timer_impl::deadline_timer_impl()
        :impl_(new impl_type)
    {}

    void deadline_timer_impl::expires_from_now(uint32_t millisec)
    {
        impl_->expires_from_now(millisec);
    }

    bool deadline_timer_impl::wait()
    {
        return impl_->wait();
    }

    void deadline_timer_impl::cancel()
    {
        impl_->cancel();
    }

    void deadline_timer_impl::async_wait(const wait_handler& handler)
    {
        impl_->async_wait(handler);
    }

    void deadline_timer::start()
    {
        async_wait(utility::bind(&deadline_timer::on_time_expires, this, utility::_1));
    }
}