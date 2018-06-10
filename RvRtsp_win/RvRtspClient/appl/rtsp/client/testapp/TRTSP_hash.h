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

#ifndef _RV_DMTR_APP_HASH_H_
#define _RV_DMTR_APP_HASH_H_

/***********************************************************************
AppHash.h

General hash table support.
This is used to hold the different objects of the test application.
***********************************************************************/


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "TRTSP_general.h"
#include "rvmemory.h"



#ifdef __cplusplus
extern "C" {
#endif

    
/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/


#define HASH_SIZE (1024)


/* Hash bucket object */
typedef struct
{
    void            *next; /* Pointer to next free object / element in the bucket */
    RvInt32         id; /* Id of the element (used as the key) */
} HashBucketObj;


/* Counters object */
struct AppHashObj_tag
{
    HashBucketObj       *element; /* Array of elements used */
    HashBucketObj       *hash[HASH_SIZE]; /* Hash table holding pointers to counter buckets */

    RvSize_t             elemSize; /* Size of each element (including the bucket header */
    RvSize_t             maxElems; /* Maximum number of elements pre-allocated */
    HashBucketObj       *firstFree; /* First free object */
    HashBucketObj       *lastFree; /* Last free object */
};


/*-----------------------------------------------------------------------*/
/*                           FUNCTIONS HEADERS                           */
/*-----------------------------------------------------------------------*/


/******************************************************************************
 * AppHashInit
 * ----------------------------------------------------------------------------
 * General: Initialize a hash object.
 *
 * Return Value: Hash object handle on success, NULL on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  elemSize - Size of elements to store in hash.
 * Output: None.
 *****************************************************************************/
AppHashObj * AppHashInit(
    IN RvSize_t         elemSize);


/******************************************************************************
 * AppHashEnd
 * ----------------------------------------------------------------------------
 * General: Destruct a hash object.
 *
 * Return Value: rvOk on success, other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hashH    - Hash object handle to use.
 * Output: None.
 *****************************************************************************/
RvStatus AppHashEnd(
    IN AppHashObj *       hashH);


/******************************************************************************
 * AppHashAdd
 * ----------------------------------------------------------------------------
 * General: Add a new element to the hash.
 *          Only elements with new id's can be added.
 *
 * Return Value: RV_OK on success, other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hashH    - Hash object handle to use.
 *         id       - Id to use for hash element.
 * Output: elem     - Element allocated.
 *****************************************************************************/
RvStatus AppHashAdd(
    IN  AppHashObj *    hashH,
    IN  RvInt32             id,
    OUT void **             elem);


/******************************************************************************
 * AppHashRemove
 * ----------------------------------------------------------------------------
 * General: Remove an element from the hash.
 *
 * Return Value: RV_OK on success, other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hashH    - Hash object handle to use.
 *         id       - Id to use for hash element.
 * Output: None.
 *****************************************************************************/
RvStatus AppHashRemove(
    IN  AppHashObj *    hashH,
    IN  RvInt32             id);


/******************************************************************************
 * AppHashGet
 * ----------------------------------------------------------------------------
 * General: Get an element from the hash.
 *
 * Return Value: The element found. NULL if not found.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hashH    - Hash object handle to use.
 *         id       - Id to use for hash element.
 * Output: None.
 *****************************************************************************/
void *AppHashGet(
    IN  AppHashObj *    hashH,
    IN  RvInt32         id);


/******************************************************************************
 * AppHashGetAny
 * ----------------------------------------------------------------------------
 * General: Get an element that was allocated in the hash.
 *          This function is SLOW and runs in o(n).
 *
 * Return Value: The element found. NULL if not found.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  hashH    - Hash object handle to use.
 *         key      - Starting point of search. Pointer to NULL to begin from
 *                    the first element.
 * Output: key      - Starting point for the next search.
 *****************************************************************************/
void *AppHashGetAny(
    IN      AppHashObj *  hashH,
    INOUT   void        **key);




#ifdef __cplusplus
}
#endif

#endif /* _RV_DMTR_APP_HASH_H_ */
