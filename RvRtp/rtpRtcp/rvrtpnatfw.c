/************************************************************************
 File Name	   : rvrtpnatfw.c
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
#include "RtpDemux.h"
#include "rvrtpnatfw.h"
#include "rtputil.h"
#include "RtcpTypes.h"
#include "RtpProfileRfc3550.h"

#ifdef __cplusplus
extern "C" {
#endif
            
#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define RTP_SOURCE      (rtpGetSource(RVRTP_RTP_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_RTP_MODULE))
RvRtpLogger             rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"

#ifdef __H323_NAT_FW__
/************************************************************************************
 *                               Static Functions Prototypes                        *
 ************************************************************************************/
static
void rtpDemuxEvent(
    RvTransport       transport, 
    RvTransportEvents ev, 
    RvBool            error, 
    void*             usrData);

/************************************************************************************
 * rtpDemuxReadWithRemoteAddress
 * description: This routine 
 *          [reads the RTP message (isPeekMessage=TRUE) or
 *           uses  the read RTP message(isPeekMessage=FALSE)] 
 *              and sets the header of the RTP message.
 *              It also retrieves the address of the RTP message sender.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        isPeekMessage - TRUE, if RvSocketPeekMessage is supported
 * output: p            - A struct of RTP param,contain the fields of RTP header.
 *        remAddressPtr - pointer to the remote address
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
static RvInt32 rtpDemuxReadWithRemoteAddress(
		IN  RvRtpSession  hRTP,
		IN  void *buf,
		IN  RvInt32 len,
        IN  RvBool isPeekMessage,
		OUT RvRtpParam* p,
		OUT RvNetAddress* remAddressPtr);
#endif

/************************************************************************************
 *                                   MODULE FUNCTIONS                               *
 ************************************************************************************/
/**************************************************************************************
 * RvRtpNatMultiplexIdSize 
 * description: returns size of multiplexID in RTP/RTCP packet before RTP/RTCP header. 
 * input:  none
 * output: none.
 * return size of multplexID.
 * Note: this function needed to add additional buffer before RTP header (to move s byte in
 *       RvRtpParam structure
 *       for example for g.711:
 *       RvRtpParam p;
 *       RvRtpParamConstruct(&p);
 *       p.sByte += RvRtpNatMultiplexIdSize(); 
 *       p.sByte += RvRtpGetHeaderSize(); 
 *       .....
 *       RvRtpWrite(hRTP, buffer, p.sByte+ payload len, &p);
 **************************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpNatMultiplexIdSize(void)
{
    RTPLOG_ENTER(NatMultiplexIdSize);	
    RTPLOG_LEAVE(NatMultiplexIdSize);	
#ifdef __H323_NAT_FW__
    return 4;
#else
    return 0;
#endif
}


#ifdef __H323_NAT_FW__

#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtpDemux" #name
/**************************************************************************************
 * RvRtpDemuxConstruct
 * description: constructs RTP demultiplexing object. The memory of the object will be
 *              allocated according to NumberOfSessions parameter.
 * input: 
 *      numberOfSessions - maximal number of demultiplexing sessions
 * output: note
 * return value: RvRtpDemux - handle to RTP demultiplexing object on success,
 *                            otherwise NULL
 **************************************************************************************/

RVAPI
RvRtpDemux RVCALLCONV RvRtpDemuxConstruct(
  IN RvUint32 numberOfSessions)
{
    RtpDemux* demuxPtr = NULL;
        
    RTPLOG_ENTER(Construct);
    if (RvMemoryAlloc(NULL, (RvSize_t)sizeof(RtpDemux), logMgr, (void**)&demuxPtr) != RV_OK)
    {
        RTPLOG_ERROR_LEAVE(Construct, "failed to allocate demux table");
        return NULL;
    }
    memset(demuxPtr, 0, sizeof(RtpDemux));

    if (RvRaConstruct(sizeof(RtpDemuxTableEntry), numberOfSessions, RV_TRUE,"ARTP Demux Entries", logMgr,&demuxPtr->demuxTbl) != RV_OK)
    {
        RvMemoryFree(demuxPtr, logMgr);
        RTPLOG_ERROR_LEAVE(Construct, "failed to allocate demux table");
        return NULL;
    }
    if (RvLockConstruct(logMgr, &demuxPtr->lock)!=RV_OK)
    {
        RvRaDestruct(demuxPtr->demuxTbl);
        RvMemoryFree(demuxPtr, logMgr);
        RTPLOG_ERROR_LEAVE(Construct, "failed to construct demux lock"); 
        return NULL;
    }
    demuxPtr->maxSessions = numberOfSessions;
    RTPLOG_LEAVE(Construct);
    return (RvRtpDemux)demuxPtr;
}

/**************************************************************************************
 * RvRtpDemuxDestruct
 * description: destructs RTP demultiplexing object. The memory of the object will be
 *              freed.
 * input: 
 *      demux - handle to the RTP demultiplexing object
 * output: note
 * return value: Non-negative value on success
 *               Negative value on failure
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDemuxDestruct(RvRtpDemux demux)
{
    RtpDemux* demuxPtr = (RtpDemux*) demux;
    RTPLOG_ENTER(Destruct);
    if (demux == NULL)
    {
        RTPLOG_ERROR_LEAVE(Destruct, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }    
    RvLockGet(&demuxPtr->lock, logMgr);
    if (demuxPtr->rtcpSessionsCounter > 0 || demuxPtr->rtpSessionsCounter > 0)
    {  
        RvLockRelease(&demuxPtr->lock, logMgr);
        RTPLOG_ERROR_LEAVE(Destruct, "failed to destruct demux object: there are living sessions");
        return RV_ERROR_UNKNOWN;
    }
    RvLockRelease(&demuxPtr->lock, logMgr);
    RvLockDestruct(&demuxPtr->lock, logMgr);
    RvRaDestruct(demuxPtr->demuxTbl);
    RvMemoryFree(demuxPtr, logMgr);
    RTPLOG_LEAVE(Destruct);
    return RV_OK;
}

/**************************************************************************************
 * RvRtpDemuxOpenedSessionsNumberGet
 * description: returns number of opened session in demux
 * input: 
 *      demux - handle to the demux object
 * output: note
 * return value: Non-negative value on success
 *               Negative value on failure
 **************************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDemuxOpenedSessionsNumberGet(RvRtpDemux demux)
{
    RtpDemux* demuxPtr = (RtpDemux*) demux;
    RvInt32 number = 0;

    RTPLOG_ENTER(OpenedSessionsNumberGet);
    if (demux == NULL)
    {
        RTPLOG_ERROR_LEAVE(OpenedSessionsNumberGet, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }    
    RvLockGet(&demuxPtr->lock, logMgr);
    number = demuxPtr->rtpSessionsCounter;
    RvLockRelease(&demuxPtr->lock, logMgr);
    RTPLOG_LEAVE(OpenedSessionsNumberGet);
    return number;
}

/**************************************************************************************
 * RvRtpDemuxGetLocalAddress 
 * description: gets the local demultiplexing address. 
 * input: 
 *        demux         - handle to RTP demultiplexing object
 * output:
 *        pRtpAddress   - pRtpAddress contains local address for RTP session
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 * Note:
 * At least one session that supports demultiplexing must be opened RvRtpDemuxOpenSession.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDemuxGetLocalAddress(
    IN     RvRtpDemux            demux,
    INOUT  RvNetAddress*         pRtpAddress)
{
    RtpDemux* demuxPtr = (RtpDemux*) demux;
    RvStatus status;

    RTPLOG_ENTER(GetLocalAddress);
    if (demux == NULL||pRtpAddress == NULL)
    {
        RTPLOG_ERROR_LEAVE(GetLocalAddress, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    RvLockGet(&demuxPtr->lock, logMgr);   
#ifdef __SIMPLE_COPY__
    memcpy(pRtpAddress, &demuxPtr->rtpAddress, sizeof(RvNetAddress));
#else
    {
        RvAddress address;
        status = RvTransportGetOption(demuxPtr->rtpTransport,
                    RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                    RVTRANSPORT_OPT_LOCALADDR, (void*)&address);
        if (status == RV_OK)
        {
            memcpy(pRtpAddress, &address, sizeof(RvNetAddress));
        }
        else
        {
            RTPLOG_ERROR_LEAVE(GetLocalAddress, "RvTransportGetOption() failed.");
            RvLockRelease(&demuxPtr->lock, logMgr);
            return status;
        }
    }
#endif
    RTPLOG_LEAVE(GetLocalAddress);
    RvLockRelease(&demuxPtr->lock, logMgr);
    return RV_OK;
}
/***************************************************************************************
 *                  Demultiplexing RTP sub sessions management
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpDemuxOpenSession 
 * description: checks local address and opens RTP/RTCP sub-session with the same address 
 *              as already opened sub-session (if exist).
 *              The first RTP/RTCP sub-session sets the local demultiplexing address. 
 * input: 
 *        demux          - handle to RTP demultiplexing object
 *        pRtpAddress    - pRtpAddress contains local address for RTP session
 *        ssrcPattern    - Synchronization source Pattern value for the RTP session.
 *        ssrcMask       - Synchronization source Mask value for the RTP session.
 *        cname          - The unique name representing the source of the RTP data. 
 *                         The name will be sent in Canonical End-Point Identifier SDES
 *                         Item through the corresponding RTCP session.
 *        sessionContext - context, usually used for application RTP session handle
 * output:
 *        pMultiplexID  - pointer to multiplexID
 * return value: RvRtpSession handle, on success,
 *               NULL value on failure
 * Note:
 * The socket of this session will be shared by all RTP sub sessions. 
 * The packets of these RTP sub-sessions will pass through the NAT/FW. 
 * If cname is not NULL the RTCP multiplexing session will be opened 
 * with the next to RTP session port.
 **************************************************************************************/
RVAPI
RvRtpSession RVCALLCONV RvRtpDemuxOpenSession (
  IN     RvRtpDemux            demux,
  IN     RvNetAddress*         pRtpAddress,
  IN     RvUint32              ssrcPattern,
  IN     RvUint32              ssrcMask,
  IN     char *                cname,
  IN     void*                 sessionContext,
  OUT    RvUint32*             pMultiplexID)
{
    RtpDemux*           demuxPtr      = (RtpDemux*) demux;
    RvRtpSession        hRTP          = NULL;
    RvRtcpSession       hRTCP         = NULL;
    RvUint32            vacantEntry   = 0;
    RvRtpSessionInfo*   sRTP          = NULL;
    rtcpSession*        sRTCP         = NULL;          
    RtpDemuxTableEntry* pEntry        = NULL;
    RvNetAddress        localAddress;
    RvNetAddress*       pLocalAddress = pRtpAddress;

    RTPLOG_ENTER(OpenSession);
    if (demux == NULL || pMultiplexID == NULL)
    {
        RTPLOG_ERROR_LEAVE(OpenSession, "NULL pointer");
        return NULL;
    }
    RvLockGet(&demuxPtr->lock, logMgr);    
    if (pRtpAddress == NULL)
    {
        RvAddressCopy((RvAddress*)&demuxPtr->rtpAddress, (RvAddress*)&localAddress);
        pLocalAddress = &localAddress;
    }
    if (RtpDemuxTablePickVacantEntry(demuxPtr, &pEntry, &vacantEntry) != RV_OK)
    {
        RvLockRelease(&demuxPtr->lock, logMgr);
        RTPLOG_ERROR_LEAVE(OpenSession, "cannot allocate demux table entry");
        return NULL;
    }
    RvLockRelease(&demuxPtr->lock, logMgr);
    hRTP = RtpOpenFrom(demux, pRtpAddress, ssrcPattern, ssrcMask,
                       NULL /*buffer*/, 0 /*bufsize*/, cname);
    if (hRTP == NULL)
    {
        RvLockGet(&demuxPtr->lock, logMgr);
        RvRaDelete(demuxPtr->demuxTbl, (RAElement) pEntry);
        RvLockRelease(&demuxPtr->lock, logMgr);
        RTPLOG_ERROR_LEAVE(OpenSession, "cannot open RTP session");
        return NULL;
    }
    RvLockGet(&demuxPtr->lock, logMgr);
    demuxPtr->runningCounter++;
    sRTP              = (RvRtpSessionInfo*) hRTP;
    pEntry->hRTP      = hRTP;
    
    if (demuxPtr->rtpSessionsCounter == 0)
    {   
        memcpy(&demuxPtr->rtpAddress, pRtpAddress, sizeof(RvNetAddress));
    }
    if (cname != NULL) /**/
    {
        hRTCP                = RvRtpGetRTCPSession(hRTP);
        pEntry->hRTCP        = hRTCP;
        sRTCP                = (rtcpSession*) hRTCP;
        if (demuxPtr->rtcpSessionsCounter == 0)
        {
            RvNetAddress rtcpAddress;
            if (RvNetGetAddressType(pRtpAddress) == RVNET_ADDRESS_IPV4)
            {  
                RvNetIpv4 IpV4;
                RvNetGetIpv4(&IpV4, pRtpAddress);
                IpV4.port++;
                RvNetCreateIpv4(&rtcpAddress, &IpV4);
            }
            else
            {
                RvNetIpv6 IpV6;
                RvNetGetIpv6(&IpV6, pRtpAddress);
                IpV6.port++;
                RvNetCreateIpv6(&rtcpAddress, &IpV6);    
            }  
            memcpy(&demuxPtr->rtcpAddress, &rtcpAddress, sizeof(RvNetAddress));
        }
        demuxPtr->rtcpSessionsCounter++;
        sRTCP->demuxTblEntry = vacantEntry;
    }
    demuxPtr->rtpSessionsCounter++;
    sRTP->demuxTblEntry =  vacantEntry;
    *pMultiplexID       =  pEntry->multiplexID = getMultiplexID(demuxPtr->runningCounter, vacantEntry);
    RvLockRelease(&demuxPtr->lock, logMgr);
    sRTP->context       = sessionContext; 
    RTPLOG_INFO((RTP_SOURCE, "Opened demuxed RTP session %#x, multiplexerID =%#x", hRTP, *pMultiplexID));
    RTPLOG_LEAVE(OpenSession);
    return hRTP;
}


/**************************************************************************************
 * RvRtpDemuxCloseSession 
 * description: closes RTP sub session and corresponding RTCP sub session (if associated).
 * input: 
 *        rtpSession    - handle to  demultiplexing RTP sub-session
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpDemuxCloseSession(
   IN RvRtpSession rtp_Session)
{
    RvRtpSessionInfo*    sRTP         = (RvRtpSessionInfo*) rtp_Session;
    RtpDemuxTableEntry*  pEntry       = NULL;
    RtpDemuxTableEntry*  pEntry2      = NULL;
    RtpDemux*            demuxPtr     = NULL;
    RvUint32             index        = 0;

    RTPLOG_ENTER(CloseSession);
    if (rtp_Session == NULL)
    {
        RTPLOG_ERROR_LEAVE(CloseSession, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    index    = sRTP->demuxTblEntry;    
    demuxPtr = (RtpDemux*) sRTP->demux;
    if (demuxPtr == NULL)
    {
        RTPLOG_ERROR_LEAVE(CloseSession, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    RvLockGet(&demuxPtr->lock, logMgr);
    if (((pEntry = (RtpDemuxTableEntry*) RvRaGet(demuxPtr->demuxTbl, index)) != NULL) &&
        (getIndexFromMultiplexID(pEntry->multiplexID)) == index)
    {
        if (demuxPtr->rtpSessionsCounter > 0)
        {
            demuxPtr->rtpSessionsCounter--;
            if (demuxPtr->rtcpSessionsCounter>0)
            {
                if (pEntry->hRTCP != NULL)
                    demuxPtr->rtcpSessionsCounter--;
                else
                {
                    rtcpSession* sRTCP         = (rtcpSession*) RvRtpGetRTCPSession(rtp_Session);
                    RvUint32 index2            = 0;
                    if (sRTCP != NULL)
                    {
                        index2 = sRTCP->demuxTblEntry;
                        if (((pEntry2 = (RtpDemuxTableEntry*) RvRaGet(demuxPtr->demuxTbl, index2)) != NULL) &&
                            (index2 == getIndexFromMultiplexID(pEntry2->multiplexID)) &&
                            pEntry2->hRTCP == (RvRtcpSession) sRTCP)
                        {
                            memset(pEntry2, 0, sizeof(RtpDemuxTableEntry));
                            RvRaDelete(demuxPtr->demuxTbl, (RAElement)pEntry2);
                            demuxPtr->rtcpSessionsCounter--;
                        }
                        /* else error */
                    }
                }           
            }
            /* initialization of table entry */
            memset(pEntry, 0, sizeof(RtpDemuxTableEntry));
            RvRaDelete(demuxPtr->demuxTbl, (RAElement)pEntry);
        }
    }
    RvLockRelease(&demuxPtr->lock, logMgr);
    RvRtpClose(rtp_Session); /* closes  RTP session [and RTCP session, if needed] */
    RTPLOG_LEAVE(CloseSession);
    return RV_OK;
}

/************************************************************************************
 * RvRtpDemuxSetEventHandler
 * description: Set an Event Handler for the multiplexed RTP sessions. The application may set
 *              an Event Handler for RTP multiplexed sessions to operate in non-blocking mode.
 * input: hdemux        - the demux object handle.
 *        eventHandler  - Pointer to the callback function that is called each time a
 *                        new RTP packet arrives to the demux.
 *        context       - The parameter is an application handle that identifies the
 *                        particular demux. The application passes the handle to
 *                        the Event Handler.
 * output: none.
 * return value: none.
 ***********************************************************************************/
RVAPI
void RVCALLCONV RvRtpDemuxSetEventHandler(
        IN  RvRtpDemux                 hdemux,
        IN  RvRtpDemuxEventHandler_CB  eventHandler,
        IN  void *                     context)
{
    RtpDemux *d = (RtpDemux *) hdemux;
    RvTransportEventCb pfnTransporteventHandler;

    RTPLOG_ENTER(SetEventHandler);
    if (d == NULL)
    {
        RTPLOG_ERROR_LEAVE(SetEventHandler, "NULL pointer");
        return;
    }

    RvLockGet(&d->lock, logMgr);

    d->rtpEvHandler      = eventHandler;
    d->rtpContext        = context;

    pfnTransporteventHandler = (eventHandler == NULL) ? NULL : rtpDemuxEvent;
    RtpSetRemoveEventHandler(d->rtpTransport, pfnTransporteventHandler, (void*)d);

    RvLockRelease(&d->lock, logMgr);
    if (eventHandler != NULL)
    {            
        RTPLOG_INFO((RTP_SOURCE, "Set RTP Event Handler for non-blocking read for demux %#x", d));
    }
    else
    {
        RTPLOG_INFO((RTP_SOURCE, "Removed RTP Event Handler for non-blocking read for demux %#x", d));
    }
    RTPLOG_LEAVE(SetEventHandler);
}

/***************************************************************************************
 *                 Receiving of Multiplexed packets
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpDemuxReadWithRemoteAddress 
 * description: receives multiplexed packets from the remote peer. 
 *        Returns hRTP, hRTCP for RvRtcpRtpPackRecv() can be found by RvRtpGetRTCPSession()
 * input:  
 *        hdemux          - the demux object handle.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 * output: 
 *        phRTP          - pointer to handle of RTP sub session to which this packet was sent.
 *        sessionContextPtr - pointer to context, usually used for application RTP session handle
 *        p              - pointer to RvRtpParam structure (payload type field must be set)
 *        remAddressPtr  - pointer to the remote address
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 **************************************************************************************/
 
RVAPI
RvStatus RVCALLCONV RvRtpDemuxReadWithRemoteAddress(
     IN     RvRtpDemux            hdemux,
     IN     void*                 buf,
     IN     RvInt32               len,
     INOUT  RvRtpSession*         phRTP,
     INOUT  void**                sessionContextPtr,
     OUT    RvRtpParam*           p,
     OUT    RvNetAddress*         remAddressPtr)
{
    RvRtpSessionInfo* s             = NULL;
    RtpDemux*         demuxPtr      = (RtpDemux*) hdemux;
    RvStatus          res           = RV_ERROR_UNKNOWN;
    RvSize_t          messageSize   = 0;
    RvUint32*         pMultiplexID  = buf;
    RvUint32          demuxTblEntry = 0;
    RvRtpSession      hRTP          = NULL; 
    RvBool            isPeekMessage = RV_TRUE;
    RTPLOG_ENTER(ReadWithRemoteAddress);
    if (phRTP==NULL || hdemux == NULL || buf == NULL)
    {
        RTPLOG_ERROR_LEAVE(ReadWithRemoteAddress, "NULL pointer");
        return res; 
    }
    
    RvLockGet(&demuxPtr->lock, logMgr);
    if (demuxPtr->rtpSessionsCounter > 0)
    {
		res = RvTransportReceiveBuffer(
                        demuxPtr->rtpTransport, (RvUint8*)buf, (RvSize_t)len,
                        0/*options*/, (RvAddress*)remAddressPtr, &messageSize);
		isPeekMessage = RV_FALSE;
		p->len        = (RvUint32)messageSize;
        if (res == RV_OK && messageSize <4)
        {
            if (isPeekMessage)
            {
                res = RvTransportReceiveBuffer(
                        demuxPtr->rtpTransport, (RvUint8*)buf, (RvSize_t)len,
                        0/*options*/, (RvAddress*)remAddressPtr, &messageSize);
            }
            RTPLOG_LEAVE(ReadWithRemoteAddress);
            RvLockRelease(&demuxPtr->lock, logMgr);
            return RV_ERROR_UNKNOWN;
        } 
        if (res == RTP_DESTINATION_UNREACHABLE)
        {    
            /* error of sending from socket to an unreachable destination.
            Since used the same socket for sending and receiving, if sending is failed,
            here we receive the sending error. In this case logging will be bombed by error messages.
            This is the design defect, but it allow to use 1 socket per RTP session instead of two.*/ 
            /* RTPLOG_ERROR_LEAVE(Read, "RvSocketReceiveBuffer(). The sending destination is unreachable."); 
            it is intentionally omitted not to produce printings loops */
            if (isPeekMessage)
            {
                res = RvTransportReceiveBuffer(
                        demuxPtr->rtpTransport, (RvUint8*)buf, (RvSize_t)len,
                        0/*options*/, (RvAddress*)remAddressPtr, &messageSize);
            }
            RTPLOG_LEAVE(ReadWithRemoteAddress);
            RvLockRelease(&demuxPtr->lock, logMgr);
            return res; 
        }
        if (res != RV_OK || messageSize <4) /* PeekMessage is already checked */
        {
            RTPLOG_ERROR_LEAVE(ReadWithRemoteAddress, "Reading failed .");  
            RvLockRelease(&demuxPtr->lock, logMgr);
            return RV_ERROR_UNKNOWN;
        }
    }
    else
    {
        RTPLOG_ERROR_LEAVE(ReadWithRemoteAddress, "No opened demultiplexed RTP sessions.");
        RvLockRelease(&demuxPtr->lock, logMgr);
        return res;
    }      
    
    if (demuxPtr->rtpSessionsCounter > 0)
    {
        RtpDemuxTableEntry* pEntry = NULL;

        ConvertFromNetwork(buf, 0, 1);
        demuxTblEntry = getIndexFromMultiplexID(*pMultiplexID);
        if (((pEntry = (RtpDemuxTableEntry*)RvRaGet(demuxPtr->demuxTbl, demuxTblEntry)) != NULL) &&
            (pEntry->multiplexID == *pMultiplexID) &&
            (pEntry->hRTP != NULL))
        {
            hRTP = pEntry->hRTP;
            
            s    = (RvRtpSessionInfo*) hRTP;
            
            res = rtpDemuxReadWithRemoteAddress(hRTP, buf, len, isPeekMessage, p, remAddressPtr);
            if (res == RV_OK)
            {
                *phRTP = hRTP;
                if (NULL != sessionContextPtr)
                    *sessionContextPtr = s->context;
            }
            else
            {
                RTPLOG_ERROR1(ReadWithRemoteAddress, "Failed to read RTP packet"); 
            }
        }
        else
        {
            RTPLOG_ERROR((RTP_SOURCE, "RvRtpDemuxReadWithRemoteAddress - Wrong multiplex index %#x or no such multiplexed RTP sssion", *pMultiplexID)); 
            if (isPeekMessage)
            {
                res = RvTransportReceiveBuffer(
                        demuxPtr->rtpTransport, (RvUint8*)&buf, (RvSize_t)len,
                        0/*options*/, (RvAddress*)remAddressPtr, &messageSize);
            }
            res = RV_ERROR_UNKNOWN;
        }
    }
    else
    {
        RTPLOG_ERROR1(ReadWithRemoteAddress,  "No more RTP sessions"); 
    }
    RTPLOG_LEAVE(ReadWithRemoteAddress);
    RvLockRelease(&demuxPtr->lock, logMgr);
    return res;
}

/***************************************************************************************
 *                   RTP remote destination management
 ***************************************************************************************/
#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtpMultiplexing" #name
/**************************************************************************************
 * RvRtpMultiplexingAddRemoteAddress 
 * description: adds the new RTP address of the remote peer or the address a multi-cast group
 *              or of multi-unicast address list to which the RTP stream will be sent. 
 * input: 
 *        hRTP          - handle to RTP demultiplexing object
 *        pRtpAddress   - pRtpAddress of remote RTP session
 *        multiplexerId - multiplexID of remote RTP session.
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 * Note:
 *        the same addresses with the same multiplexerID will be discarded
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpMultiplexingAddRemoteAddress(
	IN RvRtpSession   hRTP,   /* RTP Session Opaque Handle */
	IN RvNetAddress*  pRtpAddress,
    IN RvUint32       multiplexerId)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvAddress* pRvAddress =  NULL;
	RvStatus res;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMultiplexingAddRemoteAddress"));

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
        

        if( 
/** h.e
			s->profilePlugin != NULL && 
            s->profilePlugin->funcs != NULL &&
**/
            s->profilePlugin->funcs->addRemAddress != NULL)
        {
            s->profilePlugin->funcs->addRemAddress(s->profilePlugin, hRTP, RV_TRUE, pRtpAddress);
        }

		RvRtpAddressListAddAddress(&s->addressList, pRvAddress, &multiplexerId);
        s->remoteAddressSet = RV_TRUE;

        RTPLOG_INFO((RTP_SOURCE,"Added remote address %s port =%d to the RTP session %#x", 
            RvAddressGetString(pRvAddress, sizeof(addressStr), addressStr),
            RvAddressGetIpPort(pRvAddress), (RvUint32)hRTP));

		RvLockRelease(&s->lock, logMgr);
		res = RV_OK;
    } 
	else 
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMultiplexingAddRemoteAddress: NULL pointer or wrong address type"));      
		res = RV_ERROR_NULLPTR;
	}
	
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMultiplexingAddRemoteAddress"));
    return res;
}

/**************************************************************************************
 * RvRtpMultiplexingRemoveRemoteAddress 
 * description: removes the  RTP multiplexing address of the remote peer
 *              or the address a multi-cast group
 *              or of multi-unicast address list to which the RTP stream was sent. 
 * input: 
 *        hRTP          - handle to RTP demultiplexing object
 *        pRtpAddress   - pRtpAddress of remote RTP session
 *        multiplexerId  - multiplexID of remote RTP session.
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpMultiplexingRemoveRemoteAddress(
	IN RvRtpSession    hRTP,  
	IN RvNetAddress*   pRtpAddress,
    IN RvUint32        multiplexerId)
{
    RTPLOG_ENTER(RemoveRemoteAddress);	
    RtpRemoveRemoteAddress (hRTP, pRtpAddress, &multiplexerId);
    RTPLOG_LEAVE(RemoveRemoteAddress);	
    return RV_OK;
}

 
/***************************************************************************************
 *                   RTP Keep Alive Packet
 ***************************************************************************************/

/**************************************************************************************
 * RvRtpSendKeepAlivePacket 
 * description: Sends RTP keep alive packet to the remote session.
 * input: 
 *        hRTP           - handle to RTP session 
 *        p              - pointer to RvRtpParam structure (payload type field must be set)
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 * Note: 1) H.460.19-MA defines keepAliveInterval (5-30 sec) of such packets in case that 
 *       media is not sent. Sending of this packet is in responsibility of the application.
 *       2) In pointer to RvRtpParam payload field must be filled
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSendKeepAlivePacket(
   IN  RvRtpSession hRTP, 
   IN  RvRtpParam *   p)
{
    RvUint8 buffer[128];
    if (hRTP==NULL||p==NULL)
        return RV_ERROR_UNKNOWN;
    p->sByte = RvRtpNatMultiplexIdSize();
    p->sByte += RvRtpGetHeaderLength();
    p->len   =  RvRtpGetHeaderLength() + RvRtpNatMultiplexIdSize();
    return RvRtpWrite(hRTP, buffer, 12 + RvRtpNatMultiplexIdSize(), p);
}





/**************************************************************************************
 * Static functions                                                       *
 **************************************************************************************/
#undef FUNC_NAME
#define FUNC_NAME(name) "rtpDemux" #name

static
void rtpDemuxEvent(
    RvTransport       transport, 
    RvTransportEvents ev, 
    RvBool            error, 
    void*             usrData)
{
    RtpDemux* d = (RtpDemux*)usrData;

    RTPLOG_ENTER(DemuxEvent);    
    RV_UNUSED_ARG(transport);
    RV_UNUSED_ARG(ev);
    RV_UNUSED_ARG(error);
    if (d->rtpEvHandler)
    {
        d->rtpEvHandler((RvRtpDemux)d, d->rtpContext);
    }
    RTPLOG_LEAVE(DemuxEvent);
}

/************************************************************************************
 * rtpDemuxReadWithRemoteAddress
 * description: This routine 
 *          [reads the RTP message (isPeekMessage=TRUE) or
 *           uses  the read RTP message(isPeekMessage=FALSE)] 
 *              and sets the header of the RTP message.
 *              It also retrieves the address of the RTP message sender.
 * input: hRTP  - Handle of the RTP session.
 *        buf   - Pointer to buffer containing the RTP packet with room before first
 *                payload byte for RTP header.
 *        len   - Length in bytes of buf.
 *        isPeekMessage - TRUE, if RvSocketPeekMessage is supported
 * output: p            - A struct of RTP param,contain the fields of RTP header.
 *        remAddressPtr - pointer to the remote address
 * return value: If no error occurs, the function returns the non-negative value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
static RvInt32 rtpDemuxReadWithRemoteAddress(
		IN  RvRtpSession  hRTP,
		IN  void *buf,
		IN  RvInt32 len,
        IN  RvBool isPeekMessage,
		OUT RvRtpParam* p,
		OUT RvNetAddress* remAddressPtr)
{
    RvRtpSessionInfo* s = (RvRtpSessionInfo *)hRTP;
    RvStatus res = RV_ERROR_UNKNOWN;

    RvLogEnter(rvLogPtr, (rvLogPtr, "rtpDemuxReadWithRemoteAddress"));

    if (s==NULL)
    {
		RvLogError(rvLogPtr, (rvLogPtr, "rtpDemuxReadWithRemoteAddress: RTP session is not opened"));
		res = RV_ERROR_NULLPTR;
    }
	else
	{
		if (
			s->profilePlugin->funcs->readXRTP != NULL)
		{
			res = s->profilePlugin->funcs->readXRTP(s->profilePlugin, hRTP, buf, len, isPeekMessage, p, (RvNetAddress*)remAddressPtr);  
		}
		else 
		{
			RvLogError(rvLogPtr, (rvLogPtr, "rtpDemuxReadWithRemoteAddress: NULL pointer or wrong address type"));      
			res = RV_ERROR_NULLPTR;
		}
	}
    RvLogLeave(rvLogPtr, (rvLogPtr, "rtpDemuxReadWithRemoteAddress"));
    return res;
}


#endif /* __H323_NAT_FW__ */


#ifdef __cplusplus
}
#endif






