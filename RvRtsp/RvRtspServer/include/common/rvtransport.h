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
 * Module: CcTransport (root)
 * ----------------------------------------------------------------------------
 * CcTransport Module
 *
 * The Common Core Transport module provides the application with definition
 * of the transport abstraction (rvtransport.h)
 *
 * Definition of the Transport abstraction
 * ---------------------------------------
 * The Common Core Transport module defines abstraction of transport layer.
 * This abstraction provides the API functions that can be used in order to
 * establish connections and exchange data over it.
 *
 * Since the abstract transport doesn't know how exactly the connection should
 * be established and what is the format of the data to be sent/received,
 * the user has to create the Transport object of specific type.
 * The transport interface is defined by the RvTransportInterface structure.
 *
 * Note the transport abstraction supports non-blocking mode.
 * When working in this mode, the transport may be not ready to perform some
 * operations at the moment they are invoked. Therefore the Transport should
 * provide the user with the way to ensure the Transport readiness to perform
 * operation. RvTransportReceiveBuffer is an example of such operation.
 * In addition, the Transport should be able to notify the user of result of
 * asynchronous operation. RvTransportConnect is an example of such operation.
 * The RegisterEvent/NotifyEventCB mechanism was developed to meet these needs.
 * Before operation invocation the user has to register correspondent event.
 * When the Transport is ready to perform operation, it calls the NotifyEventCB,
 * which notifies the user that the operation can be invoked.
 * Or the Transport calls the NotifyEventCB when the operation was accomplished.
 *
 * In order to support non-blocking mode the transport abstraction expects
 * the user:
 *  1. to provide the Select Engine object
 *  2. to run event Select loop using the Select Engine API
 *
 * The RegisterEvent/NotifyEventCB API should not be used if the Transport works
 * in blocking mode, which is the default mode.
 ****************************************************************************@*/

#ifndef _RV_TRANSPORT_H
#define _RV_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                                */
/*----------------------------------------------------------------------------*/
#include "rvnettypes.h"
#include "rverror.h"

#if (RV_NET_TYPE != RV_NET_NONE)


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                Abstract transport                                          */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                Abstract transport definitions                              */
/*----------------------------------------------------------------------------*/

/*@*****************************************************************************
 * Type: RvTransport (rvtransport)
 * -----------------------------------------------------------------------------
 * The forward declaration of the abstract transport.
 ****************************************************************************@*/
typedef struct RvTransportBase* RvTransport;

/*@*****************************************************************************
 * Type: RvTransport (rvtransport)
 * -----------------------------------------------------------------------------
 * The forward declaration of the interface of the abstract transport.
 ****************************************************************************@*/
struct RvTransportInterface;

/*******************************************************************************
 ******* Asynchronous / non-blocking mode definitions
 *******
*/
/*@*****************************************************************************
 * Type: RvTransportEvents (rvtransport)
 * -----------------------------------------------------------------------------
 * The declaration of the events that can be processed by the Transport.
 * There are events of two types:
 *
 *  1. Events using which the user indicates to the Transport object about
 *     it's interest to be notified of the completed operation.
 *     An examples of such events are ACCEPT, CONNECT and CLOSE.
 *
 *  2. Events using which the Transport object indicates to the user about
 *     it's readiness to perform operation.
 *     An examples of such events are READ and WRITE. The READ event indicates
 *     that read operation can be performed (using RvTransportReceiveBuffer).
 *     The WRITE event indicates that write operation can be performed
 *     (using RvTransportSendBuffer).
 *
 * The user activates event reporting by registration the required event with
 * the Transport object using the RvTransportRegisterEvent function.
 * The Transport object notifies the user about generated event using
 * the RvTransportEventCb callback. The RvTransportEventCb callback can be set
 * on the object construction or later using the RvTransportSetOption function.
 ****************************************************************************@*/
typedef RvSelectEvents RvTransportEvents;
#define RVTRANSPORT_EVENT_READ    RvSelectRead
#define RVTRANSPORT_EVENT_WRITE   RvSelectWrite
#define RVTRANSPORT_EVENT_ACCEPT  RvSelectAccept
#define RVTRANSPORT_EVENT_CONNECT RvSelectConnect
#define RVTRANSPORT_EVENT_CLOSE   RvSelectClose

/*@*****************************************************************************
 * RvTransportEventCb (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportEventCb is used by the transport abstraction in order to
 *  notify the user of:
 *      1. Operation accomplishing (like CONNECT, ACCEPT, CLOSE)
 *      2. Readiness to perform operation (like READ or WRITE).
 *  You can set this callback into the Transport object on the object
 *  construction or later using the RvTransportSetOption function.
 *
 *  If the 'error' parameter is set to RV_TRUE, the Transport is not usable
 *  anymore. In this case the user should call the RvTransportRelease function
 *  as soon as possible in order to release resources.
 *
 * Arguments:
 * Input:   transp    - The Transport object.
 *          ev        - The occurred event.
 *          error     - RV_TRUE if any error occurred while waiting for the event
 *          usrData   - The sizeof(void*) bytes of data provided by the user
 *                      while setting this callback.
 * Output: None.
 *
 * Return Value: None.
 ****************************************************************************@*/
typedef void (*RvTransportEventCb)(
                    IN RvTransport        transp,
                    IN RvTransportEvents  ev,
                    IN RvBool             error,
                    IN void*              usrData);

/*@*****************************************************************************
 * Type: RvTransportCallbacks (rvtransport)
 * -----------------------------------------------------------------------------
 * The RvTransportCallbacks unites callbacks that can be set by the user
 * into the Transport object.
 * The 'usrData' field of this structure is not used by the Transport object.
 * The Transport object provides it back to the user while calling callbacks.
 ****************************************************************************@*/
typedef struct {
    RvTransportEventCb   pfnEvent;
    void*                usrData;
} RvTransportCallbacks;


/*----------------------------------------------------------------------------*/
/*                Abstract transport API functions                            */
/*----------------------------------------------------------------------------*/
/*@*****************************************************************************
 * RvTransportConnect (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportConnect function causes the connection-oriented Transport
 *  to establish connection to the remote peer.
 *  If Transport was configured for non-blocking mode the RvTransportConnect
 *  function registers RVTRANSPORT_EVENT_CONNECT event. Therefore,
 *  if the user set the EVENT callback, he will get the CONNECT event upon
 *  successful connection establishment. If the connection establishment failed
 *  for some reason, the 'error' parameter of the EVENT callback will be set to
 *  RV_TRUE.
 *  No data should be tried to be sent or to be received before the successful
 *  established of the connection.
 *
 * Arguments:
 * Input:   transp      - The Transport object to be connected.
 *          pRemoteAddr - The address of the remote peer to connect to.
 * Output:  None.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportConnect(
                    IN RvTransport  transp,
                    IN RvAddress*   pRemoteAddr);

/*@*****************************************************************************
 * RvTransportAccept (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportAccept function causes the connection-oriented listening
 *  Transport to accept the incoming connection. As a result of this,
 *  the new Transport object is constructed. This new Transport inherits
 *  parameters of the listening Transport.
 *  Note the caller should provide memory for the new object.
 *
 *  If Transport was configured for non-blocking mode, EVENT callback was set
 *  and user registered RVTRANSPORT_EVENT_ACCEPT with the Transport,
 *  the ACCEPT event will be generated each time there is pending incoming
 *  connection. To accept it the user has to call the RvTransportAccept
 *  function.
 *
 * Arguments:
 * Input:   transp      - The listening Transport object.
 *          options     - The options to be applied
 *                        to the new Transport object on creation.
 *                        The possible option values should be defined
 *                        by the implementation of the specific Transport.
 *          newTransp   - The memory, where the accepted Transport should be
 *                        constructed.
 * Output:  pRemoteAddr - The address where from the connection was initiated.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportAccept(
                    IN  RvTransport         transp,
                    IN  RvUint32            options,
                    OUT RvTransport*        pNewTransp,
                    OUT RvAddress*          pRemoteAddr);

/*@*****************************************************************************
 * RvTransportShutdown (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportShutdown function causes the connection-oriented Transport
 *  to initiate graceful termination of the connection.
 *  If you work in blocking mode, the connection is terminated before
 *  the RvTransportShutdown returns. In that case you have to call
 *  the RvTransportRelease immediately after the RvTransportShutdown returns.
 *  If you work in non-blocking mode, the connection is terminated after
 *  the RvTransportShutdown returns. In that case you have to register
 *  the RVTRANSPORT_EVENT_CLOSE event with the Transport in order to get
 *  notification of the termination accomplishment.
 *
 *  When the connection termination is finished, the user should call
 *  the RvTransportRelease in order to free resources, allocated
 *  by the Transport object.
 *
 *  Note the RvTransportShutdown function is provided in order to enable
 *  the user to read messages, sent by the remote peer during connection
 *  termination. If the user is not interested to get these messages,
 *  he should call the RvTransportRelease function directly.
 *
 *  Note once the RvTransportShutdown is called, the user can't send data using
 *  this Transport anymore. The user still be able to receive data.
 *
 *
 * Arguments:
 * Input:   transp - The Transport object.
 * Output:  None.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 *               If error is returned, the user should call
 *               the RvTransportRelease API function with 'graceful' parameter
 *               set to RV_FALSE.
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportShutdown(IN RvTransport transp);

/*@*****************************************************************************
 * RvTransportSendBuffer (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportSendBuffer function causes the Transport to send buffer.
 *
 * Arguments:
 * Input:   transp    - The Transport object.
 *          buff      - The buffer with the data to be sent.
 *          len       - The number of data bytes to be sent.
 *          pDest     - The address where the buffer will be sent. This argument
 *                      is ignored for the connection-oriented transports.
 *          options   - The bit mask of the sending options.
 *                      The possible option values should be defined
 *                      by the implementation of the specific Transport.
 * Output:  pSent     - The total number of bytes that were sent.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportSendBuffer(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t                len,
                    IN  RvAddress*              pDest,
                    IN  RvUint32                options,
                    OUT RvSize_t*               pSent);

/*@*****************************************************************************
 * RvTransportReceiveBuffer (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportReceiveBuffer function causes the Transport to receive data
 *  into buffer.
 *
 * Arguments:
 * Input:   transp        - The Transport object.
 *          buff          - The buffer to be filled with the received data.
 *          len           - The length of the 'buff' buffer.
 *          options       - The bit mask of the receiving options.
 *                          The possible option values should be defined
 *                          by the implementation of the specific Transport.
 * Output:  pRcvdFromAddr - The address where from the buffers were received.
 *          pReceived     - The total number of bytes that were received.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportReceiveBuffer(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t                len,
                    IN  RvUint32                options,
                    OUT RvAddress*              pRcvdFromAddr,
                    OUT RvSize_t*               pReceived);

/*@*****************************************************************************
* RvTransportGetBytesAvailable (rvtransport)
* -----------------------------------------------------------------------------
* General:
*  The RvTransportGetBytesAvailable function returns the number of bytes
*  that will be returned when the RvTransportReceiveBuffer will be called
*  for the next time.
*  Please note that the result of this function is estimation only. It can
*  be that the number of bytes provided by the function is not accurate and the
*  subsequent call to the RvTransportReceiveBuffer function will return with
*  less bytes then returned by RvTransportGetBytesAvailable  call. It is
*  especially true when the RvTransport is TURN or TLS based.
*
* Arguments:
* Input:   transp           - The Transport object.
* Output:  bytesAvailable   - The number of bytes available for reading.
*
* Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportGetBytesAvailable(
    IN  RvTransport             transp,
    OUT RvSize_t*               bytesAvailable);


/*@*****************************************************************************
 * RvTransportGetOption (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportGetOption function fetches value of the option, set into
 *  the Transport object. The type of the value depends on option.
 *  See definition of the specific option in order to get more info.
 *
 * Arguments:
 * Input:   transp  - The Transport object.
 *          type    - The identifier of option collection, where the option
 *                    belongs to.
 *                    The option type should be defined in conjunction with
 *                    the set of options that are defined by the transport
 *                    implementation. It's value should be unique among various
 *                    transport implementations.
 *          option  - The option to be fetched.
 *                    The possible option values should be defined
 *                    by the implementation of the specific Transport.
 * Output:  val     - The fetched value of the option.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportGetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    OUT void*               val);

/*@*****************************************************************************
 * RvTransportSetOption (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportSetOption function sets option into the Transport object.
 *  The type of the value depends on option.
 *  See definition of the specific option in order to get more info.
 *
 * Arguments:
 * Input:   transp  - The Transport object.
 *          type    - The identifier of option collection, where the option
 *                    belongs to.
 *                    The option type should be defined in conjunction with
 *                    the set of options that are defined by the transport
 *                    implementation. It's value should be unique among various
 *                    transport implementations.
 *          option  - The option to be set.
 *                    The possible option values should be defined
 *                    by the implementation of the specific Transport.
 *          val     - The value of the option.
 * Output:  None.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportSetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    IN  void*               val);

/*@*****************************************************************************
 * RvTransportGetType (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportGetType function fetches type of the transport object.
 *  The transport type should be defined by the transport implementation.
 *  This type should be unique among all possible transport implementations.
 *
 * Arguments:
 * Input:   transp  - The Transport object.
 *
 * Return Value: the transport type.
 ****************************************************************************@*/
RVCOREAPI
const RvChar* RVCALLCONV RvTransportGetType(IN  RvTransport  transp);

/*@*****************************************************************************
 * RvTransportSetCallbacks (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportSetCallbacks function sets set of user callbacks into
 *  the Transport object.
 *  Note this function is not thread safe, therefore it should be called only
 *  if the Transport object is in the state, where it is guaranteed it
 *  won't call the callbacks.
 *
 * Arguments:
 * Input:   transp      - The Transport object.
 *          pCallbacks  - The callbacks to be set.
 * Output:  none.
 * Return Value: RV_OK on success, RV_ERROR_XXX - otherwise.
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportSetCallbacks(
                        IN  RvTransport             transp,
                        IN  RvTransportCallbacks*   pCallbacks);

/*@*****************************************************************************
 * RvTransportGetCallbacks (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportGetCallbacks function fetches set of user callbacks used
 *  by the Transport object.
 *  Note this function is not thread safe, therefore it should be called only
 *  if the Transport object is in the state, where it is guaranteed it
 *  the callback set won't alter.
 *
 * Arguments:
 * Input:   transp      - The Transport object.
 * Output:  pCallbacks  - The callbacks to be got.
 * Return Value: RV_OK on success, RV_ERROR_XXX - otherwise.
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportGetCallbacks(
                        IN  RvTransport             transp,
                        OUT RvTransportCallbacks*   pCallbacks);

/*******************************************************************************
 ******* Asynchronous / non-blocking functions
 *******
*/
/*@*****************************************************************************
 * RvTransportRegisterEvent (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportRegisterEvent function registers one or few transport events
 *  with the Transport object. When the event occurs, the Transport calls
 *  the RvTransportEventCb callback function to notify the user of this.
 *  Note the event registration performed by previous call
 *  to the RvTransportRegisterEvent is overridden.
 *  In order to remove registration of specific event you have to provide
 *  (registered_events & ~event) mask. In order to remove all events use zero.
 *
 *  INPORTANT: when working in multi-threading mode, be sure you unlock
 *  the associated application object before you call
 *  the RvTransportRegisterEvent with zero 'events' !!!
 *  The zero 'events' causes this function to block, if there RvTransportEventCb
 *  callback is being called. That means the function waits until the event
 *  processing by the application is finished. The RvTransportRegisterEvent will
 *  block the callback returns.
 *
 * Arguments:
 * Input:   transp  - The Transport object.
 * Output:  events  - The bit mask of events.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportRegisterEvent(
                    IN RvTransport       transp,
                    IN RvTransportEvents events);

/*******************************************************************************
 ******* Life-cycle functions
 *******
*/
/*@*****************************************************************************
 * RvTransportAddRef (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportAddRef function increments the reference counter
 *  of the Transport object, preventing thus it destruction and release of
 *  it's memory.
 *
 * Arguments:
 * Input:   transp - The object to be destructed.
 * Output:  none.
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportAddRef(
                    IN  RvTransport     transp);

/*@*****************************************************************************
 * RvTransportRelease (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportRelease function decrements the reference counter
 *  of the Transport object.
 *  If the reference counter becomes to be zero, as a result of this decrement,
 *  the object is destructed, and it's memory is freed.
 *  The reference counter is incremented by RvTransportAddRef and
 *  transport-specific-create functions.
 *
 *  Note:
 *      1. Once the RvTransportRelease returns, you should not use the object
 *         anymore.
 *      2. You should be ready to handle the EVENT callback calls until
 *         the RvTransportRelease returns.
 *
 * Arguments:
 * Input:   transp - The object to be released.
 * Output:  none.
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportRelease(
                    IN  RvTransport     transp);

/*----------------------------------------------------------------------------*/
/*                Transport Interface definitions                             */
/*----------------------------------------------------------------------------*/

/*******************************************************************************
 * Transport interface
 * -----------------------------------------------------------------------------
 * The transport interface should be provided by the user, while creating
 * the Transport object of the specific type. The type specific interface
 * implements the behavior of the Transport objects of that type. The interface
 * include functions, listed below. The interface functions reflect
 * the Transport API functions, declared above. You can use documentation
 * for the Transport API in order to get a clue what the interface functions
 *should do.
 ******************************************************************************/

typedef
RvStatus (*RvTransportDestructF)(
                    IN  RvTransport             transp);

typedef
RvStatus (*RvTransportConnectF)(
                    IN  RvTransport             transp,
                    IN  RvAddress*              pRemoteAddr);

typedef
RvStatus (*RvTransportAcceptF)(
                    IN  RvTransport             transp,
                    IN  RvUint32                options,
                    OUT RvTransport*            newTransp,
                    OUT RvAddress*              pRemoteAddr);

typedef
RvStatus (*RvTransportShutdownF)(
                    IN  RvTransport         transp);

typedef
RvStatus (*RvTransportSendBufferF)(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t                len,
                    IN  RvAddress*              pDestAddr,
                    IN  RvUint32                options,
                    OUT RvSize_t*               pSent);

typedef
RvStatus (*RvTransportReceiveBufferF)(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t              	len,
                    IN  RvUint32                options,
                    OUT RvAddress*              pRcvdFromAddr,
                    OUT RvSize_t*               pReceived);

typedef
RvStatus (*RvTransportGetBytesAvailableF)(
                    IN  RvTransport             transp,
                    OUT RvSize_t*               bytesAvailable);

typedef
RvStatus (*RvTransportSetOptionF)(
                    IN  RvTransport             transp,
                    IN  RvUint32                type,
                    IN  RvUint32                option,
                    IN  void*                   val);

typedef
RvStatus (*RvTransportGetOptionF)(
                    IN  RvTransport             transp,
                    IN  RvUint32                type,
                    IN  RvUint32                option,
                    IN  void*                   val);

typedef
RvStatus (*RvTransportSetCallbacksF)(
                    IN  RvTransport             transp,
                    IN  RvTransportCallbacks*   pCallbacks);

typedef
RvStatus (*RvTransportGetCallbacksF)(
                    IN  RvTransport             transp,
                    OUT RvTransportCallbacks*   pCallbacks);

typedef
RvStatus (*RvTransportRegisterEventF)(
                    IN  RvTransport             transp,
                    IN  RvTransportEvents       events);

typedef
void (*RvTransportAddRefF)(
                    IN  RvTransport             transp);

typedef
void (*RvTransportReleaseF)(
                    IN  RvTransport             transp);

/*@*****************************************************************************
 * Type: struct RvTransportInterface (rvtransport)
 * -----------------------------------------------------------------------------
 * The declaration of the interface that should be provided by the user
 * while creating the the Transport object of the specific type.
 * The user should put this structure as the first field of the structure
 * that defines the Transport object of the specific type.
 ****************************************************************************@*/
struct RvTransportInterface
{
    const RvChar*                   strTransportType;
    RvTransportDestructF            pfnDestruct;
    RvTransportConnectF             pfnConnect;
    RvTransportAcceptF              pfnAccept;
    RvTransportShutdownF            pfnShutdown;
    RvTransportSendBufferF          pfnSendBuffer;
    RvTransportReceiveBufferF       pfnReceiveBuffer;
    RvTransportGetBytesAvailableF   pfnGetBytesAvailable;
    RvTransportSetOptionF           pfnSetOption;
    RvTransportGetOptionF           pfnGetOption;
    RvTransportSetCallbacksF        pfnSetCallbacks;
    RvTransportGetCallbacksF        pfnGetCallbacks;
    RvTransportRegisterEventF       pfnRegisterEvent;
    RvTransportAddRefF              pfnAddRef;
    RvTransportReleaseF             pfnRelease;
};

/*@*****************************************************************************
 * Type: struct RvTransportBase (rvtransport)
 * -----------------------------------------------------------------------------
 * The declaration of the abstract Transport.
 * IMPORTANT: when you go to implement the new type of Transport, you should
 * override this abstract definition, while placing
 * the RvTransportInterface structure as the first field in the structure
 * that defines the new Transport !!!
 * Otherwise you will get unpredictable behavior of the Transport API functions.
 ****************************************************************************@*/
struct RvTransportBase
{
    struct RvTransportInterface* iface;
    RvTransportCallbacks         callbacks;  /* for non-blocking operations */
    RvInt32  refCounter;
        /* Reference counter.
           The Create and AddRef functions increment it,
           the Release function decrement it.
           When the counter reaches zero, the object can be destructed,
           and it's memory can be freed. */
};


/*----------------------------------------------------------------------------*/
/*                Utility transport API functions                            */
/*----------------------------------------------------------------------------*/

/*@*****************************************************************************
 * RvTransportConstructF (rvtransport)
 * -----------------------------------------------------------------------------
 * General:
 *  The RvTransportConstructF type of function stands for the abstract
 *  Transport object Constructor function.
 *  Each Transport implementation should include such Constructor.
 *  The Constructor constructs the transport object of the specific type
 *  in the memory, provided by the Constructor caller.
 *
 * Arguments:
 * Input:   cfg     - The Transport type specific configuration structure.
 *          transp  - The memory, where the object of the specific transport
 *                    type should be constructed.
 * Output:  none.
 *
 * Return Value: RV_OK on success, error code otherwise (RV_ERROR_X).
 ****************************************************************************@*/
typedef
RvStatus (*RvTransportConstructF)(
                            IN  void*       cfg,
                            IN  RvTransport transp);



/* Names of different transport types */

#define RV_SOCKET_TRANSPORT_STR         "SocketTransport"

#define RV_TURN_TRANSPORT_STR           "TurnTransport"

#define RV_TLS_TRANSPORT_STR            "TlsSocketTransport"



/*******************************************************************************
******* Options that are applicable to the socket-style transport when
******* the transport is being created.
******* You can provide these options as the 'options' bit mask field
******* of the RvTransportSocketCfg configuration structure, while calling
******* the RvTransportCreateSocketTransport function and
******* you can provide them options as the 'options' function argument,
******* while calling the RvTransportAccept.
*/
#define RVTRANSPORT_CREATEOPT_LISTENING          0x0001
/* Causes the Transport to listen for incoming connections */

#define RVTRANSPORT_CREATEOPT_DONTCLOSESOCK      0x0002
/* Sets the not to close socket on destroy */

#define RVTRANSPORT_CREATEOPT_CLOSEEVENTTIMEOUT  0x0004
/* Causes the Transport to wait for CLOSE event from network
for no more than 5 seconds, while shutdown gracefully */

#define RVTRANSPORT_CREATEOPT_IPV6SOCKET         0x0008
/* Causes the Transport to use IPv6 socket */

#define RVTRANSPORT_CREATEOPT_NONBLOCKING        0x0010
/* Causes the Transport to work in non-blocking mode */

#define RVTRANSPORT_CREATEOPT_REUSEADDR          0x0020
/* Make possible other sockets to use the Transport local address */

#define RVTRANSPORT_CREATEOPT_NOTCPLINGER        0x0040
/* Cancels the system TCP linger timer mechanism, that is activated
by default for the Transport socket */

#define RVTRANSPORT_CREATEOPT_BROADCAST          0x0080
/* Causes the Transport socket to send broadcast packets */


#define RVTRANSPORT_CREATEOPT_NODELAY            0x0100
/* Causes the Transport socket to disable the Nagle algorithm */


/*******************************************************************************
******* Options that are applicable to the socket-style transport after
******* the transport was created.
******* You can set / retrieve these options using the RvTransportSetOption /
******* the RvTransportGetOption transport API functions.
*******
******* The RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT should be provided by user
******* to the GetOption and SetOption API functions as the 'type' argument.
******* It is needed in order to prevent setting / getting options with same
******* numeric values, that defined by the other transport implementations,
******* and that are not valid for the socket-style transport.
*******
******* Any of the option values should be larger than any of the CREATEOPT
******* option values or mask of the CREATEOPT option values. Such approach
******* enable us to detect improper usage of SET/GET options instead of
******* CREATEOPT options and vice versa. Therefore we start option values
******* from 0x1000.
*/
#define RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT         0x1234432

#define RVTRANSPORT_OPTTYPE_TURNTRANSPORT           0x1234433

#define RVTRANSPORT_OPTTYPE_TLSSOCKTRANSPORT        0x1234434

/* General options */
#define RVTRANSPORT_OPT_DONTCLOSESOCK               0x10001
#define RVTRANSPORT_OPT_SELECTENGINE                0x10002
#define RVTRANSPORT_OPT_CLOSEEVENTTIMEOUT           0x10003
#define RVTRANSPORT_OPT_SOCKET                      0x10004
#define RVTRANSPORT_OPT_LOCALPORT                   0x10005
#define RVTRANSPORT_OPT_LOCALADDR                   0x10006
#define RVTRANSPORT_OPT_PUBLICLOCALADDR             0x10007

/* Wrappers for the system socket options */
#define RVTRANSPORT_OPT_SOCK_NONBLOCKING            0x100010
#define RVTRANSPORT_OPT_SOCK_BUFSIZE                0x100011
#define RVTRANSPORT_OPT_SOCK_BROADCAST              0x100012
#define RVTRANSPORT_OPT_SOCK_BYTESAVAILABLE         0x100013
#define RVTRANSPORT_OPT_SOCK_MULTICASTINTERFACE     0x100014
#define RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_JOIN    0x100015
#define RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_LEAVE   0x100016
#define RVTRANSPORT_OPT_SOCK_MULTICASTNOLOOP        0x100017
#define RVTRANSPORT_OPT_SOCK_MULTICASTTTL           0x100018
#define RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE          0x100019

/* STUN socket options */
#define RVTRANSPORT_OPT_STUN_GET_STUN_PACKETS_RCVD		0x20309
#define RVTRANSPORT_OPT_STUN_GET_NON_STUN_PACKETS_RCVD	0x2030A
#define RVTRANSPORT_OPT_STUN_GET_STUN_PACKETS_SENT		0x2030B
#define RVTRANSPORT_OPT_STUN_GET_NON_STUN_PACKETS_SENT	0x2030C
#define RVTRANSPORT_OPT_STUN_GET_STATE                  0x2030D



/* The table below provides you with the actual type of option value,
* expected by the RvTransportGetOption and the RvTransportSetOption functions.
*
*                                            | actual type of 'val' argument
*  ------------------------------------------+---------------------------------
*                                  RvTransportGetOption
*  ------------------------------------------+---------------------------------
*  RVTRANSPORT_OPT_SELECTENGINE              | RvSelectEngine**
*  RVTRANSPORT_OPT_CLOSEEVENTTIMEOUT         | RvInt32*  (msec)
*  RVTRANSPORT_OPT_SOCK_BYTESAVAILABLE       | RvSize_t*
*  RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE        | RvInt32*
*  RVTRANSPORT_OPT_SOCKET                    | RvSocket*
*  RVTRANSPORT_OPT_LOCALPORT                 | RvUint16*
*  RVTRANSPORT_OPT_LOCALADDR                 | RvAddress*
*  RVTRANSPORT_OPT_PUBLICLOCALADDR           | RvAddress*
*
*  ------------------------------------------+---------------------------------
*                                  RvTransportSetOption
*  ------------------------------------------+---------------------------------
*  RVTRANSPORT_OPT_SELECTENGINE              | RvSelectEngine*
*  RVTRANSPORT_OPT_DONTCLOSESOCK             | RvBool*
*  RVTRANSPORT_OPT_CLOSEEVENTTIMEOUT         | RvInt32* (msec)
*  RVTRANSPORT_OPT_SOCKET                    | RvSocket*
*  RVTRANSPORT_OPT_LOCALADDR                 | RvAddress* - address to bind socket to
*  RVTRANSPORT_OPT_SOCK_NONBLOCKING          | RvBool*
*  RVTRANSPORT_OPT_SOCK_BUFSIZE              | RvInt32[]
*                                            | val[0] - send buf size
*                                            | val[1] - recv buf size
*                                            | '-1' - ignore val[i]
*  RVTRANSPORT_OPT_SOCK_BROADCAST            | RvBool*
*  RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_JOIN  | RvAddress*[]
*  RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_LEAVE | val[0] - mcast address
*                                            | val[1] - interface address;
*                                            |          if NULL, arbitrary
*                                            |          interface is chosen
*  RVTRANSPORT_OPT_SOCK_MULTICASTINTERFACE   | RvAddress* - interface address
*  RVTRANSPORT_OPT_SOCK_MULTICASTNOLOOP      | RvBool*
*  RVTRANSPORT_OPT_SOCK_MULTICASTTTL         | RvInt32*
*  RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE        | RvInt32*
*
*/

/* Create TCP/UDP socket transport instance */
RVCOREAPI RvTransport RvTransportCreateSocket(
    void* pLogMgr,
    void* pSelectEngine,
    RvBool bTcp,
    RvUint32 options,
    RvAddress* pLocAddress,
    RvTransportCallbacks* pCallbacks);



#define RVTRANSPORT_OPT_TLSOCK_TEST_CERT_FINGERPRINT         0x200001
#define RVTRANSPORT_OPT_TLSOCK_REMOTE_FINGERPRINT_CFG        0x200002

/* How long the accepted TLS connection may be kept in the pending connections list. The timeout in seconds */
#define RV_TLS_TRANSPORT_MAX_PENDING_CONN_TIMEOUT   20

typedef struct _RvTlsSockTransportCfg RvTlsSockTransportCfg;

/* Create TLS transport instance */
RVCOREAPI RvStatus RvTransportCreateTlsSocket(
    IN  RvTlsSockTransportCfg *pCfg,
    OUT RvTransport*    pTransp);




#endif /*#if (RV_NET_TYPE != RV_NET_NONE) */

#ifdef __cplusplus
}
#endif

#endif /*#ifndef _RV_TRANSPORT_H */

