/************************************************************************
 File Name     : rvrtpencryptiondata.h
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

#if !defined(RVRTPENCRYPTIONSTREAMDATA_H)
#define RVRTPENCRYPTIONSTREAMDATA_H

#include "rvrtpencryptionmode.h"
#include "rvnetaddress.h"
#include "rvrtpheader.h"
//#include "rvmemory.h"

#ifdef __cplusplus
extern "C" {
#endif


/* This struct represents data associated with a rtp/rtcp packet 
   that may be needed by an encryption or key selection algorithm. */
typedef struct RvRtpEncryptionData_
{
    RvBool              isRtp;             /* Flag, TRUE if RTP packet, False - RTCP. */
    RvBool              isEncrypting;
    RvRtpEncryptionMode mode;              /* encryption mode */
    const RvNetAddress*    remoteAddressPtr;  /* pointer to remote address */
    const RvNetAddress*    localAddressPtr;   /* pointer to local address */
    const RvRtpParam*    rtpHeaderPtr;     /* ponter to RTP header information */
} RvRtpEncryptionData;

/*******************************************************************************************
 * RvRtpEncryptionDataConstruct
 * description: this function constructs a RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: A pointer to the RvRtpEncryptionData object if successfull, NULL otherwise. 
 *******************************************************************************************/
RvRtpEncryptionData* RvRtpEncryptionDataConstruct(RvRtpEncryptionData* thisPtr);
/*******************************************************************************************
 * RvRtpEncryptionDataConstructCopy
 * description: this function constructs a copy of a RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 *         srcPtr  - the RvRtpEncryptionData object to copy into this object.
 *     allcatorPtr - the allocator to use to construct the new object.
 * returns: A pointer to the RvRtpEncryptionData object if successfull, NULL otherwise. 
 *******************************************************************************************/
RvRtpEncryptionData* RvRtpEncryptionDataConstructCopy(RvRtpEncryptionData* thisPtr, const RvRtpEncryptionData* srcPtr, void* allcatorPtr);
/*******************************************************************************************
 * RvRtpEncryptionDataDestruct
 * description: this function destroys a RvRtpEncryptionData object freeing any
 *              resources it may contain.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: none
 *******************************************************************************************/
#define RvRtpEncryptionDataDestruct(t)
/*******************************************************************************************
 * RvRtpEncryptionDataCopy
 * description: his function copies of a RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 *         srcPtr  - the RvRtpEncryptionData object to copy into this object.
 *     allcatorPtr - the allocator to use to construct the new object.
 * returns: none
 *******************************************************************************************/
void RvRtpEncryptionDataCopy(RvRtpEncryptionData* thisPtr,const RvRtpEncryptionData* srcPtr);
/*******************************************************************************************
 * RvRtpEncryptionDataIsRtp
 * description: this method gets the flag indicating whether the packet is a RTP or RTCP packet.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: RV_TRUE is the packet associated with the object is an RTP packet, 
 *          RV_FALSE, if the packet is RTCP
 *******************************************************************************************/
#define RvRtpEncryptionDataIsRtp(t)              ((t)->isRtp)
/*******************************************************************************************
 * RvRtpEncryptionDataIsEncrypting
 * description: this method gets the flag indicating whether the packet
 *              is being encrypted or decrypted.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: RV_TRUE is the packet associated with the object is to be encrypted, 
 *          RV_FALSE, if the packet is to be decrypted.
 *******************************************************************************************/
#define RvRtpEncryptionDataIsEncrypting(t)       ((t)->isEncrypting)
/*******************************************************************************************
 * RvRtpEncryptionDataGetMode
 * description: this method gets the current encryption mode from the RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: the current encryption mode.
 *******************************************************************************************/
#define RvRtpEncryptionDataGetMode(t)            ((t)->mode)
/*******************************************************************************************
 * RvRtpEncryptionDataGetRemoteAddress
 * description: this method gets the remote address from the RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: A pointer to a RvAddress that contains the remote address of the packet,
 *          this is the source address if the packet is being decypted, and the destination
 *          address if the packet is being encrypted.
 *******************************************************************************************/
#define RvRtpEncryptionDataGetRemoteAddress(t)   (const RvNetAddress*)((t)->remoteAddressPtr)
/*******************************************************************************************
 * RvRtpEncryptionDataGetLocalAddress
 * description: this method gets the local address from the RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: a pointer to a RvAddress that contains the local address of the session
 *          sending the packet.
 *******************************************************************************************/
#define RvRtpEncryptionDataGetLocalAddress(t)    (const RvNetAddress*)((t)->localAddressPtr)
/*******************************************************************************************
 * RvRtpEncryptionDataGetRtpHeader
 * description: this method gets the RTP header object from the RvRtpEncryptionData object.
 * params: thisPtr - the RvRtpEncryptionData object.
 * returns: a pointer to a RvRtpParam of the packet. This will be NULL if the current mode
 *          encrypts the header or the packet is a RTCP packet.
 *******************************************************************************************/
#define RvRtpEncryptionDataGetRtpHeader(t)       (const RvRtpParam*)((t)->rtpHeaderPtr)



/* Unpublished methods for internal use. */
#define RvRtpEncryptionDataSetIsRtp(t,r)         ((t)->isRtp = (r))
#define RvRtpEncryptionDataSetIsEncrypting(t,e)  ((t)->isEncrypting = (e))
#define RvRtpEncryptionDataSetMode(t,m)          ((t)->mode = (m))
#define RvRtpEncryptionDataSetRemoteAddress(t,a) ((t)->remoteAddressPtr = (a))
#define RvRtpEncryptionDataSetLocalAddress(t,a)  ((t)->localAddressPtr = (a))
#define RvRtpEncryptionDataSetRtpHeader(t,h)     ((t)->rtpHeaderPtr = (h))

#ifdef __cplusplus
}
#endif

#endif  /* Include guard */

