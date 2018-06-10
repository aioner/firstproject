#ifndef _MTS_FRAMEWORK_DETAIL_BOOST_DEADLINE_TIMER_IMPL_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_BOOST_DEADLINE_TIMER_IMPL_H_INCLUDED


#include "../../framework/config.h"

#ifdef _USE_BOOST
#include <boost/asio.hpp>
#endif

#include "utility/function.h"

namespace framework
{
    namespace detail
    {
        class boost_deadline_timer_impl
        {
        public:
            typedef utility::function<void (bool)> wait_handler;

            boost_deadline_timer_impl(detail::boost_asio_service& service, uint32_t millisec)
                :timer_(service.get_io_service(), boost::posix_time::millisec(millisec))
            {}

            explicit boost_deadline_timer_impl(detail::boost_asio_service& service)
                :timer_(service.get_io_service())
            {}

            ~boost_deadline_timer_impl()
            {
                cancel();
            }

            std::size_t cancel()
            {
                boost::system::error_code e;
                return timer_.cancel(e);
            }

            void expires_from_now(uint32_t millisec)
            {
                timer_.expires_from_now(boost::posix_time::millisec(millisec));
            }

            void async_wait(const wait_handler& handler)
            {
                timer_.async_wait(utility::bind(&boost_deadline_timer_impl::async_wait_handler, this, handler, boost::asio::placeholders::error));
            }

            bool wait()
            {
                boost::system::error_code e;
                timer_.wait(e);
                return !e;
            }

        private:
            void async_wait_handler(wait_handler& handler, const boost::system::error_code& e)
            {
                handler(e == boost::asio::error::operation_aborted);
            }

            boost::asio::deadline_timer timer_;
        };
    }
}
#endif //_MTS_FRAMEWORK_DETAIL_BOOST_DEADLINE_TIMER_IMPL_H_INCLUDED
