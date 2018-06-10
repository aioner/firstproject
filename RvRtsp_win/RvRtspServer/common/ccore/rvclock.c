/***********************************************************************
Filename   : rvclock.c
Description: clock functions for getting wall time
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
#include "rvclock.h"

/* Lets make error codes a little easier to type */
#define RvClockErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_CLOCK, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvClockLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->clockSource, p);}
#else
#define RvClockLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvClockLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->clockSource, p);}
#else
#define RvClockLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvClockLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->clockSource, p);}
#else
#define RvClockLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_INFO)
#define RvClockLogInfo(p)  {if (logMgr != NULL) RvLogInfo(&logMgr->clockSource, p);}
#else
#define RvClockLogInfo(p)  {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define RvClockLogDebug(p) {if (logMgr != NULL) RvLogDebug(&logMgr->clockSource, p);}
#else
#define RvClockLogDebug(p) {RV_UNUSED_ARG(logMgr);}
#endif

/********************************************************************************************
 * RvClockInit
 * Initializes the Clock module. Must be called once (and
 * only once) before any other functions in the module are called.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvClockInit(void)
{
    /* OS specific setup */
    RvAdClockInit();
    
    return RV_OK;
}

/********************************************************************************************
 * RvClockEnd
 * Shuts down the Clock module. Must be called once (and
 * only once) when no further calls to this module will be made.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvClockEnd(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvClockSourceConstruct -Constructs clock module log source
 *
 * Constructs clock module log source for printing log per specific
 * log manager instance
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvClockSourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus result;

    result = RvLogSourceConstruct(logMgr, &logMgr->clockSource, "CLOCK", "Wall Clock");

    return result;
}

/********************************************************************************************
 * RvClockGet
 * Get time since epoch -- can vary due to system time adjustments
 * returns time in seconds and fills result with seconds and nanoseconds
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : Pointer to time structure where time will be stored.
 *           If NULL, the seconds will still be returned.
 * RETURN  : if result is not NULL. Epoch is January 1, 1970
 */
RVCOREAPI
RvInt32 RVCALLCONV RvClockGet(
    IN  RvLogMgr  *logMgr,
    OUT RvTime    *t)
{
    RvTime temptime, *result;

    RvClockLogEnter((&logMgr->clockSource, "RvClockGet(logMgr=%p,t=%p)", logMgr,t));

    if(t == NULL) {  /* use temptime if user doesn't provide result struct */
        result = &temptime;
    } else result = t;

    RvAdClockGet(result);

    RvClockLogLeave((&logMgr->clockSource, "RvClockGet(logMgr=%p,t=%p) = %d", logMgr,t,RvTimeGetSecs(result)));

    return RvTimeGetSecs(result);
}


/********************************************************************************************
 * RvClockSet
 * Sets time of day in seconds and nanoseconds since January 1, 1970.
 * INPUT   : t - Pointer to time structure with the time to be set.
 *           logMgr - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 * Note    : On some systems the nanoseconds portion of the time will be ignored.
 *           On some systems setting the clock requires special priviledges.
 */
RVCOREAPI
RvStatus RVCALLCONV RvClockSet(
    IN const RvTime *t,
    IN RvLogMgr     *logMgr)
{
    RvStatus result;

    RvClockLogEnter((&logMgr->clockSource, "RvClockSet(t=%p,logMgr=%p)",t,logMgr));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(t == NULL) {
        RvClockLogError((&logMgr->clockSource, "RvClockSet, t == NULL"));
        return RvClockErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    result = RvAdClockSet(t);


    RvClockLogLeave((&logMgr->clockSource, "RvClockSet(t=%p,logMgr=%p) = 0x%x",t,logMgr,result));

    return result;
}


/********************************************************************************************
 * RvClockResolution
 * Gets the resolution of the wall clock in seconds and nanoseconds params.
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : t - Pointer to time structure where resolution will be stored.
 *               If NULL, the nanoseconds will still be returned.
 * RETURN  : nanoseconds portion of the clock resolution.
 */
RVCOREAPI
RvInt32 RVCALLCONV RvClockResolution(
    IN  RvLogMgr *logMgr,
    OUT RvTime   *t)
{
    RvTime temptime, *result;

    RvClockLogEnter((&logMgr->clockSource, "RvClockResolution(logMgr=%p,t=%p)", logMgr,t));

    if(t == NULL) {  /* use temptime if user doesn't provide result struct */
        result = &temptime;
    } else result = t;

    RvAdClockResolution(result);

    RvClockLogLeave((&logMgr->clockSource, "RvClockResolution(logMgr=%p,t=%p) = %d", logMgr,t,RvTimeGetNsecs(result)));

    return RvTimeGetNsecs(result);
}
