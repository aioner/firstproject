/***********************************************************************
Filename   : rvpqueue.c
Description: utility for a Binary Heap Priority Queue
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
#include "rvpqueue.h"
#include <string.h>

/* Basic Priority Queue using a Binary Heap. User is responsible for locking */
/* if Priority Queue is to be shared. */

static RvSize_t rvPQueueNewSize(RvPQueue *pqueue, RvSize_t newsize);
static void rvPQueueHeapify(RvPQueue *pqueue, RvSize_t index, void* item);


/********************************************************************************************
 * RvPQueueConstruct
 * Constructs a priority queue.
 * INPUT   : qtype - The type of priority queue: RV_PQUEUE_TYPE_FIXED, RV_PQUEUE_TYPE_EXPANDING, or RV_PQUEUE_TYPE_DYNAMIC.
 *           startsize - Starting size of priority queue.
 *           callbacks - Pointer to structure with callback information.
 * OUTPUT  : pqueue - Pointer to priority queue object to construct.
 * RETURN  : A pointer to the priority queue object or, if there is an error, NULL.
 * NOTE    : The callbacks structure will be copied so there is no need to maintain this
 *           structure after construction.
 *           If startsize is less than 2 it will be set to 2, which is the minimum size.
 *           DYNAMIC priority queues will never shrink below startsize.
 *           The callbacks structure must be completely filled in. A duplicate
 *           of the callbacks structure will be made so that the user doesn't have
 *           to keep their own copy of that structure. Note that the memalloc callback
 *           will be called to allocate the starting array. See header file for qtype
 *           options.
 */
RVCOREAPI
RvPQueue * RVCALLCONV RvPQueueConstruct(
    IN RvInt qtype,
    IN RvSize_t startsize,
    IN RvPQueueFuncs *callbacks,
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((pqueue == NULL) || (callbacks == NULL) ||
       (callbacks->memalloc == NULL) || (callbacks->memfree == NULL) ||
       (callbacks->itemcmp == NULL) || (callbacks->newindex == NULL))
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((qtype != RV_PQUEUE_TYPE_FIXED) && (qtype != RV_PQUEUE_TYPE_EXPANDING) &&
       (qtype != RV_PQUEUE_TYPE_DYNAMIC))
        return NULL;
    if(startsize < 2)
        return NULL;
#endif

    pqueue->qtype = qtype;
    pqueue->startsize = startsize;
    pqueue->cursize = startsize;
    pqueue->numitems = 0;
	pqueue->allocitems = 0;
	pqueue->maxconcurrentitems = 0;

    /* Copy callbacks */
    memcpy(&pqueue->callbacks, callbacks, sizeof(*callbacks));

    /* Allocate starting array (allocate extra because index 0 is unused for speed purposes) */
    pqueue->heap = NULL;
    pqueue->heap = (void **)callbacks->memalloc((RvSize_t)(pqueue->heap + startsize + 1), callbacks->memallocdata);
    if(pqueue->heap == NULL)
        return NULL;

    pqueue->heap[0] = 0;
    return pqueue;
}


/********************************************************************************************
 * RvPQueueDestruct
 * Destructs a priority queue.
 * INPUT   : pqueue - Pointer to priority queue object to destruct.
 * OUTPUT  : None
 * RETURN  : None.
 * NOTE    : Any items in the priority queue when it is destructed are considered removed.
 */
RVCOREAPI
void RVCALLCONV RvPQueueDestruct(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return;
#endif

    pqueue->callbacks.memfree(pqueue->heap, pqueue->callbacks.memfreedata);
}


/********************************************************************************************
 * RvPQueuePut
 * Puts a new item into a priority queue.
 * INPUT   : pqueue - Pointer to priority queue to add newitem into.
 *           newitem - Pointer to the item to put into the queue.
 * OUTPUT  : None
 * RETURN  : A pointer to newitem or, if the item can not be added to the priority queue, NULL.
 */
RVCOREAPI
void * RVCALLCONV RvPQueuePut(
    IN RvPQueue *pqueue,
    IN void *newitem)
{
    RvSize_t newsize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((pqueue == NULL) || (newitem == NULL))
        return NULL;
#endif

    /* Deal with running out of space in the heap array. */
    if(pqueue->numitems >= pqueue->cursize) {
        if(pqueue->qtype == RV_PQUEUE_TYPE_FIXED)
            return NULL; /* Fixed size, just return error */

        /* Calculate and allocate space, remember array index 0 is unused. */
        newsize = pqueue->cursize * 2;
        if(rvPQueueNewSize(pqueue, newsize) != newsize)
            return NULL; /* No more space available */
    }

    /* Insert new item in heap, start trying at end. */
    pqueue->numitems += 1;
	pqueue->allocitems += 1;
	if (pqueue->maxconcurrentitems < pqueue->numitems)
		pqueue->maxconcurrentitems = pqueue->numitems;

    rvPQueueHeapify(pqueue, pqueue->numitems, newitem);

    return newitem;
}


/********************************************************************************************
 * RvPQueueGet
 * Gets the highest priority item from a priority queue. The item is removed from the queue.
 * INPUT   : pqueue - Pointer to priority queue to get item from.
 * OUTPUT  : None
 * RETURN  : A pointer to the highest priority item or NULL if the priority queue is empty.
 */
RVCOREAPI
void * RVCALLCONV RvPQueueGet(
    IN RvPQueue *pqueue)
{
    /* Simply remove the highest value item in the heap. */
    return RvPQueueRemove(pqueue, 1);
}


/********************************************************************************************
 * RvPQueuePeek
 * Gets the highest priority item from a priority queue. Does not remove the item from the queue.
 * INPUT   : pqueue - Pointer to priority queue to get item from.
 * OUTPUT  : None
 * RETURN  : A pointer to the highest priority item or NULL if the priority queue is empty.
 */
RVCOREAPI
void * RVCALLCONV RvPQueuePeek(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return NULL;
#endif

    if(pqueue->numitems == 0)
        return NULL; /* empty */

    /* First item is the needed item. */
    return pqueue->heap[1];
}


/********************************************************************************************
 * RvPQueueNumItems
 * Gets number of items currently in a priority queue.
 * INPUT   : pqueue - Pointer to priority queue to get number of items of.
 * OUTPUT  : None
 * RETURN  : The number of items in the priority queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvPQueueNumItems(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return 0;
#endif

    return pqueue->numitems;
}


/********************************************************************************************
 * RvPQueueMaxConcurrentItems
 * Gets number of max concurrent items in a priority queue.
 * INPUT   : pqueue - Pointer to priority queue to get number of items of.
 * OUTPUT  : None
 * RETURN  : The number of number of max concurrent items in the priority queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvPQueueMaxConcurrentItems(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return 0;
#endif

    return pqueue->maxconcurrentitems;
}


/********************************************************************************************
 * RvPQueueAllocatedItems
 * Gets number of items ever allocated per queue.
 * INPUT   : pqueue - Pointer to priority queue to get number of items of.
 * OUTPUT  : None
 * RETURN  : The number of items allocated in the priority queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvPQueueAllocatedItems(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return 0;
#endif

    return pqueue->allocitems;
}


/********************************************************************************************
 * RvPQueueSize
 * Gets the size of a priority queue.
 * INPUT   : pqueue - Pointer to priority queue to get size of.
 * OUTPUT  : None
 * RETURN  : The size of the priority queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvPQueueSize(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return 0;
#endif

    return pqueue->cursize;
}


/********************************************************************************************
 * RvPQueueChangeSize
 * Changes the size of a priority queue.
 * INPUT   : pqueue - Pointer to priority queue to change the size of.
 *           newsize - New size of priority queue.
 * OUTPUT  : None
 * RETURN  : The size of the priority queue. If the size could not be changed than the
 *           old size of the queue is returned.
 * NOTE    : The size can not be made smaller than the current number of items in the
 *           priority queue.
 *           The size can not be set smaller than 2.}
 *           For DYNAMIC priority queues this value replaces the startsize as
 *           the minimum size of the priority queue.
 *           Force a change of the priority queue size. Can not change
 *           the size smaller than the number of items in it. DYNAMIC
 *           queues will not reduce below this value (same as startsize)
 *           Returns new size (or old size if it couldn't be changed.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvPQueueChangeSize(
    IN RvPQueue *pqueue,
    IN RvSize_t newsize)
{
    RvSize_t result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return 0;
#endif

    if((newsize < 2) || (newsize < pqueue->numitems))
        return pqueue->cursize;

    result = rvPQueueNewSize(pqueue, newsize);
    if(result == newsize)
        pqueue->startsize = newsize; /* DYNAMIC hard floor */

    return result;
}


/********************************************************************************************
 * RvPQueueClear
 * Clears all items from a priority queue.
 * INPUT   : pqueue - Pointer to priority queue to clear.
 * OUTPUT  : None
 * RETURN  : None
 * NOTE    : For DYNAMIC priority queues, their size will shrink back to
 *           their minimum size, which is their starting size unless it was
 *           changed with a call to RvPQueueChangeSize.
 */
RVCOREAPI
void RVCALLCONV RvPQueueClear(
    IN RvPQueue *pqueue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return;
#endif

    pqueue->numitems = 0;

    /* If shrinking is allowed, shrink back to starting size. */
    if(pqueue->qtype == RV_PQUEUE_TYPE_DYNAMIC)
        rvPQueueNewSize(pqueue, pqueue->startsize);
}

/********************************************************************************************
 * RvPQueueRemove
 * Removes an item from a priority queue. The item is specified by its
 * index which it is given via the newindex callback.
 * INPUT   : pqueue - Pointer to priority queue to remove item from.
 *           itemindex - Index of the item to be removed.
 * OUTPUT  : None
 * RETURN  : A pointer to item that has been removed from the priority queue or NULL there is an error.
 */
RVCOREAPI
void * RVCALLCONV RvPQueueRemove(
    IN RvPQueue *pqueue,
    IN RvSize_t itemindex)
{
    void *result, *lastitem;
    RvSize_t index, child, otherchild, lastnonleaf;
    RvSize_t newsize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(pqueue == NULL)
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(itemindex == 0)
        return NULL;
#endif

    if(pqueue->numitems == 0)
        return NULL; /* empty */

    /* Make a copy of the needed item since that's the return value. */
    result = pqueue->heap[itemindex];

    /* The last item in the heap will be re-inserted from the deleted position. */
    lastitem = pqueue->heap[pqueue->numitems];
    pqueue->numitems -= 1;
    lastnonleaf = pqueue->numitems / 2;
    index = itemindex;
    while(index <= lastnonleaf) {
        child = index * 2;      /* left child */
        otherchild = child + 1; /* right child */
        if(child != pqueue->numitems) {
            /* Only check second child if it exists, ie first child is not last item in heap. */
            if(pqueue->callbacks.itemcmp(pqueue->heap[otherchild], pqueue->heap[child]) == RV_TRUE)
                child = otherchild; /* Use other (right) child if its higher priority. */
        }

        /* Compare re-inserted item with highest child. */
        if(pqueue->callbacks.itemcmp(pqueue->heap[child], lastitem) == RV_FALSE)
            break; /* Child isn't higher, we're done. */

        /* Move child up heap and tell item that it has moved. */
        pqueue->heap[index] = pqueue->heap[child];
        pqueue->callbacks.newindex(pqueue->heap[child], index);

        index = child;
    }

    /* Store re-inserted item in proper spot and tell the item its position. */
    rvPQueueHeapify(pqueue, index, lastitem);

    /* If shrinking is allowed, do it when needed. */
    if((pqueue->qtype == RV_PQUEUE_TYPE_DYNAMIC) && (pqueue->numitems <= (pqueue->cursize / 4))) {
        newsize = pqueue->cursize / 2;

        /* Don't shrink beneath original starting size. */
        if(newsize >= pqueue->startsize)
            rvPQueueNewSize(pqueue, newsize);
    }

    return result;
}


/********************************************************************************************
 * rvPQueueNewSize
 * Change the heap size to newsize. Doesn't do any error checking so make sure newsize is valid.
 * INPUT   : pqueue - Pointer to priority queue to remove item from.
 *           itemindex - Index of the item to be removed.
 * OUTPUT  : None
 * RETURN  : Returns the new size (or old one if it couldn't do it).
 */
static RvSize_t rvPQueueNewSize(RvPQueue *pqueue, RvSize_t newsize)
{
    void *newptr;
    RvSize_t allocsize, copysize;

    allocsize = (RvSize_t)(sizeof(*pqueue->heap) * (newsize + 1));

    /* Remember that array index 0 is not used. */
    newptr = pqueue->callbacks.memalloc(allocsize, pqueue->callbacks.memallocdata);
    if(newptr == NULL)
        return pqueue->cursize; /* No memory available so don't bother. */

    /* Copy existing heap to new space, remember array index 0 is unused */
    /* Don't try to copy more than the original size !!! (segment violation) */
    if (pqueue->cursize < newsize)
        copysize = (RvSize_t)(sizeof(*pqueue->heap) * (pqueue->cursize + 1));
    else
        copysize = allocsize;
    memcpy(newptr, pqueue->heap, copysize);

    /* Get rid of old memory and set heap. */
    pqueue->callbacks.memfree(pqueue->heap, pqueue->callbacks.memfreedata);
    pqueue->cursize = newsize;
    pqueue->heap = (void **)newptr;

    return pqueue->cursize;
}


static void rvPQueueHeapify(RvPQueue *pqueue, RvSize_t index, void* item)
{
    RvSize_t parent;

    while (index > 1) {
        parent = index / 2;
        if (pqueue->callbacks.itemcmp(item, pqueue->heap[parent]) == RV_FALSE)
            break; /* New item is not higher priority than parent so stop. */

        /* Move parent down heap and tell item that it has moved. */
        pqueue->heap[index] = pqueue->heap[parent];
        pqueue->callbacks.newindex(pqueue->heap[index], index);

        index = parent;
    }

    /* Store the item in proper spot and tell the item its position. */
    pqueue->heap[index] = item;
    pqueue->callbacks.newindex(item, index);
}

#if RV_PQUEUE_TEST

/* Debugging support */

static RvInt rvPQueueEnumerateAux(RvPQueue *pq, int idx, RvPQueueEnumerator enumerator, void *ctx) {
   void **heap = pq->heap;
   RvSize_t n = pq->numitems;
   RvInt res;
   int lson;
   int rson;
   void *proot; 
   void *parent;

   if(idx > (RvInt)n) {
     return 0;
   }

   proot = heap[idx];
   parent = heap[idx >> 1];

   res = enumerator(pq, RV_PQUEUE_NODE, parent, proot, ctx);
   if(res < 0) {
       return res;
   }

   res = enumerator(pq, RV_PQUEUE_LEFT, parent, proot, ctx);
   if(res < 0) {
       return res;
   }

   lson = idx * 2;
   rson = lson + 1;
   res = rvPQueueEnumerateAux(pq, lson, enumerator, ctx);
   if(res < 0) {
       return res;
   }

   res = enumerator(pq, RV_PQUEUE_RIGHT, parent, proot, ctx);
   if(res < 0) {
       return res;
   }

   res = rvPQueueEnumerateAux(pq, rson, enumerator, ctx);

   if(res < 0) {
       return res;
   }

   res = enumerator(pq, RV_PQUEUE_UP, parent, proot, ctx);
   return res;
}

RVCOREAPI
RvInt RvPQueueEnumerate(RvPQueue *pq, RvPQueueEnumerator enumerator, void *ctx) {
    return rvPQueueEnumerateAux(pq, 1, enumerator, ctx);
}

#endif
