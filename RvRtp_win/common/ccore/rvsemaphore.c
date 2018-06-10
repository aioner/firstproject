/* rvsemaphore.c - counting sempahore functions */
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
#include "rvsemaphore.h"
#include "rvresource.h"
#include "rvlog.h"


/* Lets make error codes a little easier to type */
#define RvSemaphoreErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_SEMAPHORE, (_e))


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvSema4LogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->sema4Source, p);}
#else
#define RvSema4LogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvSema4LogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->sema4Source, p);}
#else
#define RvSema4LogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_SYNC)
#define RvSema4LogSync(p) {if (logMgr != NULL) RvLogSync(&logMgr->sema4Source, p);}
#else
#define RvSema4LogSync(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvSema4LogError(p) {if (logMgr != NULL) RvLogError(&logMgr->sema4Source, p);}
#else
#define RvSema4LogError(p) {RV_UNUSED_ARG(logMgr);}
#endif

RV_DECL_RESOURCE(Semaphore)

/********************************************************************************************
 * RvSemaphoreInit - Initializes the Semaphore module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvSemaphoreInit(void)
{
    RvStatus s = RV_OK;
    RV_RESOURCE_INIT();
#ifdef RV_ADSEMAPHORE_NEED_INIT
    s = RvAdSemaphoreInit();
#endif
    return s;
}


/********************************************************************************************
 * RvSemaphoreEnd - Shuts down the Semaphore module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvSemaphoreEnd(void)
{
  	RV_RESOURCE_END();
    return RV_OK;
}


/********************************************************************************************
 * RvSemaphoreSourceConstruct - constructs semaphore module log source
 *
 * The semaphore module log source is constructed per log manager instance.
 *
 * INPUT   : logMgr  - log manager instances
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvSemaphoreSourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus result = RV_OK;

#if (RV_SEMAPHORE_TYPE != RV_SEMAPHORE_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->sema4Source, "SEMA4", "Semaphores interface");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


#if (RV_SEMAPHORE_TYPE != RV_SEMAPHORE_NONE)
/* Don't include any other code if type is none */

/********************************************************************************************
 * RvSemaphoreConstruct - Creates a counting semaphore object.
 *
 * note: The maximum value of a sempahore is OS and architecture dependent.
 *
 * INPUT   : statcount  - Initial value of the semaphore.
 *           logMgr     - log manager instances
 * OUTPUT  : sema       - Pointer to lock object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvSemaphoreConstruct(
    IN  RvUint32    startcount,
    IN  RvLogMgr    *logMgr,
    OUT RvSemaphore *sema)
{
    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreConstruct(sem=%p)", sema));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sema == NULL) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphoreConstruct: NULL param(s)"));
        return RvSemaphoreErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdSema4Construct(sema, startcount, logMgr);
    RV_RESOURCE_INC();

    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreConstruct(sem=%p) Succeed", sema));

    return RV_OK;
}


/********************************************************************************************
 * RvSemaphoreDestruct - Destroys a counting semaphore object.
 *
 * note: Never destroy a semaphore object which has a thread suspended on it.
 *
 * INPUT   : sema       - Pointer to semaphore object to be destructed.
 *           logMgr     - log manager instances
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvSemaphoreDestruct(
    IN RvSemaphore  *sema,
    IN RvLogMgr     *logMgr)
{
    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreDestruct(sem=%p)", sema));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sema == NULL) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphoreDestruct: NULL param(s)"));
        return RvSemaphoreErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdSema4Destruct(sema, logMgr);
    RV_RESOURCE_DEC();

    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreDestruct(sem=%p) Succeed", sema));

    return RV_OK;
}


/********************************************************************************************
 * RvSemaphorePost - Increments the semaphore.
 *
 * note: The maximum value of a sempahore is OS and architecture dependent.
 *
 * INPUT   : sema       - Pointer to semaphore object to be incremented.
 *           logMgr     - log manager instances
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvSemaphorePost(
    IN RvSemaphore  *sema,
    IN RvLogMgr     *logMgr)
{
    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphorePost(sem=%p)", sema));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sema == NULL) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphorePost: NULL param(s)"));
        return RvSemaphoreErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdSema4Post(sema, logMgr);

    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphorePost(sem=%p) Succeed", sema));

    return RV_OK;
}


/********************************************************************************************
 * RvSemaphoreWait - Decrements a semaphore.
 *
 * If the semaphore is 0, it will suspend the calling task until it can.
 *
 * INPUT   : sema       - Pointer to semaphore object to be decremented.
 *           logMgr     - log manager instances
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvSemaphoreWait(
    IN RvSemaphore  *sema,
    IN RvLogMgr     *logMgr)
{
    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreWait(sem=%p)", sema));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sema == NULL) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphoreWait: NULL param(s)"));
        return RvSemaphoreErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdSema4Wait(sema, logMgr);

    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreWait(sem=%p) Succeed", sema));

    return RV_OK;
}


/********************************************************************************************
 * RvSemaphoreTryWait - Try to decrement a semaphore.
 *
 * If the semaphore is 0, it will not suspend the calling task. Instead, this function
 * returns RV_ERROR_TRY_AGAIN error code.
 *
 * INPUT   : sema       - Pointer to semaphore object to be decremented.
 *           logMgr     - log manager instances
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RvSemaphoreTryWait(
    IN RvSemaphore  *sema,
    IN RvLogMgr     *logMgr)
{
    RvStatus result;

    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreTryWait(sem=%p)", sema));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sema == NULL) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphoreTryWait: NULL param(s)"));
        return RvSemaphoreErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    result = RvAdSema4TryWait(sema, logMgr);

    if (result == RV_ERROR_TRY_AGAIN) {
        RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreTryWait(sem=%p), try again", sema));
        return RvSemaphoreErrorCode(RV_ERROR_TRY_AGAIN);
    }

    if (result != RV_OK) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphoreTryWait(sem=%p)", sema));
        return RvSemaphoreErrorCode(RV_ERROR_UNKNOWN);
    }

    /* RvSemaphoreTryWait succeed to decrease the semaphore counter */
    RvSema4LogSync((&logMgr->sema4Source, "RvSemaphoreTryWait(sem=%p) Succeed", sema));

    return RV_OK;
}


/********************************************************************************************
 * RvSemaphoreSetAttr
 *
 * Sets the options and attributes to be used when creating and using semaphore objects.
 * note: Non-reentrant function. Do not call when other threads may be calling rvsemaphore functions.
 * note: These attributes are global and will effect all semaphore functions called thereafter.
 * note: The default values for these attributes are set in rvccoreconfig.h.
 *
 * INPUT   : attr       - Pointer to OS specific semaphore attributes to begin using.
 *           logMgr     - log manager instances
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvSemaphoreSetAttr(
    IN RvSemaphoreAttr *attr,
    IN RvLogMgr        *logMgr)
{
    RvSema4LogEnter((&logMgr->sema4Source, "RvSemaphoreSetAttr(attr=%p,logMgr=%p)", attr,logMgr));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(attr == NULL) {
        RvSema4LogError((&logMgr->sema4Source, "RvSemaphoreSetAttr(attr=%p) attr is NULL", attr));
        return RvSemaphoreErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvAdSema4SetAttr(attr, logMgr);

    RvSema4LogLeave((&logMgr->sema4Source, "RvSemaphoreSetAttr(attr=%p,logMgr=%p)", attr,logMgr));

    return RV_OK;
}

#endif /* NONE */
