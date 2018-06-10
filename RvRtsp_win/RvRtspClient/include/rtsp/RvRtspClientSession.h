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
 *                              <RvRtspClientSession.h>
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

#ifndef _RV_RTSP_SESSION_H
#define _RV_RTSP_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER)
#include "RvRtspClientTypes.h"
#include "RvRtspMessageTypes.h"


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* RvRtspSessionState
 * ----------
 * This structure is used to tell the user the state of the session, and
 * to allow only permitted actions (e.g. can't issue a PLAY request without
 * issuing a SETUP request first).
 */
typedef enum
{
	RV_RTSP_SESSION_STATE_INIT = 0,		/* State after initialization and	*/
										/* after TEARDOWN was acknowledged	*/
	RV_RTSP_SESSION_STATE_READY,		/* SETUP or PAUSE acknowledged		*/
	RV_RTSP_SESSION_STATE_PLAYING,		/* PLAY acknowledged				*/
	RV_RTSP_SESSION_STATE_RECORDING		/* RECORD acknowledged				*/
} RvRtspSessionState;


/* RvRtspSessionConfiguration
 * ----------
 * This structure specifies the configuration for the Session object.
 */
typedef struct RvRtspSessionConfiguration_
{
	RvUint32	responseTimeOutResolution;			/* in milliseconds units*/
	RvUint32	pingTransmissionTimeOutResolution;	/* in milliseconds units*/
} RvRtspSessionConfiguration;



/**************************************************************************
 * RvRtspSessionOnDestruct
 * ------------------------------------------------------------------------
 * General: This callback is called by the Session layer when an someone (
 *			The user or the connection layer) calls the Session Destruct
 *			function.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession	- The session which is destructed.
 *				hApp		- Application context.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspSessionOnDestructEv)(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp);


/**************************************************************************
 * RvRtspSessionOnStateChangeEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Session layer when an RTSP
 *			response indicating request success is received on the
 *			connection for the session.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession	- The session who's state was changed.
 *				hApp		- Application context.
 *				currState	- Current session state.
 *				newState	- New session state.
 *				pResponse	- the received response.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspSessionOnStateChangeEv)(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspSessionState		currState,
						IN	RvRtspSessionState		newState,
						IN	const RvRtspResponse	*pResponse);



/**************************************************************************
 * RvRtspSessionOnErrorEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the session layer an RTSP
 *			response indicating error was received on the connection for
 *			the session.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession		- The session on which the error was received.
 *				hApp			- Application context.
 *				requestMethod	- The requested method.
 *				status			- The returned status.
 *				hphrase			- The status phrase.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspSessionOnErrorEv)(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspMethod			requestMethod,
						IN	RvRtspStatus			status,
						IN	RvRtspStringHandle		hPhrase);




/**************************************************************************
 * RvRtspSessionOnErrorExtEv
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the session layer an RTSP
 *	response indicating error was received on the connection for
 *	the session or a timeout occurred on a session request message. 
 *  It provides the error message if there is one.
 *
 * Arguments:
 * Input:	    hSession		- The session on which the error was received.
 *				hApp			- Application context.
 *				requestMethod	- The requested method.
 *				status			- The returned status.
 *				hphrase			- The status phrase.
 *				pResponse	    - the received response.
 * Output:	    None.
 *
 * Return Value:RV_OK if successful, 
 *              negative values otherwise.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspSessionOnErrorExtEv)(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspSessionAppHandle	hApp,
						IN	RvRtspMethod			requestMethod,
						IN	RvRtspStatus			status,
						IN	RvRtspStringHandle		hPhrase,
						IN	const RvRtspResponse	*pResponse);

/******************************************************************************
 * RvRtspSessionOnReceiveEv
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called when an RTSP message is received on a 
 *  client's session.
 *  This callback can be used to retrieve additional headers or header fields
 *  that are not specifically supported by the client.
 *
 * Arguments:
 * Input:       hSession         - The session handle in the client stack.
 *              hApp             - The handle of the application session object .
 *              pRequest         - The received message if it is a request,
 *                                 NULL if it is a response.
 *              pResponse        - The received message if it is a response,
 *                                 NULL if it is a request.
 *
 * Output:      None.
 *
 * Return Value:RV_OK if successful, 
 *              negative values otherwise.
 *****************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspSessionOnReceiveEv)( 
                        IN	RvRtspSessionHandle	    hSession,
                        IN 	RvRtspSessionAppHandle	hApp, 
                        IN	RvRtspRequest			*pRequest,
                        IN	RvRtspResponse			*pResponse);

/******************************************************************************
 * RvRtspSessionOnSendEv
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called when an RTSP messageis sent on a client's
 *  session. This callback can be used to edit the outgoing message and add
 *  additional headers or header fields that are not specifically 
 *  supported by the client.
 *
 * Arguments:
 * Input:       hSession         - The session handle in the client stack.
 *              hApp             - The handle of the application session object .
 *              pRequest         - The received message if it is a request,
 *                                 NULL if it is a response.
 *              pResponse        - The received message if it is a response,
 *                                 NULL if it is a request.
 *
 * Output:      None.
 *
 * Return Value:RV_OK  - if successful.
 *              Other on failure
 *****************************************************************************/
typedef	RvStatus (RVCALLCONV *RvRtspSessionOnSendEv)( 
                        IN	RvRtspSessionHandle		hSession,
                        IN 	RvRtspSessionAppHandle	hApp, 
                        IN	RvRtspRequest			*pRequest,
                        IN	RvRtspResponse			*pResponse);



/* RvRtspConnectionCallbackFunctions
 * ----------
 * This structure specifies the session's callback functions.
 */
typedef struct RvRtspSessionCallbackFunctions_
{
	RvRtspSessionOnStateChangeEv	pfnOnStateChangeEv;	/* called on successfull*/
														/* state change			*/
	RvRtspSessionOnErrorEv			pfnOnErrorEv;		/* called on error		*/
	RvRtspSessionOnDestructEv		pfnOnDestructEv;	/* called on destruct   	*/
	RvRtspSessionOnErrorExtEv		pfnOnErrorExtEv;	/* called on error provides */
                                                        /* the response message     */
    RvRtspSessionOnReceiveEv		pfnOnReceiveEv;     /* Called when a message was received on the session */
	RvRtspSessionOnSendEv		    pfnOnSendEv;        /* Called when a message is about to be sent on the session */

} RvRtspSessionCallbackFunctions;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RvRtspSessionConstruct
 * ------------------------------------------------------------------------
 * General: This method initializes the RTSP session structure.
 * 			The session starts in an INIT state.
 *
 * Return Value:	RV_OK - If the RTSP session is successfully initialized,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection			- The connection the session is on.
 *				pConfiguration		- Configuration for the session.
 *              configurationSize   - size of callbacks structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 *				pCallbacks			- Callback functions to call on events.
 *              callbacksStructSize - size of callbacks structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 *				hApp				- Application context.
 * OUTPUT	:	phSession			- The constructed session.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionConstruct(
						IN	RvRtspConnectionHandle			hConnection,
						IN	RvRtspSessionConfiguration		*pConfiguration,
                        IN	const RvSize_t                  configurationSize,
                        IN	RvRtspSessionCallbackFunctions	*pCallbacks,
                        IN	const RvSize_t                  callbacksStructSize,
                        IN	RvRtspSessionAppHandle			hApp,
						OUT	RvRtspSessionHandle				*phSession);



/**************************************************************************
 * RvRtspSessionDestruct
 * ------------------------------------------------------------------------
 * General: This method destructs the RTSP session structure.
 *
 * Return Value:	RV_OK - When the session is successfully de-initialized,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession	- the session being destructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionDestruct(
						IN	RvRtspSessionHandle	hSession);


/**************************************************************************
 * RvRtspSessionGetState
 * ------------------------------------------------------------------------
 * General: This method returns the state of the RTSP session.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	: hSession		- the session object.
 * OUTPUT	: pState		- the returned session state.
 * INOUT	: None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionGetState(
						IN	RvRtspSessionHandle	hSession,
						OUT RvRtspSessionState	*pState);



/**************************************************************************
 * RvRtspSessionGetId
 * ------------------------------------------------------------------------
 * General: This method returns the session Id.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession		- the session object.
 * OUTPUT	:	phSessionId		- string containing the RTSP Session-Id.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionGetId(
						IN	RvRtspSessionHandle	hSession,
						OUT RvRtspStringHandle	*phSessionId);



/**************************************************************************
 * RvRtspSessionSetUri
 * ------------------------------------------------------------------------
 * General: This method sets the RTSP session's URI. This is done so
 *			requests can be made on this session with a different URI. the
 *			URI is used as the target of the requests made to the server.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession		- The session object.
 *				strURI			- The new URI.
 * OUTPUT	:	None
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionSetUri(
						IN	RvRtspSessionHandle	hSession,
						IN	const RvChar		*strURI);





/**************************************************************************
 * RvRtspSessionSetup
 * ------------------------------------------------------------------------
 * General: This method sends a SETUP request on the session.
 *			This sets up an RTSP session with the server using the
 *			session's URI.
 *			The application will be notified by a callback when a response
 *			arrives or a timeout occurs.
 *
 *          Upon successful (or not) of state change the session layer will 
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hSession			- The session object.
 *				pTransportHeader	- The SETUP request's transport header.
 * OUTPUT	:	None
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionSetup(
						IN	RvRtspSessionHandle		hSession,
						IN	RvRtspTransportHeader	*pTransportHeader);

//added by lichao, 20141217 外部指定cseq
RVAPI
RvStatus RVCALLCONV RvRtspSessionSetupEx(
                               IN	RvRtspSessionHandle		hSession,
                               IN   RvUint16                cseq,
                               IN	RvRtspTransportHeader	*pTransportHeader);


/**************************************************************************
 * RvRtspSessionPlay
 * ------------------------------------------------------------------------
 * General: This method sends a PLAY request on the session, this signals
 * 			the server to start playing.
 *			If the time is NULL, playing starts immediately, if an NPT time
 *			is specified, playing is started from that position.
 *
 *          Upon successful (or not) of state change the session layer will 
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	: hSession		- the session object.
 *			  pNptTime		- the time to start playing from.
 * OUTPUT	: None
 * INOUT	: None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionPlay(
						IN RvRtspSessionHandle	hSession,
						IN RvRtspNptTime		*pNptTime);

//added by lichao, 20141217 外部指定cseq
RVAPI
RvStatus RVCALLCONV RvRtspSessionPlayEx(
                                      IN RvRtspSessionHandle    hSession,
                                      IN RvUint16               cseq,
                                      IN RvRtspNptTime          *pNptTime,
                                      IN float                  scale);

/**************************************************************************
 * RvRtspSessionPause
 * ------------------------------------------------------------------------
 * General: This method sends a PAUSE request on the session.
 *			This signals the server to pause the RTP transmission.
 *
 *          Upon successful (or not) of state change the session layer will 
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	: hSession		- the session object.
 * OUTPUT	: None
 * INOUT	: None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionPause(
						IN RvRtspSessionHandle	hSession);


//added by lichao, 20141217 外部指定cseq
RVAPI
RvStatus RVCALLCONV RvRtspSessionPauseEx(
                                        IN RvRtspSessionHandle    hSession,
                                        IN RvUint16               cseq);

/**************************************************************************
 * RvRtspSessionTeardown
 * ------------------------------------------------------------------------
 * General: This method sends a TEARDOWN request on the session.
 *			This signals the server to permanently stop the transmission.
 *			And free any allocated resources it uses for the session.
 *			The session is ended.
 *
 *          Upon successful (or not) of state change the session layer will 
 *          call RvRtspSessionOnStateChangeEv callback.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	: hSession		- the session object.
 * OUTPUT	: None
 * INOUT	: None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspSessionTeardown(
						IN RvRtspSessionHandle	hSession);

//added by lichao, 20141217 外部指定cseq
RVAPI
RvStatus RVCALLCONV RvRtspSessionTeardownEx(
                        IN RvRtspSessionHandle	hSession,
                        IN RvUint16             cseq);

/******************************************************************************
 * RvRtspSessionSendRequest
 * ----------------------------------------------------------------------------
 * General: This API can be called by the application to send any request message
 *          on a session. The application is expected to construct the request
 *          object using the RvRtspMessageConstructRequest API and to set all 
 *          the required fields of the headers, using RvRtspStringHandleSetString
 *          to set required, RvRtspStringHandle type, fields.
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
        IN  RvBool                              waitingForResponse);


RVAPI 
RvStatus RVCALLCONV RvRtspSessionGetConnectionHandle(
    IN	RvRtspSessionHandle				    hSession,
    OUT RvRtspConnectionHandle              *phConnection);

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SESSION_H*/


