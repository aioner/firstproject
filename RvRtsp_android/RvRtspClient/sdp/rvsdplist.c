/******************************************************************************
Filename    :rvsdplist.c
Description :linked list & pool of objects routines.

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
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include <string.h>
#include "rvsdpprivate.h"

/* Implementation of casting from object to list node and vice versa */
#define OBJ_FROM_NODE(node) ((node) ? (void *)((char *)(node) - (l)->iNodeOffset) : 0)
#define NODE_FROM_OBJ(obj) ((obj) ? (RvListNode *)((char *)(obj) + (l)->iNodeOffset) : 0)

 /* initialize the linked list */
void  rvSdpListInitialize(
        RvSdpList* l,                   /* points to valid memory */
        RvUint32 offset,                /* an offset between 'next' pointer within the data structs
                                         * used as linked-list nodes and the beginning of the struct */
        rvListNodeDestructFunc destrF)  /* this function will be called when some node is removed from the list */
{
    l->iNodeOffset = offset;
    l->iHead = l->iTail = NULL;
    l->iListSize = 0;
    l->iDestroyF = destrF;
}

/* to remove all elements from the list */
void rvSdpListClear(RvSdpList* l)
{
    /* void *cur, *next; */
    RvListNode *cur;
    RvListNode *next;

    cur = l->iHead;
    while (cur)
    {
		next = cur->next;
        l->iListSize--;
        if (l->iDestroyF)
            l->iDestroyF(OBJ_FROM_NODE(cur));
        cur = next;
    }
    l->iHead = l->iTail = NULL; 
    l->iListSize = 0;
}

/* to copy 'src' list to 'dest'                     
 * returns RV_SDPSTATUS_OK on success or error status otherwise */
RvSdpStatus rvSdpListCopy(              
        RvSdpList* dest,    
        RvSdpList* src, 
        rvSdpListCopyFunc cf,       /* this function (with parameter 'usrData') will be called
                                     * for each copied list node */
        void* usrData)
{
    RvSdpListIter iter;
    void *dstNode, *srcNode;

    for (srcNode = rvSdpListGetFirst(src,&iter); srcNode; srcNode = rvSdpListGetNext(&iter))
    {
        dstNode = (cf)(NULL,srcNode,usrData);
        if (!dstNode)
        {
            rvSdpListClear(dest);
            return RV_SDPSTATUS_ALLOCFAIL;
        }
        rvSdpListTailAdd(dest,dstNode);
    }
    return RV_SDPSTATUS_OK;
}

/*
 *	Removes the node from the list. The destroy function is called if 'callDestroy'
 *  parameter is RV_TRUE.
 */
static 
void rvSdpListRemoveNode(RvSdpList *l, RvListNode *curNode, RvListNode *prevNode, 
						 RvBool callDestroy)
{    

    l->iListSize --;
    
    if (l->iListSize == 0)
        l->iHead = l->iTail = NULL;
    else
    {
        if (prevNode)
        {
            prevNode->next = curNode->next;
            if (curNode == l->iTail)
            {
                l->iTail = prevNode;
            }
        }
        else
        {
			 l->iHead = curNode->next;
        }
    }
    

    if (l->iDestroyF && callDestroy) 
    {
        void *curObj = OBJ_FROM_NODE(curNode);
        l->iDestroyF(curObj); 
    }
}  

void rvSdpListRemove(RvSdpList *l, void *curNode, void *prevNode) 
{
    rvSdpListRemoveNode(l, NODE_FROM_OBJ(curNode), NODE_FROM_OBJ(prevNode),RV_TRUE);
}

/* removes the node from the list by index (zero based) */
void rvSdpListRemoveByIndex(RvSdpList* l, RvSize_t index)
{    
    RvListNode *curNode;
    RvListNode *prevNode;

    curNode = l->iHead;
    prevNode = NULL;

    while (curNode && index)
    {
        prevNode = curNode;
		curNode = curNode->next;
        index --;
    }   
    
    if (!curNode)
        return;

    rvSdpListRemoveNode(l, curNode, prevNode, RV_TRUE);
}

/* removes the node pointed by 'p' *
   returns RV_TRUE if the node was found (and removed)
   and RV_FALSE otherwise */
RvBool  rvSdpListRemoveByValue(RvSdpList* l, void* p)
{
    RvListNode *curNode, *prevNode;
    RvListNode *pNode = NODE_FROM_OBJ(p);
    
    curNode = l->iHead;
    prevNode = 0;
    
    while (curNode && curNode != pNode)
    {
        prevNode = curNode;
		curNode = curNode->next;
    }   
    
    if (!curNode)
        return RV_FALSE;
    
    rvSdpListRemoveNode(l, curNode, prevNode, RV_TRUE);
    return RV_TRUE;
}


/* returns  the node by index (or NULL if there is node with the given index) */
void* rvSdpListGetByIndex(const RvSdpList* l, RvSize_t index)
{
    RvListNode *cur = l->iHead;

    while (cur && index)
    {
		cur = cur->next;
        index--;
    }   

    return OBJ_FROM_NODE(cur);
}

/* returns the first element of the list (or NULL if the list is empty) 
 * also sets the iterator object */
void* rvSdpListGetFirst(const RvSdpList* l, RvSdpListIter* iter)
{
    iter->iList = (RvSdpList*) l;
    iter->iCurrent = l->iHead;
    iter->iPrevious = NULL;
    return OBJ_FROM_NODE(iter->iCurrent);
}

/* returns the element currently pointed by iterator */
void* rvSdpListGetIterCurrent(RvSdpListIter* iter)
{
    RvSdpList *l = iter->iList;

    return OBJ_FROM_NODE(iter->iCurrent);
}

/* returns the next element (or NULL) based on iterator state */
void* rvSdpListGetNext(RvSdpListIter* iter)
{
    RvSdpList *l = iter->iList;
    
    iter->iPrevious = iter->iCurrent;
	iter->iCurrent = iter->iCurrent->next;
    return OBJ_FROM_NODE(iter->iCurrent);
}

/* removes from list element currently pointed by iterator */
void rvSdpListRemoveCurrent(RvSdpListIter* iter)
{
    rvSdpListRemoveNode(iter->iList,iter->iCurrent,iter->iPrevious, RV_TRUE);
}

/*
 *	Adds an element to the list end.
 */
static 
void rvSdpListTailAddNode(RvSdpList *l, RvListNode *node) {
    node->next = 0;
    if(l->iListSize == 0) {
        l->iHead = l->iTail = node;
    } else {
        l->iTail->next = node;
        l->iTail = node;
    }
    l->iListSize++;
}

/* appends the node 'p' at the end of the list */
void rvSdpListTailAdd(RvSdpList* l, void* p)
{
    rvSdpListTailAddNode(l, NODE_FROM_OBJ(p));
}

/* appends the 'node' at the beginning of the list */
static
void rvSdpListHeadAddNode(RvSdpList* l, RvListNode* node) {
    node->next = l->iHead;
    if(l->iListSize == 0) {
        l->iTail = node;
    }

    l->iHead = node;
    l->iListSize++;
}


/* appends the node 'p' at the beginning of the list */
void rvSdpListHeadAdd(RvSdpList* l, void* p)
{
    rvSdpListHeadAddNode(l, NODE_FROM_OBJ(p));
}

/* returns the first list element while also detaching it from the list
 * (or NULL if the list is empty) */
void* rvSdpListPullHead(RvSdpList* l)
{
    RvListNode *head;

    if (l->iListSize == 0)
        return NULL;

    head = l->iHead;
    l->iListSize --;
    l->iHead = head->next;

    if(l->iListSize == 0) {
        l->iTail = 0;
    }

    return OBJ_FROM_NODE(head);
}


/* returns the last list element while also detaching it from the list
 * (or NULL if the list is empty) */
void* rvSdpListPullTail(RvSdpList* l)
{
    RvListNode *curr = NULL, *prev = NULL;    
	;
    for (curr = l->iHead; curr != l->iTail; prev=curr,curr=curr->next)
        ;

    rvSdpListRemoveNode(l,curr,prev,RV_FALSE);
    return curr;
}


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
        rvListNodeDestructFunc destrF)      /* called for each unused pool object when the pool is destroyed */
{
    RvSize_t cnt;
    void *p;
    
    pool->iInitSz = initSz;
    pool->iIncrSz = incrSz;
    pool->iCrtFunc = crtF;
    pool->iCrtUsrData = crtUsrData;
    rvSdpListInitialize(&pool->iFreeObjects,poolObjOffset,destrF);

    for (cnt = 0; cnt < initSz; cnt ++)
    {
        p = (crtF)(crtUsrData);
        if (!p)
        {
            rvSdpPoolDestroy(pool);
            return RV_SDPSTATUS_ALLOCFAIL;
        }
        rvSdpListHeadAdd(&pool->iFreeObjects,p);
    }

    return RV_SDPSTATUS_OK;
}

/* to destroy the pool */
void rvSdpPoolDestroy(RvSdpObjPool* pool)
{
    rvSdpListClear(&pool->iFreeObjects);
}


/* to get the object from the pool */
void* rvSdpPoolTake(RvSdpObjPool* pool)
{
    void* p;

    if (pool->iFreeObjects.iListSize == 0)
    {
        RvSize_t cnt;
        for (cnt = 0; cnt < pool->iIncrSz; cnt++)
        {
            p = (pool->iCrtFunc)(pool->iCrtUsrData);
            if (!p)
                return NULL;
            rvSdpListHeadAdd(&pool->iFreeObjects,p);
        }        
    }

    return rvSdpListPullHead(&pool->iFreeObjects);
}

/* to return the object to the pool */
void rvSdpPoolReturn(RvSdpObjPool* pool, void* p)
{
    int diff;
    rvSdpListHeadAdd(&pool->iFreeObjects,p);
    diff = pool->iFreeObjects.iListSize - pool->iInitSz;
    if (diff >= (int)pool->iIncrSz)
    {
        while (diff)
        {
            diff--;
            rvSdpListRemoveByIndex(&pool->iFreeObjects,0);
        }
    }
}

