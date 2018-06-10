///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：xt_mp_def.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：兴图新科公司mp公用定义
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef XT_MP_DEFINE_
#define XT_MP_DEFINE_

#include <stdint.h>
#include "../rv_adapter/rv_def.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MP_IN
#define MP_INOUT
#define MP_OUT
#define MP_CALLCONV	__stdcall
#define MP_TRUE		1
#define MP_FALSE	0
#define MP_NULL		0

typedef void * mp_handle;
typedef struct mp_h_s_
{
	mp_handle hmp;
} mp_h_s;
typedef mp_h_s * mp_h;

typedef struct mssrc_h_s_
{
	mp_handle hmssrc;
} mssrc_h_s;
typedef mssrc_h_s * mssrc_h;

typedef struct msink_h_s_
{
	mp_handle hmsink;
} msink_h_s;
typedef msink_h_s * msink_h;

typedef mp_handle mp_context;
typedef mp_handle msink_block;
typedef int32_t mp_bool;

//读取rtcp SR报告
typedef struct rtcp_send_report_
{
	uint32_t	ssrc;			/* 报告生成方SSRC */
	uint32_t    mNTPtimestamp;	/* Most significant 32bit of NTP timestamp */
    uint32_t    lNTPtimestamp;	/* Least significant 32bit of NTP timestamp */
    uint32_t    timestamp;		/* RTP timestamp */
    uint32_t    packets;		/* Total number of RTP data packets transmitted by the sender
								   since transmission started and up until the time this SR packet
								   was generated. */
    uint32_t    octets; /* The total number of payload octets (not including header or padding */
} rtcp_send_report;
//读取rtcp RR报告
typedef struct rtcp_receive_report_
{
	uint32_t	ssrc;			/* 报告生成方SSRC */
	uint32_t    fractionLost;	/* The fraction of RTP data packets from source specified by
                                   SSRC that were lost since previous SR/RR packet was sent. */
    uint32_t    cumulativeLost; /* Total number of RTP data packets from source specified by
                                   SSRC that have been lost since the beginning of reception. */
    uint32_t    sequenceNumber; /* Sequence number that was received from the source specified
                                   by SSRC. */
    uint32_t    jitter;			/* Estimate of the statistical variance of the RTP data packet inter
								   arrival time. */
    uint32_t    lSR;			/* The middle 32 bits of the NTP timestamp received. */
    uint32_t    dlSR;			/* Delay since the last SR. */
} rtcp_receive_report;

typedef struct _XTSSendReport
{
	long nSSRC;            /* 报告生成方SSRC */
    long mNTPtimestamp;    /* Most significant 32bit of NTP timestamp */
    long lNTPtimestamp;    /* Least significant 32bit of NTP timestamp */
    long nRtpTimestamp;    /* RTP timestamp */
    long nPackets;         /* Total number of RTP data packets transmitted by the sender
                               since transmission started and up until the time this SR packet
                               was generated. */
    long nOctets;			/* The total number of payload octets (not including header or padding */
} XTSSendReport;

typedef struct _XTSR
{
	int nDeviceType;
	int nLinkRet;
	int nFrameType;
	XTSSendReport sr;
} XTSR;

#ifdef __cplusplus
}
#endif

#endif
