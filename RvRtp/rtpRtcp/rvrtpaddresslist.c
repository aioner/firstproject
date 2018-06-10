/************************************************************************
 File Name     : rvrtpaddresslist.c
 Description   :
*************************************************************************
 Copyright (c)  2001 , RADVision, Inc. All rights reserved.
*************************************************************************
 NOTICE:
 This document contains information that is proprietary to RADVision Inc. 
 No part of this publication may be reproduced in any form whatsoever
 without written prior approval by RADVision Inc. 
 
 RADVision Inc. reserves the right to revise this publication and make
 changes without obligation to notify any person of such revisions or
 changes.
*************************************************************************/
#include "rvrtpaddresslist.h"
#include "rvmemory.h"
#include "rvrtpconfig.h"
#include "rtputil.h"
#include "rtp.h"



#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define RTP_SOURCE      (rtpGetSource(RVRTP_RTP_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_RTP_MODULE))
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"



typedef struct
{
    RvAddress        address;       /* Put first so we can cheat */
#ifdef __H323_NAT_FW__
    RvBool           isMultiplexed; /* TRUE, if remote address is multiplexed, FALSE - otherwise */
    RvUint32         multiplexID; /* multiplexerID,  if isMultiplexed is TRUE, otherwise - unused */    
#endif
    RvObjListElement elem;
#ifdef RV_RTP_DEST_POOL_OBJ
    RvObjPoolElement poolElem; /* source allocation pool */
#endif
} RvRtpAddressItem;

#if defined(RV_RTP_DEST_POOL_OBJ)
typedef struct 
{
    RvObjPool pool;
    RvMemory *region;

} RvRtpDestinationAddressPool;

RvRtpDestinationAddressPool destAddrPool;
/* pointer to destination Pool configuration */
extern RvRtpPoolConfig* poolConfig;

#endif

static RvRtpAddressItem *RvRtpAddressListAllocItem(void)
{
    RvRtpAddressItem *itemPtr;
#ifdef RV_RTP_DEST_POOL_OBJ 
    if ((itemPtr = (RvRtpAddressItem*)RvObjPoolGetItem(&destAddrPool.pool)) == NULL)
        return NULL;
#else
    if(RvMemoryAlloc(NULL, sizeof(RvRtpAddressItem), NULL, (void **)&itemPtr) != RV_OK)
        return NULL;
#endif
    RvAddressConstruct(RV_ADDRESS_TYPE_NONE, &itemPtr->address);
    return itemPtr;
}

static void RvRtpAddressListFreeItem(RvRtpAddressItem *itemPtr)
{
	RvAddressDestruct(&itemPtr->address);
#ifdef RV_RTP_DEST_POOL_OBJ 
    RvObjPoolReleaseItem(&destAddrPool.pool, itemPtr);
#else
	RvMemoryFree(itemPtr, NULL);
#endif
}

RvRtpAddressList *RvRtpAddressListConstruct(RvRtpAddressList *thisPtr)
{
	RvRtpAddressItem item;

	RvObjListConstruct(&item, &item.elem, (RvObjList *)thisPtr);
	return thisPtr;
}

void RvRtpAddressListDestruct(RvRtpAddressList *thisPtr)
{
	RvRtpAddressListRemoveAllAddresses(thisPtr);
	RvObjListDestruct((RvObjList *)thisPtr);
}

/* Prevent duplicates */
RvBool RvRtpAddressListAddAddress(IN RvRtpAddressList *thisPtr, 
                                  IN RvAddress *addressPtr, 
                                  IN RvUint32* pMultiplexID)
{
	RvBool succeded;
	RvRtpAddressItem *itemPtr;
	RvObjList *objlist;
    
#ifndef __H323_NAT_FW__   
    RV_UNUSED_ARG(pMultiplexID);
#endif
	/* Check for duplicates - uses slow method */
	objlist = (RvObjList *)thisPtr;
	succeded = RV_TRUE;
	itemPtr = (RvRtpAddressItem *)RvObjListGetNext(objlist, NULL, RV_OBJLIST_LEAVE);
	while(itemPtr != NULL) {
		if  (RvAddressCompare(&itemPtr->address, addressPtr, RV_ADDRESS_FULLADDRESS) == RV_TRUE 
#ifdef __H323_NAT_FW__
            && (((RvBool)(NULL!=pMultiplexID)) == (itemPtr->isMultiplexed)) &&
               (pMultiplexID==NULL || *pMultiplexID == itemPtr->multiplexID)
#endif            
               ) 
        {
			succeded = RV_FALSE;
			break;
		}
		itemPtr = (RvRtpAddressItem *)RvObjListGetNext(objlist, itemPtr, RV_OBJLIST_LEAVE);
	}
	if(succeded == RV_TRUE) {
		itemPtr = RvRtpAddressListAllocItem();
		if(itemPtr != NULL) 
        {
			RvAddressCopy(addressPtr, &itemPtr->address);
#ifdef __H323_NAT_FW__
            itemPtr->isMultiplexed = (pMultiplexID != NULL);
            itemPtr->multiplexID   = (itemPtr->isMultiplexed)? *pMultiplexID:0;
#endif
			RvObjListInsertBefore(objlist, NULL, itemPtr);
		} 
        else 
            succeded = RV_FALSE;
	}
	return succeded;
}

RvBool RvRtpAddressListRemoveAddress(IN RvRtpAddressList *thisPtr,
                                     IN RvAddress *address, 
                                     IN RvUint32* pMultiplexID)
{
	RvRtpAddressItem *itemPtr;
	RvObjList *objlist;

#ifndef __H323_NAT_FW__   
    RV_UNUSED_ARG(pMultiplexID);
#endif
	/* Find address - uses slow method */
	objlist = (RvObjList *)thisPtr;
	itemPtr = (RvRtpAddressItem *)RvObjListGetNext(objlist, NULL, RV_OBJLIST_LEAVE);
    while(itemPtr != NULL) 
    {
        if  (RvAddressCompare(&itemPtr->address, address, RV_ADDRESS_FULLADDRESS) == RV_TRUE 
#ifdef __H323_NAT_FW__            
            &&
            (((RvBool)(pMultiplexID != NULL)) == (itemPtr->isMultiplexed)) &&
            (pMultiplexID==NULL || *pMultiplexID == itemPtr->multiplexID)
#endif
            ) 
        {
            RvObjListRemoveItem(objlist, itemPtr);
            RvRtpAddressListFreeItem(itemPtr);
            break;
        }
        itemPtr = (RvRtpAddressItem *)RvObjListGetNext(objlist, itemPtr, RV_OBJLIST_LEAVE);
    }

	return RV_TRUE;
}

RvBool RvRtpAddressListRemoveAllAddresses(RvRtpAddressList *thisPtr)
{
	RvRtpAddressItem *itemPtr;
	RvObjList *objlist;

	/* Clear anything in list. */
	objlist = (RvObjList *)thisPtr;
	itemPtr = (RvRtpAddressItem *)RvObjListGetNext(objlist, NULL, RV_OBJLIST_REMOVE);
	while(itemPtr != NULL) {
		RvRtpAddressListFreeItem(itemPtr);
		itemPtr = (RvRtpAddressItem *)RvObjListGetNext(objlist, NULL, RV_OBJLIST_REMOVE);
	}

	return RV_TRUE;
}

/* global to get access from SRTP */
RVAPI
RvAddress*   RVCALLCONV  RvRtpAddressListGetNext(RvRtpAddressList *thisPtr, RvAddress *curitem)
{
	RvObjList *objlist;
	RvRtpAddressItem *item;
	RvRtpAddressItem *nextitem;
	

	objlist = (RvObjList *)thisPtr;
	item = (RvRtpAddressItem *)curitem; /* The cheat */
	nextitem = (RvRtpAddressItem *)RvObjListGetNext(objlist, item, RV_OBJLIST_LEAVE);

	return &nextitem->address;
}

#ifdef RV_RTP_DEST_POOL_OBJ

static void *rtpDestinationAddressPageAlloc (RvSize_t size, void *data)
{
    void *res;
    RvRtpDestinationAddressPool *pool = (RvRtpDestinationAddressPool *)data;   
    RvStatus rc;
    
    rc = RvMemoryAlloc (pool->region, size, logMgr, &res);
    if (RV_OK != rc)
        return (NULL);
    
    return (res);
} 

static void rtpDestinationAddressPageFree (void *ptr, void *data)
{
    RV_UNUSED_ARG(data);
    RvMemoryFree (ptr, logMgr);
} 


RvStatus RvRtpDestinationAddressPoolConstruct(void)
{
    RvObjPool     *  tmpPool = NULL;
    RvObjPoolFuncs   addressCB;
    RvRtpAddressItem tempDestPool;
    
    RTPLOG_ENTER(DestinationAddressPoolConstruct);

    destAddrPool.region        = NULL; /* default region */
    addressCB.objconstruct     = NULL;
    addressCB.objdestruct      = NULL;
    addressCB.objconstructdata = &destAddrPool;
    addressCB.objdestructdata  = &destAddrPool;
    addressCB.pagealloc        = rtpDestinationAddressPageAlloc;
    addressCB.pagefree         = rtpDestinationAddressPageFree;
    addressCB.pageallocdata    = &destAddrPool;
    addressCB.pagefreedata     = &destAddrPool;

    tmpPool = RvObjPoolConstruct (
        &tempDestPool,
        &(tempDestPool.poolElem), 
        &addressCB, 
        sizeof(RvRtpAddressItem)/*itemSize*/, /*?*/
        poolConfig->pageItems,
        poolConfig->pageSize, 
        poolConfig->poolType, 
        ((RV_OBJPOOL_TYPE_DYNAMIC == poolConfig->poolType) ? RV_TRUE : RV_FALSE),
        poolConfig->maxItems, 
        poolConfig->minItems, 
        poolConfig->freeLevel, 
        &(destAddrPool.pool));
    
    if (tmpPool == NULL)
    {
        RTPLOG_ERROR_LEAVE(DestinationAddressPoolConstruct, "cannot construct the pool");
        return RV_ERROR_UNKNOWN;
    }
    RTPLOG_LEAVE(DestinationAddressPoolConstruct);
    return RV_OK;
}

RvStatus RvRtpDestinationAddressPoolDestruct(void)
{   
    RTPLOG_ENTER(DestinationAddressPoolDestruct);
    RvObjPoolDestruct (&(destAddrPool.pool));
    RTPLOG_LEAVE(DestinationAddressPoolDestruct);
    return RV_OK;
}

#endif
