/* rvadsema4.c - Windows adapter semaphore functions */
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
#include "rvadsema4.h"
#include "rvlog.h"

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_POSIX)
#include <errno.h>
#endif

static RvSemaphoreAttr RvDefaultSemaphoreAttr = RV_SEMAPHORE_ATTRIBUTE_DEFAULT;

#if (RV_SEMAPHORE_TYPE != RV_SEMAPHORE_NONE)

RvStatus RvAdSema4Construct(
    OUT RvSemaphore*    sema,
    IN  RvUint32        startcount,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_POSIX)
    if(sem_init(sema, RvDefaultSemaphoreAttr, startcount) < 0)
        return RV_ERROR_UNKNOWN;
#endif
    
#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_WIN32)
    *sema = CreateSemaphore(NULL, (LONG)startcount, RV_INT32_MAX, NULL);
    if(*sema == NULL)
        return RV_ERROR_UNKNOWN;
#endif
    return RV_OK;
}


RvStatus RvAdSema4Destruct(
    IN  RvSemaphore*    sema,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);
#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_WIN32)
    if(CloseHandle(*sema) == 0)
        return RV_ERROR_UNKNOWN;
#endif  

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_POSIX)
    if(sem_destroy(sema) < 0)
        return RV_ERROR_UNKNOWN;
#endif

    return RV_OK;
}


RvStatus RvAdSema4Post(
    IN  RvSemaphore*    sema,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_WIN32)
    if(ReleaseSemaphore(*sema, 1, NULL) == 0)
        return RV_ERROR_UNKNOWN;
#endif  
#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_POSIX)
    if(sem_post(sema) < 0)
        return RV_ERROR_UNKNOWN;
#endif
    return RV_OK;
}


RvStatus RvAdSema4Wait(
    IN  RvSemaphore*    sema,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(logMgr);
#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_WIN32)
    if(WaitForSingleObject(*sema, INFINITE) != WAIT_OBJECT_0)
        return RV_ERROR_UNKNOWN;
#endif

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_POSIX)
    /* We must loop if we were awoken by a signal. */
    while(sem_wait(sema) < 0) {
        if(errno != EINTR)
             return RV_ERROR_UNKNOWN;
    }
#endif
    return RV_OK;
}


RvStatus RvAdSema4TryWait(
    IN  RvSemaphore*    sema,
    IN  RvLogMgr*       logMgr)
{
    RvUint32 status;

    RV_UNUSED_ARG(logMgr);

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_WIN32)
    status = WaitForSingleObject(*sema, 0);

    if (status == WAIT_TIMEOUT)
        return RV_ERROR_TRY_AGAIN;

    if (status != WAIT_OBJECT_0)
        return RV_ERROR_UNKNOWN;
#endif

#if (RV_SEMAPHORE_TYPE == RV_SEMAPHORE_POSIX)
    /* We must loop if we were awoken by a signal. */
    while(sem_wait(sema) < 0) {
        if(errno != EINTR)
             return RV_ERROR_UNKNOWN;
    }
#endif

    return RV_OK;
}


RvStatus RvAdSema4SetAttr(
    IN  RvSemaphoreAttr*attr,
    IN  RvLogMgr*       logMgr)
{
    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(logMgr);

    return RV_OK;
}

#endif /* RV_SEMAPHORE_TYPE != RV_SEMAPHORE_NONE */
