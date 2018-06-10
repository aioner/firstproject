/* rvlock.c - a non-recursive mutex lock */
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
#include "rvlock.h"
#include "rvlog.h"
#include "rvresource.h"


/* NOTE: all lock functions are only callable at task level */


/* Lets make error codes a little easier to type */
#define RvLockErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_LOCK, (_e))


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvLockLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->lockSource, p);}
#else
#define RvLockLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvLockLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->lockSource, p);}
#else
#define RvLockLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_SYNC)
#define RvLockLogSync(p) {if (logMgr != NULL) RvLogSync(&logMgr->lockSource, p);}
#else
#define RvLockLogSync(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvLockLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->lockSource, p);}
#else
#define RvLockLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif


/********************************************************************************************
 * RvLockInit - Initializes the Lock module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RV_DECL_RESOURCE(Lock)
RvStatus RvLockInit(void)
{
    RvStatus s = RV_OK;
	RV_RESOURCE_INIT();
    RV_RESOURCE_START_COUNT();
#ifdef RV_ADLOCK_NEED_INIT
    s = RvAdLockInit();
#endif
    return s;
}


/********************************************************************************************
 * RvLockEnd - Shuts down the Lock module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvLockEnd(void)
{
	RV_RESOURCE_END();
    RV_RESOURCE_END_COUNT();
    return RV_OK;
}


/********************************************************************************************
 * RvLockSourceConstruct - Constructs lock module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * lock module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvLockSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

#if (RV_LOCK_TYPE != RV_LOCK_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->lockSource, "LOCK", "Locks interface");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


#if (RV_LOCK_TYPE != RV_LOCK_NONE)
/* Don't include any other code if type is none */

/********************************************************************************************
 * RvLockConstruct - Creates a locking object.
 *
 * INPUT   : logMgr - log manager instance.
 * OUTPUT  : lock   - Pointer to lock object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvLockConstruct(
    IN  RvLogMgr    *logMgr,
    OUT RvLock      *lock)
{
    RvLockLogSync((&logMgr->lockSource, "RvLockConstruct(lock=%p)", lock));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(lock == NULL) {
        RvLockLogError((&logMgr->lockSource, "RvLockConstruct: NULL param(s)"));
        return RvLockErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdLockConstruct(lock, logMgr);

    RvLockLogSync((&logMgr->lockSource, "RvLockConstruct(lock=%p) Succeed", lock));

	RV_RESOURCE_INC();
    return RV_OK;
}


/********************************************************************************************
 * RvLockDestruct - Destroys a locking object.
 *
 * Never destroy a lock object which has a thread suspended on it.
 *
 * INPUT   : lock   - Pointer to lock object to be constructed.
 *           logMgr - log manager instance.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvLockDestruct(
    IN  RvLock *lock,
    IN  RvLogMgr* logMgr)
{
    RvLockLogSync((&logMgr->lockSource, "RvLockDestruct(lock=%p)", lock));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(lock == NULL) {
        RvLockLogError((&logMgr->lockSource, "RvLockDestruct: NULL param(s)"));
        return RvLockErrorCode(RV_ERROR_NULLPTR);
    }
#endif
   	RV_RESOURCE_DEC();

    RvAdLockDestruct(lock, logMgr);

    RvLockLogSync((&logMgr->lockSource, "RvLockDestruct(lock=%p) Succeed", lock));

    return RV_OK;
}


#ifndef RV_USE_MACROS
/********************************************************************************************
 * RvLockGet - Aquires a lock.
 *
 * Will suspend the calling task until the lock is available.
 *
 * INPUT   : lock   - Pointer to lock object to be aquired.
 *           logMgr - log manager instance.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvLockGet(
    IN RvLock   *lock,
    IN RvLogMgr *logMgr)
{
    RvLockLogSync((&logMgr->lockSource, "RvLockGet(lock=%p)", lock));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(lock == NULL) {
        RvLockLogError((&logMgr->lockSource, "RvLockGet: NULL param(s)"));
        return RvLockErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdLockGet(lock, logMgr);

    RvLockLogSync((&logMgr->lockSource, "RvLockGet(lock=%p) Succeed", lock));

    return RV_OK;
}
#endif /* #ifndef RV_USE_MACROS */


#ifndef RV_USE_MACROS
/********************************************************************************************
 * RvLockRelease - Releases a lock.
 *
 * Do not release a lock more times than it has been aquired.
 *
 * INPUT   : lock   - Pointer to lock object to be released.
 *           logMgr - log manager instance.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvLockRelease(
    IN RvLock   *lock,
    IN RvLogMgr *logMgr)
{
    RvLockLogSync((&logMgr->lockSource, "RvLockRelease(lock=%p)", lock));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(lock == NULL) {
        RvLockLogError((&logMgr->lockSource, "RvLockRelease: NULL param(s)"));
        return RvLockErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdLockRelease(lock, logMgr);

    RvLockLogSync((&logMgr->lockSource, "RvLockRelease(lock=%p) Succeed", lock));

    return RV_OK;
}
#endif /* #ifndef RV_USE_MACROS */


/********************************************************************************************
 * RvLockSetAttr - Sets the options and attributes to be used when creating and using lock objects.
 *
 * Do not release a lock more times than it has been aquired.
 * note: Non-reentrant function. Do not call when other threads may be calling rvlock functions.
 * note: These attributes are global and will effect all lock functions called thereafter.
 * note: The default values for these attributes are set in rvccoreconfig.h.
 * Not thread-safe
 *
 * INPUT   : attr   - Pointer to OS specific lock attributes to begin using.
 *           logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : Always returns RV_OK
 */
RvStatus RvLockSetAttr(
    IN RvLockAttr *attr,
    IN RvLogMgr   *logMgr)
{
    RvLockLogEnter((&logMgr->lockSource, "RvLockSetAttr(attr=%p,logMgr)", attr,logMgr));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(attr == NULL) {
        RvLockLogError((&logMgr->lockSource, "RvLockSetAttr(attr=%p) attr is NULL", attr));
        return RvLockErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdLockSetAttr(attr, logMgr);

    RvLockLogLeave((&logMgr->lockSource, "RvLockSetAttr(attr=%p,logMgr)", attr,logMgr));

    return RV_OK;
}

#endif /* NONE */
