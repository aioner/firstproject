/******************************************************************************
Filename    :rvsdpinit.c
Description :init the sdp library.

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
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include <string.h>
#include "rvsdpprivate.h"
#include "rvcbase.h"

#if defined(RV_SDP_ADS_IS_USED)
#include "AdsRpool.h"
#endif

#define SDP_LOG_SOURCE_NAME_LEN (13)
#define SDP_LOG_MSG_PREFIX      (24)

static void RVCALLCONV CommonCorePrintFunction(
                                               IN RvLogRecord* logRecord,
                                               IN void*        context);

/***************************************************************************
 * RvSdpMgrConstructWithConfig
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs an SDP library and defines its logging behavior. This function 
 *      has to be called prior to any other SDP API call.
 *      There are three different ways to define the logging behavior of the
 *      SDP library.
 *      1. pStackConfig is NULL. This is the default logging behavior.
 *         The SDP will create log file named as defined by RV_SDP_LOG_FILE_NAME 
 *         (defined in rvsdpconfig.h). This file will be used for parsing error
 *         messages.
 *      2. pStackConfig is not NULL and pStackConfig->logManagerPtr is set to log
 *         handle of some other RV module. In this case the log messages produced
 *         by SDP module will be printed by other RV module.
 *         When SDP is used with SIP TK use RvSipStackGetLogHandle SIP API function
 *         to get SIP module's log handle.
 *      3. pStackConfig is not NULL and pStackConfig->pfnPrintLogEntryEvHandler is set.
 *         In this case the application supplied callback (pfnPrintLogEntryEvHandler)
 *         will be called each time the log message has to be printed. The 
 *         pfnPrintLogEntryEvHandler will be called with logContext as a function 
 *         argument.
 *          
 * Return Value: 
 *      Returns RV_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          pStackCfg - Structure containing SDP Stack configuration parameters.
 *          sizeOfCfg - The size of the configuration structure.
 ***************************************************************************/
RvStatus RvSdpMgrConstructWithConfig(RvSdpStackCfg *pStackConfig, 
                                     RvUint32     sizeOfCfg)
{ 
    RvStatus crv;
    RvSdpGlobals *__globals__;
    RV_UNUSED_ARG(sizeOfCfg);
    RV_UNUSED_ARG(pStackConfig);
    
    crv = RvCBaseInit(); 
    if (crv != RV_OK)
    {
        RvCBaseEnd();
        return RV_ERROR_UNKNOWN;
    }
    if (RvCreateGlobalData(RV_SDP_GLOBALS_INDEX,RvSdpInitializeGlobals,
                                        NULL,RvSdpDestroyGlobals) != RV_OK)
    {
        RvCBaseEnd();
        return RV_ERROR_UNKNOWN;
    }    
    
    __globals__ = (RvSdpGlobals *)RvGetGlobalDataPtr(RV_SDP_GLOBALS_INDEX);    
    if (__globals__ == NULL)
    {
        RvDestroyGlobalData(RV_SDP_GLOBALS_INDEX);
        RvCBaseEnd();
        return RV_ERROR_UNKNOWN;        
    }
    
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    /*-----------------------------------------------------------------------------
    Init the log handle of the SDP module
    -------------------------------------------------------------------------------*/
    if (RV_OK != RvMemoryAlloc(NULL, sizeof(sdpGlobl), NULL, (void**)&g_sdpGlobal))
        return RV_ERROR_OUTOFRESOURCES;
    
    memset(g_sdpGlobal,0,sizeof(sdpGlobl));
    
    if ((pStackConfig == NULL) ||
        ((pStackConfig != NULL) && (pStackConfig->logManagerPtr == NULL)))
    {	
        crv = RvLogConstruct(&g_sdpGlobal->logMgr);
        if(crv != RV_OK)
        {
			//RvMemoryFree must call
            RvDestroyGlobalData(RV_SDP_GLOBALS_INDEX);
            RvCBaseEnd();
            return RV_ERROR_UNKNOWN;
        }
        g_sdpGlobal->pLogMgr = &g_sdpGlobal->logMgr;
        RvLogPrintThreadId(g_sdpGlobal->pLogMgr);

/*lint -e613 */
        if (pStackConfig && pStackConfig->disableSdpLogs)
/*lint +e613 */            
        {
            RvLogSetLevel(g_sdpGlobal->pLogMgr, 0);
        }
        else
        {
            RvLogSetLevel(g_sdpGlobal->pLogMgr, 1);
            RvLogSetGlobalMask(g_sdpGlobal->pLogMgr, RV_LOGLEVEL_ALL);
        
            /*initializing the default log incase the application did not register
            a callback function*/
            if((pStackConfig == NULL) ||
                ((pStackConfig != NULL) && 
                 (pStackConfig->pfnPrintLogEntryEvHandler == NULL)))
            {
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_TERMINAL)
                crv = RvLogListenerConstructTerminal(&g_sdpGlobal->defaultLogFileListener, 
                                    g_sdpGlobal->pLogMgr, RV_TRUE);
#elif (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_NONE)
                crv = RvLogListenerConstructLogfile(&g_sdpGlobal->defaultLogFileListener, 
                                    g_sdpGlobal->pLogMgr, RV_SDP_LOG_FILE_NAME, 1, 0, RV_FALSE);
                if(crv == RV_OK)
                {
                    g_sdpGlobal->bFileListenerInitialized = RV_TRUE;
                }
#endif
            }
            else
            {
                /*register the application callback to the common core*/
                g_sdpGlobal->stackCfg.logContext = pStackConfig->logContext;
                g_sdpGlobal->stackCfg.pfnPrintLogEntryEvHandler = 
                    pStackConfig->pfnPrintLogEntryEvHandler;
                crv = RvLogRegisterListener(g_sdpGlobal->pLogMgr, 
                    CommonCorePrintFunction, (void*)&(g_sdpGlobal->stackCfg));
                if(crv == RV_OK)
                {
                    g_sdpGlobal->bAppListenerInitialized = RV_TRUE;
                }
            }
            if(crv != RV_OK)
            {
                RvLogDestruct(&g_sdpGlobal->logMgr);
                RvMemoryFree(g_sdpGlobal,NULL);
                g_sdpGlobal = NULL;
                RvDestroyGlobalData(RV_SDP_GLOBALS_INDEX);
                RvCBaseEnd();
                return RV_ERROR_UNKNOWN;
            }
        }
    }
    else
    {
        g_sdpGlobal->pLogMgr = (RvLogMgr*)pStackConfig->logManagerPtr;
        g_sdpGlobal->bLogMgrSuppliedByApp = RV_TRUE;
    }
    crv = RvLogSourceConstruct(g_sdpGlobal->pLogMgr, &g_sdpGlobal->logSource, 
        "SDP", "SDP module");
    if(crv != RV_OK)
    {
        RvLogDestruct(g_sdpGlobal->pLogMgr);
        RvMemoryFree(g_sdpGlobal, NULL);
        g_sdpGlobal = NULL;
        RvDestroyGlobalData(RV_SDP_GLOBALS_INDEX);
        RvCBaseEnd();
        return RV_ERROR_UNKNOWN;
    }
    pSdpLogSource = &g_sdpGlobal->logSource;
    
    RvLogSourcePrintInterfacesData(pSdpLogSource, RvCCoreInterfaces());
    
#endif /*#if (RV_LOGMASK != RV_LOGLEVEL_NONE)*/

    rvSdpAllocInitialize();    
    
    RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
	        "SDP manager was constructed successfully - %s",RV_SDPSTACK_VERSION));

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
	RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
		"		RV_SDP_CHECK_BAD_SYNTAX is set"));	
#else
	RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
		"		RV_SDP_CHECK_BAD_SYNTAX is not set"));	
#endif

#if defined(RV_SDP_ADS_IS_USED)
	RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
		"		RV_SDP_ADS_IS_USED is set"));	
#else
	RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
		"		RV_SDP_ADS_IS_USED is not set"));	
#endif

#if defined(RV_SDP_USE_MACROS)
	RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
		"		RV_SDP_USE_MACROS is set"));	
#else
	RvLogInfo(&g_sdpGlobal->logSource,(&g_sdpGlobal->logSource,
		"		RV_SDP_USE_MACROS is not set"));	
#endif
    
    return RV_OK;
}

/***************************************************************************
 * RvSdpMgrConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs an SDP library with default logging behavior. This function 
 *      has to be called prior to any other SDP API call. See 
 *      RvSdpMgrConstructWithConfig for default logging behavior description.
 *      
 * Return Value: 
 *      Returns RV_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      None.
 ***************************************************************************/
RvStatus RvSdpMgrConstruct(void)
{
    return RvSdpMgrConstructWithConfig(NULL,0);
}


#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
/************************************************************************************
* CommonCorePrintFunction
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
static void RVCALLCONV CommonCorePrintFunction(
                                               IN RvLogRecord* logRecord,
                                               IN void*        context)
{
    
    RvSdpStackCfg *pStackCfg = (RvSdpStackCfg*)context;
    RvSdpLogFilters filter = RVSDP_LOG_DEBUG_FILTER;
    RvChar         *strFinalText;
    RvChar         *ptr;
    const RvChar   *strSourceName =  RvLogSourceGetName(logRecord->source);
    const RvChar   *mtypeStr;
    
    
    /* Find the message type string */
    switch (RvLogRecordGetMessageType(logRecord))
    {
    case RV_LOGID_EXCEP:
        filter = RVSDP_LOG_EXCEP_FILTER;
        mtypeStr = "EXCEP  - ";
        break;
    case RV_LOGID_ERROR:
        filter = RVSDP_LOG_ERROR_FILTER;
        mtypeStr = "ERROR  - ";
        break;
    case RV_LOGID_WARNING:
        filter = RVSDP_LOG_WARN_FILTER;
        mtypeStr = "WARN   - ";
        break;
    case RV_LOGID_INFO :
        filter = RVSDP_LOG_INFO_FILTER;
        mtypeStr = "INFO   - ";
        break;
    case RV_LOGID_DEBUG:
        mtypeStr = "DEBUG  - ";
        filter  = RVSDP_LOG_DEBUG_FILTER;
        break;
    case RV_LOGID_ENTER:
        filter = RVSDP_LOG_ENTER_FILTER;
        mtypeStr = "ENTER  - ";
        break;
    case RV_LOGID_LEAVE:
        filter = RVSDP_LOG_LEAVE_FILTER;
        mtypeStr = "LEAVE  - ";
        break;
    case RV_LOGID_SYNC:
        filter = RVSDP_LOG_SYNC_FILTER;
        mtypeStr = "SYNC   - ";
        break;
    default:
        mtypeStr = "NOT_VALID"; break;
    }
    
    ptr = (char *)logRecord->text - SDP_LOG_MSG_PREFIX;
    strFinalText = ptr;
    
    /* Format the message type */
    /*ptr = (char *)strFinalText;*/
    strcpy(ptr, mtypeStr);
    ptr += strlen(ptr);
    
    /* Pad the module name */
    memset(ptr, ' ', SDP_LOG_SOURCE_NAME_LEN+2);
    
    memcpy(ptr, strSourceName, strlen(strSourceName));
    ptr += SDP_LOG_SOURCE_NAME_LEN;
    *ptr = '-'; ptr++;
    *ptr = ' '; ptr++;
    
    /*call the application print function*/
    if(pStackCfg != NULL)
    {
        pStackCfg->pfnPrintLogEntryEvHandler(pStackCfg->logContext,filter,strFinalText);
    }
    
}
#endif /*#if (RV_LOGMASK != RV_LOGLEVEL_NONE)*/


/***************************************************************************
 * RvSdpMgrDestruct
 * ------------------------------------------------------------------------
 * General: 
 *    Destruct the sdp library. No SDP API function can be called after the 
 *    library was destructed.
 * Return Value: 
 *    None.
 * ------------------------------------------------------------------------
 * Arguments:
 *    None.
***************************************************************************/
void RvSdpMgrDestruct(void)
{
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RV_SDP_USE_GLOBALS;
    if(g_sdpGlobal != NULL)
    {
        RvLogSourceDestruct(&g_sdpGlobal->logSource);
        /*destruct the log manager only if it was not supplied by the application*/
        if(g_sdpGlobal->bLogMgrSuppliedByApp == RV_FALSE)
        {
#if (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_TERMINAL) && (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_NONE)
            if(g_sdpGlobal->bFileListenerInitialized == RV_TRUE)
            {
                RvLogListenerDestructLogfile(&g_sdpGlobal->defaultLogFileListener);
                
            }
#endif
            if(g_sdpGlobal->bAppListenerInitialized == RV_TRUE)
            {
                RvLogUnregisterListener(&g_sdpGlobal->logMgr,
                    CommonCorePrintFunction);
            }
            RvLogDestruct(&g_sdpGlobal->logMgr);
        }
        RvMemoryFree(g_sdpGlobal,NULL);
        g_sdpGlobal = NULL;
    }
#endif /*#if (RV_LOGMASK != RV_LOGLEVEL_NONE)*/

    rvSdpAllocEnd();  
        
    RvDestroyGlobalData(RV_SDP_GLOBALS_INDEX);
    RvCBaseEnd();
}


#if defined(RV_SDP_ADS_IS_USED)

/***************************************************************************
 * RvSdpRPoolAlloc
 * ------------------------------------------------------------------------
 * General: 
 *          This function is used from the sdp library to allocated.
 *          It is been stored in the allocator of every sdp object.
 *          The function "rvAllocAllocate" will operate it when we will need
 *          new space.
 * Return Value: 
 *               NULL - if there is not enough space.
 *               Pointer to the allocated space.
 * ------------------------------------------------------------------------
 * Arguments:
 *  rpoolPtr    - RPOOL_Ptr structure which holds the pool, page and offset.
 *  SizeToAlloc - The size that need to be allocate.
 ***************************************************************************/
void *RvSdpRPoolAlloc(IN void   *rpoolPtr,IN RvSize_t SizeToAlloc)
{
    RPOOL_Ptr *ptrPool = (RPOOL_Ptr *)rpoolPtr;
    
    return (RvChar*)RPOOL_AlignAppend(ptrPool->hPool,
        ptrPool->hPage,
        SizeToAlloc,
        &ptrPool->offset);
    
    
}

/***************************************************************************
 * RvSdpRPoolDealloc
 * ------------------------------------------------------------------------
 * General: 
 *          This function is used from the sdp library to deallocated.
 *          It is been stored in the allocator of every sdp object.
 *          The function "rvAllocDeallocate" will operate it when we will need
 *          to free space.
 *          The function acturally doesn't do anythings because rpool frees
 *          the memory just when a page is freed.
 * Return Value: 
 *          None.
 * ------------------------------------------------------------------------
 * Arguments:
 *  rpoolPtr      - RPOOL_Ptr structure which holds the pool, page and offset.
 *  SizeToDealloc - The size that need to be deallocate.
 *  NullPtr       - null pointer.
 ***************************************************************************/
void RvSdpRPoolDealloc (void * rpoolPtr, RvSize_t SizeToDealloc, void* NullPtr)
{
    RV_UNUSED_ARG(rpoolPtr);
    RV_UNUSED_ARG(SizeToDealloc);
    RV_UNUSED_ARG(NullPtr);
}


/***************************************************************************
 * RvSdpAllocConstruct
 * ------------------------------------------------------------------------
 * General: 
 *          Constructs an SDP allocator. An SDP allocator is used whenever 
 *          you need to allocate space from RPOOL.
 * Return Value: 
 *      Returns RV_OK if the function succeeds, or an error code if the 
 *      function fails.
 *      This function is defined only if the RV_SDP_ADS_IS_USED 
 *      compilation switch is enabled.
 * ------------------------------------------------------------------------
 * Arguments:
 *  hPool    - the pool which the SDP library will use.
 *  sdpAlloc - a pointer to the initialized allocator. Must point to valid memory.
 ***************************************************************************/
RvStatus RvSdpAllocConstruct(HRPOOL hPool, RvAlloc * sdpAlloc)
{
    HPAGE hPage;
    RvStatus eStat;
    RPOOL_Ptr *ptrPool;
    RvInt32  offset = -1;
    RV_SDP_USE_GLOBALS;    
    
    eStat = RPOOL_GetPage(hPool, 0,&hPage);
    if (RV_OK != eStat)
    {
        RvLogError(pSdpLogSource,(pSdpLogSource,
            "RvSdpAllocConstruct - error in RPOOL_GetPage"));
        return eStat;
    }
    
    eStat = RPOOL_Append(hPool, hPage, sizeof(RPOOL_Ptr),RV_TRUE, &offset);
    
    if (RV_OK != eStat)
    {
        RvLogError(pSdpLogSource,(pSdpLogSource,
            "RvSdpAllocConstruct - Failed to append memory. size %d. hPool %x, hPage %d",
            hPool, hPage, offset));
        return eStat;
    }
    
    ptrPool = (RPOOL_Ptr *)RPOOL_GetPtr(hPool, hPage, offset);
    ptrPool->hPool = hPool;
    ptrPool->hPage = hPage;
    ptrPool->offset = offset;
    
    rvAllocConstruct(sdpAlloc,
        ptrPool,
        ~0U,
        RvSdpRPoolAlloc,
        RvSdpRPoolDealloc);
    
    return RV_OK;
    
}

 /***************************************************************************
 * RvSdpAllocDestruct
 * ------------------------------------------------------------------------
 * General: 
 *          Destroys an SDP allocator. This function is called after an SDP 
 *          message is destroyed.
 *          This function is defined only if the RV_SDP_ADS_IS_USED 
 *          compilation switch is enabled.
 * Return Value: 
 *          None.
 * ------------------------------------------------------------------------
 * Arguments:
 *  sdpAlloc - an sdp allocator of the message.
 ***************************************************************************/
void RvSdpAllocDestruct(RvAlloc * sdpAlloc)
{
    RPOOL_FreePage(((RPOOL_Ptr*)sdpAlloc->pool)->hPool,
        ((RPOOL_Ptr*)sdpAlloc->pool)->hPage);
}

#endif /*RV_SDP_ADS_IS_USED*/

/* gets default SDP allocator */
RvAlloc* rvSdpGetDefaultAllocator()
{
/*
    RV_SDP_USE_GLOBALS;
    return gSdpDefaultAlloc;
*/
    return rvAllocGetDefaultAllocator();
}

