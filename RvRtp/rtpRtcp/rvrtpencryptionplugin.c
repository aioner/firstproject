/************************************************************************
 File Name     : rvrtpencryptionplugin.c
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
*************************************************************************
 $Revision: $
 $Date:   04/19/2001 $
 $Author: Scott K. Eaton $
************************************************************************/
#include "rvlog.h"
#include "rvrtpencryptionplugin.h"

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
#include "rtputil.h" /* for logger only */
#define rvLogPtr rtpGetSource(RVRTP_SECURITY_MODULE)
#else
#define rvLogPtr (NULL)
#endif
#define logMgr (NULL)

/***************************************************************************************
 * RvRtpEncryptionPlugInConstruct
 * description:  This method constructs a RvRtpEncryptionPlugIn. All of
 *               the callbacks must be suppled for this plugin to work.
 * parameters:
 *    thisPtr - the RvRtpEncryptionPlugIn object.
 *   userData - the user data associated with the object.
 *    encrypt - the RvRtpEncryptionPlugInEncryptCB callback to use with the object.
 *    decrypt - the RvRtpEncryptionPlugInGetBlockSizeCB callback to use with the object.
 * getBlockSize - the RvRtpEncryptionPlugInGetBlockSizeCB callback to use with the object. 
 * getHeaderSize - the RvRtpEncryptionPlugInGetHeaderSizeCB callback to use with the object. 
 * returns: A pointer to the object, if successful. NULL, otherwise.
 ***************************************************************************************/
RVAPI
RvRtpEncryptionPlugIn* RVCALLCONV  RvRtpEncryptionPlugInConstruct(RvRtpEncryptionPlugIn *thisPtr,
                                        void*                                userData,
                                        RvRtpEncryptionPlugInEncryptCB       encrypt,
                                        RvRtpEncryptionPlugInDecryptCB       decrypt,
                                        RvRtpEncryptionPlugInGetBlockSizeCB  getBlockSize,
                                        RvRtpEncryptionPlugInGetHeaderSizeCB getHeaderSize)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEncryptionPlugInConstruct"));
    thisPtr->encrypt       = encrypt;
    thisPtr->decrypt       = decrypt;
    thisPtr->getHeaderSize = getHeaderSize;
    thisPtr->getBlockSize  = getBlockSize;
    thisPtr->userData      = userData;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEncryptionPlugInConstruct"));
    return thisPtr;
}

/***************************************************************************************
 * RvRtpEncryptionPlugInConstructCopy
 * description:  This method constructs a copy of a RvRtpEncryptionPlugIn.
 * parameters:
 *      thisPtr - the RvRtpEncryptionPlugIn object.
 *       srcPtr - the RvRtpEncryptionPlugIn object to base the new object on.
 * allocatorPtr - The allocator to use.
 * returns: A pointer to the object, if successful. NULL, otherwise.
 ***************************************************************************************/

RVAPI
RvRtpEncryptionPlugIn* RVCALLCONV RvRtpEncryptionPlugInConstructCopy(
	RvRtpEncryptionPlugIn *thisPtr,
    const RvRtpEncryptionPlugIn *srcPtr,
    void *allocatorPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEncryptionPlugInConstructCopy"));
    thisPtr->encrypt       = srcPtr->encrypt;
    thisPtr->decrypt       = srcPtr->decrypt;
    thisPtr->getHeaderSize = srcPtr->getHeaderSize;
    thisPtr->getBlockSize  = srcPtr->getBlockSize;
    thisPtr->userData      = srcPtr->userData;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEncryptionPlugInConstructCopy"));	
	RV_UNUSED_ARG(allocatorPtr);
    return thisPtr;
}

/***************************************************************************************
 * RvRtpEncryptionPlugInDestruct 
 * description:
 *     This method destroys the plugin, freeing any recources it held.
 * parameters:
 *    thisPtr - the RvRtpEncryptionPlugIn object. 
 * returns: none
 ***************************************************************************************/

RVAPI
void RVCALLCONV RvRtpEncryptionPlugInDestruct(RvRtpEncryptionPlugIn *thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEncryptionPlugInDestruct"));
    thisPtr->encrypt       = NULL;
    thisPtr->decrypt       = NULL;
    thisPtr->getHeaderSize = NULL;
    thisPtr->getBlockSize  = NULL;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEncryptionPlugInDestruct"));
}

