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

#ifndef __RTCP_BASETYPES_H__
#define __RTCP_BASETYPES_H__

#include "rvtypes.h"
#include "rvrtpconfig.h"
#include "rvnettypes.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAXRTCPPACKET             (1470)
#define RTCP_WORKER_BUFFER_SIZE   (1600)
#define SIZEOF_RTCPHEADER         (sizeof(RvUint32) * 2)
#define DATA_PREFIX_LEN			  8                      /* 8 bytes before RTCP pack: 4 bytes - for encryption
	                                                        and another 4 bytes for NAT/FW */
#define reduceNNTP(a) (RvUint64ToRvUint32(RvUint64And(RvUint64ShiftRight(a, 16), RvUint64Const(0, 0xffffffff))))



typedef enum __rtcpType
{
	RTCP_SR		= 200,              /* sender report               */
	RTCP_RR		= 201,              /* receiver report             */
	RTCP_SDES	= 202,              /* source description items    */
	RTCP_BYE	= 203,              /* end of participation        */
	RTCP_APP	= 204,              /* application specific		   */
	RTCP_RTPFB	= 205,				/* transport layer FB message  */
	RTCP_PSFB	= 206,				/* payload specific FB message */
	RTCP_XR		= 207,              /* RTCP extended reports (RFC 3611) */
	RTCP_LAST_LEGAL  = RTCP_XR,     /* application specific        */
	RTCP_EMPTY_RR = 1				/* empty RR                    */
} RtcpType;


typedef enum RvRtcpSDesType__
{
	RV_RTCP_SDES_END   = 0,
	RV_RTCP_SDES_CNAME = 1,
	RV_RTCP_SDES_NAME  = 2,
	RV_RTCP_SDES_EMAIL = 3,
	RV_RTCP_SDES_PHONE = 4,
	RV_RTCP_SDES_LOC   = 5,
	RV_RTCP_SDES_TOOL  = 6,
	RV_RTCP_SDES_NOTE  = 7,
	RV_RTCP_SDES_PRIV  = 8
} RvRtcpSDesType;


typedef struct
{
   RvUint64     tNNTP;
   RvUint32     tRTP;
   RvUint32     nPackets;
   RvUint32     nBytes;
} rtcpSR;

typedef struct
{
   RvUint32 ssrc;
   RvUint32 bfLost;      		   /* 8Bit fraction lost and 24 bit cumulative lost */
   RvUint32 nExtMaxSeq;
   RvUint32 nJitter;
   RvUint32 tLSR;				   /* time of Last Sender Report */
   RvUint32 tDLSR;				   /* Delay since Last Sender Report */
} rtcpRR;

typedef struct
{
   RvUint16 max_seq;               /* highest seq. number seen */
   RvUint32 cycles;                /* shifted count of seq. number cycles */
   RvUint32 base_seq;              /* base seq number */
   RvUint32 bad_seq;               /* last 'bad' seq number + 1 */
   RvUint32 probation;             /* sequ. packets till source is valid */
   RvUint32 received;              /* packets received */
   RvUint32 expected_prior;        /* packet expected at last interval */
   RvUint32 received_prior;        /* packet received at last interval */
   RvUint32 transit;               /* relative trans time for prev packet */
   RvUint32 jitter;                /* estimated jitter */
   /* ... */
} rtpSource;

typedef struct
{
   RvUint8  type;
   RvUint8  length;
   char     value[RTCP_MAXSDES_LENGTH + 1];     /* leave a place for an asciiz */
} rtcpSDES;


typedef struct __rtcpInfo
{
   RvInt32        invalid;
   RvBool     active;        /* received packet from SSRC (field ssrc) */
   rtpSource  src;
   RvAddress remoteAddress;

   RvUint32   ssrc;
   RvUint32   tLSRmyTime;
   RvUint32   tLastReport;  /* time when last reportwas received from this member (in msec) */
   rtcpSR     eSR;
   rtcpRR     eToRR;
   rtcpRR     eFromRR;
   rtcpSDES   eSDESCname;

#ifdef __RTCP_XR__
   void*      eXR;
#endif

#ifdef __RTCP_FB__
   void*      fbMessage;
#endif

} rtcpInfo;

typedef struct __rtcpHeader
{
   RvUint32 bits;
   RvUint32 ssrc;
} rtcpHeader;


typedef struct
{
   RvInt32   active;
   RvInt32   collision;
   RvUint32  ssrc;
   RvUint32  timestamp;
   RvUint32  rtpClockRate;    /* clock rate of RTP media */
   RvUint8   rtpPayloadType;  /* payloadtype of RTP media */
   RvUint64  lastPackNTP;     /* last sent packet NTP time */
   RvUint32  lastPackRTPts;   /* last sent packet RTP timestamp */
   rtcpSR    eSR;
   rtcpSDES  SDESbank[RV_RTCP_SDES_PRIV+1];
} rtcpMyInfo;

typedef struct
{
   RvBool    initial;				/* first time initialization */
   RvInt64   previousTime;			/* time of last scheduled transmission */
   RvInt64   nextTime;				/* time of next scheduled transmission */
   RvUint32	 baseTimeInterval;		/* time interval in milliseconds to send next RTCP without RANDomisation */
   RvUint32  pmembers;				/* number of members at time nextTransmitTime was computed */
   RvUint32  members;				/* current number of members */
   RvUint32  senders;				/* current number of senders */
   RvUint32  rtcpBandwidth;			/* bandwidth for RTCP session */
   RvUint32  rtcpSenderBandwidth;	/* bandwidth for RTCP session */
   RvUint32  rtcpReceiverBandwidth;	/* bandwidth for RTCP session */
   RvInt32   lastRTCPPacketSize;
   RvInt32   aveRTCPPacketSize;
} rtcpTxInterval;


#ifdef __cplusplus
}
#endif

#endif  /* __RTCP_TYPES_H__ */







