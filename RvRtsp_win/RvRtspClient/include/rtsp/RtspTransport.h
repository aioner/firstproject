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
 *                              <rvrtsptransport.h>
 *
 * The Transport layer connects to the server and sets up the callbacks to be
 * called when data is received on the transport connection.
 *
 *    Author                         Date
 *    ------                        ------
 *		Shaft						8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_TRANSPORT_H
#define _RV_RTSP_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/


#include "rvmutex.h"
#include "rvqueue.h"
#include "RvRtspCommonTypes.h"

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

#define MAX_STR_IP_BUF_SIZE           16

/* RtspTransportMessage
 * ----------
 * This structure holds the buffers composing an RTSP message
 */
typedef struct RtspTransportMessage_
{
	HRPOOLELEM	hFirstLine;		/* HRPool element containing the first line	*/
	RvQueue		headersQueue;	/* holding the message headers				*/
	HRPOOLELEM	hBody;			/* HRPool element containing the body		*/
} RtspTransportMessage;



/* RtspTransportState
 * ----------
 * This type specifies the state of the transport object
 */
typedef enum
{
	RTSP_TRANSPORT_STATE_CONSTRUCTED = 0,	/* constructed					*/
	RTSP_TRANSPORT_STATE_CONNECTING,		/* constructed, called connect	*/
	RTSP_TRANSPORT_STATE_CONNECTED,			/* connect acknowlaged			*/
	RTSP_TRANSPORT_STATE_DISCONNECTING,		/* called disconnect			*/
	RTSP_TRANSPORT_STATE_DESTRUCTED			/* destructed					*/
} RtspTransportState;


/* RtspTransportTxState
 * ----------
 * This type specifies the transport transmission state
 */
typedef enum
{
	RTSP_TRANSPORT_TX_STATE_IDLE = 0,		/* Not sending on transport 	*/
	RTSP_TRANSPORT_TX_STATE_SENDING			/* Transmitting data from tx 	*/
											/* queue.						*/
} RtspTransportTxState;


/* RtspTransportRxState
 * ----------
 * This type specifies the transport reception state, indicating what part
 * of the message we are processing.
 */
typedef enum
{
	RTSP_TRANSPORT_RX_STATE_FIRST_LINE = 0,	/* Processing the first line		*/
	RTSP_TRANSPORT_RX_STATE_TESTING_EOH,	/* testing if end of headers reached*/
	RTSP_TRANSPORT_RX_STATE_HEADER_LINE,	/* Processing a header line			*/
	RTSP_TRANSPORT_RX_STATE_BODY			/* Processing a body line			*/
} RtspTransportRxState;



/**************************************************************************
 * RtspTransportOnConnectEv
 * ------------------------------------------------------------------------
 * General: this is the definition for the callback function to be called
 *			when the transport connection has been established (or failed).
 *			Used by the Connection layer to handle the connect event.
 *
 * Return Value:	returns RV_OK if successfuly completed, negative value
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	context	- the context in which the event occoured.
 *				success	- has the connection been established or failed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (*RtspTransportOnConnectEv)(
								IN void		*context,
								IN RvBool	success);


/**************************************************************************
 * RtspTransportOnDisconnectEv
 * ------------------------------------------------------------------------
 * General: this is the definition for the callback function to be called
 *			when the transport connection has been disconnected (or error
 *			occured).
 *
 * Return Value:	returns RV_OK if successfuly completed, negative value
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	context	- the context in which the event occoured.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (*RtspTransportOnDisconnectEv)(
								IN void		*context);



/**************************************************************************
 * RtspTransportOnReceiveEv
 * ------------------------------------------------------------------------
 * General: this is the definition for the callback function to be called
 *			when a message from the server is received on the transport.
 *			Used by the Connection layer to handle the returned message.
 *
 * Return Value:	returns RV_OK if successfuly completed, negative value
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	context		- the context in which the event occoured.
 *				pMessage	- pointer to the received message.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (*RtspTransportOnReceiveEv)(
								IN void							*context,
								IN const RtspTransportMessage	*pMessage);


/**************************************************************************
 * RtspTransportOnContentLengthEv
 * ------------------------------------------------------------------------
 * General: This is the definition for the callback function to be called
 *			when a header line is parsed, to retrieve the message's body length.
 *			Note: if content length is found pContentLength is set to the
 *			length, otherwise, it is not changed.
 *
 * Return Value:	RV_OK if content length is found, RV_ERROR_BADPARAM
 *					otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	context			- The header's connection.
 *				headerLine		- The header line.
 * OUTPUT	:	pContentLength	- The length of the message body.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (*RtspTransportOnContentLengthEv)(
								IN	void					*context,
								IN	HRPOOLELEM				headerLine,
								OUT	RvSize_t				*pContentLength);


/**************************************************************************
 * RtspTransportOnAcceptEv
 * ------------------------------------------------------------------------
 * General: This is the definition for the callback function to be called
 *            when a TCP connection is accepted by the transport layer
 *
 * Return Value:    RV_OK if successful, negative value otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :    context           - The RvRtsp.
 *               socket            - the new socket.
 *               strClientAddr     - Client IP.
 *               portNum           - Client port.
 * OUTPUT   :    success           - has the connection been constructed.
 * INOUT    :    None              
 *************************************************************************/
typedef RvStatus (*RtspTransportOnAcceptEv)(
                                IN    void                    *context,
                                IN RvSocket socket,
                                IN RvChar  *strClientAddr,
                                IN RvUint16 portNum,
                                OUT RvBool *success);

/**************************************************************************
 * RtspTransportOnRawBufferReceiveEv
 * ------------------------------------------------------------------------
 * General: This is the definition for the callback function to be called
 *			when a chunk of data is received on the transport.
 *          If the returned value != RV_OK then the chunk is ignored, 
 *          otherwise it is handled in the usual manner.
 *
 * Return Value:	RV_OK if the received buffer should be handled in the
 *                  usual manner or negative values if buffer should be ignored.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	context			- The header's connection.
 *				headerLine		- The header line.
 * OUTPUT	:	pContentLength	- The length of the message body.
 * INOUT	:	None.
 *************************************************************************/
typedef RvStatus (*RtspTransportOnRawBufferReceiveEv)(
                                IN  void                    *context,
                                IN  RvUint8                 *pBuff,
                                IN  RvUint32                buffSize);


/* RtspTransportCallbackFunctions
 * ----------
 * This structure specifies the transport's callback functions, called when
 * events occur on the transport.
 */
typedef struct RtspTransportCallbackFunctions_
{
	RtspTransportOnConnectEv		    pfnOnConnectEv;		/* connection was established	*/
	RtspTransportOnDisconnectEv		    pfnOnDisconnectEv;	/* connection was disconnected	*/
	RtspTransportOnReceiveEv		    pfnOnReceiveEv;		/* a message was received 		*/
	RtspTransportOnContentLengthEv	    pfnOnContentLengthEv;/* called to extract the message's content-length */
    RtspTransportOnRawBufferReceiveEv   pfnOnRawBufferReceiveEv; /* called every time a chunk is received
                                                                    on the transport */
    RtspTransportOnAcceptEv             pfnOnAcceptEv;      /* TCP connection was accepted   */ 
} RtspTransportCallbackFunctions;

/* RTSP_TRANSPORT_RECEIVE_BUFFER_SIZE
 * ----------
 * The size of the buffer with which we read from the socket.
 */
#define RTSP_TRANSPORT_RECEIVE_BUFFER_SIZE	100

/* RtspTransport
 * ----------
 * The RtspTransport object is used to handle an RvRtspConnection's transport
 * connection. It holds all the data nessecary to maintain the connection, and
 * handles the buffering and callbacks needed to allow the Connection
 * asynchronous usage of the transport.
 */
typedef struct RtspTransport_
{
	RvLogMgr						*pLogMgr;			/* log manager					*/
	RvLogSource						*pLogSrc;			/* log source					*/
	RvSelectEngine					*pSelectEngine;		/* pointer to the				*/
														/* module's Select Engine		*/
	RtspTransportCallbackFunctions	pfnCallbacks;		/* on-events callback functions	*/

    RvSocket						socket;				/* The TCP connection socket	*/
    RvSelectFd						socketFd;			/* FD to wrap the socket		*/
                                                        /* for the select engine		*/

    void							*context;			/* private data for the 		*/
														/* call backs 					*/
	HRPOOL							hRPool;				/* memory container for 		*/
														/* the transport 				*/

	RvMutex							mutex;				/* guard 						*/
	RvMutex							*pGMutex;			/* global guard against			*/
														/*destruct/construct overlapping*/
														/* with other functions			*/
	RtspTransportMessage			rxMessage;			/* for received message 		*/
	HRPOOLELEM						rxLine;				/* utility line for parsing the	*/
														/* messages						*/
	RvSize_t						rxMessageBodyLength;/* body length of message being	*/
														/* received 					*/
	RvSize_t						rxMessageBodyRead;	/* legth of body already read	*/
	RvUint8							rxBuffer[RTSP_TRANSPORT_RECEIVE_BUFFER_SIZE];/*		*/
														/* socket reading buffer		*/
	RvSize_t						rxBufferLength;		/* size of buffer used			*/
	RvInt32			    			rxBufferRead;		/* index to read from			*/
	RvBool							rxSkipEol;			/* flag for skipping end of line*/
														/* in the next buffer			*/
	RvQueue							txQueue;			/* queue buffer for sending		*/
	HRPOOLELEM 						txElement;			/* Element being transmitted	*/
	RvSize_t						txElementOffset;	/* offset from start of buffer	*/
														/* to be sent					*/
	RvUint8	 						*pTxChunk;			/* real pointer to contigous	*/
														/* txElement buf  				*/
	RvInt32 						txChunkLength;		/* length of TxChunk used 		*/
	RvSize_t						txChunkSent;		/*length of TxChunk already sent*/

	RtspTransportState				transportState;		/* state of the object			*/
	RtspTransportTxState			txState;			/* sending state				*/
	RtspTransportRxState			rxState;			/* reception state				*/
} RtspTransport;



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * RtspTransportAddUpdate
 * ------------------------------------------------------------------------
 * General: The method Adds and updates the select engine for the provided
 *          transport
 *
 * Return Value:    None.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT    :    transport         - the new connection's transport
 * OUTPUT   :    None.
 * INOUT    :    None.
 *************************************************************************/
RVAPI 
RvStatus RtspTransportAddUpdateEvents(
                  IN RtspTransport    *transport,
                  IN RvSelectEvents   selectEvents);


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
			IN	RvUint16		maxHeadersInMessage);

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
		    IN	RtspTransport	*pThis);



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
		IN	const RtspTransportCallbackFunctions	*pCallbacks);

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
			IN	const RvAddress		*sourceAddress);

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
			IN	RtspTransport	*pThis);

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
 *				hBuffer	- buffer containing the message to send.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVINTAPI RvStatus RVCALLCONV RtspTransportSend(
			IN	RtspTransport	*pThis,
			IN	HRPOOLELEM		hBuffer);

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
           IN RtspTransport     *pThis,
           IN const RvChar      *localAddr,
           IN const RvUint16    listeningPort);
#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

/************************************************************************
 * RtspTransportReceive
 * ------------------------------------------------------------------------
 * General: This function handles the reception of data from the application.
 *
 * Return Value:	None.
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
            IN  RvUint32        buffSize);


#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_TRANSPORT_H*/

