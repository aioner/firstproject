#ifndef _MTS_UTILITY_CONFIG_H_INCLUDED
#define _MTS_UTILITY_CONFIG_H_INCLUDED
#define _USE_BOOST

#if (defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__CYGWIN__) || defined(WIN64))\
	&& !defined(_OS_WINDOWS)
#define _OS_WINDOWS
#endif

#endif //_MTS_UTILITY_DETAIL_CONFIG_H_INCLUDED
