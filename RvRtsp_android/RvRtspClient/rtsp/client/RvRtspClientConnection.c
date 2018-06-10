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
 *                              <RvRtspClientConnection.c>
 *
 *  This file contains definitions relevant to the RTSP server connections.
 *  An RTSP Connection instance is a thread safe representation of a connection
 *  to an RTSP server, handling all RTSP communication to and from the server.
 *
 *    Author                         Date
 *    ------                        ------
 *      Shaft                       8/1/04
 *		Shaft						14/05/2006	- changed due new ccore 1.1.12
 *												- DNS interface has changed
 *
 *********************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER)
#include "rvcbase.h"
#include "rvstdio.h"
#include "rvares.h"
#include "ra.h"

#include "RtspClientConnectionInternal.h"
#include "RvRtspClientSession.h"
#include "RtspClientSessionInternal.h"
#include "RtspTransport.h"
#include "RtspFirstLine.h"
#include "RtspMessages.h"
#include "RtspUtilsInternal.h"
#include "RtspHeaders.h"
#include "RtspMsgInternal.h"


/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspConnectionOnConnect
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after a
 *          connection attempt has either succeeded or failed.
 *          The callback notifies the user when (and only when) the
 *          connection has been established.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context         - the Connection which was connected.
 *              success         - has the connection succeeded.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnConnect(
                        IN void     *context,
                        IN RvBool   success);


/**************************************************************************
 * RtspConnectionOnDisconnect
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after the user
 *          called RvRtspConnectionDisconnect.
 *          The callback notifies the user that the connection has been
 *          disconnected.
 *
 * Return Value:    RV_OK if successful, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context - the Connection which is going to be diconnected.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnDisconnect(
                        IN void *context);


/**************************************************************************
 * RtspConnectionOnRawBufferReceiveEv
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
static RvStatus RtspConnectionOnRawBufferReceiveEv(
                            IN      void        *context,
                            IN      RvUint8     *pBuff,
                            IN      RvUint32    buffSize);

/**************************************************************************
 * RtspConnectionHandleRequest
 * ------------------------------------------------------------------------
 * General: This method handles a request which was received on the
 *          connection. It responds with an OK for a server ping
 *          (SET_PARAMETER) or REDIRECT, or with a NOT_IMPLEMENTED response
 *          otherwise.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the Connection on which the request was received.
 *              pRequest    - the received request.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionHandleRequest(
                        IN RtspConnection       *pThis,
                        IN const RvRtspRequest  *pRequest);


/**************************************************************************
 * RtspConnectionHandleResponse
 * ------------------------------------------------------------------------
 * General: This method handles a response which was received on the
 *          connection.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - The Connection on which the response was received.
 *              pResponse   - the received response.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionHandleResponse(
                        IN RtspConnection       *pThis,
                        IN const RvRtspResponse *pResponse);


/**************************************************************************
 * RtspConnectionOnReceive
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after a message
 *          was received by it. it handles the received message via
 *          RtspConnectionHandleRequest or RtspConnectionHandleResponse
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context             - the connection on which the message
 *                                  was received.
 *              pTransportMessage   - The received transport message.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnReceive(
                        IN void                         *context,
                        IN const RtspTransportMessage   *pTransportMessage);



/**************************************************************************
 * RvRtspConnectionOnContentLength
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after a header has
 *          been parsed to find the content length of the message.
 *          Note: if content length is found pContentLength is set to the
 *          length, otherwise, it is not changed.
 *
 * Return Value:    RV_OK if content length is found, RV_ERROR_BADPARAM
 *                  otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context         - The header's connection.
 *              headerLine      - The header line.
 * OUTPUT   :   pContentLength  - The length of the message body.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnContentLength(
                                IN  void        *context,
                                IN  HRPOOLELEM  headerLine,
                                OUT RvSize_t    *pContentLength);



/**************************************************************************
 * RtspConnectionConnectToServer
 * ------------------------------------------------------------------------
 * General: This method connects to the IP address specified in the
 *          DNS results RA.
 *          (this is an internal function used by RtspConnectionOnDnsResponse
 *          and RtspConnectionOnConnect).
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the connection to connect.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionConnectToServer(
                        IN  RtspConnection      *pThis);


/**************************************************************************
 * RtspConnectionGetSessionByLastCSeq
 * ------------------------------------------------------------------------
 * General: This method search for the session that match the cseq
 *          parameter. It tries to match between the last cseq session
 *          request and the cseq parameter.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the connection to connect.
 *              cseq        - cseq to match
 * OUTPUT   :   ppSession   - an address to session object pointer.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionGetSessionByLastCSeq(
                        IN  RtspConnection      *pThis,
                        IN  RvUint16            cseq,
                        OUT RtspSession         **ppSession);



/**************************************************************************
 * RtspConnectionSendResponse
 * ------------------------------------------------------------------------
 * General: This method sends an RvRtspResponse message on the connection.
 *          This method is internally used by the Connection layer to
 *          automatically reply to requests from the server.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis           - The connection to send the response on.
 *              pResponse       - The response to send on the connection.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionSendResponse(
                        IN  RtspConnection          *pThis,
                        IN  const RvRtspResponse    *pResponse);


static RvStatus RvRtspAllocateAndSendQuery(
						IN RtspConnection*   pThis,
                        IN RvDnsEngine*      pDnsEngine,
                        IN RvDnsQueryType    DnsQueryType,
                        IN RvChar*           dnsName,
                        IN void*             context,
                        IN RvUint32*         queryId);

static RvBool RtspConnectionDescribeOnResponseTimer(
                                IN void *context);


/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */


/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RtspConnectionConstruct
 * ------------------------------------------------------------------------
 * General: This method constructs a connection, it is called by the Rtsp
 *          layer after setting the connection parameters in the connection
 *          structure.
 *          Note: The first time RtspConnectionConstruct is called, the
 *          connection is constructed, each additional time the function is
 *          called, only it's reference count is increased.
 *
 * Return Value:    RV_OK - If the connection is successfully initialized
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - The connection to be constructed.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspConnectionConstruct(
            IN  RtspConnection  *pThis)
{
    RtspTransportCallbackFunctions  transportCallbacks;
    RvStatus                        result;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionConstruct\r\n"));

    /* initializing the RtspConnection data structure */
    pThis->referenceCount   = 1;
    memset(&pThis->pfnCallbacks, 0, sizeof(pThis->pfnCallbacks));

    /* Create guard Mutex */
    result = RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);
    if ( result != RV_OK)
    {
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionConstruct - Mutex construction failed\r\n"));
        return result;
    }

    /* construct the transport */
    pThis->transport.pLogMgr        = pThis->pLogMgr;
    pThis->transport.pLogSrc        = pThis->pLogSrc;
    pThis->transport.context        = (void*)pThis;
    pThis->transport.hRPool         = pThis->hRPool;
    pThis->transport.pSelectEngine  = pThis->pSelectEngine;
    pThis->transport.pGMutex        = pThis->pGMutex;
    pThis->transport.transportState = RTSP_TRANSPORT_STATE_DESTRUCTED;

    result = RtspTransportConstruct(&pThis->transport,
                                    pThis->configuration.transmitQueueSize,
                                    pThis->configuration.maxHeadersInMessage);

    /* callback registration - callbacks will be called on events */
    transportCallbacks.pfnOnConnectEv           = RtspConnectionOnConnect,
    transportCallbacks.pfnOnDisconnectEv        = RtspConnectionOnDisconnect,
    transportCallbacks.pfnOnReceiveEv           = RtspConnectionOnReceive,
    transportCallbacks.pfnOnContentLengthEv     = RtspConnectionOnContentLength;
    transportCallbacks.pfnOnRawBufferReceiveEv  = RtspConnectionOnRawBufferReceiveEv;
    RtspTransportRegisterCallbacks( &pThis->transport, &transportCallbacks);

    /* initializing the waiting describe requests   */
    pThis->hWaitingDescribeRequests = raConstruct(
            sizeof(RtspWaitingDescribeRequest),         /* elements size            */
            pThis->configuration.maxWaitingDescribeRequests,/* max number of elements*/
            RV_FALSE,                                   /* thread safety            */
            "Describe",                                 /* RA name                  */
            pThis->pLogMgr);                            /* log manager              */

    /* initializing the DNS results RA (RA containing IP addresses of the           */
    /* requested domain name resolution                                             */
    pThis->hDNSResults = raConstruct(
            sizeof(RvAddress),                          /* elements size            */
            pThis->configuration.dnsMaxResults,         /* max number of elements   */
            RV_FALSE,                                   /* thread safety            */
            "DNS",                                      /* RA name                  */
            pThis->pLogMgr);                            /* log manager              */

    pThis->nextCSeq = 1;
    pThis->state    = RTSP_CONNECTION_STATE_CONSTRUCTED;

    pThis->hSessions = raConstruct(
            sizeof(RtspSession),                        /* elements size            */
            pThis->configuration.maxSessions,           /* max number of elements   */
            RV_FALSE,                                   /* thread safety            */
            "SESSIONS",                                 /* RA name                  */
            pThis->pLogMgr);                            /* log manager              */

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionConstruct\r\n"));

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    return RV_OK;
}



/**************************************************************************
 * RtspConnectionDestruct
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
 * INPUT    :   hConnection             - the destructed connection.
 *              destructAllReferences   - destruct even if connection count
 *                                        indicates other references.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspConnectionDestruct(
                IN  RvRtspConnectionHandle  hConnection,
                IN  RvBool                  destructAllReferences)
{
    RtspConnection              *pThis = (RtspConnection*)hConnection;
    int                         location;
    RtspWaitingDescribeRequest  *pWaitingRequest;
    RtspSession                 *pSession;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionDestruct\r\n"));

    /* if this is a regular destruct and we still have more references, */
    /* we'll only decrease the reference count                          */
    if ((destructAllReferences == RV_FALSE) && (pThis->referenceCount > 1))
    {
        pThis->referenceCount--;
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionDestruct\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RV_OK;
    }

    pThis->state            = RTSP_CONNECTION_STATE_DESTRUCTED;
    pThis->referenceCount   = 0;

    /* destroy the sessions                     */
    location = raGetNext(pThis->hSessions, -1);
    while (location >= 0)
    {
        pSession = (RtspSession*) raGet(pThis->hSessions, location);
        RvRtspSessionDestruct((RvRtspSessionHandle)pSession);
        location = raGetNext(pThis->hSessions, location);
    }
    raClear(pThis->hSessions);
    raDestruct(pThis->hSessions);
    pThis->hSessions = NULL;

    /* destroy the transport                    */
    RtspTransportDestruct(&pThis->transport);


    /* destroy the waiting describe requests    */
    location = raGetNext(pThis->hWaitingDescribeRequests, -1);
    while (location >= 0)
    {
        pWaitingRequest = (RtspWaitingDescribeRequest*) raGet(pThis->hWaitingDescribeRequests, location);
        if (pWaitingRequest->responseTimerSet == RV_TRUE)
            //modified by lichao, 20150119 解决计时器回调中内存被踩的问题
            RvTimerCancel(&pWaitingRequest->responseTimer, RV_TIMER_CANCEL_WAIT_FOR_CB);
        rpoolFree(pThis->hRPool, pWaitingRequest->hURI);
        location = raGetNext(pThis->hWaitingDescribeRequests, location);
    }
    raClear(pThis->hWaitingDescribeRequests);
    raDestruct(pThis->hWaitingDescribeRequests);
    pThis->hWaitingDescribeRequests = NULL;

    /* destroy the dns results RA               */
    raClear(pThis->hDNSResults);
    raDestruct(pThis->hDNSResults);
    pThis->hDNSResults = NULL;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionDestruct\r\n"));

    /* Destruct the guard Mutex                 */
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    return RV_OK;
}



/**************************************************************************
 * RvRtspConnectionRegisterCallbacks
 * ------------------------------------------------------------------------
 * General: This method registers the connection object's callbacks.
 *
 * Return Value:    RV_OK - If the method is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection         - The connection.
 *              pCallbacks          - callback functions to be called when
 *                                    events occur on the connection.
 *              callbacksStructSize - size of callbacks structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionRegisterCallbacks(
                        IN  RvRtspConnectionHandle                  hConnection,
                        IN  const RvRtspConnectionCallbackFunctions *pCallbacks,
                        IN  const RvSize_t                          callbacksStructSize)
{
    RtspConnection  *pThis = (RtspConnection*)hConnection;

    /* checking parameters              */
    if ((pThis == NULL) || (pCallbacks == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if (sizeof(RvRtspConnectionCallbackFunctions) != callbacksStructSize)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRegisterCallbacks\r\n"));

    /* copying the callback configuration                           */
    memcpy(&pThis->pfnCallbacks, pCallbacks, sizeof(RvRtspConnectionCallbackFunctions));

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRegisterCallbacks\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}


/**************************************************************************
 * RvRtspConnectionConnect
 * ------------------------------------------------------------------------
 * General: This method connects the connection to the server over the
 *          transport layer.
 *          Note: the method sends a DNS resolution request, and when the
 *          IP address is received the connection connects to the server.
 *
 *          Upon successful (or not) connecting the connection layer will
 *          call RvRtspConnectionOnConnectEv callback.
 *
 * Return Value:    RV_OK - If the connection is successfully connected
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection         - The connection handle.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionConnect(
                        IN  RvRtspConnectionHandle  hConnection)
{
    RtspConnection  *pThis = (RtspConnection*) hConnection;
    RvStatus        result;
    RvAddress       serverAddress;
    RvUint32        queryId;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect\r\n"));

    if (pThis->state != RTSP_CONNECTION_STATE_CONSTRUCTED)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect - Connection already connected\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    /* setting the connection variables */
    pThis->nextCSeq         = 1;
    pThis->state            = RTSP_CONNECTION_STATE_DNS_QUERY;

    /* trying to construct a server address, in case the address is already */
    /* an IP address, and not host name                                     */
    RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &serverAddress);
    if (RvAddressSetString(pThis->strServerName, &serverAddress) != NULL)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect - Address is already an IP4 address\r\n"));
        RtspConnectionOnDnsResponse(pThis, &serverAddress);

        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        return RV_OK;
    }


    RvAddressConstruct(RV_ADDRESS_TYPE_IPV6, &serverAddress);
    if (RvAddressSetString(pThis->strServerName, &serverAddress) != NULL)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect - Address is already an IP6 address\r\n"));
        RtspConnectionOnDnsResponse(pThis, &serverAddress);

        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        return RV_OK;
    }

    /* let's try to query the DNS                                   */
    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect - Query DNS Sent\r\n"));

    /* RtspConnectionOnDnsResponse will set the server's IP address and     */
    /* call the RtspTransportConnect
    */
    //begin: deleted by lichao, 20140109 fix bug:Invalid write of size 8
    // dns解析部分还不稳定，先屏蔽掉
    result = RvRtspErrorCode(RV_ERROR_NETWORK_PROBLEM);
#if 0
	result = RvRtspAllocateAndSendQuery(pThis,	
                                pThis->pDnsEngine,
                                RV_DNS_HOST_IPV4_TYPE,
                                pThis->strServerName,
                                (void*)pThis,
                                &queryId);

    result = RvRtspAllocateAndSendQuery(pThis,
                                pThis->pDnsEngine,
                                RV_DNS_HOST_IPV6_TYPE,
                                pThis->strServerName,
                                (void*)pThis,
                                &queryId);

#endif
    //end
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionConnect\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	return result;
}


/**************************************************************************
 * RvRtspConnectionDisconnect
 * ------------------------------------------------------------------------
 * General: This method disconnects the connection from the server.
 *          destroying any sessions present on the connection.
 *
 *          Upon successful (or not) disconnecting the connection layer will
 *          call RvRtspConnectionOnDisconnectEv callback.
 *
 *
 * Return Value:    RV_OK if completed successfully, negative value is
 *                  returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection     - the destructed connection.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionDisconnect(
                        IN  RvRtspConnectionHandle  hConnection)
{
    RtspConnection              *pThis = (RtspConnection*)hConnection;
    RtspWaitingDescribeRequest  *pWaitingRequest;
    RvStatus                    result;
    int                         location;
    RtspSession                 *pSession;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionDisconnect\r\n"));

    if ((pThis->state != RTSP_CONNECTION_STATE_CONNECTED) &&
        (pThis->state != RTSP_CONNECTION_STATE_CONNECTING) &&
        (pThis->state != RTSP_CONNECTION_STATE_DNS_QUERY))
    {
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionDisconnect\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_OK;
    }

    /* destroy the sessions                 */
    location = raGetNext(pThis->hSessions, -1);
    while (location >= 0)
    {
        pSession = (RtspSession*) raGet(pThis->hSessions, location);
        RvRtspSessionDestruct((RvRtspSessionHandle)pSession);
        location = raGetNext(pThis->hSessions, location);
    }
    raClear(pThis->hSessions);

    /* clear the waiting requests RA        */
    location = raGetNext(pThis->hWaitingDescribeRequests, -1);
    while (location >= 0)
    {
        pWaitingRequest = (RtspWaitingDescribeRequest*) raGet(pThis->hWaitingDescribeRequests, location);
        rpoolFree(pThis->hRPool, pWaitingRequest->hURI);
        location = raGetNext(pThis->hWaitingDescribeRequests, location);
    }

    raClear(pThis->hWaitingDescribeRequests);

    /* clear the dns results RA         */
    raClear(pThis->hDNSResults);

    pThis->state = RTSP_CONNECTION_STATE_DISCONNECTING;

    /* perform the transport diconnection*/
    result = RtspTransportDisconnect(&pThis->transport);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionDisconnect\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return result;
}


/**************************************************************************
 * RvRtspConnectionTeardownAllSessions
 * ------------------------------------------------------------------------
 * General: This method disconnects all the sessions on the connection.
 *
 * Return Value:    RV_OK - If the function completes successfully,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection     - the connection who sessions we teardown.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionTeardownAllSessions(
                        IN  RvRtspConnectionHandle  hConnection)
{
    RtspConnection  *pThis = (RtspConnection*)hConnection;
    RtspSession     *pSession;
    int             location;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionTeardownAllSessions\r\n"));

    /* go over the sessions RA and call teardown for each of the sessions   */
    location = raGetNext(pThis->hSessions, -1);
    while (location >= 0)
    {
        pSession = (RtspSession*) raGet(pThis->hSessions, location);
        RvRtspSessionTeardown((RvRtspSessionHandle) pSession);
        location = raGetNext(pThis->hSessions, location);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionTeardownAllSessions\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}

/**************************************************************************
 * RvRtspConnectionGetIPAddress
 * ------------------------------------------------------------------------
 * General: This method returns the IP address of the connected server
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection     - handle to the Connection the session on.
 *              bufferSize      - size of input buffer.
 * OUTPUT   :   buffer          - output buffer where the IP address will be copied to.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetIPAddress(
                        IN  RvRtspConnectionHandle  hConnection,
                        IN  RvUint16                bufferSize,
                        OUT RvChar                  *buffer)
{
    RtspConnection  *pThis = (RtspConnection*)hConnection;

    if (pThis   == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionGetIPAddress\r\n"));

    RvAddressGetString(&pThis->serverAddress, bufferSize, buffer);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionGetIPAddress\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetPort(
    IN	RvRtspConnectionHandle	hConnection,
    OUT	RvUint16			    *nPort)
{
    RtspConnection  *pThis = (RtspConnection*)hConnection;

    if (pThis   == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionGetIPAddress\r\n"));

    *nPort = RvAddressGetIpPort(&pThis->serverAddress);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionGetIPAddress\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}


/**************************************************************************
 * RvRtspConnectionGetSessionById
 * ------------------------------------------------------------------------
 * General: This method returns the session corresponding to the session-id
 *          specified by the parameter pSessionId.
 *          Note: this method goes over the sessions array sequentially
 *          in order to find the session, it is therefore not time
 *          efficient (O(n)) and so it is not recommended as the normal
 *          way of accessing the sessions.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection     - handle to the Connection the session on.
 *              strSessionId    - string containing the session's Session-Id.
 * OUTPUT   :   phSession       - the session with the specified Session-Id
 *                              if found, or NULL if not found.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetSessionById(
                        IN  RvRtspConnectionHandle  hConnection,
                        IN  const RvChar            *strSessionId,
                        OUT RvRtspSessionHandle     *phSession)
{
    RtspConnection  *pThis = (RtspConnection*)hConnection;
    RtspSession     *pSession;
    int             location;

    if ((pThis          == NULL) ||
        (strSessionId   == NULL) ||
        (phSession      == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionGetSessionById\r\n"));

    /* destroy the sessions                                 */
    *phSession = NULL;
    location = raGetNext(pThis->hSessions, -1);
    while (location >= 0)
    {
        pSession = (RtspSession*) raGet(pThis->hSessions, location);

        if (rpoolCompareExternal(
                                pThis->hRPool,
                                pSession->hSessionId,
                                (RvChar*)strSessionId,
                                strlen(strSessionId)) == 0)
        {
            *phSession = (RvRtspSessionHandle)pSession;
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            return RV_OK;
        }

        location = raGetNext(pThis->hSessions, location);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionGetSessionById\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RvRtspErrorCode(RV_ERROR_BADPARAM);
}


/**************************************************************************
 * RvRtspConnectionRequestDescribe
 * ------------------------------------------------------------------------
 * General: This method sends a describe request on the connection.
 *
 *          Upon successful request and response from the server the
 *          connection layer will call RvRtspConnectionOnDescribeResponseEv
 *          callback.
 *
 * Return Value:    RV_OK if the function was successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection     - The connection on which to send the request.
 *              strURI          - The URI to be described.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionRequestDescribe(
                        IN  RvRtspConnectionHandle              hConnection,
						IN	const RvChar						*strURI,
						IN	RvRtspConnectionDescribeAppHandle	hDescribe)
{
    RtspConnection              *pThis = (RtspConnection*)hConnection;
    RtspWaitingDescribeRequest  *pWaitingRequest;
    RvRtspRequest               *pRequest;
    RvTimerFunc         timeoutCallback;	/* select engine's timeout function	*/
    RvTimerQueue        *pTQueue;               /* select engine's timer queue      */
    RvInt32               location;



#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
    
    if ((pThis == NULL) || (strURI== NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
        return RV_ERROR_NULLPTR;
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
#else
    RvRtspRequest   request;

    if ((pThis == NULL) || (strURI== NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state != RTSP_CONNECTION_STATE_CONNECTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRequestDescribe\r\n"));

    location = raAdd(pThis->hWaitingDescribeRequests, (RAElement*)&pWaitingRequest);
    if (location  < 0)
    {
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRequestDescribe - out of resources\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    /* setting the message's request line   */
    pRequest->requestLine.method  = RV_RTSP_METHOD_DESCRIBE;
    pRequest->requestLine.hURI    = (RvRtspStringHandle)
                                    rpoolAllocCopyExternal(
                                                        pThis->hRPool,
                                                        0,
                                                        strURI,
                                                        strlen(strURI)+1);

    /* setting the messsage's CSeq header   */
    pRequest->cSeq.value          = pThis->nextCSeq++;
    pRequest->cSeqValid           = RV_TRUE;

    /* setting the message's Accept header  */
    pRequest->accept.hStr         = (RvRtspStringHandle)
                                    rpoolAllocCopyExternal(
                                                        pThis->hRPool,
                                                        0,
                                                        "application/sdp",
                                                        strlen("application/sdp")+1);
    pRequest->acceptValid         = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    pRequest->accept.additionalFields = NULL;
#endif

    /* the rest of the headers are null     */
    pRequest->sessionValid        = RV_FALSE;
    pRequest->rangeValid          = RV_FALSE;
    pRequest->transportValid      = RV_FALSE;
    pRequest->locationValid       = RV_FALSE;

    /*add by kongfd*/
    if(pThis->Authenticate.isUser)
    {
    	    if(pThis->Authenticate.isDigest)
    	    {
    	    	   RvChar temp_buff[1024];
    	    	   sprintf(temp_buff,"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"1c4696c41a9fde56b6107dd4db1dc680\""
    	    	   	,pThis->Authenticate.username,pThis->Authenticate.Digestrealm,pThis->Authenticate.Digestnonce,strURI);
    	    	     /* setting the messsage's Authorization:  header   */
		    pRequest->auth.hStr          = (RvRtspStringHandle) rpoolAllocCopyExternal(
		                                                        pThis->hRPool,
		                                                        0,
		                                                        temp_buff,
		                                                        strlen(temp_buff)+1);
    	    }
    	    else
    	    {
		    /* setting the messsage's Authorization:  header   */
		    RvChar temp_buff[1024];
		    RvChar *temp_buff3=NULL;
		    sprintf(temp_buff,"%s:%s",pThis->Authenticate.username,pThis->Authenticate.password);
		    temp_buff3=DataToBase64(temp_buff,strlen(temp_buff));
		    memset(temp_buff,0,sizeof(temp_buff));
		    if(temp_buff3 !=NULL)
		    {
		    	sprintf(temp_buff,"Basic %s",temp_buff3);
		    	free(temp_buff3);
		    }
		    
		    pRequest->auth.hStr          = (RvRtspStringHandle) rpoolAllocCopyExternal(
		                                                        pThis->hRPool,
		                                                        0,
		                                                       temp_buff,
		                                                        strlen(temp_buff)+1);
	    }
	    pRequest->authValid           = RV_TRUE;
    }
    /*add end*/
    

    /* Notify the application that the message is about to be sent */
    if (pThis->pfnCallbacks.pfnOnSendEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnSendEv(hConnection,
                                        pThis->hApp, 
                                        pRequest,
                                        NULL);
        
    }

    RtspConnectionSendRequest(pThis, pRequest);

    pWaitingRequest->cSeq       = pRequest->cSeq.value;
    pWaitingRequest->hURI       = rpoolAllocCopyExternal(pThis->hRPool,
                                                         0,
                                                         strURI,
                                                         strlen(strURI)+1);
	pWaitingRequest->hDescribe	= hDescribe;
    pWaitingRequest->timeoutContext.location = location;
    pWaitingRequest->timeoutContext.connection = (void *)pThis;

    /* set the response timer */
    /* setting response timer - timeout is set according to configuration   */
    RvSelectGetTimeoutInfo(pThis->pSelectEngine, &timeoutCallback, &pTQueue);
    RvTimerStart(&pWaitingRequest->responseTimer,       /* timer object            */
        pTQueue,                                        /* timer's queue            */
        RV_TIMER_TYPE_ONESHOT,                          /* timer type               */
        pThis->configuration.describeResponseTimeOut*   /* configured describe timeout (in ms)*/
        RV_TIME64_NSECPERMSEC,                          /* convert to nanoseconds */
        RtspConnectionDescribeOnResponseTimer,           /* callback to call         */
        (void*)&pWaitingRequest->timeoutContext);        /* context for the timer    */
    pWaitingRequest->responseTimerSet = RV_TRUE;
    
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRequestDescribe\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}


RVAPI
RvStatus RVCALLCONV RvRtspConnectionRequestDescribeEx(
    IN  RvRtspConnectionHandle              hConnection,
    IN	const RvChar						*strURI,
    IN  RvUint16                            cseq,
    IN	RvRtspConnectionDescribeAppHandle	hDescribe)
{
    RtspConnection              *pThis = (RtspConnection*)hConnection;
    RtspWaitingDescribeRequest  *pWaitingRequest;
    RvRtspRequest               *pRequest;
    RvTimerFunc         timeoutCallback;	/* select engine's timeout function	*/
    RvTimerQueue        *pTQueue;               /* select engine's timer queue      */
    RvInt32               location;



#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;

    if ((pThis == NULL) || (strURI== NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
        return RV_ERROR_NULLPTR;

    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
#else
    RvRtspRequest   request;

    if ((pThis == NULL) || (strURI== NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state != RTSP_CONNECTION_STATE_CONNECTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRequestDescribe\r\n"));

    location = raAdd(pThis->hWaitingDescribeRequests, (RAElement*)&pWaitingRequest);
    if (location  < 0)
    {
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRequestDescribe - out of resources\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    /* setting the message's request line   */
    pRequest->requestLine.method  = RV_RTSP_METHOD_DESCRIBE;
    pRequest->requestLine.hURI    = (RvRtspStringHandle)
        rpoolAllocCopyExternal(
        pThis->hRPool,
        0,
        strURI,
        strlen(strURI)+1);

    /* setting the messsage's CSeq header   */
    pRequest->cSeq.value          = cseq;
    pRequest->cSeqValid           = RV_TRUE;

    /* setting the message's Accept header  */
    pRequest->accept.hStr         = (RvRtspStringHandle)
        rpoolAllocCopyExternal(
        pThis->hRPool,
        0,
        "application/sdp",
        strlen("application/sdp")+1);
    pRequest->acceptValid         = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES) 
    pRequest->accept.additionalFields = NULL;
#endif

    /* the rest of the headers are null     */
    pRequest->sessionValid        = RV_FALSE;
    pRequest->rangeValid          = RV_FALSE;
    pRequest->transportValid      = RV_FALSE;
    pRequest->locationValid       = RV_FALSE;

    /*add by kongfd*/
    if(pThis->Authenticate.isUser)
    {
        if(pThis->Authenticate.isDigest)
        {
            RvChar temp_buff[1024];
            sprintf(temp_buff,"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"1c4696c41a9fde56b6107dd4db1dc680\""
                ,pThis->Authenticate.username,pThis->Authenticate.Digestrealm,pThis->Authenticate.Digestnonce,strURI);
            /* setting the messsage's Authorization:  header   */
            pRequest->auth.hStr          = (RvRtspStringHandle) rpoolAllocCopyExternal(
                pThis->hRPool,
                0,
                temp_buff,
                strlen(temp_buff)+1);
        }
        else
        {
            /* setting the messsage's Authorization:  header   */
            RvChar temp_buff[1024];
            RvChar *temp_buff3=NULL;
            sprintf(temp_buff,"%s:%s",pThis->Authenticate.username,pThis->Authenticate.password);
            temp_buff3=DataToBase64(temp_buff,strlen(temp_buff));
            memset(temp_buff,0,sizeof(temp_buff));
            if(temp_buff3 !=NULL)
            {
                sprintf(temp_buff,"Basic %s",temp_buff3);
                free(temp_buff3);
            }

            pRequest->auth.hStr          = (RvRtspStringHandle) rpoolAllocCopyExternal(
                pThis->hRPool,
                0,
                temp_buff,
                strlen(temp_buff)+1);
        }
        pRequest->authValid           = RV_TRUE;
    }
    /*add end*/


    /* Notify the application that the message is about to be sent */
    if (pThis->pfnCallbacks.pfnOnSendEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnSendEv(hConnection,
            pThis->hApp, 
            pRequest,
            NULL);

    }

    RtspConnectionSendRequest(pThis, pRequest);

    pWaitingRequest->cSeq       = pRequest->cSeq.value;
    pWaitingRequest->hURI       = rpoolAllocCopyExternal(pThis->hRPool,
        0,
        strURI,
        strlen(strURI)+1);
    pWaitingRequest->hDescribe	= hDescribe;
    pWaitingRequest->timeoutContext.location = location;
    pWaitingRequest->timeoutContext.connection = (void *)pThis;

    /* set the response timer */
    /* setting response timer - timeout is set according to configuration   */
    RvSelectGetTimeoutInfo(pThis->pSelectEngine, &timeoutCallback, &pTQueue);
    RvTimerStart(&pWaitingRequest->responseTimer,       /* timer object            */
        pTQueue,                                        /* timer's queue            */
        RV_TIMER_TYPE_ONESHOT,                          /* timer type               */
        pThis->configuration.describeResponseTimeOut*   /* configured describe timeout (in ms)*/
        RV_TIME64_NSECPERMSEC,                          /* convert to nanoseconds */
        RtspConnectionDescribeOnResponseTimer,           /* callback to call         */
        (void*)&pWaitingRequest->timeoutContext);        /* context for the timer    */
    pWaitingRequest->responseTimerSet = RV_TRUE;

    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionRequestDescribe\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}

/******************************************************************************
 * RvRtspConnectionSendRequest
 * ----------------------------------------------------------------------------
 * General: This API can be called by the application to send any request message
 *          on a connection. The application is expected to construct the request
 *          object using the RvRtspMessageConstructRequest API and to set all 
 *          the required fields of the headers, using RvRtspStringHandleSetString
 *          to set required, RvRtspStringHandle type, fields.
 *          After calling this API, it is the responsibility of the application 
 *          to destruct the request using RvRtspMessageDestructRequest.
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:   hConnection            - The connection handle in the stack.
 *          pRequest               - The filled request object.
 *
 * Output: None.
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspConnectionSendRequest(
        IN	RvRtspConnectionHandle				hConnection,
        IN  RvRtspRequest                       *pRequest)
{
    RtspConnection              *pThis = (RtspConnection*)hConnection;

    /* setting the messsage's CSeq header   */
    pRequest->cSeqValid           = RV_TRUE;
    pRequest->cSeq.value          = pThis->nextCSeq++;    

    return RtspConnectionSendRequest((RtspConnection *)hConnection, pRequest);
}

/******************************************************************************
 * RvRtspConnectionSendResponse
 * ----------------------------------------------------------------------------
 * General: This API can be called by the application to send any response message
 *          on a connection. The application is expected to construct the request
 *          object using the RvRtspMessageConstructResponse API and to set all 
 *          the required fields of the headers, using RvRtspStringHandleSetString
 *          to set required, RvRtspStringHandle type, fields.
 *          After calling this API, it is the responsibility of the application 
 *          to destruct the request using RvRtspMessageDestructResponse.
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:   hConnection            - The connection handle in the stack.
 *          pRequest               - The filled response object.
 *
 * Output: None.
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspConnectionSendResponse(
        IN	RvRtspConnectionHandle				hConnection,
        IN  RvRtspResponse                      *pResponse)
{
    return RtspConnectionSendResponse((RtspConnection *)hConnection, pResponse);
}

/******************************************************************************
 * RvRtspConnectionSendRawBuffer
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
RvStatus RVCALLCONV RvRtspConnectionSendRawBuffer(
        IN	RvRtspConnectionHandle      hConnection,
        IN  RvUint8                     *pBuff,
        IN  RvUint32                    buffSize)
{
    RtspConnection  *pThis          = (RtspConnection *)hConnection;
    HRPOOLELEM      hMessageElement = NULL;
    RvStatus        status          = RV_OK;

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
 * RvRtspConnectionReceiveRawBuffer
 * ----------------------------------------------------------------------------
 * General: This API simulates the receiving of a raw buffer chunk on a connection.
 *          This API can be used to push back into the stack a part of received 
 *          chunk that contains encapsulated stream at the beginning followed by
 *          an Rtsp message (or part of it.
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
RvStatus RVCALLCONV RvRtspConnectionReceiveRawBuffer(
        IN	RvRtspConnectionHandle      hConnection,
        IN  RvUint8                     *pBuff,
        IN  RvUint32                    buffSize)
{
    RtspConnection  *pThis          = (RtspConnection *)hConnection;
    RvStatus        status          = RV_OK;

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
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionReceiveRawBuffer\r\n"));

    RtspTransportReceive(&pThis->transport, pBuff, buffSize);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspConnectionReceiveRawBuffer status %d\r\n", status));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return status;
}

/**************************************************************************
 * RtspConnectionSendRequest
 * ------------------------------------------------------------------------
 * General: This method sends an RvRtspRequest message on the connection.
 *          (this is an internal function, to be used by the connection,
 *          and by sessions).
 *
 * Return Value:    RV_OK - If the connection is successfully de-initialized
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the connection on which to send the request.
 *              pRequest    - The message to send on the connection.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspConnectionSendRequest(
            IN  RtspConnection      *pThis,
            IN  const RvRtspRequest *pRequest)
{
    HRPOOLELEM  hMessageElement = NULL;
    RvStatus    result;

    if ((pThis == NULL) || (pRequest == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state != RTSP_CONNECTION_STATE_CONNECTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionSendRequest\r\n"));


    /* send the response message */
    result = RtspMessagesRequestSerialize(pThis->hRPool, (RvRtspRequest*)pRequest, &hMessageElement);

    /* serialize the message for sending                                            */
    if (result == RV_OK)
        result = RtspTransportSend(&pThis->transport, hMessageElement);

    /* if we sent the message element successfully, it will be freed in transport,  */
    /* otherwise, we must free it here.                                             */
    if ((result != RV_OK) && (hMessageElement != NULL))
        rpoolFree(pThis->hRPool, hMessageElement);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionSendRequest\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return result;
}


/**************************************************************************
 * RtspConnectionOnDnsResponse
 * ------------------------------------------------------------------------
 * General: This function gets the dns response, and inserts it into the
 *          DNS results Array, if we are not already trying to connect, the
 *          function also tries to connect to the server.
 *
 * Return Value:    RV_OK if the function completed successfully, negative
 *                  value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis           - The connection to which the address belongs.
 *              ipAddress       - The connection's IP address.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspConnectionOnDnsResponse(
            IN  RtspConnection      *pThis,
            IN  const RvAddress     *ipAddress)
{
	RvStatus	result = RV_OK;
    RvAddress   *pAddress;

    if ((pThis == NULL) || (ipAddress == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDnsResponse\r\n"));

    /* check that we are waiting for IP addresses       */
    if ((pThis->state != RTSP_CONNECTION_STATE_DNS_QUERY) &&
        (pThis->state != RTSP_CONNECTION_STATE_CONNECTING))
        return RvRtspErrorCode(RV_ERROR_UNKNOWN);

    /* insert the DNS result into the DNS results RA    */
    if (raAdd(pThis->hDNSResults, (RAElement*)&pAddress) < 0)
    {
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDnsResponse - out of resources\r\n"));
        return RV_ERROR_OUTOFRESOURCES;
    }
    RvAddressCopy(ipAddress, pAddress);
    RvAddressSetIpPort(pAddress, pThis->serverPort);

    /* if we are not already trying to connect to an IP */
    /* address, connect to the given IP address.        */
    if (pThis->state == RTSP_CONNECTION_STATE_DNS_QUERY)
    {
        pThis->state    = RTSP_CONNECTION_STATE_CONNECTING;
        result          = RtspConnectionConnectToServer(pThis);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDnsResponse\r\n"));
    return result;
}



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * RtspConnectionGetSessionByLastCSeq
 * ------------------------------------------------------------------------
 * General: This method search for the session that match the cseq
 *          parameter. It tries to match between the last cseq session
 *          request and the cseq parameter.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the connection to connect.
 *              cseq        - cseq to match
 * OUTPUT   :   ppSession   - an address to session object pointer.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionGetSessionByLastCSeq(
                        IN  RtspConnection      *pThis,
                        IN  RvUint16            cseq,
                        OUT RtspSession         **ppSession)
{
    RtspSession     *pSession;
    int             location;

    if ((pThis          == NULL) ||
        (ppSession      == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionGetSessionByLastCSeq\r\n"));

    *ppSession = NULL;
    location = raGetNext(pThis->hSessions, -1);
    while (location >= 0)
    {
        pSession = (RtspSession*) raGet(pThis->hSessions, location);

        if (pSession->waitingRequestCSeq == cseq || pSession->lastPingCSeq == cseq)
        {
            *ppSession = pSession;
            return RV_OK;
        }

        location = raGetNext(pThis->hSessions, location);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionGetSessionByLastCSeq\r\n"));
    return RvRtspErrorCode(RV_ERROR_BADPARAM);
}

/**************************************************************************
 * RtspConnectionOnConnect
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after a
 *          connection attempt has either succeeded or failed.
 *          The callback notifies the user when (and only when) the
 *          connection has been established.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context         - the Connection which was connected.
 *              success         - has the connection succeeded.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnConnect(
                        IN void     *context,
                        IN RvBool   success)
{
    RtspConnection *pThis = (RtspConnection*)context;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnConnect\r\n"));

    if (pThis->state != RTSP_CONNECTION_STATE_CONNECTING)
    {
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_UNKNOWN);
    }


    if (success)
    {
        /* if connection established                */
        /* perform connection connecting operations */
        pThis->state = RTSP_CONNECTION_STATE_CONNECTED;

        /* perform upper layer connecting operations */
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnConnect\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        if (pThis->pfnCallbacks.pfnOnConnectEv != NULL)
        {
            pThis->pfnCallbacks.pfnOnConnectEv(
                                                (RvRtspConnectionHandle)pThis,
                                                pThis->hApp,
                                                success);
        }

        return RV_OK;
    }

    else
    {
        /* if connection failed - try again to connect to the server    */
        if (RtspConnectionConnectToServer(pThis) != RV_OK)
        {
            pThis->state = RTSP_CONNECTION_STATE_DNS_QUERY;

            RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnConnect\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            
            /* perform upper layer connection error operations */
            if (pThis->pfnCallbacks.pfnOnConnectEv != NULL)
            {
                pThis->pfnCallbacks.pfnOnConnectEv(
                    (RvRtspConnectionHandle)pThis,
                    pThis->hApp,
                    success);
            }
            return RV_OK;
        }
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnConnect\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}




/**************************************************************************
 * RtspConnectionOnDisconnect
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after the user
 *          called RvRtspConnectionDisconnect.
 *          The callback notifies the user that the connection has been
 *          disconnected.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context - the Connection which is going to be diconnected.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnDisconnect(
                        IN void *context)
{
    RtspConnection *pThis = (RtspConnection*)context;

    /* checking parameters                          */
    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDisconnect\r\n"));


    if (pThis->state != RTSP_CONNECTION_STATE_DISCONNECTING)
        RvRtspConnectionDisconnect((RvRtspConnectionHandle)pThis);

    /* perform connection disconnecting operations  */
    pThis->state = RTSP_CONNECTION_STATE_CONSTRUCTED;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnDisconnect\r\n"));

    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    /* perform upper layer disconnecting operations */
    if (pThis->pfnCallbacks.pfnOnDisconnectEv != NULL)
        pThis->pfnCallbacks.pfnOnDisconnectEv(
                                                (RvRtspConnectionHandle)pThis,
                                                pThis->hApp);

    return RV_OK;
}

/**************************************************************************
 * RtspConnectionOnRawBufferReceiveEv
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
static RvStatus RtspConnectionOnRawBufferReceiveEv(
                            IN      void        *context,
                            IN      RvUint8     *pBuff,
                            IN      RvUint32    buffSize)
{
    RtspConnection *pThis = (RtspConnection*)context;
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
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnRawBufferReceiveEv\r\n"));

    /* Notify the application about the raw buffer */
    if (pThis->pfnCallbacks.pfnOnRawBufferReceiveEv != NULL)
    {
        status = pThis->pfnCallbacks.pfnOnRawBufferReceiveEv((RvRtspConnectionHandle)pThis, 
                                                             pThis->hApp,
                                                             pBuff,
                                                             buffSize);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnRawBufferReceiveEv\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return status;
}


/**************************************************************************
 * RtspConnectionHandleRequest
 * ------------------------------------------------------------------------
 * General: This method handles a request which was received on the
 *          connection. It responds with an OK for a server ping
 *          (SET_PARAMETER) or REDIRECT, or with a NOT_IMPLEMENTED response
 *          otherwise.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the Connection on which the request was received.
 *              pRequest    - the received request.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionHandleRequest(
                        IN RtspConnection       *pThis,
                        IN const RvRtspRequest  *pRequest)
{
    RvRtspResponse      *pResponse;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgResponse  *RtspMsgResponseMsg;
    raAdd(pThis->hRaResponses, (RAElement *)&RtspMsgResponseMsg);
    if (RtspMsgResponseMsg == NULL)
        return RV_ERROR_NULLPTR;
    memset(RtspMsgResponseMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgResponseMsg->hRtsp = pThis->hRtsp;
    RtspMsgResponseMsg->msgHeaders = NULL;
    RtspMsgResponseMsg->lastHeader = NULL;
    pResponse = &RtspMsgResponseMsg->response;
    pResponse->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgResponseMsg;
#else
    RvRtspResponse   response;
    pResponse = &response;
#endif
    RtspMessagesResponseConstruct(pResponse);

    pResponse->statusLine.hPhrase     = NULL;
    pResponse->cSeq.value             = pRequest->cSeq.value;
    pResponse->cSeqValid              = RV_TRUE;

    /* reply with an OK for PING or REDIRECT, and with NOT_IMPLEMENTED  */
    /* for others.                                                      */
    if ((pRequest->requestLine.method == RV_RTSP_METHOD_SET_PARAMETER) ||
        (pRequest->requestLine.method == RV_RTSP_METHOD_REDIRECT))
    {
        pResponse->statusLine.status      = RV_RTSP_STATUS_OK;
    }

    else
    {
        pResponse->statusLine.status      = RV_RTSP_STATUS_NOT_IMPLEMENTED;
    }

    /* Notify the application that a response is about to be sent */
    if (pThis->pfnCallbacks.pfnOnSendEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnSendEv((RvRtspConnectionHandle)pThis,
                                        pThis->hApp, 
                                        NULL,
                                        pResponse);
        
    }

    RtspConnectionSendResponse(pThis, pResponse);

    RtspMessagesResponseDestruct(pThis->hRPool, pResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaResponses, (RAElement)RtspMsgResponseMsg);
#endif

    /* call the redirect callback if this was a redirect                */
    if ((pRequest->requestLine.method == RV_RTSP_METHOD_REDIRECT) &&
        (pThis->pfnCallbacks.pfnOnRedirectRequestEv != NULL))
    {
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        pThis->pfnCallbacks.pfnOnRedirectRequestEv(
                                                (RvRtspConnectionHandle)pThis,
                                                pThis->hApp,
                                                (const RvRtspRequest*)pRequest);
        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    }

    return RV_OK;
}



/**************************************************************************
 * RtspConnectionHandleResponse
 * ------------------------------------------------------------------------
 * General: This method handles a response which was received on the
 *          connection.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - The Connection on which the response was received.
 *              pResponse   - the received response.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionHandleResponse(
                        IN RtspConnection       *pThis,
                        IN const RvRtspResponse *pResponse)
{
    int                         location;
    RtspWaitingDescribeRequest  *pWaitingRequest;
    RtspSession                 *pSession;
    RvStatus                    result;

    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionHandleResponse check if session response\r\n"));

    /* find the relevant session and let it handle the response message */

    /* find the relevant session from the last cseq request             */
    result = RtspConnectionGetSessionByLastCSeq(
               pThis,
               pResponse->cSeq.value,
               &pSession);

    /* check if this is a response to already operative session */
    if (result == RV_OK)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionHandleResponse YUP it is a session response\r\n"));
        
        RtspSessionHandleResponse(pSession, (RvRtspResponse*)pResponse);
        return RV_OK;
    }

    /* Notify the application about the received response */
    {
        if (pThis->pfnCallbacks.pfnOnReceiveEv != NULL)
        {
            pThis->pfnCallbacks.pfnOnReceiveEv((RvRtspConnectionHandle)pThis,
                                               (RvRtspConnectionAppHandle)pThis->hApp,
                                               NULL,
                                               (RvRtspResponse*)pResponse);
        }
        
    }

    /* we'll search in the waiting describe requests RA */
    location = raGetNext(pThis->hWaitingDescribeRequests, -1);
    while (location >= 0)
    {
        pWaitingRequest = (RtspWaitingDescribeRequest*) raGet(pThis->hWaitingDescribeRequests, location);

        /* check if we found an answer to describe request */
        if (pWaitingRequest->cSeq == pResponse->cSeq.value)
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionHandleResponse receive answer to describe\r\n"));

            /* Cancel the describe timer */
            if (pWaitingRequest->responseTimerSet == RV_TRUE)
            {
                RvTimerCancel(&pWaitingRequest->responseTimer, RV_TIMER_CANCEL_WAIT_FOR_CB /* ?? */);
                pWaitingRequest->responseTimerSet = RV_FALSE;
            }

            /* perform upper layer receiving operations */
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

            if ((pResponse->statusLine.status >= RV_RTSP_STATUS_GROUP_SUCCESS_BEGIN) &&
                (pResponse->statusLine.status <= RV_RTSP_STATUS_GROUP_SUCCESS_END))
            {
                /* the request was successfull - calling describe callback  */
                if (pThis->pfnCallbacks.pfnOnDescribeResponseEv != NULL)
                {
                    pThis->pfnCallbacks.pfnOnDescribeResponseEv(
                        (RvRtspConnectionHandle)pThis,
                        pThis->hApp,
                        pWaitingRequest->hDescribe,
                        (const RvRtspResponse*) pResponse,
                        (const RvRtspStringHandle)pWaitingRequest->hURI);
                }
            }

            else
            {
                /* the request was unsuccessfull - calling error callback   */
                if(pResponse->statusLine.status == RV_RTSP_STATUS_UNAUTHORIZED)
                {
                    if(pResponse->wwwauthenticateValid)
                    {
                        pThis->Authenticate.isDigest =RV_TRUE;
                        strcpy(pThis->Authenticate.Digestrealm,pResponse->wwwauthenticate.realm);
                        strcpy(pThis->Authenticate.Digestnonce,pResponse->wwwauthenticate.nonce);
                    }

                }

                if (pThis->pfnCallbacks.pfnOnErrorEv != NULL)
                {
                    pThis->pfnCallbacks.pfnOnErrorEv(
                        (RvRtspConnectionHandle)pThis,
                        pThis->hApp,
                        (const RvRtspStringHandle)pWaitingRequest->hURI,
                        RV_RTSP_METHOD_DESCRIBE,
                        pResponse->statusLine.status,
                        (const RvRtspStringHandle)pResponse->statusLine.hPhrase);
                }
                if (pThis->pfnCallbacks.pfnOnErrorExtEv != NULL)
                {
                    pThis->pfnCallbacks.pfnOnErrorExtEv(
                        (RvRtspConnectionHandle)pThis,
                        pThis->hApp,
                        pWaitingRequest->hDescribe,
                        (const RvRtspStringHandle)pWaitingRequest->hURI,
                        RV_RTSP_METHOD_DESCRIBE,
                        pResponse->statusLine.status,
                        (const RvRtspStringHandle)pResponse->statusLine.hPhrase,
                        (const RvRtspResponse*) pResponse);
                }
            }

            /* protection against deletion in the above event */
            RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
            if (pThis->destructed == RV_TRUE)
            {
                RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
                return RV_OK;
            }
            RvMutexLock(&pThis->mutex, pThis->pLogMgr);
            RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

            rpoolFree(pThis->hRPool, pWaitingRequest->hURI);
            raDeleteLocation(pThis->hWaitingDescribeRequests, location);
            return RV_OK;
        }

        location = raGetNext(pThis->hWaitingDescribeRequests, location);
    }


    /* oops we got a message that is not a reply to session request,    */
    /* nor to describe                                                  */
    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionHandleResponse receive answer to NO REQUEST\r\n"));
    return RV_OK;
 }



/**************************************************************************
 * RtspConnectionOnReceive
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after a message
 *          was received by it. it handles the received message via
 *          RtspConnectionHandleRequest or RtspConnectionHandleResponse
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context             - the connection on which the message
 *                                  was received.
 *              pTransportMessage   - The received transport message.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnReceive(
                        IN void                         *context,
                        IN const RtspTransportMessage   *pTransportMessage)
{
    RvStatus            result;
    RvRtspRequest       *pRequest;
    RvRtspResponse      *pResponse;
    RvBool              requestLineValid;
    RvBool              statusLineValid;
    RtspConnection *pThis = (RtspConnection*)context;
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

    /* checking parameters                      */
    if ((pThis == NULL) || (pTransportMessage == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state != RTSP_CONNECTION_STATE_CONNECTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive\r\n"));

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    /* Request */
    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
    {
        //Modified by lichao, 20140606 锁资源泄露
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_FALSE;
    }
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;

    /* Response */
    raAdd(pThis->hRaResponses, (RAElement *)&RtspMsgResponseMsg);
    if (RtspMsgResponseMsg == NULL)
    {
        //Modified by lichao, 20140606 锁资源泄露
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_FALSE;
    }
    memset(RtspMsgResponseMsg, 0, sizeof(RtspMsgResponse));
    RtspMsgResponseMsg->hRtsp = pThis->hRtsp;
    RtspMsgResponseMsg->msgHeaders = NULL;
    RtspMsgResponseMsg->lastHeader = NULL;
    pResponse = &RtspMsgResponseMsg->response;
    pResponse->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgResponseMsg;
#endif
    RtspMessagesRequestConstruct(pRequest);
    RtspMessagesResponseConstruct(pResponse);

    /* perform connection receiving operations  */
    result = RtspFirstLineDeserialize(pThis->hRPool, pTransportMessage->hFirstLine, &(pRequest->requestLine),
        &(pResponse->statusLine), &requestLineValid, &statusLineValid);
    if (result != RV_OK)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive Error On First Line\r\n"));
    }

    else if (requestLineValid == RV_TRUE)
    {
        /* deserialize request */
        result = RtspMessagesRequestDeserialize(pThis->hRPool,
                                                &((RtspTransportMessage*)pTransportMessage)->headersQueue,
                                                pRequest);
        /* Notify the application about the received request - clients receive requests only on connections*/
        if (result == RV_OK)
        {
            if (pThis->pfnCallbacks.pfnOnReceiveEv != NULL)
            {
                result = pThis->pfnCallbacks.pfnOnReceiveEv((RvRtspConnectionHandle)pThis,
                                                            (RvRtspConnectionAppHandle)pThis->hApp,
                                                            pRequest,
                                                            NULL);
            }
            
        }
        if (result == RV_OK)
        {
            RtspConnectionHandleRequest(pThis, pRequest);
        }
        else
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive Request Ignored\r\n"));
        }
    }

    else if (statusLineValid == RV_TRUE)
    {
        /* deserialize response */
        result = RtspMessagesResponseDeserialize(pThis->hRPool,
                                                &((RtspTransportMessage*)pTransportMessage)->headersQueue,
                                                ((RtspTransportMessage*)pTransportMessage)->hBody,
                                                pThis->configuration.maxUrlsInMessage,
                                                pResponse);
        
        if (result == RV_OK)
        {
            RtspConnectionHandleResponse(pThis, pResponse);

            /* protection against deletion in the above event */
            //Modified by lichao, 20140606 解决死锁问题
            //RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
            if (pThis->destructed == RV_TRUE)
            {
                RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
                RtspMessagesResponseDestruct(pThis->hRPool, pResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
                raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
                raDelete(pThis->hRaResponses, (RAElement)RtspMsgResponseMsg);
#endif
                //Modified by lichao, 20140606 
                RvMutexLock(&pThis->mutex, pThis->pLogMgr);

                //RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
                return RV_OK;
            }
            //RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        }

        else
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive Response Ignored\r\n"));
        }
    }

    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
    RtspMessagesResponseDestruct(pThis->hRPool, pResponse);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
                raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
                raDelete(pThis->hRaResponses, (RAElement)RtspMsgResponseMsg);
#endif

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return result;
}




/**************************************************************************
 * RvRtspConnectionOnContentLength
 * ------------------------------------------------------------------------
 * General: This callback is called by the Transport layer after a header has
 *          been parsed to find the content length of the message.
 *          Note: if content length is found pContentLength is set to the
 *          length, otherwise, it is not changed.
 *
 * Return Value:    RV_OK if content length is found, RV_ERROR_BADPARAM
 *                  otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context         - The header's connection.
 *              headerLine      - The header line.
 * OUTPUT   :   pContentLength  - The length of the message body.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionOnContentLength(
                                IN  void            *context,
                                IN  HRPOOLELEM      headerLine,
                                OUT RvSize_t        *pContentLength)
{
    RvRtspContentLengthHeader   contentLengthHeader;
    RvStatus                    result;
    RvChar                      headerNameBuffer[50];
    RvSize_t                    bufferLength;
    RvSize_t                    headerOffset;
    RvBool                      success;
    RvSize_t                    i;


    RtspConnection *pThis = (RtspConnection*)context;

    if ((pThis == NULL) || (pContentLength == NULL) || (headerLine == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnContentLength\r\n"));

    /* first find out if this is a Content-Length header */
    success = RtspUtilsRPOOLELEMGetToken(   pThis->hRPool,
                                            headerLine,
                                            0,
                                            " ",
                                            RV_FALSE,
                                            headerNameBuffer,
                                            40,
                                            &bufferLength);
    if (success == RV_FALSE)
    {
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    headerNameBuffer[bufferLength] = '\0';
    headerOffset = bufferLength + 1;

    for(i = 0; i < bufferLength; i++)
        headerNameBuffer[i] = (RvChar)tolower(headerNameBuffer[i]);

    if (strstr(headerNameBuffer, "content-length"))
    {
        /* if this is a content length header       */
        if (RtspHeadersContentLengthDeserialize(
                                            pThis->hRPool,
                                            headerLine,
                                            headerOffset,
                                            &contentLengthHeader) == RV_TRUE)
        {        
            result = RV_OK;
            *pContentLength = contentLengthHeader.value;
        }
        else
            result = RvRtspErrorCode(RV_ERROR_BADPARAM);
    }
    else
    {
        /* if this is not a content length header   */
        result = RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnContentLength\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    return result;
}



/**************************************************************************
 * RtspConnectionConnectToServer
 * ------------------------------------------------------------------------
 * General: This method connects to the IP address specified in the
 *          DNS results Array.
 *          (this is an internal function used by RtspConnectionOnDnsResponse
 *          and RtspConnectionOnConnect).
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - the connection to connect.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionConnectToServer(
                        IN  RtspConnection      *pThis)
{
    RvStatus    result;
    RvAddress   *pAddress;
    RvAddress   sourceAddress;
    RvAddress   *pSourceAddress;
    int         location;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionConnectToServer\r\n"));

    /* get the server's IP address from the dns results RA  */
    location = raGetNext(pThis->hDNSResults, -1);
    if (location < 0)
    {
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionConnectToServer\r\n"));
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }

    /* move the address from the DNS results RA into the connection structure   */
    pAddress = (RvAddress*) raGet(pThis->hDNSResults, location);
    RvAddressCopy(pAddress, &pThis->serverAddress);
    raDeleteLocation(pThis->hDNSResults, location);

    pSourceAddress = NULL;
    if (pThis->sourceIPAddressValid == RV_TRUE)
    {
        RvAddressConstruct(RV_NET_IPV4, &sourceAddress);
        RvAddressSetString(pThis->sourceIPAddress, &sourceAddress);
        pSourceAddress = &sourceAddress;
    }

    /* connect the transport                                */
    result = RtspTransportConnect(  &pThis->transport,
                                    &pThis->serverAddress,
                                    pSourceAddress);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionConnectToServer\r\n"));
	return result;
}



/**************************************************************************
 * RtspConnectionSendResponse
 * ------------------------------------------------------------------------
 * General: This method sends an RvRtspResponse message on the connection.
 *          This method is internally used by the Connection layer to
 *          automaticaly reply to requests from the server.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis           - The connection to send the response on.
 *              pResponse       - The response to send on the connection.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspConnectionSendResponse(
                        IN  RtspConnection          *pThis,
                        IN  const RvRtspResponse    *pResponse)
{
    HRPOOLELEM  hMessageElement;
    RvStatus    result;


    if ((pThis == NULL) || (pResponse == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->state == RTSP_CONNECTION_STATE_DESTRUCTED)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionSendResponse\r\n"));

    /* send the request message */
    result = RtspMessagesResponseSerialize(pThis->hRPool, (RvRtspResponse*)pResponse, &hMessageElement);

    if (result == RV_OK)
        result = RtspTransportSend(&pThis->transport, hMessageElement);

    /* if the message was sent on the transport it will be handled by it,   */
    /* otherwise we need to free it ourselves.                              */
    if ((result != RV_OK) && (hMessageElement != NULL))
        rpoolFree(pThis->hRPool, hMessageElement);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionSendResponse\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return result;
}


static RvStatus RvRtspAllocateAndSendQuery(
						IN RtspConnection*   pThis,
                        IN RvDnsEngine*      pDnsEngine,
                        IN RvDnsQueryType    DnsQueryType,
                        IN RvChar*           dnsName,
                        IN void*             context,
                        IN RvUint32*         queryId)
{
    RvStatus    crv     = RV_ERROR_UNKNOWN;
    RvChar*     pQbuff  = NULL;
    RvInt       bufLen  = 0;
    HRPOOLELEM  elem;
    
    while (RV_OK != crv)
    {
        crv = RvAresSendQuery(pDnsEngine, DnsQueryType, dnsName, RV_FALSE, pQbuff, &bufLen, context, queryId);
        if (RV_ERROR_INSUFFICIENT_BUFFER == RvErrorGetCode(crv))
        {
            elem = rpoolAlloc(pThis->hRPool, bufLen);
            rpoolGetPtr(pThis->hRPool, elem, 0, (void**)&pQbuff);
        }
        else if (RV_OK != RvErrorGetCode(crv))
        {
            RvLogError(pThis->pLogSrc, (pThis->pLogSrc,
                "RvRtspAllocateAndSendQuery - RvAresSendQuery failed. error (rv=%d)", RvErrorGetCode(crv)));
            return RvErrorGetCode(crv);
        }
    }

    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc,
        "RvRtspAllocateAndSendQuery - name=%s was sent succesfully", dnsName));
    
    return RV_OK;
}

/**************************************************************************
 * RtspConnectionDescribeOnResponseTimer
 * ------------------------------------------------------------------------
 * General: This function is called when a timeout occurs waiting for a
 *          describe response.
 *
 * Return Value:    RV_TRUE if the function is completed successfuly,
 *                  RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The connection on which the timeout occured.
 *              location    - The location of the describe request in the waiting describes RA.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspConnectionDescribeOnResponseTimer(
                                IN void *context)
{
    RtspDescribeTimeoutContext  *describeContext = (RtspDescribeTimeoutContext *)context;
    RtspConnection              *pThis           = (RtspConnection *)describeContext->connection;
    RtspWaitingDescribeRequest  *pWaitingRequest = NULL;
    RvUint32                    location         = describeContext->location;
    

    /* if an error occurs we will not reschedule the timer  */
    if (pThis == NULL )
        return RV_FALSE;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionDescribeOnResponseTimer RtspConnection: 0x%x, location: %d", pThis, location));

    /* get the waiting request object */
    pWaitingRequest = (RtspWaitingDescribeRequest *)raGet(pThis->hWaitingDescribeRequests, location);
    if (pWaitingRequest == NULL)
    {
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionDescribeOnResponseTimer"))
        return RV_FALSE;
    }

    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionDescribeOnResponseTimer"))
    
    
    /* call the error callback                              */
    if (pThis->pfnCallbacks.pfnOnErrorEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnErrorEv(
                                    (RvRtspConnectionHandle)pThis,
                                    pThis->hApp,
                                    (const RvRtspStringHandle)pWaitingRequest->hURI,
                                    RV_RTSP_METHOD_DESCRIBE,
                                    RV_RTSP_STATUS_RESPONSE_TIME_OUT,
                                    NULL);
    }
	if (pThis->pfnCallbacks.pfnOnErrorExtEv != NULL)
	{
		pThis->pfnCallbacks.pfnOnErrorExtEv(
									(RvRtspConnectionHandle)pThis,
									pThis->hApp,
                                    pWaitingRequest->hDescribe,
                                    (const RvRtspStringHandle)pWaitingRequest->hURI,
                                    RV_RTSP_METHOD_DESCRIBE,
									RV_RTSP_STATUS_RESPONSE_TIME_OUT,
									NULL,
                                    NULL);
	}

    /* Remove the describe request from the waiting list */

    /* protection against deletion in the above event */
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    if (pThis->destructed == RV_TRUE)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RV_TRUE;
    }
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    
    rpoolFree(pThis->hRPool, pWaitingRequest->hURI);
    raDeleteLocation(pThis->hWaitingDescribeRequests, location);
    return RV_TRUE;
}

RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetNextCSeq(
    IN	RvRtspConnectionHandle     hConnection,
    OUT	RvUint16           *pNextCSeq)
{
    RtspConnection         *pThis = (RtspConnection*)hConnection;

    if ((pThis == NULL) || (pNextCSeq == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);

    *pNextCSeq = pThis->nextCSeq++;

    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetRtspHandle(
    IN	RvRtspConnectionHandle	hConnection,
    OUT	RvRtspHandle           *hRtsp)
{
    RtspConnection         *pThis = (RtspConnection*)hConnection;

    if ((pThis == NULL) || (hRtsp == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);

    *hRtsp = pThis->hRtsp;

    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

#if defined(__cplusplus)
}
#endif
