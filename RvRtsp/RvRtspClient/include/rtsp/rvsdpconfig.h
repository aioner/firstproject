/******************************************************************************
Filename    :rvsdpconfig.h
Description : compilation switches defining the behavior of the SDP library

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD..
  
    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
 Author:Rafi Kiel
******************************************************************************/

#ifndef _file_rvsdpconfig_h_
#define _file_rvsdpconfig_h_

/* whether 'ads' library is used */
/* #define RV_SDP_ADS_IS_USED */
 
/* enables 'key-mgmt' attribute support */
#define RV_SDP_KEY_MGMT_ATTR 

/* enables 'crypto' attribute support */
#define RV_SDP_CRYPTO_ATTR

/* enables 'framerate' attribute support */
#define RV_SDP_FRAMERATE_ATTR

/* enables 'fmtp' attribute support */
#define RV_SDP_FMTP_ATTR

/* enables parser warnings support */
#define RV_SDP_PARSE_WARNINGS    

/* enables support of invalid syntax */
/*#define RV_SDP_CHECK_BAD_SYNTAX */

/* enable reparse functionality */
#ifdef RV_SDP_CHECK_BAD_SYNTAX
#define RV_SDP_ENABLE_REPARSE 
#endif

/* use macros instead of small functions */
#define RV_SDP_USE_MACROS 

/* name of log messages file created by SDP library */
#define RV_SDP_LOG_FILE_NAME "RvSdpLogFile.log"

/* 
 *  whether the spaces are allowed in the SDP input at the beginning of
 *  the line (before the tag letter) and between the tag letter and equal sign
 */
#define RV_SDP_SPACES_ALLOWED

 
/* the initial size of strings buffer (of each RvSdpMsg) */
#define RV_SDP_MSG_BUFFER_SIZE              1024

/* initial size of RvSdpAttribute instances pool (of each RvSdpMsg) */
#define RV_SDP_ATTRPOOL_INIT_SZ             4
/* inscrease step */
#define RV_SDP_ATTRPOOL_INCR_SZ             2

/* initial size of RvSdpOther instances pool (of each RvSdpMsg) */
#define RV_SDP_OTHERPOOL_INIT_SZ            2
/* inscrease step */
#define RV_SDP_OTHERPOOL_INCR_SZ            1

/* initial size of RvSdpEmail instances pool (of each RvSdpMsg) */
#define RV_SDP_EMAILPOOL_INIT_SZ            1
/* inscrease step */
#define RV_SDP_EMAILPOOL_INCR_SZ            1

/* initial size of RvSdpPhone instances pool (of each RvSdpMsg) */
#define RV_SDP_PHONEPOOL_INIT_SZ            1
/* inscrease step */
#define RV_SDP_PHONEPOOL_INCR_SZ            1

/* initial size of RvSdpMediaDescr instances pool (of each RvSdpMsg) */
#define RV_SDP_MEDIAPOOL_INIT_SZ            2
/* inscrease step */
#define RV_SDP_MEDIAPOOL_INCR_SZ            1

/* initial size of RvSdpRtpMap instances pool (of each RvSdpMsg) */
#define RV_SDP_RTPMAPPOOL_INIT_SZ           2
/* inscrease step */
#define RV_SDP_RTPMAPPOOL_INCR_SZ           1

/* initial size of RvSdpSessionTime instances pool (of each RvSdpMsg) */
#define RV_SDP_SESSTIMEPOOL_INIT_SZ         1
/* inscrease step */
#define RV_SDP_SESSTIMEPOOL_INCR_SZ         1

/* initial size of RvSdpTimeZoneAdjust instances pool (of each RvSdpMsg) */
#define RV_SDP_TZAPOOL_INIT_SZ              1
/* inscrease step */
#define RV_SDP_TZAPOOL_INCR_SZ              1

/* initial size of RvSdpConnection instances pool (of each RvSdpMsg) */
#define RV_SDP_CONNECTIONPOOL_INIT_SZ       2
/* inscrease step */
#define RV_SDP_CONNECTIONPOOL_INCR_SZ       1

/* initial size of RvSdpBandwidthPool instances pool (of each RvSdpMsg) */
#define RV_SDP_BANDWIDTHPOOL_INIT_SZ        2
/* inscrease step */
#define RV_SDP_BANDWIDTHPOOL_INCR_SZ        1

/* initial size of RvSdpRepeatInterval instances pool (of each RvSdpMsg) */
#define RV_SDP_REPEATINTERVALPOOL_INIT_SZ   1
/* inscrease step */
#define RV_SDP_REPEATINTERVALPOOL_INCR_SZ   1

/* initial size of RvSdpTypedTime instances pool (of each RvSdpMsg) */
#define RV_SDP_TYPEDTIMEPOOL_INIT_SZ        4
/* inscrease step */
#define RV_SDP_TYPEDTIMEPOOL_INCR_SZ        2

#define RVSDPCOREAPI  RVCOREAPI

/* enables memory allocations monitoring */
/*#define RV_SDP_USE_DEBUG_ALLOC_MODEL*/

#endif

