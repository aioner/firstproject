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


/*
  ra.c

  Fixed size array implementation.


  Format:

  +----------+------------+----------------+
  |  header  | bit vector | array of data  |
  +----------+------------+----------------+
  (raHeader)               (sizeofElement*maxNodes)


  Vacant element search is done in O(1) by linking all vacant nodes
  (link is inside the data).
  no need for vacant and data pointer (saving a lot of space).

  Note: element size >= sizeof(vacantNode).
  Bit vector: bit i=0 iff element i is free.

  */

#include "rvstdio.h"
#include "rvmemory.h"
#include "rvra.h"
#include <string.h>


#if defined(RV_RA_SUPPORT_WORKING_SET)

/* Maximum number of elements that RA will use for its working set */
#define RA_MAX_WORKING_SET_SIZE (50)

#endif  /* defined(RV_RA_SUPPORT_WORKING_SET) */



#define BIT_VECTOR(ra)      ((RvUint8 *)(ra) + RvRoundToSize(sizeof(raHeader), RV_ALIGN_SIZE))

#define BIT_VECTOR_SIZE(n)   ( RvRoundToSize((n + 7) / 8 , RV_ALIGN_SIZE) )


/* This structure holds the linked-list of vacant elements within the array.
 * It is implemented inside the elements themselves, since when they're not used,
 * their content can be used for that purpose instead.
 */
typedef struct vacantNode_tag vacantNode;
struct vacantNode_tag
{
    void*       unused; /* This unused pointer is here to protect the first parameter people
                           will put in structs inside RA even when the element is vacant.
                           It is used by EMA. */
#if defined(RV_RA_SUPPORT_WORKING_SET)
    vacantNode* prev; /* Pointer to the previous vacant element. NULL if this one is the first */
#endif
    vacantNode* next; /* Pointer to the next vacant element. NULL if this one is the last */
    int         nodeIndex; /* Current node's index. This is used for faster calculation of the
                              RvRaAdd() function. */
};



/************************************************************************
 *
 *                           Private functions
 *
 ************************************************************************/


/************************************************************************
 * raGetAllocationSize
 * purpose: Return the allocation size of an RA object
 * input  : elemSize            - Size of elements in the RA in bytes
 *          maxNumOfElements    - Number of elements in RA
 * output : none
 * return : Allocation size
 ************************************************************************/
static RvSize_t
raGetAllocationSize(int elemSize, int maxNumOfElements)
{
    int roundSize = RvRoundToSize(sizeof(vacantNode), RV_ALIGN_SIZE);

    return
        RvRoundToSize(sizeof(raHeader), RV_ALIGN_SIZE) + 
        BIT_VECTOR_SIZE(maxNumOfElements) +
        (maxNumOfElements * RvMax(roundSize, elemSize));
}


/************************************************************************
 * raBuild
 * purpose: Build the actual RA from a given allocated memory block
 * input  : buffer              - Buffer of allocated RA memory block
 *          elemSize            - Size of elements in the RA in bytes
 *          maxNumOfElements    - Number of elements in RA
 *          threadSafe          - RV_TRUE to make RvRaAdd,RvRaDelete thread-safe
 *          name                - Name of RA (used in log messages)
 * output : none
 * return : Pointer to the modified header
 ************************************************************************/
static raHeader *
raBuild(
    IN char*            buffer,
    IN int              elemSize,
    IN int              maxNumOfElements,
    IN RvBool           threadSafe,
    IN const RvChar*    name)
{
    raHeader *ra;

    /* Clear the buffer */
    memset(buffer, 0, (RvSize_t)raGetAllocationSize(elemSize, maxNumOfElements));

    /* Set the header information */
    ra = (raHeader *)buffer;

    if (threadSafe)
    {
        if (RvLockConstruct(ra->logMgr, &ra->lock) != RV_OK)
            return NULL;
    }
    ra->threadSafe = threadSafe;

    ra->maxNumOfElements = maxNumOfElements;
    ra->arrayLocation = (RvUint8*)ra + BIT_VECTOR_SIZE(maxNumOfElements) + RvRoundToSize(sizeof(raHeader), RV_ALIGN_SIZE);
    ra->curNumOfElements = 0;
    ra->sizeofElement    = (RvSize_t)elemSize;
    ra->maxUsage         = 0;
    strncpy(ra->name, name, sizeof(ra->name));
    ra->name[sizeof(ra->name)-1] = '\0';

#if defined(RV_RA_SUPPORT_WORKING_SET)
    /* Caculate a default size for our working-set */
    if ( maxNumOfElements > (RA_MAX_WORKING_SET_SIZE * 2) )
        ra->workingSetSize = RA_MAX_WORKING_SET_SIZE;
    else
        ra->workingSetSize = maxNumOfElements / 2;
#endif  /* defined(RV_RA_SUPPORT_WORKING_SET) */

    RvRaClear((HRA)ra);   /* init vacant list and pointers */

    return ra;
}

#if 0
/******************************************************************************
 * RaWatchdogResourceCallback
 * ----------------------------------------------------------------------------
 * General: Watchdog callback for accessing an RA instance.
 *          This function is an accessory function to make the handling of
 *          RA resources easier for the watchdog.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  context      - RA instance handle.
 *         resource     - The requested resource's enumeration.
 *         type         - The type of information requested.
 * Output: value        - The actual resource value.
 *****************************************************************************/
static RvStatus RVCALLCONV raGetInfo(
    IN  HRA                     raH,
    IN  RvWatchdogResourceType  type,
    OUT RvUint32*               value)
{
    raHeader* ra = (raHeader *)raH;

    if (ra == NULL)
        return RV_ERROR_UNKNOWN;

    switch (type)
    {
    case RvWatchdogMaxVal:
        *value = (RvUint32)ra->maxNumOfElements;
        break;
    case RvWatchdogMaxUsage:
        *value = (RvUint32)ra->maxUsage;
        break;
    case RvWatchdogCurrent:
        *value = (RvUint32)ra->curNumOfElements;
        break;
    default:
        return RV_ERROR_UNKNOWN;
    }

    return RV_OK;
}
#endif






/************************************************************************
*
*                           Public functions
*
************************************************************************/


/************************************************************************
 * RvRaConstruct
 * purpose: Create an RA object.
 * input  : elemSize            - Size of elements in the RA in bytes.
 *          maxNumOfElements    - Number of elements in RA.
 *          threadSafe          - RV_TRUE to make RvRaAdd,RvRaDelete thread-safe.
 *          name                - Name of RA (used in log messages).
 *          logMgr              - Log manager used.
 * output : none
 * return : Handle to RA constructed on success.
 *          NULL on failure.
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaConstruct(
    IN int              elemSize,
    IN int              maxNumOfElements,
    IN RvBool           threadSafe,
    IN const RvChar*    name,
    IN RvLogMgr*        logMgr,
    OUT HRA*            hRa)
{
    raHeader*   ra;
    int         size;
    void*       ptr=NULL;

    /* Make sure the size is at least 4 bytes per element and aligned on 32bits */
    size = (int)RvRoundToSize(RvMax(elemSize, (int)sizeof(vacantNode)), RV_ALIGN_SIZE);

    /* Allocate the amount of memory necessary */
    if(RvMemoryAlloc(NULL, (RvSize_t)raGetAllocationSize(size, maxNumOfElements), logMgr, (void**)&ptr) != RV_OK)
        return RV_ERROR_UNKNOWN;

    /* Build the RA header and elements properly */
    ra = raBuild((char *)ptr, size, maxNumOfElements, threadSafe, name);
    if (ra == NULL)
    {
        /* Error building this RA */
        RvMemoryFree(ptr, logMgr);
        return RV_ERROR_UNKNOWN;
    }

    ra->requestedSizeofElement = (RvSize_t)elemSize;
    ra->logMgr = logMgr;

    if (logMgr != NULL)
    {
        if (RvLogSourceConstruct(ra->logMgr, &ra->log, "RA", "R Array") == RV_OK)
            ra->pLog = &ra->log;
    }

#ifdef RV_RA_DEBUG
    RvLogDebug(ra->pLog,
        (ra->pLog, "RvRaConstruct (%s): %d elements of size %d (total of %d)", name, maxNumOfElements, size, maxNumOfElements*size));
#endif

    *hRa = (HRA)ra;

    return RV_OK;
}


/************************************************************************
* RvRaDestruct
* purpose: Free an RA object, deallocating all of its used memory
* input  : raH     - Handle of the RA object
* output : none
* return : none
************************************************************************/
RVCOREAPI void RVCALLCONV
RvRaDestruct(HRA raH)
{
    if (raH != NULL)
    {
        raHeader* ra = (raHeader*) raH;

        if (ra->threadSafe)
            RvLockDestruct(&ra->lock, ra->logMgr);

        if (ra->pLog != NULL)
            RvLogSourceDestruct(ra->pLog);
        RvMemoryFree(ra, ra->logMgr);
    }
}


/************************************************************************
 * RvRaClear
 * purpose: Clean an RA object from any used elements, bringing it back
 *          to the point it was when RvRaConstruct() was called.
 * input  : raH     - Handle of the RA object
 * output : none
 * return : none
 ************************************************************************/
RVCOREAPI void RVCALLCONV
RvRaClear(HRA raH)
{
    raHeader*   ra = (raHeader *)raH;
    vacantNode* lastNode;
    vacantNode* curNode;
    vacantNode* prevNode;
    char*       nextNode;
    int i;

    if (ra == NULL)
        return;

    if (ra->threadSafe)
        RvLockGet(&ra->lock, ra->logMgr);

    ra->curNumOfElements = 0;
    ra->maxUsage = 0;

    curNode = (vacantNode *)RV_RA_ELEM_DATA(ra, 0);
    lastNode = (vacantNode *)RV_RA_ELEM_DATA(ra, ra->maxNumOfElements-1);

    /* Set the first vacant element pointers to first elements in array */
    if (ra->maxNumOfElements > 0)
        ra->firstVacantElement = (RAElement)curNode;
    else
        ra->firstVacantElement = NULL;

#if defined(RV_RA_SUPPORT_WORKING_SET)
    ra->workingSetDistance = ra->workingSetSize;

    /* Set the working set element by the size of the working set we're using */
    ra->workingSetElement = (RAElement)RV_RA_ELEM_DATA(ra, ra->workingSetSize);
#else
    ra->lastVacantElement = (RAElement)lastNode;
#endif


    /* Set each element to point at the next element as the next free element */
    prevNode = NULL;
    for (i = 0; i < ra->maxNumOfElements; i++)
    {
        nextNode = ((char *)curNode) + ra->sizeofElement;
#if defined(RV_RA_SUPPORT_WORKING_SET)
        curNode->prev = prevNode;
#endif
        curNode->next = (vacantNode *)nextNode;
        curNode->nodeIndex = i;

        prevNode = curNode;
        curNode = (vacantNode *)nextNode;
    }

    if (ra->maxNumOfElements > 0)
    {
        /* Make sure last element points to NULL */
        lastNode->next = NULL;

        /* Make sure the bits vector shows all elements as free */
        memset(BIT_VECTOR(ra), 0, BIT_VECTOR_SIZE(ra->maxNumOfElements));
    }

    if (ra->threadSafe)
        RvLockRelease(&ra->lock, ra->logMgr);
}


/************************************************************************
 * RvRaSetCompareFunc
 * purpose: Set the compare function to use in RvRaFind()
 * input  : raH     - Handle of the RA object
 *          func    - Compare function to use
 * output : none
 * return : none
 ************************************************************************/
RVCOREAPI void RVCALLCONV
RvRaSetCompareFunc(IN HRA raH, IN RAECompare func)
{
    raHeader *ra = (raHeader *)raH;
    ra->compare = func;
}


/************************************************************************
 * RvRaSetPrintFunc
 * purpose: Set the print function to use in raPrint()
 * input  : raH     - Handle of the RA object
 *          func    - Print function to use
 * output : none
 * return : none
 ************************************************************************/
RVCOREAPI void RVCALLCONV
RvRaSetPrintFunc(IN HRA raH, IN RAEFunc func)
{
    raHeader *ra = (raHeader *)raH;
    ra->print = func;
}


/************************************************************************
 * RvRaGetPrintFunc
 * purpose: Set the print function to use in raPrint()
 * input  : raH     - Handle of the RA object
 * output : none
 * return : Print function used by RA (given by RvRaSetPrintFunc)
 ************************************************************************/
RAEFunc RvRaGetPrintFunc(IN HRA raH)
{
    return ((raHeader *)raH)->print;
}


/************************************************************************
 * RvRaAdd
 * purpose: Allocate an element in RA for use, without initializing its
 *          value.
 * input  : raH         - Handle of the RA object
 * output : pOutElem    - Pointer to the element added.
 *                        If given as NULL, it will not be set
 * return : Negative value on failure
 *          Non-negative value representing the location of the added
 *          element.
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaAdd(IN HRA raH, OUT RAElement *pOutElem, int* vLocation)
{
    raHeader *ra = (raHeader *)raH;
    RAElement allocatedElement;

    if (ra->threadSafe)
        RvLockGet(&ra->lock, ra->logMgr);

    /* See if there's any place in this RA */
    if (ra->firstVacantElement == NULL)
    {
        RvLogError(ra->pLog,
            (ra->pLog, "RvRaAdd (%s): Array full (%d elements)", ra->name, ra->maxNumOfElements));
        /*RvPrintf("RvRaAdd (%s): Array full (%d elements)\n", ra->name, ra->maxNumOfElements);*/
        if (pOutElem != NULL) *pOutElem = NULL;
        if (ra->threadSafe)
            RvLockRelease(&ra->lock, ra->logMgr);
        return RV_ERROR_UNKNOWN;
    }

    allocatedElement = ra->firstVacantElement;

    /* Get the element from list of vacant elements and fix that list */
    ra->firstVacantElement = (RAElement)((vacantNode *)allocatedElement)->next;

#if defined(RV_RA_SUPPORT_WORKING_SET)
    if (((vacantNode *)ra->workingSetElement)->next != NULL)
        ra->workingSetElement = (RAElement)(((vacantNode *)ra->workingSetElement)->next);
    else
        ra->workingSetDistance--;

    /* Let's see what we have to do with our free list */
    if (ra->firstVacantElement == NULL)
    {
        /* No more free elements - update the working set information */
        ra->workingSetElement = NULL;
        ra->workingSetDistance = 0;
    }
    else
    {
        /* Since we chopped off the head of the free list - we should set the new head
           of the free list as the first element: i.e - prev=NULL */
        ((vacantNode *)ra->firstVacantElement)->prev = NULL;
    }
#else
    if (ra->firstVacantElement == NULL)
        ra->lastVacantElement = NULL;
#endif

    /* Get the index of this element and set in the bit vector */
    *vLocation = ((vacantNode *)allocatedElement)->nodeIndex;
    setBitTrue(BIT_VECTOR(raH), *vLocation); /* make it occupied */

    /* Set statistical information */
    ra->curNumOfElements++;
    if (ra->curNumOfElements > ra->maxUsage)
        ra->maxUsage=ra->curNumOfElements;

    RvLogInfo(ra->pLog,
        (ra->pLog, "RvRaAdd (%s): %d current elements, Added %p, location %d", ra->name, ra->curNumOfElements, allocatedElement, *vLocation));

    if (ra->threadSafe)
        RvLockRelease(&ra->lock, ra->logMgr);

    /* Return the location */
    if (pOutElem != NULL)
        *pOutElem = allocatedElement;

    return RV_OK;
}


/************************************************************************
 * RvRaDelete
 * purpose: Delete an element from RA
 * input  : raH      - Handle of the RA object
 * output : delElem  - Element in RA to delete
 * return : Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaDelete(IN HRA raH, IN RAElement delElem)
{
    raHeader*   ra = (raHeader *)raH;
    RvStatus    status;
    int         location;

    if (delElem == NULL) return RV_ERROR_UNKNOWN;

    status = RvRaGetByPtr(raH, (void*)delElem, &location);
    if (status != RV_OK || location > ra->maxNumOfElements) return RV_ERROR_UNKNOWN;

    if (ra->threadSafe)
        RvLockGet(&ra->lock, ra->logMgr);

    if (RvRaElemIsVacant(raH, location) == RV_TRUE)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaDelete (%s): %d already deleted or was never used", ra->name, location));
        if (ra->threadSafe)
            RvLockRelease(&ra->lock, ra->logMgr);
        return RV_ERROR_UNINITIALIZED;
    }

    /* Add the element to the vacant list */
    ra->curNumOfElements--;

#if defined(RV_RA_SUPPORT_WORKING_SET)
    if (ra->firstVacantElement == NULL)
    {
        /* We don't have a free list -> this element is the only vacant one */
        ra->firstVacantElement = delElem;
    }

    if (ra->workingSetElement != NULL)
    {
        vacantNode* prev = ((vacantNode *)(ra->workingSetElement))->prev;

        if (prev != NULL)
            prev->next = (vacantNode *)delElem;
        else
            ra->firstVacantElement = delElem;
        ((vacantNode *)delElem)->prev = prev;
        ((vacantNode *)(ra->workingSetElement))->prev = (vacantNode *)delElem;
        ((vacantNode *)delElem)->next = (vacantNode *)(ra->workingSetElement);
        if (ra->workingSetDistance == ra->workingSetSize)
            ra->workingSetElement = delElem;
        else
            ra->workingSetDistance++;
    }
    else
    {
        ((vacantNode *)delElem)->prev = NULL;
        ((vacantNode *)delElem)->next = NULL;
        ra->workingSetElement = delElem;
        ra->workingSetDistance = 0;
    }
#else
    ((vacantNode *)delElem)->next = NULL;
    ((vacantNode *)delElem)->nodeIndex = location;
    if (ra->lastVacantElement != NULL)
        ((vacantNode *)ra->lastVacantElement)->next = (vacantNode *)delElem;
    else
    {
        /* ra->lastVacantElement==NULL means that ra->firstVacantElement is also
           equal to NULL, so after this delete location just only one element
           will be in this RA, in such situation both ra->firstVacantElement &
           ra->lastVacantElement should point to the deleted element
        */
        ra->firstVacantElement = delElem;
    }
    ra->lastVacantElement = delElem;
#endif

    ((vacantNode*)delElem)->nodeIndex = location;

    setBitFalse(BIT_VECTOR(raH), location); /* make it free */

    RvLogInfo(ra->pLog,
        (ra->pLog, "RvRaDelete (%s): %d current elements, Deleted %p, location %d", ra->name, ra->curNumOfElements, delElem, location));

    if (ra->threadSafe)
        RvLockRelease(&ra->lock, ra->logMgr);

    return RV_OK;
}


/************************************************************************
 * RvRaDeleteLocation
 * purpose: Delete an element from RA by its location
 * input  : raH      - Handle of the RA object
 * output : location - Location of the element in RA
 * return : Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaDeleteLocation(IN HRA raH, IN int location)
{
    raHeader*   ra = (raHeader *)raH;
    RAElement   deletedElement;

#if 0
#ifdef RV_RA_DEBUG
    /* todo: if we're going to change rtGet() to accept only valid parameters */
    /* Some validity checks for debug mode */
    if (ra == NULL) return RV_ERROR_UNKNOWN;
    if (location < 0 || location > ra->maxNumOfElements)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaDeleteLocation (%s): Bad location %d [0-%d]", ra->name, location, ra->maxNumOfElements));
        return RV_ERROR_UNKNOWN;
    }

    if (RvRaElemIsVacant(raH, location) == RV_TRUE)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaDeleteLocation (%s): Element %d already vacant", ra->name, location));
        return RV_ERROR_UNKNOWN;
    }
#endif /* RV_RA_DEBUG */
#endif /* 0 */

    if (location < 0 || location > ra->maxNumOfElements) return RV_ERROR_UNKNOWN;
    if (RvRaElemIsVacant(raH, location) == RV_TRUE) return RV_ERROR_UNKNOWN;

    deletedElement = (RAElement)RV_RA_ELEM_DATA(ra, location);

    return RvRaDelete(raH, deletedElement);
}


#ifdef RV_RA_DEBUG
/************************************************************************
 * RvRaGet
 * purpose: Get the pointer to an RA element by its location
 * input  : raH      - Handle of the RA object
 * output : location - Location of the element in RA
 * return : Pointer to the RA element
 *          In release mode, no checks are done for the validity or the
 *          vacancy of the location.
 ************************************************************************/
RVCOREAPI RAElement RVCALLCONV
RvRaGet(IN HRA raH, IN int location)
{
    raHeader* ra = (raHeader *)raH;

    if (ra == NULL) return NULL;

    if ((location < 0) || (unsigned)location >= (unsigned)(ra->maxNumOfElements))
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaGet (%s): Bad location %d [0-%d]", ra->name, location, ra->maxNumOfElements));
        return NULL;
    }

    if (RvRaElemIsVacant(raH, location) == RV_TRUE)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaGet (%s): Element %d is vacant", ra->name, location));
        return NULL;
    }

    return (RAElement)RV_RA_ELEM_DATA(raH, location);
}
#endif  /* RV_RA_DEBUG */


/************************************************************************
 * RvRaGetByPtr
 * purpose: Get the location of an RA element by its element pointer
 * input  : raH     - Handle of the RA object
 * output : ptr     - Pointer to the RA element's value
 * return : Location of the element on success
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaGetByPtr(IN HRA raH, IN const void *ptr, OUT int* position)
{
    raHeader *ra = (raHeader *)raH;
    int location;

#ifdef RV_RA_DEBUG
    /* Make sure the given pointer is a valid element */
    if (!ra) return RV_ERROR_UNKNOWN;

    if (((char *)ptr < (char *)(ra->arrayLocation)) ||
        ((char *)ptr > (char *)(ra->arrayLocation) + ra->maxNumOfElements*(ra->sizeofElement)))
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaGetByPtr (%s): Pointer %p out of bounds [%p-%p]",
                 ra->name, ptr, ra->arrayLocation,
                 (char *)(ra->arrayLocation) + ra->maxNumOfElements*(ra->sizeofElement)));
        return RV_ERROR_UNKNOWN;
    }
#endif

    /* Calculate the location of the element */
    location = (int)((RvUint8 *)ptr - ra->arrayLocation);

#ifdef RV_RA_DEBUG
    /* Make sure the pointer is aligned properly */
    if (location % ra->sizeofElement != 0)
    {   /* alignment */
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaGetByPtr (%s): Pointer %p not aligned", ra->name, ptr));

        return RV_ERROR_UNKNOWN;
    }
#endif

    *position = location/(int)(ra->sizeofElement);

#ifdef RV_RA_DEBUG
    /* Make sure element is not vacant */
    if (RvRaElemIsVacant(raH, *position))
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "RvRaGetByPtr (%s): Element %d id vacant", ra->name, *position));

        return RV_ERROR_UNKNOWN;
    }
#endif

    return RV_OK;
}



/************************************************************************
 * RvRaElemIsVacant
 * purpose: Check if an element is vacant inside RA or not
 * input  : raH         - Handle of the RA object
 *          location    - Location of RA element to check
 * output : none
 * return : RV_TRUE if element is vacant
 *          RV_FALSE if element is used
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI int RVCALLCONV
RvRaElemIsVacant(IN HRA raH, IN int location)
{
    raHeader *ra = (raHeader *)raH;

    if (!ra || location<0 || location>ra->maxNumOfElements)
        return RV_ERROR_UNKNOWN;
    return ((getBit((BIT_VECTOR(raH)), (location))) != 0) ? (RV_FALSE):(RV_TRUE);
}



/************************************************************************
 * RvRaGetName
 * purpose: Get the name of the RA object
 * input  : raH         - Handle of the RA object
 * output : none
 * return : Name of the RA
 ************************************************************************/
RVCOREAPI const char* RVCALLCONV
RvRaGetName(IN HRA raH)
{
    raHeader *ra = (raHeader *)raH;

    return (const char *)(ra->name);
}



/************************************************************************
 * RvRaGetNext
 * purpose: Get the next used element in RA.
 *          This function can be used to implement search or "doall"
 *          functions on RA.
 * input  : raH - Handle of the RA object
 *          cur - Current RA location whose next we're looking for
 *                If negative, then RvRaGetNext() will return the first
 *                used element.
 * output : none
 * return : Location of the next used element on success
 *          Negative value when no more used elements are left
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaGetNext(IN HRA raH, IN int cur, OUT int* next)
{
    raHeader* ra = (raHeader *)raH;
    int i;
    if (cur < 0)
        i = 0;
    else
        i = cur + 1;
    if (i >= ra->maxNumOfElements) return RV_ERROR_UNKNOWN; /* out of bounds */

    while ((i < ra->maxNumOfElements) && RvRaElemIsVacant(raH, i))
        i++;

    if (i >= ra->maxNumOfElements)
        return RV_ERROR_UNKNOWN;

    *next = i;

    return RV_OK;
}



/************************************************************************
 * RvRaGetStatistics
 * purpose: Get statistics information about RA.
 * input  : raH         - Handle of the RA object
 * output : stats       - Statistics information
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVCOREAPI RvStatus RVCALLCONV
RvRaGetStatistics(IN HRA raH, OUT RvRaStatistics* stats)
{
    raHeader* ra = (raHeader *)raH;
    if (ra == NULL) return RV_ERROR_UNKNOWN;

    stats->cur      = (RvUint32)ra->curNumOfElements;
    stats->maxUsage = (RvUint32)ra->maxUsage;
    stats->max      = (RvUint32)ra->maxNumOfElements;

    return RV_OK;
}



/* Desc: returns current number of elements.
 */
RVCOREAPI int RVCALLCONV
RvRaCurSize(HRA raH)
{
  raHeader *ra = (raHeader *)raH;
  if (!raH) return RV_ERROR_UNKNOWN;
  return ra->curNumOfElements;
}


/* Desc: returns number of elements that can be added to array.
 */
RVCOREAPI int RVCALLCONV
RvRaFreeSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->maxNumOfElements -
         ((raHeader *)raH)->curNumOfElements;
}

/* Desc: returns maximum number of elements.
 */
RVCOREAPI int RVCALLCONV
RvRaMaxSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->maxNumOfElements;
}




/* Maximum usage of array */
RVCOREAPI int RVCALLCONV
RvRaMaxUsage(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->maxUsage;
}





/* returns size of element */
RVCOREAPI int RVCALLCONV
RvRaElemSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return (int)((raHeader *)raH)->sizeofElement;
}


/* returns size of element (without RA internal alignment) */
RVCOREAPI int RVCALLCONV
RvRaElemRequestedSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return (int)((raHeader *)raH)->requestedSizeofElement;
}


RVCOREAPI RAECompare RVCALLCONV
RvRaFCompare(HRA raH)
{
  if (!raH) return NULL;
  return ((raHeader *)raH)->compare;
}





/* Desc: find element by key.
   Returns: location of element or RV_ERROR_UNKNOWN.
   */
RVCOREAPI int RVCALLCONV
RvRaFind(HRA raH, void *param)
{
  raHeader *ra = (raHeader *)raH;
  int i;

  if (!ra) return RV_ERROR_UNKNOWN;

  for (i=0; i<ra->maxNumOfElements; i++)
    if (!RvRaElemIsVacant(raH, i)) {
      if ((ra->compare && ra->compare((RAElement)RV_RA_ELEM_DATA(ra, i), param)) ||
      RV_RA_ELEM_DATA(ra, i) == (char *)param) /* address comparison if no compare fuction */
    return i;
    }
  return RV_ERROR_UNKNOWN;
}


RVCOREAPI int RVCALLCONV
RvRaCompare(HRA raH, RAECompare compare, void *param)
{
  raHeader *ra = (raHeader *)raH;
  int i;

  /*if (!ra) return RV_ERROR_UNKNOWN;*/

  for (i=0; i<ra->maxNumOfElements; i++)
    if (!RvRaElemIsVacant(raH, i)) {
      if ((compare && compare((RAElement)RV_RA_ELEM_DATA(ra, i), param)) ||
      RV_RA_ELEM_DATA(ra, i) == (char *)param) /* address comparison if no compare fuction */
    return i;
    }
  return RV_ERROR_UNKNOWN;
}
