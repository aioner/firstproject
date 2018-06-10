/************************************************************************
 File Name	   :   rvrtpconfig.c  --  RTP module cofiguration related 
*                 functions
 Description   : scope: private
*************************************************************************/
/***********************************************************************
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

#include "rvtypes.h"
#include "rvobjpool.h"
#include "rverror.h"
#include "rvrtpconfig.h"
#include "rtputil.h"
#include "rvrtpaddresslist.h"

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)
#define   rvLogPtr      rtpGetSource(RVRTP_RTP_MODULE)
#endif
#include "rtpLogFuncs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if(RV_LOGMASK != RV_LOGLEVEL_NONE)    
static RvRtpLogger rtpLogManager = NULL;
#define logMgr (RvRtpGetLogManager(&rtpLogManager),((RvLogMgr*)rtpLogManager))

#else

#define logMgr     (NULL)
#define RTP_SOURCE (NULL)
#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */

static RvRtpPoolConfig destinationAddressPoolConfig =
{
    NULL,
    RV_RTP_DESTINATION_POOL_PAGEITEMS_DEFAULT,
    RV_RTP_DESTINATION_POOL_PAGESIZE_DEFAULT,
    RV_RTP_DESTINATION_POOL_TYPE_DEFAULT,
    RV_RTP_DESTINATION_POOL_MAXITEMS_DEFAULT,
    RV_RTP_DESTINATION_POOL_MINITEMS_DEFAULT,
    RV_RTP_DESTINATION_POOL_FREELEVEL_DEFAULT
};

RvRtpPoolConfig* poolConfig = &destinationAddressPoolConfig;

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
       IN RvSize_t       freeLevel/*,       IN RvMemory *region get from default region*/)
{
    RTPLOG_ENTER(SetDestinationAddressPoolConfig);
    if((poolType != RV_OBJPOOL_TYPE_FIXED) && (poolType != RV_OBJPOOL_TYPE_EXPANDING) && (poolType != RV_OBJPOOL_TYPE_DYNAMIC))
    {
        RTPLOG_ERROR_LEAVE(SetDestinationAddressPoolConfig, "Invalid pool type");
        return RV_ERROR_BADPARAM;
    }
    if(freeLevel > 100)
    {
        RTPLOG_ERROR_LEAVE(SetDestinationAddressPoolConfig, "Free level must be <= 100");
        return RV_ERROR_BADPARAM;
    }
    if((maxItems < minItems) && (maxItems != 0)) 
    {
        RTPLOG_ERROR_LEAVE(SetDestinationAddressPoolConfig, "Max items must be > min items");
        return RV_ERROR_BADPARAM;
    }

    poolConfig->region    = NULL;
    poolConfig->pageItems = pageItems;
    poolConfig->pageSize  = pageSize;;
    poolConfig->poolType  = poolType;
    poolConfig->maxItems  = maxItems;
    poolConfig->minItems  = minItems;
    poolConfig->freeLevel = freeLevel;
#if(RV_LOGMASK != RV_LOGLEVEL_NONE)     
    RV_UNUSED_ARG(rtpLogManager);
#endif
    RTPLOG_LEAVE(SetDestinationAddressPoolConfig);
    return RV_OK;
}

#ifdef __cplusplus
}
#endif


