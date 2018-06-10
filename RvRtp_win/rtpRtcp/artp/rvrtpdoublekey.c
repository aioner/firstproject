/************************************************************************
 File Name     : rvrtpsinglekey.c
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
#include "rvmemory.h"
#include "rtputil.h"
#include "rvrtpdoublekey.h"
#include "rtputil.h" /* for logger only */

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define RTP_SOURCE      (rtpGetSource(RVRTP_SECURITY_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_SECURITY_MODULE))
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"
#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtp" #name

/****************************************************************************
 * RvRtpDoubleKeyDestruct
 *   descryption: destructs RvRtpDoubleKey object
 * parameters: thisPtr - pointer to RvRtpDoubleKey object
 * returns:  none
 ****************************************************************************/
void RvRtpDoubleKeyDestruct(RvRtpDoubleKey *thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyDestruct"));
    RvKeyDestruct(&thisPtr->ekey);
    RvKeyDestruct(&thisPtr->dkey);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyDestruct"));
}

static void rvRtpEncryptionKeyPlugInGetKeyCB(
	RvRtpEncryptionKeyPlugIn *pBase, 
	const RvRtpEncryptionData *pData,	 
	RvKey* pKey) 
{
	RvRtpDoubleKey *pSelf = DOWNCAST(pBase);
	RvKey *srcKey = (pData->isEncrypting) ? &(pSelf->ekey) : &(pSelf->dkey);

	RvLogEnter(rvLogPtr, (rvLogPtr, "rvRtpEncryptionKeyPlugInGetKeyCB"));
	RvKeyCopy(pKey, srcKey);
	RvLogLeave(rvLogPtr, (rvLogPtr, "rvRtpEncryptionKeyPlugInGetKeyCB"));
}

static void rvRtpEncryptionKeyPlugInReleaseCB(RvRtpEncryptionKeyPlugIn *pBase) 
{
	RvRtpDoubleKey *pSelf = DOWNCAST(pBase);
	RvLogEnter(rvLogPtr, (rvLogPtr, "rvRtpEncryptionKeyPlugInReleaseCB"));
	RvRtpDoubleKeyDestruct(pSelf);
	RvMemoryFree(pSelf, logMgr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "rvRtpEncryptionKeyPlugInReleaseCB"));
}


/* Construct/Destruct Methods */
/****************************************************************************
 * RvRtpDoubleKeyConstruct
 *   descryption: constructs RvRtpDoubleKey object
 * intput parameters: pEKey - pointer to encryption key object
 *                    pDKey - pointer to encryption key object
 * output : thisPtr - pointer to RvRtpDoubleKey object
 * returns:  pointer to RvRtpDoubleKey object, if successful,
 *           otherwise NULL
 ****************************************************************************/
RvRtpDoubleKey * RvRtpDoubleKeyConstruct(
	INOUT RvRtpDoubleKey* thisPtr,
	IN RvKey *pEKey, 
	IN RvKey *pDKey)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyConstruct"));
    if (NULL == pEKey|| NULL == pDKey||NULL == thisPtr)
	{
	    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyConstruct: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyConstruct"));
		return NULL;
	}
	RvKeyConstruct(&thisPtr->ekey);
	RvKeyConstruct(&thisPtr->dkey);
	RvKeyCopy(&thisPtr->ekey, pEKey);
	RvKeyCopy(&thisPtr->dkey, pDKey);
	thisPtr->plugIn.getKey =  rvRtpEncryptionKeyPlugInGetKeyCB;
	thisPtr->plugIn.release = NULL;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyConstruct"));
	return thisPtr;
}
/****************************************************************************
 * RvRtpDoubleKeyCreate
 *   descryption: creates a new RvRtpDoubleKey object (with allocation)
 * parameters: pEKey - pointer to encryption key object
 *             pDKey - pointer to encryption key object
 * returns:  pointer to RvRtpEncryptionKeyPlugIn object, if successful,
 *           otherwise NULL
 ****************************************************************************/
RvRtpEncryptionKeyPlugIn* RvRtpDoubleKeyCreate(
	IN RvKey *pEKey, 
	IN RvKey *pDKey) 
{
	RvRtpDoubleKey *pSelf = NULL; 
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyCreate"));
    if (NULL == pEKey|| NULL == pDKey)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyCreate: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyCreate"));
		return NULL;
	}
	RvMemoryAlloc(NULL, sizeof(RvRtpDoubleKey), logMgr, (void**)&pSelf);
	RvKeyConstruct(&pSelf->ekey);
	RvKeyConstruct(&pSelf->dkey);
	RvKeyCopy(&pSelf->ekey, pEKey);
	RvKeyCopy(&pSelf->dkey, pDKey);
	pSelf->plugIn.getKey =  rvRtpEncryptionKeyPlugInGetKeyCB;
	pSelf->plugIn.release = rvRtpEncryptionKeyPlugInReleaseCB;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeyCreate"));
	return (RvRtpEncryptionKeyPlugIn*)pSelf;
}



/* Accessors */
/****************************************************************************
 * RvRtpDoubleKeySetKeys
 *  description: 
 *          This method sets both encryption and decryption keys to RvRtpDoubleKey
 * Parameters:
 *   thisPtr - The RvRtpDoubleKey object.
 *   ekeyPtr - pointer to encryption key
 *   dkeyPtr - pointer to dencryption key
 * returns: none
 ****************************************************************************/
void RvRtpDoubleKeySetKeys(RvRtpDoubleKey *thisPtr, const RvKey* ekeyPtr, const RvKey* dkeyPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeySetKeys"));
    RvKeyCopy(&thisPtr->ekey, ekeyPtr);
    RvKeyCopy(&thisPtr->dkey, dkeyPtr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleKeySetKeys"));
}
/****************************************************************************
 * RvRtpDoubleEncKeySetKey
 *  description: 
 *          This method sets  encryption key to RvRtpDoubleKey
 * Parameters:
 *   thisPtr - The RvRtpDoubleKey object.
 *   ekeyPtr - pointer to encryption key
 * returns: none
 ****************************************************************************/
void RvRtpDoubleEncKeySetKey(RvRtpDoubleKey *thisPtr, const RvKey* ekeyPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDoubleEncKeySetKey"));
    RvKeyCopy(&thisPtr->ekey, ekeyPtr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleEncKeySetKey"));
}
/****************************************************************************
 * RvRtpDoubleDecKeySetKey
 *  description: 
 *          This method sets  encryption key to RvRtpDoubleKey
 * Parameters:
 *   thisPtr - The RvRtpDoubleKey object.
 *   dkeyPtr - pointer to dencryption key
 * returns: none
 ****************************************************************************/
void RvRtpDoubleDecKeySetKey(RvRtpDoubleKey *thisPtr, const RvKey* dkeyPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDoubleDecKeySetKey"));
    RvKeyCopy(&thisPtr->dkey, dkeyPtr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDoubleDecKeySetKey"));
}
