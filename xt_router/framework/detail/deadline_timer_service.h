#ifndef _MTS_FRAMEWORK_DETAIL_DEADLINE_TIMER_IMPL_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_DEADLINE_TIMER_IMPL_H_INCLUDED

#include "service.h"

#include "../../framework/config.h"
#ifdef _USE_BOOST
#include "boost_asio_service.h"

namespace framework
{
    namespace detail
    {
        struct deadline_timer_service_tag {};
        typedef detail::service<deadline_timer_service_tag, boost_asio_service> deadline_timer_service;
    }
}
#endif //_USE_BOOST

#endif //_MTS_FRAMEWORK_DETAIL_DEADLINE_TIMER_IMPL_H_INCLUDED
