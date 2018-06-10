/***********************************************************************
Filename   : rvosmem.c
Description: memdriver for dynamic memory allocation from OS itself
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

/* Block Pool memory manager. Not currently OS specific (so it doesn't */
/* have to be ported but could be modified to use an OS specific block pool */
/* manager if needed. */

#include "rvpoolmem.h"

#if (RV_MEMORY_TYPE == RV_MEMORY_POOL)

/* Default attribute values if user passes in NULL attr pointer */
static RvPoolMemAttr RvPoolMemDefaultAttr = {
    RV_OBJPOOL_TYPE_EXPANDING, /* pooltype */
    32,                        /* pageitems */
    0,                         /* pagesize */
    0,                         /* maxblocks */
    0,                         /* minblocks */
    0,                         /* freelevel */
    0,                         /* startblocks */
    0                          /* startpages */
};

/* Item that is put into the pool, since we just want raw memory */
/* we only need the pool element. */
typedef struct {
    RvObjPoolElement poolelem;
} RvPoolMemElem;

static void *RvPoolMemGetPage(RvSize_t size, void *data);
static void RvPoolMemFreePage(void *ptr, void *data);

/* Lets make error codes a little easier to type */
#define RvPoolMemErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_POOLMEM, (_e))

/********************************************************************************************
 * RvPoolMemInit - Initializes the PoolMem module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemInit(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvPoolMemEnd - Shuts down the PoolMem module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemEnd(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvPoolMemConstruct - Creates a pool driver memory region.
 *
 * A "pool" memory region is managed by RvObjPool module.
 * This function constructs a ObjPool object which will satisfies alloc requests.
 *
 * INPUT   : start        - Starting address of region (ignored).
 *           size         - Block size.
 *           overhead     - RvMemory overhead.
 *           moremem      - Memory region to get additional memory from.
 *           attr         - Pool configuration.
 * OUTPUT  : driverRegion - Pointer to object where region information will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemConstruct(
    IN  void*        start,
    IN  RvSize_t     size,
    IN  RvSize_t     overhead,
    IN  RvMemory*    moremem,
    IN  void*        attr,
    OUT void*        driverRegion)
{
    RvStatus result;
    RvPoolMemData *pool;
    RvPoolMemElem tempobj;
    RvPoolMemAttr *poolattr;
    RvObjPoolFuncs callbacks;
    RvBool salvage;
    size_t numblocks, numpages;

    RV_UNUSED_ARG(start);

    pool = (RvPoolMemData *)driverRegion;
    poolattr = (RvPoolMemAttr *)attr;
    if(poolattr == NULL)
        poolattr = &RvPoolMemDefaultAttr; /* Use default attributes */

    /* Each region will maintin its own lock, so create it */
    result = RvLockConstruct(NULL, &pool->lock);
    if(result != RV_OK)
        return result;

    /* save some info which is only used later for calculating statistics */
    pool->usersize = size;
    pool->overhead = overhead;
    pool->moremem = moremem;
    pool->pooltype = poolattr->pooltype;

    /* Calculate minimum needed blocksize: requested + overhead. */
    /* Must be >= sizeof(RvPoolMemElem). The real blocksize will */
    /* be calculated by rvobjpool. */
    pool->blocksize = size + overhead;
    if (pool->blocksize < sizeof(RvPoolMemElem))
        pool->blocksize = sizeof(RvPoolMemElem);

    /* Set pooldata appropriately based on the pool type. */
    if(poolattr->pooltype == RV_OBJPOOL_TYPE_DYNAMIC) {
        salvage = RV_OBJPOOL_SALVAGE_ALLOWED;
    } else salvage = RV_OBJPOOL_SALVAGE_NEVER; /* Saves overhead */

    /* Construct object pool. Object is actually empty memory so no constructor/destructor. */
    memset(&callbacks, 0, sizeof(RvObjPoolFuncs));
    callbacks.objconstruct = NULL;
    callbacks.objdestruct = NULL;
    callbacks.pagealloc = RvPoolMemGetPage;
    callbacks.pageallocdata = (void *)moremem;
    callbacks.pagefree = RvPoolMemFreePage;
    callbacks.pagefreedata = (void *)moremem;
    if(RvObjPoolConstruct(&tempobj, &tempobj.poolelem, &callbacks, pool->blocksize, poolattr->pageblocks, poolattr->pagesize, poolattr->pooltype, salvage, poolattr->maxblocks, poolattr->minblocks, poolattr->freelevel, &pool->objpool) == NULL) {
        /* Can't construct pool, abort */
        RvLockDestruct(&pool->lock, NULL);
        return RvPoolMemErrorCode(RV_ERROR_UNKNOWN);
    }

    /* Allocate starting blocks or pages. */
    result = RV_OK;
    if(poolattr->startblocks > RvObjPoolTotalItems(&pool->objpool)) {
        numblocks = poolattr->startblocks - RvObjPoolTotalItems(&pool->objpool);
        if(RvObjPoolAddItems(&pool->objpool, numblocks) < numblocks)
            result = RvPoolMemErrorCode(RV_ERROR_UNKNOWN);
    } else {
        /* Only use startpages if we're not using startblocks */
        if(poolattr->startpages > RvObjPoolTotalPages(&pool->objpool)) {
            numpages = poolattr->startpages - RvObjPoolTotalPages(&pool->objpool);
            if(RvObjPoolAddPages(&pool->objpool, numpages) < numpages)
                result = RvPoolMemErrorCode(RV_ERROR_UNKNOWN);
        }
    }
    if(result != RV_OK) {
        /* Couldn't create starting blocks/pages so undo construct. */
        RvObjPoolDestruct(&pool->objpool);
        RvLockDestruct(&pool->lock, NULL);
    }

    return result;
}

/********************************************************************************************
 * RvPoolMemDestruct - Destroys a pool driver memory region.
 *
 * Destructs the ObjPool object that has been created by RvPoolMemConstruct().
 *
 * INPUT   : driverRegion - Pointer to the region object to be destroyed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemDestruct(
    IN void*        driverRegion)
{
    RvPoolMemData *pool;
    RvStatus result = RV_OK;

    pool = (RvPoolMemData *)driverRegion;

    RvLockGet(&pool->lock, NULL);

    if (RvObjPoolDestruct(&pool->objpool) == RV_FALSE) {
        /* PoolDestruct failed so don't destroy anything */
        result = RvPoolMemErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLockRelease(&pool->lock, NULL);
    RvLockDestruct(&pool->lock, NULL);

    return result;
}

/********************************************************************************************
 * RvPoolMemAlloc - Allocates memory from a pool driver region.
 *
 * Use the ObjPool facilities to allocate a block of memory from the ObjPool object.
 *
 * INPUT   : driverRegion - Pointer to the region object.
 *           size         - Number of bytes of memory needed (used only to verify that the size
 *                          of the requested block is not bigger than the region block size).
 * OUTPUT  : result       - Pointer to where the resulting memory pointer will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemAlloc(
    IN  void*       driverRegion,
    IN  RvSize_t    size,
    OUT void**      result)
{
    RvPoolMemData *pool;
    void *ptr;
    RvStatus status;

    pool = (RvPoolMemData *)driverRegion;
    status = RvLockGet(&pool->lock, NULL);
    if(status != RV_OK)
        return status;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(size > pool->blocksize) {
        RvLockRelease(&pool->lock, NULL);
        return RvPoolMemErrorCode(RV_ERROR_OUTOFRANGE);
    }
#endif

    ptr = RvObjPoolGetItem(&pool->objpool);
    if(ptr == NULL) {
        status = RvPoolMemErrorCode(RV_ERROR_OUTOFRESOURCES);
    } else *result = ptr;
    RvLockRelease(&pool->lock, NULL);
    return status;
}

/********************************************************************************************
 * RvPoolMemFree - Frees a memory which was allocated from a pool driver region.
 *
 * Use the ObjPool facilities to release a block of memory back to the ObjPool object.
 *
 * INPUT   : driverRegion - Pointer to the region object.
 *           ptr          - Pointer to the allocated memory to be released.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemFree(
    IN  void*       driverRegion,
    IN  void*       ptr)
{
    RvPoolMemData *pool;
    RvStatus result;

    pool = (RvPoolMemData *)driverRegion;
    result = RvLockGet(&pool->lock, NULL);
    if(result == RV_OK) {
        RvObjPoolReleaseItem(&pool->objpool, ptr);
        RvLockRelease(&pool->lock, NULL);
    }
    return result;
}

/* Allocate new page, data points to memory region to allocate from. */
static void *RvPoolMemGetPage(RvSize_t size, void *data)
{
    void *result;
    RvMemory *pagemem;

    pagemem = (RvMemory *)data;
    if (RvMemoryAlloc(pagemem, size, NULL, &result) != RV_OK)
        result = NULL;
    return result;
}

/* Free page, data points to memory region it came from (but we don't need it). */
static void RvPoolMemFreePage(void *ptr, void *data)
{
    RV_UNUSED_ARG(data);
    RvMemoryFree(ptr, NULL);
}

/********************************************************************************************
 * RvPoolMemGetInfo - Returns the collected statistic information about a memory region.
 *
 * Since there's no run-time additional overhead we can always return statistics regardless
 * of the RV_MEMORY_KEEPSTATS flag except for those that depend on other memory regions
 * (which may not be keeping them).
 *
 * INPUT   : driverRegion - Pointer to the region object where statistic data is stored.
 * OUTPUT  : result       - Pointer to RvMemoryInfo structure where the statistic data
 *                          will be copied into.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvPoolMemGetInfo(
    IN  void*           driverRegion,
    OUT RvMemoryInfo*   meminfo)
{
    RvPoolMemData *pool;
    RvStatus result;
    RvMemoryInfo morememinfo;
    RvSize_t newblocks, newpages;

    pool = (RvPoolMemData *)driverRegion;
    result = RvLockGet(&pool->lock, NULL);
    if(result != RV_OK)
        return result;

    meminfo->allocs_free_now = RvObjPoolFreeItems(&pool->objpool);
    meminfo->bytes_free_now = meminfo->allocs_free_now * pool->usersize;
    meminfo->allocs_requested = RvObjPoolTotalItems(&pool->objpool) - meminfo->allocs_free_now;
    meminfo->bytes_requested_byuser = meminfo->allocs_requested * pool->usersize;
    meminfo->bytes_requested_bymem = meminfo->allocs_requested * (pool->usersize + pool->overhead);
    meminfo->bytes_requested_total = meminfo->allocs_requested * RvObjPoolItemBlockSize(&pool->objpool);
    meminfo->bytes_total_inuse = RvObjPoolTotalPages(&pool->objpool) * RvObjPoolPageSize(&pool->objpool);

    meminfo->allocs_free_total = meminfo->allocs_free_now;
    meminfo->bytes_free_total = meminfo->bytes_free_now;
    if(pool->pooltype != RV_OBJPOOL_TYPE_FIXED) {
        /* Estimate number of blocks and memory that can be aquired. */
        /* Doesn't account for fragmentation that might cause a page allocation to fail */
        result = RvMemoryGetInfo(pool->moremem, NULL, &morememinfo);
        if(result == RV_OK) {
            newpages = morememinfo.bytes_free_total / RvObjPoolPageSize(&pool->objpool);
            if(morememinfo.allocs_free_total < newpages)
               newpages = morememinfo.allocs_free_total;
            newblocks = newpages * RvObjPoolItemsPerPage(&pool->objpool);
            if(newblocks > RvObjPoolGetMaxitems(&pool->objpool)) {
                newpages = RvObjPoolGetMaxitems(&pool->objpool) / RvObjPoolItemsPerPage(&pool->objpool);
                newblocks = newpages * RvObjPoolItemsPerPage(&pool->objpool);
            }
            meminfo->allocs_free_total += newblocks;
            meminfo->bytes_free_total += (newblocks * pool->usersize);
        }
    }

    RvLockRelease(&pool->lock, NULL);
    return result;
}

#else
int prevent_warning_of_ranlib_has_no_symbols_rvpoolmem=0;
#endif /* #if (RV_MEMORY_TYPE == RV_MEMORY_POOL) */

