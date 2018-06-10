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
#include "rvcbase.h"
#include "rvstdio.h"
#include "rvlock.h"
#include "rvselect.h"
#include "rvsocket.h"
#include "rvmemory.h"
#include "rvclock.h"
#include "rvntptime.h"
#include "rvtime.h"
#include "rvtimer.h"
#include "rvtimestamp.h"
#include "rvrandomgenerator.h"
#include "rvhost.h"
#include "rvtransportsocket.h"
#include <string.h>
#include "rvrtpbuffer.h"
#include "bitfield.h"
#include "rtputil.h"
#include "rtcp.h"
#include "RtcpTypes.h"
#include "rvrtpencryptionplugin.h"
#include "rvrtpencryptionkeyplugin.h"
#include "rvrtpheader.h"
#include "payload.h"

#ifdef __H323_NAT_FW__
#include "RtpDemux.h"
#endif

#ifdef __RTCP_XR__
#include "rtcpxrplugin.h"
#endif

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define RTP_SOURCE      (rtpGetSource(RVRTP_RTCP_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_RTCP_MODULE))
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"
#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtcp" #name

#ifdef __cplusplus
extern "C" {
#endif


	
#define RTCP_MIN_TIME			  5000       /* min Regular interval between RTCP packets */

#define RTCP_BYE_REASON_LENGTH    (255)
#define MAXIPS                    20
#define MAX_DROPOUT               3000
#define MAX_MISORDER              100
#define MIN_SEQUENTIAL            2
#define RTP_SEQ_MOD               0x10000

#define ALIGNMENT                 0x10

/* RTCP header bit locations - see the standard */
#define HEADER_V                  30      /* version                       */
#define HEADER_P                  29      /* padding                       */
#define HEADER_RC                 24      /* reception report count        */
#define HEADER_PT                 16      /* packet type                   */
#define HEADER_len                0       /* packet length in 32-bit words */

/* RTCP header bit field lengths - see the standard */
#define HDR_LEN_V                 2       /* version                       */
#define HDR_LEN_P                 1       /* padding                       */
#define HDR_LEN_RC                5       /* reception report count        */
#define HDR_LEN_PT                8       /* packet type                   */
#define HDR_LEN_len               16      /* packet length in 32-bit words */


/* used to overcome byte-alignment issues */
#define RTCPHEADER_SSRCMAXCOUNT   RvInt32Const(31)
#define RTCP_BYEBACKOFFMINIMUM    RvInt32Const(50)
#define RTCP_PACKETOVERHEAD       RvUint32Const(28)   /* UDP */
#define RTCP_DEFAULTPACKETSIZE    RvUint32Const(100)  /* Default RTCP packet size for TIMER initialization */

#define SIZEOF_SR                 (sizeof(RvUint32) * 5)
#define SIZEOF_RR                 (sizeof(RvUint32) * 6)

#define SIZEOF_SDES(sdes)         (((sdes).length + 6) & 0xfc)

/* initial bit field value for RTCP headers: V=2,P=0,RC=0,PT=0,len=0 */
#define RTCP_HEADER_INIT          0x80000000

    
#define W32Len(l)  ((l + 3) / 4)  /* length in 32-bit words */


    
/* RTCP instance to use */
RvRtcpInstance rvRtcpInstance;

/* local functions */
static RvUint32 rtcpGetEstimatedRTPTime(
        IN RvRtcpSession  hRTCP,
        const RvUint64 *ntpNewTimestampPtr);

static rtcpHeader makeHeader(RvUint32 ssrc, RvUint8 count, RtcpType type, RvUint16 dataLen);
static RvBool dataAddToBuffer(void *data, RvUint32 size, RvRtpBuffer*  buf, RvUint32 *allocated);
static RvBool dataAddToBufferAndConvert(void *data, RvUint32 size, RvRtpBuffer*  buf, RvUint32 *allocated);
#define headerAddToBuffer(h, s, b, a) dataAddToBuffer((void *)h, s, b, a); 


static RvUint64 getNNTPTime(void);
static RvBool rtcpTimerCallback(IN void* key);
static void   setSDES(RvRtcpSDesType type, rtcpSDES* sdes, RvUint8 *data,
                      RvInt32 length);
static void   init_seq  (rtpSource *s, RvUint16 seq);
static RvInt32    update_seq(rtpSource *s, RvUint16 seq, RvUint32 ts, RvUint32 arrival);

static RvUint32 getLost    (rtpSource *s);
static RvUint32 getJitter  (rtpSource *s);
static RvUint32 getSequence(rtpSource *s);
static RvUint32 getSSRCfrom(RvUint8 *);
static rtcpInfo * findSSrc(rtcpSession *,RvUint32);
static RvUint32 rtcpRemoveDormantParticipants(rtcpSession *s);


static rtcpInfo *insertNewSSRC(rtcpSession *s, RvUint32 ssrc, RvAddress *remoteAddress);

static 	void rtcpScheduleAutoReports(rtcpSession *s);

static
void rtcpEventCallback(
        IN RvTransport         transport,
        IN RvTransportEvents   ev, 
        IN RvBool              error, 
        IN void*               usrData);

#ifdef __H323_NAT_FW__
static
void rtcpDemuxEventCallback(
        IN RvTransport         transport,
        IN RvTransportEvents   ev, 
        IN RvBool              error, 
        IN void*               usrData);
#endif

static RvStatus rtcpAddRTCPPacketType(
		  IN RvRtcpSession  hRTCP,
          IN RtcpType type,
          IN RvUint8 subtype,
          IN RvUint8* name,
          IN void *userData, /* application-dependent data  */
          IN RvUint32 userDataLength,
          IN OUT  RvRtpBuffer*  buf,
          IN OUT RvUint32* pCurrentIndex);

/* this function sends encrypted raw data for RTCP session */
RvStatus rtcpSessionRawSend(
		IN    RvRtcpSession hRTCP,
		IN    RvUint8*      bufPtr,
		IN    RvInt32       DataLen,
		IN    RvInt32       DataCapacity,
		INOUT RvUint32*     sentLenPtr);


RVINTAPI
RvUint32 rtcpGetTimeInterval(
		IN rtcpSession* s, 
		IN const RvUint32 minTime);


RVINTAPI
RvUint32 rtcpGetSessionMembersInfo(
		IN rtcpSession* s, 
		OUT RvUint32 *members, 
		OUT RvUint32 *senders);

RVINTAPI
RvStatus rtcpSetTimer(
					  IN rtcpSession*	s,
					  IN RvInt64		delay,
					  IN RvTimerFunc	rtcpTimerCb);

RvStatus RVCALLCONV rtcpProcessCompoundRTCPPacket(
        IN      RvRtcpSession  hRTCP,
        IN OUT  RvRtpBuffer*  buf,
        IN      RvUint64      myTime,
		IN		RvAddress*	  remoteAddress);



RvStatus RVCALLCONV rtcpProcessRTCPPacket(
    IN  rtcpSession *  s,
    IN  RvUint8 *      data,
    IN  RvInt32        dataLen,
    IN  RtcpType       type,
    IN  RvInt32        reportCount,
    IN  RvUint64       myTime,
	IN	RvAddress*	  remoteAddress);

static RvStatus rtcpSessionRawReceive(
    IN    RvRtpDemux    demux,
    IN    rtcpSession **ses,
    INOUT RvRtpBuffer*  buf,
    IN    RvSize_t      bytesToReceive,
    OUT   RvSize_t*     bytesReceivedPtr,
    OUT   RvAddress*    remoteAddressPtr);
/* == Basic RTCP Functions == */

/*=========================================================================**
**  == RvRtcpInit() ==                                                     **
**                                                                         **
**  Initializes the RTCP module.                                           **
**                                                                         **
**  RETURNS:                                                               **
**      A non-negative value upon success, or a negative integer error     **
**      code.                                                              **
**                                                                         **
**=========================================================================*/

RVAPI
RvStatus RVCALLCONV RvRtcpInit(void)
{
    RvStatus status;
	RvUint32 rtcpMaxSessions=1024;

#ifdef RVRTP_MAXSESSIONS 
	rtcpMaxSessions = RVRTP_MAXSESSIONS;
#endif	

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpInit"));
    status = RvCBaseInit();
    if (status == RV_OK)
	{
		status = RvSelectConstruct(rtcpMaxSessions, rtcpMaxSessions, logMgr, &rvRtcpInstance.selectEngine);
		if (status == RV_OK)
		{
			/* Find the pool of timers to use */
			status = RvSelectGetTimeoutInfo(rvRtcpInstance.selectEngine, NULL, &rvRtcpInstance.timersQueue);
			if (status == RV_OK)
			{
				if (rvRtcpInstance.timesInitialized == 0)
				{
					RvUint32 numAddrs = RV_RTCP_MAXIPS;
					/* Find the list of host addresses we have */
					status = RvHostLocalGetAddress(logMgr, &numAddrs, rvRtcpInstance.hostIPs);
					if (status == RV_OK)
					{
						rvRtcpInstance.addresesNum = numAddrs;
						if (numAddrs>0)
							RvAddressCopy(&rvRtcpInstance.hostIPs[0], &rvRtcpInstance.localAddress);
						else
				#if (RV_NET_TYPE & RV_NET_IPV6)
							RvAddressConstruct(RV_ADDRESS_TYPE_IPV6, &rvRtcpInstance.localAddress);
				#else
							RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &rvRtcpInstance.localAddress);
				#endif
						/* Create a random generator */
						RvRandomGeneratorConstruct(&rvRtcpInstance.randomGenerator,
							(RvRandom)(RvTimestampGet(logMgr)>>8));

						rvRtcpInstance.timesInitialized++;
						RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpInit"));

						/* ALL DONE !!! RETURN */
						return RV_OK;
					}
				}
			}
			RvSelectDestruct(rvRtcpInstance.selectEngine, rtcpMaxSessions);
		}
		RvCBaseEnd();
	}
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpInit"));
	return status;
}

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
RvInt32 RVCALLCONV rtcpInitEx(RvUint32 ip)
{
    RvInt32 rc;
    RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpInitEx"));
    if ((rc=RvRtcpInit()) >= 0)
    {
        RvAddressConstructIpv4(&rvRtcpInstance.localAddress, ip, 0);
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpInitEx"));
    return rc;
}
#endif

RVAPI
RvInt32 RVCALLCONV RvRtcpInitEx(IN RvNetAddress* pRtpAddress)
{
    RvInt32 rc;
    RvAddress* pRvAddress = NULL;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpInitEx"));
    if ((rc=RvRtcpInit()) >= 0)
    {
        if (NULL!=pRtpAddress&&RvNetGetAddressType(pRtpAddress)!=RVNET_ADDRESS_NONE)
        {
            pRvAddress = (RvAddress*) pRtpAddress->address;
            RvAddressCopy(pRvAddress, &rvRtcpInstance.localAddress);
        }
        else
            RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &rvRtcpInstance.localAddress);
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpInitEx"));
    return rc;
}

/****************************************************************************
 *   == RvRtcpEnd() ==                                                      
 *                                                                        
 *  Shuts down the RTCP module.                                           
 *                                                                       
 *  RETURNS:                                                             
 *      A non-negative value upon success, or a negative integer error   
 *      code.                                                            
 *                                                                        
 ****************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpEnd(void)
{
    RvStatus res = RV_OK;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpEnd"));
	if (rvRtcpInstance.timesInitialized>0)
	{
		res = RvSelectDestruct(rvRtcpInstance.selectEngine, 1024);
		RvRandomGeneratorDestruct(&rvRtcpInstance.randomGenerator);
		rvRtcpInstance.timesInitialized--;    
        RvCBaseEnd();
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpEnd"));
    return res;
}


/********************************************************************************
 *RvRtcpClean
 *Description:"cleans" Rtcp session without closing session.
 *  		   Which permits to use the same socket that RTCP run on again
 * input: hRCTP        - The handle of the RTCP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
********************************************************************************/
RvStatus RVCALLCONV RvRtcpClean(IN RvRtcpSession     hRTCP)    
{
	rtcpSession *s;

    if (hRTCP == NULL)
        return RV_ERROR_UNKNOWN;

	s = (rtcpSession *)hRTCP;
	
	/* Cancel the timer if one is active */
	if (s->isTimerSet)
	{
		RvTimerCancel(&s->timer, RV_TIMER_CANCEL_WAIT_FOR_CB);
		s->isTimerSet = RV_FALSE;
	}
	
	if (s->remoteAddressSet)
	{
		RvAddressDestruct(&s->remoteAddress);
	}
	s->remoteAddressSet = RV_FALSE;
	
	/* Clear the list*/
	memset(s->participantsArray,0, (RvSize_t)(sizeof(rtcpInfo))*(s->sessionMembers));
	s->myInfo.collision = 0;
	s->myInfo.active = 0;
	s->myInfo.timestamp = 0;
	memset(&(s->myInfo.eSR),0,sizeof(s->myInfo.eSR));
	
	return RV_OK;
}



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
    IN void *               context)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetRTCPRecvEventHandler"));
    s->rtcpRecvCallback=rtcpCallback;
    s->haRtcp=context; /*context is Event to inform Ring3 about RTCP arrival*/
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRTCPRecvEventHandler"));
    return RV_OK;
}

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
 *	                  void RVCALLCONV RvRtcpEventHandlerEx_CB(
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
 *  rtcpPacket - the address of coumpound RTCP packet
 *  rtcpLen    - the length of RTCP packet  
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
    IN RvRtcpSession         hRTCP,
    IN RvRtcpEventHandlerEx_CB   rtcpCallback,
    IN void *               context)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetRTCPRecvEventHandler"));
    s->rtcpRecvCallbackEx=rtcpCallback;
    s->haRtcp=context; /*context is Event to inform Ring3 about RTCP arrival*/
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRTCPRecvEventHandler"));
    return RV_OK;
}

/********************************************************************************************
* RvRtcpSessionGetContext
* Description: Return the context of the RTCP session.
* INPUT   : hRTCP          - RTCP session handle
* OUTPUT  : context		   - The session's context returned.
* RETURN  : RV_OK.
*********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionGetContext(
					IN RvRtcpSession hRTCP,
					OUT void **context)
{
	rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionGetContext"));
	*context = s->haRtcp;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionGetContext"));
	return RV_OK;
}

/*******************************************************************************************
 * RvRtcpSetRTCPSendHandlerEx
 * description: Sets an extended callback to be called when RTCP packet was sent. 
 * input: 
 *   hRTCP          - Handle of the RTCP session. 
 *   rtcpCallback   - Pointer to the callback function that is called each time 
 *                    a new RCTP packet was sent at the RCTP session. 
 *                    If extended eventHandler is set to NULL, the event handler is removed. 
 *                    The prototype of the callback is as follows: 
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
    IN RvRtcpSendHandlerEx_CB   rtcpCallback)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetRTCPSendHandler"));
    s->rtcpSendCallbackEx=rtcpCallback;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRTCPSendHandler"));
    return RV_OK;
}


/************************************************************************************
 * RvRtcpGetAllocationSize
 * description: Returns the number of bytes required for internal RTCP session data structure.
 *              The application may allocate the requested amount of memory and use it with
 *              the RvRtcpOpenFrom() function.
 * input: sessionMembers        - Maximum number of members in RTP conference.
 *
 * output: none.
 * Return Values - The function returns the number of bytes required for
 *                 internal RTCP session data structure.
 ***********************************************************************************/

RVAPI
RvInt32 RVCALLCONV RvRtcpGetAllocationSize(
    IN  RvInt32 sessionMembers)
{
    return sizeof(rtcpSession) + ALIGNMENT + sizeof(rtcpInfo) * sessionMembers;
}


/************************************************************************************
 * rtcpSetLocalAddress
 * description: Set the local address to use for calls to rtcpOpenXXX functions.
 *              This parameter overrides the value given in RvRtcpInitEx() for all
 *              subsequent calls.
 * input: ip    - Local IP address to use
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/

#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtcpSetLocalAddress(IN RvUint32 ip)
{
    RvAddressConstructIpv4(&rvRtcpInstance.localAddress, ip, 0);
    return RV_OK;
}
#endif

RVAPI
RvInt32 RVCALLCONV RvRtcpSetLocalAddress(IN RvNetAddress* pRtpAddress)
{
    RvAddress*  pRvAddress = NULL;
    if  (NULL!=pRtpAddress)
    {
        pRvAddress = (RvAddress*) pRtpAddress->address;
        RvAddressCopy(pRvAddress, &rvRtcpInstance.localAddress);
    }
    return RV_OK;
}


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
        IN  RvInt32         bufferSize)
{
  RvNetAddress rtcpAddress;
  RvNetIpv4 Ipv4;
  Ipv4.port=port;
  Ipv4.ip=0;
  RvNetCreateIpv4(&rtcpAddress, &Ipv4);
  
  return RvRtcpOpenFrom(ssrc, &rtcpAddress, cname, maxSessionMembers, buffer, bufferSize);
}
#endif

RvUint32 rtcpInitTransmissionIntervalMechanism(rtcpSession* s)
{
    RvInt64 currentTime;
    RvUint32 delay;
	RvUint32 rtcpMinInterval = RTCP_MIN_TIME/2; /* very first interval is half of minimum delay for quicker notification */

    RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpInitTransmissionIntervalMechanism"));

    s->txInterval.initial   = RV_TRUE;    /* this is an "initial" (first) interval */ 
    s->txInterval.lastRTCPPacketSize = s->txInterval.aveRTCPPacketSize
        = RTCP_DEFAULTPACKETSIZE + RTCP_PACKETOVERHEAD;
    s->txInterval.pmembers = 1;
    s->txInterval.members  = 1;
    s->txInterval.senders  = 0;
    currentTime = RvTimestampGet(logMgr);

#ifdef __RTCP_FB__
	if (s->rtcpFbPlugin != NULL)
		rtcpMinInterval = s->rtcpFbPlugin->fbTimers.tmin;
#endif

	delay = rtcpGetTimeInterval(s, rtcpMinInterval); /* in ms */
    s->txInterval.previousTime = currentTime;
    s->txInterval.nextTime = RvInt64Add(s->txInterval.previousTime,
                 RvInt64Mul(RvInt64FromRvUint32(delay), RV_TIME64_NSECPERMSEC));

    RvLogDebug(rvLogPtr, (rvLogPtr, "rtcpInitTransmissionIntervalMechanism:"));
    RvLogDebug(rvLogPtr, (rvLogPtr, "previousTime=%d", RvInt64Div(s->txInterval.previousTime, RV_TIME64_NSECPERSEC)));
    RvLogDebug(rvLogPtr, (rvLogPtr, "currentTime =%d", RvInt64Div(currentTime, RV_TIME64_NSECPERSEC)));
    RvLogDebug(rvLogPtr, (rvLogPtr, "delay       =      %d", delay/1000));
    RvLogDebug(rvLogPtr, (rvLogPtr, "nextTime    =%d", RvInt64Div(s->txInterval.nextTime, RV_TIME64_NSECPERSEC)));

    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpInitTransmissionIntervalMechanism"));
    return delay;
}

RVAPI
RvRtcpSession RVCALLCONV RvRtcpOpenFrom(
                                      IN  RvUint32      ssrc,
                                      IN  RvNetAddress* pRtcpAddress,
                                      IN  char *        cname,
                                      IN  RvInt32       maxSessionMembers,
                                      IN  void *        buffer,
                                      IN  RvInt32       bufferSize)
{
     return rtcpDemuxOpenFrom(NULL, ssrc, pRtcpAddress, cname, maxSessionMembers, buffer, bufferSize);
}
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
                                      IN  char* item)
{
    RvStatus status = RV_OK;
    rtcpSession *s = (rtcpSession *)hRTCP;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESItem(hRTCP=%p,SDEStype=%d)", hRTCP, SDEStype));

    if (hRTCP != NULL)
    {
        switch(SDEStype)
        {
        case RV_RTCP_SDES_CNAME:
        case RV_RTCP_SDES_NAME:
        case RV_RTCP_SDES_EMAIL:
        case RV_RTCP_SDES_PHONE:
        case RV_RTCP_SDES_LOC:
        case RV_RTCP_SDES_TOOL:
        case RV_RTCP_SDES_NOTE:
        case RV_RTCP_SDES_PRIV:
            {
                RvInt32 length = (RvInt32)strlen(item);
                if (length <= RTCP_MAXSDES_LENGTH)
                {
                    setSDES(SDEStype, &(s->myInfo.SDESbank[SDEStype]), (RvUint8*)item, length);
                }
                else
                {
                    RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESItem: String too long"));
                    status = RV_ERROR_OUTOFRANGE;
                }
                break;
            }
        default:
            {
                RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESItem: wrong SDES type"));
                status = RV_ERROR_BADPARAM;
            }
        }
    }
    else
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESItem: NULL RTCP session handle"));
        status = RV_ERROR_NULLPTR;
    }

    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESItem=%d", status));
    return status;
}
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
 *    To prevent the large allocation, use RvRtcpOpenFrom() and indicate the actual number of
 *    session members.
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
        IN  char *      cname)
{
    RvNetAddress rtcpAddress;
    RvNetIpv4 Ipv4;
    Ipv4.port = port;
    Ipv4.ip = 0;
    RvNetCreateIpv4(&rtcpAddress, &Ipv4);
    return RvRtcpOpen(ssrc, &rtcpAddress, cname);
}
#endif
RVAPI
RvRtcpSession RVCALLCONV RvRtcpOpen(
                                  IN  RvUint32    ssrc,
                                  IN  RvNetAddress* pRtcpAddress,
                                  IN  char *      cname)
{
    rtcpSession* s;
    RvInt32 allocSize = RvRtcpGetAllocationSize(RTCP_MAXRTPSESSIONMEMBERS);
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpOpen"));
    if (RvMemoryAlloc(NULL, (RvSize_t)allocSize, logMgr, (void**)&s) != RV_OK)
        return NULL;

    if((rtcpSession*)RvRtcpOpenFrom(ssrc, pRtcpAddress, cname, RTCP_MAXRTPSESSIONMEMBERS, (void*)s, allocSize)==NULL)
    {
        RvMemoryFree(s, logMgr);
        return NULL;
    }

    s->isAllocated = RV_TRUE;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpOpen"));
    return (RvRtcpSession)s;
}

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
                IN  RvRtcpSession  hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpClose"));
    if (hRTCP == NULL)
        return RV_ERROR_UNKNOWN;

	RvRtcpRemoveAllRemoteAddresses(hRTCP); //update by kongfd ?????
    RvRtpAddressListDestruct(&s->addressList);
    /* Cancel the timer if one is active */
    if (s->isTimerSet)
        RvTimerCancel(&s->timer, RV_TIMER_CANCEL_WAIT_FOR_CB);
#ifdef __H323_NAT_FW__    
    if (NULL != s->demux)   
    {
        RtpDemux* d = (RtpDemux*) s->demux;
        RvLockGet(&d->lock, logMgr);
        if (d->rtcpSessionsCounter <= 0)
        {
            
            /* We're probably also in non-blocking mode */
            d->rtcpSelectEngine = NULL;
            memset(&d->rtcpAddress, 0, sizeof(RvNetAddress));
            RTPLOG_DEBUG((RTP_SOURCE, "RvRtcpClose(%#x): closing the demux socket", d));
            /* This function closes the specified IP socket and all the socket's connections.*/
            RvTransportRelease(d->rtcpTransport);
            d->rtcpTransport = NULL;              
        }
        RvLockRelease(&d->lock, logMgr);
    }
    else
#endif
    {        
        /* Close the transport */
        RvTransportRelease(s->transport);
        s->transport = NULL;
    }
#ifdef __RTCP_XR__
    if (s->rtcpxrplugin!=NULL &&
        s->rtcpxrplugin->callbacks!= NULL &&
        s->rtcpxrplugin->callbacks->release != NULL)
    {        
         s->rtcpxrplugin->callbacks->release(s->rtcpxrplugin, hRTCP);
    }
#endif
	
#ifdef __RTCP_FB__
	if (s->rtcpFbPlugin != NULL)
	{   
		/* We do not have to check that 
		s->rtcpFbPlugin->callbacks != NULL and 
		s->rtcpFbPlugin->callbacks->rtcpFbDestruct != NULL
		The pointers are set while constructing FB plugin.
		*/
		s->rtcpFbPlugin->callbacks->rtcpFbDestruct((RvRtcpSession)s);
	}
#endif

    /* Destroy the lock */
    RvLockDestruct(&s->lock, logMgr);

    /* free memory allocated for rtcpSession */
    if (s->isAllocated)
        RvMemoryFree(s, logMgr);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpClose"));
    return RV_OK;
}


/*=========================================================================**
**  == rtcpSetRemoteAddress() ==                                           **
**                                                                         **
**  Defines the address of the remote peer or of the multicast group.      **
**                                                                         **
**  PARAMETERS:                                                            **
**      hRTCP    The handle of the RTCP session.                           **
**                                                                         **
**      ip       The IP address to which the RTCP packets will be sent.    **
**                                                                         **
**      port     The UDP port to which the RTCP packets should be sent.    **
**                                                                         **
**  RETURNS:                                                               **
**      A non-negative value upon success, or a negative integer error     **
**      code.                                                              **
**                                                                         **
**=========================================================================*/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
void RVCALLCONV rtcpSetRemoteAddress(
                IN  RvRtcpSession  hRTCP,     /* RTCP Session Opaque Handle */
                IN  RvUint32      ip,        /* target ip address */
                IN  RvUint16      port)      /* target UDP port */
{
    RvNetAddress rtcpAddress;
    RvNetIpv4 Ipv4;

    Ipv4.port = port;
    Ipv4.ip = ip;
    RvNetCreateIpv4(&rtcpAddress, &Ipv4);
    RvRtcpSetRemoteAddress(hRTCP, &rtcpAddress);
}
#endif
/************************************************************************************
 * RvRtcpSetRemoteAddress
 * description: Defines the address of the remote peer or of the multicast group.
 * input: hRCTP        - The handle of the RTCP session.
 *        pRtcpAddress - pointer to RvNetAddress(to which packets to be sent)
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpSetRemoteAddress(
                                     IN  RvRtcpSession  hRTCP,     /* RTCP Session Opaque Handle */
                                     IN  RvNetAddress* pRtcpAddress)    /* target ip address and target UDP port */
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvAddress* pRvAddress = NULL;
    RvAddress  destAddress;
    RvNetIpv6 Ipv6 = {{0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0}, RvUint16Const(0), RvUint32Const(0)}; /* initialization for warning elimination */

    RTPLOG_ENTER(SetRemoteAddress);
    if (s == NULL)
    {
        RTPLOG_ERROR_LEAVE(SetRemoteAddress,"NULL session handle");
        return;
    }   
    RvRtcpRemoveAllRemoteAddresses(hRTCP);
    if (pRtcpAddress == NULL || RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_NONE||
        (RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_IPV6 &&
         RvNetGetIpv6(&Ipv6, pRtcpAddress)!=RV_OK))
    {
        RvRtcpStop(hRTCP);
        RTPLOG_ERROR_LEAVE(SetRemoteAddress,"bad address parameter or IPV6 is not supported in current configuration");
        return;
    }
    else
    {
        if (RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_IPV6)
        {
           pRvAddress = RvAddressConstructIpv6(&destAddress, Ipv6.ip, Ipv6.port, Ipv6.scopeId);
        }
        else
        {
           RvNetIpv4 Ipv4;
           RvNetGetIpv4(&Ipv4, pRtcpAddress);
           pRvAddress = RvAddressConstructIpv4(&destAddress, Ipv4.ip, Ipv4.port);
        }
        if (pRvAddress != NULL)
        {
            if (
                s->profilePlugin->funcs->addRemAddress != NULL)
            {
                s->profilePlugin->funcs->addRemAddress(s->profilePlugin, s->rtpSession, RV_FALSE, pRtcpAddress);
            }
            
			RvLockGet(&s->lock, logMgr);
            RvRtpAddressListAddAddress(&s->addressList, pRvAddress, NULL);
			RvLockRelease(&s->lock, logMgr);

			/* Set a flag that we have a remote address */
            s->remoteAddressSet = RV_TRUE;

			/* We have a remote address - set a timer for automatic reports */
            if (s->isManual == RV_FALSE) 
				rtcpScheduleAutoReports(s);
        }
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRemoteAddress"));
}
/************************************************************************************
 * RvRtcpAddRemoteAddress
 * description: Adds the address of the remote peer or of
 *             the multicast group or for multiunicast.
 * input: hRCTP        - The handle of the RTCP session.
 *        pRtcpAddress - pointer to RvNetAddress(to which packets to be sent)
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpAddRemoteAddress(
    IN  RvRtcpSession  hRTCP,     /* RTCP Session Opaque Handle */
    IN  RvNetAddress* pRtcpAddress) /* target ip address and target UDP port */
{   
    RTPLOG_ENTER(AddRemoteAddress);
    rtcpDemuxAddRemoteAddress(hRTCP, pRtcpAddress, NULL);
    RTPLOG_LEAVE(AddRemoteAddress);
}
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
    IN  RvNetAddress*  pRtcpAddress)
{
    RTPLOG_ENTER(RemoveRemoteAddress);
    rtcpDemuxRemoveRemoteAddress(hRTCP, pRtcpAddress, NULL);
    RTPLOG_LEAVE(RemoveRemoteAddress);
}
/************************************************************************************
 * RvRtcpRemoveAllRemoteAddresses
 * description: removes all RTCP addresses of the remote peer or of the multicast group
 *              or of the multiunicast list with elimination of address duplication.
 * input: hRCTP        - The handle of the RTCP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpRemoveAllRemoteAddresses(
    IN  RvRtcpSession  hRTCP)
{
    rtcpSession*       s           = (rtcpSession *)hRTCP;
    RtpNatAddress*     pNatAddress = NULL;   
    
    RTPLOG_ENTER(RemoveAllRemoteAddresses);
    if (NULL == s)
    {
        RTPLOG_ERROR_LEAVE(RemoveAllRemoteAddresses,"NULL session pointer");
        return;
    }
	RvLockGet(&s->lock, logMgr);
    while((pNatAddress = (RtpNatAddress*)RvRtpAddressListGetNext(&s->addressList, NULL))!= NULL)
    {   
        if ( 
/**h.e
			s->rtpSession!=NULL && s->profilePlugin != NULL && 
            s->profilePlugin->funcs != NULL &&
**/
            s->profilePlugin->funcs->removeRemAddress != NULL)
        {
            s->profilePlugin->funcs->removeRemAddress(s->profilePlugin, s->rtpSession, (RvNetAddress *) (&pNatAddress->address));
        }
#ifdef __H323_NAT_FW__
        if (pNatAddress->isMultiplexed)
            RvRtpAddressListRemoveAddress(&s->addressList, &pNatAddress->address, &pNatAddress->multiplexID);
        else
#endif
            RvRtpAddressListRemoveAddress(&s->addressList, &pNatAddress->address, NULL);
    }
    s->remoteAddressSet = RV_FALSE;
	RvLockRelease(&s->lock, logMgr);
    RTPLOG_LEAVE(RemoveAllRemoteAddresses);
}
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
 *   RvRtpOpen
 *   RvRtcpOpen
 *   ....RvRtcpSetRemoteAddress
 *   :
 *   ...RvRtcpStop
 *   ...RvRtcpSetRemoteAddress
 *   :
 *   RvRtcpClose
 *   RvRtpClose
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpStop(
                IN  RvRtcpSession  hRTCP)     /* RTCP Session Opaque Handle */
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    
    RTPLOG_ENTER(Stop);
    /* Cancel the timer if one is active */
    if (s->isTimerSet)
    {
        RvTimerCancel(&s->timer, RV_TIMER_CANCEL_WAIT_FOR_CB);
        s->isTimerSet = RV_FALSE;
    }
    RvRtcpRemoveAllRemoteAddresses(hRTCP);

    /* Clear the list*/
    memset(s->participantsArray,0, (RvSize_t)(sizeof(rtcpInfo))*(s->sessionMembers));
    s->myInfo.collision = 0;
    s->myInfo.active = 0;
    s->myInfo.timestamp = 0;
    memset(&(s->myInfo.eSR),0,sizeof(s->myInfo.eSR));
    
    RTPLOG_LEAVE(Stop);
    return RV_OK;
}
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
        IN RvInt32        typeOfService)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvStatus result = RV_OK;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetTypeOfService"));
    if (NULL == s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetTypeOfService: NULL pointer or socket is not allocated"));
        return RV_ERROR_NULLPTR;
    }
    result = RvTransportSetOption(s->transport,
                    RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                    RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE, &typeOfService);
    if (result != RV_OK)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetTypeOfService: RvTransportSetOption(s=%p,Transport=%p) failed(res=%d)",
            s, s->transport, result));
        return result;
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetTypeOfService"));
    return result;
}
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
        OUT RvInt32*     typeOfServicePtr)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvStatus result = RV_OK;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpGetTypeOfService"));
    if (NULL == s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpGetTypeOfService: NULL pointer or socket is not allocated"));
        return RV_ERROR_NULLPTR;
    }
    result = RvTransportGetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE, typeOfServicePtr);
    if (result != RV_OK)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpGetTypeOfService: RvTransportGetSocket(s=%p,Transport=%p) failed(res=%d)",
            s, s->transport, result));
        return result;
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpGetTypeOfService"));
    return result;
}
/************************************************************************************
 * RvRtcpRTPPacketRecv
 * description: Informs the RTCP session about a packet that was received
 *              in the corresponding RTP session. Call this function after RvRtpRead().
 * input: hRCTP - The handle of the RTCP session.
 *        ssrc  - The synchronization source value of the participant that sent the packet.
 *        localTimestamp - The local RTP timestamp when the received packet arrived.
 *        myTimestamp    - The RTP timestamp from the received packet.
 *        sequence       - The packet sequence number
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
                IN  RvUint16      sequence)

{
    rtcpSession *s = (rtcpSession *)hRTCP;
    rtcpInfo *fInfo;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketRecv"));

    if (ssrc == s->myInfo.ssrc)
    {
        s->myInfo.collision = 1;
        return ERR_RTCP_SSRCCOLLISION;
    }

    /* See if we can find this source or not */
    fInfo = findSSrc(s,ssrc);

    /* If we didn't find this SSRC, we lock the RTCP database and search for the
       SSRC again. If we don't find it again - we insert it to the list, and
       finally we unlock... */
    if (!fInfo)  /* New source */
    {
        /* this section is working with threads.*/
        /* Lock the rtcp session.*/
        RvLockGet(&s->lock, logMgr);

        /* check if the ssrc is exist*/
        fInfo = findSSrc(s,ssrc);

        if (!fInfo)
        {
            /* Still no SSRC - we should add it to the list ourselves */
            fInfo = insertNewSSRC(s, ssrc, (RvAddress *)NULL);
            if (fInfo ==  NULL)
            {
                RvLockRelease(&s->lock, logMgr);
                RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketRecv :no room to add new session member"));
                RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketRecv"));
                return RV_ERROR_OUTOFRESOURCES;
                /* error  cannot add the SSRC to SSRC members list */
            }
            init_seq(&(fInfo->src),sequence);
            fInfo->src.max_seq = (RvUint16)(sequence - 1);
        }

        /* unlock the rtcp session.*/
        RvLockRelease(&s->lock, logMgr);
    }


    if (fInfo != NULL)
    {

        if (!fInfo->invalid)
        {
            fInfo->active = RV_TRUE;
            update_seq(&(fInfo->src), sequence, localTimestamp, myTimestamp);
        }   
#ifdef __RTCP_XR__
        RvLockGet(&s->lock, logMgr);
        if (s->rtcpxrplugin!= NULL &&
            s->rtcpxrplugin->callbacks!= NULL && 
            s->rtcpxrplugin->callbacks->setPktRecvInfo != NULL)
        {

            /* set information for RTCP extended reports */
            s->rtcpxrplugin->callbacks->setPktRecvInfo(
                s->rtcpxrplugin, hRTCP, fInfo, localTimestamp, myTimestamp, sequence);
        }  
        RvLockRelease(&s->lock, logMgr);
#endif

    }

    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketRecv"));
    return RV_OK;
}

/************************************************************************************
 * RvRtcpRTPPacketSent
 * description: Informs the RTCP session about a packet that was sent
 *              in the corresponding RTP session. Call this function after RvRtpWrite()
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
                IN  RvUint32      timestamp)
{
    rtcpSession *s = (rtcpSession *)hRTCP;

	if (s == NULL)
		return RV_ERROR_NULLPTR;
    
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketSent"));
    
    RvLockGet(&s->lock, logMgr);
    if (s->isShutdown)
    {
        RvLockRelease(&s->lock, logMgr);
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketSent: sent RTP packet after shutting down"));
        return ERR_RTCP_SHUTDOWN;
    }
    s->myInfo.active = 2; /* to store active state for 2 previous transmission
    interval reports, as per RFC 3550 6.3 (we_sent parameter) */
    s->myInfo.eSR.nPackets++;
    s->myInfo.eSR.nBytes += bytes;
    s->myInfo.lastPackNTP   = getNNTPTime();
    s->myInfo.lastPackRTPts = timestamp;
    
    if (s->myInfo.collision)
    {
        RvLockRelease(&s->lock, logMgr);
        return ERR_RTCP_SSRCCOLLISION;
    }   
    RvLockRelease(&s->lock, logMgr);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpRTPPacketSent"));
    return RV_OK;
}


/*=========================================================================**
**  == rtcpGetPort() ==                                                    **
**                                                                         **
**  Gets the UDP port of an RTCP session.                                  **
**                                                                         **
**  PARAMETERS:                                                            **
**      hRTCP      The handle of the RTCP session.                         **
**                                                                         **
**  RETURNS:                                                               **
**      A non-negative value upon success, or a negative integer error     **
**      code.                                                              **
**                                                                         **
**=========================================================================*/

RVAPI
RvUint16 RVCALLCONV RvRtcpGetPort(
                IN  RvRtcpSession  hRTCP)
{
    rtcpSession* s = (rtcpSession *)hRTCP;
    RvUint16 sockPort = 0;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpGetPort"));
    RvTransportGetOption(s->transport,
        RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_LOCALPORT,
        (void*)&sockPort);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpGetPort"));
    return sockPort;
}


/************************************************************************************
 * RvRtcpSetBandwidth
 * description: Sets the maximum bandwidth for a RTCP session.
 * input: hRCTP - The handle of the RTCP session.
 *    bandwidth - The bandwidth for RTCP packets (bits per second) .
 * output: none.
 * return none.
 * Note: according to standard RTCP bandwith must be about 5% of RTP bandwith
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtcpSetBandwidth(
        IN  RvRtcpSession  hRTCP,
        IN  RvUint32       bandwidth)
{
    rtcpSession* s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetBandwidth"));
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetBandwidth"));
    s->txInterval.rtcpBandwidth = bandwidth;
}

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
		IN	RvRtpParticipant  participant)
{
    rtcpSession* s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetBandwidthEx"));
	
	switch (participant)
	{
	case RVRTP_PARTICIPANT_ACTIVESENDER:
		s->txInterval.rtcpSenderBandwidth = bandwidth;
		break;
	case RVRTP_PARTICIPANT_RECEIVER:
		s->txInterval.rtcpReceiverBandwidth = bandwidth;
		break;
	}
	
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetBandwidth"));
}


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
 *  rtcpRTPPacketSent() or rtcpRTPPacketRecv() returns an error.
 ***********************************************************************************/

RVAPI
RvBool RVCALLCONV RvRtcpCheckSSRCCollision(
                IN  RvRtcpSession  hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpCheckSSRCCollision"));
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpCheckSSRCCollision"));
    return (s->myInfo.collision != 0);
}


/*=========================================================================**
**  == RvRtcpEnumParticipants() ==                                           **
**                                                                         **
**  Provides information about in the RTCP session and the corresponding   **
**  RTP session.                                                           **
**                                                                         **
**  PARAMETERS:                                                            **
**      hRTCP      The handle of the RTCP session.                         **
**                                                                         **
**      enumerator A pointer to the function that will be called once per  **
**                 SSRC in the session.                                    **
**                                                                         **
**  RETURNS:                                                               **
**      If the enumeration process was stopped by the enumerator, the      **
**      function returns RV_FALSE, otherwise RV_TRUE.                      **
**                                                                         **
**  The prototype of the SSRC enumerator is as follows:                    **
**                                                                         **
**      RvBool                                                             **
**      SSRCENUM(                                                          **
**        IN  RvRtpSession  hTRCP,                                          **
**        IN  RvUint32     ssrc                                            **
**      );                                                                 **
**                                                                         **
**  The parameters passed to the enumerator are as follows:                **
**      hRTCP      The handle of the RTCP session.                         **
**                                                                         **
**      ssrc       A synchronization source that participates in the       **
**                 session.                                                **
**                                                                         **
**  The enumerator should return RV_FALSE if it wants the enumeration      **
**  process to continue.  Returning RV_TRUE will cause                     **
**  RvRtcpEnumParticipant() to return immediately.                         **
**                                                                         **
**=========================================================================*/

RVAPI
RvBool RVCALLCONV RvRtcpEnumParticipants(
                IN RvRtcpSession hRTCP,
                IN RvRtcpSSRCENUM_CB   enumerator)
{
    RvInt32 elem, ssrc=0;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpEnumParticipants"));
    elem = RvRtcpGetEnumFirst(hRTCP, &ssrc);

    while (elem >= 0)
    {
        if (enumerator(hRTCP, ssrc))
        {
            return RV_FALSE;
        }

        elem = RvRtcpGetEnumNext(hRTCP, elem, &ssrc);
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpEnumParticipants"));
    return RV_TRUE;
}


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
RvInt32 RVCALLCONV RvRtcpGetSourceInfo(
                IN   RvRtcpSession  hRTCP,
                IN   RvUint32      ssrc,
                OUT  RvRtcpINFO *    info)


{
    rtcpSession *s = (rtcpSession *)hRTCP;
    rtcpInfo *fInfo;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpGetSourceInfo"));
    if (ssrc == s->myInfo.ssrc)
    {
        info->selfNode          = RV_TRUE;
        
        RvLockGet(&s->lock, logMgr);
        info->sr.valid          = (s->myInfo.active>1);
#if ((RV_OS_TYPE == RV_OS_TYPE_PSOS) && (RV_OS_VERSION == RV_OS_PSOS_2_0)) || \
    (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
        /* just to get rid of annoying warnings */
        info->sr.mNTPtimestamp  = /*(RvUint32)((s->myInfo.eSR.tNNTP >> 16) >> 16);*/
            RvUint64ToRvUint32(RvUint64ShiftRight(RvUint64ShiftRight(s->myInfo.eSR.tNNTP, 16),16));
#else
        info->sr.mNTPtimestamp  = /*(RvUint32)(s->myInfo.eSR.tNNTP >> 32)*/ 
            RvUint64ToRvUint32(RvUint64ShiftRight(s->myInfo.eSR.tNNTP, 32));
#endif
        info->sr.lNTPtimestamp  = /*(RvUint32)(s->myInfo.eSR.tNNTP & 0xffffffff);*/
            RvUint64ToRvUint32(RvUint64And(s->myInfo.eSR.tNNTP, RvUint64Const(0, 0xffffffff)));
        info->sr.timestamp      = s->myInfo.eSR.tRTP;
        info->sr.packets        = s->myInfo.eSR.nPackets;
        info->sr.octets         = s->myInfo.eSR.nBytes;

        /* It's our node - receiver reports are not valid */
        info->rrFrom.valid      = RV_FALSE;
        info->rrTo.valid        = RV_FALSE;

        strncpy(info->cname, s->myInfo.SDESbank[RV_RTCP_SDES_CNAME].value, sizeof(info->cname)-1);
        RvLockRelease(&s->lock, logMgr);
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpGetSourceInfo"));
        return RV_OK;
    }

    fInfo = findSSrc(s,ssrc);

    if (fInfo)
    {
        info->selfNode              = RV_FALSE;
        
        RvLockGet(&s->lock, logMgr);
        info->sr.valid              = !fInfo->invalid;
#if ((RV_OS_TYPE == RV_OS_TYPE_PSOS) && (RV_OS_VERSION == RV_OS_PSOS_2_0)) || \
    (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
        /* just to get rid of annoying warnings */
        info->sr.mNTPtimestamp      = /*(RvUint32)((fInfo->eSR.tNNTP >> 16) >> 16);*/
                RvUint64ToRvUint32(RvUint64ShiftRight(RvUint64ShiftRight(fInfo->eSR.tNNTP, 16),16));
#else
        info->sr.mNTPtimestamp      = /*(RvUint32)(fInfo->eSR.tNNTP >> 32);*/
                RvUint64ToRvUint32(RvUint64ShiftRight(fInfo->eSR.tNNTP, 32));
#endif
        info->sr.lNTPtimestamp      = /*(RvUint32)(fInfo->eSR.tNNTP & 0xffffffff);*/
                RvUint64ToRvUint32(RvUint64And(fInfo->eSR.tNNTP, RvUint64Const(0, 0xffffffff)));
        info->sr.timestamp          = fInfo->eSR.tRTP;
        info->sr.packets            = fInfo->eSR.nPackets;
        info->sr.octets             = fInfo->eSR.nBytes;

        info->rrFrom.valid          = fInfo->eFromRR.valid;
        if (RV_TRUE == fInfo->eFromRR.valid)
        {
            info->rrFrom.fractionLost   = (fInfo->eFromRR.bfLost >> 24);
            info->rrFrom.cumulativeLost = (fInfo->eFromRR.bfLost & 0xffffff);
            info->rrFrom.sequenceNumber = fInfo->eFromRR.nExtMaxSeq;
            info->rrFrom.jitter         = fInfo->eFromRR.nJitter;
            info->rrFrom.lSR            = fInfo->eFromRR.tLSR;
            info->rrFrom.dlSR           = fInfo->eFromRR.tDLSR;

            fInfo->eFromRR.valid = RV_FALSE;
        }

        info->rrTo.valid            = RV_TRUE;

        if (RV_TRUE == fInfo->eToRR.valid)
        {
            info->rrTo.fractionLost     = (fInfo->eToRR.bfLost >> 24);
            info->rrTo.cumulativeLost   = (fInfo->eToRR.bfLost & 0xffffff);
            info->rrTo.sequenceNumber   = fInfo->eToRR.nExtMaxSeq;
            info->rrTo.jitter           = fInfo->eToRR.nJitter;
            info->rrTo.lSR              = fInfo->eToRR.tLSR;
            info->rrTo.dlSR             = fInfo->eToRR.tDLSR;

            fInfo->eToRR.valid = RV_FALSE;
        }

        strncpy(info->cname, fInfo->eSDESCname.value, sizeof(info->cname)-1);

        info->rtt = fInfo->rtt;
        RvLockRelease(&s->lock, logMgr);
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpGetSourceInfo"));
    return (!fInfo) ? ERR_RTCP_ILLEGALSSRC : 0;
}

/************************************************************************************
 * RvRtcpGetRemoteAddress: shen 2014-11-10
 * description: Provides information about a RTCP remote address.
 * input: hRTCP - The handle of the RTCP session.
 *        ssrc  - The source for which information is required. Get the SSRC from the function.
 * output: remAddress  - a RTCP remote address.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpGetRemoteAddress(
									   IN   RvRtcpSession  hRTCP,
									   IN   RvUint32       ssrc,
									   OUT  RvNetAddress  *remAddress )
{
	rtcpSession *s = (rtcpSession *)hRTCP;
	rtcpInfo    *fInfo;

	fInfo = findSSrc( s, ssrc );

	if ( fInfo )
	{
		RvAddressToRvNetAddress(&fInfo->remoteAddress, remAddress);
        return RV_OK;
	}

    return ( !fInfo ) ? ERR_RTCP_ILLEGALSSRC : 0;
}


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
                IN  RvUint32      ip)
{
    RvNetAddress rtcpAddress;
    RvNetIpv4   Ipv4;
    Ipv4.ip = ip;
    Ipv4.port = 0;
    RvNetCreateIpv4(&rtcpAddress, &Ipv4);
    return RvRtcpSetGroupAddress(hRTCP, &rtcpAddress);
}
#endif
RVAPI
RvInt32 RVCALLCONV RvRtcpSetGroupAddress(
        IN  RvRtcpSession  hRTCP,
        IN  RvNetAddress* pRtcpAddress)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvAddress   mcastAddr, mcastInterface;
    RvAddress*  addresses[2];
    RvStatus    res, res1, res2;
    RvNetIpv4   Ipv4;
    RvInt32     addressType = RV_ADDRESS_TYPE_NONE;
    RvUint16    port;
    RvBool      trueVal;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetGroupAddress"));

    if (s->transport == NULL ||
        strcmp(RvTransportGetType(s->transport), RV_SOCKET_TRANSPORT_STR) != 0)
    {
        RvLogError(rvLogPtr, (rvLogPtr,
            "RvRtcpSetGroupAddress: the %p transport doesn't support multicasting",
            s->transport));
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (NULL==pRtcpAddress||
        RV_ADDRESS_TYPE_NONE == (addressType = RvAddressGetType(&rvRtcpInstance.localAddress)))
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetGroupAddress() pRtpAddress==NULL or local address is undefined"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetGroupAddress"));
        return RV_ERROR_BADPARAM;
    }
    if (RvNetGetAddressType(pRtcpAddress) == RVNET_ADDRESS_IPV6)
    {
#if (RV_NET_TYPE & RV_NET_IPV6)
        RvAddressConstructIpv6(&mcastAddr,
            RvAddressIpv6GetIp(RvAddressGetIpv6((RvAddress*)pRtcpAddress->address)),
            RV_ADDRESS_IPV6_ANYPORT,
            RvAddressGetIpv6Scope((RvAddress*)pRtcpAddress->address));
#else
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetGroupAddress: IPV6 is not supported in current configuration"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetGroupAddress"));
        return RV_ERROR_BADPARAM;
#endif
    }
    else
    {
        RvNetGetIpv4(&Ipv4, pRtcpAddress);
        RvAddressConstructIpv4(&mcastAddr, Ipv4.ip, RV_ADDRESS_IPV4_ANYPORT);
    }
    RvAddressCopy(&rvRtcpInstance.localAddress, &mcastInterface);
    res = RvTransportGetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_LOCALPORT, (void*)&port);
    if (res != RV_OK)
    {
        RTPLOG_ERROR((RTP_SOURCE, "RvRtcpSetGroupAddress - Failed to get local port(%#x)", s));
        RvAddressDestruct(&mcastAddr); RvAddressDestruct(&mcastInterface);
        return res;
    }
    RvAddressSetIpPort(&mcastInterface, port); /* any port for IPV6 and IPV4 */
        
    /* Allow sending to multicast addresses */
    trueVal = RV_TRUE;
    res2 = RvTransportSetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_BROADCAST, (void*)&trueVal);
    res1 = RvTransportSetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_MULTICASTINTERFACE, (void*)&mcastInterface);
    /* This function adds the specified address (in network format) to the specified
       Multicast interface for the specified socket.*/
    addresses[0] = &mcastAddr; addresses[1] = &mcastInterface;
    res = RvTransportSetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_JOIN, (void*)addresses);
    if (res==RV_OK && res1==RV_OK && res2==RV_OK)
    {        
        RvChar addressStr[64];
        RV_UNUSED_ARG(addressStr);
        RTPLOG_INFO((RTP_SOURCE, "RvRtcpSetGroupAddress() - Successed to set multicast group address to %s for session (%#x)", 
            RvAddressGetString(&mcastAddr, sizeof(addressStr), addressStr), s));
    }
    else
    {
		RTPLOG_ERROR((RTP_SOURCE, "RvRtcpSetGroupAddress - Failed to set multicast group address for session (%#x)", s));
    }
    RvAddressDestruct(&mcastAddr);
    RvAddressDestruct(&mcastInterface);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetGroupAddress, result = %d", res));
	RV_UNUSED_ARG(addressType);
    return res;
}

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
	IN  RvUint8       ttl)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
	RvStatus res = RV_OK;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetMulticastTTL"));
    if (NULL==s)
	{
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetMulticastTTL"));
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetMulticastTTL() NULL session handle"));
		return RV_ERROR_BADPARAM;
	}

    if (s->transport == NULL ||
        strcmp(RvTransportGetType(s->transport),RV_SOCKET_TRANSPORT_STR) != 0)
    {
        RvLogError(rvLogPtr, (rvLogPtr,
            "RvRtcpSetMulticastTTL: the %p transport doesn't support multicasting",
            s->transport));
        return RV_ERROR_ILLEGAL_ACTION;
    }

    res = RvTransportSetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_MULTICASTTTL, (void*)&ttl);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetMulticastTTL"));
	return res;
}
/************************************************************************************
 * RvRtcpGetSSRC
 * description: Returns the synchronization source value for an RTCP session.
 * input: hRTCP      - The handle of the RTCP session.
 * output: none.
 * return value: The synchronization source value for the specified RTCP session.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtcpGetSSRC(
                IN  RvRtcpSession  hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpGetSSRC"));
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpGetSSRC"));
    return s->myInfo.ssrc;
}


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
RvInt32 RVCALLCONV RvRtcpSetSSRC(
                IN  RvRtcpSession  hRTCP,
                IN  RvUint32      ssrc)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSSRC"));

    s->myInfo.ssrc      = ssrc;
    s->myInfo.collision = 0;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSSRC"));
    return RV_OK;
}

                 /* == ENDS: Accessory RTCP Functions == */



                     /* == Internal RTCP Functions == */

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
                IN  RvInt32 *     ssrc)
{
    return RvRtcpGetEnumNext(hRTCP, -1, ssrc);
}
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
                IN  RvInt32 *     ssrc)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    rtcpInfo* info;
    RvInt32 index, doAgain = 1;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpGetEnumNext"));
    if (prev < 0)
        index = 0;
    else
        index = prev+1;

    while ((doAgain == 1)&&(index < s->sessionMembers))
    {
        info = &s->participantsArray[index];
        if (!info->invalid)
        {
            doAgain = 0;
            *ssrc = info->ssrc;
        }
        else
        {
            index++;
        }
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpGetEnumNext"));
    if (index < s->sessionMembers)
        return index;
    else
        return -1;
}


RVAPI
RvStatus RVCALLCONV rtcpCreateRTCPPacket(
                IN      RvRtcpSession  hRTCP,
                IN OUT  RvRtpBuffer*  buf)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    rtcpHeader head;
    RvUint32 allocated = 0;
    RvStatus res = RV_OK, senderResult = RV_ERROR_BADPARAM;

    RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpCreateRTCPPacket"));

    if (s == NULL)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "rtcpCreateRTCPPacket: NULL RTCP session pointer"));				
        RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpCreateRTCPPacket"));
        return RV_ERROR_NULLPTR;   
    }

    RvLockGet(&s->lock, logMgr);
    senderResult = rtcpAddRTCPPacketType(hRTCP, RTCP_SR, 0, 0, NULL, 0, buf, &allocated);
    if (senderResult == RV_ERROR_BADPARAM)  /* sender report inside contains receiver report data too */

        res = rtcpAddRTCPPacketType(hRTCP, RTCP_RR, 0, 0, NULL, 0, buf, &allocated);
    RvLockRelease(&s->lock, logMgr);

    rtcpAddRTCPPacketType(hRTCP, RTCP_SDES, 1/* set inside*/, 0, NULL, 0, buf, &allocated);
    
#if defined(__RTCP_XR__)
    if (s->rtcpxrplugin!=NULL && 
        s->rtcpxrplugin->callbacks!= NULL &&
        s->rtcpxrplugin->callbacks->rtcpAddRTCPXR != NULL)
    {        
         res = s->rtcpxrplugin->callbacks->rtcpAddRTCPXR(hRTCP, buf, &allocated);
    }
    if (res != RV_OK)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "rtcpCreateRTCPPacket: Cannot add RTCP XR blocks"));				
        RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpCreateRTCPPacket"));        
        return RV_ERROR_UNKNOWN;
    }
#endif

    if (s->myInfo.collision == 1  &&
        buffValid(buf, allocated + SIZEOF_RTCPHEADER))
    {
        head = makeHeader(s->myInfo.ssrc, 1, RTCP_BYE, SIZEOF_RTCPHEADER);
        /*   
		RvRtpBuffer bufC;
        bufC = buffCreate(&head, SIZEOF_RTCPHEADER);
        buffAddToBuffer(buf, &bufC, allocated);
        allocated += SIZEOF_RTCPHEADER;
		*/ 
		headerAddToBuffer(&head, SIZEOF_RTCPHEADER, buf, &allocated);
        s->myInfo.collision = 2;
    }
    buf->length = allocated;
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpCreateRTCPPacket"));
	RV_UNUSED_ARG(res);
    return RV_OK;
}


RvStatus RVCALLCONV rtcpProcessCompoundRTCPPacket(
        IN      RvRtcpSession  hRTCP,
        IN OUT  RvRtpBuffer*  buf,
        IN      RvUint64      myTime,
		IN		RvAddress*	  remoteAddress)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    rtcpHeader *head;
    RvUint8 *currPtr = buf->buffer, *dataPtr, *compoundEnd;
    RvInt32 hdr_count, hdr_len;
    RtcpType hdr_type;
    RvBool   hdr_paddingbit = RV_FALSE;
    RvUint32 Padding = 0;

    RTPLOG_ENTER(ProcessCompoundRTCPPacket);
    compoundEnd = buf->buffer + buf->length;

    while (currPtr < compoundEnd)
    {
        head = (rtcpHeader*)(currPtr);
        ConvertFromNetwork(currPtr, 0, 1);

        hdr_count = bitfieldGet(head->bits, HEADER_RC, HDR_LEN_RC);
        hdr_type  = (RtcpType)bitfieldGet(head->bits, HEADER_PT, HDR_LEN_PT);
        hdr_paddingbit = bitfieldGet(head->bits, HEADER_P, 1);
        if (hdr_paddingbit)
        {
           Padding = *(compoundEnd-1);
           compoundEnd = buf->buffer + buf->length - Padding;
        }
        hdr_len = sizeof(RvUint32) * (bitfieldGet(head->bits, HEADER_len, HDR_LEN_len)) - Padding;
        if ((compoundEnd - currPtr) < hdr_len)
        {
            RTPLOG_ERROR_LEAVE(ProcessCompoundRTCPPacket, "Illegal RTCP packet length");
            return ERR_RTCP_ILLEGALPACKET;
        }
        if ((hdr_type > RTCP_LAST_LEGAL && hdr_type < RTCP_SR) || (hdr_len>1472) || (hdr_len<0))  // MTU size restriction is added
        {
            RTPLOG_ERROR_LEAVE(ProcessCompoundRTCPPacket, "Illegal RTCP packet parameter header type or data length");
            return ERR_RTCP_ILLEGALPACKET;
        }
        dataPtr = (RvUint8 *)head + sizeof(RvUint32);

        rtcpProcessRTCPPacket(s, dataPtr, hdr_len, hdr_type, hdr_count, myTime, remoteAddress);

        currPtr += (hdr_len + sizeof(RvUint32));
    }

    RTPLOG_LEAVE(ProcessCompoundRTCPPacket);
    return RV_OK;
}



RvStatus RVCALLCONV rtcpProcessRTCPPacket(
        IN  rtcpSession *s,
        IN  RvUint8		*data,
        IN  RvInt32		dataLen,
        IN  RtcpType	type,
        IN  RvInt32		reportCount,
        IN  RvUint64	myTime,
		IN	RvAddress	*remoteAddress)
{
    unsigned scanned = 0;
	RvBool	presentSR = RV_FALSE;
    rtcpInfo info, *fInfo=NULL;
    RvUint32 roundTripDelay = 0;
    RvUint32 nowTS = 0;

    info.ssrc = 0;

	RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpProcessRTCPPacket"));
    if (dataLen == 0)
        return RV_OK;

    switch(type)
    {
        case RTCP_SR:
        case RTCP_RR:
        case RTCP_APP:
        case RTCP_XR:
        case RTCP_RTPFB:
		case RTCP_PSFB:
			{
            ConvertFromNetwork(data, 0, 1);
            info.ssrc = *(RvUint32 *)(data);
            scanned = sizeof(RvUint32);

            RvLockGet(&s->lock, logMgr);
            if (info.ssrc == s->myInfo.ssrc)
            {
                s->myInfo.collision = 1;
                RvLockRelease(&s->lock, logMgr);
                return ERR_RTCP_SSRCCOLLISION;
            }
                
            fInfo = findSSrc(s,info.ssrc);
            if (!fInfo) /* New source */
            {
                /* insert the new source */
                fInfo = insertNewSSRC(s, *(RvUint32 *)data, remoteAddress);
            }
			else
			{
				memcpy(&fInfo->remoteAddress, remoteAddress, sizeof (RvAddress));
			}

			if (fInfo)
			{
				fInfo->tLastReport = (RvUint32)RvUint64Div(RvTimestampGet(NULL), RV_TIME64_NSECPERMSEC);
			}			
			
            RvLockRelease(&s->lock, logMgr);
            break;
        }

        default:
            break;
    }

    /* process the information */
    switch(type)
    {
        case RTCP_SR:
        {
			presentSR = RV_TRUE;
            ConvertFromNetwork(data + scanned, 0, W32Len(SIZEOF_SR));

            if (fInfo)
            {
                RvUint32 msb, lsb;
                RvLockGet(&s->lock, logMgr);
				memcpy(&msb, (data + scanned), 4);                
                memcpy(&lsb, (data + scanned+4), 4);    
                fInfo->eSR.tNNTP = RvUint64Const(msb, lsb);
                memcpy(&(fInfo->eSR.tRTP),  (data + scanned + 8), 4);
                memcpy(&(fInfo->eSR.nPackets),  (data + scanned + 12), 4);
                memcpy(&(fInfo->eSR.nBytes),  (data + scanned + 16), 4);

                fInfo->eToRR.tLSR = reduceNNTP(fInfo->eSR.tNNTP);
				RTPLOG_DEBUG((RTP_SOURCE, "RTCPSR: LSR= %#x", fInfo->eFromRR.tLSR));

                fInfo->tLSRmyTime = reduceNNTP(myTime);
                RvLockRelease(&s->lock, logMgr);
            }

            scanned += SIZEOF_SR;
        }

        /* fall into RR */

        case RTCP_RR:
        {
            if ((fInfo != NULL) && ((RvInt32)scanned < dataLen))
            {
                RvInt32 i;
                rtcpRR  rr;
                RvBool  received = RV_FALSE;

                RvLockGet(&s->lock, logMgr);
                for (i=0; i < reportCount; i++)
                {
                    memcpy(&rr , data + scanned, sizeof(int));
                    ConvertFromNetwork(&rr, 0, W32Len(sizeof(int)));
                    if (rr.ssrc == s->myInfo.ssrc)
                    {
                        fInfo->eFromRR.ssrc = rr.ssrc;
                        memcpy(&fInfo->eFromRR.bfLost, data + scanned + 4, sizeof(rtcpRR)-4);
                        ConvertFromNetwork(&fInfo->eFromRR.bfLost, 0, W32Len(sizeof(rtcpRR))-1);                        
                        fInfo->eFromRR.valid = RV_TRUE;

                        nowTS = reduceNNTP(myTime);
                        if ((nowTS -  fInfo->eFromRR.tLSR) > fInfo->eFromRR.tDLSR)
                        {
                            roundTripDelay = nowTS -  fInfo->eFromRR.tLSR - fInfo->eFromRR.tDLSR; /* 16:16 */
                        }
                        else
                        {
                            roundTripDelay = fInfo->eFromRR.tDLSR - (nowTS - fInfo->eFromRR.tLSR);
                        }

                        fInfo->rtt              = (roundTripDelay*1000)>>16;

                        received = RV_TRUE;
                        break;
                    }
                    scanned += sizeof(rtcpRR);
                }
                RvLockRelease(&s->lock, logMgr);
                if (received && ((s->roundTripDelayCallback != NULL) || (s->roundTripDelayCallbackEx != NULL)))
                {
                    //RTPLOG_DEBUG((RTP_SOURCE, "A (%#x) - LSR (%#x)-DLSR(%#x)=%#x, RTD=%d", AValue, fInfo->eFromRR.tLSR, fInfo->eFromRR.tDLSR, roundTripDelay, rtdMs));
					if (s->roundTripDelayCallbackEx != NULL)
						s->roundTripDelayCallbackEx((RvRtcpSession) s, s->roundTripDelayContext, fInfo->ssrc, fInfo->rtt, RVRTCP_ROUND_TRIP_DELAY_RTCPSR);                            
					else 
						s->roundTripDelayCallback((RvRtcpSession) s, fInfo->ssrc, fInfo->rtt, RVRTCP_ROUND_TRIP_DELAY_RTCPSR);                            
                }
            }

			if ((s->rtcpSRRRMessageArrivedCallback != NULL) || (s->rtcpSRRRMessageArrivedCallbackEx != NULL))
			{
				RTCP_ReportType reportType = (presentSR == RV_TRUE) ? (RTCP_ReportType)RTCP_SR : (RTCP_ReportType)RTCP_RR;
				if (s->rtcpSRRRMessageArrivedCallbackEx != NULL)
				  s->rtcpSRRRMessageArrivedCallbackEx((RvRtcpSession)s, reportType, fInfo->ssrc, s->rtcpSRRRContext); 
				else 
				  s->rtcpSRRRMessageArrivedCallback((RvRtcpSession)s, reportType, fInfo->ssrc); 
			}
            break;
        }

        case RTCP_SDES:
            {
                RvInt32 i;
                rtcpSDES *sdes;

                for (i = 0; (i < reportCount) && ((RvInt32)scanned < dataLen); i++)
                {
                    ConvertFromNetwork(data + scanned, 0, 1);
                    info.ssrc = *(RvUint32 *)(data + scanned);

                    fInfo = findSSrc(s,info.ssrc);

                    sdes = (rtcpSDES *)(data + scanned + sizeof(info.ssrc));

                    if (fInfo != NULL)
                    {
                        scanned += sizeof(RvUint32);
                        do
                        {
                            switch(sdes->type)
                            {
                            default:
                                /* error */
                                break;
                            case RV_RTCP_SDES_CNAME:
                                memcpy(&(fInfo->eSDESCname), sdes, (RvSize_t)(sdes->length+2));
                                fInfo->eSDESCname.value[sdes->length] = 0;
                            break;
                            case RV_RTCP_SDES_NAME:
                            case RV_RTCP_SDES_EMAIL:
                            case RV_RTCP_SDES_PHONE:
                            case RV_RTCP_SDES_LOC:
                            case RV_RTCP_SDES_TOOL:
                            case RV_RTCP_SDES_NOTE:
                            case RV_RTCP_SDES_PRIV:
								if ((NULL!=s->rtcpSDESMessageArrivedCallback) || (NULL!=s->rtcpSDESMessageArrivedCallbackEx))
								{
								   RvUint8 SDESstring[256];
								   memcpy(SDESstring, sdes->value, sdes->length);
                                   SDESstring[sdes->length] = 0;
								   RvLogDebug(rvLogPtr, (rvLogPtr, "rtcpProcessRTCPPacket: Optional SDES packet received: SSRC"
									                               ":%#x, Type =%d, string:'%s'", info.ssrc, sdes->type, sdes->value));
								   if (NULL!=s->rtcpSDESMessageArrivedCallbackEx)
								     s->rtcpSDESMessageArrivedCallbackEx((RvRtcpSession)s, s->rtcpSDESContext, (RvRtcpSDesType)sdes->type, info.ssrc, (RvUint8*)sdes->value, (RvUint32)sdes->length);
								   else 
								   s->rtcpSDESMessageArrivedCallback((RvRtcpSession)s,	(RvRtcpSDesType)sdes->type,	info.ssrc, (RvUint8*)sdes->value, (RvUint32)sdes->length);
								}
                                break;
                            }
                            scanned += (sdes->length+2);
                            sdes = (rtcpSDES *)(data + scanned);
                        }
                        while (*(RvUint8 *)(data + scanned) != 0 && ((RvInt32)scanned < dataLen));
                        scanned = (scanned+3) & 0xFFFFFFFC; /* to skip to relevant alignment */
                    }
                }
                break;
            }
        case RTCP_BYE:
        {
            RvInt32 i;
			RvInt32		ssrcsLen = reportCount * sizeof (info.ssrc); 
			RvUint8		reasonLen = 0;
			RvUint8*	reasonPtr = NULL;
			
			if (ssrcsLen < dataLen)
			{
				/* BYE with reason */
				reasonLen = *(RvUint8 *) (data + ssrcsLen);
				reasonPtr = data + ssrcsLen + 1/* the 1 byte of length */;
			}

			scanned = 0;
            for (i = 0; i < reportCount; i++)
            {
                ConvertFromNetwork(data + scanned, 0, 1);
                info.ssrc = *(RvUint32 *)(data + scanned);
                scanned += sizeof(info.ssrc);

                fInfo = findSSrc(s,info.ssrc);
                if ((fInfo != NULL) )
                {
                    /* Inform user about every SSRC/CSRC in BYE message */
					if (s->rtcpByeRecvCallback!=NULL)
					{
						s->rtcpByeRecvCallback((RvRtcpSession)s, s->rtcpByeContext, info.ssrc, reasonPtr, reasonLen);
					}
                    
					/* We don't really delete this SSRC, we just mark it as invalid */
					RvLockGet(&s->lock, logMgr);
                    fInfo->invalid = RV_TRUE;
                    fInfo->ssrc    = 0;
                    RvLockRelease(&s->lock, logMgr);
                    /* number of session members is still remained unchanged */
                }
            }
            break;
        }

        case RTCP_APP:
        {
			if ((fInfo != NULL) && ((RvInt32)scanned < dataLen))
            {
#if defined(RTP_CFG_APP_ALL_USER_DATA_UINT32)
                ConvertFromNetwork(data + scanned, 0, W32Len(dataLen-scanned));
#elif  defined(RTP_CFG_APP_NAME_IS_UINT32)
                ConvertFromNetwork(data + scanned, 0, 1);
#endif
                if (NULL!= s->rtcpAppRecvCallback)
				{
                   s->rtcpAppRecvCallback((RvRtcpSession)s, s->rtcpAppContext, (RvUint8) reportCount, info.ssrc,
                    (RvUint8 *)(data + scanned),(RvUint8*)(data + scanned + 4), (dataLen-scanned-4));
				}
                scanned += (dataLen-scanned);
            }
        }
            break;
        case RTCP_EMPTY_RR: /* no such report, produced in RTCP_RR */
        break;
        case RTCP_XR:
            {
#ifdef __RTCP_XR__
                if ((fInfo != NULL) && ((RvInt32)scanned < dataLen))
                {
                    if (s->rtcpxrplugin != NULL &&
                        s->rtcpxrplugin->callbacks != NULL &&
                        s->rtcpxrplugin->callbacks->rtcpXrPacketReceived != NULL)
                    {   
                        ConvertFromNetwork(data + scanned, 0, W32Len(dataLen-scanned));
                        s->rtcpxrplugin->callbacks->rtcpXrPacketReceived(
                            s->rtcpxrplugin, (RvRtcpSession)s, fInfo, (RvUint8 *)(data + scanned), (dataLen - scanned), myTime);
                    }
                    scanned += (dataLen-scanned);
                }
#endif /* __RTCP_XR__ */
            }
        break;
        case RTCP_RTPFB:
		case RTCP_PSFB:
            {
				RvUint32 adresseeSSRC;
				
				ConvertFromNetwork(data + scanned, 0, W32Len(dataLen-scanned));

				adresseeSSRC = *(RvUint32 *)(data + scanned);
				/* was this packet sent to us? */
				if (adresseeSSRC == s->myInfo.ssrc)
				{
					RvLogInfo(rvLogPtr, (rvLogPtr, "============== Feedback RECEIVED!  ================"));
				
					if ((fInfo != NULL) && ((RvInt32)scanned < dataLen))
					{
#ifdef __RTCP_FB__
						if (s->rtcpFbPlugin != NULL)
						{   
							/* We do not have to check that 
							s->rtcpFbPlugin->callbacks != NULL and 
							s->rtcpFbPlugin->callbacks->rtcpFbPacketReceived != NULL
							The pointers are set while constructing FB plugin.
							*/
							RvUint32 subType = reportCount;
							RvUint32 senderSSRC = *(RvUint32 *)(data);
							s->rtcpFbPlugin->callbacks->rtcpFbPacketReceived((RvRtcpSession)s, senderSSRC, 
																   type, subType, (RvUint8 *)(data + scanned), (dataLen - scanned));
                            
						}
#endif
					}
				}
            }
			break;
    }

    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpProcessRTCPPacket"));
    return RV_OK;
}

static RvUint32 rtcpGetEstimatedRTPTime(IN RvRtcpSession  hRTCP,
                                        IN const RvUint64 *ntpNewTimestampPtr)
{

    rtcpSession *s = (rtcpSession *)hRTCP;
    RvUint32   estimatedTime = s->myInfo.lastPackRTPts;
    RvLogEnter(rvLogPtr,(rvLogPtr, "rtcpDatabaseGetEstimatedRTPTime"));

    /* Generate estimate, if we have sent at least one RTP packet */
/*    if(RvNtpTimeGetSecs(&thisPtr->ntpTimestamp) != 0)*/
    if ( s->myInfo.lastPackNTP != RvUint64Const(0,0))
    {
        RvUint64  ntpDelta;
        RvUint32  ntpDeltaReduced;
        if (s->myInfo.rtpClockRate == 0)
            s->myInfo.rtpClockRate = RvRtpGetStandardPayloadClockRate(s->myInfo.rtpPayloadType);

        /* Compute the NTP Delta */
        ntpDelta = RvInt64Sub(*ntpNewTimestampPtr, s->myInfo.lastPackNTP);

        ntpDeltaReduced = RvUint64ToRvUint32(
            RvInt64ShiftRight(RvInt64ShiftRight(RvInt64ShiftLeft(ntpDelta, 16), 16), 16));
        /* 2 ShiftsRight 16 removes warning in VxWorks
         was RvInt64ShiftRight(RvInt64ShiftLeft(ntpDelta, 16), 32));*/
        /* Compute the estimate with rounding (ntpDeltaReduced is int fixed point 16.16) */
        estimatedTime += (((s->myInfo.rtpClockRate * ntpDeltaReduced)  + 0x8000) >> 16);
    }

    RvLogLeave(rvLogPtr,(rvLogPtr, "rtcpDatabaseGetEstimatedRTPTime"));

    return estimatedTime;
}

/********************************************************************************
 * RFC 3550
 ********************************************************************************/
/*
  6.7 APP: Application-Defined RTCP Packet

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |V=2|P| subtype |   PT=APP=204  |             length            |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                           SSRC/CSRC                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          name (ASCII)                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                   application-dependent data                ...
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

      The APP packet is intended for experimental use as new applications
      and new features are developed, without requiring packet type value
      registration.  APP packets with unrecognized names SHOULD be ignored.
      After testing and if wider use is justified, it is RECOMMENDED that
      each APP packet be redefined without the subtype and name fields and
      registered with IANA using an RTCP packet type.

      version (V), padding (P), length:
      As described for the SR packet (see Section 6.4.1).

      subtype: 5 bits
             May be used as a subtype to allow a set of APP packets to be
             defined under one unique name, or for any application-dependent
             data.

             packet type (PT): 8 bits
             Contains the constant 204 to identify this as an RTCP APP packet.

      name: 4 octets
            A name chosen by the person defining the set of APP packets to be
            unique with respect to other APP packets this application might
            receive.  The application creator might choose to use the
            application name, and then coordinate the allocation of subtype
            values to others who want to define new packet types for the
            application.  Alternatively, it is RECOMMENDED that others choose
            a name based on the entity they represent, then coordinate the use
            of the name within that entity.  The name is interpreted as a
            sequence of four ASCII characters, with uppercase and lowercase
            characters treated as distinct.

          application-dependent data: variable length
            Application-dependent data may or may not appear in an APP packet.
            It is interpreted by the application and not RTP itself.  It MUST
            be a multiple of 32 bits long.
          */
/*********************************************************************************************
 *  rtcpAddRTCPPacketType
 * Private function
 * description: adds the RTCP packet type to the RTCP packet to be sent.
 *              Possibilities SR, RR, SDES, APP, BYE, EMPTY RR XR
 * input: 
 *        hRTCP - The handle of the RTCP session.
 *        type  - the RTCP packet type
 *        subtype - the report header related subtype information
 *        name    - the name value for RTCP APP message
 *        userData       - application-dependent data pointer for RCTP APP message
 *        userDataLength - the length of userData
 * output: 
 *        buf            - pointer to RvRtpBuffer, to which this packet type was added
 *        pCurrentIndex  - pointer to the first free location in buffer buf
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value
 ***********************************************************************************/
static RvStatus rtcpAddRTCPPacketType(
							IN RvRtcpSession	hRTCP,
                            IN RtcpType			type,
                            IN RvUint8			subtype,
                            IN RvUint8*			name,
                            IN void *			userData, /* application-dependent data  */
                            IN RvUint32			userDataLength,
                            IN OUT RvRtpBuffer*	buf,
                            IN OUT RvUint32*	pCurrentIndex)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    rtcpHeader head;
    RvUint32 allocated = *pCurrentIndex;
    RvRtpBuffer bufC;
	RvStatus res=RV_OK;

    RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType"));
    switch (type)
    {
    case RTCP_APP:
        {
            if (buffValid(buf, allocated + SIZEOF_RTCPHEADER + 4 + userDataLength))
            {
                /* Header code */
                if (NULL!=userData &&userDataLength>0)
                    head = makeHeader(s->myInfo.ssrc, subtype, type, (RvUint16) (SIZEOF_RTCPHEADER + 4 + userDataLength)); /* sizeof(Packet) in 4 octet words - 1 */
                else
                    head = makeHeader(s->myInfo.ssrc, subtype, type, (RvUint16) (SIZEOF_RTCPHEADER + 4)); /* sizeof(Packet) in 4 octet words - 1 */
				headerAddToBuffer(&head, SIZEOF_RTCPHEADER, buf, &allocated);
				
#if defined(RTP_CFG_APP_NAME_IS_UINT32)||defined(RTP_CFG_APP_ALL_USER_DATA_UINT32)
                dataAddToBufferAndConvert(name, 4, buf, &allocated);
#else
                dataAddToBuffer(name, 4, buf, &allocated);
#endif
				

                if (NULL!=userData &&userDataLength>0)
                {
#if defined(RTP_CFG_APP_NAME_IS_UINT32)||defined(RTP_CFG_APP_ALL_USER_DATA_UINT32)
					dataAddToBufferAndConvert(userData, userDataLength, buf, &allocated);
#else
					dataAddToBuffer(userData, userDataLength, buf, &allocated);
#endif
                }
            }
            else
			{
                RvLogError(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType:RTCP_APP out of resources error"));				
                res = RV_ERROR_OUTOFRESOURCES;
			}
            break;
        }
    case RTCP_SDES:
        {
            RvInt32 Count = 1, Len = 0;
            for (Count = 1; Count <= RV_RTCP_SDES_PRIV;Count++)
                if (s->myInfo.SDESbank[Count].length)
                    Len += (s->myInfo.SDESbank[Count].length + 2);
            /* +1 for for SDES type  */
            /* +1 for for SDES length  */

            Len = (Len+4) & 0xFFFFFFFC;
            /* +1 for "NULL" termination of data items chunk */
            /* and Padding  at the end of section */

            if (buffValid(buf, allocated + SIZEOF_RTCPHEADER + Len))
            {
                RvInt32 Count = 0;
                RvRtpBuffer sdes_buf;
                /* 'sdes_buf' is inside the compound buffer 'buf' */
                sdes_buf = buffCreate(buf->buffer + allocated, (SIZEOF_RTCPHEADER + Len));

                head = makeHeader(s->myInfo.ssrc, 1, RTCP_SDES, (RvUint16)sdes_buf.length);
				headerAddToBuffer(&head, SIZEOF_RTCPHEADER, buf, &allocated);	
				
                for (Count = 1; Count <= RV_RTCP_SDES_PRIV;Count++)
                {
                    if (s->myInfo.SDESbank[Count].length)
                    {
						dataAddToBuffer(&(s->myInfo.SDESbank[Count]), s->myInfo.SDESbank[Count].length+2, buf, &allocated);	
                    }
                }
                *(RvUint8*)(buf->buffer + allocated) = '\0';
                allocated++;
                allocated += (*pCurrentIndex + sdes_buf.length - allocated); /* padding */
            }
            else
			{
                RvLogError(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType:RTCP_SDES out of resources error"));				
                res = RV_ERROR_OUTOFRESOURCES;
			}
            break;
        }
    case RTCP_SR:
        {
            RvInt32            index;
            s->myInfo.eSR.tNNTP = getNNTPTime();
            s->myInfo.eSR.tRTP =  rtcpGetEstimatedRTPTime(hRTCP, &s->myInfo.eSR.tNNTP);

            /* time of sending the report and not time of sending the last RTP packet   */
            if (buffValid(buf, SIZEOF_RTCPHEADER + SIZEOF_SR))
            {
                RvUint64 myTime = s->myInfo.eSR.tNNTP;
                RvUint8 cc = 0;
                rtcpInfo *info;

                allocated += SIZEOF_RTCPHEADER;
                /* RTCP Header setting later */
                if (s->myInfo.active==2)
                {
                    s->myInfo.active --;
                    {
                        RvUint32 msb, lsb;
                        msb = RvUint64ToRvUint32(RvUint64ShiftRight(RvUint64ShiftRight(s->myInfo.eSR.tNNTP, 16),16));
                        lsb = RvUint64ToRvUint32(RvUint64And(s->myInfo.eSR.tNNTP, RvUint64Const(0, 0xffffffff)));
                        
						dataAddToBufferAndConvert(&msb, sizeof(RvUint32), buf, &allocated);
							
						/* instead: */
						dataAddToBufferAndConvert(&lsb, sizeof(RvUint32), buf, &allocated);
                    }
					dataAddToBufferAndConvert(&(s->myInfo.eSR.tRTP), sizeof(RvUint32)*3, buf, &allocated);
						
                }
                else
                {
                    s->myInfo.active--;
                    allocated -= SIZEOF_RTCPHEADER;
					RvLogWarning(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType: SR is not issued, because no RTP data were sent since previous SR."));				
                    res = RV_ERROR_BADPARAM; /* cannot send sender report no new sent packets */
					break;
                }
                index = 1; /* 0 my SSRC */

                while( index < s->sessionMembers && cc < 31)
                {
                    info = &s->participantsArray[index];
                    if ((!info->invalid)&&info->active)
                    {
                        info->eToRR.bfLost     = getLost    (&(info->src));
                        info->eToRR.nJitter    = getJitter  (&(info->src));
                        info->eToRR.nExtMaxSeq = getSequence(&(info->src));
                        info->eToRR.tDLSR      = /* time of sending - time of receiving previous report*/
                            (info->tLSRmyTime) ?
                            (reduceNNTP(myTime)-info->tLSRmyTime) : 0;

                        info->eToRR.valid = RV_TRUE;

						if (dataAddToBufferAndConvert(&(info->eToRR), SIZEOF_RR, buf, &allocated))
                            cc++;
						
                        info->active = RV_FALSE;
                    }
                    index++;
                }
                head = makeHeader(s->myInfo.ssrc, cc, type, (RvUint16)(allocated - *pCurrentIndex));
                bufC = buffCreate(&head, SIZEOF_RTCPHEADER);
                buffAddToBuffer(buf, &bufC, *pCurrentIndex);
            }
			else
			{
                RvLogError(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType:RTCP_SR out of resources error"));				
                res = RV_ERROR_OUTOFRESOURCES;
			}
        }
        break;
    case RTCP_RR:
        {
            RvInt32            index;

            if (buffValid(buf, SIZEOF_RTCPHEADER + SIZEOF_RR))
            {
                RvUint64 myTime = getNNTPTime();
                RvUint8 cc = 0;
                rtcpInfo *info;

                allocated += SIZEOF_RTCPHEADER;
                /* RTCP Header setting later */
                index = 1; /* 0 my SSRC */

                while( index < s->sessionMembers && cc < 31)
                {
                    info = &s->participantsArray[index];
                    if ((!info->invalid)&&info->active)
                    {
                        info->eToRR.bfLost     = getLost    (&(info->src));
                        info->eToRR.nJitter    = getJitter  (&(info->src));
                        info->eToRR.nExtMaxSeq = getSequence(&(info->src));
                        info->eToRR.tDLSR      =
                            (info->tLSRmyTime) ?
                            (reduceNNTP(myTime)-info->tLSRmyTime) : 0;

                        info->eToRR.valid = RV_TRUE;

						/**
                        bufC = buffCreate(&(info->eToRR), SIZEOF_RR);
                        if (buffAddToBuffer(buf, &bufC, allocated))
                        {
                            cc++;
                            ConvertToNetwork(buf->buffer + allocated, 0, W32Len(bufC.length));
                            allocated += SIZEOF_RR;
                        }
						**/
						if (dataAddToBufferAndConvert(&(info->eToRR), SIZEOF_RR, buf, &allocated))
                            cc++;
                        info->active = RV_FALSE;
                    }
                    index++;
                }
                head = makeHeader(s->myInfo.ssrc, cc, type, (RvUint16)(allocated - *pCurrentIndex));
                bufC = buffCreate(&head, SIZEOF_RTCPHEADER);
                buffAddToBuffer(buf, &bufC, *pCurrentIndex);
            }
			else
			{
                RvLogError(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType:RTCP_RR out of resources error"));				
                res = RV_ERROR_OUTOFRESOURCES;
			}
        }
        break;
    case RTCP_BYE:
        if (buffValid(buf, allocated + SIZEOF_RTCPHEADER + userDataLength+(RTCPHEADER_SSRCMAXCOUNT-1)*4))
        {
            RvUint8	Padding[3] = {0,0,0};
            RvUint8	reasonLen =  (RvUint8) userDataLength;
            RvInt32	index = 0; /* 0 is s->myInfo.ssrc */
            RvInt32 ssrcCount = 1; /* 1 - we are! */
            RvInt32 ssrcSize = 0;
            rtcpInfo* info = NULL;
            /* search for all active members */
            while( index < s->sessionMembers)
            {
                info = &s->participantsArray[index];
                if (info->active) /* or validity check ? */
                     ssrcCount++;
                index++;
            }
			
            if (ssrcCount> RTCPHEADER_SSRCMAXCOUNT)
                ssrcCount = RTCPHEADER_SSRCMAXCOUNT;

            ssrcSize = (ssrcCount-1)*sizeof(RvUint32); /* not included own SSRC */

            if (NULL!=userData && userDataLength>0)
                head = makeHeader(s->myInfo.ssrc, (RvUint8)ssrcCount, RTCP_BYE, (RvUint16) (SIZEOF_RTCPHEADER + ssrcSize + userDataLength + 1));
            else
                head = makeHeader(s->myInfo.ssrc, (RvUint8)ssrcCount, RTCP_BYE, (RvUint16)(SIZEOF_RTCPHEADER + ssrcSize));

			/*
            bufC = buffCreate(&head, SIZEOF_RTCPHEADER);
            buffAddToBuffer(buf, &bufC, allocated);
            allocated += bufC.length;
			*/ 
            headerAddToBuffer(&head, SIZEOF_RTCPHEADER, buf, &allocated); 

            index = 1;
            while( index < s->sessionMembers && index < ssrcCount)
            {
                info = &s->participantsArray[index];
                if (info->active) /* or validity check ? */
                {
                    RvUint32 SSRC = info->ssrc;
                    ConvertToNetwork(&SSRC, 0, 1);

					/*
                    bufC = buffCreate(&info->ssrc, 4);
                    buffAddToBuffer(buf, &bufC, allocated);
                    allocated += bufC.length;
					*/
					dataAddToBuffer(&info->ssrc, 4, buf, &allocated);
                }
                index++;
            }
            /* adding the reason length */
            if (reasonLen>0)
            {
				/*
                bufC = buffCreate(&reasonLen, 1);
                buffAddToBuffer(buf, &bufC, allocated);
                allocated += bufC.length;
				*/ 
				dataAddToBuffer(&reasonLen, 1, buf, &allocated);
				
                /* adding the reason */
				/*
                bufC = buffCreate(userData, userDataLength);
                buffAddToBuffer(buf, &bufC, allocated);
                allocated += bufC.length;
				*/
				dataAddToBuffer(userData, userDataLength, buf, &allocated);
				
                if ((userDataLength+1)&0x00000003) /* must be filled by NULL pudding */
                {
                    RvInt32 PaddingSize = 4 - ((userDataLength+1)&0x00000003);
                    /*
                    bufC = buffCreate(Padding, PaddingSize);
                    buffAddToBuffer(buf, &bufC, allocated);
                    allocated += bufC.length;
					*/
					dataAddToBuffer(Padding, PaddingSize, buf, &allocated);
                }
            }
        }
		else
		{
			RvLogError(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType:RTCP_BYE out of resources error"));				
			res = RV_ERROR_OUTOFRESOURCES;
		}	
        break;
    case RTCP_EMPTY_RR:
        {
            if (buffValid(buf, SIZEOF_RTCPHEADER + SIZEOF_RR))
            {
                /* no receiver reports incide */
                head = makeHeader(s->myInfo.ssrc, 0, RTCP_RR, (RvUint16)(SIZEOF_RTCPHEADER));
				/*
                bufC = buffCreate(&head, SIZEOF_RTCPHEADER);
                buffAddToBuffer(buf, &bufC, allocated);
                allocated += SIZEOF_RTCPHEADER;
				*/
				headerAddToBuffer(&head, SIZEOF_RTCPHEADER, buf, &allocated);
            }
			else
			{
				RvLogError(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType:RTCP_EMPTY_RR out of resources error"));				
				res = RV_ERROR_OUTOFRESOURCES;
			}
        }
        break;
	
		
			
    default:
        break;
    }

    *pCurrentIndex = allocated;
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpAddRTCPPacketType"));
    return res;
}
/*********************************************************************************************
 *  RvRtcpSessionSendApps
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
 *  included in any statistical calculations, including bandwidth limitations.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendApps(IN RvRtcpSession hRTCP,
                                          IN RvRtcpAppMessage* appMessageTable,
                                          IN RvInt32 appMessageTableEntriesNum,
                                          IN RvBool bCompound)
{
/*  rtcpSession *s = (rtcpSession *)hRTCP;*/
    RvUint32 allocated = 0;
    RvUint32 data[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer buf;
    RvStatus res = RV_OK;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendApps"));

    buf = buffCreate(data+2, MAXRTCPPACKET-8);

    if (bCompound)
    {
        res = rtcpAddRTCPPacketType(hRTCP, RTCP_EMPTY_RR, 0, 0, NULL, 0, &buf, &allocated);
        res = rtcpAddRTCPPacketType(hRTCP, RTCP_SDES, 1/* set inside*/, 0, NULL, 0, &buf, &allocated);
    }
    if ((appMessageTableEntriesNum>0) &&(appMessageTable!=NULL))
    {
        RvInt32 Count = 0;
        for (Count = 0;Count < appMessageTableEntriesNum;Count++)
            if (appMessageTable[Count].userData!=NULL)
                rtcpAddRTCPPacketType(hRTCP, RTCP_APP, appMessageTable[Count].subtype,
                    appMessageTable[Count].name, appMessageTable[Count].userData,
                    appMessageTable[Count].userDataLength, &buf, &allocated);
    }
/*  return RvSocketSendBuffer(&s->socket, buf.buffer, (RvSize_t)allocated, &s->remoteAddress, logMgr, NULL);*/
    res = rtcpSessionRawSend(hRTCP, buf.buffer, allocated, MAXRTCPPACKET-8 , NULL);
    if (res >= 0)
    {
       RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendApps - Manual RTCP APP message was sent"));
    }
    else
    {
       RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendApps - failed to send RTCP APP message, error=%d", res));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendApps"));
    return res;
}

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
 *	included in any statistical calculations, including bandwidth limitations. 
 * 2) appMessageTable and appMessageTableEntriesNum are used only if bCompound is set to RV_TRUE
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendSR(
        IN RvRtcpSession hRTCP,
        IN RvBool bCompound,
        IN RvRtcpAppMessage* appMessageTable,
        IN RvInt32 appMessageTableEntriesNum)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvUint32 allocated = 0;
    RvUint32 data[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer buf;
    RvStatus senderResult = RV_ERROR_UNKNOWN;
    RvStatus res = RV_OK;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendSR"));

    if (s == NULL)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendSR: NULL RTCP session pointer"));				
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendSR"));
        return RV_ERROR_NULLPTR;   
    }
    buf = buffCreate(data+2, MAXRTCPPACKET-8);
    
    RvLockGet(&s->lock, logMgr);
    senderResult = rtcpAddRTCPPacketType(hRTCP, RTCP_SR, 0, 0, NULL, 0, &buf, &allocated);
    if (senderResult == RV_ERROR_BADPARAM)
        /* sender report inside contains recever report data too */
        res = rtcpAddRTCPPacketType(hRTCP, RTCP_RR, 0, 0, NULL, 0, &buf, &allocated);
    RvLockRelease(&s->lock, logMgr);

    if (bCompound)
    {
        res = rtcpAddRTCPPacketType(hRTCP, RTCP_SDES, 1/* set inside*/, 0, NULL, 0, &buf, &allocated);
        if ((appMessageTableEntriesNum>0) &&(appMessageTable!=NULL))
        {
            RvInt32 Count = 0;
            for (Count = 0;Count < appMessageTableEntriesNum;Count++)
                if (appMessageTable[Count].userData!=NULL)
                    rtcpAddRTCPPacketType(hRTCP, RTCP_APP, appMessageTable[Count].subtype,
                    appMessageTable[Count].name, appMessageTable[Count].userData,
                    appMessageTable[Count].userDataLength, &buf, &allocated);
        }
    }
    /*  return RvSocketSendBuffer(&s->socket, buf.buffer, (RvSize_t)allocated, &s->remoteAddress, logMgr, NULL);*/
    res = rtcpSessionRawSend(hRTCP, buf.buffer, allocated, MAXRTCPPACKET-8, NULL);
    if (res>=0)
    {
       RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendSR - Manual RTCP Sender Report was sent"));
    }
    else
    {
       RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendSR - failed to send Manual RTCP Sender Report, error=%d", res));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendSR"));
    return res;
}

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
 *	included in any statistical calculations, including bandwidth limitations. 
 * 2) appMessageTable and appMessageTableEntriesNum are used only if bCompound is set to RV_TRUE
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendRR(
        IN RvRtcpSession hRTCP,
        IN RvBool bCompound,
        IN RvRtcpAppMessage* appMessageTable,
        IN RvInt32 appMessageTableEntriesNum)
{
    /*    rtcpSession *s = (rtcpSession *)hRTCP;*/
    RvUint32 allocated = 0;
    RvUint32 data[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer buf;
    RvStatus res = RV_OK;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendRR"));

    buf = buffCreate(data+2, MAXRTCPPACKET-8); /* resulting packet */
    res = rtcpAddRTCPPacketType(hRTCP, RTCP_RR, 0, 0, NULL, 0, &buf, &allocated);

    if (bCompound)
    {
        res = rtcpAddRTCPPacketType(hRTCP, RTCP_SDES, 1/* set inside*/, 0, NULL, 0, &buf, &allocated);
        if ((appMessageTableEntriesNum>0) &&(appMessageTable!=NULL))
        {
            RvInt32 Count = 0;
            for (Count = 0;Count < appMessageTableEntriesNum;Count++)
                if (appMessageTable[Count].userData!=NULL)
                    rtcpAddRTCPPacketType(hRTCP, RTCP_APP, appMessageTable[Count].subtype,
                    appMessageTable[Count].name, appMessageTable[Count].userData,
                    appMessageTable[Count].userDataLength, &buf, &allocated);
        }
    }
    /*  return RvSocketSendBuffer(&s->socket, buf.buffer, (RvSize_t)allocated, &s->remoteAddress, logMgr, NULL);*/
    res = rtcpSessionRawSend(hRTCP, buf.buffer, allocated, MAXRTCPPACKET-8, NULL);
    if (res >= 0)
    {
       RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendRR - Manual RTCP Receiver Report was sent"));
    }
    else
    {
       RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendRR - failed to send Manual RTCP Receiver Report, error=%d", res));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendRR"));
    return res;
}

/************************************************************************************
 *	RvRtcpSessionSendBye  Sends immediate compound BYE without closing session. 
 *  input:  hRTCP - Handle of RTP session.	
 *          reason - reason for BYE
 *          length - length of the reason
 *  return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value 
 * Note: 1) after this function RvRtpClose have to close the session
 *       2) this message is not included in RTCP bandwidth
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSendBye(
        IN RvRtcpSession hRTCP,
        IN const char *reason,
        IN  RvInt32 length)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvUint32 allocated = 0;
    RvUint32 data[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer buf;
    RvStatus res = RV_OK;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendBye"));
    if (!s->isShutdown)
        s->isShutdown = RV_TRUE;
    buf = buffCreate(data+2, MAXRTCPPACKET-8); /* resulting packet */

    /* send Bye message immediately */

    res = rtcpAddRTCPPacketType(hRTCP, RTCP_EMPTY_RR, 0, 0, NULL, 0, &buf, &allocated);
    res = rtcpAddRTCPPacketType(hRTCP, RTCP_SDES, 1/* set inside*/, 0, NULL, 0, &buf, &allocated);
    res = rtcpAddRTCPPacketType(hRTCP, RTCP_BYE, 0, 0, (RvUint8*)reason, length, &buf, &allocated);

    /*  return RvSocketSendBuffer(&s->socket, buf.buffer, (RvSize_t)allocated, &s->remoteAddress, logMgr, NULL);*/
    res = rtcpSessionRawSend(hRTCP, buf.buffer, allocated, MAXRTCPPACKET-8, NULL);
    if (res >= 0)
    {
       RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendBye -  manual RTCP BYE message was sent"));
    }
    else
    {
       RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendBye - failed to send manual RTCP BYE message, error=%d", res));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendBye"));
    return res;
}




RvStatus  rtcpSessionSendShutdownBye(
    IN  RvRtcpSession hRTCP,
    IN  const char *reason,
    IN  RvInt32 length)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvUint32 allocated = 0;
    RvUint32 data[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer buf;
    RvStatus res = RV_OK;
    RvUint32 packLen = 0;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendBye"));
    if (!s->isShutdown)
        s->isShutdown = RV_TRUE;
    buf = buffCreate(data+2, MAXRTCPPACKET-8); 
    
    /* send Bye message immediately */
    
    res = rtcpAddRTCPPacketType(hRTCP, RTCP_EMPTY_RR, 0, 0, NULL, 0, &buf, &allocated);
    res = rtcpAddRTCPPacketType(hRTCP, RTCP_SDES, 1/* set inside*/, 0, NULL, 0, &buf, &allocated);
    res = rtcpAddRTCPPacketType(hRTCP, RTCP_BYE, 0, 0, (RvUint8*)reason, length, &buf, &allocated);
    
    /*  return RvSocketSendBuffer(&s->socket, buf.buffer, (RvSize_t)allocated, &s->remoteAddress, logMgr, NULL);*/
    res = rtcpSessionRawSend(hRTCP, buf.buffer, allocated, MAXRTCPPACKET-8, &packLen);
    s->txInterval.lastRTCPPacketSize = packLen + RTCP_PACKETOVERHEAD /* IP/UDP headers ? */;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSendBye"));
    return res;
}

static RvBool rtcpByeTimerCallback(IN void* key)
{
    rtcpSession* s = (rtcpSession*)key;
    RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpByeTimerCallback"));
    RvRtcpSessionSendBye((RvRtcpSession)s, s->byeReason, s->byeReasonLength);
    if (NULL!=s->rtpShutdownCompletedCallback)
        s->rtpShutdownCompletedCallback(s->rtpSession, s->byeReason, s->rtcpShutdownCompletedContext);
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpByeTimerCallback"));
    return RV_FALSE;
}
/************************************************************************************
 * RvRtpSetShutdownCompletedEventHandler
 * description: Set an Event Handler for the RTP session. The application must set
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
        IN  void *                     context)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetShutdownCompletedEventHandler"));
    if (NULL == hRTP || NULL ==hRTCP)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetShutdownCompletedEventHandler: NULL pointers"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetShutdownCompletedEventHandler"));
        return; /* error */
    }
    s->rtpSession = hRTP;
    s->rtpShutdownCompletedCallback = eventHandler;
    s->rtcpShutdownCompletedContext = context;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetShutdownCompletedEventHandler"));
}
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
RvStatus RVCALLCONV RvRtcpSessionShutdown(
		IN RvRtcpSession  hRTCP,
        IN RvChar *reason,
        IN RvUint8 reasonLength)
{
    RvTimerQueue *timerQ = NULL;
    RvChar *reasonText = NULL;
    RvUint32 textLength = 0, delay = 0;
    RvStatus status;
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown"));
    if (NULL==s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown: NULL pointer or reason is too big"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown"));
        return RV_ERROR_UNKNOWN;
    }
    s->isShutdown = RV_TRUE;
    if (!s->isTimerSet)
    {
        RvLogWarning(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown: no need for shutdown - session does not send data"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown"));
        return RV_OK;
    }
    /* Find the timer queue we are working with */
    status = RvSelectGetTimeoutInfo(s->selectEngine, NULL, &timerQ);
    if (status != RV_OK)
        timerQ = NULL;

    if (timerQ == NULL)
        timerQ = rvRtcpInstance.timersQueue;

    reasonText = reason;
    textLength = reasonLength;

    RvTimerCancel(&s->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
    s->myInfo.active = 0;      /* no senders from now */
    if (s->sessionMembers < RTCP_BYEBACKOFFMINIMUM)
    {
        /* send Bye message immediately */
        rtcpSessionSendShutdownBye(hRTCP, reasonText, reasonLength);
        if (NULL!=s->rtpShutdownCompletedCallback)
            s->rtpShutdownCompletedCallback(s->rtpSession, reason, s->rtcpShutdownCompletedContext);
    }
    else
    {
        RvInt64 currentTime = RvTimestampGet(logMgr);
        delay = rtcpGetTimeInterval(s, RTCP_MIN_TIME); /* in ms */
        s->txInterval.nextTime = RvInt64Add(s->txInterval.previousTime,
            RvInt64Mul(RvInt64FromRvUint32(delay), RV_TIME64_NSECPERMSEC));

        if (s->txInterval.nextTime > currentTime)
        {           /* We can't send BYEs now, so remember reason for later */
            s->byeReason       = reason;
            s->byeReasonLength = reasonLength;
            status = RvTimerStart(&s->timer, timerQ, RV_TIMER_TYPE_ONESHOT,
                RvInt64Mul(RV_TIME64_NSECPERMSEC,
                RvInt64Sub(s->txInterval.nextTime, currentTime)), rtcpByeTimerCallback, s);
        }
        else
        {
            rtcpSessionSendShutdownBye(hRTCP, reasonText, reasonLength);
            if (NULL!=s->rtpShutdownCompletedCallback)
                s->rtpShutdownCompletedCallback(s->rtpSession, reason, s->rtcpShutdownCompletedContext);
        }
    }
    if (reasonLength>0)
    {
        RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown -  called to RTCP Shutdown, reason =%s", reason));
    }
    else
    {
        RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown -  called to RTCP Shutdown"));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionShutdown"));
	RV_UNUSED_ARG(textLength);
    return RV_OK;
}


RVAPI
RvStatus RVCALLCONV RvRtcpSetAppEventHandler(
                                             IN RvRtcpSession            hRTCP,
                                             IN RvRtcpAppEventHandler_CB pAppEventHandler,
                                             IN void*                    rtcpAppContext)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetAppEventHandler"));
    s->rtcpAppRecvCallback = pAppEventHandler;
    s->rtcpAppContext = rtcpAppContext; /*context is Event to inform Ring3 about RTCP arrival*/
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetAppEventHandler"));
    return RV_OK;
}


RVAPI
RvStatus RVCALLCONV RvRtcpSetByeEventHandler(
                                             IN RvRtcpSession            hRTCP,
                                             IN RvRtcpByeEventHandler_CB pByeEventHandler,
                                             IN void*                    rtcpByeContext)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetByeEventHandler"));
    s->rtcpByeRecvCallback = pByeEventHandler;
    s->rtcpByeContext = rtcpByeContext; /*context is Event to inform Ring3 about RTCP arrival*/
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetByeEventHandler"));
    return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtcpSetSDESReceivedEventHandler(
      IN RvRtcpSession            hRTCP,
      IN RvRtcpSDESMessageReceived_CB SDESMessageReceived)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESReceivedEventHandler"));
    s->rtcpSDESMessageArrivedCallback = SDESMessageReceived;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESReceivedEventHandler"));
    return RV_OK;
}
/************************************************************************************
 * RvRtcpSetSDESReceivedEventHandlerEx
 * Description:
 *      Sets SDES event extended handler
 * input:  hRTCP				 - Handle of RTP session.	
 *         SDESMessageReceivedEx - pointer to USER's SDES "extended" event callback
 *		   SDESContext			 - application's context to SDES event handler	
 * output: none:                             
 * return value: the function returns a non-negative value 
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSetSDESReceivedEventHandlerEx(
		IN RvRtcpSession				  hRTCP,
		IN RvRtcpSDESMessageReceivedEx_CB SDESMessageReceived,
		IN void*						  SDESContext)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESReceivedEventHandler"));
    s->rtcpSDESMessageArrivedCallbackEx = SDESMessageReceived;
	s->rtcpSDESContext = SDESContext;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSDESReceivedEventHandler"));
    return RV_OK;
}


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
		IN RvRtcpSRRRMessageReceived_CB SRRRMessageReceived)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSRRREventHandler"));
    s->rtcpSRRRMessageArrivedCallback = SRRRMessageReceived;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSRRREventHandler"));
    return RV_OK;
}


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
		IN RvRtcpSession				hRTCP,
		IN RvRtcpSRRRMessageReceivedEx_CB SRRRMessageReceived,
		IN void*						SRRRContext)
{
    rtcpSession *s = (rtcpSession *)hRTCP;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSRRREventHandler"));
    s->rtcpSRRRMessageArrivedCallbackEx = SRRRMessageReceived;
	s->rtcpSRRRContext = SRRRContext;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSRRREventHandler"));
    return RV_OK;
}


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
		IN RvRtcpRoundTripDelay_CB      roundTripDelayFunc)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler"));
    if (NULL==s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler: NULL pointer"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler"));
        return RV_ERROR_UNKNOWN;
    }
    s->roundTripDelayCallback = roundTripDelayFunc;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler"));
    return RV_OK;
}


/************************************************************************************
 * RvRtcpSetSsrcRemoveEventHandler 
 * Description:
 *      Sets an SSRC remove event handler. The event is rised when the stack is going to 
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
		IN void*						context)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetSsrcRemovedEventHandler"));
    if (NULL==s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetSsrcRemovedEventHandler: NULL pointer"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSsrcRemovedEventHandler"));
        return RV_ERROR_UNKNOWN;
    }

    s->ssrcRemoveCallback = ssrcRemoveFunc;
    s->ssrcRemoveContext = context;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetSsrcRemovedEventHandler"));
    return RV_OK;
}


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
		IN void*					 roundTripDelayContext)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler"));
    if (NULL==s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler: NULL pointer"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler"));
        return RV_ERROR_UNKNOWN;
    }
    s->roundTripDelayCallbackEx = roundTripDelayFunc;
    s->roundTripDelayContext = roundTripDelayContext;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetRoundTripDelayEventHandler"));
    return RV_OK;
}


RVAPI
RvInt32 RVCALLCONV RvRtcpGetSocket(IN RvRtcpSession hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    if (NULL != s)
    {
        RvSocket sock = (RvSocket)RV_INVALID_SOCKET;
        RvTransportGetOption(s->transport,
            RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_SOCKET,
            (void*)&sock);
        return (RvInt32)sock;
    }
    return -1;
}

#if defined(RVRTP_SECURITY)
RVAPI
RvStatus RVCALLCONV RvRtcpSetEncryption(
    IN RvRtcpSession hRTCP,
    IN RvRtpEncryptionMode mode,
    IN RvRtpEncryptionPlugIn* enctyptorPlugInPtr,
    IN RvRtpEncryptionKeyPlugIn* keyPlugInPtr)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetEncryption"));
    if (NULL==s||enctyptorPlugInPtr==NULL||NULL == keyPlugInPtr)
        return -1;
    RvRtcpSessionSetEncryptionMode (hRTCP, mode);
    s->encryptionKeyPlugInPtr  = keyPlugInPtr;
    s->encryptionPlugInPtr = (RvRtpEncryptionPlugIn*)enctyptorPlugInPtr;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetEncryption"));
    return RV_OK;
}
#endif


RVAPI
RvStatus RVCALLCONV RvRtcpSetEncryptionNone(IN RvRtcpSession hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetEncryptionNone"));
    if (NULL == s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetEncryptionNone: hRTCP points to NULL"));
        return -1;
    }
    s->encryptionPlugInPtr = NULL;
    s->encryptionKeyPlugInPtr = NULL;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetEncryptionNone"));
    return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetEncryptionMode(IN RvRtcpSession hRTCP, IN RvRtpEncryptionMode mode)
{
    rtcpSession *s = (rtcpSession *)hRTCP;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSetEncryptionMode"));
    if (NULL == s)
        return RV_ERROR_UNKNOWN;

    s->encryptionMode = mode;
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSetEncryptionMode"));
    return RV_OK;
}

/************************************************************************************
 * RvRtcpSessionSetParam
 * description:  Sets session parameters for the session
 * input:  hRTCP - Handle of RTCP session.
 *         param - type of parameter
 *         data  - pointer to parameter
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * Note:1)   For RTP session with dynamic payload (with param = RVRTCP_PARAMS_RTPCLOCKRATE)
 *           this function should be called
 *           after RTP session opening for accurate RTCP timestamp calculation
 *           For standard payload types this call can be omitted.
 ***********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetParam(
    IN RvRtcpSession hRTCP,
    IN RvRtcpParameters param,
    IN void* data)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSessionSetParam"));
   
    if (NULL==s || data == NULL)
        return RV_ERROR_UNKNOWN;    

    switch (param)
    {
    case RVRTCP_PARAMS_RTPCLOCKRATE:
        RvLockGet(&s->lock, logMgr);
        s->myInfo.rtpClockRate = *((RvUint32*) data);
        RvLockRelease(&s->lock, logMgr);
        break;
    case RVRTCP_PARAMS_RTP_PAYLOADTYPE:
        if (s->myInfo.rtpPayloadType != *((RvUint8*) data))
        {
            RvLockGet(&s->lock, logMgr);
            s->myInfo.rtpPayloadType = *((RvUint8*) data);
            RvLockRelease(&s->lock, logMgr);
        }
        break;
    default:
        return RV_ERROR_UNKNOWN;
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSessionSetParam"));
    return RV_OK;
}

RVAPI
RvStatus  RVCALLCONV RvRtcpSetProfilePlugin(
                                            IN RvRtcpSession hRTCP, 
                                            IN RtpProfilePlugin* profilePlugin)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpSetRtpSessionProfilePlugin"));
    if (NULL == hRTCP)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "rtcpSetRtpSessionProfilePlugin"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpSetRtpSessionProfilePlugin"));
        return RV_ERROR_NULLPTR;
    }
    s->profilePlugin = profilePlugin;
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpSetRtpSessionProfilePlugin"));
    return RV_OK;
}

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
        IN RvBool			bManual)
{
    rtcpSession *s = (rtcpSession *)hRTCP;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtcpSetManual %d", bManual));
    if (NULL == hRTCP)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSetManual"));
        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtcpSetManual"));
        return RV_ERROR_NULLPTR;
    }
    s->isManual	= bManual;
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpSetRtpSessionProfilePlugin"));
	
    return RV_OK;
}

#undef FUNC_NAME
#define FUNC_NAME(name) "rtcpDemux" #name

RVAPI
RvRtcpSession RVCALLCONV rtcpDemuxOpenFrom(
    IN  RvRtpDemux    demux,
    IN  RvUint32      ssrc,
    IN  RvNetAddress* pRtcpAddress,
    IN  char *        cname,
    IN  RvInt32       maxSessionMembers,
    IN  void *        buffer,
    IN  RvInt32       bufferSize)
{
    RvRtcpSession s;
    RvAddress     localAddress;
    RvStatus      status;
    RvTransport   transp = NULL;
    RvBool        bCreateTransport = RV_FALSE;
    RvTransportSocketCfg cfg;

    RTPLOG_ENTER(OpenFrom);

    RvTransportInitSocketTransportCfg(&cfg);

    /* Prepare the local address, that should be used by transport */
	if ((NULL!=pRtcpAddress) && (RvNetGetAddressType(pRtcpAddress)!=RVNET_ADDRESS_NONE))
	{
		if (RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_IPV6)
		{
#if (RV_NET_TYPE & RV_NET_IPV6)
			RvNetIpv6 Ipv6;
			RvNetGetIpv6(&Ipv6, pRtcpAddress);
			/*    RvAddressCopy(&rvRtcpInstance.localAddress , &localAddress);*/
			RvAddressConstructIpv6(&localAddress, Ipv6.ip, Ipv6.port, Ipv6.scopeId);
            cfg.options |= RVTRANSPORT_CREATEOPT_IPV6SOCKET;
#else
			RTPLOG_ERROR_LEAVE(OpenFrom, "IPV6 is not supported in current configuration.");
			return NULL;
#endif
		}
		else
		{
			RvNetIpv4 Ipv4;
			RvNetGetIpv4(&Ipv4, pRtcpAddress);
			RvAddressConstructIpv4(&localAddress, Ipv4.ip, Ipv4.port);
		}
	}

    /* Check if the transport should be created at all */
    if (pRtcpAddress != NULL && demux == NULL)
    {
        bCreateTransport = RV_TRUE;
    }
    /* Take in account the __H323_NAT_FW__ */
#ifdef __H323_NAT_FW__
    if (NULL != demux)
    {
        RtpDemux* d = (RtpDemux*) demux;
		if (pRtcpAddress != NULL)
		{
        RvLockGet(&d->lock, logMgr);
			if ((d->rtcpSessionsCounter <= 0) ) 
			{
                bCreateTransport = RV_TRUE;
            }
            else
            {
                transp = d->rtcpTransport;
            }

            RvLockRelease(&d->lock, logMgr);
		}
		else {
			RTPLOG_ERROR_LEAVE(OpenFrom, "Wrong RTCP Address.");
			return NULL;
		}
    }
#endif

    /* Create Transport object */
    if (bCreateTransport == RV_TRUE)
    {
        cfg.protocol           = RvSocketProtocolUdp;
        cfg.pLocalAddr         = &localAddress;
        cfg.options            = RVTRANSPORT_CREATEOPT_BROADCAST;
        cfg.pLogMgr            = logMgr;

        status = RvSelectGetThreadEngine(/*logMgr*/NULL, &cfg.pSelectEngine);
        if ((status != RV_OK) || (cfg.pSelectEngine == NULL))
            cfg.pSelectEngine = rvRtcpInstance.selectEngine;

        status = RvTransportCreateSocketTransport(&cfg, &transp);
        if (status == RV_OK)
        {
            RvInt32 bufSizes[2] = {8192/*send buf*/, 8192/*recv buf*/};
            RvTransportAddRef(transp);
            /* Set size of Transport socket buffers */
            RvTransportSetOption(transp,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_BUFSIZE, &bufSizes);
        }
        else {
            /* RK */
            return NULL;
        }
    }

    RvAddressDestruct(&localAddress);

#ifdef __H323_NAT_FW__
    if (NULL != demux)
    {
        RtpDemux* d = (RtpDemux*) demux;
        RvLockGet(&d->lock, logMgr);
        if (d->rtcpSessionsCounter <= 0)
        {
            d->rtcpTransport = transp;
        }
        RvLockRelease(&d->lock, logMgr);
    }
#endif


    s = rtcpDemuxOpenEx(demux, ssrc, transp, cname, maxSessionMembers,
                        buffer, bufferSize);
    if (s == NULL  &&  transp != NULL  &&  NULL == demux)
    {
        RvTransportRelease(transp);
    }

    RTPLOG_LEAVE(OpenFrom);
    return (RvRtcpSession)s;
}

RVAPI
RvRtcpSession RVCALLCONV rtcpDemuxOpen(
    IN  RvRtpDemux    demux,
    IN  RvUint32      ssrc,
    IN  RvNetAddress* pRtcpAddress,
    IN  char *        cname)
{
    rtcpSession* s;
    RvInt32 allocSize = RvRtcpGetAllocationSize(RTCP_MAXRTPSESSIONMEMBERS);
    RTPLOG_ENTER(Open);
    if (NULL == pRtcpAddress ||
        RvMemoryAlloc(NULL, (RvSize_t)allocSize, logMgr, (void**)&s) != RV_OK)
        return NULL;

    if((rtcpSession*)rtcpDemuxOpenFrom(demux, ssrc, pRtcpAddress, cname, RTCP_MAXRTPSESSIONMEMBERS, (void*)s, allocSize)==NULL)
    {
        RvMemoryFree(s, logMgr);
        return NULL;
    }

    s->isAllocated = RV_TRUE;
    RTPLOG_LEAVE(Open);
    return (RvRtcpSession)s;
}

RVAPI
RvRtcpSession RVCALLCONV rtcpDemuxOpenEx(
    IN  RvRtpDemux    demux,
    IN  RvUint32      ssrc,
    IN  RvTransport   transp,
    IN  char *        cname,
    IN  RvInt32       maxSessionMembers, /* if 0, RTCP_MAXRTPSESSIONMEMBERS is used */
    IN  void *        buffer, /* if NULL, the memory will be allocated internally */
    IN  RvInt32       bufferSize /* ignored if buffer is NULL */)
{
    rtcpSession*    s;
    RvSize_t        allocSize;
    RvStatus        status;
    RvInt32         participantsArrayOfs;

    RV_UNUSED_ARG(demux);
    RTPLOG_ENTER(OpenEx);

    /* Initialize some parameters */
    if (maxSessionMembers == 0)
    {
        maxSessionMembers = RTCP_MAXRTPSESSIONMEMBERS;
    }
    allocSize = RvRtcpGetAllocationSize(maxSessionMembers);

    /* Run some checks */
    if (cname == NULL  ||  strlen(cname) > RTCP_MAXSDES_LENGTH)
    {
        RTPLOG_ERROR_LEAVE(OpenEx, "bad cname");
        return NULL;
    }
    if (buffer != NULL  &&  (RvSize_t)bufferSize < allocSize)
    {
        RTPLOG_ERROR_LEAVE(OpenEx," no room to open session, increase bufferSize");   
        return NULL;
    }

    /* Allocate session if need */
    if (buffer != NULL)
    {
        s = (rtcpSession *)buffer;
        memset(s, 0, (RvSize_t)bufferSize);
    }
    else
    {
        status = RvMemoryAlloc(NULL, allocSize, logMgr, (void**)&s);
        if (status != RV_OK)
        {
            RTPLOG_ERROR_LEAVE(OpenEx, "cannot allocate RTCP session");
            return NULL;
        }
        memset(s, 0, allocSize);
        s->isAllocated = RV_TRUE;
    }

	RTPLOG_DEBUG((RTP_SOURCE, "rtcpDemuxOpenEx - RTCP session opening"));

    s->sessionMembers = 1; /* my SSRC added to participant array */
    s->maxSessionMembers = maxSessionMembers;
	
	/* the RTCP session by default is Automatic: send RTCP automatically */
	/*s->isManual = RV_FALSE;*/   /* commented because all session's members already set to 0 */

    participantsArrayOfs = RvRoundToSize(sizeof(rtcpSession), ALIGNMENT);
    s->participantsArray = (rtcpInfo *) ((char *)s + participantsArrayOfs);
    s->participantsArray[0].ssrc = s->myInfo.ssrc = ssrc;
    s->txInterval.rtcpBandwidth = RV_RTCP_BANDWIDTH_DEFAULT;  /* an arbitrary default value */
	s->txInterval.rtcpSenderBandwidth = 0;        /*initially not set */
	s->txInterval.rtcpReceiverBandwidth = 0;	  /*initially not set */	
    s->transport = transp;

#ifdef __H323_NAT_FW__
    if (NULL != demux)
    {
        RtpDemux* d = (RtpDemux*) demux;
        RvLockGet(&d->lock, logMgr);

        if (d->rtcpSessionsCounter <= 0)
        {
            status = RvSelectGetThreadEngine(/*logMgr*/NULL, &d->rtcpSelectEngine);
            if ((status != RV_OK) || (d->rtcpSelectEngine == NULL))
                d->rtcpSelectEngine = rvRtcpInstance.selectEngine;
        }
        s->selectEngine = d->rtcpSelectEngine;
        s->demux        = demux;
        RvLockRelease(&d->lock, logMgr);
    }
    else
#endif
    {
        status = RvSelectGetThreadEngine(/*logMgr*/NULL, &s->selectEngine);
        if ((status != RV_OK) || (s->selectEngine == NULL))
            s->selectEngine = rvRtcpInstance.selectEngine;
    }
    
    /* Initialize lock in supplied buffer */
    if (RvLockConstruct(logMgr, &s->lock) != RV_OK)
    {
        RTPLOG_ERROR_LEAVE(OpenEx, "RvLockConstruct() failed");
        if (s->isAllocated == RV_TRUE)
            RvMemoryFree(s, logMgr);
        return NULL;
    }

    /* Register network event callback with the transport.
       In case of demux the callback may be registered already,
       so don't override it again. */
    {
        RvTransportCallbacks callbacks;
        RvTransportGetCallbacks(transp, &callbacks);
        if (callbacks.usrData == NULL)
        {
#ifdef __H323_NAT_FW__
            if (NULL != demux)
            {
                callbacks.pfnEvent = rtcpDemuxEventCallback;
                callbacks.usrData  = (void*)demux;
            }
            else
#endif
            {
                callbacks.pfnEvent = rtcpEventCallback;
                callbacks.usrData  = (void*)s;
            }
            RvTransportSetCallbacks(transp, &callbacks);
        }
    }

    /* Make sure we wait for RTCP packets */
    status = RvTransportRegisterEvent(transp, RVTRANSPORT_EVENT_READ);
    if (status != RV_OK)
    {
        RTPLOG_ERROR_LEAVE(OpenEx, "failed to register to READ event");
        if (s->isAllocated == RV_TRUE)
            RvMemoryFree(s, logMgr);
        return NULL;
    }

    memset(&(s->myInfo.SDESbank), 0, sizeof(s->myInfo.SDESbank)); /* initialization */
    setSDES(RV_RTCP_SDES_CNAME, &(s->myInfo.SDESbank[RV_RTCP_SDES_CNAME]), (RvUint8*)cname, (RvInt32)strlen(cname));
    
	RvLockGet(&s->lock, logMgr);
	RvRtpAddressListConstruct(&s->addressList);
	RvLockRelease(&s->lock, logMgr);

    RTPLOG_LEAVE(OpenEx);
    return (RvRtcpSession)s;
}

/**************************************************************************************
 * rtcpDemuxAddRemoteAddress
 * description: adds the new RTCP address of the remote peer or the address a multi-cast group
 *              or of multi-unicast address list to which the RTCP stream will be sent.
 * input:
 *        hRTCP          - handle to RTCP demultiplexing object
 *        pRtcpAddress   - pRtpAddress of remote RTCP session
 *        multiplexerId  - multiplexID of remote RTCP session.
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 * Note:
 *        the same addresses with the same multiplexerID will be discarded
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV rtcpDemuxAddRemoteAddress(
	IN RvRtcpSession  hRTCP,   /* RTCP Session Opaque Handle */
	IN RvNetAddress*  pRtcpAddress,
    IN RvUint32*      pMultiplexerId)
{
    rtcpSession *s            = (rtcpSession *)hRTCP;
    RvAddress*   pRvAddress   = NULL;
    RvAddress    destAddress;
    RvNetIpv6    Ipv6         = {{0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0}, RvUint16Const(0), RvUint32Const(0)}; /* initialization for warning elimination */

#ifndef TI_368
    RTPLOG_ENTER(AddRemoteAddress);
     if (NULL==pRtcpAddress||RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_NONE||
         (RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_IPV6 &&
         RvNetGetIpv6(&Ipv6, pRtcpAddress)!=RV_OK))
     {
         RTPLOG_ERROR_LEAVE(AddRemoteAddress, "bad address parameter or IPV6 is not supported in current configuration");
         return RV_ERROR_NOTSUPPORTED;
     }
	RvLockGet(&s->lock, logMgr);//update by kongfd ????
     if (RvNetGetAddressType(pRtcpAddress) == RVNET_ADDRESS_IPV6)
     {
         pRvAddress = RvAddressConstructIpv6(&destAddress, Ipv6.ip, Ipv6.port, Ipv6.scopeId);
     }
     else
     {
         RvNetIpv4 Ipv4;
        RvNetGetIpv4(&Ipv4, pRtcpAddress);
         pRvAddress = RvAddressConstructIpv4(&destAddress, Ipv4.ip, Ipv4.port);
     }
	 if (pRvAddress != NULL)
#else
    //if (pRvAddress != NULL)
	if (pRtcpAddress != NULL)
#endif	
    {
        if (
            s->profilePlugin->funcs->addRemAddress != NULL)
        {
            s->profilePlugin->funcs->addRemAddress(s->profilePlugin, s->rtpSession, RV_FALSE, pRtcpAddress);
        }

		#ifdef TI_368
		pRvAddress = (RvAddress*) pRtcpAddress->address;
		#endif
        RvRtpAddressListAddAddress(&s->addressList, pRvAddress, pMultiplexerId);
        s->remoteAddressSet = RV_TRUE;

        /* We have a remote client - set a timer for automatic reports*/
		if (s->isManual == RV_FALSE)
		  rtcpScheduleAutoReports(s);
    }
	RvLockRelease(&s->lock, logMgr);//update by kongfd ????
    RTPLOG_LEAVE(AddRemoteAddress);
    return RV_OK;
}

/************************************************************************************
 * rtcpDemuxRemoveRemoteAddress
 * description: removes the specified RTCP address of the remote peer or of the multicast group
 *              or of the multiunicast list with elimination of address duplication.
 *              Removes Multiplexing address, if pMultiplexID is specified.
 * input: hRCTP        - The handle of the RTCP session.
 *        pRtcpAddress - pointer to RvNetAddress to remove.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV rtcpDemuxRemoveRemoteAddress(
    IN  RvRtcpSession  hRTCP,
    IN  RvNetAddress*  pRtcpAddress,
    IN  RvUint32*      pMultiplexID)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvAddress* pRvAddress = NULL;
	RvStatus res;
	RvInt r = 0;
	int i=0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "rtcpDemuxRemoveRemoteAddress"));

#ifndef TI_368
    if (NULL==s||NULL==pRtcpAddress||RvNetGetAddressType(pRtcpAddress)==RVNET_ADDRESS_NONE)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "rtcpDemuxRemoveRemoteAddress: NULL session pointer or wrong parameter"));
        res=RV_ERROR_NULLPTR;
	}
#else
	if (NULL==s||NULL==pRtcpAddress)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "rtcpDemuxRemoveRemoteAddress: NULL session pointer or wrong parameter"));
		res=RV_ERROR_NULLPTR;
	}
#endif
    else
	{
#ifdef TI_368
		RvAddress addr;
		memcpy(&addr, pRtcpAddress->address, sizeof(RvAddress));
		pRvAddress = &addr;
#else
		pRvAddress = (RvAddress*) pRtcpAddress->address;
#endif

		RvLockGet(&s->lock, logMgr);
		if (s!=NULL && 	s->profilePlugin->funcs->removeRemAddress != NULL)
			   s->profilePlugin->funcs->removeRemAddress(s->profilePlugin, s->rtpSession, pRtcpAddress);

			RvRtpAddressListRemoveAddress(&s->addressList, pRvAddress, pMultiplexID);    
			pRvAddress = RvRtpAddressListGetNext(&s->addressList, NULL);

			if (NULL == pRvAddress)
				s->remoteAddressSet = RV_FALSE;

			res=RV_OK;

		RvLockRelease(&s->lock, logMgr);
	}

	RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpDemuxRemoveRemoteAddress"));
    return res;
}

#ifdef __H323_NAT_FW__
static
void rtcpDemuxEventCallback(
    RvTransport         transport,
    RvTransportEvents   ev, 
    RvBool              error, 
    void*               usrData)
{
    RtpDemux*    demuxPtr = (RtpDemux*)usrData;
    rtcpSession* s        = NULL;

    RvUint32 buffer[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer buf;
    RvStatus res;
    RvAddress remoteAddress;
    RvSize_t bytesReceived;


    RTPLOG_ENTER(EventCallback);
    RV_UNUSED_ARG(transport);
    RV_UNUSED_ARG(ev);
    RV_UNUSED_ARG(error);

    buf = buffCreate(buffer, MAXRTCPPACKET);
    res = rtcpSessionRawReceive((RvRtpDemux)demuxPtr, &s, &buf, (RvSize_t)buf.length, &bytesReceived, &remoteAddress);   
    if ( s==NULL || (s!=NULL && s->isShutdown) )
    {
        /* session must not receive RTCP packet after sending BYE packet */
        RTPLOG_LEAVE(rtcpDemuxTransportEvHandler);
        return;
    }
    if (res == RV_OK)
    {
        RvUint16 localPort = 0;
        buf.length = (RvUint32)bytesReceived;

        RvTransportGetOption(s->transport,
            RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_LOCALPORT,
            (void*)&localPort);

        if ((RvAddressGetIpPort(&remoteAddress) != localPort) ||
            !isMyIP(&remoteAddress))
        {
            RvInt32 res=rtcpProcessCompoundRTCPPacket((RvRtcpSession)s, &buf, getNNTPTime(), &remoteAddress);
            if (res==0) 
            {
				RvUint32 ssrc=getSSRCfrom(buf.buffer);
				//beg add lishixin
				RvAddressIpv4 * ip4 = RvAddressGetIpv4(&remoteAddress);
				RvChar * ip = NULL;
				RvUint16 port = 0;
				RtpDemuxTableEntry* pEntry = NULL;
				RvUint32 multiplexID=0;
				if(ip4 != NULL)
				{
					RvChar buff_temp[64];
					ip=RvAddressIpv4ToString(buff_temp,sizeof(buff_temp),ip4->ip);
					port = ip4->port;
				}
				pEntry = (RtpDemuxTableEntry*)RvRaGet(demuxPtr->demuxTbl, s->demuxTblEntry);
				if (pEntry != NULL)
				{
					multiplexID = pEntry->multiplexID;
				}
				//end lishixin
				if (s->rtcpRecvCallbackEx != NULL)
				{
					s->rtcpRecvCallbackEx((RvRtcpSession)s, s->haRtcp, ssrc, buf.buffer, buf.length,ip,port,RV_TRUE,multiplexID);
				}
				else if (s->rtcpRecvCallback != NULL)
				{
					s->rtcpRecvCallback((RvRtcpSession)s, s->haRtcp, ssrc);
				}
				
            }
        }
    }
    RTPLOG_LEAVE(EventCallback);
}
#endif

#undef FUNC_NAME
#define FUNC_NAME(name) "rtcp" #name
RVAPI
RvUint32  RVCALLCONV rtcpSessionGetIndex(
      IN RvRtcpSession hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;

    RTPLOG_ENTER(SessionGetIndex);
    if (NULL==s)
    {
        RTPLOG_ERROR_LEAVE(SessionGetIndex, "NULL session pointer");
        return 0;
    }
    RTPLOG_LEAVE(SessionGetIndex);
    return s->index;
}

/*****************************************************************************************************
 * rtcpPacketSend
 * description: sends regular or multiplexed RTCP packet
 * input:
 *         socketPtr      - pointer to the socket
 *         bufPtr         - pointer to the buffer
 *         DataLen        - size of buffer to send
 *         natAddressPtr  - pointer to RtpNatAddress structure
 * output
 *         bytesSent      -  pointer to sent bytes
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 *****************************************************************************************************/
RVINTAPI
RvStatus RVCALLCONV rtcpPacketSend(
    IN    rtcpSession*   s,
    IN    RvUint8*       bufPtr,
    IN    RvInt32        DataLen,
    IN    RtpNatAddress* natAddressPtr,
    OUT   RvSize_t*      bytesSent)
{
    RvStatus status = RV_ERROR_UNKNOWN;
    RTPLOG_ENTER(PacketSend);

#ifdef __H323_NAT_FW__
    if (natAddressPtr->isMultiplexed)
    {
		/* we send to multiplexed address: add extra bytes */
        RvUint32  multiplexID = natAddressPtr->multiplexID;
		
        DataLen += RvRtpNatMultiplexIdSize();

		/* leave space for MultiplexID */
		bufPtr -= RvRtpNatMultiplexIdSize();              

		/* write multiplexID into added place */
		*(RvUint32 *)bufPtr = multiplexID; 

        ConvertToNetwork(bufPtr, 0, 1);
    }
#endif
    status = RvTransportSendBuffer(
                s->transport, bufPtr, (RvSize_t)DataLen, &natAddressPtr->address,
                0 /*options*/, bytesSent);
    
    RTPLOG_ENTER(PacketSend);
    return status;
}

/********************************************************************************************
 * RvRtcpSessionGetSocket
 * Description: Gets a socket used fot rtcp session
 * This function is thread-safe.
 * INPUT   : hRTCP           - RTCP session handle
 * RETURN  : socket
 *********************************************************************************************/
RVAPI
RtpSocket RVCALLCONV RvRtcpSessionGetSocket(IN RvRtcpSession hRTCP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvSocket sock = (RvSocket)RV_INVALID_SOCKET;
	if (s == NULL)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSessionGetSocket: NULL session handle"));
		return (RtpSocket)0;
    }
    RvTransportGetOption(s->transport, RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                         RVTRANSPORT_OPT_SOCKET, (void*)&sock);
	return (RtpSocket)sock;
}

/********************************************************************************************
* RvRtcpSessionSetSocket
* Description: Set the socket to be used for this session and
* INPUT   : hRTCP          - RTCP session handle
*         : socket         - pointer to the socket to be used in this session 
*         : address        - pointer to address to which the socket is bound to. 			
* RETURN  : RV_OK on success, other on failure
*********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpSessionSetSocket(
			IN RvRtcpSession hRTCP, 
			IN RtpSocket	 *socket)
{
    rtcpSession*  s = (rtcpSession *)hRTCP;

    if (s->transport != NULL)
    {
        RvStatus status;

        status = RvTransportSetOption(s->transport,
                        RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                        RVTRANSPORT_OPT_SOCKET, (void*)socket);
        if (status != RV_OK)
        {
            RvLogError(rvLogPtr, (rvLogPtr, "RvRtcpSessionSetSocket: failed to set socket"));
            return status;
        }
    }
	return RV_OK;
}


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
			IN RtpSocket	 *socket,  
			IN RvAddress	 *address)
{
    rtcpSession	*s = (rtcpSession *)hRTCP;
	RvStatus	res;
		
	res = initiateSocket((RvSocket *)socket, address);
	if (res==RV_OK)
	{
		RvRtcpSessionSetSocket(hRTCP, socket);
		/* Make sure we wait for RTCP packets */
        RvTransportRegisterEvent(s->transport, RVTRANSPORT_EVENT_READ);
    }
	else
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSessionSetSocketEx: cannot initiate socket"));
	}
	
	return res;
}


#ifdef __RTCP_XR__
/******************************************************************************************************
 * rtcpXrPluginSetRtcpCallbacks()
 * scope: private
 * Description:
 *    Sets all static functions from RTCP module to be used in RTCP XR plugin
 * Parameters:
 *   Input: pointer to the RtcpXrPlugin plugin
 *   Output: none
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 *****************************************************************************************************/
RVAPI
RvStatus RVCALLCONV rtcpXrPluginSetRtcpCallbacks(RtcpXrPlugin* plugin)
{
    RTPLOG_ENTER(XrPluginSetRtcpCallbacks);
    if (plugin == NULL)
    {
        RTPLOG_ERROR_LEAVE(XrPluginSetRtcpCallbacks, "NULL  pointer");
        return RV_ERROR_NULLPTR;
    }
    plugin->callbacks->makeHeader        = makeHeader;
    plugin->callbacks->findSSrc          = findSSrc;
    plugin->callbacks->buffAddToBuffer   = buffAddToBuffer;
    plugin->callbacks->buffCreate        = buffCreate;
    plugin->callbacks->getNNTPTime       = getNNTPTime;
    plugin->callbacks->addRTCPPacketType = rtcpAddRTCPPacketType;
    plugin->callbacks->rawSend           = rtcpSessionRawSend;
    RTPLOG_LEAVE(XrPluginSetRtcpCallbacks);
    return RV_OK;
}
#endif

#ifdef __RTCP_FB__
/******************************************************************************************************
 * rtcpFbPluginSetRtcpMetods()
 * scope: private
 * Description:
 *    Sets all static functions from RTCP module to be used in RTCP FB plugin
 * Parameters:
 *   Input: pointer to the RtcpFbPlugin plugin
 *   Output: none
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 *****************************************************************************************************/
RVAPI
RvStatus RVCALLCONV rtcpFbPluginSetRtcpMethods(RtcpFbPlugin* fbPlugin)
{
    RTPLOG_ENTER(FbPluginSetRtcpCallbacks);
    if (fbPlugin == NULL)
    {
        RTPLOG_ERROR_LEAVE(FbPluginSetRtcpCallbacks, "NULL  pointer");
        return RV_ERROR_NULLPTR;
    }

	fbPlugin->rtcpMethods->makeHeader = makeHeader;
	fbPlugin->rtcpMethods->buffCreate = buffCreate;
	fbPlugin->rtcpMethods->dataAddToBuffer = dataAddToBuffer;
	fbPlugin->rtcpMethods->sessionRawSend = rtcpSessionRawSend;
	fbPlugin->rtcpMethods->timerCallback = rtcpTimerCallback;
	fbPlugin->rtcpMethods->bitfieldSet = bitfieldSet;
	fbPlugin->rtcpMethods->bitfieldGet = bitfieldGet;
    RTPLOG_LEAVE(FbPluginSetRtcpCallbacks);
    return RV_OK;
}
#endif

/********* STATIC FUNCTIONS *************************************************************************/

#define RV_READ2BYTES(a,b)   ((b)[0]=(a)[0],(b)[1]=(a)[1])
#define RV_WRITE2BYTES(a,b)  ((a)[0]=(b)[0],(a)[1]=(b)[1])
#define RV_READ4BYTES(a,b)   ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define RV_WRITE4BYTES(a,b)  ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])

#define  rvDataBufferReadAtUint16(buf, i, s)            (RV_READ2BYTES(buf + (i),(RvUint8*)(s)),*((RvUint16*)s) = (RvInt16 )RvConvertNetworkToHost16(*((RvUint16*)s)))
#define  rvDataBufferWriteAtUint16(buf, i, s)           {RvUint16 t = (RvInt16 )RvConvertNetworkToHost16((RvUint16)(s)); RV_WRITE2BYTES(buf + (i), (RvUint8*)&t);}
#define  rvDataBufferReadAtInt16(buf, i, s)             (RV_READ2BYTES(buf + (i),(RvUint8*)(s)),*((RvUint16*)s) = (RvInt16 )RvConvertNetworkToHost16(*((RvUint16*)s)))
#define  rvDataBufferWriteAtInt16(buf, i, s)            {RvUint16 t = (RvInt16 )RvConvertNetworkToHost16((RvUint16)(s)); RV_WRITE2BYTES(buf + (i), (RvUint8*)&t);}
#define  rvDataBufferReadAtUint32(buf, i, l)            (RV_READ4BYTES(buf + (i),(RvUint8*)(l)),*((RvUint32*)l) = RvConvertNetworkToHost32(*((RvUint32*)l)))
#define  rvDataBufferWriteAtUint32(buf, i, l)           {RvUint32 t = RvConvertNetworkToHost32((RvUint32)(l)); RV_WRITE4BYTES(buf + (i), (RvUint8*)&t);}
#define  rvDataBufferReadAtInt32(buf, i, l)             (RV_READ4BYTES(buf + (i),(RvUint8*)(l)),*((RvUint32*)l) = RvConvertNetworkToHost32(*((RvUint32*)l)))
#define  rvDataBufferWriteAtInt32(buf, i, l)            {RvUint32 t = RvConvertNetworkToHost32((RvUint32)(l)); RV_WRITE4BYTES(buf + (i), (RvUint8*)&t);}

#define RV_FREAD2BYTES(a,b)   ((b)[0] = (*(a)++),(b)[1] = (*(a)++))
#define RV_FWRITE2BYTES(a,b)  ((*--(a))=(b)[1],(*--(a))=(b)[0])
#define RV_FREAD4BYTES(a,b)   ((b)[0]=(*(a)++),(b)[1]=(*(a)++),(b)[2]=(*(a)++),(b)[3]=(*(a)++))
#define RV_FWRITE4BYTES(a,b)  ((*--(a))=(b)[3],(*--(a))=(b)[2],(*--(a))=(b)[1],(*--(a))=(b)[0])

#define  rvDataBufferWriteUint32(buf, l)                {RvUint32 t = RvConvertNetworkToHost32((RvUint32)(l)); RV_FWRITE4BYTES(buf, (RvUint8*)&t);}


RvBool rtcpSessionIsSessionInShutdown(
        IN RvRtcpSession hRTCP)

{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvBool       shutDown = RV_FALSE;

    RTPLOG_ENTER(SessionIsSessionInShutdown);
    if (NULL==s)
    {
        RTPLOG_ERROR_LEAVE(SessionIsSessionInShutdown, "NULL pointer");
        return RV_FALSE;
    }
    RvLockGet(&s->lock, logMgr);
    shutDown = s->isShutdown;
    RvLockRelease(&s->lock, logMgr);
    RTPLOG_LEAVE(SessionIsSessionInShutdown);
    return shutDown;
}

RvStatus rtcpSetRtpSession(IN RvRtcpSession hRTCP, IN RvRtpSession hRTP)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
	RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpSetRtpSession"));

    if (NULL == hRTCP || NULL == hRTP)
    {
        RTPLOG_ERROR_LEAVE(SetRtpSession, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    s->rtpSession = hRTP;

	RvLogDebug(rvLogPtr, (rvLogPtr, "rtcpSetRtpSession: RTCP=0x%x, RTP=x0%x", hRTCP, hRTP));
	RvLogLeave(rvLogPtr, (rvLogPtr, "rtcpSetRtpSession"));
    return RV_OK;
}

/*****************************************************************************************************
 * rtcpSessionRawSend
 * description: sends RTCP raw packet, 
 * input:
 *         hRTCP          - RTCP session handle
 *         bufPtr         - pointer to the buffer to send
 *         DataLen        - size of buffer to send
 *         DataCapacity   - data capacity
 * output
 *         sentLenPtr      -  pointer to sent bytes
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 *****************************************************************************************************/
RvStatus rtcpSessionRawSend(
                           IN    RvRtcpSession hRTCP,
                           IN    RvUint8*      bufPtr,
                           IN    RvInt32       DataLen,
                           IN    RvInt32       DataCapacity,
                           INOUT RvUint32*     sentLenPtr)
{
    rtcpSession *s = (rtcpSession *)hRTCP;
    RvStatus status = RV_ERROR_UNKNOWN;
    RTPLOG_ENTER(SessionRawSend);
    if (s!=NULL && 
        s->profilePlugin!=NULL &&          
        s->profilePlugin->funcs !=NULL &&
        s->profilePlugin->funcs->xrtcpRawSend!=NULL)
        status = s->profilePlugin->funcs->xrtcpRawSend(s->profilePlugin, hRTCP, bufPtr, DataLen, DataCapacity, sentLenPtr);
    RTPLOG_ENTER(SessionRawSend);
    return status;
}

RvUint32 rtcpGetSessionMembersInfo(IN rtcpSession* s, OUT RvUint32 *sessionMembersPtr, OUT RvUint32 *senderMembersPtr)
{
    RvInt32 i, senderCount = 0, memberCount = 0;
    rtcpInfo* info;

    RTPLOG_ENTER(GetSessionMembersInfo);
    if (!s->txInterval.initial)
    {
        for (i = 0; i < s->sessionMembers; ++i)
        {
            info = &s->participantsArray[i];
            if (!info->invalid)
            {
                memberCount++;
                if (info->active)
                    senderCount++;
            }
        }
        if(s->myInfo.active>0)
            senderCount++;
		if (sessionMembersPtr !=NULL) 
			*sessionMembersPtr = memberCount;
		if (senderMembersPtr != NULL)
			*senderMembersPtr  = senderCount;
    }
    else
    {
		if (sessionMembersPtr !=NULL) 
			*sessionMembersPtr  = s->txInterval.members;
		if (senderMembersPtr != NULL)
			*senderMembersPtr   = s->txInterval.senders + (s->myInfo.active>0); /* second time we_sent>0 */
    }
    RTPLOG_LEAVE(GetSessionMembersInfo);
    return senderCount;
}

#define RTCP_SENDER_BANDWIDTH_FRACTION		1 /* default fraction of sender's bandwidth:   1 from 4 */
#define RTCP_RECEIVER_BANDWIDTH_FRACTION	3 /* default fraction of reciever's bandwidth: 3 from 4 */
#define RTCP_BANDWIDTH_SCALER               4


/* Get time interval in milliseconds in which the next RTCP will be sent, based on RFC 3556 */
RvUint32 rtcpGetTimeInterval(IN rtcpSession* s, IN const RvUint32 rtcpMinTime)
{
    RvInt32 bandwidth = s->txInterval.rtcpBandwidth;
	RvInt32 senderBandwidth=0;
	RvInt32 receiverBandwidth=0;
    RvUint32 time;
    RvUint32 jitteredTime;
    RvUint32 numMembers;
    RvUint32 sessionMembers;
    RvUint32 senders;
    RvUint32 sendersTreshold;
	

    RTPLOG_ENTER(GetTimeInterval);
    rtcpGetSessionMembersInfo(s, &sessionMembers, &senders);
	
    numMembers = sessionMembers;

	/* get sender bandfwidth */
	if (s->txInterval.rtcpSenderBandwidth)
		senderBandwidth = s->txInterval.rtcpSenderBandwidth;
	else if (s->txInterval.rtcpReceiverBandwidth != 0)
	{
		senderBandwidth = s->txInterval.rtcpBandwidth - s->txInterval.rtcpReceiverBandwidth;
		if (senderBandwidth < 0)
			senderBandwidth = 0;
	}
	else /* both Sender and Receiver bandwidths are not set */
		senderBandwidth = s->txInterval.rtcpBandwidth * RTCP_SENDER_BANDWIDTH_FRACTION / RTCP_BANDWIDTH_SCALER;

	/* get receiver bandfwidth */
	if (s->txInterval.rtcpSenderBandwidth)
		receiverBandwidth = s->txInterval.rtcpSenderBandwidth;
	else if (s->txInterval.rtcpReceiverBandwidth != 0)
	{
		receiverBandwidth = s->txInterval.rtcpBandwidth - s->txInterval.rtcpReceiverBandwidth;
		if (receiverBandwidth < 0)
			receiverBandwidth = 0;
	}
	else /* both Sender and Receiver bandwidths are not set */
		receiverBandwidth = s->txInterval.rtcpBandwidth  - senderBandwidth;
				
	
	/* Check if the proportion of senders to total paticipants is less than or equal 
	   to senderBandwidth/(senderBandwidth+receiverBandwidth).
	   In case when 
	   s->txInterval.rtcpReceiverBandwidth=0 and s->txInterval.rtcpSenderBandwidth=0
	   this is the same as to check if this proportion is less than or equal to 1/4
	*/
	if ((s->txInterval.rtcpReceiverBandwidth==0) && (s->txInterval.rtcpSenderBandwidth==0))
	{
		/* numMembers * RTCP_SENDER_BANDWIDTH_FRACTION / RTCP_BANDWIDTH_SCALER */
		sendersTreshold = numMembers >> 2; 
	}
	else
		sendersTreshold = (numMembers * senderBandwidth / receiverBandwidth);

	if (senders <= sendersTreshold) 
    {
        /* Set our available bandwidth based on whether we are a sender */
        /* or just a receiver. 6                                        */
        if(s->myInfo.active>0) /* we sent */
        {
			numMembers = senders;
			bandwidth = senderBandwidth;
        }
        else
        {
            numMembers = sessionMembers - senders;
			bandwidth = receiverBandwidth;
        }
    }

    /* Set the time based on bandwidth  [bandwidth = data/time] in milliseconds */
    if(bandwidth != 0)
    {
        RvRandom randomValue;

        time = (s->txInterval.aveRTCPPacketSize * numMembers / bandwidth) * 1000; /* mul 1000 to convert to milliseconds */

        /* Check to see minimum time requirements are made */
        if(time < rtcpMinTime)
            time = rtcpMinTime;

		s->txInterval.baseTimeInterval = time;
        RvRandomGeneratorGetValue(&rvRtcpInstance.randomGenerator, &randomValue);

		/* Actual next report interval is a random number between 0.5*time  and 1.5*time */
        jitteredTime = time * (randomValue % 1000 + 500) / 1000;
        /* reconsideration compensation - divide by (e - 1.5), which is 1.21828 (as per RFC 3550) */
        jitteredTime = RvUint64ToRvUint32(
                          RvUint64Div(
                            RvUint64Mul(RvUint64Const(0, jitteredTime),
                                        RvUint64Const(0, 100000)),
                            RvUint64Const(0, 121828)));
    }
    else
    {
        jitteredTime = 0; /* no periodic transmitions */
		s->txInterval.baseTimeInterval = 0;
    }
    RTPLOG_LEAVE(GetTimeInterval);
    return  jitteredTime; /* in milliseconds */
}


RVINTAPI
RvStatus rtcpSetTimer(
        IN rtcpSession* s,
        IN RvInt64      delay,
		IN RvTimerFunc	rtcpTimerCallback
		)
{
    RvTimerQueue *timerQ = NULL;
    RvStatus status = RV_OK;

	if (s==NULL || !s->selectEngine)
	{
		return RV_ERROR_NULLPTR;
	}

    RTPLOG_ENTER(SetTimer);

    /* Find the timer queue we are working with */
    status = RvSelectGetTimeoutInfo(s->selectEngine, NULL, &timerQ);
    if (status != RV_OK)
        timerQ = NULL;

    if (timerQ == NULL)
        timerQ = rvRtcpInstance.timersQueue;

    status = RvTimerStart(&s->timer, timerQ, RV_TIMER_TYPE_ONESHOT, delay, rtcpTimerCallback, s);
    RTPLOG_LEAVE(SetTimer);
    return status;
}


static RvBool rtcpTimerCallback(IN void* key)
{
    rtcpSession* s = (rtcpSession*)key;
    RvInt64 currentTime;
    RvUint32 delay = RTCP_MIN_TIME;
	RvBool   sendRtcp = RV_TRUE;   /* important: initiate as TRUE */

    RTPLOG_ENTER(TimerCallback);
    
	if (s != NULL && s->remoteAddressSet)
    {
		RvUint32	data[MAXRTCPPACKET/sizeof(RvUint32)+1];
		RvRtpBuffer buf;
		RvUint32    rtcpPackSize = 0;
		
        RvLockGet(&s->lock, logMgr);
        if (s->isShutdown)
        {
            RvLockRelease(&s->lock, logMgr);    
            /* no more reports */
            RTPLOG_LEAVE(TimerCallback);
            return RV_FALSE;
        }
        RvLockRelease(&s->lock, logMgr);  
    
		currentTime = RvTimestampGet(logMgr);

		/* Note: time interval calculation should be done before creating the RTCP packet,
				 otherwise the "active" attribute of the senders are set to FALSE */
		delay = rtcpGetTimeInterval(s, RTCP_MIN_TIME); /* in ms */
		RvLogDebug(rvLogPtr, (rvLogPtr, "rtcpTimerCallback: currentTime=%d, Next Report in %d sec", 
				(RvUint32)RvUint64Div(currentTime,RV_TIME64_NSECPERSEC), delay/1000));
		
		buf = buffCreate(data+2, MAXRTCPPACKET-DATA_PREFIX_LEN); /* 4 byes in header remained for encryption purposes 
																	4 bytes for NAT/FW multiplexID */
		rtcpCreateRTCPPacket((RvRtcpSession)s, &buf);
		
#ifdef __RTCP_FB__
		if (s->rtcpFbPlugin != NULL)
		{
			/* this function will: 
			   - add a Feedback waiting to be sent
			   - change a delay in case this is an early scheduled FB packet 
			   - define if Regular RTCP should be sent or suppressed based on t_rr_interval, set by user
			*/
			s->rtcpFbPlugin->callbacks->rtcpFbAdd((RvRtcpSession)s, &buf, &delay, &sendRtcp);
		}
#endif
			
		/* Look for "dead" session members */
		rtcpRemoveDormantParticipants(s);

		if (sendRtcp == RV_TRUE)
		{
			rtcpSessionRawSend((RvRtcpSession)s, buf.buffer, buf.length, MAXRTCPPACKET-8, &rtcpPackSize);

			s->txInterval.lastRTCPPacketSize = rtcpPackSize + RTCP_PACKETOVERHEAD;

			/* Update the average packet size  section 6.3.3 RFC 3550 */
			s->txInterval.aveRTCPPacketSize = (s->txInterval.lastRTCPPacketSize +
											  s->txInterval.aveRTCPPacketSize*RvUint32Const(15)) / RvUint32Const(16);
			s->txInterval.previousTime = currentTime;
			s->txInterval.nextTime = RvInt64Add(s->txInterval.previousTime,
				RvInt64Mul(RvInt64FromRvUint32(delay), RV_TIME64_NSECPERMSEC));

			RvLogDebug(rvLogPtr, (rvLogPtr, "rtcpTimerCallback: PrevTime=%d, Update: Next Report in %d sec", 
						(RvUint32)RvUint64Div(s->txInterval.previousTime, RV_TIME64_NSECPERSEC), delay/1000));
			RvLogDebug(rvLogPtr, (rvLogPtr, "rtcpTimerCallback: NextTime=%d", 
						(RvUint32)RvUint64Div(s->txInterval.nextTime,RV_TIME64_NSECPERSEC)));
			
		}

		/* switch off the flag that it's initial (first) RTCP report */
		s->txInterval.initial = RV_FALSE; 

		s->txInterval.pmembers = s->txInterval.members;
    }
	
	/* reschedule the timer */
	rtcpSetTimer(s, delay * RV_TIME64_NSECPERMSEC, rtcpTimerCallback);

    RTPLOG_LEAVE(TimerCallback);
    return RV_FALSE;
}

static RvStatus rtcpSessionRawReceive(
                                      IN    RvRtpDemux  demux,
                                      IN    rtcpSession **ses,
                                      INOUT RvRtpBuffer* buf,
                                      IN    RvSize_t bytesToReceive,
                                      OUT   RvSize_t*   bytesReceivedPtr,
                                      OUT   RvAddress*  remoteAddressPtr)
{
    RvStatus       res           = RV_ERROR_UNKNOWN;
    RvRtcpSession  hRTCP         = (RvRtcpSession) *ses;
    rtcpSession    *s            = (rtcpSession    *) hRTCP;
    RvSize_t       bytesPending = 0;
    RvBool         isPeekMessage = RV_TRUE;
#ifdef __H323_NAT_FW__
    RtpDemux*      demuxPtr      = (RtpDemux*) demux;

    RTPLOG_ENTER(SessionRawReceive);
	
	if (*bytesReceivedPtr)
		*bytesReceivedPtr = 0;

    if (demuxPtr != NULL)
    {
        RvLockGet(&demuxPtr->lock, logMgr);

		res = RvTransportReceiveBuffer(
                        demuxPtr->rtcpTransport, buf->buffer, bytesToReceive,
                        0 /*options*/, remoteAddressPtr, &bytesPending);
                isPeekMessage = RV_FALSE;
        if (res == RV_OK && bytesPending<4)
        {
            /* read all spam message */
            res = RvTransportReceiveBuffer(
                    demuxPtr->rtcpTransport, buf->buffer, bytesToReceive,
                    0 /*options*/, remoteAddressPtr, &bytesPending);
            RTPLOG_LEAVE(SessionRawReceive);
            RvLockRelease(&demuxPtr->lock, logMgr);

			if (res == RV_OK && bytesReceivedPtr)
				*bytesReceivedPtr = bytesPending;

            return RV_ERROR_UNKNOWN;
        }
        if (res == RV_OK)
        {
            buf->length = (RvUint32)bytesPending;
            res = RtpDemuxGetRTCPsession(demuxPtr, buf, &bytesPending, &hRTCP);
            if (res != RV_OK)
            {
                /* read the remainder of the spam message */
                if (isPeekMessage)
                {
                    res = RvTransportReceiveBuffer(
                            demuxPtr->rtcpTransport,buf->buffer, bytesToReceive,
                            0 /*options*/, remoteAddressPtr, &bytesPending);
                }
                *bytesReceivedPtr =0;
                RTPLOG_ERROR_LEAVE(SessionRawReceive, "Cannot find the RTCP session");
                RvLockRelease(&demuxPtr->lock, logMgr);
                return res;  
            }
            s = *ses = (rtcpSession *)hRTCP;     
        }
        else            
        {    
            RTPLOG_ERROR1(SessionRawReceive, "RvSocketPeekBuffer failed");
            /*RvSocketReceiveBuffer(&sock, buf->buffer, bytesToReceive, logMgr, &bytesPending, remoteAddressPtr);*/
            RTPLOG_LEAVE(SessionRawReceive);
            RvLockRelease(&demuxPtr->lock, logMgr);
            return res;
        }

        RvLockRelease(&demuxPtr->lock, logMgr);
    }
#else
    RV_UNUSED_ARG(demux);
    RV_UNUSED_ARG(bytesPending);
    RTPLOG_ENTER(SessionRawReceive);
#endif
    
    if (s != NULL && s->profilePlugin !=NULL && 
        s->profilePlugin->funcs!=NULL &&
        s->profilePlugin->funcs->xrtcpRawReceive!= NULL)
    {
       res = s->profilePlugin->funcs->xrtcpRawReceive(
           s->profilePlugin, hRTCP, buf, bytesToReceive, isPeekMessage, bytesReceivedPtr, remoteAddressPtr);
    }
    RTPLOG_LEAVE(SessionRawReceive);
    return res;
}

static
void rtcpEventCallback(
                IN RvTransport         transport,
                IN RvTransportEvents   ev, 
                IN RvBool              error, 
                IN void*               usrData)
{
    rtcpSession* s =  (rtcpSession*)usrData;
    RvUint32     buffer[MAXRTCPPACKET/sizeof(RvUint32)+1];
    RvRtpBuffer  buf;
    RvStatus     res;
    RvAddress    remoteAddress;
    RvSize_t     bytesReceived;

    RTPLOG_ENTER(EventCallback);
    RV_UNUSED_ARG(transport);
    RV_UNUSED_ARG(ev);
    RV_UNUSED_ARG(error);

	if (s->demux)
	{
		rtcpDemuxEventCallback(transport, ev, error, s->demux);
		return;
	}

    buf = buffCreate(buffer, MAXRTCPPACKET);
    res = rtcpSessionRawReceive(NULL, &s, &buf, (RvSize_t)buf.length, &bytesReceived, &remoteAddress);   

    if (s!=NULL && s->isShutdown)
    {
        /* session must not receive RTCP packet after sending BYE packet */
        RTPLOG_LEAVE(EventCallback);
        return;
    }
    if (res == RV_OK && bytesReceived)
    {
        RvUint16 localPort = 0;

        buf.length = (RvUint32)bytesReceived;

        RvTransportGetOption(s->transport,
            RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
            RVTRANSPORT_OPT_LOCALPORT, (void*)&localPort);

        if ((RvAddressGetIpPort(&remoteAddress) != localPort) ||
            !isMyIP(&remoteAddress))
        {
            res=rtcpProcessCompoundRTCPPacket((RvRtcpSession)s, &buf, getNNTPTime(), &remoteAddress);
            if (res==0) 
            {
				RvUint32 ssrc=getSSRCfrom(buf.buffer);
				//@beg add lishixin
				RvAddressIpv4 * ip4 = RvAddressGetIpv4(&remoteAddress);
				RvChar * ip = NULL;
				RvUint16 port = 0;
				if(ip4 != NULL)
				{
					RvChar buff_temp[64];
					ip=RvAddressIpv4ToString(buff_temp,sizeof(buff_temp),ip4->ip);
					port = ip4->port;
				}
				//@end lishixin
				if (s->rtcpRecvCallbackEx != NULL)
                    s->rtcpRecvCallbackEx((RvRtcpSession)s, s->haRtcp, ssrc, buf.buffer, buf.length,ip,port,RV_FALSE,0);
				else if (s->rtcpRecvCallback != NULL)
                    s->rtcpRecvCallback((RvRtcpSession)s, s->haRtcp, ssrc);
            }
        }
    }
    RTPLOG_LEAVE(EventCallback);
}

/************************************************************
 * getNNTPTime 
 * Description: Gets NTP time.
 * Parameters: 
 *   Input: none
 *   Output: none
 * Return value NTP time.
 ************************************************************/
static RvUint64 getNNTPTime(void)
{
    RvTime t;
    RvNtpTime ntptime;
    RvUint64  result;
/*    RvUint64  timestamp;*/

    RvLogEnter(rvLogPtr, (rvLogPtr, "getNNTPTime"));
	RvClockGet(NULL, &t);
    RvNtpTimeConstructFromTime(&t, RV_NTPTIME_ABSOLUTE, &ntptime); /* convert to NTP time */
    result = RvNtpTimeTo64(&ntptime, 32, 32); /* Convert to format getNNTPTime returns */
    RvLogLeave(rvLogPtr, (rvLogPtr, "getNNTPTime"));
    return result;
}

static void setSDES(RvRtcpSDesType type, rtcpSDES* sdes, RvUint8 *data, RvInt32 length)
{
    RvLogEnter(rvLogPtr, (rvLogPtr, "setSDES"));
    sdes->type   = (unsigned char)type;
    sdes->length = (unsigned char)length;
    memcpy(sdes->value, data, (RvSize_t)length);
    memset(sdes->value+length, 0, (RvSize_t)( 4-((length+2)%sizeof(RvUint32)) ));
    RvLogLeave(rvLogPtr, (rvLogPtr, "setSDES"));
}

static void init_seq(rtpSource *s, RvUint16 seq)
{
    RvLogEnter(rvLogPtr, (rvLogPtr, "init_seq"));
    s->base_seq       = seq;
    s->max_seq        = seq;
    s->bad_seq        = RTP_SEQ_MOD + 1;
    s->cycles         = 0;
    s->received       = 0;
    s->received_prior = 0;
    s->expected_prior = 0;
    RvLogLeave(rvLogPtr, (rvLogPtr, "init_seq"));
}


static RvInt32 update_seq(rtpSource *s, RvUint16 seq, RvUint32 ts, RvUint32 arrival)
{
    RvUint16 udelta = (RvUint16)(seq - s->max_seq);
    RvLogEnter(rvLogPtr, (rvLogPtr, "update_seq"));
    if (s->probation)
    {
        if (seq == s->max_seq + 1)
        {
            s->probation--;
            s->max_seq = seq;
            if (s->probation == 0)
            {
                init_seq(s, seq);
                s->received++;
                RvLogLeave(rvLogPtr, (rvLogPtr, "update_seq"));
                return 1;
            }
        }
        else
        {
            s->probation = MIN_SEQUENTIAL - 1;
            s->max_seq = seq;
        }
        RvLogLeave(rvLogPtr, (rvLogPtr, "update_seq"));
        return RV_OK;
    }
    else if (udelta < MAX_DROPOUT)
    {
        if (seq < s->max_seq) s->cycles += RTP_SEQ_MOD;
        s->max_seq = seq;
    }
    else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER)
    {
        if (seq == s->bad_seq)
        {
            init_seq(s, seq);
        }
        else
        {
            s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
            RvLogLeave(rvLogPtr, (rvLogPtr, "update_seq"));
            return RV_OK;
        }
    }
    else
    {
   /* duplicate or reordered packet */
    }
    {
        RvInt32 transit = (RvInt32)(arrival - ts);
        RvInt32 d = (RvInt32)(transit - s->transit);
        s->transit = transit;
        if (d < 0) d = -d;
        /* calculation scaled to reduce round off error */
        s->jitter += d - ((s->jitter + 8) >> 4);
    }
    s->received++;
    RvLogLeave(rvLogPtr, (rvLogPtr, "update_seq"));
    return 1;
}


/*=========================================================================**
**  == makeHeader() ==                                                     **
**                                                                         **
**  Creates an RTCP packet header.                                         **
**                                                                         **
**  PARAMETERS:                                                            **
**      ssrc       A synchronization source value for the RTCP session.    **
**                                                                         **
**   count (subtype)                                                       **
** [count]     - A count of sender and receiver reports in the packet.     **
** [subtype]   - to allow a set of APP packets to be defined under one     **
**               unique name, or for any application-dependent             **
**               data.                                                     **
**                                                                         **
**                                                                         **
**      type       The RTCP packet type.                                   **
**                                                                         **
**      dataLen    The length of the data in the packet buffer, in         **
**                 octets, including the size of the header.               **
**                                                                         **
**  RETURNS:                                                               **
**      The function returns a header with the appropriate parameters.     **
**                                                                         **
**=========================================================================*/

static rtcpHeader makeHeader(RvUint32 ssrc, RvUint8 count, RtcpType type,
                             RvUint16 dataLen)
{
    rtcpHeader header;

    RvLogEnter(rvLogPtr, (rvLogPtr, "makeHeader"));
    header.ssrc = ssrc;

    header.bits = RTCP_HEADER_INIT;
    header.bits = bitfieldSet(header.bits, count, HEADER_RC, HDR_LEN_RC);
    header.bits = bitfieldSet(header.bits, type,  HEADER_PT, HDR_LEN_PT);
    header.bits = bitfieldSet(header.bits, W32Len(dataLen) - 1, HEADER_len, HDR_LEN_len);

    ConvertToNetwork(&header, 0, W32Len(SIZEOF_RTCPHEADER));
    RvLogLeave(rvLogPtr, (rvLogPtr, "makeHeader"));
    return header;
}


static RvBool dataAddToBuffer(IN		void *			data, 
							  IN		RvUint32		size, 
							  IN OUT	RvRtpBuffer *	buf, 
							  IN OUT	RvUint32 *		allocated)
{
	/* add if we have enough space in buf */
    if ((buf->length - *allocated) > size)
	{
		memcpy((RvUint8*)buf->buffer + *allocated, data, size);
		*allocated += size;
		return RV_TRUE;
	}
	return RV_FALSE;
}


static RvBool dataAddToBufferAndConvert(
							 IN			void *			data, 
							 IN			RvUint32		size, 
							 IN OUT		RvRtpBuffer	*	buf, 
							 IN OUT		RvUint32 *		allocated)
{
	/* add if we have enough space in buf */
    if (dataAddToBuffer(data, size, buf, allocated))
	{
		ConvertToNetwork(buf->buffer + (*allocated-size), 0, W32Len(size));
		return RV_TRUE;
	}
	return RV_FALSE;
}


static RvUint32 getLost(rtpSource *s)
{
    RvUint32 extended_max;
    RvUint32 expected;
    RvInt32 received_interval;
    RvInt32 expected_interval;
    RvInt32 lost;
    RvInt32 lost_interval;
    RvUint8 fraction;
	RvInt32 bfLost;

    RvLogEnter(rvLogPtr, (rvLogPtr, "getLost"));
    extended_max = s->cycles + s->max_seq;
    expected = extended_max - s->base_seq + 1;
    lost = expected - s->received;
    expected_interval = expected - s->expected_prior;
    s->expected_prior = expected;
    received_interval = s->received - s->received_prior;
    s->received_prior = s->received;
    lost_interval = expected_interval - received_interval;

    if (expected_interval == 0  ||  lost_interval <= 0)
        fraction = 0;
    else
        fraction = (RvUint8)((lost_interval << 8) / expected_interval);

	bfLost = lost;
	if (bfLost < 0)
	{
		bfLost &= 0xFFFFFF;  /* lost is 24 bits size */
	}

	bfLost |= (fraction << 24);
	
    RvLogLeave(rvLogPtr, (rvLogPtr, "getLost"));
    return bfLost;
}


static RvUint32 getJitter(rtpSource *s)
{
    RvLogEnter(rvLogPtr, (rvLogPtr, "getJitter"));
    RvLogLeave(rvLogPtr, (rvLogPtr, "getJitter"));
    return s->jitter >> 4;
}


static RvUint32 getSequence(rtpSource *s)
{
    return s->max_seq + s->cycles;
}



static RvUint32 getSSRCfrom(RvUint8 *head)
{
   RvUint8 *ssrcPtr = (RvUint8 *)head + sizeof(RvUint32);
   RvLogEnter(rvLogPtr, (rvLogPtr, "getSSRCfrom"));
   RvLogLeave(rvLogPtr, (rvLogPtr, "getSSRCfrom"));
   return *(RvUint32 *)(ssrcPtr);
}

/************************************************************************************
 *  findSSrc                                                                                                                        
 *  Finds the synchronization source information by the SSRC.                                                                                                                
 *  PARAMETERS:                                                             
 *   Input:   ssrc       A remote synchronization source value for the RTCP session. 
 *   Output: none.
 *  RETURNS:                                                                
 *      The function returns the pointer to rtcpInfo on success,
 *      NULL otherwise.
 ***********************************************************************************/
static rtcpInfo *findSSrc(rtcpSession *s, RvUint32 ssrc)
{
    RvInt32     index = 0;
    RvBool  doAgain = RV_TRUE;
    rtcpInfo *pInfo;

    RvLogEnter(rvLogPtr, (rvLogPtr, "findSSrc"));

    if (s == NULL)
        return NULL;

    /* Look for the given SSRC */
    while ((doAgain) && (index < s->sessionMembers))
    {
       if (s->participantsArray[index].ssrc == ssrc)
            doAgain = RV_FALSE;
       else
           index ++;

    }
    if (index < s->sessionMembers )
        pInfo = &s->participantsArray[index];
    else
        pInfo = NULL;

    RvLogLeave(rvLogPtr, (rvLogPtr, "findSSrc"));
    return pInfo;
}

/* insert new member to a participants table */
static rtcpInfo *insertNewSSRC(rtcpSession *s, RvUint32 ssrc, RvAddress* remoteAddress)
{
    rtcpInfo* pInfo = NULL;
    RvInt32 index;

    RvLogEnter(rvLogPtr, (rvLogPtr, "insertNewSSRC"));

    if (s->sessionMembers >= s->maxSessionMembers)
    {
        /* We've got too many - see if we can remove some old ones */
        for(index=0; index < s->sessionMembers; index++)
            if (s->participantsArray[index].invalid &&
                s->participantsArray[index].ssrc == 0)
                break;
            /* BYE message can transform participant to invalid */
        /* if index < s->sessionMembers found unusable index */
    }
    else
    {
        /* Add it as a new one to the list */
        index = s->sessionMembers;
        s->sessionMembers++;
    }

    if (index < s->sessionMembers)
    {
        /* Got a place for it ! */
#ifdef __RTCP_XR__        
        void* temp;
        pInfo = &s->participantsArray[index];
        temp = pInfo->eXR;
        memset(pInfo, 0, sizeof(rtcpInfo));
        pInfo->eXR = temp;
#else
        pInfo = &s->participantsArray[index];
        memset(pInfo, 0, sizeof(rtcpInfo));
#endif
        pInfo->ssrc             = ssrc;
		if (remoteAddress)
		memcpy(&pInfo->remoteAddress, remoteAddress, sizeof (RvAddress));
        pInfo->eToRR.ssrc       = ssrc;
        pInfo->active           = RV_FALSE;
        pInfo->src.probation    = MIN_SEQUENTIAL;
    }

    RvLogLeave(rvLogPtr, (rvLogPtr, "insertNewSSRC"));
    return pInfo;
}


/* Remove old member from a participants table */
static RvStatus	removeOldSSRC(rtcpSession *s, RvUint32 ssrc)
{
    RvInt32		index=0;
	RvStatus	status = RV_OK; 
	
    RvLogEnter(rvLogPtr, (rvLogPtr, "removeOldSSRC"));
	
    /* Look for the given SSRC */
    while (s->participantsArray[index].ssrc != ssrc)
    {
		if (++index == s->sessionMembers)
		{
			status = RV_ERROR_NOT_FOUND;
			break;
		}
    }

	if (index < s->sessionMembers)
    {
        /* We found it: move last participant's info on the place of this one */
		/* No need to do it if the removable participant is the last one. */
		if (index != s->sessionMembers -1)
			memcpy(&s->participantsArray[index], &s->participantsArray[s->sessionMembers -1], sizeof(rtcpInfo));
		/* clean the place */
		memset(&s->participantsArray[s->sessionMembers -1], 0, sizeof(rtcpInfo));

		/* Now decrease the number of participants */
		s->sessionMembers--;
    }

	
    RvLogLeave(rvLogPtr, (rvLogPtr, "removeOldSSRC"));
    return status;
}


static 	void rtcpScheduleAutoReports(rtcpSession *s)
{
		
	if (s->isTimerSet == RV_FALSE) 
	{
		RvUint32 delay;
		RvStatus status;
		
		delay = rtcpInitTransmissionIntervalMechanism(s);
		status = rtcpSetTimer(s, RvInt64Mul(RV_TIME64_NSECPERMSEC, RvInt64Const(1, 0, delay)), rtcpTimerCallback);
		
		if (status == RV_OK)
			s->isTimerSet = RV_TRUE;
	}
}
			
static RvUint32 rtcpRemoveDormantParticipants(rtcpSession *s)
{
	RvInt32		i;	
	rtcpInfo	*pInfo;
	RvInt32		initSessionMembers;
	RvInt32		cnt=0;

	RvLockGet(&s->lock, NULL);

	/* Browse the participants list */
	initSessionMembers = s->sessionMembers;
    for (i=0; i < initSessionMembers; i++)
    {
		pInfo = &s->participantsArray[i];
		if (pInfo->tLastReport > 0)
		{
			RvUint32 timeSinceLastReport = (RvUint32)RvUint64Div(RvTimestampGet(NULL), RV_TIME64_NSECPERMSEC) - pInfo->tLastReport;

			if (timeSinceLastReport > (5 * s->txInterval.baseTimeInterval))
			{
				RvBool res=RV_TRUE;

				/* Inform application about SSRC which is going to be removed */
				if (s->ssrcRemoveCallback != NULL)
				{
					RvLockRelease(&s->lock, NULL);
					res = s->ssrcRemoveCallback((RvRtcpSession)s, s->ssrcRemoveContext, pInfo->ssrc);
					RvLockGet(&s->lock, NULL);
				}

				if (res == RV_TRUE)
				{
					//RvRtcpRemoveRemoteAddress((RvRtcpSession)s, (RvNetAddress *)&pInfo->remoteAddress);
					removeOldSSRC(s, pInfo->ssrc);
					++cnt;
				}
			}
		}
    }

	RvLockRelease(&s->lock, NULL);
	return cnt;
}


                 /* == ENDS: Internal RTCP Functions == */
#ifdef __cplusplus
}
#endif


























