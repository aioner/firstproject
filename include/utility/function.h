#ifndef _MTS_UTILITY_FUNCTION_H_INCLUDED
#define _MTS_UTILITY_FUNCTION_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/bind/placeholders.hpp>
#endif

/*
    bind,
    function,
    ref,
    _1,
    _2
*/

namespace utility
{
#ifdef _USE_BOOST
    using boost::bind;
    using boost::function;
    using ::_1;
    using ::_2;
#endif
}

#endif //_MTS_UTILITY_FUNCTION_H_INCLUDED