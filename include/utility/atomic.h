#ifndef _MTS_UTILITY_ATOMIC_H_INCLUDED
#define _MTS_UTILITY_ATOMIC_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/atomic.hpp>
#include <boost/memory_order.hpp>
#endif

/*
    atomic<T>,
    atomic_bool,
    ...
*/

namespace utility
{
#ifdef _USE_BOOST
    using boost::atomic;
    using boost::atomic_bool;
#endif
}
#endif //_MTS_UTILITY_ATOMIC_H_INCLUDED
