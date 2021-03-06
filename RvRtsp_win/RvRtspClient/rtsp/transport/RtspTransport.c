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
 *                              <RtspTransport.c>
 *
 * The Transport layer connects to the RTSP server and sets up the callbacks to be
 * called when data is received on the transport connection.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						8/1/04
 *
 *********************************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvlog.h"
#include "rvtime.h"
#include "rvstdio.h"
#include "rvmemory.h"
#include "rvares.h"
#include "rpool.h"

#include "RtspTransport.h"
#include "RtspUtilsInternal.h"



/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/


/* Transport Functions */


/**************************************************************************
 * RtspTransportOnConnect
 * ------------------------------------------------------------------------
 * General: This method will be called when the TCP connection request made
 *			on this transport either succeeds or fails.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- transport object to which the connection
 							belongs.
 *				success		- has the connection been established or failed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportOnConnect(
						IN	RtspTransport	*pThis,
						IN 	RvBool			success);



/**************************************************************************
 * RtspTransportOnDisconnect
 * ------------------------------------------------------------------------
 * General: This method will be called when the disconnection request either
 *			succeeds or fails.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis	- transport object to which the connection belongs.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportOnDisconnect(
						IN	RtspTransport	*pThis);



/**************************************************************************
 * RtspTransportSendPrepareNextMessage
 * ------------------------------------------------------------------------
 * General: This method prepares for sending the next message. It
 *			gets a message element from the txQueue, and sets up the select
 *			engine to send the message using the RtspTransportDoSend function.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- the transport object on which the buffer
 *								will be sent.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportSendPrepareNextMessage(
						IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportDoSend
 * ------------------------------------------------------------------------
 * General: This method sends the next buffer (chunk) on the connection.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- the transport object on which the buffer
 *								will be sent.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportDoSend(
						IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportDoReceive
 * ------------------------------------------------------------------------
 * General: This method handles the reception of data from the connection.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- the transport object on which the data
 *								will be received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportDoReceive(
						IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportOnSelectEv
 * ------------------------------------------------------------------------
 * General: This method handles the interface between the select engine and
 *			the Rtsp callback functions.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pSelectEngine	- the main select engine.
 *				pFd				- the file descriptor of the socket on
 *								which the event occured.
 *				selectEvent		- the type of event which occured.
 *				error			- has an error occured.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportOnSelectEv(
		        IN RvSelectEngine*  pSelectEngine,
		        IN RvSelectFd*      pFd,
		        IN RvSelectEvents   selectEvent,
    		    IN RvBool           error);



/* Utility Functions */

/**************************************************************************
 * RtspTransportClearMessage
 * ------------------------------------------------------------------------
 * General: This function clears a message, releasing it's elements
 *			from the RPool memory.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- The RPool to which the elements in the
 *							message belong.
 *				pMessage	- Pointer to message to clear.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportClearMessage(
				IN	HRPOOL					hRPool,
				IN	RtspTransportMessage	*pMessage);





/* message Parsing */


/**************************************************************************
 * RtspTransportRxSkipEol
 * ------------------------------------------------------------------------
 * General: This function skips the characters until reaching the start
 *			of the next line (rxBufferRead is set to the last character
 *			of the line + 1).
 *
 * Return Value:	RV_TRUE if a line-feed character was found, RV_FALSE
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static RvBool  RtspTransportRxSkipEol(
				IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportRxReadLine
 * ------------------------------------------------------------------------
 * General: This function reads the buffer into the rxLine until reaching
 *			the end of the line.
 *
 * Return Value:	RV_TRUE if finished reading the line, RV_FALSE otherwise
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static RvBool RtspTransportRxReadLine(
				IN	RtspTransport	*pThis);



/**************************************************************************
 * RtspTransportRxFirstLine
 * ------------------------------------------------------------------------
 * General: This function reads the first line, until the end of it, and
 *			then changes state to RX_STATE_TESTING_EOH.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportRxFirstLine(
				IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportRxHeaderLine
 * ------------------------------------------------------------------------
 * General: This function reads a header into rxLine, pushing it into
 *			headersQueue and switching to RX_STATE_TESTING_EOH when done.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void	RtspTransportRxHeaderLine(
				IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportRxTestingEoh
 * ------------------------------------------------------------------------
 * General: This function tests if we reached the end of the headers, if
 *			so, it switches to message body or new message, otherwise, it
 *			changes the state back to RX_STATE_HEADER_LINE.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportRxTestingEoh(
				IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportRxReadBody
 * ------------------------------------------------------------------------
 * General: This function copies the buffer into the message body.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void	RtspTransportRxReadBody(
				IN	RtspTransport	*pThis);


/**************************************************************************
 * RtspTransportRxBody
 * ------------------------------------------------------------------------
 * General: This function tests if we read the entire message, if so, it
 *			moves to the next message, otherwise, it continues to read the
 *			body.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void	RtspTransportRxBody(
				IN	RtspTransport	*pThis);




/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/**************************************************************************
 * RtspTransportConstruct
 * ------------------------------------------------------------------------
 * General: This method initializes the RtspTransport object, setting it
 *			up for connecting.
 *			Note: this function relys on external values set in the
 *			transport structure by the connection.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis					- The transport to construct.
 *				transmitQueueSize		- Maximum number of messages waiting
 *										to be sent on the connection.
 *				maxHeadersInMessage		- Maximum number of headers in a
 *										message.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportConstruct(
			IN	RtspTransport	*pThis,
			IN	RvUint16		transmitQueueSize,
			IN	RvUint16		maxHeadersInMessage)
{
	RvStatus result;

	if (pThis == NULL)
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

	if (pThis->transportState != RTSP_TRANSPORT_STATE_DESTRUCTED)
    	return RvRtspErrorCode(RV_ERROR_BADPARAM);

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConstruct\r\n"));

	/* maximum number of buffers for transmission		*/
	if (transmitQueueSize == 0)
		transmitQueueSize = 1; 	/* minimum number of buffers allowed		*/

	/* callback functions, to be called when events occur on this transport	*/
	/* will be set when calling registerCallbacks.							*/
	memset(&pThis->pfnCallbacks, 0, sizeof(pThis->pfnCallbacks));

	/* Create guard Mutex */
	result = RvMutexConstruct(pThis->pLogMgr, &pThis->mutex);
	if (result != RV_OK)
	{
		RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConstruct - Mutex construction failed\r\n"));
		return result;
	}

	/* initialize the transmission queue									*/
	/* offset specifies the offset from start of buffer to send from - for	*/
	/* when we already sent a part of the buffer.							*/
	/* thread safty is taken care of by the transport mutex & by upper layers */
	pThis->txElementOffset	= 0;
	pThis->txElement		= NULL;
	RvQueueConstruct(transmitQueueSize, sizeof(HRPOOLELEM), NULL, pThis->pLogMgr, &pThis->txQueue);

	/* pTxChunk is used for sending blocks on the transport connection */
	pThis->pTxChunk			= NULL;
	pThis->txChunkLength	= 0;
	pThis->txChunkSent		= 0;

	/* initialize the receive message, an RtspTransportMessage containing	*/
	/* an RTSP message with up to maximumHeadersNumber of headers.			*/
	pThis->rxMessage.hFirstLine = NULL;
	RvQueueConstruct(maxHeadersInMessage, sizeof(HRPOOLELEM), NULL, pThis->pLogMgr, &pThis->rxMessage.headersQueue);
	pThis->rxMessage.hBody 		= NULL;
	pThis->rxLine				= NULL;
	pThis->rxMessageBodyLength	= 0;
	pThis->rxMessageBodyRead	= 0;
	pThis->rxBufferLength		= 0;
	pThis->rxBufferRead			= 0;
	pThis->rxSkipEol			= RV_FALSE;

	/* initialize the transport object's states */
	pThis->transportState	= RTSP_TRANSPORT_STATE_CONSTRUCTED;
	pThis->txState			= RTSP_TRANSPORT_TX_STATE_IDLE;
	pThis->rxState			= RTSP_TRANSPORT_RX_STATE_FIRST_LINE;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConstruct\r\n"));
	return RV_OK;
}




/**************************************************************************
 * RtspTransportDestruct
 * ------------------------------------------------------------------------
 * General: This method destructs the specified RtspTransport object,
 *			after this call the object should not be used without calling
 *			construct again.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis	- the transport to destruct.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportDestruct(
		    IN	RtspTransport	*pThis)
{
	if (pThis == NULL)
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

    /* trying to destruct an already destructed object does nothing */
    if (pThis->transportState == RTSP_TRANSPORT_STATE_DESTRUCTED)
		return RvRtspErrorCode(RV_ERROR_DESTRUCTED);

	RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportDestruct\r\n"));

	/* Clear and destruct the send transmission queue */
	RtspUtilsClearMemoryQueue(pThis->hRPool, &pThis->txQueue);
	RvQueueDestruct(&pThis->txQueue);

	/* Clear and destruct the receive transmission message (first line, header queue and body) */
	RtspTransportClearMessage(pThis->hRPool, &pThis->rxMessage);
	RvQueueDestruct(&pThis->rxMessage.headersQueue);

	if (pThis->txElement != NULL)
		rpoolFree(pThis->hRPool, pThis->txElement);

	if (pThis->rxLine != NULL)
		rpoolFree(pThis->hRPool, pThis->rxLine);

	/* destruct the socket and its relatives... */
	if (pThis->transportState != RTSP_TRANSPORT_STATE_CONSTRUCTED)
	{
		RvSelectUpdate(	pThis->pSelectEngine,
						&pThis->socketFd,
						0,
						RtspTransportOnSelectEv);
		RvSocketShutdown(&pThis->socket, RV_TRUE, pThis->pLogMgr);

		/* remove the transport's socket from the select engine */
		RvSelectRemove(pThis->pSelectEngine,&pThis->socketFd);

		/* destruct the socket's file descriptor */
	    RvFdDestruct(&pThis->socketFd);

		RvSocketDestruct(&pThis->socket, RV_FALSE, NULL, pThis->pLogMgr);
	}

	pThis->pLogMgr				= NULL;
	pThis->pLogSrc				= NULL;
	pThis->context				= NULL;
	pThis->hRPool				= NULL;
	pThis->pSelectEngine		= NULL;
	memset(&pThis->pfnCallbacks, 0, sizeof(RtspTransportCallbackFunctions));
	pThis->txElement			= NULL;
	pThis->txElementOffset		= 0;
	pThis->pTxChunk				= NULL;
	pThis->txChunkLength		= 0;
	pThis->txChunkSent			= 0;
	pThis->transportState		= RTSP_TRANSPORT_STATE_DESTRUCTED;
	pThis->txState				= RTSP_TRANSPORT_TX_STATE_IDLE;
	pThis->rxState				= RTSP_TRANSPORT_RX_STATE_FIRST_LINE;
	pThis->rxLine				= NULL;
	pThis->rxMessageBodyLength	= 0;
	pThis->rxMessageBodyRead	= 0;
	pThis->rxBufferLength		= 0;
	pThis->rxBufferRead			= 0;
	pThis->rxSkipEol			= RV_FALSE;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportDestruct\r\n"));
	RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

	/* Destruct the guard Mutex */
	RvMutexDestruct(&pThis->mutex, pThis->pLogMgr);

	RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

	return RV_OK;
}



/**************************************************************************
 * RtspTransportRegisterCallbacks
 * ------------------------------------------------------------------------
 * General: This method registers the transport object's callbacks.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis				- the transport object.
 *				pCallbacks			- callback functions to be called
 *									when events occur on the transport.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportRegisterCallbacks(
		IN	RtspTransport							*pThis,
		IN	const RtspTransportCallbackFunctions	*pCallbacks)
{
    /* checking parameters and states */
	if (pThis == NULL)
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if (pThis->transportState == RTSP_TRANSPORT_STATE_DESTRUCTED)
		return RvRtspErrorCode(RV_ERROR_DESTRUCTED);

	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportRegisterCallbacks\r\n"));

	memcpy(&pThis->pfnCallbacks, pCallbacks, sizeof(RtspTransportCallbackFunctions));

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportRegisterCallbacks\r\n"));
	RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	return RV_OK;
}


/**************************************************************************
 * RtspTransportConnect
 * ------------------------------------------------------------------------
 * General: This method requests a TCP connection to the remote address
 *			specified, and sets up a select event to be called when this
 *			request succeeds or fails.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis				- pointer to the transport to connect.
 *				remoteAddress		- the RTSP server's address.
 *				sourceAddress		- the RTSP client's address.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportConnect(
			IN	RtspTransport		*pThis,
			IN	const RvAddress		*remoteAddress,
			IN	const RvAddress		*sourceAddress)
{
	RvStatus result;

    /* checking parameters and states */
	if ((pThis == NULL) || (remoteAddress == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if (pThis->transportState == RTSP_TRANSPORT_STATE_DESTRUCTED)
		return RvRtspErrorCode(RV_ERROR_DESTRUCTED);

	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect\r\n"));

    if (pThis->transportState != RTSP_TRANSPORT_STATE_CONSTRUCTED)
	{
		RvLogWarning(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Not in right state for connecting\r\n"));
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
		return RvRtspErrorCode(RV_ERROR_NOTSUPPORTED);
	}

	/* Construct a TCP socket and set it to non blocking */
	result = RvSocketConstruct(remoteAddress->addrtype, RvSocketProtocolTcp, pThis->pLogMgr, &pThis->socket);
	if (result != RV_OK)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Can't construct socket\r\n"));
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
		return result;
	}

	/* socket is non blocking */
	result = RvSocketSetBlocking(&pThis->socket, RV_FALSE, pThis->pLogMgr);
	if (result != RV_OK)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Can't set socket non blocking mode\r\n"));
		RvSocketDestruct(&pThis->socket, RV_FALSE, NULL, pThis->pLogMgr);
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
		return result;
	}

	/* constructs a socket file descriptor for the select engine 			*/
	result = RvFdConstruct(&pThis->socketFd, &pThis->socket, pThis->pLogMgr);
	if (result != RV_OK)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Can't construct socketFd\r\n"));
		RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
		return result;
	}

 	/* setting the user-contxt field to point on the transport object 		*/
 	/* In the main select callback we could identify the desired transport 	*/
	pThis->socketFd.userContext = (void*) pThis;

	/* check for connect events on the FD									*/
    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - select connect\r\n"));
    result = RvSelectAdd(pThis->pSelectEngine, &pThis->socketFd, RvSelectConnect, RtspTransportOnSelectEv);
	if (result != RV_OK)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Can't add socketFd to select engine\r\n"));
	    RvFdDestruct(&pThis->socketFd);
		RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
		return result;
	}


	/* binding the socket to the given source address 						*/
	if (sourceAddress != NULL)
	{
		result = RvSocketBind(&pThis->socket, (RvAddress*)sourceAddress, NULL, pThis->pLogMgr);
		if (result != RV_OK)
		{
		    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Can't Bind socket\r\n"));
			RvSelectRemove(pThis->pSelectEngine,&pThis->socketFd);
		    RvFdDestruct(&pThis->socketFd);
			RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
			RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
			return result;
		}
	}

	/* start the connection between the specified socket and the specified	*/
	/* destination. when the remote side accepts this connection, this 		*/
	/* socket will receive an RvSelectConnect event in the select module.	*/
	result = RvSocketConnect(&pThis->socket, (RvAddress*)remoteAddress, pThis->pLogMgr);
	if (result != RV_OK)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect - Can't connect socket\r\n"));
		RvSelectRemove(pThis->pSelectEngine,&pThis->socketFd);
	    RvFdDestruct(&pThis->socketFd);
		RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
		return result;
	}

	pThis->transportState = RTSP_TRANSPORT_STATE_CONNECTING;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "rvRtspTransportConnect\r\n"));
	RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	return RV_OK;
}





/**************************************************************************
 * RtspTransportDisconnect
 * ------------------------------------------------------------------------
 * General: This method requests a disconnection of the transport's TCP
 *			connection to the RTSP server.
 *			The method changes the transport's state and updates the select
 *			engine accordingly.
 *
 * Return Value:	RV_OK - If the method is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- the transport object.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportDisconnect(
			IN	RtspTransport	*pThis)
{

    /* checking parameters and states */
	if (pThis == NULL)
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if (pThis->transportState == RTSP_TRANSPORT_STATE_DESTRUCTED)
		return RvRtspErrorCode(RV_ERROR_DESTRUCTED);

	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
	RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDisconnect\r\n"));

	/* checking state - we allow disconnect only on connected transports */
	if (pThis->transportState != RTSP_TRANSPORT_STATE_CONNECTED)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDisconnect - transport is not connected\r\n"));
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	    return RvRtspErrorCode(RV_ERROR_NOTSUPPORTED);
	}

	/* clear and destruct the socket */
	/* the shutdown method clears the buffers still in the socket (via the select callbacks)*/
	/* and asks the remote side to close its connection. when the remote side closes its	*/
	/* connection, an RvSocketCloseEvent will be received in the select engine				*/
	RvSelectUpdate(	pThis->pSelectEngine,
					&pThis->socketFd,
					RvSelectClose,
					RtspTransportOnSelectEv);
	RvSocketShutdown(&pThis->socket, RV_TRUE, pThis->pLogMgr);
	pThis->transportState = RTSP_TRANSPORT_STATE_DISCONNECTING;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDisconnect\r\n"));
	RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	return RV_OK;
}





/**************************************************************************
 * RtspTransportSend
 * ------------------------------------------------------------------------
 * General: Sends an RTSP message (can be either request or response) on
 *			the given RtspTransport connection.
 *			the sending is done asynchronously, so finishing the sending
 *			might need to be done in a slect engine callback
 *
 * Return Value:	RV_OK - If the connection is successfuly initialized
 *					negative value is returned when encountring an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis	- the transport on which to send.
 *				hbuffer	- buffer containing the message to send.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportSend(
			IN	RtspTransport	*pThis,
			IN	HRPOOLELEM		hBuffer)
{
 	RvStatus 	result;

    /* checking parameters and states */
	if ((pThis == NULL) || (hBuffer == NULL))
		return RvRtspErrorCode(RV_ERROR_NULLPTR);

    if (pThis->transportState == RTSP_TRANSPORT_STATE_DESTRUCTED)
		return RvRtspErrorCode(RV_ERROR_DESTRUCTED);

	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportSend\r\n"));

	/* checking state - send is only possible on a connected transport */
	if (pThis->transportState != RTSP_TRANSPORT_STATE_CONNECTED)
	{
	    RvLogWarning(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportSend - transport is not connected\r\n"));
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	    return RvRtspErrorCode(RV_ERROR_NOTSUPPORTED);
	}

	/* insert to queue */
    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportSend adding buffer to txQueue\r\n"));
	result = RvQueueSend(&pThis->txQueue, (void*)&hBuffer, sizeof(HRPOOLELEM), RV_QUEUE_SEND_NORMAL, RV_QUEUE_NOWAIT);

	if (result == RV_QUEUE_ERROR_FULL)
	{
	    RvLogWarning(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportSend - Queue OVF\r\n"));
		RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
	    return RvRtspErrorCode(RV_ERROR_OUTOFRESOURCES);
	}

	/* start transmitting only if we are in Idle mode */
	if (pThis->txState == RTSP_TRANSPORT_TX_STATE_IDLE)
		RtspTransportSendPrepareNextMessage(pThis);

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportSend\r\n"));
	RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    return RV_OK;
}

/**************************************************************************
 * RtspTransportReceive
 * ------------------------------------------------------------------------
 * General: This function handles the reception of data from the application.
 *
 * Return Value:	 None
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis	    - the transport on which to send.
 *				pBuff	    - The received data to handle.
 *              buffSize    - The buffer's size.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI void RVCALLCONV RtspTransportReceive(
			IN	RtspTransport	*pThis,
			IN	RvUint8  		*pBuff,
            IN  RvUint32        buffSize)
{
    /* TODO - do we need to lock here */
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportReceive\r\n"));

	if (pThis->transportState != RTSP_TRANSPORT_STATE_CONNECTED)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportReceive - Wrong state\r\n"));
		return;
	}

    /* Copy the received buffer into the socket receive buffer to simulate date
    that was received on the transport from the socket */
    memcpy(pThis->rxBuffer, pBuff, buffSize);
    pThis->rxBufferLength = buffSize;
    pThis->rxBufferRead = 0;

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "Received data from App - %d\r\n", pThis->rxBufferLength));
    if (pThis->rxBufferLength > 0)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, " <-- Message Chunk Received:"));
        printCharBuffer((RvChar *)(pThis->rxBuffer), (RvInt32)pThis->rxBufferLength, pThis->pLogSrc);
    }
#endif

    
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportReceive\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    /* parse the data until we reach the end of the received data */
    while (pThis->rxBufferLength > (RvSize_t)pThis->rxBufferRead)
    {
        switch (pThis->rxState)
        {
        case RTSP_TRANSPORT_RX_STATE_FIRST_LINE:
            {
                /* read the first line into rxLine, move the line into firstLine and change	*/
                /* state when finished														*/
                RtspTransportRxFirstLine(pThis);
                break;
            }
            
        case RTSP_TRANSPORT_RX_STATE_HEADER_LINE:
            {
                /* read a header into rxLine, pushing it into headersQueue and switching to	*/
                /* RX_STATE_TESTING_EOH when done									 		*/
                RtspTransportRxHeaderLine(pThis);
                break;
            }
            
        case RTSP_TRANSPORT_RX_STATE_TESTING_EOH:
            {
                /* if we reached the end of the headers, switch to message body or new 		*/
                /* message, else, change state back to RX_STATE_HEADER_LINE 				*/
                RtspTransportRxTestingEoh(pThis);
                break;
            }
            
        case RTSP_TRANSPORT_RX_STATE_BODY:
            {
                /* if we read the entire message, move to the next message, otherwise, 		*/
                /* continue reading the body 												*/
                RtspTransportRxBody(pThis);
                break;
            }
            
        default:
            {
                break;
            }
        } /* switch */
    } /* while */
}



/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/



/* Transport Functions */

/**************************************************************************
 * RtspTransportOnConnect
 * ------------------------------------------------------------------------
 * General: This method will be called when the TCP connection request made
 *			on this transport either succeeds or fails.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- transport object to which the connection
 *							belongs.
 *				success		- has the connection been established or failed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportOnConnect(
						IN	RtspTransport	*pThis,
						IN 	RvBool			success)
{

    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnConnect\r\n"));

    if (pThis->transportState != RTSP_TRANSPORT_STATE_CONNECTING)
	{
		RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnConnect - transport is not connecting\r\n"));
		return;
	}

	/* checking for failure */
	if (success == RV_FALSE)
	{
		/* if failed - destruct the socket , call pfnOnConnectEv, unlock and exit */
	    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnConnect - connection failed\r\n"));
		pThis->transportState = RTSP_TRANSPORT_STATE_CONSTRUCTED;

		RvSelectRemove(pThis->pSelectEngine,&pThis->socketFd);
	    RvFdDestruct(&pThis->socketFd);
		RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
	}

	/* performing OnConnect of a successfull connection */
	else
	{
		/* setting the callback for data reception on the transport */
		RtspUtilsClearMemoryQueue(pThis->hRPool, &pThis->txQueue);
		RtspTransportClearMessage(pThis->hRPool, &pThis->rxMessage);

		pThis->transportState	= RTSP_TRANSPORT_STATE_CONNECTED;
		pThis->rxSkipEol		= RV_FALSE;
		pThis->rxState			= RTSP_TRANSPORT_RX_STATE_FIRST_LINE;
		pThis->txState			= RTSP_TRANSPORT_TX_STATE_IDLE;

		RvSelectUpdate(	pThis->pSelectEngine,
						&pThis->socketFd,
						RvSelectRead | RvSelectClose,
						RtspTransportOnSelectEv);
	}

	RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnConnect\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    
	if (pThis->pfnCallbacks.pfnOnConnectEv != NULL)
    {
        pThis->pfnCallbacks.pfnOnConnectEv(pThis->context, success);
    }
    
    return;
}


/**************************************************************************
 * RtspTransportOnDisconnect
 * ------------------------------------------------------------------------
 * General: This method will be called when the disconnection request either
 *			succeeds or fails.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis	- transport object to which the connection belongs.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportOnDisconnect(
						IN	RtspTransport	*pThis)
{
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnDisconnect\r\n"));

	/* checking state - we allow disconnect only on connected transports */
	if	((pThis->transportState != RTSP_TRANSPORT_STATE_DISCONNECTING) &&
		 (pThis->transportState != RTSP_TRANSPORT_STATE_CONNECTED) )
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnDisconnect - transport is not disconnecting\r\n"));
	    return;
	}

	/* remove the transport's socket from the select engine */
	RvSelectRemove(pThis->pSelectEngine,&pThis->socketFd);

	/* destruct the socket's file descriptor */
    RvFdDestruct(&pThis->socketFd);

	RvSocketDestruct(&pThis->socket, RV_FALSE, NULL, pThis->pLogMgr);

	pThis->transportState = RTSP_TRANSPORT_STATE_CONSTRUCTED;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnDisconnect\r\n"));

    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

    if (pThis->pfnCallbacks.pfnOnDisconnectEv != NULL)
    {
		pThis->pfnCallbacks.pfnOnDisconnectEv(pThis->context);
    }
    
    return;
}


/**************************************************************************
 * RtspTransportSendPrepareNextMessage
 * ------------------------------------------------------------------------
 * General: This method prepares for sending the next message. It
 *			gets a message element from the txQueue, and sets up the select
 *			engine to send the message using the RtspTransportDoSend function.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport object on which the buffer
 *							will be sent.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportSendPrepareNextMessage(
						IN	RtspTransport	*pThis)
{
	RvStatus	result;

	/* find the next used location in the queue and start the state machine	*/
	result = RvQueueReceive(&pThis->txQueue,
							RV_QUEUE_NOWAIT,
							sizeof(HRPOOLELEM),
							(void*)(&pThis->txElement));

	if (RvErrorGetCode(result) == RV_QUEUE_ERROR_EMPTY)
	{
		/* nothing to send */
		pThis->txElementOffset	= 0;
		pThis->txElement 		= NULL;
		pThis->pTxChunk			= NULL;
		pThis->txChunkLength	= 0;
		pThis->txChunkSent		= 0;
		pThis->txState 			= RTSP_TRANSPORT_TX_STATE_IDLE;

		/* setting the callback for data reception on the transport */
		RvSelectUpdate(	pThis->pSelectEngine,
						&pThis->socketFd,
						RvSelectRead | RvSelectClose,
						RtspTransportOnSelectEv);
	}

	else
	{
		/* set up sending of the buffer */
		pThis->txElementOffset	= 0;
		pThis->pTxChunk			= NULL;
		pThis->txChunkLength	= 0;
		pThis->txChunkSent		= 0;
		pThis->txState 			= RTSP_TRANSPORT_TX_STATE_SENDING;

		/* setting the callback for data reception on the transport */
		RvSelectUpdate(	pThis->pSelectEngine,
						&pThis->socketFd,
						RvSelectRead | RvSelectWrite | RvSelectClose,
						RtspTransportOnSelectEv);
	}

    return;
}




/**************************************************************************
 * RtspTransportDoSend
 * ------------------------------------------------------------------------
 * General: This method sends the next buffer (chunk) on the connection.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- the transport object on which the buffer
 *								will be sent.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportDoSend(
						IN	RtspTransport	*pThis)
{
	/* for the sending of RPool chunks */
	RvStatus result;
	RvSize_t bytesSent;

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoSend\r\n"));

	if (pThis->txState != RTSP_TRANSPORT_TX_STATE_SENDING)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoSend - Idle state\r\n"));
		RvSelectUpdate(	pThis->pSelectEngine,
						&pThis->socketFd,
						RvSelectRead | RvSelectClose,
						RtspTransportOnSelectEv);
		return;
	}

	/* if we need a new memory chunk. we'll pop one from the HPOOLELEM we have*/
	if (pThis->txChunkLength == (RvInt32)pThis->txChunkSent)
	{
		/* length: if positive - size of contigous area from the pointer to the */
		/* end of the current segment, if 0 - end of buffer/out of range,	 	*/
		/* if negative - failed.												*/
		/* offset is the offset in bytes into the current element, starts at 0,	*/
		/* but can be a different value if we had to stop in the middle of		*/
		/* sending a buffer.													*/
		/* pChunk A real pointer to the place calculated by the offset.			*/
		pThis->txChunkLength = rpoolGetPtr(	pThis->hRPool,
											pThis->txElement,
											(RvInt32)pThis->txElementOffset,
											(void **)&pThis->pTxChunk);

		/* if there are no more chunks to send, we'll fetch the next buffer */
		if (pThis->txChunkLength <= 0 )
		{
			/* remove the buffer from txQueue									*/
			rpoolFree(pThis->hRPool, pThis->txElement);
			RtspTransportSendPrepareNextMessage(pThis);
		}

		else
		{
			pThis->txElementOffset += (RvSize_t)pThis->txChunkLength;
			pThis->txChunkSent		= 0;
		}
	}

	else
	{
		/* the function copies the given buffer to the operating system's memory for later	*/
		/* sending. The function will return immediately, indicating the exact amount of 	*/
		/* bytes the operating system has processed and sent.								*/
		/* if the bytesSent is less than length, we need to wait for the RvSelectWrite event*/
		/* in the select module before trying to send the buffer again.						*/
		result = RvSocketSendBuffer(&pThis->socket,
									pThis->pTxChunk + pThis->txChunkSent,
									(RvSize_t)pThis->txChunkLength - pThis->txChunkSent,
									NULL,
									pThis->pLogMgr,
									&bytesSent);

		pThis->txChunkSent	+= bytesSent;
#if RV_LOGMASK != RV_LOGLEVEL_NONE
        if ((pThis->txChunkLength > 0) && ((RvInt32)pThis->txChunkSent == pThis->txChunkLength) && result >= RV_OK)
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, " --> Message Sent:"));
            printCharBuffer((RvChar *)pThis->pTxChunk, pThis->txChunkLength, pThis->pLogSrc);
        }
#endif
	}

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportSendDoStateMachine\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    return;
}





/**************************************************************************
 * RtspTransportDoReceive
 * ------------------------------------------------------------------------
 * General: This method handles the reception of data from the connection.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- the transport object on which the data
 *								was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportDoReceive(
						IN	RtspTransport	*pThis)
{
    RvStatus status = RV_OK;

    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoReceive\r\n"));

	if (pThis->transportState != RTSP_TRANSPORT_STATE_CONNECTED)
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoReceive - Wrong state\r\n"));
		return;
	}


	/* reading as much as we can into our little rxBuffer */
	pThis->rxBufferRead = 0;
	RvSocketReceiveBuffer(	&pThis->socket,
							pThis->rxBuffer,
							RTSP_TRANSPORT_RECEIVE_BUFFER_SIZE,
							pThis->pLogMgr,
							&pThis->rxBufferLength,
							NULL);

#if RV_LOGMASK != RV_LOGLEVEL_NONE
    RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "Received data on socket - %d\r\n", pThis->rxBufferLength));
    if (pThis->rxBufferLength > 0)
    {
        RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, " <-- Message Received:"));
        printCharBuffer((RvChar *)(pThis->rxBuffer), (RvInt32)pThis->rxBufferLength, pThis->pLogSrc);
    }
#endif
    
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoReceive\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

     /* Notify the connection about the received chunk */
    if (pThis->pfnCallbacks.pfnOnRawBufferReceiveEv != NULL)
    {
        status = pThis->pfnCallbacks.pfnOnRawBufferReceiveEv((void *)pThis->context, pThis->rxBuffer, pThis->rxBufferLength);
        if (status != RV_OK)
        {
            RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoReceive - Data received on socket ignored\r\n"));
        }
    }

    if (status != RV_OK)
        /* Ignore the data */
        return;

    /* parse the data until we reach the end of the received data */
	while (pThis->rxBufferLength > (RvSize_t)pThis->rxBufferRead)
	{
		switch (pThis->rxState)
		{
			case RTSP_TRANSPORT_RX_STATE_FIRST_LINE:
			{
				/* read the first line into rxLine, move the line into firstLine and change	*/
				/* state when finished														*/
				RtspTransportRxFirstLine(pThis);
				break;
			}

			case RTSP_TRANSPORT_RX_STATE_HEADER_LINE:
			{
				/* read a header into rxLine, pushing it into headersQueue and switching to	*/
				/* RX_STATE_TESTING_EOH when done									 		*/
				RtspTransportRxHeaderLine(pThis);
				break;
			}

			case RTSP_TRANSPORT_RX_STATE_TESTING_EOH:
			{
				/* if we reached the end of the headers, switch to message body or new 		*/
				/* message, else, change state back to RX_STATE_HEADER_LINE 				*/
				RtspTransportRxTestingEoh(pThis);
				break;
			}

			case RTSP_TRANSPORT_RX_STATE_BODY:
			{
				/* if we read the entire message, move to the next message, otherwise, 		*/
				/* continue reading the body 												*/
				RtspTransportRxBody(pThis);
				break;
			}

			default:
			{
				break;
			}
		} /* switch */
	} /* while */
}


/**************************************************************************
 * RtspTransportAddUpdate
 * ------------------------------------------------------------------------
 * General: This method Adds and updates the select engine with the supplied
 *          events on the transport
 *
 * Return Value:    RV_OK if successful. A negative value otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :    transport         - the new connection's transport.
                 selectEvents      - the type of events to register for.
 * OUTPUT   :    None.
 * INOUT    :    None.
 *************************************************************************/
RVAPI 
RvStatus RtspTransportAddUpdateEvents(
                  IN RtspTransport    *transport,
                  IN RvSelectEvents   selectEvents)
{
    RvStatus result;

    result = RV_OK;

    result = RvSelectAdd(transport->pSelectEngine, &transport->socketFd,
                         selectEvents,
                         RtspTransportOnSelectEv);

    if (result != RV_OK)
    {
        RvMutexLock(&transport->mutex, transport->pLogMgr);
        RvLogError(transport->pLogSrc, (transport->pLogSrc, "Failed to add socketFd to select engine\r\n"));
        RvMutexUnlock(&transport->mutex, transport->pLogMgr);
        return result;
    }

    RvSelectUpdate(transport->pSelectEngine, &transport->socketFd,
                   selectEvents,
                   RtspTransportOnSelectEv);

    return RV_OK;
}

/**************************************************************************
 * RtspTransportOnSelectEv
 * ------------------------------------------------------------------------
 * General: This method handles the interface between the select engine and
 *			the Rtsp callback functions.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pSelectEngine	- the main select engine.
 *				pFd				- the file descriptor on which the event occured.
 *				selectEvent		- the type of event which occured.
 *				error			- has an error occured.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportOnSelectEv(
        IN RvSelectEngine*  pSelectEngine,
        IN RvSelectFd*      pFd,
        IN RvSelectEvents   selectEvent,
        IN RvBool           error)
{
	RtspTransport	*pThis;
    RvStatus        result;       
    RvSocket        newSocket;
    RvAddress       remoteAddress;
    RvBool          success;
    RvChar          strIp[MAX_STR_IP_BUF_SIZE];
    RvUint16        portNum;
    
    result       = 0;
    newSocket    = 0;
    success      = RV_FALSE;
    portNum      = 0;
    memset(&remoteAddress,0,sizeof(RvAddress));
    memset(strIp, 0, sizeof(strIp));

	/* test parameters 					*/
	if ((pSelectEngine == NULL) || (pFd == NULL))
		return;

	/* get the transport pointer		*/
	pThis = (RtspTransport*)pFd->userContext;	/* void *userContext is the transport pointer */

	if (pThis == NULL)
		return;

	RvMutexLock(pThis->pGMutex, pThis->pLogMgr);

	/* check if during the select process the transport has died	*/
	if (pThis->transportState == RTSP_TRANSPORT_STATE_DESTRUCTED)
	{
		RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
		return;
	}

	RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnSelectEv\r\n"));

	/* handle the error if there is one	*/
	if (error == RV_TRUE)
	{
		RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnSelectEv error received \r\n"));
	}


	/* switch on selectEvents, and 		*/
	/* dispatch the event accordingly	*/
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnSelectEv\r\n"));
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);

    switch(selectEvent)
    {
        case RvSelectAccept:
        {

            result = RvSocketAccept(&pThis->socket,pThis->pLogMgr,
                                    &newSocket, &remoteAddress);
            if( result != RV_OK)
            {
                RvMutexLock(&pThis->mutex, pThis->pLogMgr);
                RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "Failed to accept connection on the Socket\r\n"));
                RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
                return;
            }

            RvAddressGetString(&remoteAddress, MAX_STR_IP_BUF_SIZE, strIp);

            portNum = RvAddressGetIpPort(&remoteAddress);

            /* Transport callback on accept event */
            if (pThis->pfnCallbacks.pfnOnAcceptEv != NULL)
            {
                pThis->pfnCallbacks.pfnOnAcceptEv(pThis->context, newSocket, strIp, portNum,
                                                  &success);
            }

            if (success != RV_TRUE)
            {
                RvMutexLock(&pThis->mutex, pThis->pLogMgr);
                RvSocketDestruct(&newSocket, RV_TRUE, NULL, pThis->pLogMgr);
                RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "Connection request rejected\r\n"));
                RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            }

            break;
 
        }
        case RvSelectConnect:
		{
            RvBool isSuccess = !error;
			RtspTransportOnConnect(pThis, isSuccess); 
            return;
		}

        case RvSelectClose:
	    {
			RtspTransportOnDisconnect(pThis);
            return;
		}

	    case RvSelectRead:
	    {
			RtspTransportDoReceive(pThis);
            return;
		}

	    case RvSelectWrite:
		{
			RtspTransportDoSend(pThis);
            return;
		}

		default:
		{
			RvLogDebug(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportOnSelectEv non supported event arrived\r\n"));
			break;
		}
	}
}



/* Utility functions */


/**************************************************************************
 * RtspTransportClearMessage
 * ------------------------------------------------------------------------
 * General: This function clears a message, releasing it's elements
 *			from the RPool memory.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRPool		- The pool to which the elements in the message
 *							belong.
 *				pMessage	- pointer to message to clear.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportClearMessage(
				IN	HRPOOL					hRPool,
				IN	RtspTransportMessage	*pMessage)
{
 	if (pMessage->hFirstLine != NULL)
	{
		rpoolFree(hRPool, pMessage->hFirstLine);
		pMessage->hFirstLine = NULL;
	}

	if (pMessage->hBody != NULL)
	{
		rpoolFree(hRPool, pMessage->hBody);
		pMessage->hBody = NULL;
	}

	RtspUtilsClearMemoryQueue(hRPool, &pMessage->headersQueue);
}



/* message Parsing Functions */


/**************************************************************************
 * RtspTransportRxSkipEol
 * ------------------------------------------------------------------------
 * General: This function skips the characters until reaching the start
 *			of the next line (rxBufferRead is set to the last character
 *			of the line + 1).
 *
 * Return Value:	RV_TRUE if a line-feed character was found, RV_FALSE
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static RvBool	RtspTransportRxSkipEol(
				IN	RtspTransport	*pThis)
{
	RvBool		result;

	RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxSkipEol\r\n"));

	result = RtspUtilsStrChr(	(RvChar*)pThis->rxBuffer,
							  	(RvSize_t)pThis->rxBufferRead,
								pThis->rxBufferLength,
								LF_CHARACTER,
							   	&pThis->rxBufferRead);

	if (result == RV_TRUE)
	{
	  	pThis->rxBufferRead++;
	}

	RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxSkipEol\r\n"));

	return result;
}


/**************************************************************************
 * RtspTransportRxReadLine
 * ------------------------------------------------------------------------
 * General: This function reads the buffer into the rxLine until reaching
 *			the end of the line.
 *
 * Return Value:	RV_TRUE if finished reading the line, RV_FALSE otherwise
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static RvBool RtspTransportRxReadLine(
				IN	RtspTransport	*pThis)
{
	RvBool		eolFound;
	RvSize_t	eolIndex;
    RvInt32     lastSize;
	RvUint8		*pAppendedData;
	RvSize_t	lengthToAlloc;

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxReadLine\r\n"));

	eolFound = RtspUtilsStrDelimiters(	(RvChar*)pThis->rxBuffer,
								  		(RvSize_t)pThis->rxBufferRead,
										pThis->rxBufferLength,
								   		TERMINATING_DELIMITERS_STRING,
								   		RV_FALSE,
								   		&eolIndex);

	lengthToAlloc = eolIndex - (RvSize_t)pThis->rxBufferRead;
	if (eolFound == RV_TRUE)
		lengthToAlloc += 1;

	if (pThis->rxLine == NULL)
	{
		pThis->rxLine = rpoolAlloc(pThis->hRPool, (int)lengthToAlloc);
		lastSize = 0;
	}

	/* eolIndex now points to the last character in the line or the last character in the buffer */
	/* the copy includes the first terminating character */

	/* allocating the additional memory */
	else if (rpoolAppend(	pThis->hRPool,
							pThis->rxLine,
							(int)lengthToAlloc,
							&lastSize,
							&pAppendedData) <= 0 )
	{
	    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxReadLine- memory allocation failed\r\n"));
		return RV_FALSE;
	}

	/* copy the buffer only if buffer does not start with CRLF;
	   otherwise, simply terminate the line and return true */
	if (eolIndex - (RvSize_t)pThis->rxBufferRead > 0)
	{
		/* copying the buffer */
		pThis->rxLine = rpoolCopyFromExternal(	pThis->hRPool,
												pThis->rxLine,
												pThis->rxBuffer + pThis->rxBufferRead,
												lastSize,
												(RvInt32)eolIndex - pThis->rxBufferRead);
		if ( pThis->rxLine == NULL )
		{
			RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxReadLine - memory allocation failed\r\n"));
			return RV_FALSE;
		}
	}

	if (eolFound == RV_TRUE)
	{
		RtspUtilsRPOOLELEMSetCell(	pThis->hRPool,
									pThis->rxLine,
									(RvSize_t)lastSize + 	eolIndex - (RvSize_t)pThis->rxBufferRead,
									0);
	}

	/* advance the rxBufferRead index to the end of the line */
	pThis->rxBufferRead = (RvInt32)eolIndex;



	return eolFound;
}

/**************************************************************************
 * RtspTransportRxFirstLine
 * ------------------------------------------------------------------------
 * General: This function reads the first line, until the end of it, and
 *			then changes state to RX_STATE_TESTING_EOH.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportRxFirstLine(
				IN	RtspTransport	*pThis)
{
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxFirstLine\r\n"));

	if (pThis->rxSkipEol)
	{
		if (RtspTransportRxSkipEol(pThis) == RV_TRUE)
		{
			pThis->rxState		= RTSP_TRANSPORT_RX_STATE_TESTING_EOH;
			pThis->rxSkipEol	= RV_FALSE;
		}
	}

	/* if read entire line to rxLine - move it to FirstLine */
	else if (RtspTransportRxReadLine(pThis) == RV_TRUE)
	{
		pThis->rxMessage.hFirstLine = pThis->rxLine;
		pThis->rxLine				= NULL;
		pThis->rxMessageBodyLength	= 0;
		pThis->rxMessageBodyRead	= 0;
		pThis->rxSkipEol			= RV_TRUE;
	}

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxFirstLine\r\n"));
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
}





/**************************************************************************
 * RtspTransportRxHeaderLine
 * ------------------------------------------------------------------------
 * General: This function reads a header into rxLine, pushing it into
 *			headersQueue and switching to RX_STATE_TESTING_EOH when done.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void	RtspTransportRxHeaderLine(
				IN	RtspTransport	*pThis)
{
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxHeaderLine\r\n"));

	if (pThis->rxSkipEol)
	{
		if (RtspTransportRxSkipEol(pThis) == RV_TRUE)
		{
			pThis->rxState		= RTSP_TRANSPORT_RX_STATE_TESTING_EOH;
			pThis->rxSkipEol	= RV_FALSE;
		}
	}


	else if (RtspTransportRxReadLine(pThis) == RV_TRUE)
	{
		/* if end of line was reached, put the header line into the header queue	*/
		if (RvQueueSend(&pThis->rxMessage.headersQueue,
						(void*)(&pThis->rxLine),
						sizeof(HRPOOLELEM),
						RV_QUEUE_SEND_NORMAL,
						RV_QUEUE_NOWAIT) != RV_OK)
		{
		    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportDoReceive - Queue Error\r\n"));
			rpoolFree(pThis->hRPool, pThis->rxLine);
			pThis->rxLine = NULL;
            RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
            RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
            return;
		}

        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

        /* find out content-length */
		if (pThis->pfnCallbacks.pfnOnContentLengthEv != NULL)
        {
			pThis->pfnCallbacks.pfnOnContentLengthEv(pThis->context, pThis->rxLine, &pThis->rxMessageBodyLength);
        }
        
		pThis->rxLine		= NULL;
		pThis->rxSkipEol	= RV_TRUE;
        
        return;
	}

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxHeaderLine\r\n"));
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
}




/**************************************************************************
 * RtspTransportRxTestingEoh
 * ------------------------------------------------------------------------
 * General: This function tests if we reached the end of the headers, if
 *			so, it switches to message body or new message, otherwise, it
 *			changes the state back to RX_STATE_HEADER_LINE.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void RtspTransportRxTestingEoh(
				IN	RtspTransport	*pThis)
{
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxTestingEoh\r\n"));

	if (pThis->rxSkipEol)
	{
		if (RtspTransportRxSkipEol(pThis) == RV_TRUE)
		{
			pThis->rxSkipEol	= RV_FALSE;

			if (pThis->rxMessageBodyLength != 0)
			{
				pThis->rxState 	= RTSP_TRANSPORT_RX_STATE_BODY;
			}

			else
			{
                RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
                RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

                /* report on new message 					*/
                if (pThis->pfnCallbacks.pfnOnReceiveEv != NULL)
                {
					pThis->pfnCallbacks.pfnOnReceiveEv(pThis->context, &pThis->rxMessage);
                }

				/* Clear the receive transmission message 	*/
				if (pThis->hRPool != NULL)
				{
					RtspTransportClearMessage(pThis->hRPool, &pThis->rxMessage);
					pThis->rxState 	= RTSP_TRANSPORT_RX_STATE_FIRST_LINE;
				}

                return;
			}
		}
	}

	/* if this is the end of the headers change state to message body*/
	else if (	(pThis->rxBuffer[pThis->rxBufferRead] == CR_CHARACTER) ||
				(pThis->rxBuffer[pThis->rxBufferRead] == LF_CHARACTER) )
	{
		pThis->rxSkipEol = RV_TRUE;
	}

	/* not the end of the headers						*/
	else
	{
		pThis->rxState = RTSP_TRANSPORT_RX_STATE_HEADER_LINE;
	}

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxTestingEoh\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
}




/**************************************************************************
 * RtspTransportRxReadBody
 * ------------------------------------------------------------------------
 * General: This function copies the buffer into the message body.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void	RtspTransportRxReadBody(
				IN	RtspTransport	*pThis)
{
	RvSize_t 	lengthToCopy;
    RvInt32      lastSize;
	RvUint8		*pAppendedData;

    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxReadBody\r\n"));

	/* we copy the part of the message body which is in the buffer			*/
	if (pThis->rxBufferLength - (RvSize_t)pThis->rxBufferRead < pThis->rxMessageBodyLength - pThis->rxMessageBodyRead )
		lengthToCopy = pThis->rxBufferLength - (RvSize_t)pThis->rxBufferRead;
	else
		lengthToCopy = pThis->rxMessageBodyLength - pThis->rxMessageBodyRead;

	if (lengthToCopy == 0)
		return;

	/* if the message body is empty, we copy the buffer into a new element */
	if (pThis->rxMessage.hBody == NULL )
	{
		pThis->rxMessage.hBody = rpoolAllocCopyExternal(pThis->hRPool,
														0,
														pThis->rxBuffer + pThis->rxBufferRead,
														(RvInt32)lengthToCopy);
		if ( pThis->rxMessage.hBody == NULL )
		{
		    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxBody - memory allocation failed\r\n"));
			return;
		}
	}

	/* if the message body is not empty we append the buffer to it		 	*/
	else
	{
		/* allocating the additional memory */
		if (rpoolAppend(pThis->hRPool,
						pThis->rxMessage.hBody,
						(RvInt32)lengthToCopy,
						&lastSize,
						&pAppendedData) <= 0)
		{
		    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxBody - memory allocation failed\r\n"));
			return;
		}

		/* copying the buffer */
		pThis->rxMessage.hBody = rpoolCopyFromExternal(	pThis->hRPool,
														pThis->rxMessage.hBody,
														pThis->rxBuffer + pThis->rxBufferRead,
														(RvInt32)pThis->rxMessageBodyRead,
														(RvInt32)lengthToCopy);
		if (pThis->rxMessage.hBody == NULL)
		{
		    RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxBody - memory allocation failed\r\n"));
			return;
		}
	}

	pThis->rxMessageBodyRead 	+= lengthToCopy;
	pThis->rxBufferRead			+= (RvInt32)lengthToCopy;

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxReadBody\r\n"));
}




/**************************************************************************
 * RtspTransportRxBody
 * ------------------------------------------------------------------------
 * General: This function tests if we read the entire message, if so, it
 *			moves to the next message, otherwise, it continues to read the
 *			body.
 *
 * Return Value:	None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the transport on which the data was received.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
static void	RtspTransportRxBody(
				IN	RtspTransport	*pThis)
{
    RvMutexLock(pThis->pGMutex, pThis->pLogMgr);
    RvMutexLock(&pThis->mutex, pThis->pLogMgr);
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxBody\r\n"));

	/* reading the message body */
	RtspTransportRxReadBody(pThis);

	if (pThis->rxMessageBodyRead == pThis->rxMessageBodyLength)
	{
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);

        /* report on new message 					*/
		if (pThis->pfnCallbacks.pfnOnReceiveEv != NULL)
        {
            pThis->pfnCallbacks.pfnOnReceiveEv(pThis->context, &pThis->rxMessage);
        }

		/* Clear the receive transmission message */
		RtspTransportClearMessage(pThis->hRPool, &pThis->rxMessage);

		pThis->rxLine = NULL;
		pThis->rxState 	= RTSP_TRANSPORT_RX_STATE_FIRST_LINE;
        return;
	}

    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportRxBody\r\n"));
    RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
    RvMutexUnlock(pThis->pGMutex, pThis->pLogMgr);
}

#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
/***************************************************************************
 * RtspTransportListen 
 * ------------------------------------------------------------------------
 * General: 
 * This method creates a server socket and listens for and accepts
 * incoming client TCP connection requests. 
 * 
 * Arguments:
 * INPUT:   pThis         - the transport to destruct.
 *          *localAddr    - Local address of the server.
 *          listeningPort - listening port.
 * OUTPUT:  None.
 *
 * Return Value:  RV_OK - If the method is successfully completed,
 *                Negative values otherwise.
 ***************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportListen(
                          IN RtspTransport *pThis, 
                          IN const RvChar  *localAddr,
                          IN const RvUint16     listeningPort)
{
    RvStatus  result;
    RvAddress localIPAddr;
    RvUint32  ip;

    result = RV_OK;
    
    RvLogEnter(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportListen\r\n"));
    
    RtspUtilsClearMemoryQueue(pThis->hRPool, &pThis->txQueue);
    RtspTransportClearMessage(pThis->hRPool, &pThis->rxMessage);

#if ((RV_NET_TYPE & RV_NET_IPV6) != 0)
    if (localAddr[0] == '[')
    {
        RvAddressStringToIpv6(&ip, localAddr);
        RvAddressConstructIpv6(&localIPAddr, ip, listeningPort);

        /* Construct the socket                                                   */
        RvSocketConstruct(RV_ADDRESS_TYPE_IPV6,RvSocketProtocolTcp,
                          pThis->pLogMgr, &pThis->socket);
    }
    else
#endif /* RV_NET_IPV6 */
    {
        RvAddressStringToIpv4(&ip, localAddr);
        RvAddressConstructIpv4(&localIPAddr, ip, listeningPort);

        /* Construct the socket                                                   */
        RvSocketConstruct(RV_ADDRESS_TYPE_IPV4,RvSocketProtocolTcp,
                          pThis->pLogMgr, &pThis->socket);
    }
 
    result = RvSocketSetBlocking(&pThis->socket, RV_TRUE, pThis->pLogMgr);

    /* Bind the socket                                                        */
    result = RvSocketBind(&pThis->socket,(RvAddress*)&localIPAddr, NULL, pThis->pLogMgr);
    if (result != RV_OK)
    {
        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "Failed to bind socket\n"));
        RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return result;
    }
    
    result = RvSocketListen(&pThis->socket, 1 ,pThis->pLogMgr);
    if (result != RV_OK)
    {
        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "Failed to listen on socket\n"));
        RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return result;
    }    
    
    result = RvFdConstruct(&pThis->socketFd, &pThis->socket, pThis->pLogMgr);

    if (result != RV_OK)
    {
        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "Failed to construct socketFd\n"));
        RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return result;
    }
   
    /* setting the user-context field to point on the transport object       */
    /* In the main select callback we could identify the desired transport  */
    pThis->socketFd.userContext = (void*) pThis;

    result = RvSelectAdd(pThis->pSelectEngine, &pThis->socketFd, RvSelectAccept, RtspTransportOnSelectEv);

    if (result != RV_OK)
    {
        RvMutexLock(&pThis->mutex, pThis->pLogMgr);
        RvLogError(pThis->pLogSrc, (pThis->pLogSrc, "Failed to add socketFd to select engine\r\n"));
        RvFdDestruct(&pThis->socketFd);
        RvSocketDestruct(&pThis->socket, RV_TRUE, NULL, pThis->pLogMgr);
        RvMutexUnlock(&pThis->mutex, pThis->pLogMgr);
        return result;
    }
    
    RvLogLeave(pThis->pLogSrc, (pThis->pLogSrc, "RtspTransportListen\r\n"));

    return result;
}
#endif /* RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT */

#if defined(__cplusplus)
}
#endif
