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
 *                              <RvRtspClient.h>
 *
 * Interface functions for the entire RTSP library. The RvRtsp module is responsible
 * for initializing and de-initializing the library, and to holding the RTSP sessions
 * and connections.
 *
 *    Author                         Date
 *    ------                        ------
 *		Tom							8/1/04
 *
 *********************************************************************************/

#ifndef _RV_RTSP_CLIENT_H
#define _RV_RTSP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER)
#include "rvtypes.h"

#include "RvRtspClientTypes.h"
#include "RvRtspClientConnection.h"

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/


/* RvRtspConfiguration
 * ----------
 *	The RTSP module configuration structure, contains parameters needed for the
 *	initialization of the module.
 */
typedef struct RvRtspConfiguration
{
	RvUint16	maxConnections;			    /* Max number of servers connected in the module*/
	RvUint16	memoryElementsSize;		    /* Size of the memory pool elements				*/
	RvUint16	memoryElementsNumber;	    /* Number of the memory pool elements			*/
	RvChar		strDnsAddress[RV_RTSP_IP_ADDRESS_MAX_LENGTH];   /* The DNS IP address		*/
    RvUint16    msgRequestElementsNumber;   /* The number of elements in the hRaRequests RA */
    RvUint16    msgResponseElementsNumber;  /* The number of elements in the hRaResponses RA */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvUint16    maxRtspMsgHeadersInMessage; /* The max number of headers allowed in a message.
                                               This configuration parameter is used to determine 
                                               the size of the RtspMsgHeader pool*/
#endif
}RvRtspConfiguration;



/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/**************************************************************************
 * RvRtspInit
 * ------------------------------------------------------------------------
 * General: This method initiailzes the RvRtsp module.
 *			This method should be called before any classes or methods
 *			of the library are used. It should only be called once.
 *
 * 			Note: The RvRtspMainLoop function must be called by the same 
 *          thread that called RvRtspInit function.
 *
 * Return Value:	RV_OK - When the RTSP library is successfully initialized
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hLog			    - The RTSP module log handle, NULL if
 *                                    one should be created.
 *				pConfiguration	    - configuration for the module.
 *              configurationSize   - size of configuration structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 * OUTPUT	:	phRtsp			    - handle to the Rtsp main structure.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspInit(
						IN 	RvLogHandle					hLog,
						IN	const RvRtspConfiguration	*pConfiguration,
                        IN	const RvSize_t              configurationSize,
                        OUT	RvRtspHandle				*phRtsp);



/**************************************************************************
 * RvRtspEnd
 * ------------------------------------------------------------------------
 * General: This method de-initiailzes the RvRtsp module.
 * 			This method should be called when the RvRtsp module is no longer
 *			needed. After this call is made none of the classes or methods
 *			of this module should be used.
 *
 * Return Value:	RV_OK - If the RTSP library is successfully uninitialized
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRtsp  - handle to the Rtsp main structure.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspEnd(
						IN RvRtspHandle	hRtsp);


/**************************************************************************
 * RvRtspMainLoop
 * ------------------------------------------------------------------------
 * General: This method runs in the same user thread that called the RTSP
 *			module's Init, it runs a loop waiting for events to dispatch
 *			callback functions for.
 *          The method returns to the user after 100 miliseconds and allows 
 *          the user to perform user specific functionality before calling
 *          the method again.
 *
 * Return Value:	RV_OK - If the RTSP library is successfully uninitialized
 *                  RV_ERROR_UNINITIAIZED - when the stack is successfuly 
 *                  uninitialized. The error is returned only when the user
 *                  signaled the stack to terminate itself by calling RvRtspEnd.
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRtsp  - handle to the Rtsp main structure.
 *				timeout (milliseconds) - how much time to wait on a selectEngine Event.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspMainLoop(
						IN RvRtspHandle	hRtsp,
						IN RvUint32		timeOut);



/**************************************************************************
 * RvRtspConnectionConstruct
 * ------------------------------------------------------------------------
 * General: This method constructs a connection in hRtsp, calling the
 *			actual construct and adding the connection to the Rtsp
 *			module's connections.
 *
 * Return Value:	RV_OK if the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRtsp				- Handle to the Rtsp main structure.
 *				hApp				- application context handle.
 *				strServerURI		- The URI to connect to.
 *				sourceIPAddress		- The local IP address.
 *				pConfiguration		- The connection configuration.
 *              configurationSize   - size of configuration structure,
 *                                    is passed in order to distinguish between
 *                                    different versions of the RTSP Stack.
 * OUTPUT	:	phConnection		- Handle to the constructed connection.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionConstruct(
						IN	RvRtspHandle						hRtsp,
						IN	RvRtspConnectionAppHandle			hApp,
						IN	const RvChar						*strServerURI,
						IN	const RvChar						*sourceIPAddress,
						IN	const RvRtspConnectionConfiguration	*pConfiguration,
                        IN	const RvSize_t                      configurationSize,
                        OUT	RvRtspConnectionHandle				*phConnection);


/**************************************************************************
 * RvRtspConnectionDestruct
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
 * INPUT	:	hRtsp				    - Handle to the Rtsp main structure.
 *          :	hConnection				- the destructed connection.
 *				destructAllReferences	- destruct even if connection count
 *										  indicates other references.
 * OUTPUT	:	None.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionDestruct(
                        IN	RvRtspHandle			hRtsp,
                        IN	RvRtspConnectionHandle	hConnection,
						IN	RvBool					destructAllReferences);




/**************************************************************************
 * RvRtspGetConnectionByURI
 * ------------------------------------------------------------------------
 * General: This method returns the connection corresponding to the
 *			specified URI.
 *			Note: this method goes over the connections array sequentialy
 *			in order to find the connection, it is therefore not time
 *			efficient (O(n)) and thus not recommended as the normal way
 *			of accessing the connections.
 *
 * Return Value:	RV_OK if the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRtsp			- handle to the Rtsp main structure.
 *				strUri			- the connection's URI.
 * OUTPUT	:	phConnection	- the connection with the specified URI
 * 								or NULL if not found.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspGetConnectionByURI(
						IN	RvRtspHandle			hRtsp,
						IN	const RvChar			*strURI,
						OUT	RvRtspConnectionHandle	*phConnection);


              
/**************************************************************************
 * RvRtspGetSessionById
 * ------------------------------------------------------------------------
 * General: This method returns the session corresponding to the specified
 *			Session-Id.
 *			Note: this method goes over the sessions sequentialy in order
 *			to find the session, it is therefore not time efficient (O(n))
 *			and thus not recommended as the normal way of accessing the
 *			sessions.
 *
 * Return Value:	RV_OK if the function is successfully completed,
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRtsp			- handle to the Rtsp main structure.
 *				strSessionId	- the session's Session-Id.
 * OUTPUT	:	phSession		- the session with the specified
 * 								Session-Id, or NULL if not found.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
RvStatus RVCALLCONV RvRtspGetSessionById(
						IN	RvRtspHandle			hRtsp,
						IN	const RvChar			*strSessionId,
						OUT	RvRtspSessionHandle		*phSession);



/**************************************************************************
 * RvRtspGetSoftwareVersion
 * ------------------------------------------------------------------------
 * General: This method gets the version information of the RvRtp module.
 *
 * Return Value:	RV_OK - If successfull
 *					negative value is returned when encountering an error.
 * ------------------------------------------------------------------------
 * Arguments:
 * INPUT	:	hRtsp				- handle to the RTSP module.
 * OUTPUT	:	pMajorVersion		- The module's major version number.
 *				pMinorVersion		- The module's minor version number.
 *				pEngineeringRelease	- The module's engineering release number.
 *				pPatchLevel   		- The modules patch level.
 * INOUT	:	None.
 *************************************************************************/
RVAPI
void RVCALLCONV RvRtspGetSoftwareVersion(
					IN	RvRtspHandle	hRtsp,
					OUT RvUint8 		*pMajorVersion,
					OUT RvUint8 		*pMinorVersion,
					OUT RvUint8 		*pEngineeringRelease,
					OUT RvUint8 		*pPatchLevel);
/***********************add by kongfd*****************/
RVAPI
RvStatus RVCALLCONV RvRtspConnectionSetUser(
                        IN  RvRtspHandle            hRtsp,
                        IN  RvRtspConnectionHandle  hConnection,
                        IN  RvChar *                  username,
                        IN  RvChar *                  password);

char * DataToBase64( RvChar** buf,RvChar *Data,int DataLen);
/****************add end ****************/
#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_SERVER) */

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_H*/


