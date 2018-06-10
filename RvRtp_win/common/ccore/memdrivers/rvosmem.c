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

/* Basic dynamic memory allocation via OS standard calls. Regions and */
/* attributes are ignored. */

#include "rvosmem.h"

#if (RV_OSMEM_TYPE == RV_OSMEM_MALLOC)
#include <stdlib.h>

#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
#include "mmb_rv.h"
#define osmalloc(t)   Mmb_malloc(t)
#define osfree(t)     Mmb_free(t)
#endif

#endif

/* for osmem-type MALLOC, and VXWORKS */
#define osmalloc(t)   malloc(t)
#define osfree(t)     free(t)


#if (RV_OSMEM_TYPE == RV_OSMEM_VXWORKS)
#include <stdlib.h>
#include <memLib.h>
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_OSE)
#include "heapapi.h"
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_NUCLEUS)
#include <nucleus.h>
/* In the Nucleus environment, System_Memory is expected to have been */
/* created for us and be the default memory pool. */
extern NU_MEMORY_POOL System_Memory;
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
#include <psos.h>
#include "rvobjpool.h"

/* pSOS malloc is thread specific so we have to allocate memory */
/* segments directly. Since these segments are only available in large */
/* blocks we maintain pools of smallers blocks (power of 2 up to segment */
/* size) and allocate from the appropriate pool. Large blocks are just */
/* passed directly to pSOS. */

/* Configuration is via the following hardcoded definitions but could */
/* be extended to pass the values in during construction to allow for */
/* different pools in different pSOS memory regions. */

/* Which psos memory region to allocate segments from. */
#define PSOS_MEM_REGION 0

/* Smallest sub-allocation buffer pool to create. Must be a power */
/* of 2 and >= sizeof(RvOsMemPsosPoolElem). Also look at the size */
/* of RV_PSOS_BLOCK_OVERHEAD to see how much of the block is used */
/* for overhead. */
#define PSOS_MEM_MINBUFSIZE 32

/* Indicate what type of Object Pools and options to use (see rvobjpool.h). */
/* Also indicate how pool pages should relate to pSOS segments. Its a good */
/* idea to make (segmentsize * PAGESIZE) >= (MINBUFSIZE * 8) where segmentsize */
/* is the segment size of the pSOS region being used. */
#define PSOS_OBJPOOL_TYPE RV_OBJPOOL_TYPE_EXPANDING
#define PSOS_OBJPOOL_SALVAGE RV_OBJPOOL_SALVAGE_NEVER  /* Uses less overhead */
#define PSOS_OBJPOOL_MAXITEMS 0 /* Use all available memory */
#define PSOS_OBJPOOL_MINITEMS 0 /* No minimum level */
#define PSOS_OBJPOOL_FREELEVEL 0 /* not used unless pool is DYNAMIC */
#define PSOS_OBJPOOL_PAGESIZE 2 /* number of pSOS segments to use for each page */

/* Overhead of objpool pointer (sourcepool in RvOsMemPsosPoolElem) that needs to */
/* be stored with each block to know which pool to return it to. The OVERHEAD size */
/* must account for the alignment of the ptr that will be returned to the user. */
#define RV_PSOS_BLOCK_OVERHEAD RV_ALIGN_SIZE

/* Item that is put into the pool, only the sourcepool part of the */
/* structure is "overhead" unless the item is smaller than poolelem. */
typedef struct {
    RvOsMemPsosPool *sourcepool; /* ptr to pool block came from */
    RvObjPoolElement poolelem;   /* pool element */
} RvOsMemPsosPoolElem;

/* Info about each pool to be kept in the array. Typedef RvOsMemPsosPool is in header. */
struct RvOsMemPsosPool_s {
    RvLock lock;
    RvSize_t realsize; /* requested size being stored in this pool */
    RvObjPool objpool; /* object pool iteself */
};

static void *RvOsMemPsosBlockConstruct(void *objpool, void *data);
static void *RvOsMemPsosGetSeg(RvSize_t size, void *data);
static void RvOsMemPsosFreeSeg(void *ptr, void *data);
static RvStatus RvOsMemPsosConstructPools(RvOsMemData *region, unsigned long minbufsize);
#endif /* pSOS */

#if  (RV_OSMEM_TYPE == RV_OSMEM_SYMBIAN)
#include "rvsymbianinf.h"
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
/* OS assigned reference to the pool */
static OSPoolRef poolRef;

/* pointer to start of pool memory */
static UINT8*    poolBase;
#endif  /* OSA */
/* Size of overhead on each block needed to keep stats. Must be aligned. */
#define RV_OSMEM_STATS_OVERHEAD RV_ALIGN_SIZE

/* Lets make error codes a little easier to type */
#define RvOsMemErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_OSMEM, (_e))

/********************************************************************************************
 * RvOsMemInit - Initializes the OsMem module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemInit(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvOsMemEnd - Shuts down the OsMem module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemEnd(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvOsMemConstruct - Creates a system driver memory region.
 *
 * If statistic information is required - initializes the statistics area and constructs
 * a lock to protect this area.
 *
 * INPUT   : start        - Starting address of region (ignored).
 *           size         - Size to use for region (ignored).
 *           overhead     - RvMemory overhead (used only when statistics is on).
 *           moremem      - Memory region to get additional memory from (ignored).
 *           attr         - Region attributes (ignored).
 * OUTPUT  : driverRegion - Pointer to object where region information will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemConstruct(
    IN  void*        start,
    IN  RvSize_t     size,
    IN  RvSize_t     overhead,
    IN  RvMemory*    moremem,
    IN  void*        attr,
    OUT void*        driverRegion)
{
    RvStatus result;
#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    RvOsMemData *region;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    struct rninfo rbuf;
    unsigned long bufsize;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    OSA_STATUS status;
#endif

    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(moremem);
    RV_UNUSED_ARG(size);
    RV_UNUSED_ARG(start);
#if (RV_MEMORY_KEEPSTATS == RV_NO)
    RV_UNUSED_ARG(overhead);
#endif

    result = RV_OK;
#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    region = (RvOsMemData *)driverRegion;
#else
    RV_UNUSED_ARG(driverRegion);
#endif

    /* Most OS's have just one big heap but we'll maintain stats */
    /* for each logical region. */
#if (RV_MEMORY_KEEPSTATS == RV_YES)
    result = RvLockConstruct(NULL, &region->lock);
    if(result != RV_OK)
        return result;
    region->overhead = overhead;
    region->allocs_requested = 0;
    region->bytes_requested = 0;
    region->bytes_used = 0;
#endif

    /* Since we're doing our own management for pSOS, we'll keep */
    /* a separate set of pools for each region. */
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    region->psos_region = PSOS_MEM_REGION;

    /* Ask pSOS to find out segment size of region. */
    if(rn_info(region->psos_region, &rbuf) == 0) {
        region->segment_size = rbuf.unit_size;

        /* figure out number of buffer pools to use */
        region->numpools = 0;
        for(bufsize = PSOS_MEM_MINBUFSIZE; bufsize < region->segment_size; bufsize *= 2)
            region->numpools++;

        /* Construct the pools */
        result = RvOsMemPsosConstructPools(region, PSOS_MEM_MINBUFSIZE);
        if(result != RV_OK) {
            region->mempools = NULL;
            region->numpools = 0;
        }
    } else result = RvOsMemErrorCode(RV_ERROR_UNKNOWN); /* rn_info failed */
#endif/* pSOS */
#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)

    poolBase = (UINT8*)osmalloc(RV_OSA_MEMORY_POOL_SIZE);
    if (poolBase != NULL)
    {
        status = OSAMemPoolCreate(&poolRef, OS_VARIABLE, poolBase, RV_OSA_MEMORY_POOL_SIZE, 0, OS_PRIORITY);
        if (status != OS_SUCCESS)
            result = RvOsMemErrorCode(RV_ERROR_UNKNOWN);
    }
    else
    {
        result = RvOsMemErrorCode(RV_ERROR_UNKNOWN);
    }
#endif

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    if(result != RV_OK)
        RvLockDestruct(&region->lock,NULL);
#endif
    return result;
}

/********************************************************************************************
 * RvOsMemDestruct - Destroys a system driver memory region.
 *
 * If statistic information is required - destructs the statistics area and its lock.
 *
 * INPUT   : driverRegion - Pointer to the region object to be destroyed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemDestruct(
    IN void*        driverRegion)
{
    RvStatus result = RV_OK;
#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    RvOsMemData *region;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    RvInt pool;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    OSA_STATUS status;
#endif

#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    region = (RvOsMemData *)driverRegion;
#else
    RV_UNUSED_ARG(driverRegion);
#endif

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    result = RvLockDestruct(&region->lock,NULL);
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    if(region->mempools != NULL) {
        for(pool = 0; pool < region->numpools; pool++)
            if(RvObjPoolDestruct(&region->mempools[pool].objpool) == RV_FALSE)
                result = RvOsMemErrorCode(RV_ERROR_UNKNOWN); /* Just report it. */
        rn_retseg(region->psos_region, region->mempools);
        region->numpools = 0;
        region->mempools = NULL;
    }
#endif /* pSOS */

#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    status = OSAMemPoolDelete(poolRef);
    if (status != OS_SUCCESS)
        result = RvOsMemErrorCode(RV_ERROR_UNKNOWN);
#endif
    return result;
}

/********************************************************************************************
 * RvOsMemAlloc - Allocates memory from a system driver region.
 *
 * Use the underlying OS services to allocate a block of memory from the system heap.
 *
 * INPUT   : driverRegion - Pointer to the region object.
 *           size         - Number of bytes of memory needed.
 * OUTPUT  : result       - Pointer to where the resulting memory pointer will be stored.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemAlloc(
    IN  void*       driverRegion,
    IN  RvSize_t    size,
    OUT void**      result)
{
    void *ptr;
    RvSize_t memsize;
#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    RvOsMemData *region;
#endif

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    RvSize_t bytesused;
    RvSize_t *blocksize;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_NUCLEUS)
    int status;
#endif
#if  (RV_OSMEM_TYPE == RV_OSMEM_SYMBIAN)
	RvStatus status;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    RvInt pool;
    RvOsMemPsosPoolElem *block;
#if (RV_MEMORY_KEEPSTATS == RV_YES)
    RvSize_t segmentadjust;
#endif
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    OSA_STATUS status;
#endif

    /* rvmemory does basic error checking for us */

#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    region = (RvOsMemData *)driverRegion;
#else
    RV_UNUSED_ARG(driverRegion);
#endif
    memsize = size;

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    memsize += RV_OSMEM_STATS_OVERHEAD;
    bytesused = memsize;
#endif

    /* Call the OS to allocate the memory and set ptr to the allocated block. */
    /* Also, for statistics, set bytesused to actual amount of memory used. */
#if (RV_OSMEM_TYPE == RV_OSMEM_MALLOC) || (RV_OSMEM_TYPE == RV_OSMEM_VXWORKS)
    ptr = (void *)osmalloc(memsize);
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSE)
    /* For OSE we need to use memory shareable across processes. */
    ptr = heap_alloc_shared(memsize, (__FILE__), (__LINE__));
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_NUCLEUS)
    status = NU_Allocate_Memory(&System_Memory, &ptr, memsize, NU_NO_SUSPEND);
    if(status != NU_SUCCESS)
        ptr = NULL;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    ptr = NULL;

    /* Try from smallest possible block size and go up */
    for(pool = 0; pool < region->numpools; pool++) {
        if(memsize <= region->mempools[pool].realsize) {
            if(RvLockGet(&region->mempools[pool].lock, NULL) == RV_OK) {
                ptr = RvObjPoolGetItem(&region->mempools[pool].objpool);
                if(ptr != NULL) {
                    ptr = (void *)((RvInt8 *)ptr + RV_PSOS_BLOCK_OVERHEAD); /* Adjust past pool pointer */
#if (RV_MEMORY_KEEPSTATS == RV_YES)
                    bytesused = RvObjPoolItemBlockSize(&region->mempools[pool].objpool); /* use real size */
#endif
                    RvLockRelease(&region->mempools[pool].lock, NULL);
                    break;
                }
                RvLockRelease(&region->mempools[pool].lock, NULL);
            }
        }
    }

    /* Last resort, go directly to pSOS region */
    if(ptr == NULL) {
        memsize += RV_PSOS_BLOCK_OVERHEAD;
        if(rn_getseg(region->psos_region, memsize, RN_NOWAIT, 0, &ptr) == 0) {
            /* Set the pool pointer to NULL to indicate pSOS direct */
            block = (RvOsMemPsosPoolElem *)ptr;
            block->sourcepool = NULL;
            ptr = (void *)((RvInt8 *)ptr + RV_PSOS_BLOCK_OVERHEAD);
#if (RV_MEMORY_KEEPSTATS == RV_YES)
            /* Account for an entire segment being allocated */
            bytesused = memsize;
            segmentadjust = bytesused % (RvSize_t)region->segment_size;
            if(segmentadjust > 0)
                bytesused += ((RvSize_t)region->segment_size - segmentadjust);
            region->region_bytes_used += bytesused; /* Remember this separately */
#endif
        } else ptr = NULL; /* rn_getseg failed */
    }
#endif /* pSOS */
#if (RV_OSMEM_TYPE == RV_OSMEM_SYMBIAN)
    ptr = NULL;
    status = RvSymAlloc(memsize,&ptr);
    if(status != RV_OK)
    {
    	ptr = NULL;
    }
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    status = OSAMemPoolAlloc(poolRef, memsize, &ptr, OS_NO_SUSPEND);
    if((status != OS_SUCCESS) || (ptr == NULL))
        return RvOsMemErrorCode(RV_ERROR_OUTOFRESOURCES);
#endif

    /* Check for alloc failure */
    if(ptr == NULL)
        return RvOsMemErrorCode(RV_ERROR_OUTOFRESOURCES);

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    /* Save size in block so free can subtract it */
    blocksize = (RvSize_t *)ptr;
    ptr = (void *)((RvInt8 *)ptr + RV_OSMEM_STATS_OVERHEAD);
    *blocksize = size;

    /* Statistics calculations */
    if(RvLockGet(&region->lock,NULL) == RV_OK) {
        region->allocs_requested++;
        region->bytes_requested += size;
        region->bytes_used += bytesused;
        RvLockRelease(&region->lock,NULL);
    }
#endif

    *result = ptr;
    return RV_OK;
}

/********************************************************************************************
 * RvOsMemFree - Frees a memory which was allocated from a system driver region.
 *
 * Use the underlying OS services to free a block of memory back to the system heap.
 *
 * INPUT   : driverRegion - Pointer to the region object.
 *           ptr          - Pointer to the allocated memory to be released.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemFree(
    IN  void*       driverRegion,
    IN  void*       ptr)
{
    void * memptr;
#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    RvOsMemData *region;
#endif

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    RvSize_t *blocksize;
    RvSize_t bytesrequested, bytesused;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    RvOsMemPsosPoolElem *block;
#if (RV_MEMORY_KEEPSTATS == RV_YES)
    RvSize_t segmentadjust;
#endif
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    OSA_STATUS status;
#endif

    /* rvmemory does base error checking for us. */

#if (RV_MEMORY_KEEPSTATS == RV_YES) || (RV_OSMEM_TYPE == RV_OSMEM_PSOS) /* prevent warnings */
    region = (RvOsMemData *)driverRegion;
#else
    RV_UNUSED_ARG(driverRegion);
#endif
    memptr = ptr;

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    /* Move pointer and get saved copy of requested size */
    memptr = (void *)((RvInt8 *)memptr - RV_OSMEM_STATS_OVERHEAD);
    blocksize = (RvSize_t *) memptr;
    bytesrequested = *blocksize;
    bytesused = bytesrequested + RV_OSMEM_STATS_OVERHEAD;
#endif

    /* Call the OS to free the memory. For statistics, set bytesused to actual */
    /* amount of memory used by the allocation if its not standard. */
#if (RV_OSMEM_TYPE == RV_OSMEM_MALLOC) || (RV_OSMEM_TYPE == RV_OSMEM_VXWORKS)
    osfree(memptr);
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSE)
    heap_free_shared(memptr);
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_NUCLEUS)
    NU_Deallocate_Memory(memptr);
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    block = (RvOsMemPsosPoolElem *)((RvInt8 *)memptr - RV_PSOS_BLOCK_OVERHEAD);
    if(block->sourcepool != NULL)
    {
        /* Memory is in a pool */
        if(RvLockGet(&block->sourcepool->lock, NULL) == RV_OK)
        {
            RvObjPoolReleaseItem(&block->sourcepool->objpool, block); /* Block allocated from a pool */
#if (RV_MEMORY_KEEPSTATS == RV_YES)
            bytesused = RvObjPoolItemBlockSize(&block->sourcepool->objpool); /* Use real size */
#endif
            RvLockRelease(&block->sourcepool->lock, NULL);
        }
    }
    else
    {
        /* memory was allocated directly from the pSOS region */
        rn_retseg(region->psos_region, block);      /* Block allocated directly from pSOS */
#if (RV_MEMORY_KEEPSTATS == RV_YES)
            /* Account for an entire segment being allocated */
        bytesused += RV_PSOS_BLOCK_OVERHEAD;
        segmentadjust = bytesused % (RvSize_t)region->segment_size;
        if(segmentadjust > 0)
            bytesused += ((RvSize_t)region->segment_size - segmentadjust);
        region->region_bytes_used -= bytesused; /* Remember this separately */
#endif
    }
#endif /* pSOS */
#if (RV_OSMEM_TYPE == RV_OSMEM_SYMBIAN)
    RvSymFree(memptr);
#endif

#if (RV_OSMEM_TYPE == RV_OSMEM_OSA)
    status = OSAMemPoolFree(poolRef, memptr);
    if (status != OS_SUCCESS)
        return RvOsMemErrorCode(RV_ERROR_UNKNOWN);
#endif

    /* Statistics collection */
#if (RV_MEMORY_KEEPSTATS == RV_YES)
    if(RvLockGet(&region->lock,NULL) == RV_OK) {
        region->allocs_requested--;
        region->bytes_requested -= bytesrequested;
        region->bytes_used -= bytesused;
        RvLockRelease(&region->lock,NULL);
    }
#endif

    return RV_OK;
}

/********************************************************************************************
 * RvOsMemGetInfo - Returns the collected statistic information about a memory region.
 *
 * INPUT   : driverRegion - Pointer to the region object where statistic data is stored.
 * OUTPUT  : result       - Pointer to RvMemoryInfo structure where the statistic data
 *                          will be copied into.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvOsMemGetInfo(
    IN  void*           driverRegion,
    OUT RvMemoryInfo*   meminfo)
{
#if (RV_MEMORY_KEEPSTATS == RV_YES)
    RvOsMemData *region;
#if (RV_OSMEM_TYPE == RV_OSMEM_VXWORKS)
    MEM_PART_STATS pinfo;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    RvInt pool;
    struct rninfo rbuf;
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_NUCLEUS)
    CHAR name[10];
    VOID *start_address;
    UNSIGNED pool_size, min_allocation, available, task_waiting;
    OPTION suspend_type;
    NU_TASK *first_task;
#endif
#endif /* Stats */

    /* Start with everything at 0 in case we don't or can't calculate it */
    meminfo->bytes_requested_byuser = 0;
    meminfo->bytes_requested_bymem = 0;
    meminfo->bytes_requested_total = 0;
    meminfo->bytes_total_inuse = 0;
    meminfo->allocs_requested = 0;
    meminfo->bytes_free_now = 0;
    meminfo->bytes_free_total = 0;
    meminfo->allocs_free_now = 0;
    meminfo->allocs_free_total = 0;

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    region = (RvOsMemData *)driverRegion;

    if(RvLockGet(&region->lock,NULL) != RV_OK)
        return RV_OK; /* Not much we can do other than return nothing */

    /* Common calculations */
    meminfo->bytes_requested_byuser = region->bytes_requested - (region->allocs_requested * region->overhead);
    meminfo->bytes_requested_bymem = region->bytes_requested;
    meminfo->bytes_requested_total = region->bytes_used;
    meminfo->allocs_requested = region->allocs_requested;

    /* Calls to OS to fill in rest (Generic Malloc does nothing) */
#if (RV_OSMEM_TYPE == RV_OSMEM_VXWORKS)
    meminfo->bytes_total_inuse = meminfo->bytes_requested_total; /* we don't know internal overhead */
    if(memPartInfoGet(memSysPartId, &pinfo) == OK) {
        meminfo->bytes_free_now = pinfo.numBytesFree;
        meminfo->allocs_free_now = pinfo.numBytesFree / (region->overhead + RV_OSMEM_STATS_OVERHEAD + RV_ALIGN_SIZE); /* best guess */;
        meminfo->bytes_free_total = pinfo.numBytesFree;
        meminfo->allocs_free_total = meminfo->allocs_free_now;
    }
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
    /* Account for array overhead in total memory used. */
    meminfo->bytes_total_inuse = region->region_bytes_used; /* start with direct allocations */
    for(pool = 0; pool < region->numpools; pool++) {
        if(RvLockGet(&region->mempools[pool].lock,NULL) == RV_OK) {
            meminfo->bytes_total_inuse += (RvObjPoolTotalPages(&region->mempools[pool].objpool) * RvObjPoolPageSize(&region->mempools[pool].objpool));
            meminfo->bytes_free_now += (RvObjPoolFreeItems(&region->mempools[pool].objpool) * region->mempools[pool].realsize);
            meminfo->allocs_free_now += RvObjPoolFreeItems(&region->mempools[pool].objpool);
            RvLockRelease(&region->mempools[pool].lock,NULL);
        }
    }

    /* Do the best we can at free totals */
    meminfo->bytes_free_total = meminfo->bytes_free_now;
    meminfo->allocs_free_total = meminfo->allocs_free_now;
    if(rn_info(region->psos_region, &rbuf) == 0) {
        /* Get info from pSOS region. Doesn't account for overhead or */
        /* subdivision but its the best we can do for pSOS. */
        meminfo->bytes_free_total += rbuf.free_bytes;
        meminfo->allocs_free_total += (rbuf.free_bytes / rbuf.unit_size);
    }
#endif /* pSOS */
#if (RV_OSMEM_TYPE == RV_OSMEM_NUCLEUS)
    meminfo->bytes_total_inuse = meminfo->bytes_requested_total; /* we don't know internal overhead */
    if(NU_Memory_Pool_Information(&System_Memory, name, &start_address, &pool_size, &min_allocation, &available, &suspend_type, &task_waiting, &first_task) == NU_SUCCESS) {
        meminfo->bytes_free_now = available;
        meminfo->allocs_free_now = available / (region->overhead + RV_OSMEM_STATS_OVERHEAD + RV_ALIGN_SIZE); /* best guess */
        meminfo->bytes_free_total = available;
        meminfo->allocs_free_total = meminfo->allocs_free_now;
    }
#endif
#if (RV_OSMEM_TYPE == RV_OSMEM_OSE)
    meminfo->bytes_total_inuse = meminfo->bytes_requested_total; /* we don't know internal overhead */
    /* No current method for finding out heap statistics. */
#endif

    RvLockRelease(&region->lock,NULL);
#else
    RV_UNUSED_ARG(driverRegion);
#endif /* KEEPSTATS */

    return RV_OK;
}

#if (RV_OSMEM_TYPE == RV_OSMEM_PSOS)
/* pSOS additional functions */

/* Keep pool construction separate to make things cleaner */
static RvStatus RvOsMemPsosConstructPools(RvOsMemData *region, unsigned long minbufsize)
{
    RvStatus result;
    RvObjPoolFuncs callbacks;
    RvInt pool;
    unsigned long bufsize;
    RvOsMemPsosPoolElem tempobj;
#if (RV_MEMORY_KEEPSTATS == RV_YES)
    RvSize_t segmentadjust;
#endif

    if(region->numpools == 0) {
        region->mempools = NULL; /* No pools indicates to go to pSOS for everything */
        return RV_OK; /* We don't need any pools so we're done. */
    }

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((sizeof(RvOsMemPsosPoolElem) > minbufsize) || (RV_PSOS_BLOCK_OVERHEAD >= minbufsize))
        return(RV_ERROR_OUTOFRANGE);
#endif

    /* Allocate memory for memory pool array */
    if(rn_getseg(region->psos_region, region->numpools * sizeof(RvOsMemPsosPool), RN_NOWAIT, 0, (void **)&region->mempools) != 0)
        return RvOsMemErrorCode(RV_ERROR_OUTOFRESOURCES);

    /* all pools use same callbacks */
    memset(&callbacks, 0, sizeof(RvObjPoolFuncs));
    callbacks.objconstruct = RvOsMemPsosBlockConstruct;
    callbacks.objdestruct = NULL; /* Nothing to destruct */
    callbacks.objdestructdata = NULL;
    callbacks.pagealloc = RvOsMemPsosGetSeg;
    callbacks.pageallocdata = (void *)&region->psos_region;
    callbacks.pagefree = RvOsMemPsosFreeSeg;
    callbacks.pagefreedata = (void *)&region->psos_region;

    /* Construct object pools */
    result = RV_OK;
    bufsize = minbufsize;
    pool = 0;
    while((pool < region->numpools) && (result == RV_OK)) {
        region->mempools[pool].realsize = (RvSize_t)bufsize - RV_PSOS_BLOCK_OVERHEAD;
        callbacks.objconstructdata = (void *)&region->mempools[pool]; /* construct needs proper pool pointer */
        if(RvObjPoolConstruct(&tempobj, &tempobj.poolelem, &callbacks, bufsize, 0,
                              (region->segment_size * PSOS_OBJPOOL_PAGESIZE), PSOS_OBJPOOL_TYPE, PSOS_OBJPOOL_SALVAGE,
                              PSOS_OBJPOOL_MAXITEMS, PSOS_OBJPOOL_MINITEMS, PSOS_OBJPOOL_FREELEVEL, &region->mempools[pool].objpool) != NULL) {
            result = RvLockConstruct(NULL, &region->mempools[pool].lock); /* Give each pool its own lock for speed. */
            if(result != RV_OK)
                RvObjPoolDestruct(&region->mempools[pool].objpool); /* No lock, delete pool we just made */
        } else result = RvOsMemErrorCode(RV_ERROR_UNKNOWN); /* PoolCOnstruct failed */
        bufsize *= 2;
        pool++;
    }

    if(result != RV_OK) {
        /* Construction failed in middle, undo stuff already created */
        while(pool > 0) {
            pool--;
            RvObjPoolDestruct(&region->mempools[pool].objpool);
            RvLockDestruct(&region->mempools[pool].lock, NULL);
        }
        rn_retseg(region->psos_region, region->mempools);
    }

#if (RV_MEMORY_KEEPSTATS == RV_YES)
    /* Remember how much region memory we're using. */
    region->region_bytes_used = region->numpools * sizeof(RvOsMemPsosPool);
    segmentadjust = region->region_bytes_used % (RvSize_t)region->segment_size;
    if(segmentadjust > 0)
        region->region_bytes_used += ((RvSize_t)region->segment_size - segmentadjust); /* Account for segment size */
#endif

    return result;
}

/* ObjPool callbacks (3 of them) used by pSOS */

/* Each block needs to contain a pointer back to the pool it came from */
/* so we might as well do it when the items are constructed. The data */
/* parameter contains the pointer to the pool. */
static void *RvOsMemPsosBlockConstruct(void *item, void *data)
{
    RvOsMemPsosPoolElem *block;

    block = (RvOsMemPsosPoolElem *)item;
    block->sourcepool = (RvOsMemPsosPool *)data;
    return item;
}

/* Allocate regions from pSOS for pages, data = pointer to psos region id */
static void *RvOsMemPsosGetSeg(RvSize_t size, void *data)
{
    void *result;
    unsigned long *region_id;

    region_id = (unsigned long *)data;
    if(rn_getseg(*region_id, size, RN_NOWAIT, 0, &result) != 0)
        result = NULL;

    return result;
}

/* Free regions from pSOS, data = pointer to psos region id */
static void RvOsMemPsosFreeSeg(void *ptr, void *data)
{
    unsigned long *region_id;

    region_id = (unsigned long *)data;
    rn_retseg(*region_id, ptr);
}

#endif /* pSOS */
