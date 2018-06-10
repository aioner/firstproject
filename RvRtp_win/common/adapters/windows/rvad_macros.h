/* rvadmutex_macros.h - rvadmutex_macros header file */
/************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
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

#ifndef RV_AD_MACROS_H
#define RV_AD_MACROS_H

#ifdef RV_USE_MACROS

#include "rvadmutex.h"
#include "rvadlock_t.h"

struct RvLogMgrInternal;

RVINLINE RvStatus RvAdMutexLock(
                        IN  RvMutex*                  mu,
                        IN  struct RvLogMgrInternal*  logMgr)
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

RVINLINE RvStatus RvAdMutexUnlock(
                        IN  RvMutex*   mu,
                        IN  struct RvLogMgrInternal*  logMgr)
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

RVINLINE RvStatus RvAdLockGet(
                        IN  RvLock*                   lock,
                        IN  struct RvLogMgrInternal*  logMgr)
{
    RV_UNUSED_ARG(logMgr);

#if (RV_LOCK_TYPE == RV_LOCK_WIN32_MUTEX)
    if(WaitForSingleObject(*lock, INFINITE) != WAIT_OBJECT_0)
        return RV_ERROR_UNKNOWN;
#elif (RV_LOCK_TYPE == RV_LOCK_WIN32_CRITICAL)
    EnterCriticalSection(&lock->lock);
#if defined(RV_LOCK_WIN32_DEBUG)
    RvAssert(lock->isLocked == 0);
    lock->isLocked++;
#endif
#endif  /* RV_LOCK_TYPE */

    return RV_OK;
}

RVINLINE RvStatus RvAdLockRelease(
                        IN  RvLock*                   lock,
                        IN  struct RvLogMgrInternal*  logMgr)
{
    RV_UNUSED_ARG(logMgr);

#if (RV_LOCK_TYPE == RV_LOCK_WIN32_MUTEX)
    if(ReleaseMutex(*lock) == 0)
        return RV_ERROR_UNKNOWN;
#elif (RV_LOCK_TYPE == RV_LOCK_WIN32_CRITICAL)
#if defined(RV_LOCK_WIN32_DEBUG)
    RvAssert(lock->isLocked == 1);
    lock->isLocked--;
#endif
    LeaveCriticalSection(&lock->lock);
#endif  /* RV_LOCK_TYPE */

    return RV_OK;
}


#endif /* #ifdef RV_USE_MACROS */
#endif /* #ifndef RV_AD_MACROS_H */
