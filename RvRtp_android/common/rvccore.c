/***********************************************************************
Filename   : rvccore.c
Description: ccore initialization and shutdown functions
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
#include "rvassert.h"
#include "rvccore.h"
#include "rvccoreversion.h"
#include "rvccorestrings.h"
#include "rvccoreglobals.h"
#if (RV_IMS_IPSEC_ENABLED == RV_YES)        
#include "rvimsipsec.h"
#endif    
        
static const RvChar RvCCoreVersionString[] = { RV_COMMON_CORE_VERSION };


/* Each string here is NULL terminated, while the last one has 2 NULL terminations after
   it, to make stopping easier */
static const RvChar RvCCoreCompilationStrings[] = {
#ifdef RV_DEBUG
    "@(#)Common Core version " RV_COMMON_CORE_VERSION " (Debug)\0"
#endif
#ifdef RV_RELEASE
    "@(#)Common Core version " RV_COMMON_CORE_VERSION " (Release)\0"
#endif
    "@(#)OS=" RV_OS_STRING "\0"
    "@(#)Compiler=" RV_TOOL_STRING "\0"
    "@(#)Architecture=" RV_ARCH_ENDIAN_STRING ", " RV_ARCH_BITS_STRING "\0"
    RV_TIMING_MODULES_STRING "\0"
    RV_THREAD_MODULES_STRING "\0"
    RV_MEMORY_MODULES_STRING "\0"
    RV_NETWORK_MODULES_STRING "\0"
    RV_ANSI_MODULES_STRING "\0"
    RV_CHECK_STRING "\0"
    RV_USR_CONFIG_STRING "\0"
    "\0" };


#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
static RvUint rvWindowsVersion = RV_OS_WIN32_GENERIC;
#endif


/* Most modules in ccore require their init and end function to be called. */
/* The exceptions to this are drivers, whose init function are manage by */
/* the appropriate driver manager. */

/* Module include files for every module that requires init and end be called */
#include "rvtm.h"
#include "rvclock.h"
#include "rvtimestamp.h"
#include "rv64ascii.h"
#include "rvsemaphore.h"
#include "rvmutex.h"
#include "rvlock.h"
#include "rvthread.h"
#include "rvmemory.h"
#include "rvloglistener.h"
#include "rvlog.h"
#include "rvselect.h"
#include "rvsocket.h"
#include "rvtransportsocket.h"
#include "rvhost.h"
#include "rvtls.h"
#include "rvares.h"
#include "rvnotify.h"
#include "rvstdio.h"


/* Structure containing definitions for module calls */
typedef struct {
    RvStatus (*init)(void);
    RvStatus (*end)(void);
} RvCCoreModuleFuncs;

/* Array of Init and End functions that core will execute. The Init */
/* functions will be called in the order of this array and the End */
/* functions will be called in reverse order. */
static const RvCCoreModuleFuncs RvCCoreModules[] = {
    { RvTimestampInit, RvTimestampEnd },
    { RvSemaphoreInit, RvSemaphoreEnd },
    { RvMutexInit, RvMutexEnd },
    { RvLockInit, RvLockEnd },
    { Rv64AsciiInit, Rv64AsciiEnd },
    { RvTmInit, RvTmEnd },
    { RvClockInit, RvClockEnd },
    { RvMemoryInit, RvMemoryEnd },
    { RvThreadInit, RvThreadEnd },
    { RvLogListenerInit, RvLogListenerEnd },
    { RvLogInit, RvLogEnd },
    { RvSelectInit, RvSelectEnd },
    { RvSocketInit, RvSocketEnd },
    { RvTransportInit, RvTransportEnd },    
#if (RV_IMS_IPSEC_ENABLED == RV_YES)        
    { RvIpsecInit, RvIpsecEnd },  
#endif    
    { RvHostInit, RvHostEnd },
    { RvTLSInit, RvTLSEnd },
    { RvAresInit, RvAresEnd },
    { RvStdioInit, RvStdioEnd },
    { RvNotifierInit, RvNotifierEnd },
    { NULL, NULL } /* List must end with NULLs */
};

/* Lets make error codes a little easier to type */
#define RvCCoreErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_CCORE, (_e))

/*static RvInt RvCCoreInitCount = 0; / * Use to make sure we only run once */
/*static RvInt RvCCoreNumModules; */

/********************************************************************************************
 * RvCCoreInit
 * Initializes  all of the modules in the ccore library. Must be
 * called before any other functions in this library are called.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 * REMARKS :
 * This function is not reentrant and it must not be called simultaneously
 * from multiple threads.
 * Further calls to this function will do nothing except keep track of the
 * number of times it has been called so that RvCCoreEnd will not actually
 * shut down until it has been called the same number of times.
 */
RvStatus RvCCoreInit(void)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;

    if(RvCCoreInitCount != 0) {  /* We're already running */
        RvCCoreInitCount++;
        return RV_OK;
    }

    /* Make sure that known type sizes have the actual "known" sizes for them */
    RvAssert(sizeof(RvInt8) == 1);
    RvAssert(sizeof(RvUint8) == 1);
    RvAssert(sizeof(RvInt16) == 2);
    RvAssert(sizeof(RvUint16) == 2);
    RvAssert(sizeof(RvInt32) == 4);
    RvAssert(sizeof(RvUint32) == 4);
    RvAssert(sizeof(RvInt64) == 8);
    RvAssert(sizeof(RvUint64) == 8);

    /* for Win32 we are going to find out the Windows version: NT / 2K / XP */
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    {
        OSVERSIONINFO versionInfo;

        memset(&versionInfo, 0, sizeof(versionInfo));
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
        result = GetVersionEx(&versionInfo);

        if (result > 0 && versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (versionInfo.dwMajorVersion <= 4)
                rvWindowsVersion = RV_OS_WIN32_NT4;
            else if (versionInfo.dwMajorVersion == 5 && versionInfo.dwMinorVersion == 0)
                rvWindowsVersion = RV_OS_WIN32_2000;
            else if (versionInfo.dwMajorVersion == 5 && versionInfo.dwMinorVersion == 1)
                rvWindowsVersion = RV_OS_WIN32_XP;
            else if (versionInfo.dwMajorVersion == 5 && versionInfo.dwMinorVersion == 2)
                rvWindowsVersion = RV_OS_WIN32_2003;
        }
    }
#endif
    
    RvCCoreNumModules = 0;
    for(;;) {
        if(RvCCoreModules[RvCCoreNumModules].init == NULL)
            break;
        result = RvCCoreModules[RvCCoreNumModules].init();

        if(result != RV_OK) {

            /* Something failed, call end for modules that have already had init called. */
            for(RvCCoreNumModules = RvCCoreNumModules - 1; RvCCoreNumModules >= 0; RvCCoreNumModules--)
                RvCCoreModules[RvCCoreNumModules].end();
            return result;
        }
        RvCCoreNumModules++;
    }

    RvCCoreInitCount++;
    return RV_OK;
}


/********************************************************************************************
 * RvCCoreEnd
 * Shuts down the all of the modules in the ccore library. No further
 * calls to any other functions in this library may be made after this
 * function is called.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : RV_OK if successful otherwise an error code.
 * REMARKS :
 * This function is not reentrant and it must not be called simultaneously
 * from multiple threads.
 * The shut down will not actually be performed until it has been called
 * the same number of times that RvCCoreInit was called.
 */
RvStatus RvCCoreEnd(void)
{
   RV_USE_CCORE_GLOBALS;
    RvStatus result, lastresult;

#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(RvCCoreInitCount <= 0)
        return RvCCoreErrorCode(RV_ERROR_UNKNOWN);
#endif

    if(RvCCoreInitCount > 1) { /* We're not at the last end yet */
        RvCCoreInitCount--;
        return RV_OK;
    }

    result = RV_OK;

    /* Go backwards for end. Don't stop for errors but save the first one we get. */
    for(RvCCoreNumModules = (RvCCoreNumModules - 1); RvCCoreNumModules >= 0; RvCCoreNumModules--) {
        lastresult = RvCCoreModules[RvCCoreNumModules].end();
        if(result == RV_OK)
            result = lastresult;
    }

    RvCCoreInitCount = 0;
    return result;
}


/********************************************************************************************
 * RvCCoreVersion
 * returns a pointer to a the version string for the ccore library.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : a pointer to a the version string for the ccore library.
 */
RVCOREAPI const RvChar * RVCALLCONV RvCCoreVersion(void)
{
    return RvCCoreVersionString;
}


/********************************************************************************************
 * RvCCoreInitialized
 * return a boolean answer, if ccore module has been already initialized.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : TRUE if ccore was already initialized, False otherwise,
 */
RVCOREAPI RvBool RVCALLCONV RvCCoreInitialized(void)
{
    RV_USE_CCORE_GLOBALS;
    return (RvCCoreInitCount > 0);
}


/********************************************************************************************
 * RvCCoreInterfaces
 * Get the compilation flags used with the common core.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : NULL terminated strings with the compilation flags used when the common core
 *           was compiled.
 * REMARKS :
 * Last string in this list is of length 0, so it's easy to know when to stop.
 * First 4 letters on each line are used for the Unix "what" utility, so you can skip them.
 */
RVCOREAPI const RvChar * RVCALLCONV RvCCoreInterfaces(void)
{
    return RvCCoreCompilationStrings;
}


/********************************************************************************************
 * RvCCoreWhat
 * Print out the compilation string.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : None.
 */
RVCOREAPI void RVCALLCONV RvCCoreWhat(void)
{
    RvChar* s = (RvChar*)RvCCoreInterfaces();

    while ((*s) != '\0')
    {
        /*RvPrintf("%s\n", s + 4);*/
        s += strlen(s) + 1;
    }
}


#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
/********************************************************************************************
 * RvWindowsVersion
 * Get Windows version of the platform we are running on.
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : Windows version: RV_OS_WIN32_NT4, RV_OS_WIN32_2000 or RV_OS_WIN32_XP.
 */
RVCOREAPI RvUint RVCALLCONV RvWindowsVersion(void)
{
    return rvWindowsVersion;
}
#endif
