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


#include "rvstdio.h"
#include "rvmemory.h"
#include "rvtimestamp.h"
#include "rvrandomgenerator.h"
#include "rvhost.h"
#include "rvsocket.h"
#include "rvselect.h"
#include "rvcbase.h"
#include "rvtransportsocket.h"

#include "bitfield.h"
#include "rtputil.h"

#include "rtp.h"
#include "rtcp.h"
#include "rvlog.h"
#include "rvrtpaddresslist.h"

#include "RtpProfileRfc3550.h"

#ifdef __H323_NAT_FW__
#include "RtpDemux.h"
#endif


#ifdef __RTP_OVER_STUN__
#include "rvrtpstunfw.h"
#endif

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)
#define rvLogPtr         (rtpGetSource(RVRTP_RTP_MODULE))
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"

#ifdef __cplusplus
extern "C" {
#endif


#define BUILD_VER_NUM(_max, _min, _dot1, _dot2) \
    ((RvUint32)((_max << 24) + (_min << 16) + (_dot1 << 8) + _dot2))

#define VERSION_STR    "Add-on RTP/RTCP Version 3.6.0.19"
#define VERSION_NUM    BUILD_VER_NUM(3, 6, 0, 19)

/* RTP instance to use */
static RvUint32      rtpTimesInitialized   = 0;       /* Times the RTP was initialized */
RvRtpInstance        rvRtpInstance;

// 会话创建关闭全局锁
RvLock g_rtplock;

static
void rtpEvent(
    RvTransport       transport,
    RvTransportEvents ev,
    RvBool            error,
    void*             usrData);

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
RvInt32 RVCALLCONV RvRtpInit(void)
{
    RvStatus status;
	RvUint32 rtpMaxSessions=1024;

    status = RvCBaseInit();
    if (status != RV_OK)
        return status;

    if (rtpTimesInitialized == 0)
    {
        RvUint32 numAddrs = RV_RTP_MAXIPS;
        RvUint32 countModules = 0;

        for (countModules = 0; countModules < RVRTP_MODULES_NUMBER; countModules++)
        {
            rvRtpInstance.logManager.bLogSourceInited[countModules] = RV_FALSE;
        }
        /* Find the list of host addresses we have */
        status = RvHostLocalGetAddress(logMgr, &numAddrs, rvRtpInstance.hostIPs);
        if (status != RV_OK)
        {
            RvCBaseEnd();
            return status;
        }
        rvRtpInstance.addressesNum = numAddrs;
        /* Create a random generator */
        RvRandomGeneratorConstruct(&rvRtpInstance.randomGenerator,
            (RvRandom)(RvTimestampGet(logMgr)>>8));

        /* Creating the destination addresses pool */
#if defined (RV_RTP_DEST_POOL_OBJ)
        RvRtpDestinationAddressPoolConstruct();
#endif
        if (numAddrs>0)
            RvAddressCopy(&rvRtpInstance.hostIPs[0], &rvRtpInstance.rvLocalAddress);
        else
#if (RV_NET_TYPE & RV_NET_IPV6)
            RvAddressConstruct(RV_ADDRESS_TYPE_IPV6, &rvRtpInstance.rvLocalAddress);
#else
            RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &rvRtpInstance.rvLocalAddress);
#endif

		// zzx 20140524
		RvLockConstruct(logMgr, &g_rtplock);
    }

#ifdef RVRTP_MAXSESSIONS
	rtpMaxSessions = RVRTP_MAXSESSIONS;
#endif
    status = RvSelectConstruct(rtpMaxSessions, 0, logMgr, &rvRtpInstance.selectEngine);
    if (status != RV_OK)
    {
        RvCBaseEnd();
        return status;
    }
    rtpTimesInitialized++;
    return RV_OK;
}


/************************************************************************************
 * RvRtpSetPortRange
 * description: Set the RTP/RTCP ports range to RTP stack instance
 * input: firstPort - first port that may be used for RTP session
 *		  lastPort - lat port that may be used for RTP session
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpSetPortRange(RvUint16 firstPort, RvUint16 lastPort)
{
    RvStatus status = RV_OK;

	if (firstPort > lastPort)
		status = ERR_RTP_PORTRANGE;
	else
	{
		/* If firstPort is not even set next value, which is even number */
		if (firstPort & 1)
			++firstPort;

		/* If lastPort is not even set previous value, which is even number */
		if (lastPort & 1)
			--lastPort;

		status = rtpPortRangeConstruct(&rvRtpInstance.portRange, firstPort, lastPort);
	}

	return status;
}


#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtpInitEx(RvUint32 ip)
{
    RvInt32 rc;
    if ((rc = RvRtpInit()) != RV_ERROR_UNKNOWN)
    {
        RvAddressConstructIpv4(&rvRtpInstance.rvLocalAddress, ip, RV_ADDRESS_IPV4_ANYPORT);
    }
    return rc;
}
#endif

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
 * - RvRtpInit() binds to the 揳ny?IPV4 address.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpInitEx(RvNetAddress* pRtpAddress)
{
    RvInt32 rc = -1;
    RV_UNUSED_ARG(pRtpAddress);

	rc=RvRtpInit();

    return rc;
}

/************************************************************************************
 * rtpSetLocalAddress
 * description: Set the local address to use for calls to rtpOpenXXX functions.
 *              This parameter overrides the value given in rtpInitEx() for all
 *              subsequent calls.
 * input: ip    - Local IP address to use
 * output: none.
 * return value: Non-negative value on success
 *               negative value on failure
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtpSetLocalAddress(IN RvUint32 ip)
{
	RvNetAddress rtpAddress;
    RvNetIpv4   Ipv4;
    Ipv4.ip = ip;
    Ipv4.port = 0;
    RvNetCreateIpv4(&rtpAddress, &Ipv4);
	return RvRtpSetLocalAddress(&rtpAddress);
}
#endif

RVAPI
RvInt32 RVCALLCONV RvRtpSetLocalAddress(IN RvNetAddress* pRtpAddress)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetLocalAddress"));
	if (NULL!=pRtpAddress)
	{
        RvAddressCopy((RvAddress*)pRtpAddress->address, &rvRtpInstance.rvLocalAddress);
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetLocalAddress"));
    return RV_OK;
}
/************************************************************************************
 * RvRtpEnd
 * description: Shuts down an instance of an RTP module.
 * input: none.
 * output: none.
 * return value: none.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtpEnd(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpEnd"));
    rtpTimesInitialized--;

    if (rtpTimesInitialized == 0)
    {
        RvRandomGeneratorDestruct(&rvRtpInstance.randomGenerator);
        RvSelectDestruct(rvRtpInstance.selectEngine, 0);
#if defined(RV_RTP_DEST_POOL_OBJ)
        RvRtpDestinationAddressPoolDestruct();
#endif
		if (rvRtpInstance.portRange.portBitMap != NULL)
			rtpPortRangeDestruct(&rvRtpInstance.portRange);
    }
    RvCBaseEnd();

    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpEnd"));
}

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
RvInt32 RVCALLCONV RvRtpGetAllocationSize(void)
{
    return (RvRoundToSize(sizeof(RvRtpSessionInfo), RV_ALIGN_SIZE)
          + RvRoundToSize(sizeof(RtpProfilePlugin), RV_ALIGN_SIZE));
}

/************************************************************************************
 * RvRtpOpenFrom
 * description: Opens an RTP session in the memory that the application allocated.
 * input: pRtpAddress - contains The UDP port number to be used for the RTP session.
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
        IN  RvInt32         bufferSize)
{
	RvNetAddress rtpAddress;
    RvNetIpv4   Ipv4;
    Ipv4.ip =   0;
    Ipv4.port = port;
    RvNetCreateIpv4(&rtpAddress, &Ipv4);
	return RtpOpenFrom(NULL, &rtpAddress, ssrcPattern, ssrcMask, buffer,
                       bufferSize, NULL /*cname*/);
}
#endif

RVAPI
RvRtpSession RVCALLCONV RvRtpOpenFrom(
									IN  RvNetAddress* pRtpAddress,
									IN  RvUint32    ssrcPattern,
									IN  RvUint32    ssrcMask,
									IN  void*       buffer,
									IN  RvInt32         bufferSize)
{

    RvRtpSession         res;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpOpenFrom"));
	res = RtpOpenFrom(NULL, pRtpAddress, ssrcPattern, ssrcMask, buffer,
                      bufferSize, NULL /*cname*/);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpOpenFrom"));
    return res;
}

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
        IN  RvUint32    ssrcMask)
{
	RvNetAddress rtpAddress;
    RvNetIpv4   Ipv4;
    Ipv4.ip = 0;
    Ipv4.port = port;
    RvNetCreateIpv4(&rtpAddress, &Ipv4);
    return RtpOpenFrom(NULL, &rtpAddress, ssrcPattern, ssrcMask,
                       NULL /*buffer*/, 0 /*bufsize*/, NULL /*cname*/);
}
#endif
RVAPI
RvRtpSession RVCALLCONV RvRtpOpen(	  IN  RvNetAddress* pRtpAddress,
									  IN  RvUint32    ssrcPattern,
									  IN  RvUint32    ssrcMask)
{
    RvRtpSession s = NULL;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpOpen"));
    s = RtpOpenFrom(NULL, pRtpAddress, ssrcPattern, ssrcMask,
                    NULL /*buffer*/, 0 /*bufsize*/, NULL /*cname*/);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpOpen"));
	return s;
}
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
        IN  char *      cname)
{
	RvNetAddress rtpAddress;
    RvNetIpv4 Ipv4;

    Ipv4.ip = 0;
    Ipv4.port = port;
    RvNetCreateIpv4(&rtpAddress, &Ipv4);
    return RtpOpenFrom(NULL, &rtpAddress, ssrcPattern, ssrcMask,
                       NULL /*buffer*/, 0 /*bufsize*/, cname);
}
#endif

RVAPI
RvRtpSession RVCALLCONV RvRtpOpenEx(
								  IN  RvNetAddress* pRtpAddress,
								  IN  RvUint32      ssrcPattern,
								  IN  RvUint32      ssrcMask,
								  IN  char *        cname)
{
    RvRtpSession res;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpOpenEx"));
    return RtpOpenFrom(NULL, pRtpAddress, ssrcPattern, ssrcMask,
                       NULL /*buffer*/, 0 /*bufsize*/, cname);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpOpenEx"));
    return res;
}

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
        IN  RvTransport     rtcpTransp)
{
    RvRtpSession s = NULL;
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpOpenEx2(transp=%p)", transp));
    s = RtpOpen(NULL, transp, ssrcPattern, ssrcMask, NULL /*buffer*/,
                0 /*bufsize*/, cname, rtcpTransp);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpOpenEx2(transp=%p)", transp));
    return s;
}

/************************************************************************************
 * RvRtpClose
 * description: Close RTP session.
 * input: hRTP - Handle of the RTP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpClose(
        IN  RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s            = (RvRtpSessionInfo *)hRTP;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpClose"));
    if (s)
    {
        if (s->hRTCP)
		{
			RvLogDebug(rvLogPtr, (rvLogPtr, "RvRtpClose(%#x):  closing RTCP session", s));
            RvRtcpClose(s->hRTCP);
			s->hRTCP = NULL;
		}

        RvLogDebug(rvLogPtr, (rvLogPtr, "RvRtpClose: removing the address list"));
        RvRtpRemoveAllRemoteAddresses(hRTP);
		RvRtpAddressListDestruct(&s->addressList);
        /* Send UDP data through specified socket to the local host */
        {
            RtpProfilePlugin* pluginPtr = s->profilePlugin;
			pluginPtr->funcs->release(pluginPtr, hRTP);  /* lock inside */
            s->profilePlugin = NULL;
        }
        RvLockGet(&s->lock, logMgr);
		if (s->encryptionKeyPlugInPtr!=NULL && s->encryptionKeyPlugInPtr->release!=NULL)
		{
		    RvLogDebug(rvLogPtr, (rvLogPtr, "RvRtpClose: releasing the encryption key plug-in"));
			s->encryptionKeyPlugInPtr->release(s->encryptionKeyPlugInPtr);

        }
        RvRtpResume(hRTP);
        RvLockRelease(&s->lock, logMgr);
        RTPLOG_DEBUG((rvLogPtr, "RvRtpClose(%#x): sent zero packet to exit from blocked socket", s));
#ifdef __H323_NAT_FW__
        if (NULL != s->demux)
        {
            RtpDemux* d = (RtpDemux*) s->demux;
            RvLockGet(&d->lock, logMgr);
            if (d->rtpSessionsCounter <= 0)
            {
                if (d->rtpEvHandler != NULL)
                {
                    d->rtpEvHandler    = NULL;
                }
                d->rtpContext   = NULL;
                RTPLOG_DEBUG((rvLogPtr, "RvRtpClose(%#x): closing the demux socket", d));
                /* This function closes the specified IP socket and all the socket's connections.*/
                RvTransportRelease(d->rtpTransport);
                d->rtpTransport = NULL;
            }
            RvLockRelease(&d->lock, logMgr);
        }
        else
#endif
        {
			rtpPortRangeFreeRtpPort(&rvRtpInstance.portRange, RvRtpGetPort(hRTP));

            RTPLOG_DEBUG((rvLogPtr, "RvRtpClose(%#x): closing the socket", s));
            /* This function closes the specified IP socket and all the socket's connections.*/
            RvTransportRelease(s->transport);
        }


        RvLockDestruct(&s->lock, logMgr);

printf("\nrtp close start:%#x\n",s);
        if (s->isAllocated)
		{
            memset(s, 0, sizeof(RvRtpSessionInfo));  /* knowingly to crash usage of invalid handle */
            RTPLOG_DEBUG((rvLogPtr, "RvRtpClose(%#x): releasing the memory ", s));
            RvMemoryFree(s, logMgr);
printf("\nrtp close free call\n");
		}
        else
            memset(s, 0, sizeof(RvRtpSessionInfo));  /* knowingly to crash usage of invalid handle */
        RTPLOG_INFO((rvLogPtr, "Closed RTP session (%#x)", s));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpClose"));
    return RV_OK;
}

/************************************************************************************
 * rtpGetSSRC
 * description: Returns the current SSRC (synchronization source value) of the RTP session.
 * input: hRTP - Handle of the RTP session.
 * output: none.
 * return value: the function returns the current SSRC value.
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpGetSSRC(
        IN  RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetSSRC"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetSSRC"));
    return s->sSrc;
}

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
        IN  RvRtpSession          hRTP,
        IN  RvRtpEventHandler_CB  eventHandler,
        IN  void *                context)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvTransportEventCb pfnTransporteventHandler;

    RvLogEnter(rvLogPtr, (rvLogPtr, "SetEventHandler"));
    if (s == NULL)
    {
        RTPLOG_ERROR_LEAVE(SetEventHandler, "NULL session handle");
        return;
    }

    s->eventHandler = eventHandler;
    s->context      = context;

    /* Update the Transport layer: register/unregister READ event callback */
    pfnTransporteventHandler = (eventHandler == NULL) ? NULL : rtpEvent;
    RtpSetRemoveEventHandler(s->transport, pfnTransporteventHandler, (void*)s);

    if (eventHandler != NULL)
    {
        RTPLOG_INFO((rvLogPtr, "Set RTP Event Handler for non-blocking read for session %#x", s));
    }
    else
    {
        RTPLOG_INFO((rvLogPtr, "Removed RTP Event Handler for non-blocking read for session %#x", s));
    }
    RvLogLeave(rvLogPtr, (rvLogPtr, "SetEventHandler"));
}

/************************************************************************************
 * rtpSetRemoteAddress
 * description: Defines the address of the remote peer or the address of a multicast
 *              group to which the RTP stream will be sent.
 * input: hRTP  - Handle of the RTP session.
 *        ip    - IP address to which RTP packets should be sent.
 *        port  - UDP port to which RTP packets should be sent.
 * output: none.
 * return value: none.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
void RVCALLCONV rtpSetRemoteAddress(
        IN RvRtpSession  hRTP,   /* RTP Session Opaque Handle */
        IN RvUint32     ip,
        IN RvUint16     port)
{
	RvNetAddress rtpAddress;
    RvNetIpv4 Ipv4;
    Ipv4.ip = ip;
    Ipv4.port = port;
    RvNetCreateIpv4(&rtpAddress, &Ipv4);
	RvRtpSetRemoteAddress(hRTP, &rtpAddress);
}
#endif
RVAPI
void RVCALLCONV RvRtpSetRemoteAddress(
									IN RvRtpSession  hRTP,   /* RTP Session Opaque Handle */
									IN RvNetAddress* pRtpAddress)

{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvAddress* pRvAddress =  NULL;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetRemoteAddress"));

	if (s != NULL && pRtpAddress!=NULL && RvNetGetAddressType(pRtpAddress)!= RVNET_ADDRESS_NONE)
    {
        if (s->remoteAddressSet)
        {
            RvRtpRemoveAllRemoteAddresses(hRTP);
        }
        RvLockGet(&s->lock, logMgr);
        pRvAddress = (RvAddress*) pRtpAddress->address;
        if (
            s->profilePlugin->funcs->addRemAddress != NULL)
        {
            s->profilePlugin->funcs->addRemAddress(s->profilePlugin, hRTP, RV_TRUE, pRtpAddress);
        }
		RvRtpAddressListAddAddress(&s->addressList, pRvAddress, NULL);
        s->remoteAddressSet = RV_TRUE;
        RvLockRelease(&s->lock, logMgr);
    }
    else {
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetRemoteAddress: NULL pointer or wrong address type"));
    }

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetRemoteAddress"));

}
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
	IN RvRtpSession  hRTP,   /* RTP Session Opaque Handle */
	IN RvNetAddress* pRtpAddress)

{
    RTPLOG_ENTER(AddRemoteAddress);
    RtpAddRemoteAddress(hRTP, pRtpAddress, NULL);
    RTPLOG_LEAVE(AddRemoteAddress);
}
/************************************************************************************
 * RvRtpRemoveRemoteAddress
 * description: removes the specified RTP address of the remote peer or the address
 *              of a multicast group or of multiunicast address list to which the
 *              RTP stream was sent.
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
	IN RvNetAddress* pRtpAddress)
{
    RvLogEnter(rvLogPtr,(rvLogPtr, "RvRtpRemoveRemoteAddress"));
    RtpRemoveRemoteAddress(hRTP, pRtpAddress, NULL);
    RvLogLeave(rvLogPtr,(rvLogPtr, "RvRtpRemoveRemoteAddress"));
    return RV_OK;
}
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
RvStatus RVCALLCONV RvRtpRemoveAllRemoteAddresses(IN RvRtpSession  hRTP)
{
    RvRtpSessionInfo* s          = (RvRtpSessionInfo *)hRTP;
    RtpNatAddress*     pNatAddress = NULL;
	RvUint32* pMultiplexID = NULL;
	RvStatus  res;

    RvLogEnter(rvLogPtr,(rvLogPtr, "RvRtpSetRemoteAddress"));
    if (s == NULL)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetRemoteAddress: NULL session pointer"));
		res = RV_ERROR_UNKNOWN;
	}
	else
	{
		RvLockGet(&s->lock, logMgr);

		while((pNatAddress = (RtpNatAddress*)RvRtpAddressListGetNext(&s->addressList, NULL))!= NULL)
		{
			if (
/**h.e
			s->profilePlugin != NULL &&
			s->profilePlugin->funcs != NULL &&
**/
				s->profilePlugin->funcs->removeRemAddress != NULL)
				s->profilePlugin->funcs->removeRemAddress(s->profilePlugin, hRTP, (RvNetAddress *) &pNatAddress->address);
#ifdef __H323_NAT_FW__
				if (pNatAddress->isMultiplexed)
				{
					pMultiplexID = &pNatAddress->multiplexID;
				}
#endif
				RvRtpAddressListRemoveAddress(&s->addressList, &pNatAddress->address, pMultiplexID);
			}

			s->remoteAddressSet = RV_FALSE;

			RvLockRelease(&s->lock, logMgr);
			RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtpSetRemoteAddress: all remote addresses are removed for the RTP session %#x", hRTP));
			res=RV_OK;
	}

	RvLogLeave(rvLogPtr,(rvLogPtr, "RvRtpSetRemoteAddress"));
	return res;
}


/************************************************************************************
 * RvRtpPackEx
 * description: This routine is the Extension of RvRtpPack.
 *				It does the same as RvRtpPack plus permits to use a SSRC,
 *				specified as a parameter instead of default session's ssrc
 * input: hRTP  - Handle of the RTP session.
 *		  ssrc  - the synchronization source to be used as SSRC of the outgoing packet
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
		IN  RvInt32	      ssrc,
        IN  void *        buf,
        IN  RvInt32       len,
        IN  RvRtpParam *  p)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvUint32 *header;
    RvUint32 seq;

    RTPLOG_ENTER(Pack);
    if (p->extensionBit && p->extensionLength>0 && p->extensionData==NULL)
    {

        RTPLOG_ERROR_LEAVE(Pack, "NULL RTP header extension data");
        return RV_ERROR_NULLPTR;
    }
    p->sByte-=(RvRtpNatMultiplexIdSize() + 12 +(p->extensionBit)*(4+(p->extensionLength<<2)));
    p->len=len - p->sByte;

    if (s->useSequenceNumber)
        s->sequenceNumber=p->sequenceNumber;
    p->sequenceNumber=s->sequenceNumber;
    seq = s->sequenceNumber;

    /* sets the fields inside RTP message.*/
    header=(RvUint32*)((char*)buf + p->sByte);
    header[0]=0;
    header[0]=bitfieldSet(header[0],2,30,2);                /* protocol version 2 */
    header[0]=bitfieldSet(header[0],p->paddingBit,29,1);	/* padding bit if exist */
    header[0]=bitfieldSet(header[0],p->extensionBit,28,1);	/* extension bit if exist */
    header[0]=bitfieldSet(header[0],p->marker,23,1);
    header[0]=bitfieldSet(header[0],p->payload,16,7);
    header[0]=bitfieldSet(header[0],seq,0,16);
    header[1]=p->timestamp;
    header[2]=ssrc;

    //add by zhouzx 2016/04/20
    header[2] = p->sSrc;

    /* increment the internal sequence number for this session */

    s->sequenceNumber++;
    if (s->useSequenceNumber)
    {
        /* nothing to do roc is updated inside SRTP module */
    }
    else
    {
        if (s->sequenceNumber == RvUint16Const(0)) /* added for SRTP support */
            s->roc++;
    }

    if (p->extensionBit)
    {
        header[3] = 0;
        header[3] = bitfieldSet(header[3], p->extensionLength,  0,16);
        header[3] = bitfieldSet(header[3], p->extensionProfile,16,16);
        if (p->extensionLength>0)
        {
            RvUint32 count = 0;
            for (count=0;count<p->extensionLength;count++)
            {
                header[4+count] = p->extensionData[count];
            }
        }
        ConvertToNetwork(header, 0, 4 + p->extensionLength);
    }
    else
    {
        /* converts an array of 4-byte integers from host format to network format.*/
        ConvertToNetwork(header, 0, 3);
    }
    RTPLOG_LEAVE(Pack);
    return RV_OK;
}


#ifdef RVRTP_OLD_CONVENTION_API
/************************************************************************************
 * RvRtpOLDPack
 * description: This routine sets the RTP header.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        p     - A struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpOLDPack(
        IN  RvRtpSession  hRTP,
        IN  void *       buf,
        IN  RvInt32      len,
        IN  RvRtpParam *   p)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvUint32 *header;
    RvUint32 seq;

    RTPLOG_ENTER(OLDPack);
    p->sByte-=12;
    p->len=len - p->sByte;

    if (s->useSequenceNumber)
        s->sequenceNumber=p->sequenceNumber;
    p->sequenceNumber=s->sequenceNumber;
    seq = s->sequenceNumber;

    /* sets the fields inside RTP message.*/
    header=(RvUint32*)((char*)buf + p->sByte);
    header[0]=0;
    header[0]=bitfieldSet(header[0],2,30,2);
    header[0]=bitfieldSet(header[0],p->paddingBit,29,1);	/* padding bit if exist */
    header[0]=bitfieldSet(header[0],p->marker,23,1);
    header[0]=bitfieldSet(header[0],p->payload,16,7);
    header[0]=bitfieldSet(header[0],seq,0,16);
    header[1]=p->timestamp;
    header[2]=s->sSrc;

    /* increment the internal sequence number for this session */

    s->sequenceNumber++;
    if (s->useSequenceNumber)
    {
        /* nothing to do roc is updated inside SRTP module */
    }
    else
    {
        if (s->sequenceNumber == RvUint16Const(0)) /* added for SRTP support */
            s->roc++;
    }
    /* converts an array of 4-byte integers from host format to network format.*/
    ConvertToNetwork(header, 0, 3);
    RTPLOG_LEAVE(OLDPack);
    return RV_OK;
}
#else

/************************************************************************************
 * RvRtpPack
 * description: This routine sets the RTP header.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        p     - A struct of RTP param.
 * output: none.
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpPack(
        IN  RvRtpSession  hRTP,
        IN  void *        buf,
        IN  RvInt32       len,
        IN  RvRtpParam *  p)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	return RvRtpPackEx(hRTP, s->sSrc, buf, len, p);
}
#endif

/************************************************************************************
 * RvRtpSetRtpSequenceNumber
 * description: This routine sets the RTP sequence number for packet that will be sent
 *              This is not documented function, used only for IOT.
 * input: hRTP  - Handle of the RTP session.
 *        sn    - sequence number to set
 * return value:  If no error occurs, the function returns 0.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSetRtpSequenceNumber(IN  RvRtpSession  hRTP, RvUint16 sn)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;

    RTPLOG_ENTER(SetRtpSequenceNumber);
    if (NULL==s)
    {
        RTPLOG_ERROR_LEAVE(SetRtpSequenceNumber, "NULL RTP session handle");
        return RV_ERROR_NULLPTR;
    }
    s->sequenceNumber = sn;
    RTPLOG_LEAVE(SetRtpSequenceNumber);
	return RV_OK;
}
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
RvUint32 RVCALLCONV RvRtpGetRtpSequenceNumber(IN  RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;

    RTPLOG_ENTER(GetRtpSequenceNumber);
    if (NULL==s)
    {
        RTPLOG_ERROR_LEAVE(GetRtpSequenceNumber, "NULL RTP session handle");
        return 0;
    }
    RTPLOG_LEAVE(GetRtpSequenceNumber);
	return s->sequenceNumber;
}


/************************************************************************************
 * RvRtpWriteEx
 * description: This routine is the Extension of RvRtpWrite.
 *				It does the same as RvRtpWrite plus permits to use a user
 *				specified SSRC as a parameter instead of default sessions's SSRC
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
		IN     RvInt32        ssrc,
        IN     void *         buf,
        IN     RvInt32        len,
        INOUT  RvRtpParam *   p)
{
    RvStatus res = RV_ERROR_UNKNOWN;
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;

	RvLogEnter(rvLogPtr,(rvLogPtr, "RvRtpWriteEx"));

	if (s == NULL)
	{
        RvLogError(rvLogPtr,(rvLogPtr, "RvRtpWriteEx: NULL session handle or session is not opened"));
        res = RV_ERROR_NULLPTR;
	}
	else
	{
		if (s->hRTCP != NULL)
		{
			if (rtcpSessionIsSessionInShutdown(s->hRTCP))
			{
				RvLogError(rvLogPtr,(rvLogPtr, "RvRtpWriteEx: Can not send. The session is in shutdown or RTCP BYE report was sent"));
				RvLogLeave(rvLogPtr,(rvLogPtr, "RvRtpWriteEx"));
				return RV_ERROR_ILLEGAL_ACTION;
			}
		}

		RvLockGet(&s->lock, logMgr);
		res = s->profilePlugin->funcs->writeXRTP(hRTP, ssrc, buf, len, p);

		RvLockRelease(&s->lock, logMgr);
		if ((s->hRTCP != NULL) && (res == RV_OK))
		{
			RvRtcpSessionSetParam(s->hRTCP, RVRTCP_PARAMS_RTP_PAYLOADTYPE, &p->payload);
			/* inform the RTCP session about a packet that was sent in the corresponding RTP session.*/
			res = RvRtcpRTPPacketSent(s->hRTCP, len - RvRtpGetHeaderLength(), p->timestamp);

		}
	}

	RvLogLeave(rvLogPtr,(rvLogPtr, "RvRtpWriteEx"));
    return res;
}

/************************************************************************************
 * RvRtpWrite
 * description:  This routine sends RTP packet.
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
        IN     void *         buf,
        IN     RvInt32        len,
        INOUT  RvRtpParam *   p)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	return RvRtpWriteEx(hRTP, s->sSrc, buf, len, p);

}
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
        IN  RvRtpSession  hRTP,
        IN  void *buf,
        IN  RvInt32 len,
        OUT RvRtpParam* p)
{
    RvUint32 *header=(RvUint32*)buf;

    RTPLOG_ENTER(Unpack);

    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(hRTP);

    if (p->len < 12)
    {
        RTPLOG_ERROR_LEAVE(Unpack, "This packet is probably corrupted.");
        return RV_ERROR_UNKNOWN;
    }

	ConvertFromNetwork(buf, 0, 3);
    ConvertFromNetwork(buf, 3, (RvInt32)bitfieldGet(header[0], 24, 4));
	if (bitfieldGet(header[0],30,2)!=2)
    {
        RTPLOG_ERROR_LEAVE(Unpack, "Incorrect RTP version number.");
        return RV_ERROR_UNKNOWN;
    }
    p->timestamp=header[1];
    p->sequenceNumber=(RvUint16)bitfieldGet(header[0],0,16);
    p->sSrc=header[2];
    p->marker=bitfieldGet(header[0],23,1);
    p->payload=(unsigned char)bitfieldGet(header[0],16,7);
    p->extensionBit = bitfieldGet(header[0],28,1);
    p->sByte=12+bitfieldGet(header[0],24,4)*sizeof(RvUint32);

    if (p->extensionBit)/*Extension Bit Set*/
    {
        RvInt32 xStart=p->sByte / sizeof(RvUint32);

        ConvertFromNetwork(buf, xStart, 1);
        p->extensionProfile = (RvUint16) bitfieldGet(header[xStart], 16, 16);
        p->extensionLength  = (RvUint16) bitfieldGet(header[xStart],  0, 16);
        p->sByte += ((p->extensionLength+1)*sizeof(RvUint32));

        if (p->sByte > p->len)
        {
            /* This packet is probably corrupted */
            p->sByte = 12;
            RTPLOG_ERROR_LEAVE(Unpack, "This packet is probably corrupted.");
            return RV_ERROR_UNKNOWN;
        }
        if (p->extensionLength>0)
        {
            p->extensionData    =  &header[xStart+1];
            /* Leave the rest of extension header as is (as we got it)
			 ConvertFromNetwork(buf, xStart+1, p->extensionLength);
			*/
        }
        else
        {
            p->extensionData    = NULL;
        }

		/* Restore 1st 32 bit word of extension header to network order */
        ConvertToNetwork(buf, xStart, 1);
    }

    if (bitfieldGet(header[0],29,1))/*Padding Bit Set*/
    {
        p->len-=((char*)buf)[p->len-1];
		if (p->len < p->sByte)
        {
            RTPLOG_ERROR_LEAVE(Unpack, "This packet is probably corrupted.");
            return RV_ERROR_UNKNOWN;
        }
    }

	/* Restore header to initial network order*/
    ConvertToNetwork(buf, 3, (RvInt32)bitfieldGet(header[0], 24, 4));
	ConvertToNetwork(buf, 0, 3);

	RTPLOG_LEAVE(Unpack);
    return RV_OK;
}

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
		OUT RvNetAddress* remAddressPtr)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvStatus res = RV_ERROR_UNKNOWN;

	RvLogEnter(rvLogPtr,(rvLogPtr, "RvRtpReadWithRemoteAddress"));
    if (s==NULL || s->profilePlugin == NULL || remAddressPtr == NULL)
    {
		RvLogEnter(rvLogPtr,(rvLogPtr, "RvRtpReadWithRemoteAddress: RTP session is not opened"));
    }
	else
	{
		res = s->profilePlugin->funcs->readXRTP(s->profilePlugin, hRTP, buf, len, RV_TRUE, p, (RvNetAddress*)remAddressPtr);
	}

	RvLogLeave(rvLogPtr,(rvLogPtr, "RvRtpReadWithRemoteAddress"));
    return res;
}

/*#define __STUN_DEBUGGING_CHECK__*/
/************************************************************************************
 * RvRtpRead
 * description: This routine sets the header of the RTP message.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *
 * output: p    - A struct of RTP param,contain the fields of RTP header.
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpRead(
							 IN  RvRtpSession  hRTP,
							 IN  void *buf,
							 IN  RvInt32 len,
							 OUT RvRtpParam* p)
{
    RvNetAddress  remAddress;
    RvInt32 res = 0;
    RTPLOG_ENTER(Read);

#if (defined(__RTP_OVER_STUN__) && defined(__STUN_DEBUGGING_CHECK__))
    {
        RvSize_t messageLen = 0;
        res = RvRtpReadRawData(hRTP, buf, len, &messageLen, &remAddress);
        if (res != RV_OK)
        {
            if (res == RTP_DESTINATION_UNREACHABLE)
            {
                RTPLOG_LEAVE(Read);
            }
            else
            {
                RTPLOG_ERROR_LEAVE(Read, "RvRtpReadRawData() failed");
            }
            return res;
        }
        p->len = messageLen;
        res = RvRtpParseRawReadData(hRTP, buf, messageLen, &remAddress, p);
        if (res != RV_OK)
        {
            RTPLOG_ERROR_LEAVE(Read, "RvRtpParseRawReadData() failed");
            return res;
        }
    }
#else
    res = RvRtpReadWithRemoteAddress(hRTP, buf, len, p, &remAddress);
#endif
    RTPLOG_LEAVE(Read);
    return res;
}

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
 * return value: If no error occurs, the function returns the non-neagtive value.
 *               Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpReadEx(
        IN  RvRtpSession  hRTP,
        IN  void *       buf,
        IN  RvInt32      len,
        IN  RvUint32     timestamp,
        OUT RvRtpParam *   p)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvInt32 retVal;

    RTPLOG_ENTER(ReadEx);
    retVal = RvRtpRead(hRTP, buf, len, p);

    if (s->hRTCP  &&  retVal >= 0)
    {
    /* Informs the RTCP session about a packet that was received in the corresponding
        RTP session.*/
        RTPLOG_DEBUG((rvLogPtr, "RvRtpReadEx: informing RTCP session about received RTP packet"));
        RvRtcpRTPPacketRecv(s->hRTCP, p->sSrc, timestamp,  p->timestamp, p->sequenceNumber);
    }
    RTPLOG_LEAVE(ReadEx);
    return retVal;
}


/************************************************************************************
 * RvRtpReadFrom: shen 2014-10-23
************************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpReadFrom(
                                 IN  RvRtpSession  hRTP,
				 IN  void *buf,
				 IN  RvInt32 len,
				 OUT RvRtpParam* p,
				 OUT RvNetAddress *remAddress)
{
    RvInt32 res = 0;
    RTPLOG_ENTER(Read);

#if (defined(__RTP_OVER_STUN__) && defined(__STUN_DEBUGGING_CHECK__))
    {
	RvSize_t messageLen = 0;
	res = RvRtpReadRawData(hRTP, buf, len, &messageLen, remAddress);
	if (res != RV_OK)
	{
	    if (res == RTP_DESTINATION_UNREACHABLE)
	    {
		RTPLOG_LEAVE(Read);
    	    }
	    else
	    {
		RTPLOG_ERROR_LEAVE(Read, "RvRtpReadRawData() failed");
	    }
	    return res;
	}
	p->len = messageLen;
	res = RvRtpParseRawReadData(hRTP, buf, messageLen, remAddress, p);
	if (res != RV_OK)
	{
	    RTPLOG_ERROR_LEAVE(Read, "RvRtpParseRawReadData() failed");
	    return res;
	}
    }
#else
    res = RvRtpReadWithRemoteAddress(hRTP, buf, len, p, remAddress);
#endif
    RTPLOG_LEAVE(Read);
    return res;
}


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
        IN RvRtpSession  hRTP)   /* RTP Session Opaque Handle */
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvUint16 sockPort = 0;

    RTPLOG_ENTER(GetPort);
    RvTransportGetOption(s->transport,
        RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_LOCALPORT,
        (void*)&sockPort);
    RTPLOG_LEAVE(GetPort);
    return sockPort;
}

/************************************************************************************
 * RvRtpGetVersion
 * description:  Returns the RTP version of the installed RTP Stack.
 * input:  none.
 * output: none.
 * return value: If no error occurs, the function returns the current version value.
 *               Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
char * RVCALLCONV RvRtpGetVersion(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetVersion"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetVersion"));
    return (char*)VERSION_STR;
}

/************************************************************************************
 * RvRtpGetVersionNum
 * description:  Returns the RTP version of the installed RTP Stack.
 * input:  none.
 * output: none.
 * return value: If no error occurs, the function returns the current version value.
 *               Otherwise, it returns a negative value.
 ***********************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpGetVersionNum(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetVersionNum"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetVersionNum"));
    return VERSION_NUM;
}


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
        IN  RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetRTCPSession"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetRTCPSession"));
    return (s->hRTCP);
}

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
        IN  RvRtcpSession  hRTCP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetRTCPSession"));
    s->hRTCP = hRTCP;
    rtcpSetRtpSession(hRTCP, hRTP);
    RvRtcpSetProfilePlugin(s->hRTCP, s->profilePlugin);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetRTCPSession"));
    return RV_OK;
}

/************************************************************************************
 * RvRtpGetHeaderLength
 * description:  return the header of RTP message.
 * input:  none.
 * output: none.
 * return value:The return value is twelve.
 * NOTE:  in the FUTURE: RvRtpGetHeaderLength p must be adjusted to number of csrcs
 *        and  will be with the following semantics: RvRtpGetHeaderLength(IN RvRtpParam*p)
 ***********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpGetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetHeaderLength"));
    return 12;
}

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
        IN  RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvRandom randomValue;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpRegenSSRC"));
    /* those line is to prevent collision.*/
    RvRandomGeneratorGetValue(&rvRtpInstance.randomGenerator, &randomValue);
    s->sSrc = (RvUint32)randomValue * RvInt64ToRvUint32(RvInt64ShiftRight(RvTimestampGet(logMgr), 16));
    s->sSrc &= ~s->sSrcMask;
    s->sSrc |= s->sSrcPattern;
	RvLogInfo(rvLogPtr, (rvLogPtr, "RvRtpRegenSSRC: SSRC=%x", s->sSrc));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpRegenSSRC"));
    return s->sSrc;
}

/************************************************************************************
 * rtpSetGroupAddress
 * description:  Defines a multicast IP for the RTP session.
 * input:  hRTP  - Handle of RTP session.
 *         ip    - Multicast IP address for the RTP session.
 * output: none.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
#ifdef RVRTP_OLD_CONVENTION_API
RVAPI
RvInt32 RVCALLCONV rtpSetGroupAddress(
        IN RvRtpSession hRTP,    /* RTP Session Opaque Handle */
        IN RvUint32     ip)
{
	RvNetAddress rtpAddress;
    RvNetIpv4   Ipv4;
    Ipv4.ip = ip;
    Ipv4.port = 0;
    RvNetCreateIpv4(&rtpAddress, &Ipv4);
	return	RvRtpSetGroupAddress(hRTP, &rtpAddress);
}
#endif
RVAPI
RvInt32 RVCALLCONV RvRtpSetGroupAddress(
									  IN RvRtpSession hRTP,    /* RTP Session Opaque Handle */
									  IN RvNetAddress* pRtpAddress)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvAddress   mcastAddr, mcastInterface;
    RvAddress*  addresses[2];
    RvStatus    res, res1, res2;
    RvNetIpv4   Ipv4;
	RvInt32     addressType = RV_ADDRESS_TYPE_NONE;
    RvUint16    port;
    RvBool      trueVal;
	RTPLOG_ENTER(SetGroupAddress);

    if (s->transport == NULL ||
        strcmp(RvTransportGetType(s->transport),RV_SOCKET_TRANSPORT_STR) != 0)
    {
        RvLogError(rvLogPtr, (rvLogPtr,
            "RvRtpSetGroupAddress: the %p transport doesn't support multicasting",
            s->transport));
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (NULL==pRtpAddress||
		RV_ADDRESS_TYPE_NONE == (addressType = RvAddressGetType(&rvRtpInstance.rvLocalAddress)))
	{
		RTPLOG_ERROR_LEAVE(SetGroupAddress, "pRtpAddress==NULL or local address is undefined");
        return RV_ERROR_BADPARAM;
	}
	if (RvNetGetAddressType(pRtpAddress) == RVNET_ADDRESS_IPV6)
	{
#if (RV_NET_TYPE & RV_NET_IPV6)
		RvAddressConstructIpv6(&mcastAddr,
			RvAddressIpv6GetIp(RvAddressGetIpv6((RvAddress*)pRtpAddress->address)),
			RV_ADDRESS_IPV6_ANYPORT/*RvAddressGetIpPort((RvAddress*)pRtpAddress->address*/,
			RvAddressGetIpv6Scope((RvAddress*)pRtpAddress->address));
#else
		RTPLOG_ERROR_LEAVE(SetGroupAddress, "IPV6 is not supported in current configuration");
		return RV_ERROR_BADPARAM;
#endif
	}
	else
	{
		RvNetGetIpv4(&Ipv4, pRtpAddress);
		RvAddressConstructIpv4(&mcastAddr, Ipv4.ip, RV_ADDRESS_IPV4_ANYPORT);
	}
    if (!RvAddressIsMulticastIp(&mcastAddr))
    {
		RTPLOG_ERROR_LEAVE(SetGroupAddress, "Non-multicast IP");
		return RV_ERROR_BADPARAM;
    }
    RvAddressCopy(&rvRtpInstance.rvLocalAddress, &mcastInterface);
    res = RvTransportGetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_LOCALPORT, (void*)&port);
    if (res != RV_OK)
    {
        RTPLOG_ERROR_LEAVE(RvRtpSetGroupAddress, "Failed to get local port");
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
        RTPLOG_INFO((rvLogPtr, "Successed to set multicast group address to %s for session (%#x)",
            RvAddressGetString(&mcastAddr, sizeof(addressStr), addressStr), s));
    }
    else
    {
		RTPLOG_ERROR((rvLogPtr, "RvRtpSetGroupAddress - Failed to set multicast group address for session (%#x)", s));
    }
    RvAddressDestruct(&mcastAddr);
    RvAddressDestruct(&mcastInterface);
	RV_UNUSED_ARG(addressType);
	RTPLOG_LEAVE(SetGroupAddress);
    return res;
}
/************************************************************************************
 * RvRtpSetMulticastTTL
 * description:  Defines a multicast Time To Live (TTL) for the RTP session.
 * input:  hRTP  - Handle of RTP session.
 *         ttl   - [0..255] ttl threshold for multicast
 * output: none.
 * return value:  If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 * Note: the function is supported with IP stack that has full multicast support
 ***********************************************************************************/

RVAPI
RvInt32 RVCALLCONV RvRtpSetMulticastTTL(
		IN  RvRtpSession  hRTP,
		IN  RvUint8       ttl)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvStatus res = RV_OK;

	RTPLOG_ENTER(SetMulticastTTL);
    if (NULL==s)
	{
		RTPLOG_ERROR_LEAVE(SetMulticastTTL, "NULL session handle");
		return RV_ERROR_BADPARAM;
	}

    if (s->transport == NULL ||
        strcmp(RvTransportGetType(s->transport),RV_SOCKET_TRANSPORT_STR) != 0)
    {
        RvLogError(rvLogPtr, (rvLogPtr,
            "RvRtpSetMulticastTTL: the %p transport doesn't support multicasting",
            s->transport));
        return RV_ERROR_ILLEGAL_ACTION;
    }

    res = RvTransportSetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_MULTICASTTTL, (void*)&ttl);
    if (res==RV_OK)
    {
        RTPLOG_INFO((rvLogPtr, "Multicast TTL set to %d for session (%#x)", (RvInt32)ttl, s));
    }
    else
    {
        RTPLOG_ERROR((rvLogPtr, "RvRtpSetMulticastTTL - Failed to set multicast TTL for session (%#x)", s));
    }
	RTPLOG_LEAVE(SetMulticastTTL);
	return res;
}
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
        IN  RvRtpSession hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvAddress localAddress;

    RvStatus status;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpResume"));

    if (s->transport == NULL ||
        strcmp(RvTransportGetType(s->transport),RV_SOCKET_TRANSPORT_STR) != 0)
    {
        if (s->transport!= NULL)
        {
            RvLogWarning(rvLogPtr, (rvLogPtr,
                "RvRtpResume: the %p transport(%s) doesn't support resume",
                s->transport, RvTransportGetType(s->transport)));
            return RV_OK;
        }
        else
        {
            RvLogError(rvLogPtr, (rvLogPtr,
                "RvRtpResume: the session %p has NULL transport pointer",
                s, RvTransportGetType(s->transport)));
            return RV_ERROR_ILLEGAL_ACTION;
        }

    }
    status = RvTransportGetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_LOCALADDR,
                &localAddress);

    if (status == RV_OK)
    {
        RvChar*   buffDummy = "";
        RvSize_t  buffLen = 1;
#if 0
		if (!isMyIP(&localAddress))
		{
			RvUint16  Port  = RvAddressGetIpPort(&localAddress);
			RvInt32   type  = RvAddressGetType(&localAddress);
			RvUint32  Count = 0;
			for (Count = 0; Count < rvRtpInstance.addressesNum; Count++)
				if (type == RvAddressGetType(&rvRtpInstance.hostIPs[Count]))
				{
					/*			RvAddressCopy(&rvRtpInstance.hostIPs[0], &localAddress);*/
					RvAddressCopy(&rvRtpInstance.hostIPs[Count], &localAddress);
					RvAddressSetIpPort(&localAddress, Port);
					break;
				}
		}
#endif
        /* We send a dummy buffer to get this connection released from its blocking mode */
        status = RvTransportSendBuffer(s->transport, (RvUint8*)buffDummy,
                    buffLen, &localAddress, 0/*options*/, NULL/*pSent*/);
        RvAddressDestruct(&localAddress);
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpResume"));
    return status;
}

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
                IN RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpUseSequenceNumber"));
    s->useSequenceNumber = 1;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpUseSequenceNumber"));
    return RV_OK;
}

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
        IN RvInt32 size)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvInt32  bufSizes[2] = {-1/*send buf*/, -1/*recv buf*/};
    RvStatus retv;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpSetReceiveBufferSize: %d", size));
    bufSizes[1] = size;
    retv = RvTransportSetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_BUFSIZE, (void*)&bufSizes);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RtpSetReceiveBufferSize: %d", size));
    return retv;
}

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
                IN RvInt32 size)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvInt32  bufSizes[2] = {-1/*send buf*/, -1/*recv buf*/};
    RvStatus retv;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetTransmitBufferSize: %d", size));
    bufSizes[0] = size;
    retv = RvTransportSetOption(s->transport,
            RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
            RVTRANSPORT_OPT_SOCK_BUFSIZE, (void*)&bufSizes);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetTransmitBufferSize: %d", size));
    return retv;
}

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
                IN RvInt32 size)
{
  return RvRtpSetTransmitBufferSize(hRTP, size);
}


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
                IN RvRtpSession  hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvInt32  bytes = -1;
    RvStatus ret;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetAvailableBytes"));

    ret = RvTransportGetOption(s->transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_BYTESAVAILABLE, (void*)&bytes);
    if (ret != RV_OK)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpGetAvailableBytes"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetAvailableBytes"));
        return ret;
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetAvailableBytes: %d", bytes));
    return bytes;
}


/* Obsolete function. Use RvRtpSessionGetSocket() instead of it */
RVAPI
RvInt32 RVCALLCONV RvRtpGetSocket(IN RvRtpSession hRTP)
{
    RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvSocket sock = (RvSocket)RV_INVALID_SOCKET;
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#if defined(_WIN64)
	return -1; /* not supported */
#endif
#endif
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetSocket"));
	if (NULL != s)
	{
        RvTransportGetOption(s->transport,
            RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_SOCKET,
            (void*)&sock);
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetSocket"));
		return (RvInt32)sock;
	}

	RvLogError(rvLogPtr, (rvLogPtr, "RvRtpGetSocket: NULL pointer or RTP session is not opened"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetSocket"));
	return -1;
}

#ifdef RVRTP_SECURITY
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
	IN RvRtpEncryptionKeyPlugIn* keyPlugInPtr)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetEncryption"));
	if (NULL == s || NULL == enctyptorPlugInPtr || NULL == keyPlugInPtr)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetEncryption: NULL pointer"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetEncryption"));
		return -1;
	}
	s->encryptionMode = mode;
	RvLogDebug(rvLogPtr, (rvLogPtr, "RvRtpSetEncryption: encryption plug-ins setting"));
	s->encryptionKeyPlugInPtr  = (RvRtpEncryptionKeyPlugIn*) keyPlugInPtr;
	s->encryptionPlugInPtr     = (RvRtpEncryptionPlugIn*)    enctyptorPlugInPtr;
	if (s->hRTCP!=NULL)
		RvRtcpSetEncryption(s->hRTCP, mode, enctyptorPlugInPtr, keyPlugInPtr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetEncryption"));
	return RV_OK;
}
#endif
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
#include "rvrtpdoublekey.h"
RVAPI
RvStatus RVCALLCONV RvRtpSetDoubleKeyEncryption(
	IN RvRtpSession hRtp,
	IN RvRtpEncryptionMode mode,
	IN RvKey *pEKey,
	IN RvKey *pDKey,
	IN RvRtpEncryptionPlugIn *pCrypto)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRtp;
	RvRtpEncryptionKeyPlugIn* keypluginPtr = RvRtpDoubleKeyCreate(pEKey, pDKey);

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetDoubleKeyEncryption"));
	if (NULL == s || NULL == pCrypto|| NULL == keypluginPtr)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetDoubleKeyEncryption: NULL pointer"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetDoubleKeyEncryption"));
		return -1;
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetDoubleKeyEncryption"));
	return RvRtpSetEncryption(hRtp, mode, pCrypto, keypluginPtr);
}
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
RvStatus RVCALLCONV RvRtpSetEncryptionNone(IN RvRtpSession hRTP)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetEncryptionNone"));
	if (NULL == s)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetEncryptionNone: NULL pointer"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetEncryptionNone"));
		return -1;
	}
	s->encryptionPlugInPtr = NULL;
	s->encryptionKeyPlugInPtr = NULL;
	if (s->hRTCP!=NULL)
		RvRtcpSetEncryptionNone(s->hRTCP);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetEncryptionNone"));
	return RV_OK;
}
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
RvStatus RVCALLCONV RvRtpSessionSetEncryptionMode(IN RvRtpSession hRTP, IN RvRtpEncryptionMode mode)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSessionSetEncryptionMode"));
	if (NULL == s)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSessionSetEncryptionMode: NULL pointer"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSessionSetEncryptionMode"));
		return -1;
	}
	s->encryptionMode = mode;
	if (s->hRTCP!=NULL)
		RvRtcpSessionSetEncryptionMode(s->hRTCP, mode);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSessionSetEncryptionMode"));
	return RV_OK;
}

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
 ********************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSetTypeOfService(
        IN RvRtpSession hRTP,
        IN RvInt32        typeOfService)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvStatus result = RV_OK;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetTypeOfService: %d", typeOfService));
	if (NULL == s)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetTypeOfService: NULL pointer or socket is not allocated"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetTypeOfService"));
		return RV_ERROR_NULLPTR;
	}
    result = RvTransportSetOption(s->transport,
                    RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                    RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE, (void*)&typeOfService);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetTypeOfService: %d(res=%d)", typeOfService, result));
	return result;
}
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
        OUT RvInt32*     typeOfServicePtr)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvStatus result = RV_OK;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetTypeOfService"));
	if (NULL == s)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpGetTypeOfService: NULL pointer or socket is not allocated"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetTypeOfService"));
		return RV_ERROR_NULLPTR;
	}
    result = RvTransportGetOption(s->transport,
                    RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                    RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE, (void*)typeOfServicePtr);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetTypeOfService: %d(res=%d)",
                                    *typeOfServicePtr, result));
	return result;
}

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
				IN RvUint8 reasonLength)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvStatus result = RV_OK;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSessionShutdown"));
	if (NULL == s || NULL == s->hRTCP)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSessionShutdown: NULL session handle"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSessionShutdown"));
		return RV_ERROR_NULLPTR;
	}
	result = RvRtcpSessionShutdown(s->hRTCP, reason, reasonLength);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSessionShutdown"));
	return result;
}


/*
   To perform a "KeepAlive" message the application sends a RTP packet
   with an unknown payload type.
   Normally the peer will ignore it, as RFC 3550 states that "a receiver
   MUST ignore packets with payload types that it does not understand".
*/

RVAPI
RvStatus RVCALLCONV RvRtpSessionSendKeepAlive(
				IN RvRtpSession  hRTP,
				IN RvUint32 unknownPayload)
{
    RvRtpParam p;
	RvUint8	   buf[128];

    /* Initializing RvRtpParam structure */
    RvRtpParamConstruct(&p);

	p.payload=(RvUint8)unknownPayload;

    /* Getting the RTP header size with payload header size (0 - in case of g.711) */
    p.sByte = RvRtpGetHeaderLength();

    /* RvRtpWrite sends the RTP packet without payload */
    return RvRtpWrite(hRTP, (void *)buf, p.sByte, &p);

}


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
		IN RvBool       bIsBlocking)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
	RvStatus result;
    RvBool bNonblockingMode = (bIsBlocking==RV_TRUE)?RV_FALSE:RV_TRUE;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetSocketBlockingMode: %s",
                                    (bIsBlocking?"RV_TRUE":"RV_FALSE")));
	if (NULL == s)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetSocketBlockingMode: NULL session handle"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetSocketBlockingMode"));
		return RV_ERROR_NULLPTR;
    }
    result = RvTransportSetOption(s->transport,
                    RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                    RVTRANSPORT_OPT_SOCK_NONBLOCKING, (void*)&bNonblockingMode);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetSocketBlockingMode: %s(res=%d)",
                          bIsBlocking?"RV_TRUE":"RV_FALSE", result));
    return result;
}


/********************************************************************************************
* RvRtpSessionGetSocket
* Description: Gets a socket used fot this session
* INPUT   : hRTP           - RTP session handle
* RETURN  : socket
*********************************************************************************************/
RVAPI
RtpSocket RVCALLCONV RvRtpSessionGetSocket(IN RvRtpSession hRTP)
{
	RvRtpSessionInfo *s = (RvRtpSessionInfo *)hRTP;
    RvSocket sock = (RvSocket)RV_INVALID_SOCKET;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSessionGetSocket"));
	if (s == NULL)
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSessionGetSocket: NULL session handle"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSessionGetSocket"));
		return (RtpSocket)0;
    }
    RvTransportGetOption(s->transport, RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                         RVTRANSPORT_OPT_SOCKET, (void*)&sock);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSessionGetSocket %d", sock));
	return (RtpSocket)sock;
}

RVAPI
RvStatus RVCALLCONV RvRtpSessionSetSocket(
						IN RvRtpSession hRTP,
						IN RtpSocket	*socket)
{
    RvStatus             status = RV_OK;
    RvRtpSessionInfo*    s = (RvRtpSessionInfo *)hRTP;

    if (s->transport != NULL)
    {
        status = RvTransportSetOption(s->transport,
                        RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                        RVTRANSPORT_OPT_SOCKET, (void*)socket);
    }
    return status;
}


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
			IN RvAddress	*address)
{
	RvStatus         res;

	res = initiateSocket((RvSocket	*)socket, address);
	if (res==RV_OK)
		RvRtpSessionSetSocket(hRTP, socket);
	else
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSessionSetSocketEx: cannot initiate socket"));
    }

    return res;
}


RVAPI
RvBool   RVCALLCONV  isMyIP(RvAddress* address)
{
    RvUint32 i;
    RvBool Result = RV_FALSE;

    for (i = 0; (i < rvRtpInstance.addressesNum); i++)
    {
        if (RvAddressCompare(&rvRtpInstance.hostIPs[i], address, RV_ADDRESS_BASEADDRESS))
        {
            Result = RV_TRUE;
            break;
        }
    }

    return Result;
}
/* == Internal RTP Functions == */

#undef FUNC_NAME
#define FUNC_NAME(name) "rtp" #name

static
void rtpEvent(
    RvTransport       transport,
    RvTransportEvents ev,
    RvBool            error,
    void*             usrData)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo*)usrData;

    RTPLOG_ENTER(Event);

    RV_UNUSED_ARG(transport);
    RV_UNUSED_ARG(ev);
    RV_UNUSED_ARG(error);

    if (s->eventHandler)
    {
        s->eventHandler((RvRtpSession)s, s->context);
    }
    RTPLOG_LEAVE(Event);
}

/* == ENDS: Internal RTP Functions == */




#ifdef __cplusplus
}
#endif
























