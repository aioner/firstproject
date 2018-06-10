/*
*********************************************************************************
*                                                                               *
* NOTICE:                                                                       *
* This document contains information that is confidential and proprietary to    *
* RADVision LTD.. No part of this publication may be reproduced in any form     *
* whatsoever without written prior approval by RADVision LTD..                  *
*                                                                               *
* RADVision LTD. reserves the right to revise this publication and make changes *
* without obligation to notify any person of such revisions or changes.         *
*********************************************************************************
*/


/*********************************************************************************
 *                              <RvRtspClient.c>
 *
 * Interface functions for the entire RTSP library. The RvRtsp module is responsible
 * for initializing and de-initializing the library, and to holding the RTSP sessions
 * and connections.
 *
 *    Author                         Date
 *    ------                        ------
 *      Shaft                       8/1/04
 *		Shaft						14/05/2006	- added definitions for new ccore 1.1.12
 *												- DNS interface has changed
 *
 *********************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILED                           */
/*-----------------------------------------------------------------------*/

#include "rvtypes.h"
#include "rvstdio.h"
#include "rvcbase.h"
#include "rvansi.h"
#include "rvmemory.h"

#include "RvRtspClient.h"
#include "RtspObject.h"
#include "RtspUtilsInternal.h"
#include "RtspClientConnectionInternal.h"
#include "RtspMsgInternal.h"

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER)
#define MAJOR_VERSION           2  /* The RTSP lib's major version number. */
#define MINOR_VERSION           0  /* The RTSP lib's minor version number. */
#define ENGINEERING_RELEASE     0  /* The RTSP stack's engineering release number. */
#define PATCH_LEVEL             0  /* The RTSP stack's patch level. */

/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RtspAresNewItemCB
 * ------------------------------------------------------------------------
 * General: This callback method is called when a DNS response is received.
 *
 * Return Value:    RV_OK - If successfull
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The connection on which the DNS request was made.
 *              pDnsData    - holds the IP address for the host name given
 *                          to the DNS.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspAresNewItemCB(
                    IN  void            *context,
                    IN  RvUint32        queryId,
                    IN  RvDnsData       *pDnsData);



/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RvRtspInit
 * ------------------------------------------------------------------------
 * General: This method initiailzes the RvRtsp module.
 *          This method should be called before any classes or methods
 *          of the library are used. It should only be called once.
 *
 *          Note: The RvRtspMainLoop function must be called by the same
 *          thread that called RvRtspInit function.
 *
 * Return Value:    RV_OK - When the RTSP library is successfully initialized
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hLog			    - The RTSP module log handle, NULL if
 *                                    one should be created.
 *				pConfiguration	    - configuration for the module.
 *              configurationSize   - size of configuration structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 * OUTPUT   :   phRtsp              - handle to the Rtsp main structure.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspInit(
						IN 	RvLogHandle					hLog,
						IN	const RvRtspConfiguration	*pConfiguration,
                        IN	const RvSize_t              configurationSize,
                        OUT	RvRtspHandle				*phRtsp)
{
    RvStatus            result;
    RvAddress           dnsServer;
    RvInt               maxServers = 5;
    RvInt               maxDomains = 5;
    RvRtsp				*pThis;
    RvRtspConfiguration *configuration;
    RvSize_t            requestObjSize = sizeof(RvRtspRequest);
    RvSize_t            responseObjSize = sizeof(RvRtspResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvUint32            maxRtspMsgHeaders = 0;
#endif

    result  = RvCBaseInit();
    if (RV_OK != result)
    {
    	return result;
    }

    /* alloc space for RTSP stack */
    RvMemoryAlloc(NULL, sizeof(RvRtsp), NULL, (void**)&pThis);

    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (sizeof(RvRtspConfiguration) != configurationSize)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    memset(pThis,0 , sizeof(RvRtsp));
    *phRtsp     = (RvRtspHandle)pThis;

    /* Allocate space for the stack configuration */
    RvMemoryAlloc(NULL, sizeof(RvRtspConfiguration), NULL, (void**)&configuration);
    if (configuration == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    memset(configuration, 0, sizeof(RvRtspConfiguration));
    pThis->configuration = (RvRtspConfigurationHandle)configuration;

     /* Create log source */
    if (hLog == NULL)
    {
#if RV_LOGMASK != RV_LOGLEVEL_NONE
	    /* Create log source */
        RvMemoryAlloc(NULL, sizeof(RvLogMgr), NULL, (void**)&pThis->pLogMgr);
        RvLogConstruct(pThis->pLogMgr);
        RvLogSetGlobalMask(pThis->pLogMgr, RV_LOGLEVEL_ALL);
		RvLogListenerConstructLogfile(&pThis->logListener, pThis->pLogMgr, "D:/rtsp_client.txt",1,0,RV_TRUE);
        pThis->bCreatedLogMgr = RV_TRUE;
#endif
    }
    else
    {
	    pThis->pLogMgr = (RvLogMgr*)hLog;
        pThis->bCreatedLogMgr = RV_FALSE;
    }

    if (pThis->pLogMgr != NULL)
    {
        RvLogSourceConstruct(pThis->pLogMgr, &pThis->logSrc, "RTSP", "Rtsp Stack");
    }

    /* Rtsp module configuration initialization */
    memcpy(configuration, pConfiguration, sizeof(RvRtspConfiguration));

    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspInit\r\n"));

    /* Create guard Mutex */
    result = RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);
    if (result != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc, "RvRtspInit - Mutex construction failed\r\n"));
        return result;
    }

    /* Create global Mutex */
    result = RvMutexConstruct(pThis->pLogMgr, &pThis->gMutex);
    if (result != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc, "RvRtspInit - GMutex construction failed\r\n"));
        return result;
    }

    /* constructing the memory RPool                                                    */
    /* Note: thread safety is needed because hRPool is used across different structures */
    pThis->hRPool = rpoolConstruct(
                        configuration->memoryElementsSize,    /* Elements size            */
                        configuration->memoryElementsNumber,  /* Max number of elements   */
                        RV_TRUE,                                    /* ThreadSafety             */
                        "Rtsp",                                     /* Name                     */
                        pThis->pLogMgr);                            /* Log manager              */


    if (pThis->hRPool == NULL)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    requestObjSize = sizeof(RtspMsgRequest);
    responseObjSize = sizeof(RtspMsgResponse);
#endif
    /* Construct the requests RA */
    if (configuration->msgRequestElementsNumber > 0)
    {
        pThis->hRaRequests = raConstruct(requestObjSize, 
                                         configuration->msgRequestElementsNumber, 
                                         RV_TRUE,
                                         "RtspMsgRequests",
                                         pThis->pLogMgr);
        if (pThis->hRaRequests == NULL)
            return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    else
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    /* Construct the responses RA */
    if (configuration->msgResponseElementsNumber > 0)
    {
        pThis->hRaResponses = raConstruct(responseObjSize, 
                                         configuration->msgResponseElementsNumber, 
                                         RV_TRUE,
                                         "RtspMsgResponses",
                                         pThis->pLogMgr);
        if (pThis->hRaResponses == NULL)
            return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    else
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    /* Construct the RtspMsgHeaders RA */
    maxRtspMsgHeaders = (configuration->msgRequestElementsNumber + 
        configuration->msgRequestElementsNumber) * configuration->maxRtspMsgHeadersInMessage;

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

    /* construct select engine */
    result = RvSelectConstruct(
                    4096,                                   /* hash size for the engine     */
                    configuration->maxConnections,
                    pThis->pLogMgr,
                    &pThis->pSelectEngine);

    if (result != RV_OK)
        return result;

    RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspInit - Select Engine Constructed\r\n"));

    /* construct DNS engine */
	result = RvAresConstructN(	pThis->pSelectEngine,
								RtspAresNewItemCB,
                                maxServers,
                                maxDomains,
								DNS_TCP_RECEIVE_BUFFER_LENGTH,
                                pThis->pLogMgr,
								&pThis->dnsEngine);
    if (result != RV_OK)
        return result;

    RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspInit - Ares DNS Engine Constructed\r\n"));

    /* construct DNS server address */
    RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &dnsServer);
    RvAddressSetString(configuration->strDnsAddress, &dnsServer);
    result = RvAresSetParams(&pThis->dnsEngine, 10, 4, &dnsServer, 1,NULL,0);

    if (result != RV_OK)
        return result;

    pThis->doTerminate = RV_FALSE;
    pThis->terminated = RV_FALSE;


    /* Initializing the connections structure   */
    pThis->hRaConnections = raConstruct(
            sizeof(RtspConnection),                 /* elements size            */
            configuration->maxConnections,    /* max number of elements   */
            RV_FALSE,                               /* thread safety            */
            "Connections",                          /* RA name                  */
            pThis->pLogMgr);                        /* log manager              */

    RvLogDebug(&pThis->logSrc, (    &pThis->logSrc,
                                "RvRtspInit - RTSP Module Initialized, Version: %03u.%03u.%03u, patch = %u\r\n",
                                MAJOR_VERSION,
                                MINOR_VERSION,
                                ENGINEERING_RELEASE,
                                PATCH_LEVEL));

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspInit\r\n"));

    return RV_OK;
}



/**************************************************************************
 * RvRtspEnd
 * ------------------------------------------------------------------------
 * General: This method de-initiailzes the RvRtsp module.
 *          This method should be called when the RvRtsp module is no longer
 *          needed. After this call is made none of the classes or methods
 *          of this module should be used.
 *
 * Return Value:    RV_OK - If the RTSP library is successfully uninitialized
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp  - handle to the Rtsp main structure.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspEnd(
                        IN RvRtspHandle hRtsp)
{
    RvStatus		status = RV_OK;
	RvRtsp          *pThis = (RvRtsp*)hRtsp;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspEnd\r\n"));

    /* Signal termination to the Main Loop */
    pThis->doTerminate = RV_TRUE;

    /* allow the main loop thread to terminate */
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspEnd\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    status = RvCBaseEnd();
    
    return status;
}



/**************************************************************************
 * RvRtspMainLoop
 * ------------------------------------------------------------------------
 * General: This method runs in the same user thread that called the RTSP
 *          module's Init, it runs a loop waiting for events to dispatch
 *          callback functions for.
 *          The method returns to the user after 100 miliseconds and allows
 *          the user to perform user specific functionality before calling
 *          the method again.
 *
 * Return Value:    RV_OK - If the RTSP library is successfully uninitialized
 *                  RV_ERROR_UNINITIAIZED - when the stack is successfuly
 *                  uninitialized. The error is returned only when the user
 *                  signaled the stack to terminate itself by calling RvRtspEnd.
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp  - handle to the Rtsp main structure.
 *
 *				14/05/2006
 *				timeout (milliseconds) - how much time to wait on a selectEngine Event.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspMainLoop(
						IN RvRtspHandle	hRtsp,
						IN RvUint32		timeOut)

{
    RvRtsp              *pThis = (RvRtsp *) hRtsp;
    int                 location;
    RtspConnection      *pConnection;
    RvBool              doTerminate;
    RvRtspConfiguration *configuration;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspMainLoop\r\n"));
    doTerminate = pThis->doTerminate;
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    if (doTerminate != RV_TRUE)
    {
        RvSelectWaitAndBlock(pThis->pSelectEngine, timeOut*RV_TIME64_NSECPERMSEC);
        return RV_OK;
    }

    configuration = (RvRtspConfiguration *)pThis->configuration;

    /* in case the user called this function after calling the      */
    /* rvRtspEnd API which signaled the main loop to terminate      */

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);

    pThis->terminated = RV_TRUE;

    /* Destruct all connections which were left undestructed                        */
    /* We don't need to take care for the sessions because each connection will     */
    /* destruct all sessions related to it.                                         */
    location = raGetNext(pThis->hRaConnections, -1);
    while (location >= 0)
    {
        pConnection = (RtspConnection*) raGet(pThis->hRaConnections, location);
        RvRtspConnectionDestruct(hRtsp, (RvRtspConnectionHandle)pConnection,RV_TRUE);
        location = raGetNext(pThis->hRaConnections, location);
    }

    raClear(pThis->hRaConnections);
    raDestruct(pThis->hRaConnections);
    pThis->hRaConnections = NULL;

    /* destruct the dns engine */
    RvAresDestruct(&pThis->dnsEngine);

    /* destruct the select engine */
    RvSelectDestruct(pThis->pSelectEngine, configuration->maxConnections);
    pThis->pSelectEngine = NULL;

    /* Destruct the pool */
    rpoolDestruct(pThis->hRPool);
    pThis->hRPool = NULL;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    /* Destruct RtspMsg RAs */
    if (pThis->hRaRequests != NULL)
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
    if (pThis->hRaHeaders != NULL)
    {
        raClear(pThis->hRaHeaders);
        raDestruct(pThis->hRaHeaders);
        pThis->hRaHeaders = NULL;
    }
#endif



    /* Destruct the guard Mutexes */
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->gMutex, pThis->pLogMgr);

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    /* Destruct the log source */
    RvLogSourceDestruct(&pThis->logSrc);
    if (pThis->bCreatedLogMgr)
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
 * RvRtspConnectionConstruct
 * ------------------------------------------------------------------------
 * General: This method constructs a connection in hRtsp, calling the
 *          actual construct and adding the connection to the Rtsp
 *          module's connections.
 *
 * Return Value:    RV_OK if the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp               - Handle to the Rtsp main structure.
 *              hApp                - application context handle.
 *              strServerURI        - The URI to connect to.
 *              sourceIPAddress     - The local IP address.
 *              pConfiguration      - The connection configuration.
 *              configurationSize   - size of configuration structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 * OUTPUT   :   phConnection        - Handle to the constructed connection.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionConstruct(
                        IN  RvRtspHandle                        hRtsp,
                        IN  RvRtspConnectionAppHandle           hApp,
                        IN  const RvChar                        *strServerURI,
                        IN  const RvChar                        *sourceIPAddress,
                        IN  const RvRtspConnectionConfiguration *pConfiguration,
                        IN  const RvSize_t                      configurationSize,
                        OUT RvRtspConnectionHandle              *phConnection)
{
    RvStatus        result;
    RvRtsp          *pThis = (RvRtsp*)hRtsp;
    RtspConnection  *pConnection;


    if ((hRtsp          == NULL)    ||
        (strServerURI   == NULL)    ||
        (pConfiguration == NULL)    ||
        (phConnection   == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    
    *phConnection = NULL;

    if (sizeof(RvRtspConnectionConfiguration) != configurationSize)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }


    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct\r\n"));

    RvRtspGetConnectionByURI(hRtsp, strServerURI, phConnection);

    /* if a connection to the server was already established    */
    /* we'll just return it                                     */
    if (*phConnection != NULL)
    {
        pConnection = (RtspConnection*)*phConnection;
        pConnection->referenceCount++;
        RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct - already exists\r\n"));
        RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
        return RV_OK;
    }


    /* if a connection to the server was not established yet... */
    /* we'll construct a new connection                         */
    if (raAdd(pThis->hRaConnections, (RAElement*)&pConnection) < 0)
    {
        /* it seems that we don't have any place left in the RA */
        RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct - Out of resources\r\n"));
        RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }

    pConnection->pLogMgr        = pThis->pLogMgr;
    pConnection->pLogSrc        = &pThis->logSrc;
    pConnection->hApp           = hApp;
    pConnection->hRtsp          = hRtsp;
    pConnection->hRPool         = pThis->hRPool;
    pConnection->hRaRequests    = pThis->hRaRequests;
    pConnection->hRaResponses   = pThis->hRaResponses;
    pConnection->pGMutex        = &pThis->gMutex;
    pConnection->pSelectEngine  = pThis->pSelectEngine;
    pConnection->pDnsEngine     = &pThis->dnsEngine;
    pConnection->destructed     = RV_FALSE;
    memcpy(&pConnection->configuration, pConfiguration, sizeof(RvRtspConnectionConfiguration));

    if (sourceIPAddress == NULL)
    {
        pConnection->sourceIPAddressValid = RV_FALSE;
    }

    else
    {
        strncpy(pConnection->sourceIPAddress, sourceIPAddress, sizeof(pConnection->sourceIPAddress) - 1);
        pConnection->sourceIPAddressValid = RV_TRUE;
    }

    //add by kongfd for Authenticate Construct
	memset(pConnection->Authenticate.username,0,MAX_AUTHCHAR_LEN);
	memset(pConnection->Authenticate.password,0,MAX_AUTHCHAR_LEN);
	memset(pConnection->Authenticate.Digestrealm,0,MAX_AUTHCHAR_LEN);
	memset(pConnection->Authenticate.Digestnonce,0,MAX_AUTHCHAR_LEN);
	memset(pConnection->Authenticate.Basicrealm,0,MAX_AUTHCHAR_LEN);

    pConnection->Authenticate.isBasic =RV_FALSE;
    pConnection->Authenticate.isDigest =RV_FALSE;
    pConnection->Authenticate.isUser =RV_FALSE;
    
    //add end

    /* We set the server's address parameters - first serverName and    */
    /* serverAddress's port and then serverAddress's IP address on DNS  */
    /* response                                                         */
    RtspUtilsGetIpAddressFromUriEx(strServerURI,pConnection->strServerName,&pConnection->serverPort,NULL,
    								pConnection->Authenticate.username,pConnection->Authenticate.password,&pConnection->Authenticate.isUser);
 
    result = RtspConnectionConstruct(pConnection);
    *phConnection = (RvRtspConnectionHandle)pConnection;

    RvLogDebug(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct - new\r\n"));
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionConstruct\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    return result;
}

/**************************************************************************
 * RvRtspConnectionDestruct
 * ------------------------------------------------------------------------
 * General: This method de-initializes the connection and releases it's
 *          resources. This includes destructing the connection's sessions.
 *          Note: each time RvRtspConnectionDestruct is called on the
 *          connection, it's reference count decreases, when a destruct is
 *          called on the last reference, the connection is destructed.
 *          if destructAllReferences is set, the connection is destructed
 *          immediately.
 *
 * Return Value:    RV_OK - If the connection is successfully de-initialized
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp                   - Handle to the Rtsp main structure.
 *          :   hConnection             - the destructed connection.
 *              destructAllReferences   - destruct even if connection count
 *                                        indicates other references.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionDestruct(
                        IN  RvRtspHandle            hRtsp,
                        IN  RvRtspConnectionHandle  hConnection,
                        IN  RvBool                  destructAllReferences)
{
    RvRtsp          *pThis = (RvRtsp*)hRtsp;
    RtspConnection  *pConnection;
    pConnection = (RtspConnection*)hConnection;

    if ((hRtsp == NULL) || (hConnection == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionDestruct\r\n"));

    RtspConnectionDestruct(hConnection, destructAllReferences);

    /* removing the connection from the RA connection                   */
    /* we'll do this only in case when all references to the connection */
    /* are destructed                                                   */
    if (pConnection->referenceCount == 0)
    {
        raDelete(pThis->hRaConnections, (RAElement) pConnection);
    }

    pConnection->destructed = RV_TRUE;
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionDestruct\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtspConnectionDestructEx(
    IN  RvRtspConnectionHandle  hConnection,
    IN  RvBool                  destructAllReferences)
{
    return RvRtspConnectionDestruct(((RtspConnection*)hConnection)->hRtsp, hConnection, destructAllReferences);
}

#if 0
/**************************************************************************
 * RvRtspGetConnectionByURI
 * ------------------------------------------------------------------------
 * General: This method returns the connection corresponding to the
 *          specified URI.
 *          Note: this method goes over the connections array sequentialy
 *          in order to find the connection, it is therefore not time
 *          efficient (O(n)) and thus not recommended as the normal way
 *          of accessing the connections.
 *
 * Return Value:    RV_OK if the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp           - handle to the Rtsp main structure.
 *              strUri          - the connection's URI.
 * OUTPUT   :   phConnection    - the connection with the specified URI
 *                              or NULL if not found.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspGetConnectionByURI(
                        IN  RvRtspHandle            hRtsp,
                        IN  const RvChar            *strURI,
                        OUT RvRtspConnectionHandle  *phConnection)
{
    RvRtsp              *pThis = (RvRtsp*)hRtsp;
    int                 location;
    RtspConnection      *pConnection;
    RvChar              strServerName[RV_RTSP_SERVER_NAME_MAX_LENGTH];
    RvUint16            serverPort;


    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI\r\n"));

    *phConnection = NULL;

    if (RtspUtilsGetIpAddressFromUri(strURI, RV_RTSP_SERVER_NAME_MAX_LENGTH-1, strServerName, &serverPort) != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI bad param\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    location = raGetNext(pThis->hRaConnections, -1);
    while (location >= 0)
    {
        pConnection = (RtspConnection*) raGet(pThis->hRaConnections, location);

        /* check the connection */
        if ((pConnection->state != RTSP_CONNECTION_STATE_DESTRUCTED) &&
            (strcmp(strServerName, pConnection->strServerName) == 0) &&
            (serverPort == pConnection->serverPort) )
        {
            *phConnection = (RvRtspConnectionHandle) pConnection;
            RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
            return RV_OK;
        }

        location = raGetNext(pThis->hRaConnections, location);
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI\r\n"));
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}
#endif

RVAPI
RvStatus RVCALLCONV RvRtspGetConnectionByURI(
                        IN  RvRtspHandle            hRtsp,
                        IN  const RvChar            *strURI,
                        OUT RvRtspConnectionHandle  *phConnection )
{
	RvRtsp              *pThis = (RvRtsp*)hRtsp;
	int                 location;
	RtspConnection      *pConnection;
	RvChar              strServerName[RV_RTSP_SERVER_NAME_MAX_LENGTH];
	RvUint16            serverPort;


	RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
	RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI\r\n"));

	*phConnection = NULL;

    if (RtspUtilsGetIpAddressFromUriEx(strURI, strServerName, &serverPort,NULL,NULL,NULL,NULL) != RV_OK)
    {
        RvLogError(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI bad param\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    location = raGetNext(pThis->hRaConnections, -1);
    while (location >= 0)
    {
        pConnection = (RtspConnection*) raGet(pThis->hRaConnections, location);

        /* check the connection */
        if ((pConnection->state != RTSP_CONNECTION_STATE_DESTRUCTED) &&
            (strcmp(strServerName, pConnection->strServerName) == 0) &&
            (serverPort == pConnection->serverPort) )
        {
            *phConnection = (RvRtspConnectionHandle) pConnection;
            RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
            return RV_OK;
        }

        location = raGetNext(pThis->hRaConnections, location);
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetConnectionByURI\r\n"));
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}

/**************************************************************************
 * RvRtspGetSessionById
 * ------------------------------------------------------------------------
 * General: This method returns the session corresponding to the specified
 *          Session-Id.
 *          Note: this method goes over the sessions sequentialy in order
 *          to find the session, it is therefore not time efficient (O(n))
 *          and thus not recommended as the normal way of accessing the
 *          sessions.
 *
 * Return Value:    RV_OK if the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp           - handle to the Rtsp main structure.
 *              strSessionId    - the session's Session-Id.
 * OUTPUT   :   phSession       - the session with the specified
 *                              Session-Id, or NULL if not found.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspGetSessionById(
                        IN  RvRtspHandle            hRtsp,
                        IN  const RvChar            *strSessionId,
                        OUT RvRtspSessionHandle     *phSession)
{
    RvRtsp              *pThis = (RvRtsp*)hRtsp;
    int                 location;
    RtspConnection      *pConnection;

    if ((hRtsp == NULL) || (strSessionId == NULL) || (phSession == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetSessionById\r\n"));

    *phSession = NULL;

    location = raGetNext(pThis->hRaConnections, -1);
    while (location >= 0)
    {
        pConnection = (RtspConnection*) raGet(pThis->hRaConnections, location);

        /* check the connection */
        if (pConnection->state != RTSP_CONNECTION_STATE_DESTRUCTED)
        {
            RvRtspConnectionGetSessionById((RvRtspConnectionHandle)pConnection, strSessionId, phSession);
            if (phSession != NULL)
            {
                RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetSessionById\r\n"));
                RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
                RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
                return RV_OK;
            }
        }

        location = raGetNext(pThis->hRaConnections, location);
    }

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetSessionById\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    return RV_OK;
}





/**************************************************************************
 * RvRtspGetSoftwareVersion
 * ------------------------------------------------------------------------
 * General: This method gets the version information of the RvRtp module.
 *
 * Return Value:    RV_OK - If successfull
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hRtsp               - handle to the RTSP module.
 * OUTPUT   :   pMajorVersion       - The module's major version number.
 *              pMinorVersion       - The module's minor version number.
 *              pEngineeringRelease - The module's engineering release number.
 *              pPatchLevel         - The modules patch level.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
void RVCALLCONV RvRtspGetSoftwareVersion(
                    IN  RvRtspHandle    hRtsp,
                    OUT RvUint8         *pMajorVersion,
                    OUT RvUint8         *pMinorVersion,
                    OUT RvUint8         *pEngineeringRelease,
                    OUT RvUint8         *pPatchLevel)
{
    RvRtsp  *pThis = (RvRtsp*)hRtsp;

    if (pThis == NULL)
        return;

    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetSoftwareVersion\r\n"));

    *pMajorVersion       = MAJOR_VERSION;
    *pMinorVersion       = MINOR_VERSION;
    *pEngineeringRelease = ENGINEERING_RELEASE;
    *pPatchLevel         = PATCH_LEVEL;

    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspGetSoftwareVersion\r\n"));
}



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RtspAresNewItemCB
 * ------------------------------------------------------------------------
 * General: This callback method is called when a DNS response is received.
 *
 * Return Value:    RV_OK - If successfull
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The connection on which the DNS request was made.
 *              pDnsData    - holds the IP address for the host name given
 *                          to the DNS.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspAresNewItemCB(
                    IN  void            *context,
                    IN  RvUint32        queryId,
                    IN  RvDnsData       *pDnsData)
{
    RtspConnection  *pConnection = (RtspConnection*)context;
    RvChar          strBuf[100];


    RvMutexLock(pConnection->pGMutex, pConnection->pLogMgr);
    if (pConnection->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pConnection->pGMutex, pConnection->pLogMgr);
        return RV_OK;
    }

    RvMutexLock(&pConnection->mutex, pConnection->pLogMgr);
    RvMutexUnlock(pConnection->pGMutex, pConnection->pLogMgr);
    RvLogEnter(pConnection->pLogSrc, (pConnection->pLogSrc, "RtspAresNewItemCB\r\n"));

    /* dnsData->data contains the IP address */
    RvAddressGetString(&pDnsData->data.hostAddress, 50, strBuf);
    RvLogDebug(pConnection->pLogSrc, (  pConnection->pLogSrc,
                                        "RtspAresNewItemCB Status %d IP Address %s\r\n",
                                        pDnsData->dataType, strBuf));
    switch (pDnsData->dataType)
    {
        case RV_DNS_HOST_IPV4_TYPE:
        case RV_DNS_HOST_IPV6_TYPE:
        {
            RtspConnectionOnDnsResponse(pConnection, &pDnsData->data.hostAddress);
			break;
		}
		case RV_DNS_ENDOFLIST_TYPE:
		{
			if (pConnection->queryIdIpv4 == queryId)
			{
				RvMemoryFree(pConnection->pIpv4DNSMem, NULL);
			}

			else
			if (pConnection->queryIdIpv6 == queryId)
			{
				RvMemoryFree(pConnection->pIpv6DNSMem, NULL);
			}
            break;
        }

        default:
        {
            break;
        }
    } /* switch */

    RvLogLeave(pConnection->pLogSrc, (pConnection->pLogSrc, "RtspAresNewItemCB\r\n"));
    RvMutexUnlock(&pConnection->mutex, pConnection->pLogMgr);
    return RV_OK;
}

/*add by kongfd*/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionSetUser(
                        IN  RvRtspHandle            hRtsp,
                        IN  RvRtspConnectionHandle  hConnection,
                        IN  RvChar *                  username,
                        IN  RvChar *                  password)
{
    RvRtsp          *pThis = (RvRtsp*)hRtsp;
    RtspConnection  *pConnection;
    pConnection = (RtspConnection*)hConnection;

    if ((hRtsp == NULL) || (hConnection == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    if ((username == NULL) || (password == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);        

    RvMutexLock(&pThis->gMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionDestruct\r\n"));

    strcpy(pConnection->Authenticate.username,username);
    strcpy(pConnection->Authenticate.password,password);    
    pConnection->Authenticate.isUser = RV_TRUE;
    
    RvLogLeave(&pThis->logSrc, (&pThis->logSrc, "RvRtspConnectionSetUser\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->gMutex, pThis->pLogMgr);
    return RV_OK;
}
static char base64_charset[] =
				   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char * DataToBase64(char *Data,int DataLen)
{
	char *res, *result;
	char * retstr=NULL;
	int  t;
	int  k;

	if(Data==NULL || DataLen<=0)
	{
		return retstr;
	}
	result = res =malloc((DataLen+2)/3 * 4 + 1);
	for (k = 0; k < DataLen/3; ++k)
	{
		*res++ = base64_charset[(*Data >> 2)];
		t = ((*Data & 0x03) << 4);
		Data++;
		*res++ = base64_charset[t | (*Data >> 4)];
		t = ((*Data & 0x0F) << 2);
		Data++;
		*res++ = base64_charset[t | (*Data >> 6)];
		*res++ = base64_charset[*Data & 0x3F];
		Data++;
	}
	switch (DataLen % 3)
	{
		case 2:
			*res++ = base64_charset[*Data >> 2];
			t =  ((*Data & 0x03) << 4);
			Data++;
			*res++ = base64_charset[t | (*Data >> 4)];
			*res++ = base64_charset[(*Data++ & 0x0F) << 2];
			*res++ = '=';
			break;
		case 1:
			*res++ = base64_charset[*Data >> 2];
			*res++ = base64_charset[(*Data++ & 0x03) << 4];
			*res++ = '=';
			*res++ = '=';
			break;
	}
	*res = 0;
	return result;
}

RVAPI
RvStatus RVCALLCONV RvRtspConnectionSetUserEx(
    IN  RvRtspConnectionHandle  hConnection,
    IN  RvChar *                  username,
    IN  RvChar *                  password)
{
    return RvRtspConnectionSetUser(((RtspConnection *)hConnection)->hRtsp, hConnection, username, password);
}

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */


/*add end*/
#if defined(__cplusplus)
}
#endif
