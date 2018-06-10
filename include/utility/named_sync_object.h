#ifndef _UTILITY_NAMED_SYNC_OBJECT_H_INCLUDED
#define _UTILITY_NAMED_SYNC_OBJECT_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#endif

/*
    named_mutex
    named_condition
*/


namespace utility
{
#ifdef _USE_BOOST
    using boost::named_mutex;
    using boost::named_condition;
#endif
}

#endif //_UTILITY_NAMED_SYNC_OBJECT_H_INCLUDED