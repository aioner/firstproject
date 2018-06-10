/************************************************************************
 File Name	   : rvrtpstunfw.h
 Description   : header file for STUN/FW traversal (for SIP)
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

#ifndef __RTP_STUN_FW_H__
#define __RTP_STUN_FW_H__

#include "rtp.h"
#include "rtcp.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __RTP_OVER_STUN__
/************************************************************************************************
 * STUN (Simple Traversal of UDP through NATs (Network Address Translation)) is a protocol
 * for assisting devices behind a NAT firewall or router with their packet routing. 
 ************************************************************************************************/

/************************************************************************************************
 *                              Types definitions                                               *
 ************************************************************************************************/

/***************************************************************************
 * RvRtcpRawBufferReceived_CB
 * ------------------------------------------------------------------------
 * General: Exposes the raw data buffer to an application that contains exactly
 *          one RTCP/STUN message that was received on the UDP layer.
 *          The application can dump the data by means of this callback.
 *          Also, the application can order the RTCP Stack to discard the buffer
 *          and not to parse it, by means of the pbDiscardBuffer parameter.
 *
 * Return Value: none.
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:   hRTCP            - A handle to the RTCP session object.
 *          buffer           - pointer to the buffer,which contains the message
 *          buffLen          - length of the message in the buffer (in bytes)
 *          remoteAddress    - pointer on remote address, that sent this message
 *          context			 - a "context" supploed by user
 * Output:  pbDiscardBuffer  - if set to RV_TRUE, the buffer will be not
 *                             processed, the resources will be freed. 
 ***************************************************************************/
typedef void (RVCALLCONV *RvRtcpRawBufferReceived_CB)(
       IN  RvRtcpSession    hRTCP,
       IN  RvUint8*         buffer,
       IN  RvSize_t         buffLen,
       IN  RvNetAddress*    remoteAddress, 
	   IN  void*			context,
       OUT RvBool*          pbDiscardBuffer);

/************************************************************************************************
 *                              Functions definitions                                           *
 ************************************************************************************************/
/***************************************************************************
 * RvRtpSendRawData
 * ------------------------------------------------------------------------
 * General: Sends user buffer on the socket of RTP session.
 *          It can be used for passing of STUN protocol messages
 *          through the RTP session.
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:   hRTP             - A handle to the RTP session object.
 *          buffer           - pointer to the buffer,which contains the message
 *          buffLen          - length of the message in the buffer (in bytes)
 *
 * Output: none
 * -------------------------------------------------------------------------
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: Remote RTP address have to be set before to calling this function
 *       See RvRtpAddRemoteAddress().
 ***************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSendRawData(
       IN  RvRtpSession     hRTP,
       IN  RvUint8*         buffer,
       IN  RvSize_t         buffLen);

/***************************************************************************
 * RvRtcpSendRawData
 * ------------------------------------------------------------------------
 * General: Sends user buffer on the socket of RTCP session.
 *          Can be used for passing of of STUN protocol messages through
 *          the RTCP session.
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:   hRTCP            - A handle to the RTP session object.
 *          buffer           - pointer to the buffer,which contains the message
 *          buffLen          - length of the message in the buffer (in bytes)
 *
 * Output: none
 * -------------------------------------------------------------------------
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: Remote RTCP address have to be set before to calling this function
 *       See RvRtcpAddRemoteAddress().
 ***************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSendRawData(
       IN  RvRtcpSession    hRTCP,
       IN  RvUint8*         buffer,
       IN  RvSize_t         buffLen);


/***************************************************************************
 * RvRtpReadRawData()
 * ------------------------------------------------------------------------
 * General: Reads/Peeks raw message on RTP session socket
 * ------------------------------------------------------------------------
 * Arguments:
 * input:  hRTP          - Handle of the RTP session.
 *         buffer        - Pointer to buffer, to which raw UDP message will be placed
 *         buffSize      - The maximal size of buffer
 *        
 * output: pMessageLen               - pointer to the read message length
 *         pIsPeekMessageSupported   - Sets RV_TRUE, if RvRtpRawReadData()
 *                                     Peeks at the incoming data. The data is copied
 *                                     into the buffer but is not removed from the input queue.
 *                                     Sets RV_FALSE - otherwise (buffer is removed from the input queue).
 *                                      
 *         pRemoteAddress            - the pointer to the address of message sender
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpReadRawData(
    IN  RvRtpSession     hRTP,
    IN  RvUint8*         buffer,
    IN  RvSize_t         buffSize,
    OUT RvSize_t*        pMessageLen,
    OUT RvNetAddress*    pRemoteAddress);


/************************************************************************************
 * RvRtpParseRawData()
 * description: This routine parse [and decrypt, if needed] RTP message and
 *              and sets the header of the RTP message.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        remAddressPtr - pointer to the remote address needed for SRTP
 * output: p    - A struct of RTP param,contain the fields of RTP header.
 *         buf  - Filled buffer with decrypted data (if decryption is needed)
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpParseRawReadData(
		IN    RvRtpSession  hRTP,
		INOUT void*         buf,
		IN    RvInt32       len,
		IN    RvNetAddress* remAddressPtr,
        OUT   RvRtpParam*   p);

/***************************************************************************
 * RvRtcpSetRawBufferReceivedEventHandler
 * ------------------------------------------------------------------------
 * General: Sets callback for receiving of raw data on the RTCP session socket
 * ------------------------------------------------------------------------
 * Arguments:
 * input: hRTP          - Handle of the RTP session.
 *        callback      - Pointer to the callback function that is called each time a
 *                        new packet arrives to the RTP session's socket.
 *        context       - The parameter is an application handle that identifies the
 *                        particular RTP session. The application passes the handle to
 *                        the callback.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetRawBufferReceivedEventHandler(
       IN  RvRtcpSession               hRTCP,
       IN  RvRtcpRawBufferReceived_CB  callback,
       IN  void*                       context);

#endif /* __RTP_OVER_STUN__ */


#ifdef __cplusplus
}
#endif

#endif  /* __RTP_STUN_FW_H__ */

