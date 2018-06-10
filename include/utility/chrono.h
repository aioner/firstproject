#ifndef _UTILITY_CHRONO_H_INCLUDED
#define _UTILITY_CHRONO_H_INCLUDED

#include "config.h"

#ifdef _USE_BOOST
#include <boost/chrono.hpp>
#endif

/*
    chrono::time_point
    chrono::duration
    chrono::steady_clock
    chrono::system_clock
    ...
*/


namespace utility
{
#ifdef _USE_BOOST
    namespace chrono = boost::chrono;
#endif
}

#endif //_UTILITY_CHRONO_H_INCLUDED
