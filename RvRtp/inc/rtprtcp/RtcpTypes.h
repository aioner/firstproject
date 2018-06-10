                                                                                     /************************************************************************
 File Name	   : RtcpTypes.h
 Description   : scope: Private
     RTCP related type definitions
*************************************************************************
************************************************************************
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

#ifndef __RTCP_TYPES_H__
#define __RTCP_TYPES_H__

#include "rvtypes.h"
#include "rvtransport.h"
#include "rvrtpconfig.h"
#include "rtcp.h"
#include "rvrtpaddresslist.h"
#include "rtpProfilePlugin.h"
#include "rvrtpnatfw.h"
#include "rvrtpstunfw.h"
#include "rtcpbasetypes.h"
#ifdef __RTCP_XR__
#include "rtcpxrplugin.h"
#endif
#ifdef __RTCP_FB__
#include "rtcpfbplugin.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


typedef struct __rtcpSession
{
   RvBool                       isAllocated;
   RvBool                       isShutdown;
   RvTransport                  transport;
   RvSelectEngine               *selectEngine;     /* Select engine used by this fd */
   rtcpMyInfo                   myInfo;
   RvRtpAddressList             addressList; /* address list to store the remote addresses for unicast/multicast */
   RvBool                       remoteAddressSet;
   RvTimer                      timer;             /* Timer of this RTCP session */
   RvBool                       isTimerSet;        /* RV_TRUE if we started a timer for this session */
   rtcpInfo*                    participantsArray; /* session members information array */
   RvInt32                      sessionMembers;
   RvInt32                      maxSessionMembers;
   RvRtcpEventHandler_CB        rtcpRecvCallback;
   RvRtcpEventHandlerEx_CB      rtcpRecvCallbackEx; /* Extended Callback to inform application that RTCP was!!! received */
   RvRtcpSendHandlerEx_CB       rtcpSendCallbackEx; /* Callback to inform application that RTCP was!!! sent */
   void*                        haRtcp;
   RvRtcpAppEventHandler_CB     rtcpAppRecvCallback; /* Callback for received RTCP APP messages */
   void*                        rtcpAppContext;
   RvRtcpByeEventHandler_CB     rtcpByeRecvCallback; /* Callback for received RTCP BYE messages */
   void*                        rtcpByeContext;
   RvChar*                      byeReason;
   RvUint32                     byeReasonLength;
   RvRtpShutdownCompleted_CB    rtpShutdownCompletedCallback; /* Callback defines the function for RTP session closing */
   void*                        rtcpShutdownCompletedContext;
   RvRtcpSDESMessageReceived_CB rtcpSDESMessageArrivedCallback;
   RvRtcpSDESMessageReceivedEx_CB rtcpSDESMessageArrivedCallbackEx;
   void*                        rtcpSDESContext;
   RvRtcpSRRRMessageReceived_CB	rtcpSRRRMessageArrivedCallback; /* Callback for received RTCP SR/RR messages */
   RvRtcpSRRRMessageReceivedEx_CB	rtcpSRRRMessageArrivedCallbackEx; /* Callback for received RTCP SR/RR messages */
   void*                        rtcpSRRRContext;
   RvRtpSession                 rtpSession;
   RvBool						isManual;
   rtcpTxInterval               txInterval;
   RvRtpEncryptionPlugIn        *encryptionPlugInPtr;   /* Registered encryption plug in */
   RvRtpEncryptionKeyPlugIn     *encryptionKeyPlugInPtr;/* Registered encryption key plug in */
   RvRtpEncryptionMode          encryptionMode;
   RvUint32                     index; /* RTCP sequence number (required by some encryption plugins) */
   RvLock                       lock; /* Lock of this session. Used to protect the session members */
   RtpProfilePlugin             *profilePlugin;
#ifdef __H323_NAT_FW__
   RvRtpDemux                   demux;           /* handle of demultiplexing object */
   RvUint32                     demuxTblEntry;   /* index in demux table */
#endif
#ifdef __RTP_OVER_STUN__
   RvRtcpRawBufferReceived_CB   rtcpRawReadEventHandler;  /* the raw data received message indication event handler on RTP socket */
   void*                        rtcpRawReadContext;       /* the context for raw data received message indication event handler */
#endif
   RvRtcpRoundTripDelay_CB      roundTripDelayCallback;
   RvRtcpRoundTripDelayEx_CB    roundTripDelayCallbackEx;
   void*                        roundTripDelayContext;
   RvRtcpSsrcRemove_CB			ssrcRemoveCallback;
   void*                        ssrcRemoveContext;
#ifdef __RTCP_XR__
   RtcpXrPlugin*                rtcpxrplugin;     /* RTCP XR plug-in */
#endif
#ifdef __RTCP_FB__
   RtcpFbPlugin*                rtcpFbPlugin;     /* RTCP FB plug-in */
#endif
   
} rtcpSession;



#define RV_RTCP_MAXIPS RvUint32Const(20)

/* Internal RTCP instance used. There a single such object */
typedef struct
{
    RvSelectEngine*     selectEngine; /* select engine used for RTP/RTCP messages */
    RvAddress           localAddress;
    RvAddress           hostIPs[RV_RTCP_MAXIPS]; /* Local host IPs */
    RvUint32            addresesNum;      /* number of addresses in host list */
    RvUint32            timesInitialized; /* Times the RTP was initialized */

    RvTimerQueue*       timersQueue; /* Timers queue to use */

    RvRandomGenerator   randomGenerator; /* Random numbers generator to use */
} RvRtcpInstance;

extern RvRtcpInstance rvRtcpInstance;

RvUint32 rtcpInitTransmissionIntervalMechanism(
        IN rtcpSession* s);


/*****************************************************************************************************
 * rtcpPacketSend
 * description: sends regular or multiplexed RTCP packet
 * scope: private
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
    OUT   RvSize_t*      bytesSent);

RVAPI
RvRtcpSession RVCALLCONV rtcpDemuxOpenFrom(
    IN  RvRtpDemux    demux,
    IN  RvUint32      ssrc,
    IN  RvNetAddress* pRtcpAddress,
    IN  char *        cname,
    IN  RvInt32       maxSessionMembers,
    IN  void *        buffer,
    IN  RvInt32       bufferSize);

RVAPI
RvRtcpSession RVCALLCONV rtcpDemuxOpen(
    IN  RvRtpDemux    demux,
    IN  RvUint32      ssrc,
    IN  RvNetAddress* pRtcpAddress,
    IN  char *        cname);

RVAPI
RvRtcpSession RVCALLCONV rtcpDemuxOpenEx(
    IN  RvRtpDemux    demux,
    IN  RvUint32      ssrc,
    IN  RvTransport   transp,
    IN  char *        cname,
    IN  RvInt32       maxSessionMembers, /* if 0, RTCP_MAXRTPSESSIONMEMBERS is used */
    IN  void *        buffer, /* if NULL, the memory will be allocated internally */
    IN  RvInt32       bufferSize /* ignored if buffer is NULL */);

/**************************************************************************************
 * rtcpDemuxAddRemoteAddress
 * description: adds the new RTCP address of the remote peer or the address a multi-cast group
 *              or of multi-unicast address list to which the RTCP stream will be sent.
 *              
 * input:
 *        hRTCP           - handle to RTCP demultiplexing object
 *        pRtcpAddress    - pRtpAddress of remote RTCP session
 *        pMultiplexId  - pointer to multiplexID of remote peer.
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
    IN RvUint32*      pMultiplexId);


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
    IN  RvUint32*      pMultiplexID);

#ifdef __cplusplus
}
#endif

#endif  /* __RTCP_TYPES_H__ */







