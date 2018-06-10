/************************************************************************
 File Name	   : RtpDemux.h
 Description   : scope: private
                 RTP/RTCP demultiplexer structures definitions, 
                 acessory and auxilary functions
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

#ifndef __RTP_DEMUX_H__
#define __RTP_DEMUX_H__

#include "rvra.h"
#include "rvlock.h"
#include "rvmemory.h"
#include "rvsocket.h"
#include "rvnetaddress.h"
#include "rtp.h"
#include "rtcp.h"
#include "rtputil.h"

#ifdef __H323_NAT_FW__

#ifdef __cplusplus
extern "C" {
#endif

#define getIndexFromMultiplexID(x)   (x & 0x0000FFFF)
#define getMultiplexID(runningCounter, vacantEntry)   (((runningCounter<<16) &0xFFFF0000)|(vacantEntry))
/* Demux table entry */
typedef struct 
{
   RvRtpSession  hRTP;         /* RTP session handle */
   RvRtcpSession hRTCP;        /* RTCP session handle */
   RvUint32      multiplexID;  /* allocated multiplexID */
} RtpDemuxTableEntry;


typedef  struct 
{
    RvLock                    lock;                 /* lock to defend this object           */
/* RTP related parameters */
    RvTransport               rtpTransport;         /* transport of RTP Multiplexing session */
    RvRtpDemuxEventHandler_CB rtpEvHandler;         /* RTP received message indication event handler */
    void *                    rtpContext;           /* RTP received message indication event handler context */
    RvNetAddress              rtpAddress;           /* local address of RTP multi session   */
    RvInt32                   rtpSessionsCounter;   /* RTP sub sessions counter             */
/* RTCP related parameters */    
    RvTransport               rtcpTransport;        /* transport of RTP Multi session       */
    RvSelectEngine*           rtcpSelectEngine;     /* RTCP select engine                   */
    RvNetAddress              rtcpAddress;          /* local address of RTCP multi session  */
    RvInt32                   rtcpSessionsCounter;  /* RTP sub sessions counter             */
/* common parameters       */
    RvUint32                  maxSessions;          /* max session number                   */
    RvUint32                  runningCounter;       /* auxiliary running counter            */
    HRA                       demuxTbl;             /* pointer to array of multiplexed
                                                       sessions information. This array
                                                       helps to find hRTP or hRTCP according
                                                       to multiplexID                       */
} RtpDemux;

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
    INOUT RvUint32*   vacantEntryPtr);

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
    INOUT RvSocket*   socketPtr);


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
     INOUT RvSize_t*         bytesReceivedPtr,
     INOUT RvRtcpSession*   pHRTCP);

RVAPI
RvRtpSession RVCALLCONV RtpDemuxOpenFrom(
    IN  RvRtpDemux    demux,
    IN  RvNetAddress* pRtpAddress,
    IN  RvUint32      ssrcPattern,
    IN  RvUint32      ssrcMask,
    IN  void*         buffer,
    IN  RvInt32       bufferSize);

#ifdef __cplusplus
}
#endif

#endif  /* __RTP_DEMUX_H__ */

#endif /* __H323_NAT_FW__ */
