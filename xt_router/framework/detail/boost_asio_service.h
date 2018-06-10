#ifndef _MTS_FRAMEWORK_DETAIL_BOOST_ASIO_SERVICE_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_BOOST_ASIO_SERVICE_H_INCLUDED

#include "../../framework/config.h"

#ifdef _USE_BOOST
#include <boost/asio.hpp>

namespace framework
{
    namespace detail
    {
        class boost_asio_service
        {
        public:
            boost_asio_service()
                :work_(io_)
            {}

            size_t run()
            {
                boost::system::error_code ec;
                return io_.run(ec);
            }

            void stop()
            {
                io_.stop();
            }

            bool stoped() const
            {
                return io_.stopped();
            }

            template<typename Handler>
            void post(Handler& handler)
            {
                io_.post<Handler>(handler);
            }

            boost::asio::io_service& get_io_service()
            {
                return io_;
            }

        private:
            boost::asio::io_service io_;
            boost::asio::io_service::work work_;
        };
    }
}
#endif

#endif //_MTS_FRAMEWORK_DETAIL_BOOST_ASIO_SERVICE_H_INCLUDED
