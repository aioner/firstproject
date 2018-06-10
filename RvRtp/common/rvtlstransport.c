/* rvtls.c - Common Core TLS functionality support module */

/************************************************************************
Copyright (c) 2002 RADVISION Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVISION LTD.
No part of this publication may be reproduced in any form whatsoever
without written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
************************************************************************/

#include "rvtypes.h"

#include "rvmutex.h"
#include "rvtls.h"
#include "rvtransport.h"
#include "rvtransportsocket.h"
#include "rvtlstransport.h"
#include "rvtimestamp.h"
#include <string.h>

#if (RV_TLS_TYPE != RV_TLS_NONE)

#define TLS_SOCK_LOG_ERR(f) \
    RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id))
#define TLS_SOCK_LOG_ERR_1(f,p1) \
    RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1))
#define TLS_SOCK_LOG_ERR_2(f,p1,p2) \
    RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2))
#define TLS_SOCK_LOG_ERR_3(f,p1,p2,p3) \
    RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2,p3))
#define TLS_SOCK_LOG_ERR_4(f,p1,p2,p3,p4) \
    RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2,p3,p4))
#define TLS_SOCK_LOG_ERR_5(f,p1,p2,p3,p4,p5) \
    RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2,p3,p4,p5))

#define TLS_SOCK_LOG_DBG(f) \
    RvLogDebug(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id))
#define TLS_SOCK_LOG_DBG_1(f,p1) \
    RvLogDebug(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1))
#define TLS_SOCK_LOG_DBG_2(f,p1,p2) \
    RvLogDebug(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2))
#define TLS_SOCK_LOG_DBG_3(f,p1,p2,p3) \
    RvLogDebug(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2,p3))
#define TLS_SOCK_LOG_DBG_4(f,p1,p2,p3,p4) \
    RvLogDebug(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2,p3,p4))
#define TLS_SOCK_LOG_DBG_5(f,p1,p2,p3,p4,p5) \
    RvLogDebug(&pLogMgr->TLSSource, (&pLogMgr->TLSSource, FUNCTION_NAME ": ts:%d: " f,pTlsSock->id,p1,p2,p3,p4,p5))

#define DEFINE_LM(_t)  RvLogMgr* pLogMgr = (_t)->pTranspSocket->pLogMgr


typedef enum {
    RvTlsTranspCreated = 157,
    RvTlsTranspHandshaking,
    RvTlsTranspConnected,
} RvTlsTransportState;

typedef struct _RvTlsSockTransport RvTlsSockTransport;

typedef struct {
    struct _RvTlsSockTransport**    pPendingConns;
    RvInt                           pendingConnsNum;
    RvInt                           pendingConnsCnt;
    RvInt64                         lastHangTest;
    RvInt64                         hangTestFrequency;
    RvInt64                         hangTestThreshold;
} RvTlsListeningSockData;

struct _RvTlsSockTransport {

    struct RvTransportBase          transpBase;
    RvSocketTransport*              pTranspSocket;
    RvInt                           id;
    RvInt                           refCount;

    RvTlsSockTransportCfg           tlsSockCfg;
    RvTLSSession                    tlsSess;
    RvMutex                         tlsSessMtx;
    RvBool                          tlsConstructed;

    RvSelectEvents                  realCurrEvents;
    RvTLSEvents                     realTlsEvent;

    RvTlsTransportState             tlsState;
    RvTLSEngine*                    pTlsEngine;
    RvMutex*                        pTlsEngineMtx;

    RvBool                          bIsClient;
    RvInt64                         lastTreatedTs;

    RvTlsListeningSockData*         pListening;
    struct _RvTlsSockTransport*     pListener;
    RvInt                           indexInListener;
};

extern struct RvTransportInterface gTlsSocketTransportInterface;

static RvStatus tlsSockTransportCreate(
    IN  RvTlsSockTransportCfg*  pCfg,
    IN  RvSocketTransport*      pSocketTransp,
    OUT RvTlsSockTransport**    ppTlsSock);

static RvStatus tlsSockInitTlsSess(
    RvTlsSockTransport* pTlsSock);

static RvStatus tlsSockHandleHandshake(
    RvTlsSockTransport* pTlsSock);

static RvStatus RvTlsSockTranspDestruct(
    IN  RvTransport         transp);

static void tlsSockRejectConn(
   RvTlsSockTransport* pTlsSock)
{
#define FUNCTION_NAME "TlsSockRejectConn"
    RvStatus rvs;
    RvAddress remAddr;
    RvSocket nsock;
    DEFINE_LM(pTlsSock);

    TLS_SOCK_LOG_ERR_1("Pending conns at max (%d), reject the connection",pTlsSock->pListening->pendingConnsNum);
    /* accept the socket */
    rvs = RvSocketAccept(&pTlsSock->pTranspSocket->sock,pLogMgr,&nsock,&remAddr);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("Failed in RvSocketAccept (when rejecting)");
    }
    else {
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
        RvChar addrTxt[100];
        TLS_SOCK_LOG_DBG_2("Rejecting connection from %s:%d",
            RvAddressGetString(&remAddr,sizeof(addrTxt),addrTxt),RvAddressGetIpPort(&remAddr));
#endif
        RvSocketDestruct(&nsock,RV_FALSE,NULL,pLogMgr);
    }
    return;
#undef FUNCTION_NAME
}

/* creates the new RvTlsSockTransport instance and stores its in the pPendingConns array of the acceptor */
static RvTlsSockTransport* tlsSockTreatAcceptEvent(
   RvTlsSockTransport* pTlsSock)
{
#define FUNCTION_NAME "TlsSockTreatAcceptEvent"
    RvInt cnt;
    RvSocketTransport* pNewSockTransp;
    RvAddress remAddr;
    RvStatus rvs;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    DEFINE_LM(pTlsSock);
    RvChar addrTxt[100];
#endif

    /* first try to find the free slot in the pending connections array,
       if there is no slot, the connection will be rejected */
    for (cnt = 0; cnt < pTlsSock->pListening->pendingConnsNum; cnt++)
    {
        if (pTlsSock->pListening->pPendingConns[cnt] == NULL)
            break;
    }

    /* no free slot reject the connection */
    if (cnt == pTlsSock->pListening->pendingConnsNum)
    {
        TLS_SOCK_LOG_ERR_2("No room for pending conn (%d,%d)",
            pTlsSock->pListening->pendingConnsNum,pTlsSock->pListening->pendingConnsCnt);
        tlsSockRejectConn(pTlsSock);
        return NULL;
    }

    /* first perform the regular TCP accept and produce the instance of the RvSocketTransport*/
    rvs = RvTransportAccept((RvTransport)pTlsSock->pTranspSocket,
        pTlsSock->tlsSockCfg.socketTransportCfg.options,(RvTransport*)&pNewSockTransp,&remAddr);
    RvTransportAddRef((RvTransport)pNewSockTransp);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("RvTransportAccept failed");
        return NULL;
    }

    TLS_SOCK_LOG_DBG_2("Accepted connection from %s:%d",
        RvAddressGetString(&remAddr,sizeof(addrTxt),addrTxt),RvAddressGetIpPort(&remAddr));

    /* create the new instance of RvTlsSockTransport */
    rvs = tlsSockTransportCreate(&pTlsSock->tlsSockCfg,pNewSockTransp,
        &pTlsSock->pListening->pPendingConns[cnt]);
    if (rvs != RV_OK)
    {
        /* no need to destruct the 'pNewSockTransp', it gets destructed in the TlsSockTransportCreate */
        TLS_SOCK_LOG_ERR("TlsSockTransportCreate failed");
        return NULL;
    }

    /* tell the newly created instance about its 'father'. This is to allow this instance to destroy itself
       using the tlsSockCleanHangConn function.
    */
    pTlsSock->pListening->pPendingConns[cnt]->pListener = pTlsSock;
    pTlsSock->pListening->pPendingConns[cnt]->indexInListener = cnt;
    pTlsSock->pListening->pPendingConns[cnt]->lastTreatedTs = RvTimestampGet(NULL);
    pTlsSock->pListening->pendingConnsCnt++;
    return pTlsSock->pListening->pPendingConns[cnt];
#undef FUNCTION_NAME
}

/*  This is to destroy and clean the RvTlsSockTransport instance that is in the process of being 'accepted'.
    That is the this instance has been created as the result 'Accept' event but has not yet given to the application,
    and there was select error on this instance. Since application does not 'know' about this instance we have to
    destroy it ourselves.
*/
static void tlsSockCleanHangConn(
    RvTlsSockTransport *pTlsSock)
{
#define FUNCTION_NAME "tlsSockCheckHangConns"
    RvTlsSockTransport *pListener;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);

    pListener = pTlsSock->pListener;

    RvMutexLock(&pListener->pTranspSocket->mutex,pLogMgr);
    pListener->pListening->pPendingConns[pTlsSock->indexInListener] = NULL;
    pListener->pListening->pendingConnsCnt--;
    RvMutexUnlock(&pListener->pTranspSocket->mutex,pLogMgr);
    RvTlsSockTranspDestruct((RvTransport)pTlsSock);
#undef FUNCTION_NAME
}

static void tlsSockCheckHangConns(
    RvTlsSockTransport *pTlsSock)
{
#define FUNCTION_NAME "tlsSockCheckHangConns"
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    DEFINE_LM(pTlsSock);
#endif
    RvInt64 now,diff;
    RvInt cnt;
    RvTlsSockTransport *pThis;

    if (!pTlsSock->pListening)
        return;

    now = RvTimestampGet(NULL);
    diff = RvInt64Sub(now,pTlsSock->pListening->lastHangTest);
    if (RvInt64IsLessThan(diff,pTlsSock->pListening->hangTestFrequency))
        return;


    for (cnt = 0; cnt < pTlsSock->pListening->pendingConnsNum; cnt++)
    {
        pThis = pTlsSock->pListening->pPendingConns[cnt];
        if (!pThis)
            continue;
        diff = RvInt64Sub(now,pThis->lastTreatedTs);
        if (RvInt64IsLessThan(diff,pTlsSock->pListening->hangTestThreshold))
            continue;

        /* need to clean this one */
        TLS_SOCK_LOG_DBG_1("Pending tlsSock ts:%d: is idle too long, cleaning it",pThis->id);
        tlsSockCleanHangConn(pThis);
    }
#undef FUNCTION_NAME
}

static RvStatus TlsSockTranspShutdown(
    IN  RvTlsSockTransport* pTlsSock,
    OUT RvBool* pWouldBlock);


/* the main select events callback */
static void tlsSockEventCB(
    IN RvTransport        transp,
    IN RvTransportEvents  ev,
    IN RvBool             error,
    IN void*              usrData)
{
    RvTlsSockTransport *pTlsSock = (RvTlsSockTransport*) usrData;
    DEFINE_LM(pTlsSock);
    RvStatus rvs;
    RvTlsSockTransport *pThis = pTlsSock;
    RvTLSEvents tlsEvent;

    RV_UNUSED_ARG(transp);
#define FUNCTION_NAME "tlsSockEventCB"


    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    pTlsSock->lastTreatedTs = RvTimestampGet(NULL);

    /* test stuck pending connections */
    tlsSockCheckHangConns(pThis);

    TLS_SOCK_LOG_DBG_2("event %d, error %d",ev,error);

    if (ev == RVTRANSPORT_EVENT_CLOSE)
    {
        RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
        (pTlsSock->transpBase.callbacks.pfnEvent)((RvTransport)pTlsSock,
                        RVTRANSPORT_EVENT_CLOSE,RV_FALSE,pTlsSock->transpBase.callbacks.usrData);
        return;
    }

    if (pTlsSock->tlsState == RvTlsTranspCreated)
    {
        if (error)
        {
            TLS_SOCK_LOG_ERR_1("%s error or close event",(ev == RVTRANSPORT_EVENT_CONNECT)?"Connect":"Accept");
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            if (pTlsSock->bIsClient)
            {
                (pTlsSock->transpBase.callbacks.pfnEvent)((RvTransport)pTlsSock,
                    RVTRANSPORT_EVENT_CONNECT,RV_TRUE,pTlsSock->transpBase.callbacks.usrData);
            }
            else {
                tlsSockCleanHangConn(pTlsSock);
            }
            return;
        }

        /* the connect event may come for client only, the accept event may
           come for listening socket only, verify it */
        if (!((ev == RVTRANSPORT_EVENT_CONNECT && pThis->bIsClient) ||
            (ev == RVTRANSPORT_EVENT_ACCEPT && pThis->pListening)))
        {
            TLS_SOCK_LOG_ERR("unexpected event (state=created), ignore it");
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return;
        }

        if (ev == RVTRANSPORT_EVENT_ACCEPT)
        {
            /* we will accept this connection and start the TLS handhs
             */
            pThis = tlsSockTreatAcceptEvent(pTlsSock);
            if (pThis == NULL)
            {
                TLS_SOCK_LOG_ERR("TlsSockTreatAcceptEvent failed");
                RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
                return;
            }
        }
        rvs = tlsSockInitTlsSess(pThis);
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("tlsSockInitTlsSess failed");
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return;
        }
    }
    else {
        if (!(ev == RVTRANSPORT_EVENT_READ || ev == RVTRANSPORT_EVENT_WRITE || ev == RVTRANSPORT_EVENT_CLOSE))
        {
            TLS_SOCK_LOG_ERR_1("unexpected event (state=%d), ignore it",pTlsSock->tlsState);
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return;
        }

        if (error)
        {
            TLS_SOCK_LOG_ERR_1("error on select",pTlsSock->tlsState);
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return;
        }
    }

    if (pThis->tlsState == RvTlsTranspCreated)
    {
        tlsEvent = RV_TLS_HANDSHAKE_EV;
        pThis->tlsState = RvTlsTranspHandshaking;
    }
    else {

        if (!pTlsSock->tlsConstructed)
        {
            TLS_SOCK_LOG_ERR("tls session is not constructed also it must be");
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return;
        }
        RvTLSTranslateSelectEvents(&pThis->tlsSess,ev,&pThis->tlsSessMtx,pLogMgr,&tlsEvent);
    }

    TLS_SOCK_LOG_DBG_2("select event %d, tls event %d",ev,tlsEvent);

    if (tlsEvent == RV_TLS_HANDSHAKE_EV)
    {
        RvTransport t;
        if (pThis->bIsClient)
            t = (RvTransport)pTlsSock;
        else
            t = (RvTransport)pTlsSock->pListener;

        rvs = tlsSockHandleHandshake(pThis);
        if (rvs == RV_ERROR_TRY_AGAIN)
        {
            TLS_SOCK_LOG_DBG("Handshake not completed yet");
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return;
        }
        else if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("tlsSockHandleHandshake error");
            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            (pTlsSock->transpBase.callbacks.pfnEvent)(t,
                (pThis->bIsClient)?RVTRANSPORT_EVENT_CONNECT:RVTRANSPORT_EVENT_ACCEPT,
                RV_TRUE,pTlsSock->transpBase.callbacks.usrData);
            return;
        }

        TLS_SOCK_LOG_DBG("Handshake completed");

        pThis->tlsState = RvTlsTranspConnected;

        RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

        (pTlsSock->transpBase.callbacks.pfnEvent)(t,
            (pThis->bIsClient)?RVTRANSPORT_EVENT_CONNECT:RVTRANSPORT_EVENT_ACCEPT,
            RV_FALSE,pTlsSock->transpBase.callbacks.usrData);
        return;
    }

    if (tlsEvent == RV_TLS_READ_EV || tlsEvent == RV_TLS_WRITE_EV)
    {
        RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
        (pTlsSock->transpBase.callbacks.pfnEvent)((RvTransport)pTlsSock,
            (tlsEvent == RV_TLS_READ_EV)?RVTRANSPORT_EVENT_READ:RVTRANSPORT_EVENT_WRITE,
            RV_FALSE,pTlsSock->transpBase.callbacks.usrData);
        return;
    }
    else if (tlsEvent == RV_TLS_SHUTDOWN_EV)
    {
        RvBool wouldBlock;
        rvs = TlsSockTranspShutdown(pTlsSock,&wouldBlock);
        RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

        if (!wouldBlock)
            (pTlsSock->transpBase.callbacks.pfnEvent)((RvTransport)pTlsSock,
                            RVTRANSPORT_EVENT_CLOSE,(rvs != RV_OK),pTlsSock->transpBase.callbacks.usrData);

    }

    return;
#undef FUNCTION_NAME
}
static RvStatus tlsSockInitTlsSess(
    RvTlsSockTransport* pTlsSock)
{
#define FUNCTION_NAME "tlsSockInitTlsSess"
    RvStatus rvs;
    DEFINE_LM(pTlsSock);

    RvTLSSessionCfg tlsSessCfg;

    rvs = RvMutexConstruct(pLogMgr,&pTlsSock->tlsSessMtx);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("Failed in RvMutexConstruct");
        return RV_ERROR_UNKNOWN;
    }

    memset(&tlsSessCfg,0,sizeof(tlsSessCfg));
    tlsSessCfg.bDisablePartialSend = RV_FALSE;

    rvs = RvTLSSessionConstructEx(pTlsSock->pTlsEngine,pTlsSock->pTlsEngineMtx,
        &pTlsSock->tlsSessMtx,&tlsSessCfg,pLogMgr,&pTlsSock->tlsSess);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("Failed in RvTLSSessionConstructEx");
        return RV_ERROR_UNKNOWN;
    }
    pTlsSock->tlsConstructed = RV_TRUE;

    return RV_OK;
#undef FUNCTION_NAME
}

static RvStatus TransportTlsSocketTestRemoteCertifFingerprint(
    IN  RvTlsSockTransport*     pTlsSock,
    IN  RvChar*                 fingerprint,
    IN  RvUint                  fingerprintLen,
    IN  RvHashFuncEnum          hash);


static RvStatus tlsSockHandleHandshake(
    RvTlsSockTransport* pTlsSock)
{
#define FUNCTION_NAME "tlsSockHandleHandshake"
    RvStatus rvs;
    DEFINE_LM(pTlsSock);

    if (pTlsSock->bIsClient)
    {
        rvs = RvTLSSessionClientHandshake(&pTlsSock->tlsSess,pTlsSock->tlsSockCfg.onCertCB,
            pTlsSock->pTranspSocket->sock,&pTlsSock->tlsSessMtx,pLogMgr);
    }
    else
    {
        rvs = RvTLSSessionServerHandshake(&pTlsSock->tlsSess,pTlsSock->tlsSockCfg.onCertCB,
            pTlsSock->pTranspSocket->sock,&pTlsSock->tlsSessMtx,pLogMgr);

    }
    pTlsSock->tlsState = RvTlsTranspHandshaking;

    if(RvErrorGetCode(rvs) == RV_TLS_ERROR_WILL_BLOCK)
    {
        RvSelectEvents sockEvs;
        rvs = RvTLSTranslateTLSEvents(&pTlsSock->tlsSess,RV_TLS_HANDSHAKE_EV,&pTlsSock->tlsSessMtx,pLogMgr,&sockEvs);
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("Failed in RvTLSTranslateTLSEvents");
            return RV_ERROR_UNKNOWN;
        }

        rvs = RvSockTranspRegisterEvent((RvTransport)pTlsSock->pTranspSocket,sockEvs);
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR_1("Failed in RvSockTranspRegisterEvent for ev %d",sockEvs);
            return RV_ERROR_UNKNOWN;
        }
        TLS_SOCK_LOG_DBG_1("Handshake not ready, registered for %d",sockEvs);
        return RV_ERROR_TRY_AGAIN;
    }

    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("RvTLSSessionServerHandshake/RvTLSSessionClientHandshake failed");
        return RV_ERROR_UNKNOWN;
    }

    if (pTlsSock->tlsSockCfg.options&RVTRANSPORT_OPT_TLSOCK_TEST_CERT_FINGERPRINT)
    {
        rvs = TransportTlsSocketTestRemoteCertifFingerprint(pTlsSock,
            (RvChar*)pTlsSock->tlsSockCfg.peerCertFingerprint,
            pTlsSock->tlsSockCfg.peerCertFingerprintLen,
            pTlsSock->tlsSockCfg.peerCertFingerprintHashFunc);
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("TransportTlsSocketTestRemoteCertifFingerprint failed");
            return RV_ERROR_UNKNOWN;
        }
    }

    return RV_OK;
#undef FUNCTION_NAME
}

/* creates the instance RvTlsSockTransport with the given configuration,
   may create the RvSocketTransport instance or use the given through 'pSocketTransp' */
static RvStatus tlsSockTransportCreate(
    IN  RvTlsSockTransportCfg*  pCfg,
    IN  RvSocketTransport*      pSocketTransp,
    OUT RvTlsSockTransport**    ppTlsSock)
{
    RvStatus rvs;
    static int RunningId = 1;
    RvLogMgr *pLogMgr;
    RvTlsSockTransport* pTlsSock;

#define FUNCTION_NAME "TlsSockTransportCreate"

    if (!pCfg)
        return RV_ERROR_UNKNOWN;

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    if (!pCfg->socketTransportCfg.pLogMgr)
        return RV_ERROR_UNKNOWN;
#endif

    pLogMgr = pCfg->socketTransportCfg.pLogMgr;

    RvLogEnter(&pLogMgr->TLSSource, (&pLogMgr->TLSSource,
        "TlsSockTransportCreate: Creating the TLS transport"));

    rvs = RvMemoryAlloc(NULL,sizeof(RvTlsSockTransport),NULL,(void**)ppTlsSock);
    if (rvs != RV_OK)
    {
        RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource,
            "TlsSockTransportCreate: RvMemoryAlloc failed"));
        if (pSocketTransp)
            RvTransportRelease((RvTransport)pSocketTransp);
        return RV_ERROR_UNKNOWN;
    }

    pTlsSock = *ppTlsSock;

    memset(pTlsSock,0,sizeof(RvTlsSockTransport));
    pTlsSock->id = RunningId++;

    pTlsSock->transpBase.callbacks = pCfg->socketTransportCfg.callbacks;


    pCfg->socketTransportCfg.callbacks.pfnEvent = tlsSockEventCB;
    pCfg->socketTransportCfg.callbacks.usrData = pTlsSock;

    if (pSocketTransp == NULL)
    {
        rvs = RvTransportCreateSocketTransport((void*)&pCfg->socketTransportCfg,(RvTransport*)&pTlsSock->pTranspSocket);
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("Failed in RvTransportCreateSocketTransport");
            goto failed;
        }
        RvTransportAddRef((RvTransport)pTlsSock->pTranspSocket);
    }
    else {
        rvs = RvTransportSetCallbacks((RvTransport)pSocketTransp,&(pCfg->socketTransportCfg.callbacks));
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("Failed in RvTransportSetCallbacks");
            goto failed;
        }
        pTlsSock->pTranspSocket = pSocketTransp;
    }

    pCfg->socketTransportCfg.callbacks.pfnEvent = pTlsSock->transpBase.callbacks.pfnEvent;
    pCfg->socketTransportCfg.callbacks.usrData = pTlsSock->transpBase.callbacks.usrData;

    pTlsSock->transpBase.iface = &gTlsSocketTransportInterface;

    memcpy(&pTlsSock->tlsSockCfg,pCfg,sizeof(RvTlsSockTransportCfg));

    if (pCfg->socketTransportCfg.options & RVTRANSPORT_CREATEOPT_LISTENING)
    {
        rvs = RvMemoryAlloc(NULL,sizeof(RvTlsListeningSockData),NULL,(void**)&pTlsSock->pListening);
        if (rvs != RV_OK)
        {
            RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource,
                "TlsSockTransportCreate: RvMemoryAlloc failed (for pListening)"));
            goto failed;
        }
        memset(pTlsSock->pListening,0,sizeof(RvTlsListeningSockData*));

        pTlsSock->pListening->pendingConnsNum = RV_TLS_SOCK_PENDING_CONNS_MAX;
        pTlsSock->pListening->pendingConnsCnt = 0;
        rvs = RvMemoryAlloc(NULL,sizeof(RvTlsSockTransport*)*pTlsSock->pListening->pendingConnsNum,NULL,(void**)&pTlsSock->pListening->pPendingConns);
        if (rvs != RV_OK)
        {
            RvLogError(&pLogMgr->TLSSource, (&pLogMgr->TLSSource,
                "RvMemoryAlloc failed (for pending conns)"));
            goto failed;
        }

        pTlsSock->pListening->hangTestFrequency = RvInt64Mul(RvInt64FromRvUint(5),RV_TIME_NSECPERSEC);
        pTlsSock->pListening->hangTestThreshold =
            RvInt64Mul(RvInt64FromRvUint(RV_TLS_TRANSPORT_MAX_PENDING_CONN_TIMEOUT),RV_TIME_NSECPERSEC);

        memset(pTlsSock->pListening->pPendingConns,0,sizeof(RvTlsSockTransport*)*pTlsSock->pListening->pendingConnsNum);
        pTlsSock->tlsSockCfg.socketTransportCfg.options &= ~RVTRANSPORT_CREATEOPT_LISTENING;
    }

    pTlsSock->tlsState = RvTlsTranspCreated;
    pTlsSock->pTlsEngine = pCfg->pTlsEngine;
    pTlsSock->pTlsEngineMtx = pCfg->pTlsEngineMtx;

    TLS_SOCK_LOG_DBG_1("Created TLS socket 0x%p",pTlsSock);
    return RV_OK;

failed:

    if (pTlsSock->pListening)
    {
        if (pTlsSock->pListening->pPendingConns)
            RvMemoryFree(pTlsSock->pListening->pPendingConns,pLogMgr);
        RvMemoryFree(pTlsSock->pListening,pLogMgr);
        pTlsSock->pListening = NULL;
    }

    if (pTlsSock->pTranspSocket)
    {
        RvTransportRelease((RvTransport)pTlsSock->pTranspSocket);
        pTlsSock->pTranspSocket = NULL;
    }
    RvMemoryFree((void*)pTlsSock,pLogMgr);
    *ppTlsSock = NULL;
    return RV_ERROR_UNKNOWN;
#undef FUNCTION_NAME
}

RVCOREAPI RvStatus RvTransportCreateTlsSocket(
    IN  RvTlsSockTransportCfg *pCfg,
    OUT RvTransport*    pTransp)
{
    return tlsSockTransportCreate(pCfg,NULL,(RvTlsSockTransport**)pTransp);
}



static RvStatus RvTlsSockTranspDestruct(
    IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvTlsSockTranspDestruct"
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*)transp;
    DEFINE_LM(pTlsSock);

    TLS_SOCK_LOG_DBG("Called");

    if (pTlsSock->tlsConstructed)
    {
        RvTLSSessionDestruct(&pTlsSock->tlsSess,&pTlsSock->tlsSessMtx,pLogMgr);
        RvMutexDestruct(&pTlsSock->tlsSessMtx,pLogMgr);
        pTlsSock->tlsConstructed = RV_FALSE;
    }


    if (pTlsSock->pListening)
    {
        if (pTlsSock->pListening->pPendingConns)
            RvMemoryFree(pTlsSock->pListening->pPendingConns,pLogMgr);
        RvMemoryFree(pTlsSock->pListening,pLogMgr);
        pTlsSock->pListening = NULL;
    }

    if (pTlsSock->pTranspSocket)
    {
        RvTransportRelease((RvTransport)pTlsSock->pTranspSocket);
        pTlsSock->pTranspSocket = NULL;
    }
    RvMemoryFree((void*)pTlsSock,pLogMgr);
    return RV_OK;
#undef FUNCTION_NAME
}

static RvStatus RvTlsSockTranspConnect(
    IN  RvTransport         transp,
    IN  RvAddress*          pRemoteAddr)
{
#define FUNCTION_NAME "RvTlsSockTranspConnect"
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    RvStatus rvs = RV_OK;
    DEFINE_LM(pTlsSock);
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvChar addrTxt[100];
    TLS_SOCK_LOG_DBG_2("Connecting to %s:%d",
        RvAddressGetString(pRemoteAddr,sizeof(addrTxt),addrTxt),RvAddressGetIpPort(pRemoteAddr));
#endif
    RV_UNUSED_ARG(pLogMgr);

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    if (pTlsSock->tlsState == RvTlsTranspCreated && !pTlsSock->pListening)
    {
        pTlsSock->bIsClient = RV_TRUE;
        rvs = (pTlsSock->pTranspSocket->base.iface->pfnConnect)((RvTransport)pTlsSock->pTranspSocket,pRemoteAddr);
        goto end;
    }
    else {
        TLS_SOCK_LOG_ERR_1("Called in wrong state %d",pTlsSock->tlsState);
    }

end:
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    return rvs;
#undef FUNCTION_NAME
}

/* Application initiated 'accept'.
   Picks from the array of the pending connections the instance of RvTlsSockTransport */
static RvStatus RvTlsSockTranspAccept(
    IN  RvTransport         transp,
    IN  RvUint32            options,
    OUT RvTransport*        pNewTransp,
    OUT RvAddress*          pRemoteAddr)
{
#define FUNCTION_NAME "RvTlsSockTranspAccept"
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    RvTlsSockTransport* pNewTlsSock = NULL;
    RvStatus rvs = RV_ERROR_TRY_AGAIN;
    RvInt cnt;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(options);

    *pNewTransp = NULL;

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    if (pTlsSock->pListening->pendingConnsCnt == 0)
    {
        TLS_SOCK_LOG_ERR("No pending conns");
        goto end;
    }

    for (cnt = 0; cnt < pTlsSock->pListening->pendingConnsNum; cnt++)
    {
        if (pTlsSock->pListening->pPendingConns[cnt] != NULL)
            break;
    }

    if (cnt == pTlsSock->pListening->pendingConnsNum)
    {
        TLS_SOCK_LOG_ERR_1("No pending conns, though pendingConnsCnt is %d",
            pTlsSock->pListening->pendingConnsCnt);
        pTlsSock->pListening->pendingConnsCnt = 0;
        goto end;
    }

    pNewTlsSock = pTlsSock->pListening->pPendingConns[cnt];
    pTlsSock->pListening->pPendingConns[cnt] = NULL;
    pTlsSock->pListening->pendingConnsCnt--;

    TLS_SOCK_LOG_DBG_3("Accepted pendConn(ind-%d), Cnt %d, ts:%d:",
        cnt,pTlsSock->pListening->pendingConnsCnt,pNewTlsSock->id);

    if (pRemoteAddr)
    {
        rvs = RvSocketGetRemoteAddress(&pNewTlsSock->pTranspSocket->sock,pLogMgr,pRemoteAddr);
        if (rvs != RV_OK)
        {
            TLS_SOCK_LOG_ERR("Failed in RvSocketGetRemoteAddress");
            memset(pRemoteAddr,0,sizeof(RvAddress));
        }
    }

    rvs = RV_OK;
    *pNewTransp = (RvTransport) pNewTlsSock;

end:
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    return rvs;
#undef FUNCTION_NAME
}

static RvStatus updateTlsEvents(
    RvTlsSockTransport* pTlsSock,
    RvTLSEvents         tlsEv,
    RvSelectEvents      selEv);

extern struct RvTransportInterface* RvTransportSocketGetInterface();


static RvStatus TlsSockTranspShutdown(
    IN  RvTlsSockTransport* pTlsSock,
    OUT RvBool* pWouldBlock)
{
#define FUNCTION_NAME "TlsSockTranspShutdown"
    RvStatus rvs;
    DEFINE_LM(pTlsSock);
    struct RvTransportInterface* sockInterface;

    TLS_SOCK_LOG_DBG("Called");

    if (pWouldBlock)
        *pWouldBlock = RV_FALSE;

    if (pTlsSock->tlsState == RvTlsTranspConnected && pTlsSock->tlsConstructed)
    {
        rvs = RvTLSSessionShutdown(&pTlsSock->tlsSess,&pTlsSock->tlsSessMtx,pLogMgr);
        if (RV_TLS_ERROR_WILL_BLOCK == RvErrorGetCode(rvs) ||
            RV_TLS_ERROR_INCOMPLETE == RvErrorGetCode(rvs))
        {
            TLS_SOCK_LOG_DBG_1("Call to shutdown blocked (rv=%d)",RvErrorGetCode(rvs));
            pTlsSock->realTlsEvent = 0;
            rvs = updateTlsEvents(pTlsSock,0,0);
            if (rvs != RV_OK)
            {
                TLS_SOCK_LOG_ERR("updateTlsEvents failed");
            }
            if (pWouldBlock)
                *pWouldBlock = RV_TRUE;
            return RV_OK;
        }
        else if (RV_OK != RvErrorGetCode(rvs))
        {
            TLS_SOCK_LOG_ERR_1("RvTLSSessionShutdown failed with %d",RvErrorGetCode(rvs));
            return RV_ERROR_UNKNOWN;
        }
        TLS_SOCK_LOG_DBG("RvTLSSessionShutdown success, proceed to socket shutdown");
        RvTLSSessionDestruct(&pTlsSock->tlsSess,&pTlsSock->tlsSessMtx,pLogMgr);
        RvMutexDestruct(&pTlsSock->tlsSessMtx,pLogMgr);
        pTlsSock->tlsConstructed = RV_FALSE;
    }

    sockInterface = RvTransportSocketGetInterface();
    rvs = sockInterface->pfnShutdown((RvTransport)pTlsSock->pTranspSocket);
    rvs = updateTlsEvents(pTlsSock,0,RVTRANSPORT_EVENT_CLOSE);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("updateTlsEvents failed");
    }
    if (rvs == RV_OK)
    {
        if (pWouldBlock)
            *pWouldBlock = RV_TRUE;
    }


    return rvs;

#undef FUNCTION_NAME
}

static RvStatus RvTlsSockTranspShutdown(
    IN  RvTransport         transp)
{
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    RvStatus rvs;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);
    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    rvs = TlsSockTranspShutdown(pTlsSock,NULL);
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    return rvs;
}



static RvStatus updateTlsEvents(
    RvTlsSockTransport* pTlsSock,
    RvTLSEvents         tlsEv,
    RvSelectEvents      selEv)
{
#define FUNCTION_NAME "updateTlsEvents"
    RvStatus rvs;
    DEFINE_LM(pTlsSock);
    TLS_SOCK_LOG_DBG_3("realEvent %d, tlsEv %d, selEv %d",pTlsSock->realTlsEvent,tlsEv,selEv);

    if (selEv == 0)
    {
        pTlsSock->realTlsEvent |= tlsEv;
        RvTLSTranslateTLSEvents(&pTlsSock->tlsSess,pTlsSock->realTlsEvent,&pTlsSock->tlsSessMtx,pLogMgr,&selEv);
    }
    selEv |= RV_SELECT_CLOSE;
    pTlsSock->realCurrEvents = selEv;
    TLS_SOCK_LOG_DBG_1("realCurrentEvent %d",selEv);
    rvs = (pTlsSock->pTranspSocket->base.iface->pfnRegisterEvent)((RvTransport)pTlsSock->pTranspSocket,selEv);
    return RV_OK;
#undef FUNCTION_NAME
}

static RvStatus RvTlsSockTranspSendBuffer(
    IN  RvTransport         transp,
    IN  RvUint8*            buff,
    IN  RvSize_t            len,
    IN  RvAddress*          pDestAddr,
    IN  RvUint32            options,
    OUT RvSize_t*           pSent)
{
#define FUNCTION_NAME "RvTlsSockTranspSendBuffer"
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    DEFINE_LM(pTlsSock);
    RvStatus rvs = RV_OK;
    RvTLSEvents pendingTlsEv;
    RV_UNUSED_ARG(pDestAddr);
    RV_UNUSED_ARG(options);

    *pSent = 0;

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    if (pTlsSock->tlsState != RvTlsTranspConnected)
    {
        TLS_SOCK_LOG_ERR_1("Called in wrong state %d",pTlsSock->tlsState);
        RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
        return RV_ERROR_UNKNOWN;
    }

    pendingTlsEv = 0;
    rvs = RvTLSSessionSendBufferEx(&pTlsSock->tlsSess,(RvChar*)buff,(RvInt)len,&pTlsSock->tlsSessMtx,pLogMgr,&pendingTlsEv,pSent);
    if(RvErrorGetCode(rvs) == RV_TLS_ERROR_WILL_BLOCK)
    {
        TLS_SOCK_LOG_DBG_1("Would-block, pending tls event %d",pendingTlsEv);
        rvs = RV_ERROR_TRY_AGAIN;
    }
    else if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR_1("RvTLSSessionSendBufferEx failed with %d",rvs);
        rvs = RV_ERROR_UNKNOWN;
    }
    else {
        TLS_SOCK_LOG_DBG_2("Could send %d bytes, pending tls event %d",*pSent,pendingTlsEv);
        rvs = RV_OK;
    }
    rvs = updateTlsEvents(pTlsSock,pendingTlsEv,0);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("updateTlsEvents failed");
    }

    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

#undef  FUNCTION_NAME
    return rvs;
}

static RvStatus RvTlsSockTranspReceiveBuffer(
    IN  RvTransport         transp,
    IN  RvUint8*            buff,
    IN  RvSize_t            len,
    IN  RvUint32            options,
    OUT RvAddress*          pRcvdFromAddr,
    OUT RvSize_t*           pReceived)
{
#define FUNCTION_NAME "RvTlsSockTranspReceiveBuffer"
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    DEFINE_LM(pTlsSock);
    RvStatus rvs = RV_OK;
    RvTLSEvents pendingTlsEv;
    RvInt n;
    RV_UNUSED_ARG(pRcvdFromAddr);
    RV_UNUSED_ARG(options);

    *pReceived = 0;

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    if (pTlsSock->tlsState != RvTlsTranspConnected)
    {
        TLS_SOCK_LOG_ERR_1("Called in wrong state %d",pTlsSock->tlsState);
        RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
        return RV_ERROR_UNKNOWN;
    }

    n = (RvInt) len;
    pendingTlsEv = 0;
    rvs = RvTLSSessionReceiveBufferEx(&pTlsSock->tlsSess,(RvChar*)buff,&pTlsSock->tlsSessMtx,pLogMgr,&n,&pendingTlsEv);
    if(RvErrorGetCode(rvs) == RV_TLS_ERROR_WILL_BLOCK)
    {
        TLS_SOCK_LOG_DBG_1("Would-block pending tls event %d",pendingTlsEv);
        rvs = RV_ERROR_TRY_AGAIN;
    }
    else if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR_1("RvTLSSessionReceiveBufferEx failed with %d",rvs);
        rvs = RV_ERROR_UNKNOWN;
    }
    else {
        *pReceived = (RvSize_t)n;
        TLS_SOCK_LOG_DBG_2("Could receive %d bytes (pending  tls event %d)",*pReceived,pendingTlsEv);
        rvs = RV_OK;
    }
    rvs = updateTlsEvents(pTlsSock,pendingTlsEv,0);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("updateTlsEvents failed");
    }
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

#undef  FUNCTION_NAME
    return rvs;
}

static RvStatus RvTlsSockTranspGetBytesAvailable(
    IN  RvTransport         transp,
    IN  RvSize_t*           bytesAvailable)
{
    RvStatus rvs;
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    DEFINE_LM(pTlsSock);

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    rvs = RvSocketGetBytesAvailable(&pTlsSock->pTranspSocket->sock,pLogMgr,bytesAvailable);
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    return rvs;
}
 

static RvStatus RvTlsSockTranspSetOption(
    IN  RvTransport         transp,
    IN  RvUint32            type,
    IN  RvUint32            option,
    IN  void*               val)
{
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    struct RvTransportInterface* sockInterface = RvTransportSocketGetInterface();
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);

    if (pTlsSock->pTranspSocket == NULL)
        return RV_ERROR_BADPARAM;

    if (type == RVTRANSPORT_OPTTYPE_TLSSOCKTRANSPORT)
    {
        if (option == RVTRANSPORT_OPT_TLSOCK_REMOTE_FINGERPRINT_CFG)
        {
            RvTlsSockTransportCfg* pTlsCfg = (RvTlsSockTransportCfg*) val;
            RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

            memcpy(pTlsSock->tlsSockCfg.peerCertFingerprint,pTlsCfg->peerCertFingerprint,pTlsCfg->peerCertFingerprintLen);
            pTlsSock->tlsSockCfg.peerCertFingerprintLen = pTlsCfg->peerCertFingerprintLen;
            pTlsSock->tlsSockCfg.peerCertFingerprintHashFunc = pTlsCfg->peerCertFingerprintHashFunc;

            RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
            return RV_OK;
        }
    }


    return sockInterface->pfnSetOption((RvTransport)pTlsSock->pTranspSocket,type,option,val);
}

static RvStatus RvTlsSockTranspGetOption(
    IN  RvTransport         transp,
    IN  RvUint32            type,
    IN  RvUint32            option,
    OUT void*               val)
{
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    struct RvTransportInterface* sockInterface = RvTransportSocketGetInterface();
    if (pTlsSock->pTranspSocket == NULL)
        return RV_ERROR_BADPARAM;
    return sockInterface->pfnGetOption((RvTransport)pTlsSock->pTranspSocket,type,option,val);
}

static RvStatus RvTlsSockTranspRegisterEvent(
    IN  RvTransport         transp,
    IN  RvTransportEvents   events)
{
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    RvStatus rvs = RV_OK;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    if (pTlsSock->tlsState == RvTlsTranspCreated)
    {
        rvs = updateTlsEvents(pTlsSock,0,events);
        goto end;
    }
    else {
        RvTLSEvents tlsEv = 0;

        if (events & RVTRANSPORT_EVENT_READ)
            tlsEv |= RV_TLS_READ_EV;
        if (events & RVTRANSPORT_EVENT_WRITE)
            tlsEv |= RV_TLS_WRITE_EV;
        rvs = updateTlsEvents(pTlsSock,tlsEv,0);
        goto end;
    }

end:
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    return rvs;
}

static void RvTlsSockTranspAddRef(
    IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvTlsSockTranspAddRef"
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);
    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    pTlsSock->refCount ++;
    TLS_SOCK_LOG_DBG_1("called %d",pTlsSock->refCount);
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
#undef FUNCTION_NAME
}

static void RvTlsSockTranspRelease(
    IN  RvTransport         transp)
{
#define FUNCTION_NAME "RvTlsSockTranspRelease"
    RvBool done = RV_FALSE;
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    pTlsSock->refCount --;
    if (pTlsSock->refCount < 0)
        printf("gewalt");
    TLS_SOCK_LOG_DBG_1("called %d",pTlsSock->refCount);
    if (pTlsSock->refCount == 0)
        done = RV_TRUE;
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);

    if (done)
    {
        TLS_SOCK_LOG_DBG("Destructing indeed");
        RvTlsSockTranspDestruct((RvTransport)pTlsSock);
    }

#undef FUNCTION_NAME
}

static RvStatus TransportTlsSocketTestRemoteCertifFingerprint(
    IN  RvTlsSockTransport*     pTlsSock,
    IN  RvChar*                 fingerprint,
    IN  RvUint                  fingerprintLen,
    IN  RvHashFuncEnum          hash)
{
#define FUNCTION_NAME "TransportTlsSocketTestRemoteCertifFingerprint"
    DEFINE_LM(pTlsSock);
    RvUint8 fingerprint2[RV_TLS_FINGERPRINT_MAX_LEN];
    RvUint fingerprintLen2 = sizeof(fingerprint2);
    RvStatus rvs;

    if (hash == RvHashFuncUndefined || fingerprint == NULL || fingerprintLen <= 0)
    {
        TLS_SOCK_LOG_ERR("Bad params");
        return RV_ERROR_UNKNOWN;
    }

    rvs = RvTLSSessionGetCertificateFingerprint(&pTlsSock->tlsSess,&pTlsSock->tlsSessMtx,pLogMgr,
        hash,fingerprint2,&fingerprintLen2);
    if (rvs != RV_OK)
    {
        TLS_SOCK_LOG_ERR("RvTLSSessionGetCertificateFingerprint failed");
        return RV_ERROR_UNKNOWN;
    }

    if (fingerprintLen != fingerprintLen2 ||
        memcmp(fingerprint,fingerprint2,fingerprintLen2) != 0)
    {
        TLS_SOCK_LOG_ERR_2("the expected and real peer certificate fingerprints do not match (%d,%d)",
            fingerprintLen,fingerprintLen2);
        return RV_ERROR_UNKNOWN;
    }

    return RV_OK;
#undef FUNCTION_NAME
}

RVCOREAPI RvStatus RvTransportTlsSocketTestRemoteCertifFingerprint(
    IN  RvTransport             transp,
    IN  RvChar*                 fingerprint,
    IN  RvUint                  fingerprintLen,
    IN  RvHashFuncEnum          hash)
{
    RvTlsSockTransport* pTlsSock = (RvTlsSockTransport*) transp;
    RvStatus rvs;
    DEFINE_LM(pTlsSock);
    RV_UNUSED_ARG(pLogMgr);

    RvMutexLock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    rvs = TransportTlsSocketTestRemoteCertifFingerprint(pTlsSock,fingerprint,fingerprintLen,hash);
    RvMutexUnlock(&pTlsSock->pTranspSocket->mutex,pLogMgr);
    return rvs;
}

struct RvTransportInterface gTlsSocketTransportInterface =
{
    RV_TLS_TRANSPORT_STR,
    RvTlsSockTranspDestruct,
    RvTlsSockTranspConnect,
    RvTlsSockTranspAccept,
    RvTlsSockTranspShutdown,
    RvTlsSockTranspSendBuffer,
    RvTlsSockTranspReceiveBuffer,
    RvTlsSockTranspGetBytesAvailable,
    RvTlsSockTranspSetOption,
    RvTlsSockTranspGetOption,
    NULL,   /* SetCallbacks */
    NULL,   /* GetCallbacks */
    RvTlsSockTranspRegisterEvent,
    RvTlsSockTranspAddRef,
    RvTlsSockTranspRelease
};

#else

int prevent_warning_of_ranlib_has_no_symbols_rvtlstransport=0;


#endif /* (RV_TLS_TYPE != RV_TLS_NONE) */
