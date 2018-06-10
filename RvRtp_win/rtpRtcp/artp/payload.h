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

#ifndef __PAYLOAD_H
#define __PAYLOAD_H


#include "rverror.h"
#include "rvrtpconfig.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "rvrtpinterface.h"

typedef enum
{
/* Audio payloads */
    RV_RTP_PAYLOAD_PCMU  =  0,
    RV_RTP_PAYLOAD_GSM =  3,        /* RFC 3551: Audio Clock Rate:8,000 Hz       1 channel */
    RV_RTP_PAYLOAD_G7231 =  4,
    RV_RTP_PAYLOAD_DVI4_8K =  5,    /* RFC 3551: Audio Clock Rate:8,000 Hz       1 channel */
    RV_RTP_PAYLOAD_DVI4_16K =  6,   /* RFC 3551: Audio Clock Rate:16,000 Hz      1 channel */
    RV_RTP_PAYLOAD_PCMA  =  8,
    RV_RTP_PAYLOAD_G722  =  9,
    RV_RTP_PAYLOAD_L16_2  =  10,    /* RFC 3551: Audio Clock Rate:44,100 Hz      2 channels */
    RV_RTP_PAYLOAD_L16_1  =  11,    /* RFC 3551: Audio Clock Rate:44,100 Hz      1 channel */
    RV_RTP_PAYLOAD_QCELP  =  12,	/* RFC 3551: Audio Clock Rate:8,000 Hz       1 channel */
    RV_RTP_PAYLOAD_CN  =  13,		/* RFC 3551: Confort noise Audio Clock Rate:8,000 Hz       1 channel */
    RV_RTP_PAYLOAD_MPA  =  14,		/* RFC 3551: Audio Clock Rate:90,000 Hz      (RFC) */
	RV_RTP_PAYLOAD_G728  = 15,
	RV_RTP_PAYLOAD_DVI4_11K  = 16,	/* RFC 3551: Audio Clock Rate:11,025 Hz      1 channel */
	RV_RTP_PAYLOAD_DVI4_22K  = 17,	/* RFC 3551: Audio Clock Rate:22,050 Hz      1 channel */
    RV_RTP_PAYLOAD_G729  = 18,
/* Video payloads */
    RV_RTP_PAYLOAD_CELB  = 25,      /* TBD RFC 3551: Video Clock Rate:90,000 Hz      */
    RV_RTP_PAYLOAD_JPEG  = 26,      /* TBD RFC 3551: Video Clock Rate:90,000 Hz      */
    RV_RTP_PAYLOAD_NV  = 28,        /* TBD RFC 3551: Video Clock Rate:90,000 Hz      */
    RV_RTP_PAYLOAD_H261  = 31,
    RV_RTP_PAYLOAD_MPV  = 32,       /* TBD RFC 3551: Video Clock Rate:90,000 Hz      */
    RV_RTP_PAYLOAD_MP2T  = 33,      /* TBD RFC 3551: A/V  Clock Rate:90,000 Hz      */
    RV_RTP_PAYLOAD_H263  = 34		/* TBD RFC 3551: A/V  Clock Rate:90,000 Hz      */
/* AMR uses dynamic payload type */
/* DTMF uses dynamic payload type */
/* MPEG-4 uses dynamic payload type */
} RvRtpPayloadType;

/************************************************************************************
 *	RvRtpGetStandardPayloadClockRate returns standard payload Clock rate
 *  input PayloadType - payload type
 *  returns clock rate if payload type is from standard payload types (RvRtpPayloadType)
 *                     otherwise returns 0
 ************************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpGetStandardPayloadClockRate(
	IN RvInt32 PayloadType);

/************************************************************************************
 *	RvRtpGetStandardPayloadName returns standard payload types name for enumerations
 *  input PayloadType - payload type
 *  returns payload type name string
 ************************************************************************************/

RVAPI
const char* RVCALLCONV RvRtpGetStandardPayloadName(RvInt32 PayloadType);
/* == G.711 U-Law == */

/********************************************************************************
 * RvRtpPCMUPack  sets standard payload type of g711 PCMU to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to PCMU (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpPCMUPack(
        IN  void *      buf,
        IN  RvInt32     len,
        INOUT  RvRtpParam *  p,
        IN  void     *  param);
/********************************************************************************
 * RvRtpPCMUUnpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpPCMUUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);
/********************************************************************************
 * RvRtpPCMUGetHeaderLength
 * description: returns the size of RTP header. (no g.711 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no g.711 payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpPCMUGetHeaderLength(void);


/* == G.711 A-Law == */
/********************************************************************************
 * RvRtpPCMAPack  sets standard payload type of g711 PCMA to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to PCMA (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpPCMAPack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);
/********************************************************************************
 * RvRtpPCMAUnpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpPCMAUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);
/********************************************************************************
 * RvRtpPCMAGetHeaderLength
 * description: returns the size of RTP header. (no g.711 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no g.711 payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpPCMAGetHeaderLength(void);


/* == G.722 == */
/********************************************************************************
 * RvRtpG722Pack  sets standard payload type of g722 to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to g722 (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG722Pack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);

/********************************************************************************
 * RvRtpG722Unpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG722Unpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);
/********************************************************************************
 * RvRtpG722GetHeaderLength
 * description: returns the size of RTP header. (no g.722 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no g.722 payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG722GetHeaderLength(void);

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
    IN    void*param);

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
    OUT void*param);

/********************************************************************************
 * RvRtpG7221GetHeaderLength
 * description: returns the size of RTP header. (no g.722.1 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG7221GetHeaderLength(void);

/* == G.728 == */
/********************************************************************************
 * RvRtpG728Pack  sets standard payload type of g728 to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to g728 (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG728Pack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);

/********************************************************************************
 * RvRtpG728Unpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG728Unpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);

RVAPI
RvInt32 RVCALLCONV RvRtpG728GetHeaderLength(void);

/* == G.729 == */
/********************************************************************************
 * RvRtpG729Pack  sets standard payload type of g729 to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to g729 (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG729Pack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);

/********************************************************************************
 * RvRtpG729Unpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG729Unpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);
/********************************************************************************
 * RvRtpG729GetHeaderLength
 * description: returns the size of RTP header. (no g.729 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no g.729 payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG729GetHeaderLength(void);


/* == G.723.1 == */
/********************************************************************************
 * RvRtpG7231Pack  sets standard payload type of g723.1 to RTP header
 * INPUT: buf - buffer to serialize (unused)
 *        len - length of the buf   (unused)
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to g723.1 (unused)
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG7231Pack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);
/********************************************************************************
 * RvRtpG7231Unpack dummy function - no payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to payload header
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpG7231Unpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);
/********************************************************************************
 * RvRtpG7231GetHeaderLength
 * description: returns the size of RTP header. (no g.723.1 payload header)
 * input: none
 * output: none
 * result : the size of RTP header. (no g.723.1 payload header)
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpG7231GetHeaderLength(void);


/* == H.261 == */
/* RFC 2032
 *
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    .                                                               .
    .                          RTP header                           .
    .                                                               .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          H.261  header                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          H.261 stream ...                     .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct
{
    RvInt32 sBit;   /* Start bit position (SBIT): 3 bits
                       Number of most significant bits that should be ignored
                       in the first data octet. */
    RvInt32 eBit;   /* End bit position (EBIT): 3 bits
                       Number of least significant bits that should be ignored
                       in the last data octet. */
    RvInt32 i;      /* INTRA-frame encoded data (I): 1 bit
                       Set to 1 if this stream contains only INTRA-frame coded
                       blocks. Set to 0 if this stream may or may not contain
                       INTRA-frame coded blocks. The sense of this bit may not
                       change during the course of the RTP session. */
    RvInt32 v;      /* Motion Vector flag (V): 1 bit
                       Set to 0 if motion vectors are not used in this stream.
                       Set to 1 if motion vectors may or may not be used in
                       this stream. The sense of this bit may not change during
                       the course of the session. */
    RvInt32 gobN;   /* GOB number (GOBN): 4 bits
                       Encodes the GOB number in effect at the start of the
                       packet. Set to 0 if the packet begins with a GOB header. */
    RvInt32 mbaP;   /* Macroblock address predictor (MBAP): 5 bits
                       Encodes the macroblock address predictor (i.e. the last
                       MBA encoded in the previous packet). This predictor ranges
                       from 0-32 (to predict the valid MBAs 1-33), but because
                       the bit stream cannot be fragmented between a GOB header
                       and MB 1, the predictor at the start of the packet can
                       never be 0. Therefore, the range is 1-32, which is biased
                       by -1 to fit in 5 bits. For example, if MBAP is 0, the
                       value of the MBA predictor is 1. Set to 0 if the packet
                       begins with a GOB header. */
    RvInt32 quant;  /* Quantizer (QUANT): 5 bits
                       Quantizer value (MQUANT or GQUANT) in effect prior to the
                       start of this packet. Set to 0 if the packet begins with
                       a GOB header. */
    RvInt32 hMvd;   /* Horizontal motion vector data (HMVD): 5 bits
                       Reference horizontal motion vector data (MVD). Set to 0
                       if V flag is 0 or if the packet begins with a GOB header,
                       or when the MTYPE of the last MB encoded in the previous
                       packet was not MC. HMVD is encoded as a 2's complement
                       number, and `10000' corresponding to the value -16 is
                       forbidden (motion vector fields range from +/-15). */
    RvInt32 vMvd;   /* Vertical motion vector data (VMVD): 5 bits
                       Reference vertical motion vector data (MVD). Set to 0 if
                       V flag is 0 or if the packet begins with a GOB header, or
                       when the MTYPE of the last MB encoded in the previous
                       packet was not MC. VMVD is encoded as a 2's complement
                       number, and `10000' corresponding to the value -16 is
                       forbidden (motion vector fields range from +/-15). */
} RvRtpPayloadH261;
/********************************************************************************
 * RvRtpH261Pack serializes H261 payload header into buffer and sets standard
 *               payload type of H.261 to RTP header
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH261
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH261Pack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);
/********************************************************************************
 * RvRtpH261Unpack reads buffer and fills H261 payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH261
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH261Unpack(
        OUT  void*buf,
        IN   RvInt32     len,
        OUT  RvRtpParam*p,
        OUT  void *      param);
/********************************************************************************
 * RvRtpH261GetHeaderLength
 * description: returns the size of H261 payload header (RTP header is included)
 * input: none
 * output: none
 * result : the size of H261 payload header
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH261GetHeaderLength(void);
/* == H.263 ==
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 RTP header                                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 H.263 payload header                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 H.263 bitstream                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
/**********************************************************************
 * H.263 Microsoft version
 **********************************************************************
 This is an RTP helper class which handles the Microsoft version of the H.263 payload type.

	This is header structure for the Microsoft version of the H.263 header. These
diagrams use standard IETF RFC notation for bit layout,the bit address in the code will
appear reversed compared to this layout.

 F=0,P=0

	0					1					2					3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|P|SBIT |EBIT | SRC |		   |I|A|S|						   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 F=0,P=1

	0					1					2					3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|P|SBIT |EBIT | SRC |		   |I|A|S|DBQ| TRB |	  TR	   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


 F=1,P=0

	0					1					2					3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|P|SBIT |EBIT | SRC |	QUANT  |I|A|S|	GOBN   |	  MBA	   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |	 HMV1	   |	 VMV1	   |	 HMV2	   |	 VMV2	   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 F=1,P=1

	0					1					2					3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|P|SBIT |EBIT | SRC |	QUANT  |I|A|S|	GOBN   |	  MBA	   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |	 HMV1	   |	 VMV1	   |	 HMV2	   |	 VMV2	   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |									 |DBQ| TRB |	  TR	   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct
{
    RvInt32 f;    /* F: 1 bit - mode bit */
    RvInt32 p;    /* P: 1 bit - optional PB-frames mode bit */
    RvInt32 sBit; /* SBIT: 3 bits - the start bit */
    RvInt32 eBit; /* EBIT: 3 bits - the end bit */
    RvInt32 src;  /* SRC:  3 bits - the source format */
    RvInt32 i;    /* I: 1 bit - the picture encoding type bit */
    RvInt32 a;    /* A: 1 bit - advanced prediction option */
    RvInt32 s;    /* S: 1 bit - syntax-based arithmetic coding option */
    RvInt32 dbq;  /* DBQ: 2 bits - the B frame differential quantization parameter */
    RvInt32 trb;  /* TRB: 3 bits - the B frame temporal reference */
    RvInt32 tr;   /* TR: 8 bits -  Temporal Reference for the P frame */
    RvInt32 gobN; /* GOBN: 5 bits - the GOB number effective at the start of the packet */
    RvInt32 mbaP; /* MBA: 8 bits - the address within the GOB of the first MB in the packet */
    RvInt32 quant;/* QUANT: 5 bits - the quantization for the first MB coded at the starting of the packet */
    RvInt32 hMv1; /* HMV1: 8 bits - the horizontal motion vector 1 */
    RvInt32 vMv1; /* VMV1: 8 bits - the vertical motion vector 1   */
    RvInt32 hMv2; /* HMV2: 8 bits - the horizontal motion vector 2 */
    RvInt32 vMv2; /* VMV2: 8 bits - the vertical motion vector 2   */

} RvRtpPayloadH263;
/********************************************************************************
 * RvRtpH263Pack serializes H263 payload header into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH263
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH263Pack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);

/********************************************************************************
 * RvRtpH263Unpack reads buffer and fills H263 payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH263
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH263Unpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);

/********************************************************************************
 * RvRtpH263GetHeaderLength
 * description: returns the size of H263 payload header (RTP header is included)
 * input: none
 * output: none
 * result : the size of H263 payload header
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH263GetHeaderLength(void);

/* == H.263a == RFC 2190 version
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 RTP header                                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 H.263 payload header                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 H.263 bitstream                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct
{
    RvInt32 f;    /* F: 1 bit -  The flag bit indicates the mode of the payload header. F=0, mode A;
                     F=1, mode B or mode C depending on P bit defined below. */
    RvInt32 p;    /* Optional PB-frames mode as defined by the H.263 [4]. "0" implies
                     normal I or P frame, "1" PB-frames. When F=1, P also indicates modes:
                     mode B if P=0, mode C if P=1. */
    RvInt32 sBit; /* SBIT: 3 bits
                     Start bit position specifies number of most significant bits that
                     shall be ignored in the first data byte. */
    RvInt32 eBit; /* EBIT: 3 bits
                     End bit position specifies number of least significant bits that
                     shall be ignored in the last data byte. */
    RvInt32 src;  /* SRC : 3 bits
                     Source format, bit 6,7 and 8 in PTYPE defined by H.263 [4], specifies
                     the resolution of the current picture. */
    RvInt32 i;    /* I:  1 bit.
                     Picture coding type, bit 9 in PTYPE defined by H.263[4], "0" is
                     intra-coded, "1" is inter-coded. */
    RvInt32 u;    /* U: 1 bit
                     Set to 1 if the Unrestricted Motion Vector option, bit 10 in PTYPE
                     defined by H.263 [4] was set to 1 in the current picture header,
                     otherwise 0. */
    RvInt32 a;    /* A: 1 bit
                     Set to 1 if the Advanced Prediction option, bit 12 in PTYPE defined
                     by H.263 [4] was set to 1 for current picutre header, otherwise 0. */
    RvInt32 s;    /* S: 1 bit
                     Set to 1 if the Syntax-based Arithmetic Coding option, bit 11 in
                     PTYPE defined by the H.263 [4] was set to 1 for current picture
                     header, otherwise 0. */
    RvInt32 dbq;  /* DBQ: 2 bits
                     Differential quantization parameter used to calculate quantizer for
                     the B frame based on quantizer for the P frame, when PB-frames option
                     is used. The value should be the same as DBQUANT defined by H.263
                     [4].  Set to zero if PB-frames option is not used. */
    RvInt32 trb;  /* TRB: 3 bits
                     Temporal Reference for the B frame as defined by H.263 [4]. Set to
                     zero if PB-frames option is not used. */
    RvInt32 tr;   /* TR: 8 bits
                     Temporal Reference for the P frame as defined by H.263 [4]. Set to
                     zero if the PB-frames option is not used. */
	RvInt32 gobN; /* GOBN: 5 bits
                     GOB number in effect at the start of the packet. GOB number is
                     specified differently for different resolutions. See H.263 [4] for
                     details. */
	RvInt32 mbaP; /* MBA: 9 bits
                     The address within the GOB of the first MB in the packet, counting
                     from zero in scan order. For example, the third MB in any GOB is
                     given MBA = 2. */
    RvInt32 quant; /* QUANT: 5 bits
                     Quantization value for the first MB coded at the starting of the
                     packet.  Set to 0 if the packet begins with a GOB header. This is the
                     equivalent of GQUANT defined by the H.263 [4]. */
	RvInt32 hMv1;
	RvInt32 vMv1;  /* HMV1, VMV1: 7 bits each.
                     Horizontal and vertical motion vector predictors for the first MB in
                     this packet [4]. When four motion vectors are used for current MB
                     with advanced prediction option, these would be the motion vector
                     predictors for block number 1 in the MB. Each 7 bits field encodes a
                     motion vector predictor in half pixel resolution as a 2's complement
                     number. */
    RvInt32 hMv2;
    RvInt32 vMv2;  /* HMV2, VMV2: 7 bits each.
                     Horizontal and vertical motion vector predictors for block number 3
                     in the first MB in this packet when four motion vectors are used with
                     the advanced prediction option. This is needed because block number 3
                     in the MB needs different motion vector predictors from other blocks
                     in the MB. These two fields are not used when the MB only has one
                     motion vector. See the H.263 [4] for block organization in a
                     macroblock.  Each 7 bits field encodes a motion vector predictor in
                     half pixel resolution as a 2's complement number. */
} RvRtpPayloadH263a;

/********************************************************************************
 * RvRtpH263aPack serializes H263a payload header into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH263a
 * result : return RV_OK if there are no errors
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpH263aPack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);

/********************************************************************************
 * RvRtpH263aUnpack reads buffer and fills H263a payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH263a
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH263aUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);

/********************************************************************************
 * RvRtpH263aGetHeaderLength
 * description: returns the size of H263a payload header (RTP header is included)
 * input: none
 * output: none
 * result : the size of H263a payload header
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH263aGetHeaderLength(void);



/* ================================================== */
/* ==== DTMF inBand (via RvRtp payload) - RFC 2833 ==== */
/* ================================================== */

  /*
  There are two types/options of transmitting DTMF in RvRtp payload:

   1) identify the frequency component of the voice-band signals, and
      than transmit them 'as is' to their destination.
   2) identify the tones and translate them into an event-name. the
      event-name is a logical description of DTMF tones, fax-related tones etc.
      the receiver, on this case, produces the signals appropriate to
      the event name that was received.

  each of these options has a different method for transmitting the information inside
  an RvRtp payload. since they are not both mandatory, an endpoint may indicate
  support for receiving these tones/events by including the following flags in the
  terminal capability set (TCS):

  (*) 'receiveRvRtpAudioTelephoneEventCapability' - for event-name method.
  (*) 'receiveRvRtpAudioToneCapability' - for tones-frequency method.

  at this phase, as noted in the MRD (and as recommended in RFC2833) the mandatory
  telephone events are the following DTMF's:

                0,1,2,3,4,5,6,7,8,9,#,*,A,B,C,D.

  the events will be identified by an enumeration.
  */

/* DTMF event name. according to RFC2833
   - the order is very important! do not change the order! */
typedef enum
{
    RVRTP_DTMF_EVENT_0,
    RVRTP_DTMF_EVENT_1,
    RVRTP_DTMF_EVENT_2,
    RVRTP_DTMF_EVENT_3,
    RVRTP_DTMF_EVENT_4,
    RVRTP_DTMF_EVENT_5,
    RVRTP_DTMF_EVENT_6,
    RVRTP_DTMF_EVENT_7,
    RVRTP_DTMF_EVENT_8,
    RVRTP_DTMF_EVENT_9,
    RVRTP_DTMF_EVENT_ASTERISK,
    RVRTP_DTMF_EVENT_POUND,
    RVRTP_DTMF_EVENT_A,
    RVRTP_DTMF_EVENT_B,
    RVRTP_DTMF_EVENT_C,
    RVRTP_DTMF_EVENT_D,
    RVRTP_DTMF_EVENT_FLASH

} RvRtpDtmfEvent;


/************************************************************************
 * RvRtpDtmfEventParams
 * This struct holds all the information needed/received for RFC2833
 * packet with event mode
 ***********************************************************************/
typedef struct
{
    RvRtpDtmfEvent event;         /* type of current event received or to-send */
    RvBool       end;           /* the E (end bit) - indicating the last packet of this event */
    RvUint8      volume;        /* volume of current event received or to-send */
    RvUint32     duration;      /* duration of current event received or to-send */

} RvRtpDtmfEventParams;



/************************************************************************
 * RvRtpDtmfEventPack
 *
 * purpose: set the payload format, for sending DTMF events in band,
 *          as described in RFC2833, section 3.5.
 *
 * input  : buf - buffer pointer that will be sent
 *          len - length of the buffer.
 *          p - RvRtp header default parameters.
 *          param - a structure containing the required parameters for DTMF events.
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDtmfEventPack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);


/************************************************************************
 * RvRtpDtmfEventUnpack
 *
 * purpose: evaluates the DTMF events from the received packed.
 *
 * input  : len - length of the buffer.
 * output : buf - the received buffer.
 *          p - RvRtp header parameters that were received.
 *          param - the received parameters for DTMF events.
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDtmfEventUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);


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
RvInt32 RVCALLCONV RvRtpDtmfEventGetHeaderLength(void);


/* == Telephony Tones according to RFC2833 == */

/************************************************************************
 * RvRtpDtmfTonesParams
 * This struct holds all the information needed/received for RFC2833
 * packet with tone mode
 ***********************************************************************/
typedef struct
{
    RvUint16     modulation;        /* The modulation frequency, in Hz */
    RvBool       T;                 /* the 'T' bit. True means divide by 3 */
    RvUint8      volume;            /* volume of current tone received or to-send */
    RvUint32     duration;          /* duration of current tone received or to-send */
    RvUint16     *freqList;         /* the list of frequencies to send/received */
    RvUint32     freqListLength;    /* length of the frequencies list */

} RvRtpDtmfTonesParams;


/************************************************************************
 * RvRtpDtmfTonesPack
 *
 * purpose: set the payload format, for sending telephony tones inband,
 *          as described in RFC2833, section 4.4.
 *
 * input  : buf - buffer pointer that will be sent
 *          len - length of the buffer.
 *          p - RvRtp header default parameters.
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
RvStatus RVCALLCONV RvRtpDtmfTonesPack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  param);


/************************************************************************
 * RvRtpDtmfTonesUnpack
 *
 * purpose: evaluates the telephony tones from the received packed.
 *
 * input  : len - length of the buffer.
 * output : buf - the received buffer.
 *          p - RvRtp header parameters that were received.
 *          param - the received parameters for telephony tones.
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpDtmfTonesUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      param);


/************************************************************************
 * RvRtpDtmfTonesGetByIndex
 *
 * purpose: find the requested frequency in the received message.
 *
 * input  : buf - the received buffer.
 *          index - index of the frequency inside the frequency list.
 *          p - RvRtp header parameters that were received.
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
        OUT  RvUint16 *  frequency);



/* ========================================================= */
/* ==== Annex Q & H.281 - Far End Camera Control (FECC) ==== */
/* ========================================================= */
/*
    The FECC protocol is designed to function along with H.224, in order to
    allow a multi-point video conferencing to be individually controlled from
    any video terminal.
*/

typedef enum
{
    RVRTP_ANNEXQ_PROCEDURES_START_ACTION = 1,
    RVRTP_ANNEXQ_PROCEDURES_CONTINUE_ACTION = 2,
    RVRTP_ANNEXQ_PROCEDURES_STOP_ACTION = 3,
    RVRTP_ANNEXQ_PROCEDURES_SELECT_VIDEO_SOURCE = 4,
    RVRTP_ANNEXQ_PROCEDURES_VIDEO_SOURCE_SWITCHED = 5,
    RVRTP_ANNEXQ_PROCEDURES_STORE_AS_PRESET = 6,
    RVRTP_ANNEXQ_PROCEDURES_ACTIVATE_PRESET = 7

} RvRtpAnnexQProcedures;


typedef enum
{
    RVRTP_ANNEXQ_MOVE_DISABLE = 0,  /* = 00(binary) */
    RVRTP_ANNEXQ_MOVE_LEFT_DOWN_OUT = 2, /* = 10(binary) */
    RVRTP_ANNEXQ_MOVE_RIGHT_UP_IN = 3  /* = 11(binary) */

} RvRtpAnnexQMoveCamera;


/************************************************************************
 * RvRtpAnnexQActions
 * This struct holds the various possible positions for a camera. the user
 * can request to move the camera in several positions concurrently.
 ***********************************************************************/
typedef struct
{
    RvRtpAnnexQMoveCamera pan;
    RvRtpAnnexQMoveCamera tilt;
    RvRtpAnnexQMoveCamera zoom;
    RvRtpAnnexQMoveCamera focus;

} RvRtpAnnexQActions;


/************************************************************************
 * AnnexQCameraMode
 * This struct holds the user request for camera method of imaging.
 ***********************************************************************/
typedef struct
{
    RvBool stillImage;
    RvBool doubleResolutionStillImage;

} AnnexQCameraMode;


/************************************************************************
 * RvRtpAnnexQParam
 * This struct holds all the information needed/received for H.281
 * packet with FECC capability request.
 ***********************************************************************/
typedef struct
{
    RvRtpAnnexQProcedures procedure;
    RvRtpAnnexQActions    action;
    RvUint8             timeOut;
    RvUint8             videoSource;
    AnnexQCameraMode    mode;
    RvUint8             preset;

} RvRtpAnnexQParam;


/************************************************************************
 * RvRtpAnnexQMessagePack
 *
 * purpose: set the payload format, for sending Far end camera commands inband,
 *          as described in H.281.
 *          Several AnnexQ commands can be sent in the same packet. Using
 *          RvRtpAnnexQMessagePack on the same buffer for several such messages
 *          places them in reverse order.
 *
 * input  : buf - buffer pointer that will be sent
 *          len - length of the buffer.
 *          p - RvRtp header default parameters.
 *          param - a structure containing the required parameters for FECC.
 * output : none
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpAnnexQMessagePack(
        IN  void *      buf,
        IN  RvInt32     len,
        IN  RvRtpParam *  p,
        IN  void     *  params);


/************************************************************************
 * RvRtpAnnexQMessageUnpack
 *
 * purpose: evaluates the FECC commands from the received packed.
 *
 * input  : len - length of the buffer.
 * output : buf - the received buffer.
 *          p - RvRtp header parameters that were received.
 *          param - the received parameters for FECC.
 * return : Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpAnnexQMessageUnpack(
        OUT  void *      buf,
        IN   RvInt32     len,
        OUT  RvRtpParam *  p,
        OUT  void *      params);

/**********************************************************************
* AMR and AMR-WB RFC 3267
*********************************************************************/

typedef struct RvRtpPayloadAMR_
{
	RvBool	bemode;   /* RvTrue = Bandwidth Efficient Mode, RvFalse = Octet aligned Mode */
	RvBool	interopt; /* RvTrue = Interleaved option (in Octet Aligned Mode only) */
	RvBool	crcopt;   /* RvTrue = CRC option (in Octet Aligned Mode only) */
	RvUint8	cmr;      /* 4 bit CMR (values: 0-7 AMR, 0-8 AMR-WB, 14 = SPEECH_LOST (AMR-WB only), 15 = NO_DATA */
	RvUint8	ill;      /* 4 bit ILL value (used only with Interleaved option) */
	RvUint8	ilp;      /* 4 bit ILL value (used only with Interleaved option) */
	RvUint8 nbits;    /* Number of bits data is offset by (Bandwidth Efficient Mode Only) */
	RvUint8 xval;     /* Value of the extra bits (nbits bits) of data that end in the last header byte */
	RvUint32 tocsize; /* Number of frames listed in table of contents */
	RvUint8 *ftypes;  /* Array (of size tocsize) of frame types for each frame listed in the table of contents */
	RvUint8 *fqual;   /* Array (of size tocsize) of frame quality indicators for each frame listed in the table of contents */
	RvUint32 framecnt;/* Number of data frames (frames listed in table of contents that are not of type 14 or 15) */
	RvUint8 *crcs;    /* Array of crcs, one for each data frame (used only with CRC option) */
	RvUint32 totalsize; /* Total size of entire AMR header including TOC and CRCs */

} RvRtpPayloadAMR;

#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
RVAPI
RvInt32  RVCALLCONV RvRtpPayloadAMRToString(const RvRtpPayloadAMR* thisPtr,
								 char *buffer,
								 size_t bufferSize,
								 const char *prefix);
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
											  OUT RvUint32 *framecount);
/************************************************************************
 * RvRtpPayloadAMRSetArrays
 * description: This method sets arrays that the table of contents information should
 * be stored in by the RvRtpPayloadAMRUnpack function. The size of
 * the ftypes and fqual arrays should be at least as big as the size of
 * the table of contents calculated by the RvRtpPayloadAMRCalcArraySizes
 * function. The size of the crcs array should be at least as big as the
 * number of frames calculated by the RvRtpPayloadAMRCalcArraySizes function.
 * If the CRC option is not being used, crcs is not used and can be set to NULL.
 * Paremeters:
 *      input - output  thisPtr - The RvRtpPayloadAMR object.
 *				input   ftypes  - Array to store frame types.
 *				input   fqual   - Array to store frame quality indicators.
 *				input   crcs    - Array to store CRCs.
 * See also: RvRtpPayloadAMRCalcArraySizes
 ************************************************************************/
RVAPI
void RVCALLCONV RvRtpPayloadAMRSetArrays(
	INOUT RvRtpPayloadAMR* thisPtr,
	IN    RvUint8 *ftypes,
	IN    RvUint8 *fqual,
	IN    RvUint8 *crcs);
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
void RVCALLCONV RvRtpPayloadAMRSetTOC(
	INOUT RvRtpPayloadAMR* thisPtr,
	IN    RvUint32 tocsize,
	IN    RvUint8 *frametypes,
	IN    RvUint8 *qualitybits);

/************************************************************************
 *  RvRtpAMRPack
 * description: This method serializes the payload header into a data buffer, and configures the
 * RTP Header's payload type. Before calling this function, the Table of Contents
 * must be set using the RvRtpPayloadAMRSetTOC function. If CRCs are being used,
 * they should also be set before calling this function.
 * Creating an AMR payload is a little more complicated than most. Here are the
 * steps required to properly create and send an AMR payload:
 * bullet list:
 *   - Construct a RvRtpPayloadAMR object using RvRtpPayloadAMRInit.
 *   - Set the RvRtpPayloadAMR mode (BEMode) and options (InterleavedOption, CRCOption).
 *   - Collect the frame (or multiple frame) data that is to be sent.
 *   - Set the Table of Contents using RvRtpPayloadAMRSetTOC.
 *   - Set the CMR
 *   - Optionally, set the ILL and ILP.
 *   - Optionally, set the CRCs for each frame.
 *   - If using Bandwidth Efficient mode, get the data bit offset.
 *   - If using Bandwidth Efficient mode, shift the frame data to the left by that offset (set extra bits in last byte to 0).
 *   - If using Bandwidth Efficient mode, store the bits shifted out of the first frame data byte.
 *
 *   - Using the header sizes, frame data size, create a data buffer.
 *   - Use RvRtpAMRPack to write the AMR header into the data buffer.
 *   - Set the payload type.
 *   - Set the Marker bit.
 *   - Set the timestamp.
 *   - Send the RTP packet using RvRtpWrite.
 *   - Destroy the data buffer.
 *   - Clean up anything related to the frame data.
 *
 * The above steps should be followed each time an AMR packet is to be
 * sent, except that the RTP header object and the RvRtpPayloadAMR object
 * can be reused, so that it is not necessary to destruct and then construct
 * them again each time a packet is sent as long as RvRtpWrite has
 * completed before these objects are reused.
 * Parameters:
 *      input  param  - pointer to the RvRtpPayloadAMR object.
 *      input  p      - RvRtp header default parameters.
 *      output buf    - The data buffer that the header is to be serialized into.
 *      output len    - size of buf
 * returns: Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpAMRPack(
		IN  void *      buf,
		IN  RvInt32     len,
		IN  RvRtpParam *  p,
		IN  void     *  param);

/************************************************************************
 * RvRtpAMRUnpack
 * description:
 * This method unserializes the payload header from a data buffer.
 * The mode and option settings should be set before calling this
 * function. Also, the storage arrays for the frame types, frame
 * quality indicators, and (optionally) the CRCs MUST be set before
 * making this call.
 * Receiving an AMR payload is a little more complicated than most. Here are the
 * steps required to properly receive and parse an AMR payload:
 * bulletlist:
 *    - Using the header sizes, worst case frame header/data size, allocate data buffer and RTP header.
 *    - Receive the RTP packet using RvRtpRead.
 *    - Init RvRtpPayloadAMR object using RvRtpAMRPayloadInit.
 *    - Set the RvRtpPayloadAMR mode and options (BEMode, InterleavedOption, CRCOption).
 *    - Use the RvRtpPayloadAMRCalcArraySizes to detrmine Table and Contents size and the number of frames in the packet.
 *    - Use the RvRtpPayloadAMRSetArrays to set arrays for storing frame types, frame quality indicators, and (optionally) CRCs.
 *    - Use RvRtpAMRUnpack to read the AMR header from the data buffer.
 *    - The Table of Contents data (and optionally, CRCs) will now be stored in the arrays that were set with RvRtpPayloadAMRSetArrays.
 *    - RvRtpParam must be filled after RvRtpRead with the payload type, Marker bit, timestamp.
 *    - Get the CMR.
 *    - Optionally, get the ILL and ILP.
 *    - If using Bandwidth Efficient mode, get the data bit offset.
 *    - If using Bandwidth Efficient mode, shift the frame data to the right by that offset.
 *    - If using Bandwidth Efficient mode, get the high order bits that are part of the first frame data byte .
 *    - Continue normal processing of the frame data.
 * Parameters:
 *     output param - The RvRtpPayloadAMR object.
 *     input  buf   - The data buffer that the header is to be unserialized from.
 *     input  len   - data buf length
 *     input  p     - RvRtp header parameters that were received.
 * returns: Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpAMRUnpack(
		IN   void *      buf,
		IN   RvInt32     len,
		IN   RvRtpParam *  p,
		OUT  void *      param);

/************************************************************************
 * RvRtpAMRGetHeaderLength
 * description:  This method gets the number of bytes required to
 *               serialize the payload's header (includes RTP header)
 * parameters:
 *	    thisPtr - pointer to the RvvRtpPayloadAMR.
 *
 * returns: The number of bytes required to serialize the payload's header.
 ************************************************************************/

RVAPI
RvInt32 RVCALLCONV RvRtpAMRGetHeaderLength(RvRtpPayloadAMR* thisPtr);
/************************************************************************
 * RvRtpAMRPayloadInit
 * description:  initializes the AMR payload structure
 * parameters:
 *	    thisPtr - pointer to the RvRtpPayloadAMR.
 * returns: Non-negative value on success
 *          Negative value on failure
 ************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpAMRPayloadInit(RvRtpPayloadAMR* thisPtr);


/**********************************************************************
 * MPEG 4 RFC 3640
 *********************************************************************/

typedef enum
{
		RV_RTPSTATUS_Succeeded = 0,			/* The operation completed successfully. */
		RV_RTPSTATUS_Failed,				/* The operation failed */
		RV_RTPSTATUS_NotEnoughRoomInDataBuffer, /* RvDataBuffer not large enough to fit all data */
		RV_RTPSTATUS_NotSupported,          /* Request not supported. */
		RV_RTPSTATUS_WrongParameter         /* wrong parameter. */
} RvRtpMpeg4Status;



typedef enum
{
    RV_RTPPAYLOADMPEG4_MODE_GENERIC  = 0x01,  /* any stream type        */
	RV_RTPPAYLOADMPEG4_MODE_CBR_CELP = 0x02,  /* Constant bit-rate CELP */
	RV_RTPPAYLOADMPEG4_MODE_VBR_CELP = 0x04,  /* Variable bit-rate CELP */
	RV_RTPPAYLOADMPEG4_MODE_LBR_AAC  = 0x08,  /* Low bit-rate AAC       */
	RV_RTPPAYLOADMPEG4_MODE_HBR_AAC  = 0x10	  /* High bit-rate AAC      */
}	RvRtpMpeg4Mode;


typedef enum
{
	RV_RTPPAYLOADMPEG4_MEDIA_TYPE_NOT_SET = 0,		/* INVALID media type */
	RV_RTPPAYLOADMPEG4_MEDIA_TYPE_AUDIO = 1,        /* "audio"  */
	RV_RTPPAYLOADMPEG4_MEDIA_TYPE_VIDEO,			/* "video" */
	RV_RTPPAYLOADMPEG4_MEDIA_TYPE_APPLICATION		/* "application" */

} RvRtpMpeg4MediaType;

typedef struct RvRtpMpeg4MimeFormat_
{
	/* Header section parameters */
	RvBool	AuHeaderSection;			/* AU header section presence (RFC 3640 (3.2.1) page 10)*/
										/* Zero means that the field is not present */
	RvUint8  AuSizeSize;                 /* size bits of AuSize of AU Header */
	RvUint16 ConstantAuSize;             /* constant size Bytes? of all AUs
											if not 0 then AuSize field is not present AU Header */
	                                     /* 0 in AuSizeSize and ConstantAuSize indicates single AU or
                                            fragment of AU each RTP packet */

	RvUint8 AuIndexOrAuIndexDeltaSize;  /* size in bits of AuIndex Or AuIndexDelta of AU Header */
	RvUint8 CTS_deltaSize;				/* size in bits of CTS_delta of AU Header */
	RvUint8 DTS_deltaSize;				/* size in bits of DTS_delta of AU Header */
	RvUint8 StreamStateSize;			/* size in bits of DTS_delta of AU Header */
	/* Auxiliary section parameters */
	RvBool  RAP_flag;					/* TRUE presents */
	RvUint8 AuxiliaryDataSizeSize;	    /* size bits of auxiliary-data-size of Auxiliary Section */
	/* Data section parameters */
	/* MPEG-4 common parameters */
	RvRtpMpeg4MediaType mediaType;		/* 0 -invalid, 1 - audio; 2 - video; 3 - application */

    RvRtpMpeg4Mode	mode;				/* 5 standard modes are defined above */
	RvBool			Interleaving;		/* Interleaving support  */
	RvBool			Fragmentation;		/* Fragmentation support */
	RvBool			Concatination;		/* Concatination support */
	RvUint32		ClockRate;
	RvUint32		ConstantDuration;   /* Interleaving: Access Units have a constant time duration, if:
											  TS(i+1) - TS(i) = constant
										   for any i, where:
										   i indicates the index of the AU the original order, and
								           TS(i) denotes the time stamp of AU(i) */
	RvUint32        MaxDisplacement;
}	RvRtpMpeg4MimeFormat;

/* NOTE:  For right encoding/decoding RvRtpMpeg4Config and RvRtpMpeg4MimeFormat
		  must be the same for sending and receiving sides !!!!
		  This can be obtained by control exchange messages.*/

/************************************************************************
 * RvRtpMpeg4MimeFormatConstruct
 * description:  The RvRtpMpeg4MimeFormatConstruct constructor.
 * parameters:
 * input:	 mode    - MPEG-4 mode
 * output:   thisPtr - pointer to the RvRtpMpeg4MimeFormat.
 * returns: pointer to the RvRtpMpeg4MimeFormat.
 ************************************************************************/
RVAPI
RvRtpMpeg4MimeFormat*  RVCALLCONV RvRtpMpeg4MimeFormatConstruct(
		INOUT RvRtpMpeg4MimeFormat* thisPtr,
		IN RvRtpMpeg4Mode mode);
/************************************************************************
 * RvRtpMpeg4MimeFormatDestruct
 * description:  The RvRtpMpeg4MimeFormatDestruct destructor.
 * parameters:
 * input:   thisPtr - pointer to the RvRtpMpeg4MimeFormat.
 * returns: none.
 ************************************************************************/
#define   RvRtpMpeg4MimeFormatDestruct(thisPtr)
/************************************************************************
 * RvRtpMpeg4MimeFormatCheck
 * description:  checks validity of RvRtpMpeg4MimeFormat
 * parameters:
 * input:   thisPtr - pointer to the RvRtpMpeg4MimeFormat.
 * output:  none
 * returns: RvRtpStatus
 ************************************************************************/
RVAPI
RvRtpMpeg4Status  RVCALLCONV RvRtpMpeg4MimeFormatCheck(
	    RvRtpMpeg4MimeFormat* mimeFormatPtr);

/* common mpeg4 configuration */

  /* Auxiliary section configuration */


/* NOTE:  For right encoding/decoding RvRtpMpeg4Config and RvRtpMpeg4MimeFormat
		  must be the same for sending and receiving sides !!!!
		  This can be obtained by control exchange messages.*/


/*	The Auxiliary Section consists of the auxiliary-data-size field
  followed by the auxiliary-data field.  Receivers MAY (but are not
  required to) parse the auxiliary-data field; to facilitate skipping
  of the auxiliary-data field by receivers, the auxiliary-data-size
  field indicates the length bits of the auxiliary-data.  If the
  concatenation of the auxiliary-data-size and the auxiliary-data
  fields consume a non-integer number of octets, up to 7 zero padding
  bits MUST be inserted immediately after the auxiliary data order
  to achieve octet-alignment.  See figure 4.

  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+
  | auxiliary-data-size   | auxiliary-data       |padding bits |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+

	Figure 4: The fields the Auxiliary Section


  The length bits of the auxiliary-data-size field is configurable
  by a MIME format parameter(see section 4.1).  The default length of
  zero indicates that the entire Auxiliary Section is absent. */


typedef struct RvRtpMpeg4AuxiliarySection_
{
	RvSize_t Size;		/*  specifies the length bits of the immediately
			  		        following auxiliary-data field; */
	RvUint8*  BufferPtr;	/*  the pointer to a buffer containing auxiliary-data field */
    RvSize_t StartBit;  /*  the number of the first bit of the auxiliary-data field in the buffer*/

} RvRtpMpeg4AuxiliarySection;


typedef struct RvRtpMpeg4Header_
{
	RvUint16 AuSize;    	/* Indicates the size octets of the associated Access Unit
							the Access Unit Data Section the same RTP packet.  When
							the AU-size is associated with an AU fragment, the AU size
							indicates the size of the entire AU and not the size of the
							fragment.  this case, the size of the fragment is known
							from the size of the AU data section.  This can be exploited
							to determine whether a packet contains an entire AU or a
							fragment, which is particularly useful after losing a packet
							carrying the last fragment of an AU. */

	RvUint32 AuIndex;		/* Indicates the serial number of the associated Access Unit
							(fragment).  For each (decoding order) consecutive AU or AU
							fragment, the serial number is incremented with 1.  When
							present, the AU-Index field occurs the first AU-header
							the AU Header Section, but MUST NOT!!! occur any subsequent
							(non-first) AU-header that Section.  To encode the serial
							number any such non-first AU-header, the AU-Index-delta
							field is used. */

	RvUint32 AuIndexDelta; /* The AU-Index-delta field is an unsigned integer
							that specifies the serial number of the associated AU as the
							difference with respect to the serial number of the previous
							Access Unit.  Hence, for the n-th (n>1) AU the serial number
							is found from:
							 AU-Index(n) = AU-Index(n-1) + AU-Index-delta(n) + 1
							If the AU-Index field is present the first AU-header
							the AU Header Section, then the AU-Index-delta field MUST be
							present any subsequent (non-first) AU-header.  When the
							AU-Index-delta is coded with the value 0, it indicates that
							the Access Units are consecutive decoding order.  An
							AU-Index-delta value larger than 0 signals that interleaving
							is applied. */

	RvBool CTS_flag;		/* Indicates whether the CTS-delta field is present.
							A value of 1 indicates that the field is present, a value
							of 0 that it is not present.
							The CTS-flag field MUST be present each AU-header if the
							length of the CTS-delta field is signaled to be larger than
							zero.  that case, the CTS-flag field MUST have the value 0
							the first AU-header and MAY have the value 1 all
							non-first AU-headers.  The CTS-flag field SHOULD be 0 for
							any non-first fragment of an Access Unit. */

	RvInt32 CTS_delta;		/* Encodes the CTS by specifying the value of CTS as a 2's
							complement offset (delta) from the time stamp the RTP
							header of this RTP packet.  The CTS MUST use the same clock
							rate as the time stamp the RTP header. */

	RvBool DTS_flag;		/* Indicates whether the DTS-delta field is present.  A value
							of 1 indicates that DTS-delta is present, a value of 0 that
							it is not present.
							The DTS-flag field MUST be present each AU-header if the
							length of the DTS-delta field is signaled to be larger than
							zero.  The DTS-flag field MUST have the same value for all
							fragments of an Access Unit. */

	RvInt32 DTS_delta;		/* Specifies the value of the DTS as a 2's complement
							offset (delta) from the CTS.  The DTS MUST use the
							same clock rate as the time stamp the RTP header.  The
							DTS-delta field MUST have the same value for all fragments of
							an Access Unit. */

	RvBool  RAP_flag;		/* Indicates when set to 1 that the associated Access Unit
							provides a random access point to the content of the stream.
							If an Access Unit is fragmented, the RAP flag, if present,
							MUST be set to 0 for each non-first fragment of the AU.*/

	RvUint32 StreamState;	/* Specifies the state of the stream for an AU of an
							MPEG-4 system stream; each state is identified by a value of
							a modulo counter.  ISO/IEC 14496-1, MPEG-4 system streams
							use the AU_SequenceNumber to signal stream states.  When the
							stream state changes, the value of stream-state MUST be
							incremented by one.  */
                            /* Note: no relation is required between stream-states of different streams.  */

    RvUint8 *Data;           /*The pointer to the AU in buffer*/
    RvSize_t DataSize;      /*The size of the AU in bytes*/

/* The length of the fields */
} RvRtpMpeg4Header;


typedef struct RvRtpMpeg4_
{
    /* EACH SECTION IS BYTE ALIGHNED !!! */
	/* Header section beg*/
	RvUint16 AuHeadersLength; /* 2 bytes field packet if header section present
							   specifies the length bits of the following AU headers,
							   excluding padding bits.
							   OPTIONAL FIELD depend on configuration of MPEG4
							   from capabilities exchange (H.245) or (SDP)           */


    RvRtpMpeg4Header AuHeader[RV_RTPPAYLOADMPEG4_MAX_AU_IN_PACKET];
										/* OPTIONAL FIELDS depends on
										   presentation of AuHeaderLengh */
	/* padding bits for header section must be calculated on the send stage */
	/* Header section end */
    RvRtpMpeg4AuxiliarySection AuxiliarySection;
	/* Au data section */
    RvUint32 PacketLength; 	 /* if packet length  - this value contains the real length of MPEG4 payload Serialized Size */

	RvRtpMpeg4MimeFormat* mimeFormatPtr; /* for serialization/unserialization purposes */
    RvUint32 AUsNumber; /* number of AUs payload */

 	RvBool	 markerBit;      /* RTP header marker bit, use it for RvRtpWrite */
	RvSize_t maxPayloadSize; /* Maximum payload size in bytes */

} RvRtpPayloadMpeg4;
/********************************************************************************
 * RvRtpMpeg4Construct - the RvRtpPayloadMpeg4 constructor
 * input/output - pointer to RvRtpPayloadMpeg4
 * result :  pointer to RvRtpPayloadMpeg4
 ********************************************************************************/
RVAPI
RvRtpPayloadMpeg4* RVCALLCONV RvRtpMpeg4Construct(
		INOUT RvRtpPayloadMpeg4* thisPtr);
/********************************************************************************
 * RvRtpMpeg4Destruct - the RvRtpPayloadMpeg4 destructor
 * input/output - pointer to RvRtpPayloadMpeg4
 * result :  none
 ********************************************************************************/
#define  RvRtpMpeg4Destruct(thisPtr)
/********************************************************************************
 * RvRtpMpeg4Unpack reads buffer and fills MPEG-4 payload
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpeg4
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus  RVCALLCONV RvRtpMpeg4Unpack(
		RvUint8* buf,
		RvSize_t payloadSize,
		RvRtpParam* p,
		RvRtpPayloadMpeg4* param);
/********************************************************************************
 * RvRtpMpeg4Pack serialises MPEG-4 payload into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpeg4
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvStatus  RVCALLCONV RvRtpMpeg4Pack(
		RvUint8* bufferPtr,
		RvSize_t payloadSize,
        RvRtpParam *p,
		RvRtpPayloadMpeg4* thisPtr);
/********************************************************************************
 * RvRtpMpeg4GetAuxiliarySection gets the auxiliary section data.
 * INPUT: thisPtr - pointer to RvRtpPayloadMpeg4
 *        auxiliarySession - pointer to data buffer to be filled with auxiliary section
 *        auxiliarySessionBufferSize - maximal size to be filled
 * OUTPUT: none
 * result : RvRtpStatus
 ********************************************************************************/
RVAPI
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4GetAuxiliarySection(
		IN RvRtpPayloadMpeg4* thisPtr,
		INOUT RvUint8* auxiliarySession,
		IN RvSize_t auxiliarySessionBufferSize);
/********************************************************************************
 * RvRtpMpeg4SetAuxiliarySection sets the auxiliary section data to RvRtpPayloadMpeg4.
 * INPUT: auxiliarySession - pointer to filled auxiliary session data buffer
 *        auxiliarySessionBufferSize - size of auxiliarySession
 * OUTPUT: thisPtr - pointer to RvRtpPayloadMpeg4
 * result : RvRtpStatus
 ********************************************************************************/
RVAPI
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4SetAuxiliarySection(
        INOUT RvRtpPayloadMpeg4* thisPtr,
		IN RvUint8* auxiliarySession,
		IN RvSize_t auxiliarySessionSizeInBits);

/********************************************************************************
 * RvRtpMpeg4AddAccessUnit
 * description - adds an access unit to the payload.
 *               Access unit is represented by the 3 last parameters.
 *               auPtr point to the beginning of entire access unit.
 *               auOffset represents the offset inside the auPtr,
 *               It should be 0 for first use and the function will advance it accordance
 *               to the size of the processed fragment.
 * Parameters:
 * INPUT/OUTPUT: thisPtr - the pointer to the RvRtpPayloadMpeg4 object (AU to be added)
 * INPUT: mpeg4HeaderPtr - the pointer to the header object describing AU.
 *                 auPtr - the pointer to the beginning of entire access unit.
 *              auOffset - the pointer to the offset of the current fragment inside auPtr.
 *                         Must point to 0 before the first call to RvRtpMpeg4AddAccessUnit
 *                         with this AU.
 *                auSize - the size of the entire AU.
 * result : RvRtpStatus
 ********************************************************************************/
RVAPI
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4AddAccessUnit(
		 INOUT RvRtpPayloadMpeg4*	        thisPtr,
		 IN RvRtpMpeg4Header*	mpeg4HeaderPtr,
		 IN RvUint8*		    auPtr,     /* pointer to access unit */
		 IN RvSize_t*			auOffset,
         IN RvSize_t            auSize);    /* size of the access unit*/
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
 * result : RvRtpStatus
 ********************************************************************************/
RVAPI
RvRtpMpeg4Status RVCALLCONV RvRtpMpeg4GetAccessUnit(
	 	 IN RvRtpPayloadMpeg4*			thisPtr,
         OUT RvRtpMpeg4Header*	        auHeader,
         IN RvSize_t					auNumber,
		 INOUT RvUint8*					auPtr,
		 INOUT RvSize_t*			    auOffset,
         INOUT RvSize_t*				auSize);

/********************************************************************************
 * RvRtpMpeg4GetPayloadLength
 * description - Returns the size of the payload header.
 *               For MPEG4 this size includes AUHeaders,
 *               Auxiliary section and one or more AUs.
 * Parameters:
 * INPUT:       thisPtr - the pointer to the RvRtpPayloadMpeg4 object
 * OUTPOT: none
 * result : The size of the payload.
 ********************************************************************************/
RVAPI
RvUint32 RVCALLCONV RvRtpMpeg4GetPayloadLength(
		 const RvRtpPayloadMpeg4* thisPtr);

/* for Debug Purposes only */
#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
RVAPI
RvInt32 RVCALLCONV RvRtpPayloadMpeg4ToString(const RvRtpPayloadMpeg4* thisPtr,
											 char *buffer,
											 size_t bufferSize,
											 const char *prefix);
#endif
/* == H.264 ==  draft-ietf-avt-rtp-h264-04.txt */

typedef enum _RvRtpPayloadH624NALTypes
{
	/*  non-aggregative NAL unit packet 1-23 */
	RVRTP_H264_NALU_TYPE_SLICE = 1,
	RVRTP_H264_NALU_TYPE_DPA   = 2,
	RVRTP_H264_NALU_TYPE_DPB   = 3,
	RVRTP_H264_NALU_TYPE_DPC   = 4,
	RVRTP_H264_NALU_TYPE_IDR   = 5,
	RVRTP_H264_NALU_TYPE_SEI   = 6,
	RVRTP_H264_NALU_TYPE_SPS   = 7,
	RVRTP_H264_NALU_TYPE_PPS   = 8,
	RVRTP_H264_NALU_TYPE_PD    = 9,
	RVRTP_H264_NALU_TYPE_EOSEQ = 10,
	RVRTP_H264_NALU_TYPE_EOSTREAM = 11,
	RVRTP_H264_NALU_TYPE_FILL     = 12,
/*	RVRTP_H264_SNAL_13 = 13,
	RVRTP_H264_SNAL_14 = 14,
	RVRTP_H264_SNAL_15 = 15,
	RVRTP_H264_SNAL_16 = 16,
	RVRTP_H264_SNAL_17 = 17,
	RVRTP_H264_SNAL_18 = 18,
	RVRTP_H264_SNAL_19 = 19,
	RVRTP_H264_SNAL_20 = 20,
	RVRTP_H264_SNAL_21 = 21,
	RVRTP_H264_SNAL_22 = 22,
	RVRTP_H264_SNAL_23 = 23,	*/
	RVRTP_H264_STAP_A = 24,			/* Single-time aggregation packet */
	RVRTP_H264_STAP_B = 25,			/* Single-time aggregation packet */
	RVRTP_H264_MTAP16 = 26,          /* Multi-time aggregation packet */
	RVRTP_H264_MTAP24 = 27,          /* Multi-time aggregation packet */
	RVRTP_H264_FU_A = 28,            /* Fragmentation unit */
	RVRTP_H264_FU_B = 29             /* Fragmentation unit */

} RVRTP_H264NalTypes;

/* Common NAL unit header */
typedef struct _RvRtpH264NalUnitHdr
{	/* NAL header */
    RvUint32  F,			 /* 1 bit  (forbidden_zero_bit) */
              NRI;           /* 2 bits (nal_ref_idc) */
    RVRTP_H264NalTypes Type; /* 5 bits (nal_unit_type) */

} RvRtpH264NalUnitHdr;

/* Single-time aggregation packet - sub NAL */
typedef struct _RvRtpH264StapNalU  /* 5.7.1 */
{
   RvUint16 Size;                     /* STAP NALU size */
   RvRtpH264NalUnitHdr    StapHdr;    /* STAP header */
   RvUint8*				  pData;      /* pointer to NAL payload inside of STAP */

} RvRtpH264StapNalU;
/* Single-time aggregation packet for type A */
typedef struct _RvRtpH264StapANalU
{
	RvRtpH264StapNalU Nal[RVRTP_H264_AP_NAL_UNITS_MAX]; /* payload part of STAP A*/

} RvRtpH264StapANalU;
/* Single-time aggregation packet for type B */
typedef struct _RvRtpH264StapBNalU
{
    RvUint16              Don;		/* 16bit decoding order number */
    RvRtpH264StapNalU Nal[RVRTP_H264_AP_NAL_UNITS_MAX]; /* payload part of STAP B*/

} RvRtpH264StapBNalU;
/* Multi-time aggregation packet sub Nal type*/
typedef struct _RvRtpH264MtapNalU  /* 5.7.1 */
{
	RvUint16  Size;                     /* MTAP16 NALU size */
    RvUint8   Dond;                     /* decoding order number difference */
	RvUint32  TSoffset;                 /* RVRTP_H264_MTAP16 - least 16 bits timestamp offset */
										/* RVRTP_H264_MTAP24 - least 24 bits timestamp offset */
	RvUint8*  pData;                     /* NAL unit */
} RvRtpH264MtapNalU;


typedef struct _RvRtpH264MtapXNalU
{
    RvUint16             Donb;		                         /* 16bit decoding order number base  */
    RvRtpH264MtapNalU    Nal[RVRTP_H264_AP_NAL_UNITS_MAX];   /* payload part of MTAP16 and MTAP24 */
} RvRtpH264MtapXNalU;

/* Fragmented NAL unit header struct */
typedef struct _RvRtpH264FUHdr
{
  RvUint32  S:1,		/* 1 bit  - Start bit, when one, indicates the start of a fragmented NAL
						unit.  Otherwise, when the following FU payload is not the start
						of a fragmented NAL unit payload, the Start bit is set to zero.
						*/
			E:1,        /* 1 bit  (The End bit, when one, indicates the end of a fragmented NAL
						unit, i.e., the last byte of the payload is also the last byte of
						the fragmented NAL unit.  Otherwise, when the following FU
						payload is not the last fragment of a fragmented NAL unit, the
						End bit is set to zero. */
			R:1,		/* 1 bit  Reserved bit MUST be equal to 0
						   and MUST be ignored by the receiver. */
			Type:5;		/* 5 bits (nal_unit_type) */

} RvRtpH264FUHdr;
/* Fragmented NAL unit struct type A */
typedef struct _RvRtpH264FU_A
{
    RvRtpH264FUHdr        FuHdr;               /* 8bits FU header   */
	RvUint8*              pNalFragment;        /* payload with optional padding */

} RvRtpH264FU_A;

/* Fragmented NAL unit struct type B */
typedef struct _RvRtpH264FU_B
{
	RvUint16       Don;         /* 16 bits decoding order number  */
	RvRtpH264FU_A  FU;          /* Remark: Don is placed after FU header and before pNalFragment */
} RvRtpH264FU_B;

/* H.264 payload struct */
typedef struct
{
	RvRtpH264NalUnitHdr NalHdr; /* NAL Indicator in case of Fragmentation */
	union    /* used with specific NAL unit types */
	{  /* aggregation packets */
	   RvUint8*       pSingleNal;        /* 1 <= Type <= 23 */
	   RvRtpH264StapANalU     STAP_A;    /* Type = RVRTP_H264_STAP_A */
	   RvRtpH264StapBNalU     STAP_B;    /* Type = RVRTP_H264_STAP_B */
	   RvRtpH264MtapXNalU     MTAP;      /* Type = RVRTP_H264_MTAP16 or RVRTP_H264_MTAP24 */
	   RvRtpH264FU_A      FU_A;      /* Type = RVRTP_H264_FU_A */
	   RvRtpH264FU_B      FU_B;      /* Type = RVRTP_H264_FU_B */

	} modes; /* H264 aggregation modes */
	RvUint32 Length; /* length of payload filled when unpacked or
	                    by RvRtpH264AddSTAP or RvRtpH264AddMTAP - i.e. before packing */

} RvRtpPayloadH264;
/**********************************************************************************
 * RvRtpH264Construct
 * description: initializes the H264 payload
 * input:       pPayloadH264 - pointer to H264 payload
 * output:      none
 * result : return RV_OK if there are no errors
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH264Construct(RvRtpPayloadH264* pPayloadH264);
/**********************************************************************************
 * RvRtpH264Destruct
 * description: destructs the H264 payload
 * input:       pPayloadH264 - pointer to H264 payload
 * output:      none
 * result : return RV_OK if there are no errors
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpH264Destruct(RvRtpPayloadH264* pPayloadH264);
/**********************************************************************************
 * RvRtpH264AddNalToStap
 * description: inserts the sub-NAL into STAP packet (STAP_A & STAP_B)
 * input:       pStapNal        - pointer to sub NAL to be inserted to STAP payload
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
		IN RvUint16             Don);
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
		IN RvUint16             Donb);

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
    IN  void     *  param);

/********************************************************************************
 * RvRtpH264Pack serializes RvRtpPayloadH264 into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadH264
 * result : return RV_OK if there are no errors
 * Remark: 1) Field Length of RvRtpPayloadH264 must be filled
 *         2) This function performs additional copying and 
 *            not recommenced for usage when RTP packet should contain
 *            one NAL unit only. Use function RvRtpH2641NalPack() in this case.
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpH264Pack(
	IN  void *      buf,
	IN  RvInt32     len,
	IN  RvRtpParam *  p,
	IN  void     *  param);

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
	IN   void *      buf,
	IN   RvInt32     len,
	IN   RvRtpParam *  p,
	OUT  void *      param);
/********************************************************************************
 * RvRtpH264GetLength returns the full size H.264 includes RTP header,
 *                    H.264 payload header and H.264 actual payload
 * input: thisPtr -  pointer to RvRtpPayloadH264
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH264GetLength(
	IN const RvRtpPayloadH264* thisPtr);

#ifdef RVRTP_INCLUDE_TOSTRING_METHODS
RVAPI
RvInt32 RVCALLCONV RvRtpPayloadH264ToString(
	IN const RvRtpPayloadH264* thisPtr,
	INOUT char *buffer,
    IN size_t bufferSize,
	IN const char *prefix);
#endif

/********************************************************************************
 *  RFC 2435 RTP Payload Format for JPEG-compressed Video
 * The RTP timestamp is in units of 90000Hz.  The same timestamp MUST
 * appear in each fragment of a given frame.  The RTP marker bit MUST be
 * set in the last packet of a frame.
 ********************************************************************************
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | Type-specific |              Fragment Offset                  |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |      Type     |       Q       |     Width     |     Height    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Restart Marker header

 This header MUST be present immediately after the main JPEG header
 when using types 64-127.  It provides the additional information
 required to properly decode a data stream containing restart markers.

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |       Restart Interval        |F|L|       Restart Count       |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Quantization Table header

 This header MUST be present after the main JPEG header (and after the
 Restart Marker header, if present) when using Q values 128-255.  It
 provides a way to specify the quantization tables associated with
 this Q value in-band.

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |      MBZ      |   Precision   |             Length            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                    Quantization Table Data                    |
 |                              ...                              |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ********************************************************************************/
typedef struct
{
	/* JPEG header */
    RvInt32 Type_specific;   /* 8b. Interpretation depends on the value of the type field.  If no
    interpretation is specified, this field MUST be zeroed on transmission and ignored on reception. */
	RvInt32 FragmentOffset;  /* 24b. offset in bytes of the current packet in	the JPEG frame data. */
	RvInt32 Type;            /* 8b.  The type field specifies the information that would otherwise be
	    					    present in a JPEG abbreviated table-specification as well as the
							    additional JFIF-style parameters not defined by JPEG.  */
	RvInt32 Q;               /* 8b. the quantization tables for this frame.  */
	RvInt32 Width;           /* 8b. the width of the image in 8-pixel multiples */
	RvInt32 Height;          /* 8b. the height of the image in 8-pixel multiples */
	/* restart Marker (Type in 64-127)*/
	RvInt32 RestartInterval; /* 16b number of MCUs that appear between restart markers.
							    It is identical to the 16 bit value that would appear in the DRI
								marker segment of a JFIF header.  This value MUST NOT be zero. */
	RvInt32 F;              /* 1b First bit */
	RvInt32 L;              /* 1b Last  bit */
	RvInt32 RestartCount;   /* 14bit Restart Count  */
/* Quantization Tables are not implemented */
	/* Quantization Table header   Q is in 128-255 */
	RvInt32 MBZ;
	RvInt32 Precision;      /* 8b.  specifies the size of the coefficients in Quantization Table table */
	RvInt32 Length;         /* 16b. length in bytes of the quantization table data to follow. */
	RvUint32* QTdata;        /* pointer to Quantization Table data buffer (size in 32bit words is specified in Length */
    /* Quantization Table Data must be added after according to the length value */
} RvRtpPayloadJpeg;

/********************************************************************************
 * RvRtpJpegGetLength returns the full size JPEG header includes RTP header,
 * input: thisPtr -  pointer to RvRtpPayloadJpeg, that must be already filled
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpJpegGetLength(IN const RvRtpPayloadJpeg* thisPtr);
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
								   OUT void*param);
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
								 IN    void*param);


/********************************************************************************
 * RFC 2250 - RTP Payload Format for MPEG1/MPEG2 Video
 ********************************************************************************
	 MPEG Video-specific header

   This header shall be attached to each RTP packet after the RTP fixed
   header.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    MBZ  |T|         TR        | |N|S|B|E|  P  | | BFC | | FFC |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                   AN              FBV     FFV


  MPEG-2 Video-specific header extension

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|X|E|f_[0,0]|f_[0,1]|f_[1,0]|f_[1,1]| DC| PS|T|P|C|Q|V|A|R|H|G|D|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


 ********************************************************************************/
typedef struct __MPEG2_header_extension_struct
{
	/* 	MPEG-2 (Two) specific header extension */
    RvUint32 X;     /* Unused (1 bit). Must be set to zero in current specification. */
	RvUint32 E;     /* 1b Extensions present (1 bit). */
	RvUint32 f_00;  /* forward horizontal f_code (4 bits) */
	RvUint32 f_01;  /* forward vertical f_code (4 bits) */
	RvUint32 f_10;  /* backward horizontal f_code (4 bits) */
	RvUint32 f_11;  /* backward vertical f_code (4 bits) */
	RvUint32 DC;    /* intra_DC_precision (2 bits) */
	RvUint32 PS;    /* picture_structure (2 bits) */
	RvUint32 T;     /* top_field_first (1 bit) */
	RvUint32 P;     /* frame_predicted_frame_dct (1 bit) */
	RvUint32 C;     /* concealment_motion_vectors (1 bit) */
	RvUint32 Q;     /* q_scale type (1 bit) */
	RvUint32 V;     /* intra_vlc_format (1 bit) */
	RvUint32 A;     /* alternate scan (1 bit) */
	RvUint32 R;     /* repeat_first_field (1 bit) */
	RvUint32 H;     /* chroma_420_type (1 bit) */
	RvUint32 G;     /* progressive frame (1 bit) */
	RvUint32 D;     /* composite_display_flag (1 bit). */

} Mpeg2Ext;

typedef struct __MPEG1_MPEG2_header_struct
{
    RvUint32 MBZ; /* 5b Unused. Must be set to zero in current specification.*/
	RvUint32 T;   /* 1b TRUE if MPEG-2 (Two) specific header extension present.*/
	RvUint32 TR;  /* 10 bits - Temporal-Reference */
	RvUint32 AN;  /* 1b Active N bit for error resilience */
	RvUint32 N;   /* 1b New picture header */
	RvUint32 S;   /* 1b Sequence-header-present */
	RvUint32 B;   /* 1b Beginning-of-slice (BS) */
	RvUint32 E;   /* 1b End-of-slice (ES) */
	RvUint32 P;   /* 3b Picture-Type. I (1), P (2), B (3) or D (4). */
	RvUint32 FBV; /* 1b full_pel_backward_vector */
	RvUint32 BFC; /* 3b backward_f_code */
	RvUint32 FFV; /* 1b full_pel_forward_vector */
	RvUint32 FFC; /* 3b forward_f_code */
/* 	MPEG-2 (Two) specific header extension */
	Mpeg2Ext mpeg2ext; /* 32 bit MPEG-2 extension */
} RvRtpPayloadMpegVideo;

typedef struct __MPEG_AUDIO_struct
{
    RvUint32 MBZ;         /* 16b Unused. Must be set to zero in current specification.*/
	RvUint32 Frag_offset; /* 16b  Byte offset into the audio frame for the data	in this packet. */

} RvRtpPayloadMpegAudio; /* Note Timestamp of MPEG-Audio is 90000Hz units */

/********************************************************************************
 * RvRtpMpegVUnpack reads buffer and fills Video MPEG-1/2 payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegVideo
 * result : return RV_OK if there are no errors
 * Note: which Unpack function to call RvRtpMpegAUnpack or RvRtpMpegVUnpack
 *       must be selected according to the payload type of received packet
 *       RV_RTP_PAYLOAD_MPV or RV_RTP_PAYLOAD_MPA
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegVUnpack(OUT void*buf,
								   IN  RvInt32 len,
								   OUT RvRtpParam*p,
								   OUT void*param);
/********************************************************************************
 * RvRtpMpegVPack serializes RvRtpPayloadMpegVideo into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegVideo
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegVPack(IN    void*buf,
								 IN    RvInt32 len,
								 IN    RvRtpParam*p,
								 IN    void*param);



/********************************************************************************
 * RvRtpMpegAUnpack reads buffer and fills Audio MPEG-1/2 payload header
 * input: buf - buffer to read from
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegAudio
 * result : return RV_OK if there are no errors
 * Note: which Unpack function to call RvRtpMpegAUnpack or RvRtpMpegVUnpack
 *       must be selected according to the payload type of received packet
 *       RV_RTP_PAYLOAD_MPV or RV_RTP_PAYLOAD_MPA
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegAUnpack(OUT void*buf,
								   IN  RvInt32 len,
								   OUT RvRtpParam*p,
								   OUT void*param);
/********************************************************************************
 * RvRtpMpegAPack serializes RvRtpPayloadMpegAudio into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpPayloadMpegAudio
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RvInt32 RVCALLCONV RvRtpMpegAPack(IN    void*buf,
								 IN    RvInt32 len,
								 IN    RvRtpParam*p,
								 IN    void*param);

/********************************************************************************
 * RvRtpMpegAGetLength returns the full size MPEG Audio header includes RTP header,
 * input: thisPtr -  pointer to RvRtpPayloadMpegAudio, that must be already filled
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpMpegAGetLength(IN const RvRtpPayloadMpegAudio* thisPtr);
/********************************************************************************
 * RvRtpMpegVGetLength returns the full size MPEG Video header includes RTP header,
 * input: thisPtr -  pointer to RvRtpPayloadMpegAudio, that must be already filled
 * output: none
 * result : return RV_OK if there are no errors
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpMpegVGetLength(IN const RvRtpPayloadMpegVideo* thisPtr);

/********************************************************************************
 *            RFC 2429. RTP Payload Format for the 1998 Version of
 *                    ITU-T Rec. H.263 Video (H.263+)
 ********************************************************************************/
#define RVRTP_H263PLUS_MAX_PLEN_VALUE (10)
typedef struct
{
    RvUint32 p;
            /* Indicates the picture start or a picture segment (GOB/Slice) start
            or a video sequence end (EOS or EOSBS).  Two bytes of zero bits
            then have to be prefixed to the payload of such a packet to compose
            a complete picture/GOB/slice/EOS/EOSBS start code.  This bit allows
            the omission of the two first bytes of the start codes, thus
            improving the compression ratio. */
    RvUint32 v;
            /* Indicates the presence of an 8 bit field containing information for
            Video Redundancy Coding (VRC), which follows immediately after the
            initial 16 bits of the payload header if present.  For syntax and
            semantics of that 8 bit VRC field */
    RvUint32 pLen; /*6 bits
            Length in bytes of the extra picture header. If no extra picture header
            is attached, PLEN is 0. If PLEN>0, the extra picture header is attached
            immediately following the rest of the payload header. Note the length
            reflects the omission of the first two bytes of the picture start code (PSC). */
    RvUint32 pEbit; /* 3 bits
            Indicates the number of bits that shall be ignored in the last byte of
            the picture header. If PLEN is not zero, the ignored bits shall be the
            least significant bits of the byte. If PLEN is zero, then PEBIT shall
            also be zero. */

    /* Video Redundancy Coding Header Extension */

    RvUint32 Tid; /* 3 bits
             Thread ID. Up to 7 threads are allowed. Each frame of H.263+ VRC data
             will use as reference information only sync frames or frames within the
             same thread. By convention, thread 0 is expected to be the "canonical"
             thread, which is the thread from which the sync frame should ideally be used.
             In the case of corruption or loss of the thread 0 representation, a
             representation of the sync frame with a higher thread number can be used
             by the decoder. Lower thread numbers are expected to contain equal or better
             representations of the sync frames than higher thread numbers in the absence
             of data corruption or loss. See [7] for a detailed discussion of VRC. */
    RvUint32 Trun; /* 4 bits
             Monotonically increasing (modulo 16) 4 bit number counting the packet
             number within each thread. */

    RvUint32 s; /* A bit that indicates that the packet content is for a sync frame.
             See details in RFC 2429 */

/* See H.263+ (H.263v2) Syntax and semantics chapter, Picture Layer section
   for parsing/filling of picture layer if PLEN is set */

/*  RvRtpH263PlusPack() Parameter */    
    RvUint8* pGBSCorSSC; /* Picture header for Packets that begin with GBSC or SSC                             
                            first zero-16 bits will be ommited as per RFC,
                            needed only when pLen > 0 */

/*  RvRtpH263PlusUnpack Parameter */
    RvUint8  pLenInfo[RVRTP_H263PLUS_MAX_PLEN_VALUE+2];  /* stores plen>0 information after 2 first zero bytes of psc */

} RvRtpH263Plus;

/********************************************************************************
 * RvRtpH263PlusPack serializes H263+ payload header into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpH263Plus
 * result : return RV_OK if there are no errors
 * Note: before packing sbyte must be set to RvRtpH263PlusGetHeaderLength() value.
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH263PlusPack(
    IN	void*        buf,
    IN	RvInt32      len,
    IN	RvRtpParam*  p,
    IN	void*        param);

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
    OUT	    void*       param);

/********************************************************************************
 * RvRtpH263PlusGetHeaderLength()
 * Description: returns the full size H263+ Video header 
 *              includes RTP header. This function can be used before calling 
 *              to RvRtpH263PlusPack().
 * input: h263plus -  pointer to RvRtpH263Plus, that must be already filled
 *              
 * output: none
 * result : returns the full size H263+ Video header includes RTP header
 *          or negative value on error 
 ********************************************************************************/
RVAPI
RvInt32 RVCALLCONV RvRtpH263PlusGetHeaderLength(IN RvRtpH263Plus *h263plus);

/*********************************************************************************
 *  IETF RFC 3558. The RTP payload format for Enhanced Variable Rate Codec (EVRC) 
 *  Speech and Selectable Mode Vocoder (SMV) Speech. 
 *  Two sub-formats are specified for different application scenarios. 
 *  A bundled/interleaved format is included to reduce the effect 
 *  of packet loss on speech quality and amortize the overhead of 
 *  the RTP header over more than one speech frame. 
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      RTP Header [4]                           |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |R|R| LLL | NNN | MMM |  Count  |  TOC  |  ...  |  TOC  |padding|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        one or more codec data frames, one per TOC entry       |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Reserved (RR): 2 bits
      Reserved bits.  MUST be set to zero by sender, SHOULD be ignored
      by receiver.

   Interleave Length (LLL): 3 bits
      Indicates the length of interleave; a value of 0 indicates
      bundling, a special case of interleaving.  See Section 6 and
      Section 7 for more detailed discussion.
   Interleave Index (NNN): 3 bits
      Indicates the index within an interleave group.  MUST have a value
      less than or equal to the value of LLL.  Values of NNN greater
      than the value of LLL are invalid.  Packet with invalid NNN values
      SHOULD be ignored by the receiver.

   Mode Request (MMM): 3 bits
      The Mode Request field is used to signal Mode Request information.
      See Section 10 for details.

   Frame Count (Count): 5 bits
      The number of ToC fields (and vocoder frames) present in the
      packet is the value of the frame count field plus one.  A value of
      zero indicates that the packet contains one ToC field, while a
      value of 31 indicates that the packet contains 32 ToC fields.

   Padding (padding): 0 or 4 bits
      This padding ensures that codec data frames start on an octet
      boundary.  When the frame count is odd, the sender MUST add 4 bits
      of padding following the last TOC.  When the frame count is even,
      the sender MUST NOT add padding bits.  If padding is present, the
      padding bits MUST be set to zero by sender, and SHOULD be ignored
      by receiver.

   The Table of Contents field (ToC) provides information on the codec
	  data frame(s) in the packet.  There is one ToC entry for each codec
      data frame.  The detailed formats of the ToC field and codec data
      frames are specified:
	  Each ToC entry is occupies four bits.  The format of the bits is
      indicated below:

       0 1 2 3
      +-+-+-+-+
      |fr type|
      +-+-+-+-+

   Frame Type: 4 bits
      The frame type indicates the type of the corresponding codec data
      frame in the RTP packet.
      For EVRC and SMV codecs, the frame type values and size of the
      associated codec data frame are described in the table below:

   Value   Rate      Total codec data frame size (in octets)
   ---------------------------------------------------------
     0     Blank      0    (0 bit)
     1     1/8        2    (16 bits)
     2     1/4        5    (40 bits; not valid for EVRC)
     3     1/2       10    (80 bits)
     4     1         22    (171 bits; 5 padded at end with zeros)
     5     Erasure    0    (SHOULD NOT be transmitted by sender)
  
 ********************************************************************************/
#define MAX_TOC			32
#define RR_BITS			2
#define LLL_BITS		3
#define NNN_BITS		3
#define MMM_BITS		3
#define COUNT_BITS		5
#define TOC_BITS		4
#define PADDING_BITS	4

typedef enum					/* Value   Rate      Total codec data frame size (in octets) */
{
	RV_RATE_EvrcSmv_0bits	= 0, /*  0     Blank      0    (0 bit)    */
	RV_RATE_EvrcSmv_16bits  = 1, /*  1     1/8        2    (16 bits)  */
	RV_RATE_EvrcSmv_40bits  = 2, /*  2     1/4        5    (40 bits; not valid for EVRC) */
	RV_RATE_EvrcSmv_80bits  = 3, /*  3     1/2       10    (80 bits)  */
	RV_RATE_EvrcSmv_171bits = 4, /*  4     1         22    (171 bits; 5 padded at end with zeros) */
	RV_RATE_EvrcSmv_Erasure = 5  /*  5     Erasure    0    (SHOULD NOT be transmitted by sender) */
} RvEvrcSmvTocFrmType;

typedef struct 
{
	RvUint16				rr;			  /* Reserved (RR): 2 bits */
	RvUint16				lll;		  /* Interleave Length (LLL): 3 bits */
	RvUint16				nnn;		  /* Interleave Index (NNN): 3 bits */
	RvUint16				mmm;		  /* Mode Request (MMM): 3 bits */
	RvUint16				count;		  /* Frame Count (Count): 5 bits */
	RvUint16				toc[MAX_TOC]; /* The Table of Contents field (ToC) */
	RvUint16				padding;	  /* Padding (padding): 0 or 4 bits */
} RvRtpEvrcSmv;

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
	IN  RvRtpEvrcSmv *  param);

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
	OUT  RvRtpEvrcSmv *   param);



/*******************************************************************************
 * RFC 3389                                                                    *
 * Real-time Transport Protocol (RTP) Payload for Comfort Noise (CN)           *
 *******************************************************************************/
#define RVRTP_CN_MAX_MODEL_ORDER (255)
typedef struct 
{
    RvUint8             modelOrder;       /* spectral model order must be Filled before calling 
                                             for function RvRtpCnPack() */
/*
                        0 1 2 3 4 5 6 7
                       +-+-+-+-+-+-+-+-+
                       |0|   level     |          0-bit is always 0
                       +-+-+-+-+-+-+-+-+
*/
	RvUint8				level;			                       /* Noise Level  0-127 */
                                                               /* 0 - 254 coefficients, 255 is reserved for future use */
    RvUint8             rc[RVRTP_CN_MAX_MODEL_ORDER];          /* array of quantized reflection coefficients */

} RvRtpComfNoisePayload;

/********************************************************************************
 * RvRtpCnPack serializes RvRtpComfNoisePayload into buffer
 * INPUT: buf - buffer to serialize
 *        len - length of the buf
 *        p   - pointer to RvRtpParam ( rtp header)
 * OUT    param -  pointer to RvRtpConfNoisePayload
 * result : return RV_OK if there are no errors
 * Remarks: 1. modelOrder must be filled. Assumption: 
 *                modelOrder <= RVRTP_CN_MAX_MODEL_ORDER
 *          2. After using RvRtpCnPack() function use RvRtpWrite() function
 *             with len parameter equal to p->sByte for sending RTP packet 
 *             with Comfort Noise.
 ********************************************************************************/

RVAPI
RvStatus RVCALLCONV RvRtpCnPack(
	IN  void *			buf,
	IN  RvInt32			len,
	IN  RvRtpParam *	p,
	IN  RvRtpComfNoisePayload*  param);

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
	OUT  RvRtpComfNoisePayload*   param);

/* == JPEG200 == */
/* RFC 5371
*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
.                                                               .
.                          RTP header                           .
.                                                               .
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       JPeg 2000 header                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       JPeg 2000 stream ...                    .
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
/* Jpeg 2000 payload header format
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|tp |MHF|mh_id|T|     priority  |           tile number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|reserved       |             fragment offset                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct
{
   RvUint8  tp; /*
   tp (type): 2 bits
   This field indicates how a JPEG 2000 image is scanned (progressive
   or interlace).
   0: The payload is progressively scanned.
   1: The payload is part of an odd field of an interlaced video
   frame.  The height specified in the JPEG 2000 main header is
   half of the height of the entire displayed image.  In a
   receiver, an odd field should be de-interlaced with the even
   field following it so that lines from each image are displayed
   alternately.
   2: The payload is part of an even field of an interlaced video
   signal.
*/
   RvUint8  MHF;
/*
MHF (Main Header Flag): 2 bits
MHF indicates whether a main header or packet of a main header is
in the RTP packet.
If there is no header, MHF has a value of 0.  If there is just a
part of a fragmented header, MHF has a value of 1.  If there is
the last part of a fragmented header, MHF has value of 2.  If the
whole header is in the packet, MHF has a value of 3.
+-----------+----------------------------------+
| MHF Value | Description                      |
+-----------+----------------------------------+
|     0     | no main header in the payload    |
|     1     | piece of fragmented header       |
|     2     | last part of a fragmented header |
|     3     | a whole main header              |
+-----------+----------------------------------+
Table. MHF Usage Values */
   RvUint8  mh_id;
/*
mh_id (Main Header Identification): 3 bits
Main header identification value.  This is used for JPEG 2000 main
header recovery.
For implementations following only this specification, the sender
SHOULD set this value to 0 and the receiver SHOULD ignore this
field on processing.
*/
   RvUint8  T;
/* T (Tile field invalidation flag): 1 bit
The T bit indicates whether the tile number field is valid or
invalid.  A sender MUST set the T bit to 1 when invalid and 0 when
valid.
There are two cases where the tile number field is invalid:
*  When an RTP packet holds only the main header.  A sender cannot
set any number in the tile number field, as no JPEG 2000 tile-
part bit-stream is included in the RTP packet.
*  Multiple tile-parts are packed together in a single payload.
If there are multiple tiles packed into a single payload, there
is no meaning to assign a number to the tile number field.
*/
   RvUint8  priority;
/*   priority: 8 bits - The priority field indicates the importance of the JPEG 2000
   packet included in the payload.  Typically, a higher priority
   value is set in the packets containing JPEG 2000 packets that
   contain the lower sub-bands.
   For implementations following only this specification, the sender
   SHOULD set this value to 255 and the receiver SHOULD ignore this
   field on processing.
*/
   RvUint16 tileNumber;
/*   tile number: 16 bits
   This field shows the tile number of the payload.  This field is
   only valid when the T bit is 0.  If the T bit is set to 1, the
   receiver MUST ignore this field. */
   
   RvUint8 reserved;  
/*   R (Reserved): 8 bits.   This bit is reserved for future use.  Senders MUST set this to 0.
   Receivers MUST ignore this field. */

   RvUint32 fragmentOffset; 
/*   fragment offset: 24 bits
   This value MUST be set to the byte offset of the current payload
   in relation to the very beginning of each JPEG 2000 codestream
   (JPEG 2000 frame).
   Byte offsets are calculated from the start of each JPEG 2000
   code-stream up to the current position where the current payload
   would fit into the complete JPEG 2000 codestream.
   To perform scalable video delivery by using multiple RTP sessions,
   the offset value from the first byte of the same frame is set for
   fragment offset.  It is then possible to deliver layered video
   using multiple RTP sessions; the fragment offset might not start
   from 0 in some RTP sessions even if the packet is the first one
   received in the RTP session. */

} RvRtpPayloadJpeg2000;

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
    IN  void *                 param);

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
    OUT  void *                  param);

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
RvInt32 RVCALLCONV RvRtpJpeg2000GetHeaderLength(void);

#ifdef __cplusplus
}
#endif

#endif  /* __PAYLOAD_H */
