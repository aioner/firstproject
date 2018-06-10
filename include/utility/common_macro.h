#ifndef _MTS_UTILITY_COMMON_MACRO_H_INCLUDED
#define _MTS_UTILITY_COMMON_MACRO_H_INCLUDED

#include <cassert>

#ifdef _DEBUG
#define ASSERT(flag)  assert(flag)
#else
#define ASSERT(flag)
#endif

#define VERIFY(flag)  assert(flag)

#define __in__
#define __out__
#define __in_out__

#define NO_THROW   throw()

#endif //_MTS_UTILITY_COMMON_MACRO_H_INCLUDED
