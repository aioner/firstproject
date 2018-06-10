/************************************************************************
 File Name	   : RtpDemux.h
 Description   : header file for RTP demux table related functions
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
#include "RtcpTypes.h"

#ifdef __H323_NAT_FW__

#ifdef __cplusplus
extern "C" {
#endif


#if(RV_LOGMASK != RV_LOGLEVEL_NONE)   
#define RTP_SOURCE      (rtpGetSource(RVRTP_RTP_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_RTP_MODULE))
static  RvRtpLogger      rtpLogManager = NULL;
#define logMgr          (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))
#else
#define logMgr          (NULL)
#define rvLogPtr        (NULL)
#endif
#include "rtpLogFuncs.h"
#undef FUNC_NAME
#define FUNC_NAME(name) "RtpDemux" #name


/************************************************************************************
 * RtpDemuxTablePickVacantEntry
 * description: allocates place in Demux Table, if available.
 * input: 
 *        demuxPtr          - pointer to demux object.
 * output:
 *        vacantEntryPtr    - Pointer to the vacant entry in Demux table
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RvStatus RtpDemuxTablePickVacantEntry(
    IN    RtpDemux*   demuxPtr,
    INOUT RtpDemuxTableEntry** pEntry,
    INOUT RvUint32*   vacantEntryPtr)
{
    RvInt32   location = -1;;
    
    RTPLOG_ENTER(TablePickVacantEntry);
    
    if (demuxPtr == NULL || vacantEntryPtr == NULL)
    {
        RTPLOG_ERROR_LEAVE(TablePickVacantEntry, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    if (RvRaAdd(demuxPtr->demuxTbl, (RAElement*)pEntry, &location) == RV_OK && location >= 0)
    {
        *vacantEntryPtr = (RvUint32)location;
        RTPLOG_LEAVE(TablePickVacantEntry);
        return RV_OK;
    }
    *vacantEntryPtr = 0xFFFFFFFF;
    RTPLOG_ERROR_LEAVE(TablePickVacantEntry, "no vacant table entry");
    return RV_ERROR_OUTOFRESOURCES;
}

/************************************************************************************
 * RtpDemuxGetRtcpSocket
 * description: retrieves RTCP socket value.
 * input: 
 *        demuxPtr          - pointer to demux object.
 * output:
 *        socketPtr         - Pointer to RTCP RvSocket
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RvStatus RtpDemuxGetRtcpSocket(
    IN    RtpDemux*   demuxPtr,
    INOUT RvSocket*   socketPtr)
{
    RvStatus rv;
    RTPLOG_ENTER(GetRtcpSocket);
    if (demuxPtr == NULL || socketPtr == NULL)
    {
        RTPLOG_ERROR_LEAVE(GetRtcpSocket, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    RvLockGet(&demuxPtr->lock, logMgr);
    rv = RvTransportGetOption(demuxPtr->rtcpTransport,
            RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT, RVTRANSPORT_OPT_SOCKET,
            (void*)socketPtr);
    if (rv != RV_OK)
    {
        RTPLOG_ERROR_LEAVE(GetRtcpSocket, "Bad Transport");
        RvLockRelease(&demuxPtr->lock, logMgr);
        return rv;
    }
    RvLockRelease(&demuxPtr->lock, logMgr);
    RTPLOG_LEAVE(GetRtcpSocket);
    return RV_OK;
}


/************************************************************************************
 * RtpDemuxGetRTCPsession
 * description: retrieves RTCP session handle by multiplexID value.
 * input: 
 *        demuxPtr          - pointer to demux object.
 *        buf               - pointer to RvRtpBuffer structure
 * output:
 *        bytesReceivedPtr  - pointer to received bytes in buf
 *        pHRTCP            - Pointer to RTCP session handle
 * return value:  If no error occurs, the function returns the non-neagtive value.
 *                Otherwise, it returns a negative value.
 ***********************************************************************************/
RvStatus RtpDemuxGetRTCPsession(
                                IN    RtpDemux*        demuxPtr,
                                INOUT RvRtpBuffer*     buf,
                                INOUT RvSize_t*        bytesReceivedPtr,
                                INOUT RvRtcpSession*   pHRTCP)
{
    RvUint32  *multiplexIDPtr = (RvUint32*)buf->buffer;
    RvUint32  demuxTblEntry = 0xFFFFFFFF;
    RvRtcpSession hRTCP = NULL;
    RtpDemuxTableEntry* pEntry = NULL; 

    RTPLOG_ENTER(GetRTCPsession);
    if (demuxPtr == NULL || pHRTCP == NULL)
    {
        RTPLOG_ERROR_LEAVE(GetRTCPsession, "NULL pointer");
        return RV_ERROR_NULLPTR;
    }
    RvLockGet(&demuxPtr->lock, logMgr);
    ConvertFromNetwork(buf->buffer, 0, 1);
    demuxTblEntry = getIndexFromMultiplexID(*multiplexIDPtr);
    pEntry = (RtpDemuxTableEntry*)RvRaGet(demuxPtr->demuxTbl, demuxTblEntry);
    if (pEntry != NULL &&
        ((hRTCP = pEntry->hRTCP) != NULL) &&
        (pEntry->multiplexID == *multiplexIDPtr) &&
        (((rtcpSession *) (hRTCP))->demuxTblEntry == demuxTblEntry))
    {
        *pHRTCP = hRTCP;
        buf->buffer       += RvRtpNatMultiplexIdSize();
        buf->length       -= RvRtpNatMultiplexIdSize();        
        *bytesReceivedPtr -= RvRtpNatMultiplexIdSize();
        RvLockRelease(&demuxPtr->lock, logMgr);
        RTPLOG_LEAVE(GetRTCPsession);
        return RV_OK;
    }
    RvLockRelease(&demuxPtr->lock, logMgr);
    RTPLOG_ERROR_LEAVE(GetRTCPsession, "Cannot find RTCP session");
    return RV_ERROR_UNKNOWN;
}


#ifdef __cplusplus
}
#endif

#else

int prevent_warning_of_ranlib_has_no_symbols_artp_rtpdemux=0;

#endif /* #ifdef __H323_NAT_FW__ */
