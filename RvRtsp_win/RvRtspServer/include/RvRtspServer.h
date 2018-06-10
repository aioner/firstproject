/*
 ********************************************************************************
 *                                                                              *
 * NOTICE:                                                                      *
 * This document contains information that is confidential and proprietary to   *
 * RADVision LTD.. No part of this publication may be reproduced in any form    *
 * whatsoever without written prior approval by RADVision LTD..                 *
 *                                                                              *
 * RADVision LTD. reserves the right to revise this publication and make changes*
 * without obligation to notify any person of such revisions or changes.        *
 ********************************************************************************
*/

/*******************************************************************************
 *                              <RvRtspServer.h>                                
 *                                                                              
 * Interface functions for the entire RTSP library. The RvRtsp module is  
 * responsible for initializing and de-initializing the library, and for holding
 * the RTSP sessions and connections on the Server.                                          
 *******************************************************************************/

#ifndef _RV_RTSP_SERVER_H
#define _RV_RTSP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "RvRtspUsrConfig.h"
#if (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT)
#include "rvtypes.h"

#include "RvRtspServerTypes.h"
#include "RvRtspServerConnection.h"

/*@****************************************************************************
 * Module: RtspServer (root)
 * ----------------------------------------------------------------------------
 * Title:  3.RTSP Server API Functions
 *
 * The RTSP Server functions are interface functions for the entire RTSP
 * library. The RTSP Server Toolkit is responsible for initializing and
 * de-initializing the library, and for holding the RTSP sessions and
 * connections.
 *
 * You can find the RTSP Server functions in the RvRtspServer.h file.
 ***************************************************************************@*/


/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/*@****************************************************************************
 * Type: RvRtspServerConfiguration (RtspServer)
 * ---------------------------------------------------------------------
 * The RTSP Server module configuration structure. Contains
 * information about the initialization of the module.
 ***************************************************************************@*/
typedef struct RvRtspServerConfiguration
{
    RvUint16    maxListeningPorts;
        /* Maximum number of listening ports on which the Server waits for clients. */

    RvUint16    maxConnections;
        /* Maximum number of Client connections. */

    RvUint16    maxSessionsPerConnection;
        /* Maximum number of sessions per connection. */

    RvUint16    maxRequests;
        /* Maximum number of requests. */

    RvUint16    maxResponses;
        /* Maximum number of responses. */

    RvUint16    memoryElementsSize;
        /* Size of memory pool elements. */

    RvUint16    memoryElementsNumber;
        /* Number of memory pool elements. */
#if (RV_RTSP_USE_RTSP_MSG == RV_YES)
    RvUint16    maxRtspMsgHeadersInMessage; 
        /* The maximum number of headers allowed in a message.
           This configuration parameter is used to determine 
           the size of the RtspMsgHeader pool. */
#endif
   RvLogMessageType   messageMask;
        /* Indicates the type of message to be logged. */
}   RvRtspServerConfiguration;


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
/*-----------------------------------------------------------------------*/


/*@**************************************************************************
 * RvRtspServerInit (RtspServer)
 * ------------------------------------------------------------------------
 * General:
 *  This API initializes the RTSP Server Toolkit. The API constructs the 
 *  memory RPool and the Select Engine. It creates the log manager and 
 *  global and guard mutexes. It also initializes the listening port, 
 *  connection, session, request message, and response message structures.
 *  This API must be called before any APIs or functions of the 
 *  library are used. It should be called only once.                     
 * 
 * Arguments:                                                           
 * Input:  hLog              - The RTSP Server Toolkit log handle. 
 *                             NULL if one should be created.    
 *         pConfiguration    - Configuration of the RTSP Server Toolkit. Must  
 *                             be populated correctly by the application.               
 *         configurationSize - Size of the configuration structure,    
 *                             passed to distinguish between the different   
 *                             versions of the RTSP Stack.   
 * Output: phRtsp            - Handle to the RTSP main structure. 
 *         
 * Return Value: RV_OK if the RTSP Library is initialized successfully. 
 *               Negative values otherwise.            
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerInit(
    IN RvLogHandle                     hLog,
    IN const RvRtspServerConfiguration *pConfiguration,
    IN const RvSize_t                  configurationSize,
    OUT RvRtspHandle                   *phRtsp);


/*@**************************************************************************
 * RvRtspServerEnd (RtspServer)
 * ------------------------------------------------------------------------
 * General:
 *  This API deinitializes the RTSP Server Toolkit by turning the 
 *  doTerminate flag to True. This API should be called when 
 *  the RTSP Server Toolkit is no longer needed. After this call 
 *  is made, none of the APIs or functions of the RTSP Server Toolkit 
 *  should be used. 
 * 
 * Arguments:                                                           
 * Input:  hRtsp     - Handle to the RTSP main structure used
 *                     to access the RTSP Server Toolkit. 
 *                 
 * Output: None.    
 *                                                       
 * Return Value: RV_OK if the RTSP Library is uninitialized successfully. 
 *               Negative values otherwise.        
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerEnd(
    IN RvRtspHandle hRtsp);

/*@**************************************************************************
 * RvRtspServerMainLoop (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API runs in the same user thread that called the Init of the  
 *  RTSP Server module. It runs a loop waiting for events for which to  
 *  dispatch callback functions. The API returns to the user after 'timeout' 
 *  milliseconds and allows the user to perform user-specific 
 *  functionality before calling the API again. This API also destructs
 *  all resources allocated for the Stack on termination. Termination is 
 *  triggered by calling the RvRtspServerEnd() API. 
 *                                                        
 * Arguments:                                                           
 * Input:  hRtsp                 - Handle to the RTSP main structure.     
 *         timeout(milliseconds) - Time to wait for a selectEngine event.
 *                                 The select engine waits for a select
 *                                 event for 'timeout' milliseconds, in 
 *                                 the absence of which it returns to the 
 *                                 user.        
 * Output: None.                                                             
 *        
 * Return Value:
 *            RV_OK if the RTSP Library is uninitialized successfully. 
 *            RV_ERROR_UNINITIALIZED when the Stack is
 *            uninitialized successfully. The error is returned only when 
 *            the user signaled the Stack to terminate itself by calling 
 *            RvRtspServerEnd.
 *            Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerMainLoop(
    IN RvRtspHandle hRtsp,
    IN RvUint32     timeOut);

/*@**************************************************************************
 * RvRtspServerStartListening (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API is called once the Server is configured and initialized
 *  successfully. This API constructs the listening socket 
 *  and listens for incoming Client connections on the listening sockets
 *  created. This API should be called only after calling the RvRtspInit()
 *  API.
 * Arguments:                                                           
 * Input:   hRtsp              - Handle to the RTSP main structure.
 *          hApp               - Application context used to access
 *                               the connection and the session on the 
 *                               application.
 *          listeningIPAddress - The IP address on which the Server 
 *                               listens for Client connections. The 
 *                               application is required to provide this
 *                               address to connect to the Server.
 *          pConfiguration     - The connection configuration that must
 *                               be populated in the Server application.
 *          configurationSize  - The size of the configuration structure.
 *                               It is used to identify between different
 *                               versions of the RTSP Stack.
 * Output:  phConnection       - Handle to the listening connection. 
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerStartListening(
    IN RvRtspHandle                          hRtsp,
    IN RvRtspServerConnectionAppHandle       hApp,
    IN const RvChar                          *listeningIPAddress,
    IN const RvRtspServerListenConfiguration *pConfiguration,
    IN const RvSize_t                        configurationSize,
    OUT RvRtspServerConnectionHandle         *phConnection);

/*@**************************************************************************
 * RvRtspServerStopListening (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API is called by the application when the Server is 
 *  deinitialized. It destructs the connection and then deletes
 *  the listening ports from the RA. 
 *
 * Arguments:                                                           
 * Input:    hConnection   -  Handle to the listening connection for 
 *                            accessing the connection to be destructed.
 * Output:   None.
 *
 * Return Value:  RV_OK if successful.
 *                Negative values otherwise.
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerStopListening(
    IN RvRtspServerConnectionHandle         hConnection);


/*@**************************************************************************
 * RvRtspServerConnectionConstruct (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API constructs an RTSP connection in hRtsp for calling the actual 
 *  construct and adding the connection to the connections of the RTSP module.
 *  This API is called only when the TCP connection request has been 
 *  accepted by the Server. It is called after the 
 *  RvRtspServerConnectionOnAcceptEv() event has been generated.
 * 
 * Arguments:                                                           
 * Input: hRtsp              - Handle to the RTSP main structure.
 *        hApp               - The application context handle used to 
 *                             access the RTSP connection.
 *        socket             - The newly-created socket on which the 
 *                             Server listens for incoming requests.
 *        clientIPAddress    - The Client's IP address required
 *                             to create the connection.
 *        clientPort         - The Client's port required to create the 
 *                             connection.
 *        pConfiguration     - Configuration for the connection to be 
 *                             populated by the application.
 *        configurationSize  - Size of the callbacks structure,
 *                             passed to distinguish between
 *                             different versions of the RTSP Stack.
 * Output: phConnection      - Handle to the constructed connection. This 
 *                             handle is used to access the newly-created
 *                             connection.
 *      
 * Return Value: RV_OK if successful.                                     
 *               Negative values otherwise.            
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionConstruct(
    IN RvRtspHandle                        hRtsp,
    IN RvRtspServerConnectionAppHandle     hApp,
    IN RvSocket                            newSocket,
    IN const RvChar                        *clientIPAddress,
    IN RvUint16                            clientPort,
    IN const RvRtspServerConnectionConfiguration *pConfiguration,
    IN const RvSize_t                     configurationSize,
    OUT RvRtspServerConnectionHandle       *phConnection);

/*@**************************************************************************
 * RvRtspServerConnectionDestruct (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API deinitializes a connection and releases its resources. 
 *  This API in effect also destroys all sessions in the connection.
 *
 * Arguments:                                                           
 * Input:  hConnection           - Handle to the destructed connection.  
 *         destructAllReferences - Destruct even if the connection reference
 *                                 count is not zero.                      
 * Output: None.                                                              
 *                                                                 
 * Return Value: RV_OK on successful disconnection.                      
 *               Negative values otherwise.            
 ***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvRtspServerConnectionDestruct(
    IN RvRtspServerConnectionHandle hConnection,
    IN RvBool                       destructAllReferences);

/*@**************************************************************************
 * RvRtspServerGetConnectionByIPPort (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API returns the connection corresponding to the 
 *  Client's IP and port number. It identifies the connection on the
 *  given IP and port number and returns a handle to the same.
 *
 * Arguments:                                                           
 * Input:  hRtsp         - Handle to the RTSP main structure.     
 *         strClientIP   - The Client's IP address.               
 *         ClientPort    - The Client's port number.
 * Output: phConnection  - The connection with the specified IP 
 *                         address, or NULL if not found. 
 *
 * Return Value: RV_OK if the function is completed successfully. 
 *               Negative values otherwise.                       
 ***************************************************************************@*/
RVAPI RvStatus RvRtspServerGetConnectionByIPPort(
    IN  RvRtspHandle                 hRtsp,
    IN  const RvChar                 *strClientIP,
    IN  RvUint16                     clientPort,
    OUT RvRtspServerConnectionHandle *phConnection);

/*@**************************************************************************
 * RvRtspServerGetSoftwareVersion (RtspServer)
 * ------------------------------------------------------------------------
 * General: 
 *  This API retrieves the version information of the RTSP Server Toolkit 
 *  version.                                                     
 *
 * Arguments:                                                           
 * Input:  hRtsp               - Handle to the RTSP main structure.     
 * Output: pMajorVersion       - The module's major version number.     
 *         pMinorVersion       - The module's minor version number.     
 *         pEngineeringRelease - The module's engineering release number.
 *         pPatchLevel         - The module's patch level.  
 *            
 * Return Value: RV_OK if successful.                                   
 *               Negative values otherwise.            
 ***************************************************************************@*/
RVAPI void RVCALLCONV RvRtspServerGetSoftwareVersion(
    IN RvRtspHandle hRtsp,
    OUT RvUint8     *pMajorVersion,
    OUT RvUint8     *pMinorVersion,
    OUT RvUint8     *pEngineeringRelease,
    OUT RvUint8     *pPatchLevel);

#endif /* (RV_RTSP_TYPE != RV_RTSP_TYPE_CLIENT) */

#ifdef __cplusplus
}
#endif

#endif  /*END OF: define _RV_RTSP_SERVER_H*/


