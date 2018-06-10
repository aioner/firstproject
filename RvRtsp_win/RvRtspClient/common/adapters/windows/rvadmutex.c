/* rvadmutex.c - Windows adapter recursive mutex functions */
/************************************************************************
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
#include "rvadmutex.h"
#include "rvlog.h"
#include "rvccoreglobals.h"

#if (RV_MUTEX_TYPE != RV_MUTEX_NONE)

RvStatus RvAdMutexConstruct(
    OUT RvMutex*        mu,
    IN  RvLogMgr*       logMgr)
{
    RV_USE_CCORE_GLOBALS;
    RV_UNUSED_ARG(logMgr);
    RV_UNUSE_GLOBALS;

#if (RV_MUTEX_TYPE == RV_MUTEX_WIN32_MUTEX)
    mu->mtx = CreateMutex(NULL, FALSE, NULL);
    if(mu->mtx == NULL)
        return RV_ERROR_UNKNOWN;
#elif (RV_MUTEX_TYPE == RV_MUTEX_WIN32_CRITICAL)
#if (_WIN32_WINNT >= 0x0500)
    if(InitializeCriticalSectionAndSpinCount(&mu->mtx, RvDefaultMutexAttr) != 0) /* tune spincount for MP systems */
        return RV_ERROR_UNKNOWN;
#else
    InitializeCriticalSection(&mu->mtx);
#endif
#endif

    mu->count = 0;

    return RV_OK;
}


RvStatus RvAdMutexDestruct(
    IN  RvMutex*        mu,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);

#if (RV_MUTEX_TYPE == RV_MUTEX_WIN32_MUTEX)
    if(CloseHandle(mu->mtx) == 0)
        return RV_ERROR_UNKNOWN;
#elif (RV_MUTEX_TYPE == RV_MUTEX_WIN32_CRITICAL)
    DeleteCriticalSection(&mu->mtx);
#endif

    return RV_OK;
}


RvStatus RvAdMutexLock(
    IN  RvMutex*        mu,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);

#if (RV_MUTEX_TYPE == RV_MUTEX_WIN32_MUTEX)
    if(WaitForSingleObject(mu->mtx, INFINITE) != WAIT_OBJECT_0)
        return RV_ERROR_UNKNOWN;
#elif (RV_MUTEX_TYPE == RV_MUTEX_WIN32_CRITICAL)
    EnterCriticalSection(&mu->mtx);
#endif

    mu->count++;

    return RV_OK;
}


RvStatus RvAdMutexUnlock(
    IN  RvMutex*        mu,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);

    mu->count--;

#if (RV_MUTEX_TYPE == RV_MUTEX_WIN32_MUTEX)
    if(ReleaseMutex(mu->mtx) == 0)
        return RV_ERROR_UNKNOWN;
#elif (RV_MUTEX_TYPE == RV_MUTEX_WIN32_CRITICAL)
    LeaveCriticalSection(&mu->mtx);
#endif

    return RV_OK;
}


RvStatus RvAdMutexSetAttr(
    IN  RvMutexAttr*    attr,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(logMgr);

    return RV_OK;
}

#endif /* RV_MUTEX_TYPE != RV_MUTEX_NONE */
