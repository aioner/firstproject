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
 *                              rvtransportsocket.c
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
 * The Common Core RvSocketTransport module provides the user with
 * the implementation of the Socket-style Transport.
 * The Socket-style transport simply wraps the UDP/TCP socket.
 * The transport object can be created using
 * the RvTransportCreateSocketTransport API function, which wraps the generic
 * RvTransportCreate API function.
 ****************************************************************************@*/

/*----------------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                                */
/*----------------------------------------------------------------------------*/
#include "rvtransportsocket.h"
#include "rvmemory.h"
#include "rvstrutils.h"
#include "rvsocket.h"


/*----------------------------------------------------------------------------*/
/*   RvSocketTransport  interface (implementation of abstract Transport i-face */
/*----------------------------------------------------------------------------*/
struct RvTransportInterface gSocketTransportInterface =
{
    RV_SOCKET_TRANSPORT_STR,
    RvSockTranspDestruct,
    RvSockTranspConnect,
    RvSockTranspAccept,
    RvSockTranspShutdown,
    RvSockTranspSendBuffer,
    RvSockTranspReceiveBuffer,
    RvSockTranspGetBytesAvailable,
    RvSockTranspSetOption,
    RvSockTranspGetOption,
    NULL,   /* SetCallbacks */
    NULL,   /* GetCallbacks */
    RvSockTranspRegisterEvent,
    RvSockTranspAddRef,
    RvSockTranspRelease
};



/* Statuses of the RvSocketTransport object.
   These statuses can be assigned to the 'status' field of the RvSocketTransport */
#define SOCKTRANSP_STATUS_CONNECTED       0x0001
#define SOCKTRANSP_STATUS_SHUTTINGDOWN    0x0002
#define SOCKTRANSP_STATUS_DESTRUCTING     0x0004
#define SOCKTRANSP_STATUS_SOCKERROR       0x0008
#define SOCKTRANSP_STATUS_WAITINGEVENTS   0x0010
#define SOCKTRANSP_STATUS_CLOSETIMERRUNS  0x0020
#define SOCKTRANSP_STATUS_CLOSEEVHANDLED  0x0040

/* Hardware configuration of the Socket Transport */
#define SOCKTRANSP_CFG_SOCKQUEUELEN 32
#define SOCKTRANSP_CFG_CLOSEEVENTTO 5000 /* 5 sec timeout for CLOSE event */



/*----------------------------------------------------------------------------*/
/*                        UTILITY DEFINITIONS                                 */
/*----------------------------------------------------------------------------*/
/*******************************************************************************
 * LOGGING UTILITIES
 ******************************************************************************/
#define TRANSPORT_LOG_EXCEP(f) \
    if (logMgr) {RvLogExcep(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f))}
#define TRANSPORT_LOG_EXCEP_1(f,p1) \
    if (logMgr) {RvLogExcep(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1))}
#define TRANSPORT_LOG_EXCEP_2(f,p1,p2) \
    if (logMgr) {RvLogExcep(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2))}


#define TRANSPORT_LOG_ERR(f) \
    if (logMgr) {RvLogError(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f))}
#define TRANSPORT_LOG_ERR_1(f,p1) \
    if (logMgr) {RvLogError(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1))}
#define TRANSPORT_LOG_ERR_2(f,p1,p2) \
    if (logMgr) {RvLogError(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2))}
#define TRANSPORT_LOG_ERR_3(f,p1,p2,p3) \
    if (logMgr) {RvLogError(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3))}
#define TRANSPORT_LOG_ERR_4(f,p1,p2,p3,p4) \
    if (logMgr) {RvLogError(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4))}
#define TRANSPORT_LOG_ERR_5(f,p1,p2,p3,p4,p5) \
    if (logMgr) {RvLogError(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5))}

#define TRANSPORT_LOG_WARN(f) \
    if (logMgr) {RvLogWarning(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f))}
#define TRANSPORT_LOG_WARN_1(f,p1) \
    if (logMgr) {RvLogWarning(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1))}
#define TRANSPORT_LOG_WARN_2(f,p1,p2) \
    if (logMgr) {RvLogWarning(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2))}
#define TRANSPORT_LOG_WARN_3(f,p1,p2,p3) \
    if (logMgr) {RvLogWarning(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3))}

#define TRANSPORT_LOG_DBG(f) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f))}
#define TRANSPORT_LOG_DBG_1(f,p1) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1))}
#define TRANSPORT_LOG_DBG_2(f,p1,p2) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2))}
#define TRANSPORT_LOG_DBG_3(f,p1,p2,p3) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3))}
#define TRANSPORT_LOG_DBG_4(f,p1,p2,p3,p4) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4))}
#define TRANSPORT_LOG_DBG_5(f,p1,p2,p3,p4,p5) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5))}
#define TRANSPORT_LOG_DBG_6(f,p1,p2,p3,p4,p5,p6) \
    if (logMgr) {RvLogDebug(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5,p6))}

#define TRANSPORT_LOG_INFO(f) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f))}
#define TRANSPORT_LOG_INFO_1(f,p1) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1))}
#define TRANSPORT_LOG_INFO_2(f,p1,p2) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2))}
#define TRANSPORT_LOG_INFO_3(f,p1,p2,p3) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3))}
#define TRANSPORT_LOG_INFO_4(f,p1,p2,p3,p4) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4))}
#define TRANSPORT_LOG_INFO_5(f,p1,p2,p3,p4,p5) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5))}
#define TRANSPORT_LOG_INFO_6(f,p1,p2,p3,p4,p5,p6) \
    if (logMgr) {RvLogInfo(&logMgr->transportSource, (&logMgr->transportSource, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5,p6))}

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
#define TRANSPORT_DEFINE_LOG_MGR(_transp) RvLogMgr* logMgr = ((RvSocketTransport*)(_transp))->pLogMgr
#else
#define TRANSPORT_DEFINE_LOG_MGR(_transp) RvLogMgr* logMgr = NULL; RV_UNUSED_ARG(logMgr)
#endif

/*******************************************************************************
 * LOCKING UTILITIES
 ******************************************************************************/
#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
#define TRANSPORT_LOCK(_transp) \
{ \
    RvStatus retValLock = RvMutexLock(&((RvSocketTransport*)_transp)->mutex, logMgr); \
    if (RV_OK != retValLock) \
    { \
        retValLock = RvErrorGetCode(retValLock); \
        TRANSPORT_LOG_ERR_2("%p: failed to lock(retv=%d)", _transp, retValLock); \
        return retValLock; \
    } \
}
#define TRANSPORT_LOCK_VOID(_transp) \
{ \
    RvStatus retValLock = RvMutexLock(&((RvSocketTransport*)_transp)->mutex, logMgr); \
    if (RV_OK != retValLock) \
    { \
        retValLock = RvErrorGetCode(retValLock); \
        TRANSPORT_LOG_ERR_2("%p: failed to lock(retv=%d)", _transp, retValLock); \
        return; \
    } \
}
#define TRANSPORT_UNLOCK(_transp) \
    RvMutexUnlock(&((RvSocketTransport*)_transp)->mutex, logMgr)


#define TRANSPORT_RESTORE(_transp) \
    RvMutexRestore(&((RvSocketTransport*)_transp)->mutex, logMgr, numLocks)

#define TRANSPORT_RELEASE(_transp) \
    RvMutexRelease(&((RvSocketTransport*)_transp)->mutex, logMgr, &numLocks)

#else
#define TRANSPORT_LOCK(_transp)
#define TRANSPORT_LOCK_VOID(_transp)
#define TRANSPORT_UNLOCK(_transp)
#define TRANSPORT_RELEASE(_transp)
#define TRANSPORT_RESTORE(_transp)
#endif


/*----------------------------------------------------------------------------*/
/*                        STATIC FUNCTION DECLARATIONS                        */
/*----------------------------------------------------------------------------*/
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
static
const RvChar* ConvertSelectEvent2Str(IN RvInt selectEvent);
static
RvTransportEvents ConvertSelectEvent2SockTranspEvent(IN RvInt selectEvent);
#endif /* #if (RV_LOGMASK != RV_LOGLEVEL_NONE) */

static
RvStatus ConstructSocket(
            IN RvSocketTransport* pSockTransp,
            IN RvUint32           options);

static
RvStatus SetCreationOptions(
            IN RvSocketTransport* pSockTransp,
            IN RvUint32           options);

static
RvBool ForbidFunctionCall(
            IN RvSocketTransport* pSockTransp,
            IN RvUint32         forbiddingStatus,
            IN const RvChar*    strFuncName);

static
void SelectCB(
            IN RvSelectEngine*  selectEngine,
            IN RvSelectFd*      fd,
            IN RvSelectEvents   selectEvent,
            IN RvBool           error);

static
RvBool TimerCloseEventCB(
            IN void*    ctx,
            IN RvTimer* timer);

static
RvStatus RegisterEvent(
            IN RvSocketTransport*  pSockTransp,
            IN RvTransportEvents   events);

static
RvBool IsShutdownable(
            IN RvSocketTransport*  pSockTransp);

static
RvStatus SetOption(
            IN  RvSocketTransport*  pSockTransp,
            IN  RvUint32            option,
            IN  void*               val);

static
RvStatus GetOption(
            IN  RvSocketTransport*  pSockTransp,
            IN  RvUint32            option,
            IN  void*               val);

static
void SafeSelectRemove(IN RvSocketTransport* pSockTransp);

static
void SafeTimerCancel(IN RvSocketTransport* pSockTransp);

static
void CallEventCallback(
            IN  RvSocketTransport*  pSockTransp,
            IN  RvTransportEvents   event,
            IN  RvBool              error);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                          INTERNAL TRANSPORT MODULE API                     */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

RvStatus RvTransportInit(void)
{
    return RV_OK;
}

RvStatus RvTransportEnd(void)
{
    return RV_OK;
}

RvStatus RvTransportSourceConstruct(IN RvLogMgr* logMgr)
{
    RvStatus status;
    status = RvLogSourceConstruct(logMgr, &logMgr->transportSource,
        "TRANSPORT", "Transport Layer support");
    return status;
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*                          SOCKET TRANSPORT                                  */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                 SOCKET TRANSPORT PUBLIC API IMPLEMENTATION                 */
/*----------------------------------------------------------------------------*/
RVCOREAPI
RvStatus RVCALLCONV RvTransportCreateSocketTransport(
                    IN  RvTransportSocketCfg* pCfg,
                    OUT RvTransport*          pTransp)
{
#define FUNCTION_NAME "RvTransportCreateSocketTransport"
    RvStatus rvs;
    RvLogMgr* logMgr = pCfg->pLogMgr;

    rvs = RvMemoryAlloc(NULL, sizeof(RvSocketTransport), pCfg->pLogMgr, (void**)pTransp);
    if (rvs != RV_OK)
    {
        TRANSPORT_LOG_ERR("Failed in RvMemoryAlloc");
        *pTransp = NULL;
        return RV_ERROR_OUTOFRESOURCES;
    }
    memset(*pTransp,0,sizeof(RvSocketTransport));

    if (RvSockTranspConstruct(pCfg,*pTransp) != RV_OK)
    {
        TRANSPORT_LOG_ERR("Failed in RvSockTranspConstruct");
        RvMemoryFree((void*)*pTransp,pCfg->pLogMgr);
        *pTransp = NULL;
        return RV_ERROR_OUTOFRESOURCES;
    }

    return RV_OK;
#undef FUNCTION_NAME
}

RVCOREAPI
RvTransportConstructF RVCALLCONV RvTransportGetSocketTransportConstructor(void)
{
    return (RvTransportConstructF)RvSockTranspConstruct;
}

RVCOREAPI
RvSize_t RVCALLCONV RvTransportGetSocketTransportSize(void)
{
    return sizeof(RvSocketTransport);
}

/*----------------------------------------------------------------------------*/
/*                 SOCKET TRANSPORT INTERBNAL API IMPLEMENTATION              */
/*----------------------------------------------------------------------------*/
RVCOREAPI
void RVCALLCONV RvTransportInitSocketTransportCfg(OUT RvTransportSocketCfg* pCfg)
{
    memset(pCfg, 0, sizeof(RvTransportSocketCfg));
    pCfg->sock = RV_INVALID_SOCKET;
}

/*----------------------------------------------------------------------------*/
/*                 SOCKET TRANSPORT INTERFACE IMPLEMENTATION                  */
/*----------------------------------------------------------------------------*/
RVCOREAPI
RvStatus RvSockTranspConstruct(
                    IN  void*          cfg,
                    IN  RvTransport    transp)
{
#define FUNCTION_NAME "RvSockTranspConstruct"
    RvStatus              retv;
    RvSocketTransport*    pSockTransp = (RvSocketTransport*)transp;
    RvTransportSocketCfg* pCfg = (RvTransportSocketCfg*)cfg;
    RvLogMgr*             logMgr = pCfg->pLogMgr;
    RvBool                sockConstructed = RV_FALSE;
    RvBool                fdConstructed = RV_FALSE;

    if (pCfg->options & 0xffff0000)
    {
        TRANSPORT_LOG_ERR_1("Bad create-time options were provided: 0x%x. The RVTRANSPORT_CREATEOPT_XXX options were expected",
            pCfg->options);
        return RV_ERROR_BADPARAM;
    }

    pSockTransp->base.iface      = &gSocketTransportInterface;  /* This one can be replaced by the derived class constructor */
    pSockTransp->iface           = &gSocketTransportInterface;  /* This one still with us forever :) */
    pSockTransp->base.callbacks  = pCfg->callbacks;
    pSockTransp->sock            = RV_INVALID_SOCKET;
    pSockTransp->pSelectEngine   = pCfg->pSelectEngine;
    pSockTransp->pLogMgr         = pCfg->pLogMgr;
    pSockTransp->sockProto       = pCfg->protocol;
    pSockTransp->options         = pCfg->options;

    /* Open socket. If user provided ready-to-use socket - use it. */
    if (pCfg->sock != RV_INVALID_SOCKET)
    {
        pSockTransp->sock = pCfg->sock;
    }
    else
    {
        retv = ConstructSocket(pSockTransp, pCfg->options);
        if (retv != RV_OK)
        {
            TRANSPORT_LOG_ERR_1("failed to construct socket(retv=%d)", retv);
            goto failed;
        }
        sockConstructed = RV_TRUE;
    }

    /* Bind socket to the provided IP:port */
    if (pCfg->pLocalAddr != NULL)
    {
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
        RvAddress boundAddr;
        RvChar    strIp[64] = {'\0'};
        RvAddressGetString(pCfg->pLocalAddr, sizeof(strIp), strIp);
        TRANSPORT_LOG_INFO_4("%p: goes to bind sock=%d to %s:%d",
            pSockTransp, pSockTransp->sock, strIp, RvAddressGetIpPort(pCfg->pLocalAddr));
#endif

        retv = RvSocketBind(&pSockTransp->sock, pCfg->pLocalAddr, NULL, logMgr);
        if (retv != RV_OK)
        {
            retv = RvErrorGetCode(retv);

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
            RvAddressGetString(pCfg->pLocalAddr, sizeof(strIp), strIp);
            TRANSPORT_LOG_ERR_5("%p: Failed to bind sock=%d to %s:%d (retv=%d)",
                pSockTransp, pSockTransp->sock, strIp, RvAddressGetIpPort(pCfg->pLocalAddr),retv);
#endif
            goto failed;
        }

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
        GetOption(pSockTransp, RVTRANSPORT_OPT_LOCALADDR, &boundAddr);
        RvAddressGetString(&boundAddr, sizeof(strIp), strIp);
        TRANSPORT_LOG_DBG_4("%p: sock=%d was bound to %s:%d",
            pSockTransp, pSockTransp->sock, strIp, RvAddressGetIpPort(&boundAddr));
        RvAddressDestruct(&boundAddr);
#endif
    }

    /* Construct Select Engine descriptor */
    retv = RvFdConstruct(&pSockTransp->fd, &pSockTransp->sock, logMgr);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_2("Failed to construct RvFd (sock=%d, retv=%d)",
            pSockTransp->sock, retv);
        goto failed;
    }
    RvFdSetUserContext(&pSockTransp->fd, pSockTransp);
    fdConstructed = RV_TRUE;

    /* Start to listen for incoming connections if applicable.
    */
    if (pCfg->options & RVTRANSPORT_CREATEOPT_LISTENING)
    {
        if (pCfg->protocol == RvSocketProtocolUdp)
        {
            TRANSPORT_LOG_WARN_1("RVTRANSPORT_CREATEOPT_LISTENING option was suppressed for UDP sock %d", pSockTransp->sock);
        }
        else
        {
            retv = RvSocketListen(&pSockTransp->sock,SOCKTRANSP_CFG_SOCKQUEUELEN,logMgr);
            if (retv != RV_OK)
            {
                retv = RvErrorGetCode(retv);
                TRANSPORT_LOG_ERR_2("RvSocketListen failed(sock=%d,retv=%d)",
                    pSockTransp->sock, retv);
                goto failed;
            }
        }
    }

    if (pCfg->options & RVTRANSPORT_CREATEOPT_CLOSEEVENTTIMEOUT)
    {
        pSockTransp->closeEvTimeout = SOCKTRANSP_CFG_CLOSEEVENTTO;
    }
    RvTimerConstruct(&pSockTransp->timer);

    /* Construct object lock */
#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    retv = RvMutexConstruct(logMgr, &pSockTransp->mutex);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_1("failed to construct mutex(retv=%d)", retv);
        goto failed;
    }
#endif /* #if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE) */

    TRANSPORT_LOG_DBG_4("%p RvSocketTransport was constructed: sock=%d, status=0x%x, options=0x%x",
        pSockTransp, pSockTransp->sock, pSockTransp->status, pSockTransp->options);

    return RV_OK;


failed:
    if (fdConstructed == RV_TRUE)
    {
        RvFdDestruct(&pSockTransp->fd);
    }
    if (sockConstructed == RV_TRUE)
    {
        RvSocketDestruct(&pSockTransp->sock, RV_FALSE /*cleanSocket*/, NULL /*portRange*/, logMgr);
    }

    return retv;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspDestruct(
                    IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvSockTranspDestruct"
    RvSocketTransport* pSockTransp = (RvSocketTransport*)transp;
    TRANSPORT_DEFINE_LOG_MGR(transp);

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,SOCKTRANSP_STATUS_DESTRUCTING,FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }
    pSockTransp->status |= SOCKTRANSP_STATUS_DESTRUCTING;

    TRANSPORT_LOG_DBG_4("%p: sock=%d, options=0x%x, status=0x%x",
        pSockTransp, pSockTransp->sock, pSockTransp->options, pSockTransp->status);

    /* Remove from Select, if waiting for events.
       Use the SafeSelectRemove that waits for the Select thread to return from
       the SelectCB in order to prevent object destructing, when the object
       is being used in SelectCB. (see usage of WAIT_FOR_CALLBACK_COMPLETION).
    */
    if (pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS)
    {
        SafeSelectRemove(pSockTransp);
    }

    /* Cancel CLOSE event timer, if the last is running.
       Use the SafeTimerCancel that waits for the Timer thread to return from
       the Timer Callback in order to prevent object destructing, when the object
       is being used in the Timer Callback
       (see usage of RV_TIMER_CANCEL_WAIT_FOR_CB).
    */
    if (pSockTransp->status & SOCKTRANSP_STATUS_CLOSETIMERRUNS)
    {
        SafeTimerCancel(pSockTransp);
    }

    /* Now we can try to Destruct the Transport finally.
    */

    /* Unlock the Transport, since it is not in use by anyone now
    */
    TRANSPORT_UNLOCK(pSockTransp);

    RvTimerDestruct(&pSockTransp->timer);

    if (pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS)
        RvSelectRemove(pSockTransp->pSelectEngine, &pSockTransp->fd);
    RvFdDestruct(&pSockTransp->fd);

    if (pSockTransp->sock != RV_INVALID_SOCKET  &&
        !(pSockTransp->options & RVTRANSPORT_CREATEOPT_DONTCLOSESOCK))
    {
        RvSocketDestruct(&pSockTransp->sock, RV_FALSE/*cleanSocket*/, NULL /*portRange*/, logMgr);
    }

#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    RvMutexDestruct(&pSockTransp->mutex,logMgr);
#endif

    TRANSPORT_LOG_INFO_1("%p: transport was destructed", pSockTransp);
    return RV_OK;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspConnect(
                    IN  RvTransport             transp,
                    IN  RvAddress*              pRemoteAddr)
{
#define FUNCTION_NAME "RvSockTranspConnect"
    RvSocketTransport*    pSockTransp = (RvSocketTransport*)transp;
    RvStatus            retv;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvChar strIp[64] = {'\0'};
    RvAddressGetString(pRemoteAddr, sizeof(strIp), strIp);

    TRANSPORT_LOG_DBG_4("%p(sock=%d): connecting to %s:%d",
        pSockTransp, pSockTransp->sock, strIp, RvAddressGetIpPort(pRemoteAddr));
#endif

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp, SOCKTRANSP_STATUS_DESTRUCTING, FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (pSockTransp->sockProto == RvSocketProtocolUdp)
    {
        TRANSPORT_LOG_ERR_2("%p(sock=%d): can't connect UDP socket",
            pSockTransp, pSockTransp->sock);
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_UNKNOWN;
    }

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)  || \
    (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
    if (pSockTransp->options & RVTRANSPORT_CREATEOPT_NONBLOCKING)
    {
        retv = RegisterEvent(pSockTransp, RVTRANSPORT_EVENT_CONNECT);
        if (retv != RV_OK)
        {
            TRANSPORT_LOG_ERR_3("%p(sock=%d): failed to register event(retv=%d)",
                                pSockTransp, pSockTransp->sock, retv);
            TRANSPORT_UNLOCK(transp);
            return RV_ERROR_UNKNOWN;
        }
    }
#endif

    retv = RvSocketConnect(&pSockTransp->sock, pRemoteAddr, logMgr);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): failed to connect(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_UNKNOWN;
    }

#if (RV_OS_TYPE != RV_OS_TYPE_WIN32) && (RV_OS_TYPE != RV_OS_TYPE_NUCLEUS)  && \
    (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN)
    if (pSockTransp->options & RVTRANSPORT_CREATEOPT_NONBLOCKING)
    {
        retv = RegisterEvent(pSockTransp, RVTRANSPORT_EVENT_CONNECT);
        if (retv != RV_OK)
        {
            TRANSPORT_LOG_ERR_3("%p(sock=%d): failed to register event(retv=%d)",
                pSockTransp, pSockTransp->sock, retv);
            TRANSPORT_UNLOCK(transp);
            return RV_ERROR_UNKNOWN;
        }
    }
#endif

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspAccept(
                    IN  RvTransport         transp,
                    IN  RvUint32            options,
                    OUT RvTransport*        pNewTransp,
                    OUT RvAddress*          pRemoteAddr)
{
#define FUNCTION_NAME "RvSockTranspAccept"
    RvSocketTransport*      pSockTransp = (RvSocketTransport*)transp;
    RvSocket              newSock;
    RvStatus              retv;
    RvTransportSocketCfg  cfg;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    *pNewTransp = NULL;

    if (options & 0xffff0000)
    {
        TRANSPORT_LOG_ERR_2("%p: Bad create-time options were provided: 0x%x. The RVTRANSPORT_CREATEOPT_XXX options were expected",
            transp, options);
        return RV_ERROR_BADPARAM;
    }

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp, SOCKTRANSP_STATUS_DESTRUCTING, FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    /* Accept connection */
    retv = RvSocketAccept(&pSockTransp->sock, logMgr, &newSock, pRemoteAddr);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSelectAccept failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    /* Create Transport for the accepted connection */
    RvTransportInitSocketTransportCfg(&cfg);
    cfg.protocol        = pSockTransp->sockProto;
    cfg.sock            = newSock;
    cfg.options         = options;
    cfg.pSelectEngine   = pSockTransp->pSelectEngine;
    cfg.pLogMgr         = pSockTransp->pLogMgr;
    cfg.callbacks       = pSockTransp->base.callbacks;

    retv = RvMemoryAlloc(NULL, sizeof(RvSocketTransport), logMgr, (void**)pNewTransp);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_2("%p(sock=%d): failed in RvMemoryAlloc",
            pSockTransp, pSockTransp->sock);
        RvSocketDestruct(&newSock, RV_FALSE /*clean*/, NULL /*port range*/, logMgr);
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_OUTOFRESOURCES;
    }
    memset(*pNewTransp,0,sizeof(RvSocketTransport));

    retv = RvSockTranspConstruct(&cfg, *pNewTransp);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_3("%p(sock=%d): failed to create new transport(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        RvSocketDestruct(&newSock, RV_FALSE /*clean*/, NULL /*port range*/, logMgr);
        RvMemoryFree(*pNewTransp,logMgr);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    retv = SetCreationOptions((RvSocketTransport*)(*pNewTransp), options);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_2("failed to set 0x%x options (sock=%d, retv=%d)",
            pSockTransp->sock, retv);
        RvSockTranspDestruct(*pNewTransp);
        RvMemoryFree(*pNewTransp,logMgr);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    /* Mark transport as connected in order to enable shutdown on destruct */
    ((RvSocketTransport*)(*pNewTransp))->status |= SOCKTRANSP_STATUS_CONNECTED;

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    {
        RvChar      strIp[64] = {'\0'};
        RvUint16    port;

        RvAddressGetString(pRemoteAddr, sizeof(strIp), strIp);
        port = RvAddressGetIpPort(pRemoteAddr);

        TRANSPORT_LOG_DBG_6("%p(sock=%d): accepted connection from %s:%d, new Transport=%p(sock=%d)",
            pSockTransp, pSockTransp->sock, strIp, port, *pNewTransp, newSock);
    }
#endif

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspShutdown(
                       IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvSockTranspShutdown"
    RvSocketTransport*    pSockTransp = (RvSocketTransport*)transp;
    RvStatus            retv;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
                        (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR |
                        SOCKTRANSP_STATUS_SHUTTINGDOWN), FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (RV_FALSE == IsShutdownable(pSockTransp))
    {
        TRANSPORT_LOG_DBG_2("%p(sock=%d): the socket can't be shutdown. Return.",
            pSockTransp, pSockTransp->sock);
        TRANSPORT_UNLOCK(transp);
        return RV_OK;
    }

    pSockTransp->status |= SOCKTRANSP_STATUS_SHUTTINGDOWN;

    retv = RvSocketShutdown(&pSockTransp->sock, RV_FALSE /*cleanSocket*/, logMgr);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSocketShutdown failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    TRANSPORT_LOG_DBG_3("%p(sock=%d): socket was shutdown(status=0x%x)",
        pSockTransp, pSockTransp->sock, pSockTransp->status);

    /* There is nothing to be done anymore for blocking socket  */
    if (!(pSockTransp->options & RVTRANSPORT_CREATEOPT_NONBLOCKING))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_OK;
    }

    /* For non-blocking socket start timer to handle timeout for CLOSE event */
    if (pSockTransp->closeEvTimeout != 0  &&  pSockTransp->pSelectEngine != NULL)
    {
        RvTimerQueue* tqueue;
        RvInt64       int64delay = RvInt64FromRvInt32(pSockTransp->closeEvTimeout);

        /* Find out timer queue handle */
        retv = RvSelectGetTimeoutInfo(pSockTransp->pSelectEngine, NULL, &tqueue);
        if (retv != RV_OK)
        {
            retv = RvErrorGetCode(retv);
            TRANSPORT_LOG_ERR_2("%p: Failed to get Timer Queue(retv=%d)",
                pSockTransp, retv);
            TRANSPORT_UNLOCK(transp);
            return retv;
        }

        /* Run timer */
        retv = RvTimerStartEx(&pSockTransp->timer, tqueue, RV_TIMER_TYPE_ONESHOT,
                              int64delay, TimerCloseEventCB, (void*)pSockTransp);
        if (retv != RV_OK)
        {
            retv = RvErrorGetCode(retv);
            TRANSPORT_LOG_ERR_2("%p: Failed to start CLOSE event timer(retv=%d)",
                pSockTransp, retv);
            TRANSPORT_UNLOCK(transp);
            return retv;
        }

        pSockTransp->status |= SOCKTRANSP_STATUS_CLOSETIMERRUNS;
    }

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RVCOREAPI RvStatus RvSockTranspSendBuffer(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t                len,
                    IN  RvAddress*              pDestAddr,
                    IN  RvUint32                options,
                    OUT RvSize_t*               pSent)
{
#define FUNCTION_NAME "RvSockTranspSendBuffer"
    RvSocketTransport* pSockTransp = (RvSocketTransport*)transp;
    RvStatus         retv;
    RvChar           strIp[64] = {'\0'};
    RvUint16         port = 0;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    RV_UNUSED_ARG(options);

    if (pSent)
        *pSent = 0;

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
        (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR |
        SOCKTRANSP_STATUS_SHUTTINGDOWN), FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (pDestAddr != NULL)
    {
        RvAddressGetString(pDestAddr, sizeof(strIp), strIp);
        port = RvAddressGetIpPort(pDestAddr);
    }
    else
    {
        if (pSockTransp->sockProto == RvSocketProtocolUdp)
        {
            TRANSPORT_LOG_ERR_2("%p(sock=%d): no destination address for UDP",
                pSockTransp, pSockTransp->sock);
            TRANSPORT_UNLOCK(transp);
            return RV_ERROR_BADPARAM;
        }
    }

    TRANSPORT_LOG_DBG_6("%p(sock=%d): buff=%p,buffLen=%d,addr=%s:%d",
        pSockTransp, pSockTransp->sock, buff, len, strIp, port);

    retv = RvSocketSendBuffer(&pSockTransp->sock, buff, len, pDestAddr, logMgr, pSent);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSocketSendManyBuffers failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RVCOREAPI
RvStatus RVCALLCONV RvSockTranspSendManyBuffers(
                    IN  RvTransport             transp,
                    IN  RvUint32                numOfBuff,
                    IN  RvUint8**               ppBuff,
                    IN  RvSize_t*               pLen,
                    IN  RvAddress*              pDestAddr,
                    IN  RvUint32                options,
                    OUT RvSize_t*               pSent)
{
#define FUNCTION_NAME "RvSockTranspSendManyBuffers"
    RvSocketTransport*  pSockTransp = (RvSocketTransport*)transp;
    RvStatus            retv;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvChar              strIp[64] = {'\0'};
    RvUint16            port = 0;
#endif /*#if (RV_LOGMASK != RV_LOGLEVEL_NONE)*/
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    RV_UNUSED_ARG(options);

    if (pSent)
        *pSent = 0;

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
                        (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR |
                         SOCKTRANSP_STATUS_SHUTTINGDOWN), FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (pDestAddr != NULL)
    {
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
        RvAddressGetString(pDestAddr, sizeof(strIp), strIp);
        port = RvAddressGetIpPort(pDestAddr);
#endif /*#if (RV_LOGMASK != RV_LOGLEVEL_NONE)*/
    }
    else
    {
        if (pSockTransp->sockProto == RvSocketProtocolUdp)
        {
            TRANSPORT_LOG_ERR_2("%p(sock=%d): no destination address for UDP",
                pSockTransp, pSockTransp->sock);
            TRANSPORT_UNLOCK(transp);
            return RV_ERROR_BADPARAM;
        }
    }

    TRANSPORT_LOG_DBG_6("%p(sock=%d): buff=%p,buffLen=%d,addr=%s:%d",
        pSockTransp, pSockTransp->sock, ppBuff[0], pLen[0], strIp, port);

    if (numOfBuff == 1)
    {
        retv = RvSocketSendBuffer(&pSockTransp->sock, ppBuff[0], pLen[0],
                                  pDestAddr, logMgr, pSent);
    }
    else
    {
        retv = RvSocketSendManyBuffers(&pSockTransp->sock, numOfBuff, ppBuff, pLen,
                                       pDestAddr, logMgr, pSent);
    }
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSocketSendManyBuffers failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspReceiveBuffer(
                    IN  RvTransport             transp,
                    IN  RvUint8*                buff,
                    IN  RvSize_t              	len,
                    IN  RvUint32                options,
                    IN  RvAddress*              pRcvdFromAddr,
                    OUT RvSize_t*               pReceived)
{
#define FUNCTION_NAME "RvSockTranspReceiveBuffer"
    RvSocketTransport* pSockTransp = (RvSocketTransport*)transp;
    RvStatus        retv;
    TRANSPORT_DEFINE_LOG_MGR(transp);

    RV_UNUSED_ARG(options);

    if (pReceived)
        *pReceived = 0;

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
                    (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR),
                    FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    TRANSPORT_LOG_DBG_4("%p(sock=%d): buff=%p, buffLen=%d",
        pSockTransp, pSockTransp->sock, buff, len);

    retv = RvSocketReceiveBuffer(&pSockTransp->sock, buff, len, logMgr, pReceived, pRcvdFromAddr);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSocketReceiveBuffer failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    if (pRcvdFromAddr != NULL)
    {
        RvChar    strIp[64] = {'\0'};
        RvUint16  port = 0;

        RvAddressGetString(pRcvdFromAddr, sizeof(strIp), strIp);
        port = RvAddressGetIpPort(pRcvdFromAddr);

        TRANSPORT_LOG_DBG_6("%p(sock=%d): buff=%p,received=%d, addr=%s:%d",
            pSockTransp, pSockTransp->sock, buff, (pReceived)?*pReceived:0, strIp, port);
    }
    else
    {
        TRANSPORT_LOG_DBG_4("%p(sock=%d): buff=%p,received=%d",
            pSockTransp, pSockTransp->sock, buff, (pReceived)?*pReceived:0);
    }
#endif

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RVCOREAPI RvStatus RvSockTranspGetBytesAvailable(
    IN  RvTransport             transp,
    OUT RvSize_t*               bytesAvailable)
{
#define FUNCTION_NAME "RvSockTranspGetBytesAvailable"
    RvSocketTransport* pSockTransp = (RvSocketTransport*)transp;
    RvStatus        retv;
    TRANSPORT_DEFINE_LOG_MGR(transp);

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
        (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR),
        FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    retv = RvSocketGetBytesAvailable(&pSockTransp->sock, logMgr, bytesAvailable);

    TRANSPORT_LOG_DBG_4("%p(sock=%d): retv=%d,bytesAvailable=%d",
        pSockTransp, pSockTransp->sock, retv, *bytesAvailable);

    TRANSPORT_UNLOCK(transp);
    return retv;
#undef FUNCTION_NAME
}


RVCOREAPI
RvStatus RVCALLCONV RvSockTranspReceiveManyBuffers(
                    IN  RvTransport             transp,
                    IN  RvUint32                numOfBuff,
                    IN  RvUint8**               ppBuff,
                    IN  RvSize_t*              	pLen,
                    IN  RvUint32                options,
                    IN  RvAddress*              pRcvdFromAddr,
                    OUT RvSize_t*               pReceived)
{
#define FUNCTION_NAME "RvSockTranspReceiveManyBuffers"
    RvSocketTransport* pSockTransp = (RvSocketTransport*)transp;
    RvStatus        retv;
    TRANSPORT_DEFINE_LOG_MGR(transp);

    RV_UNUSED_ARG(options);

    if (pReceived)
        *pReceived = 0;

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
                        (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR),
                        FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    TRANSPORT_LOG_DBG_4("%p(sock=%d): buff=%p, buffLen=%d",
        pSockTransp, pSockTransp->sock, ppBuff[0], pLen[0]);

    if (numOfBuff == 1)
    {
        retv = RvSocketReceiveBuffer(&pSockTransp->sock, ppBuff[0], pLen[0],
                                     logMgr, pReceived, pRcvdFromAddr);
    }
    else
    {
        retv = RvSocketReceiveManyBuffers(&pSockTransp->sock, numOfBuff, ppBuff,
                                          pLen, logMgr, pReceived, pRcvdFromAddr);
    }
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSocketReceiveManyBuffers failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    if (pRcvdFromAddr != NULL)
    {
        RvChar    strIp[64] = {'\0'};
        RvUint16  port = 0;

        RvAddressGetString(pRcvdFromAddr, sizeof(strIp), strIp);
        port = RvAddressGetIpPort(pRcvdFromAddr);

        TRANSPORT_LOG_DBG_6("%p(sock=%d): buff=%p,received=%d, addr=%s:%d",
            pSockTransp, pSockTransp->sock, ppBuff[0], (pReceived)?*pReceived:0, strIp, port);
    }
    else
    {
        TRANSPORT_LOG_DBG_4("%p(sock=%d): buff=%p,received=%d",
            pSockTransp, pSockTransp->sock, ppBuff[0], (pReceived)?*pReceived:0);
    }
#endif

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspRegisterEvent(
                    IN RvTransport       transp,
                    IN RvTransportEvents events)
{
#define FUNCTION_NAME "RvSockTranspRegisterEvent"
    RvSocketTransport*   pSockTransp = (RvSocketTransport*)transp;
    RvStatus          retv;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
                        (SOCKTRANSP_STATUS_DESTRUCTING),
                        FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (pSockTransp->pSelectEngine == NULL)
    {
        TRANSPORT_LOG_ERR_1("%p: select engine was not set", pSockTransp);
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_UNKNOWN;
    }

    retv = RegisterEvent(pSockTransp, events);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_3("%p: failed to register events 0x%x (retv=%d)",
            pSockTransp, events, retv);
        TRANSPORT_UNLOCK(transp);
        return retv;
    }

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

void RvSockTranspAddRef(
                            IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvSockTranspAddRef"
    RvSocketTransport*   pSockTransp = (RvSocketTransport*)transp;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    TRANSPORT_LOCK_VOID(transp);
    pSockTransp->base.refCounter++;
    TRANSPORT_LOG_INFO_2("%p: Updated Reference Counter: %d", pSockTransp, pSockTransp->base.refCounter);
    TRANSPORT_UNLOCK(transp);

#undef FUNCTION_NAME
}

void RvSockTranspRelease(
                            IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvSockTranspRelease"
    RvSocketTransport*   pSockTransp = (RvSocketTransport*)transp;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    TRANSPORT_LOCK_VOID(transp);

    pSockTransp->base.refCounter--;

    TRANSPORT_LOG_INFO_2("%p: Updated Reference Counter: %d", pSockTransp, pSockTransp->base.refCounter);

    if (pSockTransp->base.refCounter < 0)
    {
        TRANSPORT_LOG_EXCEP_2("%p: Negative reference count !!!! %d", pSockTransp, pSockTransp->base.refCounter);
    }


    if (pSockTransp->base.refCounter == 0)
    {
        TRANSPORT_UNLOCK(transp);
        transp->iface->pfnDestruct(transp);
        RvMemoryFree((void*)transp, NULL/*logMgr*/);
    }
    else
    {
        TRANSPORT_UNLOCK(transp);
    }

#undef FUNCTION_NAME
}

RvStatus RvSockTranspSetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    IN  void*               val)
{
#define FUNCTION_NAME "RvSockTranspSetOption"
    RvSocketTransport*    pSockTransp = (RvSocketTransport*)transp;
    RvStatus            retv = RV_OK;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp,
                        (SOCKTRANSP_STATUS_DESTRUCTING | SOCKTRANSP_STATUS_SOCKERROR |
                         SOCKTRANSP_STATUS_SHUTTINGDOWN), FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (type != RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT)
    {
        TRANSPORT_LOG_ERR_4("%p(sock=%d): bad option type (%d), the RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT is expected",
            pSockTransp, pSockTransp->sock, type, RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT);
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_BADPARAM;
    }

    retv = SetOption(pSockTransp, option, val);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_4("%p(sock=%d): failed for option %d(retv=%d)",
            pSockTransp, pSockTransp->sock, option, retv);
    }
    else
    {
        TRANSPORT_LOG_DBG_4("%p(sock=%d): option %d was set (updated options: 0x%x)",
            pSockTransp, pSockTransp->sock, option, pSockTransp->options);
    }

    TRANSPORT_UNLOCK(transp);
    return retv;

#undef FUNCTION_NAME
}

RvStatus RvSockTranspGetOption(
                    IN  RvTransport         transp,
                    IN  RvUint32            type,
                    IN  RvUint32            option,
                    OUT void*               val)
{
#define FUNCTION_NAME "SockTranspGetOption"
    RvSocketTransport*   pSockTransp = (RvSocketTransport*)transp;
    RvStatus           retv = RV_OK;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    TRANSPORT_LOCK(transp);
    if (RV_TRUE == ForbidFunctionCall(pSockTransp, SOCKTRANSP_STATUS_DESTRUCTING, FUNCTION_NAME))
    {
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_ILLEGAL_ACTION;
    }

    if (type != RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT)
    {
        TRANSPORT_LOG_ERR_4("%p(sock=%d): bad option collection (%d), the RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT is expected",
            pSockTransp, pSockTransp->sock, type, RVTRANSPORT_OPTTYPE_SOCKETTRANSPORT);
        TRANSPORT_UNLOCK(transp);
        return RV_ERROR_BADPARAM;
    }

    retv = GetOption(pSockTransp, option, val);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_4("%p(sock=%d): failed for option %d(retv=%d)",
            pSockTransp, pSockTransp->sock, option, retv);
    }

    TRANSPORT_UNLOCK(transp);
    return RV_OK;

#undef FUNCTION_NAME
}

/*----------------------------------------------------------------------------*/
/*                        STATIC FUNCTION IMPLEMENTATIONS                     */
/*----------------------------------------------------------------------------*/
/*******************************************************************************
 * ConvertSelectEvent2SockTranspEvent
 * -----------------------------------------------------------------------------
 * General:
 *  Converts Select Event value into the Transport Event value.
 *
 * Arguments:
 * Input:   selectEvent - The Select Event value.
 * Output:  none.
 *
 * Return Value: The Transport Event value.
 ******************************************************************************/
static
RvTransportEvents ConvertSelectEvent2SockTranspEvent(IN RvInt selectEvent)
{
    switch (selectEvent)
    {
        case RvSelectRead:      return RVTRANSPORT_EVENT_READ;
        case RvSelectWrite:     return RVTRANSPORT_EVENT_WRITE;
        case RvSelectAccept:    return RVTRANSPORT_EVENT_ACCEPT;
        case RvSelectConnect:   return RVTRANSPORT_EVENT_CONNECT;
        case RvSelectClose:     return RVTRANSPORT_EVENT_CLOSE;
        default:
            return 0;
    }
}

/*******************************************************************************
 * ConstructSocket
 * -----------------------------------------------------------------------------
 * General:
 *  Constructs the RvSocket object, which causes the socket() system call,
 *  and applies various CREATEOPT options to the created system socket.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object, which owns the RvSocket.
 * Output:  none.
 *
 * Return Value: RV_OK on success, RV_ERROR_XXX otherwise.
 ******************************************************************************/
static
RvStatus ConstructSocket(
                IN RvSocketTransport*  pSockTransp,
                IN RvUint32            options)
{
#define FUNCTION_NAME "ConstructSocket"
    RvStatus   retv;
    RvInt      addrType;
    RvInt      sockOptions;

    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    addrType = (options & RVTRANSPORT_CREATEOPT_IPV6SOCKET) ?
               RV_ADDRESS_TYPE_IPV6 : RV_ADDRESS_TYPE_IPV4;

    /* Ensure the IPv6 is supported */
    if (addrType == RV_ADDRESS_TYPE_IPV6)
    {
#if !(RV_NET_TYPE & RV_NET_IPV6)
        TRANSPORT_LOG_ERR("RVTRANSPORT_OPT_SOCK_IPV6 is not supported. Please compile with IPv6");
        return RV_ERROR_BADPARAM;
#endif
    }

    /* Prepare options */
    sockOptions = 0;
    if (options & RVTRANSPORT_CREATEOPT_REUSEADDR)
        sockOptions |= RV_SOCKET_OPT_REUSE_ADDR;
    if (options & RVTRANSPORT_CREATEOPT_NOTCPLINGER)
        sockOptions |= RV_SOCKET_OPT_LINGER;
    if (options & RVTRANSPORT_CREATEOPT_NODELAY)
        sockOptions |= RV_SOCKET_OPT_TCP_DELAY;



    /* Construct socket */
    retv = RvSocketConstruct2(addrType, pSockTransp->sockProto, sockOptions,
                              logMgr, &pSockTransp->sock);
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_1("failed to construct socket (retv=%d)", retv);
        return retv;
    }

    retv = SetCreationOptions(pSockTransp, options);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_2("failed to set 0x%x options (sock=%d, retv=%d)",
            pSockTransp->sock, retv);
        RvSocketDestruct(&pSockTransp->sock, RV_FALSE /*cleanSocket*/, NULL /*portRange*/, logMgr);
        return retv;
    }

    return RV_OK;
#undef FUNCTION_NAME
}

/*******************************************************************************
 * SetCreationOptions
 * -----------------------------------------------------------------------------
 * General:
 *  Applies various CREATEOPT options to the system socket.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object, which owns the RvSocket.
 *          options     - Options to be applied to the system socket.
 * Output:  none.
 *
 * Return Value: RV_OK on success, RV_ERROR_XXX otherwise.
 ******************************************************************************/
static
RvStatus SetCreationOptions(
                    IN RvSocketTransport* pSockTransp,
                    IN RvUint32           options)
{
#define FUNCTION_NAME "SetCreationOptions"
    RvStatus   retv;
    RvBool     bIsNonBlocking;

    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    /* Set blocking mode */
    bIsNonBlocking = (options & RVTRANSPORT_CREATEOPT_NONBLOCKING) ? RV_TRUE : RV_FALSE;
    retv = SetOption(pSockTransp, RVTRANSPORT_OPT_SOCK_NONBLOCKING,
        (void*)&bIsNonBlocking);
    if (retv != RV_OK)
    {
        TRANSPORT_LOG_ERR_2("failed to set blocking mode (sock=%d, retv=%d)",
            pSockTransp->sock, retv);
        return retv;
    }

    /* Set broadcast */
    if (options & RVTRANSPORT_CREATEOPT_BROADCAST)
    {
        RvBool bBroadcastEnabled = RV_TRUE;
        retv = SetOption(pSockTransp, RVTRANSPORT_OPT_SOCK_BROADCAST,
            (void*)&bBroadcastEnabled);
        if (retv != RV_OK)
        {
            TRANSPORT_LOG_ERR_2("failed to set broadcast(sock=%d, retv=%d)",
                pSockTransp->sock, retv);
            return retv;
        }
    }

    return RV_OK;
#undef FUNCTION_NAME

}

/*******************************************************************************
 * ForbidFunctionCall
 * -----------------------------------------------------------------------------
 * General:
 *  Checks if the status of the Transport enables to perform operation,
 *  invoked by the user, that can't be performed when the object is in
 *  the 'forbiddingStatus' status.
 *
 * Arguments:
 * Input:   pSockTransp      - The Transport object.
 *          forbiddingStatus - The status that forbids the user operation.
 *          strFuncName      - The name of the user operation.
 * Output:  none.
 *
 * Return Value: RV_TRUE, if the operation can't be performed.
 ******************************************************************************/
static
RvBool ForbidFunctionCall(
                    IN RvSocketTransport*   pSockTransp,
                    IN RvUint32             forbiddingStatus,
                    IN const RvChar*        strFuncName)
{
#define FUNCTION_NAME "ForbidFunctionCall"

    RvBool forbid;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    forbid = (pSockTransp->status & forbiddingStatus) ? RV_TRUE : RV_FALSE;
    if (forbid == RV_TRUE)
    {
        TRANSPORT_LOG_ERR_3("%p: the %s call is forbidden due to status 0x%x",
            pSockTransp, strFuncName, pSockTransp->status);
    }

    RV_UNUSED_ARG(strFuncName);
    return forbid;
#undef FUNCTION_NAME
}

/*******************************************************************************
 * SelectCB
 * -----------------------------------------------------------------------------
 * General:
 *  The callback, called by the Select Engine, when some event occurs on
 *  on the Transport socket. The event is translated to the Transport Event and
 *  is exposed to the user, using the pfnEvent user callback.
 *  If error occurred on socket (see the 'error' parameter), the status of
 *  the Transport is updated in such way, that further sending / receiving
 *  will not be possible anymore. The user has to terminate the object.
 *
 * Arguments:
 * Input:   selectEngine - The Select Engine.
 *          fd           - The RvSelectFd structure, owned by the Transport.
 *          selectEvent  - The occurred event.
 *          error        - RV_TRUE, if error occurred on the socket.
 * Output:  none.
 *
 * Return Value: none.
 ******************************************************************************/
static
void SelectCB(
                    IN RvSelectEngine*  selectEngine,
                    IN RvSelectFd*      fd,
                    IN RvSelectEvents   selectEvent,
                    IN RvBool           error)
{
#define FUNCTION_NAME "SelectCB"

    RvSocketTransport*  pSockTransp = RvFdGetUserContext(fd);
    RvTransportEvents   event;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    RV_UNUSED_ARG(selectEngine);

    TRANSPORT_LOCK_VOID(pSockTransp);

    TRANSPORT_LOG_DBG_5("%p(sock=%d): event %s, error=%d, status=0x%x",
        pSockTransp, pSockTransp->sock, ConvertSelectEvent2Str(selectEvent), error, pSockTransp->status);

    if (!(pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS))
    {
        TRANSPORT_LOG_DBG_3("%p(sock=%d): Transport doesn't wait for events. Probably error occured on socket(status=0x%x)",
            pSockTransp, pSockTransp->sock, pSockTransp->status);
        TRANSPORT_UNLOCK(pSockTransp);
        return;
    }

    event = ConvertSelectEvent2SockTranspEvent(selectEvent);
    if (!(pSockTransp->events & event))
    {
        TRANSPORT_LOG_DBG_3("%p(sock=%d): Transport doesn't wait for %s",
            pSockTransp, pSockTransp->sock, ConvertSelectEvent2Str(selectEvent));
        TRANSPORT_UNLOCK(pSockTransp);
        return;
    }

    if ((pSockTransp->status & SOCKTRANSP_STATUS_DESTRUCTING) && selectEvent != RvSelectClose)
    {
        TRANSPORT_LOG_DBG_3("%p(sock=%d): Transport is being destroyed: ignore %s event",
            pSockTransp, pSockTransp->sock, ConvertSelectEvent2Str(selectEvent));
        TRANSPORT_UNLOCK(pSockTransp);
        return;
    }

    /* Handle the select event */
    switch (selectEvent)
    {
        case RvSelectConnect:
            /*Mark transport as CONNECTED to enable shutdown from destruct*/
            if (error == RV_FALSE)
            {
                pSockTransp->status |= SOCKTRANSP_STATUS_CONNECTED;
            }
            break;

        case RvSelectClose:
            {
                /* If CLOSE event was handled from within TimerCloseEventCB,
                   ignore this event.
                */
                if (pSockTransp->status & SOCKTRANSP_STATUS_CLOSEEVHANDLED)
                {
                    TRANSPORT_LOG_DBG_2("%p(sock=%d): ignore RvSelectClose event, since TimerCloseEvent fired",
                        pSockTransp, pSockTransp->sock);
                    TRANSPORT_UNLOCK(pSockTransp);
                    return;
                }

                pSockTransp->status |= SOCKTRANSP_STATUS_CLOSEEVHANDLED;

                /* Cancel Timer, if the last runs.
                   We can not to wait for the Timer callback finish, since
                   the callback will do nothing due to
                   SOCKTRANSP_STATUS_CLOSEEVHANDLED status, that was set above.
                */
                if (pSockTransp->status & SOCKTRANSP_STATUS_CLOSETIMERRUNS)
                {
                    RvTimerCancel(&pSockTransp->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
                }
            }
            break;

        default:
            break;
    } /* ENDOF: switch (selectEvent) */


    /* If error occurred on socket, the transport is not usable anymore.
       The user is expected to destruct the Transport as soon as possible after
       the event (+error) notification callback will be called.
       In order to prevent further events - remove Transport from select.
       Note the 'error' argument is not boolean! On Windows it provides
       the WSA error code! Therefore don't use if (error == RV_TRUE)!
    */
    if (error != RV_FALSE)
    {
        TRANSPORT_LOG_DBG_3("%p(sock=%d): socket reported error %d",
            pSockTransp, pSockTransp->sock, error);
        pSockTransp->status |= SOCKTRANSP_STATUS_SOCKERROR;
        RvSelectRemove(pSockTransp->pSelectEngine, &pSockTransp->fd);
        pSockTransp->status &= ~SOCKTRANSP_STATUS_WAITINGEVENTS;
        pSockTransp->events = 0;
    }


    /* Report the event to the user
    */
    if (pSockTransp->base.callbacks.pfnEvent == NULL)
    {
        TRANSPORT_LOG_ERR_2("%p(sock=%d): user didn't set Event callback, ignore event",
            pSockTransp, pSockTransp->sock);
        TRANSPORT_UNLOCK(pSockTransp);
        return;
    }
    CallEventCallback(pSockTransp, event, error);

    /* Don't unlock here, since the CallEventCallback unlocks the object.
    */
    /*
    TRANSPORT_UNLOCK(pSockTransp);
    */
    return;

#undef FUNCTION_NAME
}

/*******************************************************************************
 * TimerCloseEventCB
 * -----------------------------------------------------------------------------
 * General:
 *  The callback, called by the system, when the CLOSE event timer expires.
 *  The CLOSE event timer is run during the RvSockTranspShutdown call.
 *  It is needed on systems, where the CLOSE event is not supported,
 *  so the user can't determine if the shutdown was finished.
 *
 * Arguments:
 * Input:   ctx   - The Transport object.
 *          timer - not in use.
 * Output:  none.
 *
 * Return Value: is not used.
 ******************************************************************************/
static
RvBool TimerCloseEventCB(
               IN void*    ctx,
               IN RvTimer* timer)
{
#define FUNCTION_NAME "TimerCloseEventCB"

    RvSocketTransport*  pSockTransp = (RvSocketTransport*)ctx;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    RV_UNUSED_ARG(timer);

    TRANSPORT_LOCK(pSockTransp);

    TRANSPORT_LOG_DBG_3("%p(sock=%d): CLOSE event timer fired (%s msec)",
        pSockTransp, pSockTransp->sock, pSockTransp->closeEvTimeout);

    pSockTransp->status &= ~SOCKTRANSP_STATUS_CLOSETIMERRUNS;

    /* Notify the user of the CLOSE event, if the last was not generated by
       the network yet due to race condition between the timer thread and
       and the Select thread.
    */
    if (!(pSockTransp->status & SOCKTRANSP_STATUS_CLOSEEVHANDLED))
    {
        pSockTransp->status |= SOCKTRANSP_STATUS_CLOSEEVHANDLED;
        CallEventCallback(pSockTransp, RvSelectClose, RV_FALSE/*error*/);
        /* Don't unlock here, since the CallEventCallback unlocks the object
        */
    }
    else
    {
        TRANSPORT_UNLOCK(pSockTransp);
    }

    return RV_TRUE; /* The return value doesn't matter */

#undef FUNCTION_NAME
}

/*******************************************************************************
 * RegisterEvent
 * -----------------------------------------------------------------------------
 * General:
 *  Translates the requested Transport events to the Select events and registers
 *  the Transport socket with the Select Engine for these Select events.
 *  If the requested events are ZERO, the socket is removed from the Select
 *  Engine.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 *          events      - The bit mask of Transport events.
 * Output:  none.
 *
 * Return Value: RV_OK on success, RV_ERROR_XXX otherwise.
 ******************************************************************************/
static
RvStatus RegisterEvent(
            IN RvSocketTransport*   pSockTransp,
            IN RvTransportEvents events)
{
#define FUNCTION_NAME "RegisterEvent"
    RvStatus          retv = RV_OK;
    RvSelectEvents    selectEvents = 0;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    RvInt32 numLocks = 0;
#endif

    /* Translate Transport events to the Select events */
    if (events & RVTRANSPORT_EVENT_CONNECT)
    {
        selectEvents = RvSelectConnect;
    }
    else if (events & RVTRANSPORT_EVENT_ACCEPT)
    {
        selectEvents = RvSelectAccept;
    }
    else
    {
        if (events & RVTRANSPORT_EVENT_READ)
            selectEvents |= RvSelectRead;
        if (events & RVTRANSPORT_EVENT_WRITE)
            selectEvents |= RvSelectWrite;
        if (events & RVTRANSPORT_EVENT_CLOSE)
            selectEvents |= RvSelectClose;
    }

    TRANSPORT_LOG_DBG_6("%p(sock=%d): new events=0x%x, old events=0x%x, select events=0x%x, status=0x%x",
        pSockTransp, pSockTransp->sock, events, pSockTransp->events, selectEvents, pSockTransp->status);

    pSockTransp->events = events;

    if (pSockTransp->fd.events == selectEvents)
    {
        TRANSPORT_LOG_DBG_2("%p(sock=%d): set of select events was not changed. Return.",
            pSockTransp, pSockTransp->sock);
        return RV_OK;
    }

    /* Check, if transport should be removed from Select */
    if (selectEvents == 0)
    {
        if (pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS)
        {
            TRANSPORT_RELEASE(pSockTransp);
            RvSelectRemoveEx(pSockTransp->pSelectEngine, &pSockTransp->fd,
                RV_TRUE /*wait for callback completion*/, NULL /*pbWasInCallback*/);
            TRANSPORT_RESTORE(pSockTransp);
            pSockTransp->status &= ~SOCKTRANSP_STATUS_WAITINGEVENTS;
            TRANSPORT_LOG_INFO_2("%p(sock=%d): transport was removed from select",
                pSockTransp, pSockTransp->sock);
        }
        return RV_OK;
    }

    /* Add/update transport events to/in Select */
    if (pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS)
    {
        retv = RvSelectUpdate(pSockTransp->pSelectEngine, &pSockTransp->fd, selectEvents, SelectCB);
    }
    else
    {
        retv = RvSelectAdd(pSockTransp->pSelectEngine, &pSockTransp->fd, selectEvents, SelectCB);
    }
    if (retv != RV_OK)
    {
        retv = RvErrorGetCode(retv);
        TRANSPORT_LOG_ERR_3("%p(sock=%d): RvSelectAdd/RvSelectUpdate failed(retv=%d)",
            pSockTransp, pSockTransp->sock, retv);
        return retv;
    }

    pSockTransp->status |= SOCKTRANSP_STATUS_WAITINGEVENTS;

    return RV_OK;

#undef FUNCTION_NAME
}

/*******************************************************************************
 * IsShutdownable
 * -----------------------------------------------------------------------------
 * General:
 *  Checks, if the shutdown() system call can be called on the Transport socket.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 * Output:  none.
 *
 * Return Value: RV_TRUE, if the shutdown() can be called, RV_FALSE - o/w.
 ******************************************************************************/
static
RvBool IsShutdownable(
                      IN RvSocketTransport*   pSockTransp)
{
#define FUNCTION_NAME "IsShutdownable"
    RvBool isShutdownable;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    if (pSockTransp->sockProto != RvSocketProtocolTcp  ||
        (pSockTransp->options & RVTRANSPORT_CREATEOPT_DONTCLOSESOCK) ||
        (pSockTransp->status  & SOCKTRANSP_STATUS_SHUTTINGDOWN)     ||
        (pSockTransp->status  & SOCKTRANSP_STATUS_SOCKERROR)        ||
        !(pSockTransp->status & SOCKTRANSP_STATUS_CONNECTED))
    {
        isShutdownable = RV_FALSE;
    }
    else
    {
        isShutdownable = RV_TRUE;
    }

    TRANSPORT_LOG_DBG_5("%p: isShutdownable=%s (sockProto=%d, options=0x%x, status=0x%x)",
        pSockTransp, (isShutdownable?"TRUE":"FALSE"), pSockTransp->sockProto,
        pSockTransp->options, pSockTransp->status);

    return isShutdownable;

#undef FUNCTION_NAME
}

/*******************************************************************************
 * SetOption
 * -----------------------------------------------------------------------------
 * General:
 *  Internal implementation of the RvTransportSetOption function,
 *  which assumes the Transport in the state, where the options can be set.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 *          option      - The option to be set.
 *          val         - The option value.
 *                        The format of the value to be provided here is
 *                        option specific. Please see option definition
 *                        in the rvtransportsocket.h file for more info.
 * Output:  none.
 *
 * Return Value: RV_OK on success, RV_ERROR_XXX - otherwise.
 ******************************************************************************/
static
RvStatus SetOption(
            IN  RvSocketTransport*  pSockTransp,
            IN  RvUint32            option,
            IN  void*               val)
{
#define FUNCTION_NAME "SetOption"
    RvStatus retv = RV_OK;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    switch(option)
    {
        case RVTRANSPORT_OPT_SELECTENGINE:
            if ((pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS) &&
                (val == NULL || val != (void*)pSockTransp->pSelectEngine))
            {
                if (pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS)
                {
                    RvSelectRemoveEx(pSockTransp->pSelectEngine, &pSockTransp->fd,
                        RV_FALSE /*waitForCallbackCompletion*/, NULL /*pbWasInCallback*/);
                    pSockTransp->status &= ~SOCKTRANSP_STATUS_WAITINGEVENTS;
                    pSockTransp->events = 0;
                }
            }
            pSockTransp->pSelectEngine = (RvSelectEngine*)val;
            break;

        case RVTRANSPORT_OPT_CLOSEEVENTTIMEOUT:
            pSockTransp->closeEvTimeout = *(RvInt32*)val;
            break;

        case RVTRANSPORT_OPT_SOCK_NONBLOCKING:
            {
                RvBool isBlocking = (*(RvBool*)val == RV_TRUE) ? RV_FALSE : RV_TRUE;
                retv = RvSocketSetBlocking(&pSockTransp->sock, isBlocking, logMgr);
                if (retv == RV_OK)
                {
                    pSockTransp->options = (isBlocking == RV_FALSE) ?
                        pSockTransp->options | RVTRANSPORT_CREATEOPT_NONBLOCKING :
                        pSockTransp->options & ~RVTRANSPORT_CREATEOPT_NONBLOCKING;
                }
            }
            break;

        case RVTRANSPORT_OPT_DONTCLOSESOCK:
            pSockTransp->options = (*(RvBool*)val == RV_TRUE) ?
                        pSockTransp->options | RVTRANSPORT_CREATEOPT_DONTCLOSESOCK :
                        pSockTransp->options & ~RVTRANSPORT_CREATEOPT_DONTCLOSESOCK;
            break;

        case RVTRANSPORT_OPT_SOCK_BUFSIZE:
            {
                RvInt32* sizes = (RvInt32*)val;
                retv = RvSocketSetBuffers(&pSockTransp->sock, sizes[0], sizes[1], logMgr);
            }
            break;

        case RVTRANSPORT_OPT_SOCK_BROADCAST:
            retv = RvSocketSetBroadcast(&pSockTransp->sock, *(RvBool*)val, logMgr);
            break;

        case RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_JOIN:
            {
                RvAddress* mcastAddr = ((RvAddress**)val)[0];
                RvAddress* ifaceAddr = ((RvAddress**)val)[1];

                retv = RvSocketJoinMulticastGroup(
                            &pSockTransp->sock, mcastAddr, ifaceAddr, logMgr);
            }
            break;

        case RVTRANSPORT_OPT_SOCK_MULTICASTGROUP_LEAVE:
            {
                RvAddress* mcastAddr = ((RvAddress**)val)[0];
                RvAddress* ifaceAddr = ((RvAddress**)val)[1];

                retv = RvSocketLeaveMulticastGroup(
                            &pSockTransp->sock, mcastAddr, ifaceAddr, logMgr);
            }
            break;

        case RVTRANSPORT_OPT_SOCK_MULTICASTINTERFACE:
            retv = RvSocketSetMulticastInterface(&pSockTransp->sock,(RvAddress*)val,logMgr);
            break;

        case RVTRANSPORT_OPT_SOCK_MULTICASTNOLOOP:
            retv = RvSocketSetMulticastLoop(&pSockTransp->sock, *(RvBool*)val, logMgr);
            break;

        case RVTRANSPORT_OPT_SOCK_MULTICASTTTL:
            retv = RvSocketSetMulticastTtl(&pSockTransp->sock, *(RvInt*)val, logMgr);
            break;

        case RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE:
            retv = RvSocketSetTypeOfService(&pSockTransp->sock, *(RvInt*)val, logMgr);
            break;

        case RVTRANSPORT_OPT_SOCKET:
            /* Close the current socket */
            if (pSockTransp->sock != RV_INVALID_SOCKET)
            {
                /* Firstly remove the socket from the Select Engine. */
                if (pSockTransp->status & SOCKTRANSP_STATUS_WAITINGEVENTS)
                {
                    RvSelectRemoveEx(pSockTransp->pSelectEngine, &pSockTransp->fd,
                        RV_FALSE /*waitForCallbackCompletion*/, NULL /*pbWasInCallback*/);
                    pSockTransp->status &= ~SOCKTRANSP_STATUS_WAITINGEVENTS;
                    pSockTransp->events = 0;
                }
                RvFdDestruct(&pSockTransp->fd);
                /* Now we can close the socket */
                if (!(pSockTransp->options & RVTRANSPORT_CREATEOPT_DONTCLOSESOCK))
                {
                    RvSocketDestruct(&pSockTransp->sock, RV_FALSE/*cleanSocket*/, NULL /*portRange*/, logMgr);
                }
            }
            pSockTransp->sock = *(RvSocket*)val;
            RvFdConstruct(&pSockTransp->fd, &pSockTransp->sock, logMgr);
            TRANSPORT_LOG_DBG_2("%p: new socket was set: %d",pSockTransp, pSockTransp->sock);
            break;

        case RVTRANSPORT_OPT_LOCALADDR:
            if (pSockTransp->sock != RV_INVALID_SOCKET)
            {
                RvAddress* pLocalAddr = (RvAddress*)val;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
                RvChar    strIp[64] = {'\0'};
                RvAddressGetString(pLocalAddr, sizeof(strIp), strIp);
                TRANSPORT_LOG_INFO_4("%p: goes to bind sock=%d to %s:%d",
                    pSockTransp, pSockTransp->sock, strIp, RvAddressGetIpPort(pLocalAddr));
#endif
                retv = RvSocketBind(&pSockTransp->sock, pLocalAddr, NULL, logMgr);
            }
            break;

        default:
            retv = RV_ERROR_NOTSUPPORTED;
    } /* ENDOF: switch(option) */

    return RvErrorGetCode(retv);
#undef FUNCTION_NAME
}

/*******************************************************************************
 * GetOption
 * -----------------------------------------------------------------------------
 * General:
 *  Internal implementation of the RvTransportGetOption function,
 *  which assumes the Transport is in the state, where the options can be got.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 *          option      - The option to be set.
 *          val         - The option value.
 *                        The format of the value to be provided here is
 *                        option specific. Please see option definition
 *                        in the rvtransportsocket.h file for more info.
 * Output:  none.
 *
 * Return Value: RV_OK on success, RV_ERROR_XXX - otherwise.
 ******************************************************************************/
static
RvStatus GetOption(
            IN  RvSocketTransport*    pSockTransp,
            IN  RvUint32            option,
            IN  void*               val)
{
#define FUNCTION_NAME "GetOption"
    RvStatus retv = RV_OK;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);

    switch (option)
    {
        case RVTRANSPORT_OPT_SELECTENGINE:
            *(RvSelectEngine**)val = pSockTransp->pSelectEngine;
            break;

        case RVTRANSPORT_OPT_CLOSEEVENTTIMEOUT:
            *(RvInt32*)val = pSockTransp->closeEvTimeout;
            break;

        case RVTRANSPORT_OPT_SOCK_BYTESAVAILABLE:
            retv = RvSocketGetBytesAvailable(&pSockTransp->sock,logMgr,(RvSize_t*)val);
            break;

        case RVTRANSPORT_OPT_SOCK_TYPEOFSERVICE:
            retv = RvSocketGetTypeOfService(&pSockTransp->sock, logMgr, (RvInt32*)val);
            break;

        case RVTRANSPORT_OPT_SOCKET:
            *(RvSocket*)val = pSockTransp->sock;
            break;

        case RVTRANSPORT_OPT_LOCALPORT:
            {
                RvAddress localAddress;
                if (pSockTransp->sock == RV_INVALID_SOCKET)
                {
                    TRANSPORT_LOG_ERR_1("%p: can't get local port: there is no socket", pSockTransp);
                    return RV_ERROR_ILLEGAL_ACTION;
                }
                retv = RvSocketGetLocalAddress(&pSockTransp->sock, logMgr, &localAddress);
                if (retv != RV_OK)
                {
                    retv = RvErrorGetCode(retv);
                    TRANSPORT_LOG_ERR_2("%p: failed to get local address(retv=%d)",
                        pSockTransp, retv);
                    return retv;
                }
                *(RvUint16*)val = RvAddressGetIpPort(&localAddress);
                RvAddressDestruct(&localAddress);
            }
            break;

        case RVTRANSPORT_OPT_LOCALADDR:
        case RVTRANSPORT_OPT_PUBLICLOCALADDR:

            if (pSockTransp->sock == RV_INVALID_SOCKET)
            {
                TRANSPORT_LOG_ERR_1("%p: can't get local address: there is no socket",
                    pSockTransp);
                return RV_ERROR_ILLEGAL_ACTION;
            }
            retv = RvSocketGetLocalAddress(&pSockTransp->sock, logMgr, (RvAddress*)val);
            break;

        default:
            retv = RV_ERROR_NOTSUPPORTED;
    } /* ENDOF: switch (option) */

    return RvErrorGetCode(retv);
#undef FUNCTION_NAME
}

/*******************************************************************************
 * SafeSelectRemove
 * -----------------------------------------------------------------------------
 * General:
 *  Removes Transport socket from the Select Engine.
 *  If the select event is being handled by the Transport object from within
 *  the SelectCB callback in context of other thread, the SafeSelectRemove
 *  suspends the current thread until the SelectCB will return.
 *
 *  IMPORTANT: the SafeSelectRemove unlocks the Transport object and locks it
 *             again !!!
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 * Output:  none.
 *
 * Return Value: none.
 ******************************************************************************/
static
void SafeSelectRemove(IN RvSocketTransport* pSockTransp)
{
#define FUNCTION_NAME "SafeSelectRemove"

#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    RvInt32 numLocks = 0;
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);
#endif /*#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)*/

    TRANSPORT_RELEASE(pSockTransp);

    RvSelectRemoveEx(pSockTransp->pSelectEngine, &pSockTransp->fd,
                     RV_TRUE /*waitForCallbackCompletion*/, NULL /*pbWasInCallback*/);

    TRANSPORT_RESTORE(pSockTransp);

    pSockTransp->status &= ~SOCKTRANSP_STATUS_WAITINGEVENTS;
    pSockTransp->events = 0;
#undef FUNCTION_NAME
}

/*******************************************************************************
 * SafeTimerCancel
 * -----------------------------------------------------------------------------
 * General:
 *  Cancels the CLOSE event timer, if the last runs.
 *  If the timer event is being handled by the Transport object from within
 *  the TimerCloseEventCB callback in context of other thread,
 *  the SafeTimerCancel suspends the current thread until the TimerCloseEventCB
 *  will return.
 *
 *  IMPORTANT: the SafeTimerCancel unlocks the Transport object and locks it
 *             again !!!
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 * Output:  none.
 *
 * Return Value: none.
 ******************************************************************************/
static
void SafeTimerCancel(IN RvSocketTransport* pSockTransp)
{
#define FUNCTION_NAME "SafeTimerCancel"
#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);
    RvInt32 numLocks = 0;
#endif /*#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)*/

    TRANSPORT_RELEASE(pSockTransp);
    RvTimerCancel(&pSockTransp->timer, RV_TIMER_CANCEL_WAIT_FOR_CB);
    TRANSPORT_RESTORE(pSockTransp);

    pSockTransp->status &= ~SOCKTRANSP_STATUS_CLOSETIMERRUNS;
#undef FUNCTION_NAME
}

/*******************************************************************************
 * CallEventCallback
 * -----------------------------------------------------------------------------
 * General:
 *  Wraps call to the user pfnEvent callback, while unlocking the Transport
 *  object before this.
 *
 * Arguments:
 * Input:   pSockTransp - The Transport object.
 *          event       - The transport event, to be reported using the pfnEvent
 *          error       - RV_TRUE, if error on Transport occurred, while
 *                        handling the transport event.
 * Output:  none.
 *
 * Return Value: none.
 ******************************************************************************/
static
void CallEventCallback(
            IN  RvSocketTransport*  pSockTransp,
            IN  RvTransportEvents   event,
            IN  RvBool              error)
{
#define FUNCTION_NAME "CallEventCallback"

    RvTransportCallbacks cbs = pSockTransp->base.callbacks;
#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)
    TRANSPORT_DEFINE_LOG_MGR(pSockTransp);
#endif /*#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)*/

    TRANSPORT_UNLOCK(pSockTransp);

    (*cbs.pfnEvent)((RvTransport)pSockTransp, event, error, cbs.usrData);

#undef FUNCTION_NAME
}

/*******************************************************************************
 * ConvertSelectEvent2Str
 * -----------------------------------------------------------------------------
 * General:
 *  COnvert the Select Event value to the string.
 *
 * Arguments:
 * Input:   selectEvent - The Select Event value.
 * Output:  none.
 *
 * Return Value: The string.
 ******************************************************************************/
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
static
const RvChar* ConvertSelectEvent2Str(IN RvInt selectEvent)
{
    switch (selectEvent)
    {
    case RvSelectRead:      return "READ";
    case RvSelectWrite:     return "WRITE";
    case RvSelectAccept:    return "ACCEPT";
    case RvSelectConnect:   return "CONNECT";
    case RvSelectClose:     return "CLOSE";
    default:
        return "UNDEFINED";
    }
}
#endif /* #if (RV_LOGMASK != RV_LOGLEVEL_NONE) */


/* Create TCP/UDP socket transport instance */
RVCOREAPI RvTransport RvTransportCreateSocket(
    void* pLogMgr,
    void* pSelectEngine,
    RvBool bTcp,
    RvUint32 options,
    RvAddress* pLocAddress,
    RvTransportCallbacks* pCallbacks)
{
    RvTransportSocketCfg sockCfg;
    RvStatus rvs;
    RvTransport transp;

    RvTransportInitSocketTransportCfg(&sockCfg);
    if (pCallbacks)
        sockCfg.callbacks = *pCallbacks;

    sockCfg.options = options;
    sockCfg.pLocalAddr = pLocAddress;
    sockCfg.pLogMgr = pLogMgr;
    sockCfg.pSelectEngine = pSelectEngine;
    sockCfg.protocol = (bTcp)?RvSocketProtocolTcp:RvSocketProtocolUdp;

    rvs = RvTransportCreateSocketTransport(&sockCfg,&transp);
    if (rvs == RV_OK)
        return transp;
    return NULL;
}

struct RvTransportInterface* RvTransportSocketGetInterface()
{
    return &gSocketTransportInterface;
}

