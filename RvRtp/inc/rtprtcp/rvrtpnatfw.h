/************************************************************************
 File Name	   : rvrtpnatfw.h
 Description   : header file for NAT/FW traversal for H.323
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

#ifndef __RTP_NATFW_H__
#define __RTP_NATFW_H__

#include "rtp.h"
#include "rtcp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Declaration of Demux handle type */
RV_DECLARE_HANDLE(RvRtpDemux); 
/**************************************************************************************
 * RvRtpDemuxEventHandler_CB
 * description: Callback for receiving RTP messages in non-blocking mode associated with hDemux
 * Input Parameters:
 *   hDemux - handle to RTP demultiplexing object
 *   context  - an application handle that passed by RvRtpDemuxSetEventHandler
 * Output: None
 * 
 * Return Values: None
 **************************************************************************************/
typedef void (*RvRtpDemuxEventHandler_CB)
(
        IN  RvRtpDemux   hDemux,
        IN  void *       context
);

/**************************************************************************************
 * RvRtpNatMultiplexIdSize() 
 * 
 * description: returns size of multiplexID in RTP/RTCP packet before RTP/RTCP header. 
 * 
 * Input:  None
 * 
 * Output: None
 * 
 * Return Values: Returns the size of multplexID.
 * 
 * Note: This function is needed when an additional buffer is added before the RTP header.
 *       For example, for g.711:
 *       RvRtpParam p;
 *       RvRtpParamConstruct(&p);
 *       p.sByte = RvRtpNatMultiplexIdSize(); 
 *       p.sByte += RvRtpPCMUGetHeaderLength(); 
 *       .....
 *       RvRtpWrite(hRTP, buffer, p.sByte+ payload len, &p);
 **************************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpNatMultiplexIdSize(void);

#ifdef __H323_NAT_FW__
/**************************************************************************************
 * RvRtpDemuxConstruct()
 * 
 * Description: Constructs an RTP demultiplexing object. The memory of the object will be
 *              allocated according to the NumberOfSessions parameter.
 * 
 * Input: NumberOfSessions - The maximal number of demultiplexing sessions.
 * 
 * Output: None
 *
 * Return Values: RvRtpDemux - Returns the handle to the RTP demultiplexing object on success, 
 *                             otherwise returns NULL.
 **************************************************************************************/

RVAPI
RvRtpDemux RVCALLCONV RvRtpDemuxConstruct(
  IN RvUint32 numberOfSessions); /* Can be expanded with other parameters. */

/**************************************************************************************
 * RvRtpDemuxDestruct()
 * 
 * Description: Destructs the RTP demultiplexing object. The memory of the object will be
 *              freed.
 * 
 * Input: demux - The handle to the RTP demultiplexing object.
 * 
 * Output: None
 * 
 * Return Values: Returns a non-negative value on success or a negative value on failure.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDemuxDestruct(IN RvRtpDemux demux);

/***************************************************************************************
 *                  Demultiplexing RTP and RTCP Sub-session Management
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpDemuxOpenSession() 
 * 
 * Description: Checks the local address and opens an RTP/RTCP sub-session with the same address 
 *              as the sub-session that was already opened (if it exists).
 *              The first RTP/RTCP sub-session sets the local demultiplexing address. 
 * 
 * Input: demux          - The handle to the RTP demultiplexing object.
 *        pRtpAddress    - Contains the local address for the RTP session.
 *        ssrcPattern    - The Synchronization source pattern value for the RTP session.
 *        ssrcMask       - The synchronization source mask value for the RTP session.
 *        cname          - The unique name representing the source of the RTP data. 
 *                         The name will be sent in the canonical endpoint identifier SDES
 *                         item through the corresponding RTCP session.
 *        sessionContext - The context, usually used for the application RTP session handle.
 * 
 * Output: pMultiplexID   - The pointer to multiplexID.
 * 
 * Return Values: Returns the RvRtpSession handle on success or a NULL value on failure.
 * 
 * Note:
 * The socket of this session will be shared by all RTP sub sessions. 
 * The packets of these RTP sub-sessions will pass through the NAT/FW. 
 * If cname is not NULL, the RTCP multiplexing session will be opened 
 * with the port that is equal to the RTP session port +1.
 **************************************************************************************/
RVAPI
RvRtpSession RVCALLCONV RvRtpDemuxOpenSession (
  IN     RvRtpDemux            demux,
  IN     RvNetAddress*         pRtpAddress,
  IN     RvUint32              ssrcPattern,
  IN     RvUint32              ssrcMask,
  IN     char *                cname,
  IN     void*                 sessionContext,
  OUT    RvUint32*             pMultiplexID);


/**************************************************************************************
 * RvRtpDemuxCloseSession() 
 * 
 * Description: Closes the RTP sub-session and the corresponding RTCP sub-session (if associated).
 * 
 * Input: rtpSession - The handle to the demultiplexing RTP sub-session.
 * 
 * Output: None
 * 
 * Return Values: Returns a non-negative value on success or a negative value on failure.
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpDemuxCloseSession(
   IN RvRtpSession rtpSession);

/************************************************************************************
 * RvRtpDemuxSetEventHandler()
 * 
 * Description: Sets an event handler for the multiplexed RTP sessions. The application may set
 *              an event handler for RTP multiplexed sessions to operate in non-blocking mode.
 * 
 * Input: hdemux        - The demux object handle.
 *        eventHandler  - A pointer to the callback function that is called each time a
 *                        new RTP packet arrives to the demux.
 *        context       - An application handle that identifies the
 *                        particular demux. The application passes the handle to
 *                        the Event Handler.
 * 
 * 
 * Output: None
 *
 * Return Values: None
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtpDemuxSetEventHandler(
        IN  RvRtpDemux                 hdemux,
        IN  RvRtpDemuxEventHandler_CB  eventHandler,
        IN  void *                     context);

/**************************************************************************************
 * RvRtcpDemuxOpenSession()
 * 
 * Description: Checks the local address and opens an RTCP sub-session with the same address 
 *              as the sub-session that was already opened (if it exists).
 *              The first RTCP sub-session sets the local demultiplexing address. 
 *              This function can be used only if the RTCP demultiplexing port is
 *              not consecutive to the RTP demultiplexing port, or if the local IP for 
 *              RTP and RTCP are different.
 *
 * Input: demux          - The handle to RTP demultiplexing object.
 *        pRtpAddress    - Contains the local address for the RTP session.
 *        ssrc           - Synchronization source value of the RTP session.
 *        cname          - A non-NULL value of the unique name representing the source of the RTP data. 
 *                         The name will be sent in the canonical end-point identifier SDES
 *                         item through the corresponding RTCP session.
 * Output: pMultiplexID  - A pointer to multiplexID.
 * 
 * Return Values: RvRtcpSession handle, on success, NULL value on failure
 *
 * Note:
 * The socket of this session will be shared by all RTCP sub sessions. 
 * The packets of these RTCP sub-sessions will pass through the NAT/FW. 
 **************************************************************************************/

RVAPI
RvRtcpSession RVCALLCONV RvRtcpDemuxOpenSession (
    IN         RvRtpDemux            demux,
    IN         RvUint32              ssrc,
    IN         RvNetAddress*         pRtcpAddress,
    IN         char*                 cname,
    OUT        RvUint32*             pMultiplexID);

/**************************************************************************************
 * 
 * RvRtcpDemuxCloseSession() 
 * 
 * Description: Closes the demultiplexed RTCP sub-session.
 * 
 * Input: rtcpSession - The handle to the demultiplexing RTP sub-session.
 * 
 * Output: None
 * 
 * Return Values: Returns a non-negative value on success or a negative value on failure.
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtcpDemuxCloseSession(
   IN RvRtcpSession rtcpSession);

/**************************************************************************************
 * RvRtpDemuxGetLocalAddress() 
 *
 * Description: Gets the local RTP demultiplexing address. 
 *
 * Input: demux - The handle to the RTP demultiplexing object.
 *
 * Output: pRtpAddress - Contains the local address for the RTP session.
 *
 * Return Values: If an error occurs, the function returns a negative value.Otherwise returns
 *                a non-negative value.
 *
 * Note: At least one session that supports demultiplexing must be opened by 
 *       RvRtpDemuxOpenSession.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDemuxGetLocalAddress(
    IN     RvRtpDemux            demux,
    INOUT  RvNetAddress*         pRtpAddress);

/**************************************************************************************
 * RvRtpDemuxOpenedSessionsNumberGet()
 *
 * Description: Returns the number of opened session in the demux.
 *
 * Input: demux - The handle to the RTP demux object.
 *
 * Output: None
 *
 * Return Values: Non-negative value on success or Negative value on failure
 **************************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDemuxOpenedSessionsNumberGet(RvRtpDemux demux);

/**************************************************************************************
 * RvRtcpDemuxGetLocalAddress() 
 *
 * Description: Gets the local RTCP multiplexing address. 
 *
 * Input: demux - The handle to the RTP session.
 *
 * Output: pRtcpAddress  - Contains the local address for the RTCP session.
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise 
 *                 returns a non-negative value.
 *
 * Note:
 * At least one RTCP session that supports multiplexing must be opened by 
 * RvRtpDemuxOpenSession or RvRtpDemuxOpenSession.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpDemuxGetLocalAddress(
    IN     RvRtpDemux            demux,
    INOUT  RvNetAddress*         pRtcpAddress);

/***************************************************************************************
 *                   RTP and RTCP Remote Destination Management
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpMultiplexingAddRemoteAddress() 
 *
 * Description: Adds the new RTP address of the remote peer or the address of a multi-cast group,
 *              or of a multi-unicast address list to which the RTP stream will be sent.
 *
 * Input: hRTP          - The handle to the RTP session.
 *        pRtpAddress   - The pRtpAddress of the remote RTP session.
 *        multiplexerId - The multiplexID of the remote RTP session.
 *
 * Output: None
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise,
 *                returns a non-negative value.
 *
 * Note: Identical addresses that have the same multiplexerID will be added only once.
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpMultiplexingAddRemoteAddress(
	IN RvRtpSession   hRTP,   /* RTP Session Opaque Handle */
	IN RvNetAddress*  pRtpAddress,
    IN RvUint32       multiplexerId);

/**************************************************************************************
 * RvRtpMultiplexingAddRemoteAddress() 
 *
 * Description: Adds the new RTCP address of the remote peer or the address of a multi-cast group,
 *              or of the multi-unicast address list to which the RTCP stream will be sent.
 *
 * Input: hRTCP          - The handle to the RTCP session.
 *        pRtcpAddress   - The pRtpAddress of the remote RTCP session.
 *        multiplexerId  - The multiplexID of the remote RTCP session.
 *
 * Output: None
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise
 *                the function returns a non-negative value.
 *
 * Note: Identical addresses that have the same multiplexerID will be added only once.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpMultiplexingAddRemoteAddress(
	IN RvRtcpSession  hRTCP,   /* RTCP Session Opaque Handle */
	IN RvNetAddress*  pRtcpAddress,
    IN RvUint32       multiplexerId);

/**************************************************************************************
 * RvRtpMultiplexingRemoveRemoteAddress() 
 *
 * Description: Removes one of the following addresses from the list 
 *              to which the RTP stream was sent:
 *              the RTCP multiplexing address of the remote peer, the multi-cast 
 *              group address, or the multi-unicast address.
 *
 * Input: hRTP           - The handle to the RTP demultiplexing object.
 *        pRtpAddress    - The pRtpAddress of the remote RTP session.
 *        multiplexerId  - The multiplexID of the remote RTP session.
 *
 * Output: None
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise
 *                the function returns a non-negative value.
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpMultiplexingRemoveRemoteAddress(
	IN RvRtpSession    hRTP,   /* RTP Session Opaque Handle */
	IN RvNetAddress*   pRtpAddress,
    IN RvUint32        multiplexerId);

/**************************************************************************************
 * RvRtcpMultiplexingRemoveRemoteAddress() 
 *
 * Description: Removes one of the following addresses from the list 
 *              to which the RTCP stream was sent:
 *              the RTCP multiplexing address of the remote peer, the multi-cast 
 *              group address, or the multi-unicast address.
 *
 * Input: hRTCP           - The handle to RTCP demultiplexing object.
 *        pRtcpAddress    - The pRtpAddress of the remote RTCP session.
 *        multiplexerId   - The multiplexID of the remote RTCP session.
 *
 * Output: none.
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise
 *                returns a non-negative value.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpMultiplexingRemoveRemoteAddress (
	IN RvRtcpSession  hRTCP,   /* RTCP Session Opaque Handle */
	IN RvNetAddress*  pRtcpAddress,
    IN RvUint32       multiplexerId);
 
/***************************************************************************************
 *                   RTP Keep Alive Packet
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpSendKeepAlivePacket()
 * 
 * Description: Sends an RTP keep-alive packet to the remote session.
 * 
 * Input: hRTP - The handle to the RTP session. 
 *        p    - A pointer to the RvRtpParam structure (the payload type field must be set).
 * 
 * Output: None
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise
 *              the function returns a non-negative value.
 *
 * Note: 1) H.460.19-MA defines the keepAliveInterval (5-30 seconds) of such packets in case that 
 *       media is not sent. Sending of this packet is the responsibility of the application.
 *       2) The payload field must be filled in the structure to which pointer p points.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSendKeepAlivePacket(
   IN  RvRtpSession hRTP, 
   IN  RvRtpParam *   p);


/***************************************************************************************
 *                 Receiving of Multiplexed Packets
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpDemuxReadWithRemoteAddress() 
 *
 * Description: Receives multiplexed packets from the remote peer.            
 *
 * Input: hdemux - The demux object handle.
 *        buf    - A pointer to the buffer containing the RTP packet with space before the first
 *                 payload byte for the RTP header.
 *        len    - The length of buf, in bytes.
 *
 * Output: phRTP             - A pointer to the handle of the RTP sub session to which this packet was sent.
 *         sessionContextPtr - A pointer to the context, usually used for the application RTP session handle.
 *         p                 - A pointer to the RvRtpParam structure (the payload type field must be set).
 *         remAddressPtr     - A pointer to the remote address
 *
 * Return Values: If an error occurs, the function returns a negative value. Otherwise
 *                the function returns a non-negative value.
 **************************************************************************************/
 
RVAPI
RvStatus RVCALLCONV RvRtpDemuxReadWithRemoteAddress(
     IN     RvRtpDemux            hdemux,
     IN     void*                 buf,
     IN     RvInt32               len,
     INOUT  RvRtpSession*         phRTP,
     INOUT  void**                sessionContextPtr,
     OUT    RvRtpParam*           p,
     OUT    RvNetAddress*         remAddressPtr);

#endif /* #ifdef __H323_NAT_FW__ */




#ifdef __cplusplus
}
#endif

#endif  /* __RTP_NATFW_H__ */

