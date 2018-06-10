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
#include "copybits.h"
#include "ra.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif



#if defined(RV_RA_SUPPORT_WORKING_SET)

/* Maximum number of elements that RA will use for its working set */
#define RA_MAX_WORKING_SET_SIZE (50)

#endif  /* defined(RV_RA_SUPPORT_WORKING_SET) */



#define BIT_VECTOR(ra)      ((RvUint8 *)(ra) + RvRoundToSize(sizeof(raHeader), RV_ALIGN_SIZE))

#define BIT_VECTOR_SIZE(n)   ( RvRoundToSize((n + 7) / 8 , RV_ALIGN_SIZE) )

/* Convert an element's pointer into its index in the array */
#define ELEM_INDEX(ra, ptr)   ( ((int)((char *)ptr - ra->arrayLocation)) / ra->sizeofElement )


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
                              raAdd() function. */
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
static int
raGetAllocationSize(int elemSize, int maxNumOfElements)
{
    int roundSize = RvRoundToSize(sizeof(vacantNode), RV_ALIGN_SIZE);

    return
        (int)RvRoundToSize(sizeof(raHeader), RV_ALIGN_SIZE) + 
        BIT_VECTOR_SIZE(maxNumOfElements) +
        (maxNumOfElements * RvMax(roundSize, elemSize));
}


/************************************************************************
 * raBuild
 * purpose: Build the actual RA from a given allocated memory block
 * input  : buffer              - Buffer of allocated RA memory block
 *          elemSize            - Size of elements in the RA in bytes
 *          maxNumOfElements    - Number of elements in RA
 *          threadSafe          - RV_TRUE to make raAdd,raDelete thread-safe
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

//    ra->watchdog = NULL;

    raClear((HRA)ra);   /* init vacant list and pointers */

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
static int RVCALLCONV RaWatchdogResourceCallback(
    IN  void*                   context,
    IN  RvUint32                resource,
    IN  RvWatchdogResourceType  type,
    OUT RvUint32*               value)
{
    raHeader* ra = (raHeader *)context;

    RV_UNUSED_ARG(resource);

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
 * raConstruct
 * purpose: Create an RA object.
 * input  : elemSize            - Size of elements in the RA in bytes.
 *          maxNumOfElements    - Number of elements in RA.
 *          threadSafe          - RV_TRUE to make raAdd,raDelete thread-safe.
 *          name                - Name of RA (used in log messages).
 *          logMgr              - Log manager used.
 * output : none
 * return : Handle to RA constructed on success.
 *          NULL on failure.
 ************************************************************************/
RVINTAPI HRA RVCALLCONV
raConstruct(
    IN int              elemSize,
    IN int              maxNumOfElements,
    IN RvBool           threadSafe,
    IN const RvChar*    name,
    IN RvLogMgr*        logMgr)
{
    raHeader*   ra;
    int         size;
    void*       ptr=NULL;

    /* Make sure the size is at least 4 bytes per element and aligned on 32bits */
    size = (int)RvRoundToSize(RvMax(elemSize, (int)sizeof(vacantNode)), RV_ALIGN_SIZE);

    /* Allocate the amount of memory necessary */
    if(RvMemoryAlloc(NULL, (RvSize_t)raGetAllocationSize(size, maxNumOfElements), logMgr, (void**)&ptr) != RV_OK)
        return NULL;

    /* Build the RA header and elements properly */
    ra = raBuild((char *)ptr, size, maxNumOfElements, threadSafe, name);
    if (ra == NULL)
    {
        /* Error building this RA */
        RvMemoryFree(ptr, logMgr);
        return NULL;
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
        (ra->pLog, "raConstruct (%s): %d elements of size %d (total of %d)", name, maxNumOfElements, size, maxNumOfElements*size));
#endif

    return (HRA)ra;
}


/************************************************************************
* raDestruct
* purpose: Free an RA object, deallocating all of its used memory
* input  : raH     - Handle of the RA object
* output : none
* return : none
************************************************************************/
RVINTAPI void RVCALLCONV
raDestruct(HRA raH)
{
    if (raH != NULL)
    {
        raHeader* ra = (raHeader*) raH;

        /* Remove watchdog resource if we have to */
//        if (ra->watchdog != NULL)
//            RvWatchdogDeleteResource(ra->watchdog, ra->watchdogResource);

        if (ra->threadSafe)
            RvLockDestruct(&ra->lock, ra->logMgr);

        if (ra->pLog != NULL)
            RvLogSourceDestruct(ra->pLog);
        RvMemoryFree(ra, ra->logMgr);
    }
}


/************************************************************************
 * raClear
 * purpose: Clean an RA object from any used elements, bringing it back
 *          to the point it was when raConstruct() was called.
 * input  : raH     - Handle of the RA object
 * output : none
 * return : none
 ************************************************************************/
RVINTAPI void RVCALLCONV
raClear(HRA raH)
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
 * raSetCompareFunc
 * purpose: Set the compare function to use in raFind()
 * input  : raH     - Handle of the RA object
 *          func    - Compare function to use
 * output : none
 * return : none
 ************************************************************************/
RVINTAPI void RVCALLCONV
raSetCompareFunc(IN HRA raH, IN RAECompare func)
{
    raHeader *ra = (raHeader *)raH;
    ra->compare = func;
}


/************************************************************************
 * raSetPrintFunc
 * purpose: Set the print function to use in raPrint()
 * input  : raH     - Handle of the RA object
 *          func    - Print function to use
 * output : none
 * return : none
 ************************************************************************/
RVINTAPI void RVCALLCONV
raSetPrintFunc(IN HRA raH, IN RAEFunc func)
{
    raHeader *ra = (raHeader *)raH;
    ra->print = func;
}


/************************************************************************
 * raGetPrintFunc
 * purpose: Set the print function to use in raPrint()
 * input  : raH     - Handle of the RA object
 * output : none
 * return : Print function used by RA (given by raSetPrintFunc)
 ************************************************************************/
RAEFunc raGetPrintFunc(IN HRA raH)
{
    return ((raHeader *)raH)->print;
}


/************************************************************************
 * raAdd
 * purpose: Allocate an element in RA for use, without initializing its
 *          value.
 * input  : raH         - Handle of the RA object
 * output : pOutElem    - Pointer to the element added.
 *                        If given as NULL, it will not be set
 * return : Negative value on failure
 *          Non-negative value representing the location of the added
 *          element.
 ************************************************************************/
RVINTAPI int RVCALLCONV
raAdd(IN HRA raH, OUT RAElement *pOutElem)
{
    raHeader *ra = (raHeader *)raH;
    RAElement allocatedElement;
    int vLocation;

    if (ra->threadSafe)
        RvLockGet(&ra->lock, ra->logMgr);

    /* See if there's any place in this RA */
    if (ra->firstVacantElement == NULL)
    {
        RvLogError(ra->pLog,
            (ra->pLog, "raAdd (%s): Array full (%d elements)", ra->name, ra->maxNumOfElements));
        RvPrintf("raAdd (%s): Array full (%d elements)\n", ra->name, ra->maxNumOfElements);
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
    vLocation = ((vacantNode *)allocatedElement)->nodeIndex;
    setBitTrue(BIT_VECTOR(ra), vLocation); /* make it occupied */

    /* Set statistical information */
    ra->curNumOfElements++;
    if (ra->curNumOfElements > ra->maxUsage)
        ra->maxUsage=ra->curNumOfElements;

    RvLogInfo(ra->pLog,
        (ra->pLog, "raAdd (%s): %d current elements, Added %p, location %d", ra->name, ra->curNumOfElements, allocatedElement, vLocation));

    if (ra->threadSafe)
        RvLockRelease(&ra->lock, ra->logMgr);

    /* Return the location */
    if (pOutElem != NULL)
        *pOutElem = allocatedElement;
    return vLocation;
}


/************************************************************************
 * raDelete
 * purpose: Delete an element from RA
 * input  : raH      - Handle of the RA object
 * output : delElem  - Element in RA to delete
 * return : Negative value on failure
 ************************************************************************/
RVINTAPI int RVCALLCONV
raDelete(IN HRA raH, IN RAElement delElem)
{
    raHeader*   ra = (raHeader *)raH;
    int         location;

    if (delElem == NULL) return RV_ERROR_UNKNOWN;

    location = raGetByPtr(raH, (void*)delElem);
    if (location < 0 || location > ra->maxNumOfElements) return RV_ERROR_UNKNOWN;

    if (ra->threadSafe)
        RvLockGet(&ra->lock, ra->logMgr);

    if (raElemIsVacant(raH, location) == RV_TRUE)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "raDelete (%s): %d already deleted or was never used", ra->name, location));
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

    setBitFalse(BIT_VECTOR(ra), location); /* make it free */

    RvLogInfo(ra->pLog,
        (ra->pLog, "raDelete (%s): %d current elements, Deleted %p, location %d", ra->name, ra->curNumOfElements, delElem, location));

    if (ra->threadSafe)
        RvLockRelease(&ra->lock, ra->logMgr);

    return RV_OK;
}


/************************************************************************
 * raDeleteLocation
 * purpose: Delete an element from RA by its location
 * input  : raH      - Handle of the RA object
 * output : location - Location of the element in RA
 * return : Negative value on failure
 ************************************************************************/
RVINTAPI int RVCALLCONV
raDeleteLocation(IN HRA raH, IN int location)
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
            (ra->pLog, "raDeleteLocation (%s): Bad location %d [0-%d]", ra->name, location, ra->maxNumOfElements));
        return RV_ERROR_UNKNOWN;
    }

    if (raElemIsVacant(raH, location) == RV_TRUE)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "raDeleteLocation (%s): Element %d already vacant", ra->name, location));
        return RV_ERROR_UNKNOWN;
    }
#endif /* RV_RA_DEBUG */
#endif /* 0 */

    if (location < 0 || location > ra->maxNumOfElements) return RV_ERROR_UNKNOWN;
    if ((raElemIsVacant(raH, location)) == RV_TRUE) return RV_ERROR_UNKNOWN;

    deletedElement = (RAElement)RV_RA_ELEM_DATA(ra, location);

    return raDelete(raH, deletedElement);
}


#ifdef RV_RA_DEBUG
/************************************************************************
 * raGet
 * purpose: Get the pointer to an RA element by its location
 * input  : raH      - Handle of the RA object
 * output : location - Location of the element in RA
 * return : Pointer to the RA element
 *          In release mode, no checks are done for the validity or the
 *          vacancy of the location.
 ************************************************************************/
RVINTAPI RAElement RVCALLCONV
raGet(IN HRA raH, IN int location)
{
    raHeader* ra = (raHeader *)raH;

    if (ra == NULL) return NULL;

    if ((location < 0) || (unsigned)location >= (unsigned)(ra->maxNumOfElements))
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "raGet (%s): Bad location %d [0-%d]", ra->name, location, ra->maxNumOfElements));
        return NULL;
    }

    if (raElemIsVacant(raH, location) == RV_TRUE)
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "raGet (%s): Element %d is vacant", ra->name, location));
        return NULL;
    }

    return (RAElement)RV_RA_ELEM_DATA(raH, location);
}
#endif  /* RV_RA_DEBUG */


/************************************************************************
 * raGetByPtr
 * purpose: Get the location of an RA element by its element pointer
 * input  : raH     - Handle of the RA object
 * output : ptr     - Pointer to the RA element's value
 * return : Location of the element on success
 *          Negative value on failure
 ************************************************************************/
RVINTAPI int RVCALLCONV
raGetByPtr(IN HRA raH, IN void *ptr)
{
    raHeader *ra = (raHeader *)raH;
    int location = -1;
    int position;

#ifdef RV_RA_DEBUG
    /* Make sure the given pointer is a valid element */
    if (!ra) return RV_ERROR_UNKNOWN;

    if (((char *)ptr < (char *)(ra->arrayLocation)) ||
        ((char *)ptr > (char *)(ra->arrayLocation) + ra->maxNumOfElements*(int)(ra->sizeofElement)))
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "raGetByPtr (%s): Pointer %p out of bounds [%p-%p]",
                 ra->name, ptr, ra->arrayLocation,
                 (char *)(ra->arrayLocation) + ra->maxNumOfElements*(ra->sizeofElement)));
        return RV_ERROR_UNKNOWN;
    }
#endif

    /* Calculate the location of the element */
    location = (int)((RvUint8 *)ptr - ra->arrayLocation);

#ifdef RV_RA_DEBUG
    /* Make sure the pointer is aligned properly */
    if (location % (int)ra->sizeofElement != 0)
    {   /* alignment */
        RvLogExcep(ra->pLog,
            (ra->pLog, "raGetByPtr (%s): Pointer %p not aligned", ra->name, ptr));

        return RV_ERROR_UNKNOWN;
    }
#endif

    position = location/(int)(ra->sizeofElement);

#ifdef RV_RA_DEBUG
    /* Make sure element is not vacant */
    if (raElemIsVacant(raH, position))
    {
        RvLogExcep(ra->pLog,
            (ra->pLog, "raGetByPtr (%s): Element %d id vacant", ra->name, position));

        return RV_ERROR_UNKNOWN;
    }
#endif

    return position;
}



/************************************************************************
 * raElemIsVacant
 * purpose: Check if an element is vacant inside RA or not
 * input  : raH         - Handle of the RA object
 *          location    - Location of RA element to check
 * output : none
 * return : RV_TRUE if element is vacant
 *          RV_FALSE if element is used
 *          Negative value on failure
 ************************************************************************/
int raElemIsVacant(
    IN HRA raH,
    IN int location)
{
    raHeader *ra = (raHeader *)raH;

    if (!ra || location<0 || location>ra->maxNumOfElements)
        return RV_ERROR_UNKNOWN;
    return ((getBit((BIT_VECTOR(ra)), (location))) != 0) ? (RV_FALSE):(RV_TRUE);
}



/************************************************************************
 * raGetName
 * purpose: Get the name of the RA object
 * input  : raH         - Handle of the RA object
 * output : none
 * return : Name of the RA
 ************************************************************************/
const char* raGetName(IN HRA raH)
{
    raHeader *ra = (raHeader *)raH;

    return (const char *)(ra->name);
}



/************************************************************************
 * raGetNext
 * purpose: Get the next used element in RA.
 *          This function can be used to implement search or "doall"
 *          functions on RA.
 * input  : raH - Handle of the RA object
 *          cur - Current RA location whose next we're looking for
 *                If negative, then raGetNext() will return the first
 *                used element.
 * output : none
 * return : Location of the next used element on success
 *          Negative value when no more used elements are left
 ************************************************************************/
RVINTAPI int RVCALLCONV
raGetNext(
    IN HRA  raH,
    IN int  cur)
{
    raHeader* ra = (raHeader *)raH;
    int i;
    if (cur < 0)
        i = 0;
    else
        i = cur + 1;
    if (i >= ra->maxNumOfElements) return RV_ERROR_UNKNOWN; /* out of bounds */

    while ((i < ra->maxNumOfElements) && raElemIsVacant(raH, i))
        i++;

    if (i < ra->maxNumOfElements)
        return i;
    else
        return RV_ERROR_UNKNOWN;
}

#if 0
/******************************************************************************
 * raAddWatchdogResource
 * ----------------------------------------------------------------------------
 * General: Add an RA instance as a resource handled by the watchdog.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  raH          - RA instance handle.
 *         watchdog     - The watchdog instance to add this RA to.
 *         name         - Name of resource to use.
 *         group        - Group name for this resource.
 *         description  - Short description about this resource.
 *****************************************************************************/
RVINTAPI RvStatus RVCALLCONV
raAddWatchdogResource(
    IN HRA          raH,
    IN RvWatchdog   *watchdog,
    IN const char   *name,
    IN const char   *group,
    IN const char   *description)
{
    RvStatus status;
    raHeader* ra = (raHeader *)raH;

    /* Make sure we're not trying this more than once */
    if (ra->watchdog != NULL)
        return RV_ERROR_UNKNOWN;

    status = RvWatchdogAddResource(watchdog, name, group, description,
        RaWatchdogResourceCallback, (void *)raH, &ra->watchdogResource);
    if (status >= 0)
        ra->watchdog = watchdog; /* Got it */

    return status;
}

#endif

/************************************************************************
 * raGetStatistics
 * purpose: Get statistics information about RA.
 * input  : raH         - Handle of the RA object
 * output : stats       - Statistics information
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVINTAPI int RVCALLCONV
raGetStatistics(IN HRA raH, OUT RvRaStatistics* stats)
{
    raHeader* ra = (raHeader *)raH;
    if (ra == NULL) return RV_ERROR_UNKNOWN;

    stats->cur      = (RvUint32)ra->curNumOfElements;
    stats->maxUsage = (RvUint32)ra->maxUsage;
    stats->max      = (RvUint32)ra->maxNumOfElements;

    return 0;
}





/* Desc: returns current number of elements.
 */
RVINTAPI int RVCALLCONV
raCurSize(HRA raH)
{
  raHeader *ra = (raHeader *)raH;
  if (!raH) return RV_ERROR_UNKNOWN;
  return ra->curNumOfElements;
}


/* Desc: returns number of elements that can be added to array.
 */
RVINTAPI int RVCALLCONV
raFreeSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->maxNumOfElements -
         ((raHeader *)raH)->curNumOfElements;
}

/* Desc: returns maximum number of elements.
 */
RVINTAPI int RVCALLCONV
raMaxSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->maxNumOfElements;
}




/* Maximum usage of array */
RVINTAPI int RVCALLCONV
raMaxUsage(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->maxUsage;
}





/* returns size of element */
RVINTAPI int RVCALLCONV
raElemSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->sizeofElement;
}


/* returns size of element (without RA internal alignment) */
RVINTAPI int RVCALLCONV
raElemRequestedSize(HRA raH)
{
  if (!raH) return RV_ERROR_UNKNOWN;
  return ((raHeader *)raH)->requestedSizeofElement;
}


RAECompare
raFCompare(HRA raH)
{
  if (!raH) return NULL;
  return ((raHeader *)raH)->compare;
}





/* Desc: find element by key.
   Returns: location of element or RV_ERROR_UNKNOWN.
   */
RVINTAPI int RVCALLCONV
raFind(HRA raH, void *param)
{
  raHeader *ra = (raHeader *)raH;
  int i;

  if (!ra) return RV_ERROR_UNKNOWN;

  for (i=0; i<ra->maxNumOfElements; i++)
    if (!raElemIsVacant(raH, i)) {
      if ((ra->compare && ra->compare((RAElement)RV_RA_ELEM_DATA(ra, i), param)) ||
          RV_RA_ELEM_DATA(ra, i) == (char *)param) /* address comparison if no compare fuction */
        return i;
    }
  return RV_ERROR_UNKNOWN;
}


int
raCompare(HRA raH, RAECompare compare, void *param)
{
  raHeader *ra = (raHeader *)raH;
  int i;

  /*if (!ra) return RV_ERROR_UNKNOWN;*/

  for (i=0; i<ra->maxNumOfElements; i++)
    if (!raElemIsVacant(raH, i)) {
      if ((compare && compare((RAElement)RV_RA_ELEM_DATA(ra, i), param)) ||
          RV_RA_ELEM_DATA(ra, i) == (char *)param) /* address comparison if no compare fuction */
        return i;
    }
  return RV_ERROR_UNKNOWN;
}




#ifdef __cplusplus
}
#endif



