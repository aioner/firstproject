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

#include "rvlock.h"
#include "rvmemory.h"
#include "rvsocket.h"
#include "rvnetaddress.h"
#include "rtp.h"
#include "rtcp.h"
#include "RtpDemux.h"
#include "rvrtpnatfw.h"
#include "RtcpTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __H323_NAT_FW__

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
/************************************************************************************
 *                               Static Functions Prototypes                        *
 ************************************************************************************/



/************************************************************************************
 *                                   MODULE FUNCTIONS                               *
 ************************************************************************************/

/**************************************************************************************
 * RvRtcpDemuxGetLocalAddress 
 * description: gets the local RTCP demultiplexing address. 
 * input: 
 *        demux         - handle to RTP demultiplexing object
 * output:
 *        pRtcpAddress   - pRtpAddress contains local address for RTCP session
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 * Note:
 * At least one session that supports demultiplexing must be opened by 
 * RvRtpDemuxOpenSession or RvRtcpDemuxOpenSession.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpDemuxGetLocalAddress(
    IN     RvRtpDemux            demux,
    INOUT  RvNetAddress*         pRtcpAddress)
{
    RtpDemux* demuxPtr = (RtpDemux*) demux;
    RvStatus status;

    RTPLOG_ENTER(GetLocalAddress);
    if (demux == NULL||pRtcpAddress == NULL)
    {
        RTPLOG_ERROR_LEAVE(GetLocalAddress, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    RvLockGet(&demuxPtr->lock, logMgr);   
#ifdef __SIMPLE_COPY__
    memcpy(pRtpAddress, &demuxPtr->rtcpAddress, sizeof(RvNetAddress));
#else
    {
        RvAddress address;
        status = RvTransportGetOption(demuxPtr->rtcpTransport,
                    RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT,
                    RVTRANSPORT_OPT_LOCALADDR, (void*)&address);
        if (status == RV_OK)
        {
            memcpy(pRtcpAddress, &address, sizeof(RvNetAddress));
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
 *                  Demultiplexing RTCP sub sessions management
 ***************************************************************************************/

/**************************************************************************************
 * RvRtcpDemuxOpenSession
 * description: checks local address and opens RTCP sub-session with the same address,
 *              as already opened sub-session (if exist).
 *              The first RTCP sub-session sets the local demultiplexing address.
 *              This function can be used only if RTCP demultiplexing port must be
 *              non-consecutive to RTP demultiplexing port, or if local IP for
 *              RTP and RTCP are different.
 * input:
 *        demux         - handle to RTP demultiplexing object
 *        pRtpAddress   - pRtpAddress contains local address for RTP session
 *        ssrc          - Synchronization source value of the RTP session.
 *        cname         - Non NULL value of the unique name representing the source of the RTP data.
 *                        The name will be sent in Canonical End-Point Identifier SDES
 *                        Item through the corresponding RTCP session.
 * output:
 *        pMultiplexID  - pointer to multiplexID
 * return value: RvRtcpSession handle, on success,
 *               NULL value on failure
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
    OUT        RvUint32*             pMultiplexID)
{
    RtpDemux*           demuxPtr     = (RtpDemux*) demux;
    RvRtcpSession       hRTCP        = NULL;
    RvUint32            vacantEntry  = 0;
    rtcpSession*        sRTCP        = NULL;
    RtpDemuxTableEntry* pEntry       = NULL;
    RvLogEnter(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxOpenSession"));

    hRTCP= RvRtcpOpen(ssrc, pRtcpAddress, cname);

    if (hRTCP == NULL)
    {
        RvLogEnter(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxOpenSession: cannot open RTCP session"));
        RvLogLeave(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxOpenSession"));
        return NULL;
    }
    RvLockGet(&demuxPtr->lock, logMgr);
    if (RtpDemuxTablePickVacantEntry(demuxPtr, &pEntry, &vacantEntry) != RV_OK)
    {
        RvLockRelease(&demuxPtr->lock, logMgr);
        RvRtcpClose(hRTCP);
        RvLogEnter(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxOpenSession: cannot allocate demux table entry"));
        RvLogLeave(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxOpenSession"));
        return NULL;
    }
    demuxPtr->runningCounter++;

    sRTCP                                 = (rtcpSession*) hRTCP;
    if (demuxPtr->rtcpSessionsCounter==0)
    {
        demuxPtr->rtcpTransport = sRTCP->transport;
        memcpy(&demuxPtr->rtcpAddress, pRtcpAddress, sizeof(RvNetAddress));
    }

    demuxPtr->rtcpSessionsCounter++;
    sRTCP->demux         = demux;
    sRTCP->demuxTblEntry = vacantEntry;
    *pMultiplexID        =  pEntry->multiplexID = getMultiplexID(demuxPtr->runningCounter, vacantEntry);

    RvLockRelease(&demuxPtr->lock, logMgr);

    RvLogLeave(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxOpenSession"));
    return hRTCP;
}

/**************************************************************************************
 * RvRtcpDemuxCloseSession
 * description: closes demultiplexed RTCP sub session.
 * input:
 *        hRTCP    - handle to  demultiplexing RTP sub-session
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 **************************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtcpDemuxCloseSession(
   IN RvRtcpSession hRTCP)
{
    rtcpSession*        sRTCP       = (rtcpSession*) hRTCP;
    RtpDemux*           demuxPtr    = NULL;
    RvUint32            index       = 0;
    RtpDemuxTableEntry* pEntry      = NULL;
    RvStatus            res         = RV_ERROR_UNKNOWN;

    RvLogEnter(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxCloseSession"));
    if (hRTCP == NULL)
    {
        RvLogError(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxCloseSession - NULL pointer"));
        RvLogLeave(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxCloseSession"));
        return RV_ERROR_NULLPTR;
    }
    RvLockGet(&demuxPtr->lock, logMgr);
    index    = sRTCP->demuxTblEntry;
    demuxPtr = (RtpDemux*) sRTCP->demux;
    
    if ((sRTCP->demux!=NULL) && 
        ((pEntry = (RtpDemuxTableEntry*) RvRaGet(demuxPtr->demuxTbl, index))!=NULL) &&
        ((getIndexFromMultiplexID(pEntry->multiplexID)) == index))
    {
        memset(pEntry, 0, sizeof(RtpDemuxTableEntry));
        /* initialization of table entry */
        res = RvRaDelete(demuxPtr->demuxTbl, (RAElement) pEntry);
    }
    RvLockRelease(&demuxPtr->lock, logMgr);
    RvRtcpClose(hRTCP); /* closes  RTCP */

    RvLogLeave(RTP_SOURCE, (RTP_SOURCE, "RvRtcpDemuxCloseSession"));
    return RV_OK;
}

/***************************************************************************************
 *                   RTCP remote destination management
 ***************************************************************************************/


/**************************************************************************************
 * RvRtcpMultiplexingAddRemoteAddress
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
RvStatus RVCALLCONV RvRtcpMultiplexingAddRemoteAddress(
	IN RvRtcpSession  hRTCP,   /* RTCP Session Opaque Handle */
	IN RvNetAddress*  pRtcpAddress,
    IN RvUint32       multiplexerId)
{
    RTPLOG_ENTER(MultiplexingAddRemoteAddress);
    rtcpDemuxAddRemoteAddress(hRTCP, pRtcpAddress, &multiplexerId);
    RTPLOG_LEAVE(MultiplexingAddRemoteAddress);
    return RV_OK;
}

/**************************************************************************************
 * RvRtcpMultiplexingRemoveRemoteAddress
 * description: removes the  RTCP multiplexing address of the remote peer or the address
 *              a multi-cast group or of multi-unicast address list
 *              to which the RTCP stream was sent.
 * input:
 *        hRTCP           - handle to RTCP demultiplexing object
 *        pRtcpAddress    - pRtpAddress of remote RTCP session
 *        multiplexerId   - multiplexID of remote RTCP session.
 * output: none.
 * return value:If an error occurs, the function returns a negative value.
 *              If no error occurs, the function returns a non-negative value.
 **************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtcpMultiplexingRemoveRemoteAddress (
	IN RvRtcpSession  hRTCP,   /* RTCP Session Opaque Handle */
	IN RvNetAddress*  pRtcpAddress,
    IN RvUint32       multiplexerId)
{
    RTPLOG_ENTER(MultiplexingRemoveRemoteAddress);
    rtcpDemuxRemoveRemoteAddress(hRTCP, pRtcpAddress, &multiplexerId);
    RTPLOG_LEAVE(MultiplexingRemoveRemoteAddress);
    return RV_OK;
}

#else

int prevent_warning_of_ranlib_has_no_symbols_artp_rvrtcpnatfw=0;


#endif /* __H323_NAT_FW__ */



#ifdef __cplusplus
}
#endif






