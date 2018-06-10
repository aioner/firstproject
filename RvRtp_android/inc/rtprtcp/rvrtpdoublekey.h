/************************************************************************
 File Name     : rvRtpDoubleKey.h
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
************************************************************************/

#if !defined(RVRtpDoubleKey_H)
#define RVRtpDoubleKey_H

#include "rvtypes.h"
#include "rvrtpencryptionkeyplugin.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************
 * type: RvRtpDoubleKey 
 * description:	
 *          This struct is an implementation of the RvRtpEncryptionKeyPlugIn that
 *          uses the same encryption key and same dencryption key 
 *          for all sent/received packets of the session.
 *          The keys values must be set before
 *          the plug-in is registered with the RvRtpSession/RvRtcpSession.
 **************************************************************************************/
typedef struct RvRtpDoubleKey_
{
	RvRtpEncryptionKeyPlugIn plugIn; /* base class plug-in */
	RvKey	                 ekey; /* encryption key */
	RvKey	                 dkey; /* dencryption key */
} RvRtpDoubleKey;

#define DOWNCAST(base) (RvRtpDoubleKey *)(base)
/*

   example:
   description:
      This is an example of how to construct, setup, and register
	  a RvRtpDoubleKey object with a session. The same keys for encryption
	  and decryption used is a 56-bit key.
   code: 
   RvRtpDoubleKey* keyManagerPtr;
   RvKey key;
   RvRtpEncryptionMode mode = RV_RTPENCRYPTIONMODE_RFC3550;
   RvRtp3DesEncryption 3DesEncryptor;
   RvUint8 keyData[7] = \{1,2,3,4,5,6,7\};
   
	 /\* Initialize the Key *\/
	 RvKeyConstruct(&key);
	 RvKeySetValue(&key,keyData,56);
	 
	 /\* Initialize the key plug-in*\/
	 keyManagerPtr = RvRtpDoubleKeyCreate(&key, &key); /\* same key for encryption and decryption *\/
	   
	 /\* Register the key plug-in*\/
	RvRtpSetEncryption(&session, mode, rvRtp3DesEncryptionGetPlugIn(&3DesEncryptor), keyManagerPtr);
		 
	/\* Clean up the key*\/
	RvKeyDestruct(&key);
*/

/* Construct/Destruct Methods */

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
	IN RvKey *pDKey);
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
	IN RvKey *pDKey);
/****************************************************************************
 * RvRtpDoubleKeyDestruct
 *   descryption: destructs RvRtpDoubleKey object
 * parameters: thisPtr - pointer to RvRtpDoubleKey object
 * returns:  none
 ****************************************************************************/
void RvRtpDoubleKeyDestruct(RvRtpDoubleKey *thisPtr);

/* Accessors */
/****************************************************************************
 * RvRtpDoubleKeyGetPlugIn
 *  description: 
 *          This method retreives the RvRtpEncryptionKeyPlugIn interface to be registered 
 *          with the RvRtpSession/RvRtcpSession.
 * Parameters:
 *   thisPtr - The RvRtpDoubleKey object.
 * returns: The RvRtpEncryptionKeyPlugIn interface.
 ****************************************************************************/
#define RvRtpDoubleKeyGetPlugIn(t) &(t)->plugIn
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
void RvRtpDoubleKeySetKeys(RvRtpDoubleKey *thisPtr, const RvKey* ekeyPtr,  const RvKey* dkeyPtr);
/****************************************************************************
 * RvRtpDoubleEncKeySetKey
 *  description: 
 *          This method sets  encryption key to RvRtpDoubleKey
 * Parameters:
 *   thisPtr - The RvRtpDoubleKey object.
 *   ekeyPtr - pointer to encryption key
 * returns: none
 ****************************************************************************/
void RvRtpDoubleEncKeySetKey(RvRtpDoubleKey *thisPtr, const RvKey* ekeyPtr);
/****************************************************************************
 * RvRtpDoubleDecKeySetKey
 *  description: 
 *          This method sets  encryption key to RvRtpDoubleKey
 * Parameters:
 *   thisPtr - The RvRtpDoubleKey object.
 *   dkeyPtr - pointer to dencryption key
 * returns: none
 ****************************************************************************/
void RvRtpDoubleDecKeySetKey(RvRtpDoubleKey *thisPtr, const RvKey* dkeyPtr);

#ifdef __cplusplus
}
#endif

#endif  /* Include guard */

