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
 *                              <RvRtspClientConnection.h>
 *
 *	This file contains definitions relevant to the RTSP server connections.
 *	An RTSP Connection instance is a thread safe representation of a connection
 *	to an RTSP server, handling all RTSP communication to and from the server.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_CONNECTION_H
#define _RV_RTSP_CONNECTION_H

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
#include "RvRtspClientSession.h"

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* RvRtspConnectionConfiguration
 * ----------
 * This structure specifies the configuration for the connection object.
 */
typedef struct RvRtspConnectionConfiguration_
{
	RvUint16	maxSessions;				/* Max number of sessions per connection*/
	RvUint16	maxWaitingDescribeRequests;	/* Max number of Describe requests in	*/
											/* the hWaitingDescribeRequests Array	*/
	RvUint16	transmitQueueSize;			/* Max number of messages waiting to be */
											/* sent on the connection				*/
	RvUint16	maxHeadersInMessage;		/* Max number of headers in a message	*/
	RvUint16	maxUrlsInMessage;			/* Max number of URLs in a connection	*/
	RvUint16	dnsMaxResults;				/* Max number of IP results from DNS	*/
    RvUint32	describeResponseTimeOut;    /* in milliseconds units*/
} RvRtspConnectionConfiguration;




/**************************************************************************
 * RvRtspConnectionOnConnectEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Connection layer when the 
 *          connection with the server has been established.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection	- the Connection which has been connected.
 *				hApp		- application context.
 *				success		- has the connect succeeded or failed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspConnectionOnConnectEv)(
								IN RvRtspConnectionHandle		hConnection,
								IN RvRtspConnectionAppHandle	hApp,
								IN RvBool						success);


/**************************************************************************
 * RvRtspConnectionOnDisconnectEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Connection layer when the 
 *          connection with the server has been disconnected.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection	- The Connection which was disconnected.
 *				hApp		- application context.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspConnectionOnDisconnectEv)(
								IN RvRtspConnectionHandle		hConnection,
								IN RvRtspConnectionAppHandle	hApp);



/**************************************************************************
 * RvRtspConnectionOnErrorEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Connection layer after an error
 *			has occurred on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- The Connection on which the error occurred.
 *				hApp			- application context.
 *				hURI			- The URI requested in the message that
 *								  caused the error.
 *				requestMethod	- The requested method.
 *				status			- The response status.
 *				phrase			- The response phrase.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspConnectionOnErrorEv)(
								IN	RvRtspConnectionHandle		hConnection,
								IN	RvRtspConnectionAppHandle	hApp,
								IN	RvRtspStringHandle			hURI,
								IN	RvRtspMethod				requestMethod,
								IN	RvRtspStatus				status,
								IN	RvRtspStringHandle			hPhrase);



/**************************************************************************
 * RvRtspConnectionOnDescribeResponseEv
 * ------------------------------------------------------------------------
 * General: This method sends a describe request on the connection.
 *
 *          Upon successful request and response from the server the 
 *          connection layer will call RvRtspConnectionOnDescribeResponseEv 
 *          callback.
 *
 * Return Value:	RV_OK if the function was successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- The connection on which to send the request.
 *				strURI			- The URI to be described.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspConnectionOnDescribeResponseEv)(
								IN	RvRtspConnectionHandle				hConnection,
								IN	RvRtspConnectionAppHandle			hApp,
								IN	RvRtspConnectionDescribeAppHandle	hDescribe,
								IN	const RvRtspResponse				*pDescribeResponse,
								IN	RvRtspStringHandle					hURI);


/**************************************************************************
 * RvRtspConnectionOnRedirectRequestEv
 * ------------------------------------------------------------------------
 * General: This callback is called by the Connection layer after a redirect
 *			request is received on the connection.
 *
 * Return Value:	RV_OK if successfull, negative values otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- The Connection on which the redirect
 *								  request arrived.
 *				hApp			- application context.
 *				pRequest		- The received request message.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspConnectionOnRedirectRequestEv)(
								IN	RvRtspConnectionHandle		hConnection,
								IN  RvRtspConnectionAppHandle	hApp,
								IN	const RvRtspRequest			*pRequest);



/**************************************************************************
 * RvRtspConnectionOnErrorExtEv
 * ------------------------------------------------------------------------
 * General: 
 *  This callback is called by the Connection layer after an error
 *	has occurred on the connection.
 *
 * Arguments:
 * Input:	    hConnection		- The Connection on which the error occurred.
 *				hApp			- application context.
 *              hDescribe       - The application handle for the failed describe request
 *                                or NULL if the error is not on describe request.
 *				hURI			- The URI requested in the message that
 *								  caused the error.
 *				requestMethod	- The requested method.
 *				status			- The response status.
 *				phrase			- The response phrase.
 *				pResponse	    - the received response.
 * Output:	    None.
 *
 * Return Value:RV_OK if successful
 *              Negative values otherwise.
 *************************************************************************/
typedef RvStatus (RVCALLCONV *RvRtspConnectionOnErrorExtEv)(
								IN	RvRtspConnectionHandle		        hConnection,
								IN	RvRtspConnectionAppHandle	        hApp,
                                IN  RvRtspConnectionDescribeAppHandle   hDescribe,
								IN	RvRtspStringHandle			        hURI,
								IN	RvRtspMethod				        requestMethod,
								IN	RvRtspStatus				        status,
								IN	RvRtspStringHandle			        hPhrase,
								IN	const RvRtspResponse		        *pResponse);

/******************************************************************************
 * RvRtspConnectionOnReceiveEv
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called when an RTSP message that is not
 *  related to a session is received on a client's connection.
 *  This callback can be used to retrieve additional headers or header fields
 *  that are not specifically supported by the client.
 *
 * Arguments:
 * Input:        hConnection      - The connection handle in the client stack.
 *               hApp             - The handle of the application connection object .
 *               pRequest         - The received message if it is a request,
 *                                  NULL if it is a response.
 *               pResponse        - The received message if it is a response,
 *                                  NULL if it is a request.
 *
 * Return Value: RV_OK  - if successful.
 *               Other on failure
 *****************************************************************************/
typedef RvStatus (RVCALLCONV * RvRtspConnectionOnReceiveEv)(
                                IN	RvRtspConnectionHandle		hConnection,
                                IN	RvRtspConnectionAppHandle	hApp,
                                IN	RvRtspRequest			    *pRequest,
                                IN	RvRtspResponse			    *pResponse);
typedef RvStatus (RVCALLCONV * RvRtspConnectionOnUnauthorized)(
                                IN	RvRtspConnectionHandle		hConnection,
                                IN	RvRtspConnectionAppHandle	hApp,
                                IN	RvRtspRequest			    *pRequest,
                                IN	RvRtspResponse			    *pResponse);                                    

/******************************************************************************
 * RvRtspConnectionOnSendEv
 * ----------------------------------------------------------------------------
 * General: 
 *  This callback is called when an RTSP message that is not
 *  related to a session is sent on a client's connection.
 *  This callback can be used to edit the outgoing message and add
 *  additional headers or header fields that are not specifically 
 *  supported by the client.
 *
 * Arguments:
 * Input:       hConnection      - The connection handle in the client stack.
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
typedef RvStatus (RVCALLCONV * 	RvRtspConnectionOnSendEv)(
                                IN	RvRtspConnectionHandle		hConnection,
                                IN	RvRtspConnectionAppHandle	hApp, 
                                IN	RvRtspRequest			    *pRequest,
                                IN	RvRtspResponse			    *pResponse);

/**************************************************************************
 * RvRtspConnectionOnRawBufferReceiveEv
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
 * Input:	    hConnection			- The connection handle in the client stack.
 *              hApp                - The handle of the application connection object.
 *				pBuff		        - The received buffer.
 *              buffSize            - The received buffer size.
 * Output:	    None
 *
 * Return Value:RV_OK if the received buffer should be handled in the
 *              usual manner.
 *              Negative values if buffer should be ignored
 *              by the transport layer.
 *************************************************************************/
typedef RvStatus (RVCALLCONV * RvRtspConnectionOnRawBufferReceiveEv)(
                                IN	RvRtspConnectionHandle      hConnection,
                                IN	RvRtspConnectionAppHandle	hApp, 
                                IN  RvUint8                     *pBuff,
                                IN  RvUint32                    buffSize);



/* RvRtspConnectionCallbackFunctions
 * ----------
 * This structure specifies the connection's callback functions.
 */
typedef struct RvRtspConnectionCallbackFunctions_
{
	RvRtspConnectionOnConnectEv				pfnOnConnectEv;			/* Called when a connection		*/
																/* is established.				*/
	RvRtspConnectionOnDisconnectEv			pfnOnDisconnectEv;		/* called when the connection	*/
																/* is disconnected				*/
	RvRtspConnectionOnErrorEv				pfnOnErrorEv;			/* called when an error occurs	*/
																/* on the transport 			*/
	RvRtspConnectionOnDescribeResponseEv	pfnOnDescribeResponseEv;/* called when a Describe		*/
																/* response message is received	*/
																/* on the transport				*/
	RvRtspConnectionOnRedirectRequestEv		pfnOnRedirectRequestEv;	/* called when a Redirect		*/
																/* request message is received	*/
																/* on the transport				*/
	RvRtspConnectionOnErrorExtEv			pfnOnErrorExtEv;		/* called when an error occurs	*/
																/* on the transport, provides   */
	                                                            /* the response message.		*/
	RvRtspConnectionOnReceiveEv			    pfnOnReceiveEv;         /* Called when a message was received on the connection */
	RvRtspConnectionOnSendEv			    pfnOnSendEv;            /* Called when a message is about to be sent on the connection */
	RvRtspConnectionOnRawBufferReceiveEv    pfnOnRawBufferReceiveEv; /* Called when a raw buffer is received on the connection */

} RvRtspConnectionCallbackFunctions;



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RvRtspConnectionRegisterCallbacks
 * ------------------------------------------------------------------------
 * General: This method registers the connection object's callbacks.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		    - The connection.
 *				pCallbacks	    	- callback functions to be called when
 *								      events occur on the connection.
 *              callbacksStructSize - size of callbacks structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionRegisterCallbacks(
						IN	RvRtspConnectionHandle					hConnection,
						IN	const RvRtspConnectionCallbackFunctions	*pCallbacks,
                        IN	const RvSize_t                          callbacksStructSize);


/**************************************************************************
 * RvRtspConnectionConnect
 * ------------------------------------------------------------------------
 * General: This method connects the connection to the server over the
 *			transport layer.
 *			Note: the method sends a DNS resolution request, and when the
 *			IP address is received the connection connects to the server.
 *
 *          Upon successful (or not) connecting the connection layer will 
 *          call RvRtspConnectionOnConnectEv callback.
 *
 * Return Value:	RV_OK - If the connection is successfully connected
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection			- The connection handle.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionConnect(
						IN	RvRtspConnectionHandle	hConnection);



/**************************************************************************
 * RvRtspConnectionDisconnect
 * ------------------------------------------------------------------------
 * General: This method disconnects the connection from the server.
 *			destroying any sessions present on the connection.
 *         
 *          Upon successful (or not) disconnecting the connection layer will 
 *          call RvRtspConnectionOnDisconnectEv callback.
 *
 *
 * Return Value:	RV_OK if completed successfully, negative value is
 *					returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- the destructed connection.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionDisconnect(
						IN	RvRtspConnectionHandle	hConnection);



/**************************************************************************
 * RvRtspConnectionTeardownAllSessions
 * ------------------------------------------------------------------------
 * General: This method disconnects all the sessions on the connection.
 *
 * Return Value:	RV_OK - If the function completes successfully,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- the connection who sessions we teardown.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionTeardownAllSessions(
						IN	RvRtspConnectionHandle	hConnection);


/**************************************************************************
 * RvRtspConnectionGetIPAddress
 * ------------------------------------------------------------------------
 * General: This method returns the IP address of the connected server
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- handle to the Connection the session on.
 *				bufferSize      - size of input buffer.
 * OUTPUT	:	buffer          - output buffer where the IP address will be copied to.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetIPAddress(
						IN	RvRtspConnectionHandle	hConnection,
                        IN  RvUint16                bufferSize,
						OUT	RvChar			        *buffer);

//added by lichao, 20141222
RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetPort(
    IN	RvRtspConnectionHandle	hConnection,
    OUT	RvUint16			    *nPort);


/**************************************************************************
 * RvRtspConnectionGetSessionById
 * ------------------------------------------------------------------------
 * General: This method returns the session corresponding to the session-id
 *			specified by the parameter pSessionId.
 *			Note: this method goes over the sessions array sequentially
 *			in order to find the session, it is therefore not time
 *			efficient (O(n)) and so it is not recommended as the normal
 *			way of accessing the sessions.
 *
 * Return Value:	RV_OK - When the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- handle to the Connection the session on.
 *				strSessionId	- string containing the session's Session-Id.
 * OUTPUT	:	phSession		- the session with the specified Session-Id
 *								if found, or NULL if not found.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetSessionById(
						IN	RvRtspConnectionHandle	hConnection,
						IN	const RvChar			*strSessionId,
						OUT	RvRtspSessionHandle		*phSession);



/**************************************************************************
 * RvRtspConnectionRequestDescribe
 * ------------------------------------------------------------------------
 * General: This method sends a describe request on the connection.
 *
 * Return Value:	RV_OK if the function was successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection		- The connection on which to send the request.
 *				strURI			- The URI to be described.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionRequestDescribe(
						IN	RvRtspConnectionHandle				hConnection,
						IN	const RvChar						*strURI,
						IN	RvRtspConnectionDescribeAppHandle	hDescribe);

//added by lichao, 20141218
RVAPI
RvStatus RVCALLCONV RvRtspConnectionRequestDescribeEx(
    IN	RvRtspConnectionHandle				hConnection,
    IN	const RvChar						*strURI,
    IN RvUint16                             cseq,
    IN	RvRtspConnectionDescribeAppHandle	hDescribe);

/******************************************************************************
 * RvRtspClientConnectionSendRequest
 * ----------------------------------------------------------------------------
 * General: This API can be called by the application to send any request message
 *          on a connection. The application is expected to construct the request
 *          object using the RvRtspMessageConstructRequest API and to set all 
 *          the required fields of the headers, using RvRtspStringHandleSetString
 *          to set required, RvRtspStringHandle type, fields.
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
        IN  RvRtspRequest                       *pRequest);

/******************************************************************************
 * RvRtspConnectionSendResponse
 * ----------------------------------------------------------------------------
 * General: This API can be called by the application to send any response message
 *          on a connection. The application is expected to construct the request
 *          object using the RvRtspMessageConstructResponse API and to set all 
 *          the required fields of the headers, using RvRtspStringHandleSetString
 *          to set required, RvRtspStringHandle type, fields.
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
        IN  RvRtspResponse                      *pResponse);

/******************************************************************************
 * RvRtspConnectionSendRawBuffer
 * ----------------------------------------------------------------------------
 * General: This API sends an application raw buffer as is, on the given 
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
        IN  RvUint32                    buffSize);

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
        IN  RvUint32                    buffSize);

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

//added by lichao, 20141217 get the next cseq
RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetNextCSeq(
    IN	RvRtspConnectionHandle	hConnection,
    OUT	RvUint16           *pNextCSeq);

RVAPI
RvStatus RVCALLCONV RvRtspConnectionGetRtspHandle(
    IN	RvRtspConnectionHandle	hConnection,
    OUT	RvRtspHandle           *hRtsp);

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_CONNECTION_H*/


