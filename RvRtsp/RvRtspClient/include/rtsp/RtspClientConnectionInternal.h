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
 *                              <RtspClientConnectionInternal.h>
 *
 * Internal header - used inside the RvRtsp library.
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

#ifndef _RV_RTSP_CONNECTION_INTERNAL_H
#define _RV_RTSP_CONNECTION_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvccore.h"
#include "rvaddress.h"
#include "rvsocket.h"
#include "rvmutex.h"
#include "ares.h"
#include "rpool.h"
#include "ra.h"


#include "RvRtspClientTypes.h"
#include "RvRtspClientConnection.h"
#include "RtspTransport.h"

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* RtspConnectionState
 * ----------
 * This type specifies the state of the connection object
 */
typedef enum
{
	RTSP_CONNECTION_STATE_CONSTRUCTED = 0,	/* constructed							*/
	RTSP_CONNECTION_STATE_DNS_QUERY,		/* before connecting - call dns engine	*/
	RTSP_CONNECTION_STATE_CONNECTING,		/* - call transport connect				*/
	RTSP_CONNECTION_STATE_CONNECTED,		/* transport returned OnConnect			*/
	RTSP_CONNECTION_STATE_DISCONNECTING,	/* - call sessions to disconnect, 		*/
											/* - wait, session are disconnected		*/
	RTSP_CONNECTION_STATE_DESTRUCTED		/* object destructed					*/
} RtspConnectionState;

/* RtspDescribeTimeoutContext
 * ----------
 * This structure specifies the context for the describe responseTimer.
 */
typedef struct  {
    void*          connection;
    RvUint32       location;
}RtspDescribeTimeoutContext;

/* RtspWaitingDescribeRequest
 * ----------
 * This structure specifies the configuration for the connection object.
 */
typedef struct RtspWaitingDescribeRequest_
{
	RvUint16							cSeq;		        /* The Describe request CSeq			*/
	HRPOOLELEM							hURI;		        /* The Describe request URI				*/
	RvRtspConnectionDescribeAppHandle	hDescribe;	        /* Context for the callback functions	*/
    RvTimer				                responseTimer;		/* timer for requests timeout		    */
	RvBool					            responseTimerSet;	/* is the response timer set		    */
    RtspDescribeTimeoutContext          timeoutContext;     /* The context for the timer            */
} RtspWaitingDescribeRequest;
/*add by kongfd**/
#define MAX_AUTHCHAR_LEN 64
typedef struct Authenticate_
{
	RvBool  isUser;
	RvChar username[MAX_AUTHCHAR_LEN];
	RvChar password[MAX_AUTHCHAR_LEN];
	RvBool  isDigest ;
	RvChar Digestrealm[MAX_AUTHCHAR_LEN];
	RvChar Digestnonce[MAX_AUTHCHAR_LEN];
	RvBool  stale;
	RvBool  isBasic;
	RvChar Basicrealm[MAX_AUTHCHAR_LEN];
} RtspAuthenticate;
/*add end*/

/* RvRtspConnection
 * ----------
 * The RTSP Server structure holds all data relevant to a specific RTSP server.
 */
typedef struct RtspConnection_
{
	RvAddress				serverAddress;			/* The server's address for GetByAddress*/
	RvChar					strServerName[RV_RTSP_SERVER_NAME_MAX_LENGTH];	/* Server name	*/
	RvUint16				serverPort;				/* Server port							*/
	RvChar					sourceIPAddress[RV_RTSP_IP_ADDRESS_MAX_LENGTH];	/* Server IP	*/
	RvBool					sourceIPAddressValid;	/* Is the source address valid			*/

	RvLogMgr				*pLogMgr;				/* Log manager							*/
	RvLogSource				*pLogSrc;				/* Log source							*/
	RvRtspConnectionAppHandle hApp;					/* Context for the callback functions	*/
	RvSelectEngine			*pSelectEngine;			/* Select engine for callback functions	*/
	RvDnsEngine     		*pDnsEngine;			/* Dns engine for domain name resolving	*/
	HRA						hDNSResults;			/* RA for IP address result from the DNS*/

    RvRtspHandle            hRtsp;                  /* The Rtsp object handle               */
	HRPOOL					hRPool;					/* The module's memory pool				*/
    HRA                     hRaRequests;            /* The RtspMsgRequests RA on the rtsp object */
    HRA                     hRaResponses;           /* The RtspMsgResponses RA on the rtsp object */

	RvMutex					mutex;					/* Guard against collisions				*/
	RvMutex					*pGMutex;				/* Global guard against destruct/		*/
													/* construct overlapping 				*/
    RvBool                  destructed;

	RtspConnectionState		state;					/* The connection's current state		*/
	RvRtspConnectionConfiguration configuration;	/* The connection's configuration		*/

	RtspTransport			transport;				/* The connection's transport 			*/

	HRA						hSessions;  			/* Sessions array						*/
	HRA						hWaitingDescribeRequests;/* Sessions array						*/
	RvUint16				nextCSeq;				/* The next CSeq to use for the server	*/

	RvUint16				referenceCount;			/* number of references to the			*/
													/* connection (number of times 			*/
													/* construct was called on it			*/
	RvRtspConnectionCallbackFunctions pfnCallbacks;

	void*					pIpv4DNSMem;
	void*					pIpv6DNSMem;

	RvUint32				queryIdIpv4;
	RvUint32				queryIdIpv6;
	/*add by kongfd*/
	RtspAuthenticate		Authenticate;
	/*add end*/

} RtspConnection;



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/

/**************************************************************************
 * RtspConnectionConstruct
 * ------------------------------------------------------------------------
 * General: This method constructs a connection, it is called by the Rtsp
 *			layer after setting the connection parameters in the connection
 *			structure.
 *			Note: The first time RtspConnectionConstruct is called, the
 *			connection is constructed, each additional time the function is
 *			called, only it's reference count is increased.
 *
 * Return Value:	RV_OK - If the connection is successfully initialized
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- The connection to be constructed.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RtspConnectionConstruct(
			IN	RtspConnection	*pThis);


/**************************************************************************
 * RtspConnectionDestruct
 * ------------------------------------------------------------------------
 * General: This method de-initializes the connection and releases it's
 * 			resources. This	includes destructing the connection's sessions.
 *			Note: each time RvRtspConnectionDestruct is called on the
 *			connection, it's reference count decreases, when a destruct is
 *			called on the last reference, the connection is destructed.
 *			if destructAllReferences is set, the connection is destructed
 *			immediately.
 *
 * Return Value:	RV_OK - If the connection is successfully de-initialized
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hConnection				- the destructed connection.
 *				destructAllReferences	- destruct even if connection count
 *										  indicates other references.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RtspConnectionDestruct(
			IN	RvRtspConnectionHandle	hConnection,
			IN	RvBool					destructAllReferences);




/**************************************************************************
 * RtspConnectionSendRequest
 * ------------------------------------------------------------------------
 * General: This method sends an RvRtspRequest message on the connection.
 *			(this is an internal function, to be used by the connection,
 *			and by sessions).
 *
 * Return Value:	RV_OK - If the connection is successfully de-initialized
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis		- the connection on which to send the request.
 *				pRequest	- The message to send on the connection.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RtspConnectionSendRequest(
			IN	RtspConnection		*pThis,
			IN	const RvRtspRequest	*pRequest);



/**************************************************************************
 * RtspConnectionOnDnsResponse
 * ------------------------------------------------------------------------
 * General: This function gets the dns response, and inserts it into the
 *			DNS	results Array, if we are not already trying to connect, the
 *			function also tries to connect to the server.
 *
 * Return Value:	RV_OK if the function completed successfully, negative
 *					value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	pThis			- The connection to which the address belongs.
 *				ipAddress		- The connection's IP address.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RvStatus RtspConnectionOnDnsResponse(
			IN	RtspConnection		*pThis,
			IN	const RvAddress		*ipAddress);


#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_CONNECTION_INTERNAL_H*/


