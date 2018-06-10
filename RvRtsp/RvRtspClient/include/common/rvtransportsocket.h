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
 *                              rvtransport.c
 *
 *    Author                        Date
 *    ------                        ------------
 *                                  01-July-2009
 ******************************************************************************/


/*@*****************************************************************************
 * Module: CcTransportSocket (root)
 * ----------------------------------------------------------------------------
 * CcTransportSocket Module
 *
 * The Common Core SocketTransport module provides the user with
 * the implementation of the Socket-style Transport.
 * The Socket-style transport simply wraps the UDP/TCP socket.
 * The transport object can be created
 * using the RvTransportCreateSocketTransport API function, which wraps
 * the generic RvTransportCreate API function.
 ****************************************************************************@*/

#ifndef _RV_TRANSPORTSOCKET_H
#define _RV_TRANSPORTSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                                */
/*----------------------------------------------------------------------------*/
#include "rvtransport.h"
#include "rvlog.h"
#include "rvselect.h"

#if (RV_NET_TYPE != RV_NET_NONE)


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                          INTERNAL API                                      */
/*                                                                            */
/* I put the internal API here and not in rvtransport.h, because the last     */
/* one is exposed to the application.                                         */
/* The proper place to hold this code would be the rvtranportInternal.h file, */
/* but it causes one more file of nano size to be maintained. If you go to    */
/* add the rvtranportInternal.h, please move the internal API there.          */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*******************************************************************************
 * RvTransportInit
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportInit function initializes the transport module.
 * Arguments:
 * Input:  None.
 * Output: None.
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ******************************************************************************/
RvStatus RvTransportInit(void);

/*******************************************************************************
 * RvTransportEnd
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportEnd function frees resources allocated by the transport
 *  module. No Transport API should be called after call to this function.
 * Arguments:
 * Input:  None.
 * Output: None.
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ******************************************************************************/
RvStatus RvTransportEnd(void);

/*******************************************************************************
 * RvTransportSourceConstruct
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportSourceConstruct function construct a Log Source object
 *  for the Transport module, using the provided Log Manager.
 * Arguments:
 * Input:  logMgr - the Log Manager object, where the Log Source should be added
 * Output: None.
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ******************************************************************************/
RvStatus RvTransportSourceConstruct(IN RvLogMgr* logMgr);


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                socket-style transport                                      */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                socket-style transport definitions                          */
/*----------------------------------------------------------------------------*/

/*@*****************************************************************************
 * Type: RvTransportSocketCfg (rvtransport)
 * -----------------------------------------------------------------------------
 * The RvTransportSocketCfg unites configuration parameters needed
 * for the socket-style transport creation.
 *
 * INPORTANT: Note you should call RvTransportInitSocketTransportCfg function
 *            before you are going to use this structure!
 ****************************************************************************@*/
typedef struct
{
    RvSocketProtocol      protocol;   /* UDP / TCP / ... */
    RvAddress*            pLocalAddr; /* addr to bind socket to. Can be NULL. */
    RvSocket              sock;
        /* Socket created and bound by the user.
           This parameter is optional. If provided, no socket will be created
           while constructing the Transport object. The 'sock' will be used
           instead. Put RV_INVALID_SOCKET here, if you have no ready-to-use
           socket, or use RvTransportInitTransportSocketCfg.
        */
    RvUint32              options;   /* A bit mask of the creation-time options */
    RvSelectEngine*       pSelectEngine;
    RvLogMgr*             pLogMgr;
    RvTransportCallbacks  callbacks; /* is required for non-blocking mode */
} RvTransportSocketCfg;



/*----------------------------------------------------------------------------*/
/*                Socket-style transport functions                            */
/*----------------------------------------------------------------------------*/
/*@*****************************************************************************
 * RvTransportCreateSocketTransport (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportCreateSocketTransport function allocates memory for
 *  the socket-style transport object and calls object constructor
 *  API function, while enabling the user to create the socket-style transport
 *  object.
 *
 * Arguments:
 * Input:   pCfg    - The configuration of the Transport object to be created.
 * Output:  pTransp - The pointer to the created Transport object.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportCreateSocketTransport(
                    IN  RvTransportSocketCfg* pCfg,
                    OUT RvTransport*          pTransp);

/*@*****************************************************************************
 * RvTransportGetSocketTransportConstructor (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportGetSocketTransportConstructor function fetches
 *  the constructor function, which constructs the socket-style transport
 *  in the user memory.
 *  This constructor can be passed to the RvTransportCreate API in order to
 *  create the socket-style transport object. The RvTransportCreate allocates
 *  needed memory and calls the constructor.
 *
 * Arguments:
 * Input:   none.
 * Output:  none.
 *
 * Return Value: the constructor function.
 ****************************************************************************@*/
RVCOREAPI
RvTransportConstructF RVCALLCONV RvTransportGetSocketTransportConstructor(void);

/*@*****************************************************************************
 * RvTransportGetSocketTransportSize (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportGetSocketTransportSize function fetches the size of memory
 *  (in bytes), needed to hold the socket-style transport object.
 *  This size can be passed to the RvTransportCreate API in order to
 *  create the socket-style transport object.
 *
 * Arguments:
 * Input:   none.
 * Output:  none.
 *
 * Return Value: the constructor function.
 ****************************************************************************@*/
RVCOREAPI
RvSize_t RVCALLCONV RvTransportGetSocketTransportSize(void);

/*@*****************************************************************************
 * RvTransportInitSocketTransportCfg (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportInitSocketTransportCfg function initializes
 *  the RvTransportSocketCfg structure required for creation of
 *  the socket-style transport object.
 *  IMPORTANT: you should initialize the configuration structure using this
 *             function only! Don't use 'memset(..., 0, ...)'!
 *
 * Arguments:
 * Input:   None.
 * Output:  pCfg - The configuration to be initialized.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
void RVCALLCONV RvTransportInitSocketTransportCfg(OUT RvTransportSocketCfg* pCfg);


/*----------------------------------------------------------------------------*/
/*                socket-style transport object                               */
/*                                                                            */
/*  We define the object here in order to enable it to be reused by           */
/*  implementations of more complex transports                                */
/*----------------------------------------------------------------------------*/

typedef struct
{
    /* Implement transport object hierarchy - put here the superclass,
       that contains the Table of Virtual Functions at the first place.
       Note it should be the first field in the transport structure !!!
    */
    struct RvTransportBase  base;

    /* Hold one more pointer to the socket-style transport interface
       in addition to this, stored in the struct RvTransportBase.
       This pointer is not overrode during inheritance,
       enabling thus the derived transport types to access the socket-style
       transport interface.
    */
    struct RvTransportInterface*   iface;

    /* User provided data
    */
    RvTransportEvents       events;
    RvSelectEngine*         pSelectEngine;
    RvLogMgr*               pLogMgr;

    /* Internal data
    */
    RvSocket                sock;
    RvSocketProtocol        sockProto;
    RvSelectFd              fd;
    RvUint32                status;
    RvTimer                 timer;
    RvInt32                 closeEvTimeout;
    RvUint32                options;

    /* Thread safety
    */
    RvMutex                 mutex;

} RvSocketTransport;

/*----------------------------------------------------------------------------*/
/*                socket-style transport interface                            */
/*----------------------------------------------------------------------------*/
/* RvSockTranspConstruct constructs the socket-style transport in the memory
    allocated by the user. If you don't want to bother with memory allocation,
    use Transport API: RvTransportCreate or RvTransportCreateSocketTransport.
*/
RVCOREAPI
RvStatus RvSockTranspConstruct(
                    IN  void*          cfg,
                    IN  RvTransport    transp);

/* RvSockTranspDestruct destructs the socket-style transport.
    It doesn't free object memory! If you don't want to bother with memory free,
    use the Transport API: RvTransportRelease.
*/
RvStatus RvSockTranspDestruct(
                    IN  RvTransport         transp);

/* RvSockTranspConnect is a socket-style implementation
    of the RvTransportConnect API function. See it for more details.
*/
RvStatus RvSockTranspConnect(
                    IN  RvTransport         transp,
                    IN  RvAddress*          pRemoteAddr);

/* RvSockTranspAccept is a socket-style implementation
    of the RvTransportAccept API function. See it for more details.
*/
RvStatus RvSockTranspAccept(
                    IN  RvTransport         transp,
                    IN  RvUint32            options,
                    OUT RvTransport*        pNewTransp,
                    OUT RvAddress*          pRemoteAddr);

/* RvSockTranspShutdown is a socket-style implementation
    of the RvTransportShutdown API function. See it for more details.
*/
RvStatus RvSockTranspShutdown(
                    IN  RvTransport         transp);

/* RvSockTranspSendBuffer is a socket-style implementation
    of the RvTransportSendBuffer API function. See it for more details.
*/
RVCOREAPI RvStatus RvSockTranspSendBuffer(
                    IN  RvTransport         transp,
                    IN  RvUint8*            buff,
                    IN  RvSize_t            len,
                    IN  RvAddress*          pDestAddr,
                    IN  RvUint32            options,
                    OUT RvSize_t*           pSent);

/* RvSockTranspSendManyBuffers is a socket-style implementation
    of the RvTransportSendManyBuffers API function. See it for more details.
*/
RVCOREAPI
RvStatus RVCALLCONV RvSockTranspSendManyBuffers(
                    IN  RvTransport         transp,
                    IN  RvUint32            numOfBuff,
                    IN  RvUint8**           ppBuff,
                    IN  RvSize_t*           pLen,
                    IN  RvAddress*          pDestAddr,
                    IN  RvUint32            options,
                    OUT RvSize_t*           pSent);

/* RvSockTranspReceiveBuffer is a socket-style implementation
    of the RvTransportReceiveBuffer API function. See it for more details.
*/
RvStatus RvSockTranspReceiveBuffer(
                    IN  RvTransport         transp,
                    IN  RvUint8*            buff,
                    IN  RvSize_t            len,
                    IN  RvUint32            options,
                    OUT RvAddress*          pRcvdFromAddr,
                    OUT RvSize_t*           pReceived);

RVCOREAPI RvStatus RvSockTranspGetBytesAvailable(
    IN  RvTransport             transp,
    OUT RvSize_t*               bytesAvailable);


/* RvSockTranspReceiveManyBuffers is a socket-style implementation
    of the RvTransportReceiveManyBuffers API function. See it for more details.
*/
RVCOREAPI
RvStatus RVCALLCONV RvSockTranspReceiveManyBuffers(
                    IN  RvTransport         transp,
                    IN  RvUint32            numOfBuff,
                    IN  RvUint8**           ppBuff,
                    IN  RvSize_t*           pLen,
                    IN  RvUint32            options,
                    OUT RvAddress*          pRcvdFromAddr,
                    OUT RvSize_t*           pReceived);

/* RvSockTranspSetOption is a socket-style implementation
    of the RvTransportSetOption API function. See it for more details.
*/
RvStatus RvSockTranspSetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    IN  void*               val);

/* RvSockTranspGetOption is a socket-style implementation
    of the RvTransportGetOption API function. See it for more details.
*/
RvStatus RvSockTranspGetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    OUT void*               val);

/* RvSockTranspRegisterEvent is a socket-style implementation
    of the RvTransportRegisterEvent API function. See it for more details.
*/
RvStatus RvSockTranspRegisterEvent(
                    IN  RvTransport         transp,
                    IN  RvTransportEvents   events);

/* RvSockTranspAddRef is a socket-style implementation
    of the RvTransportAddRef API function. See it for more details.
*/
void RvSockTranspAddRef(
                    IN  RvTransport         transp);

/* RvSockTranspRelease is a socket-style implementation
    of the RvTransportRelease API function. See it for more details.
*/
void RvSockTranspRelease(
                    IN  RvTransport         transp);

#endif /*#if (RV_NET_TYPE != RV_NET_NONE) */


#ifdef __cplusplus
}
#endif

#endif /*#ifndef _RV_TRANSPORTSOCKET_H */

