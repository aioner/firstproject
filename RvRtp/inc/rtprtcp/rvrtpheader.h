/***********************************************************************
        Copyright (c) 2004 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

#ifndef __RVRTPHEADER_H
#define __RVRTPHEADER_H

#include "rvtypes.h"
#include "rverror.h"
#include "rvrtpconfig.h"
#include "rvrtpinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RvRtpParam_
{
	IN OUT  RvUint32    timestamp;         /* RTP timestamp */
	IN OUT  RvBool      marker;            /* RTP marker bit */
	IN OUT  RvUint8     payload;           /* RTP payload type */
	
	OUT     RvUint32    sSrc;              /* SSRC of received RTP packet */
	OUT     RvUint16    sequenceNumber;    /* sequence number of received rtp packet */
	OUT     RvUint16    extSequenceNumber; /* sequence number of received rtp packet extended by number of cycles. see RFC 3550 A.1*/
	OUT     RvInt32     sByte;             /* offset that points on actual beginning of the RTP payload */
	INOUT   RvInt32     len;               /* length of receiving payload */ 
	                                       /* when sending packet with encryption used 
	                                          len is the size of buffer for sending.
	                                          Encryption may add additional padding.
	                                          SRTP may add authentication and MKI information to a packet.*/
	/* The following fields are not present in OLD API and should not be filled if RVRTP_OLD_CONVENTION_API is defined */
    INOUT   RvBool      extensionBit;      /* TRUE, if extension is present */
    INOUT   RvUint16    extensionProfile;  /* application-depended profile */
    INOUT   RvUint16    extensionLength;   /* the length in 4-byte integers of the extensionData field, 0 is valid */
    INOUT   RvUint32*   extensionData;     /* pointer to the extension data */


	IN      RvBool      paddingBit;        /* for internal usage only */


} RvRtpParam;

/*===========================================================================
**  == RvRtpParamConstruct() ==                                            **
**                                                                         **
**  initializes a RvRtpParam structure                                     **
**                                                                         **
**  PARAMETERS:                                                            **
**      param      pointer to RvRtpParam structure.                        **
**                                                                         **
**  RETURN  : RV_OK on success, other on failure                           **
**                                                                         **
**=========================================================================*/

RVAPI
RvStatus RVCALLCONV RvRtpParamConstruct(INOUT RvRtpParam* param);

/*===========================================================================
**  == RvRtpParamGetExtensionSize() ==                                     **
**                                                                         **
**  returns RTP header extension size in Bytes                             **
**                                                                         **
**  PARAMETERS:                                                            **
**      INPUT param      pointer to RvRtpParam structure, that filled with **
**                       extension fields.                                 **
**                                                                         **
**  RETURN  : RV_OK on success, other on failure                           **
**                                                                         **
**=========================================================================*/
RVAPI
RvUint32 RVCALLCONV RvRtpParamGetExtensionSize(IN RvRtpParam* param);

#ifdef __cplusplus
}
#endif

#endif /* __RVRTPHEADER_H */

