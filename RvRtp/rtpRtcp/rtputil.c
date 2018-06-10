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

#include "rvtypes.h"
#include "rvlock.h"
#include "rvmemory.h"
#include "rtputil.h"
#include "RtpProfileRfc3550.h"
#include "RtcpTypes.h" /* only for rtcpDemuxOpen */

#ifdef __H323_NAT_FW__
#include "RtpDemux.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define rvLogPtr        (rtpGetSource(RVRTP_RTP_MODULE))
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"

#define ALIGNMENT                 0x10

extern RvRtpInstance        rvRtpInstance;

/* local functions */
RvBool rtpPortRangeIsSet(RvRtpPortRange *portRange);
void RvRtpInitSocketTransportCfg(RvTransportSocketCfg *pCfg);


RVAPI
void   RVCALLCONV ConvertToNetwork(void *data, RvInt32 pos, RvInt32 n)
{
    RvInt32 i;
    for (i = pos; i < pos + n; ++i)
      ((RvUint32*)data)[i] = RvConvertHostToNetwork32(((RvUint32*)data)[i]);
}

RVAPI
void   RVCALLCONV ConvertFromNetwork(void *data, RvInt32 pos, RvInt32 n)
{
    RvInt32 i;
    for (i = pos; i < pos + n; ++i)
      ((RvUint32*)data)[i] = RvConvertNetworkToHost32(((RvUint32*)data)[i]);
}

RVAPI
void   RVCALLCONV ConvertToNetwork2(void *data, RvInt32 pos, RvInt32 n)
{
    RvInt32 i;
    for (i = pos; i < pos + n; ++i)
        ((RvUint16*)data)[i] = RvConvertHostToNetwork16(((RvUint16*)data)[i]);
}

RVAPI
void   RVCALLCONV ConvertFromNetwork2(void *data, RvInt32 pos, RvInt32 n)
{
    RvInt32 i;
    for (i = pos; i < pos + n; ++i)
        ((RvUint16*)data)[i] = RvConvertNetworkToHost16(((RvUint16*)data)[i]);
}

#undef FUNC_NAME
#define FUNC_NAME(name) "Rtp" #name

/*******************************************************************************
 * RtpSendPacket
 * scope: Private
 * description sends regular or multiplexed packet to the remote peer
 *
 * input:
 *    socketPtr      - pointer to socket
 *    buffer         - filled buffer
 *    p              - pointer to filled RvRtpParam structure
 *    natAddressPtr  - pointer to RtpNatAddress
 *    paddingSize    - size of padding
 * output: none
 * return value: Non-negative value on success
 *               Negative value on failure
 *******************************************************************************/
RVINTAPI
RvStatus  RVCALLCONV RtpSendPacket(
    IN RvTransport      transport, 
    IN RvUint8*         buffer,
    IN RvUint32         paddingSize,
    IN RvRtpParam*      p,
    IN RtpNatAddress*   natAddressPtr)
{
    RvStatus  res = RV_ERROR_UNKNOWN;
    RvUint8*  bufToSend = buffer+p->sByte;
    RvSize_t  lenToSend = (RvSize_t)p->len + paddingSize;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpSendPacket"));
    
#ifdef __H323_NAT_FW__
    {

        if (natAddressPtr->isMultiplexed)
        {
            RvUint32* multiplexID  = (RvUint32*)buffer;
            *multiplexID = natAddressPtr->multiplexID;
            ConvertToNetwork(buffer, 0, 1);
            res = RvTransportSendBuffer(transport, bufToSend, lenToSend,
                    &natAddressPtr->address, 0 /*options*/, NULL /*pSent*/);
        }
        else
        {
            bufToSend += RvRtpNatMultiplexIdSize();
            lenToSend -= RvRtpNatMultiplexIdSize();
            res = RvTransportSendBuffer(transport, bufToSend, lenToSend,
                    &natAddressPtr->address, 0 /*options*/, NULL /*pSent*/);
        }
    }
#else
    {
	    res = RvTransportSendBuffer(transport, bufToSend, lenToSend,
                &natAddressPtr->address, 0 /*options*/, NULL /*pSent*/);
    }
#endif
	RvLogLeave(rvLogPtr, (rvLogPtr, "RtpSendPacket"));
    return res;
}

/************************************************************************************
 * RtpOpenFrom
 * description: Opens an RTP session in the memory that the application allocated.
 *              If cname argument is not NULL, opens the associated RTCP session,
 *              while using the port next to the RTP port for the RTCP session socket.
 * scope:       private
 * input: demux       - demux object handle, if demultiplexing is supported, o/w NULL
 *        pRtpAddress - contains The UDP port number to be used for the RTP session.
 *        ssrcPattern - Synchronization source Pattern value for the RTP session.
 *        ssrcMask    - Synchronization source Mask value for the RTP session.
 *        buffer      - Application allocated buffer with a value no less than the
 *                      value returned by the function RvRtpGetAllocationSize().
 *                      Can be NULL. If NULL, the memory will be allocated
 *                      internally.
 *        bufferSize  - size of the buffer.
 *        cname       - The unique name representing the source of the RTP data.
 *                      Can be NULL.
 *                      If NULL, no associated RTCP session will be opened.
 * output: none.
 * return value: If no error occurs, the function returns the handle for the opened RTP
 *               session. Otherwise, it returns NULL.
 *Note:
 *	1) RtpOpenFrom opens an RTP session in the memory that the application!!! allocated.
 *     therefore RvRtpSessionShutdown should not be used.
 *  2) RtpOpenFrom opens one socket with the same port for receiving and for
 *     sending (no demultiplexing).
 *  3) if demultiplexing is supported first session will be opened with demux=NULL
 *     for other demultiplexed sessions association of RTP session with shared socket
 *     will be in function RvRtpDemuxOpenSession.
 ***********************************************************************************/
RVAPI
RvRtpSession RVCALLCONV RtpOpenFrom(
        IN  RvRtpDemux    demux,
        IN  RvNetAddress* pRtpAddress,
        IN  RvUint32      ssrcPattern,
        IN  RvUint32      ssrcMask,
        IN  void*         buffer,
        IN  RvInt32       bufferSize,
        IN  char *        cname)
{
    RvStatus     res = RV_OK;
    RvAddress    localAddress;
	RvUint16 port=0; 
	RvUint16 initPort=0;
    RvTransport  rtpTransport = NULL;

    RvTransport  rtcpTransport = NULL;
    RvBool       bCreateRtpTransport = RV_FALSE;
    RvBool       bCreateRtcpTransport = RV_FALSE;
	RvBool		 bWaitForTransport = RV_FALSE;
    RvRtpSession s;
    RvTransportSocketCfg cfg;
    
	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpOpenFrom"));

    /* Run some checks  */
    if (cname != NULL)
    {
        if (NULL == pRtpAddress || RvNetGetAddressType(pRtpAddress) == RVNET_ADDRESS_NONE)
        {
            RTPLOG_ERROR_LEAVE(RtpOpenFrom, "NULL pointer or wrong address type.");
            return NULL;
        }
    }

	/* If an application has the Transport for RTP - get it */
	if (rvRtpInstance.rvTransportGet != NULL)
	{
		rvRtpInstance.rvTransportGet(RTP_SESSION, &rtpTransport, &bWaitForTransport);
	}
	else  /* Find out if the transport (RTP/RTCP use Socket Transport by default) should/could be created.  */
    if (pRtpAddress != NULL && demux == NULL)  // shen 2014-05-04 add code '&& demux == NULL'
    {
        bCreateRtpTransport = RV_TRUE;
		RvRtpInitSocketTransportCfg(&cfg);


    /* Take in account the __H323_NAT_FW__ */
#ifdef __H323_NAT_FW__
		if (pRtpAddress != NULL && demux != NULL)
		{
			RtpDemux* d = (RtpDemux*)demux;
			RvLockGet(&d->lock, logMgr);
			if (d->rtpSessionsCounter <= 0)
			{
				bCreateRtpTransport = RV_TRUE;
				RvTransportInitSocketTransportCfg(&cfg);

				res = RvSelectGetThreadEngine(/*logMgr*/NULL, &d->rtcpSelectEngine);
				if ((res != RV_OK) || (d->rtcpSelectEngine == NULL))
					d->rtcpSelectEngine = rvRtcpInstance.selectEngine;

				cfg.pSelectEngine = d->rtcpSelectEngine;
			}
			else
			{
				rtpTransport = d->rtpTransport;
			}
			RvLockRelease(&d->lock, logMgr);
		}
#endif

		do 
		{
			if ((pRtpAddress != NULL) && (RvNetGetAddressType(pRtpAddress) != RVNET_ADDRESS_NONE))
			{
				initPort = RvAddressGetIpPort((RvAddress*)pRtpAddress->address);
				if ((initPort == 0) && rtpPortRangeIsSet(&rvRtpInstance.portRange))
				{
					port = rtpPortRangeGetRtpPort(&rvRtpInstance.portRange);
					if (port == 0)
					{
						RTPLOG_ERROR_LEAVE(OpenFrom," no ports to open session.");   
						return NULL;
					}
				}
				else 
					port=initPort;
			
				if (RvNetGetAddressType(pRtpAddress) == RVNET_ADDRESS_IPV6)
				{
#if (RV_NET_TYPE & RV_NET_IPV6)
					RvAddressConstructIpv6(&localAddress, 
						RvAddressIpv6GetIp(RvAddressGetIpv6((RvAddress*)pRtpAddress->address)),
						port,
						RvAddressGetIpv6Scope((RvAddress*)pRtpAddress->address));				
                    cfg.options |= RVTRANSPORT_CREATEOPT_IPV6SOCKET;
#else	
					RTPLOG_ERROR_LEAVE(OpenFrom,"IPV6 is not supported in current configuration.");  
					return NULL;
#endif
				}
				else
				{
					RvUint32 ip = RvAddressIpv4GetIp(RvAddressGetIpv4((RvAddress*)pRtpAddress->address));
					RvAddressConstructIpv4(&localAddress, ip, (RvUint16)port);
				}
			}


			/* 3. Create the transport */
			if (bCreateRtpTransport == RV_TRUE)
			{
				cfg.pLocalAddr = &localAddress;
				/* Create the transport */
				res = RvTransportCreateSocketTransport(&cfg, &rtpTransport);
				if (res == RV_OK)
				{
					RvInt32 bufSizes[2] = {8192/*send buf*/, 8192/*recv buf*/};
            
                    RvTransportAddRef(rtpTransport);
					/* Set size of Transport socket buffers */
					RvTransportSetOption(rtpTransport,
						RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
						RVTRANSPORT_OPT_SOCK_BUFSIZE, &bufSizes);
            
					RvLogInfo(rvLogPtr,(rvLogPtr,
						"RtpOpenFrom: transport=%p bound to port=%d",
						rtpTransport, port));

					/* Update demux with the created transport */
#ifdef __H323_NAT_FW__    
					if (NULL != demux)
					{
						RtpDemux* d = (RtpDemux*)demux;
						RvLockGet(&d->lock, logMgr);
						if (d->rtpSessionsCounter <= 0)
						{
							d->rtpTransport = rtpTransport;
						}
						RvLockRelease(&d->lock, logMgr);
					}
#endif

				}
				else
				{
					rtpPortRangeFreeRtpPort(&rvRtpInstance.portRange, port);
				}
			}

		/* We break the loop If there is no free ports in the range */	
        } while ((bCreateRtpTransport == RV_TRUE) && (res != RV_OK) && rtpPortRangeIsSet(&rvRtpInstance.portRange)
            && port >= rvRtpInstance.portRange.fromPort && port < rvRtpInstance.portRange.toPort); 

        if (res != RV_OK)
        {
            RvLogError(rvLogPtr,(rvLogPtr,
                "RtpOpenFrom: failed to open session on port=%d", port));
            return NULL;
        }
		if (pRtpAddress != NULL)
		{
			if (initPort==0)
			{
				RvAddressSetIpPort((RvAddress*)pRtpAddress->address, port);
			}
		}
	}

	/* If an application has the Transport for RTCP - get it */
	if (rvRtpInstance.rvTransportGet != NULL)
	{
		rvRtpInstance.rvTransportGet(RTCP_SESSION, &rtcpTransport, &bWaitForTransport);
	}
	else 
	{
		/* 4. Create RTCP transport */
		if (cname != NULL  &&  pRtpAddress != NULL)
		{
			RvUint16 rtcpPort;

			/* 1. Find out if the transport should be created */
			if (demux == NULL)
			{
				bCreateRtcpTransport = RV_TRUE;
			}

#ifdef __H323_NAT_FW__    
			if (demux != NULL)
			{
				RtpDemux* d = (RtpDemux*)demux;

				RvLockGet(&d->lock, logMgr);
				if ((pRtpAddress != NULL) && (d->rtcpSessionsCounter <= 0))
				{
					bCreateRtcpTransport = RV_TRUE;
				}
				else
				{
					rtcpTransport = d->rtcpTransport;
				}
				RvLockRelease(&d->lock, logMgr);
			}
#endif

			/* Create transport if need */
			if (bCreateRtcpTransport == RV_TRUE)
			{
				////////////////////////////////  shen 2014-05-04
				RvUint32 ip   = RvAddressIpv4GetIp(RvAddressGetIpv4((RvAddress*)pRtpAddress->address));
				RvUint16 port = RvAddressGetIpPort((RvAddress*)pRtpAddress->address);

				RvRtpInitSocketTransportCfg(&cfg);
				RvAddressConstructIpv4(&localAddress, ip, port);
				cfg.pLocalAddr = &localAddress;  
				////////////////////////////////

				rtcpPort = (RvUint16) (RvAddressGetIpPort(cfg.pLocalAddr) + 1);
				RvAddressSetIpPort(cfg.pLocalAddr, rtcpPort);
            
				if (cfg.pSelectEngine == NULL)
				{
					res = RvSelectGetThreadEngine(/*logMgr*/NULL, &cfg.pSelectEngine);
					if ((res != RV_OK) || (cfg.pSelectEngine == NULL))
						cfg.pSelectEngine = rvRtcpInstance.selectEngine;
				}

				/* Create the transport */
				res = RvTransportCreateSocketTransport(&cfg, &rtcpTransport);
				if (res == RV_OK)
				{
					RvInt32 bufSizes[2] = {8192/*send buf*/, 8192/*recv buf*/};

                    RvTransportAddRef(rtcpTransport);
					/* Set size of Transport socket buffers */
					RvTransportSetOption(rtcpTransport,
						RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
						RVTRANSPORT_OPT_SOCK_BUFSIZE, &bufSizes);
                
					RvLogInfo(rvLogPtr,(rvLogPtr,
						"RtpOpenFrom: RTCP transport=%p bound to port=%d",
						rtcpTransport, rtcpPort));

					/* Update demux with the created transport */
#ifdef __H323_NAT_FW__    
					if (NULL != demux)
					{
						RtpDemux* d = (RtpDemux*)demux;
						RvLockGet(&d->lock, logMgr);
						if (d->rtcpSessionsCounter <= 0)
						{
							d->rtcpTransport = rtcpTransport;
						}
						RvLockRelease(&d->lock, logMgr);
					}
#endif
				}
				else
				{
					RvLogError(rvLogPtr,(rvLogPtr,
						"RtpOpenFrom: failed to create RTCP transport bound to port=%d",
						rtcpPort));
					if (bCreateRtpTransport == RV_TRUE  &&  rtpTransport != NULL  &&  NULL == demux)
						RvTransportRelease(rtpTransport);
					return NULL;
				}
			}
		}
	}

    /* 5. Open the RTP session  */
    s = RtpOpen(demux, rtpTransport, ssrcPattern, ssrcMask, buffer, bufferSize,
                cname, rtcpTransport);
    if (s == NULL)
    {
        if (bCreateRtpTransport == RV_TRUE  &&  rtpTransport != NULL  &&  NULL == demux)
            RvTransportRelease(rtpTransport);
        if (bCreateRtcpTransport == RV_TRUE  &&  rtcpTransport != NULL  &&  NULL == demux)
            RvTransportRelease(rtcpTransport);
        RvLogError(rvLogPtr, (rvLogPtr,
            "RtpOpenFrom: failed to open session"));
        return NULL;
    }

    return s;
}
	

/************************************************************************************
 * RtpOpen
 * description: opens the RTP session, just like the RtpOpenFrom does, with one
 *              difference: the RtpOpen doesn't open socket, instead it uses
 *              the ready-to-send/recv transport object, provided by
 *              the application as a function argument.
 *              The application can get this transport from the ICE Stack, for
 *              example. Or it may implement it's own type of transport,
 *              using the abstract transport API defined in the rvtransport.h.
 *
 *              In addition, the RtpOpen function opens associated RTCP
 *              session, if the cname argument is not NULL.
 *
 * input: demux       - demux object handle, if demultiplexing is supported.
 *                      Can be NULL.
 *        transp      - the transport object to be used for sending/receiving.
 *        ssrcPattern - Synchronization source Pattern value for the RTP session
 *        ssrcMask    - Synchronization source Mask value for the RTP session.
 *        buffer      - Application allocated buffer with a value no less than
 *                      the value returned by RvRtpGetAllocationSize().
 *                      Can be NULL. If NULL, the memory for the session will be
 *                      allocated internally.
 *        bufferSize  - size of the buffer.
 *        cname       - The unique name representing the source of the RTP data.
 *                      Can be NULL.
 *                      If NULL, no associated RTCP session will be opened.
 *        rtcpTransp  - The transport to be used by the associated RTCP session.
 * output: none.
 * return value: If no error occurs, the function returns the handle
 *               of the opened RTP session. Otherwise, it returns NULL.
 *Note:
 *	1) RtpOpen opens an RTP session in the memory that the application!!! allocated.
 *     therefore RvRtpSessionShutdown should not be used.
 *  2) RtpOpen uses same transport for receiving and for sending
 *     (no demultiplexing).
 *  3) if demultiplexing is supported first session will be opened with
 *     demux=NULL. For other demultiplexed sessions association of RTP session
 *     with shared transport will be in function RvRtpDemuxOpenSession.
 ***********************************************************************************/
RVAPI
RvRtpSession RVCALLCONV RtpOpen(
                            IN  RvRtpDemux    demux,
                            IN  RvTransport   transp,
                            IN  RvUint32      ssrcPattern,
                            IN  RvUint32      ssrcMask,
                            IN  void*         buffer,
                            IN  RvInt32       bufferSize,
                            IN  char *        cname,
                            IN  RvTransport   rtcpTransp)
{
    RvRtpSessionInfo *s;
    RvRandom         randomValue;
    RvStatus         res = RV_OK;
    RtpProfileRfc3550* rtpProfilePtr = NULL;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpOpen"));

    /* Run some checks */
    if (buffer != NULL  &&  bufferSize < RvRtpGetAllocationSize())
	{
        RTPLOG_ERROR_LEAVE(OpenEx2," no room to open session, increase bufferSize");   
		return NULL;
	}
    if (cname != NULL  &&  rtcpTransp == NULL)
    {
        RTPLOG_ERROR_LEAVE(OpenEx2," RTCP transport was not provided, reset cname");   
        return NULL;
    }

    /* 1. Open RTP session */

    /* Allocate session if need */
    if (buffer != NULL)
    {
        s = (RvRtpSessionInfo *)buffer;
        memset(s, 0, (RvSize_t)bufferSize);
    }
    else
    {
        res = RvMemoryAlloc(NULL, (RvSize_t)RvRtpGetAllocationSize(), logMgr, (void**)&s);
        if (res != RV_OK)
        {
            RTPLOG_ERROR_LEAVE(OpenEx2, "cannot allocate RTP session");
            return NULL;
        }
        memset(s, 0, (RvSize_t)RvRtpGetAllocationSize());
        s->isAllocated = RV_TRUE;
    }

    /* Get a random value for the beginning sequence number */
    RvRandomGeneratorGetValue(&rvRtpInstance.randomGenerator, &randomValue);
    RTPLOG_DEBUG((rvLogPtr, "RtpOpenFrom - generated random value %#x", randomValue));   
    /* Encryption callbacks, key and e. mode must be set after opening of session
	s->encryptionPlugInPtr = NULL;
	s->encryptionKeyPlugInPtr = NULL;   NULL callbacks automatically by memset*/
	//s->isAllocated    = RV_TRUE; //mod l4x leak memory
    s->sSrcPattern    = ssrcPattern;
    s->sSrcMask       = ssrcMask;
    s->sequenceNumber = (RvUint16)randomValue;
    s->transport      = transp;

#ifdef __H323_NAT_FW__    
    if (NULL != demux)
    {
        RtpDemux* d = (RtpDemux*)demux;
        RvLockGet(&d->lock, logMgr);
        s->demux  = demux;
        RvLockRelease(&d->lock, logMgr);
    }
#endif

    res = RvLockConstruct(logMgr, &s->lock); 
    if (res == RV_OK)
    {
        RvRtpRegenSSRC((RvRtpSession)s);
	    RvRtpAddressListConstruct(&s->addressList);	
    }
    else
    {	
        RTPLOG_ERROR_LEAVE(OpenEx2, "failed");
        if (s->isAllocated == RV_TRUE)
            RvMemoryFree(s, logMgr);
        return (RvRtpSession)NULL;
    }

    rtpProfilePtr = (RtpProfileRfc3550*) (((RvUint8*)s) + RvRoundToSize(sizeof(RvRtpSessionInfo), RV_ALIGN_SIZE));
    RtpProfileRfc3550Construct(rtpProfilePtr);
    s->profilePlugin = RtpProfileRfc3550GetPlugIn(rtpProfilePtr);
    RTPLOG_INFO((rvLogPtr, "RTP session opened (handle = %p, SSRC=%#x)", s, s->sSrc));        


    /* 2. Open RTCP session */
    if (cname)
    {
        RvLogDebug(rvLogPtr, (rvLogPtr,
            "RtpOpen: RTP session %p, cname %s, rtcpTransprt=%p - open RTCP session",
            s, cname, rtcpTransp));

        s->hRTCP = rtcpDemuxOpenEx(demux, s->sSrc, rtcpTransp, cname,
                                   0 /*maxSessionMembers*/, NULL /*buffer*/,
                                   0 /*bufferSize*/);
        if (s->hRTCP == NULL)
        {
            RtpProfileRfc3550Destruct(rtpProfilePtr);
            if (s->isAllocated == RV_TRUE)
                RvMemoryFree(s, logMgr);
            RTPLOG_ERROR_LEAVE(OpenEx2,"rtcpDemuxOpenEx failed");
            return NULL;
        }
        else
        {
            RvLogDebug(rvLogPtr, (rvLogPtr,
                "RtpOpen: RTCP opened - %p (RTP %p)", s->hRTCP, s));

            ((rtcpSession*)s->hRTCP)->transport = rtcpTransp;

            rtcpSetRtpSession(s->hRTCP, (RvRtpSession)s);
            RvRtcpSetEncryptionNone(s->hRTCP);
            RvRtcpSessionSetEncryptionMode(s->hRTCP, RV_RTPENCRYPTIONMODE_RFC1889);
            RvRtcpSetProfilePlugin(s->hRTCP, s->profilePlugin);
        }
    }

    RvLogLeave(rvLogPtr, (rvLogPtr, "RtpOpen"));
    return (RvRtpSession)s;
}

/************************************************************************************
 * RtpSetRemoveEventHandler
 * description: sets or removes RTP event handler.
 * input: transport             - pointer to the Transport object.
 *        transportEventHandler - callback function for Read events.
 *                                If NULL - the callback will be removed,
 *                                the READ event registration will be canceled.
 *        ctx - context to be provided by the Transport layer while calling
 *              the event handler.
 * output: none.
 ***********************************************************************************/
void RtpSetRemoveEventHandler(
        IN RvTransport        transport,
        IN RvTransportEventCb transportEventHandler,
        IN void*              context)
{
    RvTransportCallbacks transportCallbacks;
    RvBool               boolVal;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpSetRemoveEventHandler"));

    RvTransportGetCallbacks(transport, &transportCallbacks);

    if (transportEventHandler != NULL)
    {
        RvStatus        status;
        RvSelectEngine* selectEngine;

        /* Override the Event callback */
        transportCallbacks.pfnEvent = transportEventHandler;
        transportCallbacks.usrData  = context;
        RvTransportSetCallbacks(transport, &transportCallbacks);

        /* Set Select Engine to be used with the Transport */
        status = RvSelectGetThreadEngine(/*logMgr*/NULL, &selectEngine);
        if ((status != RV_OK) || (selectEngine == NULL))
            selectEngine = rvRtpInstance.selectEngine;
        RvTransportSetOption(transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SELECTENGINE, (void*)selectEngine);

        /* Set the Transport to the non-blocking mode */
        boolVal = RV_TRUE;
        RvTransportSetOption(transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_NONBLOCKING, (void*)&boolVal);

        /* Register the Transport for the READ events */
        RvTransportRegisterEvent(transport, RVTRANSPORT_EVENT_READ);
    }
    else
    {
        /* Set the Transport to the blocking mode */
        boolVal = RV_FALSE;
        RvTransportSetOption(transport,
                RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                RVTRANSPORT_OPT_SOCK_NONBLOCKING, (void*)&boolVal);

        /* Unregister the Transport for the READ event */
        RvTransportRegisterEvent(transport, 0);
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RtpSetRemoveEventHandler"));
}

/************************************************************************************
 * RtpAddRemoteAddress
 * description: Adds the new RTP address of the remote peer or the address of a multicast
 *              group or of multiunicast address list to which the RTP stream will be sent.
 *              Adds multiplexID, if specified
 * input: hRTP  - Handle of the RTP session.
 *        pRtpAddress contains
 *            ip    - IP address to which RTP packets should be sent.
 *            port  - UDP port to which RTP packets should be sent.
 * output: none.
 * return value: none.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RtpAddRemoteAddress(
	IN RvRtpSession  hRTP,   /* RTP Session Opaque Handle */
	IN RvNetAddress* pRtpAddress,
    IN RvUint32*     pMultiplexID)
									  
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvAddress* pRvAddress =  NULL;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpAddRemoteAddress"));
#ifndef TI_368
	if (s != NULL && pRtpAddress!=NULL && RvNetGetAddressType(pRtpAddress)!= RVNET_ADDRESS_NONE)
#else
    if (s != NULL && pRtpAddress!=NULL)
#endif
    {
        RvChar addressStr[64];
        RV_UNUSED_ARG(addressStr); /* in case of RV_LOGLEVEL_NONE warning fix */
		
		RvLockGet(&s->lock, logMgr);
        pRvAddress = (RvAddress*) pRtpAddress->address;
        

        if (s->profilePlugin->funcs->addRemAddress != NULL)
        {
            s->profilePlugin->funcs->addRemAddress(s->profilePlugin, hRTP, RV_TRUE, pRtpAddress);
        }


		RvRtpAddressListAddAddress(&s->addressList, pRvAddress, pMultiplexID);
        s->remoteAddressSet = RV_TRUE;

        RTPLOG_INFO((rvLogPtr,"Added remote address %s port =%d to the RTP session %p", 
            RvAddressGetString(pRvAddress, sizeof(addressStr), addressStr),
            RvAddressGetIpPort(pRvAddress), (void*)hRTP));
		RvLockRelease(&s->lock, logMgr);

    } 
    else 
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RtpAddRemoteAddress: NULL pointer or wrong address type"));      
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RtpAddRemoteAddress"));

}

/************************************************************************************
 * RtpRemoveRemoteAddress
 * description: removes the specified RTP address of the remote peer or the address 
 *              of a multicast group or of multiunicast address list to which the 
 *              RTP stream was sent.
 *              Removes RTP remote address with multiplexID, if pMultiplexID is specified
 * input: hRTP  - Handle of the RTP session.
 *        pRtpAddress contains
 *            ip    - IP address to which RTP packets should be sent.
 *            port  - UDP port to which RTP packets should be sent.
 *        pMultiplexID - pointer to multiplexID of the remote RTP
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RtpRemoveRemoteAddress(
	IN RvRtpSession  hRTP,
	IN RvNetAddress* pRtpAddress,
    IN RvUint32*     pMultiplexID)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;		
    RvAddress* pRvAddress =  NULL;
    RvChar addressStr[64];
	RvStatus res;
  
    RV_UNUSED_ARG(addressStr); /* in case of RV_LOGLEVEL_NONE warning fix */    
	RvLogEnter(rvLogPtr, (rvLogPtr, "RtpRemoveRemoteAddress"));

#ifndef TI_368
    if ((s == NULL) || (pRtpAddress == NULL) || (RvNetGetAddressType(pRtpAddress)== RVNET_ADDRESS_NONE))
    {
        RvLogError(rvLogPtr, (rvLogPtr, "RtpRemoveRemoteAddress: NULL pointer or wrong address type"));      
        res = RV_ERROR_UNKNOWN;
    }
#else
	if ((s == NULL) || (pRtpAddress == NULL))
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RtpRemoveRemoteAddress: NULL pointer or wrong address type"));      
		res = RV_ERROR_UNKNOWN;
	}
#endif
	else
	{
		RvLockGet(&s->lock, logMgr);

#ifdef TI_368
		RvAddress addr;
		memcpy(&addr, pRtpAddress->address, sizeof(RvAddress));
		pRvAddress = &addr;
#else
		pRvAddress = (RvAddress*) pRtpAddress->address;
#endif

		RvRtpAddressListRemoveAddress(&s->addressList, pRvAddress, pMultiplexID);

		if (s->profilePlugin->funcs->removeRemAddress != NULL)
		{
			s->profilePlugin->funcs->removeRemAddress(s->profilePlugin, hRTP, pRtpAddress);
		}

		RvLogLeave(rvLogPtr, (rvLogPtr,"Removed remote address %s port =%d from the RTP session %p", 
			RvAddressGetString(pRvAddress, sizeof(addressStr), addressStr),
			RvAddressGetIpPort(pRvAddress), (void*)hRTP));    

		pRvAddress = RvRtpAddressListGetNext(&s->addressList, NULL);
		if (NULL == pRvAddress)
			s->remoteAddressSet = RV_FALSE;
		RvLockRelease(&s->lock, logMgr);    

		res = RV_OK;
	}

	RvLogLeave(rvLogPtr, (rvLogPtr, "RtpRemoveRemoteAddress"));
    return res;
}

//h.e transport
RVAPI
void RVCALLCONV RvRtpSetTransportCB(
	IN RvRtpInstance   *rtpInstance, 
	IN RvTransportGetCB transportCb)
{	
	rtpInstance->rvTransportGet = transportCb;
}
//===



RvStatus initiateSocket(
	IN RvSocket		*socket,  
	IN RvAddress	*address)
{
	RvStatus res;

	RvSocketSetBuffers(socket, 8192, 8192, logMgr);
	res = RvSocketSetBroadcast(socket, RV_TRUE, logMgr);
	if (res == RV_OK)
	res = RvSocketSetBlocking(socket, RV_TRUE, logMgr);
	if (res == RV_OK)
	res = RvSocketBind(socket, address, NULL, logMgr);

	return res;
}


RvStatus rtpPortRangeConstruct(
	 IN RvRtpPortRange *portRange,
	 IN RvUint16 firstPort, 
	 IN RvUint16 lastPort)
{
    RvStatus res;
	RvSize_t bitMapSize = (RvSize_t)(lastPort - firstPort + 2)/8 + 1;
	RvSize_t extrabits =  8 - (RvSize_t)(lastPort - firstPort + 2) % 8;


	res = RvMemoryAlloc(NULL, bitMapSize, logMgr, (void**)&portRange->portBitMap);
	if (res==RV_OK)
	{
		memset(portRange->portBitMap, 0, bitMapSize);
		/* mark padding bits as busy */
		switch (extrabits)
		{
		case 2: portRange->portBitMap[bitMapSize-1] = 0xC0; break;
		case 4: portRange->portBitMap[bitMapSize-1] = 0xF0; break;
		case 6: portRange->portBitMap[bitMapSize-1] = 0xFC;break;
		default: break;
		}

		portRange->fromPort = firstPort;
		portRange->toPort = lastPort;
		portRange->next = portRange->fromPort;
		res = RvLockConstruct(logMgr, &portRange->lock);
	}

	return res;
}

void rtpPortRangeDestruct(IN RvRtpPortRange *portRange)
{
	
	RvMemoryFree(portRange->portBitMap, logMgr);
	RvLockDestruct(&portRange->lock, logMgr);
	
	return;
}


RvBool rtpPortRangeIsSet(RvRtpPortRange *portRange)
{
	if ((portRange->fromPort == 0) && (portRange->toPort == 0))
		return RV_FALSE;
	else
		return RV_TRUE;

}


RvUint16 rtpPortRangeGetRtpPort(RvRtpPortRange *portRange)
{
	RvUint32 byteCnt=0;
	RvUint32 bitCnt;
	RvUint32 portCnt=0;
	RvChar   mask = 0x01;
	RvUint32 port=0;
	RvUint32 startBit=0;
	RvUint32 startByte=0;
	RvUint32 bitMapSize=((portRange->toPort - portRange->fromPort) / 8) + 1;
	RvBool   bStartFromBegining=RV_FALSE;
	RvBool	 bFreePorts=RV_TRUE;


    RvLockGet(&portRange->lock, NULL);

	/* find a place in bitMap to start */
	startByte = (portRange->next - portRange->fromPort) / 8;
	startBit  = (portRange->next - portRange->fromPort) % 8;

	mask = (RvChar)(mask << startBit);
	byteCnt = startByte;
	portCnt += byteCnt*8;

	while (portRange->portBitMap[byteCnt] == 0xFF)  /* while whole byte is full ...*/
	{
		
		portCnt += 8;  /* num of bits (ports) in 1 byte */
		++byteCnt;

		if (byteCnt >= bitMapSize)
		{
			portCnt = 0;  /* num of bits (ports) in 1 byte */
			byteCnt = 0;
			startBit = 0;
			bStartFromBegining = RV_TRUE;
			break;
		}
	}

	if (bStartFromBegining)
	while (portRange->portBitMap[byteCnt] == 0xFF)  /* while whole byte is full ...*/
	{
			
		portCnt += 8;  /* num of bits (ports) in 1 byte */
		++byteCnt;
			
		if (byteCnt >= bitMapSize)
		{
			bFreePorts=RV_FALSE;		
			break;
		}
	}


	if (bFreePorts)
	{
		for (bitCnt=startBit; bitCnt < 8; bitCnt+=2 /* "move" in pairs */)    
		{
			
			if ((portRange->portBitMap[byteCnt] & mask) == 0)
			{
				portCnt = portCnt + bitCnt;
				portRange->portBitMap[byteCnt] |= mask;	/* the first free port is found, mark it as busy */
				port = portRange->fromPort + portCnt;

				/* set next port (RTCP) as busy. Set port IS in the SAME byte */
				portRange->portBitMap[byteCnt] |= (mask << 1);	/* the first free port is found, mark it as busy */
				break;
			}
			mask = (RvChar)(mask << 2);  /* we interested only in even bits */
		}
    
		if (port != 0)
			portRange->next = (RvUint16)(port + 2);

		if (portRange->next > portRange->toPort)
			portRange->next = portRange->fromPort;
	}

	RvLockRelease(&portRange->lock, NULL);
	return (RvUint16)port;
}

	
RvStatus rtpPortRangeFreeRtpPort(RvRtpPortRange *portRange, RvUint16 port)
{
	RvUint32 byteCnt=0;
	RvUint32 bitCnt;
	RvUint32 portCnt=0;
	RvChar   mask = 0x01;

	/* We deal with even (RTP) ports ONLY */
	if (port & 1)
		return RV_ERROR_BADPARAM;
	
	if(port < portRange->fromPort || port > portRange->toPort)
		return RV_ERROR_BADPARAM;

    if(portRange->fromPort == 0 && portRange->toPort == 0)
        return RV_ERROR_BADPARAM;

	RvLockGet(&portRange->lock, NULL);
	portCnt = port - portRange->fromPort;
	byteCnt = portCnt / 8;
	bitCnt = portCnt - (byteCnt << 3);         /* shift on 3 = mul 8 */
	mask = (RvChar)(mask << bitCnt);
	portRange->portBitMap[byteCnt] &= ~mask;	/* the first free port is found, mark it as busy */
	portRange->portBitMap[byteCnt] &= ~(mask<<1);	/* the first free port is found, mark it as busy */
	RvLockRelease(&portRange->lock, NULL);
	return RV_OK;
}


/* Initiate the Transport configuration that uses this local address */
void RvRtpInitSocketTransportCfg(RvTransportSocketCfg *pCfg)
{
	RvTransportInitSocketTransportCfg(pCfg);
	
	/* set default values */
	pCfg->protocol  = RvSocketProtocolUdp;
	pCfg->options   = RVTRANSPORT_CREATEOPT_BROADCAST;
	pCfg->pLogMgr    = logMgr;
}
	

#ifdef __cplusplus
}
#endif










