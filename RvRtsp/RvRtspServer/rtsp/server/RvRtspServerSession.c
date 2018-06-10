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


/*******************************************************************************
*                            <RvRtspServerSession.c>
*
*  This file contains definitions relevant to the RTSP sessions.
*  An RTSP Session instance is a thread safe representation of a session 
*  conducted on top of an RTSP connection.
*
******************************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif

    /*----------------------------------------------------------------------------*/
    /*                           INCLUDE HEADER FILES                             */
    /*----------------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include <string.h>
#include <stdio.h>

#include "RvRtspServerTypes.h"
#include "RvRtspServerSession.h"
#include "RtspServerSessionInternal.h"
#include "RtspUtilsInternal.h"
#include "RtspMessages.h"
#include "RtspMsgInternal.h"

    /*----------------------------------------------------------------------------*/
    /*                             TYPE DEFINITIONS                               */
    /*----------------------------------------------------------------------------*/

    static RvInt                     randNum = 1;

    /*----------------------------------------------------------------------------*/
    /*                        STATIC FUNCTION PROTOTYPE                           */
    /*----------------------------------------------------------------------------*/

    /******************************************************************************
    * RtspServerSessionAddToList
    * ----------------------------------------------------------------------------
    * General: 
    *  Adds a session object to the list of sessions in the connection.
    *
    * Arguments:
    * Input:   sessionList         - Session list.
    *          newSession          - The session object to be added to the list.
    * Output:  sessionList         - Session list.
    *
    * Return Value:  RV_OK - if successful.
    *                Negative values otherwise.
    *****************************************************************************/
    static void RtspServerSessionAddToList(
        INOUT RtspServerSession     **sessionList,
        IN    RtspServerSession     *newSession);


    /******************************************************************************
    * RtspServerSessionDeleteFromList 
    * ----------------------------------------------------------------------------
    * General: 
    *  removes session object from the list of sessions in the connection.
    *
    * Arguments:
    * Input:   sessionList         - Session list.
    *          pSession            - The session object to be deleted from the list.
    * Output:  sessionList         - Session list.
    *
    * Return Value:  RV_OK - if successful.
    *                Negative values otherwise.
    *****************************************************************************/
    static void RtspServerSessionDeleteFromList(
        INOUT RtspServerSession         **sessionList,
        IN    RtspServerSession         *pSession);



    static RvBool RtspServerSessionOnPingTimer(
        IN void *context);

    static RvBool RvRtspTimerSetup(IN void *context);
    /*----------------------------------------------------------------------------*/
    /*                              MODULE FUNCTIONS                              */
    /*----------------------------------------------------------------------------*/


    /*******************************************************************************
    * RvRtspServerSessionConstruct
    * ----------------------------------------------------------------------------
    * General: 
    *  This API initializes the RTSP sessions structure. It constructs a new
    *  session and adds it to the connection RA object. It generates a session 
    *  ID if the application has not provided one. It then increments the
    *  session count in the connection. The new session starts in an INIT state.
    *
    * Arguments:
    * Input:   hConnection         -  The connection on which the new session is 
    *                                 to be constructed on.
    *          hApp                -  Application Handle used to access the 
    *                                 session.
    *          pConfiguration      -  Configuration for the session.
    *          configurationSize   -  Size of callbacks structure,
    *                                 is passed in order to distinguish between
    *                                 different versions of the RTSP Stack.
    *          pCallbacks          -  Callback functions to call on events
    *                                 populated by the application.
    *          callbacksStructSize -  Size of callbacks structure,
    *                                 is passed in order to distinguish between
    *                                 different versions of the RTSP Stack.
    *          phSessionId         -  Session Id is the unique identifier for a 
    *                                 session.
    * Output:  phSession           -  The handle to the constructed session used
    *                                 by the application to access the session.
    *          phSessionId         -  If this points to a NULL handle, then the
    *                                 application has not provided the sessionId
    *                                 and the stack needs to generate the sessionId,
    *                                 populate it in the session, and return the
    *                                 updated handle to the application.
    *                                 If it points to a non-null value, then we
    *                                 update the session with the provided sessionid 
    *                                 handle
    *
    * Return Value:  RV_OK - if successful.
    *                Negative values otherwise.
    ******************************************************************************/
    RVAPI RvStatus RVCALLCONV RvRtspServerSessionConstruct(
        IN  RvRtspServerConnectionHandle          hConnection,
        IN  RvRtspServerSessionAppHandle          hApp,
        IN  const RvRtspServerSessionConfiguration *pConfiguration,
        IN  const RvSize_t                        configurationSize,
        IN  const RvRtspServerSessionCallbackFunctions  *pCallbacks,
        IN  const RvSize_t                        callbacksStructSize,
        OUT RvRtspServerSessionHandle             *phSession,
        INOUT RvRtspStringHandle                  *phSessionId)
    {
        RtspServerSession         *pThis;
        RtspServerConnection      *pConnection;
        RvChar                    strSessionId[50];

        pThis        = NULL;
        pConnection  = (RtspServerConnection*)hConnection;

        if (pConnection == NULL)
            return RvRtspErrorCode(RV_ERROR_NULLPTR);

        if (sizeof(RvRtspServerSessionConfiguration) != configurationSize)
            return RvRtspErrorCode(RV_ERROR_BADPARAM);

        if (sizeof(RvRtspServerSessionCallbackFunctions) != callbacksStructSize)
            return RvRtspErrorCode(RV_ERROR_BADPARAM);

        RvMutexLock(&pConnection->mutex, pConnection->pLogMgr);
        RvLogEnter(pConnection->pLogSrc, (pConnection->pLogSrc, "RvRtspServerSessionConstruct\r\n"));

        /* We need to find out if we exceeded the session maximum        */
        /* instances allowed for the connection                          */

        if ( pConnection->sessionCount >= pConnection->maxSessionsPerConnection)
        {
            RvLogDebug(pConnection->pLogSrc, (pConnection->pLogSrc, "RvRtspServerSessionConstruct- Max Sessions reached\r\n"));
            RvLogLeave(pConnection->pLogSrc, (pConnection->pLogSrc, "RvRtspServerSessionConstruct\r\n"));
            RvMutexUnlock(&pConnection->mutex, pConnection->pLogMgr);
            return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
        }

        if (raAdd(pConnection->hRaSessions, (RAElement*)&pThis) < 0)
        {
            RvMutexUnlock(&pConnection->mutex, pConnection->pLogMgr);
            return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
        }

        /* Increment the sessionCount in the connection                  */
        pConnection->sessionCount++;

        /* We need to construct a new session and insert it to  the      */
        /* connection object RA.                                         */
        pThis->pLogMgr              = pConnection->pLogMgr;
        pThis->pLogSrc              = pConnection->pLogSrc;
        pThis->hRPool               = pConnection->hRPool;
        pThis->pSelectEngine        = pConnection->pSelectEngine;
        pThis->pConnection          = pConnection;
        pThis->hRaRequests          = pConnection->hRaRequests;
        pThis->hRaResponses         = pConnection->hRaResponses;
        pThis->hRtsp                = pConnection->hRtsp;
        pThis->hApp                 = hApp;

        memcpy(&pThis->pfnCallbacks, pCallbacks, sizeof(RvRtspServerSessionCallbackFunctions));

        pThis->pGMutex              = pConnection->pGMutex;

        memcpy(&pThis->configuration, pConfiguration, sizeof(RvRtspServerSessionConfiguration));

        pThis->destructed           = RV_FALSE;
        pThis->state                = RV_RTSP_SESSION_STATE_INIT;
        pThis->hURI                 = (HRPOOLELEM)pConfiguration->strURI;
        pThis->waitingClientRequest = RV_RTSP_METHOD_SETUP; 
        pThis->pNext                 = NULL;
        pThis->pPrev                 = NULL;

        pThis->requestTimer = 0;
        pThis->pingTimerSet = RV_FALSE;


        if (*phSessionId == NULL)
        {
            /* The application has not provided a sessionId. We need to set the
            sessionId */

            /* Get a random number to use as the sessionID */
            sprintf(strSessionId, "%d", randNum);

            /* Set the sessionID in the session object */
            pThis->hSessionId = rpoolAllocCopyExternal(pConnection->hRPool,
                0,
                strSessionId,
                strlen(strSessionId)+1);

            /* Return a handle to the sessionID to the application */
            *phSessionId = (RvRtspStringHandle)rpoolAllocCopyExternal(pConnection->hRPool,
                0,
                strSessionId,
                strlen(strSessionId)+1);

            /* Increment the randNum */
            randNum++;

        }
        else
        {
            /* The application has provided a sessionId - need to copy it */
            /* copying the string to the buffer             */
            RtspUtilsHPOOLELEMStrCpy(pThis->hRPool, (HRPOOLELEM)*phSessionId, 0, strSessionId, 50);

            pThis->hSessionId = rpoolAllocCopyExternal(pConnection->hRPool,
                0,
                strSessionId,
                strlen(strSessionId)+1);
        }

        RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);

        *phSession                  = (RvRtspServerSessionHandle)pThis;

        RtspServerSessionAddToList((RtspServerSession **)&pConnection->hSessionList, pThis);

        RvLogLeave(pConnection->pLogSrc, (pConnection->pLogSrc, "RtspServerSessionConstruct\r\n"));
        RvMutexUnlock(&pConnection->mutex, pConnection->pLogMgr);

        return RV_OK;
    }



    /*******************************************************************************
    * RvRtspServerSessionDestruct
    * ----------------------------------------------------------------------------
    * General: 
    *  This API is called to destruct an RTSP session. It frees up the resources 
    *  used by the session, release the RPool elements in the session and deletes
    *  the session from the list of sessions in the connection. It is called when
    *  TEARDOWN is requested or a connection is disconnected.
    *
    * Arguments:
    * Input:   hSession    - the handle to the session being destructed. It is 
    *                        to the access the session to be destructed.
    * Output:  None.
    *
    * Return Value:  RV_OK - if successful.
    *                Negative values otherwise.
    ******************************************************************************/
    RVAPI RvStatus RVCALLCONV RvRtspServerSessionDestruct(
        IN  RvRtspServerSessionHandle hSession)
    {
        RtspServerSession     *pThis;
        RtspServerConnection  *pConnection;
        RvStatus              result;

        pThis         = (RtspServerSession*)hSession;
        if (pThis == NULL)
        {
            return RvRtspErrorCode(RV_ERROR_BADPARAM);
        }
        pConnection   = (RtspServerConnection*)pThis->pConnection;
        result        = RV_OK;

        if ((pConnection == NULL) || 
            (pConnection->hSessionList == NULL))
        {
            return RvRtspErrorCode(RV_ERROR_NULLPTR);
        }

        RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

        if (pThis->destructed == RV_TRUE)
        {
            RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
            return RvRtspErrorCode(RV_ERROR_DESTRUCTED);
        }

        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerSessionDestruct\r\n"));

        /* freeing the session's used resources */
        if (pThis->hURI)
        {
            rpoolFree(pThis->hRPool, pThis->hURI);
            pThis->hURI = NULL;

        }
        if (pThis->pingTimerSet)
        {
            RvTimerCancel(&pThis->pingTimer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        }

        if (pThis->hSessionId)
        {
            rpoolFree(pThis->hRPool, pThis->hSessionId);
            pThis->hSessionId = NULL;
        }

        pThis->destructed   = RV_TRUE;

        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerSessionDestruct\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);

        RtspServerSessionDeleteFromList((RtspServerSession**)&pConnection->hSessionList,
            (RtspServerSession*)hSession);

        raDelete(pConnection->hRaSessions, (RAElement)pThis);

        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

        /* decrease the sessionCount in the connection     */
        pConnection->sessionCount--;

        /* If this is the last session in the connection, disconnect the connection */
        if (pConnection->sessionCount == 0)
        {
            if (pConnection->pfnCallbacks.pfnServerOnDisconnectEv!= NULL)
            {
                result = pConnection->pfnCallbacks.pfnServerOnDisconnectEv(
                    (RvRtspServerConnectionHandle)pConnection,
                    (RvRtspServerConnectionAppHandle)pConnection->hApp);
            }
        }

        return result;
    }


    /*******************************************************************************
    * RvRtspServerSessionSendResponse
    * ----------------------------------------------------------------------------
    * General: 
    *  This method sends the response on a session for the request received on 
    *  that session. It then changes the session's state as required and then
    *  generates the RvRtspServerOnStateChangeEv callback.
    *  
    *
    * Arguments:
    * Input:   hSession    - the session handle used to access the session.
    *          pResponse   - response to be sent. It is populated based on the 
    *                        request received.
    * Output:  None.
    *
    * Return Value:  RV_OK - if successful.
    *                Negative values otherwise.
    *******************************************************************************/
    RVAPI RvStatus RVCALLCONV RvRtspServerSessionSendResponse (
        IN     RvRtspServerSessionHandle     hSession,
        IN     RvRtspResponse                *pResponse)
    {
        RtspServerSession             *pThis;
        RvRtspServerSessionState      currState;
        RvRtspServerSessionState      newState;
        RvStatus                      result;

        pThis         = (RtspServerSession*)hSession;
        currState     = RV_RTSP_SESSION_STATE_INIT;
        newState      = RV_RTSP_SESSION_STATE_INIT;
        result        = RV_OK;

        if ((pThis == NULL) || (pResponse == NULL))
        {
            return RvRtspErrorCode(RV_ERROR_NULLPTR);
        }

        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerSessionSendResponse\r\n"));

        /* Notify the application that a message is about to be sent */
        if (pThis->pfnCallbacks.pfnServerOnSendEv != NULL)
        {
            pThis->pfnCallbacks.pfnServerOnSendEv(hSession,
                pThis->hApp, 
                NULL,
                pResponse);

        }
        //pThis->requestTimer =  RvTimestampGet(pThis->pLogMgr);
        result = RtspServerConnectionSendResponse(pThis->pConnection, pResponse);

        if (result == RV_OK)
        {
            currState = pThis->state;

            switch(currState)
            {
            case RV_RTSP_SESSION_STATE_INIT:
                {
                    if (pThis->waitingClientRequest == RV_RTSP_METHOD_SETUP)
                        newState = RV_RTSP_SESSION_STATE_READY;
                    break;
                }
            case RV_RTSP_SESSION_STATE_READY:
                {
                    if (pThis->waitingClientRequest == RV_RTSP_METHOD_PLAY)
                        newState = RV_RTSP_SESSION_STATE_PLAYING;

                    else if(pThis->waitingClientRequest == RV_RTSP_METHOD_PAUSE) 
                        newState = RV_RTSP_SESSION_STATE_READY;
                    else if (pThis->waitingClientRequest == RV_RTSP_METHOD_SETUP)
                        newState = RV_RTSP_SESSION_STATE_READY;

                    else if (pThis->waitingClientRequest == RV_RTSP_METHOD_TEARDOWN)
                        newState = RV_RTSP_SESSION_STATE_INIT;

                    else if (pThis->waitingClientRequest == RV_RTSP_METHOD_GET_PARAMETER)
                        newState = currState;
                    //modified by lichao, 20140920 处理options心跳
                    else if (RV_RTSP_METHOD_OPTIONS == pThis->waitingClientRequest)
                    {
                        newState = currState;
                    }

                    break;
                }
            case RV_RTSP_SESSION_STATE_PLAYING:
                {
                    if(pThis->waitingClientRequest == RV_RTSP_METHOD_PLAY) 
                        newState = RV_RTSP_SESSION_STATE_PLAYING;

                    else if(pThis->waitingClientRequest == RV_RTSP_METHOD_PAUSE) 
                        newState = RV_RTSP_SESSION_STATE_READY;

                    else if(pThis->waitingClientRequest == RV_RTSP_METHOD_SETUP)
                        newState = RV_RTSP_SESSION_STATE_READY;
                    else if(pThis->waitingClientRequest == RV_RTSP_METHOD_TEARDOWN)
                        newState = RV_RTSP_SESSION_STATE_INIT;
                    else if (pThis->waitingClientRequest == RV_RTSP_METHOD_GET_PARAMETER)
                        newState = currState;
                    //modified by lichao, 20140920 处理options心跳
                    else if (RV_RTSP_METHOD_OPTIONS == pThis->waitingClientRequest)
                    {
                        newState = currState;
                    }

                    break;
                }
            default:
                RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerSessionSendResponse- state invalid\r\n"));
                break;
            }

            pThis->state = newState;
        } /* end of if (result == RV_OK)  */

        else
        {
            RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerSessionSendResponse- send response failed\r\n"));
        }

        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RvRtspServerSessionSendResponse\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        /* perform session external operations */

        if( result == RV_OK)
        {
            /* Session callback to notify state change */
            if (pThis->pfnCallbacks.pfnServerOnStateChangeEv != NULL)
            {
                result = pThis->pfnCallbacks.pfnServerOnStateChangeEv(
                    (RvRtspServerSessionHandle)pThis,
                    (RvRtspServerSessionAppHandle)pThis->hApp,
                    currState,
                    newState);
            }
        }

        return result;
    }

    /**************************************************************************
    * RtspServerSessionInit
    * ------------------------------------------------------------------------
    * General: 
    *  This method constructs an RA object for the session and in effect, 
    *  initializes it.
    *
    * Arguments:
    * Input:   maxSessions - maximum number of sessions that can exist at one
    *                        time on the server.
    *          logMgr      - log manager used.
    * Output:  None.
    *
    * Return Value:  HRA - constructed RA element.
    *************************************************************************/
    HRA RtspServerSessionInit(
        IN RvUint           maxSessions,
        IN RvLogMgr*        logMgr)
    {
        return  raConstruct(
            sizeof(RtspServerSession), /* Elements size  */
            maxSessions              , /* Max number of sessions */ 
            RV_TRUE                  , /* Thread safety   */
            "Sessions per Connection", /* RA name         */
            logMgr);                   /* Log Manager     */     
    }

    /**************************************************************************
    * RtspServerSessionHandleRequest
    * ------------------------------------------------------------------------
    * General: 
    *  This method handles a request which was received on the
    *  session and generates the RvRtspServerSessionOnReceiveEv callback.
    *
    * Arguments:
    * Input:   pThis       - Pointer to the Session on which the request 
    *                        was received.
    *          pRequest    - Pointer to the received request structure.
    * Output:  None.
    *
    * Return Value:  RV_OK - if successful.
    *                Negative values otherwise.
    *************************************************************************/
    RvStatus RtspServerSessionHandleRequest(
        IN RtspServerSession      *pThis,
        IN RvRtspRequest          *pRequest)
    {
        RvStatus       result;
        RvChar         reqStrURI[100];
        RvChar         sessStrURI[100];
        int success = 0;

        memset((void *)reqStrURI,'\0',sizeof(reqStrURI));
        memset((void *)sessStrURI,'\0',sizeof(sessStrURI));

        result       = RV_OK;

        if ((pThis == NULL) || (pRequest == NULL))
        {
            return RvRtspErrorCode(RV_ERROR_NULLPTR);
        } 

        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerSessionHandleResponse\r\n"));

        pThis->requestTimer =  RvTimestampGet(pThis->pLogMgr);
        pThis->waitingClientRequest = pRequest->requestLine.method;

        RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspServerSessionHandleResponse\r\n"));
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

        RtspUtilsHPOOLELEMStrCpy(pThis->hRPool,  (HRPOOLELEM)pRequest->requestLine.hURI, 0, reqStrURI, 100);
        RtspUtilsHPOOLELEMStrCpy(pThis->hRPool,  (HRPOOLELEM)pThis->configuration.strURI, 0, sessStrURI, 100);


        if (strstr(reqStrURI, sessStrURI) == NULL)
        {
            success = 1;

            //options * 心跳处理
            if ( (NULL!=pRequest&&RV_RTSP_METHOD_OPTIONS == pRequest->requestLine.method) ||
                (NULL!=pRequest&&RV_RTSP_METHOD_GET_PARAMETER == pRequest->requestLine.method) )
            {
                success = 1;
            }
            else if ((NULL != pRequest) && (RV_RTSP_METHOD_OPTIONS == pRequest->requestLine.method) && (0 == strcmp(reqStrURI, "*")))
            {
                success = 1;
            }
            else if (pThis->pfnCallbacks.pfnServerOnErrorEv != NULL)
            {
                printf("reqStrURI[%s] sessStrURI[%s]\n", reqStrURI,sessStrURI);
                // edit by zhouzx 2015/11/06
                //关闭消息异常
                //             result = pThis->pfnCallbacks.pfnServerOnErrorEv(
                //                      (RvRtspServerSessionHandle)pThis,
                //                      (RvRtspServerSessionAppHandle)pThis->hApp,
                //                      pRequest->requestLine.method,
                //                      RV_RTSP_STATUS_BAD_REQUEST,
                //                      NULL);
            }
        }
        else
        {
            success = 1;
        }

        if (1 == success)
        { 
            /* Session callback on message receive                       */
            if (pThis->pfnCallbacks.pfnServerOnReceiveEv != NULL)
            {
                result = pThis->pfnCallbacks.pfnServerOnReceiveEv(
                    (RvRtspServerSessionHandle)pThis, 
                    (RvRtspServerSessionAppHandle)pThis->hApp,
                    pRequest);
            }
        }
        RvRtspTimerSetup((void*)pThis);
        return result;
    }

    /******************************************************************************
    * RtspServerSessionAddToList
    * ----------------------------------------------------------------------------
    * General: 
    *  Adds a session object to the linked list of sessions in the connection.
    *
    * Arguments:
    * Input:   sessionList         - Pointer to the session list.
    *          newSession          - The session object to be added to the list.
    * Output:  sessionList         - Pointer to the updated Session list.
    *
    * Return Value:  None.
    *****************************************************************************/
    static void RtspServerSessionAddToList(
        INOUT RtspServerSession     **sessionList, 
        IN    RtspServerSession     *newSession)
    {
        RtspServerSession     *pListStart;   /* Pointer to session list */
        RtspServerSession     *pLastSession; /* Temp variable used to find the last session in 
                                             the list */

        pListStart     = *sessionList; 
        pLastSession   = pListStart;

        /* If the list is NULL , this Session is the first element of the list */
        if (pListStart == NULL)
        {
            pListStart = newSession;

            /* Update the sessionList  */
            *sessionList   = pListStart;

        }
        else
        {
            /* locate the last session in the list  */
            while (pLastSession->pNext != NULL)
            {
                pLastSession = pLastSession->pNext;
            }

            /* Add the new session to the list  */ 
            pLastSession->pNext   = newSession;
            newSession->pPrev     = pLastSession;
        }

    }

    /******************************************************************************
    * RtspServerSessionDeleteFromList 
    * ----------------------------------------------------------------------------
    * General: 
    *  Removes session object from the linked list of sessions in the connection.
    *
    * Arguments:
    * Input:   sessionList      - Pointer to the list of Sessions in the 
    *                             connection.
    *          pSession         - The session object to be deleted from the list.
    * Output:  sessionList      - The updated list of Sessions.
    *
    * Return Value:  None.
    *****************************************************************************/
    static void RtspServerSessionDeleteFromList(
        INOUT RtspServerSession         **sessionList,
        IN    RtspServerSession         *pSession) 
    {
        RtspServerSession    *listStart;

        listStart = *sessionList;

        if (listStart == NULL)
            return; 

        if (pSession == listStart)
        {
            /* Removing the element from the head of the list */
            listStart = listStart->pNext;

            /* If the session is not the last session in the list */
            if (listStart != NULL)
                listStart->pPrev = NULL;

            /* Assign the new list to the sessionList  */
            *sessionList = listStart;
        }
        else
        {
            /* Remove the session from the list   */
            pSession->pPrev->pNext = pSession->pNext;
            if (pSession->pNext != NULL)
                pSession->pNext->pPrev = pSession->pPrev;
        }

        pSession->pNext = NULL;
        pSession->pPrev = NULL;

    }


    static RvBool RtspServerSessionOnPingTimer(
        IN void *context)
    {
        RtspServerSession *pThis = (RtspServerSession*)context;
        if (NULL == pThis)
        {
            return RV_FALSE;
        }
        do
        {
            RvInt64 now = RvTimestampGet(pThis->pLogMgr);
            if (0 == pThis->requestTimer || 0 == pThis->configuration.timeOutInterval )
            {
                break;
            }
            if ((now - pThis->requestTimer) > pThis->configuration.timeOutInterval*1000000ULL)
            {
                pThis->pfnCallbacks.pfnServerOnErrorEv((RvRtspServerSessionHandle)pThis,
                    pThis->hApp,RV_RTSP_METHOD_OPTIONS,RV_RTSP_STATUS_REQUEST_TIME_OUT,NULL);
            }
        } while (0);

        return RV_TRUE;
    }

    static RvBool RvRtspTimerSetup(IN void *context)
    {
        RtspServerSession             *pThis;
        RvTimerFunc         timeoutCallback;	/* select engine's timeout function	*/
        RvTimerQueue        *pTQueue;           /* select engine's timer queue      */

        pThis         = (RtspServerSession*)context;
        if (pThis == NULL)
        {
            return RV_FALSE;
        }

        if (!pThis->pingTimerSet && pThis->configuration.checkTimerInterval >0)
        {
            RvSelectGetTimeoutInfo(pThis->pSelectEngine, &timeoutCallback, &pTQueue);
            RvTimerStart(&pThis->pingTimer,          /* timer object         */
                pTQueue,                             /* timer's queue        */
                RV_TIMER_TYPE_PERIODIC,              /* PERIODIC or ONESHOT */
                pThis->configuration.checkTimerInterval*(RV_TIME64_NSECPERSEC/1000),    /* time in nanoseconds  */
                RtspServerSessionOnPingTimer,             /* callback to call     */
                (void*)pThis);                           /* context for the timer*/
            pThis->pingTimerSet = RV_TRUE;
        }
        return RV_TRUE;
    }
#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#if defined(__cplusplus)
}
#endif

