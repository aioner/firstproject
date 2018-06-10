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
 *                         <RvRtspServerConnection.h>
 *
 * This file contains definitions relevant to the RTSP server connections.
 * An RTSP Connection instance is a thread safe representation of a
 * connection to an RTSP server, handling all RTSP communication to and
 * from the server.
 *
 ******************************************************************************/

#ifndef _RV_RTSP_SERVER_CONNECTION_H
#define _RV_RTSP_SERVER_CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                           INCLUDE HEADER FILES                             */
/*----------------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "rvsocket.h"

#include "RvRtspServerTypes.h"
#include "RvRtspMessageTypes.h"
#include "RvRtspServerSession.h"

/*@****************************************************************************
 * Module: RtspServerConnection (root)
 * ----------------------------------------------------------------------------
 * Title:  5.RTSP Server Connection API Functions
 *
 * The RTSP Server connection functions are definitions relevant to the
 * RTSP Server connections.
 * An RTSP Connection instance is a threadsafe representation of a
 * connection to an RTSP Server, handling all RTSP communication to and
 * from the Server.
 *
 * You can find the RTSP Server connection functions in the
 * RvRtspServerConnection.h file.
 ***************************************************************************@*/


/*----------------------------------------------------------------------------*/
/*                             TYPE DEFINITIONS                               */
/*----------------------------------------------------------------------------*/


/*@****************************************************************************
 * Type: RvRtspServerConnectionConfiguration (RtspServerConnection)
 * ----------------------------------------------------------------------------
 * This structure specifies the configuration of the connection.
***************************************************************************@*/
typedef struct RvRtspServerConnectionConfiguration_
{

    RvUint16    transmitQueueSize;
        /* Maximum number of messages waiting to be sent on the connection. */

    RvUint16    maxHeadersInMessage;
        /* Maximum number of headers in a message. */
}RvRtspServerConnectionConfiguration;

/*@****************************************************************************
 * Type: RvRtspServerListenConfiguration (RtspServerConnection)
 * ---------------------------------------------------------------------
 * The configuration of the RTSP Server's listening port.
 ***************************************************************************@*/
typedef struct RvRtspServerListenConfiguration
{
    RvUint16                               listeningPort;
        /* Port number. */

    RvRtspServerConnectionConfiguration    connConfiguration;
        /* Configuration of the listening port connection. */
} RvRtspServerListenConfiguration;

/*@**************************************************************************
 * RvRtspServerConnectionOnDisconnectEv (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General:
 *  This callback is called by the Connection layer when the
 *  connection with the Server has been disconnected.
 *
 * Arguments:
 * Input:   hConnection     - The connection that was disconnected.
 *          hApp            - The application context handle.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
typedef RvStatus (RVCALLCONV *RvRtspServerConnectionOnDisconnectEv)(
    IN RvRtspServerConnectionHandle       hConnection,
    IN RvRtspServerConnectionAppHandle    hApp);



/*@**************************************************************************
 * RvRtspServerConnectionOnErrorEv (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General:
 *  This callback is called by the Connection layer after an error
 *  has occurred on the connection.
 *
 * Arguments:
 * Input:   hConnection   - The connection on which the error occurred.
 *          hApp          - The application context handle.
 *          hURI          - The URI requested in the message that
 *                          caused the error.
 *          requestMethod - The requested method.
 *          status        - The response status.
 *          phrase        - The response phrase.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
typedef RvStatus (RVCALLCONV *RvRtspServerConnectionOnErrorEv)(
    IN RvRtspServerConnectionHandle    hConnection,
    IN RvRtspServerConnectionAppHandle hApp,
    IN RvRtspStringHandle              hURI,
    IN RvRtspMethod                    requestMethod,
    IN RvRtspStatus                    status,
    IN RvRtspStringHandle              hPhrase);


/*@**************************************************************************
 * RvRtspServerConnectionOnReceiveEv (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General:
 *  This callback is called by the Connection layer when a message
 *  is received on the connection.
 *
 * Arguments:
 * Input:    hConnection             - The connection on which the
 *                                     message was received.
 *           hApp                    - The application context handle.
 *           eMsgType                - Request/Response message type.
 *           pRequest                - Request message.
 *           pResponse               - Response message.
 * Output:   None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
typedef RvStatus (RVCALLCONV *RvRtspServerConnectionOnReceiveEv)(
    IN RvRtspServerConnectionHandle     hConnection,
    IN RvRtspServerConnectionAppHandle  hApp,
    IN RvRtspMsgType                    eMsgType, 
    IN RvRtspRequest                    *pRequest,
    IN RvRtspResponse                   *pResponse);

/*@**************************************************************************
 * RvRtspServerConnectionOnAcceptEv (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General:
 *  This callback is called when the Server receives an
 *  incoming TCP connection request from a Client. This function
 *  decides whether to accept or reject the connection request.
 *
 * Arguments:
 * Input:    hConnection            - The connection on which the
 *                                    message was received.
 *           hApp                   - The application context handle.
 *           socket                 - New socket.
 *           strClientIP            - Client IP.
 *           portNum                - Client port.
 *           success                - Boolean to indicate acceptance/rejection
 *                                    of the connection.
 * Output:   success                - Boolean to indicate acceptance/rejection
 *                                    of the connection.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
typedef RvStatus (RVCALLCONV *RvRtspServerConnectionOnAcceptEv)(
    IN RvRtspServerConnectionHandle     hConnection,
    IN RvRtspServerConnectionAppHandle  hApp,
    IN RvSocket                         socket,
    IN RvChar *                         strClientIP,
    IN RvUint16                         portNum,
    INOUT RvBool *                      success);

/*@****************************************************************************
 * RvRtspServerConnectionOnSendEv
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called when an RTSP message that is not
 *  related to a session is sent on a server's connection.
 *  This callback can be used to edit the outgoing message and add
 *  additional headers or header fields that are not specifically 
 *  supported by the server.
 *
 * Arguments:
 * Input:       hConnection      - The connection handle in the server stack.
 *              hApp             - The handle of the application connection object .
 *              pRequest         - The received message if it is a request,
 *                                 NULL if it is a response.
 *              pResponse        - The received message if it is a response,
 *                                 NULL if it is a request.
 *
 *  Output:     None
 *
 * Return Value:RV_OK  - if successful.
 *              Other on failure
 *****************************************************************************/
typedef RvStatus (RVCALLCONV * 	RvRtspServerConnectionOnSendEv)(
    IN	RvRtspServerConnectionHandle		hConnection,
    IN	RvRtspServerConnectionAppHandle     hApp, 
    IN	RvRtspRequest                       *pRequest,
    IN	RvRtspResponse                      *pResponse);


/*@**************************************************************************
 * RvRtspConnectionOnRawBufferReceiveEv (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General: 
 *  This is the definition for the callback function to be called
 *	when a chunk of data is received on the connection.
 *  The application can use the callback to extract interleaved
 *  (encapsulated) data from the buffer.
 *
 * 
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:	    hConnection			- The connection handle in the Stack.
 *              hApp                - The handle of the application connection object.
 *				pBuff		        - The received buffer.
 *              buffSize            - The received buffer size.
 * Output:	    None
 *
 * Return Value:RV_OK if the received buffer should be handled in the
 *              usual manner.
 *              Negative values if the buffer should be ignored
 *              by the Transport layer.
 ***************************************************************************@*/
typedef RvStatus (RVCALLCONV * RvRtspConnectionOnRawBufferReceiveEv)(
    IN	RvRtspServerConnectionHandle        hConnection,
    IN	RvRtspServerConnectionAppHandle	    hApp, 
    IN  RvUint8                             *pBuff,
    IN  RvUint32                            buffSize);

/*@****************************************************************************
 * Type: RvRtspServerConnectionCallbackFunctions (RtspServerConnection)
 * ----------------------------------------------------------------------------
 * This structure specifies the connection's callback functions.
 ***************************************************************************@*/
typedef struct RvRtspServerConnectionCallbackFunctions_
{
    RvRtspServerConnectionOnAcceptEv          pfnServerOnAcceptEv;
        /* Called when a connection is constructed. */

    RvRtspServerConnectionOnDisconnectEv      pfnServerOnDisconnectEv;
        /* Called when the connection is disconnected. */

    RvRtspServerConnectionOnErrorEv           pfnServerOnErrorEv;
        /* Called when an error occurs on the transport. */

    RvRtspServerConnectionOnReceiveEv         pfnServerOnReceiveEv;
        /* Called when a message is received on the transport. */
        
    RvRtspConnectionOnRawBufferReceiveEv      pfnOnRawBufferReceiveEv; 
        /* Called when a raw buffer is received on the connection */
    
    RvRtspServerConnectionOnSendEv            pfnServerOnSendEv;
        /* Called when a message is about to be sent on the connection */

} RvRtspServerConnectionCallbackFunctions;



/*----------------------------------------------------------------------------*/
/*                              FUNCTIONS HEADERS                             */
/*----------------------------------------------------------------------------*/

/*@**************************************************************************
 * RvRtspServerConnectionRegisterCallbacks (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General: 
 *  This API is called to register all callbacks of the connection object.
 *  The callback functions are triggered when an event occurs only if
 *  they have been registered. For connection callbacks to be triggered when 
 *  an event occurs, all connection callbacks should be registered.
 *
 * Arguments:
 * Input:   hConnection    - The connection handle on which the callbacks
 *                           are registered.
 *          pCallbacks     - The callback functions to be called when
 *                           events occur on the connection.
 *          callbacksStructSize - The size of callbacks structure,
 *                                passed to distinguish between
 *                                different versions of the RTSP Server Toolkit.
 * Output:  None.
 *
 * Return Value:   RV_OK if successful.
 *                 Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionRegisterCallbacks(
    IN  RvRtspServerConnectionHandle                  hConnection,
    IN  const RvRtspServerConnectionCallbackFunctions *pCallbacks,
    IN  const RvSize_t                                callbacksStructSize);


/*@**************************************************************************
 * RvRtspServerConnectionSendResponse (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends an RvRtspResponse message object on the connection. The
 *  response object is populated according to the request received from the 
 *  Client.
 *
 * Arguments:
 * Input:   hConnection     - The connection handle used to access the 
 *                            connection on which to send the response.
 *          pResponse       - The response to be sent on the connection.
 * Output:  None.

 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionSendResponse(
    IN  RvRtspServerConnectionHandle      hConnection,
    IN  RvRtspResponse              	  *pResponse);



/*@**************************************************************************
 * RvRtspServerConnectionSendRequest (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends a request message on the connection.
 *
 * Arguments:
 * Input:   hConnection - The connection handle used to access the 
 *                        connection on which to send the request.
 *          pRequest    - The request message to be sent on the connection.
 * Output:  None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionSendRequest(
    IN  RvRtspServerConnectionHandle      hConnection,
    IN  RvRtspRequest              		  *pRequest);

/*@**************************************************************************
 * RvRtspServerConnectionSendRawBuffer (RtspServerConnection)
 * ----------------------------------------------------------------------------
 * General: This API sends an application raw buffer as is on the given 
 *          RTSP connection. 
 *          This API can be used to send embedded binary data (such as RTP) 
 *          on the same TCP connection used by the RTSP.
 *          The encapsulated stream (such as an RTP packet), including the 
 *          ASCII dollar sign (24 hexadecimal), followed by a one-byte channel
 *          identifier, followed by the length of the encapsulated binary data
 *          as a binary, two-byte integer in network byte order, must be fully
 *          provided by the application.
 *          
 * Arguments:
 * Input:   hConnection   - The connection handle in the Stack.
 *          pBuff         - The encapsulated buffer to be sent on the 
 *                          connection.
 *          buffSize      - The size of the buffer, in bytes.
 *
 * Output: None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI 
RvStatus RVCALLCONV RvRtspServerConnectionSendRawBuffer(
        IN	RvRtspServerConnectionHandle    hConnection,
        IN  RvUint8                         *pBuff,
        IN  RvUint32                        buffSize);

/*@**************************************************************************
 * RvRtspServerConnectionReceiveRawBuffer (RtspServerConnection)
 * ----------------------------------------------------------------------------
 * General: This API simulates receiving a raw buffer chunk on a connection.
 *          The API can be used to push back into the Stack part of the  
 *          received chunk that contains an encapsulated stream at its  
 *          beginning, followed by an RTSP message (or part of one).
 *          
 * Arguments:
 * Input:   hConnection   - The connection handle in the Stack.
 *          pBuff         - The raw buffer to be pushed back into the Stack.
 *          buffSize      - The size of the raw buffer, in bytes.
 *
 * Output: None.
 *
 * Return Value:  RV_OK if successful.
 *                Other on failure.
 ***************************************************************************@*/
RVAPI 
RvStatus RVCALLCONV RvRtspServerConnectionReceiveRawBuffer(
        IN	RvRtspServerConnectionHandle    hConnection,
        IN  RvUint8                         *pBuff,
        IN  RvUint32                        buffSize);

/*@**************************************************************************
 * RvRtspServerConnectionGetSessionById (RtspServerConnection)
 * ------------------------------------------------------------------------
 * General: 
 *  This method returns the session corresponding to the session ID
 *  specified by the parameter strSessionId. This is used to determine 
 *  whether or not a session with the provided session ID already exists.
 *
 * Arguments:
 * Input:   hConnection     - Handle to the connection required to access
 *                            the connection on which to check for the 
 *                            session's existence. 
 *          strSessionId    - String containing the session's session ID.
 * Output:  phSession       - The session with the specified session ID
 *                            if found, or NULL if not found.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RvRtspServerConnectionGetSessionById(
    IN RvRtspServerConnectionHandle   hConnection,
    IN  const RvChar                  *strSessionId,
    OUT RvRtspServerSessionHandle     *phSession);

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#ifdef __cplusplus
}
#endif

#endif  /* end of define _RV_RTSP_SERVER_CONNECTION_H */
