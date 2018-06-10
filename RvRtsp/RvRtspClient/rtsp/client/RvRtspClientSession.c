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
 *                              <RvRtspClientSession.c>
 *
 *  This file contains definitions relevant to the RTSP sessions.
 *  An RTSP Session instance is a thread safe representation of a session conducted
 *  on top of an RTSP connection.
 *
 *    Author                         Date
 *    ------                        ------
 *      Shaft                       8/1/04
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
#include "RvRtspClientTypes.h"
#include "RvRtspClientSession.h"
#include "RtspClientSessionInternal.h"
#include "RtspUtilsInternal.h"
#include "RtspMessages.h"
#include "RtspMsgInternal.h"
#include <string.h>

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
 * RtspSessionSendRequest
 * ------------------------------------------------------------------------
 * General: This function sends an RTSP request on the connection.
 *          This is an internal function used by the session layer.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hSession            - The session object.
 *              pRequest            - The request to send.
 *              waitingForResponse  - Do we wait for a response to the request.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspSessionSendRequest(
                        IN  RvRtspSessionHandle     hSession,
                        IN  RvRtspRequest           *pRequest,
                        IN  RvBool                  waitingForResponse);



/**************************************************************************
 * RtspSessionOnResponseTimer
 * ------------------------------------------------------------------------
 * General: This function is called when a timeout occurs waiting for a
 *          request response.
 *
 * Return Value:    RV_TRUE if the function is completed successfuly,
 *                  RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The session on which the timeout occured.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspSessionOnResponseTimer(
                        IN void *context);



/**************************************************************************
 * RtspSessionOnPingTimer
 * ------------------------------------------------------------------------
 * General: This function is called when a ping needs to be sent on the
 *          connection.
 *
 * Return Value:    RV_TRUE if the function is completed successfuly,
 *                  RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The session to send the keep-alive on.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspSessionOnPingTimer(
                    IN void *context);


/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RvRtspSessionConstruct
 * ------------------------------------------------------------------------
 * General: This method initializes the RTSP session structure.
 *          The session starts in an INIT state.
 *
 * Return Value:    RV_OK - If the RTSP session is successfully initialized,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hConnection         - The connection the session is on.
 *              pConfiguration      - Configuration for the session.
 *              configurationSize   - size of callbacks structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 *              pCallbacks          - Callback functions to call on events.
 *              callbacksStructSize - size of callbacks structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 *              hApp                - Application context.
 * OUTPUT   :   phSession           - The constructed session.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionConstruct(
                        IN  RvRtspConnectionHandle          hConnection,
                        IN  RvRtspSessionConfiguration      *pConfiguration,
                        IN  const RvSize_t                  configurationSize,
                        IN  RvRtspSessionCallbackFunctions  *pCallbacks,
                        IN  const RvSize_t                  callbacksStructSize,
                        IN  RvRtspSessionAppHandle          hApp,
                        OUT RvRtspSessionHandle             *phSession)
{
    RtspSession     *pThis;
    RtspConnection  *pConnection = (RtspConnection*)hConnection;

    if (hConnection == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if (sizeof(RvRtspSessionConfiguration) != configurationSize)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    if (sizeof(RvRtspSessionCallbackFunctions) != callbacksStructSize)
        return RvRtspErrorCode(RV_ERROR_BADPARAM);

    RvMutexLock(&pConnection->mutex, pConnection->pLogMgr);
    RvLogEnter(pConnection->pLogSrc, (pConnection->pLogSrc, "RvRtspSessionConstruct\r\n"));

    /* Task #1: We need to find out if we exceeded the session maximum      */
    /*          instances allowed for the connection                        */
    if (raAdd(pConnection->hSessions, (RAElement*)&pThis) < 0)
    {
        RvMutexUnlock(&pConnection->mutex, pConnection->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
    }


    /* Task #2: We need to construct a new session and insert it to  the    */
    /*          connection object RA.                                       */
    pThis->pLogMgr              = pConnection->pLogMgr;
    pThis->pLogSrc              = pConnection->pLogSrc;
    pThis->hRtsp                = pConnection->hRtsp;
    pThis->hRPool               = pConnection->hRPool;
    pThis->hRaRequests          = pConnection->hRaRequests;
    pThis->hRaResponses         = pConnection->hRaResponses;
    pThis->hApp                 = hApp;
    pThis->pSelectEngine        = pConnection->pSelectEngine;
    pThis->pConnection          = pConnection;
    memcpy(&pThis->pfnCallbacks, pCallbacks, sizeof(RvRtspSessionCallbackFunctions));
    pThis->pGMutex              = pConnection->pGMutex;
    memcpy(&pThis->configuration, pConfiguration, sizeof(RvRtspSessionConfiguration));
    pThis->state                = RV_RTSP_SESSION_STATE_INIT;
    pThis->hURI                 = NULL;
    pThis->hSessionId           = NULL;
    pThis->waitingForResponse   = RV_FALSE;
	pThis->waitingRequest		= RV_RTSP_METHOD_DESCRIBE;
    pThis->waitingRequestCSeq   = 0;
    pThis->pingTimerSet         = RV_FALSE;
    pThis->responseTimerSet     = RV_FALSE;
    pThis->destructed           = RV_FALSE;
    *phSession                  = (RvRtspSessionHandle)pThis;

    RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);

    RvLogLeave(pConnection->pLogSrc, (pConnection->pLogSrc, "RtspSessionConstruct\r\n"));
    RvMutexUnlock(&pConnection->mutex, pConnection->pLogMgr);

    return RV_OK;
}



/**************************************************************************
 * RvRtspSessionDestruct
 * ------------------------------------------------------------------------
 * General: This method destructs the RTSP session structure.
 *
 * Return Value:    RV_OK - When the session is successfully de-initialized,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hSession    - the session being destructed.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionDestruct(
                        IN  RvRtspSessionHandle hSession)
{
    RtspSession *pThis = (RtspSession*)hSession;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->destructed == RV_TRUE)
    {
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
        return RvRtspErrorCode(RV_ERROR_DESTRUCTED);
    }

    /* canceling the session's timers                                   */
    if (pThis->pingTimerSet == RV_TRUE)
        RvTimerCancel(&pThis->pingTimer,RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);

    if (pThis->responseTimerSet == RV_TRUE)
        RvTimerCancel(&pThis->responseTimer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionDestruct\r\n"));

    /* freeing the session's used resources                             */
    pThis->responseTimerSet     = RV_FALSE;
    pThis->pingTimerSet         = RV_FALSE;

    if (pThis->hURI)
    {
        rpoolFree(pThis->hRPool, pThis->hURI);
        pThis->hURI = NULL;

    }

    if (pThis->hSessionId)
    {
        rpoolFree(pThis->hRPool, pThis->hSessionId);
        pThis->hSessionId = NULL;
    }

    pThis->destructed   = RV_TRUE;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionDestruct\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);

    raDelete(pThis->pConnection->hSessions, (RAElement) pThis);

    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
	if (pThis->pfnCallbacks.pfnOnDestructEv != NULL)
	{
        pThis->pfnCallbacks.pfnOnDestructEv(
						(RvRtspSessionHandle)pThis,
						pThis->hApp);
	}

    return RV_OK;
}


/**************************************************************************
 * RvRtspSessionGetState
 * ------------------------------------------------------------------------
 * General: This method returns the state of the RTSP session.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    : hSession      - the session object.
 * OUTPUT   : pState        - the returned session state.
 * INOUT    : None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionGetState(
                        IN  RvRtspSessionHandle hSession,
                        OUT RvRtspSessionState  *pState)
{
    RtspSession *pThis = (RtspSession*)hSession;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionGetState\r\n"));

    /* returning the state  */
    *pState = pThis->state;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionGetState\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}


/**************************************************************************
 * RvRtspSessionGetId
 * ------------------------------------------------------------------------
 * General: This method returns the session Id.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hSession        - the session object.
 * OUTPUT   :   phSessionId     - string containing the RTSP Session-Id.
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionGetId(
                        IN  RvRtspSessionHandle hSession,
                        OUT RvRtspStringHandle  *phSessionId)
{
    RtspSession *pThis = (RtspSession*)hSession;

    if (pThis == NULL)
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionGetId\r\n"));

    /* returning the session id */
    *phSessionId    = (RvRtspStringHandle)pThis->hSessionId;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionGetId\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;

}


/**************************************************************************
 * RvRtspSessionSetUri
 * ------------------------------------------------------------------------
 * General: This method sets the RTSP session's URI. This is done so
 *          requests can be made on this session with a different URI. the
 *          URI is used as the target of the requests made to the server.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hSession        - The session object.
 *              strURI          - The new URI.
 * OUTPUT   :   None
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionSetUri(
                        IN  RvRtspSessionHandle hSession,
                        IN  const RvChar        *strURI)
{
    RtspSession *pThis = (RtspSession*)hSession;

    if ((pThis == NULL) || (strURI == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionSetUri\r\n"));

    /* replacing the old URI with the new   */
    if (pThis->hURI != NULL)
        rpoolFree(pThis->hRPool, pThis->hURI);

    pThis->hURI = rpoolAllocCopyExternal(pThis->hRPool, 0, strURI, strlen(strURI)+1);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionSetUri\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}





/**************************************************************************
 * RvRtspSessionSetup
 * ------------------------------------------------------------------------
 * General: This method sends a SETUP request on the session.
 *          This sets up an RTSP session with the server using the
 *          session's URI.
 *          The application will be notified by a callback when a response
 *          arrives or a timeout occurs.
 *
 *          Upon successful (or not) of state change the session layer will
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hSession            - The session object.
 *              pTransportHeader    - The SETUP request's transport header.
 * OUTPUT   :   None
 * INOUT    :   None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionSetup(
                        IN  RvRtspSessionHandle     hSession,
                        IN  RvRtspTransportHeader   *pTransportHeader)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
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
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if ((hSession == NULL) || (pTransportHeader == NULL))
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_SETUP;
    pRequest->transportValid  = RV_TRUE;
    memcpy(&pRequest->transport, pTransportHeader, sizeof(RvRtspTransportHeader));
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)   
    //pRequest->transport.additionalFields = NULL;
#endif    
    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    return RV_OK;
}

//added by lichao, 20141217
RVAPI
RvStatus RVCALLCONV RvRtspSessionSetupEx(
                                       IN  RvRtspSessionHandle     hSession,
                                       IN  RvUint16                 cseq,
                                       IN  RvRtspTransportHeader   *pTransportHeader)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
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
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if ((hSession == NULL) || (pTransportHeader == NULL))
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_SETUP;
    pRequest->transportValid  = RV_TRUE;
    memcpy(&pRequest->transport, pTransportHeader, sizeof(RvRtspTransportHeader));

    pRequest->cSeq.value = cseq;
    pRequest->cSeqValid = RV_TRUE;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)   
    //pRequest->transport.additionalFields = NULL;
#endif    
    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    return RV_OK;
}

/**************************************************************************
 * RvRtspSessionPlay
 * ------------------------------------------------------------------------
 * General: This method sends a PLAY request on the session, this signals
 *          the server to start playing.
 *          If the time is NULL, playing starts immediately, if an NPT time
 *          is specified, playing is started from that position.
 *
 *          Upon successful (or not) of state change the session layer will
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    : hSession      - the session object.
 *            pNptTime      - the time to start playing from.
 * OUTPUT   : None
 * INOUT    : None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionPlay(
                        IN RvRtspSessionHandle  hSession,
                        IN RvRtspNptTime        *pNptTime)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
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
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if (hSession == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (pThis->hSessionId == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_PLAY;

    if (pNptTime != NULL)
    {
        memcpy(&pRequest->range.startTime, pNptTime, sizeof(RvRtspNptTime));
        pRequest->range.endTime.format    = RV_RTSP_NPT_FORMAT_NOT_EXISTS;
        pRequest->rangeValid              = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pRequest->range.additionalFields  = NULL;
#endif
    }

    
    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    return RV_OK;
}

//added by lichao, 20141217
RVAPI
RvStatus RVCALLCONV RvRtspSessionPlayEx(
                                      IN RvRtspSessionHandle  hSession,
                                      IN RvUint16             cseq,
                                      IN RvRtspNptTime        *pNptTime,
                                      IN float                scale)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
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
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if (hSession == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (pThis->hSessionId == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_PLAY;

    if (pNptTime != NULL)
    {
        memcpy(&pRequest->range.startTime, pNptTime, sizeof(RvRtspNptTime));
        pRequest->range.endTime.format    = RV_RTSP_NPT_FORMAT_NOT_EXISTS;
        pRequest->rangeValid              = RV_TRUE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        pRequest->range.additionalFields  = NULL;
#endif
    }

    pRequest->cSeq.value = cseq;
    pRequest->cSeqValid = RV_TRUE;

    (void)scale;

    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    return RV_OK;
}

/**************************************************************************
 * RvRtspSessionPause
 * ------------------------------------------------------------------------
 * General: This method sends a PAUSE request on the session.
 *          This signals the server to pause the RTP transmission.
 *
 *          Upon successful (or not) of state change the session layer will
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    : hSession      - the session object.
 * OUTPUT   : None
 * INOUT    : None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionPause(
                        IN RvRtspSessionHandle  hSession)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
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
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if (hSession == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (pThis->hSessionId == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_PAUSE;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pRequest->session.additionalFields = NULL;
#endif
    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    return RV_OK;
}

RVAPI
RvStatus RVCALLCONV RvRtspSessionPauseEx(
                                       IN RvRtspSessionHandle  hSession,
                                       IN RvUint16             cseq)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
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
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if (hSession == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (pThis->hSessionId == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_PAUSE;

    pRequest->cSeq.value = cseq;
    pRequest->cSeqValid = RV_TRUE;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pRequest->session.additionalFields = NULL;
#endif
    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    return RV_OK;
}


/**************************************************************************
 * RvRtspSessionTeardown
 * ------------------------------------------------------------------------
 * General: This method sends a TEARDOWN request on the session.
 *          This signals the server to permanently stop the transmission.
 *          And free any allocated resources it uses for the session.
 *          The session is ended.
 *
 *          Upon successful (or not) of state change the session layer will
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    : hSession      - the session object.
 * OUTPUT   : None
 * INOUT    : None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionTeardown(
                        IN RvRtspSessionHandle  hSession)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
        return RV_ERROR_NULLPTR;
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->request.hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
#else
    RvRtspRequest   request;
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if (hSession == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (pThis->hSessionId == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_TEARDOWN;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pRequest->session.additionalFields = NULL;
#endif

    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif


    return RV_OK;
}

//added by lichao, 20141217
RVAPI
RvStatus RVCALLCONV RvRtspSessionTeardownEx(
            IN RvRtspSessionHandle  hSession,
            IN RvUint16 cseq)
{
    RvRtspRequest   *pRequest;
    RtspSession     *pThis = (RtspSession*)hSession;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
        return RV_ERROR_NULLPTR;
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->request.hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
#else
    RvRtspRequest   request;
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    if (hSession == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    if (pThis->hSessionId == NULL)
    {
        //added by lichao,20151224 Òì³£·ÖÖ§ÊÍ·Å×ÊÔ´
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
        raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif
        return RvRtspErrorCode(RV_ERROR_BADPARAM);
    }

    pRequest->requestLine.method  = RV_RTSP_METHOD_TEARDOWN;

    pRequest->cSeq.value = cseq;
    pRequest->cSeqValid = RV_TRUE;

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    pRequest->session.additionalFields = NULL;
#endif

    RtspSessionSendRequest(hSession, pRequest, RV_TRUE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif


    return RV_OK;
}

/******************************************************************************
 * RvRtspSessionSendRequest
 * ----------------------------------------------------------------------------
 * General: This API can be called by the application to send any request message
 *          on a session. The application is expected to construct the request
 *          object using the RvRtspMessageConstructRequest API and to set all 
 *          the required fields of the headers, using RvRtspStringHandleSetString
 *          to set required, RvRtspStringHandle type, fields.
 *          After calling this API, it is the responsibility of the application 
 *          to destruct the request using RvRtspMessageDestructRequest.
 * Return Value: RV_OK  - if successful.
 *               Other on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:   hSession               - The session handle in the stack.
 *          pRequest               - The filled request object.
 *          waitingForResponse     - RV_TRUE if the session waits for a r
 *                                   esponse to the request.
 * Output: None.
 *****************************************************************************/
RVAPI 
RvStatus RVCALLCONV RvRtspSessionSendRequest(
        IN	RvRtspSessionHandle				    hSession,
        IN  RvRtspRequest                       *pRequest,
        IN  RvBool                              waitingForResponse)
{
    return RtspSessionSendRequest(hSession, pRequest, waitingForResponse);
}

/**************************************************************************
 * RtspSessionHandleResponse
 * ------------------------------------------------------------------------
 * General: This method handles a response which was received on the
 *          session.
 *
 * Return Value:    RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   pThis       - The Session on which the response was received.
 *              pResponse   - the received response.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
RvStatus RtspSessionHandleResponse(
                    IN RtspSession      *pThis,
                    IN RvRtspResponse   *pResponse)
{
	RvTimerFunc         timeoutCallback;	/* select engine's timeout function	*/
    RvTimerQueue        *pTQueue;           /* select engine's timer queue      */
    RvRtspSessionState  newState;
    RvRtspSessionState  oldState;
    RvBool              setPingTimer    = RV_FALSE;
    RvBool              responseError   = RV_FALSE;

    /* checking parameters                      */
    if ((pThis == NULL) || (pResponse == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);


    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse\r\n"));

    if (pResponse->cSeq.value == pThis->lastPingCSeq)
    {
        /* This is a ping response */
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_OK;
    }

    /* Notify the application about the received response */
    if (pThis->pfnCallbacks.pfnOnReceiveEv != NULL)
    {
    	RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse fnOnReceiveEv start\r\n"));
        pThis->pfnCallbacks.pfnOnReceiveEv((RvRtspSessionHandle)pThis,
                                           (RvRtspSessionAppHandle)pThis->hApp,
                                            NULL,
                                            pResponse);
    }
    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "check if this is a response for the message that we are waiting fo\r\n"));
    /* check if this is a response for the message that we are waiting for */
    if ((pThis->waitingForResponse != RV_TRUE)  ||
        (pResponse->cSeq.value != pThis->waitingRequestCSeq))
    {
       RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse %d\r\n",__LINE__));
        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_OK;
    }

    /* perform session receiving operations */
    pThis->waitingForResponse = RV_FALSE;

    if (pThis->responseTimerSet == RV_TRUE)
        RvTimerCancel(&pThis->responseTimer, RV_TIMER_CANCEL_WAIT_FOR_CB);

    pThis->responseTimerSet = RV_FALSE;


    if ((pResponse->statusLine.status < RV_RTSP_STATUS_GROUP_SUCCESS_BEGIN) ||
        (pResponse->statusLine.status > RV_RTSP_STATUS_GROUP_SUCCESS_END))
    {
        responseError = RV_TRUE;
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        /* perform session external operations */
         RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse %d\r\n",__LINE__));
        if (pThis->pfnCallbacks.pfnOnErrorEv != NULL)
        {
            pThis->pfnCallbacks.pfnOnErrorEv(
                                    (RvRtspSessionHandle)pThis,
                                    pThis->hApp,
                                    pThis->waitingRequest,
                                    pResponse->statusLine.status,
                                    pResponse->statusLine.hPhrase);
		}
		if (pThis->pfnCallbacks.pfnOnErrorExtEv != NULL)
		{
            pThis->pfnCallbacks.pfnOnErrorExtEv(
									(RvRtspSessionHandle)pThis,
									pThis->hApp,
									pThis->waitingRequest,
									pResponse->statusLine.status,
									pResponse->statusLine.hPhrase,
					                (const RvRtspResponse*)pResponse);
		}
        if (pThis->pfnCallbacks.pfnOnErrorEv != NULL || pThis->pfnCallbacks.pfnOnErrorExtEv != NULL)
            return RV_OK;
    }

    /* we received a good response to our request                   */
    /* now what is left is to update the session state, update      */
    /* several inner variables and issue a Ping timer if necessary  */

    /* copy the session ID to the session object internal data   */
    if ((pResponse->sessionValid == RV_TRUE) && (pThis->hSessionId == NULL))
    {
        pThis->hSessionId               = (HRPOOLELEM)pResponse->session.hSessionId;
        pResponse->sessionValid         = RV_FALSE;
        pResponse->session.hSessionId   = NULL;
    }

    newState = pThis->state;

    /* operate state machine                                    */
    switch (pThis->state)
    {
        case RV_RTSP_SESSION_STATE_INIT:
        {
            if (pThis->waitingRequest == RV_RTSP_METHOD_SETUP)
                newState = RV_RTSP_SESSION_STATE_READY;
            break;
        }

        case RV_RTSP_SESSION_STATE_READY:
        {
            if (pThis->waitingRequest == RV_RTSP_METHOD_PLAY)
                newState = RV_RTSP_SESSION_STATE_PLAYING;

            else if (pThis->waitingRequest == RV_RTSP_METHOD_TEARDOWN)
                newState = RV_RTSP_SESSION_STATE_INIT;

            break;
        }

        case RV_RTSP_SESSION_STATE_PLAYING:
        {
            if (pThis->waitingRequest == RV_RTSP_METHOD_PAUSE)
                newState = RV_RTSP_SESSION_STATE_READY;

            else if (pThis->waitingRequest == RV_RTSP_METHOD_TEARDOWN)
                newState = RV_RTSP_SESSION_STATE_INIT;

            else if (pThis->waitingRequest == RV_RTSP_METHOD_SETUP)
                newState = RV_RTSP_SESSION_STATE_READY;
            break;
        }

        default:
        {
            RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse - state invalid\r\n"));
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            return RV_OK;
        }
    } /* switch (pThis->state) */

    if (responseError == RV_TRUE)
    {
        if (pThis->waitingRequest == RV_RTSP_METHOD_TEARDOWN)
        {
            newState = RV_RTSP_SESSION_STATE_INIT;
        }

        else
        {
            newState = pThis->state;
        }
    }

    if (newState == RV_RTSP_SESSION_STATE_INIT)
    {
        setPingTimer = RV_FALSE;

        if (pThis->hURI)
        {
            rpoolFree(pThis->hRPool, pThis->hURI);
            pThis->hURI = NULL;
        }

        if (pThis->hSessionId)
        {
            rpoolFree(pThis->hRPool, pThis->hSessionId);
            pThis->hSessionId = NULL;
        }

        pThis->waitingRequestCSeq   = 0;
    }

    else
    {
        setPingTimer = RV_TRUE;
    }
     RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse %d\r\n",__LINE__));
    if ((pThis->pingTimerSet == RV_TRUE) && (setPingTimer == RV_FALSE))
    {
        RvTimerCancel(&pThis->pingTimer,RV_TIMER_CANCEL_WAIT_FOR_CB);
        pThis->pingTimerSet = RV_FALSE;
    }

    /* setting response timer - timeout is set according to configuration   */
    else if ((pThis->configuration.pingTransmissionTimeOutResolution > 0) && 
        (pThis->pingTimerSet == RV_FALSE) && (setPingTimer == RV_TRUE))
    {
		RvSelectGetTimeoutInfo(pThis->pSelectEngine, &timeoutCallback, &pTQueue);
        RvTimerStart(   &pThis->pingTimer,                  /* timer object         */
                        pTQueue,                            /* timer's queue        */
                        RV_TIMER_TYPE_PERIODIC,             /* PERIODIC or ONESHOT */
                        pThis->configuration.pingTransmissionTimeOutResolution*
                            (RV_TIME64_NSECPERSEC/1000),    /* time in nanoseconds  */
                        RtspSessionOnPingTimer,             /* callback to call     */
                        (void*)pThis);                      /* context for the timer*/
        pThis->pingTimerSet = RV_TRUE;
    }


    oldState        = pThis->state;
    pThis->state    = newState;
     RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse %d\r\n",__LINE__));	
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspConnectionOnReceive\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    /* perform session external operations */
    if (pThis->pfnCallbacks.pfnOnStateChangeEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnStateChangeEv(
                                        (RvRtspSessionHandle)pThis,
                                        pThis->hApp,
                                        oldState,
                                        newState,
                                        pResponse);

    }
     RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionHandleResponse %d\r\n",__LINE__));
    return RV_OK;
}



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspSessionSendRequest
 * ------------------------------------------------------------------------
 * General: This function sends an RTSP request on the connection.
 *          This is an internal function used by the session layer.
 *
 * Return Value:    RV_OK - When the function is successfully completed,
 *                  negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   hSession            - The session object.
 *              pRequest            - The request to send.
 *              waitingForResponse  - Do we wait for a response to the request.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvStatus RtspSessionSendRequest(
                        IN  RvRtspSessionHandle     hSession,
                        IN  RvRtspRequest           *pRequest,
                        IN  RvBool                  waitingForResponse)
{
    RtspSession         *pThis = (RtspSession*)hSession;
	RvTimerFunc         timeoutCallback;	/* select engine's timeout function	*/
    RvTimerQueue        *pTQueue;               /* select engine's timer queue      */


    if ((pThis == NULL) || (pRequest == NULL))
        return RvRtspErrorCode(RV_ERROR_NULLPTR);

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexLock(&pThis->pConnection->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionSendRequest\r\n"));

    /* we can't send a message requiring a response while another is still in process */
    if ((waitingForResponse == RV_TRUE) && (pThis->waitingForResponse == RV_TRUE))
    {
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionSendRequest - Another request is already in the pipes\r\n"));
        RvMutexUnlock(&pThis->pConnection->mutex, pThis->pLogMgr);
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_OK;
    }

    /* setting the session communication specific parameters to the request     */

    /* setting the message's request line                                       */
    if (pRequest->requestLine.hURI == NULL)
        pRequest->requestLine.hURI  = (RvRtspStringHandle)pThis->hURI;

    /* setting the messsage's CSeq header                                       */
    if (RV_FALSE == pRequest->cSeqValid)    //modified by lichao 20141217 允许外部指定cseq
    {
        pRequest->cSeq.value        = pThis->pConnection->nextCSeq++;
        pRequest->cSeqValid         = RV_TRUE;
    }

    /* setting the request CSeq so we can match the response to the request     */
    if (waitingForResponse)
        pThis->waitingRequestCSeq   = pRequest->cSeq.value;
    else if (pRequest->requestLine.method == RV_RTSP_METHOD_OPTIONS)
        pThis->lastPingCSeq = pRequest->cSeq.value;

    /* setting the session header                                               */
    if (pThis->hSessionId != NULL)
    {
        pRequest->session.timeout       = 0;
        pRequest->session.hSessionId    = (RvRtspStringHandle)pThis->hSessionId;
        pRequest->sessionValid          = RV_TRUE;
    }

    /*add by kongfd*/
    if(pThis->pConnection->Authenticate.isUser)
    {
    	    if(pThis->pConnection->Authenticate.isDigest)
    	    {
    	    	   RvChar temp_buff[1024];
    	    	   sprintf(temp_buff,"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"1c4696c41a9fde56b6107dd4db1dc680\""
    	    	   	,pThis->pConnection->Authenticate.username,pThis->pConnection->Authenticate.Digestrealm,pThis->pConnection->Authenticate.Digestnonce,pThis->hURI);
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
		    sprintf(temp_buff,"%s:%s",pThis->pConnection->Authenticate.username,pThis->pConnection->Authenticate.password);
		    DataToBase64(&temp_buff3,temp_buff,strlen(temp_buff));
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

    
    if (pThis->pfnCallbacks.pfnOnSendEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnSendEv(hSession,
                                        pThis->hApp, 
                                        pRequest,
                                        NULL);
        
    }


    /* sending the request on the connection                                    */
    RtspConnectionSendRequest(pThis->pConnection, pRequest);

    /* removing the session communication specific parameters from the request  */
    pRequest->requestLine.hURI      = NULL;
    pRequest->session.hSessionId    = NULL;
    pRequest->sessionValid          = RV_FALSE;

    /* setting the response timer if needed                                     */
    if (waitingForResponse == RV_TRUE)
    {
        pThis->waitingForResponse   = RV_TRUE;
        pThis->waitingRequest       = pRequest->requestLine.method;
        pThis->responseTimerSet     = RV_TRUE;


        /* setting response timer - timeout is set according to configuration   */
		RvSelectGetTimeoutInfo(pThis->pSelectEngine, &timeoutCallback, &pTQueue);
        RvTimerStart(   &pThis->responseTimer,                  /* timer object             */
                        pTQueue,                                /* timer's queue            */
                        RV_TIMER_TYPE_ONESHOT,                  /* timer type               */
                        pThis->configuration.responseTimeOutResolution*
                            (RV_TIME64_NSECPERSEC/1000),        /* timeout in nanoseconds   */
                        RtspSessionOnResponseTimer,             /* callback to call         */
                        (void*)pThis);                          /* context for the timer    */
    }

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspSessionSetup\r\n"));
    RvMutexUnlock(&pThis->pConnection->mutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}




/**************************************************************************
 * RtspSessionOnResponseTimer
 * ------------------------------------------------------------------------
 * General: This function is called when a timeout occurs waiting for a
 *          request response.
 *
 * Return Value:    RV_TRUE if the function is completed successfuly,
 *                  RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The session on which the timeout occured.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
static RvBool RtspSessionOnResponseTimer(
                                IN void *context)
{
    RtspSession *pThis = (RtspSession*)context;

    /* if an error occurs we will not reschedule the timer  */
    if (pThis == NULL)
        return RV_FALSE;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionOnResponseTimer\r\n"));

    /* handle the timeout                                   */
    if (pThis->waitingForResponse == RV_FALSE)
    {
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return RV_FALSE;
    }

    pThis->waitingForResponse   = RV_FALSE;
    pThis->responseTimerSet     = RV_FALSE;
    pThis->waitingRequestCSeq   = 0;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionOnResponseTimer\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    /* call the error callback                              */
    if (pThis->pfnCallbacks.pfnOnErrorEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnErrorEv(
                                    (RvRtspSessionHandle)pThis,
                                    pThis->hApp,
                                    pThis->waitingRequest,
                                    RV_RTSP_STATUS_RESPONSE_TIME_OUT,
                                    NULL);
    }
	if (pThis->pfnCallbacks.pfnOnErrorExtEv != NULL)
	{
		pThis->pfnCallbacks.pfnOnErrorExtEv(
									(RvRtspSessionHandle)pThis,
									pThis->hApp,
									pThis->waitingRequest,
									RV_RTSP_STATUS_RESPONSE_TIME_OUT,
									NULL,
                                    NULL);
	}

    return RV_TRUE;
}



/**************************************************************************
 * RtspSessionOnPingTimer
 * ------------------------------------------------------------------------
 * General: This function is called when a ping needs to be sent on the
 *          connection.
 *
 * Return Value:    RV_TRUE if the function is completed successfuly,
 *                  RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :   context     - The session to send the keep-alive on.
 * OUTPUT   :   None.
 * INOUT    :   None.
 *************************************************************************/
 static RvBool RtspSessionOnPingTimer(
                IN void *context)
 {
    RtspSession     *pThis = (RtspSession*)context;
    RvRtspRequest   *pRequest;
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RtspMsgRequest  *RtspMsgRequestMsg;
#else
    RvRtspRequest   request;
    pRequest = &request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
#endif

    /* if an error occurs we will not reschedule the timer  */
    if (pThis == NULL)
        return RV_FALSE;

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionOnPingTimer\r\n"));

#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raAdd(pThis->hRaRequests, (RAElement *)&RtspMsgRequestMsg);
    if (RtspMsgRequestMsg == NULL)
        return RV_FALSE;
    memset(RtspMsgRequestMsg, 0, sizeof(RtspMsgRequest));
    RtspMsgRequestMsg->hRtsp = pThis->hRtsp;
    RtspMsgRequestMsg->msgHeaders = NULL;
    RtspMsgRequestMsg->lastHeader = NULL;
    pRequest = &RtspMsgRequestMsg->request;
    memset(pRequest, 0, sizeof(RvRtspRequest));
    pRequest->hRtspMsgMessage = (RvRtspMsgMessageHandle)RtspMsgRequestMsg;
#endif

    pRequest->requestLine.method  = RV_RTSP_METHOD_OPTIONS;

    RtspSessionSendRequest((RvRtspSessionHandle)pThis, pRequest, RV_FALSE);
    RtspMessagesRequestDestruct(pThis->hRPool, pRequest);
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    raDelete(pThis->hRaRequests, (RAElement)RtspMsgRequestMsg);
#endif

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspSessionOnPingTimer\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_TRUE;
 }

 RVAPI
 RvStatus RVCALLCONV RvRtspSessionGetConnectionHandle(
     IN	RvRtspSessionHandle				    hSession,
     OUT RvRtspConnectionHandle              *phConnection)
{
    RtspSession     *pThis = (RtspSession*)hSession;

    if ((NULL == pThis) || (NULL == phConnection))
    {
        return RvRtspErrorCode(RV_ERROR_NULLPTR);
    }

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    *phConnection = (RvRtspConnectionHandle)pThis->pConnection;
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    return RV_OK;
}

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

#if defined(__cplusplus)
}
#endif
