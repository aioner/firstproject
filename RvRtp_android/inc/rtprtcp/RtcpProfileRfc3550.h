/************************************************************************
 File Name	   : RtcpProfileRfc3550.h
 Description   : scope: Private
     RTCP related type definitions for RTP profile 3550
*************************************************************************
***********************************************************************
        Copyright (c) 2005 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

#ifndef __RTCP_PROFILE_RFC_3550_H__
#define __RTCP_PROFILE_RFC_3550_H__

#include "rvtypes.h"
#include "rtpProfilePlugin.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************
 * RtpProfileRfc3550RtcpRawReceive
 * description: this function is called, when plugin have to receive 
 *              the RTCP compound packet filled buffer.
 * input: hRTCP            - Handle of the XRTCP session.
 *        bytesToReceive   - The maximal length to receive.
 *        isPeekMessage    - TRUE, if RvSocketPeekMessage is supported
 * output:
 *        buf              - Pointer to buffer for receiving a message.
 *        bytesReceivedPtr - pointer to received RTCP size
 *        remoteAddressPtr - Pointer to the remote address 
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RvStatus  RtpProfileRfc3550RtcpRawReceive(
     IN     struct __RtpProfilePlugin* plugin,
     IN     RvRtcpSession              hRTCP,
     INOUT  RvRtpBuffer*               buf,
     IN     RvSize_t                   bytesToReceive,
     IN     RvBool                     isPeekMessage,
     OUT    RvSize_t*                  bytesReceivedPtr,
     OUT    RvAddress*                 remoteAddressPtr);

/************************************************************************************
 * RtpProfileRfc3550RtcpRawSend
 * description: this function is called, when plugin have to send
 *              the RTCP compound packet filled buffer.
 * input: plugin           - pointer to this plugin 
 *        hRTCP            - Handle of the RTCP session.
 *        bufPtr           - Pointer to buffer containing the RCTP packet.
 *        DataLen          - Length in bytes of RTCP data buf.
 *        DataCapacity     - data capacity of RTCP buffer
 *        sentLenPtr       - for bandwith calculation only
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RvStatus  RtpProfileRfc3550RtcpRawSend(
        struct __RtpProfilePlugin* plugin,                                   
        IN    RvRtcpSession hRTCP,
        IN    RvUint8*      bufPtr,
        IN    RvInt32       DataLen,
        IN    RvInt32       DataCapacity,
        INOUT RvUint32*     sentLenPtr);

#ifdef __cplusplus
}
#endif

#endif  /* __RTCP_PROFILE_RFC_3550_H__ */


