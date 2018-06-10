/************************************************************************
 File Name	   : rvrtpaddresslist.h
 Description   :
*************************************************************************
 Copyright (c)	2001 , RADVision, Inc. All rights reserved.
*************************************************************************
 NOTICE:
 This document contains information that is proprietary to RADVision Inc. 
 No part of this publication may be reproduced in any form whatsoever
 without written prior approval by RADVision Inc. 
 
 RADVision Inc. reserves the right to revise this publication and make
 changes without obligation to notify any person of such revisions or
 changes.
*************************************************************************/
#if !defined(RVRTPADDRESSLIST_H)
#define RVRTPADDRESSLIST_H

#include "rvtypes.h"
#include "rvaddress.h"
#include "rvobjlist.h"
#include "rverror.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************
*	RvRtpAddressList private module
******************************************************************/
typedef struct 
{
    RvAddress        address;       /* Put first so we can cheat */
#ifdef __H323_NAT_FW__
    RvBool           isMultiplexed; /* TRUE, if remote address is multiplexed, FALSE - otherwise */
    RvUint32         multiplexID; /* multiplexerID,  if isMultiplexed is TRUE, otherwise - unused */   
#endif
} RtpNatAddress;
    
typedef RvObjList RvRtpAddressList;

RvRtpAddressList *RvRtpAddressListConstruct(RvRtpAddressList *thisPtr);
void			  RvRtpAddressListDestruct(RvRtpAddressList *thisPtr);
RvBool RvRtpAddressListAddAddress(IN RvRtpAddressList *thisPtr, 
                                  IN RvAddress *addressPtr, 
                                  IN RvUint32* pMultiplexID);
RvBool RvRtpAddressListRemoveAddress(IN RvRtpAddressList *thisPtr,
                                     IN RvAddress *address, 
                                     IN RvUint32* pMultiplexID);
RvBool	 RvRtpAddressListRemoveAllAddresses(RvRtpAddressList *thisPtr);

RVAPI
RvAddress*   RVCALLCONV  RvRtpAddressListGetNext(RvRtpAddressList *thisPtr, RvAddress *curitem);


#if defined(RV_RTP_DEST_POOL_OBJ)
RvStatus RvRtpDestinationAddressPoolConstruct(void);
RvStatus RvRtpDestinationAddressPoolDestruct(void);
#endif

#ifdef __cplusplus
}
#endif

#endif


