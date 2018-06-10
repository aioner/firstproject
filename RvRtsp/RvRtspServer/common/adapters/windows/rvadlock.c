/* rvadlock.c - Windows adapter lock functions */
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
#include "rvadlock.h"
#include "rvlog.h"
#include "rvccoreglobals.h"
#if defined(RV_LOCK_WIN32_DEBUG)
#include "rvassert.h"
#endif


#if (RV_LOCK_TYPE != RV_LOCK_NONE)

RvStatus RvAdLockConstruct(
    OUT RvLock*         lock,
    IN  RvLogMgr*       logMgr)
{
    RV_USE_CCORE_GLOBALS;        
    RV_UNUSED_ARG(logMgr);

    RV_UNUSE_GLOBALS;

#if (RV_LOCK_TYPE == RV_LOCK_WIN32_MUTEX)

    *lock = CreateMutex(NULL, FALSE, NULL);
    if(*lock == NULL)
        return RV_ERROR_UNKNOWN;

#elif (RV_LOCK_TYPE == RV_LOCK_WIN32_CRITICAL)
#if defined(RV_LOCK_WIN32_DEBUG)
    lock->isLocked = 0;
#endif
#if (_WIN32_WINNT >= 0x0500)
    if(InitializeCriticalSectionAndSpinCount(&lock->lock, RvDefaultLockAttr) != 0) /* tune spincount for MP systems */
        return RV_ERROR_UNKNOWN;
#else
    InitializeCriticalSection(&lock->lock);
#endif

#endif  /* RV_LOCK_TYPE */

    return RV_OK;
}


RvStatus RvAdLockDestruct(
    IN  RvLock*         lock,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);
    
#if (RV_LOCK_TYPE == RV_LOCK_WIN32_MUTEX)

    if(CloseHandle(*lock) == 0)
        return RV_ERROR_UNKNOWN;

#elif (RV_LOCK_TYPE == RV_LOCK_WIN32_CRITICAL)
#if defined(RV_LOCK_WIN32_DEBUG)
    RvAssert(lock->isLocked == 0);
#endif
    DeleteCriticalSection(&lock->lock);
    
#endif  /* RV_LOCK_TYPE */

    return RV_OK;
}


RvStatus RvAdLockGet(
    IN  RvLock*         lock,
    IN  RvLogMgr*       logMgr)
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


RvStatus RvAdLockRelease(
    IN  RvLock*         lock,
    IN  RvLogMgr*       logMgr)
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


RvStatus RvAdLockSetAttr(
    IN  RvLockAttr*     attr,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(logMgr);

    return RV_OK;
}

#endif /* RV_LOCK_TYPE != RV_LOCK_NONE */
