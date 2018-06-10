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
 ********************************************************************************
 */


/*******************************************************************************
 *                           <RvRtspServerSession.h>
 *
 * This file contains definitions relevant to the RTSP Server sessions.
 * An RTSP Session instance is a thread safe representation of a session 
 * conducted on top of an RTSP connection.
 *
 ******************************************************************************/


#ifndef _RV_RTSP_SERVER_SESSION_H
#define _RV_RTSP_SERVER_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                           INCLUDE HEADER FILES                             */
/*----------------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "RvRtspServerTypes.h"
#include "RvRtspMessageTypes.h"
#include "RvRtspServerConnection.h"

/*@****************************************************************************
 * Module: RtspServerSession (root)
 * ----------------------------------------------------------------------------
 * Title:  6.RTSP Server Session API Functions
 *
 * The RTSP Server session functions are definitions relevant to the
 * RTSP Server sessions.
 * An RTSP Session instance is a threadsafe representation of a session
 * conducted on top of an RTSP connection.
 *
 * You can find the RTSP Server session functions in the
 * RvRtspServerSession.h file.
 ***************************************************************************@*/

/*----------------------------------------------------------------------------*/
/*                             TYPE DEFINITIONS                               */
/*----------------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspServerSessionState (RtspServerSession)
 * ----------------------------------------------------------------------------
 * This enumeration is used to represent the state of the session. The state  
 * exists from the time a request is received on the session until a response 
 * is sent that changes the state.
 ***************************************************************************@*/
typedef enum
{
    RV_RTSP_SESSION_STATE_INIT = 0,
        /* State after initialization and after TEARDOWN was acknowledged. */

    RV_RTSP_SESSION_STATE_READY,
        /* SETUP or PAUSE was acknowledged. */

    RV_RTSP_SESSION_STATE_PLAYING
        /* PLAY was acknowledged. */

} RvRtspServerSessionState;

/*@****************************************************************************
 * Type: RvRtspServerSessionConfiguration (RtspServerSession)
 * ----------------------------------------------------------------------------
 * This structure specifies the configuration of the Server session object.
 ***************************************************************************@*/
typedef struct RvRtspServerSessionConfiguration_
{
    RvRtspStringHandle      strURI; 
    RvUint32	checkTimerInterval;   /* in milliseconds units*/
    RvUint32	timeOutInterval;     /* in milliseconds units*/
        /* The message URI. */

} RvRtspServerSessionConfiguration;

/*@**************************************************************************
 * RvRtspServerSessionOnDestructEv (RtspServerSession)
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called by the session layer when a session is being 
 *  destructed. In this callback, the application should release all its 
 *  resources for the session.
 *
 * Arguments:
 * Input:   hSession    - The handle to the session being destructed.  
 *                        Gives access to the session to be destructed.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
typedef RvStatus (RVCALLCONV * RvRtspServerSessionOnDestructEv)(
    IN      RvRtspServerSessionHandle             hSession,
    IN      RvRtspServerSessionAppHandle          hApp);

/*@**************************************************************************
 * RvRtspServerSessionOnStateChangeEv (RtspServerSession)
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Session layer when an RTSP
 *  Request received on the connection for the session
 *  is processed successfully and the state of the session has been 
 *  changed.
 *
 * Arguments:
 * Input:   hSession        - The session for which the state was changed.
 *          hApp            - The application context.
 *          currState       - The current session state.
 *          newState        - The new session state.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ****************************************************************************@*/
typedef RvStatus (RVCALLCONV * RvRtspServerSessionOnStateChangeEv)(
    IN      RvRtspServerSessionHandle             hSession,
    IN      RvRtspServerSessionAppHandle          hApp,
    IN      RvRtspServerSessionState              currState,
    IN      RvRtspServerSessionState              newState);

/*@**************************************************************************
 * RvRtspServerSessionOnErrorEv (RtspServerSession)
 * ------------------------------------------------------------------------
 * General:
 *  This callback is called by the session layer when an error is
 *  encountered on the session.
 *
 * Arguments:
 * Input:   hSession          - The session on which the error occurred.
 *          hApp              - The application context.
 *          requestMethod     - The requested method.
 *          status            - The returned status.
 *          hPhrase           - The status phrase.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ****************************************************************************@*/
typedef RvStatus (RVCALLCONV * RvRtspServerSessionOnErrorEv)(
    IN      RvRtspServerSessionHandle             hSession,
    IN      RvRtspServerSessionAppHandle          hApp,
    IN      RvRtspMethod                          requestMethod,
    IN      RvRtspStatus                          status,
    IN      RvRtspStringHandle                    hPhrase);

/*@**************************************************************************
 * RvRtspServerSessionOnReceiveEv (RtspServerSession)
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the session layer when an RTSP
 *  message was received on the connection for the session. 
 *  In this callback, the application has access to the headers and fields of
 *  the message.
 *
 * Arguments:
 * Input:   hSession        - The session on which the message was received.
 *          hApp            - The application context.
 *          pRequest        - The request message.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ****************************************************************************@*/
typedef RvStatus (RVCALLCONV * RvRtspServerSessionOnReceiveEv)( 
    IN     RvRtspServerSessionHandle             hSession,
    IN     RvRtspServerSessionAppHandle          hApp,
    IN     RvRtspRequest                         *pRequest);

/*@****************************************************************************
 * RvRtspServerSessionOnSendEv
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called when an RTSP messageis sent on a server's
 *  session. This callback can be used to edit the outgoing message and add
 *  additional headers or header fields that are not specifically 
 *  supported by the server.
 *
 * Arguments:
 * Input:       hSession         - The session handle in the server stack.
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
typedef	RvStatus (RVCALLCONV *RvRtspServerSessionOnSendEv)( 
                        IN	RvRtspServerSessionHandle		hSession,
                        IN 	RvRtspServerSessionAppHandle	hApp, 
                        IN	RvRtspRequest		            *pRequest,
                        IN	RvRtspResponse                  *pResponse);

/*@****************************************************************************
 * Type: RvRtspSessionCallbackFunctions (RtspServerSession)
 * ----------------------------------------------------------------------------
 * This structure specifies the callback functions of the session.
 ****************************************************************************@*/
typedef struct RvRtspServerSessionCallbackFunctions_
{
    RvRtspServerSessionOnStateChangeEv        pfnServerOnStateChangeEv;
        /* Called on successful state change. */

    RvRtspServerSessionOnErrorEv              pfnServerOnErrorEv;
        /* Called on error. */

    RvRtspServerSessionOnDestructEv           pfnServerOnDestructEv;
        /* Called on destruct. */

    RvRtspServerSessionOnReceiveEv            pfnServerOnReceiveEv;
        /* Called on message received on the session. */

    RvRtspServerSessionOnSendEv               pfnServerOnSendEv;
        /* called when a message is about to be sent on the Session */

} RvRtspServerSessionCallbackFunctions;



/*----------------------------------------------------------------------------*/
/*                              FUNCTIONS HEADERS                             */
/*----------------------------------------------------------------------------*/


/*@**************************************************************************
 * RvRtspServerSessionConstruct (RtspServerSession)
 * ------------------------------------------------------------------------
 * General: 
 *  This API initializes the RTSP session's structure. It constructs a new
 *  session and adds it to the connection RA object. It generates a session 
 *  ID if the application has not provided one. It then increments the
 *  session count in the connection. The new session starts in an INIT state.
 *
 * Arguments:
 * Input:   hConnection         -  The connection on which the new session
 *                                 will be constructed.
 *          hApp                -  Application handle used to access the 
 *                                 connection and the session.
 *          pConfiguration      -  Configuration for the session.
 *          configurationSize   -  Size of callbacks structure,
 *                                 passed to distinguish between
 *                                 different versions of the RTSP Stack.
 *          pCallbacks          -  Callback functions to call on events
 *                                 populated by the application.
 *          callbacksStructSize -  Size of callbacks structure,
 *                                 passed to distinguish between
 *                                 different versions of the RTSP Stack.
 *          phSessionId         -  The Session Id is the unique identifier for 
 *                                 a session.
 * Output:  phSession           -  The handle to the constructed session used
 *                                 by the application to access the session.
 *          phSessionId         -  If this points to a NULL handle, then the
 *                                 application has not provided the sessionId
 *                                 and the Stack needs to generate the 
 *                                 sessionId, populate it in the session, and 
 *                                 return the updated handle to the 
 *                                 application.
 *                                 If it points to a non-null value, then 
 *                                 the session should be updated with the  
 *                                 provided Session Id handle.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerSessionConstruct(
    IN  RvRtspServerConnectionHandle           hConnection,
    IN  RvRtspServerSessionAppHandle           hApp,
    IN  const RvRtspServerSessionConfiguration *pConfiguration,
    IN  const RvSize_t                         configurationSize,
    IN  const RvRtspServerSessionCallbackFunctions  *pCallbacks,
    IN  const RvSize_t                         callbacksStructSize,
    OUT RvRtspServerSessionHandle              *phSession,
    INOUT RvRtspStringHandle                   *phSessionId);


/*@**************************************************************************
 * RvRtspServerSessionDestruct (RtspServerSession)
 * ------------------------------------------------------------------------
 * General: 
 *  This API is called to destruct an RTSP session. It frees up the resources 
 *  used by the session, release the RPool elements in the session and deletes
 *  the session from the list of sessions in the connection. It is called when
 *  TEARDOWN is requested or a connection is disconnected.
 *
 * Arguments:
 * Input:   hSession    - The handle to the session being destructed. It gives 
 *                        access to the session to be destructed.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerSessionDestruct(
    IN  RvRtspServerSessionHandle hSession);


/*@**************************************************************************
 * RvRtspServerSessionSendResponse (RtspServerSession)
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends the response on a session to the request received on 
 *  that session. It then changes the session's state as required and 
 *  generates the RvRtspServerOnStateChangeEv callback.
 *  
 *
 * Arguments:
 * Input:   hSession    - The session handle used to access the session.
 *          pResponse   - The response to be sent. It is populated based  
 *                        on the received request.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerSessionSendResponse(
    IN     RvRtspServerSessionHandle     hSession,
    IN     RvRtspResponse                *pResponse);


#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SERVER_SESSION_H*/


