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
 *                     <RtspServerConnectionInternal.h>
 *
 * Internal header - used inside the RvRtsp library.
 *
 * This file contains definitions relevant to the RTSP server connections.
 * An RTSP Connection instance is a thread safe representation of a connection
 * to an RTSP server, handling all RTSP communication to and from the server.
 *
 ******************************************************************************/


#ifndef _RV_RTSP_SERVER_CONNECTION_INTERNAL_H
#define _RV_RTSP_SERVER_CONNECTION_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*                            INCLUDE HEADER FILES                            */
/*----------------------------------------------------------------------------*/
#include "rvtypes.h"
#include "rvccore.h"
#include "rvaddress.h"
#include "rvsocket.h"
#include "rvmutex.h"
#include "ares.h"
#include "rpool.h"
#include "ra.h"

#include "RvRtspServerTypes.h"
#include "RvRtspServerConnection.h"
#include "RtspTransport.h"

/*----------------------------------------------------------------------------*/
/*                             TYPE DEFINITIONS                               */
/*----------------------------------------------------------------------------*/

#define MAX_SESSIONID_LENGTH   32 /* Session Id Length */

/*
 * RtspServerConnectionState
 * ----------------------------------------------------------------------------
 * This type specifies the state of the connection object
 */
typedef enum
{
    RTSP_CONNECTION_STATE_CONSTRUCTED = 0,  
        /* constructed                   */

    RTSP_CONNECTION_STATE_CONNECTED,        
        /* connected                     */

    RTSP_CONNECTION_STATE_DESTRUCTED        
        /* object destructed             */

} RtspServerConnectionState;


/*
 *  RtspServerConnection
 * ----------------------------------------------------------------------------
 * The RTSP Server Connection structure holds all data relevant to the 
 * RTSP server.
 */
typedef struct RtspServerConnection_
{
    RvChar                      sourceIPAddress[RV_RTSP_IP_ADDRESS_MAX_LENGTH]; 
        /* Client IP */

    RvBool                      sourceIPAddressValid; 
        /* Is the source address valid */

    RvUint16                    clientPort;       
        /* Client port */

    RvLogMgr                    *pLogMgr;          
        /* Log manager */

    RvLogSource                 *pLogSrc;       
        /* Log source */
    
    RvRtspHandle                hRtsp;
        /* The Rtsp object handle */

    RvRtspServerConnectionAppHandle    hApp;   
        /* Context for the callback functions */

    RvSelectEngine              *pSelectEngine; 
        /* Select engine for callback functions */

    HRPOOL                      hRPool;              
        /* The module's memory pool */

    RvMutex                     mutex;              
        /* Guard against collisions */

    RvMutex                     *pGMutex;           
        /* Global guard against destruct construct overlapping */

    RvRtspServerConnectionCallbackFunctions    pfnCallbacks;   
        /* On event callback functions  */

    RvBool                      destructed;          
        /* connection exists/destructed */

    RtspServerConnectionState   state;    
        /* The connection's current state */

    RvRtspServerConnectionConfiguration        configuration;
        /* The connection's configuration */

    RtspTransport               transport;    
        /* The connection's transport */

    RvUint16    maxSessionsPerConnection;
        /* Maximum number of Sessions per connection */

    RvRtspServerSessionHandle   hSessionList;
        /* Linked list of session handles */

    HRA                         hRaSessions;    
        /* Sessions array */

    HRA                         hRaRequests;    
        /* RA of RtspMsgRequests to be used when using RtspMsg module */

    HRA                         hRaResponses;   
        /* RA of RtspMsgResponses to be used when using RtspMsg module */

    RvUint16                    sessionCount;
        /* number of sessions in that connection */

    RvUint16                    referenceCount;    
        /* number of references to the connection: number of times 
           construct was called on it */

} RtspServerConnection;


/*----------------------------------------------------------------------------*/
/*                              FUNCTIONS HEADERS                             */
/*----------------------------------------------------------------------------*/


/******************************************************************************
 * RtspServerConnectionConstruct
 * ----------------------------------------------------------------------------
 * General: 
 *  This method constructs a connection, it is called after 
 *  setting the connection parameters in the connection
 *  structure.
 *
 * Arguments:
 * Input:   pThis       - The connection to be constructed.
 *          reAttachConnection  - If RV_TRUE - A broken connection is re-established 
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 ******************************************************************************/
RvStatus RtspServerConnectionConstruct(
    IN  RtspServerConnection  *pThis,
    IN  RvBool                reAttachConnection);



/*******************************************************************************
 * RtspServerConnectionDestruct
 * ----------------------------------------------------------------------------
 * General: 
 *  This method de-initializes the connection and releases it's
 *  resources. This includes destructing the connection's sessions.
 *  Note: each time RvRtspServerSessionDestruct is called on the
 *  connection, it's reference count decreases, when a destruct is
 *  called on the last reference, the connection is destructed.
 *  if destructAllReferences is set, the connection is destructed
 *  immediately.
 *
 * Arguments:
 * Input:   hConnection             - The connection to be destructed .
 *          destructAllReferences   - Destruct even if connection count
 *                                    indicates other references.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *******************************************************************************/
RvStatus RtspServerConnectionDestruct(
    IN  RvRtspServerConnectionHandle  hConnection,
    IN  RvBool                        destructAllReferences);

/**************************************************************************
 * RtspServerConnectionStartListening
 * ------------------------------------------------------------------------
 * General: 
 *  This method listens on the port for new connections.
 *
 * Arguments:
 * Input:   pThis               - The  listening connection.
 *          listeningIPAddress  - The IP address on which the Server 
 *                                listens for clients connections.
 *          listeningPort       - Port number.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerConnectionStartListening(
                IN  RtspServerConnection  *pThis,
                IN  const RvChar          *listeningIPAddress,
                IN  const RvUint16        listeningPort);

/**************************************************************************
 * RtspServerConnectionSendResponse
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends an RvRtspResponse message on the connection.
 *  (this is an internal function, to be used by the connection,
 *  and by sessions).
 *
 * Arguments:
 * Input:   pThis       - The connection on which to send the response.
 *          pResponse   - The response message to be sent on the connection.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerConnectionSendResponse(
    IN  RtspServerConnection      *pThis,
    IN  const RvRtspResponse      *pResponse);

/**************************************************************************
 * RtspServerConnectionSendRequest
 * ------------------------------------------------------------------------
 * General: 
 *  This method sends an RvRtspRequest message on the connection.
 *  (this is an internal function, to be used by the connection,
 *  and by sessions).
 *
 * Arguments:
 * Input:   pThis       - The connection on which to send the request.
 *          pRequest    - The request message to be sent on the connection.
 * Output:  None.
 *
 * Return Value:  RV_OK - if successful.
 *                Negative values otherwise.
 *************************************************************************/
RvStatus RtspServerConnectionSendRequest(
    IN  RtspServerConnection      *pThis,
    IN  const RvRtspRequest       *pRequest);


#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SERVER_CONNECTION_INTERNAL_H*/

