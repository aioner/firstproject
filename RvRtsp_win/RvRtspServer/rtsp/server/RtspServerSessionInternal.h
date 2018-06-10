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
 *                       <RtspServerSessionInternal.h>
 *
 * This file contains definitions relevant to the RTSP sessions.
 * An RTSP Session instance is a thread safe representation of a session 
 * conducted on top of an RTSP connection.
 *
 ******************************************************************************/

#ifndef _RV_RTSP_SERVER_SESSION_INTERNAL_H
#define _RV_RTSP_SERVER_SESSION_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif


/*---------------------------------------------------------------------------*/
/*                           INCLUDE HEADER FILES                            */
/*---------------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvcbase.h"
#include "rvares.h"
#include "rvmutex.h"
#include "rvlog.h"

#include "RvRtspServerTypes.h"
#include "RvRtspServerSession.h"
#include "RtspServerConnectionInternal.h"

/*---------------------------------------------------------------------------*/
/*                             TYPE DEFINITIONS                              */
/*---------------------------------------------------------------------------*/


/* 
 * RtspServerSession
 * ----------------------------------------------------------------------------
 * The RtspServerSession structure holds all the information relevant for the 
 * session.  This structure holds a the session-id, the application context, 
 * and a pointer to the RTSP connection.
 */
typedef struct RtspSession_
{
    RvLogMgr                                *pLogMgr;             
        /* Log manager for the Session */

    RvLogSource                             *pLogSrc;          
        /* Log source for the Session */

    HRPOOL                                  hRPool;                 
        /* The module's memory pool */

    RvSelectEngine                          *pSelectEngine;       
        /* Select engine */

    RtspServerConnection                    *pConnection;   
        /* The connection for the session */

    RvRtspServerSessionAppHandle            hApp;
        /* Context for the callback functions */

    RvRtspServerSessionCallbackFunctions    pfnCallbacks;  
        /* On event callback functions */

    RvMutex                                 mutex;                  
        /* Guard against collisions */

    RvMutex                                 *pGMutex;              
        /* Global guard against destruct/construct overlap */

    RvBool                                  destructed;
        /* Whether the session is already destroyed */

    RvRtspServerSessionState                state; 
        /* The current session state */

    RvRtspHandle                            hRtsp;
        /* The Rtsp object handle */

    HRPOOLELEM                              hURI;               
        /* Session's current URI */

    HRPOOLELEM                              hSessionId;         
        /* the RTSP Session-Id */

    HRA                                     hRaRequests;
        /* RA of RtspMsgRequests to be used when using RtspMsg module */

    HRA                                     hRaResponses;
        /* RA of RtspMsgResponses to be used when using RtspMsg module */

    RvRtspMethod                            waitingClientRequest;   
        /* which method was requested by client */

    RvRtspServerSessionConfiguration        configuration;  
        /* The session's configuration */

    struct RtspSession_                     *pNext;
        /* Points to next element in the list */

    struct RtspSession_                     *pPrev;
        /* Points to the previous element in the list */

    RvTimer     pingTimer;
       /* timer for keep-alive*/
    RvBool      pingTimerSet; 
       /* is the ping timer set*/

    RvInt64    requestTimer;
     /* the time of recv request.*/
} RtspServerSession;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RtspServerSessionHandleRequest
 * ------------------------------------------------------------------------
 * General: 
 *  This method handles a request which was received on the
 *  session.
 *
 * Arguments:
 * Input:   pThis       - The Session on which the response was received.
 *          pRequest    - The received request.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerSessionHandleRequest(
    IN RtspServerSession      *pThis,
    IN RvRtspRequest          *pRequest);

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SERVER_SESSION_INTERNAL_H*/


