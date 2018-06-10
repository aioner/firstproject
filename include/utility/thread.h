#ifndef _MTS_UTILITY_THREAD_H_INCLUDED
#define _MTS_UTILITY_THREAD_H_INCLUDED
#include "config.h"

#ifdef _USE_BOOST
#include <boost/thread.hpp>
#endif

namespace utility
{
#ifdef _USE_BOOST
    using boost::thread;
    using boost::thread_group;
    namespace this_thread = boost::this_thread;
#endif
}

#endif //_MTS_UTILITY_THREAD_H_INCLUDED
