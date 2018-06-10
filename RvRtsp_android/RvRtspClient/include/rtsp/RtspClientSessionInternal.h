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
 *                              <RtspClientSessionInternal.h>
 *
 *	This file contains definitions relevant to the RTSP sessions.
 *	An RTSP Session instance is a thread safe representation of a session conducted
 *	on top of an RTSP connection.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_SESSION_INTERNAL_H
#define _RV_RTSP_SESSION_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvcbase.h"
#include "rvares.h"
#include "rvmutex.h"
#include "rvlog.h"

#include "RvRtspClientTypes.h"
#include "RvRtspClientSession.h"
#include "RtspClientConnectionInternal.h"

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* RvRtspSession
 * ----------
 * the RvRtspSession structure holds all the information relevant for the session.
 * The structure holds a the session-id, the application context, and a pointer to
 * the RTSP connection.
 * The application may only issue one request at a time.
 */
typedef struct RtspSession_
{
	RvLogMgr					*pLogMgr;			/* Log manager for the Session		*/
	RvLogSource					*pLogSrc;			/* Log source for the Session		*/
    RvRtspHandle                hRtsp;              /* The Rtsp object handle           */
	HRPOOL						hRPool;				/* The module's memory pool			*/
    HRA                         hRaRequests;        /* The RtspMsgRequests RA on the rtsp object */
    HRA                         hRaResponses;       /* The RtspMsgResponses RA on the rtsp object */
	RvRtspSessionAppHandle		hApp;				/*Context for the callback functions*/
	RvSelectEngine				*pSelectEngine;		/* Select engine 					*/

	RtspConnection				*pConnection;		/* The connection the session is on	*/

	RvRtspSessionCallbackFunctions pfnCallbacks;	/* On event callback functions		*/

	RvMutex						mutex;				/* Guard against collisions			*/
	RvMutex						*pGMutex;			/* Global guard against				*/
													/* destruct/construct overlap		*/
    RvBool                      destructed;

	RvRtspSessionState			state;				/* The current session state		*/
	HRPOOLELEM					hURI;				/* Session's current URI, set by	*/
													/* the SetUri function				*/
	HRPOOLELEM					hSessionId;			/* the RTSP Session-Id				*/


	RvBool						waitingForResponse;	/* Has a request been issued		*/
	RvRtspMethod				waitingRequest;		/* which method was requested		*/
	RvUint16					waitingRequestCSeq;	/* CSeq of the request waiting for	*/
													/* response							*/
    RvUint16                    lastPingCSeq;       /* The CSeq of the last sent ping   */
	RvRtspSessionConfiguration	configuration;		/* The session's configuration		*/

	RvTimer						pingTimer;			/* timer for keep-alive				*/
	RvBool						pingTimerSet;		/* is the ping timer set			*/
	RvUint						sessionTimeout;		/* time period to wait between pings*/

	RvTimer						responseTimer;		/* timer for requests timeout		*/
	RvBool						responseTimerSet;	/* is the response timer set		*/
} RtspSession;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * RtspSessionHandleResponse
 * ------------------------------------------------------------------------
 * General: This method handles a response which was received on the
 *			session.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- The Session on which the response was received.
 *				pResponse	- the received response.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
 RvStatus RtspSessionHandleResponse(
					IN RtspSession		*pThis,
					IN RvRtspResponse	*pResponse);


#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SESSION_INTERNAL_H*/


