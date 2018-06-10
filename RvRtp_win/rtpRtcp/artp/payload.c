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
#include "rverror.h"
#include "rvrtpconfig.h"
#include "bitfield.h"
#include "rtp.h"
#include "rtputil.h"
#include "payload.h"
#include "rvassert.h"

#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
#include <stdio.h>
#endif

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)
#define RTP_SOURCE      (rtpGetSource(RVRTP_PAYLOAD_MODULE))
#define rvLogPtr        (rtpGetSource(RVRTP_PAYLOAD_MODULE))
#else
#define rvLogPtr        (NULL)
#endif
#define logMgr          (NULL)

#include "rtpLogFuncs.h"
#undef FUNC_NAME
#define FUNC_NAME(name) "RvRtp" #name

#ifdef __cplusplus
extern "C" {
#endif

/* compliant to RFC 3551 */
static const RvInt32 standardPayload[] = {
	    8000, /* PCMU */
		8000, /* 1016 */
		8000, /* G726-32 */
		8000, /* GSM */
		8000, /* G7231 */
		8000, /* DVI4 */
		16000, /* DVI4 */
		8000, /* LPC */
		8000, /* PCMA */
		8000, /* G722 */
		41100, /* L16 */
		41100, /* L16 */
		8000, /* QCELP */
		8000, /* Comfort Noise */
		90000, /* MPA */
		8000, /* G728 */
		11025, /* DVI4 */
		22050, /* DVI4 */
		8000, /* G729 */
		0, /* Reserved */
		0, /* Unassigned */
		0, /* Unassigned */
		0, /* Unassigned */
		0, /* Unassigned */
		0, /* Unassigned */
		90000, /* CelB */
		90000, /* JPEG */
		0, /* Unassigned */
		90000, /* nv */
		0, /* Unassigned */
		0, /* Unassigned */
		90000, /* H261 */
		90000, /* MPV */
		90000, /* MP2T */
		90000  /* H263 */
};

static const char* standardPayloadNames[] = {
	    "G711-PCMU",
		"1016",
		"G726-32",
		"GSM",
		"G723.1",
		"DVI4-8000",
		"DVI4-16000",
		"LPC",
		"G711-PCMA",
		"G722",
		"L16-2Ch",
		"L16-1Ch",
		"QCELP",
		"Comfort Noise",
		"MPA",
		"G728",
		"DVI4-11025",
		"DVI4-22050",
		"G729",
		NULL, /* Reserved */
		NULL, /* Unassigned */
		NULL, /* Unassigned */
		NULL, /* Unassigned */
		NULL, /* Unassigned */
		NULL, /* Unassigned */
		"CelB",
		"JPEG",
		NULL, /* Unassigned */
		"nv",
		NULL, /* Unassigned */
		NULL, /* Unassigned */
		"H261",
		"MPV",
		"MP2T",
		"H263"
};

static const int standardPayloadSize = sizeof(standardPayload)/sizeof(RvInt32);

/* Standard payload functions */
RVAPI
RvInt32 RVCALLCONV RvRtpGetStandardPayloadClockRate(IN RvInt32 PayloadType)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadClockRate"));
	if (PayloadType >= standardPayloadSize || PayloadType < 0)
	{
	    RvLogWarning(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadClockRate: wrong payload type or dynamic payload type."));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadClockRate"));
		return 0;
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadClockRate"));
	return standardPayload[PayloadType];
}

RVAPI
const char* RVCALLCONV RvRtpGetStandardPayloadName(RvInt32 PayloadType)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadName"));
	if (PayloadType >= standardPayloadSize || PayloadType < 0)
	{
		RvLogWarning(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadName: wrong payload type or dynamic payload type."));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadName"));
		return NULL;
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpGetStandardPayloadName"));
	return standardPayloadNames[PayloadType];
}


RVAPI
RvStatus RVCALLCONV RvRtpPCMUPack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPCMUPack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);

    p->payload=RV_RTP_PAYLOAD_PCMU;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPCMUPack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpPCMUUnpack(
                                OUT void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPCMUUnpack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPCMUUnpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpPCMUGetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPCMUGetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPCMUGetHeaderLength"));
    return RvRtpGetHeaderLength();
}

RVAPI
RvInt32 RVCALLCONV RvRtpPCMAPack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPCMAPack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    p->payload=RV_RTP_PAYLOAD_PCMA;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPCMAPack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpPCMAUnpack(
                                OUT     void*buf,
                                IN  RvInt32 len,
                                OUT     RvRtpParam*p,
                                OUT void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPCMAUnpack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPCMAUnpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpPCMAGetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPCMAGetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPCMAGetHeaderLength"));
    return RvRtpGetHeaderLength();
}

RVAPI
RvInt32 RVCALLCONV RvRtpG722Pack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG722Pack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    p->payload=RV_RTP_PAYLOAD_G722;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG722Pack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG722Unpack(
                                OUT     void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG722Unpack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG722Unpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG722GetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG722GetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG722GetHeaderLength"));
    return RvRtpGetHeaderLength();
}

RVAPI
RvInt32 RVCALLCONV RvRtpG728Pack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG728Pack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    p->payload=RV_RTP_PAYLOAD_G728;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG728Pack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG728Unpack(
                                OUT void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG728Unpack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG728Unpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG728GetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG728GetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG728GetHeaderLength"));
    return RvRtpGetHeaderLength();
}

RVAPI
RvInt32 RVCALLCONV RvRtpG729Pack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG729Pack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);

    p->payload=RV_RTP_PAYLOAD_G729;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG729Pack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG729Unpack(
                                OUT void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG729Unpack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG729Unpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG729GetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG729GetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG729GetHeaderLength"));
    return RvRtpGetHeaderLength();
}

RVAPI
RvInt32 RVCALLCONV RvRtpG7231Pack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG7231Pack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);

    p->payload=RV_RTP_PAYLOAD_G7231;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG7231Pack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG7231Unpack(
                                OUT void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG7231Unpack"));
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG7231Unpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpG7231GetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpG7231GetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpG7231GetHeaderLength"));
    return RvRtpGetHeaderLength();
}


RVAPI
RvInt32 RVCALLCONV RvRtpH261Pack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
    RvRtpPayloadH261*h261=(RvRtpPayloadH261*)param;
    RvUint32*hPtr;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH261Pack"));

    p->sByte-=4;
    hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
    hPtr[0]=bitfieldSet(0,h261->v,24,1);
    hPtr[0]=bitfieldSet(hPtr[0],h261->i,25,1);

    RV_UNUSED_ARG(len);

    if (h261->gobN)
    {
        hPtr[0]=bitfieldSet(hPtr[0],h261->vMvd,0,5);
        hPtr[0]=bitfieldSet(hPtr[0],h261->hMvd,5,5);
        hPtr[0]=bitfieldSet(hPtr[0],h261->quant,10,5);
        hPtr[0]=bitfieldSet(hPtr[0],h261->mbaP,15,5);
        hPtr[0]=bitfieldSet(hPtr[0],h261->gobN,20,4);
    }
    hPtr[0]=bitfieldSet(hPtr[0],h261->eBit,26,3);
    hPtr[0]=bitfieldSet(hPtr[0],h261->sBit,29,3);
    p->payload=RV_RTP_PAYLOAD_H261;

    ConvertToNetwork(hPtr, 0, 1);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH261Pack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpH261Unpack(
                                OUT void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
    RvRtpPayloadH261*h261=(RvRtpPayloadH261*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH261Unpack"));
	RV_UNUSED_ARG(len);
    p->sByte += 4;
    ConvertFromNetwork(hPtr, 0, 1);

    h261->vMvd=bitfieldGet(hPtr[0],0,5);
    h261->hMvd=bitfieldGet(hPtr[0],5,5);
    h261->quant=bitfieldGet(hPtr[0],10,5);
    h261->mbaP=bitfieldGet(hPtr[0],15,5);
    h261->gobN=bitfieldGet(hPtr[0],20,4);
    h261->v=bitfieldGet(hPtr[0],24,1);
    h261->i=bitfieldGet(hPtr[0],25,1);
    h261->eBit=bitfieldGet(hPtr[0],26,3);
    h261->sBit=bitfieldGet(hPtr[0],29,3);

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH261Unpack"));
    return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpH261GetHeaderLength()
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH261GetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH261GetHeaderLength"));
    return RvRtpGetHeaderLength()+4;
}


RvInt32 RVCALLCONV RvRtpH263Pack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
    RvRtpPayloadH263*h263=(RvRtpPayloadH263*)param;
    RvUint32*hPtr=NULL;
    RvInt32 dwords=0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH263Pack"));
    RV_UNUSED_ARG(len);

    if (h263->p)
    {
        dwords=1;
        p->sByte-=4;
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

        hPtr[0]=bitfieldSet(0,h263->dbq,11,2);
        hPtr[0]=bitfieldSet(hPtr[0],h263->trb,8,3);
        hPtr[0]=bitfieldSet(hPtr[0],h263->tr,0,8);
    }

    if (h263->f)
    {
        dwords+=2;
        p->sByte-=8;
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
        hPtr[0]=bitfieldSet(0,h263->mbaP,0,8);
        hPtr[0]=bitfieldSet(hPtr[0],h263->gobN,8,5);
        hPtr[0]=bitfieldSet(hPtr[0],h263->quant,16,5);


        hPtr[1]=bitfieldSet(      0,h263->vMv2, 0,8);
        hPtr[1]=bitfieldSet(hPtr[1],h263->hMv2, 8,8);
        hPtr[1]=bitfieldSet(hPtr[1],h263->vMv1,16,8);
        hPtr[1]=bitfieldSet(hPtr[1],h263->hMv1,24,8);
    }

    if (!h263->f && !h263->p)
    {
        dwords=1;
        p->sByte-=4;
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
        hPtr[0]=0;
    }

    hPtr[0]=bitfieldSet(hPtr[0],h263->f,31,1);
    hPtr[0]=bitfieldSet(hPtr[0],h263->p,30,1);
    hPtr[0]=bitfieldSet(hPtr[0],h263->sBit,27,3);
    hPtr[0]=bitfieldSet(hPtr[0],h263->eBit,24,3);
    hPtr[0]=bitfieldSet(hPtr[0],h263->src,21,3);
    hPtr[0]=bitfieldSet(hPtr[0],h263->i,15,1);
    hPtr[0]=bitfieldSet(hPtr[0],h263->a,14,1);
    hPtr[0]=bitfieldSet(hPtr[0],h263->s,13,1);
    p->payload=RV_RTP_PAYLOAD_H263;

    ConvertToNetwork(hPtr, 0, dwords);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH263Pack"));
    return RV_OK;
}

RvInt32 RVCALLCONV RvRtpH263Unpack(
                                OUT     void*buf,
                                IN  RvInt32 len,
                                OUT     RvRtpParam*p,
                                OUT void*param)
{
    RvRtpPayloadH263*h263=(RvRtpPayloadH263*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH263Unpack"));
    RV_UNUSED_ARG(len);

    p->sByte+=4;

    ConvertFromNetwork(hPtr, 0, 1);

    h263->f=bitfieldGet(hPtr[0],31,1);
    h263->p=bitfieldGet(hPtr[0],30,1);

    h263->sBit=bitfieldGet(hPtr[0],27,3);
    h263->eBit=bitfieldGet(hPtr[0],24,3);
    h263->src=bitfieldGet(hPtr[0],21,3);
    h263->i=bitfieldGet(hPtr[0],15,1);
    h263->a=bitfieldGet(hPtr[0],14,1);
    h263->s=bitfieldGet(hPtr[0],13,1);

    if (h263->f)
    {
        RvInt32 aDwords=h263->p+1;

        ConvertFromNetwork(hPtr, 1, aDwords);

        p->sByte+=4*aDwords;
        h263->mbaP=bitfieldGet(hPtr[0],0,8);
        h263->gobN=bitfieldGet(hPtr[0],8,5);
        h263->quant=bitfieldGet(hPtr[0],16,5);


        h263->vMv2=bitfieldGet(hPtr[1],0,8);
        h263->hMv2=bitfieldGet(hPtr[1],8,8);
        h263->vMv1=bitfieldGet(hPtr[1],16,8);
        h263->hMv1=bitfieldGet(hPtr[1],24,8);
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte-4);
    }

    if (h263->p)
    {

        h263->dbq=bitfieldGet(hPtr[0],11,2);
        h263->trb=bitfieldGet(hPtr[0],8,3);
        h263->tr=bitfieldGet(hPtr[0],0,8);
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH263Pack"));
    return RV_OK;
}

RvInt32 RVCALLCONV RvRtpH263GetHeaderLength()
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH263GetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH263GetHeaderLength"));
    return RvRtpGetHeaderLength()+12;
}

RvInt32 RVCALLCONV RvRtpH263aPack(
                              IN    void*buf,
                              IN    RvInt32 len,
                              IN    RvRtpParam*p,
                              IN    void*param)
{
    RvRtpPayloadH263a*h263a=(RvRtpPayloadH263a*)param;
    RvUint32*hPtr=NULL;
    RvInt32 dwords=0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH263aPack"));
    RV_UNUSED_ARG(len);

    if (h263a->p)
    {
        dwords=1;
        p->sByte-=4;
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

        hPtr[0]=bitfieldSet(0,h263a->dbq,11,2);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->trb,8,3);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->tr,0,8);
    }

    if (h263a->f)
    {
        dwords+=2;
        p->sByte-=8;
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
        hPtr[0]=bitfieldSet(0,h263a->mbaP,2,9);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->gobN,11,5);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->quant,16,5);


        hPtr[1]=bitfieldSet(      0,h263a->vMv2, 0,7);
        hPtr[1]=bitfieldSet(hPtr[1],h263a->hMv2, 7,7);
        hPtr[1]=bitfieldSet(hPtr[1],h263a->vMv1,14,7);
        hPtr[1]=bitfieldSet(hPtr[1],h263a->hMv1,21,7);

        hPtr[1]=bitfieldSet(hPtr[1],h263a->a,28,1);
        hPtr[1]=bitfieldSet(hPtr[1],h263a->s,29,1);
        hPtr[1]=bitfieldSet(hPtr[1],h263a->u,30,1);
        hPtr[1]=bitfieldSet(hPtr[1],h263a->i,31,1);
    }

    if (!h263a->f && !h263a->p)
    {
        dwords=1;
        p->sByte-=4;
        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
        hPtr[0]=bitfieldSet(      0,h263a->a,17,1);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->s,18,1);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->u,19,1);
        hPtr[0]=bitfieldSet(hPtr[0],h263a->i,20,1); ;
    }

    hPtr[0]=bitfieldSet(hPtr[0],h263a->f,31,1);
    hPtr[0]=bitfieldSet(hPtr[0],h263a->p,30,1);
    hPtr[0]=bitfieldSet(hPtr[0],h263a->sBit,27,3);
    hPtr[0]=bitfieldSet(hPtr[0],h263a->eBit,24,3);
    hPtr[0]=bitfieldSet(hPtr[0],h263a->src,21,3);

    p->payload=RV_RTP_PAYLOAD_H263;

    ConvertToNetwork(hPtr, 0, dwords);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH263aPack"));
    return RV_OK;
}

RvInt32 RVCALLCONV RvRtpH263aUnpack(
                                OUT void*buf,
                                IN  RvInt32 len,
                                OUT RvRtpParam*p,
                                OUT void*param)
{
    RvRtpPayloadH263a*h263a=(RvRtpPayloadH263a*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH263aUnpack"));
    RV_UNUSED_ARG(len);

    p->sByte+=4;

    ConvertFromNetwork(hPtr, 0, 1);

    h263a->f=bitfieldGet(hPtr[0],31,1);
    h263a->p=bitfieldGet(hPtr[0],30,1);

    h263a->sBit=bitfieldGet(hPtr[0],27,3);
    h263a->eBit=bitfieldGet(hPtr[0],24,3);
    h263a->src=bitfieldGet(hPtr[0],21,3);

    if (h263a->f)
    {
        RvInt32 aDwords=h263a->p+1;
        ConvertFromNetwork(hPtr, 1, aDwords);

        p->sByte+=4*aDwords;
        h263a->mbaP=bitfieldGet(hPtr[0],2,9);
        h263a->gobN=bitfieldGet(hPtr[0],11,5);
        h263a->quant=bitfieldGet(hPtr[0],16,5);


        h263a->vMv2=bitfieldGet(hPtr[1],0,7);
        h263a->hMv2=bitfieldGet(hPtr[1],7,7);
        h263a->vMv1=bitfieldGet(hPtr[1],14,7);
        h263a->hMv1=bitfieldGet(hPtr[1],21,7);

        h263a->i=bitfieldGet(hPtr[1],31,1);
        h263a->u=bitfieldGet(hPtr[1],30,1);
        h263a->s=bitfieldGet(hPtr[1],29,1);
        h263a->a=bitfieldGet(hPtr[1],28,1);

        hPtr=(RvUint32*)((RvUint8*)buf+p->sByte-4);
    }

    if (h263a->p)
    {

        h263a->dbq=bitfieldGet(hPtr[0],11,2);
        h263a->trb=bitfieldGet(hPtr[0],8,3);
        h263a->tr=bitfieldGet(hPtr[0],0,8);
    }

    if (!h263a->f && !h263a->p)
    {
      h263a->i=bitfieldGet(hPtr[0],20,1);
      h263a->u=bitfieldGet(hPtr[0],19,1);
      h263a->s=bitfieldGet(hPtr[0],18,1);
      h263a->a=bitfieldGet(hPtr[0],17,1);

    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH263aUnpack"));
    return RV_OK;
}

RvInt32 RVCALLCONV RvRtpH263aGetHeaderLength()
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH263aGetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH263aGetHeaderLength"));
    return RvRtpGetHeaderLength()+12;
}




/* ==== DTMF inBand (via RTP payload) - RFC 2833 ==== */

/************************************************************************
 * RvRtpDtmfEventPack
 *
 * purpose: set the payload format, for sending DTMF events inband,
 *          as described in RFC2833, section 3.5.
 *
 * input  : buf - buffer pointer that will be sent
 *          len - length of the buffer.
 *          p - RTP header default parameters.
 *          param - a structure containing the required parameters for DTMF events.
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDtmfEventPack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param)
{
    RvRtpDtmfEventParams *dtmf = (RvRtpDtmfEventParams *)param;
    RvUint8*hPtr;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDtmfEventPack"));

    RV_UNUSED_ARG(len);

    p->sByte -= 4;
    hPtr = (RvUint8*)buf + p->sByte;

    /* Due to some issues with the ordering of bytes that wasn't easy to fix using
       "conventional" means we deal with this as a bytes buffer... */
    /*                0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7
       Original:     |     event     |E|R| volume    |          duration             |
    */
    hPtr[0] = (RvUint8) dtmf->event;
    hPtr[1] = (RvUint8)((RvUint8)((dtmf->end & 1) << 7) | (RvUint8)(dtmf->volume & 0x3f));
    hPtr[2] = (RvUint8)((dtmf->duration >> 8) & 0xff);
    hPtr[3] = (RvUint8)(dtmf->duration & 0xff);

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfEventPack"));
    return RV_OK;
}


/************************************************************************
 * RvRtpDtmfEventUnpack
 *
 * purpose: evaluates the DTMF events from the received packed.
 *
 * input  : len - length of the buffer.
 * output : buf - the received buffer.
 *          p - RTP header parameters that were received.
 *          param - the received parameters for DTMF events.
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDtmfEventUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param)
{
    RvRtpDtmfEventParams *dtmf = (RvRtpDtmfEventParams *)param;
    RvUint8*hPtr = (RvUint8*)buf + p->sByte;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDtmfEventUnpack"));
    RV_UNUSED_ARG(len);

    p->sByte += 4;

    /* See Pack() function for explanation on ordering */
    dtmf->event = (RvRtpDtmfEvent)hPtr[0];
    dtmf->end = (RvBool)(hPtr[1] >> 7);
    dtmf->volume = (RvUint8)(hPtr[1] & 0x3f);
    dtmf->duration = ((RvUint32)hPtr[2] << 8) | (RvUint32)hPtr[3];
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfEventUnpack"));
    return RV_OK;
}


/******************************************************************************
 * RvRtpDtmfEventGetHeaderLength
 * ----------------------------------------------------------------------------
 * General: Returns the length of a DTMF event payload.
 *          This length should be placed as the len parameter to RvRtpWrite().
 *
 * Return Value: Length of a DTMF event payload.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  none.
 * Output: none.
 *****************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDtmfEventGetHeaderLength(void)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDtmfEventGetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfEventGetHeaderLength"));
    return RvRtpGetHeaderLength() + 4;
}


/* Telephony Tones according to RFC2833 */

/************************************************************************
 * RvRtpDtmfTonesPack
 *
 * purpose: set the payload format, for sending telephony tones inband,
 *          as described in RFC2833, section 4.4.
 *
 * input  : buf - buffer pointer that will be sent
 *          len - length of the buffer.
 *          p - RTP header default parameters.
 *          param - a structure containing the required parameters for telephony tones.
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 *
 * Important: one of the members of the 'RvRtpDtmfTonesParams' struct(param) is a pointer
 * notes      to an integer array, that symbolizes the frequencies that form the tone.
 *            the array is not limited in size, since a single tone can contain any
 *            number of frequencies. this is the reason there is a member 'freqListLength'
 *            in the struct as well.
 ************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDtmfTonesPack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param)
{
    RvRtpDtmfTonesParams *dtmf = (RvRtpDtmfTonesParams *)param;
    RvUint8* hPtr;
    RvUint32 listLen = 0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesPack"));

    RV_UNUSED_ARG(len);

    /* We place the bytes in "wrong order" - the ConvertToNetwork function
       is going to change the bytes position anyway... */
    /*                0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7
       Original:     |    modulation   |T|  volume   |          duration             |
    */

    p->sByte -= (4 + (((dtmf->freqListLength + 1) / 2) * 4));
    hPtr = (RvUint8*)buf + p->sByte;

    hPtr[0] = (RvUint8)((dtmf->modulation >> 1) & 0xff);
    hPtr[1] = (RvUint8)(((dtmf->modulation & 1) << 7) |
                        ((dtmf->T & 1)<< 6) |
                        (dtmf->volume & 0x3f));
    hPtr[2] = (RvUint8)((dtmf->duration >> 8) & 0xff);
    hPtr[3] = (RvUint8)(dtmf->duration & 0xff);

    hPtr += 4;

    while (listLen < dtmf->freqListLength)
    {
        /*                0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7
           Original:     |R R R R|       frequency       |R R R R|       frequency       |
        */
        hPtr[0] = (RvUint8)((dtmf->freqList[listLen] >> 8) & 0x0f);
        hPtr[1] = (RvUint8)(dtmf->freqList[listLen] & 0xff);
        hPtr += 2;
        listLen++;
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesPack"));
    return RV_OK;
}


/************************************************************************
 * RvRtpDtmfTonesUnpack
 *
 * purpose: evaluates the telephony tones from the received packed.
 *
 * input  : len - length of the buffer.
 * output : buf - the received buffer.
 *          p - RTP header parameters that were received.
 *          param - the received parameters for telephony tones.
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpDtmfTonesUnpack(
        IN   void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param)
{
    RvRtpDtmfTonesParams *dtmf = (RvRtpDtmfTonesParams *)param;
    RvUint8*hPtr = (RvUint8*)buf + p->sByte;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesUnpack"));

    RV_UNUSED_ARG(len);

    /* set the offset right after the tone's first header,
       and before the frequencies list */
    p->sByte += 4;

    /* See Pack() function for explanation on ordering */
    dtmf->modulation = (RvUint16)((hPtr[1] >> 7) | ((RvUint16)hPtr[0] << 1));
    /*dtmf->T = (RvBool)((hPtr[2] & 0x40) != 0);*/
    dtmf->T = (RvBool)((hPtr[1] >> 6) & 0x1);

    dtmf->volume = (RvUint8)(hPtr[1] & 0x3f);
    dtmf->duration = ((RvUint32)hPtr[2] << 8) | (RvUint32)hPtr[3];

    p->len -= 16; /* removing RTP header and 4 first bytes of telephony tones header */
    p->len >>= 1; /* find out of many frequencies are there in the payload - divide by 2 */
    dtmf->freqListLength = p->len;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesUnpack"));
    return RV_OK;
}


/************************************************************************
 * RvRtpDtmfTonesGetByIndex
 *
 * purpose: find the requested frequency in the received message.
 *
 * input  : buf - the received buffer.
 *          index - index of the frequency inside the frequency list.
 *          p - RTP header parameters that were received.
 *          param - the received parameters for telephony tones.
 * output : frequency - The requested frequency
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDtmfTonesGetByIndex(
        IN   void *      buf,
        IN   RvUint32    index,
        IN   RvRtpParam *  p,
        IN   void *      param,
        OUT  RvUint16 *  frequency)
{
    RvRtpDtmfTonesParams *dtmf = (RvRtpDtmfTonesParams *)param;
    RvUint8 *hPtr = (RvUint8*)buf + p->sByte;
    RvUint32 level;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesGetByIndex"));

	if (index > dtmf->freqListLength)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesGetByIndex(): wrong index"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesGetByIndex"));
        return RV_ERROR_OUTOFRANGE;
	}
    level = index << 1;

    /* Get the right frequency from inside the header */
    *frequency = (RvUint16)(((((RvUint16)hPtr[level]) << 8) | hPtr[level+1]) & 0x0fff);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDtmfTonesGetByIndex"));
    return RV_OK;
}



/* ==== Annex Q & H.281 - Far End Camera Control (FECC) ==== */

/************************************************************************
 * RvRtpAnnexQMessagePack
 *
 * purpose: Set the payload format, for sending Far end camera commands inband,
 *          as described in H.281.
 *          Several AnnexQ commands can be sent in the same packet. Using
 *          RvRtpAnnexQMessagePack on the same buffer for several such messages
 *          places them in reverse order.
 *
 * input  : buf - buffer pointer that will be sent
 *          len - length of the buffer.
 *          p - RTP header default parameters.
 *          param - a structure containing the required parameters for FECC.
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpAnnexQMessagePack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  params)
{
    RvRtpAnnexQParam *param = (RvRtpAnnexQParam *)params;
    RvUint8*hPtr;
    RvInt32 length = 2;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpAnnexQMessagePack"));

    RV_UNUSED_ARG(len);

    p->sByte-=2;
    if (param->procedure == RVRTP_ANNEXQ_PROCEDURES_START_ACTION)
        p->sByte-=1;

    hPtr=(RvUint8*)((RvUint8*)buf+p->sByte);
    hPtr[0]=(RvUint8) param->procedure;

    switch (param->procedure)
    {
    case RVRTP_ANNEXQ_PROCEDURES_START_ACTION:
        length++;
        hPtr[2]=(RvUint8)bitfieldSet(0, param->timeOut, 0, 4);    /* set timeout */
        /* No break - we continue as if it's a continue or a stop action */

    case RVRTP_ANNEXQ_PROCEDURES_CONTINUE_ACTION:
    case RVRTP_ANNEXQ_PROCEDURES_STOP_ACTION:
        hPtr[1]=(RvUint8)bitfieldSet(0, param->action.pan, 6, 2);        /* set pan   */
        hPtr[1]=(RvUint8)bitfieldSet(hPtr[1], param->action.tilt, 4, 2); /* set tilt  */
        hPtr[1]=(RvUint8)bitfieldSet(hPtr[1], param->action.zoom, 2, 2); /* set zoom  */
        hPtr[1]=(RvUint8)bitfieldSet(hPtr[1], param->action.focus, 0, 2);/* set focus */
        break;

    case RVRTP_ANNEXQ_PROCEDURES_SELECT_VIDEO_SOURCE:
    case RVRTP_ANNEXQ_PROCEDURES_VIDEO_SOURCE_SWITCHED:
        hPtr[1]=(RvUint8)bitfieldSet(0, param->videoSource, 4, 4);    /* set video source */
        hPtr[1]=(RvUint8)bitfieldSet(hPtr[1], param->mode.stillImage, 1, 1); /* set M1 */
        hPtr[1]=(RvUint8)bitfieldSet(hPtr[1], param->mode.doubleResolutionStillImage, 0, 1); /* set M0 */
        break;

    case RVRTP_ANNEXQ_PROCEDURES_STORE_AS_PRESET:
    case RVRTP_ANNEXQ_PROCEDURES_ACTIVATE_PRESET:
        hPtr[1]=(RvUint8)bitfieldSet(0, param->preset, 4, 4);     /* set preset */
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAnnexQMessagePack"));
    /*ConvertToNetwork(hPtr, 0, 1); Already in network format */
    return RV_OK;
}


/************************************************************************
 * RvRtpAnnexQMessageUnpack
 *
 * purpose: evaluates the FECC commands from the received packed.
 *
 * input  : len - length of the buffer.
 * output : buf - the received buffer.
 *          p - RTP header parameters that were received.
 *          param - the received parameters for FECC.
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpAnnexQMessageUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      params)
{
    RvRtpAnnexQParam *param = (RvRtpAnnexQParam *)params;
    RvUint8*hPtr=(RvUint8*)((RvUint8*)buf+p->sByte);

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpAnnexQMessageUnpack"));

    RV_UNUSED_ARG(len);

    p->sByte+=2;
    /* ConvertFromNetwork(hPtr, 0, 1); Read directly from network format */

    param->procedure = (RvRtpAnnexQProcedures)hPtr[0];

    switch (param->procedure)
    {
    case RVRTP_ANNEXQ_PROCEDURES_START_ACTION:
        param->timeOut = (RvUint8)bitfieldGet(hPtr[2],0,4);  /* get timeout */
        p->sByte++;
        /* No break - we continue as if it's a continue or a stop action */

    case RVRTP_ANNEXQ_PROCEDURES_CONTINUE_ACTION:
    case RVRTP_ANNEXQ_PROCEDURES_STOP_ACTION:
        param->action.pan= (RvRtpAnnexQMoveCamera)bitfieldGet(hPtr[1],6,2);    /* get pan */
        param->action.tilt= (RvRtpAnnexQMoveCamera)bitfieldGet(hPtr[1],4,2);   /* get tilt */
        param->action.zoom= (RvRtpAnnexQMoveCamera)bitfieldGet(hPtr[1],2,2);   /* get zoom */
        param->action.focus= (RvRtpAnnexQMoveCamera)bitfieldGet(hPtr[1],0,2);  /* get focus */
        break;

    case RVRTP_ANNEXQ_PROCEDURES_SELECT_VIDEO_SOURCE:
    case RVRTP_ANNEXQ_PROCEDURES_VIDEO_SOURCE_SWITCHED:
        param->videoSource = (RvUint8)bitfieldGet(hPtr[1],4,4);  /* get video source */
        param->mode.stillImage = bitfieldGet(hPtr[1],1,1);  /* get M1 */
        param->mode.doubleResolutionStillImage = bitfieldGet(hPtr[1],0,1);  /* get M0 */
        break;

    case RVRTP_ANNEXQ_PROCEDURES_STORE_AS_PRESET:
    case RVRTP_ANNEXQ_PROCEDURES_ACTIVATE_PRESET:
        param->preset = (RvUint8)bitfieldGet(hPtr[1],4,4);   /* get preset */
        break;
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAnnexQMessageUnpack"));
    return RV_OK;
}

/**********************************************************************
* AMR and AMR-WB RFC 3267
*********************************************************************/
#define RV_RTPAMR_SPEECH_LOST 14
#define RV_RTPAMR_NO_DATA 15
/* Calucate parameters based on current settings. */
static void rvRtpPayloadAMRDoCalcs(RvRtpPayloadAMR* thisPtr)
{
	RvUint32 totalbits;
	int i;
	RvLogEnter(rvLogPtr, (rvLogPtr, "rvRtpPayloadAMRDoCalcs"));
	/* Read TOC to find actual number of frames */
	thisPtr->framecnt = 0;
	for(i = 0; i < (int)thisPtr->tocsize; i++) {
		if((thisPtr->ftypes[i] != RV_RTPAMR_SPEECH_LOST) && (thisPtr->ftypes[i] != RV_RTPAMR_NO_DATA))
			thisPtr->framecnt++;
	}

	totalbits = 4 + (6 * thisPtr->tocsize); /* CMR + TOC entries */
	if(thisPtr->bemode == RV_TRUE) {
		/* Bandwidth Efficient Mode - calculate bit alignment */
		thisPtr->nbits = (RvUint8) (8 - (totalbits % 8)); /* # of data bits sharing last byte of header */
		totalbits += thisPtr->nbits; /* Assume data in last header byte is part of the header */
	} else {
		/* Octet Aligned Mode - add padding and options */
		totalbits += 4; /* CMR padding */
		totalbits += 2 * thisPtr->tocsize; /* TOC entry padding */
		if(thisPtr->interopt == RV_TRUE)
			totalbits += 8; /* ILL and ILP */
		if(thisPtr->crcopt == RV_TRUE)
			totalbits += 8 * thisPtr->framecnt; /* CRCs */
		thisPtr->nbits = 0;
	}

	thisPtr->totalsize = totalbits / 8;
	RvLogLeave(rvLogPtr, (rvLogPtr, "rvRtpPayloadAMRDoCalcs"));
}

#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
RVAPI
RvInt32 RVCALLCONV RvRtpPayloadAMRToString(const RvRtpPayloadAMR* thisPtr,
								 char *buffer,
								 size_t bufferSize,
								 const char *prefix)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRToString"));
	RV_UNUSED_ARG(thisPtr);
	RV_UNUSED_ARG(buffer);
	RV_UNUSED_ARG(bufferSize);
	RV_UNUSED_ARG(prefix);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRToString"));
	return RV_OK;
}
#endif
/************************************************************************
 * RvRtpPayloadAMRCalcArraySizes
 * purpose: This method calculates the size of the table of contents and the number
 * of frames of an RTP packet before it has been unserialized so that the
 * proper size arrays can be allocated and set.
 * {p: Make sure that the mode and any required options are set for the
 * RvRtpPayloadAMR object before this function is called.
 * Parameters
 * input
 *  thisPtr  - The RvRtpPayloadAMR object.
 *  buf      - The data buffer that has the header to calculate the sizes of.
 *  len      - buf length
 * output
 *  tocsize - Location where the size of the table of contents will be stored.
 *  framecount - Location where the number of frames will be stored.
 *
 * see_also: RvRtpPayloadAMRSetArrays
 ************************************************************************/
RVAPI
void RVCALLCONV RvRtpPayloadAMRCalcArraySizes(
								   IN RvRtpPayloadAMR* thisPtr,
								   IN void *      buf,
								   IN RvInt32     len,
								   OUT RvUint32 *tocsize,
								   OUT RvUint32 *framecount)
{
/*	size_t header, length;*/
	RvUint8 data, ftype;
	RvUint32 bedata;
	int bitcount, i;
	RvInt32 Index = 0;
	RvUint8* bufferPtr = (RvUint8*) buf;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRCalcArraySizes"));

	/* Save original position */

/*
		header = rvDataBufferGetAvailableFrontCapacity(bufferPtr);
		length = rvDataBufferGetLength(bufferPtr);*/


	/* Scan table of contents */
	if(thisPtr->bemode == RV_TRUE) {
		/* Bandwidth Efficient Mode */

		/* Skip first 4 bits (CMR) then scan TOC */
		i = 0;
		*framecount = 0;
/*		rvDataBufferReadUint8(bufferPtr, &data);*/
		data = bufferPtr[Index++];
		bitcount = 4;
		bedata = (RvUint32)data;
		do {
			if(bitcount < 6) {
/*				rvDataBufferReadUint8(bufferPtr, &data);*/
				data = bufferPtr[Index++];
				bedata = (bedata << 8) | data;
				bitcount += 8;
			}
			data = (RvUint8)(bedata >> (bitcount - 6));
			bitcount -= 6;
			ftype = (RvUint8) ((data >> 1) & 0xF);
			if((ftype != RV_RTPAMR_SPEECH_LOST) && (ftype != RV_RTPAMR_NO_DATA))
				*framecount += 1;
			i++;
		} while(data &0x20); /* F bit = 0 in last entry */
		*tocsize = (RvUint32)i;

	} else {
		/* Octet Aligned Mode */

		/* Skip CMR byte and (optionally) the Interleave header */
/*		rvDataBufferReadUint8(bufferPtr, &data);*/
		data = bufferPtr[Index++];
		if(thisPtr->interopt == RV_TRUE)
/*			rvDataBufferReadUint8(bufferPtr, &data);*/
			data = bufferPtr[Index++];
		/* Table of Contents */
		i = 0;
		*framecount = 0;
		do {
/*			rvDataBufferReadUint8(bufferPtr, &data);*/
			data = bufferPtr[Index++];
			ftype = (RvUint8) ((data >> 3) & 0xF);
			if((ftype != RV_RTPAMR_SPEECH_LOST) && (ftype != RV_RTPAMR_NO_DATA))
				*framecount += 1;
			i++;
		} while(data & 0x80); /* F bit = 0 in last entry */
		*tocsize = (RvUint32)i;
	}
	RV_UNUSED_ARG(len);
	/* Restore original poition */
/*	rvDataBufferSetDataPosition(bufferPtr, header, length);	*/
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRCalcArraySizes"));
}

/************************************************************************
 * RvRtpPayloadAMRSetArrays
 * description: This method sets arrays that the table of contents information should
 * be stored in by the RvRtpAMRUnpack function. The size of
 * the ftypes and fqual arrays should be at least as big as the size of
 * the table of contents calculated by the rvRtpPayloadAMRCalcArraySizes
 * function. The size of the crcs array should be at least as big as the
 * number of frames caclulated by the RvRtpPayloadAMRCalcArraySizes function.
 * If the CRC option is not being used, crcs is not used and can be set to NULL.
 * Paremeters:
 *      input - output  thisPtr - The RvRtpPayloadAMR object.
 *				input   ftypes  - Array to store frame types.
 *				input   fqual   - Array to store frame quality indicators.
 *				input   crcs    - Array to store CRCs.
 * See also: RvRtpPayloadAMRCalcArraySizes
 ************************************************************************/

RVAPI
void RVCALLCONV RvRtpPayloadAMRSetArrays(RvRtpPayloadAMR* thisPtr,
							  RvUint8 *ftypes,
							  RvUint8 *fqual,
							  RvUint8 *crcs)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRSetArrays"));
	thisPtr->ftypes = ftypes;
	thisPtr->fqual = fqual;
	thisPtr->crcs = crcs;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRSetArrays"));
}

/************************************************************************
 * RvRtpPayloadAMRSetTOC
 * description: This method sets the information to be used for the table of contents.
 * Based on this information, it also calculates the frame count, header
 * size, and bit offsets that will be used. Insure that the arrays pointed
 * to remain valid until the AMR header has been serialized.
 * parameters:
 * input - output thisPtr     - The RvRtpPayloadAMR object.
 *			input tocsize     - Number of items in the table of contents.
 *			input frametypes  - Array of frame types (4 bit value, upper bits ignored).
 *			input qualitybits - Array of frame quality indicators (1 bit value, upper bits ignored).
 ************************************************************************/

RVAPI
void RVCALLCONV RvRtpPayloadAMRSetTOC(RvRtpPayloadAMR* thisPtr,
						   RvUint32 tocsize,
						   RvUint8 *frametypes,
						   RvUint8 *qualitybits)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRSetTOC"));
	thisPtr->tocsize = tocsize;
	thisPtr->ftypes = frametypes;
	thisPtr->fqual = qualitybits;
	rvRtpPayloadAMRDoCalcs(thisPtr);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadAMRSetTOC"));
}

RVAPI
RvStatus RVCALLCONV RvRtpAMRPack(
								 IN  void *      buf,
								 IN  RvInt32     len,
								 IN  RvRtpParam *  p,
								 IN  void     *  param)
{
	RvRtpPayloadAMR* thisPtr = (RvRtpPayloadAMR*) param;
	RvUint8* bufferPtr = ((RvUint8*) buf);
	RvUint8 data, fbit;
	RvUint32 bedata;
	int bitcount, i;
	RvUint8 *dataPtr;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpAMRPack"));
	/* Writing backwards makes things easier, so find last byte of header. */
	dataPtr = (RvUint8 *)bufferPtr + thisPtr->totalsize - 1;

	if(thisPtr->bemode == RV_TRUE) {
		/* Bandwidth Efficient Mode */

		/* Start with carried over bits from first data byte */
		bedata = (RvUint32)thisPtr->xval;
		bitcount = thisPtr->nbits;

		if(thisPtr->tocsize > 0) {
			/* Table of Contents */
			fbit = RvUint8Const(0);  /* Last entry, F=0 */
			i = thisPtr->tocsize;
			do {
				i--;
				data = (RvUint8)(fbit | (thisPtr->ftypes[i] << 1) | thisPtr->fqual[i]);
				bedata |= ((RvUint32)data << bitcount);
				bitcount += 6;
				if(bitcount >= 8) {
					*(dataPtr--) = (RvUint8)(bedata & RvUint32Const(0xFF));
					bedata = bedata >> 8;
					bitcount -= 8;
				}
				fbit = RvUint8Const(0x20);  /* Other entries, F=1 */
			} while(i > 0);
		}

		/* CMR - Should always end up on byte boundry */
		bedata |= ((RvUint32)thisPtr->cmr << bitcount);
		*dataPtr = (RvUint8)(bedata & RvUint32Const(0xFF));

	} else {
		/* Octet Aligned Mode */

		if(thisPtr->crcopt == RV_TRUE) {
			/* CRC list, one for each frame */
			i = thisPtr->framecnt;
			do {
				i--;
				*(dataPtr--) = thisPtr->crcs[i];
			} while(i > 0);
		}

		if(thisPtr->tocsize > 0) {
			/* Table of Contents */
			fbit = RvUint8Const(0);  /* Last entry, F=0 */
			i = thisPtr->tocsize;
			do {
				i--;
				*(dataPtr--) = (RvUint8)(fbit | (thisPtr->ftypes[i] << 3) | (thisPtr->fqual[i] << 2));
				fbit = RvUint8Const(0x80);  /* Other entries, F=1 */
			} while(i > 0);
		}

		/* Interleave header */
		if(thisPtr->interopt == RV_TRUE)
			*(dataPtr--) = (RvUint8)((thisPtr->ill << 4) | thisPtr->ilp);

		/* CMR */
		*dataPtr = (RvUint8)(thisPtr->cmr << 4);
	}

	p->sByte -= thisPtr->totalsize;
	
	RV_UNUSED_ARG(len);
	RV_UNUSED_ARG(p);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAMRPack"));
	return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtpAMRUnpack(
								   OUT  void *      buf,
								   IN   RvInt32     len,
								   OUT  RvRtpParam *  p,
								   OUT  void *      param)
{
	RvRtpPayloadAMR* thisPtr = (RvRtpPayloadAMR*) param;
	RvUint8* bufferPtr = (RvUint8*) buf + p->sByte;
	RvUint8 data;
	RvUint32 bedata;
	int bitcount, i;
	RvInt32 Index = 0;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpAMRUnpack"));
	/* The ftypes, fqual, and (optionally) crcs arrays must be set before calling this function. */

	if(thisPtr->bemode == RV_TRUE) {
		/* Bandwidth Efficient Mode */
		thisPtr->ill = 0;
		thisPtr->ilp = 0;

		/* CMR */
		data = bufferPtr[Index++];
		thisPtr->cmr = (RvUint8) (data >> 4);
		bitcount = 4;
		bedata = (RvUint32)data;

		/* Table of Contents */
		i = 0;
		thisPtr->framecnt = 0;
		do {
			if(bitcount < 6) {
				data = bufferPtr[Index++];
				bedata = (bedata << 8) | data;
				bitcount += 8;
			}
			data = (RvUint8)(bedata >> (bitcount - 6));
			bitcount -= 6;
			thisPtr->ftypes[i] = (RvUint8) ((data >> 1) & 0xF);
			thisPtr->fqual[i] = (RvUint8) (data & 1);
			if((thisPtr->ftypes[i] != RV_RTPAMR_SPEECH_LOST) && (thisPtr->ftypes[i] != RV_RTPAMR_NO_DATA))
				thisPtr->framecnt++;
			i++;
		} while(data &0x20); /* F bit = 0 in last entry */
		thisPtr->tocsize = (RvUint32)i;

		/* Extra bits from first data byte are left */
		data = (RvUint8)bedata;
		thisPtr->nbits = (RvUint8) bitcount;
		thisPtr->xval = (RvUint8) (data & (~((RvUint8)0xFF << bitcount)));

		/* Calculate total size */
		thisPtr->totalsize = ((thisPtr->tocsize * 6) + (RvUint32)thisPtr->nbits + 4) / 8;

	} else {
		/* Octet Aligned Mode */
		thisPtr->nbits = 0;
		thisPtr->xval = 0;

		/* CMR */
		data = bufferPtr[Index++];
		thisPtr->cmr = (RvUint8) ((data >> 4));
		thisPtr->totalsize = 1;

		if(thisPtr->interopt == RV_TRUE) {
			/* Interleave Header */
			data = bufferPtr[Index++];
			thisPtr->ill = (RvUint8) (data >> 4);
			thisPtr->ilp = (RvUint8) (data & 0xF);
			thisPtr->totalsize++;
		} else {
			thisPtr->ill = 0;
			thisPtr->ilp = 0;
		}

		/* Table of Contents */
		i = 0;
		thisPtr->framecnt = 0;
		do {
			data = bufferPtr[Index++];
			thisPtr->ftypes[i] = (RvUint8) ((data >> 3) & 0xF);
			thisPtr->fqual[i] = (RvUint8) (((data >> 2) & 1));
			if((thisPtr->ftypes[i] != RV_RTPAMR_SPEECH_LOST) && (thisPtr->ftypes[i] != RV_RTPAMR_NO_DATA))
				thisPtr->framecnt++;
			i++;
		} while(data & 0x80); /* F bit = 0 in last entry */
		thisPtr->tocsize = (RvUint32)i;
		thisPtr->totalsize += thisPtr->tocsize;

		if(thisPtr->crcopt == RV_TRUE) {
			/* CRC list, one for each frame */
			for(i = 0; i < (int)thisPtr->framecnt; i++)
				thisPtr->crcs[i] = bufferPtr[Index++];
			thisPtr->totalsize += thisPtr->framecnt;

		}
	}

	p->sByte += thisPtr->totalsize;
    

	RV_UNUSED_ARG(len);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAMRUnpack"));
	return RV_OK;
}

RVAPI
RvInt32 RVCALLCONV RvRtpAMRGetHeaderLength(RvRtpPayloadAMR* thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpAMRGetHeaderLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAMRGetHeaderLength"));
	return RvRtpGetHeaderLength() + thisPtr->totalsize;
}

RVAPI
RvStatus RVCALLCONV RvRtpAMRPayloadInit(RvRtpPayloadAMR* thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpAMRPayloadInit"));

	if (NULL == thisPtr)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpAMRPayloadInit(): NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAMRPayloadInit"));
		return RV_ERROR_NULLPTR;
	}
	thisPtr->bemode = RV_FALSE;
	thisPtr->interopt = RV_FALSE;
	thisPtr->crcopt = RV_FALSE;
	thisPtr->cmr = 0;
	thisPtr->ill = 0;
	thisPtr->ilp = 0;
	thisPtr->nbits = 0;
	thisPtr->xval = 0;
	thisPtr->tocsize = 0;
	thisPtr->framecnt = 0;
	thisPtr->totalsize = 0;
	thisPtr->ftypes = NULL;
	thisPtr->fqual = NULL;
	thisPtr->crcs = NULL;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpAMRPayloadInit"));
	return RV_OK;
}

/********************************************************************************
 * RvRtpMpeg4Construct - the RvRtpPayloadMpeg4 constructor
 * input/output - pointer to RvRtpPayloadMpeg4
 * result :  pointer to RvRtpPayloadMpeg4
 ********************************************************************************/
RvRtpPayloadMpeg4* RVCALLCONV RvRtpMpeg4Construct(RvRtpPayloadMpeg4* thisPtr)
{
	RvUint32 Count = 0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Construct"));

	if (NULL==thisPtr)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Construct(): NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Construct"));
		return NULL;
	}
	for (Count = 0; Count< RV_RTPPAYLOADMPEG4_MAX_AU_IN_PACKET; Count++)
	{
		memset(&(thisPtr->AuHeader[Count]), 0, sizeof(RvRtpMpeg4Header));
	}
	thisPtr->AuHeadersLength = 0;
	thisPtr->AUsNumber = 0;
	thisPtr->PacketLength = 0;
	thisPtr->AuHeadersLength = 0;
	thisPtr->mimeFormatPtr = NULL;
	thisPtr->maxPayloadSize = RV_RTPPAYLOADMPEG4_MAX_PAYLOAD_SIZE; /* 28 UDP +12 RTP HEADER for fragmentation */
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Construct"));
    return thisPtr;
}
/************************************************************************
 * RvRtpMpeg4MimeFormatConstruct
 * description:  The RvRtpMpeg4MimeFormatConstruct constructor.
 * parameters:
 * input:	 mode    - MPEG-4 mode
 * output:   thisPtr - pointer to the RvRtpMpeg4MimeFormat.
 * returns: pointer to the RvRtpMpeg4MimeFormat.
 ************************************************************************/
RvRtpMpeg4MimeFormat*  RVCALLCONV RvRtpMpeg4MimeFormatConstruct(
	RvRtpMpeg4MimeFormat* format,
	RvRtpMpeg4Mode mode)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4MimeFormatConstruct"));
	memset(format,0,sizeof(format));
	format->mode=mode;

	switch(mode)
	{
		case RV_RTPPAYLOADMPEG4_MODE_GENERIC:
			format->RAP_flag=RV_TRUE;
		break;
		case RV_RTPPAYLOADMPEG4_MODE_CBR_CELP:
			format->Interleaving=RV_FALSE;
			format->Fragmentation=RV_FALSE;
			format->AuHeaderSection=RV_FALSE;
			format->AuxiliaryDataSizeSize=0;
			format->Concatination=RV_TRUE;
			format->AuSizeSize=0;
		break;
		case RV_RTPPAYLOADMPEG4_MODE_LBR_AAC:
			format->Interleaving=RV_TRUE;
		case RV_RTPPAYLOADMPEG4_MODE_VBR_CELP:
			format->Fragmentation=RV_FALSE;
			format->AuHeaderSection=RV_TRUE;
			format->AuxiliaryDataSizeSize=0;
			format->AuSizeSize=6;
			format->AuIndexOrAuIndexDeltaSize=2;
		break;
		case RV_RTPPAYLOADMPEG4_MODE_HBR_AAC:
			format->Fragmentation=RV_TRUE;
			format->AuHeaderSection=RV_TRUE;
			format->AuxiliaryDataSizeSize=0;
			format->AuSizeSize=13;
			format->AuIndexOrAuIndexDeltaSize=3;
		break;
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4MimeFormatConstruct"));
	return format;
}
/********************************************************************************
 * RvRtpMpeg4Unpack reads buffer and fills MPEG-4 payload
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpeg4
 * result : return RV_OK if there are no errors
 * Note:	mimeFormatPtr must be checked by RvRtpMpeg4MimeFormatCheck
 *          before calling to this function
 ********************************************************************************/
RvStatus RVCALLCONV RvRtpMpeg4Unpack(
		IN RvUint8* buf,
		IN RvSize_t payloadSize,
		IN RvRtpParam* p,
		INOUT RvRtpPayloadMpeg4* thisPtr)
{
	RvRtpMpeg4MimeFormat* mimeFormatPtr = thisPtr->mimeFormatPtr; /* for serialization/unserialization purposes */
    RvSize_t header; /* current number of AU header or AU in packet */
    RvUint32 currentBits=0;
    RvSize_t auSize = 0;
	RvUint8*  bufferPtr = buf + p->sByte;
	RvUint8*  buffer = bufferPtr;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Unpack"));

    thisPtr->PacketLength = (RvUint32)payloadSize;

	if (mimeFormatPtr->AuHeaderSection)
	{
        thisPtr->AuHeadersLength  = (RvUint16) rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, 16);

        bufferPtr+=sizeof(RvUint16); /*Lets adjust to the beggining of the AU-header(1)*/
        currentBits = 0;
        header = 0;
		while (currentBits < thisPtr->AuHeadersLength)
		{
			memset(&thisPtr->AuHeader[header], 0,  sizeof (thisPtr->AuHeader[header]));
            /* header readings */
			if (mimeFormatPtr->AuSizeSize>0)
                thisPtr->AuHeader[header].AuSize = (RvUint16)
                    rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->AuSizeSize);
            else if (mimeFormatPtr->ConstantAuSize>0)
		            thisPtr->AuHeader[header].AuSize = mimeFormatPtr->ConstantAuSize;
                else
                    thisPtr->AuHeader[header].AuSize = 0;


            if (mimeFormatPtr->AuIndexOrAuIndexDeltaSize>0)
			{
				if (header==0)  /* This is AuIndex */
				{
					/* Lets get the serial number */
                    thisPtr->AuHeader[header].AuIndex =
                        rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->AuIndexOrAuIndexDeltaSize);
					thisPtr->AuHeader[header].AuIndexDelta = 0;
				}
				else	/* This is AuIndexDelta */
				{
                    thisPtr->AuHeader[header].AuIndexDelta =
                        rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->AuIndexOrAuIndexDeltaSize);
					if ((!mimeFormatPtr->Interleaving) && (thisPtr->AuHeader[header].AuIndexDelta!=0))
						return RV_ERROR_NOTSUPPORTED; /* ERROR interleaving is not supported with non zero AuIndexDelta*/
					thisPtr->AuHeader[header].AuIndex =
						thisPtr->AuHeader[header-1].AuIndex + thisPtr->AuHeader[header].AuIndexDelta + 1;
				}
			}

			if (mimeFormatPtr->CTS_deltaSize > 0) /* CTS_flag is present only if CTS_deltaSize >0 */
				thisPtr->AuHeader[header].CTS_flag =
                    (rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, 1) == 0) ? RV_FALSE : RV_TRUE;

            if (mimeFormatPtr->CTS_deltaSize>0 && thisPtr->AuHeader[header].CTS_flag)
                thisPtr->AuHeader[header].CTS_delta =
                    rvBitfieldReadSigned(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->CTS_deltaSize);

            if (mimeFormatPtr->DTS_deltaSize > 0) /* DTS_flag is present only if DTS_deltaSize >0 */
				thisPtr->AuHeader[header].DTS_flag =
                    (rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, 1) == 0) ? RV_FALSE : RV_TRUE;

            if (mimeFormatPtr->DTS_deltaSize>0 && thisPtr->AuHeader[header].DTS_flag)
                thisPtr->AuHeader[header].DTS_delta =
                    rvBitfieldReadSigned(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->DTS_deltaSize);

            if (mimeFormatPtr->RAP_flag)
				thisPtr->AuHeader[header].RAP_flag =
					(rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, 1) == 0) ? RV_FALSE : RV_TRUE;

			if (mimeFormatPtr->StreamStateSize>0)
                thisPtr->AuHeader[header].StreamState=
                    rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->StreamStateSize);
			header++;
		}
		thisPtr->AUsNumber = (RvUint32)header;
        rvBitfieldAdjustToNextByte(&currentBits);
	}
	/* Auxiliary Section unserializing */
    if (mimeFormatPtr->AuxiliaryDataSizeSize > 0)
	{
        thisPtr->AuxiliarySection.Size =
            rvBitfieldRead(bufferPtr, (RvSize_t *)&currentBits, mimeFormatPtr->AuxiliaryDataSizeSize);
        thisPtr->AuxiliarySection.BufferPtr = bufferPtr;
        thisPtr->AuxiliarySection.StartBit = currentBits;
        currentBits+=(RvUint32)thisPtr->AuxiliarySection.Size;
        rvBitfieldAdjustToNextByte(&currentBits);
    }

    /* AU Data Section */
	if (mimeFormatPtr->ConstantAuSize > 0)
		thisPtr->AUsNumber = (RvUint32)(payloadSize - currentBits/8)/mimeFormatPtr->ConstantAuSize;

    bufferPtr+=currentBits/8;

    for (header=0; header<thisPtr->AUsNumber; header++)
	{
		if (mimeFormatPtr->AuSizeSize>0)
			auSize = thisPtr->AuHeader[header].AuSize;
		else if (mimeFormatPtr->ConstantAuSize>0)
			auSize = mimeFormatPtr->ConstantAuSize; /* in bytes */

        thisPtr->AuHeader[header].Data = bufferPtr;
        thisPtr->AuHeader[header].DataSize = auSize;
		if (header==0 && thisPtr->AUsNumber==1 && mimeFormatPtr->Fragmentation)
			thisPtr->AuHeader[header].DataSize = payloadSize - (bufferPtr - buffer);
		bufferPtr += thisPtr->AuHeader[header].DataSize;
	}
	
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Unpack"));
	return RV_OK;
}
/********************************************************************************
 * RvRtpMpeg4GetAuxiliarySection gets the auxiliary section data.
 * INPUT: thisPtr - pointer to RvRtpPayloadMpeg4
 *        auxiliarySession - pointer to data buffer to be filled with auxiliary section
 *        auxiliarySessionBufferSize - maximal size to be filled
 * OUTPUT: none
 * result : RvRtpMpeg4Status
 ********************************************************************************/
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4GetAuxiliarySection(
		RvRtpPayloadMpeg4* thisPtr,
		RvUint8* auxiliarySession,
		RvSize_t auxiliarySessionBufferSize)
{
    RvSize_t byteNumber;
    RvUint8* bufferPtr= thisPtr->AuxiliarySection.BufferPtr;
    RvSize_t currentBits = thisPtr->AuxiliarySection.StartBit;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAuxiliarySection"));

    if ((thisPtr->AuxiliarySection.Size == 0)  ||
		  (auxiliarySessionBufferSize < ((thisPtr->AuxiliarySection.Size >> 3) + 1)))
        return RV_RTPSTATUS_NotEnoughRoomInDataBuffer;

	for (byteNumber=0; byteNumber<thisPtr->AuxiliarySection.Size>>3;byteNumber++)
        auxiliarySession[byteNumber] = (RvChar) rvBitfieldRead(bufferPtr, &currentBits, 8);

    if (thisPtr->AuxiliarySection.Size % 8)
        auxiliarySession[byteNumber] = (RvChar) rvBitfieldRead(bufferPtr, &currentBits, thisPtr->AuxiliarySection.Size % 8);

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAuxiliarySection"));
    return RV_RTPSTATUS_Succeeded;
}
/********************************************************************************
 * RvRtpMpeg4SetAuxiliarySection sets the auxiliary section data to RvRtpPayloadMpeg4.
 * INPUT: auxiliarySession - pointer to filled auxiliary session data buffer
 *        auxiliarySessionBufferSize - size of auxiliarySession
 * OUTPUT: thisPtr - pointer to RvRtpPayloadMpeg4
 * result : RvRtpMpeg4Status
 ********************************************************************************/
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4SetAuxiliarySection(
        RvRtpPayloadMpeg4* thisPtr,
		RvUint8* auxiliarySession,
		RvSize_t auxiliarySessionSizeInBits)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4SetAuxiliarySection"));
    thisPtr->AuxiliarySection.BufferPtr = auxiliarySession;
    thisPtr->AuxiliarySection.Size      = auxiliarySessionSizeInBits;
    thisPtr->AuxiliarySection.StartBit  = 0;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4SetAuxiliarySection"));
    return RV_RTPSTATUS_Succeeded;
}
/********************************************************************************
 * RvRtpMpeg4GetAccessUnit
 * description - Gets access unit(AU) from a RvRtpPayloadMpeg4 object
 * Parameters:
 * INPUT:       thisPtr - the pointer to the RvRtpPayloadMpeg4 object
 * INPUT: mpeg4HeaderPtr - the pointer to the header object describing AU to be filled.
 *              auNumber - is number of AU inside of RvRtpPayloadMpeg4 object to be filled
 *                 auPtr - auPtr point to the buffer to be filled by entire access unit.
 *              auOffset - the pointer to the offset inside of auPtr buffer to fill from
 *                auSize - the size of auPtr buffer.
 * result : RvRtpMpeg4Status
 ********************************************************************************/
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4GetAccessUnit(
	 	 RvRtpPayloadMpeg4*			thisPtr,
         RvRtpMpeg4Header*	auHeader,
         RvSize_t					auNumber,
		 RvUint8*					auPtr,
		 RvSize_t*					auOffset,
         RvSize_t*					auSize)
{
	RvSize_t offset=((auOffset)?*auOffset:0);
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAccessUnit"));

	if (auHeader==NULL || auPtr==NULL || (auNumber>=thisPtr->AUsNumber))
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAccessUnit(): wrong input parameter"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAccessUnit"));
        return RV_RTPSTATUS_Failed;
	}

    if ((*auSize - offset) < thisPtr->AuHeader[auNumber].DataSize)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAccessUnit(): not enough room in data buffer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAccessUnit"));
		return RV_RTPSTATUS_NotEnoughRoomInDataBuffer;
	}

    *auHeader = thisPtr->AuHeader[auNumber];
    memcpy(auPtr +  offset, thisPtr->AuHeader[auNumber].Data, thisPtr->AuHeader[auNumber].DataSize);
	if (auOffset)
		*auOffset+=thisPtr->AuHeader[auNumber].DataSize;
    *auSize = thisPtr->AuHeader[auNumber].AuSize;

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetAccessUnit"));
    return RV_RTPSTATUS_Succeeded;
}

RvUint32 RVCALLCONV RvRtpMpeg4GetPayloadLength(const RvRtpPayloadMpeg4* thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetPayloadLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4GetPayloadLength"));
	return thisPtr->PacketLength; /* serialized size */
}
/********************************************************************************
 * RvRtpMpeg4Pack serialises MPEG-4 payload into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpeg4
 * result : return RV_OK if there are no errors
 * Note:	mimeFormatPtr must be checked by RvRtpPayloadMpeg4MimeFormatCheck
 *         before calling to this function
 ********************************************************************************/
RvStatus RVCALLCONV RvRtpMpeg4Pack(
	RvUint8* bufferPtr,
    RvSize_t payloadSize,
	RvRtpParam * p,
	RvRtpPayloadMpeg4* thisPtr)
{
	RvRtpMpeg4MimeFormat* mimeFormatPtr = thisPtr->mimeFormatPtr; /* for serialization/unserialization purposes */
    RvSize_t  currentBits=0;
    RvSize_t header; /* current number of AU header or AU in packet */
    RvSize_t byteNumber;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Pack"));

	memset(bufferPtr, 0, payloadSize);
	thisPtr->PacketLength = 0;
	/* CBR_CELP does not support header section */
    if (mimeFormatPtr->AuHeaderSection)
	{
        rvBitfieldWrite(bufferPtr, &currentBits, 16, thisPtr->AuHeadersLength) ;

        thisPtr->PacketLength = sizeof(RvUint16);
        currentBits = 0;
        bufferPtr+=sizeof(RvUint16);
        header = 0;
		while (currentBits < thisPtr->AuHeadersLength )
		{
			/* header readings */
			if (mimeFormatPtr->AuSizeSize>0)
			{
				/* AuSize is RvUint16 */
                rvBitfieldWrite(bufferPtr, &currentBits, mimeFormatPtr->AuSizeSize, thisPtr->AuHeader[header].AuSize);
			}

            if (mimeFormatPtr->AuIndexOrAuIndexDeltaSize>0)
			{

				if (header==0)  /* This is AuIndex */
				{	/* Lets write the serial number */
                    rvBitfieldWrite(bufferPtr, &currentBits, mimeFormatPtr->AuIndexOrAuIndexDeltaSize, thisPtr->AuHeader[header].AuIndex);
				}
				else	/* AuIndexDelta meaning */
				{
                    rvBitfieldWrite(bufferPtr, &currentBits, mimeFormatPtr->AuIndexOrAuIndexDeltaSize, thisPtr->AuHeader[header].AuIndexDelta);
					if ((!mimeFormatPtr->Interleaving)&& (thisPtr->AuHeader[header].AuIndexDelta!=0))
						return RV_ERROR_NOTSUPPORTED; /* ERROR interleaving is not supported with non zero AuIndexDelta*/
                    /*	already	done before in RvRtpMpeg4GetAccessUnit thisPtr->AuHeader[header].AuIndex =
  			            thisPtr->AuHeader[header-1].AuIndex + thisPtr->AuHeader[header].AuIndexDelta + 1;	*/
				}
			}
    		if (mimeFormatPtr->CTS_deltaSize > 0) /* CTS_flag is present only if CTS_deltaSize >0 */
                rvBitfieldWrite(bufferPtr, &currentBits, 1, thisPtr->AuHeader[header].CTS_flag) ;

            if (mimeFormatPtr->CTS_deltaSize>0 && thisPtr->AuHeader[header].CTS_flag)
				rvBitfieldWrite(bufferPtr, &currentBits,  mimeFormatPtr->CTS_deltaSize, thisPtr->AuHeader[header].CTS_delta);

            if (mimeFormatPtr->DTS_deltaSize > 0) /* DTS_flag is present only if DTS_deltaSize >0 */
                rvBitfieldWrite(bufferPtr, &currentBits, 1, thisPtr->AuHeader[header].DTS_flag) ;

            if (mimeFormatPtr->DTS_deltaSize>0 && thisPtr->AuHeader[header].DTS_flag)
				rvBitfieldWrite(bufferPtr, &currentBits,  mimeFormatPtr->DTS_deltaSize, thisPtr->AuHeader[header].DTS_delta);

			if (mimeFormatPtr->RAP_flag)
				rvBitfieldWrite(bufferPtr, &currentBits, 1, thisPtr->AuHeader[header].RAP_flag) ;
			if (mimeFormatPtr->StreamStateSize>0)
			{
                rvBitfieldWrite(bufferPtr, &currentBits,  mimeFormatPtr->StreamStateSize, thisPtr->AuHeader[header].StreamState);
			}
			header++;
		}
        rvBitfieldAdjustToNextByte(&currentBits);
	}
	/* Auxiliary Section serializing */

	if (mimeFormatPtr->AuxiliaryDataSizeSize > 0)
	{
        rvBitfieldWrite(bufferPtr, &currentBits,  mimeFormatPtr->AuxiliaryDataSizeSize, (RvUint32)thisPtr->AuxiliarySection.Size);

        /* AuxiliarySection.Size in Bits */
		for (byteNumber=0; byteNumber<thisPtr->AuxiliarySection.Size/8; byteNumber++)
		{
            rvBitfieldWrite(bufferPtr, &currentBits,  8, thisPtr->AuxiliarySection.BufferPtr[byteNumber]);
		}

        if (thisPtr->AuxiliarySection.Size % 8)
		{
            rvBitfieldWrite(bufferPtr, &currentBits,  thisPtr->AuxiliarySection.Size % 8, thisPtr->AuxiliarySection.BufferPtr[byteNumber]);
		}
        rvBitfieldAdjustToNextByte(&currentBits);/* adjusting to AU Data Section */
	}

    thisPtr->PacketLength += (RvUint32)currentBits/8;
    bufferPtr+=currentBits/8;

    /* AU Data Section */
	for (header=0; header < thisPtr->AUsNumber; header++)
	{
		memcpy(bufferPtr,thisPtr->AuHeader[header].Data, thisPtr->AuHeader[header].DataSize);
        bufferPtr+=thisPtr->AuHeader[header].DataSize;
        thisPtr->PacketLength += (RvUint32)thisPtr->AuHeader[header].DataSize;
	}
	p->marker = thisPtr->markerBit;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4Pack"));
	return RV_OK;
}
/********************************************************************************
 * RvRtpMpeg4AddAccessUnit
 * description - adds an access unit to the payload.
 *               Access unit is represented by the 3 last parameters.
 *               auPtr point to the begining of entire access unit.
 *               auOffset represents the offset inside the auPtr,
 *               It should be 0 for first use and the function will advance it accordance
 *               to the size of the processed fragment.
 * Parameters:
 * INPUT/OUTPUT: thisPtr - the pointer to the RvRtpPayloadMpeg4 object (AU to be added)
 * INPUT: mpeg4HeaderPtr - the pointer to the header object describing AU.
 *                 auPtr - the pointer to the begining of entire access unit.
 *              auOffset - the pointer to the offset of the current fragment inside auPtr.
 *                         Must point to 0 before the first call to rvRtpPayloadMpeg4AddAccessUnit
 *                         with this AU.
 *                auSize - the size of the entire AU.
 * result : RvRtpMpeg4Status
 ********************************************************************************/

RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4AddAccessUnit(
		 RvRtpPayloadMpeg4*	        thisPtr,
		 RvRtpMpeg4Header*	mpeg4HeaderPtr,
		 RvUint8*			        auPtr,     /* pointer to access unit */
		 RvSize_t*					auOffset,
         RvSize_t                   auSize)    /* size of the access unit */
{
	RvRtpMpeg4MimeFormat* mimeFormatPtr; /* for serialization/unserialization purposes */
    RvUint32 AuHeaderNumber = 0;
    RvUint32 fragmentLength	= 0;
    RvUint16 auHeaderLength	= 0;
	RvUint32 concatinatedPacketLen = 0;
	RvSize_t offset;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));

	if (thisPtr==NULL || auPtr == NULL || thisPtr->mimeFormatPtr == NULL)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
		return RV_RTPSTATUS_Failed;
	}
	mimeFormatPtr = thisPtr->mimeFormatPtr;

	if (mimeFormatPtr->ConstantAuSize>0 && auSize!=mimeFormatPtr->ConstantAuSize)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit(): constant AU size mismatch"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
		return RV_RTPSTATUS_Failed;
	}
	if (mimeFormatPtr->Fragmentation && auOffset==NULL)
	{
	    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: NULL auOffset in case of enabled fragmentation"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
		return RV_RTPSTATUS_Failed;
	}
	if (mimeFormatPtr->Fragmentation && auOffset && *auOffset>auSize)
	{
	    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: *auOffset exceeds auSize."));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
		return RV_RTPSTATUS_Failed;
	}
	if (mimeFormatPtr->Concatination)
	{
		if (thisPtr->AUsNumber >= RV_RTPPAYLOADMPEG4_MAX_AU_IN_PACKET)
		{
		    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: cannot continue concatinate."
				" The maximum AUs in RTP packet is %d.", RV_RTPPAYLOADMPEG4_MAX_AU_IN_PACKET));
	        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
			return RV_RTPSTATUS_Failed;
		}
	}
	else
	{
		if (thisPtr->AUsNumber > 0)
		{
		    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: concatination is not permitted."));
		    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
			return RV_RTPSTATUS_Failed;
		}
	}

	if (mimeFormatPtr->AuHeaderSection)
	{
		if (mpeg4HeaderPtr)
			auHeaderLength =(RvUint16)(mimeFormatPtr->AuSizeSize + mimeFormatPtr->AuIndexOrAuIndexDeltaSize +
							(mimeFormatPtr->CTS_deltaSize>0) + ((mpeg4HeaderPtr->CTS_flag)?mimeFormatPtr->CTS_deltaSize:0) +
							(mimeFormatPtr->CTS_deltaSize>0) + ((mpeg4HeaderPtr->CTS_flag)?mimeFormatPtr->DTS_deltaSize:0) +
							(mimeFormatPtr->RAP_flag==RV_TRUE) + mimeFormatPtr->StreamStateSize);
		else
			auHeaderLength =(RvUint16)(mimeFormatPtr->AuSizeSize + mimeFormatPtr->AuIndexOrAuIndexDeltaSize +
							(mimeFormatPtr->RAP_flag==RV_TRUE) + mimeFormatPtr->StreamStateSize);
		concatinatedPacketLen=auHeaderLength + thisPtr->AuHeadersLength;
		rvBitfieldAdjustToNextByte(&concatinatedPacketLen);
		concatinatedPacketLen/=8;
		concatinatedPacketLen+=2;
	}
	offset = (auOffset)?(*auOffset):0;

    if (mimeFormatPtr->AuxiliaryDataSizeSize > 0)
    {
		concatinatedPacketLen*=8;
        concatinatedPacketLen+=mimeFormatPtr->AuxiliaryDataSizeSize+(RvUint32)thisPtr->AuxiliarySection.Size;
        rvBitfieldAdjustToNextByte(&concatinatedPacketLen);
        concatinatedPacketLen/=8;
    }
	if (thisPtr->PacketLength + concatinatedPacketLen + auSize - offset > thisPtr->maxPayloadSize)
	{/*Fragmentation needed*/
		if (!mimeFormatPtr->Fragmentation)
		{
	        RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: fragmentation is not supported"));
	        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
			return RV_RTPSTATUS_NotSupported;
		}
		/* packet must be fragmented */
		if (thisPtr->AUsNumber>0)
        {
		    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: AU cannot be fragmented,"
				                            " because fragment of other AU or completed AU is already in payload."));
	        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
			return RV_RTPSTATUS_NotEnoughRoomInDataBuffer;
			/* AU cannot be fragmented, because fragment or completed
			 AU already in payload (RFC). Serialize the payload.
			 timestamp not to be changed */
        }
		fragmentLength = (RvUint32)thisPtr->maxPayloadSize - (thisPtr->PacketLength + concatinatedPacketLen);
        thisPtr->markerBit = RV_FALSE;
	}

	if (!mimeFormatPtr->Concatination && thisPtr->AUsNumber>0)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: concatination is not supported."));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
		return RV_RTPSTATUS_NotSupported;
	}
	AuHeaderNumber = thisPtr->AUsNumber;

    if (mpeg4HeaderPtr && mimeFormatPtr->AuHeaderSection)
	{
		thisPtr->AuHeader[AuHeaderNumber]=*mpeg4HeaderPtr;
		if (mimeFormatPtr->AuSizeSize>0)
		{
			if (auSize != mpeg4HeaderPtr->AuSize)
			{
			    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit: header size mismatch"));
		        RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
				return RV_RTPSTATUS_Failed;
			}
			if (mimeFormatPtr->Fragmentation)
			{
				if ((mpeg4HeaderPtr->AuSize >= auSize) && fragmentLength==0)
				{
					/* Last fragment or not fragmented AU*/
					thisPtr->markerBit = RV_TRUE; /*end of AU in packet */
				}
			}
		}
		if (mimeFormatPtr->AuIndexOrAuIndexDeltaSize>0)
		{
			if (AuHeaderNumber==0)  /* This is AuIndex */
			{
				thisPtr->AuHeader[AuHeaderNumber].AuIndexDelta	= 0;
			}
			else
			{
				if (!mimeFormatPtr->Interleaving)
					thisPtr->AuHeader[AuHeaderNumber].AuIndexDelta = 0;/* 0 if no interleaving */
				thisPtr->AuHeader[AuHeaderNumber].AuIndex =
                    thisPtr->AuHeader[AuHeaderNumber-1].AuIndex + thisPtr->AuHeader[AuHeaderNumber].AuIndexDelta + 1;
			}
		}
		if (auOffset && *auOffset) /* non first fragment of AU */
			thisPtr->AuHeader[AuHeaderNumber].RAP_flag = RV_FALSE;

		thisPtr->AuHeadersLength = (RvUint16)(thisPtr->AuHeadersLength + auHeaderLength);
	}

    /* DATA part available in each AU */
    thisPtr->AuHeader[AuHeaderNumber].Data = auPtr+offset;
    if (fragmentLength)
		thisPtr->AuHeader[AuHeaderNumber].DataSize = fragmentLength;
	else
		thisPtr->AuHeader[AuHeaderNumber].DataSize = auSize - offset;

    thisPtr->PacketLength += (RvUint32)thisPtr->AuHeader[AuHeaderNumber].DataSize;

	if (auOffset)
		*auOffset+=(fragmentLength)?fragmentLength:auSize;
	thisPtr->AUsNumber++;

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4AddAccessUnit"));
    return RV_RTPSTATUS_Succeeded;
}
/************************************************************************
 * RvRtpMpeg4MimeFormatCheck
 * description:  checks validity of RvRtpMpeg4MimeFormat
 * parameters:
 * input:   thisPtr - pointer to the RvRtpMpeg4MimeFormat.
 * output:  none
 * returns: RvRtpMpeg4Status
 ************************************************************************/
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4MimeFormatCheck(RvRtpMpeg4MimeFormat* mimeFormatPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpeg4MimeFormatCheck"));
	if (NULL == mimeFormatPtr)
		return RV_RTPSTATUS_Failed;
	if (mimeFormatPtr->AuSizeSize>16||
		mimeFormatPtr->AuIndexOrAuIndexDeltaSize > 32||
		mimeFormatPtr->AuxiliaryDataSizeSize > 32||
		mimeFormatPtr->CTS_deltaSize > 32||
		mimeFormatPtr->DTS_deltaSize > 32||
		mimeFormatPtr->StreamStateSize > 32)
		return RV_RTPSTATUS_Failed;
    if (mimeFormatPtr->mode!=RV_RTPPAYLOADMPEG4_MODE_GENERIC&&mimeFormatPtr->RAP_flag)
		return RV_RTPSTATUS_Failed;
	switch (mimeFormatPtr->mode)
	{
	case RV_RTPPAYLOADMPEG4_MODE_GENERIC: /* no restrictions */
		if ((mimeFormatPtr->AuSizeSize > 0) && (((RvUint32Const(1) << (mimeFormatPtr->AuSizeSize + 1)) - 1) < mimeFormatPtr->ConstantAuSize))
			return RV_RTPSTATUS_Failed;
		break;
	case RV_RTPPAYLOADMPEG4_MODE_CBR_CELP: /* according to RFC3640 */
    /* all frames are equal size */
		if (mimeFormatPtr->AuSizeSize>16)
			return RV_RTPSTATUS_Failed; /* used  ConstantAuSize */

		if (mimeFormatPtr->Fragmentation)
			return RV_RTPSTATUS_Failed; /* no support for fragmentation */

		if (mimeFormatPtr->AuHeaderSection||mimeFormatPtr->AuSizeSize>0)
			return RV_RTPSTATUS_Failed; /* no header section */

		if (mimeFormatPtr->AuxiliaryDataSizeSize>0)
			return RV_RTPSTATUS_Failed; /* no Auxiliaryheader section */

		if (mimeFormatPtr->Interleaving||mimeFormatPtr->AuIndexOrAuIndexDeltaSize > 0)
			return RV_RTPSTATUS_Failed; /* no support for interleaving */

		if (mimeFormatPtr->ConstantAuSize == 0)
			return RV_RTPSTATUS_Failed; /* no header section */
		break;
	case RV_RTPPAYLOADMPEG4_MODE_VBR_CELP:
		if (!mimeFormatPtr->AuHeaderSection)
			return RV_RTPSTATUS_Failed; /* header section presents */
		if (mimeFormatPtr->Fragmentation)
			return RV_RTPSTATUS_Failed; /* no support for fragmentation */
		if (mimeFormatPtr->AuxiliaryDataSizeSize>0)
			return RV_RTPSTATUS_Failed; /* no Auxiliaryheader section */
		/* Required parameters set */
		if (mimeFormatPtr->AuSizeSize!=6)
			return RV_RTPSTATUS_Failed; /* CELP frame size must be coded by 6 bits */
		if (mimeFormatPtr->AuIndexOrAuIndexDeltaSize!=2)
			return RV_RTPSTATUS_Failed; /* AuIndexOrAuIndexDeltaSize size must be coded by 2 bits */
        /* RFC 3640:
			    In addition to the required MIME format parameters, the following
			    parameters MUST be present: sizeLength, indexLength, and
			    indexDeltaLength.  CELP frames always have a fixed duration per
			    Access Unit; when interleaving in this mode, this specific duration
			    MUST be signaled by the MIME format parameter constantDuration.  In
			    addition, the parameter maxDisplacement MUST be present when
			    interleaving.
        */
		break;
	case RV_RTPPAYLOADMPEG4_MODE_LBR_AAC:
		if (!mimeFormatPtr->AuHeaderSection)
			return RV_RTPSTATUS_Failed; /* header section presents */
		if (mimeFormatPtr->Fragmentation)
			return RV_RTPSTATUS_Failed; /* no support for fragmentation */
		if (mimeFormatPtr->AuxiliaryDataSizeSize>0)
			return RV_RTPSTATUS_Failed; /* no Auxiliaryheader section */
		/* Required parameters set */
		if (mimeFormatPtr->AuSizeSize!=6)
			return RV_RTPSTATUS_Failed; /* AAC frame size must be coded by 6 bits */
		if (mimeFormatPtr->AuIndexOrAuIndexDeltaSize!=2)
			return RV_RTPSTATUS_Failed; /* AuIndexOrAuIndexDeltaSize size must be coded by 2 bits */
        /* RFC 3640:
		    In addition to the required MIME format parameters, the following
		    parameters MUST be present: sizeLength, indexLength, and
		    indexDeltaLength.  AAC frames always have a fixed duration per Access
		    Unit; when interleaving in this mode, this specific duration MUST be
		    signaled by the MIME format parameter constantDuration.  In addition,
		    the parameter maxDisplacement MUST be present when interleaving.
        */
		break;
	case RV_RTPPAYLOADMPEG4_MODE_HBR_AAC:
		if (mimeFormatPtr->AuxiliaryDataSizeSize>0)
			return RV_RTPSTATUS_Failed; /* no Auxiliaryheader section */
		if (!mimeFormatPtr->AuHeaderSection)
			return RV_RTPSTATUS_Failed; /* header section */
		if (mimeFormatPtr->AuSizeSize!=13)
			return RV_RTPSTATUS_Failed; /* AAC frame size must be coded by 13 bits */
		if (mimeFormatPtr->AuIndexOrAuIndexDeltaSize!=3)
			return RV_RTPSTATUS_Failed; /* AuIndexOrAuIndexDeltaSize size must be coded by 3 bits */
		break;
	default:
		return RV_RTPSTATUS_Failed;
	}

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpeg4MimeFormatCheck"));
	return RV_RTPSTATUS_Succeeded;
}

#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
/*for debug purposes only buffer must be long enough */
RVAPI
RvInt32 RVCALLCONV RvRtpPayloadMpeg4ToString(
		const RvRtpPayloadMpeg4* thisPtr,
		char *buffer,
		size_t bufferSize,
		const char *prefix)
{
    RvUint32 offset = 0, HeaderNum = 0, Count = 0;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPayloadMpeg4ToString"));
	if (thisPtr==NULL||thisPtr->mimeFormatPtr==NULL)
	{
	    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadMpeg4ToString: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadMpeg4ToString"));
		return RV_RTPSTATUS_WrongParameter;
	}
	offset += sprintf(&buffer[offset],"%sCodec: MPEG-4 Mode:%#x\n",prefix, thisPtr->mimeFormatPtr->mode);
	if ((thisPtr->mimeFormatPtr->mode & 0x1D) && thisPtr->mimeFormatPtr->AuHeaderSection)
	{
		offset += sprintf(&buffer[offset],"%sAU Headers Section Size:         %d\n", prefix,
			thisPtr->AuHeadersLength);
		while (HeaderNum < thisPtr->AUsNumber)
		{
			if (thisPtr->mimeFormatPtr->AuSizeSize > 0)
				offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) size:              %d\n",
				prefix, HeaderNum, thisPtr->mimeFormatPtr->AuSizeSize,
				thisPtr->AuHeader[HeaderNum].AuSize);

			if (thisPtr->mimeFormatPtr->AuIndexOrAuIndexDeltaSize > 0)
			{
				if (HeaderNum==0)
					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) AuIndex:           %d\n",
					prefix, HeaderNum, thisPtr->mimeFormatPtr->AuIndexOrAuIndexDeltaSize,
					thisPtr->AuHeader[HeaderNum].AuIndex);
				else
					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) AuIndexDelta:      %d\n",
					prefix, HeaderNum, thisPtr->mimeFormatPtr->AuIndexOrAuIndexDeltaSize,
					thisPtr->AuHeader[HeaderNum].AuIndexDelta);
			}
			if (thisPtr->mimeFormatPtr->mode & RV_RTPPAYLOADMPEG4_MODE_GENERIC)
			{
				if (thisPtr->mimeFormatPtr->CTS_deltaSize > 0)
					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) CTS_flag:          %d\n",
					prefix, HeaderNum, 1, thisPtr->AuHeader[HeaderNum].CTS_flag);

				if (thisPtr->mimeFormatPtr->CTS_deltaSize > 0)
					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) CTS_delta:         %d\n",
					prefix, HeaderNum, thisPtr->mimeFormatPtr->CTS_deltaSize,
					thisPtr->AuHeader[HeaderNum].CTS_delta);

				if (thisPtr->mimeFormatPtr->DTS_deltaSize > 0)

					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) DTS_flag:          %d\n",
					prefix, HeaderNum, 1, thisPtr->AuHeader[HeaderNum].DTS_flag);

				if (thisPtr->mimeFormatPtr->DTS_deltaSize > 0)
					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) DTS_delta:         %d\n",
					prefix, HeaderNum, thisPtr->mimeFormatPtr->DTS_deltaSize,
					thisPtr->AuHeader[HeaderNum].DTS_delta);

				offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) RAP_flag:          %d\n",
					prefix, HeaderNum, 1, thisPtr->AuHeader[HeaderNum].RAP_flag);

				if (thisPtr->mimeFormatPtr->StreamStateSize > 0)
					offset += sprintf(&buffer[offset],"%sAU [%2d](%2d) StreamState:       %d\n",
					prefix, HeaderNum, thisPtr->mimeFormatPtr->StreamStateSize,
					thisPtr->AuHeader[HeaderNum].StreamState);
			}
			HeaderNum++;
		}
	}
	else
	{
		if (thisPtr->mimeFormatPtr->ConstantAuSize > 0)
			offset += sprintf(&buffer[offset],"%sAU  size:              %d\n",
			prefix, thisPtr->mimeFormatPtr->ConstantAuSize);
	}
	if ((thisPtr->mimeFormatPtr->mode &  RV_RTPPAYLOADMPEG4_MODE_GENERIC) &&
		 thisPtr->mimeFormatPtr->AuxiliaryDataSizeSize>0)
	{
		offset += sprintf(&buffer[offset],"%sAuxiliary Data Size(%2d):		    %d\n",
			prefix, thisPtr->mimeFormatPtr->AuxiliaryDataSizeSize, (RvInt32)thisPtr->AuxiliarySection.Size);
		{
			offset += sprintf(&buffer[offset],"%sAuxiliary Data:", prefix);
			for (Count=0;Count<thisPtr->AuxiliarySection.Size;Count++)
			{
				offset += sprintf(&buffer[offset],"%x%x", thisPtr->AuxiliarySection.BufferPtr[Count]>>4,
					thisPtr->AuxiliarySection.BufferPtr[Count]&0x0F);
				if ((Count%16)==0)
					offset += sprintf(&buffer[offset],"\n");
			}
			offset += sprintf(&buffer[offset],"\n");
		}
	}

	for (HeaderNum = 0; HeaderNum < thisPtr->AUsNumber; HeaderNum++)
	{
		RvUint32 AuSize = 0;
		if (thisPtr->mimeFormatPtr->mode & 0x1D)
		{

			if (thisPtr->mimeFormatPtr->AuSizeSize>0 && thisPtr->AuHeader[HeaderNum].AuSize>0)
			{
				if (thisPtr->AuHeader[HeaderNum].DataSize < thisPtr->AuHeader[HeaderNum].AuSize)
					AuSize = (RvUint32)thisPtr->AuHeader[HeaderNum].DataSize;
				else
					AuSize = (RvUint32)thisPtr->AuHeader[HeaderNum].AuSize;
			}
			else if (thisPtr->mimeFormatPtr->ConstantAuSize>0)
				AuSize = thisPtr->mimeFormatPtr->ConstantAuSize;
		}
		else  /* CBR_CELP mode */
			AuSize = thisPtr->mimeFormatPtr->ConstantAuSize;

		offset += sprintf(&buffer[offset],"\n%sAUData[%2d](%d Bytes):",	prefix, HeaderNum, AuSize);
		if (AuSize> 1500)
            return RV_RTPSTATUS_WrongParameter;

		for (Count=0;Count<AuSize;Count++)
		{
			if ((Count%16)==0)
				offset += sprintf(&buffer[offset],"\n%sAUData + %d:", prefix, Count);
			offset += sprintf(&buffer[offset],"%x%x ", thisPtr->AuHeader[HeaderNum].Data[Count]>>4,
													   thisPtr->AuHeader[HeaderNum].Data[Count]&0x0F);
		}
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadMpeg4ToString"));
	RV_UNUSED_ARG(bufferSize);
    return offset;
}
#endif
/* == H.264 ==  draft-ietf-avt-rtp-h264-04.txt */
#if 1
#define RV_FREAD2BYTES(a,b)   ((b)[0] = (*(a)++),(b)[1] = (*(a)++))
#define RV_FWRITE2BYTES(a,b)  ((*--(a))=(b)[1],(*--(a))=(b)[0])
#define RV_FREAD4BYTES(a,b)   ((b)[0]=(*(a)++),(b)[1]=(*(a)++),(b)[2]=(*(a)++),(b)[3]=(*(a)++))
#define RV_FWRITE4BYTES(a,b)  ((*--(a))=(b)[3],(*--(a))=(b)[2],(*--(a))=(b)[1],(*--(a))=(b)[0])

#define  rvDataBufferReadUint16(buffer, s) 		   (RV_FREAD2BYTES(buffer,(RvUint8*)(s)),*((RvUint16*)s) = (RvUint16)RvConvertNetworkToHost16(*((RvUint16*)s)))
#define  rvDataBufferReadUint32(buffer, l) 	       (RV_FREAD4BYTES(buffer,(RvUint8*)(l)),*((RvUint32*)l) = RvConvertNetworkToHost32(*((RvUint32*)l)))

#define RV_RREAD2BYTES(a,b)   ((b)[1] = (*--(a)),(b)[0] = (*--(a)))
#define RV_RWRITE2BYTES(a,b)  ((*(a)++)=(b)[0],(*(a)++)=(b)[1])
#define RV_RREAD4BYTES(a,b)   ((b)[3] = (*--(a)),(b)[2] = (*--(a)),(b)[1] = (*--(a)),(b)[0] = (*--(a)))
#define RV_RWRITE4BYTES(a,b)  ((*(a)++)=(b)[0],(*(a)++)=(b)[1],(*(a)++)=(b)[2],(*(a)++)=(b)[3])

#define  rvDataBufferWriteEndUint32(buffer, l)				{RvUint32 t = RvConvertNetworkToHost32((RvUint32)(l)); RV_RWRITE4BYTES(buffer, (RvUint8*)&t);}
#define  rvDataBufferWriteEndUint16(buffer, s)				{RvUint16 t = (RvUint16)RvConvertNetworkToHost16((RvUint16)(s)); RV_RWRITE2BYTES(buffer, (RvUint8*)&t);}

#define RV_MODULO16(Value)	(((RvUint32)Value) - ((((RvUint32)Value)>>4)<<4))
#endif
RVAPI
RvStatus RVCALLCONV RvRtpH264Construct(RvRtpPayloadH264* pPayloadH264)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264Construct"));
	if (NULL==pPayloadH264)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264Construct: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Construct"));
		return RV_ERROR_BADPARAM;
	}
	memset(pPayloadH264, 0, sizeof(RvRtpPayloadH264));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Construct"));
	return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtpH264Destruct(RvRtpPayloadH264* pPayloadH264)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264Destruct"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Destruct"));
	return RvRtpH264Construct(pPayloadH264);
}
/**********************************************************************************
 * RvRtpH264AddNalToStap
 * description: inserts the sub-NAL into STAP packet (STAP_A & STAP_B)
 * input:       pStapNal     - pointer to sub NAL to be inserted to STAP payload
 *              Don          - 16 bits decoding order number, in case of STAP B
 *                             In case of STAP A the parameter is unusable.
 * input/output:      pPayloadH264 - pointer to H264 payload
 * result : return RV_OK if there are no errors
 * Note: 1) RvRtpH264AddNalToStap does not allocate data for sub NAL, hence
 *          data, on which pStap points must exist until RvRtpH264Pack is called.
 ***********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpH264AddNalToStap(
		INOUT RvRtpPayloadH264* pPayloadH264,
		IN RvRtpH264StapNalU*   pStapNal,
		IN RvUint16             Don)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));


	if (NULL==pPayloadH264||pPayloadH264->NalHdr.Type > RVRTP_H264_STAP_B || pPayloadH264->NalHdr.Type < RVRTP_H264_STAP_A)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap: bad parameter"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));
		return RV_ERROR_BADPARAM;
	}
	if (pPayloadH264->NalHdr.Type==RVRTP_H264_STAP_A)
	{
		RvInt32 Count = 0;
		for (Count=0;Count<RVRTP_H264_AP_NAL_UNITS_MAX;Count++)
			if (pPayloadH264->modes.STAP_A.Nal[Count].pData==NULL)
				break;
		if (Count==RVRTP_H264_AP_NAL_UNITS_MAX)
		{
			RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap: out of resources"));
			RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));
			return RV_ERROR_OUTOFRESOURCES;
		}
        if (Count==0)
			pPayloadH264->Length ++; /* includes aggregated NALU header (1Byte) */

		pPayloadH264->modes.STAP_A.Nal[Count].pData   = pStapNal->pData;
		pPayloadH264->modes.STAP_A.Nal[Count].Size    = pStapNal->Size;
		pPayloadH264->modes.STAP_A.Nal[Count].StapHdr = pStapNal->StapHdr;
		pPayloadH264->Length += (pStapNal->Size + 3/* STAP header+ size*/);
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));
		return  RV_OK;
	}
	if (pPayloadH264->NalHdr.Type==RVRTP_H264_STAP_B)
	{
		RvInt32 Count = 0;
		for (Count=0;Count<RVRTP_H264_AP_NAL_UNITS_MAX;Count++)
			if (pPayloadH264->modes.STAP_B.Nal[Count].pData==NULL)
				break;
			if (Count==RVRTP_H264_AP_NAL_UNITS_MAX)
			{
			    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap: out of resources"));
				RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));
				return RV_ERROR_OUTOFRESOURCES;
			}
			pPayloadH264->modes.STAP_B.Nal[Count].pData   = pStapNal->pData;
			pPayloadH264->modes.STAP_B.Nal[Count].Size    = pStapNal->Size;
			pPayloadH264->modes.STAP_B.Nal[Count].StapHdr = pStapNal->StapHdr;
		    pPayloadH264->Length += (pStapNal->Size + 3/* STAP header + size*/);
			if (Count==0)
			{
				pPayloadH264->modes.STAP_B.Don = Don;
				pPayloadH264->Length += 3; /* includes aggregated NALU header (1Byte) */
			}
			RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));
			return  RV_OK;
	}
	RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap: bad parameter"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToStap"));
	return RV_ERROR_BADPARAM;
}
/**********************************************************************************
 * RvRtpH264AddNalToMtap
 * description: inserts the sub-NAL into MTAP packet (MTAP_A & MTAP_B)
 * input:       pMtapNal      - pointer to sub NAL to be inserted to MTAP payload
 *              Donb         - 16bit decoding order number base , in case of MTAP B
 *                             In case of MTAP A the parameter is unusable.
 * input/output:      pPayloadH264 - pointer to H264 payload
 * result : return RV_OK if there are no errors
 * Note: 1) RvRtpH264AddNalToMtap does not allocate data for sub NAL, hence
 *          data, on which pStap points must exist until RvRtpH264Pack is called.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH264AddNalToMtap(
		INOUT RvRtpPayloadH264* pPayloadH264,
		IN RvRtpH264MtapNalU*   pMtapNal,
		IN RvUint16             Donb)
{
	RvInt32 Count = 0;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToMtap"));
	if (NULL==pPayloadH264||pPayloadH264->NalHdr.Type > RVRTP_H264_MTAP24 || pPayloadH264->NalHdr.Type < RVRTP_H264_MTAP16)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToMtap: bad parameter"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToMtap"));
		return RV_ERROR_BADPARAM;
	}

	for (Count=0;Count<RVRTP_H264_AP_NAL_UNITS_MAX;Count++)
		if (pPayloadH264->modes.MTAP.Nal[Count].pData==NULL)
			break;

	if (Count==RVRTP_H264_AP_NAL_UNITS_MAX)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToMtap: out of resources"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToMtap"));
		return RV_ERROR_OUTOFRESOURCES;
	}
	pPayloadH264->modes.MTAP.Nal[Count].pData    = pMtapNal->pData;
	pPayloadH264->modes.MTAP.Nal[Count].Size     = pMtapNal->Size;
	pPayloadH264->modes.MTAP.Nal[Count].Dond     = pMtapNal->Dond;
	pPayloadH264->modes.MTAP.Nal[Count].TSoffset = pMtapNal->TSoffset;
    pPayloadH264->Length += (pMtapNal->Size + 3/* Dond + Size */+((pPayloadH264->NalHdr.Type==RVRTP_H264_MTAP24)?3:2));
	if (Count==0)
	{
		pPayloadH264->modes.MTAP.Donb = Donb;
	    pPayloadH264->Length += 3;	/* includes aggregated header */
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264AddNalToMtap"));
	return  RV_OK;
}
/********************************************************************************
 * RvRtpH264Unpack reads buffer and fills H264 payload
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH264
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH264Unpack(
									IN  void *      buf,
									IN   RvInt32     len,
									IN  RvRtpParam *  p,
									OUT  void *      param)
{
    RvUint8* pBuffer;
	RvRtpPayloadH264* pPayloadH264 = (RvRtpPayloadH264*) param;
	RvUint32 data32 = 0;
	RvUint16 data16 = 0;
	RvUint32 ReadValue = 0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264Unpack"));

	if (NULL==buf||NULL==p)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264Unpack: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Unpack"));
		return RV_ERROR_BADPARAM;
	}
    pBuffer = ((RvUint8*)buf) + p->sByte;
    pPayloadH264->NalHdr.F    = *pBuffer >> 7;
	pPayloadH264->NalHdr.NRI  = (*pBuffer & 0x7F)>>5;
	pPayloadH264->NalHdr.Type = (RVRTP_H264NalTypes)(*pBuffer & 0x1F);
    pBuffer++;
	if (pPayloadH264->NalHdr.Type < 1|| pPayloadH264->NalHdr.Type>RVRTP_H264_FU_B)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264Unpack: bad NAL header type"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Unpack"));
		return RV_ERROR_BADPARAM;
	}
	if (pPayloadH264->NalHdr.Type >= 1 && pPayloadH264->NalHdr.Type<RVRTP_H264_STAP_A)
	{
		pPayloadH264->modes.pSingleNal = pBuffer;
		pBuffer += len - p->sByte - 1;
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_STAP_A)
	{
		RvUint32 Count = 0;
		while (pBuffer < ((RvUint8*)buf) + len && Count < RVRTP_H264_AP_NAL_UNITS_MAX)
		{
			rvDataBufferReadUint32(pBuffer, &data32);
			pPayloadH264->modes.STAP_A.Nal[Count].Size = (RvUint16)bitfieldGet(data32, 16, 16);
			pPayloadH264->modes.STAP_A.Nal[Count].StapHdr.F    = (RvUint32)bitfieldGet(data32, 15, 1);
			pPayloadH264->modes.STAP_A.Nal[Count].StapHdr.NRI  = (RvUint32)bitfieldGet(data32, 13, 2);
			pPayloadH264->modes.STAP_A.Nal[Count].StapHdr.Type = (RVRTP_H264NalTypes) bitfieldGet(data32, 8, 5);
			pBuffer --;
			pPayloadH264->modes.STAP_A.Nal[Count].pData = pBuffer;
			pBuffer += pPayloadH264->modes.STAP_A.Nal[Count].Size;
			Count++;
		}
	} else if(pPayloadH264->NalHdr.Type == RVRTP_H264_STAP_B)
	{
		RvUint32 Count = 0;
		rvDataBufferReadUint16(pBuffer, &pPayloadH264->modes.STAP_B.Don);
		while (pBuffer < ((RvUint8*)buf) + len && Count < RVRTP_H264_AP_NAL_UNITS_MAX)
		{
			rvDataBufferReadUint32(pBuffer, &data32);
			ReadValue = (RvUint32)bitfieldGet(data32, 16, 16);
			pPayloadH264->modes.STAP_B.Nal[Count].Size = (RvUint16)ReadValue;
			pPayloadH264->modes.STAP_B.Nal[Count].StapHdr.F    = (RvUint32)bitfieldGet(data32, 15, 1);
			pPayloadH264->modes.STAP_B.Nal[Count].StapHdr.NRI  = (RvUint32)bitfieldGet(data32, 13, 2);
			pPayloadH264->modes.STAP_B.Nal[Count].StapHdr.Type = (RVRTP_H264NalTypes)bitfieldGet(data32, 8, 5);
			pBuffer --;
			pPayloadH264->modes.STAP_B.Nal[Count].pData = pBuffer;
			pBuffer += pPayloadH264->modes.STAP_B.Nal[Count].Size;
			Count++;
		}
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_MTAP16||pPayloadH264->NalHdr.Type == RVRTP_H264_MTAP24)
	{
		RvUint32 Count = 0;
		rvDataBufferReadUint16(pBuffer, &pPayloadH264->modes.MTAP.Donb);
		while (pBuffer < ((RvUint8*)buf) + len && Count < RVRTP_H264_AP_NAL_UNITS_MAX)
		{
			rvDataBufferReadUint32(pBuffer, &data32);
			ReadValue = (RvUint32)bitfieldGet(data32, 16, 16);
			pPayloadH264->modes.MTAP.Nal[Count].Size = (RvUint16)ReadValue;
			pPayloadH264->modes.MTAP.Nal[Count].Dond    = (RvUint8) bitfieldGet(data32, 8, 8);
			pBuffer --;
			if (pPayloadH264->NalHdr.Type == RVRTP_H264_MTAP16)
			{
				rvDataBufferReadUint16(pBuffer, &data16);
				pPayloadH264->modes.MTAP.Nal[Count].TSoffset = data16;
			}
			else
			{
				rvDataBufferReadUint32(pBuffer, &data32);
				pPayloadH264->modes.MTAP.Nal[Count].TSoffset = (RvUint8) bitfieldGet(data32, 8, 24);
				pBuffer --;
			}
			pPayloadH264->modes.MTAP.Nal[Count].pData = pBuffer;
			pBuffer += pPayloadH264->modes.MTAP.Nal[Count].Size;
			Count++;
		}
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_FU_A)
	{
		pPayloadH264->modes.FU_A.FuHdr.S    = *pBuffer >> 7;
		pPayloadH264->modes.FU_A.FuHdr.E    = (*pBuffer & 0x40)>>6;
		pPayloadH264->modes.FU_A.FuHdr.R    = (*pBuffer & 0x20)>>4;
		pPayloadH264->modes.FU_A.FuHdr.Type = (*pBuffer & 0x1F);
		pBuffer++;
		pPayloadH264->modes.FU_A.pNalFragment = pBuffer;
		pBuffer += (len -  p->sByte - 2);
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_FU_B)
	{
		pPayloadH264->modes.FU_A.FuHdr.S    = *pBuffer >> 7;
		pPayloadH264->modes.FU_A.FuHdr.E    = (*pBuffer & 0x40)>>6;
		pPayloadH264->modes.FU_A.FuHdr.R    = (*pBuffer & 0x20)>>4;
		pPayloadH264->modes.FU_A.FuHdr.Type = (*pBuffer & 0x1F);
		pBuffer++;
		rvDataBufferReadUint16(pBuffer, &data16);
		pPayloadH264->modes.FU_B.Don = (RvUint16) data16;
		pPayloadH264->modes.FU_B.FU.pNalFragment = pBuffer;
		pBuffer +=  (len -  p->sByte - 4);
	}
	pPayloadH264->Length = (RvUint32)(pBuffer -((RvUint8*)buf+ p->sByte));

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Unpack"));
    return  RV_OK;
}
/********************************************************************************
* RvRtpH2641NalPack() serializes RvRtpPayloadH264 into buffer.
*                     This function is used when packet constrains 1 NAL unit.                      
* INPUT: buf - buffer to serialize
*        len - length of the buf
*        p   - pointer to RvRtpParam ( rtp header)
* OUT    param -  pointer to RvRtpPayloadH264 with 1NAL unit header values inside.
* result : return RV_OK if there are no errors
* Remark: Field Length of RvRtpPayloadH264 must be filled as 1.
*         This function does not perform coping of payload.
********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH2641NalPack(
                                  IN  void *      buf,
                                  IN  RvInt32     len,
                                  IN  RvRtpParam *  p,
                                  IN  void     *  param)
{
    RvRtpPayloadH264* pPayloadH264 = (RvRtpPayloadH264*) param;
    RV_UNUSED_ARG(len);
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH2641NalPack"));
    RvAssert (pPayloadH264->Length != 1);
    p->sByte-=1;
    ((RvUint8*)buf+p->sByte)[0]=(RvUint8)((pPayloadH264->NalHdr.F<<7) + (pPayloadH264->NalHdr.NRI<<5) + pPayloadH264->NalHdr.Type);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH2641NalPack"));
    return RV_OK;
}
/********************************************************************************
 * RvRtpH264Pack serializes aggregated H.264 payload into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH264
 * result : return RV_OK if there are no errors
 * Remark: 1) Field Length of RvRtpPayloadH264 must be filled
 *         2) This function performs additional copying and 
 *            not recommenced for usage when RTP packet should contain
 *            one NAL unit only. Use function RvRtpH2641NalPack() in this case.
 *         3) RvRtpH264Pack function serialises RvRtpPayloadH264 that includes 
 *            H.264 payload ao each NAL unit. 
 *            So no needs to place H.264 payload after calling RvRtpH264Pack()
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH264Pack(
	IN  void *      buf,
	IN  RvInt32     len,
	IN  RvRtpParam *  p,
	IN  void     *  param)
{
    RvUint8* pBuffer;
	RvRtpPayloadH264* pPayloadH264 = (RvRtpPayloadH264*) param;
	RvUint32 Length = 0;
	RvUint32 data32 = 0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264Pack"));

	if (NULL==buf||NULL==p||(RvInt32)(pPayloadH264->Length + p->sByte)>len)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264Pack: NULL pointer of not enough length"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Pack"));
		return RV_ERROR_BADPARAM;
	}
    pBuffer = ((RvUint8*)buf) + p->sByte; /* space for RTP header */
	if (pPayloadH264->NalHdr.Type < 1|| pPayloadH264->NalHdr.Type > RVRTP_H264_FU_B)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264Pack: bad NAL header type"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Pack"));
		return RV_ERROR_BADPARAM;
	}
	pBuffer[0] = (RvUint8)((pPayloadH264->NalHdr.F<<7) + (pPayloadH264->NalHdr.NRI<<5) + pPayloadH264->NalHdr.Type);
    pBuffer++;
	Length++;

	if (pPayloadH264->NalHdr.Type >= 1 && pPayloadH264->NalHdr.Type<RVRTP_H264_STAP_A)
	{
		memcpy(pBuffer, pPayloadH264->modes.pSingleNal, pPayloadH264->Length - Length);
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_STAP_A)
	{
		RvUint32 Count = 0;
		while (Length < pPayloadH264->Length && pPayloadH264->modes.STAP_A.Nal[Count].pData!=NULL)
		{
			data32 = bitfieldSet(0,      pPayloadH264->modes.STAP_A.Nal[Count].Size,         16, 16);
			data32 = bitfieldSet(data32, pPayloadH264->modes.STAP_A.Nal[Count].StapHdr.F,    15, 1);
			data32 = bitfieldSet(data32, pPayloadH264->modes.STAP_A.Nal[Count].StapHdr.NRI,  13, 2);
			data32 = bitfieldSet(data32, pPayloadH264->modes.STAP_A.Nal[Count].StapHdr.Type,  8, 5);
			rvDataBufferWriteEndUint32(pBuffer, data32);
			pBuffer --;
			Length += 3;
			memcpy(pBuffer, pPayloadH264->modes.STAP_A.Nal[Count].pData,
				pPayloadH264->modes.STAP_A.Nal[Count].Size);
			Length  += pPayloadH264->modes.STAP_A.Nal[Count].Size;
			pBuffer += pPayloadH264->modes.STAP_A.Nal[Count].Size;
			Count++;
		}
	} else if(pPayloadH264->NalHdr.Type == RVRTP_H264_STAP_B)
	{
		RvUint32 Count = 0;
		rvDataBufferWriteEndUint16(pBuffer, pPayloadH264->modes.STAP_B.Don);
		Length +=2;
		while (Length < pPayloadH264->Length && pPayloadH264->modes.STAP_B.Nal[Count].pData!=NULL)
		{
			data32 = bitfieldSet(0,      pPayloadH264->modes.STAP_B.Nal[Count].Size,         16, 16);
			data32 = bitfieldSet(data32, pPayloadH264->modes.STAP_B.Nal[Count].StapHdr.F,    15, 1);
			data32 = bitfieldSet(data32, pPayloadH264->modes.STAP_B.Nal[Count].StapHdr.NRI,  13, 2);
			data32 = bitfieldSet(data32, pPayloadH264->modes.STAP_B.Nal[Count].StapHdr.Type,  8, 5);
			rvDataBufferWriteEndUint32(pBuffer, data32);
			pBuffer --;
			Length += 3;
			memcpy(pBuffer, pPayloadH264->modes.STAP_B.Nal[Count].pData,
				pPayloadH264->modes.STAP_B.Nal[Count].Size);
			Length  += pPayloadH264->modes.STAP_B.Nal[Count].Size;
			pBuffer += pPayloadH264->modes.STAP_B.Nal[Count].Size;
			Count++;
		}
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_MTAP16||pPayloadH264->NalHdr.Type == RVRTP_H264_MTAP24)
	{
		RvUint32 Count = 0;
		rvDataBufferWriteEndUint16(pBuffer, pPayloadH264->modes.MTAP.Donb);
		Length +=2;
		while (Length < pPayloadH264->Length && pPayloadH264->modes.MTAP.Nal[Count].pData!=NULL)
		{
			data32 = bitfieldSet(0,      pPayloadH264->modes.MTAP.Nal[Count].Size,   16, 16);
			data32 = bitfieldSet(data32, pPayloadH264->modes.MTAP.Nal[Count].Dond,    8, 8);
			rvDataBufferWriteEndUint32(pBuffer, data32);
			pBuffer --;
			Length += 3;
			if (pPayloadH264->NalHdr.Type == RVRTP_H264_MTAP16)
			{
				rvDataBufferWriteEndUint16(pBuffer, pPayloadH264->modes.MTAP.Nal[Count].TSoffset);
				Length += 2;
			}
			else
			{
				data32 = bitfieldSet(0,   pPayloadH264->modes.MTAP.Nal[Count].TSoffset,   8, 24);
				rvDataBufferWriteEndUint32(pBuffer, data32);
				pBuffer --;
				Length += 3;
			}
			memcpy(pBuffer, pPayloadH264->modes.MTAP.Nal->pData,
				pPayloadH264->modes.MTAP.Nal[Count].Size);
			Length  += pPayloadH264->modes.MTAP.Nal[Count].Size;
			pBuffer += pPayloadH264->modes.MTAP.Nal[Count].Size;
			Count++;
		}
	}
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_FU_A)
    {
        data32 = bitfieldSet(0,        pPayloadH264->modes.FU_A.FuHdr.S,    7, 1);
        data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_A.FuHdr.E,    6, 1);        
        data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_A.FuHdr.R,    5, 1);        
        data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_A.FuHdr.Type, 0, 5);        
        *pBuffer = (RvUint8) data32;
        pBuffer++;
        memcpy(pBuffer, pPayloadH264->modes.FU_A.pNalFragment, pPayloadH264->Length - Length -1);
    }
	else if(pPayloadH264->NalHdr.Type == RVRTP_H264_FU_B)
	{
		data32 = bitfieldSet(0,        pPayloadH264->modes.FU_B.FU.FuHdr.S,  31, 1);
		data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_B.FU.FuHdr.E,  30, 1);
		data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_B.FU.FuHdr.R,  29, 1);
		data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_B.FU.FuHdr.Type, 24, 5);
		data32 = bitfieldSet(data32,   pPayloadH264->modes.FU_B.Don, 8, 16);
		rvDataBufferWriteEndUint32(pBuffer, data32);
        pBuffer --;
		memcpy(pBuffer, pPayloadH264->modes.FU_B.FU.pNalFragment, pPayloadH264->Length - Length - 3);
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264Pack"));
	return RV_OK;
}

#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
RVAPI
RvInt32 RVCALLCONV RvRtpPayloadH264ToString(
	const RvRtpPayloadH264* thisPtr,
	char *buffer,
	size_t bufferSize,
	const char *prefix)
{
    RvUint32 offset = 0, subNal = 0, Count = 0;
	RvUint32 Curr = 0; /* current byte */
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));

	if (thisPtr==NULL)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
		return RV_ERROR_BADPARAM;
	}
	if (thisPtr->NalHdr.Type>RVRTP_H264_FU_B||thisPtr->NalHdr.Type<1)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString: bad NAL header type"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
		return RV_ERROR_BADPARAM;
	}
	offset += sprintf(&buffer[offset],"%sCodec: H264 F    (1):%d\n", prefix, thisPtr->NalHdr.F);
	offset += sprintf(&buffer[offset],"%sCodec: H264 NRI  (2):%d\n", prefix, thisPtr->NalHdr.NRI);
	offset += sprintf(&buffer[offset],"%sCodec: H264 Type (5):%d\n", prefix, thisPtr->NalHdr.Type);
	Curr ++;


	switch(thisPtr->NalHdr.Type)
	{
	case RVRTP_H264_STAP_A:	/* Single-time aggregation packet */
	case RVRTP_H264_STAP_B:	/* Single-time aggregation packet */
		{
			RvRtpH264StapNalU* pNalU = NULL;

			if (thisPtr->NalHdr.Type == RVRTP_H264_STAP_B)
			{
			   offset += sprintf(&buffer[offset],"%sCodec: STAP_B Don (16):%#x\n", prefix, thisPtr->modes.STAP_B.Don);
			   Curr +=2;
			   pNalU = (RvRtpH264StapNalU*) &thisPtr->modes.STAP_B.Nal[0];
			}
			else
			   pNalU = (RvRtpH264StapNalU*) &thisPtr->modes.STAP_A.Nal[0];

			while((Curr<thisPtr->Length) && (subNal<RVRTP_H264_AP_NAL_UNITS_MAX)&&pNalU[subNal].pData)
			{
				offset += sprintf(&buffer[offset],"%sCodec: STAP Size (16):%d\n",
					prefix, pNalU[subNal].Size);
				Curr +=2;
				offset += sprintf(&buffer[offset],"%sCodec: STAP Header [F(1):%d,NRI(2):%d,Type(5):%d]\n",
					prefix,	pNalU[subNal].StapHdr.F, pNalU[subNal].StapHdr.NRI,	pNalU[subNal].StapHdr.Type);
				Curr ++;

				offset += sprintf(&buffer[offset],"%sCodec: STAP [%d] NAL(%d Bytes):",
					prefix, subNal, pNalU[subNal].Size);

				if ((pNalU[subNal].Size) > (1500-41))
				{
					RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString: too big packet"));
					RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
					return RV_RTPSTATUS_WrongParameter;
				}
				for (Count=0; Count < pNalU[subNal].Size; Count++)
				{
					if (RV_MODULO16(Count)==0)
						offset += sprintf(&buffer[offset],"\n%sCodec: STAP[%d] NAL data + %d:", prefix, subNal, Count);
					offset += sprintf(&buffer[offset],"%x%x", pNalU[subNal].pData[Count] >>4, pNalU[subNal].pData[Count]&0x0F);
				}
				offset += sprintf(&buffer[offset],"\n");
				subNal++;
			}
		}
	break;
	case RVRTP_H264_MTAP16:	/* Multi-time aggregation packet */
	case RVRTP_H264_MTAP24:	/* Multi-time aggregation packet */
		{
			offset += sprintf(&buffer[offset],"%sCodec: MTAP Donb (16):%#x\n", prefix, thisPtr->modes.MTAP.Donb);
			Curr +=2;
			while(Curr< thisPtr->Length&&subNal<RVRTP_H264_AP_NAL_UNITS_MAX&&thisPtr->modes.MTAP.Nal[subNal].pData)
			{
				offset += sprintf(&buffer[offset],"%sCodec: MTAP[%d] Size (16):%#x\n", prefix, subNal, thisPtr->modes.MTAP.Nal[subNal].Size);
				Curr +=2;
				offset += sprintf(&buffer[offset],"%sCodec: MTAP[%d] Dond (8):%#x\n", prefix, subNal, thisPtr->modes.MTAP.Nal[subNal].Dond);
				Curr ++;
				if (thisPtr->NalHdr.Type==RVRTP_H264_MTAP16)
				{
					offset += sprintf(&buffer[offset],"%sCodec: MTAP[%d] TSoffset (16):%#x\n", prefix, subNal, thisPtr->modes.MTAP.Nal[subNal].TSoffset);
					Curr +=2;
                }
				else
				{
					offset += sprintf(&buffer[offset],"%sCodec: MTAP[%d] TSoffset (24):%#x\n", prefix, subNal, thisPtr->modes.MTAP.Nal[subNal].TSoffset);
					Curr +=3;
				}

				offset += sprintf(&buffer[offset],"%sCodec: MTAP[%d] NAL(%d Bytes):", prefix, subNal, thisPtr->modes.MTAP.Nal[subNal].Size);
				if ((thisPtr->modes.MTAP.Nal[subNal].Size) > (1500-41))
				{
					RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString: too big packet"));
					RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
					return RV_RTPSTATUS_WrongParameter;
				}

				for (Count=0;Count < thisPtr->modes.MTAP.Nal[subNal].Size;Count++)
				{
					if (RV_MODULO16(Count)==0)
						offset += sprintf(&buffer[offset],"\n%sCodec: MTAP[%d] NAL data + %d:", prefix, subNal, Count);
					offset += sprintf(&buffer[offset],"%x%x", thisPtr->modes.MTAP.Nal[subNal].pData[Count] >>4,
															  thisPtr->modes.MTAP.Nal[subNal].pData[Count]&0x0F);
				}
			    offset += sprintf(&buffer[offset],"\n");
				subNal++;
			}
           	break;
		}
	case RVRTP_H264_FU_A:	/* Fragmentation unit */
	case RVRTP_H264_FU_B:	/* Fragmentation unit */
		{
            RvRtpH264FU_A* pFU = NULL;
			if (thisPtr->NalHdr.Type==RVRTP_H264_FU_A)
			{
			    offset += sprintf(&buffer[offset],"%sCodec: FU_A:\n", prefix);
				pFU = (RvRtpH264FU_A*) &thisPtr->modes.FU_A;
			}
			else
			{
				offset += sprintf(&buffer[offset],"%sCodec: FU_B:\n", prefix);
				pFU = (RvRtpH264FU_A*) &thisPtr->modes.FU_B.FU;
			}
			offset += sprintf(&buffer[offset],"%sCodec: Header(8) [S(1):%d,E(1):%d,R(1):%d,Type(5):%d]\n",
				prefix,	pFU->FuHdr.S, pFU->FuHdr.E,	pFU->FuHdr.R, pFU->FuHdr.Type);
			Curr ++;
			if (thisPtr->NalHdr.Type==RVRTP_H264_FU_B)
			{
				offset += sprintf(&buffer[offset],"%sCodec: FU_B DON(16) :%d\n", prefix,	thisPtr->modes.FU_B.Don);
				Curr +=2;
			}
			offset += sprintf(&buffer[offset],"%sCodec: FU(%d Bytes):\n", prefix, thisPtr->Length-Curr);
			if ((thisPtr->Length-Curr) > (1500-Curr))
			{
				RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString: too big packet"));
				RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
				return RV_RTPSTATUS_WrongParameter;
			}
			for (Count=0;Count < thisPtr->Length-Curr;Count++)
			{
				if (RV_MODULO16(Count)==0)
					offset += sprintf(&buffer[offset],"\n%sCodec: FU data + %d:", prefix, Count);
				offset += sprintf(&buffer[offset],"%x%x", pFU->pNalFragment[Count] >>4, pFU->pNalFragment[Count]&0x0F);
			}
			offset += sprintf(&buffer[offset],"\n");
			break;
		}
	default: /* non-aggregated packet */
		{
			offset += sprintf(&buffer[offset],"%sNAL(%d Bytes):", prefix, thisPtr->Length-Curr);
			if ((thisPtr->Length-Curr) > (1500-41))
			{
				RvLogError(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString: too big packet"));
				RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
				return RV_RTPSTATUS_WrongParameter;
			}
			for (Count=0;Count < thisPtr->Length-1;Count++)
			{
				if (RV_MODULO16(Count)==0)
					offset += sprintf(&buffer[offset],"\n%sNAL data + %d:", prefix, Count);
				offset += sprintf(&buffer[offset],"%x%x", thisPtr->modes.pSingleNal[Count] >>4,
					thisPtr->modes.pSingleNal[Count]&0x0F);
			}
			offset += sprintf(&buffer[offset],"\n");
			break;
		}
	}
	RV_UNUSED_ARG(bufferSize);

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpPayloadH264ToString"));
    return offset;
}
#endif
RVAPI
RvInt32 RVCALLCONV RvRtpH264GetLength(const RvRtpPayloadH264* thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpH264GetLength"));
	if (NULL==thisPtr)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpH264GetLength: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264GetLength"));
		return RV_ERROR_NULLPTR;
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpH264GetLength"));
	return RvRtpGetHeaderLength() + thisPtr->Length;
}

/********************************************************************************
 * RvRtpJpegPack serialises RvRtpPayloadJpeg into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadJpeg
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpJpegPack(IN    void*buf,
								 IN    RvInt32 len,
								 IN    RvRtpParam*p,
								 IN    void*param)
{
    RvRtpPayloadJpeg* jpeg=(RvRtpPayloadJpeg*)param;
    RvUint32*hPtr=NULL;
    RvInt32 dwords=0, Count = 0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpJpegPack"));
    RV_UNUSED_ARG(len);

    if (jpeg->Type < 64 && jpeg->Q < 128)
	{ /* only basic header */
		dwords=2;
		p->sByte-=8;
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	}

    if (jpeg->Type < 64 && jpeg->Q >= 128)
	{ /* only basic header */
		dwords= (3+jpeg->Length);
		p->sByte -= 4*dwords;
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

		hPtr[2]=bitfieldSet(      0, jpeg->Length,          0,  16);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->Precision,       16,  8);
		hPtr[2]=bitfieldSet(hPtr[2], 0 /* MBZ */,             24,  8);
		for (Count=0;Count<jpeg->Length;Count++)
		  hPtr[3+Count]=bitfieldSet(0, ((RvUint32*)jpeg->QTdata)[Count], 0,  32);
	}

    if ((jpeg->Type >= 64 && jpeg->Type <= 127) && jpeg->Q < 128)
	{ /* with Restart Marker header appears immediately following the main JPEG header */
		dwords=3;
		p->sByte-= 12;
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

		hPtr[2]=bitfieldSet(      0, jpeg->RestartCount,     0,  14);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->L,               14,  1);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->F,               15,  1);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->RestartInterval, 16,  16);
	}

    if ((jpeg->Type >= 64 && jpeg->Type <= 127) && jpeg->Q >= 128)
	{ /* with Restart Marker header appears immediately following the main JPEG header */
		dwords=(4+jpeg->Length);
		p->sByte -= 4*dwords;
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

		hPtr[2]=bitfieldSet(      0, jpeg->RestartCount,     0,  14);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->L,               14,  1);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->F,               15,  1);
		hPtr[2]=bitfieldSet(hPtr[2], jpeg->RestartInterval, 16,  16);

		hPtr[3]=bitfieldSet(      0, jpeg->Length,          0,  16);
		hPtr[3]=bitfieldSet(hPtr[3], jpeg->Precision,       16,  8);
		hPtr[3]=bitfieldSet(hPtr[3], 0 /* MBZ */,             24,  8);
		if (jpeg->Length>0)
		for (Count=0;Count<jpeg->Length;Count++)
			hPtr[4+Count]=bitfieldSet(0, ((RvUint32*)jpeg->QTdata)[Count], 0,  32);
	}

	hPtr[0]=bitfieldSet(0,       jpeg->FragmentOffset,  0, 24);
	hPtr[0]=bitfieldSet(hPtr[0], jpeg->Type_specific,  24,  8);

	hPtr[1]=bitfieldSet(      0, jpeg->Height,          0,  8);
	hPtr[1]=bitfieldSet(hPtr[1], jpeg->Width,           8,  8);
	hPtr[1]=bitfieldSet(hPtr[1], jpeg->Q,              16,  8);
	hPtr[1]=bitfieldSet(hPtr[1], jpeg->Type,           24,  8);

    p->payload=RV_RTP_PAYLOAD_JPEG;
    ConvertToNetwork(hPtr, 0, dwords);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpJpegPack"));
    return RV_OK;
}

/********************************************************************************
 * RvRtpJpegUnpack reads buffer and fills Jpeg payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadJpeg
 * result : return RV_OK if there are no errors
 ********************************************************************************/

RvInt32 RVCALLCONV RvRtpJpegUnpack(OUT void*buf,
								   IN  RvInt32 len,
								   OUT RvRtpParam*p,
								   OUT void*param)
{
    RvRtpPayloadJpeg*jpeg=(RvRtpPayloadJpeg*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	RvInt32 Count = 0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpJpegUnpack"));
    RV_UNUSED_ARG(len);

    ConvertFromNetwork(hPtr, 0, 2);
    p->sByte += 8;
    jpeg->Type_specific  = bitfieldGet(hPtr[0], 24, 8);
    jpeg->FragmentOffset = bitfieldGet(hPtr[0], 0, 24);
    jpeg->Height         = bitfieldGet(hPtr[1],  0,  8);
    jpeg->Width          = bitfieldGet(hPtr[1],  8,  8);
    jpeg->Q              = bitfieldGet(hPtr[1], 16,  8);
    jpeg->Type           = bitfieldGet(hPtr[1], 24,  8);

    if ((jpeg->Type >= 64 && jpeg->Type <= 127))
	{
	    p->sByte += 4;
		ConvertFromNetwork(hPtr, 2, 1);
		jpeg->RestartCount    = bitfieldGet(hPtr[2],  0,  14);
		jpeg->L               = bitfieldGet(hPtr[2],  14,  1);
		jpeg->F               = bitfieldGet(hPtr[2],  15,  1);
		jpeg->RestartInterval = bitfieldGet(hPtr[2],  16,  16);
	}
    if ((jpeg->Q >= 128))
	{
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	    p->sByte += 4;
		ConvertFromNetwork(hPtr, 0, 1);
		jpeg->Length          = bitfieldGet(hPtr[0],  0,  16);
		jpeg->Precision       = bitfieldGet(hPtr[0],  16,  8);
		jpeg->MBZ             = bitfieldGet(hPtr[0],  24,  8); /* must be zero */
		if (jpeg->Length>0)
		{
			p->sByte += 4*jpeg->Length;
			ConvertFromNetwork(hPtr, 1, jpeg->Length);
			for (Count = 0; Count < jpeg->Length; Count++)
				jpeg->QTdata[Count] = bitfieldGet(hPtr[1+Count],  0,  32);
		}
	}

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpJpegUnpack"));
    return RV_OK;
}

/********************************************************************************
 * RvRtpJpegGetLength returns the full size JPEG header includes RTP header,
 * input: thisPtr -  pointer to RvRtpPayloadJpeg, that must be already filled
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/

RVAPI
RvInt32 RVCALLCONV RvRtpJpegGetLength(IN const RvRtpPayloadJpeg* thisPtr)
{
    RvUint32 Count = 0;
    if (thisPtr->Type>=64 && thisPtr->Type<=127) Count++;
    if (thisPtr->Q>=128) Count += (1+thisPtr->Length);
	return RvRtpGetHeaderLength()+ 8 + (Count*4);
}


/********************************************************************************
 * RvRtpMpegVUnpack reads buffer and fills Video MPEG-1/2 payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegVideo
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegVUnpack(OUT void*buf,
								   IN  RvInt32 len,
								   OUT RvRtpParam*p,
								   OUT void*param)
{
	RvRtpPayloadMpegVideo*mpeg=(RvRtpPayloadMpegVideo*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpegVUnpack"));
    RV_UNUSED_ARG(len);

    ConvertFromNetwork(hPtr, 0, 1);
    p->sByte += 4;
    mpeg->MBZ   = bitfieldGet(hPtr[0], 27, 5); /* must be zero */
    mpeg->T     = bitfieldGet(hPtr[0], 26, 1);
    mpeg->TR    = bitfieldGet(hPtr[0], 16, 10);
    mpeg->AN    = bitfieldGet(hPtr[0], 15, 1);
    mpeg->N     = bitfieldGet(hPtr[0], 14, 1);
    mpeg->S     = bitfieldGet(hPtr[0], 13, 1);
    mpeg->B     = bitfieldGet(hPtr[0], 12, 1);
    mpeg->E     = bitfieldGet(hPtr[0], 11, 1);
    mpeg->P     = bitfieldGet(hPtr[0],  8, 3);
    mpeg->FBV   = bitfieldGet(hPtr[0],  7, 1);
    mpeg->BFC   = bitfieldGet(hPtr[0],  4, 3);
    mpeg->FFV   = bitfieldGet(hPtr[0],  3, 1);
    mpeg->FFC   = bitfieldGet(hPtr[0],  0, 3);

    if (mpeg->T)
	{
       hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	   ConvertFromNetwork(hPtr, 0, 1);
	   p->sByte += 4;
	   mpeg->mpeg2ext.X    = bitfieldGet(hPtr[0], 31, 1); /* must be zero */
	   mpeg->mpeg2ext.E    = bitfieldGet(hPtr[0], 30, 1);
	   mpeg->mpeg2ext.f_00 = bitfieldGet(hPtr[0], 26, 4);
	   mpeg->mpeg2ext.f_01 = bitfieldGet(hPtr[0], 22, 4);
	   mpeg->mpeg2ext.f_10 = bitfieldGet(hPtr[0], 18, 4);
	   mpeg->mpeg2ext.f_11 = bitfieldGet(hPtr[0], 14, 4);
	   mpeg->mpeg2ext.DC   = bitfieldGet(hPtr[0], 12, 2);
	   mpeg->mpeg2ext.PS   = bitfieldGet(hPtr[0], 10, 2);
	   mpeg->mpeg2ext.T    = bitfieldGet(hPtr[0],  9, 1);
	   mpeg->mpeg2ext.P    = bitfieldGet(hPtr[0],  8, 1);
	   mpeg->mpeg2ext.C    = bitfieldGet(hPtr[0],  7, 1);
	   mpeg->mpeg2ext.Q    = bitfieldGet(hPtr[0],  6, 1);
	   mpeg->mpeg2ext.V    = bitfieldGet(hPtr[0],  5, 1);
	   mpeg->mpeg2ext.A    = bitfieldGet(hPtr[0],  4, 1);
	   mpeg->mpeg2ext.R    = bitfieldGet(hPtr[0],  3, 1);
	   mpeg->mpeg2ext.H    = bitfieldGet(hPtr[0],  2, 1);
	   mpeg->mpeg2ext.G    = bitfieldGet(hPtr[0],  1, 1);
	   mpeg->mpeg2ext.D    = bitfieldGet(hPtr[0],  0, 1);
	}
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpegVUnpack"));
	return RV_OK;
}
/********************************************************************************
 * RvRtpMpegVPack serialises RvRtpPayloadMpegVideo into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegVideo
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegVPack(IN    void*buf,
								 IN    RvInt32 len,
								 IN    RvRtpParam*p,
								 IN    void*param)
{
	RvRtpPayloadMpegVideo* mpeg=(RvRtpPayloadMpegVideo*)param;
    RvUint32*hPtr=NULL;
    RvInt32 dwords=0;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpegVPack"));
    RV_UNUSED_ARG(len);

    if (mpeg->T)
	{ /* only basic header */
		dwords=2;
		p->sByte-=8;
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

		hPtr[1]=  bitfieldSet(0, 0/* X must be zero */,     31, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.E,    30, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.f_00, 26, 4);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.f_01, 22, 4);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.f_10, 18, 4);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.f_11, 14, 4);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.DC,   12, 2);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.PS,   10, 2);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.T,     9, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.P,     8, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.C,     7, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.Q,     6, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.V,     5, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.A,     4, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.R,     3, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.H,     2, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.G,     1, 1);
		hPtr[1]=  bitfieldSet(hPtr[1], mpeg->mpeg2ext.D,     0, 1);
	}
	else
	{
		dwords=1;
		p->sByte-=4;
		hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	}

	hPtr[0] = bitfieldSet(0,  /* MBZ*/   0,  27, 5);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->T,  26, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->TR, 16, 10);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->AN, 15, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->N,  14, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->S,  13, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->B,  12, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->E,  11, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->P,   8, 3);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->FBV, 7, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->BFC, 4, 3);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->FFV, 3, 1);
	hPtr[0] = bitfieldSet(hPtr[0], mpeg->FFC, 0, 3);

    p->payload = RV_RTP_PAYLOAD_MPV;
    ConvertToNetwork(hPtr, 0, dwords);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpegVPack"));
	return RV_OK;
}
/********************************************************************************
 * RvRtpMpegAUnpack reads buffer and fills Audio MPEG-1/2 payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegAudio
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegAUnpack(OUT void*buf,
								   IN  RvInt32 len,
								   OUT RvRtpParam*p,
								   OUT void*param)
{
	RvRtpPayloadMpegAudio*mpega=(RvRtpPayloadMpegAudio*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpegAUnpack"));
    RV_UNUSED_ARG(len);

    ConvertFromNetwork(hPtr, 0, 1);
    p->sByte += 4;
    mpega->MBZ         = bitfieldGet(hPtr[0], 16, 16); /* must be zero */
    mpega->Frag_offset = bitfieldGet(hPtr[0], 0,  16);

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpegAUnpack"));
	return RV_OK;
}
/********************************************************************************
 * RvRtpMpegAPack serialises RvRtpPayloadMpegAudio into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegAudio
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegAPack(IN    void*buf,
								  IN    RvInt32 len,
								  IN    RvRtpParam*p,
								  IN    void*param)
{
	RvRtpPayloadMpegAudio* mpega=(RvRtpPayloadMpegAudio*)param;
    RvUint32*hPtr=NULL;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpegAPack"));
    RV_UNUSED_ARG(len);

	p->sByte-=4;
	hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

	hPtr[0]=  bitfieldSet(0, 0/* X must be zero */,     16, 16);
	hPtr[0]=  bitfieldSet(hPtr[0], mpega->Frag_offset,   0, 16);

	p->payload = RV_RTP_PAYLOAD_MPA;
	ConvertToNetwork(hPtr, 0, 1);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpegAPack"));
	return RV_OK;
}
/********************************************************************************
 * RvRtpMpegVGetLength returns the full size MPEG Video header includes RTP header,
 * input: thisPtr -  pointer to RvRtpPayloadMpegAudio, that must be already filled
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpMpegVGetLength(IN const RvRtpPayloadMpegVideo* thisPtr)
{
	RvInt32 Ext = 0;

	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpegVGetLength"));
	if (thisPtr->T) Ext = 4;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpegVGetLength"));

    return (RvRtpGetHeaderLength() + 4 + Ext);
}
/********************************************************************************
 * RvRtpMpegAGetLength returns the full size MPEG Audio header includes RTP header,
 * input: thisPtr -  pointer to RvRtpPayloadMpegAudio, that must be already filled
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpMpegAGetLength(IN const RvRtpPayloadMpegAudio* thisPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpMpegAGetLength"));
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpMpegAGetLength"));
	RV_UNUSED_ARG(thisPtr);
	return RvRtpGetHeaderLength() + 4;
}

/********************************************************************************
 * RvRtpG7221Pack  sets standard payload type of g722.1 to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to g722.1 (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG7221Pack(
                                  IN    void*buf,
                                  IN    RvInt32 len,
                                  IN    RvRtpParam*p,
                                  IN    void*param)
{
    RTPLOG_ENTER(G7221Pack);
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);            /* uses dynamic payload type */
    RTPLOG_LEAVE(G7221Pack);
    return RV_OK;
}

/********************************************************************************
 * RvRtpG7221Unpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG7221Unpack(
                                    OUT void*buf,
                                    IN  RvInt32 len,
                                    OUT RvRtpParam*p,
                                    OUT void*param)
{
    RTPLOG_ENTER(G7221Unpack);
    RV_UNUSED_ARG(buf);
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(param);
    RV_UNUSED_ARG(p);          /* uses dynamic payload type */
    RTPLOG_LEAVE(G7221Unpack);
    return RV_OK;
}

/********************************************************************************
 * RvRtpG7221GetHeaderLength
 * description: returns the size of RTP header. (no g.722.1 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG7221GetHeaderLength(void)
{
    RTPLOG_ENTER(G7221GetHeaderLength);
    RTPLOG_LEAVE(G7221GetHeaderLength);
    return RvRtpGetHeaderLength();
}

/********************************************************************************
 * RvRtpH263PlusPack serializes H263+ payload header into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpH263Plus
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH263PlusPack(
    IN	void*        buf,
    IN	RvInt32      len,
    IN	RvRtpParam*  p,
    IN	void*        param)
{
    RvRtpH263Plus* h263   = (RvRtpH263Plus*)param;
    RvUint16*      hPtr   =  NULL;
    RvUint8*       vrc;
    RV_UNUSED_ARG(len);
    RTPLOG_ENTER(H263PlusPack);
    
    /* At the beginning p->sByte must point to the next byte after the end of the H263+ payload header */

    p->sByte -= (2 + h263->pLen + (h263->v==0?0:1));

    /* hPtr points to the first byte of the RFC 2429 payload header */
    hPtr     = (RvUint16*)((RvUint8*)buf+p->sByte);
    hPtr[0]  = (RvUint16) bitfieldSet2(0,           0, 11, 5);/* RR */
    hPtr[0]  = (RvUint16) bitfieldSet2((RvUint16)h263->p,     0, 10, 1);/* P */
    hPtr[0]  = (RvUint16) bitfieldSet2((RvUint16)h263->v,     0,  9, 1);/* V */
    hPtr[0]  = (RvUint16) bitfieldSet2((RvUint16)h263->pLen,  0,  3, 6);/* PLEN */
    hPtr[0]  = (RvUint16) bitfieldSet2((RvUint16)h263->pEbit, 0,  0, 3);/* PEBIT */

    /* p->payload >= 96; Dynamic payload type */
	ConvertToNetwork2(hPtr, 0, 1);        
    p->sByte +=2;
    vrc = ((RvUint8*)buf + p->sByte);
    if (h263->v)
    {
       /* Video Redundancy Coding Header Extension */
       *vrc = (RvUint8) bitfieldSet(h263->Tid,  0,  5, 3);/* Tid */
       *vrc = (RvUint8) bitfieldSet(h263->Trun, 0,  1, 4);/* Trun */
       *vrc = (RvUint8) bitfieldSet(h263->s,    0,  0, 1);/* s */
       p->sByte ++;
       vrc++;
    } 
    if (h263->p) /* P==1 */
    {
        if (h263->pLen)
        {
            p->sByte += h263->pLen;
            /* first 2 zero bytes are ommited */
            memcpy(vrc, h263->pGBSCorSSC + 2, h263->pLen);
        }    
        /* first 2 zero bytes of bit-stream must be ommited as well 
           as per RFC 2429 5.1 */
    }
    RTPLOG_LEAVE(H263PlusPack);
    return RV_OK;
}

/********************************************************************************
 * RvRtpH263PlusUnpack reads buffer and fills H263+ payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpH263Plus
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV   RvRtpH263PlusUnpack(
    OUT     void*       buf,
    IN	    RvInt32     len,
    OUT     RvRtpParam* p,
    OUT	    void*       param)
{
    RvRtpH263Plus *h263 = (RvRtpH263Plus*)param;
    RvUint16 *hPtr      = (RvUint16 *)((RvUint8*)buf+p->sByte);
    RvUint8*  vrc = NULL;
    RV_UNUSED_ARG(len);
    RTPLOG_ENTER(H263PlusUnpack);

    p->sByte    += 2;

    ConvertFromNetwork2(hPtr, 0, 1);

    h263->p     = bitfieldGet2(hPtr[0], 10, 1);
    h263->v     = bitfieldGet2(hPtr[0],  9, 1);
    h263->pLen  = bitfieldGet2(hPtr[0],  3, 6);
    h263->pEbit = bitfieldGet2(hPtr[0],  0, 3);
    vrc = (RvUint8*)&hPtr[1];

    if (h263->v)
    {
        p->sByte += 1;
        h263->Tid  =  bitfieldGet(*vrc, 5, 3);/* Tid */
        h263->Trun =  bitfieldGet(*vrc, 1, 4);/* Trun */
        h263->s    =  bitfieldGet(*vrc, 0, 1);/* s */
        vrc++;
    }

    if (h263->p)
    {   
        if (h263->pLen > RVRTP_H263PLUS_MAX_PLEN_VALUE)
        {

            RTPLOG_ERROR1(H263PlusUnpack, "pLen is out of range or RVRTP_H263PLUS_MAX_PLEN_VALUE must be increased");
            RTPLOG_LEAVE(H263PlusUnpack);            
            return RV_ERROR_OUTOFRANGE;
        }

        if (h263->pLen)
        {       
            p->sByte += h263->pLen;
            memcpy(&h263->pLenInfo[2], vrc, h263->pLen);
            h263->pLenInfo[0] = 0;  /* setting of 2 first zero bytes of psc */
            h263->pLenInfo[1] = 0;
        }
        p->sByte -= 2;
        ((RvUint8*)buf)[p->sByte]   = 0;
        ((RvUint8*)buf)[p->sByte+1] = 0;
    }
    RTPLOG_LEAVE(H263PlusUnpack);
    return 0;
}

/********************************************************************************
 * RvRtpH263PlusGetHeaderLength()
 * Description: returns the full size H263+ Video header 
 *              includes RTP header,
 * input: h263plus -  pointer to RvRtpH263Plus, that must be already filled
 * output: none
 * result : returns the full size H263+ Video header includes RTP header
 *          or negative value on error
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH263PlusGetHeaderLength(IN RvRtpH263Plus *h263plus)
{  
    RTPLOG_ENTER(H261GetHeaderLength);
    if (h263plus == NULL)
    {
            RTPLOG_ERROR1(H261GetHeaderLength, "NULL pointer");
            RTPLOG_LEAVE(H261GetHeaderLength);            
            return RV_ERROR_UNKNOWN;
    }

    RTPLOG_LEAVE(H261GetHeaderLength);
    return RvRtpGetHeaderLength() + 2 + h263plus->pLen + (h263plus->v==0?0:1);
}



/********************************************************************************
 * RvRtpEvrcSmvPack serializes RvRtpEvrcSmv into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpEvrcSmv
 * result : return RV_OK if there are no errors
 * Remark: Field Length of RvRtpEvrcSmv must be filled
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpEvrcSmvPack(
	IN  void *			buf,
	IN  RvInt32			len,
	IN  RvRtpParam *	p,
	IN  RvRtpEvrcSmv *  param)
{
	RvUint16 *hPtr      = (RvUint16 *)((RvUint8*)buf+p->sByte);
    RvUint16  words=1,shift;
	RvUint32 i;

	RV_UNUSED_ARG(len);

    RTPLOG_ENTER(RvRtpEvrcSmvPack);
 
	hPtr[0]=bitfieldSet2(0,param->rr,0,RR_BITS);
	hPtr[0]=bitfieldSet2(hPtr[0],param->lll,2,LLL_BITS);
	hPtr[0]=bitfieldSet2(hPtr[0],param->nnn,5,NNN_BITS);
	hPtr[0]=bitfieldSet2(hPtr[0],param->mmm,8,MMM_BITS);
	hPtr[0]=bitfieldSet2(hPtr[0],param->count,11,COUNT_BITS);

	shift = 0;
	for (i=0; i<=param->count; i++)
	{
		if ((i)%4 == 0 && i > 0)
		{
			words++;
			shift = 0;
			hPtr[words] = 0;
		} 
		hPtr[words] = bitfieldSet2(hPtr[words],param->toc[i],shift,TOC_BITS);
		shift+=TOC_BITS;
	}
	if (param->count%2 == 0)
	{
		hPtr[words] = bitfieldSet2(hPtr[words],/*param->padding*/0,shift,PADDING_BITS);
		p->sByte-=((param->count+2)/2); /* ??? Check */
	} else
	{	
		p->sByte-=((param->count+1)/2); /* ??? Check */
	}

    ConvertToNetwork2(hPtr, 0, words);
    RTPLOG_LEAVE(RvRtpEvrcSmvPack);
    return RV_OK;
}

/********************************************************************************
 * RvRtpEvrcSmvUnpack reads buffer and fills RvRtpEvrcSmv payload
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpEvrcSmv
 * result : return RV_OK if there are no errors
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpEvrcSmvUnpack(
	IN   void *			  buf,
	IN   RvInt32		  len,
	IN   RvRtpParam *	  p,
	OUT  RvRtpEvrcSmv *   param)
{
	RvUint16*hPtr=(RvUint16*)((RvUint8*)buf+p->sByte);
	RvUint16  words=1,shift;
	RvUint32 i;

	RV_UNUSED_ARG(len);

    RTPLOG_ENTER(RvRtpEvrcSmvUnpack);
	
	p->sByte+=2;
    ConvertFromNetwork2(hPtr, 0, 1);	

	param->rr    = bitfieldGet2(hPtr[0],0,RR_BITS);
	param->lll   = bitfieldGet2(hPtr[0],2,LLL_BITS);
    param->nnn   = bitfieldGet2(hPtr[0],5,NNN_BITS);
	param->mmm   = bitfieldGet2(hPtr[0],8,MMM_BITS);
	param->count = bitfieldGet2(hPtr[0],11,COUNT_BITS);

	shift = 0;
	ConvertFromNetwork2(hPtr+words, 0, 1);	
	for (i=0; i<=param->count; i++)
	{
		if ((i)%4 == 0 && i > 0)
		{
			words++;
			shift = 0;
			ConvertFromNetwork2(hPtr+words, 0, 1);
		} 
		param->toc[i] = bitfieldGet2(hPtr[words],shift,TOC_BITS);
		shift+=TOC_BITS;
	}

	if (param->count%2 == 0)
	{
		p->sByte+=((param->count+2)/2); /* ??? Check */
	} else
	{	
		p->sByte+=((param->count+1)/2); /* ??? Check */
	}

    RTPLOG_LEAVE(RvRtpEvrcSmvUnpack);
	return RV_OK;
}
/*******************************************************************************
 * RFC 3389                                                                    *
 * Real-time Transport Protocol (RTP) Payload for Comfort Noise (CN)           *
 *******************************************************************************/

/********************************************************************************
 * RvRtpCnPack serializes RvRtpComfNoisePayload into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpConfNoisePayload
 * result : return RV_OK if there are no errors
 * Remarks: 1. modelOrder must be filled. Assumption: 
 *                modelOrder <= RVRTP_CN_MAX_MODEL_ORDER
 *          2. After calling of RvRtpCnPack() function use RvRtpWrite() function
 *             with len parameter equal to p->sByte for sending RTP packet 
 *             with Comfort Noise.
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpCnPack(
	IN  void *			buf,
	IN  RvInt32			len,
	IN  RvRtpParam *	p,
	IN  RvRtpComfNoisePayload*  param)
{
    RvUint8* cnStart; 
    RTPLOG_ENTER(CnPack);
    RV_UNUSED_ARG(len);
    if (param == NULL|| p == NULL)
    {
        RTPLOG_ERROR1(CnPack, "NULL pointer");
        RTPLOG_LEAVE(CnPack);  
        return RV_ERROR_NULLPTR;
    }
    if ((RvRtpGetHeaderLength() + param->modelOrder + 1) < len) 
    {
        RTPLOG_ERROR1(CnPack, "Unsufficient buffer space");
        RTPLOG_LEAVE(CnPack);  
        return RV_ERROR_INSUFFICIENT_BUFFER;
    }  
    if (param->modelOrder == 255)
    {
        RTPLOG_ERROR1(CnPack, "wrong modelOrder");
        RTPLOG_LEAVE(CnPack);  
        return RV_ERROR_BADPARAM;
    }
    p->sByte = RvRtpGetHeaderLength();
    cnStart = ((RvUint8*)buf) + p->sByte;
    p->payload = RV_RTP_PAYLOAD_CN;
    *cnStart = (RvUint8)(param->level & 0x7F);
    cnStart++;
    if (param->modelOrder>0)
    {
        memcpy(cnStart, param->rc, param->modelOrder);
    }
    p->sByte += (param->modelOrder+1);
    RTPLOG_LEAVE(CnPack);
    return RV_OK;
}

/********************************************************************************
 * RvRtpCnUnpack reads buffer and fills RvRtpConfNoisePayload payload.
 *               This function must be called after RvRtpRead() or RvRtpReadEx()
 *               functions.
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpConfNoisePayload
 * result : return RV_OK if there are no errors
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpCnUnpack(
	IN   void *			  buf,
	IN   RvInt32		  len,
	IN   RvRtpParam *	  p,
	OUT  RvRtpComfNoisePayload*   param)
{
    RvUint8* cnStart;  
    RvInt32  modelOrder;
    RTPLOG_ENTER(CnUnpack);
    if (param == NULL|| p == NULL)
    {
        RTPLOG_ERROR1(CnUnpack, "NULL pointer");
        RTPLOG_LEAVE(CnUnpack);  
        return RV_ERROR_NULLPTR;
    }
    if (p->payload != RV_RTP_PAYLOAD_CN)
    {
        RTPLOG_ERROR1(CnUnpack, "bad payload type value");
        RTPLOG_LEAVE(CnUnpack);  
        return RV_ERROR_BADPARAM;
    }
    modelOrder = (len - 1 - p->sByte);
    if (modelOrder>254 || modelOrder<0)
    {
        RTPLOG_ERROR1(CnUnpack, "bad parameter");
        RTPLOG_LEAVE(CnUnpack);  
        return RV_ERROR_BADPARAM;
    }
    cnStart = ((RvUint8*)buf) + p->sByte;
    param->level = (RvUint8)(*cnStart & 0x7F);
    cnStart++;
    param->modelOrder = (RvUint8) modelOrder;
    if (modelOrder > 0)
    {
        memcpy(param->rc, cnStart, modelOrder);
    }
    RTPLOG_LEAVE(CnUnpack);
    return RV_OK;
}

/********************************************************************************
* RvRtpJpeg2000Pack serializes RvRtpPayloadJpeg2000 into buffer
* INPUT: buf - buffer to serialize
*        len - length of the buf
*        p   - pointer to RvRtpParam ( rtp header)
* OUT    param -  pointer to RvRtpPayloadJpeg2000
* result : return RV_OK if there are no errors
********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpJpeg2000Pack(
    IN  void *			       buf,
    IN  RvInt32			       len,
    IN  RvRtpParam *	       p,
    IN  void *                 param)
{
    RvRtpPayloadJpeg2000* jpeg2000 = (RvRtpPayloadJpeg2000*)param;
    RvUint32*hPtr;

    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpJpeg2000Pack"));

    p->sByte-=8;
    hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);

    hPtr[0]=bitfieldSet(0,       jpeg2000->tp,         30, 2);
    hPtr[0]=bitfieldSet(hPtr[0], jpeg2000->MHF,        28, 2);
    hPtr[0]=bitfieldSet(hPtr[0], jpeg2000->mh_id,      25, 3);
    hPtr[0]=bitfieldSet(hPtr[0], jpeg2000->T,          24, 1);
    hPtr[0]=bitfieldSet(hPtr[0], jpeg2000->priority,   16, 8);
    hPtr[0]=bitfieldSet(hPtr[0], jpeg2000->tileNumber,  0, 16);
    RV_UNUSED_ARG(len);
    hPtr[1]=bitfieldSet(0,       /*jpeg2000->reserved*/ 0,    24, 8); /* for future use */
    hPtr[1]=bitfieldSet(hPtr[1], jpeg2000->fragmentOffset,  0, 24);

    ConvertToNetwork(hPtr, 0, 2);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpJpeg2000Pack"));
    return RV_OK;
}

/********************************************************************************
* RvRtpJpeg2000Unpack reads buffer and fills RvRtpPayloadJpeg2000 payload.
*               This function must be called after RvRtpRead() or RvRtpReadEx()
*               functions.
* input: buf - buffer to read from
*        len - length of the buf
*        p   - pointer to RvRtpParam ( rtp header)
* OUT    param -  pointer to RvRtpPayloadJpeg2000
* result : return RV_OK if there are no errors
********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpJpeg2000Unpack(
    IN   void *			         buf,
    IN   RvInt32		         len,
    IN   RvRtpParam *	         p,
    OUT  void *                  param)
{
    RvRtpPayloadJpeg2000* jpeg2000=(RvRtpPayloadJpeg2000*)param;
    RvUint32*hPtr=(RvUint32*)((RvUint8*)buf+p->sByte);
    RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpJpeg2000Unpack"));
    RV_UNUSED_ARG(len);

    p->sByte+=8;
    ConvertFromNetwork(hPtr, 0, 2);
    jpeg2000->tp             = (RvUint8) bitfieldGet(hPtr[0], 30, 2);
    jpeg2000->MHF            = (RvUint8) bitfieldGet(hPtr[0], 28, 2);
    jpeg2000->mh_id          = (RvUint8) bitfieldGet(hPtr[0], 25, 3);
    jpeg2000->T              = (RvUint8) bitfieldGet(hPtr[0], 24, 1);
    jpeg2000->priority       = (RvUint8) bitfieldGet(hPtr[0], 16, 8);
    jpeg2000->tileNumber     = (RvUint16)bitfieldGet(hPtr[0], 0,  16);

    jpeg2000->reserved       = (RvUint8) bitfieldGet(hPtr[1], 24, 8); /* for future use. should be ignored by receiver */
    jpeg2000->fragmentOffset =           bitfieldGet(hPtr[1], 0, 24);
    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpJpeg2000Unpack"));
    return RV_OK;
}

/********************************************************************************
* RvRtpJpeg2000GetHeaderLength()
* Description: returns the full size Jpeg2000 Video header 
*              includes RTP header,
* input: none
* output: none
* result : returns the full size Jpeg200 Video header includes RTP header
*          or negative value on error
********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpJpeg2000GetHeaderLength(void)
{  
    RTPLOG_ENTER(Jpeg2000GetHeaderLength);
    RTPLOG_LEAVE(Jpeg2000GetHeaderLength);
    return RvRtpGetHeaderLength() + 8;
}

#ifdef __cplusplus
}
#endif

