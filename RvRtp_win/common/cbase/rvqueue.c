/* rvqueue.c - queue functions */
/************************************************************************
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
#include "rvqueue.h"
#include "string.h"

/* Lets make error codes a little easier to type */
#define RvQueueErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CBASE, RV_CBASE_MODULE_QUEUE, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
#define RvQueueLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->queueSource, p);}
#define RvQueueLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->queueSource, p);}
#define RvQueueLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->queueSource, p);}
#else
#define RvQueueLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#define RvQueueLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#define RvQueueLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif


/********************************************************************************************
 * RvQueueInit - Initializes the Queue module
 *
 * Must be called once (and only once) before any other functions in the module are called.
 * Thread-safe function
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : always RV_OK
 */
RvStatus RvQueueInit(void)
{
    return RV_OK;
}


/********************************************************************************************
 * RvQueueEnd - Shuts down the Queue module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 * Thread-safe function
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : always RV_OK
 */
RvStatus RvQueueEnd(void)
{
    return RV_OK;
}


/********************************************************************************************
 * RvQueueSourceConstruct - Constructs queue module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * queue module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvQueueSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

#if (RV_QUEUE_TYPE != RV_QUEUE_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->queueSource, "QUEUE", "Queue functions");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


#if (RV_QUEUE_TYPE != RV_QUEUE_NONE)

/********************************************************************************************
 * RvQueueConstruct - Constructs a queue object based on the parameters passed in.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 * Memory is allocated from region bufmem unless the OS allocates its own memory
 * and doesn't provide any control over it.
 *
 * INPUT   : numitems - number of items that the queue should be able to hold.
 *           itemsize - Maximum size of each item that will put put into the queue.
 *           bufmem   - Memory region to allocate queue memeory from (NULL = default region).
 *           logMgr   - log manager
 * OUTPUT  : queue    - constructed queue
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvQueueConstruct(
    IN  RvSize_t    numitems,
    IN  RvSize_t    itemsize,
    IN  RvMemory    *bufmem,
    IN  RvLogMgr    *logMgr,
    OUT RvQueue     *queue)
{
    RvStatus result;

    RvQueueLogEnter((&logMgr->queueSource, "RvQueueConstruct(queue=%p)", queue));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL) {
        RvQueueLogError((&logMgr->queueSource, "RvQueueConstruct: NULL param(s)"));
        return RvQueueErrorCode(RV_ERROR_NULLPTR);
    }
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((numitems == 0) || (itemsize == 0)) {
        RvQueueLogError((&logMgr->queueSource, "RvQueueConstruct: Range"));
        return RvQueueErrorCode(RV_ERROR_OUTOFRANGE);
    }
#endif

    queue->logMgr = logMgr;
    if (logMgr != NULL)
        queue->queueSource = &logMgr->queueSource;
    else
        queue->queueSource = NULL;

    result = RvLockConstruct(logMgr, &queue->lock);
    if(result != RV_OK) {
        RvQueueLogError((&logMgr->queueSource, "RvQueueConstruct: RvLockConstruct"));
        return result;
    }

    result = RvSemaphoreConstruct(RvUint32Const(0),logMgr, &queue->emptysem);
    if(result != RV_OK) {
        RvLockDestruct(&queue->lock,logMgr);
        RvQueueLogError((&logMgr->queueSource, "RvQueueConstruct: RvSemaphoreConstruct (emptysem)"));
        return result;
    }
    result = RvSemaphoreConstruct(RvUint32Const(0), logMgr, &queue->fullsem);
    if(result != RV_OK) {
        RvSemaphoreDestruct(&queue->emptysem,logMgr);
        RvLockDestruct(&queue->lock,logMgr);
        RvQueueLogError((&logMgr->queueSource, "RvQueueConstruct: RvSemaphoreConstruct (fullsem)"));
        return result;
    }
    queue->waitempty = 0;
    queue->waitfull = 0;
    queue->notifyempty = 0;
    queue->notifyfull = 0;
    queue->stopped = RV_FALSE;

    /* Allocate it from the requested region */
    result = RvMemoryAlloc(bufmem, (numitems * itemsize),logMgr, &queue->bufstart);
    if(result != RV_OK) {
        RvLockDestruct(&queue->lock,logMgr);
        RvSemaphoreDestruct(&queue->fullsem,logMgr);
        RvSemaphoreDestruct(&queue->emptysem,logMgr);
        RvQueueLogError((&logMgr->queueSource, "RvQueueConstruct: RvMemoryAlloc"));
        return result;
    }

    /* Remember last item in buffer for speedier check. */
    queue->bufend = (void *)((RvInt8 *)queue->bufstart + ((numitems - 1) * itemsize));

    queue->firstitem = NULL;
    queue->lastitem = NULL;
    queue->size = numitems;
    queue->itemsize = itemsize;
    queue->curitems = 0;

    RvQueueLogLeave((&logMgr->queueSource, "RvQueueConstruct(queue=%p)", queue));

    return RV_OK;
}

/********************************************************************************************
 * RvQueueDestruct - Destructs a queue.
 *
 * All items in the queue are lost when the queue is destructed.
 * Insure that no threads are operating on the queue when it is
 * destructing since not all operating systems handle the situation
 * gracefully.
 *
 * INPUT   : queue    - Pointer to queue object to be Destructed.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvQueueDestruct(
    IN RvQueue *queue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL)
        return RvQueueErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueDestruct(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);
    RvMemoryFree(queue->bufstart, queue->logMgr);
    RvSemaphoreDestruct(&queue->fullsem,queue->logMgr);
    RvSemaphoreDestruct(&queue->emptysem,queue->logMgr);
    RvLockRelease(&queue->lock,queue->logMgr);
    RvLockDestruct(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueDestruct(queue=%p)", queue));

    return RV_OK;
}


/********************************************************************************************
 * RvQueueReceive - Gets an item from the front of a queue.
 *
 * A thread waiting for data on a queue will be resumed and return
 * RV_QUEUE_ERROR_STOPPED if the queue is stopped.
 * If the buffer is smaller than the item in the queue, the data
 * copied into the buffer will be truncated.
 *
 * INPUT   : queue    - Pointer to queue object to get item from.
 *           wait     - Set to RV_QUEUE_WAIT if thread should wait until item is available.
 *                      If set to RV_QUEUE_NOWAIT, RV_QUEUE_ERROR_EMPTY will be returned if
 *                      no item is available.
 * OUTPUT  : buf      - Pointer to buffer that item will be copied to.
 *           bufsize  - Size of the buffer that buf points to.
 * RETURN  : RV_OK if successful otherwise an error code. An error code of RV_QUEUE_ERROR_STOPPED
 *          indicates that the queue has been permanently stopped and there are no more items
 *          available in the queue.
 */
RVCOREAPI RvStatus RVCALLCONV RvQueueReceive(
    IN  RvQueue     *queue,
    IN  RvBool      wait,
    OUT RvSize_t    bufsize,
    OUT void        *buf)
{
    RvSize_t copysize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((queue == NULL) || (buf == NULL))
        return RvQueueErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueReceive(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);

    if(queue->waitempty >= queue->curitems) {
        /* queue is empty or other tasks already waiting for available data */
        if(queue->stopped == RV_TRUE) {
        /* Just indicate when queue has been stopped. */
            RvLockRelease(&queue->lock,queue->logMgr);
            RvLogError(queue->queueSource,
                (queue->queueSource, "RvQueueReceive(queue=%p): Queue has been stopped", queue));
            return RvQueueErrorCode(RV_QUEUE_ERROR_STOPPED);
        }
        if(wait == RV_QUEUE_NOWAIT) {
            /* Don't wait, just return and indicate empty */
            RvLockRelease(&queue->lock,queue->logMgr);
            RvLogWarning(queue->queueSource,
                (queue->queueSource, "RvQueueReceive(queue=%p): Queue is empty", queue));
            return RvQueueErrorCode(RV_QUEUE_ERROR_EMPTY);
        }

        /* Wait until something is available. Loop in case queue is cleared */
        /* after semaphore has notified us to get data. */
        do {
            queue->waitempty += 1;
            RvLockRelease(&queue->lock,queue->logMgr);
            RvSemaphoreWait(&queue->emptysem,queue->logMgr);
            RvLockGet(&queue->lock,queue->logMgr);
            queue->waitempty -= 1;
            queue->notifyempty -= 1; /* notify received */

            /* The queue may have stopped while we were waiting. */
            if(queue->stopped == RV_TRUE) {
                /* Just indicate when queue has been stopped. */
                RvLockRelease(&queue->lock,queue->logMgr);
                RvLogError(queue->queueSource,
                    (queue->queueSource, "RvQueueReceive(queue=%p): Queue has been stopped", queue));
                return RvQueueErrorCode(RV_QUEUE_ERROR_STOPPED);
            }
        } while(queue->curitems == 0);
    }

    /* Copy out data (safely) */
    copysize = RvMin(queue->itemsize, bufsize);
    memcpy(buf, queue->firstitem, copysize);

    /* Adjust queue information */
    queue->curitems -= 1;
    if(queue->firstitem == queue->bufend) {
        queue->firstitem = queue->bufstart; /* Wrap to top of buffer */
    } else queue->firstitem = (void *)((RvInt8 *)queue->firstitem + queue->itemsize);

    /* Wake up someone waiting to put item on queue. */
    if(queue->waitfull > queue->notifyfull) {
        queue->notifyfull += 1;
        RvSemaphorePost(&queue->fullsem,queue->logMgr);
    }

    RvLockRelease(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueReceive(queue=%p)", queue));

    return RV_OK;
}


/********************************************************************************************
 * RvQueueSend - Puts an item into a queue.
 *
 * A thread waiting to put data on a queue will be resumed and return
 * RV_QUEUE_ERROR_STOPPED if the queue is stopped.
 *
 * INPUT   : queue    - Pointer to queue object where item should be placed.
 *           buf      - Pointer to buffer that contains the item.
 *           bufsize  - Size of the item that buf points to.
 *           priority - Set to RV_QUEUE_SEND_NORMAL to put item at the end of the queue.
 *                      Set to RV_QUEUE_SEND_URGENT to put the item at the front
 *                      of the queue.
 *           wait     - Set to RV_QUEUE_WAIT if thread should wait until space is available
 *                      in the queue for the item. If set to RV_QUEUE_NOWAIT,
 *                      RV_QUEUE_ERROR_FULL will be returned if there is no space in the queue.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code. An error code of RV_QUEUE_ERROR_STOPPED
 *          indicates that the queue has been permanently stopped and no more items are allowed
 *          to be put in the queue.
 */
RVCOREAPI
RvStatus RVCALLCONV RvQueueSend(
    IN  RvQueue     *queue,
    IN  void        *buf,
    IN  RvSize_t    bufsize,
    IN  RvInt       priority,
    IN  RvBool      wait)
{
    RvSize_t copysize;
    void *newitem;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((queue == NULL) || (buf == NULL))
        return RvQueueErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueSend(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);

    if(queue->stopped == RV_TRUE) {
        /* Just indicate when queue has been stopped. */
        RvLockRelease(&queue->lock,queue->logMgr);
        RvLogError(queue->queueSource,
            (queue->queueSource, "RvQueueSend(queue=%p): Queue has been stopped", queue));
        return RvQueueErrorCode(RV_QUEUE_ERROR_STOPPED);
    }

    if(queue->waitfull >= (queue->size - queue->curitems)) {
        /* queue is full or tasks already waiting for available space */
        if(wait == RV_QUEUE_NOWAIT) {
            /* Don't wait, just return and indicate full */
            RvLockRelease(&queue->lock,NULL);
            RvLogError(queue->queueSource,
                (queue->queueSource, "RvQueueSend(queue=%p): Queue is full", queue));
            return RvQueueErrorCode(RV_QUEUE_ERROR_FULL);
        }

        /* Wait until there is space available. */
        queue->waitfull += 1;
        RvLockRelease(&queue->lock,queue->logMgr);
        RvSemaphoreWait(&queue->fullsem,queue->logMgr);
        RvLockGet(&queue->lock,queue->logMgr);
        queue->waitfull -= 1;
        queue->notifyfull -= 1; /* notify received */

        /* The queue may have stopped while we were waiting. */
        if(queue->stopped == RV_TRUE) {
            /* Just indicate when queue has been stopped. */
            RvLockRelease(&queue->lock,queue->logMgr);
            RvLogError(queue->queueSource,
                (queue->queueSource, "RvQueueSend(queue=%p): Queue has been stopped", queue));
            return RvQueueErrorCode(RV_QUEUE_ERROR_STOPPED);
        }
    }

    /* Adjust queue information */
    if(queue->curitems == 0) {
        /* queue is empty, putting on front or end is the same */
        queue->firstitem = queue->bufstart;
        queue->lastitem = queue->bufstart;
        newitem = queue->bufstart;
    } else {
        if(priority == RV_QUEUE_SEND_NORMAL) {
            /* Put on end of queue */
            if(queue->lastitem == queue->bufend) {
                queue->lastitem = queue->bufstart; /* Wrap to top of buffer */
            } else queue->lastitem = (void *)((RvInt8 *)queue->lastitem + queue->itemsize);
            newitem = queue->lastitem;
        } else {
            /* Put on front of queue (URGENT) */
            if(queue->firstitem == queue->bufstart) {
                queue->firstitem = queue->bufend; /* Wrap to end of buffer */
            } else queue->firstitem = (void *)((RvInt8 *)queue->firstitem - queue->itemsize);
            newitem = queue->firstitem;
        }
    }
    queue->curitems += 1;

    /* Copy in data (safely) */
    copysize = RvMin(queue->itemsize, bufsize);
    memcpy(newitem, buf, copysize);

    /* Wake up someone waiting to get item from queue */
    if(queue->waitempty > queue->notifyempty) {
        queue->notifyempty += 1;
        RvSemaphorePost(&queue->emptysem,queue->logMgr);
    }

    RvLockRelease(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueSend(queue=%p)", queue));

    return RV_OK;
}


/********************************************************************************************
 * RvQueueStop - Permanently stops a queue.
 *
 * No more items will be allowed to be put on the queue.
 * Threads waiting to get items from the queue or waiting to put items onto the queue
 * will be resumed (and return with an error code indicating that the queue has been
 * stopped). Items already on the queue will still be available.
 * When a queue is stopped it is permanent, there is no way to undo it.
 *
 * INPUT   : queue    - Pointer to queue object to be stopped.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvQueueStop(
    IN RvQueue *queue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL)
        return RvQueueErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueStop(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);
    if(queue->stopped == RV_TRUE) {
        RvLockRelease(&queue->lock,queue->logMgr);
        RvLogWarning(queue->queueSource,
            (queue->queueSource, "RvQueueStop(queue=%p): Queue is already stopped", queue));
        return RV_OK;
    }

    queue->stopped = RV_TRUE;

    /* If any tasks are blocked trying to send, release them. */
    while(queue->notifyfull < queue->waitfull) {
        queue->notifyfull += 1;
        RvSemaphorePost(&queue->fullsem,queue->logMgr);
    }

    /* If any tasks are blocked trying to receive, release them. */
    while(queue->notifyempty < queue->waitempty) {
        queue->notifyempty += 1;
        RvSemaphorePost(&queue->emptysem,queue->logMgr);
    }

    RvLockRelease(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueStop(queue=%p)", queue));

    return RV_OK;
}


/********************************************************************************************
 * RvQueueClear - Clears all items from a queue.
 *
 * note:  Stopped queues may be cleared.
 *
 * INPUT   : queue    - Pointer to queue object to be cleared.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI RvStatus RVCALLCONV RvQueueClear(
    IN RvQueue *queue)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL)
        return RvQueueErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueClear(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);
    queue->curitems = 0;

    /* Notify sending tasks about the space now available. */
    while((queue->notifyfull < queue->waitfull)  && (queue->notifyfull < queue->size)) {
        queue->notifyfull += 1;
        RvSemaphorePost(&queue->fullsem,queue->logMgr);
    }

    RvLockRelease(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueClear(queue=%p)", queue));

    return RV_OK;
}


/********************************************************************************************
 * RvQueueIsStopped - Checks to see if a queue has been stopped.
 *
 * INPUT   : queue    - Pointer to queue object to check.
 * OUTPUT  : none
 * RETURN  : Returns RV_TRUE if the queue has been stopped, RV_FALSE otherwise.
 */
RVCOREAPI RvBool RVCALLCONV RvQueueIsStopped(
    IN RvQueue *queue)
{
    RvBool result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL)
        return RvQueueErrorCode(RV_TRUE);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueIsStopped(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);
    result = queue->stopped;
    RvLockRelease(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueIsStopped(queue=%p)", queue));

    return result;
}


/********************************************************************************************
 * RvQueueSize - Finds out the size of the queue.
 *
 * INPUT   : queue    - Pointer to queue object to check.
 * OUTPUT  : none
 * RETURN  : The maximum number of items that the queue can hold.
 */
RVCOREAPI RvSize_t RVCALLCONV RvQueueSize(
    IN RvQueue *queue)
{
    RvSize_t size;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL)
        return 0;
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueSize(queue=%p)", queue));

    size = queue->size; /* no locks since this never changes */

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueSize(queue=%p)", queue));

    return size;
}


/********************************************************************************************
 * RvQueueItems - Finds out the number of items currently on the queue.
 *
 * INPUT   : queue    - Pointer to queue object to check.
 * OUTPUT  : none
 * RETURN  : The current number of items on the queue.
 */
RVCOREAPI RvSize_t RVCALLCONV RvQueueItems(
    IN RvQueue *queue)
{
    RvSize_t result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(queue == NULL)
        return RvQueueErrorCode(RV_TRUE);
#endif

    RvLogEnter(queue->queueSource, (queue->queueSource, "RvQueueItems(queue=%p)", queue));

    RvLockGet(&queue->lock,queue->logMgr);
    result = queue->curitems;
    RvLockRelease(&queue->lock,queue->logMgr);

    RvLogLeave(queue->queueSource, (queue->queueSource, "RvQueueItems(queue=%p)", queue));

    return result;
}

#endif /* (RV_QUEUE_TYPE != RV_QUEUE_NONE) */
