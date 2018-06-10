/***********************************************************************
        Copyright (c) 2002 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

#ifndef __RTP_H
#define __RTP_H

#include "rvtypes.h"
#include "rvtransport.h"
#include "rvrtpconfig.h"
#include "rvrtpinterface.h"
#include "rvnetaddress.h"
#include "rvrtpheader.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)    
#if defined(_WIN64)
typedef unsigned char* RtpSocket;
#else
typedef int RtpSocket;
#endif
#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)
typedef unsigned char* RtpSocket;
#else
typedef int RtpSocket;
#endif	

#if defined(RVRTP_SECURITY)
#include "rvkey.h"
#include "rvrtpencryptionmode.h"
#include "rvrtpencryptionplugin.h"
#include "rvrtpencryptionkeyplugin.h"
#endif

#define RVVXDAPI RVAPI
#define VXDCALLCONV RVCALLCONV
#define CALLCONVC


#define ERR_RTP_PORTRANGE     -450   


RV_DECLARE_HANDLE(RvRtpSession);

#ifndef __RTCP_H
RV_DECLARE_HANDLE(RvRtcpSession);
#endif

RV_DECLARE_HANDLE(RvRtpLogger);

/* Callback for receiving RTP messages in non-blocking mode */
 
typedef void (*RvRtpEventHandler_CB)
    (
        IN  RvRtpSession  hRTP,
        IN  void *       context
    );

/* Callback for receiving event, when shutdown process is completed */
/* if reason for shutdown was dynamically allocated in this callback it can be released */
typedef RvBool (RVCALLCONV *RvRtpShutdownCompleted_CB)(
		IN  RvRtpSession  hRTP,
		IN  RvChar*      reason,     
		IN  void *       context);

                       /* == Basic RTP Functions == */
/************************************************************************************
 * RvRtpInit
 * description: Initializes the instance of the RTP module. 
 * input: none
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Note: RvRtpInit() sets local address as IPV4 any address (IP=0.0.0.0; Port = 0)
 *       In IPV6 envinriment use RvRtpInitEx(). 
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpInit(void);


/************************************************************************************
 * RvRtpInitEx
 * description: Initializes the RTP Stack and specifies the local IP address 
 *              to which all RTP sessions will be bound. 
 * input: pRvRtpAddress - pointer to RvNetAddress, which specifies 
 *		the local IP (IPV4/6) address to which all RTP sessions will be bound. 
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Remarks 
 * - This function can be used instead of RvRtpInit().
 * - RvRtpInit() binds to the “any” IPV4 address. 
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtpInitEx(RvUint32 ip);
#endif

RVAPI
RvInt32 RVCALLCONV RvRtpInitEx(RvNetAddress* pRvRtpAddress); /* IP */

/************************************************************************************
 * RvRtpEnd
 * description: Shuts down an instance of an RTP module. 
 * input: none.
 * output: none.
 * return value: none.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtpEnd(void);



/************************************************************************************
 * RvRtpSetPortRange
 * description: Set the RTP/RTCP ports range to RTP stack instance
 * input: firstPort - first port that may be used for RTP session
 *		  lastPort - last port that may be used for RTP session
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetPortRange(RvUint16 firstPort, RvUint16 lastPort);

/************************************************************************************
 * RvRtpSetLocalAddress
 * description: Set the local address to use for calls to RvRtpOpenXXX functions.
 *              This parameter overrides the value given in RvRtpInitEx() for all
 *              subsequent calls.
 * input: pRtpAddress points on RvNetAddress which contains ip - Local IP address to use
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtpSetLocalAddress(IN RvUint32 ip);
#endif

RVAPI
RvInt32 RVCALLCONV RvRtpSetLocalAddress(IN RvNetAddress* pRtpAddress);

/************************************************************************************
 * RvRtpGetAllocationSize
 * description: returns the number of bytes that the application should allocate 
 *              in memory for an RTP session data structure. 
 * input:  none.
 * output: none.
 * return value: The number of bytes required for (internal) RTP session data structure.
 * Note:
 *            This function works together with the RvRtpOpenFrom() function. 
 *            RvRtpGetAllocationSize() indicates to the application the size of the buffer, 
 *            which is one of the parameters required by the RvRtpOpenFrom() function. 
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpGetAllocationSize(void);

/************************************************************************************
 * RvRtpOpenFrom
 * description: Opens an RTP session in the memory that the application allocated.
 * input: pRtpAddress contains The UDP port number to be used for the RTP session.
 *        ssrcPattern - Synchronization source Pattern value for the RTP session.
 *        ssrcMask    - Synchronization source Mask value for the RTP session.
 *        buffer      - Application allocated buffer with a value no less than the
 *                      value returned by the function RvRtpGetAllocationSize().
 *        bufferSize  - size of the buffer.
 * output: none.
 * return value: If no error occurs, the function returns the handle for the opened RTP
 *               session. Otherwise, it returns NULL.
 *Note:
 *	1) RvRtpOpenFrom opens an RTP session in the memory that the application!!! allocated.
 *     therefore RvRtpSessionShutdown should not be used.
 *  2) RvRtpOpenFrom opens one socket with the same port for receiving and for sending.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvRtpSession RVCALLCONV rtpOpenFrom(
									IN  RvUint16    port,
									IN  RvUint32    ssrcPattern,
									IN  RvUint32    ssrcMask,
									IN  void*       buffer,
									IN  RvInt32     bufferSize);
#endif
RVAPI
RvRtpSession RVCALLCONV RvRtpOpenFrom(
	IN  RvNetAddress*	pRtpAddress,
    IN  RvUint32		ssrcPattern,
    IN  RvUint32		ssrcMask,
    IN  void*			buffer,
    IN  RvInt32         bufferSize);

/************************************************************************************
 * RvRtpOpen
 * description: Opens an RTP session. The RTP Stack allocates an object and the
 *              memory needed for the RTP session. It also opens a socket and waits
 *              for packets. RvRtpOpen() also returns the handle of this session to
 *              the application.
 * input: pRtpAddress contains  the UDP port number to be used for the RTP session.
 *        ssrcPattern - Synchronization source Pattern value for the RTP session.
 *        ssrcMask    - Synchronization source Mask value for the RTP session.
 * output: none.
 * return value: If no error occurs, the function returns the handle for the opened RTP
 *               session. Otherwise, it returns NULL.
 * Note:
 *     RvRtpOpen opens one socket with the same port for receiving and for sending.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvRtpSession RVCALLCONV rtpOpen(
								IN  RvUint16    port,
								IN  RvUint32    ssrcPattern,
								IN  RvUint32    ssrcMask);
#endif
RVAPI
RvRtpSession RVCALLCONV RvRtpOpen(
	IN  RvNetAddress* pRtpAddress,
    IN  RvUint32    ssrcPattern,
    IN  RvUint32    ssrcMask);

/************************************************************************************
 * RvRtpOpenEx
 * description: Opens an RTP session and an associated RTCP session.
 * input: pRtpAddress contains  the UDP port number to be used for the RTP session.
 *        ssrcPattern - Synchronization source Pattern value for the RTP session.
 *        ssrcMask    - Synchronization source Mask value for the RTP session.
 *        cname       - The unique name representing the source of the RTP data.
 * output: none.
 * return value: If no error occurs, the function returns the handle for the open
 *               RTP session. Otherwise, the function returns NULL.
 * Note:
 * RvRtpOpenEx opens one socket for RTP session with the same port for receiving
 * and for sending, and one for RTCP session with the next port for receiving
 * and for sending.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvRtpSession RVCALLCONV rtpOpenEx(
								  IN  RvUint16    port,
								  IN  RvUint32    ssrcPattern,
								  IN  RvUint32    ssrcMask,
								  IN  char *      cname);
#endif
RVAPI
RvRtpSession RVCALLCONV RvRtpOpenEx(
	IN  RvNetAddress* pRtpAddress,
    IN  RvUint32    ssrcPattern,
    IN  RvUint32    ssrcMask,
    IN  char *  cname);

/************************************************************************************
 * RvRtpOpenEx2
 * description: Opens an RTP session just like the RvRtpOpenFrom does.
 *              The RTP Stack allocates an object and the memory needed for
 *              the RTP session.
 *              To differ from the RvRtpOpenFrom, the RvRtpOpenEx2 uses
 *              the transport provided by the application in order to send/recv
 *              packets, so it doesn't open socket.
 *
 *              In addition the RvRtpOpenEx2 opens the associated RTCP session,
 *              if the cname argument is not NULL. Just like the RvRtpOpenEx
 *              does. To differ from the RvRtpOpenEx, the RvRtpOpenEx2 uses
 *              transport provided by the application in order to exchange RTCP
 *              packets. See the rtcpTransp argument.
 *
 * input: transp      - The transport.
 *                      The application can get this transport from the ICE
 *                      Stack, for example. Or it may implement it's own type of
 *                      transport, using the abstract transport API defined
 *                      in the rvtransport.h file.
 *        ssrcPattern - Synchronization source Pattern value for the RTP session.
 *        ssrcMask    - Synchronization source Mask value for the RTP session.
 *        cname       - The unique name representing the source of the RTP data.
 *                      If NULL, no associated RTCP session will be opened.
 *        rtcpTransp  - The transport to be used by the associated RTCP session.
 * output: none.
 * return value: the opened RTP session handle, if no error occurs. O/w - NULL.
 * Note:
 *     RvRtpOpenEx2 uses same transport for receiving and for sending.
 ***********************************************************************************/
RVAPI
RvRtpSession RVCALLCONV RvRtpOpenEx2(
	    IN  RvTransport     transp,
        IN  RvUint32        ssrcPattern,
        IN  RvUint32        ssrcMask,
        IN  char *          cname,
        IN  RvTransport     rtcpTransp);

/************************************************************************************
 * RvRtpGetRtpSequenceNumber
 * description: This routine gets the RTP sequence number for packet that will be sent
 * input: hRTP  - Handle of the RTP session.
 * return value:  If no error occurs, the function returns RTP sequence number.
 *                Otherwise, it returns 0.
 * Note: For SRTP implementation use this function only after RvSrtpConstruct().
 *       RvSrtpConstruct() restricts sequence number to be from 0 to 2^15-1 in order
 *       to avoid packet lost problem, when initial sequence number is 0xFFFF, 
 *       see user guide for clarification.
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV  RvRtpGetRtpSequenceNumber(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpClose
 * description: Close RTP session.
 * input: hRTP - Handle of the RTP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 * Note: RvRtpClose should not be called immediately if RvRtcpSessionShutdown was called,
 *       only after RvRtpShutdownCompleted_CB execution.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpClose(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpGetSSRC
 * description: Returns the current SSRC (synchronization source value) of the RTP session.
 * input: hRTP - Handle of the RTP session.
 * output: none.
 * return value: The function returns the current SSRC value.
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpGetSSRC(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpSetEventHandler
 * description: Set an Event Handler for the RTP session. The application must set
 *              an Event Handler for each RTP session.
 * input: hRTP          - Handle of the RTP session.
 *        eventHandler  - Pointer to the callback function that is called each time a
 *                        new RTP packet arrives to the RTP session.
 *        context       - The parameter is an application handle that identifies the
 *                        particular RTP session. The application passes the handle to
 *                        the Event Handler.
 * output: none.
 * return value: none.
 ***********************************************************************************/

RVAPI
void RVCALLCONV RvRtpSetEventHandler(
        IN  RvRtpSession        hRTP,
        IN  RvRtpEventHandler_CB  eventHandler,
        IN  void *             context);

/************************************************************************************
 * RvRtpSetShutdownCompletedEventHandler
 * description: Set an shutdown completed Event Handler for the RTP session. 
 *              The application must set
 *              this Event Handler for each RTP session, if RvRtpSessionShutdown
 *              will be used.
 * input: hRTP          - Handle of the RTP session to be called with the eventHandler.
 *        hRTCP         - Handle of the RTCP session.
 *        eventHandler  - Pointer to the callback function that is called each time  when
 *                        shutdown is completed.
 *        context       - The parameter is an application handle that identifies the
 *                        particular RTP session. The application passes the handle to
 *                        the Event Handler.
 * output: none.
 * return value: none.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtpSetShutdownCompletedEventHandler(
        IN  RvRtpSession        hRTP,
        IN  RvRtcpSession       hRTCP,
        IN  RvRtpShutdownCompleted_CB  eventHandler,
        IN  void *                     context);

/*******************************************************************************************
 * RvRtpSessionShutdown
 * description:  Sends a BYE packet after the appropriate (algorithmically calculated) delay.
 * input:  hRTCP - Handle of RTP session.
 *         reason - for BYE message
 *         reasonLength - length of reason
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 * Notes:
 *  1)  Don't release the reason pointer until RvRtpShutdownCompleted_CB is callback called
 *  2)  Don't call to RvRtpClose until RvRtpShutdownCompleted_CB is callback called
 ******************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSessionShutdown(
		IN RvRtpSession  hRTP, 
		IN RvChar *reason, 
		IN RvUint8 reasonLength);
/************************************************************************************
 * RvRtpSetRemoteAddress
 * description: Defines the address of the remote peer or the address of a multicast
 *              group to which the RTP stream will be sent.
 * input: hRTP  - Handle of the RTP session.
 *        pRtpAddress contains
 *            ip    - IP address to which RTP packets should be sent.
 *            port  - UDP port to which RTP packets should be sent.
 * output: none.
 * return value: none.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
void RVCALLCONV rtpSetRemoteAddress(
									IN RvRtpSession  hRTP,   /* RTP Session Opaque Handle */
									IN RvUint32     ip,
									IN RvUint16     port);
#endif
RVAPI
void RVCALLCONV RvRtpSetRemoteAddress(
        IN  RvRtpSession  hRTP,
		IN  RvNetAddress* pRtpAddress);
/************************************************************************************
 * RvRtpAddRemoteAddress
 * description: Adds the new RTP address of the remote peer or the address of a multicast
 *              group or of multiunicast address list to which the RTP stream will be sent.
 * input: hRTP  - Handle of the RTP session.
 *        pRtpAddress contains
 *            ip    - IP address to which RTP packets should be sent.
 *            port  - UDP port to which RTP packets should be sent.
 * output: none.
 * return value: none.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtpAddRemoteAddress(
	IN RvRtpSession  hRTP,
	IN RvNetAddress* pRtpAddress);
/************************************************************************************
 * RvRtpRemoveRemoteAddress
 * description: removes the specified RTP address of the remote peer or the address of a multicast
 *              group or of multiunicast address list to which the RTP stream was sent.
 * input: hRTP  - Handle of the RTP session.
 *        pRtpAddress contains
 *            ip    - IP address to which RTP packets should be sent.
 *            port  - UDP port to which RTP packets should be sent.
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpRemoveRemoteAddress(
	IN RvRtpSession  hRTP,
	IN RvNetAddress* pRtpAddress);
/************************************************************************************
 * RvRtpRemoveAllRemoteAddresses
 * description: removes all RTP addresses of the remote peer or the address 
 *              of a multicast group or of multiunicast address list to which the 
 *              RTP stream was sent.
 * input: hRTP  - Handle of the RTP session.
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpRemoveAllRemoteAddresses(IN RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpWrite
 * description: This routine sends RTP packet.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf (RTP header + RTP data).
 *        p     - pointer to a struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * NOTES:
 * 1. In case of encryption according to RFC 3550 (9. Security) or in case
 *   of SRTP p.len must contain the actual length of buffer, which must be
 *   more then len, because of encryption padding or SRTP authentication tag (recommended)
 *   or/and SRTP MKI string (optional).
 * 2. When NAT/FW traversal of RTP/RTCP according to H.460.19-MA is supported
 *    (__H323_NAT_FW__ is defined  in rvrtpconfig.h file). 
 *    RvRtpNatMultiplexIdSize() function have to be used before RvRtpXXXPack() function such way:
 *    p->sByte += RvRtpNatMultiplexIdSize(); in order to allocate multiplexerId before the RTP packet.
 * 3. RvRtpWrite changes struct on which points p for internal purposes
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpWrite(
        IN     RvRtpSession   hRTP,
        IN     void  *        buf,
        IN     RvInt32        len,
        INOUT  RvRtpParam *   p);



/************************************************************************************
 * RvRtpWriteEx 
 * description: This routine is the Extension of RvRtpWrite. 
 *				It does the same as RvRtpWrite plus permites to use a SSRC,
 *				specified as a parameter instead of default sessions's ssrc
 * input: hRTP  - Handle of the RTP session.
 *		  ssrc  - SSRC of the stream, explicitly set instead of default RTP session ssrc
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf (RTP header + RTP data).
 *        p     - pointer to a struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpWriteEx(
        IN     RvRtpSession   hRTP,
		IN	   RvInt32		  ssrc, 
        IN     void  *        buf,
        IN     RvInt32        len,
        INOUT  RvRtpParam *   p);


/************************************************************************************
 * RvRtpPack
 * description: This routine sets the RTP header.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        p     - A struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpPack(
        IN  RvRtpSession  hRTP,
        IN  void *       buf,
        IN  RvInt32      len,
        IN  RvRtpParam *   p);



/************************************************************************************
 * RvRtpPackEx
 * description: This routine is the Extension of RvRtpPack. 
 *				It does the same as RvRtpPack plus permites to use a SSRC,
 *				specified as a parameter instead of default sessions's src
 * input: hRTP  - Handle of the RTP session.
 *		  ssrc  - the stream source to be used as SSRC of the packet
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        p     - A struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpPackEx(
        IN  RvRtpSession  hRTP,
		IN  RvInt32       ssrc,
        IN  void *        buf,
        IN  RvInt32       len,
        IN  RvRtpParam *  p);


/************************************************************************************
 * RvRtpUnpack
 * description: Gets the RTP header from a buffer. 
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet
 *        len   - The length in bytes of the RTP packet.
 *        p     - A structure of rtpParam with the RTP header information.
 * output: none.
 * return value:  If an error occurs, the function returns a negative value.
 *                If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/

RVAPI
RvInt32 RVCALLCONV RvRtpUnpack(
        IN  RvRtpSession     hRTP,
        IN  void *          buf,
        IN  RvInt32         len,
        OUT RvRtpParam*       p);


/************************************************************************************
 * RvRtpRead
 * description: This routine reads the RTP message and sets the header of the RTP message.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *
 * output: p    - A struct of RTP param,contain the fields of RTP header.
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Notes: 
 * 1) This function returns an error, when RvRtpWrite was called to unreachable destination.
 * 2) This function returns an error, when RvRtpClose was called to close the session 
 *    (for exiting from blocked mode).
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpRead(
        IN   RvRtpSession  hRTP,
        IN   void *       buf,
        IN   RvInt32      len,
        OUT  RvRtpParam *   p);

/************************************************************************************
 * RvRtpReadEx
 * description: Receives an RTP packet and updates the corresponding RTCP session.
 * input: hRTP      - Handle of the RTP session.
 *        buf       - Pointer to buffer containing the RTP packet with room before first
 *                    payload byte for RTP header.
 *        len       - Length in bytes of buf.
 *        timestamp - The local RTP timestamp when the received packet arrived. 
 *        p         - A struct of RTP param,contain the fields of RTP header.
 * output: none.
 * return value: If no error occurs, the function returns the non-negative value.
 *               Otherwise, it returns a negative value.
 * Notes: 
 * 1) This function returns an error, when RvRtpWrite was called to unreachable destination.
 * 2) This function returns an error, when RvRtpClose was called to close the session 
 *    (for exiting from blocked mode).
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpReadEx(
        IN   RvRtpSession  hRTP,
        IN   void *       buf,
        IN   RvInt32      len,
        IN   RvUint32     timestamp,
        OUT  RvRtpParam *   p);

/************************************************************************************
 * RvRtpReadWithRemoteAddress
 * description: This routine reads the RTP message and sets the header of the RTP message.
 *              It also retrieves the address of the RTP message sender.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *
 * output: p            - A struct of RTP param,contain the fields of RTP header.
 *        remAddressPtr - pointer to the remote address
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpReadWithRemoteAddress(
		IN  RvRtpSession  hRTP,
		IN  void *buf,
		IN  RvInt32 len,
		OUT RvRtpParam* p,
		OUT RvNetAddress* remAddressPtr);

/************************************************************************************
 * RvRtpGetPort
 * description: Returns the current port of the RTP session.
 * input: hRTP - Handle of the RTP session.
 * output: none.
 * return value: If no error occurs, the function returns the current port value.
 *               Otherwise, it returns 0.
 ***********************************************************************************/
RVAPI
RvUint16 RVCALLCONV RvRtpGetPort(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpGetVersion
 * description:  Returns the RTP version of the installed RTP Stack.
 * input:  none.
 * output: none.
 * return value: If no error occurs, the function returns the current version value.
 *               Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
char * RVCALLCONV RvRtpGetVersion(void);

/************************************************************************************
 * RvRtpGetVersionNum
 * description:  Returns the RTP version number of the installed RTP Stack.
 * input:  none.
 * output: none.
 * return value: If no error occurs, the function returns the current version value.
 *               Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpGetVersionNum(void);


                    /* == ENDS: Basic RTP Functions == */



                     /* == Accessory RTP Functions == */

/************************************************************************************
 * RvRtpGetRTCPSession
 * description:  Returns the RTCP session.
 * input:  hRTP - Handle of RTP session.
 * output: none.
 * return value: hRTCP - Handle of RTCP session.
 ***********************************************************************************/
RVAPI
RvRtcpSession RVCALLCONV RvRtpGetRTCPSession(
        IN  RvRtpSession  hRTP);


/************************************************************************************
 * RvRtpSetRTCPSession
 * description:  set the RTCP session.
 * input:  hRTP  - Handle of RTP session.
 *         hRTCP - Handle of RTCP session.
 * output: none.
 * return value:return 0.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetRTCPSession(
        IN  RvRtpSession   hRTP,
        IN  RvRtcpSession  hRTCP);

/************************************************************************************
 * RvRtpGetHeaderLength
 * description:  return the header of RTP message.
 * input:  none.
 * output: none.
 * return value:The return value is twelve.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpGetHeaderLength(void);

/************************************************************************************
 * RvRtpRegenSSRC
 * description:  Generates a new synchronization source value for the RTP session.
 *               This function, in conjunction with RvRtpGetSSRC() may be used to
 *               change the SSRC value when an SSRC collision is detected.
 * input:  hRTP  - Handle of RTP session.
 * output: none.
 * return value: ssrc 
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpRegenSSRC(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpSetGroupAddress
 * description:  Defines a multicast IP for the RTP session.
 * input:  hRTP  - Handle of RTP session.+
 *		   pRtpAddress - pointer to RvNetAddress contains
 *						 Multicast IP address for the RTP session.
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *               Otherwise, it returns a negative value.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtpSetGroupAddress(
									  IN RvRtpSession hRTP,    /* RTP Session Opaque Handle */
									  IN RvUint32     ip);
#endif
RVAPI
RvInt32 RVCALLCONV RvRtpSetGroupAddress(
        IN  RvRtpSession  hRTP,
		IN  RvNetAddress* pRtpAddress);
/************************************************************************************
 * RvRtpSetMulticastTTL
 * description:  Defines a multicast Time To Live (TTL) for the RTP session 
 *               (multicast sending).
 * input:  hRTP  - Handle of RTP session.+
 *         ttl   - [0..255] ttl threshold for multicast
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: the function is supported with IP stack that has full support of multicast
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetMulticastTTL(
		IN  RvRtpSession  hRTP,
		IN  RvUint8       ttl);
/************************************************************************************
 * RvRtpResume
 * description:  Causes a blocked RvRtpRead() or RvRtpReadEx() function running in
 *               another thread to fail.
 * input:  hRTP  - Handle of RTP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpResume(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpUseSequenceNumber
 * description:  Forces the Stack to accept user input for the sequence number of
 *               the RTP packet. The RTP Stack usually determines the sequence number.
 *               However, the application can force its own sequence number.
 *               Call RvRtpUseSequenceNumber() at the beginning of the RTP session and
 *               then specify the sequence number in the RvRtpParam structure of the
 *               RvRtpWrite() function.
 * input:  hRTP  - Handle of RTP session.
 * output: none.
 * return value: return 0.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpUseSequenceNumber(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpGetRtpSequenceNumber
 * description: This routine gets the RTP sequence number for packet that will be sent
 * input: hRTP  - Handle of the RTP session.
 * return value:  If no error occurs, the function returns RTP sequence number.
 *                Otherwise, it returns 0.
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV  RvRtpGetRtpSequenceNumber(
        IN  RvRtpSession  hRTP);

/************************************************************************************
 * RvRtpSetReceiveBufferSize
 * description:  Changes the RTP session receive buffer size.
 * input:  hRTP  - Handle of RTP session.
 * output: none.
 * return value: return 0.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetReceiveBufferSize(
        IN RvRtpSession  hRTP,
        IN RvInt32 size);

/************************************************************************************
 * RvRtpSetTransmitBufferSize
 * description:  Changes the RTP session transmit buffer size.
 * input:  hRTP - Handle of RTP session.
 *         size - The new transmit buffer size.
 * output: none.
 * return value: return 0.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetTransmitBufferSize(
                IN RvRtpSession  hRTP,
                IN RvInt32 size);


/************************************************************************************
 * RvRtpSetTrasmitBufferSize
 * description:  Changes the RTP session transmit buffer size.
 * input:  hRTP - Handle of RTP session.
 *         size - The new transmit buffer size.
 * output: none.
 * return value: return 0.
 * comment     : obsolete function provided for compatibility with prev. version
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetTrasmitBufferSize(
                IN RvRtpSession  hRTP,
                IN RvInt32 size);


/************************************************************************************
 * RvRtpGetAvailableBytes
 * description:  Gets the number of bytes available for reading in the RTP session.
 * input:  hRTP - Handle of RTP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpGetAvailableBytes(
                IN RvRtpSession  hRTP);


         
#if defined(RVRTP_SECURITY)
/************************************************************************************
 * RvRtpSetEncryption
 * description:  Sets encryption parameters for RTP session
 * input:  hRTP - Handle of RTP session.
 *         mode - encryption mode for the session
 *         enctyptorPlugInPtr - pointer to user defined encryption plug in
 *         keyPlugInPtr       - pointer to user defined key manager plug in
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: if corresponding automatic RTCP session was opened
 *       the same encryption and decryption plugins and mode are set for it too.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtpSetEncryption(       
	IN RvRtpSession hRTP, 
	IN RvRtpEncryptionMode mode,
	IN RvRtpEncryptionPlugIn* enctyptorPlugInPtr,
	IN RvRtpEncryptionKeyPlugIn* keyPlugInPtr);

/************************************************************************************
 * RvRtpSetDoubleKeyEncryption
 * description:  Sets encryption plug-in and encryption/decryption keys for RTP session.
 *               This function is only an example of usage of RvRtpDoubleKey plugin.
 *               Pay attention that this implementation allocates RvRtpDoubleKey plugin and
 *               releases it, when RvRtpClose is called.
 *               User can implement other key management plugin and call to RvRtpSetEncryption()
 * input:  hRTP - Handle of RTP session.
 *         mode - encryption mode for the session
 *         pEKey - pointer to encryption key
 *         pDKey - pointer to decryption key
 *         pCrypto - pointer to encryption plug in
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: if corresponding automatic RTCP session was opened
 *       the same encryption and decryption keys, encryption plug in and mode are set for it too.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtpSetDoubleKeyEncryption(
    IN RvRtpSession hRtp,
    IN RvRtpEncryptionMode mode,
    IN RvKey *pEKey,
    IN RvKey *pDKey,
    IN RvRtpEncryptionPlugIn *pCrypto);

#endif

/************************************************************************************
 * RvRtpSetEncryptionNone
 * description:  Cancels encryption for the session
 * input:  hRTP - Handle of RTP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: if corresponding automatic RTCP session was opened RvRtpSetEncryptionNone
 *       cancel encryption for it too.
 ***********************************************************************************/    
RVAPI
RvStatus RVCALLCONV RvRtpSetEncryptionNone(IN RvRtpSession hRTP);
/************************************************************************************
 * RvRtpSessionSetEncryptionMode
 * description:  Sets encryption mode for the session
 * input:  hRTP - Handle of RTP session.
 *         mode - encryption mode
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: if corresponding automatic RTCP session was opened RvRtpSessionSetEncryptionMode
 *       sets encryption moden for it too.
 ***********************************************************************************/ 
#if defined(RVRTP_SECURITY)
RVAPI
RvStatus RVCALLCONV RvRtpSessionSetEncryptionMode(IN RvRtpSession hRTP, IN RvRtpEncryptionMode mode);
#endif

/********************************************************************************************
 * RvRtpSetTypeOfService
 * Set the type of service (DiffServ Code Point) of the socket (IP_TOS)
 * This function is supported by few operating systems.
 * IPV6 does not support type of service.
 * This function is thread-safe.
 * INPUT   : hRTP           - RTP session to set TOS byte
 *           typeOfService  - type of service to set
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 * Remark: In Windows environment for setting the TOS byte RvRtpOpen or RvRtpOpenEx
 *         must be called with local IP address and not with "any" address.
 ********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSetTypeOfService(
        IN RvRtpSession hRTP,
        IN RvInt32        typeOfService);

/********************************************************************************************
 * RvRtpGetTypeOfService
 * Get the type of service (DiffServ Code Point) of the socket (IP_TOS)
 * This function is supported by few operating systems.
 * IPV6 does not support type of service.
 * This function is thread-safe.
 * INPUT   : hRTP           - RTP session handle
 * OUTPUT  : typeOfServicePtr  - pointer to type of service to set
 * RETURN  : RV_OK on success, other on failure
 *********************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpGetTypeOfService(
		IN RvRtpSession hRTP,
		OUT RvInt32*     typeOfServicePtr);


/********************************************************************************************
 * RvRtpSetSocketBlockingMode
 * Description: Sets blocking mode for session socket
 * This function is thread-safe.
 * INPUT   : hRTP           - RTP session handle
 *         : bIsBlocking    - Blocking mode
 * RETURN  : RV_OK on success, other on failure
 *********************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpSetSocketBlockingMode(
		IN RvRtpSession hRTP,
		IN RvBool       bIsBlocking);

/********************************************************************************************
* RvRtpSessionGetSocket
* Description: Gets a socket used for this session
* INPUT   : hRTP           - RTP session handle
* RETURN  : socket
*********************************************************************************************/
RVAPI
RtpSocket RVCALLCONV RvRtpSessionGetSocket(IN RvRtpSession hRTP);


RVAPI
RvStatus RVCALLCONV RvRtpSessionSetSocket(
										  IN RvRtpSession hRTP, 
										  IN RtpSocket	  *socket);


/********************************************************************************************
* RvRtpSessionSetSocketEx
* Description: Set the socket to be used for this session and
*			   Also bind the socket to specified address.
* INPUT   : hRTP           - RTP session handle
*         : socket         - pointef to the socket to be used in this session 
*         : address        - pointer to address to which the socket is bound to. 			
* RETURN  : RV_OK on success, other on failure
*********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSessionSetSocketEx(
			IN RvRtpSession hRTP, 
			IN RtpSocket	*socket,  
			IN RvAddress	*address);



RVAPI
RvStatus RVCALLCONV RvRtpSessionSendKeepAlive(
									  IN RvRtpSession  hRTP, 
									  RvUint32 unknownPayload);
											  

/* == ENDS: Accessory RTP Functions == */
#ifdef __cplusplus
}
#endif

#endif  /* __RTP_H */










































