/************************************************************************
 File Name	   : rtpLogFuncs.h
 Description   : logging funcs
*************************************************************************
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
***********************************************************************
$Revision: $
$Date:   03/29/2005 $
$Author: Michael Zak $
************************************************************************/

#ifndef __RTP_LOG_FUNCS_H_
#define __RTP_LOG_FUNCS_H_

#include "rvrtplogger.h"


#ifdef __cplusplus
extern "C" {
#endif
    
#if(RV_LOGMASK != RV_LOGLEVEL_NONE)
    /* Fetches RvLogSource object */
//#define RTP_SOURCE  -> must be defined locally
    
    /* Fetches RvLogMgr object */
//#define RTP_MANAGER  -> must be defined locally
    
    /* Log error macro with multiple parameters
    * Usage:
    *  RTPLOG_ERROR((RTP_SOURCE, format,...));
    */
    
#define RTPLOG_ERROR(params) RvLogError(rvLogPtr, params)
  
    
    /* Log info macro with multiple parameters
    * Usage:
    *  RTPLOG_INFO((RTP_SOURCE, format,...));
    */
#define RTPLOG_WARNING(params)  RvLogWarning(rvLogPtr, params)

    /* Log info macro with multiple parameters
    * Usage:
    *  RTPLOG_INFO((RTP_SOURCE, format,...));
    */
#define RTPLOG_INFO(params)  RvLogInfo(rvLogPtr, params)
    
    /* Log debug macro with multiple parameters
    * Usage:
    *  RTPLOG_DEBUG((RTP_SOURCE, format,...));
    */
#define RTPLOG_DEBUG(params)  RvLogDebug(rvLogPtr, params)
    /*
    *  Makes long function name from the short one
    */
#define FUNC_NAME(name) "RvRtp" #name
    
    /* Log function entry macro
    * Usage:
    *  RTPLOG_ENTER(shortFunctionName);
    *  shortFunctionName - function name without object name, e.g
    *   short name of 'RvRtpConstruct' is just 'Construct'
    *   and macros call should be:
    *
    *  RTPLOG_ENTER(Construct);
    *
    * Pay attention: no quotes needed
    */
#define RTPLOG_ENTER(funcName) \
    RvLogEnter(rvLogPtr, (rvLogPtr, FUNC_NAME(funcName)))
    
    /* Log function leave macro
    * Usage:
    *  RTPLOG_LEAVE(shortFunctionName);
    */
#define RTPLOG_LEAVE(funcName) \
    RvLogLeave(rvLogPtr, (rvLogPtr, FUNC_NAME(funcName)))
    

    /*
    *  Logs simple error message
    *  Arguments:
    *    func - short function name
    *    msg - message to log
    */
#define RTPLOG_WARNING1(func, msg) RTPLOG_WARNING((rvLogPtr, FUNC_NAME(func) " - " msg))
    
    /*
    *  Logs simple error message
    *  Arguments:
    *    func - short function name
    *    msg - message to log
    */
#define RTPLOG_ERROR1(func, msg) RTPLOG_ERROR((rvLogPtr, FUNC_NAME(func) " - " msg))
    
    /*
    *  Logs simple info message
    *  Arguments:
    *    func - short function name
    *    msg - message to log
    */
#define RTPLOG_INFO1(func, msg)  VPHLOG_INFO((rvLogPtr, FUNC_NAME(func) " - " msg))
    
    /*
    *  Logs simple error message and leave message afterwards
    */
#define RTPLOG_ERROR_LEAVE(funcName, msg) \
{\
    RTPLOG_ERROR1(funcName, msg); \
    RTPLOG_LEAVE(funcName); \
}   

#else

#define RTP_SOURCE  (NULL)

#define RTPLOG_ERROR(params)
#define RTPLOG_WARNING(params)    

#define RTPLOG_INFO(params)
#define RTPLOG_DEBUG(params)
#define RTPLOG_ENTER(funcName)
#define RTPLOG_LEAVE(funcName)
#define RTPLOG_ERROR1(func, msg)
#define RTPLOG_INFO1(func, msg)
#define RTPLOG_WARNING1(func, msg)
#define RTPLOG_ERROR_LEAVE(funcName, msg)

#endif /*(RV_LOGMASK != RV_LOGLEVEL_NONE)*/

#ifdef __cplusplus
}
#endif


#endif /*__RTP_LOG_FUNCS_H_*/

