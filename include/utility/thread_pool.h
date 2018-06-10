#ifndef _MTS_UTILITY_THREADPOOL_H_INCLUDED
#define _MTS_UTILITY_THREADPOOL_H_INCLUDED
#include "config.h"

#ifdef _USE_BOOST
#ifndef TIME_UTC
#define TIME_UTC 1
#endif

#include <boost/threadpool.hpp>
#endif

namespace utility
{
    namespace threadpool
    {
#ifdef _USE_BOOST
        using namespace boost::threadpool;
#endif
    }
}

#endif //_MTS_UTILITY_THREADPOOL_H_INCLUDED
