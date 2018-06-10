/************************************************************************
 File Name	   : rvencriptionkeyplugin.h
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
************************************************************************/

#if !defined(RVENCRYPTIONKEYPLUGIN_H)
#define RVENCRYPTIONKEYPLUGIN_H

#include "rvtypes.h"
#include "rvrtpencryptiondata.h"
#include "rvkey.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************
*	RvRtpEncryptionKeyPlugIn
******************************************************************/
struct RvRtpEncryptionKeyPlugIn_;

/**************************************************************************************
 * RvRtpEncryptionKeyPlugInGetKeyCB
 * description:
 *          This callback is called when the RTP session needs a key to encrypt or decrypt a
 *          packet.
 * parameters:
 *   plugInPtr - A pointer to the RvRtpEncryptionKeyPlugIn object.
 *     dataPtr - Information related to the packet that needs an key.
 *      keyPtr - The key to be set in by this callback.
 ******************************************************************************************/
typedef void (*RvRtpEncryptionKeyPlugInGetKeyCB)(
		struct RvRtpEncryptionKeyPlugIn_* plugInPtr, 
		const RvRtpEncryptionData* dataPtr, 
		RvKey* keyPtr);

/**************************************************************************************
 * RvRtpEncryptionKeyPlugInReleaseCB
 * description:
 *          This callback is called when the RTP session no needs in key manager plug in
 * parameters:
 *   plugInPtr - A pointer to the RvRtpEncryptionKeyPlugIn object.
 ******************************************************************************************/
typedef void (*RvRtpEncryptionKeyPlugInReleaseCB)(
		struct RvRtpEncryptionKeyPlugIn_ *pPlugIn);

/**************************************************************************************
 * type: RvRtpEncryptionKeyPlugIn
 * descryption:
 *          This interface is used to allow the RTP Session to choose a key
 *          based on information in the current RTP/RTCP packet. To use this
 *          interface the user needs implement the RvRtpEncryptionKeyPlugInGetKeyCB
 *          and RvRtpEncryptionKeyPluginReleaseCB (for session if needed) callbacks
 **************************************************************************************/
typedef struct RvRtpEncryptionKeyPlugIn_
{
	RvRtpEncryptionKeyPlugInGetKeyCB	getKey;
	RvRtpEncryptionKeyPlugInReleaseCB   release;

} RvRtpEncryptionKeyPlugIn;

//#define rvRtpEncryptionKeyPlugInConstruct(_t, _d, _cb)    ((_t)->userData = (_d),(_t)->getKey = (_cb)/*,(_t)*/)
//#define rvRtpEncryptionKeyPlugInConstructCopy(_t, _s, _a) ((_t)->userData = (_s)->userData,(_t)->getKey = (_s)->getKey,(_t))
#define RvRtpEncryptionKeyPlugInDestruct(_t);
//#define rvRtpEncryptionKeyPlugInSetUserData(_t, _d)       ((_t)->userData = (_d))
//#define rvRtpEncryptionKeyPlugInGetUserData(_t)	          ((_t)->userData)
#define RvRtpEncryptionKeyPlugInGetKey(_t,_d,_k)          (_t)->getKey((_t),(_d),(_k))

#ifdef __cplusplus
}
#endif

#endif	/* Include guard */

