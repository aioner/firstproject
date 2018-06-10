/************************************************************************
 File Name     : rvrtpencryptiondata.c
 Description   :
*************************************************************************
 Copyright (c)  2002 , RADVision, Inc. All rights reserved.
*************************************************************************
 NOTICE:
 This document contains information that is proprietary to RADVISION Inc.
 No part of this publication may be reproduced in any form whatsoever
 without written prior approval by RADVISION Inc.

 RADVISION Inc. reserves the right to revise this publication and make
 changes without obligation to notify any person of such revisions or
 changes.
*************************************************************************
 $Revision: $
 $Date:   07/08/2002 $
 $Author: Scott K. Eaton $
************************************************************************/
#include "rvlog.h"
#include "rvrtpencryptiondata.h"

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
#include "rtputil.h" /* for logger only */
#define  rvLogPtr     rtpGetSource(RVRTP_SECURITY_MODULE)
#else
#define  rvLogPtr  (NULL)
#endif
/*******************************************************************************************
 * RvRtpEncryptionDataConstruct
 * description: this function constructs a RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: A pointer to the RvRtpEncryptionData object if successfull, NULL otherwise. 
 *******************************************************************************************/
RvRtpEncryptionData* RvRtpEncryptionDataConstruct(RvRtpEncryptionData* thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEncryptionDataConstruct"));
    thisPtr->isRtp = RV_FALSE;
    thisPtr->mode = RV_RTPENCRYPTIONMODE_RFC1889;
    thisPtr->remoteAddressPtr = NULL;
    thisPtr->rtpHeaderPtr = NULL;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEncryptionDataConstruct"));
    return thisPtr;
}
/*******************************************************************************************
 * RvRtpEncryptionDataConstructCopy
 * description: this function constructs a copy of a RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 *         srcPtr  - the RvRtpEncryptionData object to copy into this object.
 *     allcatorPtr - the allocator to use to construct the new object.
 * returns: A pointer to the RvRtpEncryptionData object if successfull, NULL otherwise. 
 *******************************************************************************************/
RvRtpEncryptionData* RvRtpEncryptionDataConstructCopy(RvRtpEncryptionData* thisPtr, const RvRtpEncryptionData* srcPtr, void* allcatorPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEncryptionDataConstructCopy"));	
    thisPtr->isRtp            = srcPtr->isRtp;
    thisPtr->mode             = srcPtr->mode;
    thisPtr->remoteAddressPtr = srcPtr->remoteAddressPtr;
    thisPtr->rtpHeaderPtr     = srcPtr->rtpHeaderPtr;
	
	RV_UNUSED_ARG(allcatorPtr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEncryptionDataConstructCopy"));
    return thisPtr;
}
/*******************************************************************************************
 * RvRtpEncryptionDataCopy
 * description: his function copies of a RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 *         srcPtr  - the RvRtpEncryptionData object to copy into this object.
 *     allcatorPtr - the allocator to use to construct the new object.
 * returns: none
 *******************************************************************************************/
void RvRtpEncryptionDataCopy(RvRtpEncryptionData* thisPtr,const RvRtpEncryptionData* srcPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEncryptionDataCopy"));
    thisPtr->isRtp            = srcPtr->isRtp;
    thisPtr->mode             = srcPtr->mode;
    thisPtr->remoteAddressPtr = srcPtr->remoteAddressPtr;
    thisPtr->rtpHeaderPtr     = srcPtr->rtpHeaderPtr;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEncryptionDataCopy"));
}

