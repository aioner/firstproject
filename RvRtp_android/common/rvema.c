/***********************************************************************
        Copyright (c) 2003 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

/* Used when we want to debug only a specific resource type */
/*#define EMA_DEBUG_SPECIFIC
#define RV_EMA_DEBUG_DEADLOCKS
#define EMA_SPECIFIC_NAME "TRANSPORT Hosts"*/


#include "rvmutex.h"
#include "rvlock.h"
#include "rvmemory.h"
#include "rvlog.h"
#include "rvra.h"
#include "rvema.h"
#if defined(RV_EMA_DEBUG_DEADLOCKS)
#include "rvthread.h"
#endif
#include <string.h>


#ifdef EMA_DEBUG_SPECIFIC
#define RvEmaLogInfo(_params) {if (ema->logSpecific) RvLogInfo(&ema->log, _params);}
#define RvEmaLogDebug(_params) {if (ema->logSpecific) RvLogDebug(&ema->log, _params);}
#define RvEmaLogWarning(_params) {if (ema->logSpecific) RvLogWarning(&ema->log, _params);}
#define RvEmaLogError(_params) {if (ema->logSpecific) RvLogError(&ema->log, _params);}
#define RvEmaLogExcep(_params) {if (ema->logSpecific) RvLogExcep(&ema->log, _params);}

#else
#define RvEmaLogInfo(_params) RvLogInfo(&ema->log, _params)
#define RvEmaLogDebug(_params) RvLogDebug(&ema->log, _params)
#define RvEmaLogWarning(_params) RvLogWarning(&ema->log, _params)
#define RvEmaLogError(_params) RvLogError(&ema->log, _params)
#define RvEmaLogExcep(_params) RvLogExcep(&ema->log, _params)
#endif


/************************************************************************
 * emaObject struct
 * Holds the information of an emaObject
 ************************************************************************/
typedef struct
{
    HRA         ra; /* Handle to RA we're using */
    RvLogMgr*   logMgr; /* Log manager used */
    RvLogSource log; /* Log to use for messages */
    RvLock      lock; /* Lock we're using */
    RvEmaLockType lockType; /* Type of locking mechanism to use */
    RvSize_t    elemSize; /* Size of each element inside EMA */
    RvUint32    type; /* Integer representing the type of objects stored in EMA */
    void*       userData; /* User related information associated with this EMA object */
    void const* instance; /* Instance associated with this EMA object */
    RvUint32    markCount; /* Number of items currently deleted and marked */

#ifdef EMA_DEBUG_SPECIFIC
    RvBool      logSpecific; /* Are we logging this EMA? */
#endif
} emaObject;


/************************************************************************
 * emaElem struct
 * Holds the information of an ema element
 *
 * Note: The mutex for the element is held in the end of the allocation
 *       if using emaNormalLocks. If emaLinkedLocks is used, then in the
 *       end of the allocation we'll have to pointer to the linked element
 *       in another EMA construct.
 ************************************************************************/
typedef struct
{
    emaObject*      ema; /* Pointer to the EMA object.
                            When this pointer is NULL, it indicates that the element
                            was deleted. We use this for RvEmaLock().
                            THIS MUST BE THE FIRST FIELD IN THIS STRUCT! */
#ifdef RV_EMA_DEBUG
    RvUint32        debug1; /* Debugging bytes == EMA_DEBUG_BYTES */
#endif

    RvUint32        flags; /* Reference count and locks count.
                              It also holds a bit indicating if element was deleted */
    void*           appHandle; /* Application's handle of the EMA object */

#ifdef RV_EMA_DEBUG_DEADLOCKS
    RvThreadId      lockedThread; /* Indication who's the thread that locked this object */
#ifdef RV_EMA_DEBUG
    const RvChar*   filename; /* Filename that locked this object */
    int             lineno; /* Line number that locked this object */
#endif
#endif
#ifdef RV_EMA_DEBUG
    RvUint32        debug2; /* Debugging bytes == EMA_DEBUG_BYTES */
#endif
} emaElem;



/* Bytes used when debuging memory allocations */
#define EMA_DEBUG_BYTES (0xdeadbeef)

/* Indication in the reference count that this element was deleted */
#define EMA_ALREADY_DELETED     0x80000000

/* Indication of the reference count's actual value */
#define EMA_GET_REFERENCE(elem)         ((elem)->flags & 0x0000ffff)
#define EMA_INC_REFERENCE(elem,count)   ((elem)->flags += (RvUint16)(count))
#define EMA_DEC_REFERENCE(elem,count)   ((elem)->flags -= (RvUint16)(count))

/* Indication of the locks count's actual value */
#define EMA_GET_LOCKS(elem)             (((elem)->flags & 0x7fff0000) >> 16)
#define EMA_INC_LOCKS(elem,count)       ((elem)->flags += (((RvUint32)(count)) << 16))
#define EMA_DEC_LOCKS(elem,count)       ((elem)->flags -= (((RvUint32)(count)) << 16))

#define SIZEOF_EMAELEM RvRoundToSize(sizeof(emaElem), RV_ALIGN_SIZE)


/************************************************************************
 *
 *                              Private functions
 *
 ************************************************************************/


/************************************************************************
 * emaDeleteElement
 * purpose: Delete an element used by ema.
 * input  : ema         - EMA object used
 *          rElem       - Element to delete
 *          location    - Location of element deleted
 *          decCount    - RV_TRUE if we need to decrease count of marked/deleted elements
 *          functionName- Name of function that called this one - used for log
 * output : none
 * return : none
 ************************************************************************/
static void emaDeleteElement(
    IN emaObject*   ema,
    IN emaElem*     rElem,
    IN int          location,
    IN RvBool       decCount,
    IN const char*  functionName)
{
    RvBool locked = RV_TRUE;
    EMAElement parent = NULL;
    RV_UNUSED_ARG(location);
    RV_UNUSED_ARG(functionName);

    /* First of all, let's lock the element and remove the reference to ema on it.
       Doing this, allows us to stay blocked on an RvEmaLock() call while trying to
       delete the element */
    switch (ema->lockType)
    {
        case RvEmaNoLocks:        break;
        case RvEmaNormalLocks:    RvMutexLock((RvMutex *)((char*)rElem + SIZEOF_EMAELEM + ema->elemSize), ema->logMgr); break;
        case RvEmaLinkedLocks:
        {
            parent = *((EMAElement*)((char*)rElem + SIZEOF_EMAELEM + ema->elemSize));
            if (parent != NULL)
                locked = RvEmaLock(parent);
            else
                locked = RV_FALSE;
            break;
        }
    }
    rElem->ema = NULL;
    if (locked)
    {
        switch (ema->lockType)
        {
            case RvEmaNoLocks:        break;
            case RvEmaNormalLocks:    RvMutexUnlock((RvMutex *)((char*)rElem + SIZEOF_EMAELEM + ema->elemSize), ema->logMgr); break;
            case RvEmaLinkedLocks:    RvEmaUnlock(parent); break;
        }
    }

    /* Do all the rest here */
    RvLockGet(&ema->lock, ema->logMgr);

    if (decCount)
        ema->markCount--;

    RvRaDelete(ema->ra, (RAElement)rElem);

    RvLockRelease(&ema->lock, ema->logMgr);

    RvEmaLogDebug((&ema->log, "%s (%s): %p deleted (location=%d)", functionName, RvRaGetName(ema->ra), rElem, location));
}







/************************************************************************
 *
 *                              Public functions
 *
 ************************************************************************/


/************************************************************************
 * RvEmaConstruct
 * purpose: Create an EMA object
 * input  : elemSize            - Size of elements in the EMA in bytes
 *          maxNumOfElements    - Number of elements in EMA
 *          lockType            - Type of locking mechanism to use
 *          name                - Name of EMA (used in log messages)
 *          type                - Integer representing the type of objects
 *                                stored in this EMA.
 *          userData            - User related information associated with
 *                                this EMA object.
 *          instance            - Instance associated with this EMA object.
 *          logMgr              - Log manager to use.
 * output : none
 * return : Handle to RA constructed on success
 *          NULL on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaConstruct(
    IN RvInt            elemSize,
    IN RvInt            maxNumOfElements,
    IN RvEmaLockType    lockType,
    IN const char*      name,
    IN RvUint32         type,
    IN void*            userData,
    IN void const*      instance,
    IN RvLogMgr*        logMgr,
    OUT HEMA*           hEma)
{
    emaObject* ema;
    int allocSize = 0;

    /* Allocate the object */
    if (RvMemoryAlloc(NULL, sizeof(emaObject), logMgr, (void**)&ema) != RV_OK)
        return RV_ERROR_UNKNOWN;

    /* Remember the type of elements stored */
    ema->type = type;
    ema->userData = userData;
    ema->instance = instance;
    ema->logMgr = logMgr;
#ifdef EMA_DEBUG_SPECIFIC
    ema->logSpecific = (strcmp(name, EMA_SPECIFIC_NAME) == 0);
#endif

    /* Create a log handle for our use */
    RvLogSourceConstruct(ema->logMgr, &ema->log, "EMA", "Enhanced Memory Allocator");

    /* Calculate the size of each element (platform-aligned) */
    ema->elemSize = RvRoundToSize(elemSize, RV_ALIGN_SIZE);

    /* See if we need a lock for each element */
    switch (lockType)
    {
        case RvEmaNoLocks:        allocSize = (int)(ema->elemSize + SIZEOF_EMAELEM); break;
        case RvEmaNormalLocks:    allocSize = (int)(ema->elemSize + SIZEOF_EMAELEM + sizeof(RvMutex)); break;
        case RvEmaLinkedLocks:    allocSize = (int)(ema->elemSize + SIZEOF_EMAELEM + sizeof(EMAElement*)); break;
    }

    /* Create the RA */
    RvRaConstruct(allocSize, maxNumOfElements, RV_FALSE, name, logMgr, &ema->ra);

    /* Create the mutex */
    RvLockConstruct(ema->logMgr, &ema->lock);
    ema->lockType = lockType;
    ema->markCount = 0;

#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    if (lockType == RvEmaNormalLocks)
    {
        int i, j;
        RvStatus res = RV_OK;

        /* Initialize all the mutexes we need */
        for (i = 0; i < maxNumOfElements; i++)
        {
            res = RvMutexConstruct(ema->logMgr, (RvMutex *) (RV_RA_ELEM_DATA(ema->ra, i) + SIZEOF_EMAELEM + ema->elemSize));
            if (res != RV_OK)
                break;
        }
        if (res != RV_OK)
        {
            RvEmaLogError((&ema->log, "RvEmaConstruct (%s): could not construct mutex", RvRaGetName(ema->ra)));
            RvLockDestruct(&ema->lock, ema->logMgr);
            for (j = 0; j < i; j++)
            {
                RvMutexDestruct((RvMutex *) (RV_RA_ELEM_DATA(ema->ra, j) + SIZEOF_EMAELEM + ema->elemSize), ema->logMgr);
            }
            RvRaDestruct(ema->ra);
            RvLogSourceDestruct(&ema->log);
            RvMemoryFree(ema, ema->logMgr);
            return RV_ERROR_UNKNOWN;
        }
    }
#endif

    *hEma = (HEMA)ema;

    return RV_OK;
}


/************************************************************************
 * RvEmaDestruct
 * purpose: Free an EMA object, deallocating all of its used memory
 * input  : emaH   - Handle of the EMA object
 * output : none
 * return : none
 ************************************************************************/
RVCOREAPI void RVCALLCONV
RvEmaDestruct(IN HEMA emaH)
{
    emaObject* ema = (emaObject *)emaH;
    int numElems;

    if (ema == NULL) return;

    numElems = RvRaCurSize(ema->ra);
    if (numElems > 0)
    {
        RvEmaLogWarning((&ema->log, "RvEmaDestruct (%s): %d elements not deleted", RvRaGetName(ema->ra), numElems));
    }

    /* Remove lock */
    RvLockDestruct(&ema->lock, ema->logMgr);

#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    if (ema->lockType == RvEmaNormalLocks)
    {
        int i;

        /* End all the mutexes we initialized */
        for (i = 0; i < RvRaMaxSize(ema->ra); i++)
        {
            RvMutexDestruct((RvMutex *) (RV_RA_ELEM_DATA(ema->ra, i) + SIZEOF_EMAELEM + ema->elemSize), ema->logMgr);
        }
    }
#endif

    /* Free any used memory and RA */
    RvRaDestruct(ema->ra);
    RvLogSourceDestruct(&ema->log);
    RvMemoryFree(ema, ema->logMgr);
}


/************************************************************************
 * RvEmaClear
 * purpose: Clear EMA from all used objects
 * input  : emaH   - Handle of the EMA object
 * output : none
 * return : none
 ************************************************************************/
RVCOREAPI void RVCALLCONV
RvEmaClear(IN HEMA emaH)
{
    emaObject* ema = (emaObject *)emaH;

    RvRaClear(ema->ra);
}


/************************************************************************
 * RvEmaAdd
 * purpose: Allocate an element in EMA for use, without initializing its
 *          value.
 *          This automatically locks the EMA object.
 * input  : emaH       - Handle of the EMA object
 *          appHandle   - Application's handle for the EMA object
 * output : none
 * return : Pointer to element added on success
 *          NULL on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaAdd(IN HEMA emaH, IN void* appHandle, OUT EMAElement* emaElement)
{
    emaObject* ema = (emaObject *)emaH;
    emaElem* elem;
    char* ptr;
    int location = -1;

    /* Use RA for our purposes */
    RvLockGet(&ema->lock, ema->logMgr);
    RvRaAdd(ema->ra, (RAElement*)&elem, &location);
    RvLockRelease(&ema->lock, ema->logMgr);

    /* Make sure it was allocated */
    if (location < 0)
    {
        RvEmaLogError((&ema->log, "RvEmaAdd (%s): Out of resources", RvRaGetName(ema->ra)));
        return RV_ERROR_UNKNOWN;
    }

    /* Set the element's OO information */
    elem->ema = ema;
    elem->flags = 0;
    elem->appHandle = appHandle;
#if defined(RV_EMA_DEBUG)
    elem->debug1 = EMA_DEBUG_BYTES;
    elem->debug2 = EMA_DEBUG_BYTES;
#endif

    RvEmaLogDebug((&ema->log, "RvEmaAdd (%s): Got %p (location=%d)", RvRaGetName(ema->ra), elem, location));

    ptr = (char*)elem + SIZEOF_EMAELEM;

    /* Allocate a mutex if necessary */
    switch (ema->lockType)
    {
        case RvEmaLinkedLocks: memset(ptr + ema->elemSize, 0, sizeof(EMAElement*)); break;
        default:             break;
    }

    /* Calculate and return the position of the true element */
    *emaElement = (EMAElement*)ptr;

    return RV_OK;
}


/************************************************************************
 * RvEmaDelete
 * purpose: Delete an element from EMA
 * input  : elem    - Element to delete
 * return : Negative value on failure
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaDeleteDebug(IN EMAElement elem, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaDelete(IN EMAElement elem)
#endif
{
    emaObject* ema;
    emaElem* rElem;
    int location;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Find the location */
    RvRaGetByPtr(ema->ra, rElem, &location);

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log, "RvEmaDelete (%s): Deleting %d,%p,refCount=%d (%s:%d)",
            RvRaGetName(ema->ra), location, rElem,
            EMA_GET_REFERENCE(rElem), filename, lineno));
#else
    RvEmaLogDebug((&ema->log, "RvEmaDelete (%s): Deleting %d,%p (refCount=%d)",
            RvRaGetName(ema->ra), location, rElem,
            EMA_GET_REFERENCE(rElem)));
#endif

#if defined(RV_EMA_DEBUG)
    if ((rElem->debug1 != EMA_DEBUG_BYTES) || (rElem->debug2 != EMA_DEBUG_BYTES))
    {
        RvEmaLogExcep((&ema->log, "RvEmaDelete (%s): Someone is messing with memory %p", RvRaGetName(ema->ra), rElem));
    }
#endif

    /* Check the reference count */
    if (rElem->flags == 0)
    {
        /* No one is looking for this guy - we can delete it */
        emaDeleteElement(ema, rElem, location, RV_FALSE, "RvEmaDelete");
    }
    else
    {
        RvLockGet(&ema->lock, ema->logMgr);

#if defined(RV_EMA_DEBUG)
        if ((rElem->flags & EMA_ALREADY_DELETED) != 0)
        {
            RvEmaLogExcep((&ema->log, "RvEmaDelete (%s): Deleting an element %p for the second time",
                     RvRaGetName(ema->ra), rElem));
        }
#endif
        ema->markCount++;
        rElem->flags |= EMA_ALREADY_DELETED;

        RvLockRelease(&ema->lock, ema->logMgr);
    }


    return RV_OK;
}


/************************************************************************
 * RvEmaLinkToElement
 * purpose: Link an EMA element to another element, from a different
 *          EMA construct. This function should be used when the EMA we're
 *          dealing with was created with emaLinkedLocks. This function
 *          allows the element to use a different element's lock.
 *          This function will only work if the element has no reference
 *          count at present.
 * input  : elem        - Element to link
 *          otherElem   - Element we're linking to. Should be constructed
 *                        with emaNormalLocks or linked to such element.
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaLinkToElement(IN EMAElement elem, IN EMAElement otherElem)
{
    emaObject*  ema;
    emaElem*    rElem;
    EMAElement* parent;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Make sure we've got the element */
    if (ema == NULL) return RV_ERROR_UNKNOWN;

    RvEmaLogDebug((&ema->log, "RvEmaLinkToElement (%s): Linking %p to %p", RvRaGetName(ema->ra), rElem, otherElem));

#if defined(RV_EMA_DEBUG)
    if (ema->lockType != RvEmaLinkedLocks)
    {
        RvEmaLogExcep((&ema->log, "RvEmaLinkToElement (%s): This EMA cannot be linked", RvRaGetName(ema->ra)));
        return RV_ERROR_UNKNOWN;
    }
#endif

    /* Find place of parent */
    parent = (EMAElement*)((char*)elem + ema->elemSize);
    if (EMA_GET_REFERENCE(rElem) > 0)
    {
        RvEmaLogError((&ema->log, "RvEmaLinkToElement (%s): Cannot link %p - has a positive reference count",
                 RvRaGetName(ema->ra), rElem));
        return RV_ERROR_UNKNOWN;
    }

    *parent = otherElem;

    return RV_OK;
}


/************************************************************************
 * RvEmaLock
 * purpose: Lock an element in EMA for use from the executing thread only
 *          This function will succeed only if the element exists
 * input  : elem    - Element to lock
 * output : none
 * return : RV_OK      - When the element exists and was locked
 *          RV_ERROR   - When the element doesn't exist (NULL are was deleted)
 *                       In this case, there's no need to call RvEmaUnlock().
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaLockDebug(IN EMAElement elem, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaLock(IN EMAElement elem)
#endif
{
    emaObject*  ema;
    emaElem*    rElem;
    RvBool      status = RV_TRUE;
    EMAElement  parent = NULL;
#if defined(RV_EMA_DEBUG_DEADLOCKS)
    RvThreadId  curThreadId = RvThreadCurrentId();
#endif

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Make sure we've got the element */
    if (ema == NULL) return RV_ERROR_UNKNOWN;

    RvLogEnter(&ema->log, (&ema->log,"RvEmaLock(elem=%p)",elem));

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log, "RvEmaLock (%s): Locking %p (%s:%d)", RvRaGetName(ema->ra), rElem, filename, lineno));
#endif

    switch (ema->lockType)
    {
#if defined(RV_EMA_DEBUG)
        case RvEmaNoLocks:
            RvEmaLogExcep((&ema->log, "RvEmaLock (%s): This EMA cannot be locked", RvRaGetName(ema->ra)));
            break;
#endif
        case RvEmaNormalLocks:
            /* We lock it */
            RvMutexLock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr);

            /* Now that it's locked, see if the element still exists */
            if (rElem->ema == NULL)
            {
                /* Seems like someone has deleted this element when we were trying to lock it */
#if defined(RV_EMA_DEBUG)
                RvEmaLogDebug((&ema->log, "RvEmaLock (%s): Unlocking deleted element %p", RvRaGetName(ema->ra), rElem));
#endif
                status = RV_FALSE;

                /* Release the lock - we shouldn't go on with it */
                RvMutexUnlock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr);
            }
            break;

        case RvEmaLinkedLocks:
        {
            /* We lock the parent */
            parent = *((EMAElement*)((char*)elem + ema->elemSize));
            if (parent != NULL)
                status = RvEmaLock(parent);
            else
                status = RV_FALSE;

            if (status == RV_FALSE)
            {
                RvEmaLogWarning((&ema->log, "RvEmaLock (%s): Couldn't lock parent=%p of %p for some reason",
                         RvRaGetName(ema->ra), parent, elem));
            }
            break;
        }
        default:
            break;
    }

    /* Make sure we increment the reference count on this one */
    if (status == RV_TRUE)
    {
#if defined(RV_EMA_DEBUG_DEADLOCKS)
        rElem->lockedThread = curThreadId;
#if defined(RV_EMA_DEBUG)
        rElem->filename = filename;
        rElem->lineno = lineno;
#endif
#endif
        EMA_INC_REFERENCE(rElem,1);
        EMA_INC_LOCKS(rElem,1);
    }

    RvLogLeave(&ema->log, (&ema->log,"RvEmaLock(elem=%p)",elem));

    return status == RV_TRUE ? RV_OK : RV_ERROR_UNKNOWN;
}


/************************************************************************
 * RvEmaUnlock
 * purpose: Unlock an element in EMA that were previously locked by
 *          RvEmaLock() from the same thread
 * input  : elem    - Element to unlock
 * output : none
 * return : RV_TRUE    if element still exists
 *          RV_FALSE   if element was deleted and is not valid anymore
 *          Negative value on failure
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaUnlockDebug(IN EMAElement elem, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaUnlock(IN EMAElement elem)
#endif
{
    emaObject* ema;
    emaElem* rElem;
    int elemExists;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log,
        "RvEmaUnlock (%s): Unlocking %p (%s:%d)", RvRaGetName(ema->ra), rElem, filename, lineno));

    if (EMA_GET_LOCKS(rElem) == 0)
    {
        RvEmaLogExcep((&ema->log,
            "RvEmaUnlock (%s): Element %p (%s:%d) is not locked!", RvRaGetName(ema->ra), rElem, filename, lineno));
        return RV_OK;
    }
#endif

    EMA_DEC_REFERENCE(rElem,1);
    EMA_DEC_LOCKS(rElem,1);
    elemExists = ((rElem->flags & EMA_ALREADY_DELETED) == 0);

    /* Delete element if we're done with it */
    if (rElem->flags == EMA_ALREADY_DELETED)
    {
        int location;
        RvRaGetByPtr(ema->ra, rElem, &location);
        emaDeleteElement(ema, rElem, location, RV_TRUE, "RvEmaUnlock");
    }

    switch (ema->lockType)
    {
#if defined(RV_EMA_DEBUG)
        case RvEmaNoLocks:
            RvEmaLogExcep((&ema->log, "RvEmaUnlock (%s): This EMA cannot be unlocked", RvRaGetName(ema->ra)));
            break;
#endif
        case RvEmaNormalLocks:
            /* We lock it */
            RvMutexUnlock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr);
            break;
        case RvEmaLinkedLocks:
        {
            /* We lock the parent */
            int result = RV_ERROR_UNKNOWN;
            EMAElement  parent = *((EMAElement*)((char*)elem + ema->elemSize));
            if (parent != NULL)
                result = RvEmaUnlock(parent);

            if (result < 0)
            {
                RvEmaLogWarning((&ema->log, "RvEmaUnlock (%s): Couldn't unlock parent=%p of %p for some reason",
                         RvRaGetName(ema->ra), parent, elem));
                elemExists = result;
            }
            break;
        }
        default:
            break;
    }

    return elemExists;
}


/************************************************************************
 * RvEmaMark
 * purpose: Mark an element in EMA for use, not letting anyone delete
 *          this element until it is release.
 *          This automatically locks the EMA object.
 * input  : elem    - Element to mark
 * output : none
 * return : Negative value on failure
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaMarkDebug(IN EMAElement elem, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaMark(IN EMAElement elem)
#endif
{
    emaObject* ema;
    emaElem* rElem;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log, "RvEmaMark (%s): Marking %p, refCount=%d (%s:%d)",
             RvRaGetName(ema->ra), rElem, EMA_GET_REFERENCE(rElem), filename, lineno));

    if ((rElem->debug1 != EMA_DEBUG_BYTES) || (rElem->debug2 != EMA_DEBUG_BYTES))
    {
        RvEmaLogExcep((&ema->log, "RvEmaMark (%s): Someone is messing with memory %p", RvRaGetName(ema->ra), rElem));
    }
#endif

    /* Increase the reference count */
    switch (ema->lockType)
    {
        case RvEmaNoLocks:        break;
        case RvEmaNormalLocks:    RvMutexLock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr); break;
        case RvEmaLinkedLocks:
            RvEmaLock(*((EMAElement*)((char*)elem + ema->elemSize)));
            RvEmaMark(*((EMAElement*)((char*)elem + ema->elemSize)));
            break;
    }

    EMA_INC_REFERENCE(rElem,1);

    switch (ema->lockType)
    {
        case RvEmaNoLocks:        break;
        case RvEmaNormalLocks:    RvMutexUnlock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr); break;
        case RvEmaLinkedLocks:    RvEmaUnlock(*((EMAElement*)((char*)elem + ema->elemSize))); break;
    }

    return RV_OK;
}


/************************************************************************
 * RvEmaRelease
 * purpose: Release an element in EMA after it was marked using
 *          RvEmaMark(), returning an indication if this element
 *          still exists.
 *          This automatically locks the EMA object.
 * input  : elem    - Element to mark
 * output : none
 * return : RV_TRUE    if element still exists
 *          RV_FALSE   if element was deleted and is not valid anymore
 *          Negative value on failure
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaReleaseDebug(IN EMAElement elem, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaRelease(IN EMAElement elem)
#endif
{
    emaObject* ema;
    emaElem* rElem;
    int elemExists;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log,
        "RvEmaRelease (%s): Releasing %p, refCount=%d (%s:%d)",
             RvRaGetName(ema->ra), rElem, EMA_GET_REFERENCE(rElem), filename, lineno));

    if (EMA_GET_REFERENCE(rElem) == 0)
    {
        RvEmaLogExcep((&ema->log,
            "RvEmaRelease (%s): Releasing %p (%s:%d) when it's not marked!",
            RvRaGetName(ema->ra), rElem, filename, lineno));
    }

    if ((rElem->debug1 != EMA_DEBUG_BYTES) || (rElem->debug2 != EMA_DEBUG_BYTES))
    {
        RvEmaLogExcep((&ema->log,
            "RvEmaRelease (%s): Someone is messing with memory %p", RvRaGetName(ema->ra), rElem));
    }
#else
    RvEmaLogDebug((&ema->log,
        "RvEmaRelease (%s): Releasing %p, refCount=%d", RvRaGetName(ema->ra), rElem, EMA_GET_REFERENCE(rElem)));
#endif

    /* Decrease the reference count */
    switch (ema->lockType)
    {
        case RvEmaNoLocks:        break;
        case RvEmaNormalLocks:    RvMutexLock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr); break;
        case RvEmaLinkedLocks:
            RvEmaLock(*((EMAElement*)((char*)elem + ema->elemSize)));
            RvEmaRelease(*((EMAElement*)((char*)elem + ema->elemSize)));
            break;
    }

    EMA_DEC_REFERENCE(rElem,1);
    elemExists = ((rElem->flags & EMA_ALREADY_DELETED) == 0);

    /* Delete element if we're done with it */
    if (rElem->flags == EMA_ALREADY_DELETED)
    {
        int location;
        RvRaGetByPtr(ema->ra, rElem, &location);
        emaDeleteElement(ema, rElem, location, RV_TRUE, "RvEmaRelease");
    }

    switch (ema->lockType)
    {
        case RvEmaNoLocks:        break;
        case RvEmaNormalLocks:    RvMutexUnlock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr); break;
        case RvEmaLinkedLocks:
            RvEmaUnlock(*((EMAElement*)((char*)elem + ema->elemSize)));
            break;
    }

    return elemExists;
}


/************************************************************************
 * RvEmaWasDeleted
 * purpose: Check if an element in EMA was deleted after a call to
 *          RvEmaMark().
 * input  : elem    - Element to mark
 * output : none
 * return : RV_TRUE    if element was deleted
 *          RV_FALSE   if element still exists
 ************************************************************************/
RVCOREAPI RvBool RVCALLCONV
RvEmaWasDeleted(IN EMAElement elem)
{
    emaElem* rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    emaObject *ema = NULL;

    ema = rElem->ema;

	if(ema!=NULL)
	{
		RvEmaLogDebug((&ema->log, "RvEmaWasDeleted (%s): Checking %p (refCount=%d)",
				 RvRaGetName(ema->ra), rElem, EMA_GET_REFERENCE(rElem)));
	}

#if defined(RV_EMA_DEBUG)
    if ((rElem->debug1 != EMA_DEBUG_BYTES) || (rElem->debug2 != EMA_DEBUG_BYTES))
    {
		if(ema!=NULL)
		{
			RvEmaLogExcep((&ema->log, "RvEmaWasDeleted (%s): Someone is messing with memory %p",
				RvRaGetName(ema->ra), rElem));
		}
    }
#endif
#endif

    /* Check if element was deleted */
    return ((rElem->flags & EMA_ALREADY_DELETED) != 0);
}


/************************************************************************
 * RvEmaPrepareForCallback
 * purpose: Prepare an element in EMA for use in a callback to the app.
 *          This function will make sure the element is unlocked the necessary
 *          number of times, and then marked once (so the app won't delete
 *          this element).
 *          RvEmaReturnFromCallback() should be called after the callback,
 *          with the return value of this function.
 * input  : elem    - Element to prepare
 * output : none
 * return : Number of times the element was locked on success
 *          Negative value on failure
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaPrepareForCallbackDebug(IN EMAElement elem, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaPrepareForCallback(IN EMAElement elem)
#endif
{
    emaObject*  ema;
    emaElem*    rElem;
    int         numLocks;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Make sure we've got an element */
    if (ema == NULL) return RV_ERROR_UNKNOWN;

#if defined(RV_EMA_DEBUG)
    if (EMA_GET_LOCKS(rElem) == 0)
    {
        RvEmaLogInfo((&ema->log,
            "RvEmaPrepareForCallback (%s): on %p (%s:%d) when not locked!",
                 RvRaGetName(ema->ra), rElem, filename, lineno));
    }
#endif

    /* Increase the reference count */
    EMA_INC_REFERENCE(rElem,1);

    /* See where the do we have to unlock */
    switch (ema->lockType)
    {
        case RvEmaNormalLocks:
        {
            /* We unlock the element */
            int i;
            numLocks = EMA_GET_LOCKS(rElem);

            /* First we decrease the number of locks, and then we actually exit them... */
            EMA_DEC_LOCKS(rElem,numLocks);
            for (i = 0; i < numLocks; i++)
            {
                RvMutexUnlock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr);
            }
            break;
        }
        case RvEmaLinkedLocks:
        {
            /* We must prepare the parent */
            EMAElement  parent = *((EMAElement*)((char*)elem + ema->elemSize));
            if (parent != NULL)
                numLocks = RvEmaPrepareForCallback(parent);
            else
                numLocks = RV_ERROR_UNKNOWN;

            if (numLocks < 0)
            {
                RvEmaLogWarning((&ema->log,
                    "RvEmaPrepareForCallback (%s): Couldn't prepare parent=%p of %p for some reason",
                         RvRaGetName(ema->ra), parent, elem));
            }
            break;
        }
        default:
            numLocks = 0;
            break;
    }

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log, "RvEmaPrepareForCallback (%s): on %p - locked %d times (%s:%d)",
             RvRaGetName(ema->ra), rElem, numLocks, filename, lineno));
#endif
    return numLocks;
}


/************************************************************************
 * RvEmaReturnFromCallback
 * purpose: Make sure the EMA element knows it has returned from a
 *          callback. This function will ensure that the element is
 *          locked again with the specified number of times. It will also
 *          release the element, and if timersLocked==0, and the element
 *          was deleted by the app in the callback, the element will also
 *          be permanently deleted.
 * input  : elem    - Element to prepare
 * output : none
 * return : RV_TRUE    if element still exists
 *          RV_FALSE   if element was deleted and is not valid anymore
 *          Negative value on failure
 ************************************************************************/
#if defined(RV_EMA_DEBUG) && (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI RvStatus RVCALLCONV
RvEmaReturnFromCallbackDebug(IN EMAElement elem, IN int timesLocked, IN const char* filename, IN int lineno)
#else
RVCOREAPI RvStatus RVCALLCONV
RvEmaReturnFromCallback(IN EMAElement elem, IN int timesLocked)
#endif
{
    emaObject*  ema;
    emaElem*    rElem;
    int         status = RV_ERROR_UNKNOWN;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Make sure we've got an element */
    if (ema == NULL) return RV_ERROR_UNKNOWN;

#if defined(RV_EMA_DEBUG)
    RvEmaLogDebug((&ema->log, "RvEmaReturnFromCallback (%s): on %p, %d times (%s:%d)",
             RvRaGetName(ema->ra), rElem, timesLocked, filename, lineno));
#endif

    /* See where the do we have to unlock */
    switch (ema->lockType)
    {
        case RvEmaNormalLocks:
        {
            /* We unlock the element */
            int i;

            /* First we should lock, and only then: increase the number of locks */
            for (i = 0; i < timesLocked; i++)
            {
                RvMutexLock((RvMutex *)((char*)elem + ema->elemSize), ema->logMgr);
            }
            EMA_INC_LOCKS(rElem,timesLocked);
            status = 0;
            break;
        }
        case RvEmaLinkedLocks:
        {
            /* We must prepare the parent */
            EMAElement  parent = *((EMAElement*)((char*)elem + ema->elemSize));
            if (parent != NULL)
                status = RvEmaReturnFromCallback(parent, timesLocked);
            else
                status = RV_ERROR_UNKNOWN;

            if (status < 0)
            {
                RvEmaLogWarning((&ema->log, "RvEmaReturnFromCallback (%s): Couldn't return parent=%p of %p for some reason",
                         RvRaGetName(ema->ra), parent, elem));
            }
            break;
        }
        default:
            break;
    }

    /* Decrease the reference count */
    EMA_DEC_REFERENCE(rElem,1);

    /* Delete element if we're done with it */
    if (rElem->flags == EMA_ALREADY_DELETED)
    {
        int location;
        RvRaGetByPtr(ema->ra, rElem, &location);
        emaDeleteElement(ema, rElem, location, RV_TRUE, "RvEmaReturnFromCallback");
    }
    else
    {
        /* We didn't delete this element... */
        status = RV_TRUE;
    }

    return status;
}


/************************************************************************
 * RvEmaSetApplicationHandle
 * purpose: Set the application handle of an element in EMA
 * input  : elem        - Element in EMA
 *          appHandle   - Application's handle for the element
 * output : none
 * return : Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaSetApplicationHandle(IN EMAElement elem, IN void* appHandle)
{
    emaElem* rElem;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    rElem->appHandle = appHandle;

#if defined(RV_EMA_DEBUG)
    {
        /* Make sure element is not vacant */
        emaObject* ema;
        int location;

        ema = rElem->ema;
        RvRaGetByPtr(ema->ra, rElem, &location);

        if (RvRaElemIsVacant(ema->ra, location))
        {
            RvEmaLogExcep((&ema->log, "RvEmaSetApplicationHandle (%s): Element %d,%p is vacant",
                     RvRaGetName(ema->ra), location, rElem));
        }
    }
#endif

    return RV_OK;
}


/************************************************************************
 * RvEmaGetApplicationHandle
 * purpose: Get the application's handle of an element in EMA
 * input  : elem        - Element in EMA
 * output : appHandle   - Application's handle for the element
 * return : Pointer to the application handle
 *          NULL on failure
 ************************************************************************/
RVCOREAPI void* RVCALLCONV
RvEmaGetApplicationHandle(IN EMAElement elem)
{
    emaElem* rElem;

    if (elem == NULL) return NULL;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);

#if defined(RV_EMA_DEBUG)
    {
        /* Make sure element is not vacant */
        emaObject* ema;
        int location;

        ema = rElem->ema;
        RvRaGetByPtr(ema->ra, rElem, &location);

        if (RvRaElemIsVacant(ema->ra, location))
        {
            RvEmaLogExcep((&ema->log, "RvEmaGetApplicationHandle (%s): Element %d,%p is vacant",
                     RvRaGetName(ema->ra), location, rElem));
        }
    }
#endif

    return rElem->appHandle;
}


/************************************************************************
 * RvEmaGetType
 * purpose: Return the type of the element inside the EMA object.
 *          This is the type given in RvEmaConstruct().
 * input  : elem    - Element in EMA
 * output : none
 * return : The element's type on success
 *          0 on failure
 ************************************************************************/
RVCOREAPI RvUint32 RVCALLCONV
RvEmaGetType(IN EMAElement elem)
{
    emaObject* ema;
    emaElem* rElem;

    if (elem == NULL) return 0;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    return ema->type;
}


/************************************************************************
 * RvEmaGetUserData
 * purpose: Return the user related data of the element inside the EMA
 *          object. This is the userData given in RvEmaConstruct().
 * input  : elem    - Element in EMA
 * output : none
 * return : The element's user data pointer on success
 *          NULL on failure
 ************************************************************************/
RVCOREAPI void* RVCALLCONV
RvEmaGetUserData(IN EMAElement elem)
{
    emaObject* ema;
    emaElem* rElem;

    if (elem == NULL) return NULL;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    return ema->userData;
}

/************************************************************************
 * emaGetUserDataByInstance
 * purpose: Return the user related data inside the EMA object, by the
 *          EMA instance returned by RvEmaConstruct().
 *          This is the userData given in RvEmaConstruct().
 * input  : emaH   - handle to the EMA
 * output : none
 * return : The user data pointer on success
 *          NULL on failure
 ************************************************************************/
void* emaGetUserDataByInstance(IN HEMA emaH)
{
    emaObject* ema = (emaObject *)emaH;

    if (emaH == NULL) return NULL;

    /* Find out our information */
    return ema->userData;
}

/************************************************************************
 * RvEmaGetInstance
 * purpose: Return the instance of this EMA element.
 *          This is the instance given in RvEmaConstruct().
 * input  : elem    - Element in EMA
 * output : none
 * return : The element's instance on success
 *          NULL on failure
 ************************************************************************/
RVCOREAPI void const* RVCALLCONV
RvEmaGetInstance(IN EMAElement elem)
{
    emaObject* ema;
    emaElem* rElem;

    if (elem == NULL) return NULL;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    if (ema)
        return ema->instance;
    else
        return NULL;
}


/************************************************************************
 * emaGetObject
 * purpose: Return the EMA object this element is in
 * input  : elem    - Element in EMA
 * output : none
 * return : The element's EMA object on success
 *          NULL on failure
 ************************************************************************/
HEMA emaGetObject(IN EMAElement elem)
{
    emaElem* rElem;

    if (elem == NULL) return NULL;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);

    return (HEMA)rElem->ema;
}


/************************************************************************
 * RvEmaDoAll
 * purpose: Call a function on all used elements in EMA
 * input  : emaH        - Handle of the EMA object
 *          func        - Function to execute on all elements
 *          param       - Context to use when executing the function
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaDoAll(
    IN HEMA     emaH,
    IN EMAFunc  func,
    IN void*    param)
{
    emaObject* ema = (emaObject *)emaH;
    int        i;

    /* Pass through all the elements, executing the functions on those
       which are used */
    for (i = 0; i < RvRaMaxSize(ema->ra); i++)
    if (!RvRaElemIsVacant(ema->ra, i))
    {
        char *elem  = (char *)RvRaGet(ema->ra, i);

        /* We change the context by the return value of the function */
        param = func(elem + SIZEOF_EMAELEM, param);
    }

    return RV_OK;
}


/************************************************************************
 * RvEmaGetNext
 * purpose: Get the next used element in EMA.
 *          This function can be used to implement search or "doall"
 *          functions on EMA.
 * input  : emaH        - Handle of the EMA object
 *          cur         - Current EMA element whose next we're looking for
 *                        If NULL, then RvEmaGetNext() will return the first
 *                        used element.
 * output : none
 * return : Pointer to the next used element on success
 *          NULL when no more used elements are left
 ************************************************************************/
RVCOREAPI EMAElement RVCALLCONV
RvEmaGetNext(
    IN HEMA         emaH,
    IN EMAElement   cur)
{
    emaObject* ema = (emaObject *)emaH;
    int location, curLoc;
    RvStatus status;

    /* Find out our element information */
    if (cur != NULL)
    {
        emaElem* rElem = (emaElem *)((char*)cur - SIZEOF_EMAELEM);
        RvRaGetByPtr(ema->ra, rElem, &curLoc);
    }
    else
        curLoc = RV_ERROR_UNKNOWN;

    /* Find the location */
    status = RvRaGetNext(ema->ra, curLoc, &location);

    if (status == RV_OK)
        return (EMAElement)((char*)RvRaGet(ema->ra, location) + SIZEOF_EMAELEM);
    else
        return NULL;
}


/************************************************************************
 * RvEmaGetIndex
 * purpose: Returns the index of the element in the ema
 * input  : elem    - Element in EMA
 * output : none
 * return : The element's index on success
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaGetIndex(IN EMAElement elem, OUT int* elemIndex)
{
    emaObject* ema;
    emaElem* rElem;

    if (elem == NULL) return RV_ERROR_UNKNOWN;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Find the location */
    return RvRaGetByPtr(ema->ra, rElem, elemIndex);
}


/************************************************************************
 * RvEmaGetByIndex
 * purpose: Returns the ema element by its index.
 *          This function does not check if the element is vacant or not.
 * input  : emaH    - Handle of the EMA object
 *          index   - Index of element searched
 * output : none
 * return : Pointer to the element on success
 *          NULL on failure
 ************************************************************************/
RVCOREAPI EMAElement RVCALLCONV
RvEmaGetByIndex(
    IN HEMA emaH,
    IN int  index)
{
    emaObject* ema;
    RvBool isVacant;
    char * pRA = NULL;

    ema = (emaObject *)emaH;

    if ((index < 0) || (index >= RvRaMaxSize(ema->ra)))
        return NULL;

    RvLockGet(&ema->lock, ema->logMgr);
    isVacant = RvRaElemIsVacant(ema->ra, index);
    if (!isVacant)
    pRA = (char*)RvRaGet(ema->ra, index);
    RvLockRelease(&ema->lock, ema->logMgr);
    if (pRA != NULL)
        return (EMAElement)(pRA + SIZEOF_EMAELEM);

    return NULL;
}


/************************************************************************
 * RvEmaGetStatistics
 * purpose: Get statistics information about EMA.
 *          The number of used items also includes those deleted, but still
 *          marked.
 * input  : emaH        - Handle of the EMA object
 * output : stats       - Statistics information
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvEmaGetStatistics(IN HEMA emaH, OUT RvEmaStatistics* stats)
{
    emaObject* ema = (emaObject *)emaH;
    if (ema == NULL) return RV_ERROR_UNKNOWN;

    stats->numMarked = ema->markCount;
    return RvRaGetStatistics(ema->ra, &stats->elems);
}

/************************************************************************
 * RvEmaIsVacant
 * purpose: Returns whether the given object is free for allocation
 * input  : elem    - Element in EMA
 * output : none
 * return : RV_TRUE  - if the elemnt is not allocated
 *          RV_FALSE - otherwise
 ************************************************************************/
RVCOREAPI RvBool RVCALLCONV
RvEmaIsVacant(IN EMAElement elem)
{
    emaObject* ema;
    emaElem* rElem;

    if (elem == NULL) return RV_FALSE;

    /* Find out our element information */
    rElem = (emaElem *)((char*)elem - SIZEOF_EMAELEM);
    ema = rElem->ema;

    /* Find the location */
    return (!ema);
}
