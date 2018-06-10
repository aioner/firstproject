/* rvcbase.c - cbase initialization and shutdown */
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
#include "rvcbase.h"
#include "rvccoreglobals.h"
#include "rvstdio.h"

/* Most modules in cbase require their init and end function to be called. */

/* Module include files for every module that requires init and end be called */
#include "rvqueue.h"
#include "rvtimer.h"
#include "rvtimerengine.h"

#if (RV_THREAD_TYPE == RV_THREAD_SYMBIAN)
#include "rvsymbianinf.h"
#endif

/* Structure containing definitions for module calls */
typedef struct {
    RvStatus (*init)(void);
    RvStatus (*end)(void);
} RvCBaseModuleFuncs;

/* Array of Init and End functions that core will execute. The Init */
/* functions will be called in the order of this array and the End */
/* functions will be called in reverse order. */
static const RvCBaseModuleFuncs RvCBaseModules[] = {
    { RvQueueInit, RvQueueEnd },
    { RvTimerInit, RvTimerEnd },
    { RvTimerEngineInit, RvTimerEngineEnd },
    { NULL, NULL } /* List must end with NULLs */
};

/* Lets make error codes a little easier to type */
#define RvCBaseErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CBASE, RV_CBASE_MODULE_CBASE, (_e))

/* 
   static RvInt RvCBaseInitCount = 0;  Use to make sure we only run once 
   static RvInt RvCBaseNumModules;    Total number of initiated modules 
*/


/********************************************************************************************
 * RvCBaseInit - common core initialization function
 * Must be called before any other calls to any cbase modules. It is not
 * reentrant so simultaneous calls to it (and RvCBaseEnd) MUST NOT be made.
 * Not thread-safe function
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK or appropriate error code
 */
RVCOREAPI RvStatus RVCALLCONV RvCBaseInit(void)
{
    RvStatus result;
    RvCCoreGlobals *__globals__;
    RvBool  glDataSrvStarted = RV_FALSE;

    /* start global data services */
    result = RvStartGlobalDataServices(&glDataSrvStarted);
    if (result != RV_OK)
    {
        return result;
    }

    __globals__ = (RvCCoreGlobals *)RvGetGlobalDataPtr(RV_CCORE_GLOBALS_INDEX);
    if (__globals__ == NULL)
    {
        result = RvCreateGlobalData(RV_CCORE_GLOBALS_INDEX,RvCCoreInitializeGlobals,NULL,RvCCoreDestroyGlobals);
        if (result != RV_OK)
        {
            goto failure;
        }

        __globals__ = (RvCCoreGlobals *)RvGetGlobalDataPtr(RV_CCORE_GLOBALS_INDEX);
        if (__globals__ == NULL)
        {
            goto failure;
        }
    }

    if(RvCBaseInitCount != 0) {  /* We're already running */
        RvCBaseInitCount++;
        return RV_OK;
    }

    /* CCore is requred for CBase so initialize it. */
    result = RvCCoreInit();
    if(result != RV_OK)
    {
        goto failure;
    }

    RvCBaseNumModules = 0;
    for(;;) {
        if(RvCBaseModules[RvCBaseNumModules].init == NULL)
            break;
        result = RvCBaseModules[RvCBaseNumModules].init();
        if(result != RV_OK) {
            /* Something failed, call end for modules that have already had init called. */
            for(RvCBaseNumModules = RvCBaseNumModules - 1; RvCBaseNumModules >= 0; RvCBaseNumModules--)
                RvCBaseModules[RvCBaseNumModules].end();
            goto failure;
        }
        RvCBaseNumModules++;
    }

    RvCBaseInitCount++;
    return RV_OK;

failure:
    if (glDataSrvStarted)
        RvStopGlobalDataServices();
    return result;
}


/********************************************************************************************
 * RvCBaseEnd - common core shutdown function
 * Must be called before system exit to clean up. It is not reentrant so
 * simultaneous calls to it (and RvCBaseInit) MUST NOT be made.
 * Not thread-safe function
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK or appropriate error code
 */
RVCOREAPI RvStatus RVCALLCONV RvCBaseEnd(void)
{
	RV_USE_CCORE_GLOBALS;
    RvStatus result, lastresult, coreresult;

#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(RvCBaseInitCount <= 0)
        return RvCBaseErrorCode(RV_ERROR_UNKNOWN);
#endif

    if(RvCBaseInitCount > 1) { /* We're not at the last end yet */
        RvCBaseInitCount--;
        return RV_OK;
    }

    result = RV_OK;

    /* Go backwards for end. Don't stop for errors but save the first one we get. */
    for(RvCBaseNumModules = (RvCBaseNumModules - 1); RvCBaseNumModules >= 0; RvCBaseNumModules--) {
        lastresult = RvCBaseModules[RvCBaseNumModules].end();
        if(result == RV_OK)
            result = lastresult;
    }

    RvCBaseInitCount = 0;

    coreresult = RvCCoreEnd();
    if(result == RV_OK)
        result = coreresult;
#if (RV_THREAD_TYPE == RV_THREAD_SYMBIAN)
    RvSymbianHeapInit();
#endif

    RvDestroyGlobalData(RV_CCORE_GLOBALS_INDEX);
    RvStopGlobalDataServices();

    return result;
}
