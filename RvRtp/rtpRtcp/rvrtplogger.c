/************************************************************************
 File Name	   : rvrtplogger.c
 Description   : Functions definitions related to logger printouts
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

#include "rvlog.h"
#include "rtputil.h"
#include "rvrtplogger.h"

#ifdef __cplusplus
extern "C" {
#endif
    
extern RvRtpInstance rvRtpInstance;

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

/* Logger static variables */
static RvLogSource* rvLogPtr  = NULL;
static RvLogMgr*    logMgr    = NULL;
static RvRtpLogManager*  rtpLogManagerPtr = NULL;

/* <= LOGGER related functions => */
static const char* rtpModuleName       = "RTP";
static const char* rtcpModuleName      = "RTCP";
static const char* payloadModuleName   = "Payload";
static const char* securityModuleName  = "Security";
static const char* srtpModuleName      = "SRTP";
static const char* errorModuleName     = "Unknown";
static const char* rtcpXrModuleName    = "RTCPXR";
static const char* rtpFecModuleName    = "FEC";
static const char* rtcpFbModuleName    = "RTCPFB";

static const RvChar* rtpLoggerModuleName(IN RvRtpModule module)
{
    const RvChar* str = NULL;
    switch(module)
    {
    case RVRTP_RTP_MODULE:
        str = rtpModuleName;
        break;
    case RVRTP_RTCP_MODULE:
        str = rtcpModuleName;
        break;
    case RVRTP_PAYLOAD_MODULE:
        str = payloadModuleName;
        break;
    case RVRTP_SECURITY_MODULE: 
        str = securityModuleName;
        break;
    case RVRTP_SRTP_MODULE:
        str = srtpModuleName;
        break;
    case RVRTP_RTCP_XR_MODULE:
        str = rtcpXrModuleName;
        break;
    case RVRTP_FEC_MODULE:
        str = rtpFecModuleName;
        break;        
    case RVRTP_RTCP_FB_MODULE:
        str = rtcpFbModuleName;
        break;
    default:
        str = errorModuleName;
        break;
    }
    return str;
}


RVAPI
RvLogSource* RVCALLCONV   rtpGetSource(RvRtpModule module)
{
    RvLogSource* logPtr = NULL;
    if (NULL == logMgr)
    {
        return NULL;
    }	
    if (rvRtpInstance.logManager.bLogSourceInited[(int)module])
    {
        logPtr = &rvRtpInstance.logManager.logSource[(int)module];
    }
    return logPtr;
}


RvStatus  RvRtpSourceConstruct(RvRtpModule module)
{
    RvStatus result = RV_ERROR_LIBCODE_RTP;
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSourceConstruct"));
	if (NULL==logMgr)
	{
	    RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSourceConstruct"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSourceConstruct"));
		return result;
	}	
	rvLogPtr = &rvRtpInstance.logManager.logSource[module];
	result = RvLogSourceConstruct(logMgr, rvLogPtr, rtpLoggerModuleName(module), "RTP/RTCP");
    if (result == RV_OK)
    {
        rvRtpInstance.logManager.bLogSourceInited[module] = RV_TRUE;
    }
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSourceConstruct"));
	return result;
}

RvStatus  RvRtpLogSourceDestruct(RvRtpModule module)
{
    RvStatus result = RV_ERROR_LIBCODE_RTP;
    RvLogSource* source = &rvRtpInstance.logManager.logSource[module];
/*    RvLogEnter(source, (source, "RvLogSourceDestruct"));*/
    if (NULL==logMgr)
    {
/*        RvLogError(source, (source, "RvLogSourceDestruct"));
        RvLogLeave(source, (source, "RvLogSourceDestruct"));*/
        return result;
    }	
/*    RvLogLeave(source, (source, "RvLogSourceDestruct"));    */
    rvRtpInstance.logManager.bLogSourceInited[module] = RV_FALSE;    
    result = RvLogSourceDestruct(source);    
    return result;
}
/****************************************************************************
 * RvRtpGetLogManager gets RvRtpLogger
 * for using of RTP/RTCP log manager in external/internal applications
 ****************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpGetLogManager(OUT RvRtpLogger* logMgrPtr)
{
	*logMgrPtr = (RvRtpLogger) logMgr;
	return RV_OK;
}
/****************************************************************************
 * RvRtpSetExternalLogManager sets external product log manager
 * for using in RTP/RTCP 
 ****************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtpSetExternalLogManager(IN RvRtpLogger* handlePtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetExternalLogManager"));	
	if (NULL == handlePtr)
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetExternalLogManager: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetExternalLogManager"));
		return RV_ERROR_NULLPTR;
	}
	logMgr = (RvLogMgr*) handlePtr;
	rtpLogManagerPtr = &rvRtpInstance.logManager;
	rvRtpInstance.logManager.bExternal = RV_TRUE;
	rvRtpInstance.logManager.printCB = NULL;
    {
        RvInt32 modulesCount = 0;
            for (modulesCount = 0; modulesCount < RVRTP_MODULES_NUMBER; modulesCount++)
                RvRtpSourceConstruct((RvRtpModule)modulesCount);
    }
/*
#if defined(RVRTP_SET_RTP_GLOBAL_MASK_DEFAULT)
	RvRtpSetLogModuleFilter((RvRtpLogMngr)&rvRtpInstance.LogManager, RVRTP_GLOBAL_MASK, RVRTP_GLOBAL_MASK_DEFAULT);
#endif
*/
	RvRtpSetLogModuleFilter(RVRTP_RTP_MODULE,      RVRTP_RTP_MODULE_MASK_DEFAULT);
	RvRtpSetLogModuleFilter(RVRTP_RTCP_MODULE,     RVRTP_RTCP_MODULE_MASK_DEFAULT);
	RvRtpSetLogModuleFilter(RVRTP_PAYLOAD_MODULE,  RVRTP_PAYLOAD_MODULE_MASK_DEFAULT);
	RvRtpSetLogModuleFilter(RVRTP_SECURITY_MODULE, RVRTP_SECURITY_MODULE_MASK_DEFAULT);	
    RvRtpSetLogModuleFilter(RVRTP_SRTP_MODULE,     RVRTP_SRTP_MODULE_MASK_DEFAULT);
	RvRtpSetLogModuleFilter(RVRTP_RTCP_XR_MODULE,  RVRTP_RTCP_XR_MODULE_MASK_DEFAULT);
	RvRtpSetLogModuleFilter(RVRTP_FEC_MODULE,      RVRTP_FEC_MODULE_MASK_DEFAULT);
	RvRtpSetLogModuleFilter(RVRTP_RTCP_FB_MODULE,  RVRTP_RTCP_FB_MODULE_MASK_DEFAULT);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetExternalLogManager"));
	return RV_OK;
}

#if !defined(RVRTP_INTERNAL_LOGGER)
static void RVCALLCONV LogPrintCallbackFunction(
	IN RvLogRecord* pLogRecord,
	IN void*        context);
#endif
/************************************************************************************
 * RvRtpCreateLogManager
 * description:  creates and executes internal log - manager
 *               and after it sets for all modules logger masks to default masks
 * output: logMgrPtr.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: 1) RV_LOGLISTENER_TYPE must be difined in common core
 *       2) RV_RTP_LOGFILE_NAME must be definef in rvrtpconfig.h
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV  RvRtpCreateLogManager(OUT RvRtpLogger* loggerPtr)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpCreateLogManager"));	
    if (NULL==rtpLogManagerPtr)
	{
		RvLogConstruct(&rvRtpInstance.logManager.logMngr);
		logMgr = &rvRtpInstance.logManager.logMngr;
		rtpLogManagerPtr = &rvRtpInstance.logManager;
	    rvRtpInstance.logManager.bExternal = RV_FALSE;
        *loggerPtr= (RvRtpLogger) logMgr;
	    rvRtpInstance.logManager.printCB = NULL; /* can be set if not defined RVRTP_INTERNAL_LOGGER */
#if defined(RVRTP_INTERNAL_LOGGER)
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_TERMINAL)
		RvLogListenerConstructTerminal(&rvRtpInstance.logManager.listener, logMgr, RV_TRUE);
#elif (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)
		RvLogListenerConstructLogfile(&rvRtpInstance.logManager.listener, logMgr, RV_RTP_LOGFILE_NAME, 1, 0, RV_TRUE);
#endif
#else /* undefined RVRTP_INTERNAL_LOGGER */
		/*register the application callback to the common core*/
        RvLogRegisterListener(logMgr, LogPrintCallbackFunction,
			(void*)&rvRtpInstance.logManager);
#endif
        {
            RvInt32 modulesCount = 0;
            for (modulesCount = 0; modulesCount < RVRTP_MODULES_NUMBER; modulesCount++)
                RvRtpSourceConstruct((RvRtpModule)modulesCount);
        }

#if defined(RVRTP_SET_RTP_GLOBAL_MASK_DEFAULT)
		RvRtpSetLogModuleFilter(RVRTP_GLOBAL_MASK,      RVRTP_GLOBAL_MASK_DEFAULT);
#endif		
		RvRtpSetLogModuleFilter( RVRTP_RTP_MODULE,      RVRTP_RTP_MODULE_MASK_DEFAULT);
		RvRtpSetLogModuleFilter( RVRTP_RTCP_MODULE,     RVRTP_RTCP_MODULE_MASK_DEFAULT);
		RvRtpSetLogModuleFilter( RVRTP_PAYLOAD_MODULE,  RVRTP_PAYLOAD_MODULE_MASK_DEFAULT);
		RvRtpSetLogModuleFilter( RVRTP_SECURITY_MODULE, RVRTP_SECURITY_MODULE_MASK_DEFAULT);
		RvRtpSetLogModuleFilter( RVRTP_SRTP_MODULE,     RVRTP_SRTP_MODULE_MASK_DEFAULT);
	}
	else
        *loggerPtr= (RvRtpLogger) logMgr;

	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpCreateLogManager"));	
	return RV_OK;
}

/************************************************************************************
 * RvRtpDestructLogManager
 * description:  destract log- manager
 * input: logMgrPtr.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 * NOTE: 1) must be created by RvRtpCreateLogManager or used external
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV  RvRtpDestructLogManager(IN RvRtpLogger* loggerPtr)
{
/*	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpDestructLogManager"));*/
	
    if (NULL==rtpLogManagerPtr)
		return RV_OK; /* already destructed */

	if (NULL==loggerPtr||logMgr!=(RvLogMgr*)(*loggerPtr))
	{
/*		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpDestructLogManager"));
	    RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpDestructLogManager"));*/
        return RV_ERROR_BADPARAM;
	}

    {
        RvInt32 modulesCount = 0;
        for (modulesCount = 0; modulesCount < RVRTP_MODULES_NUMBER; modulesCount++)
            RvRtpLogSourceDestruct((RvRtpModule)modulesCount);
    }

	if (!rvRtpInstance.logManager.bExternal)
		RvLogDestruct(&rvRtpInstance.logManager.logMngr);
	logMgr = NULL;
	rtpLogManagerPtr = NULL;
	return RV_OK;
}


RVAPI
RvStatus RVCALLCONV RvRtpSetLogModuleFilter( 
		IN RvRtpModule module,
		IN RvRtpLoggerFilter filter)
{
	RvLogSource* logSourcePtr = NULL;
	RvLogMessageType Mask = 0;
	RvBool bGlobal = RV_FALSE;
	RvStatus res = RV_OK;
	
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetLogModuleFilter"));
    if ( NULL==rtpLogManagerPtr) /* destructed manager */
	{
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetLogModuleFilter: NULL pointer"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetLogModuleFilter"));
		return RV_ERROR_NULLPTR;
	}

	switch(module)
	{
	case RVRTP_RTP_MODULE:
	case RVRTP_RTCP_MODULE:
	case RVRTP_PAYLOAD_MODULE:
	case RVRTP_SECURITY_MODULE:
    case RVRTP_SRTP_MODULE:
    case RVRTP_RTCP_XR_MODULE:
    case RVRTP_FEC_MODULE:
    case RVRTP_RTCP_FB_MODULE:
		logSourcePtr = &rtpLogManagerPtr->logSource[module];
		break;
	case RVRTP_GLOBAL_MASK:
		bGlobal = RV_TRUE;
		break;
	default:
		RvLogError(rvLogPtr, (rvLogPtr, "RvRtpSetLogModuleFilter: wrong RvRtpModule parameter"));
		RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetLogModuleFilter"));	
		return RV_ERROR_BADPARAM;
	}

	if (filter & RVRTP_LOG_DEBUG_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_DEBUG);
	if (filter & RVRTP_LOG_INFO_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_INFO);
	if (filter & RVRTP_LOG_WARN_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_WARNING);
	if (filter & RVRTP_LOG_ERROR_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_ERROR);
	if (filter & RVRTP_LOG_EXCEP_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_EXCEP);
	if (filter & RVRTP_LOG_LOCKDBG_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_SYNC);
	if (filter & RVRTP_LOG_ENTER_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_ENTER);
	if (filter & RVRTP_LOG_LEAVE_FILTER)
		 Mask = (RvUint8)(Mask|RV_LOGLEVEL_LEAVE);
	
    if (bGlobal)
		res = RvLogSetGlobalMask(logMgr, Mask);
	else
		res = RvLogSourceSetMask(logSourcePtr, Mask);
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetLogModuleFilter"));	
	return res;
}

/************************************************************************************
 * RvRtpSetPrintEntryCallback
 * description:  sets user defined callback function for entry printing
 * input: printEntryCB - user defined callback function.
 *        context - user-context
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/ 
RVAPI
RvStatus RVCALLCONV RvRtpSetPrintEntryCallback( 
		IN RvRtpPrintLogEntry_CB printEntryCB,
		IN void* context)
{
	RvLogEnter(rvLogPtr, (rvLogPtr, "RvRtpSetPrintEntryCallback"));	
	if (NULL==rtpLogManagerPtr)   /* destructed log manager */
		return RV_ERROR_UNKNOWN; 
	rtpLogManagerPtr->printCB = printEntryCB;
	rtpLogManagerPtr->logContext = context;
	RvLogLeave(rvLogPtr, (rvLogPtr, "RvRtpSetPrintEntryCallback"));	
	return RV_OK;
}
/* == ENDS: Accessory RTP Functions == */
/* rtpGetLogManager used for accessing to rtpLogManagerPtr */
RvStatus rtpGetLogManager(OUT RvRtpLogManager** lmngrPtr)
{
	if (NULL==lmngrPtr)
		return RV_ERROR_NULLPTR;
	*lmngrPtr = rtpLogManagerPtr;
	return RV_OK;
}

/************************************************************************************
* LogPrintCallbackFunction
* ----------------------------------------------------------------------------------
* General: This function will be registered to the log module as the
*          function that prints to the log. It will call the application
*          print function.
* Return Value: void
* ----------------------------------------------------------------------------------
* Arguments:
* Input:   logRecord - log record received from the common core.
*          context   - log context.
***********************************************************************************/
RVCOREAPI
RvChar* RVCALLCONV logFormatMessage(
        IN  RvLogRecord*    logRecord,
        OUT RvUint32*       size);

#if !defined(RVRTP_INTERNAL_LOGGER)
static void RVCALLCONV LogPrintCallbackFunction(
		IN RvLogRecord* pLogRecord,
		IN void*        context)
{
	RvRtpLogManager* rtpLogMngrPtr  =  (RvRtpLogManager*)context;
    RvChar          *strFinalText = logFormatMessage(pLogRecord, NULL);
	RvLogMessageType type = 0;
    /*call the application print function*/
    if((NULL != rtpLogMngrPtr) && (NULL != rtpLogMngrPtr->printCB))
    {
		switch (pLogRecord->messageType)
		{
		case RV_LOGID_DEBUG:       type = RVRTP_LOG_DEBUG_FILTER;   break;
		case RV_LOGID_INFO:        type = RVRTP_LOG_INFO_FILTER;    break;
		case RV_LOGID_WARNING:     type = RVRTP_LOG_WARN_FILTER;    break;
		case RV_LOGID_ERROR:       type = RVRTP_LOG_ERROR_FILTER;   break;
		case RV_LOGID_EXCEP:       type = RVRTP_LOG_EXCEP_FILTER;   break;
		case RV_LOGID_SYNC:        type = RVRTP_LOG_LOCKDBG_FILTER; break;
		case RV_LOGID_ENTER:       type = RVRTP_LOG_ENTER_FILTER;   break;
		case RV_LOGID_LEAVE:       type = RVRTP_LOG_LEAVE_FILTER;   break;
		}
        rtpLogMngrPtr->printCB(rtpLogMngrPtr->logContext,	type, strFinalText);
    }
}
#endif

#else

int prevent_warning_of_ranlib_has_no_symbols_artp_rvrtplogger=0;

#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */

#ifdef __cplusplus
}
#endif



