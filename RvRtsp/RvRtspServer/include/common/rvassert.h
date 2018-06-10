/************************************************************************
 File Name     : rvassert.h
 Description   : This file replaces the ANSI assert.h for portability.
                 The functions are a one-to-one mapping with the 
                 ANSI functions in assert.h.
************************************************************************
        Copyright (c) 2001 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/


#if !defined(RV_ASSERT_H)
#define RV_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rvccore.h"

#if !defined(RV_ASSERT_TYPE) || ((RV_ASSERT_TYPE != RV_ASSERT_ANSI) && (RV_ASSERT_TYPE != RV_ASSERT_WINCE))
#error RV_ASSERT_TYPE not set properly
#endif

#if !defined(RV_ASSERT_ENABLE) || ((RV_ASSERT_ENABLE != RV_NO) && (RV_ASSERT_ENABLE != RV_YES))
#error RV_ASSERT_ENABLE not set properly
#endif


#if (RV_ASSERT_ENABLE == RV_YES)

#if (RV_ASSERT_TYPE == RV_ASSERT_ANSI)
#if (RV_OS_TYPE == RV_OS_TYPE_PSOS)
#include "rvstdio.h"
#endif
#include <assert.h>

/********************************************************************************************
 * RvAssert
 * This function is equivalent to the ANSI assert macro. If the test fails
 * the macro will try to display an error message then shut down the system.
 * This function is compiled out if RV_DEBUG is not defined.
 * INPUT   : test              - The expression to assert.
 */
#define RvAssert assert
#endif

#if (RV_ASSERT_TYPE == RV_ASSERT_WINCE)
#define RvAssert(a) ASSERT(a)
#endif

#else

/* Compile out the function */
#define RvAssert(_t)

#endif

#ifdef __cplusplus
}
#endif

#endif  /* Include guard */

