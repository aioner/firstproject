/***********************************************************************
Filename   : rvobjpool.c
Description: utility for building pools of objects (structures)
************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
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
#include "rvobjpool.h"
#include <string.h>

/* Basic pool utility for structures. User is responsible for locking */
/* if pool is to be shared. */

/* Define additional data to be stored with each page. The OVERHEAD size */
/* must account for proper alignment of the items. */
typedef struct {
    RvObjPool *pool;
    RvObjListElement pagelistelem;
    RvSize_t maxelements;
    RvSize_t freecount;
} RvObjPoolPage;
#define RV_OBJPOOL_PAGE_OVERHEAD RvRoundToSize(sizeof(RvObjPoolPage), RV_ALIGN_SIZE)

/* Define additional data to be stored with each item. The OVERHEAD size */
/* must account for the alignment of the ptr that will be returned to the user. */
typedef RvObjPoolPage *RvObjPoolData;
#define RV_OBJPOOL_ITEM_OVERHEAD RV_ALIGN_SIZE

static RvObjPoolPage *RvObjPoolNewPage(RvObjPool *objpool);
static RvBool RvObjPoolRemovePage(RvObjPoolPage *page);

/********************************************************************************************
 * RvObjPoolConstruct
 * Constructs an object pool.
 * INPUT   :    itemtemp - Pointer to an object of the type to be stored in the pool.
 *              elementptr - Pointer to the element within itemtemp to use for this pool.
 *              callbacks - Pointer to structure with callback information.
 *              itemsize - The size of the item to being stored in the pool.
 *              pageitems - Number of items per page (0 = calculate from pagesize).
 *              pagesize - Size of each page (0 = calculate from pageitems).
 *              pooltype - The type of block pool: RV_OBJPOOL_TYPE_FIXED, RV_OBJPOOL_TYPE_EXPANDING, or RV_OB
 *              salvage - Set to RV_OBJPOOL_SALVAGE_ALLOWED to allow page salvage, otherwise RV_OBJPOOL_SALVA
 *                          Must be set to RV_OBJPOOL_SALVAGE_ALLOWED for DYNAMIC pools. Enabling this
 *                          options incurs some additional memory overhead.
 *              maxitems - Number of items never to exceed this value (0 = no max).
 *              minitems - Number of items never to go below this value.
 *              freelevel - Minimum number of items free per 100 (0 to 100). Used for DYNAMIC pools only.
 * OUTPUT  :    objpool - Pointer to object pool object to construct.
 * RETURN  : A pointer to the object pool object or, if there is an error, NULL.
 * NOTE    :    The itemtemp and elementptr pointers are simply used to find the offset
 *              within the structure where the RvObjPoolElement element is located.
 *              Anything the itemtemp pointer may point is is never touched.
 *              The callbacks structure will be copied so there is no need to maintain this
 *              structure after construction.
 *              Disabling the salvage option reduces memory overhead. For DYNAMIC pools
 *              the salvage option must be enabled.
 */
RVCOREAPI
RvObjPool * RVCALLCONV RvObjPoolConstruct(
    IN void *itemtemp,
    IN RvObjPoolElement *elementptr,
    IN RvObjPoolFuncs *callbacks,
    IN RvSize_t itemsize,
    IN RvSize_t pageitems,
    IN RvSize_t pagesize,
    IN RvInt pooltype,
    IN RvBool salvage,
    IN RvSize_t maxitems,
    IN RvSize_t minitems,
    IN RvSize_t freelevel,
    OUT RvObjPool *objpool)
{
    RvObjPoolPage poolpage;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((objpool == NULL) || (callbacks == NULL) || (callbacks->pagealloc == NULL) ||
       (callbacks->pagefree == NULL))
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((salvage != RV_OBJPOOL_SALVAGE_ALLOWED) && (salvage != RV_OBJPOOL_SALVAGE_NEVER))
        return NULL;
    if(freelevel > 100)
        return NULL;
    if((maxitems < minitems) && (maxitems != 0))
        return NULL;
#endif

    /* Set options based on pool type and data parameter */
    switch(pooltype) {
        case RV_OBJPOOL_TYPE_FIXED: objpool->autoexpand = RV_FALSE;
                                    objpool->autoreduct = RV_FALSE;
                                    break;
        case RV_OBJPOOL_TYPE_EXPANDING: objpool->autoexpand = RV_TRUE;
                                        objpool->autoreduct = RV_FALSE;
                                        break;
        case RV_OBJPOOL_TYPE_DYNAMIC: objpool->autoexpand = RV_TRUE;
                                      objpool->autoreduct = RV_TRUE;
                                      if(salvage != RV_OBJPOOL_SALVAGE_ALLOWED)
                                          return NULL; /* DYNAMIC must allow salvage */
                                      break;
        default: return NULL; /* invalid pooltype */
    }

    if(maxitems == 0) {
        objpool->maxitems = (RvSize_t)(~0); /* Allow easy setting of no maximum */
    } else objpool->maxitems = maxitems;
    objpool->minitems = minitems;
    objpool->allowsalvage = salvage;
    objpool->reductlevel = freelevel;

    /* Set item size and calculate actual size of each block */
    objpool->itemsize = itemsize;
    objpool->blocksize = itemsize + RV_ALIGN_SIZE - 1 - ((itemsize - 1) % RV_ALIGN_SIZE); /* insure alignment */
    if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED)
        objpool->blocksize += RV_OBJPOOL_ITEM_OVERHEAD; /* We'll need space to store the page pointer */
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(objpool->blocksize < objpool->itemsize)
        return(NULL); /* overflow */
#endif

    /* Calculate page information based on user input */
    if(pageitems == 0) {
        /* User wants us to calculate items per page */
        objpool->pageitems = (pagesize - RV_OBJPOOL_PAGE_OVERHEAD) / objpool->blocksize;
    } else objpool->pageitems = pageitems;
    if(pagesize == 0) {
        /* User wants us to calculate size of each page */
        objpool->pagesize = (pageitems * objpool->blocksize) + RV_OBJPOOL_PAGE_OVERHEAD;
    } else objpool->pagesize = pagesize;
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((objpool->pagesize <= RV_OBJPOOL_PAGE_OVERHEAD) || (objpool->pageitems == 0))
        return NULL; /* pagesize not big enough for one block. */
    if(((objpool->pagesize - RV_OBJPOOL_PAGE_OVERHEAD) / objpool->blocksize) < objpool->pageitems)
        return NULL; /* calculation overflowed */
#endif

    /* Copy callbacks */
    memcpy(&objpool->callbacks, callbacks, sizeof(*callbacks));

    /* Set up free list (a pool element is simply a list elelment) */
    if(RvObjListConstruct(itemtemp, elementptr, &objpool->freelist) == NULL)
        return NULL;

    /* Create our list of pages */
    RvObjListConstruct(&poolpage, &poolpage.pagelistelem, &objpool->pagelist);

    /* If a minimum is set, start with that amount. */
    if(objpool->minitems > 0) {
        if(RvObjPoolAddItems(objpool, objpool->minitems) < objpool->minitems) {
            /* If we can't get the minimum, then undo everything and abort. */
            RvObjPoolDestruct(objpool);
            return NULL;
        }
    }

    return objpool;
}


/********************************************************************************************
 * RvObjPoolDestruct
 * Destructs an object pool.
 * INPUT   :    objpool - Pointer to object pool object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_TRUE if object pool has been destructed, RV_FALSE if not.
 * NOTE    : A pool can only be destructed if all items have been returned to
 *           the pool. Trying to destroy a pool with items missing will simply
 *           do nothing and return a value of RV_FALSE.
 * Will destruct all items, free all pages and then destroy the pool.
 * Returns RV_TRUE upon success. Will fail, returning RV_FALSE, if some
 * items have not been returned to the pool.
 */
RVCOREAPI
RvBool RVCALLCONV RvObjPoolDestruct(
    IN RvObjPool *objpool)
{
    void *item;
    RvObjPoolPage *page;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return RV_FALSE;
#endif

    if(RvObjPoolTotalItems(objpool) != RvObjPoolFreeItems(objpool))
        return RV_FALSE; /* All items not returned to pool */

    /* Destruct all items (don't bother if no destruct callback) */
    if(objpool->callbacks.objdestruct != NULL) {
        for(;;) {
            item = RvObjListGetNext(&objpool->freelist, NULL, RV_OBJLIST_REMOVE);
            if(item == NULL) break;
            objpool->callbacks.objdestruct(item, objpool->callbacks.objdestructdata);
        }
    }

    /* Free up all the pages */
    for(;;) {
        page = (RvObjPoolPage *)RvObjListGetNext(&objpool->pagelist, NULL, RV_OBJLIST_REMOVE);
        if(page == NULL) break;
        objpool->callbacks.pagefree((void *)page, objpool->callbacks.pagefreedata);
    }

    /* That'it, nothing else needs to be cleaned up */
    return RV_TRUE;
}


/********************************************************************************************
 * RvObjPoolTotalItems
 * Gets the total number of items in an object pool.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Total number of items in the object pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolTotalItems(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return RvObjListSize(&objpool->pagelist) * objpool->pageitems;
}


/********************************************************************************************
 * RvObjPoolFreeItems
 * Gets the number of items currently available in an object pool.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Number of items currently available in the object pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolFreeItems(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return RvObjListSize(&objpool->freelist);
}


/********************************************************************************************
 * RvObjPoolTotalPages
 * Gets the total number of pages in an object pool.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Total number of pages in the object pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolTotalPages(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return RvObjListSize(&objpool->pagelist);
}


/********************************************************************************************
 * RvObjPoolItemsPerPage
 * Gets the number of items per page in an object pool.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Number of items per page in the object pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolItemsPerPage(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return objpool->pageitems;
}

/********************************************************************************************
 * RvObjPoolBlockSize
 * Gets the actual size of the memory block used by each item.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Actual size of the memory block used by each item.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolItemBlockSize(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return objpool->blocksize;
}

/********************************************************************************************
 * RvObjPoolPageSize
 * Gets the page size (in bytes) of an object pool.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Page size, in bytes, of the object pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolPageSize(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return objpool->pagesize;
}

/********************************************************************************************
 * RvObjPoolGetMaxitems
 * Gets the current maxitems setting. This is the maximum size that
 * the object pool may be.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Maximum number of items allowed in the object pool.
 * NOTE    : The actual maximum number of items that may be in the pool may
 *           be less than this amount since items are allocated a page at a time.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolGetMaxitems(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return objpool->maxitems;
}

/********************************************************************************************
 * RvObjPoolSetMaxitems
 * Sets the current maxitems setting. This is the maximum size that the object pool may be.
 * INPUT   : objpool - Pointer to object pool to set maxitems for.
 *           new - New value for maxitems (0 = no maximum).
 * OUTPUT  : None.
 * RETURN  : RV_TRUE if maxitems set successfully, otherwise RV_FALSE.
 * NOTE    : The actual maximum number of items that may be in the pool may
 *           be less than this amount since items are allocated a page at a time.
 *           Setting maxitems to be lower than minitems or to a value smaller
 *           than the current size ifs not allowed.
 *           This setting should not normally be used for FIXED pools.
 */
RVCOREAPI
RvBool RVCALLCONV RvObjPoolSetMaxitems(
    IN RvObjPool *objpool,
    IN RvSize_t newmax)
{
    RvSize_t maxitems;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return RV_FALSE;
#endif

    maxitems = newmax;
    if(maxitems == 0)
        maxitems = (RvSize_t)(~0);

    /* We don't allow reducing this beneath the min or the */
    /* current number of items since that gets way too messy. */
    if((maxitems < objpool->minitems) || (maxitems < RvObjPoolTotalItems(objpool)))
        return RV_FALSE;

    objpool->maxitems = maxitems;
    return RV_TRUE;
}

/********************************************************************************************
 * RvObjPoolGetMinitems
 * Gets the current minitems setting. This is the minimum size that the object pool may be.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Minimum number of items allowed in the object pool.
 * NOTE    : The actual minimum number of items that may be in the pool may
 *           be larger than this amount since items are allocated a page at a time.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolGetMinitems(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return objpool->minitems;
}

/********************************************************************************************
 * RvObjPoolSetMinitems
 * Sets the current minitems setting. This is the minimum size that
 * the object pool may be.
 * INPUT   : objpool - Pointer to object pool to set minitems for.
 *           new - New value for minitems.
 * OUTPUT  : None.
 * RETURN  : RV_TRUE if minitems set successfully, otherwise RV_FALSE.
 * NOTE    : The actual minimum number of items that may be in the pool may
 *           be larger than this amount since items are allocated a page at a time.
 *           The actual minimum number of items that may be in the pool may
 *           be larger than this amount since items are allocated a page at a
 *           time.
 *           Setting minitems to be larger than maxitems is not allowed.
 *           This setting should not normally be used for FIXED pools.
 */
RVCOREAPI
RvBool RVCALLCONV RvObjPoolSetMinitems(
    IN RvObjPool *objpool,
    IN RvSize_t newmin)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return RV_FALSE;
#endif

    /* Don't allow min/max to cross. */
    if(newmin > ((objpool->maxitems / objpool->pageitems) * objpool->pageitems))
        return RV_FALSE;

    /* If we are below the minimum, add pages until we're not */
    while(newmin > RvObjPoolTotalItems(objpool)) {
        if(RvObjPoolAddPages(objpool, 1) != 1)
            break; /* We can't add any more pages so just stop */
    }

    objpool->minitems = newmin;
    return RV_TRUE;
}

/********************************************************************************************
 * RvObjPoolGetFreelevel
 * Gets the current freelevel setting. This is the minimum number of free items
 * per 100 that should be maintained.
 * INPUT   : objpool - Pointer to object pool to get information about.
 * OUTPUT  : None.
 * RETURN  : Minumum number of free items per 100 to be maintained (0 to 100).
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolGetFreelevel(
    IN RvObjPool *objpool)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    return objpool->reductlevel;
}

/********************************************************************************************
 * RvObjPoolSetFreelevel
 * Sets the current freelevel setting. This is the minimum number of free items
 * per 100 that should be maintained.
 * INPUT   : objpool - Pointer to object pool to set freelevel for.
 *           newlevel - New value for freelevel.
 * OUTPUT  : None.
 * RETURN  : RV_TRUE if freelevel set successfully, otherwise RV_FALSE.
 * NOTE    : The freelevel setting is only used for DYNAMIC pools.
 *           Setting freelevel to 0 will cause a page to be released as soon
 *           as it has all items returned to it.
 *           Setting freelevel to 100 will never release pages and is the
 *           equivalent of using an EXPANDED pool.
 */
RVCOREAPI
RvBool RVCALLCONV RvObjPoolSetFreelevel(
    IN RvObjPool *objpool,
    IN RvSize_t newlevel)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(newlevel > 100)
        return RV_FALSE;
#endif

    /* Trying to adjust to pool for a new level is to messy so */
    /* just set the new value and let it get there naturally. */
    objpool->reductlevel = newlevel;
    return RV_TRUE;
}

/********************************************************************************************
 * RvObjPoolAddItems
 * Adds the requested number of items to a pool.
 * INPUT   : objpool - Pointer to object pool to add items to.
 *           numitems - Number of items to add (0 indicates to add a page of items).
 * OUTPUT  : None.
 * RETURN  : Actual number of items added.
 * NOTE    : The actual number of items added may be larger than that
 *           requested since items are added in multiples of the number
 *           of items in a page.
 *           If the actual number of items is less than that requested
 *           it means that those items could not be added because
 *           no more pages could be allocated, no more items could
 *           be constructed, or the maxitems limit was reached.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolAddItems(
    IN RvObjPool *objpool,
    IN RvSize_t numitems)
{
    RvSize_t numpages, newpages;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    if(numitems > 0) {
        numpages = ((numitems - 1) / objpool->pageitems) + 1;
    } else numpages = 1;

    newpages = RvObjPoolAddPages(objpool, numpages);
    return (newpages * objpool->pageitems);
}

/********************************************************************************************
 * RvObjPoolAddPages
 * Adds the requested number of pages to a pool.
 * INPUT   : objpool - Pointer to object pool to add pages to.
 *           numpages - Number of pages to add.
 * OUTPUT  : None.
 * RETURN  : Actual number of pages added.
 * NOTE    : If the actual number of items is less than that requested
 *           it means that those items could not be added because
 *           no more pages could be allocated, no more items could
 *           be constructed, or the maxitems limit was reached.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolAddPages(
    IN RvObjPool *objpool,
    IN RvSize_t numpages)
{
    RvSize_t i, errcount;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif

    errcount = 0;
    for(i = 0; i < numpages; i++) {
        if(RvObjPoolNewPage(objpool) == NULL)
            errcount += 1;
    }
    return (numpages - errcount);
}

/********************************************************************************************
 * RvObjPoolSalvage
 * Release any pages in an object pool which have no items allocated from them.
 * INPUT   : objpool - Pointer to object pool to salvage.
 * OUTPUT  : None.
 * RETURN  : Actual number of pages added.
 * NOTE    : Calling this function on an object pool which had the salvage option
 *           disabled will do nothing (and return a 0).
 *           The number of pages released is limited by the minitems setting.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvObjPoolSalvage(
    IN RvObjPool *objpool)
{
    RvObjPoolPage *page, *prevpage;
    RvSize_t result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return 0;
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_NEVER)
        return 0;
#endif

    result = 0;
    page = NULL; /* start with first page */
    for(;;) {
        page = (RvObjPoolPage *)RvObjListGetNext(&objpool->pagelist, page, RV_OBJLIST_LEAVE);
        if(page == NULL) break;
        if(page->freecount == page->maxelements) {
            /* page is not being used */
            prevpage = (RvObjPoolPage *)RvObjListGetPrevious(&objpool->pagelist, page, RV_OBJLIST_LEAVE);
            if(RvObjPoolRemovePage(page) == RV_TRUE) {
                result += 1;
                page = prevpage; /* deleted page isn't in the list anymore */
            }
        }
    }

    return result;
}

/********************************************************************************************
 * RvObjPoolGetItem
 * Get an item from a pool.
 * INPUT   : objpool - Pointer to object pool to get item from.
 * OUTPUT  : None.
 * RETURN  : Pointer to object allocated or NULL is none are available.
 * NOTE    : Items may only be returned to the pool they came from.
 */
RVCOREAPI
void * RVCALLCONV RvObjPoolGetItem(
    IN RvObjPool *objpool)
{
    RvObjPoolPage **pageref;
    void *item;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return NULL;
#endif

    item = RvObjListGetNext(&objpool->freelist, NULL, RV_OBJLIST_REMOVE);
    if((item == NULL) && (objpool->autoexpand == RV_TRUE)) {
        /* No items and we're allowed to make more so make them and try again */
        RvObjPoolNewPage(objpool);
        item = RvObjListGetNext(&objpool->freelist, NULL, RV_OBJLIST_REMOVE);
    }

    /* If we're keeping page references, increment item count */
    if((item != NULL) && (objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED)) {
        pageref = (RvObjPoolPage **)((RvInt8 *)item - RV_OBJPOOL_ITEM_OVERHEAD);
        (*pageref)->freecount--;
    }

    return item;
}

/********************************************************************************************
 * RvObjPoolReleaseItem
 * Returns an item back to a pool.
 * INPUT   : objpool - Pointer to object pool to return item to.
 *           item - Pointer to item to be returned.
 * OUTPUT  : None.
 * RETURN  : RV_TRUE if successful otherwise RV_FALSE.
 */
RVCOREAPI
RvBool RVCALLCONV RvObjPoolReleaseItem(
    IN RvObjPool *objpool,
    IN void *item)
{
    RvObjPoolPage **pageref;
    RvSize_t total, minlevel;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((objpool == NULL) || (item == NULL))
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED) {
        pageref = (RvObjPoolPage **)((RvInt8 *)item - RV_OBJPOOL_ITEM_OVERHEAD);
        if((*pageref)->pool != objpool)
            return RV_FALSE; /* Object being returned to wrong pool */
    }
#endif

    /* Put item at front of free list (so it gets reused first) */
    if(RvObjListInsertAfter(&objpool->freelist, NULL, item) == NULL)
        return RV_FALSE;

    if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED) {
        /* Page reference only exists if pages can be removed */
        pageref = (RvObjPoolPage **)((RvInt8 *)item - RV_OBJPOOL_ITEM_OVERHEAD);
        (*pageref)->freecount++;
        if((objpool->autoreduct == RV_TRUE) && ((*pageref)->freecount == (*pageref)->maxelements)) {
            /* The page is no longer in use so we may want to delete the page */
            if(objpool->reductlevel > 0) {
                /* Calculate free level */
                total = RvObjPoolTotalItems(objpool) - objpool->pageitems;
                minlevel = (objpool->reductlevel * (total / 100)) +
                           ((objpool->reductlevel * (total % 100)) / 100);
                if(RvObjPoolFreeItems(objpool) > (minlevel + objpool->pageitems))
                    RvObjPoolRemovePage(*pageref);
            } else RvObjPoolRemovePage(*pageref); /* reductlevel = 0 means always delete */
        }
    }

    return RV_TRUE;
}


static
RvBool RvObjPoolInitPage(RvObjPool *objpool, RvObjPoolPage *page) {
    RvObjPoolPage **pageref;
    RvSize_t count;
    void *item, *useritem;
    
    /* Create each item in page */
    item = (void *)((RvInt8 *)page + RV_OBJPOOL_PAGE_OVERHEAD);
    for(count = 0; count < objpool->pageitems; count++) {
        if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED) {
            /* Add page reference before actual item */
            pageref = (RvObjPoolPage **)item;
            *pageref = page;
            useritem = (void *)((RvInt8 *)item + RV_OBJPOOL_ITEM_OVERHEAD);
        } else useritem = item;

        /* Call user construct if there is one */
        if(objpool->callbacks.objconstruct != NULL) {
            if(objpool->callbacks.objconstruct(useritem, objpool->callbacks.objconstructdata) == NULL) {
                /* User can't construct object, we need to undo things */
                while(count > 0) {
                    count--;
                    item = (void *)((RvInt8 *)item - objpool->blocksize);
                    if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED) {
                        useritem = (void *)((RvInt8 *)item - RV_OBJPOOL_ITEM_OVERHEAD);
                    } else useritem = item;
                    RvObjListRemoveItem(&objpool->freelist, useritem);
                    if(objpool->callbacks.objdestruct != NULL)
                        objpool->callbacks.objdestruct(useritem, objpool->callbacks.objdestructdata);
                }
                objpool->callbacks.pagefree((void *)page, objpool->callbacks.pagefreedata);
                return RV_FALSE; /* Page is undone, bail out */
            }
        }
        /* Add new item to end of pool's free list */
        RvObjListInsertBefore(&objpool->freelist, NULL, useritem);

        item = (void *)((RvInt8 *)item + objpool->blocksize);
    }

    /* Set up page info */
    page->pool = objpool;
    page->maxelements = objpool->pageitems;
    page->freecount = objpool->pageitems;
    RvObjListInsertBefore(&objpool->pagelist, NULL, (void *)page); /* Put page at end of list */
    return RV_TRUE;
}

/********************************************************************************************
 * RvObjPoolNewPage
 * Create new page and add it to a pool
 * INPUT   : objpool - Pointer to object pool to return item to.
 * OUTPUT  : None.
 * RETURN  : Pointer to object pool
 */
static RvObjPoolPage *RvObjPoolNewPage(
    OUT RvObjPool *objpool)
{
    RvObjPoolPage *page;
    RvBool b;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(objpool == NULL)
        return NULL;
#endif

    if(((RvObjListSize(&objpool->pagelist) + 1) * objpool->pageitems) > objpool->maxitems)
        return RV_FALSE; /* adding a page would go above maximum */

    page = (RvObjPoolPage *)objpool->callbacks.pagealloc(objpool->pagesize, objpool->callbacks.pageallocdata);
    if(page == NULL)
        return NULL; /* can't get a new page */

    b = RvObjPoolInitPage(objpool, page);
    return b ? page : 0;
}

/********************************************************************************************
 * RvObjPoolReset
 *
 * Resets an object pool. 
 * INPUT   : objpool - Pointer to object pool to return item to.
 * OUTPUT  : None.
 * RETURN  : None
 */
RVCOREAPI
void RVCALLCONV RvObjPoolReset(IN RvObjPool *objpool) {
    RvObjPoolPage *cur = 0;

    RvObjListReset(&objpool->freelist);

    while((cur = (RvObjPoolPage *)RvObjListGetNext(&objpool->pagelist, cur, RV_TRUE)) != 0) {
        RvObjPoolInitPage(objpool, cur);
    }
}


/********************************************************************************************
 * RvObjPoolRemovePage
 * Remove a page from its object pool,
 * INPUT   : page - Pointer to object pool page.
 * OUTPUT  : None.
 * RETURN  : returns RV_TRUE if successful.
 */
static RvBool RvObjPoolRemovePage(
    IN RvObjPoolPage *page)
{
    RvObjPool *objpool;
    RvSize_t count;
    void *item, *useritem;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(page == NULL)
        return RV_FALSE;
#endif

    objpool = page->pool;
    if(((RvObjListSize(&objpool->pagelist) - 1) * objpool->pageitems) < objpool->minitems)
        return RV_FALSE; /* removing a page would go below minimum */

    RvObjListRemoveItem(&objpool->pagelist, (void *)page);

    /* Remove each item in page */
    item = (void *)((RvInt8 *)page + RV_OBJPOOL_PAGE_OVERHEAD);
    for(count = 0; count < objpool->pageitems; count++) {
        if(objpool->allowsalvage == RV_OBJPOOL_SALVAGE_ALLOWED) {
            /* skip page reference before actual item */
            useritem = (void *)((RvInt8 *)item + RV_OBJPOOL_ITEM_OVERHEAD);
        } else useritem = item;

        RvObjListRemoveItem(&objpool->freelist, useritem); /* remove from freelist */

        /* Call user destruct if there is one */
        if(objpool->callbacks.objdestruct != NULL)
            objpool->callbacks.objdestruct(useritem, objpool->callbacks.objdestructdata);

        item = (void *)((RvInt8 *)item + objpool->blocksize);
    }

    /* free up page and we're done */
    objpool->callbacks.pagefree(page, objpool->callbacks.pagefreedata);
    return RV_TRUE;
}

#if RV_OBJPOOL_TEST

/* Enumerator
 *
 * typedef RvBool (*RvObjPoolEnumerator)(RvObjPool *objpool, void *enumCtx, void *item, RvBool isFreeItem);
 */

#include "rvmemory.h"

static RvInt rvObjPoolFindIdx(RvObjPool *objpool, void *vitem) {
    char *firstPageItem;
    char *lastPageItem;
    void *curPage = 0;
    RvObjList *pageList = &objpool->pagelist;
    RvUint32 pageSize = objpool->pagesize;
    RvUint32 itemSize = objpool->blocksize;
    char *item = (char *)vitem;
    RvInt firstPageIdx = 0;

    while(curPage = RvObjListGetNext(pageList, curPage, RV_FALSE)) {
        firstPageItem = (char *)curPage + RV_OBJPOOL_PAGE_OVERHEAD;
        lastPageItem = firstPageItem + pageSize;
        if(item >= firstPageItem && item < lastPageItem) {
            RvInt idx = (item - firstPageItem) / itemSize;
            return firstPageIdx + idx;
        }
        firstPageIdx += objpool->pageitems;
    }

    /* Index of free item wasn't found. Shouldn't happen */
    return -1;
}

static void* rvObjPoolFindItem(RvObjPool *objpool, RvInt idx) {
    RvObjList *pageList = &objpool->pagelist;
    RvInt itemsInPage = objpool->pageitems;
    char *curPage = 0;
    RvInt idxPage = (idx + 1) / itemsInPage;
    RvInt idxInPage = idx % itemsInPage;
    RvInt i;
    RvChar *item;

    for(i = 0; i <= idxPage; i++) {
        curPage = RvObjListGetNext(pageList, curPage, RV_FALSE);
        if(curPage == 0) {
            return 0;
        }
    }

    item = curPage + RV_OBJPOOL_PAGE_OVERHEAD + idxInPage * objpool->blocksize;
    return item;
}

RVCOREAPI
RvInt RvObjPoolEnumerate(RvObjPool *objpool, RvObjPoolEnumerator enumerator, void *enumCtx) {
    RvInt32 totalItems = RvObjPoolTotalItems(objpool);
    RvUint8 sItemState[1000];
    RvUint8 *itemState = sItemState;
    RvInt s;
    RvUint32 itemStateSize;
    RvUint32 freeItems;
    RvUint32 i;
    void *curItem;
    RvObjList *freeList;
    RvInt retval = 0;

    if(totalItems == 0) {
        return 0;
    }

    itemStateSize = totalItems * sizeof(*itemState);

    if(itemStateSize > sizeof(sItemState)) {
        s = RvMemoryAlloc(NULL, itemStateSize, NULL, (void **)&itemState);
        if(s != RV_OK) {
            return s;
        }
    }

    freeItems = RvObjPoolFreeItems(objpool);
    memset(itemState, 1, itemStateSize);
    freeList = &objpool->freelist;
    curItem = 0;


    for(i = 0; i < freeItems; i++) {
        RvInt idx;

        curItem = RvObjListGetNext(freeList, curItem, RV_FALSE);
        if(curItem == 0) {
            retval = -1;
            goto clear;
        }


        idx = rvObjPoolFindIdx(objpool, curItem);
        if(idx < 0) {
            retval = -2;
            goto clear;
        }

        itemState[idx] = 0;
    }

    for(i = 0; i < (RvUint32)totalItems; i++) {
        if(!itemState[i]) continue;

        curItem = rvObjPoolFindItem(objpool, i);
        if(curItem == 0) {
            retval = -3;
            goto clear;
        }

        if(enumerator) {
            if(!enumerator(objpool, enumCtx, curItem, RV_FALSE)) {
                goto clear;
            }
        }

    }

clear:
    if(itemState != sItemState && itemState) {
        RvMemoryFree(itemState, 0);
    }
    return retval;
}

#endif
