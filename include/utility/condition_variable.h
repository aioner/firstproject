#ifndef _MTS_UTILITY_CONDITION_VARIABLE_H_INCLUDED
#define _MTS_UTILITY_CONDITION_VARIABLE_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/condition.hpp>
#endif

/*
    condition_variable_any,
    condition
    ...
*/

namespace utility
{
#ifdef _USE_BOOST
    using boost::condition_variable_any;
    using boost::condition;
#endif
}
#endif //_MTS_UTILITY_CONDITION_VARIABLE_H_INCLUDED