#ifndef _MTS_FRAMEWORK_DETAIL_EVENT_CONTEXT_SERVICE_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_EVENT_CONTEXT_SERVICE_H_INCLUDED

#include "service.h"

#include "utility/config.h"
#ifdef _USE_BOOST
#include "boost_asio_service.h"

namespace framework
{
    namespace detail
    {
        struct event_context_service_tag {};
        typedef detail::service<event_context_service_tag, boost_asio_service> event_context_service;
    }
}
#endif

#endif //_MTS_FRAMEWORK_EVENT_CONTEXT_SERVICE_H_INCLUDED