/*
 ********************************************************************************
 *                                                                              *
 * NOTICE:                                                                      *
 * This document contains information that is confidential and proprietary to   *
 * RADVision LTD.. No part of this publication may be reproduced in any form    *
 * whatsoever without written prior approval by RADVision LTD..                 *
 *                                                                              *
 * RADVision LTD. reserves the right to revise this publication and make changes*
 * without obligation to notify any person of such revisions or changes.        *
 ********************************************************************************
*/

/********************************************************************************
 *                              <RvRtspServer.c>                                
 *                                                                              
 * Interface functions for the entire RTSP library. The RvRtsp module is        
 * responsible for initializing and de-initializing the library, and holds
 * the RTSP sessions and connections on the server.                                           
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "rvtypes.h"
#include "rvstdio.h"
#include "rvcbase.h"
#include "rvansi.h"
#include "rvmemory.h"

#include "RvRtspServer.h"
#include "RtspObject.h"
#include "RtspUtilsInternal.h"
#include "RtspServerConnectionInternal.h"
#include "RtspServerSession.h"
#include "RtspMsgInternal.h"

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

#define MAJOR_VERSION          2  /* The RTSP lib's major version number.        */
#define MINOR_VERSION          0  /* The RTSP lib's minor version number.        */
#define ENGINEERING_RELEASE    0  /* The RTSP stack's engineering release number.*/
#define PATCH_LEVEL            0  /* The RTSP stack's patch level.               */

/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

/************************************************************************
 * RvRtspServerInit                                                     
 * --------------------------------------------------------------------- 
 * General:
 *  This API initializes the RvRtsp module. This API constructs the 
 *  memory RPool and the Select Engine. It creates the log manager and 
 *  global and guard mutexes. It also initializes the listening port, 
 *  connection, session, request message and response message structures.
 *  This API must be called before any APIs or functions of the 
 *  library are used. It should only be called once.                     
 * 
 * Arguments:                                                           
 * Input:  hLog              - The RTSP module log handle, NULL if one     
 *                             should be created.                          
 *         pConfiguration    - Configuration for the module.Must be 
 *                             populated correctly by the application.               
 *         configurationSize - Size of configuration structure is passed   
 *                             in order to distinguish between different   
 *                             versions of the RTSP Stack.   
 * Output: phRtsp            - Handle to the Rtsp main structure. 
 *         
 * Return Value: RV_OK - If the RTSP Library is successfully initialized 
 *               Negative values otherwise.            
 ***********************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerInit(
    IN RvLogHandle                     hLog,
    IN const RvRtspServerConfiguration *pConfiguration,
    IN const RvSize_t                  configurationSize,
    OUT RvRtspHandle                   *phRtsp)
{
    RvRtsp                    *pThis;
    RvStatus                  result;
    RvRtspServerConfiguration *configuration;
    RvSize_t            requestObjSize = sizeof(RvRtspRequest);
    RvSize_t            responseObjSize = sizeof(RvRtspResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvUint32            maxRtspMsgHeaders = 0;
#endif
    
    result = RvCBaseInit();

    if(result !=RV_OK)
    {
        return result;
    }
    
    /* Allocate memory for the RTSP stack */
    RvMemoryAlloc(NULL,sizeof(RvRtsp),NULL,(void**)&pThis);
    if(pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    memset(pThis,0 , sizeof(RvRtsp));

    *phRtsp = (RvRtspHandle)pThis;

    if(sizeof(RvRtspServerConfiguration)  != configurationSize)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    
    memset(pThis,0 , sizeof(RvRtsp));    

    /* Allocate space for the stack configuration */
    RvMemoryAlloc(NULL, sizeof(RvRtspServerConfiguration), NULL, (void**)&configuration);
    
    if (configuration == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
     
    memset(configuration, 0, sizeof(RvRtspServerConfiguration));
    
    pThis->configuration = (RvRtspConfigurationHandle)configuration;

    /* Rtsp module configuration initialization */
    memcpy(configuration, pConfiguration, 
                 sizeof(RvRtspServerConfiguration));

    /* Create log source */
    if (hLog == NULL)
    {
#if RV_LOGMASK != RV_LOGLEVEL_NONE
         RvMemoryAlloc(NULL, sizeof(RvLogMgr), NULL, (void**)&pThis->pLogMgr);
         RvLogConstruct(pThis->pLogMgr);
         RvLogSetGlobalMask(pThis->pLogMgr, configuration->messageMask);
#ifndef _ARM_PLATFORM
         RvLogListenerConstructLogfile(&pThis->logListener, pThis->pLogMgr,
                                       "rtspServerStack.txt",1,0,RV_TRUE);
#endif
         pThis->bLogCreated = RV_TRUE;
#endif
    }
    else
    {
         pThis->pLogMgr = (RvLogMgr*)hLog;
         pThis->bLogCreated = RV_FALSE;
    }

    if (pThis->pLogMgr != NULL)
    {
         RvLogSourceConstruct(pThis->pLogMgr, &pThis->logSrc, "RTSP",
                              "Rtsp Server Stack");
    }

    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerInit\r\n"));

    /* Create guard Mutex */
    result = RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);
    if (result != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc,
                   "RvRtspServerInit - Mutex construction failed\r\n"));
        return result;
    }

    /* Create global Mutex */
    result = RvMutexConstruct(pThis->pLogMgr, &pThis->gMutex);
    if (result != RV_OK)
    {
        RvLogError(&pThis->logSrc,(&pThis->logSrc,
                   "RvRtspServerInit - GMutex construction failed\r\n"));
        return result;
    }

    /* Constructing the memory RPool */
    /* Note: Thread safety is needed because hrPool is used across different  */
    /* structures */
    pThis->hRPool = rpoolConstruct(
                     configuration->memoryElementsSize, /* Elements size*/
                     configuration->memoryElementsNumber, 
                                                   /* Max number of elements  */
                      RV_TRUE,                     /* ThreadSafety            */
                      "RtspServer",                /* Name                    */
                      pThis->pLogMgr);             /* Log manager             */
    
    if (pThis->hRPool == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    } 
    
    /* Construct select engine */ 
    result = RvSelectConstruct(
                       4096,                      /* Hash size for the engine */
                       configuration->maxListeningPorts,  
                       pThis->pLogMgr,
                       &pThis->pSelectEngine);
    
    if (result != RV_OK)
    {
        return result; 
    }
    
    RvLogDebug(&pThis->logSrc, (&pThis->logSrc, 
             "RvRtspServerInit - Select Engine Constructed\r\n"));
    
    pThis->doTerminate = RV_FALSE;
    pThis->terminated = RV_FALSE;
   
    /* Initializing the listening ports structure   */
    pThis->hRaListeningPorts = raConstruct(
                      sizeof(RtspServerConnection), /*  Elements size  */
                      configuration->maxListeningPorts, 
                                      /* Max number of listening ports */ 
                      RV_TRUE,                      /* Thread safety   */
                      "Listening Ports",            /* RA name         */
                      pThis->pLogMgr);              /* Log Manager     */
  
    
    /* Initializing the connections structure   */
    pThis->hRaConnections = raConstruct(
                      sizeof(RtspServerConnection), /*  Elements size  */
                      configuration->maxConnections,
                                      /* Max number of Connections     */          
                      RV_TRUE,                      /* Thread safety   */
                      "Connections",                /* RA name         */
                      pThis->pLogMgr);              /* Log Manager     */

    
    /* Initializing the sessions structure   */
    pThis->hRaSessions = RtspServerSessionInit(
                         configuration->maxSessionsPerConnection *
                         configuration->maxConnections, /* max Sessions */
                         pThis->pLogMgr);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    requestObjSize = sizeof(RtspMsgRequest);
    responseObjSize = sizeof(RtspMsgResponse);
#endif 
    /* Initializing the request message structure  */
    pThis->hRaRequests = raConstruct(
                      requestObjSize, /*  Elements size          */
                      configuration->maxRequests,
                                      /* Max number of requests        */
                      RV_TRUE,                      /* Thread safety   */
                      "Requests",                   /* RA name         */
                      pThis->pLogMgr);              /* Log Manager     */

    /* Initializing the response message structure  */
    pThis->hRaResponses = raConstruct(
                      responseObjSize,              /*  Elements size         */
                      configuration->maxResponses,
                                      /* Max number of responses       */
                      RV_TRUE,                      /* Thread safety   */
                      "Responses",                  /* RA name         */
                      pThis->pLogMgr);              /* Log Manager     */

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    /* Construct the RtspMsgHeaders RA */
    maxRtspMsgHeaders = (configuration->memoryElementsNumber + 
        configuration->memoryElementsNumber) * configuration->maxRtspMsgHeadersInMessage;

    if (maxRtspMsgHeaders > 0)
    {
        pThis->hRaHeaders = raConstruct(sizeof(RtspMsgHeader),
                                        maxRtspMsgHeaders,
                                        RV_TRUE,
                                        "RtspMshHeaders",
                                        pThis->pLogMgr);
        if (pThis->hRaHeaders == NULL)
            return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
#endif                      

    /* Version information */
    RvLogDebug(&pThis->logSrc,( &pThis->logSrc,
              "RvRtspServerInit - RTSP Server Module Initialized, Version: %03u.%03u.%03u, patch = %u\r\n",
               MAJOR_VERSION, 
               MINOR_VERSION,
               ENGINEERING_RELEASE,
               PATCH_LEVEL));    
  
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspInitServer\r\n"));

    return RV_OK;
}

    
/***********************************************************************
 * RvRtspServerEnd                                                      
 * --------------------------------------------------------------------- 
 * General:
 *  This API de-initializes the RvRtsp module by turning the 
 *  doTerminate flag to true. This API should be called when 
 *  the RvRtsp module is no longer needed. After this call 
 *  is made none of the APIs or functions of this module should be used. 
 * 
 * Arguments:                                                           
 * Input:  hRtsp     - Handle to the Rtsp main structure which is used
 *                     to access the RvRtsp module 
 *                 
 * Output: None.    
 *                                                       
 * Return Value: RV_OK-If the RTSP Library is successfully uninitialized 
 *               Negative values otherwise.            
 ***********************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerEnd(
    IN RvRtspHandle hRtsp)
{
    RvStatus   status   =  RV_OK;
    RvRtsp     *pThis   =  (RvRtsp*)hRtsp;
   
    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerEnd\r\n"));

    /* Signal termination to the Main Loop */
    pThis->doTerminate = RV_TRUE;

    /* allow the main loop thread to terminate */
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspiServerEnd\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    RvCBaseEnd();

    return status;
}


/***********************************************************************
 * RvRtspServerMainLoop                                                 
 * --------------------------------------------------------------------- 
 * General: 
 *  This API runs in the same user thread that called the RTSP server 
 *  module's Init, it runs a loop waiting for events to dispatch callback
 *  functions for. The API returns to the user after 'timeout' 
 *  milliseconds and allows the user to perform user specific 
 *  functionality before calling the API again. This API also destructs
 *  all resources allocated for the stack on termination. Termination is 
 *  triggered by calling RvRtspServerEnd() API. 
 *                                                        
 * Arguments:                                                           
 * Input:  hRtsp                 - Handle to the Rtsp main structure.     
 *         timeout(milliseconds) - Time to wait on a selectEngine event.
 *                                 The select engine waits for a select
 *                                 event for timeout milliseconds in 
 *                                 absence of which it returns to the 
 *                                 user.        
 * Output: None.                                                             
 *        
 * Return Value:
 *            RV_OK - If the RTSP Library is successfully uninitialized. 
 *            RV_ERROR_UNINITIAIZED - When the stack is successfuly
 *            uninitialized. The error is returned only when the user
 *            signalled the stack to terminate itself by calling 
 *            RvRtspServerEnd.
 *            Negative value otherwise.
 ***********************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerMainLoop(
    IN RvRtspHandle hRtsp,
    IN RvUint32     timeOut)
{
    RvRtsp                    *pThis;
    RvInt32                   location;
    RtspServerConnection      *pConnection;
    RvBool                    doTerminate;
    RvRtspServerConfiguration *configuration;
 
    pThis = (RvRtsp*)hRtsp;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerMainLoop\r\n"));
    doTerminate = pThis->doTerminate;
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    if (doTerminate != RV_TRUE)
    {
        RvSelectWaitAndBlock(pThis->pSelectEngine, timeOut*RV_TIME64_NSECPERMSEC);

		RvTime tm;
		tm.sec = 0;
		tm.nsec = 1000000;
		RvThreadSleep(&tm, NULL);

		return RV_OK;
    }

    configuration = (RvRtspServerConfiguration *)pThis->configuration;

    /* In case the user called this function after calling the         */
    /* RvRtspServerEnd API which signaled the main loop to terminate   */

    
    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);

    pThis->terminated = RV_TRUE;

    /* Destruct all connections which were left undestructed                  */
    location = raGetNext(pThis->hRaConnections, -1);
    while (location >= 0)
    {
        pConnection = (RtspServerConnection*)raGet(pThis->hRaConnections,location);
        RvRtspServerConnectionDestruct((RvRtspServerConnectionHandle)pConnection,RV_TRUE);
        location = raGetNext(pThis->hRaConnections, location);
    }

    raClear(pThis->hRaConnections);
    raDestruct(pThis->hRaConnections);
    pThis->hRaConnections = NULL;

    /* Destruct all listening ports  */
    location = raGetNext(pThis->hRaListeningPorts, -1);
    while (location >= 0)
    {
        pConnection = (RtspServerConnection*)raGet(pThis->hRaListeningPorts,location);
        RvRtspServerStopListening((RvRtspServerConnectionHandle)pConnection);
        location = raGetNext(pThis->hRaListeningPorts, location);
    }

    raClear(pThis->hRaListeningPorts);
    raDestruct(pThis->hRaListeningPorts);
    pThis->hRaListeningPorts = NULL;

    raClear(pThis->hRaSessions);
    raDestruct(pThis->hRaSessions);
    pThis->hRaSessions = NULL;

    if (pThis->hRaRequests!= NULL)
    { 
        raClear(pThis->hRaRequests);
        raDestruct(pThis->hRaRequests);
        pThis->hRaRequests = NULL;
    }

    if (pThis->hRaResponses != NULL)
    { 
        raClear(pThis->hRaResponses);
        raDestruct(pThis->hRaResponses);
        pThis->hRaResponses = NULL;
    }

    /* Destruct the select engine */
    RvSelectDestruct(pThis->pSelectEngine, 
                     configuration->maxListeningPorts);
    pThis->pSelectEngine = NULL;

    /* Destruct the pool */
    rpoolDestruct(pThis->hRPool);
    pThis->hRPool = NULL;

    /* Destruct the guard Mutexes */
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->gMutex, pThis->pLogMgr);

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    /* Destruct the log source */
    RvLogSourceDestruct(&pThis->logSrc);
    if (pThis->bLogCreated)
    {
        RvLogListenerDestructLogfile(&pThis->logListener);
        RvLogDestruct(pThis->pLogMgr);
        RvMemoryFree((void*)pThis->pLogMgr, NULL);
    }
#endif
    RvMemoryFree((void *)pThis->configuration, NULL);
    RvMemoryFree((void*)pThis, NULL);

    return RvRtspErrorCode(RV_ERROR_UNINITIALIZED);
}

/**************************************************************************
 * RvRtspServerStartListening                                                     
 * ------------------------------------------------------------------------
 * General: 
 *  This API is called once the server is successfully configured
 *  and initialized. This API constructs the listening socket 
 *  and listens for incoming client connections on the listening sockets
 *  created. This API should be called only after calling the RvRtspInit()
 *  API.
 * Arguments:                                                           
 * Input:   hRtsp              - Handle to the Rtsp main structure.
 *          hApp               - Application context used to access
 *                               the connection on the application.
 *          listeningIPAddress - The IP address on which the Server 
 *                               listens for clients connections. The 
 *                               application is required to provide this
 *                               address to connect to the server.
 *          pConfiguration     - The connection configuration which must
 *                               be populated in the server application.
 *          configurationSize  - The size of the configuration structure.
 *                               It is used to identify between different
 *                               versions of the RTSP stack.
 * Output:  phConnection       - Handle to the listening connection. 
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerStartListening(
    IN RvRtspHandle                          hRtsp,
    IN RvRtspServerConnectionAppHandle       hApp,
    IN const RvChar                          *listeningIPAddress,
    IN const RvRtspServerListenConfiguration *pConfiguration,
    IN const RvSize_t                        configurationSize,
    OUT RvRtspServerConnectionHandle         *phConnection)
{
    RvRtsp                *pThis;
    RtspServerConnection  *pConnection;
    RvStatus               result;

    pThis         = (RvRtsp*)hRtsp;
    result        = RV_OK;
    *phConnection = NULL;

    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerStartListening\r\n"));
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);

    if(sizeof(RvRtspServerListenConfiguration)  != configurationSize)
    {
         return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    if (raAdd(pThis->hRaListeningPorts, (RAElement*)&pConnection) < 0)
    {
        /* RA seems to be out of memory */
        RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerStartListening - Out of resources\r\n"));
        RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerStartListening\r\n"));
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }

    memset(pConnection, 0, sizeof(RtspServerConnection));

    pConnection->hApp           = (RvRtspServerConnectionAppHandle)hApp;
    pConnection->pLogMgr        = pThis->pLogMgr;
    pConnection->pLogSrc        = &pThis->logSrc;
    pConnection->hRPool         = pThis->hRPool;
    pConnection->hRtsp          = hRtsp;
    pConnection->pGMutex        = &pThis->gMutex;
    pConnection->pSelectEngine  = pThis->pSelectEngine;
    pConnection->destructed     = RV_FALSE;

    memcpy(&pConnection->configuration, &pConfiguration->connConfiguration, sizeof(RvRtspServerConnectionConfiguration));

    strncpy(pConnection->sourceIPAddress, listeningIPAddress, sizeof(pConnection->sourceIPAddress) - 1);

    pConnection->sourceIPAddressValid = RV_TRUE;
    pConnection->clientPort     = pConfiguration->listeningPort;

    result = RtspServerConnectionConstruct(pConnection, RV_FALSE);

    pConnection->transport.context   = (void*)pConnection;

    *phConnection = (RvRtspServerConnectionHandle)pConnection;

    result = RtspServerConnectionStartListening(pConnection, listeningIPAddress, pConfiguration->listeningPort);

    if ( result != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc,
                   "RvRtspServerStartListening -  listen failed\n"));
        raDelete(pThis->hRaListeningPorts, (RAElement) pConnection);
        return result;
    }

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerStartListening\r\n"));
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);

    return RV_OK;
}

/***********************************************************************
 * RvRtspServerStopListening                                                     
 * --------------------------------------------------------------------- 
 * General: 
 *  This API is called by the application when the server is 
 *  de-initialized. It destructs the connection and then deletes
 *  the listening ports from the RA. 
 *
 * Arguments:                                                           
 * Input:    hConnection   -  Handle to the listening connection. It is 
 *                            to access the connection to be destructed.
 * Output:   None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 ***********************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerStopListening(
    IN RvRtspServerConnectionHandle         hConnection)
{
    RvRtsp                *pThis;
    RtspServerConnection  *pConnection;

    pConnection = (RtspServerConnection*)hConnection;

    if (pConnection == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pThis       = (RvRtsp*)pConnection->hRtsp;

    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerStopListening\r\n"));

    pConnection->transport.transportState = RTSP_TRANSPORT_STATE_DISCONNECTING;

    RtspServerConnectionDestruct(hConnection, RV_TRUE);

    /* Removing the connection from the RA connection                   */
    /* This is done once all sessions in the connection are destructed*/
    if (pConnection->referenceCount == 0)
    {
        raDelete(pThis->hRaListeningPorts, (RAElement) pConnection);
    }

    pConnection->destructed = RV_TRUE;

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerStopListening\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);

    return RV_OK;

}

/***********************************************************************
 * RvRtspServerConnectionConstruct                                      
 * ---------------------------------------------------------------------
 * General: 
 *  This API constructs an RTSP connection in hRtsp calling the actual 
 *  construct and adding the connection to the Rtsp module's connections.
 *  This API is called only when the TCP connection request has been 
 *  accepted by the server. It is called after the OnAcceptEv event
 *  has been generated.
 * 
 * Arguments:                                                           
 * Input: hRtsp              - Handle to the Rtsp main structure.
 *        hApp               - The application context handle used to 
 *                             access the rtsp connection.
 *        socket             - The newly socket created on which the 
 *                             server is listening for incoming requests
 *        clientIPAddress    - The client's IP address which is required
 *                             create the connection.
 *        clientPort         - The client port required to create the 
 *                             connection
 *        pConfiguration     - Configuration for the connection to be 
 *                             populated by the application.
 *        configurationSize  - Size of callbacks structure,
 *                             is passed in order to distinguish between
 *                             different versions of the RTSP Stack.
 * Output: phConnection      - Handle to the constructed connection. This 
 *                             handle is used to access the newly created
 *                             connection.
 *      
 * Return Value: RV_OK-If successful                                     
 *               Negative values otherwise.            
 ***********************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionConstruct(
    IN RvRtspHandle                        hRtsp,
    IN RvRtspServerConnectionAppHandle     hApp,
    IN RvSocket                            newSocket,
    IN const RvChar                        *clientIPAddress, 
    IN RvUint16                            clientPort,
    IN const RvRtspServerConnectionConfiguration *pConfiguration,
    IN const RvSize_t                     configurationSize,
    OUT RvRtspServerConnectionHandle       *phConnection)
{
    RvStatus                        result;
    RvRtsp                          *pThis;
    RtspServerConnection            *pConnection = NULL;
    RvBool                          reAttachConnection = RV_FALSE;

    pThis         = (RvRtsp*)hRtsp;
     
    if ((hRtsp           == NULL) || 
        (clientIPAddress == NULL) ||
        (pConfiguration  == NULL) ||
        (phConnection    == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (sizeof(RvRtspServerConnectionConfiguration) != configurationSize)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    
    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc,
                                "RvRtspServerConnectionConstruct\r\n"));


    if (*phConnection != NULL)
    {
        /* Re-establishing of a beoken connection */
        pConnection = (RtspServerConnection*)*phConnection;
        reAttachConnection = RV_TRUE;
        RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionConstruct - Re-Establishing Connection\r\n"));
        RtspTransportDestruct(&pConnection->transport);
    }

    if (reAttachConnection == RV_FALSE)
    {
        /* Find out if we already have a connection from the same IP and port */
        RvRtspServerGetConnectionByIPPort(hRtsp, clientIPAddress, clientPort, phConnection);
        
        /* If a connection with the same client exists the stack should only    */
        /* return the handle to the existing connection.                        */
        
        if (*phConnection != NULL)
        {
            /* Connection already exist */
            pConnection = (RtspServerConnection*)*phConnection;
            pConnection->referenceCount++;
            RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionConstruct - already exists\r\n"));
            RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionConstruct\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
            return RV_OK;
        }
        
        /* Creation of a new connection */
        if (raAdd(pThis->hRaConnections, (RAElement*)&pConnection) < 0)
        {
            /* RA seems to be out of memory */
            RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionConstruct - Out of resources\r\n"));
            RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionConstruct\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
            
            return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
        }
        
        memset(pConnection, 0, sizeof(RtspServerConnection));
        
        pConnection->hApp                     = hApp;
        pConnection->pLogMgr                  = pThis->pLogMgr;
        pConnection->pLogSrc                  = &pThis->logSrc;
        pConnection->hRtsp                    = hRtsp;
        pConnection->hRPool                   = pThis->hRPool;
        pConnection->pGMutex                  = &pThis->gMutex;
        pConnection->pSelectEngine            = pThis->pSelectEngine;
        pConnection->destructed               = RV_FALSE;
        pConnection->maxSessionsPerConnection = ((RvRtspServerConfiguration *)pThis->configuration)->maxSessionsPerConnection;
        pConnection->hRaSessions              = pThis->hRaSessions;
        pConnection->hRaRequests              = pThis->hRaRequests;
        pConnection->hRaResponses             = pThis->hRaResponses;
        pConnection->hSessionList             = NULL;
        
        memcpy(&pConnection->configuration, pConfiguration, configurationSize);
    }

    if (pConnection == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }
    
    strncpy(pConnection->sourceIPAddress, clientIPAddress, sizeof(pConnection->sourceIPAddress) - 1); 
    
    pConnection->clientPort           = clientPort;
    pConnection->sourceIPAddressValid = RV_TRUE;

    
    result = RtspServerConnectionConstruct(pConnection, reAttachConnection);

    pConnection->transport.transportState       = RTSP_TRANSPORT_STATE_CONNECTED;
    pConnection->transport.socketFd.userContext = &pConnection->transport;
    pConnection->transport.rxSkipEol            = RV_FALSE;
    pConnection->transport.rxState              = RTSP_TRANSPORT_RX_STATE_FIRST_LINE;
    pConnection->transport.txState              = RTSP_TRANSPORT_TX_STATE_IDLE;
    pConnection->transport.socketFd.userContext = &pConnection->transport;
    pConnection->transport.socket               = newSocket;

    result = RvFdConstruct(&pConnection->transport.socketFd, &pConnection->transport.socket,
                           pConnection->transport.pLogMgr);

    if (result != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc,
                   "RvRtspServerConnectionConstruct - Failed to construct socketFd\r\n"));
        return result;
    }

    /* Register the new socketFd to receive the events */
    result = RtspTransportAddUpdateEvents(&pConnection->transport, RvSelectRead |RvSelectClose);

    if (result != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc,
                   "RvRtspServerConnectionConstruct - Add Update failed\r\n"));
        return result;
    }

    *phConnection = (RvRtspServerConnectionHandle)pConnection;
    
    RvLogDebug(&pThis->logSrc, (&pThis->logSrc,
                               "RvRtspServerConnectionConstruct - new connection created.\n"));
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc,
                               "RvRtspServerConnectionConstruct\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    
    return result;

}
    
/***********************************************************************
 * RvRtspServerConnectionDestruct                                       
 * --------------------------------------------------------------------- 
 * General: 
 *  This API de-initializes a connection and releases its resources. 
 *  This API in effect also destroys all the sessions in the connection.
 *
 * Arguments:                                                           
 * Input:  hConnection           - Handle to the connection to be destructed.  
 *         destructAllReferences - Destruct even if connection reference
 *                                 count is not zero.                      
 * Output: None.                                                              
 *                                                                 
 * Return Value: RV_OK - On successful disconnection.                      
 *               Negative values otherwise.            
 ***********************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionDestruct(
    IN RvRtspServerConnectionHandle hConnection,
    IN RvBool                       destructAllReferences)
{
    RvRtsp                *pThis;
    RtspServerConnection  *pConnection;

    pConnection = (RtspServerConnection*)hConnection;
    pThis       = (RvRtsp*)pConnection->hRtsp;

    if ((pThis == NULL) || (hConnection == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionDestruct\r\n"));

    RtspServerConnectionDestruct(hConnection, destructAllReferences);

    /* Removing the connection from the RA connection                   */
    /* This is done once all sessions in the connection are destructed*/
    if (pConnection->referenceCount == 0)
    {
        raDelete(pThis->hRaConnections, (RAElement) pConnection);
    }

    pConnection->destructed = RV_TRUE;

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerConnectionDestruct\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);

    return RV_OK;
}

  
/***********************************************************************
 * RvRtspServerGetConnectionByIPPort
 * ---------------------------------------------------------------------
 * General: 
 *  This API returns the connection corresponding to the 
 *  client IP and the port number. It identifies the connection on the
 *  given IP and port number and returns a handle to the same.
 *
 * Arguments:                                                           
 * Input:  hRtsp         - Handle to the Rtsp main structure.     
 *         strClientIP   - The client's IP address.               
 *         ClientPort    - The client port number.
 * Output: phConnection  - The connection with the specified IP 
 *                         address or NULL if not found. 
 *
 * Return Value: RV_OK - If function is successfully completed. 
 *               Negative values otherwise.                       
 ***********************************************************************/
RVAPI RvStatus RvRtspServerGetConnectionByIPPort(
    IN  RvRtspHandle                 hRtsp,
    IN  const RvChar                 *strClientIP, 
    IN  RvUint16                     clientPort,
    OUT RvRtspServerConnectionHandle *phConnection)
{
    RvRtsp                *pThis;
    RvInt32               location;
    RtspServerConnection  *pConnection;

    pThis         = (RvRtsp*)hRtsp;
    *phConnection = NULL;
    
    if ((pThis == NULL) || (strClientIP == NULL))
    {
       return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerGetConnectionByIPPort\r\n"));

    location = raGetNext(pThis->hRaConnections, -1);
    while (location >= 0)
    {
        pConnection = (RtspServerConnection*) raGet(pThis->hRaConnections, location);
        /* check the connection */
        if ((pConnection->state != RTSP_CONNECTION_STATE_DESTRUCTED) &&
            (strcmp(strClientIP, pConnection->sourceIPAddress) == 0) &&
            (clientPort == pConnection->clientPort) )
        {
            *phConnection = (RvRtspServerConnectionHandle) pConnection;
            RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerGetConnectionByIPPort\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
            return RV_OK;
        }

        location = raGetNext(pThis->hRaConnections, location);
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerGetConnectionByIPPort\r\n"));
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    
    return RV_OK;
}


/***********************************************************************
 * RvRtspServerGetSoftwareVersion                                       
 * --------------------------------------------------------------------- 
 * General: 
 *  This API retrieves the version information of the RvRtp     
 *  module.                                                     
 *
 * Arguments:                                                           
 * Input:  hRtsp               - Handle to the Rtsp main structure.     
 * Output: pMajorVersion       - The module's major version number.     
 *         pMinorVersion       - The module's minor version number.     
 *         pEngineeringRelease - The module's engineering release number
 *         pPatchLevel         - The module's patch level.  
 *            
 * Return Value: RV_OK - If successful.                                   
 *               Negative values otherwise.            
 ***********************************************************************/
RVAPI void RVCALLCONV RvRtspServerGetSoftwareVersion(
    IN RvRtspHandle hRtsp,
    OUT RvUint8 *pMajorVersion,
    OUT RvUint8 *pMinorVersion,
    OUT RvUint8 *pEngineeringRelease,
    OUT RvUint8 *pPatchLevel)
{
    RvRtsp  *pThis = (RvRtsp*)hRtsp;

    if (pThis == NULL)
        return;

    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerGetSoftwareVersion\r\n"));

    *pMajorVersion       = MAJOR_VERSION;
    *pMinorVersion       = MINOR_VERSION;
    *pEngineeringRelease = ENGINEERING_RELEASE;
    *pPatchLevel         = PATCH_LEVEL;

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspServerGetSoftwareVersion\r\n"));
}
   
#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#if defined(__cplusplus)
}
#endif
 
