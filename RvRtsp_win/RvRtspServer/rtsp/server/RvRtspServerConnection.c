/*
 *******************************************************************************
 *                                                                             *
 * NOTICE:                                                                     *
 * This document contains information that is confidential and proprietary to  *
 * RADVision LTD.. No part of this publication may be reproduced in any form   *
 * whatsoever without written prior approval by RADVision LTD..                *
 *                                                                             *
 * RADVision LTD. reserves the right to revise this publication and make       *
 * changes without obligation to notify any person of such revisions           *
 * or changes.                                                                 *
 *******************************************************************************
 */


/******************************************************************************
 *                            <RvRtspServerConnection.c>
 *
 *  This file contains definitions relevant to the RTSP server connections.
 *  An RTSP Server Connection instance is a thread safe representation of a 
 *  connection to an RTSP server, handling all RTSP communication to and from 
 *  the server.
 *
 *****************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                           INCLUDE HEADER FILES                             */
/*----------------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "rvcbase.h"
#include "rvstdio.h"
#include "rvares.h"
#include "ra.h"

#include "RvRtspServerConnection.h"
#include "RtspServerConnectionInternal.h"
#include "RvRtspServerSession.h"
#include "RtspServerSessionInternal.h"
#include "RtspTransport.h"
#include "RtspFirstLine.h"
#include "RtspMessages.h"
#include "RtspUtilsInternal.h"
#include "RtspHeaders.h"
#include "RtspMsgInternal.h"


/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/
/**************************************************************************
 * RtspServerConnectionOnRawBufferReceiveEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer whenever a buffer
 *          is read from the socket.
 *          The callback notifies the application so that it can check
 *          if there is encapsulated binary stream in it.
 *
 * Return Value:    RV_OK if process of received buffer should continue
 *                  in the usual manner, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - the Connection on which the buffer was received.
 *              pBuff       -   The received buffer.
 *              buffSize    -   The received buffer size.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspServerConnectionOnRawBufferReceiveEv(
                            IN      void        *context,
                            IN      RvUint8     *pBuff,
                            IN      RvUint32    buffSize);

/**************************************************************************
 * RtspServerConnectionHandleResponse
 * ------------------------------------------------------------------------
 * General: 
 *  This method handles a response which was received on the
 *  connection. The method does not call the session callback as 
 *  no response is expected on the session.
 *
 * Arguments:
 * Input:   pThis       - The Connection on which the response was received.
 *          pResponse   - the received response.
 * Output:  None.
 *
 * Return Value: RV_OK - if successful.
 *               Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionHandleResponse(
    IN RtspServerConnection       *pThis,
    IN const RvRtspResponse       *pResponse);


/**************************************************************************
 * RtspServerConnectionHandleRequest
 * ------------------------------------------------------------------------
 * General: 
 *  This method handles a request from the client which was received on the
 *  connection. It calls either the connection on receive callback or the 
 *  session on recieve callback depending on whether a session exists or 
 *  not.
 *
 * Arguments:
 * Input:    pThis       - the Connection on which the request was received.
 *           pRequest    - the received request.
 * Output:   None.
 *
 * Return Value:    RV_OK - if successful.
 *                  Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionHandleRequest(
    IN RtspServerConnection       *pThis,
    IN const RvRtspRequest        *pRequest);


/**************************************************************************
 * RtspServerConnectionOnReceive
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Transport layer after a message
 *  was received by it. It handles the received message via
 *  RtspServerConnectionHandleRequest/RtspServerConnectionHandleResponse
 *
 * Arguments:
 * Input:   context            - The connection on which the message
 *                               was received.
 *          pTransportMessage  - The received transport message.
 * Output:  None.
 *
 * Return Value:    RV_OK - if successful.
 *                  Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionOnReceive(
    IN void                         *context,
    IN const RtspTransportMessage   *pTransportMessage);


/**************************************************************************
 * RtspServerConnectionOnAccept
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Transport layer after a TCP
 *  connection is accepted by it. 
 *
 * Arguments:
 * Input:   context      - The connection handle.
 *          newSocket    - the new socket.
 *          strClientIP  - Client IP.
 *          portNum      - Client port.
 *          success      - Indicates if the connection was constructed
 *                         successfully
 * Output:  success      - Indicates if the connection was constructed
 *                         successfully
 *
 * Return Value:    RV_OK - if successful.
 *                  Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionOnAccept(
    IN void*     context,
    IN RvSocket  newSocket,
    IN RvChar    *strClientIP,
    IN RvUint16  portNum,
    INOUT RvBool *success);
                 
/**************************************************************************
 * RtspServerConnectionOnDisconnect
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Transport layer after a TCP
 *  connection is disconnected.
 *
 * Arguments:
 * Input:    context        - The connection handle.
 * Output:   None.
 *
 * Return Value:    RV_OK - if successful.
 *                  Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionOnDisconnect(
    IN void *context);


/*----------------------------------------------------------------------------*/
/*                              MODULE FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

/******************************************************************************
 * RtspServerConnectionConstruct
 * ----------------------------------------------------------------------------
 * General: 
 *  This method constructs a connection, it is called after 
 *  setting the connection parameters in the connection
 *  structure.
 *
 * Arguments:
 * Input:   pThis       - The connection to be constructed.
 *          reAttachConnection  - If RV_TRUE - A broken connection is re-established
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 ******************************************************************************/
RvStatus RtspServerConnectionConstruct(
    IN  RtspServerConnection  *pThis,
    IN  RvBool                reAttachConnection)
{
    RvStatus                        result;
    RtspTransportCallbackFunctions  transportCallbacks;

    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionConstruct\r\n"));

    if (reAttachConnection == RV_FALSE)
    {
        /* initializing the RtspServerConnection data structure */
        pThis->sessionCount   = 0;
        pThis->referenceCount = 1;
        
        memset(&pThis->pfnCallbacks, 0, sizeof(pThis->pfnCallbacks));
        /* Create guard Mutex */
        result = RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);
        if ( result != RV_OK)
        {
            RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionConstruct - Mutex construction failed\r\n"));
            return result;
    }
    }
    

    /* construct the transport */
    pThis->transport.pLogMgr        = pThis->pLogMgr;
    pThis->transport.pLogSrc        = pThis->pLogSrc;
    pThis->transport.context        = (void*)pThis;
    pThis->transport.hRPool         = pThis->hRPool;
    pThis->transport.pSelectEngine  = pThis->pSelectEngine;
    pThis->transport.pGMutex        = pThis->pGMutex;
    pThis->transport.transportState = RTSP_TRANSPORT_STATE_DESTRUCTED;

    result = RtspTransportConstruct(&pThis->transport, pThis->configuration.transmitQueueSize, pThis->configuration.maxHeadersInMessage);

    /* callback registration - callbacks will be called on events */
    transportCallbacks.pfnOnAcceptEv             = RtspServerConnectionOnAccept,
    transportCallbacks.pfnOnConnectEv            = NULL,
    transportCallbacks.pfnOnDisconnectEv         = RtspServerConnectionOnDisconnect,
    transportCallbacks.pfnOnReceiveEv            = RtspServerConnectionOnReceive,
    transportCallbacks.pfnOnContentLengthEv      = NULL;
    transportCallbacks.pfnOnRawBufferReceiveEv   = RtspServerConnectionOnRawBufferReceiveEv;
    RtspTransportRegisterCallbacks( &pThis->transport, &transportCallbacks);

    /* Change connection state to constructed*/
    pThis->state    = RTSP_CONNECTION_STATE_CONSTRUCTED;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionConstruct\r\n"));

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    return RV_OK;
}


/******************************************************************************
 * RtspServerConnectionDestruct
 * ----------------------------------------------------------------------------
 * General: 
 *  This method de-initializes the connection and releases it's
 *  resources. This includes destructing the connection's sessions.
 *  Note: each time RvRtspServerSessionDestruct is called on the
 *  connection, it's reference count decreases, when a destruct is
 *  called on the last reference, the connection is destructed.
 *  if destructAllReferences is set, the connection is destructed
 *  immediately.
 *
 * Arguments:
 * Input:   hConnection             - The destructed connection.
 *          destructAllReferences   - Destruct even if connection count
 *                                    indicates other references.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 ******************************************************************************/
RvStatus RtspServerConnectionDestruct(
    IN  RvRtspServerConnectionHandle  hConnection,
    IN  RvBool                        destructAllReferences)
{
    RtspServerConnection        *pThis;
    RtspServerSession           *pSessionList;

    pThis     = (RtspServerConnection*)hConnection;

    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionDestruct\r\n"));

    /* if this is a regular destruct and we still have more references, */
    /* we'll only decrease the reference count                          */
    if ((destructAllReferences == RV_FALSE) && (pThis->referenceCount > 1))
    {
        pThis->referenceCount--;
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionDestruct\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RV_OK;
    }

    pThis->state            = RTSP_CONNECTION_STATE_DESTRUCTED;
    pThis->referenceCount   = 0;
    pThis->sessionCount     = 0;

    /* destroy the sessions                                                   */
    pSessionList = (RtspServerSession *)pThis->hSessionList;

    while (pSessionList != NULL)
    {
        RvRtspServerSessionDestruct((RvRtspServerSessionHandle)pSessionList);
        pSessionList = pSessionList->pNext;
    }

    /* destroy the transport                                                  */
    RtspTransportDestruct(&pThis->transport);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionDestruct\r\n"));

    /* Destruct the guard Mutex                                               */
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    return RV_OK;
}

/**************************************************************************
 * RtspServerConnectionStartListening
 * ------------------------------------------------------------------------
 * General: 
 *  This method listens on the listeningPort for new incomming connection
 *  requests. This function is called once the Server has been initialized.
 *
 * Arguments:
 * Input:   pThis               - The connection object on which the 
 *                                listening socket is to be created.
 *          listeningIPAddress  - The IP address on which the Server 
 *                                listens for clients connections.
 *          listeningPort       - Port number.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerConnectionStartListening(
                IN  RtspServerConnection  *pThis,
                IN  const RvChar          *listeningIPAddress,
                IN  const RvUint16        listeningPort)
{
    RvStatus result;

    result = RV_OK;

    result = RtspTransportListen(&pThis->transport, listeningIPAddress, listeningPort);

    return result;  
}

/**************************************************************************
 * RvRtspServerConnectionRegisterCallbacks
 * ------------------------------------------------------------------------
 * General: 
 *  This API is called to register all the connection object's callbacks.
 *  The callback functions are triggered when an event occurs only once
 *  they have been registered. It is essential to register all the 
 *  connection callbacks for them to be triggered on an event occurence.
 *
 * Arguments:
 * Input:   hConnection    - The connection handle on which the callbacks
 *                           are registered.
 *          pCallbacks     - callback functions to be called when
 *                           events occur on the connection.
 *          callbacksStructSize - size of callbacks structure,
 *                                is passed in order to distinguish between
 *                                different versions of the RTSP Stack.
 * Output:  None.
 *
 * Return Value:   RV_OK - if successful.
 *                 Negative values otherwise.
 *************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionRegisterCallbacks(
    IN  RvRtspServerConnectionHandle                  hConnection,
    IN  const RvRtspServerConnectionCallbackFunctions *pCallbacks,
    IN  const RvSize_t                                callbacksStructSize)
{
    RtspServerConnection  *pThis;

    pThis  = (RtspServerConnection*)hConnection;

    /* checking parameters */
    if ((pThis == NULL) || (pCallbacks == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    if (sizeof(RvRtspServerConnectionCallbackFunctions) != callbacksStructSize)
    {
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionRegisterCallbacks\r\n"));

    /* copying the callback configuration */
    memcpy(&pThis->pfnCallbacks, pCallbacks, sizeof(RvRtspServerConnectionCallbackFunctions));

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionRegisterCallbacks\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}

/******************************************************************************
 * RvRtspServerConnectionSendRawBuffer
 * ----------------------------------------------------------------------------
 * General: This API sends an application raw buffer as is, on the the given 
 *          Rtsp connection. 
 *          This API can be used to send embedded binary data (such as rtp) 
 *          on the same TCP connection used by the rtsp.
 *          The encapsulated stream (such as an rtp packet), including the ASCII dollar
 *          sign (24 hexadecimal), followed by a one-byte channel identifier,
 *          followed by the length of the encapsulated binary data as a binary,
 *          two-byte integer in network byte order, must be fully provided by the
 *          application.
 *          
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:   hConnection   - The connection handle in the stack.
 *          pBuff         - The encapsulated buffer to be sent on the connection.
 *          buffSize      - The size of the buffer in bytes.
 *
 * Output: None.
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspServerConnectionSendRawBuffer(
        IN	RvRtspServerConnectionHandle    hConnection,
        IN  RvUint8                         *pBuff,
        IN  RvUint32                        buffSize)
{
    RtspServerConnection    *pThis          = (RtspServerConnection *)hConnection;
    HRPOOLELEM              hMessageElement = NULL;
    RvStatus                status          = RV_OK;

    if ((pThis == NULL) || (pBuff == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionSendRawBuffer\r\n"));

    /* Allocate and set the rpool element */
    hMessageElement = rpoolAllocCopyExternal(pThis->hRPool, 0, pBuff, buffSize);
    
    if (hMessageElement != NULL)
    {
        status = RtspTransportSend(&pThis->transport, hMessageElement);
        rpoolFree(pThis->hRPool, hMessageElement);
    }
    else
    {
        status = RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionSendRawBuffer status %d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return status;
}

/******************************************************************************
 * RvRtspServerConnectionReceiveRawBuffer
 * ----------------------------------------------------------------------------
 * General: This API simulates the receiving of a raw buffer chunk on a connection.
 *          This API can be used to push back into the stack a part of received 
 *          chunk that contains encapsulated stream at the beginning followed by
 *          an Rtsp message (or part of it).
 *          
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:   hConnection   - The connection handle in the stack.
 *          pBuff         - The raw buffer to be pushed back into the stack.
 *          buffSize      - The size of the raw buffer in bytes.
 *
 * Output: None.
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspServerConnectionReceiveRawBuffer(
        IN	RvRtspServerConnectionHandle    hConnection,
        IN  RvUint8                         *pBuff,
        IN  RvUint32                        buffSize)
{
    RtspServerConnection  *pThis          = (RtspServerConnection *)hConnection;
    RvStatus              status          = RV_OK;

    if ((pThis == NULL) || (pBuff == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSeverConnectionReceiveRawBuffer\r\n"));

    RtspTransportReceive(&pThis->transport, pBuff, buffSize);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionReceiveRawBuffer status %d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return status;
}

/******************************************************************************
 * RvRtspServerConnectionSendRequest
 * ----------------------------------------------------------------------------
 * General: 
 *  This method sends a request message on the connection.
 *
 * Arguments:
 * Input:   hConnection - The connection handle used to access the 
 *                         connection to send the request on.
 *          pRequest    - The request message to be sent on the connection.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionSendRequest(
    IN  RvRtspServerConnectionHandle      hConnection,
    IN  RvRtspRequest              		 *pRequest)
{
    RtspServerConnection *pThis;
    RvStatus             result;

    pThis                = (RtspServerConnection*)hConnection;
    result               = RV_OK;

    if ((pThis == NULL) || (pRequest == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionSendRequest\r\n"));

    /* Notify the application that a response is about to be sent */
    if (pThis->pfnCallbacks.pfnServerOnSendEv != NULL)
    {
        pThis->pfnCallbacks.pfnServerOnSendEv((RvRtspServerConnectionHandle)pThis,
                                        pThis->hApp, 
                                        pRequest,
                                        NULL);
        
    }
  
    result = RtspServerConnectionSendRequest(pThis, pRequest);

    if (result != RV_OK)
    {
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc,
                   "RvRtspServerConnectionSendRequest - send request failed\n"));
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionSendRequest\r\n"));

    return result;
}


/******************************************************************************
 * RvRtspServerConnectionSendResponse
 * ----------------------------------------------------------------------------
 * General: 
 *  This method sends an RvRtspResponse message object on the connection. The
 *  response object is populated according to the request received from the 
 *  client.
 *
 * Arguments:
 * Input:   hConnection     - The connection handle used to access the 
 *                            connection to send the response on.
 *          pResponse       - The response to be sent on the connection.
 * Output:  None.

 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 ******************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionSendResponse(
    IN  RvRtspServerConnectionHandle      hConnection,
    IN  RvRtspResponse              	  *pResponse)
{

    RtspServerConnection *pThis;
    RvStatus             result;

    pThis        = (RtspServerConnection *)hConnection;
    result       = RV_OK;

    if ((pThis == NULL) || (pResponse == NULL))
    {
       return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionSendResponse\r\n"));
  
    /* Notify the application that the message is about to be sent */
    if (pThis->pfnCallbacks.pfnServerOnSendEv != NULL)
    {
        pThis->pfnCallbacks.pfnServerOnSendEv(hConnection,
                                        pThis->hApp, 
                                        NULL,
                                        pResponse);
        
    }

    result = RtspServerConnectionSendResponse(pThis, pResponse);

    if (result != RV_OK)
    {
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc,
                   "RvRtspServerConnectionSendResponse - send response failed\n"));
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionSendResponse\r\n"));

    return result;
}


/******************************************************************************
 * RvRtspServerConnectionGetSessionById
 * ----------------------------------------------------------------------------
 * General: 
 *  This method returns the session corresponding to the session-id
 *  specified by the parameter strSessionId. This is used to determine 
 *  whether a session with the session ID provided already exists or not.
 *
 * Arguments:
 * Input:   hConnection     - handle to the Connection required to access
 *                            the connection on which to check for the 
 *                            session's existence. 
 *          strSessionId    - string containing the session's Session-Id.
 * Output:  phSession       - the session with the specified Session-Id
 *                            if found, or NULL if not found.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 ******************************************************************************/
RVAPI RvStatus RvRtspServerConnectionGetSessionById(
    IN RvRtspServerConnectionHandle   hConnection,
    IN  const RvChar                  *strSessionId,
    OUT RvRtspServerSessionHandle     *phSession)
{
    RtspServerConnection      *pThis;
    RtspServerSession         *pSession;

    pThis           = (RtspServerConnection *)hConnection;
    pSession        = NULL;
    

    if ((pThis        == NULL) ||
        (strSessionId == NULL) ||
        (phSession    == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    *phSession      = NULL;

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionGetSessionById\r\n"));

    pSession = (RtspServerSession*)pThis->hSessionList;

    while (pSession != NULL)
    {
        if (rpoolCompareExternal(
                                pSession->hRPool,
                                pSession->hSessionId,
                                (void *)strSessionId,
                                strlen(strSessionId)) == 0)
        {
            *phSession = (RvRtspServerSessionHandle)pSession;

            return RV_OK;
        }
		else
		{
			RvUint32 id1 = 0;
			RvUint32 id2 = 0;
			RvChar sessionStr[MAX_SESSIONID_LENGTH];
			memset(sessionStr, '\0', MAX_SESSIONID_LENGTH);

			/* Convert the hSessionId to a string  */
			RtspUtilsHPOOLELEMStrCpy(pSession->hRPool, pSession->hSessionId, 0, sessionStr, MAX_SESSIONID_LENGTH);

			id1 = atoi(strSessionId);
			id2 = atoi(sessionStr);
			if (id1==id2)
			{
				*phSession = (RvRtspServerSessionHandle)pSession;

				return RV_OK;
			}
		}

        pSession = pSession->pNext;
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerConnectionGetSessionById\r\n"));

    return RvRtspErrorCode(RV_ERROR_BADPARAM);
}

/**************************************************************************
 * RtspServerConnectionOnRawBufferReceiveEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer whenever a buffer
 *          is read from the socket.
 *          The callback notifies the application so that it can check
 *          if there is encapsulated binary stream in it.
 *
 * Return Value:    RV_OK if process of received buffer should continue
 *                  in the usual manner, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - the Connection on which the buffer was received.
 *              pBuff       -   The received buffer.
 *              buffSize    -   The received buffer size.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspServerConnectionOnRawBufferReceiveEv(
                            IN      void        *context,
                            IN      RvUint8     *pBuff,
                            IN      RvUint32    buffSize)
{
    RtspServerConnection *pThis = (RtspServerConnection*)context;
    RvStatus        status = RV_OK;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionOnRawBufferReceiveEv\r\n"));

    /* Notify the application about the raw buffer */
    if (pThis->pfnCallbacks.pfnOnRawBufferReceiveEv != NULL)
    {
        status = pThis->pfnCallbacks.pfnOnRawBufferReceiveEv((RvRtspServerConnectionHandle)pThis, 
                                                             pThis->hApp,
                                                             pBuff,
                                                             buffSize);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionOnRawBufferReceiveEv\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return status;
}

/**************************************************************************
 * RtspServerConnectionHandleResponse
 * ------------------------------------------------------------------------
 * General: 
 *  This method handles a response which was received on the
 *  connection. The method does not call the session callback as 
 *  no response is expected on the session.
 *
 * Arguments:
 * Input:   pThis       - The Connection on which the response was received.
 *          pResponse   - the received response.
 * Output:  None.
 *
 * Return Value: RV_OK - if successful.
 *               Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionHandleResponse(
    IN RtspServerConnection       *pThis,
    IN const RvRtspResponse       *pResponse)
{
 
    RvStatus result;

    result = RV_OK;

    if ((pThis == NULL) || (pResponse == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionHandleResponse\r\n"));
    

    /* Connection callback on message receive */
    if (pThis->pfnCallbacks.pfnServerOnReceiveEv!= NULL)
    {
        result = pThis->pfnCallbacks.pfnServerOnReceiveEv(
               (RvRtspServerConnectionHandle)pThis,
               (RvRtspServerConnectionAppHandle)pThis->hApp,
               RV_RTSP_MSG_TYPE_RESPONSE, NULL, (RvRtspResponse *)pResponse);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionHandleResponse\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return result;
}


/**************************************************************************
 * RtspServerConnectionHandleRequest
 * ------------------------------------------------------------------------
 * General: 
 *  This method handles a request from the client which was received on the
 *  connection. It calls either the connection on receive callback or the 
 *  session on recieve callback depending on whether a session exists or 
 *  not.
 *
 * Arguments:
 * Input:    pThis       - the Connection on which the request was received.
 *           pRequest    - the received request.
 * Output:   None.
 *
 * Return Value:    RV_OK - if successful.
 *                  Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionHandleRequest(
    IN RtspServerConnection       *pThis,
    IN const RvRtspRequest        *pRequest)
{
    RvStatus                  result;
    RvChar                    sessionStr[MAX_SESSIONID_LENGTH];
    RvRtspServerSessionHandle hSession;
    
    result = RV_OK;
    memset(sessionStr, '\0', MAX_SESSIONID_LENGTH);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionHandleRequest\r\n"));

    /* Convert the hSessionId to a string  */
    RtspUtilsHPOOLELEMStrCpy(pThis->hRPool, (HRPOOLELEM)pRequest->session.hSessionId, 0, 
                             sessionStr, MAX_SESSIONID_LENGTH);

    /* Check if a session with the sessionId exists already  */
    result = RvRtspServerConnectionGetSessionById((RvRtspServerConnectionHandle)pThis, sessionStr, &hSession);

    if (result != RV_OK)
    {
        /* Connection callback on message receive */
        if (pThis->pfnCallbacks.pfnServerOnReceiveEv!= NULL)
        {
            result = pThis->pfnCallbacks.pfnServerOnReceiveEv(
                      (RvRtspServerConnectionHandle)pThis,
                      (RvRtspServerConnectionAppHandle)pThis->hApp,
                      RV_RTSP_MSG_TYPE_REQUEST, (RvRtspRequest *)pRequest, NULL);
        }
    }
    else
    {
        RtspServerSessionHandleRequest((RtspServerSession *)hSession, (RvRtspRequest *)pRequest);
    }
    
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionHandleRequest\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return result;
}

/**************************************************************************
 * RtspServerConnectionOnReceive
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Transport layer after a message
 *  was received by it. It handles the received message via
 *  RtspServerConnectionHandleRequest/RtspServerConnectionHandleResponse
 *
 * Arguments:
 * Input:   context    - The connection on which the message
 *                       was received.
 *          pMessage   - The received transport message.
 * Output:  None.
 *
 * Return Value:   RV_OK - if successful.
 *                 Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionOnReceive(
    IN void                         *context,
    IN const RtspTransportMessage   *pMessage)
{

    RtspServerConnection *pThis ;
    RvStatus            result;
    RvRtspRequest       *pRequest;
    RvRtspResponse      *pResponse;
    RvBool              requestLineValid;
    RvBool              statusLineValid;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
    RtspMsgResponse *RtspMsgResponseMsg;
#else
    RvRtspRequest   request;
    RvRtspResponse  response;
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
    pResponse = &response;
    memset(pResponse, 0, sizeof(RvRtspResponse));
#endif

    pThis             = (RtspServerConnection*)context;
    result            = RV_OK;
    requestLineValid  = RV_FALSE;
    statusLineValid   = RV_FALSE;

    /* Checking parameters */
    if ((pThis == NULL) || (pMessage == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive\r\n"));

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    /* Request */
    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
        return RV_FALSE;
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;

    /* Response */
    raAdd(pThis->hRaResponses, (RAElement *)&RtspMsgResponseMsg);
    if (RtspMsgResponseMsg == NULL)
        return RV_FALSE;
    memset(RtspMsgResponseMsg, 0, sizeof(RtspMsgResponse));
    RtspMsgResponseMsg->hRtsp = pThis->hRtsp;
    RtspMsgResponseMsg->msgHeaders = NULL;
    RtspMsgResponseMsg->lastHeader = NULL;
    pResponse = &RtspMsgResponseMsg->response;
    pResponse->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgResponseMsg;
#endif
    RtspMessagesRequestConstruct(pRequest);
    RtspMessagesResponseConstruct(pResponse);

    /* Receive the message on the connection  */
    result = RtspFirstLineDeserialize(pThis->hRPool,
                                      pMessage->hFirstLine,
                                      &pRequest->requestLine,
                                      &pResponse->statusLine,
                                      &requestLineValid, &statusLineValid);
    if (result != RV_OK)
    {
        RvLogDebug(pThis->pLogSrc,(pThis->pLogSrc,
                                  "RtspServerConnectionOnReceive Error On First Line\n"));
    }
    else if (requestLineValid == RV_TRUE) /* Message received is a request */
    {

        /* Deserialize request */
        result = RtspMessagesRequestDeserialize(pThis->hRPool,
                    &((RtspTransportMessage *)pMessage)->headersQueue,
                    pRequest);
            
        if (result == RV_OK)
        {
            RtspServerConnectionHandleRequest(pThis, pRequest);
        }
        else
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive Error On Request\r\n"));
        }
    }

    else if (statusLineValid == RV_TRUE)
    {
        /* deserialize response */
        result = RtspMessagesResponseDeserialize(pThis->hRPool,
                                                &((RtspTransportMessage *)pMessage)->headersQueue,
                                                pMessage->hBody,
                                                0, /* No RTP info streams are expected at server */
                                                pResponse);
        
        if (result == RV_OK)
        {
            RtspServerConnectionHandleResponse(pThis, pResponse);
            /* protection against deletion in the above event */
            RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
            if (pThis->destructed == RV_TRUE)
            {
                RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
                RtspMessagesResponseDestruct(pThis->hRPool, pResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
                raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
                raDelete(pThis->hRaResponses, (RAElement)RtspMsgResponseMsg);
#endif
                RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
                return RV_OK;
            }
            RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        }
        else
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive Error On Response\r\n"));
        }

    }

    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
    RtspMessagesResponseDestruct(pThis->hRPool, pResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
                raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
                raDelete(pThis->hRaResponses, (RAElement)RtspMsgResponseMsg);
#endif

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionOnReceive\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return result;
}


/**************************************************************************
 * RtspServerConnectionOnAccept
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Transport layer after a TCP
 *  connection is accepted by it. 
 *
 * Arguments:
 * Input:    context        - The connection handle.
 *           newSocket      - The new socket.
 *           strClientIP    - Client IP.
 *           portNum        - Client port.
 *           success        - Indicates if the connection was constructed
 *                            successfully.
 * Output:   success        - Indicates if the connection was constructed
 *                            successfully.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionOnAccept(
    IN void      *context,
    IN RvSocket  newSocket,
    IN RvChar    *strClientIP,
    IN RvUint16  portNum,
    INOUT RvBool *success)
{

    RvStatus             result;
    RtspServerConnection *pThis;
  
    result    = RV_OK;
    pThis     = (RtspServerConnection *)context;

    if ((pThis == NULL) || (strClientIP == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionOnAccept\r\n"));

    if (pThis->pfnCallbacks.pfnServerOnAcceptEv != NULL)
    {
        result = pThis->pfnCallbacks.pfnServerOnAcceptEv(
               (RvRtspServerConnectionHandle)pThis, pThis->hApp,
               newSocket, strClientIP, portNum, success);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionOnAccept\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return result;

}


/**************************************************************************
 * RtspServerConnectionOnDisconnect
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Transport layer after a TCP
 *  connection between the client and the server is disconnected.
 *
 * Arguments:
 * Input:    context     - Handle to the connection that has been 
 *                         disconnected.
 * Output:   None.
 *
 * Return Value:   RV_OK - if successful.
 *                 Negative values otherwise.
 *************************************************************************/
static RvStatus RtspServerConnectionOnDisconnect(
    IN void *context)
{

    RvStatus             result;
    RtspServerConnection *pThis;

    result   = RV_OK;
    pThis    = (RtspServerConnection*)context;

    /* checking parameters                          */
    if (pThis == NULL)
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDisconnect\r\n"));

    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDisconnect\r\n"));

    /* perform upper layer disconnecting operations */
    if (pThis->pfnCallbacks.pfnServerOnDisconnectEv != NULL)
    {
        result = pThis->pfnCallbacks.pfnServerOnDisconnectEv(
                            (RvRtspServerConnectionHandle)pThis,
                            (RvRtspServerConnectionAppHandle)pThis->hApp);
    }
    return result;

}


/**************************************************************************
 * RtspServerConnectionSendResponse
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends an RvRtspResponse message on the connection.
 *  (this is an internal function, to be used by the connection,
 *  and by sessions).
 *
 * Arguments:
 * Input:   pThis       - The connection on which to send the request.
 *          pResponse   - The message to send on the connection.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerConnectionSendResponse(
    IN  RtspServerConnection      *pThis,
    IN  const RvRtspResponse      *pResponse)
{
    HRPOOLELEM           hMessageElement;
    RvStatus             result;

    hMessageElement = NULL;
    result          = RV_OK;

    if ((pThis == NULL) || (pResponse == NULL))
    {
       return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionSendResponse\r\n"));
    
    /* send the request message                                               */
    result = RtspMessagesResponseSerialize(pThis->hRPool, 
                                 (RvRtspResponse*)pResponse, &hMessageElement);

    if (result == RV_OK)
    {
        result = RtspTransportSend(&pThis->transport, hMessageElement);
    }
    
    /* if the message was sent on the transport it will be handled by it,     */
    /* otherwise we need to free it ourselves.                                */
    if ((result != RV_OK) && (hMessageElement != NULL))
    {
        rpoolFree(pThis->hRPool, hMessageElement);
    }
    
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionSendResponse\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return result;
}


/**************************************************************************
 * RtspServerConnectionSendRequest
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends an RvRtspRequest message on the connection.
 *  (this is an internal function, to be used by the connection,
 *  and by sessions).
 *
 * Arguments:
 * Input:   pThis       - The connection on which to send the request.
 *          pRequest    - The message to send on the connection.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerConnectionSendRequest(
    IN  RtspServerConnection      *pThis,
    IN  const RvRtspRequest       *pRequest)

{
    HRPOOLELEM           hMessageElement;
    RvStatus             result;

    hMessageElement      = NULL;
    result               = RV_OK;

    if ((pThis == NULL) || (pRequest == NULL))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }
    
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->state != RTSP_CONNECTION_STATE_CONSTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionSendRequest\r\n"));

    /* serialize the message for sending                                      */
    result = RtspMessagesRequestSerialize(pThis->hRPool, 
                                   (RvRtspRequest*)pRequest, &hMessageElement);

    /* send the response message                                              */
    if (result == RV_OK)
    {
        result = RtspTransportSend(&pThis->transport, hMessageElement);
    }
    
    /* if we sent the message element successfully, it will be freed in       */
    /* transport, otherwise, we must free it here.                            */
    if ((result != RV_OK) && (hMessageElement != NULL))
    {
        rpoolFree(pThis->hRPool, hMessageElement);
    }
    
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerConnectionSendRequest\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return result;
}

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#if defined(__cplusplus)
}
#endif

