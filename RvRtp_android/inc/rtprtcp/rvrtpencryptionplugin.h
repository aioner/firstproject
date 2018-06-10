/************************************************************************
 File Name	   : rvencriptionplugin.h
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
*************************************************************************
 $Revision: $
 $Date:   01/09/2001 $
 $Author: Scott K. Eaton $
************************************************************************/

#if !defined(RVENCRYPTIONPLUGIN_H)
#define RVENCRYPTIONPLUGIN_H

#include "rvtypes.h"
#include "rvrtpencryptiondata.h"
#include "rvkey.h"
/*#include "rvmemory.h"*/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************
*	RvRtpEncryptionPlugIn
******************************************************************/
struct RvRtpEncryptionPlugIn_;

/**************************************************************************
 * RvRtpEncryptionPlugInEncryptCB
 * description: This callback is called when the RTP session is needs to
 *              encrypt an outgoing data packet.}
 * parameters:
 *   thisPtr - A pointer to the RvRtpEncryptionPlugIn object.
 * srcBuffer - A pointer to the plain text buffer.
 * dstBuffer - A pointer to the cipher text buffer.
 * byteLength - Length of the plain text buffer.
 * dataPtr   - Data related to the RTP/RTCP packet. 
 * key       - The key to use for encryption. 
 ***************************************************************************/
typedef void (*RvRtpEncryptionPlugInEncryptCB)(struct RvRtpEncryptionPlugIn_ *thisPtr,
								const RvUint8 *srcBuffer,
								RvUint8 *dstBuffer,
								RvUint32 byteLength,
                                RvRtpEncryptionData* dataPtr,
                                const  RvKey* key);

/**************************************************************************
 * RvRtpEncryptionPlugInDecryptCB
 * description: This callback is called when the RTP session is needs to
 *              decrypt an incoming data packet.
 * parameters:
 *     thisPtr - A pointer to the RvRtpEncryptionPlugIn object. 
 *   srcBuffer - A pointer to the cipher text buffer.
 *   dstBuffer - A pointer to the plain text buffer.
 *  byteLength - Length of the cipher text buffer.
 *     dataPtr - Data related to the RTP/RTCP packet.
 *         key - The key to use for decryption.}}
 ***************************************************************************/
typedef void (*RvRtpEncryptionPlugInDecryptCB)(struct RvRtpEncryptionPlugIn_ *,
								const RvUint8 *srcBuffer,
								RvUint8 *dstBuffer,
								RvUint32 byteLength,
                                RvRtpEncryptionData* dataPtr,
                                const RvKey* key);

/**************************************************************************
 * RvRtpEncryptionPlugInGetBlockSizeCB
 * description: This callback is called when the RTP session needs to
 *              find out what block size the encryption algorithm is using.
 *              The block size is in bytes. If the encryption algorithm is
 *              a stream algorithm this function should return a value of 1.
 *              The block size must be 1 or a multiple of 4 (i.e. valid
 *              values are: 1,4,8,12,16,20,...).
 * parameters:
 *     thisPtr - A pointer to the RvRtpEncryptionPlugIn object. 
 * returns: RvUint32, The required block size in bytes.
 **************************************************************************/
typedef RvUint32 (*RvRtpEncryptionPlugInGetBlockSizeCB)(struct RvRtpEncryptionPlugIn_ *thisPtr);
/**************************************************************************
 * RvRtpEncryptionPlugInGetHeaderSizeCB
 * description: This callback is called when the RTP session needs to
 *              find out what header size the encryption algorithm is using.
 * parameters:
 *     thisPtr - A pointer to the RvRtpEncryptionPlugIn object. 
 * returns: RvUint32, The required header size in bytes.
 **************************************************************************/
typedef RvUint32 (*RvRtpEncryptionPlugInGetHeaderSizeCB)(struct RvRtpEncryptionPlugIn_ *thisPtr);

/***************************************************************************************
 * RvRtpEncryptionPlugIn :
 * description:
 *   This interface is used to allow the RTP Session to encrypt/decrypt
 *   data. To use this interface the user MUST implement all of the
 *   following callbacks:
 *              RvRtpEncryptionPlugInEncryptCB 
 *              RvRtpEncryptionPlugInDecryptCB
 *              RvRtpEncryptionPlugInGetBlockSizeCB
 *              RvRtpEncryptionPlugInGetHeaderSizeCB
 * see also examples of usage in sample: 
 *   RvRtpDesEncryption
 *   RvRtp3DesEncryption
 ****************************************************************************************/
typedef struct RvRtpEncryptionPlugIn_
{
	RvRtpEncryptionPlugInEncryptCB		  encrypt;
	RvRtpEncryptionPlugInDecryptCB		  decrypt;
	RvRtpEncryptionPlugInGetBlockSizeCB   getBlockSize;
	RvRtpEncryptionPlugInGetHeaderSizeCB  getHeaderSize;
	void*								  userData;

} RvRtpEncryptionPlugIn;
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
RvRtpEncryptionPlugIn* RVCALLCONV RvRtpEncryptionPlugInConstruct(
	RvRtpEncryptionPlugIn *thisPtr,
	void*								 userData,
	RvRtpEncryptionPlugInEncryptCB		 encrypt,
	RvRtpEncryptionPlugInDecryptCB		 decrypt,
	RvRtpEncryptionPlugInGetBlockSizeCB  getBlockSize,
	RvRtpEncryptionPlugInGetHeaderSizeCB getHeaderSize);
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
	void *allocatorPtr);
/***************************************************************************************
 * RvRtpEncryptionPlugInDestruct 
 * description:
 *     This method destroys the plugin, freeing any recources it held.
 * parameters:
 *    thisPtr - the RvRtpEncryptionPlugIn object. 
 * returns: none
 ***************************************************************************************/
RVAPI
void RVCALLCONV RvRtpEncryptionPlugInDestruct(RvRtpEncryptionPlugIn *thisPtr);
/***************************************************************************************
 * RvRtpEncryptionPlugInSetUserData 
 * description:
 *      This method sets the plugin's user data. This is a piece of
 *      data that the user can use for what ever they like, it is
 *      not used internally by the class.
 * parameters:
 *       thisPtr - the RvRtpEncryptionPlugIn object. 
 *      userData - the user data.
 * returns : none
 ***************************************************************************************/
#define  RvRtpEncryptionPlugInSetUserData(thisPtr, d)  ((thisPtr)->userData = (d))
/***************************************************************************************
 * RvRtpEncryptionPlugInGetUserData  
 * description:
 *      This method returns the user supplied data from the plugin.  
 * Parameters:
 *   thisPtr - the RvRtpEncryptionPlugIn object. 
 * returns: The user supplied data.
 ***************************************************************************************/
#define  RvRtpEncryptionPlugInGetUserData(thisPtr)	   ((thisPtr)->userData)

/* Unpublished protected methods */
#define  RvRtpEncryptionPlugInEncrypt(thisPtr, src, dst, length, data, key)	  (thisPtr)->encrypt((thisPtr),(src),(dst),(length),(data),(key))
#define  RvRtpEncryptionPlugInDecrypt(thisPtr, src, dst, lengthPtr, data, key) (thisPtr)->decrypt((thisPtr),(src),(dst),(lengthPtr),(data),(key))
#define  RvRtpEncryptionPlugInGetHeaderSize(thisPtr)					  (thisPtr)->getHeaderSize((thisPtr))
#define  RvRtpEncryptionPlugInGetBlockSize(thisPtr) 					  (thisPtr)->getBlockSize((thisPtr))

#ifdef __cplusplus
}
#endif

#endif	/* Include guard */

