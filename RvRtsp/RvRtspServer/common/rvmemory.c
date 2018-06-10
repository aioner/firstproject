/* rvmemory.c - memory management functions */
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
#include "rvmemory.h"
#include "rvlock.h"
#include "rvccoreglobals.h"
#include <string.h>

#if (RV_MEMORY_DEBUGINFO == RV_YES)
#include "rvthread.h"
#endif


/* This module does not need to be ported to different operating systems. */
/* Some of memory drivers do need to be ported. It is part of the ccore */
/* layer because the memory drivers are dependent upon its existance and */
/* so are some other modules at this layer. */

/* Structure containing definitions for driver calls. Each driver must provide all of these functions. */
/* The RvMemoryDriverFuncs typedef is in rvmemory.h because a forward declaration is needed. */
struct RvMemoryDriverFuncs_s {
    RvStatus (*init)(void);                                              /* Called once at startup. */
    RvStatus (*end)(void);                                               /* Called once at shutdown */
    RvStatus (*construct)(void *start, RvSize_t size, RvSize_t overhead, RvMemory *moremem, void *attr, void *driverRegion); /* Construct a memory region. */
    RvStatus (*destruct)(void *driverRegion);                            /* Destruct a memory region */
    RvStatus (*alloc)(void *driverRegion, RvSize_t size, void **result); /* Allocate from a memory region. */
    RvStatus (*free)(void *driverRegion, void *ptr);                     /* Deallocate from a memory region. */
    RvStatus (*info)(void *driverRegion, RvMemoryInfo *meminfo);         /* Get statistics about a memory region */
};

/* Array of available memory drivers. Must end with NULL pointers. */
/* NOTE that the first driver is used as the default and thus the  */
/* Init calls MUST NOT depend on any other memory drivers being available. */
static const RvMemoryDriverFuncs RvMemoryDrivers[] = {
    { RvOsMemInit, RvOsMemEnd, RvOsMemConstruct, RvOsMemDestruct, RvOsMemAlloc, RvOsMemFree, RvOsMemGetInfo },
#if (RV_MEMORY_TYPE == RV_MEMORY_POOL)
    { RvPoolMemInit, RvPoolMemEnd, RvPoolMemConstruct, RvPoolMemDestruct, RvPoolMemAlloc, RvPoolMemFree, RvPoolMemGetInfo },
#endif
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/* Lets make error codes a little easier to type */
#define RvMemoryErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_MEMORY, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvMemLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->memorySource, p);}
#else
#define RvMemLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvMemLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->memorySource, p);}
#else
#define RvMemLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvMemLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->memorySource, p);}
#else
#define RvMemLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_INFO)
#define RvMemLogInfo(p)  {if (logMgr != NULL) RvLogInfo(&logMgr->memorySource, p);}
#else
#define RvMemLogInfo(p)  {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define RvMemLogDebug(p) {if (logMgr != NULL) RvLogDebug(&logMgr->memorySource, p);}
#else
#define RvMemLogDebug(p)  {RV_UNUSED_ARG(logMgr);}
#endif


#if (RV_MEMORY_DEBUGCHECK == RV_YES)
#define RV_MEMORY_BOUNDRY_SIZE 16 /* minumum size */
#define RV_MEMORY_HEAD_FILL 0x5A  /* head boundry marker */
#define RV_MEMORY_TAIL_FILL 0xA5  /* tail boundry marker */
#define RV_MEMORY_ALLOC_FILL 0xAA /* alloced memory filler */
#define RV_MEMORY_FREE_FILL 0xDD  /* freed memory filler */
#else
#define RV_MEMORY_BOUNDRY_SIZE 0
#endif

/* Define additional data to be stored at the beginning of each allocation. */
/* The OVERHEAD size must account for the alignment of the ptr that */
/* will be returned to the user. */
typedef struct {
    RvMemory *region; /* always needed */
#if (RV_MEMORY_DEBUGINFO == RV_YES)
    RvObjListElement alloclistelem;
    RvInt line;
    const RvChar *filename;
    RvThread *thread;
    RvThreadId id;
#endif
#if (RV_MEMORY_DEBUGCHECK == RV_YES)
    RvBool freed;
    size_t reqsize;
    RvUint8 boundry[RV_MEMORY_BOUNDRY_SIZE]; /* Must be last item in structure */
#endif
} RvMemoryAllocHead;
#define RV_MEMORY_ALLOC_HEAD_OVERHEAD RvRoundToSize(sizeof(RvMemoryAllocHead), RV_ALIGN_SIZE)

/* Total overhead used by rvmemory for each allocation (including end */
/* boundry which is always the requested size. */
#define RV_MEMORY_ALLOC_OVERHEAD (RV_MEMORY_ALLOC_HEAD_OVERHEAD + RV_MEMORY_BOUNDRY_SIZE)

static RvStatus RvMemoryDefaultMemCB(RvSize_t size);

/*
static RvMemory RvDefaultRegion;     / * Default memory region: OsMem driver, also handle out-of-memory callback * /
static RvMemoryFunc RvUserDefaultCB; / * User changable out-of-memory callback for default region * /
static RvObjList RvRegionList;       / * List of currently constructed regions * /
static RvLock RvUserDefaultLock;     / * Use for locking RvUserDefaultCB * /
static RvLock RvMemoryLock;          / * Use for locking during construct/destruct * /
static RvStatus RvDriverStatus[RV_MEMORY_NUMDRIVERS]; / * Holds status of individual drivers * /
static RvInt rvActualNumDrivers;     / * Actual number of memory drivers * /
*/

/********************************************************************************************
 * RvMemoryInit - inits Initializes the Memory module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 * A default region is created with the default driver.
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvMemoryInit(void)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;

#if (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE)
    /* to avoid warnings (ghs) */
    RV_UNUSED_ARG(RvUserDefaultLock);
    RV_UNUSED_ARG(RvMemoryLock);
#endif

    /* Default driver (OS mem) is always present */
    rvActualNumDrivers = 1;

    /* Mark default driver so in case we fail we'll abort RvMemoryEnd */
    RvDriverStatus[0] = RvMemoryErrorCode(RV_ERROR_UNKNOWN);

    /* Set up basics */
    RvUserDefaultCB = NULL;
    result = RvLockConstruct(NULL, &RvUserDefaultLock);
    if(result != RV_OK)
        return result;
    result = RvLockConstruct(NULL, &RvMemoryLock);
    if(result != RV_OK) {
        RvLockDestruct(&RvUserDefaultLock,NULL);
        return result;
    }

    /* Set up region list */
    RvObjListConstruct(&RvDefaultRegion, &RvDefaultRegion.listElem, &RvRegionList);

    /* We need to set up the default region first, then initialize everything else */
    RvDriverStatus[0] = RvMemoryDrivers[0].init();
    if(RvDriverStatus[0] == RV_OK)
        RvDriverStatus[0] = RvMemoryConstruct(RV_MEMORY_DRIVER_OSMEM, (RvChar*)"Default", NULL, 0, NULL, RvMemoryDefaultMemCB, NULL,NULL, &RvDefaultRegion);
    if(RvDriverStatus[0] != RV_OK) {
        RvLockDestruct(&RvUserDefaultLock,NULL);
        RvLockDestruct(&RvMemoryLock,NULL);
        return RvDriverStatus[0];
    }

    /* Initialize the rest of the drivers and save the results */
    for (; RvMemoryDrivers[rvActualNumDrivers].init != NULL; ++rvActualNumDrivers)
        RvDriverStatus[rvActualNumDrivers] = RvMemoryDrivers[rvActualNumDrivers].init();

    return RV_OK;
}

/********************************************************************************************
 * RvMemoryEnd - Shuts down the Memory module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 * The default region is destroyed.
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvMemoryEnd(void)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;
    RvInt i;

    if(RvDriverStatus[0] != RV_OK)
        return RvDriverStatus[0];   /* No default driver, init must have failed badly */

    /* Call end functions in reverse order and only if init was successfull */
    for (i = rvActualNumDrivers - 1; i > 0; --i) {
        if (RvDriverStatus[i] == RV_OK)
            RvDriverStatus[i] = RvMemoryDrivers[i].end();
    }

    /* Destroy default region */
    result = RvMemoryDestruct(&RvDefaultRegion,NULL);

    /* Call end for default driver */
    RvDriverStatus[0] = RvMemoryDrivers[0].end();
    if(result == RV_OK)
        result = RvDriverStatus[0];

    /* No regions should be left */
    if((result == RV_OK) && (RvObjListSize(&RvRegionList) != 0))
       result = RvMemoryErrorCode(RV_ERROR_UNKNOWN);

    RvObjListDestruct(&RvRegionList);
    RvLockDestruct(&RvMemoryLock,NULL);
    RvLockDestruct(&RvUserDefaultLock,NULL);
    return result;
}

/********************************************************************************************
 * RvMemorySourceConstruct -Constructs memory module log source
 *
 * Constructs memory module log source for printing log per specific
 * log manager instance
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvMemorySourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus result;

    result = RvLogSourceConstruct(logMgr, &logMgr->memorySource, "MEMORY", "Dynamic memory allocation");

    return result;
}


/********************************************************************************************
 * RvMemoryConstruct - Creates a memory region.
 *
 * Different drivers may interpret some of the parameters
 * in different ways. Refer to those drivers for further information.
 * note:  A copy of the name string passed in is made. The maximum size of the
 *      the string is RV_MEMORY_MAX_NAMESIZE and names that are too long
 *      will be truncated.
 *
 * INPUT   : drivernum  - Driver ID of memory allocation driver to use.
 *                      See rvmemory.h for current list.
 *           name       - Pointer to string name to use for this region (a copy will be made).
 *           start      - Starting address of region (not used by all drivers).
 *           size       - Size to use for region (driver specific).
 *           moremem    - Memory region to get additional memory from (not used by all drivers).
 *           nomem      - Callback function that will be called if no more memory can be
 *                      aquired (NULL is allowed).
 *           attr       - Driver specific parameters.
 *           logMgr     - log manager instance
 * OUTPUT  : region     - Pointer to object where region information will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvMemoryConstruct(
    IN  RvInt           drivernum,
    IN  RvChar          *name,
    IN  void            *start,
    IN  RvSize_t        size,
    IN  RvMemory        *moremem,
    IN  RvMemoryFunc    nomem,
    IN  void            *attr,
    IN  RvLogMgr        *logMgr,
    OUT RvMemory        *region)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;
#if (RV_MEMORY_DEBUGINFO == RV_YES)
    RvMemoryAllocHead tmpheader;
#endif

    RvMemLogEnter((&logMgr->memorySource, "RvMemoryConstruct(region=0x%p)", region));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(region == NULL) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryConstruct: NULL param(s)"));
        return RvMemoryErrorCode(RV_ERROR_NULLPTR);
    }
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((drivernum < 0) || (drivernum >= rvActualNumDrivers)) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryConstruct: Range"));
        return RvMemoryErrorCode(RV_ERROR_OUTOFRANGE);
    }
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(RvDriverStatus[drivernum] != RV_OK) {
        RvMemLogError((&logMgr->memorySource,
            "RvMemoryConstruct: Status(%d)", RvDriverStatus[drivernum]));
        return RvMemoryErrorCode(RV_MEMORY_ERROR_DRIVERFAILED);
    }
#endif

    /* Create a list of allocated blocks for debugging and a lock for it. */
#if (RV_MEMORY_DEBUGINFO == RV_YES)
    result = RvLockConstruct(logMgr, &region->listlock);
    if(result != RV_OK) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryConstruct: RvLockConstruct"));
        return result;
    }
    if(RvObjListConstruct(&tmpheader, &tmpheader.alloclistelem, &region->alloclist) == NULL) {
        RvLockDestruct(&region->listlock,logMgr);
        RvMemLogError((&logMgr->memorySource, "RvMemoryConstruct: RvObjListConstruct"));
        return RvMemoryErrorCode(RV_ERROR_UNKNOWN);
    }
#endif

    /* Fill in region information with passed in info */
    region->drivernum = drivernum;
    region->drivercalls = &RvMemoryDrivers[drivernum];
    region->start = start;
    region->size = size;
    region->moremem = moremem;
    region->nomem = nomem;
    region->driverRegion = NULL; /* just in case */
    if(name != NULL) {
        strncpy(region->name, name, RV_MEMORY_MAX_NAMESIZE);
        region->name[RV_MEMORY_MAX_NAMESIZE - 1] = '\0';
    } else region->name[0] = '\0';

    /* Each driver needs to set the pointers for driverData in this switch statement. */
    /* We can't assume each union item aligns to the same start point. */
    switch(drivernum) {
    case RV_MEMORY_DRIVER_OSMEM:
        region->driverRegion = (void *)&region->driverData.osMemData;
        break;
#if (RV_MEMORY_TYPE == RV_MEMORY_POOL)
    case RV_MEMORY_DRIVER_POOLMEM:
        region->driverRegion = (void *)&region->driverData.poolMemData;
        break;
#endif
    }

    /* Call specific driver construct */
    result = region->drivercalls->construct(region->start, region->size, RV_MEMORY_ALLOC_OVERHEAD, region->moremem, attr, region->driverRegion);
    if(result != RV_OK) {
#if (RV_MEMORY_DEBUGINFO == RV_YES)
        /* Undo debugging stuff if construction failed. */
        RvObjListDestruct(&region->alloclist);
        RvLockDestruct(&region->listlock,logMgr);
#endif
        RvMemLogError((&logMgr->memorySource, "RvMemoryConstruct: driver construct"));
    }
    else {
        /* Do any operations that require the construct/destruct lock inside this block */
        RvLockGet(&RvMemoryLock,logMgr);

        /* Add to end region list */
        RvObjListInsertBefore(&RvRegionList, NULL, region);

        RvLockRelease(&RvMemoryLock,logMgr);
    }

    RvMemLogLeave((&logMgr->memorySource, "RvMemoryConstruct(region=%p)", region));

    return result;
}


/********************************************************************************************
 * RvMemoryDestruct - Destroys a memory region.
 *
 *  note:  Some drivers allow destroying regions with outstanding allocations
 *      and some do not.
 *
 * INPUT   : region -Pointer to the region object to be destroyed.
 *           logMgr - log manager instance
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvMemoryDestruct(
    IN RvMemory *region,
    IN RvLogMgr *logMgr)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;
    RvInt origdriver;

    RvMemLogEnter((&logMgr->memorySource, "RvMemoryDestruct(region=%p)", region));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(region == NULL) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryDestruct: NULL param(s)"));
        return RvMemoryErrorCode(RV_ERROR_NULLPTR);
    }
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(region->drivernum < 0) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryDestruct: Destructed"));
        return RvMemoryErrorCode(RV_ERROR_DESTRUCTED);
    }
    if(RvDriverStatus[region->drivernum] != RV_OK) {
        RvMemLogError((&logMgr->memorySource,
            "RvMemoryDestruct: Status(%d)", RvDriverStatus[region->drivernum]));
        return RvMemoryErrorCode(RV_MEMORY_ERROR_DRIVERFAILED);
    }
#endif

    /* Do any operations that require the construct/destruct lock inside this block */
    RvLockGet(&RvMemoryLock,logMgr);

    /* Check to see if the region already destructed. Since we don't need locking for */
    /* anything else in the structure, we assume drivernum is monotonic so we don't */
    /* have to lock anything when we check during alloc and free. Since we already */
    /* need to use the global lock we'll use it for modifying drivernum. */
    if(region->drivernum < 0) {
        RvLockRelease(&RvMemoryLock,logMgr);
        return RV_OK;
    }

    /* Remove region from list */
    RvObjListRemoveItem(&RvRegionList, region);

    origdriver = region->drivernum; /* save it for undo */
    region->drivernum = -1; /* indicate destruction */

    RvLockRelease(&RvMemoryLock,logMgr);

    result = region->drivercalls->destruct(region->driverRegion);

    if(result != RV_OK) {
        /* We couldn't destroy it so undo things. This leaves an unlocked */
        /* gap when the region looks destructed, then reappears, but since */
        /* the region shouldn't be in use while calling destruct and its a */
        /* failure mode, we don't worry about it. */
        RvLockGet(&RvMemoryLock,logMgr);
        region->drivernum = origdriver;
        RvObjListInsertBefore(&RvRegionList, NULL, region);
        RvLockRelease(&RvMemoryLock,logMgr);
        RvMemLogError((&logMgr->memorySource, "RvMemoryDestruct: driver destruct"));
    }

#if (RV_MEMORY_DEBUGINFO == RV_YES)
    else {
        /* If we sucessfully destructed, destroy the allocated block list and its lock. */
        RvLockGet(&region->listlock,logMgr);
        RvObjListDestruct(&region->alloclist);
        RvLockRelease(&region->listlock,logMgr);
        RvLockDestruct(&region->listlock,logMgr);
    }
#endif

    RvMemLogLeave((&logMgr->memorySource, "RvMemoryDestruct(region=%p)", region));

    return result;
}


/********************************************************************************************
 * RvMemoryAlloc - Allocates memory from a region.
 *
 * INPUT   : reqregion  - Pointer to the region to request the memory from (NULL =
 *                      default region).
 *           size       - Number of bytes of memory needed.
 *           logMgr     - log manager instance
 * OUTPUT  : resultptr  - Pointer to where the resulting memory pointer will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
#if (RV_MEMORY_DEBUGINFO == RV_NO)
RVCOREAPI
RvStatus RVCALLCONV RvMemoryAlloc(
    IN  RvMemory        *reqregion,
    IN  RvSize_t        size,
    IN  RvLogMgr        *logMgr,
    OUT void            **resultptr)
#else
/* Special Alloc for debugging */
RVCOREAPI
RvStatus RVCALLCONV RvMemoryAllocDbg(
    IN  RvMemory    *reqregion,
    IN  RvSize_t    size,
    IN  RvInt       line,
    IN  const RvChar      *filename,
    IN  RvLogMgr    *logMgr,
    OUT void        **resultptr)
#endif
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;
    RvMemory *region;
    RvSize_t realsize;
    RvMemoryAllocHead *allochead;
#if (RV_MEMORY_DEBUGCHECK == RV_YES)
    RvUint8 *allocend;
#endif

#if (RV_MEMORY_DEBUGINFO == RV_NO)
    RvMemLogEnter((&logMgr->memorySource, "RvMemoryAlloc(size=%d)", size));
#else
    RvMemLogEnter((&logMgr->memorySource, "RvMemoryAlloc(size=%d,%s:%d)", size, filename, line));
#endif

    /* To save time we don't lock region and assume the driver will */
    /* do any needed locking. This does imply that RvMemoryDestruct */
    /* can not be called until it is known that no other calls will */
    /* be made on that region. */

    /* If requested region is NULL, use the default region */
    region = reqregion;
    if(region == NULL)
        region = &RvDefaultRegion;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(resultptr == NULL) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryAlloc: NULL param(s)"));
        return RvMemoryErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    *resultptr = NULL;

#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(region->drivernum < 0) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryAlloc: Destructed"));
        return RvMemoryErrorCode(RV_ERROR_DESTRUCTED);
    }
    if(RvDriverStatus[region->drivernum] != RV_OK) {
        RvMemLogError((&logMgr->memorySource,
            "RvMemoryAlloc: Status(%d)", RvDriverStatus[region->drivernum]));
        return RvMemoryErrorCode(RV_MEMORY_ERROR_DRIVERFAILED);
    }
#endif

    /* Add overhead to needed size */
    realsize = size + RV_MEMORY_ALLOC_OVERHEAD;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(realsize < size) /* overhead caused us to wrap */
        return RvMemoryErrorCode(RV_ERROR_OUTOFRANGE);
#endif

    /* Call driver to allocate space and deal with out-of-memory callback */
    for(;;) {
        result = region->drivercalls->alloc(region->driverRegion, realsize, resultptr);

        /* Don't retry if we didn't run out of memory or the user callback doesn't return RV_OK */
        if((RvErrorGetCode(result) != RV_ERROR_OUTOFRESOURCES) || (region->nomem == NULL) || (region->nomem(realsize) != RV_OK))
            break;
    }

    if (result != RV_OK)
    {
        RvMemLogError((&logMgr->memorySource, "RvMemoryAlloc: driver alloc"));
    }
    else {
        /* Store pointer back to region along with any other info */
        /* and bump resultptr to proper position. */
        allochead = (RvMemoryAllocHead *)*resultptr;
        *resultptr = (void *)((RvInt8 *)*resultptr + RV_MEMORY_ALLOC_HEAD_OVERHEAD);
        allochead->region = region;

        RvMemLogInfo((&logMgr->memorySource, "RvMemoryAlloc(block allocated=%p)", *resultptr));

#if (RV_MEMORY_DEBUGINFO == RV_YES)
        /* Include additional debugging information */
        allochead->line = line;
        allochead->filename = filename;
        allochead->thread = RvThreadCurrentEx(RV_FALSE);
        allochead->id = RvThreadCurrentId();

        /* Add block to end of allocated blocks list. */
        if(RvLockGet(&region->listlock,logMgr) == RV_OK) {
            RvObjListInsertBefore(&region->alloclist, NULL, allochead);
            RvLockRelease(&region->listlock,logMgr);
        }
#endif

#if (RV_MEMORY_DEBUGCHECK == RV_YES)
        allochead->freed = RV_FALSE; /* Used to check for memory freed multiple times */

        /* Set boundry at head and at the end. The one at the head */
        /* be bigger than the request size because it goes to the */
        /* pointer to be returned, which is aligned. */
        allochead->reqsize = size;
        memset(&allochead->boundry[0], RV_MEMORY_HEAD_FILL, (RvSize_t)((RvUint8 *)*resultptr - (RvUint8 *)&allochead->boundry[0]));
        allocend = (RvUint8 *)*resultptr + size;
        memset(allocend, RV_MEMORY_TAIL_FILL, RV_MEMORY_BOUNDRY_SIZE);

        /* Set memory to make sure 0 isn't assumed. */
        memset(*resultptr, RV_MEMORY_ALLOC_FILL, size);
#endif
    }

    RvMemLogLeave((&logMgr->memorySource, "RvMemoryAlloc(size=%d)", size));

    return result;
}


/********************************************************************************************
 * RvMemoryFree - Returns allocated memory back to region it came from.
 *
 * INPUT   : ptr            - Pointer to allocated memory. Must be the same pointer returned
 *                          by rvMemoryAlloc.
 *           logMgrParam    - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvMemoryFree(
    IN void     *ptr,
    IN RvLogMgr *logMgrParam)
{
    RvStatus result;
    RvMemoryAllocHead *allochead;
    RvLogMgr* logMgr = logMgrParam;  /* logMgrParam might be located inside the allocated memory */
#if (RV_MEMORY_DEBUGCHECK == RV_YES)
    RvUint8 *tmpptr;
    size_t i;
#endif


    RvMemLogEnter((&logMgr->memorySource, "RvMemoryFree(address=%p)", ptr));

    /* To save time we don't lock region (see note in RvMemoryAlloc) */

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(ptr == NULL) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryFree: NULL param(s)"));
        return RvMemoryErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    /* Region pointer should be stored right before ptr */
    allochead = (RvMemoryAllocHead *)((RvInt8 *)ptr - RV_MEMORY_ALLOC_HEAD_OVERHEAD);

    /* Do boundry and sanity checking. */
#if (RV_MEMORY_DEBUGCHECK == RV_YES)
    /* Check to make sure block hasn't already been freed. */
    if(allochead->freed != RV_FALSE) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryFree: freed"));
        return RvMemoryErrorCode(RV_ERROR_UNKNOWN);
    }
    allochead->freed = RV_TRUE; /* Mark as freed */

    /* Check header boundry (variable size). */
    tmpptr = (RvUint8 *)&allochead->boundry[0];
    while(tmpptr != (RvUint8 *)ptr) {
        if(*tmpptr != RV_MEMORY_HEAD_FILL) {
            RvMemLogError((&logMgr->memorySource, "RvMemoryFree: Head overrun"));
            return RvMemoryErrorCode(RV_MEMORY_ERROR_OVERRUN);
        }
        tmpptr += 1;
    }

    /* Check boundry at end (always requested size). */
    tmpptr = (RvUint8 *)ptr + allochead->reqsize;
    for(i = 0; i < RV_MEMORY_BOUNDRY_SIZE; i++) {
        if(*tmpptr != RV_MEMORY_TAIL_FILL) {
            RvMemLogError((&logMgr->memorySource, "RvMemoryFree: Tail verrun"));
            return RvMemoryErrorCode(RV_MEMORY_ERROR_OVERRUN);
        }
        tmpptr += 1;
    }

    /* Overwrite free memory to insure its not still being used. */
    memset(ptr, RV_MEMORY_FREE_FILL, allochead->reqsize);
#endif

#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    {
        RV_USE_CCORE_GLOBALS;
        if(allochead->region->drivernum < 0) {
            RvMemLogError((&logMgr->memorySource, "RvMemoryFree: Destructed"));
            return RvMemoryErrorCode(RV_ERROR_DESTRUCTED);
        }
        if(RvDriverStatus[allochead->region->drivernum] != RV_OK) {
            RvMemLogError((&logMgr->memorySource,
                "RvMemoryFree: Status(%d)", RvDriverStatus[allochead->region->drivernum]));
            return RvMemoryErrorCode(RV_MEMORY_ERROR_DRIVERFAILED);
        }
    }
#endif

#if (RV_MEMORY_DEBUGINFO == RV_YES)
    /* Remove block from allocated block list */
    if(RvLockGet(&allochead->region->listlock,logMgr) == RV_OK) {
        RvObjListRemoveItem(&allochead->region->alloclist, allochead);
        RvLockRelease(&allochead->region->listlock,logMgr);
    }
#endif

    result = (allochead->region->drivercalls->free)(allochead->region->driverRegion, allochead);

    RvMemLogLeave((&logMgr->memorySource, "RvMemoryFree(address=%p; status=%d)", ptr, result));

    return result;
}


/********************************************************************************************
 * RvMemoryGetInfo - Returns statistics about a memory region.
 *
 * INPUT   : reqregion  - Pointer to the region to request information from
 *                      (NULL = default region).
 * OUTPUT  : memory     - Pointer to a structure which will be filled in with the statistics.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvMemoryGetInfo(
    IN  RvMemory        *reqregion,
    IN  RvLogMgr        *logMgr,
    OUT RvMemoryInfo    *meminfo)
{
    RvStatus result;
    RvMemory *region;
    RV_USE_CCORE_GLOBALS;

    RvMemLogEnter((&logMgr->memorySource, "RvMemoryGetInfo"));

    /* If requested region is NULL, use the default region */
    region = reqregion;
    if(region == NULL)
        region = &RvDefaultRegion;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(meminfo == NULL) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryGetInfo: NULL param(s)"));
        return RvMemoryErrorCode(RV_ERROR_NULLPTR);
    }
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(region->drivernum < 0) {
        RvMemLogError((&logMgr->memorySource, "RvMemoryGetInfo: Destructed"));
        return RvMemoryErrorCode(RV_ERROR_DESTRUCTED);
    }
    if(RvDriverStatus[region->drivernum] != RV_OK) {
        RvMemLogError((&logMgr->memorySource,
            "RvMemoryGetInfo: Status(%d)", RvDriverStatus[region->drivernum]));
        return RvMemoryErrorCode(RV_MEMORY_ERROR_DRIVERFAILED);
    }
#endif

    /* We fill in the first 2 fields and pass the rest into the driver */
    strcpy(meminfo->name, region->name);
    meminfo->drivernum = region->drivernum;

    result = region->drivercalls->info(region->driverRegion, meminfo);

    RvMemLogLeave((&logMgr->memorySource, "RvMemoryGetInfo"));

    return result;
}


#if RV_MEMORY_DEBUGINFO == RV_YES

RVCOREAPI
RvStatus RVCALLCONV RvMemoryPrintDbg(
    IN  RvMemory    *reqregion,
    IN  RvLogMgr    *logMgr)
{
    RvMemory *region;
    RV_USE_CCORE_GLOBALS;

#if RV_MEMORY_DEBUGCHECK == RV_YES
    RvSize_t totalActive = 0;
    RvSize_t totalAllocations = 0;
#endif


    RvMemLogDebug((&logMgr->memorySource, "Current allocations in region=%p", reqregion));

    /* If requested region is NULL, use the default region */
    region = reqregion;
    if (region == NULL)
        region = &RvDefaultRegion;

    if (RvLockGet(&region->listlock,logMgr) == RV_OK)
    {
        RvMemoryAllocHead *cur;
        cur = (RvMemoryAllocHead *)RvObjListGetNext(&region->alloclist, NULL, RV_OBJLIST_LEAVE);
        while (cur != NULL)
        {
#if RV_MEMORY_DEBUGCHECK == RV_YES
            RvMemLogDebug((&logMgr->memorySource, "Allocated %d byte starting at %p in %s:%d",
                   cur->reqsize, (RvInt8 *)cur + RV_MEMORY_ALLOC_HEAD_OVERHEAD, cur->filename, cur->line));

            totalActive += cur->reqsize;
            totalAllocations++;
#else
            RvMemLogDebug((&logMgr->memorySource, "Allocated %p in %s:%d",
                   (RvInt8 *)cur + RV_MEMORY_ALLOC_HEAD_OVERHEAD, cur->filename, cur->line));
#endif


            cur = (RvMemoryAllocHead *)RvObjListGetNext(&region->alloclist, cur, RV_OBJLIST_LEAVE);
        }
#if RV_MEMORY_DEBUGCHECK == RV_YES
        RvMemLogDebug((&logMgr->memorySource, "Total currently allocated %d byte in %d allocations", (RvInt)totalActive, (RvInt)totalAllocations));
#endif
        RvLockRelease(&region->listlock,logMgr);
    }

    return RV_OK;
}
#endif

#if RV_MEMORY_DEBUGCHECK == RV_YES

RVCOREAPI
RvStatus RVCALLCONV RvMemoryCheckOverrun(
	IN void		*ptr,
	IN RvLogMgr	*logMgrParam)
{
    RvMemoryAllocHead *allochead;
    RvLogMgr* logMgr = logMgrParam;  /* logMgrParam might be located inside the allocated memory */
    RvUint8 *tmpptr;
    size_t i;

    /* Region pointer should be stored right before ptr */
    allochead = (RvMemoryAllocHead *)((RvInt8 *)ptr - RV_MEMORY_ALLOC_HEAD_OVERHEAD);

    /* Do boundry and sanity checking. */
    /* Check header boundry (variable size). */
    tmpptr = (RvUint8 *)&allochead->boundry[0];
    while(tmpptr != (RvUint8 *)ptr) {
        if(*tmpptr != RV_MEMORY_HEAD_FILL) {
            RvMemLogError((&logMgr->memorySource, "RvMemoryCheckOverrun: Head overrun"));
            return RvMemoryErrorCode(RV_MEMORY_ERROR_OVERRUN);
        }
        tmpptr += 1;
    }

    /* Check boundry at end (always requested size). */
    tmpptr = (RvUint8 *)ptr + allochead->reqsize;
    for(i = 0; i < RV_MEMORY_BOUNDRY_SIZE; i++) {
        if(*tmpptr != RV_MEMORY_TAIL_FILL) {
            RvMemLogError((&logMgr->memorySource, "RvMemoryCheckOverrun: Tail overrun"));
            return RvMemoryErrorCode(RV_MEMORY_ERROR_OVERRUN);
        }
        tmpptr += 1;
    }

    return RV_OK;
}
#endif

/********************************************************************************************
 * RvMemorySetDefaultMemCB - Sets an out of memory callback for the default region.
 *
 * INPUT   : func   - Out of memory callback (NULL = none).
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvMemorySetDefaultMemCB(
    IN RvMemoryFunc func)
{
    RvStatus result;
    RV_USE_CCORE_GLOBALS;

    result = RvLockGet(&RvUserDefaultLock,NULL);
    if(result == RV_OK) {
        RvUserDefaultCB = func;
        RvLockRelease(&RvUserDefaultLock,NULL);
    }
    return result;
}


/********************************************************************************************
 * RvMemoryGetDefaultMemCB - Gets the current out of memory callback for the default region.
 *
 * INPUT   : none
 * OUTPUT  : func   - Function pointer where the callback pointer is stored
 *                  (result can be NULL).
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvMemoryGetDefaultMemCB(
    OUT RvMemoryFunc *func)
{
    RvStatus result;
    RV_USE_CCORE_GLOBALS;

    result = RvLockGet(&RvUserDefaultLock,NULL);
    if(result == RV_OK) {
        *func = RvUserDefaultCB;
        RvLockRelease(&RvUserDefaultLock,NULL);
    }
    return result;
}




/********************************************************************************************
 * RvMemoryDefaultMemCB - Out-of-memory callback for default memory region.
 *
 * It calls the function set by the user.
 *
 * INPUT   : size   - size that stack wanted to allocate
 * OUTPUT  : none
 * RETURN  : RV_OK if memory allocation should be tried again.
 */
static RvStatus RvMemoryDefaultMemCB(
    IN RvSize_t size)
{
    RvStatus result;
    RV_USE_CCORE_GLOBALS;

    result = RvLockGet(&RvUserDefaultLock,NULL);
    if(result == RV_OK) {
        if(RvUserDefaultCB != NULL) {
            result = RvUserDefaultCB(size);
        } else result = RvMemoryErrorCode(RV_ERROR_OUTOFRESOURCES);
        RvLockRelease(&RvUserDefaultLock,NULL);
    }
    return result;
}
