/* rvmutex.c - recursive mutex functions */
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
#include "rvmutex.h"
#include "rvresource.h"
#include "rvlog.h"


/* NOTE: all mutex functions are only callable at task level */


/* Lets make error codes a little easier to type */
#define RvMutexErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_MUTEX, (_e))


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvMutexLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->mutexSource, p);}
#else
#define RvMutexLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvMutexLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->mutexSource, p);}
#else
#define RvMutexLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_SYNC)
#define RvMutexLogSync(p) {if (logMgr != NULL) RvLogSync(&logMgr->mutexSource, p);}
#else
#define RvMutexLogSync(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvMutexLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->mutexSource, p);}
#else
#define RvMutexLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_EXCEP)
#define RvMutexLogExcep(p) {if (logMgr != NULL) RvLogExcep(&logMgr->mutexSource, p);}
#else
#define RvMutexLogExcep(p) {RV_UNUSED_ARG(logMgr);}
#endif

RV_DECL_RESOURCE(Mutex)

/********************************************************************************************
 * RvMutexInit - Initializes the Mutex module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : Always RV_OK
 */
RvStatus RvMutexInit(void)
{
    RV_RESOURCE_INIT();
    return RV_OK;
}


/********************************************************************************************
 * RvMutexEnd - Shuts down the Mutex module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : Always RV_OK
 */
RvStatus RvMutexEnd(void)
{
   	RV_RESOURCE_END();
    return RV_OK;
}


/********************************************************************************************
 * RvMutexSourceConstruct - Constructs lock module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * lock module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvMutexSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

#if (RV_LOCK_TYPE != RV_LOCK_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->mutexSource, "MUTEX", "Recursive Mutex Interface");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


#if (RV_MUTEX_TYPE != RV_MUTEX_NONE)
/* Don't include any other code if type is none */

/********************************************************************************************
 * RvMutexConstruct - Creates a recursive mutex object.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : mu     - Pointer to mutex object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvMutexConstruct(
    IN  RvLogMgr*  logMgr,
    OUT RvMutex*   mu)
{
    RvMutexLogSync((&logMgr->mutexSource, "RvMutexConstruct(mutex=%p)", mu));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(mu == NULL) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexConstruct NULL pointer input"));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdMutexConstruct(mu, logMgr);
    RV_RESOURCE_INC();

#if defined(RV_MUTEX_DEBUG)
    mu->filename = NULL;
    mu->lineno = 0;
#endif

    RvMutexLogSync((&logMgr->mutexSource, "RvMutexConstruct(mutex=%p) Succeed", mu));

    return RV_OK;
}


/********************************************************************************************
 * RvMutexDestruct - Destroys a recursive mutex object.
 *
 * note: Never destroy a mutex object which has a thread suspended on it.
 *
 * INPUT   : mu     - Pointer to recursive mutex object to be destructed.
 *           logMgr - log manager instance
 * OUTPUT  :
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvMutexDestruct(
    IN  RvMutex*   mu,
    IN  RvLogMgr*  logMgr)
{
    RvMutexLogSync((&logMgr->mutexSource, "RvMutexDestruct(mutex=%p)", mu));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(mu == NULL) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexDestruct mutex is NULL pointer"));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdMutexDestruct(mu, logMgr);
    RV_RESOURCE_DEC();

    RvMutexLogSync((&logMgr->mutexSource, "RvMutexDestruct(mutex=%p) Succeed", mu));

    return RV_OK;
}


/********************************************************************************************
 * RvMutexLock - Aquires a recursive mutex.
 *
 * Will suspend the calling task until the mutex is available.
 *
 * INPUT   : mu     - Pointer to mutex object to be aquired.
 *           logMgr - log manager instance
 * OUTPUT  :
 * RETURN  : RV_OK if successful otherwise an error code.
 */
#if defined(RV_MUTEX_DEBUG)
RVCOREAPI
RvStatus RVCALLCONV RvMutexLockDbg(
	IN  RvMutex*        mu,
	IN  RvLogMgr*       logMgr,
    IN  const RvChar*   filename,
    IN  RvInt           lineno)
#else
RVCOREAPI
RvStatus RVCALLCONV RvMutexLock(
    IN  RvMutex*   mu,
    IN  RvLogMgr*  logMgr)
#endif
{
    RvMutexLogSync((&logMgr->mutexSource, "RvMutexLock(mutex=%p)", mu));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(mu == NULL) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexLock(mutex=%p) mutex is NULL", mu));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdMutexLock(mu, logMgr);

#if defined(RV_MUTEX_DEBUG)
    mu->filename = filename;
    mu->lineno = lineno;
#endif


    
    RvMutexLogSync((&logMgr->mutexSource, "RvMutexLock(mutex=%p) Succeed, counter = %d", mu,mu->count));

    return RV_OK;
}


/********************************************************************************************
 * RvMutexUnlock - Unlocks a recursive mutex.
 *
 * INPUT   : mu     - Pointer to mutex object to be unlocked.
 *           logMgr - log manager instance
 * OUTPUT  :
 * RETURN  : RV_OK if successful otherwise an error code.
 */
#if defined(RV_MUTEX_DEBUG)
RVCOREAPI
RvStatus RVCALLCONV RvMutexUnlockDbg(
	IN  RvMutex*        mu,
	IN  RvLogMgr*       logMgr,
    IN  const RvChar*   filename,
    IN  RvInt           lineno)
#else
RVCOREAPI
RvStatus RVCALLCONV RvMutexUnlock(
    IN  RvMutex*   mu,
    IN  RvLogMgr*  logMgr)
#endif
{
    RvMutexLogSync((&logMgr->mutexSource, "RvMutexUnlock(mutex=%p)", mu));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(mu == NULL) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexUnlock(mutex=%p) mutex is NULL", mu));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

#if defined(RV_MUTEX_DEBUG)
    mu->filename = filename;
    mu->lineno = -lineno;
#endif

    RvAdMutexUnlock(mu, logMgr);

#if RV_LOGMASK & RV_LOGLEVEL_EXCEP
    if(mu->count < 0) {
        RvMutexLogExcep((&logMgr->mutexSource, "RvMutexUnlock(mutex=%p) counter is negative (%d), probably to many unlock's", mu, mu->count));
    } else
#endif
    {
        RvMutexLogSync((&logMgr->mutexSource, "RvMutexUnlock(mutex=%p) Succeed, counter = %d", mu,mu->count));
    }

    return RV_OK;
}


/********************************************************************************************
 * RvMutexSetAttr
 *
 * Sets the options and attributes to be used when creating and using mutex objects.
 * note: Non-reentrant function. Do not call when other threads may be calling rvmutex functions.
 * note: These attributes are global and will effect all mutex functions called thereafter.
 * note: The default values for these attributes are set in rvccoreconfig.h.
 *
 * INPUT   : mu     - Pointer to mutex object to be unlocked.
 *           logMgr - log manager instance
 * OUTPUT  :
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvMutexSetAttr(
    IN RvMutexAttr *attr,
    IN RvLogMgr    *logMgr)
{
    RvMutexLogEnter((&logMgr->mutexSource, "RvMutexSetAttr(attr=%p,logMgr=%p)",attr,logMgr));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(attr == NULL) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexSetAttr(attr=%p) attr is NULL", attr));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdMutexSetAttr(attr, logMgr);

    RvMutexLogLeave((&logMgr->mutexSource, "RvMutexSetAttr(attr=%p,logMgr=%p)",attr,logMgr));

    return RV_OK;
}


/********************************************************************************************
 * RvMutexGetLockCounter - Returns the number of times the mutex has been locked by
 *                         this thread.
 *
 * INPUT   : mu     - Pointer to mutex object
 *           logMgr - log manager instance
 * OUTPUT  : lockCnt - lock counter to be returned
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvMutexGetLockCounter(
    IN  RvMutex     *mu,
    IN  RvLogMgr    *logMgr,
    OUT RvInt32     *lockCnt)
{
    RvMutexLogEnter((&logMgr->mutexSource, "RvMutexGetLockCounter(mutex=%p,lockCnt=%p)",
                                           mu,lockCnt));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((mu == NULL) || (lockCnt == NULL)) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexUnlock(mutex=%p,lockCnt=%p) NULL pointer input",
                                               mu,lockCnt));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    *lockCnt = mu->count;

    RvMutexLogLeave((&logMgr->mutexSource, "RvMutexUnlock(mutex=%p,lockCnt=%p) lock counter = %d",
                                           mu,lockCnt,*lockCnt));

    return RV_OK;
}


/********************************************************************************************
 * RvMutexRelease - Unlocks a mutex recursively until the mutex is released completely.
 *
 * INPUT   : mu     - Pointer to mutex object
 *           logMgr - log manager instance
 * OUTPUT  : lockCnt - lock counter to be returned
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvMutexRelease(
    IN  RvMutex     *mu,
    IN  RvLogMgr    *logMgr,
    OUT RvInt32     *lockCnt)
{
    RvInt32 i;

    RvMutexLogEnter((&logMgr->mutexSource, "RvMutexRelease(mutex=%p,lockCnt=%p)",
                                           mu,lockCnt));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((mu == NULL) || (lockCnt == NULL)) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexRelease(mutex=%p,lockCnt=%p) NULL pointer input",
                                               mu,lockCnt));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvMutexGetLockCounter(mu, logMgr, lockCnt);

    for (i=0; i < *lockCnt; ++i)
        RvMutexUnlock(mu, logMgr);

    RvMutexLogLeave((&logMgr->mutexSource, "RvMutexRelease(mutex=%p,lockCnt=%p) lock counter = %d",
                                           mu,lockCnt,*lockCnt));

    return RV_OK;
}


/********************************************************************************************
 * RvMutexRestore - Locks a mutex recursively lockCnt times (restores a mutex to its
 *                  previously saved state).
 *
 * INPUT   : mu     - Pointer to mutex object
 *           logMgr - log manager instance
 * OUTPUT  : lockCnt - the original lock counter
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvMutexRestore(
    IN  RvMutex     *mu,
    IN  RvLogMgr    *logMgr,
    IN  RvInt32     lockCnt)
{
    RvInt32 i;

    RvMutexLogEnter((&logMgr->mutexSource, "RvMutexRestore(mutex=%p,lockCnt=%d)",
                                           mu,lockCnt));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(mu == NULL) {
        RvMutexLogError((&logMgr->mutexSource, "RvMutexRestore(mutex=%p) NULL pointer input",
                                               mu));
        return RvMutexErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    for (i=0; i < lockCnt; ++i)
        RvMutexLock(mu, logMgr);

    RvMutexLogLeave((&logMgr->mutexSource, "RvMutexRestore(mutex=%p,lockCnt=%d)",
                                           mu,lockCnt));

    return RV_OK;
}

#endif /* RV_MUTEX_TYPE != RV_MUTEX_NONE */
