/************************************************************************
 File Name     : rvrtpconfig.h
 Description   : This file contains compile time constants used to
                 configure the RADVISION RTP module.
*************************************************************************
 Copyright (c)  2001 , RADVision, Inc. All rights reserved.
*************************************************************************
 NOTICE:
 This document contains information that is proprietary to RADVision Inc.
 No part of this publication may be reproduced in any form whatsoever
 without written prior approval by RADVision Inc.

 RADVision Inc. reserves the right to revise this publication and make
 changes without obligation to notify any person of such revisions or
 changes.
************************************************************************/

#if !defined(RVRTPCONFIG_H)
#define RVRTPCONFIG_H

#include "rverror.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pool types */
typedef enum
{
    RvRtpPoolTypeFixed,
    RvRtpPoolTypeExpanding,           /* default value */
    RvRtpPoolTypeDynamic

} RvRtpPoolType;

/* Destination pool configuration type */
typedef struct __PoolConfig
{
    void*     region;    /* Memory region to allocate contexts from (NULL = use default) */
    RvSize_t  pageItems; /* Number of context objects per memory page (0 = calculate from pageItems). */
    RvSize_t  pageSize;  /* Size of memory page (0 = calculate from pageItems). */
    RvInt     poolType;  /* RvRtpPoolTypeFixed, RvRtpPoolTypeExpanding, or RvRtpPoolTypeDynamic. */
    RvSize_t  maxItems;  /* Number of context objects will never exceed this value (0 = no max). */
    RvSize_t  minItems;  /* Number of context objects will never go below this value. */
    RvSize_t  freeLevel; /* Minimum number of context objects per 100 (0 to 100) - DYNAMIC pools only. */

} RvRtpPoolConfig;

/* For backward (RTP version 2.0/2.1) compatibility */
#define RVRTP_OLD_CONVENTION_API

#define RVRTP_INCLUDE_TOSTRING_METHODS              /* for debug purposes print methods only */
#define RVRTP_MAXSESSIONS					(512)	/* Max RTP/RTCP sessions that can be opened simultaneously */
#define RTCP_MAXRTPSESSIONMEMBERS           (1024)    /* default RTCP session max RTP members */
#define RTCP_MAXSDES_LENGTH                 (255)   /* source description max length */
#define RV_RTCP_BANDWIDTH_DEFAULT           (500)   /* default RTCP bandwith: bytes per second */
#define RVRTP_SECURITY
#define MAX_FB_SIZE							96		/* maximum size of FeedBack in bytes.
													   96 bytes permit to combine 6 FB messages in 1 packet*/


/* logger configuration for each of the RTP library modules*/
#define RVRTP_SET_RTP_GLOBAL_MASK_DEFAULT    /* to set RVRTP_GLOBAL_MASK_DEFAULT */
#define RVRTP_GLOBAL_MASK_DEFAULT		     (RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_RTP_MODULE_MASK_DEFAULT        (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_RTCP_MODULE_MASK_DEFAULT       (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_PAYLOAD_MODULE_MASK_DEFAULT    (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_SECURITY_MODULE_MASK_DEFAULT   (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_SRTP_MODULE_MASK_DEFAULT       (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_RTCP_XR_MODULE_MASK_DEFAULT    (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_FEC_MODULE_MASK_DEFAULT        (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)
#define RVRTP_RTCP_FB_MODULE_MASK_DEFAULT    (RVRTP_LOG_ERROR_FILTER|RVRTP_LOG_EXCEP_FILTER)

/* RVRTP_INTERNAL_LOGGER defines  internal logger listener for RTP/RTCP library
 type of listener must be defined in rvusrconfig.h
 RV_LOGLISTENER_TYPE is RV_LOGLISTENER_TERMINAL means
 that all logger printings are passed through the TERMINAL.
 RV_LOGLISTENER_TYPE is RV_LOGLISTENER_FILE_AND_TERMINAL means
 that all logger printings are passed through the file: RV_RTP_LOGFILE_NAME.
 Undefined RVRTP_INTERNAL_LOGGER means that listeners must be specified
 in external to RTP library code */
/*#define RVRTP_INTERNAL_LOGGER*/

#define RV_RTP_LOGFILE_NAME    "rtplog.txt"

/* H.264 configuration */
#define RVRTP_H264_AP_NAL_UNITS_MAX           (4) /* Maximum Nal units in aggregation packet */
/* MPEG-4 configuration */
#define RV_RTPPAYLOADMPEG4_MAX_AU_IN_PACKET   (4)    /* maximal number of AU in the packet */
#define RV_RTPPAYLOADMPEG4_MAX_PAYLOAD_SIZE   (1200)   /* for MPEG-4 for fragmentation */

/* From Jay code */
/* The maximum RTCP packet size, in bytes, expected to be received.*/
#define RV_RTPSESSION_MAXIMUM_RTCP_RECEIVED_PACKET_SIZE   4048

/* The maximum RTCP packet size, in bytes, expected to be sent.*/
#define RV_RTPSESSION_MAXIMUM_RTCP_SENT_PACKET_SIZE   4048

/* Define this variable if you need the ToString methods in this package */
#define RV_RTP_INCLUDE_TOSTRING_METHODS



/* RTP Destination pool confiruration */
/* #define    RV_RTP_DEST_POOL_OBJ *//* If RTP_DEST_POOL_OBJ is undefined,
                                        each time when needed dynamic allocation is used for allocation of destination addresses.
                                        If RTP_DEST_POOL_OBJ is defined, destination addresses are allocated from RTP Destination Pool */
                                     /* Note: This pool is used for allocation of RTP and RTCP destination addresses for the session */


#define RV_RTP_DESTINATION_POOL_TYPE_DEFAULT         RvRtpPoolTypeExpanding
#define RV_RTP_DESTINATION_POOL_PAGEITEMS_DEFAULT    200 /* 200 addresses per page (this includes RTP and RTCP addresses) */
#define RV_RTP_DESTINATION_POOL_PAGESIZE_DEFAULT     0   /* calculated automatically */
#define RV_RTP_DESTINATION_POOL_MAXITEMS_DEFAULT     0   /* calculated automatically */
#define RV_RTP_DESTINATION_POOL_MINITEMS_DEFAULT     0   /* calculated automatically */
#define RV_RTP_DESTINATION_POOL_FREELEVEL_DEFAULT    0   /* calculated automatically */


#if defined(RV_RTP_DEST_POOL_OBJ)

/************************************************************************************
 * RvRtpSetDestinationAddressPoolConfig
 * description: This function sets the behavior of the RTP/RTCP Destination Address pool.
 *              A destination Address object is required to locate the address of remote
 *              RTP/RTCP party to send.
 *              Fixed pools are just that, fixed, and will only allow the
 *              specified number of objects to exist. Expanding pools will
 *              expand as necessary, but will never shrink (until the
 *              plugin is unregistered). Dynamic pools will, in addition to
 *              expanding, shrink based on the freeLevel parameter.
 *              By default, this pool will be configured as an expanding
 *              pool with 200 Destination Address  objects per memory page and the memory
 *              being allocated from the default region.
 -------------------------------------------------------------------------------------
 * input:
 *        poolType       - The type of pool.
 *        pageItems      - The number of objects per page (0 = calculate from pageSize).
 *        pageSize       - The size of a memory page (0 = calculate from pageItems).
 *        maxItems       - The number of objects will never exceed this value (0 = no max).
 *        minItems       - The number of objects will never go below this value.
 *        freeLevel      - The minimum number of objects per 100 (0 to 100) - DYNAMIC pools only.
 * output: none.
 * return value: Non-negative value on success
 *               Negative value on failure
 * Note: this function can be called before RvRtpInit in order to change Default Address
 *       Pool configuration
 ************************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSetDestinationAddressPoolConfig(
       IN RvRtpPoolType  poolType,
       IN RvSize_t       pageItems,
       IN RvSize_t       pageSize,
       IN RvSize_t       maxItems,
       IN RvSize_t       minItems,
       IN RvSize_t       freeLevel);

#endif

/******************************************************************************************************
 * Definition of __H323_NAT_FW__ allows NAT/FW traversal of RTP/RTCP according to H.460.19-MA.        *
 * Note: This version does not support H.323 NAT/FW traversal of SRTP/SRTCP according to H.460.19-MA  *
 ******************************************************************************************************/
#undef __H323_NAT_FW__
//define by kongfd 
#define __H323_NAT_FW__
#define NO_SSL


#define RvSocketPeekBuffer(sock, buffer, len, logMgr, bytesPending, remoteAddressPtr)  RV_ERROR_NOTSUPPORTED

/************************************************************************************************
 * Definition of __RTP_OVER_STUN__ allows RTP/RTCP passing over STUN.
 * STUN (Simple Traversal of UDP through NATs (Network Address Translation)) is a protocol
 * for assisting devices behind a NAT firewall or router with their packet routing.
 ************************************************************************************************/
#define __RTP_OVER_STUN__

/************************************************************************************************
 * Definition of __RTCP_XR__ allows RTCP Extended Reports according to RFC 3611.
 ************************************************************************************************/
/*#define __RTCP_XR__*/
/************************************************************************************************
 * Definition of __RTCP_FB__ allows RTCP Feedback according to RFC 4585.
 ************************************************************************************************/
/*#define __RTCP_FB__*/
/************************************************************************************************
 * RTP with Turn server
 ************************************************************************************************/
#undef __RTP_TURN__ /* should be here to remove double definition if exist */
/*#define __RTP_TURN__*/


#ifdef __cplusplus
}
#endif

#endif  /* Include guard */











































