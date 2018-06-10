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

#ifndef __RTCP_H
#define __RTCP_H

#include "rvtypes.h"
#include "rtcpbasetypes.h"
#include "rvrtpinterface.h"
#include "rvnetaddress.h"

#if defined(RVRTP_SECURITY)
#include "rvkey.h"
#endif
#include "rvrtpencryptionmode.h"

	
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __RTP_H
RV_DECLARE_HANDLE(RvRtcpSession);
#define RVVXDAPI RVAPI
#define VXDCALLCONV RVCALLCONV
#define CALLCONVC
#endif


#define ERR_RTCP_GENERALERROR     -350   /* general error */
#define ERR_RTCP_SSRCCOLLISION    -351   /* ssrc collision */
#define ERR_RTCP_ILLEGALSSRC      -352   /* an illegal ssrc was specified */
#define ERR_RTCP_ILLEGALSDES      -353   /* an illegal sdes was specified */
#define ERR_RTCP_ILLEGALPACKET    -354   /* illegal RTCP pkt encountered */
#define ERR_RTCP_SHUTDOWN         -355   /* shutdown state - to send packet is not permitted */




typedef RvInt32 RTCP_ReportType;


typedef enum RvRtpParticipant_
{
	RVRTP_PARTICIPANT_ACTIVESENDER,
	RVRTP_PARTICIPANT_RECEIVER
} RvRtpParticipant;

typedef enum RvRtcpParameters_
{
	RVRTCP_PARAMS_RTPCLOCKRATE = 0,
	RVRTCP_PARAMS_RTP_PAYLOADTYPE
} RvRtcpParameters;		

typedef enum RvRtcpRoundTripDelaySourceReporter__
{
   	RVRTCP_ROUND_TRIP_DELAY_RTCPSR = 0,
	RVRTCP_ROUND_TRIP_DELAY_RTCPXR
} RvRtcpRoundTripDelaySourceReporter;

typedef void (RVCALLCONV *RvRtcpEventHandler_CB)(
        IN RvRtcpSession,
        IN void * context,
        IN RvUint32 ssrc);
//mod lishixin add four params
typedef void (RVCALLCONV *RvRtcpEventHandlerEx_CB)(
		IN RvRtcpSession,
		IN void * context,
		IN RvUint32 ssrc,
		IN RvUint8 *,
		IN RvUint32,
		IN RvUint8 *,
		IN RvUint16,
		IN RvBool,
		IN RvUint32);

typedef void (RVCALLCONV *RvRtcpSendHandlerEx_CB)(
	    IN RvRtcpSession,
/*h.e 22.02.07*/
		IN void * context,
		IN RvUint32 ssrc,
/*===*/
		IN RvUint8 *,
		IN RvUint32);

typedef RvBool (RVCALLCONV *RvRtcpSSRCENUM_CB)(
        IN  RvRtcpSession  hRTCP,
        IN  RvUint32      ssrc);

typedef RvBool (RVCALLCONV *RvRtcpAppEventHandler_CB)(
		IN  RvRtcpSession  hRTCP,
		IN  void * rtcpAppContext,
		IN  RvUint8        subtype,
		IN  RvUint32       ssrc,
		IN  RvUint8*       name,
		IN  RvUint8*       userData, 
		IN  RvUint32       userDataLen); /* userData in bites!, not in 4 octet words */

typedef RvBool (RVCALLCONV *RvRtcpByeEventHandler_CB)(
		IN  RvRtcpSession  hRTCP,
		IN  void * rtcpByeContext,
		IN  RvUint32       ssrc,
		IN  RvUint8*       userReason, 
		IN  RvUint32       userReasonLen); /* userData in bites!, not in 4 octet words */

/******************************************************************************
 * RvRtcpSDESMessageReceived_CB
 * ----------------------------------------------------------------------------
 * General: Indication of receiving SDES RTCP report with specified type
 * Return Value: None
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input: hRTCP    - The RvRtcpSession handle.
 *       SDESType  - The SDES report type
 *       SSRC      - RTP source 
 *       SDES      - pointer to Source Description
 *       SDES_len  - length of SDES in bytes                        
 *****************************************************************************/

typedef void (RVCALLCONV *RvRtcpSDESMessageReceived_CB)(
	IN  RvRtcpSession  hRTCP,
	IN  RvRtcpSDesType SDESType,
	IN  RvUint32       ssrc,
	IN  RvUint8*       SDES, 
	IN  RvUint32       SDES_len); /* userData in bites!, not in 4 octet words */
	

typedef void (RVCALLCONV *RvRtcpSDESMessageReceivedEx_CB)(
	IN  RvRtcpSession  hRTCP,
	IN  void *		   rtcpSDESContext,
	IN  RvRtcpSDesType SDESType,
	IN  RvUint32       ssrc,
	IN  RvUint8*       SDES, 
	IN  RvUint32       SDES_len); /* userData in bites!, not in 4 octet words */
	


/******************************************************************************
 * RvRtcpSRRRMessageReceived_CB
 * ----------------------------------------------------------------------------
 * General: Indication of receiving SR/RR RTCP messages 
 * Return Value: None
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input: hRTCP    - The RvRtcpSession handle.
 *       reportType  - The Sender or Receiver report type
 *       ssrc      - RTP source 
 *****************************************************************************/

typedef void (RVCALLCONV *RvRtcpSRRRMessageReceived_CB)(
	IN  RvRtcpSession	hRTCP,
	IN  RTCP_ReportType	reportType,
	IN  RvUint32        ssrc);


/******************************************************************************
 * RvRtcpSRRRMessageReceivedEx_CB
 * ----------------------------------------------------------------------------
 * General: Indication of receiving SR/RR RTCP messages 
 * Return Value: None
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input: hRTCP     - The RvRtcpSession handle.
 *       reportType - The Sender or Receiver report type
 *       ssrc       - RTP source 
 *       context    - the application context
 *****************************************************************************/

typedef void (RVCALLCONV *RvRtcpSRRRMessageReceivedEx_CB)(
	IN  RvRtcpSession	hRTCP,
	IN  RTCP_ReportType	reportType,
	IN  RvUint32        ssrc,
	IN  void			*context); 



/********************************************************************************
 Callback type RvRtcpRoundTripDelay_CB
 Description:
 The following callback passes to an application the round trip delay in ms. 
   Input Parameters:
      hRTCP                - the RTCP session handle
      ssrc                 - the remote SSRC
      roundTripDelayMs     - the round trip delay in ms.
      sourceOfreport       - the source of report (RTCP Sender and Receiver reports (RFC 3550)
                                                   or RTCP Extended Reports (RFC 3611))                                                   
*********************************************************************************/
typedef void (RVCALLCONV *RvRtcpRoundTripDelay_CB)(
        IN RvRtcpSession                      hRTCP,
        IN RvUint32                           ssrc,
        IN RvUint32                           roundTripDelayMs,
        IN RvRtcpRoundTripDelaySourceReporter sourceOfreport);


/********************************************************************************
 Callback type RvRtcpRoundTripDelayEx_CB
 Description:
 The following callback passes to an application the round trip delay in ms. 
   Input Parameters:
      hRTCP						- the RTCP session handle
	  rtcpRoundTripDelayContext	- Application context 
      ssrc						- the remote SSRC
      roundTripDelayMs			- the round trip delay in ms.
      sourceOfreport			- the source of report (RTCP Sender and Receiver reports (RFC 3550)
                                                   or RTCP Extended Reports (RFC 3611))                                                   
*********************************************************************************/
typedef void (RVCALLCONV *RvRtcpRoundTripDelayEx_CB)(
        IN RvRtcpSession                      hRTCP,
		IN void *							  rtcpRoundTripDelayContext,
        IN RvUint32                           ssrc,
        IN RvUint32                           roundTripDelayMs,
        IN RvRtcpRoundTripDelaySourceReporter sourceOfreport);


/********************************************************************************
 Callback type RvRtcpSsrcRemove_CB
 Description:
 This callback informs an application than SSRC was removed from remote participants list
 (when an SSRC does not send RTCP packets for 5 "RTCP intervals")
   Input Parameters:
      hRTCP			- the RTCP session handle
	  context		- Application context 
      ssrc			- the remote SSRC
*********************************************************************************/
typedef RvBool (RVCALLCONV *RvRtcpSsrcRemove_CB)(
        IN RvRtcpSession				hRTCP,
		IN void *						context,
        IN RvUint32                     ssrc);

typedef struct {
    RvBool      valid; /* RV_TRUE if this struct contains valid information. */
    RvUint32    mNTPtimestamp; /* Most significant 32bit of NTP timestamp */
    RvUint32    lNTPtimestamp; /* Least significant 32bit of NTP timestamp */
    RvUint32    timestamp; /* RTP timestamp */
    RvUint32    packets; /* Total number of RTP data packets transmitted by the sender
                            since transmission started and up until the time this SR packet
                            was generated. */
    RvUint32    octets; /* The total number of payload octets (not including header or padding */
} RvRtcpSRINFO;

typedef struct {
    RvBool      valid; /* RV_TRUE if this struct contains valid information. */
    RvUint32    fractionLost; /* The fraction of RTP data packets from source specified by
                                 SSRC that were lost since previous SR/RR packet was sent. */
    RvUint32    cumulativeLost; /* Total number of RTP data packets from source specified by
                                   SSRC that have been lost since the beginning of reception. */
    RvUint32    sequenceNumber; /* Sequence number that was received from the source specified
                                   by SSRC. */
    RvUint32    jitter; /* Estimate of the statistical variance of the RTP data packet inter
                           arrival time. */
    RvUint32    lSR; /* The middle 32 bits of the NTP timestamp received. */
    RvUint32    dlSR; /* Delay since the last SR. */
} RvRtcpRRINFO;

typedef struct {
    RvBool      selfNode; /* RV_TRUE if this structure contains information about a source
                             created by this session. In this case, only sr and cname fields
                             are relevant in the structure. */
    RvRtcpSRINFO  sr; /* Sender report information. For selfNode==RV_TRUE, this field contains
                       the report that will be sent to other sources. Otherwise, this is the
                       last report received from remote sources. */
    RvRtcpRRINFO  rrFrom; /* Receiver report information. The last, if any, receiver report
                           received from the source identified by the ssrc parameter about the
                           source that created this session. */
    RvRtcpRRINFO  rrTo; /* Local receiver report information. Information about the source
                         identifier by the ssrc parameter about the source that created this
                         session. */
    char        cname[255]; /* cname of the source that is identified by the ssrc parameter */
} RvRtcpINFO;

/* structure for one APP RTCP message */
typedef struct compoundAppMessage_
{
	RvUint8  subtype;		/* 5 least significant bits are used */
	RvUint8  name[4];     /* 4 octet name */
	RvUint8* userData;	/* multiple by 4 octet user data */
	RvUint32 userDataLength;
	
} RvRtcpAppMessage;

                      /* == Basic RTCP Functions == */
/************************************************************************************
 * RvRtcpInit
 * description: Initializes the RTCP module. 
 * input: none.
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpInit(void);

/************************************************************************************
 * RvRtcpInitEx
 * description: Initializes the RTCP Stack and specifies the local IP address to which all
 *              RTCP sessions will be bound. 
 * input: pRtpAddress - pointer to RvNetAddress which contains 
 *         the local IPV4/6 address to which all RTCP sessions will be bound.
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Remarks 
 * - This function can be used instead of RvRtcpInit().
 * - RvRtcpInit() binds to the “any?IP address.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvStatus RVCALLCONV rtcpInitEx(IN RvUint32);
#endif

RVAPI
RvStatus RVCALLCONV RvRtcpInitEx(IN RvNetAddress* pRtpAddress);

/************************************************************************************
 * RvRtcpEnd
 * description: Shuts down the RTCP module. 
 * input: none.
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpEnd(void);


/************************************************************************************
 * RvRtcpSetManual
 * description:  Sets the manual RTCP packets transmissions (bManual=RV_TRUE)
 * input:  hRTCP	- Handle of RTCP session.
 *         bManual	- Boolean var defines mode (manual ar automatic) for RTCP transmissions
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * Note:   For RTP session to transmit Control protocol manually 
 *         this function MUST be called before!!! the remote address is set.
 ***********************************************************************************/

RVAPI
RvStatus  RVCALLCONV RvRtcpSetManual(
	IN RvRtcpSession	hRTCP, 
	IN RvBool			bManual);
									 


/************************************************************************************
 * RvRtcpSetLocalAddress
 * description: Set the local address to use for calls to rtcpOpenXXX functions.
 *              This parameter overrides the value given in RvRtcpInitEx() for all
 *              subsequent calls.
 * input: pRtpAddress contains the Local IP address to use
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvStatus RVCALLCONV rtcpSetLocalAddress(IN RvUint32 ip);
#endif
RVAPI
RvStatus RVCALLCONV RvRtcpSetLocalAddress(IN RvNetAddress* pRtpAddress);

/*******************************************************************************************
 * RvRtcpSetRTCPRecvEventHandler
 * description: Sets an event handler for the RTCP session. 
 * input: 
 *   hRTCP          - Handle of the RTCP session. 
 *   rtcpCallback   - Pointer to the callback function that is called each time 
 *                    a new RCTP packet arrives at the RCTP session. 
 *                    If eventHandler is set to NULL, the event handler is removed. 
 *                    The prototype of the callback is as follows: 
 *  
 *	                  void RVCALLCONV RvRtcpEventHandler_CB(
 *		                   IN RvRtcpSession,
 *		                   IN void * context,
 *		                   IN RvUint32 ssrc);
 *
 *	                  where: 		   
 *			                hRTCP is the handle of the RTCP session.
 *			                context is the same context passed by the application when calling 
 *                           RvRtcpSetRTCPRecvEventHandler().
 *			                ssrc is the synchronization source from which the packet was received.
 *	context - An application handle that identifies the particular RTCP session. 
 *            The application passes the handle to the event handler. The RTCP Stack 
 *            does not modify the parameter but simply passes it back to the application. 
 *			 			   
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Remarks 
 *  The application can set an event handler for each RTCP session. 
 *  The event handler will be called whenever an RTCP packet is received for this session. 
 ********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetRTCPRecvEventHandler(
    IN RvRtcpSession         hRTCP,
    IN RvRtcpEventHandler_CB   rtcpCallback,
    IN void *               context);



/*******************************************************************************************
 * RvRtcpSetRTCPRecvEventHandlerEx
 * description: Sets an extended event handler for the RTCP session. 
 * input: 
 *   hRTCP          - Handle of the RTCP session. 
 *   rtcpCallback   - Pointer to the callback function that is called each time 
 *                    a new RCTP packet arrives at the RCTP session. 
 *                    If extended eventHandler is set to NULL, the event handler is removed. 
 *                    The prototype of the callback is as follows: 
 *  
 *	                  void RVCALLCONV RvRtcpEventHandler_CB(
 *		                   IN RvRtcpSession,
 *		                   IN void * context,
 *		                   IN RvUint32 ssrc,
 *                         IN RvChar * rtcpPacket
 *						   IN RvUint32 rtcpLen);
 *
 *	                  where: 		   
 *			                hRTCP is the handle of the RTCP session.
 *			                context is the same context passed by the application when calling 
 *                           RvRtcpSetRTCPRecvEventHandler().
 *			                ssrc is the synchronization source from which the packet was received.
 *	context - An application handle that identifies the particular RTCP session. 
 *            The application passes the handle to the event handler. The RTCP Stack 
 *            does not modify the parameter but simply passes it back to the application. 
 *			 			   
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Remarks 
 *  The application can set an event handler for each RTCP session. 
 *  The event handler will be called whenever an RTCP packet is received for this session. 
 ********************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtcpSetRTCPRecvEventHandlerEx(
    IN RvRtcpSession			hRTCP,
    IN RvRtcpEventHandlerEx_CB  rtcpCallback,
    IN void *					context);



/*******************************************************************************************
 * RvRtcpSetRTCPSendHandlerEx
 * description: Sets an extended callback to be called when RTCP packet was sent. 
 * input: 
 *   hRTCP          - Handle of the RTCP session. 
 *   rtcpCallback   - Pointer to the callback function that is called each time 
 *                    a new RCTP packet was sent at the RCTP session. 
 *                    If extended eventHandler is set to NULL, the event handler is removed. 
 *                    The prototype of the callback is as follows: 
 *			 			   
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Remarks 
 *  The application can set an event handler for each RTCP session. 
 *  The event handler will be called whenever an RTCP packet is received for this session. 
 ********************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtcpSetRTCPSendHandlerEx(
    IN RvRtcpSession         hRTCP,
    IN RvRtcpSendHandlerEx_CB   rtcpCallback);

/************************************************************************************
 * RvRtcpGetAllocationSize
 * description: Returns the number of bytes required for internal RTCP session data structure.
 *              The application may allocate the requested amount of memory and use it with 
 *				the RvRtcpOpenFrom() function. 
 * input: sessionMembers        - Maximum number of members in RTP conference. 
 *
 * output: none.
 * Return Values - The function returns the number of bytes required for 
 *                 internal RTCP session data structure. 
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpGetAllocationSize(
    IN  RvInt32 sessionMembers);

/************************************************************************************
 * RvRtcpOpenFrom
 * description: Opens an RTCP session in the memory that the application allocated.
 * input: ssrc        - Synchronization source value for the RTCP session.
 *        pRtcpAddress contains the IP address (UDP port number) to be used for the RTCP session.
 *        cname       - Unique name representing the source of the RTP data. 
 *                      Text that identifies the session. Must not be NULL. 
 *        maxSessionMembers - Maximum number of different SSRC that can be handled
 *        buffer      - Application allocated buffer with a value no less than the
 *                      value returned by the function RvRtpGetAllocationSize().
 *        bufferSize  - size of the buffer.
 * output: none.
 * return value: If no error occurs, the function returns the handle for the opened RTP
 *               session. Otherwise, it returns NULL.
 * Remarks 
 *  - Before calling RvRtpOpenFrom() the application must call RvRtpGetAllocationSize() 
 *    in order to get the size of the memory allocated by the application.
 *  - The RTP port(in pRtcpAddress) should be an even number. The port for an RTCP session
 *    is always RTP port + 1.
 *  - If the port parameter is equal to zero in the RvRtpOpenFrom() call, then an arbitrary
 *    port number will be used for the new RTP session. Using a zero port number may cause
 *    problems with some codecs that anticipate the RTP and RTCP ports to be near each other.
 *    We recommend defining a port number and not using zero. 
 *  - RvRtpOpenFrom() generates the synchronization source value as follows:
 *  - ssrc = ((Random 32 bit number) AND NOT ssrcMask)) OR ssrcPattern 
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvRtcpSession RVCALLCONV rtcpOpenFrom(
									  IN  RvUint32    ssrc,
									  IN  RvUint16    port,
									  IN  char *      cname,
									  IN  RvInt32         maxSessionMembers,
									  IN  void *      buffer,
									  IN  RvInt32         bufferSize);
#endif
RVAPI
RvRtcpSession RVCALLCONV RvRtcpOpenFrom(
    IN  RvUint32    ssrc,
    IN  RvNetAddress* pRtcpAddress,
    IN  char *      cname,
    IN  RvInt32         maxSessionMembers,
    IN  void *      buffer,
    IN  RvInt32         bufferSize);


/************************************************************************************
 * RvRtcpOpen
 * description: Opens a new RTCP session. 
 * input: ssrc        - Synchronization source value for the RTCP session.
 *        pRtcpAddress contains the IP address (UDP port number) to be used for the RTCP session.
 *        cname       - Unique name representing the source of the RTP data. 
 *                      Text that identifies the session. Must not be NULL. 
 * output: none.
 * return value: If no error occurs, the function returns a handle for the
 *                new RTCP session. Otherwise it returns NULL. 
 * Remarks 
 *  - RvRtcpOpen() allocates a default number of 50 RTCP session members. 
 *    This may result in a large allocation irrespective of whether there are only a few members. 
 *	  To prevent the large allocation, use RvRtcpOpenFrom() and indicate the actual number of 
 *	  session members.
 *  - If the port parameter is equal to zero in the RvRtcpOpen() call, then an arbitrary
 *    port number will be used for the new RTCP session. Using a zero port number may 
 *    cause problems with some codecs that anticipate the RTP and RTCP ports 
 *    to be near each other. We recommend defining a port number and not using zero. 
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvRtcpSession RVCALLCONV rtcpOpen(
								  IN  RvUint32    ssrc,
								  IN  RvUint16    port,
								  IN  char *      cname);
#endif
RVAPI
RvRtcpSession RVCALLCONV RvRtcpOpen(
									IN  RvUint32    ssrc,
									IN  RvNetAddress* pRtcpAddress,
									IN  char *      cname);

/************************************************************************************
 * RvRtcpClose
 * description: Closes an RTCP session. 
 * input: hRCTP        - The handle of the RTCP session. 
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpClose(
        IN  RvRtcpSession  hRCTP);



/********************************************************************************
 *RvRtcpClean
 *Description:"cleans" Rtcp session without closing session.
 *  		   Which permits to use the same socket that RTCP run on again
 * input: hRCTP        - The handle of the RTCP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpClean(
								IN  RvRtcpSession  hRCTP);


/************************************************************************************
 * RvRtcpSetRemoteAddress
 * description: Defines the address of the remote peer or of the multicast group.  
 * input: hRCTP        - The handle of the RTCP session. 
 *        pRtcpAddress - pointer to RvNetAddress(to which packets to be sent)
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
void RVCALLCONV rtcpSetRemoteAddress(
									 IN  RvRtcpSession  hRTCP,     /* RTCP Session Opaque Handle */
									 IN  RvUint32      ip,        /* target ip address */
									 IN  RvUint16      port);
#endif

RVAPI
void RVCALLCONV RvRtcpSetRemoteAddress(
        IN  RvRtcpSession  hRTCP,
		IN  RvNetAddress* pRtcpAddress);

/************************************************************************************
 * RvRtcpAddRemoteAddress
 * description: Adds the new RTCP address of the remote peer or of the multicast group 
 *              or of the multiunicast list with elimination of address duplication.  
 * input: hRCTP        - The handle of the RTCP session. 
 *        pRtcpAddress - pointer to RvNetAddress(to which packets to be sent)
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpAddRemoteAddress(
	IN  RvRtcpSession  hRTCP,   
	IN  RvNetAddress* pRtcpAddress);

/************************************************************************************
 * RvRtcpRemoveRemoteAddress
 * description: removes the specified RTCP address of the remote peer or of the multicast group 
 *              or of the multiunicast list with elimination of address duplication.  
 * input: hRCTP        - The handle of the RTCP session. 
 *        pRtcpAddress - pointer to RvNetAddress to remove.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpRemoveRemoteAddress(
	IN  RvRtcpSession  hRTCP,   
	IN  RvNetAddress* pRtcpAddress);

/************************************************************************************
 * RvRtcpRemoveAllRemoteAddresses
 * description: removes all RTCP addresses of the remote peer or of the multicast group 
 *              or of the multi-unicast list with elimination of address duplication.  
 * input: hRCTP        - The handle of the RTCP session. 
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpRemoveAllRemoteAddresses(
	IN  RvRtcpSession  hRTCP);
/************************************************************************************
 * RvRtcpSetSDESItem
 * description: Defines and sets the SDES Items to the sessions
 * input: hRCTP        - The handle of the RTCP session (which must be opened). 
 *        SDEStype     - type of SDES item
 *        item -         string of SDES item
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 * Remark: CNAME - is set through session opening
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSDESItem(
						IN  RvRtcpSession  hRTCP,
						IN  RvRtcpSDesType SDEStype,
						IN  char* item);


/************************************************************************************
 * RvRtcpStop
 * description: Stops RTCP session without closing it. This allows multiple subsequent
 *              RTCP sessions to share the same hRTCP without need to close/open UDP socket. 
 * input: hRCTP        - The handle of the RTCP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 * Remarks 
 * - This function is used when it would be wasteful to call RvRtcpOpen() every time
 *   there is a call, for example in a gateway. 
 *   When the call ends, rtcpStop() clears the call and enables the resumption of a new call,
 * as follows: 
 *
 *	 RvRtpOpen 
 *	 RvRtcpOpen 
 *	 ....RvRtcpSetRemoteAddress 
 *	 : 
 *	 ...RvRtcpStop 
 *	 ...RvRtcpSetRemoteAddress 
 *	 : 
 *	 RvRtcpClose 
 *	 RvRtpClose 	   
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpStop(
        IN  RvRtcpSession  hRTCP);

/************************************************************************************
 * RvRtcpRTPPacketRecv
 * description: Informs the RTCP session about a packet that was received 
 *              in the corresponding RTP session. Call this function after RvRtpRead(). 
 * input: hRCTP - The handle of the RTCP session.
 *        ssrc  - The synchronization source value of the participant that sent the packet. 
 *        localTimestamp - The local RTP timestamp when the received packet arrived. 
 *        myTimestamp    - The RTP timestamp from the received packet. 
 *	      sequence       - The packet sequence number
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpRTPPacketRecv(
        IN  RvRtcpSession  hRTCP,
        IN  RvUint32      ssrc,
        IN  RvUint32      localTimestamp,
        IN  RvUint32      myTimestamp,
        IN  RvUint16      sequence);
/************************************************************************************
 * RvRtcpRTPPacketSent
 * description: Informs the RTCP session about a packet that was sent in the corresponding
 * RTP session. Call this function after RvRtpWrite() in case of manual RTCP session opening.
 * input: hRCTP - The handle of the RTCP session.
 *        bytes - The number of bytes in the sent packet. 
 *    timestamp - The RTP timestamp from the sent packet
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpRTPPacketSent(
        IN  RvRtcpSession  hRTCP,
        IN  RvInt32       bytes,
        IN  RvUint32      timestamp);

/************************************************************************************
 * RvRtcpGetPort
 * description: Gets the UDP port of an RTCP session. 
 * input: hRCTP - The handle of the RTCP session.
 * output: none.
 * return value: If no error occurs, the function returns the UDP port number. 
 ***********************************************************************************/
RVAPI
RvUint16 RVCALLCONV RvRtcpGetPort(
        IN  RvRtcpSession  hRCTP);

/************************************************************************************
 * RvRtcpSetBandwidth
 * description: Sets the maximum bandwidth for a RTCP session.
 * input: hRCTP - The handle of the RTCP session.
 *    bandwidth - The bandwidth for RTCP packets.   
 * output: none.
 * return none.
 * Note: according to standard RTCP bandwith must be about 5% of RTP bandwith
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpSetBandwidth(
        IN  RvRtcpSession  hRCTP,
        IN  RvUint32       bandwidth);



/*****************************************************************************************
 * RvRtcpSetBandwidthEx
 * description: Sets the maximum bandwidth for a RTCP session according to paticipant type
 * input: hRCTP - The handle of the RTCP session.
 *        bandwidth - The bandwidth for RTCP packets (bits per second).
 *        participant - the type of RTP session participant: Active Sender or not. 
 * output: none.
 * return none.
 * Note: according to standard RTCP bandwith must be about 5% of RTP bandwith
 ****************************************************************************************/
RVAPI
void RVCALLCONV RvRtcpSetBandwidthEx(
        IN  RvRtcpSession  hRTCP,
        IN  RvUint32       bandwidth,
		IN	RvRtpParticipant  participant);


                   /* == ENDS: Basic RTCP Functions == */



                    /* == Accessory RTCP Functions == */

/************************************************************************************
 * RvRtcpCheckSSRCCollision
 * description: Checks for SSRC collisions in the RTCP session and the corresponding
 *              RTP session. 
 * input: hRCTP - The handle of the RTCP session.
 * output: none.
 * return value:
 *  If no collision was detected, the function returns RV_FALSE. Otherwise, it returns RV_TRUE. 
 *
 * Remarks 
 *  You can check if an SSRC collision has occurred, by calling this function 
 *  RvRtcpRTPPacketSent() or RvRtcpRTPPacketRecv() returns an error.  
 ***********************************************************************************/
RVAPI
RvBool RVCALLCONV RvRtcpCheckSSRCCollision(
        IN  RvRtcpSession  hRTCP);


/************************************************************************************
 * RvRtcpEnumParticipants
 * description: The function enumerates all participants in an RTCP session by passing
 *              the SSRC of each session participant, in turn, to an application defined
 *              callback function, enumerator. The function, RvRtcpEnumParticipants()
 *              continues until
 *              the last participant is enumerated or the callback function returns RV_TRUE.  
 * input: hRCTP - The handle of the RTCP session.
 *   enumerator - A pointer to an application-defined function that will be called once
 *                per SSRC in the session. 
 *                The prototype of the SSRC enumerator is as follows:                   
 *                                                      
 *                RvBool                                                             
 *                   SSRCENUM(                                                          
 *                     IN  RvRtpSession  hTRCP,                                         
 *                     IN  RvUint32     ssrc                                            
 *                   );                                                                 
 *                                                                       
 *                The parameters passed to the enumerator are as follows:  
 *                   hRTCP      The handle of the RTCP session.                                                                             **
 *                   ssrc       A synchronization source that participates in the       
 *                 session.                                               
 *                                                             
 *                The enumerator should return RV_FALSE if it wants the enumeration 
 *                process to continue.  Returning RV_TRUE will cause               
 *                RvRtcpEnumParticipant() to return immediately.                    
 * output: none.
 * return value:
 *  The function, RvRtcpEnumParticipants() continues until the last participant is enumerated
 *    or the callback function returns RV_TRUE.
 ***********************************************************************************/
/* ouch! */
#define rtcpEnumParticipients  RvRtcpEnumParticipants

RVAPI
RvBool RVCALLCONV RvRtcpEnumParticipants(
        IN  RvRtcpSession  hRTCP,
        IN  RvRtcpSSRCENUM_CB    enumerator);

/************************************************************************************
 * RvRtcpGetSourceInfo
 * description: Provides information about a particular synchronization source.    
 * input: hRTCP - The handle of the RTCP session. 
 *        ssrc  - The source for which information is required. Get the SSRC from the function. 
 * output: info  - Information about the synchronization source.
 * return value: If an error occurs, the function returns a negative value. 
 *               If no error occurs, the function returns a non-negative value. 
 * Remark: To get Self information use the own SSRC of the session. 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpGetSourceInfo(
        IN   RvRtcpSession hRTCP,
        IN   RvUint32     ssrc,
        OUT  RvRtcpINFO*    info);
#if 0
/************************************************************************************
 * RvRtcpGetOptionalSDESInfo -  This function is obsolete.
 * This function is obsolete. For retrieving additional SDES information
 * use function RvRtcpSetSDESReceivedEventHandler that sets the callback function that
 * is called, when SDES information arives in RTCP packet. This mechanism allows to
 * significantly decrease the size of dynamic allocation for RTCP session.
 ************************************************************************************
 * RvRtcpGetOptionalSDESInfo
 * description: Provides SDES optional (not CNAME) information about a particular 
 *              synchronization source.    
 * input: hRTCP      - The handle of the RTCP session. 
 *        ssrc       - The source for which information is required. Get the SSRC from the function. 
 *        SDEStype   - SDEStype information (NAME, EMAIL, PHONE, ...)
 *        MaxSDESLen - length of user buffer for SDES optional information
 * output: info  - pointer to write SDES optional information about the synchronization source.
 * return value: If an error occurs, the function returns a negative value. 
 *               If no error occurs, the function returns a non-negative value. 
 * Remark:  - To get Self information use the own SSRC of the session. 
 *          - Max SDES information size is 255 (RFC 3550).
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpGetOptionalSDESInfo(
		IN   RvRtcpSession hRTCP,
		IN   RvUint32      ssrc,
		IN  RvRtcpSDesType SDEStype,
		IN  RvInt32 MaxSDESLen,
		OUT  RvInt8* info);
#endif
/************************************************************************************
 * RvRtcpSetGroupAddress
 * description: Specifies a multicast address for an RTCP session.  
 * input: hRTCP      - The handle of the RTCP session. 
 *        pRtcpAddress - pointer to multicast address
 * output: none.
 * return value: If an error occurs, the function returns a negative value. 
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtcpSetGroupAddress(
									   IN  RvRtcpSession  hRTCP,
									   IN  RvUint32      ip);
#endif
RVAPI
RvStatus RVCALLCONV RvRtcpSetGroupAddress(
        IN  RvRtcpSession  hRTCP,
		IN  RvNetAddress* pRtcpAddress);
/************************************************************************************
 * RvRtcpSetMulticastTTL
 * description:  Defines a multicast Time To Live (TTL) for the RTCP session 
 *               (multicast sending).
 * input:  hRTCP  - Handle of RTCP session.
 *         ttl   -  ttl threshold for multicast
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: the function is supported with IP stack that has full multicast support
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpSetMulticastTTL(
	IN  RvRtcpSession  hRTCP,
	IN  RvUint8       ttl);
/************************************************************************************
 * RvRtcpGetSSRC
 * description: Returns the synchronization source value for an RTCP session. 
 * input: hRTCP      - The handle of the RTCP session. 
 * output: none.
 * return value: The synchronization source value for the specified RTCP session. 
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpGetSSRC(
        IN  RvRtcpSession  hRTCP);
/************************************************************************************
 * RvRtcpSetSSRC
 * description: Changes the synchronization source value for an RTCP session. 
 *              If a new SSRC was regenerated after collision,
 *              call this function to notify RTCP about the new SSRC. 
 * input: hRTCP  - The handle of the RTCP session. 
 *        ssrc   - A synchronization source value for the RTCP session. 
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value. 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSSRC(
        IN  RvRtcpSession  hRTCP,
        IN  RvUint32      ssrc);

                 /* == ENDS: Accessory RTCP Functions == */
/************************************************************************************
 * RvRtcpGetEnumNext
 * description: obtains the SSRC of the next session participant 
 * input: hRTCP  - The handle of the RTCP session. 
 *        prev   - index of the current participant
 * output: ssrc   - pointer to a synchronization source value of the next session participant
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 *               (next session participant index)
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpGetEnumNext(
                IN  RvRtcpSession  hRTCP,
                IN  RvInt32       prev,
                IN  RvInt32 *     ssrc);

/************************************************************************************
 * RvRtcpGetEnumFirst
 * description: obtains the SSRC of the first valid session participant
 * input: hRTCP  - The handle of the RTCP session. 
 * output: ssrc   - pointer to a synchronization source value of the first session participant
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 *               (first session participant index)
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpGetEnumFirst(
                IN  RvRtcpSession  hRTCP,
                IN  RvInt32 *     ssrc);

/************************************************************************************
 *	RvRtcpSetAppEventHandler  setting APP event callback
 *  input:  hRTCP - Handle of RTP session.	
 *          pAppEventHandler - pointer to USER's app event callback
 *          rtcpByeContext - context for callback
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 * Note: after this function RvRtpClose have to close the session
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetAppEventHandler(
	IN RvRtcpSession         hRTCP,
	IN RvRtcpAppEventHandler_CB pAppEventHandler,
	IN void* rtcpAppContext);

/************************************************************************************
 *	RvRtcpSetByeEventHandler  setting BYE event callback
 *  input:  hRTCP - Handle of RTP session.	
 *          pByeEventHandler - pointer to USER's app event callback
 *          rtcpByeContext - context for callback
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 * Note: after this function RvRtpClose have to close the session
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetByeEventHandler(
	IN RvRtcpSession         hRTCP,
	IN RvRtcpByeEventHandler_CB pByeEventHandler,
	IN void* rtcpByeContext);
/************************************************************************************
 *	RvRtcpSetSDESReceivedEventHandler  setting SDES message event callback
 *  input:  hRTCP - Handle of RTP session.	
 *          SDESMessageReceived - pointer to USER's SDES event callback
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSDESReceivedEventHandler(
		IN RvRtcpSession                hRTCP,
		IN RvRtcpSDESMessageReceived_CB SDESMessageReceived);

/************************************************************************************
 * RvRtcpSetRoundTripDelayEventHandler 
 * Description:
 *      Sets round trip delay event handler
 * input:  hRTCP              - Handle of RTP session.	
 *         roundTripDelayFunc - pointer to USER's round trip delay event callback
 * output: none:                             
 * return value: If an error occurs, the function returns a negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSDESReceivedEventHandlerEx(
		IN RvRtcpSession				  hRTCP,
		IN RvRtcpSDESMessageReceivedEx_CB SDESMessageReceived,
		IN void*						  SDESContext);


/************************************************************************************
 * RvRtcpSetSRRREventHandler 
 * Description:
 *      Sets SR/RR event handler
 * input:  hRTCP              - Handle of RTP session.	
 *         SRRRMessageReceived - pointer to USER's SR/RR event callback
 * output: none:                             
 * return value: the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSRRREventHandler(
		IN RvRtcpSession				hRTCP,
		IN RvRtcpSRRRMessageReceived_CB SRRRMessageReceived);




/************************************************************************************
 * RvRtcpSetSRRREventHandlerEx 
 * Description:
 *      Sets SR/RR event handler
 * input:  hRTCP              - Handle of RTP session.	
 *         SRRRMessageReceived - pointer to USER's SR/RR event callback
 *		   SRRRContext			 - application's context to SDES event handler	
 * output: none:                             
 * return value: the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSRRREventHandlerEx(
		IN RvRtcpSession					hRTCP,
		IN RvRtcpSRRRMessageReceivedEx_CB	SRRRMessageReceived,
		IN void*							SRRRContext);


/************************************************************************************
 * RvRtcpSetRoundTripDelayEventHandler 
 * Description:
 *      Sets round trip delay event handler
 * input:  hRTCP              - Handle of RTP session.	
 *         roundTripDelayFunc - pointer to USER's round trip delay event callback
 * output: none:                             
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetRoundTripDelayEventHandler(
		IN RvRtcpSession                hRTCP,
		IN RvRtcpRoundTripDelay_CB      roundTripDelayFunc);


/************************************************************************************
 * RvRtcpSetRoundTripDelayEventHandlerEx 
 * Description:
 *      Sets round trip delay event extended handler
 * input:  hRTCP              - Handle of RTP session.	
 *         roundTripDelayFunc - pointer to USER's round trip delay event callback
 *		   roundTripDelayContext - application's context to RoundTripDelay event handler	 
 * output: none:                             
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetRoundTripDelayEventHandlerEx(
		IN RvRtcpSession             hRTCP,
		IN RvRtcpRoundTripDelayEx_CB roundTripDelayFunc,
		IN void*					 roundTripDelayContext);



/************************************************************************************
 * RvRtcpSetSsrcRemoveEventHandler 
 * Description:
 *      Sets an SSRC removed event handler. The event is rized when the stack is going to 
 *		remove dormant(not sending 5 RTCP cycles) SSRC.
 * input:  hRTCP              - Handle of RTP session.	
 *         roundTripDelayFunc - pointer to USER's round trip delay event callback
 *		   roundTripDelayContext - application's context to RoundTripDelay event handler	 
 * output: none:                             
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSsrcRemoveEventHandler(
		IN RvRtcpSession				hRTCP,
		IN RvRtcpSsrcRemove_CB			ssrcRemoveFunc,
		IN void*						context);


/*********************************************************************************************
 *	RvRtcpSessionSendApps 
 * description: sends manual compound APP RTCP report.
 * input: hRTCP
 *           - The handle of the RTCP session. 
 *        appMessageTable 
 *           - pointer to APP messages table (to be sent)
 *        appMessageTableEntriesNum - number of messages in appMessageTable
 *        bCompound -to send compound report (ALL SDES + empty RR + APP)
 * output: none. 
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 *  Remark:
 * This function will not affect the normal periodic RTCP messages and is not
 *	included in any statistical calulations, including bandwith limitations. 
 ***********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendApps(IN RvRtcpSession hRTCP,
										  IN RvRtcpAppMessage* appMessageTable,
										  IN RvInt32 appMessageTableEntriesNum, 
										  IN RvBool bCompound);

/************************************************************************************
 *	RvRtcpSessionSendBye  Sends immediate compound BYE without closing session. 
 *  input:  hRTCP - Handle of RTP session.	
 *          reason - reason for BYE
 *          length - length of the reason
 *  return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 * Note: 1) after this function RvRtpClose have to close the session
 *       2) this message is not included in RTCP bandwith
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendBye(
	IN RvRtcpSession hRTCP,
	IN const char *reason,
	IN RvInt32 length);

/*******************************************************************************************
 * RvRtcpSessionShutdown
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
RvStatus RVCALLCONV RvRtcpSessionShutdown(IN RvRtcpSession  hRTCP, 
										  IN RvChar *reason, 
										  IN RvUint8 reasonLength);
/*********************************************************************************************
 *	RvRtcpSessionSendRR 
 * description: sends manual compound  RTCP Receiver report.
 * input: hRTCP
 *           - The handle of the RTCP session. 
 *        bCompound - if set to RV_TRUE to send compound report (RR + ALL SDES + APP)
 *                    if set to RV_FALSE to send RR only
 *        appMessageTable 
 *           - pointer to APP messages table (to be sent)
 *        appMessageTableEntriesNum - number of messages in appMessageTable
 *        
 * output: none. 
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 *  Remark:
 * 1) This function will not affect the normal periodic RTCP messages and is not
 *	included in any statistical calulations, including bandwith limitations. 
 * 2) appMessageTable and appMessageTableEntriesNum are used only if bCompound is set to RV_TRUE
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendRR(
	IN RvRtcpSession hRTCP, 
	IN RvBool bCompound,
	IN RvRtcpAppMessage* appMessageTable,
	IN RvInt32 appMessageTableEntriesNum);

/*********************************************************************************************
 *	RvRtcpSessionSendSR 
 * description: sends manual compound  RTCP sender report (receiver report part of it included).
 * input: hRTCP
 *           - The handle of the RTCP session. 
 *        bCompound - if set to RV_TRUE to send compound report (RR + ALL SDES + APP)
 *                    if set to RV_FALSE to send RR only
 *        appMessageTable 
 *           - pointer to APP messages table (to be sent)
 *        appMessageTableEntriesNum - number of messages in appMessageTable
 *        
 * output: none. 
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 *  Remark:
 * 1) This function will not affect the normal periodic RTCP messages and is not
 *	included in any statistical calulations, including bandwith limitations. 
 * 2) appMessageTable and appMessageTableEntriesNum are used only if bCompound is set to RV_TRUE
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendSR(
	IN RvRtcpSession hRTCP, 
	IN RvBool bCompound,
	IN RvRtcpAppMessage* appMessageTable,
	IN RvInt32 appMessageTableEntriesNum);


#if defined(RVRTP_SECURITY)
/************************************************************************************
 * RvRtcpSetEncryption
 * description:  Sets encryption plug-n and decryption keys for RTCP session
 * input:  hRTCP - Handle of RTCP session.
 *         mode - encryption mode
 *         enctyptorDesPlugInPtr - pointer to ecryption plug in
 *         keyPlugInPtr          - pointer to key manager plug in
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE:  if pointer (ekeyPtr or dkeyPtr) is NULL then this key will not set.
 ***********************************************************************************/  
RVAPI
RvStatus RVCALLCONV RvRtcpSetEncryption(
	IN RvRtcpSession hRTCP,
	IN RvRtpEncryptionMode mode,
	IN RvRtpEncryptionPlugIn* enctyptorPlugInPtr,
	IN RvRtpEncryptionKeyPlugIn* keyPlugInPtr);
#endif
/************************************************************************************
 * RvRtcpSetEncryptionNone
 * description:  Cancels encryption for the session
 * input:  hRTCP - Handle of RTCP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/    

RVAPI
RvStatus RVCALLCONV RvRtcpSetEncryptionNone(IN RvRtcpSession hRTCP);
/************************************************************************************
 * RvRtcpSessionSetEncryptionMode
 * description:  Sets encryption mode for the session
 * input:  hRTCP - Handle of RTCP session.
 *         mode - encryption mode
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetEncryptionMode(
		IN RvRtcpSession hRTCP, IN RvRtpEncryptionMode mode);
/************************************************************************************
 * RvRtcpSessionSetParam
 * description:  Sets session parameters for the session
 * input:  hRTCP - Handle of RTCP session.
 *         param - type of parameter
 *         data  - pointer to parameter
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * Note:1)   for RTP session with dynamic payload (with param = RVRTCP_PARAMS_RTPCLOCKRATE)
 *           this function should be called
 *           after RTP session opening for accurate RTCP timestamp calculation
 *       For standard payload types this call can be omitted.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetParam(
					IN RvRtcpSession hRTCP, 
					IN RvRtcpParameters param, 
					IN void* data);

/********************************************************************************************
 * RvRtcpSetTypeOfService
 * Set the type of service (DiffServ Code Point) of the socket (IP_TOS)
 * This function is supported by few operating systems.
 * IPV6 does not support type of service.
 * This function is thread-safe.
 * INPUT   : hRTCP           - RTCP session to set TOS byte
 *           typeOfService  - type of service to set
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 * Remark: In Windows environment for setting the TOS byte RvRtpOpen or RvRtpOpenEx
 *         must be called with local IP address and not with "any" address. 
 ********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetTypeOfService(
        IN RvRtcpSession hRTCP,
        IN RvInt32        typeOfService);

/********************************************************************************************
 * RvRtcpGetTypeOfService
 * Get the type of service (DiffServ Code Point) of the socket (IP_TOS)
 * This function is supported by few operating systems.
 * IPV6 does not support type of service.
 * This function is thread-safe.
 * INPUT   : hRTCP           - RTCP session handle
 * OUTPUT  : typeOfServicePtr  - pointer to type of service to set
 * RETURN  : RV_OK on success, other on failure
 *********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpGetTypeOfService(
        IN RvRtcpSession hRTCP,
        OUT RvInt32*     typeOfServicePtr);

/********************************************************************************************
 * RvRtcpSessionGetSocket
 * Description: Gets a socket used fot rtcp session
 * This function is thread-safe.
 * INPUT   : hRTCP           - RTCP session handle
 * RETURN  : socket
 *********************************************************************************************/
RVAPI
RtpSocket RVCALLCONV RvRtcpSessionGetSocket(IN RvRtcpSession hRTCP);

/********************************************************************************************
* RvRtcpSessionSetSocket
* Description: Set the socket to be used for this session and
* INPUT   : hRTCP          - RTCP session handle
*         : socket         - pointef to the socket to be used in this session 
*         : address        - pointer to address to which the socket is bound to. 			
* RETURN  : RV_OK on success, other on failure
*********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetSocket(
			IN RvRtcpSession hRCTP, 
			IN RtpSocket	 *socket);


/********************************************************************************************
* RvRtcpSessionSetSocketEx
* Description: Set the socket to be used for this session and
*			   Also bind the socket to specified address.
* INPUT   : hRTCP          - RTCP session handle
*         : socket         - pointer to the socket to be used in this session 
*         : address        - pointer to address to which the socket is bound to. 			
* RETURN  : RV_OK on success, other on failure
*********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetSocketEx(
			IN RvRtcpSession hRTCP, 
			IN RtpSocket	*socket,  
			IN RvAddress	*address);

/********************************************************************************************
* RvRtcpSessionGetContext
* Description: Return the context of the RTCP session.
* INPUT   : hRTCP          - RTCP session handle
* OUTPUT  : context		   - The session's context returned.
* RETURN  : RV_OK on success, other on failure
*********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionGetContext(
					IN RvRtcpSession hRTCP,
					OUT void **context);

#ifdef __cplusplus
}
#endif

#endif  /* __RTCP_H */





















