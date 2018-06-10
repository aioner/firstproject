#ifndef _MTS_UTILITY_MUTEX_H_INCLUDED
#define _MTS_UTILITY_MUTEX_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/thread.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#endif

/*
    mutex,
    shared_mutex,
    recursive_mutex,
    lock_guard,
    shared_lock,
    unique_lock,
    once_flag,
    call_once
*/

namespace utility
{
#ifdef _USE_BOOST
    //using namespace boost;
    using boost::mutex;
    using boost::shared_mutex;
    using boost::recursive_mutex;
    using boost::lock_guard;

    typedef boost::shared_mutex read_write_mutex;
    typedef boost::unique_lock<boost::shared_mutex> write_lock;
    typedef boost::shared_lock<boost::shared_mutex> read_lock;

    using boost::detail::spinlock;
#endif

    class null_mutex
    {
    public:
        void lock() {}
        void unlock() {}
    };
}

#endif //_MTS_UTILITY_MUTEX_H_INCLUDED
