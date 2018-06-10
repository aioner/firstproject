/******************************************************************************
Filename    : rvsdplist.h
Description : functions regarding use of SDP library linked list and pool of objects

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD..
  
    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
 Author:Rafi Kiel
******************************************************************************/

#ifndef _file_rvsdplist_h_
#define _file_rvsdplist_h_

#ifdef __cplusplus
extern "C" {
#endif
    
/* typedef void* RvListNode; */

typedef struct _RvListNode RvListNode;

struct _RvListNode {
    RvListNode *next;   
};

/* this function will be called when some elements are deleted from the LList */
typedef void (*rvListNodeDestructFunc)(void* node);
/* this function is called for each LListNode when LList instance is copied to other
   LList instance*/
typedef void* (*rvSdpListCopyFunc)(void* dest, void* src, void* usrData);

typedef struct 
{    
    RvUint32                iNodeOffset; /* the offest of the 'iNext' (typed void*) struct member
                                            from the beginning of the struct */
    RvUint32                iListSize;   /* the current size (nodes number) of the list */
    RvListNode              *iHead;       /* points to the first node */
    RvListNode              *iTail;       /* points to the last node */
    rvListNodeDestructFunc  iDestroyF;   /* will be called each time the node is removed from the list */   
} RvSdpList;

/* list iterator */
typedef struct 
{
    RvSdpList       *iList;     /* the list being iterated */
    RvListNode      *iCurrent;   /* the current node */
    RvListNode      *iPrevious;  /* the previous node */
} RvSdpListIter;


/* to be called by objects pool to allocate more objects */
typedef void* (*rvCreatePoolObjFunc)(void* usrData);

/* Pool of objects */
typedef struct 
{
    RvSdpList           iFreeObjects;  /* the list of free objects currently contained in the pool */
    RvSize_t            iInitSz;       /* initial size of objects pool, this number             
                                        * of objects will be allocated by the pool during pool initialize */
    RvSize_t            iIncrSz;       /* this number of objects will be allocated when there iFreeObjects 
                                        * gets empty */
    rvCreatePoolObjFunc iCrtFunc;      /* callback called when pool needs to allocate more objects */
    void*               iCrtUsrData;   /* will be given as an argument when calling iCrtFunc */
} RvSdpObjPool;


#include "rvtypes.h"
#include "rvsdpenums.h"

 /* initialize the linked list */
void  rvSdpListInitialize(
        RvSdpList* l,                   /* points to valid memory */
        RvUint32 offset,                /* an offset between 'next' pointer within the data structs
                                         * used as linked-list nodes and the beginning of the struct */
        rvListNodeDestructFunc destrF); /* this function will be called when some node is removed from the list */

/* to remove all elements from the list */
RVSDPCOREAPI void rvSdpListClear(       
        RvSdpList* l);          

/* to copy 'src' list to 'dest'                     
 * returns RV_SDPSTATUS_OK on success or error status otherwise */
RvSdpStatus rvSdpListCopy(              
        RvSdpList* dest,    
        RvSdpList* src, 
        rvSdpListCopyFunc cf,       /* this function (with parameter 'usrData') will be called
                                     * for each copied list node */
        void* usrData);

/* removes the node from the list by index (zero based) */
RVSDPCOREAPI void  rvSdpListRemoveByIndex(
        RvSdpList* l, 
        RvSize_t index);

/* removes the node pointed by 'p' *
   returns RV_TRUE if the node was found (and removed)
   and RV_FALSE otherwise */
RvBool rvSdpListRemoveByValue(
        RvSdpList* l, 
        void* p);

/* returns  the node by index (or NULL if there is node with the given index) */
RVSDPCOREAPI void* rvSdpListGetByIndex(
        const RvSdpList* l, 
        RvSize_t index);

/* appends the node 'p' at the end of the list */
void rvSdpListTailAdd(
        RvSdpList* l, 
        void* p);

/* appends the node 'p' at the beginning of the list */
void rvSdpListHeadAdd(
        RvSdpList* l, 
        void* p);

/* returns the first list element while also detaching it from the list
 * (or NULL if the list is empty) */
void* rvSdpListPullHead(
        RvSdpList* l);

/* returns the last list element while also detaching it from the list
 * (or NULL if the list is empty) */
void* rvSdpListPullTail(
        RvSdpList* l);

/* returns the first element of the list (or NULL if the list is empty) 
 * also sets the iterator object */
RVSDPCOREAPI void* rvSdpListGetFirst(
        const RvSdpList* l, 
        RvSdpListIter* iter);

/* returns the next element (or NULL) based on iterator state */
RVSDPCOREAPI void* rvSdpListGetNext(
        RvSdpListIter* iter);

/* returns the element currently pointed by iterator */
void* rvSdpListGetIterCurrent(
        RvSdpListIter* iter);

/* removes from list element currently pointed by iterator */
RVSDPCOREAPI void rvSdpListRemoveCurrent(
        RvSdpListIter* iter);


/* to initialize the pool */
/* returns RV_SDPSTATUS_OK on success or error otherwise */
RvSdpStatus rvSdpPoolInitialize(
        RvSdpObjPool* pool,                 /* must point to the valid pool object */
        RvSize_t poolObjOffset,             /* an offset of 'next' data-member within the 
                                             * structures to be kept in pool*/
        RvSize_t initSz,                    /* the initial size of pool (this number of pool objects
                                             * will be allocated when pool is initialized */
        RvSize_t incrSz,                    /* increase step */
        rvCreatePoolObjFunc crtF,           /* called to allocate one pool object (with 'crtUsrData' as an argument */
        void* crtUsrData, 
        rvListNodeDestructFunc destrF);     /* called for each unused pool object when the pool is destroyed */

/* to destroy the pool */
void rvSdpPoolDestroy(
        RvSdpObjPool* pool);

/* to get the object from the pool */
void* rvSdpPoolTake(
        RvSdpObjPool* pool);

/* to return the object to the pool */
void rvSdpPoolReturn(
        RvSdpObjPool* pool, 
        void* p);


#ifdef __cplusplus
}
#endif
#endif

