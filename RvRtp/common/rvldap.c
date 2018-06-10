/* rvldap.c - LDAP source file */
/************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/

#if (RV_LDAP_TYPE != RV_LDAP_NONE)
#include "rvldap.h"
#include "rvscache.h"
#include <ldap.h>

#include "rvmemory.h"
#include "rvselect.h"
#include "rvema.h"
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif


#define MAX_LDAP_RESULT_ENTRIES 64
#define LDAP_DEFAULT_STRING_SIZE 256
#define LDAP_DEFAULT_TX_TIMEOUT_MSEC 4000

#define LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)   ((RvChar*)tx+sizeof(ldapTransaction_T)+0*tx->ldap->maxStringSize)

RvBool RvLdapTimerEvent(IN void*  userData);

typedef struct {
    HLDAPAPP hLdapApp;

    RvSize_t maxTransactions;
    RvInt32 transactionTimeout;
    RvSize_t maxStringSize;

    LDAPCallbacks callbacks;
    LDAPEvents events;

    RvLogMgr* logMgr; /* Log manager to use */
    RvLogSource ldapLog;
    RvLogSource *pLdapLog;
    RvSelectEngine* selectEngine;
    RvTimerQueue* timerQ;

    HEMA txEma;

    RvLdapState eState;
    RvInt32 activeTransactions;

    HLDAPCb hLdap;
    RvSelectFd fd;
} ldapElement_T;


#define SCRATCH_AREA_SIZE 200

typedef struct {
    ldapElement_T * ldap;

    RvLdapRequestState  eState;
    RvInt32 transactionTimeout;
    LDAPTransactionEvents events;

    HLDAPRequestCb hRequest;
    HLDAPMessageCb hResponse;
    RvTimer timer;

    RvLdapRequestType requestType;
    RvLdapSaslConnectCtx *pSaslConnectCtx;
    RvLdapPagedSearchCtx  pagedSearchCtx;
    RvInt32 errorCode;
    RvInt32 resultCount;
    RvChar * searchBase;
    RvLdapScope eScope;
    RvChar *searchFilter;
    RvChar * * attributesToReturn;
    LDAPAttrList* attrList;
    RvInt32 nAttrList;

    RvChar *searchUrl;

    HLDAPEntryCb hLastEntry;
    HLDAPAttributeCb hLastAttribute;
    HLDAPValuesCb hLastValues;
    RvInt32 numValues;
    RvUint8 buf[SCRATCH_AREA_SIZE];
    RvSCache scache;
} ldapTransaction_T;

void RvLdapFdEvent(
    IN RvSelectEngine*  selectEngine,
    IN RvSelectFd*      fd,
    IN RvSelectEvents   selectEvent,
    IN RvBool           error);

RvStatus RvLdapBindAnsweredEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest,
    IN HLDAPMessage * arrHMesssage,
    IN RvInt32 numResults,
    IN RvLdapErrorType errorCode);

RvStatus RvLdapSaslBindAnsweredEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest,
    IN HLDAPMessage * arrHMesssage,
    IN RvInt32 numResults,
    IN RvLdapErrorType errorCode);

RvStatus RvLdapBindAbandonedEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest);

RvStatus RvLdapSaslBindAbandonedEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest);

static RvStatus StartTxTimer(ldapTransaction_T *tx);
static RvStatus ContinueTx(ldapTransaction_T *tx, HLDAPRequestCb tid);



/******************************************************************************
 * RvLdapInit
 * ----------------------------------------------------------------------------
 * General: Initialize the LDAP and register to the server. Calling this API
 * again will rebind the LDAP instance to a new server, but would clear all
 * pending request to the former server. Rebinding will re-set the server
 * address, name and password, but not the maximum number of transactions. In
 * order to change that, the user must end the LDAP module and restart it.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapApp          - Application handle to the LDAP instance
 *         pLdapConfig       - Pointer to the configuration structure
 *         ldapConfigSize    - Size of the configuration structure
 *         pLdapCallbacks    - Pointer to the implementation structure
 *         ldapCallbacksSize - Size of the implementation structure
 *         pLdapEvents       - Pointer to the event structure
 *         ldapEventsSize    - Size of the event structure
 * Output: pHLdap            - Pointer to the LDAP handle
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapInit(
    IN HLDAPAPP hLdapApp,
    IN LDAPConfiguration * pLdapConfig,
    IN RvSize_t ldapConfigSize,
    IN LDAPCallbacks * pLdapCallbacks,
    IN RvSize_t ldapCallbacksSize,
    IN LDAPEvents * pLdapEvents,
    IN RvSize_t ldapEventsSize,
    IN RvLogMgr* logMgr,
    OUT HLDAP * pHLdap)
{
    ldapElement_T * ldap;

    RV_UNUSED_ARG(ldapConfigSize);

    /* allocate and clear memory */
    if (RvMemoryAlloc(NULL, sizeof(ldapElement_T), logMgr, (void**)&ldap) != RV_OK)
        return RV_ERROR_OUTOFRESOURCES;
    memset(ldap, 0, sizeof(ldapElement_T));

    /* set the parameters */
    ldap->hLdapApp = hLdapApp;

    ldap->maxTransactions = pLdapConfig->maxTransactions;
    
    if (pLdapConfig->transactionTimeout > 0)
        ldap->transactionTimeout = pLdapConfig->transactionTimeout;
    else
        ldap->transactionTimeout = LDAP_DEFAULT_TX_TIMEOUT_MSEC;

    if (pLdapConfig->maxStringSize > 0)
        ldap->maxStringSize = pLdapConfig->maxStringSize;
    else
        ldap->maxStringSize = LDAP_DEFAULT_STRING_SIZE;

    /* set the events and callbacks */
    memcpy(&ldap->callbacks, pLdapCallbacks, ldapCallbacksSize);
    memcpy(&ldap->events, pLdapEvents, ldapEventsSize);

    /* create log source */
    ldap->logMgr = logMgr;
    if(logMgr != 0) {
        RvLogSourceConstruct(logMgr, &ldap->ldapLog, "LDAP", "LDAP Communication add-on");
        ldap->pLdapLog = &ldap->ldapLog;
    } else {
        ldap->pLdapLog = 0;
    }

    /* Although we have a timer engine, we want to add timers to it, so we try
       to construct one on our own */
    if (RvSelectConstruct(2048, (RvUint32)ldap->maxTransactions, logMgr, &ldap->selectEngine) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: Construct select engine failed"));
        if(ldap->pLdapLog) {
            RvLogSourceDestruct(ldap->pLdapLog);
        }
        RvMemoryFree(ldap, logMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    if (RvSelectGetTimeoutInfo(ldap->selectEngine, NULL, &ldap->timerQ) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: Construct select engine failed"));
        RvSelectDestruct(ldap->selectEngine, (RvUint32)ldap->maxTransactions);
        if(ldap->pLdapLog) {
            RvLogSourceDestruct(ldap->pLdapLog);
        }
        RvMemoryFree(ldap, logMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    /* Construct EMA for the transactions */
    if (RvEmaConstruct(sizeof(ldapTransaction_T)+ldap->maxStringSize,
                       pLdapConfig->maxTransactions, RvEmaNormalLocks, "ldapTx", 0, NULL,
                       ldap, logMgr, &ldap->txEma) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: EMA allocation failed"));
        RvSelectDestruct(ldap->selectEngine, (RvUint32)ldap->maxTransactions);
        if(ldap->pLdapLog) {
            RvLogSourceDestruct(ldap->pLdapLog);
        }
        RvMemoryFree(ldap, logMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    /* initialize the LDAP implementation */
    if (ldap->callbacks.RvLdapInitialize(pLdapConfig->strLdapSeverAddress, &ldap->hLdap) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: LDAP implementation initialization failed"));
        RvEmaDestruct(ldap->txEma);
        RvSelectDestruct(ldap->selectEngine, (RvUint32)ldap->maxTransactions);
        if(ldap->pLdapLog) {
            RvLogSourceDestruct(ldap->pLdapLog);
        }
        RvMemoryFree(ldap, logMgr);
        return RV_ERROR_OUTOFRESOURCES;
    }

    /* set the state */
    ldap->eState = RvLdapStateInitialized;
    ldap->activeTransactions = 0;
    
    *pHLdap = (HLDAP)ldap;

    RvLogInfo(ldap->pLdapLog, (ldap->pLdapLog, "LDAP Initialized, hApp=%p, ldap=%p", hLdapApp, ldap));

    return RV_OK;
}


/******************************************************************************
 * RvLdapSetSessionOption
 * ----------------------------------------------------------------------------
 * General: Set session preferences.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 *         ldapOption        - The name of the option to be modified
 *         ldapData          - Pointer to the option value
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapSetSessionOption(
    IN HLDAP hLdap,
    IN RvLdapOption ldapOption,
    IN void* ldapData)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    RvStatus status;

    status = ldap->callbacks.RvLdapSetOption(ldap->hLdap, ldapOption, ldapData);
    if (status != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "RvLdapSetSessionOption: status=%d", status));
        return RV_ERROR_UNKNOWN;
    }

    return RV_OK;
}


/******************************************************************************
 * RvLdapConnect
 * ----------------------------------------------------------------------------
 * General: Connect to LDAP server.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 *         pLdapConfig       - Pointer to the configuration structure
 * Output: None.
 *****************************************************************************/
static RvStatus RvLdapSimpleConnect(
    IN HLDAP hLdap,
    IN LDAPConfiguration * pLdapConfig)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    ldapTransaction_T * bindTx;
    RvSocket sock;
    RvStatus s;
    
    RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapConnect(ldap=%p, ldapConfig=%p)",
        ldap, pLdapConfig));
    if(pLdapConfig->bUseTls == RV_TRUE) {
        s = ldap->callbacks.RvLdapStartTls(ldap->hLdap, 0);
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapConnect: StartTls failed"));
        if(s != RV_OK) {
            return s;
        }
    }
    
    /* now, bind to the server */
    if (RvLdapStartTx((HLDAP)ldap, (HLDAPAppRequest)NULL, (HLDAPRequest*)&bindTx) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: LDAP transaction initialization failed"));
        return RV_ERROR_OUTOFRESOURCES;
    }

    bindTx->requestType = RvLdapRequestTypeConnect;
    bindTx->events.RvLdapTxAnsweredEv = RvLdapBindAnsweredEv;
    bindTx->events.RvLdapTxAbandonedEv = RvLdapBindAbandonedEv;

    if (ldap->callbacks.RvLdapBind(ldap->hLdap, pLdapConfig->eConnectionType,
        pLdapConfig->strUsername, pLdapConfig->strPassword, (HLDAPRequestCb *)&bindTx->hRequest) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: LDAP implementation bind failed"));
        RvLdapEndTx((HLDAPRequest)bindTx);
        return RV_ERROR_OUTOFRESOURCES;
    }

    if(StartTxTimer(bindTx) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: cannot set bind timeout"));
        ldap->callbacks.RvLdapUnbind(ldap->hLdap);
        RvLdapEndTx((HLDAPRequest)bindTx);
        return RV_ERROR_OUTOFRESOURCES;
    }

    bindTx->eState = RvLdapRequestStateSent;

    /* get the select FD */
    if (ldap->callbacks.RvLdapGetFd(ldap->hLdap, &sock) != RV_OK)
    {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog,
            "LDAP Initialization Error: getting file descriptor failed"));
        ldap->callbacks.RvLdapUnbind(ldap->hLdap);
        RvLdapEndTx((HLDAPRequest)bindTx);
        return RV_ERROR_OUTOFRESOURCES;
    }

    RvFdConstruct(&ldap->fd, &sock, ldap->logMgr);

    RvSelectAdd(ldap->selectEngine, &ldap->fd, RvSelectRead | RvSelectClose, RvLdapFdEvent);
    
    RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapConnect(ldap=%p, ldapConfig=%p)",
        ldap, pLdapConfig));

    return RV_OK;
}

#define AdjustMemSize(fieldName)  \
    if(curInParams->fieldName != 0) { \
    fieldSize = strlen(curInParams->fieldName); \
    memSize += fieldSize + 1; \
    }

#define CopyField(fieldName) \
{ \
    const RvChar *fieldName = curInParams->fieldName; \
    if(fieldName == 0) { \
    curParams->fieldName = 0; \
    } else { \
    fieldSize = strlen(fieldName); \
    curParams->fieldName = cur; \
    strcpy(cur, fieldName); \
    cur += fieldSize + 1; \
    } \
}

static RvStatus CreateSaslConnectCtx(RvLdapConnectParams *pSaslConnectParams, 
                                   RvSize_t nSaslConnectParams,
                                   RvLdapSaslConnectCtx **ppCtx) {

    RvSize_t i;
    RvSize_t memSize = sizeof(RvLdapSaslConnectCtx) + RV_ALIGN_DATASIZE + nSaslConnectParams * sizeof(*pSaslConnectParams) + 1;
    RvChar *buf;
    RvStatus s = RV_OK;
    RvLdapSaslConnectCtx *pCtx;
    RvChar *cur;
    RvSize_t fieldSize;



    for(i = 0; i < nSaslConnectParams; i++) {
        RvLdapConnectParams *curInParams = &pSaslConnectParams[i];

        if(pSaslConnectParams->mech == 0) {
            return RV_ERROR_BADPARAM;
        }

        fieldSize = strlen(pSaslConnectParams->mech);
        /* Mech appears twice */
        memSize += 2 * fieldSize + 2;

        AdjustMemSize(authcid);
        AdjustMemSize(authzid);
        AdjustMemSize(passwd); 
        AdjustMemSize(realm);
    }


    s = RvMemoryAlloc(0, memSize, 0, (void **)&buf);
    if(s != RV_OK) {
        return s;
    }

    cur = buf;
    pCtx = (RvLdapSaslConnectCtx *)cur;
    pCtx->choosenMech = 0;
    pCtx->resultMsg = 0;
    pCtx->msgId = 0;
    cur += sizeof(*pCtx);
    cur = (RvChar *)RvAlignData(cur);

    pCtx->pSaslConnectParams = (RvLdapConnectParams*)cur;
    pCtx->nSaslConnectParams = nSaslConnectParams;

    cur += sizeof(*pSaslConnectParams) * nSaslConnectParams;


    for(i = 0; i < nSaslConnectParams; i++) {
        RvLdapConnectParams *curParams = &pCtx->pSaslConnectParams[i];
        RvLdapConnectParams *curInParams = &pSaslConnectParams[i];

        CopyField(authcid);
        CopyField(authzid);
        CopyField(mech);
        CopyField(passwd);
        CopyField(realm);
    }
    
    pCtx->mechs = cur;

    for(i = 0; i < nSaslConnectParams; i++) {
        RvLdapConnectParams *curParams = &pSaslConnectParams[i];

        strcpy(cur, curParams->mech);
        cur += strlen(cur);
        *cur = ' ';
        cur++;
    }

    cur[-1] = 0;
    *ppCtx = pCtx;
    return RV_OK;
}

static RvStatus DeleteSaslConnectCtx(RvLdapSaslConnectCtx *pCtx) {
    RvStatus s;

    s = RvMemoryFree(pCtx, 0);
    return s;
}



static RvStatus RvLdapSaslConnect(
    IN HLDAP hLdap, 
    IN LDAPConfiguration *pLdapConfig) {

#define SASL_CTX_CREATED 1
#define TX_CREATED 2
#define BIND_STARTED 4
#define TIMER_STARTED 8

    ldapElement_T *ldap = (ldapElement_T *)hLdap;
    ldapTransaction_T *bindTx = 0;
    RvSocket sock;
    RvStatus s;
    RvLdapConnectParams *pSaslConnectParams = pLdapConfig->pSaslConnectParams;
    RvSize_t nSaslConnectParams = pLdapConfig->nSaslConnectParams;
    RvLdapSaslConnectCtx *pSaslCtx = 0;
    RvInt stage = 0;

    RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSaslConnect(ldap=%p, ldapConfig=%p)",
        ldap, pLdapConfig));

    if(ldap->callbacks.RvLdapSaslBind == 0) {
        return RV_ERROR_NOTSUPPORTED;
    }

    if(pSaslConnectParams == 0 || nSaslConnectParams == 0) {
        RvLogError(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSaslConnect: no SASL connect params found"));
        return RV_ERROR_BADPARAM;
    }

    s = CreateSaslConnectCtx(pSaslConnectParams, nSaslConnectParams, &pSaslCtx);
    if(s != RV_OK) {
        goto failure;
    }

    stage |= SASL_CTX_CREATED;

    if(pLdapConfig->bUseTls == RV_TRUE) {
        s = ldap->callbacks.RvLdapStartTls(ldap->hLdap, 0);
        if(s != RV_OK) {
            RvLogError(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSaslConnect: StartTls failed"));
            goto failure;
        }
    }

    /* now, bind to the server */
    s = RvLdapStartTx((HLDAP)ldap, (HLDAPAppRequest)NULL, (HLDAPRequest*)&bindTx);
    if(s != RV_OK) {
        goto failure;
    }
    bindTx->requestType = RvLdapResquestTypeSaslConnect;
    bindTx->pSaslConnectCtx = pSaslCtx;
    pSaslCtx = 0;
    bindTx->events.RvLdapTxAnsweredEv = RvLdapSaslBindAnsweredEv;
    bindTx->events.RvLdapTxAbandonedEv = RvLdapSaslBindAbandonedEv;

    s = ldap->callbacks.RvLdapSaslBind(ldap->hLdap, bindTx->pSaslConnectCtx);
    bindTx->hRequest = (HLDAPRequestCb)bindTx->pSaslConnectCtx->msgId;
    if(s != RV_OK && s != RvLdapErrorTypeSaslBindInProgress) {
        goto failure;
    }

    stage |= BIND_STARTED;

    s = StartTxTimer(bindTx);

    if(s != RV_OK) {
        goto failure;
    }

    stage |= TIMER_STARTED;
    bindTx->eState = RvLdapRequestStateSent;

    s = ldap->callbacks.RvLdapGetFd(ldap->hLdap, &sock);
    if(s != RV_OK) {
        goto failure;
    }

    RvFdConstruct(&ldap->fd, &sock, ldap->logMgr);
    RvSelectAdd(ldap->selectEngine, &ldap->fd, RvSelectRead | RvSelectClose, RvLdapFdEvent);

    RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSaslConnect(ldap=%p, ldapConfig=%p)",
        ldap, pLdapConfig));

    return RV_OK;
failure:
    if(stage & SASL_CTX_CREATED && pSaslCtx) {
        DeleteSaslConnectCtx(pSaslCtx);
    }

    if(stage & TX_CREATED) {
        RvLdapEndTx((HLDAPRequest)bindTx);
    }

    if(stage & BIND_STARTED) {
        ldap->callbacks.RvLdapUnbind(ldap->hLdap);
    }

    if(stage & TIMER_STARTED) {
        RvTimerCancel(&bindTx->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
    }

    return s;

}

RVAPI RvStatus RVCALLCONV RvLdapConnect(IN HLDAP hLdap, IN LDAPConfiguration *pLdapConfig) {
    switch(pLdapConfig->eConnectionType) {
        case ldapConnectionMethodSasl:
            return RvLdapSaslConnect(hLdap, pLdapConfig);
        case ldapConnectionMethodSimple:
        case ldapConnectionMethodKRBV41:
        case ldapConnectionMethodKRBV42:
            return RvLdapSimpleConnect(hLdap, pLdapConfig);
        default:
            return RV_ERROR_BADPARAM;
    }
}


/******************************************************************************
 * RvLdapEnd
 * ----------------------------------------------------------------------------
 * General: Close the LDAP stack and free its memory.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapEnd(
    IN HLDAP hLdap)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    EMAElement elem = NULL, next;

    RvLogInfo(ldap->pLdapLog, (ldap->pLdapLog, "LDAP Terminating, hApp=%p, ldap=%p", ldap->hLdapApp, ldap));

    RvSelectRemove(ldap->selectEngine, &ldap->fd);
    RvFdDestruct(&ldap->fd);
    ldap->callbacks.RvLdapUnbind(ldap->hLdap);
    RvSelectDestruct(ldap->selectEngine, (RvUint32)ldap->maxTransactions);

    elem = RvEmaGetNext(ldap->txEma, elem);
    while (elem != NULL)
    {
        next = RvEmaGetNext(ldap->txEma, elem);
        RvLdapEndTx((HLDAPRequest)elem);
        elem = next;
    }

    RvEmaDestruct(ldap->txEma);
    if(ldap->pLdapLog) {
        RvLogSourceDestruct(ldap->pLdapLog);
    }
    RvMemoryFree(ldap, ldap->logMgr);
    return RV_OK;
}


/******************************************************************************
 * RvLdapGetParam
 * ----------------------------------------------------------------------------
 * General: This function may be used to see the current configuration values,
 * the module state, the current number of waiting transactions, etc.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 *         eParam            - Parameter to get
 *         value             - Size of the memory given in the next pointer
 * Output: value             - Value of the parameter or its real size
 *         string            - Pointer to the parameter, if not a value.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapGetParam(
    IN HLDAP hLdap,
    IN RvLdapParm eParam,
    INOUT RvInt32 * value,
    OUT RvChar * string)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    RvStatus res = RV_OK;

    RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapGetParam(ldap=%p, param=%d, value=%d)",
        ldap, eParam, *value));

    switch(eParam)
    {
    case (RvLdapParmState):
        *value = (RvInt32)ldap->eState;
        break;

    case (RvLdapParmNumActiveTransactions):
        *value = ldap->activeTransactions;
        break;

    case (RvLdapParmDefaultTimeout):
        *value = ldap->transactionTimeout;
        break;

    case (RvLdapParmDefaultEvents):
        if (*value >= (RvInt32)sizeof(ldap->events))
        {
            memcpy(string, &ldap->events, sizeof(ldap->events));
        }
        else
        {
            res = RV_ERROR_INSUFFICIENT_BUFFER;
        }
        *value = sizeof(ldap->events);
        break;

    default:
        res = RV_ERROR_BADPARAM;
    }

    RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapGetParam(ldap=%p, param=%d, value=%d) = %d",
        ldap, eParam, *value, res));
    return res;
}


/******************************************************************************
 * RvLdapSetParam
 * ----------------------------------------------------------------------------
 * General: This function may be used to set some configuration values.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 *         eParam            - Parameter to set
 *         value             - Value of the parameter or its size
 *         string            - Pointer to the parameter, if not a value.
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapSetParam(
    IN HLDAP hLdap,
    IN RvLdapParm eParam,
    IN RvInt32 value,
    IN const RvChar * string)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    RvStatus res = RV_OK;

    RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSetParam(ldap=%p, param=%d, value=%d)",
        ldap, eParam, value));

    switch(eParam)
    {
    case (RvLdapParmState):
        res = RV_ERROR_ILLEGAL_ACTION;
        break;
    
    case (RvLdapParmNumActiveTransactions):
        res = RV_ERROR_ILLEGAL_ACTION;
        break;

    case (RvLdapParmDefaultTimeout):
        ldap->transactionTimeout = value;
        break;

    case (RvLdapParmDefaultEvents):
        if ((RvSize_t)value > sizeof(ldap->events))
            value = sizeof(ldap->events);

        memset(&ldap->events, 0, sizeof(ldap->events));
        memcpy(&ldap->events, string, (RvSize_t)value);
        break;

    default:
        res = RV_ERROR_BADPARAM;
    }

    RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSetParam(ldap=%p, param=%d, value=%d) = %d",
        ldap, eParam, value, res));
    return res;
}


/******************************************************************************
 * RvLdapGetLastError
 * ----------------------------------------------------------------------------
 * General: This function gets the last error from the LDAP stack.
 *
 * Return Value: The error type.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 * Output: None.
 *****************************************************************************/
RVAPI RvLdapErrorType RVCALLCONV RvLdapGetLastError(
    IN HLDAP hLdap)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    RvLdapErrorType errorCode;

    /* check if there is any general error */
    errorCode = ldap->callbacks.RvLdapGetErrorNumber(ldap->hLdap);

    if (errorCode == RvLdapErrorTypeSuccess)
    {
        /* maybe there is an error in one of the transactions */
        ldapTransaction_T * tx = NULL;
        RvBool bStop = RV_FALSE;

        while (!bStop && ((tx=(ldapTransaction_T *)RvEmaGetNext(ldap->txEma, (EMAElement)tx)) != NULL))
        {
            if (RvEmaLock((EMAElement)tx) == RV_OK)
            {
                if (tx->errorCode != RvLdapErrorTypeSuccess)
                {
                    /* found a transaction with an error, return that error */
                    errorCode = tx->errorCode;
                    bStop = RV_TRUE;
                }
                RvEmaUnlock((EMAElement)tx);
            }
        }
    }
    return errorCode;
}


/******************************************************************************
 * This is the Request API for use of the application. This API will also be
 * used by functions in other modules when communication with the LDAP service.
 *****************************************************************************/


/******************************************************************************
 * RvLdapStartTx
 * ----------------------------------------------------------------------------
 * General: This function is used to create an LDAP transaction.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 *         hAppRequest       - Application handle to the request
 * Output: pHRequest         - The handle to the request
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapStartTx(
    IN HLDAP hLdap,
    IN HLDAPAppRequest hAppRequest,
    OUT HLDAPRequest * pHRequest)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    ldapTransaction_T * tx;

    if (RvEmaAdd(ldap->txEma, (void*)hAppRequest, (EMAElement*)&tx) != RV_OK)
        return RV_ERROR_OUTOFRESOURCES;

    memset(tx, 0, sizeof(*tx)+ldap->maxStringSize);

    tx->ldap = ldap;
    tx->eState = RvLdapRequestStateAllocated;
    tx->eScope = ldapScopeBase;
    tx->transactionTimeout = ldap->transactionTimeout;
    tx->events.RvLdapTxAnsweredEv = ldap->events.RvLdapTxAnsweredEv;
    tx->events.RvLdapTxAbandonedEv = ldap->events.RvLdapTxAbandonedEv;
    RvSCacheConstruct(&tx->scache, tx->buf, sizeof(tx->buf));

    ldap->activeTransactions++;

    RvLogInfo(ldap->pLdapLog, (ldap->pLdapLog,
        "RvLdapStartTx: tx=%p", tx));
    
    *pHRequest = (HLDAPRequest)tx;
    return RV_OK;
}


/******************************************************************************
 * RvLdapGetTxParam
 * ----------------------------------------------------------------------------
 * General: This function is used to get the transaction state and the search
 * values, and after receiving a response to get result values
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hRequest          - The LDAP request handle
 *         hMessage          - The LDAP message handle, if getting a result param
 *         eParam            - Parameter to get
 *         strAttribute      - Attribute string, when needed
 *         index             - Index of the parameter to get
 *         value             - Size of the memory given in the next pointer
 * Output: value             - Value of the parameter or its real size
 *         string            - Pointer to the parameter, if not a value.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapGetTxParam(
    IN HLDAPRequest hRequest,
    IN HLDAPMessage hMessage,
    IN RvLdapRequestParam eParam,
    IN const RvChar * strAttribute,
    IN RvInt32 index,
    INOUT RvInt32 * value,
    OUT RvChar * string)
{
    ldapTransaction_T * tx = (ldapTransaction_T *) hRequest;
    ldapElement_T * ldap = tx->ldap;
    RvStatus res = RV_OK;

    if (RvEmaLock((EMAElement)tx) != RV_OK)
    {
        return RV_ERROR_INVALID_HANDLE;
    }

    RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapGetTxParam(ldap=%p, tx=%p, msg=%p, param=%d, "
        "strAttr=%s, index=%d, value=%d)", ldap, tx, hMessage, eParam,
        (strAttribute != NULL)? strAttribute : "(null)", index, *value));

    switch (eParam)
    {
    case(RvLdapRequestParamState):
        *value = (RvInt32)tx->eState;
        break;

    case(RvLdapRequestParamTimeout):
        *value = (RvInt32)tx->transactionTimeout;
        break;

    case(RvLdapRequestParamCallback):
        if (*value >= (RvInt32)sizeof(tx->events))
        {
            memcpy(string, &tx->events, sizeof(tx->events));
        }
        else
        {
            res = RV_ERROR_INSUFFICIENT_BUFFER;
        }
        *value = sizeof(tx->events);
        break;

    case(RvLdapRequestParamScope):
        *value = (RvInt32)tx->eScope;
        break;

    case RvLdapRequestParamPageSize:
        *value = (RvInt32)tx->pagedSearchCtx.pageSize;
        break;

    case(RvLdapRequestParamSearchBase):
        if (*value < (RvInt32)sizeof(RvChar*))
        {
            res = RV_ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        *value = sizeof(RvChar*);
        *(RvChar**)string = tx->searchBase;
        break;

    case(RvLdapRequestParamSearchFilter):
        if (*value < (RvInt32)sizeof(RvChar*))
        {
            res = RV_ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        *value = sizeof(RvChar*);
        *(RvChar**)string = tx->searchFilter;
        break;

    case RvLdapRequestParamSearchUrl:
        if (*value < (RvInt32)sizeof(RvChar*))
        {
            res = RV_ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        *value = sizeof(RvChar*);
        *(RvChar**)string = tx->searchUrl;
        break;

    case(RvLdapRequestParamAttributesToReturn):
        if (*value < (RvInt32)sizeof(RvChar*))
        {
            res = RV_ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        *value = sizeof(RvChar*);
        *(RvChar***)string = tx->attributesToReturn;
        break;
        
    case RvLdapRequestParamAttrList:
        *(LDAPAttrList**)string = tx->attrList;
        *value = tx->nAttrList;
        break;
        
    case(RvLdapRequestParamMessageDistinguishedName):
        if (ldap->callbacks.RvLdapGetEntryDn != NULL)
        {
            res = ldap->callbacks.RvLdapGetEntryDn(ldap->hLdap, (HLDAPEntryCb)hMessage, value, string);
        }
        else
        {
            res = RV_ERROR_NOTSUPPORTED;
        }
        break;

    case(RvLdapRequestParamMessageFirstAttribute):
        if (hMessage == NULL)
        {
            res = RV_ERROR_NULLPTR;
            break;
        }
        if (tx->hLastEntry != NULL)
        {
            /* free the previous values */
            ldap->callbacks.RvLdapFreeResult(ldap->hLdap, NULL, tx->hLastAttribute, tx->hLastValues);
            tx->hLastAttribute = NULL;
            tx->hLastValues = NULL;
            LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)[0] = 0;
            tx->numValues = -1;
        }
        {
            RvInt32 length = ldap->maxStringSize;

            tx->hLastEntry = (HLDAPEntryCb)hMessage;

            res = ldap->callbacks.RvLdapResultGetNextAtribute(ldap->hLdap, (HLDAPEntryCb)hMessage,
                &tx->hLastAttribute, &tx->hLastValues, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx), &length, &tx->numValues);
            if (res == RV_OK)
            {
                if (*value < (RvInt32)sizeof(RvChar*))
                {
                    res = RV_ERROR_INSUFFICIENT_BUFFER;
                    break;
                }
                /* set value to the attribute's data len */
                *value = length;
                *(RvChar**)string = LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx);
            }
        }
        break;

    case(RvLdapRequestParamMessageNextAttribute):
        if (hMessage == NULL)
        {
            res = RV_ERROR_NULLPTR;
        }
        else if (hMessage != (HLDAPMessage)tx->hLastEntry)
        {
            res = RV_ERROR_ILLEGAL_ACTION;
        }
        else
        {
            RvInt32 length = ldap->maxStringSize;

            res = ldap->callbacks.RvLdapResultGetNextAtribute(ldap->hLdap, (HLDAPEntryCb)hMessage,
                &tx->hLastAttribute, &tx->hLastValues, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx), &length, &tx->numValues);
            if (res == RV_OK)
            {
                if (*value < (RvInt32)sizeof(RvChar*))
                {
                    res = RV_ERROR_INSUFFICIENT_BUFFER;
                    break;
                }
                /* set value to the attribute's data len */
                *value = length;
                *(RvChar**)string = LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx);
            }
        }
        break;

    case(RvLdapRequestParamMessageAttributeNumberOfValues):
        if (hMessage == NULL)
        {
            res = RV_ERROR_NULLPTR;
            break;
        }
        if ((hMessage == (HLDAPMessage)tx->hLastEntry) &&
            (strcmp(strAttribute, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)) == 0))
        {
            /* we need to get the number of values for the current attribute */
            *value = tx->numValues;
            break;
        }
        /* we need to find a new attribute */
        if ((tx->hLastEntry != NULL))
        {
            /* free the previous values */
            ldap->callbacks.RvLdapFreeResult(ldap->hLdap, NULL, tx->hLastAttribute, tx->hLastValues);
            tx->hLastAttribute = NULL;
            tx->hLastValues = NULL;
            LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)[0] = 0;
            tx->numValues = -1;
        }
        {
            RvInt32 length;

            tx->hLastEntry = (HLDAPEntryCb)hMessage;

            while (res == RV_OK)
            {
                length = ldap->maxStringSize;

                res = ldap->callbacks.RvLdapResultGetNextAtribute(ldap->hLdap, (HLDAPEntryCb)hMessage,
                    &tx->hLastAttribute, &tx->hLastValues, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx), &length, &tx->numValues);

                if ((res == RV_OK) && (strcmp(strAttribute, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)) == 0))
                {
                    *value = tx->numValues;
                    break;
                }
            }
        }
        break;

    case(RvLdapRequestParamMessageAttributeValue):
        if (hMessage == NULL)
        {
            res = RV_ERROR_NULLPTR;
            break;
        }
        if ((hMessage == (HLDAPMessage)tx->hLastEntry) &&
            (strcmp(strAttribute, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)) == 0))
        {
            /* we need to get the indexed value for the current attribute */
            res = ldap->callbacks.RvLdapGetAtributeValue(ldap->hLdap,
                tx->hLastAttribute, tx->hLastValues, index, value, (RvChar**)string);
            break;
        }
        /* we need to get a new attribute */
        if ((tx->hLastEntry != NULL))
        {
            /* free the previous values */
            ldap->callbacks.RvLdapFreeResult(ldap->hLdap, NULL, tx->hLastAttribute, tx->hLastValues);
            tx->hLastAttribute = NULL;
            tx->hLastValues = NULL;
            LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)[0] = 0;
            tx->numValues = -1;
        }
        {
            RvInt32 length;

            tx->hLastEntry = (HLDAPEntryCb)hMessage;

            while (res == RV_OK)
            {
                length = ldap->maxStringSize;

                res = ldap->callbacks.RvLdapResultGetNextAtribute(ldap->hLdap, (HLDAPEntryCb)hMessage,
                    &tx->hLastAttribute, &tx->hLastValues, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx), &length, &tx->numValues);

                if ((res == RV_OK) && (strcmp(strAttribute, LDAP_TX_GET_LAST_ATTRIBUTE_STRING(tx)) == 0))
                {
                    /* we found the attribute, get the indexed value for the current attribute */
                    res = ldap->callbacks.RvLdapGetAtributeValue(ldap->hLdap,
                        tx->hLastAttribute, tx->hLastValues, index, value, (RvChar**)string);
                    break;
                }
            }
        }
        break;

    case(RvLdapRequestParamMessageErrorString):
        {
            const RvChar * str = ldap->callbacks.RvLdapGetErrorString(tx->errorCode);
            if (*value < (RvInt32)strlen(str))
            {
                res = RV_ERROR_INSUFFICIENT_BUFFER;
                break;
            }
            *value = (RvInt32)strlen(str);
            strcpy(string, str);
        }
        break;

    case RvLdapRequestHasMoreResults:
        *value = RvLdapTxHasMoreResults(hRequest);
        break;

    default:
        res = RV_ERROR_BADPARAM;
    }

    RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapGetTxParam(ldap=%p, tx=%p, msg=%p, param=%d, "
        "strAttr=%s, index=%d, value=%d)", ldap, tx, hMessage, eParam,
        (strAttribute != NULL)? strAttribute : "(null)", index, *value));
    RvEmaUnlock((EMAElement)tx);
    return res;
}

RVAPI RvInt RvLdapTxHasMoreResults(IN HLDAPRequest hRequest) {
    ldapTransaction_T *tx = (ldapTransaction_T *)hRequest;

    if((tx->requestType != RvLdapRequestTypeSearch && tx->requestType != RvLdapRequestTypeUrlSearch) ||
        tx->pagedSearchCtx.bAnswered == 0) {
        return -1;
    }

    return tx->pagedSearchCtx.cookie != 0;
}



/******************************************************************************
 * RvLdapSetTxParam
 * ----------------------------------------------------------------------------
 * General: This function is used prior to sending a transaction to set the
 * transaction parameters. Parameters include the transaction scope, the search
 * base, the search filter, the attributes to return, and the timeout. It is
 * also possible to set the callback functions for the transaction results. If
 * not set, the default events will be raised.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hRequest          - The LDAP request handle
 *         eParam            - Parameter to set
 *         index             - Index of the parameter to set
 *         value             - Value of the parameter or its size
 *         string            - Pointer to the parameter, if not a value.
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapSetTxParam(
    IN HLDAPRequest hRequest,
    IN RvLdapRequestParam eParam,
    IN RvInt32 index,
    IN RvInt32 value,
    IN const RvChar * string)
{
    ldapTransaction_T * tx = (ldapTransaction_T *)hRequest;
    RvStatus res = RV_OK;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    ldapElement_T * ldap = tx->ldap;
#else
    RV_UNUSED_ARG(index);
#endif

    if (RvEmaLock((EMAElement)tx) != RV_OK)
    {
        return RV_ERROR_INVALID_HANDLE;
    }

    RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSetTxParam(ldap=%p, tx=%p, param=%d, index=%d, value=%d)",
        ldap, tx, eParam, index, value));

    switch (eParam)
    {
    case RvLdapRequestParamState:
        res = RV_ERROR_ILLEGAL_ACTION;
        break;

    case RvLdapRequestParamTimeout:
        if (tx->eState == RvLdapRequestStateAllocated)
        {
            tx->transactionTimeout = value;
        }
        else
            res = RV_ERROR_ILLEGAL_ACTION;
        break;

    case RvLdapRequestParamCallback:
        if ((RvSize_t)value > sizeof(tx->events))
            value = sizeof(tx->events);

        memset(&tx->events, 0, sizeof(tx->events));
        memcpy(&tx->events, string, (RvSize_t)value);
        break;

    case RvLdapRequestParamRequestType:
        /* Changed by Ran, 5-7-05.  Notify SIP server guys */
        tx->requestType = (RvLdapRequestType)value;
        break;

    case RvLdapRequestParamPageSize:
        tx->pagedSearchCtx.pageSize = (RvUlong)value;
        break;

    case RvLdapRequestParamScope:
        if (tx->eState == RvLdapRequestStateAllocated)
        {
            if (value > ldapScope_MIN && value < ldapScope_MAX)
                tx->eScope = value;
            else
                res = RV_ERROR_BADPARAM;
        }
        else
            res = RV_ERROR_ILLEGAL_ACTION;
        break;

    case RvLdapRequestParamSearchBase:
        res = RvSCacheStrDup(&tx->scache, string, &tx->searchBase);
        break;

    case RvLdapRequestParamSearchFilter:
        res = RvSCacheStrDup(&tx->scache, string, &tx->searchFilter);
        break;

    case RvLdapRequestParamSearchUrl:
        res = RvSCacheStrDup(&tx->scache, string, &tx->searchUrl);
        break;

    case RvLdapRequestParamAttributesToReturn:
        res = RvSCacheCopyStringArray(&tx->scache, (RvChar **)string, &tx->attributesToReturn);
        break;

    case RvLdapRequestParamAttrList:
        tx->attrList  = (LDAPAttrList*)string;
        tx->nAttrList = value;
        break;

    case RvLdapRequestParamMessageDistinguishedName:
    case RvLdapRequestParamMessageFirstAttribute:
    case RvLdapRequestParamMessageNextAttribute:
    case RvLdapRequestParamMessageAttributeNumberOfValues:
    case RvLdapRequestParamMessageAttributeValue:
        res = RV_ERROR_ILLEGAL_ACTION;
        break;
        
    default:
        res = RV_ERROR_BADPARAM;
    }

    RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog, "RvLdapSetTxParam(ldap=%p, tx=%p, param=%d, index=%d, value=%d)",
        ldap, tx, eParam, index, value));
    RvEmaUnlock((EMAElement)tx);
    return res;
}



/******************************************************************************
 * RvLdapSendTx
 * ----------------------------------------------------------------------------
 * General: This function sends the transaction to the LDAP server.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdap             - The LDAP instance handle
 *         hRequest          - The LDAP request handle
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapSendTx(
    IN HLDAP hLdap,
    IN HLDAPRequest hRequest)
{
    ldapElement_T * ldap = (ldapElement_T *)hLdap;
    ldapTransaction_T * tx = (ldapTransaction_T *)hRequest;
    RvStatus res = RV_ERROR_UNKNOWN;

    if (RvEmaLock((EMAElement)tx) == RV_OK)
    {
        RvLogEnter(ldap->pLdapLog, (ldap->pLdapLog,
            "RvLdapSendTx: tx=%p", hRequest));

        switch(tx->requestType) {
        case RvLdapRequestTypeSearch:
            res = ldap->callbacks.RvLdapStartSearch(ldap->hLdap, tx->searchBase, tx->eScope,
                                                    tx->searchFilter, tx->attributesToReturn, 
                                                    tx->pagedSearchCtx.pageSize,
                                                    tx->pagedSearchCtx.cookie,
                                                    &tx->hRequest);
        	break;

        case RvLdapRequestTypeUrlSearch:
            res = ldap->callbacks.RvLdapStartUrlSearch(ldap->hLdap, tx->searchUrl, &tx->hRequest);
            break;
            
        case RvLdapRequestTypeDelete:
            res = ldap->callbacks.RvLdapDelete(ldap->hLdap, tx->searchBase, &tx->hRequest);
        	break;
            
        case RvLdapRequestTypeAdd:
            res = ldap->callbacks.RvLdapAdd(ldap->hLdap, tx->searchBase, tx->attrList,
                                            tx->nAttrList, &tx->hRequest);
        	break;
            
        case RvLdapRequestTypeModify:
            res = ldap->callbacks.RvLdapModify(ldap->hLdap, tx->searchBase, tx->attrList,
                                               tx->nAttrList, &tx->hRequest);
        	break;

        default:
            break;
        }

        if (res == RV_OK)
        {
            res = StartTxTimer(tx);
            if (res == RV_OK)
                tx->eState = RvLdapRequestStateSent;
        }

        RvLogLeave(ldap->pLdapLog, (ldap->pLdapLog,
            "RvLdapSendTx: tx=%p [%d]", hRequest, res));
        RvEmaUnlock((EMAElement)tx);
    }
    return res;
}


/******************************************************************************
 * RvLdapEndTx
 * ----------------------------------------------------------------------------
 * General: This function will free an LDAP query.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hRequest          - The LDAP request handle
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapEndTx(
    IN HLDAPRequest hRequest)
{
    ldapTransaction_T * tx = (ldapTransaction_T *)hRequest;
    RvSCache *scache = &tx->scache;

    if (RvEmaLock((EMAElement)tx) == RV_OK)
    {
        struct berval *cookie;

        RvLogInfo(tx->ldap->pLdapLog, (tx->ldap->pLdapLog,
            "RvLdapEndTx(tx=%p)", tx));

        if(tx->pSaslConnectCtx) {
            DeleteSaslConnectCtx(tx->pSaslConnectCtx);
            tx->pSaslConnectCtx = 0;
        }

        cookie = tx->pagedSearchCtx.cookie;

        if(cookie) {
            ber_bvfree(cookie);
            tx->pagedSearchCtx.cookie = 0;
        }

        if(tx->eState == RvLdapRequestStateSent) {
            tx->ldap->callbacks.RvLdapEndRequest(tx->ldap->hLdap, tx->hRequest);
        }

        if (tx->eState >= RvLdapRequestStateSent)
        {
            RvTimerCancel(&tx->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        }
        tx->ldap->callbacks.RvLdapFreeResult(tx->ldap->hLdap, tx->hResponse, tx->hLastAttribute, tx->hLastValues);
        RvSCacheFree(scache, tx->searchBase);
        RvSCacheFree(scache, tx->searchFilter);
        RvSCacheFree(scache, tx->searchUrl);
        RvSCacheFreeStringArray(scache, tx->attributesToReturn);

        RvEmaDelete((EMAElement)tx);
        RvEmaUnlock((EMAElement)tx);
    }

    return RV_OK;
}

static RvStatus StartTxTimer(ldapTransaction_T *tx) {
    RvStatus s;
    ldapElement_T *ldap = tx->ldap;

    s = RvTimerStart(&tx->timer, ldap->timerQ, RV_TIMER_TYPE_ONESHOT,
                      RvInt64Mul(RvInt64FromRvInt(tx->transactionTimeout), RV_TIME64_NSECPERMSEC),
                      RvLdapTimerEvent, (void *)tx);
    return s;
}

#if 0
static RvStatus ResetTx(ldapTransaction_T *tx) {
    EMAElement etx = (EMAElement)tx;
    RvStatus s = RV_OK;
    ldapElement_T *ldap = tx->ldap;

    RvEmaLock(etx);

    if(tx->eState != RvLdapRequestStateAnswered) {
        s = RV_ERROR_ILLEGAL_ACTION;
        goto end;
    }

    RvTimerCancel(&tx->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
    ldap->callbacks.RvLdapFreeResult(ldap->hLdap, tx->hResponse, tx->hLastAttribute, tx->hLastValues);
    tx->eState = RvLdapRequestStateAllocated;
    tx->hRequest = 0;
    tx->hResponse = 0;
    tx->hLastAttribute = 0;
    tx->hLastValues = 0;
end:
    RvEmaUnlock(etx);
    return s;
}
#endif


RVAPI RvStatus RVCALLCONV RvLdapSearchNext(HLDAPRequest hRequest) {
    ldapTransaction_T *tx = (ldapTransaction_T *)hRequest;
    ldapElement_T *ldap = tx->ldap;
    RvStatus s = RV_OK;
    RvInt hasMoreResults;

    RvEmaLock(tx);
    
    hasMoreResults = RvLdapTxHasMoreResults(hRequest);
    if(hasMoreResults <= 0) {
        s = RV_LDAP_ERROR_NOMORERESULTS;
        goto finish;
    }

    ldap->callbacks.RvLdapFreeResult(ldap->hLdap, tx->hResponse, tx->hLastAttribute, tx->hLastValues);
    tx->eState = RvLdapRequestStateAllocated;
    tx->hRequest = 0;
    tx->hResponse = 0;
    tx->hLastAttribute = 0;
    tx->hLastValues = 0;

    s = ldap->callbacks.RvLdapStartSearch(ldap->hLdap, tx->searchBase, tx->eScope,
        tx->searchFilter, tx->attributesToReturn, 
        tx->pagedSearchCtx.pageSize,
        tx->pagedSearchCtx.cookie,
        &tx->hRequest);

    if(s != RV_OK) {
        goto finish;
    }

    StartTxTimer(tx);

finish:
    
    RvEmaUnlock(tx);

    return s;
}


static RvStatus ContinueTx(ldapTransaction_T *tx, HLDAPRequestCb tid) {
    EMAElement etx = (EMAElement)tx;
    RvStatus s = RV_OK;
    ldapElement_T *ldap = tx->ldap;

    RvEmaLock(etx);

    if(tx->eState != RvLdapRequestStateAnswered) {
        s = RV_ERROR_ILLEGAL_ACTION;
        goto end;
    }

    RvTimerCancel(&tx->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
    ldap->callbacks.RvLdapFreeResult(ldap->hLdap, tx->hResponse, tx->hLastAttribute, tx->hLastValues);
    tx->eState = RvLdapRequestStateAllocated;
    tx->hRequest = tid;
    tx->hResponse = 0;
    tx->hLastAttribute = 0;
    tx->hLastValues = 0;
    s = StartTxTimer(tx);
end:
    RvEmaUnlock(etx);
    return s;
}


/******************************************************************************
 * RvLdapNotifyResults
 * ----------------------------------------------------------------------------
 * General: Used to notify the application of transaction results. The
 *          transaction must be locked when this function is called.
 *
 * Return Value: None.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : tx                - pointer to the ldap transaction
 *         errorCode         - the error code received
 *         resultCount       - number of results received
 * Output: None.
 *****************************************************************************/
void RvLdapNotifyResults(
    IN ldapTransaction_T *tx,
    IN RvLdapErrorType errorCode,
    IN RvInt32 resultCount)
{
    HLDAPAppRequest hAppRequest = RvEmaGetApplicationHandle((EMAElement)tx);
    HLDAPMessage arrHMesssage[MAX_LDAP_RESULT_ENTRIES];
    HLDAPEntryCb pHEntryCb = NULL;
    int i, timesLocked;
    RvLdapRequestType requestType = tx->requestType;
    RvBool bSearchRequest = requestType == RvLdapRequestTypeSearch || 
                            requestType == RvLdapRequestTypeUrlSearch;


    RvTimerCancel(&tx->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
    RvTimerDestruct(&tx->timer);

    if(errorCode == 0) {
        tx->eState = RvLdapRequestStateAnswered;
        tx->resultCount = RvMin(resultCount, MAX_LDAP_RESULT_ENTRIES);

        if(bSearchRequest) {
            RvLdapPagedSearchCtx *pagedSearchCtx = &tx->pagedSearchCtx;

            /* It was a paged search, see if there are more results */
            if(pagedSearchCtx->pageSize != 0) {
                pagedSearchCtx->bAnswered = RV_TRUE;
                tx->ldap->callbacks.RvLdapGetPagedSearchInfo(
                    tx->ldap->hLdap, 
                    (HLDAPMessageCb)tx->hResponse,
                    &pagedSearchCtx->cookie);
            }
        }

        /* get all the entries */
        for (i=0; i<tx->resultCount; i++)
        {
            tx->ldap->callbacks.RvLdapGetNextResult(tx->ldap->hLdap, (HLDAPMessageCb)tx->hResponse, &pHEntryCb);
            arrHMesssage[i] = (HLDAPMessage)pHEntryCb;
        }

        RvLogInfo(tx->ldap->pLdapLog, (tx->ldap->pLdapLog,
            "RvLdapNotifyResults: calling RvLdapTxAnsweredEv(tx=%p, txApp=%p, resultCount=%d)",
            tx, hAppRequest, resultCount));
    }
    else
    {
        tx->eState = RvLdapRequestStateError;
        tx->resultCount = 0;
        arrHMesssage[0] = NULL;

        RvLogInfo(tx->ldap->pLdapLog, (tx->ldap->pLdapLog,
            "RvLdapNotifyResults: calling RvLdapTxAnsweredEv(tx=%p, txApp=%p, error=%d (%s))",
            tx, hAppRequest, errorCode, tx->ldap->callbacks.RvLdapGetErrorString(tx->errorCode)));

    }

    timesLocked = RvEmaPrepareForCallback((EMAElement)tx);
    tx->events.RvLdapTxAnsweredEv(hAppRequest, (HLDAPRequest)tx, arrHMesssage,
        tx->resultCount, errorCode);
    RvEmaReturnFromCallback((EMAElement)tx, timesLocked);
}


/******************************************************************************
 * RvLdapFdEvent
 * ----------------------------------------------------------------------------
 * General: Callback that is executed when an event occurs on the LDAP file
 *          descriptor
 *
 * Return Value: None.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : selectEngine      - Events engine of this fd
 *         fd                - File descriptor that this event occured on
 *         selectEvent       - Event that happened
 *         error             - RV_TRUE if an error occured
 * Output: None.
 *****************************************************************************/
void RvLdapFdEvent(
    IN RvSelectEngine*  selectEngine,
    IN RvSelectFd*      fd,
    IN RvSelectEvents   selectEvent,
    IN RvBool           error)
{
    ldapElement_T * ldap = RV_GET_STRUCT(ldapElement_T, fd, fd);
    RvBool bPoll = RV_TRUE, bStop = RV_FALSE;
    HLDAPRequestCb hRequest = NULL;
    RvInt32 resultCount = 0;
    RvLdapErrorType errorCode = RvLdapErrorTypeSuccess; /* optimistic, aren't we? */
    HLDAPMessageCb hResult = NULL;
    ldapTransaction_T *tx;

    RV_UNUSED_ARG(selectEngine);

    RvLogInfo(ldap->pLdapLog, (ldap->pLdapLog,
        "RvLdapFdEvent: event=%d, error=%d", selectEvent, error));

    /* notify the LDAP module as to the event */
    ldap->callbacks.RvLdapReceivedEvent(ldap->hLdap, &bPoll);

    if (!bPoll)
        return;

    if (error != RV_FALSE || selectEvent & RV_SELECT_CLOSE)
    {
        ldap->eState = RvLdapStateDisconnected;
        
        /* we expect the application to call RvLdapEnd().
         * but just in case let's remove the socket from select
         */
        RvSelectRemove(selectEngine, fd);
        
        if (ldap->events.RvLdapStateChangedEv != NULL)
            ldap->events.RvLdapStateChangedEv(ldap->hLdapApp, (HLDAP)ldap, RvLdapStateDisconnected);
        return;
    }

    /* we are supposed to poll ourselves. */
    if ((ldap->callbacks.RvLdapGetAllResults(ldap->hLdap, &hRequest, &errorCode, &resultCount, &hResult) != RV_OK) ||
        (hRequest == NULL) || (resultCount == 0))
    {
        /* no results */
        RvLdapErrorType errorType = ldap->callbacks.RvLdapGetErrorNumber(ldap->hLdap);
        RvLogInfo(ldap->pLdapLog, (ldap->pLdapLog,
            "RvLdapFdEvent: RvLdapGetAllResults() returned with error; RvLdapGetErrorNumber = %d",
            errorType));
        return;
    }

    /* got results. we are going to go over the active EMA elements to find the relevant one */
    /* TODO: add a hash table to make this more efficient */
    tx = (ldapTransaction_T *)RvEmaGetNext(ldap->txEma, (EMAElement)NULL);

    while (!bStop && tx != NULL)
    {
        if (RvEmaLock((EMAElement)tx) == RV_OK)
        {
            ldapTransaction_T *txOrig = tx;
            
            if ((tx->hRequest == hRequest) && (tx->hResponse == NULL))
            {
                tx->hResponse = hResult;
                tx->errorCode = errorCode;

                RvLdapNotifyResults(tx, errorCode, resultCount);
                bStop = RV_TRUE;
            }
            else
            {
                tx = (ldapTransaction_T *)RvEmaGetNext(ldap->txEma, (EMAElement)tx);
            }

            RvEmaUnlock((EMAElement)txOrig);
        }
        else
        {
            tx = (ldapTransaction_T *)RvEmaGetNext(ldap->txEma, (EMAElement)NULL);
        }
    }
}


/******************************************************************************
 * RvLdapTimerEvent
 * ----------------------------------------------------------------------------
 * General: Callback that is executed when an timeout occurs for a transaction.
 *
 * Return Value: RV_FALSE
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : data              - Points to the transaction object
 * Output: None.
 *****************************************************************************/
RvBool RvLdapTimerEvent(IN void*  userData)
{
    ldapTransaction_T * tx = (ldapTransaction_T *)userData;

    if (RvEmaLock((EMAElement)tx) == RV_OK)
    {
        HLDAPRequestCb hRequest = tx->hRequest;
        RvInt32 resultCount = 0;
        RvLdapErrorType errorCode = RvLdapErrorTypeSuccess; /* optimistic, aren't we? */

        /* the transaction timed out, but we'll give it just one more chance */
        if ((tx->ldap->callbacks.RvLdapGetAllResults(tx->ldap->hLdap, &hRequest, &errorCode, &resultCount, &tx->hResponse) == RV_OK) &&
            (tx->hResponse != NULL) && (resultCount > 0))
        {
            /* we got results! yay! */
            RvLdapNotifyResults(tx, errorCode, resultCount);
        }
        else
        {
            /* no results received. raise the abandoned event */
            HLDAPAppRequest hAppRequest = RvEmaGetApplicationHandle((EMAElement)tx);
            int timesLocked;

            tx->eState = RvLdapRequestStateTimeout;

            RvLogInfo(tx->ldap->pLdapLog, (tx->ldap->pLdapLog,
                "RvLdapTimerEvent: calling RvLdapTxAbandonedEv(tx=%p, txApp=%p)",
                tx, hAppRequest));

            timesLocked = RvEmaPrepareForCallback((EMAElement)tx);
            tx->events.RvLdapTxAbandonedEv(hAppRequest, (HLDAPRequest)tx);
            RvEmaReturnFromCallback((EMAElement)tx, timesLocked);
        }
        RvEmaUnlock((EMAElement)tx);
    }
    return RV_FALSE;
}

RvStatus RvLdapSaslBindAnsweredEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest,
    IN HLDAPMessage * arrHMesssage,
    IN RvInt32 numResults,
    IN RvLdapErrorType errorCode) {

    ldapTransaction_T *bindTx = (ldapTransaction_T *)hRequest;
    RvStatus s = RV_OK;
    RvLdapSaslConnectCtx *pSaslCtx = bindTx->pSaslConnectCtx;
    ldapElement_T *ldap = bindTx->ldap;

    RV_UNUSED_ARG(numResults);
    RV_UNUSED_ARG(hAppRequest);

    if(errorCode == RvLdapErrorTypeSuccess) {
        pSaslCtx->resultMsg = (void *)*arrHMesssage;
        s = ldap->callbacks.RvLdapSaslBind(ldap->hLdap, pSaslCtx);
        if(s == RV_LDAP_ERROR_SASL_BIND_IN_PROGRESS) {
            ContinueTx(bindTx, (HLDAPRequestCb)pSaslCtx->msgId);
            /* Start new one */
            return RV_OK;
        } 
    }

    RvEmaLock((EMAElement)bindTx);

    if(errorCode != RvLdapErrorTypeSuccess || s != RV_OK) {
        ldap->eState = RvLdapStateError;
    } else {
        ldap->eState = RvLdapStateConnected;
    }

    RvLogInfo(ldap->pLdapLog, (bindTx->ldap->pLdapLog,
        "RvLdapSaslBindAnsweredEv: RvLdapStateChangedEv(ldap=%p, state=%d)", ldap, ldap->eState));

    if (ldap->events.RvLdapStateChangedEv != NULL)
    {
        int timesLocked = RvEmaPrepareForCallback((EMAElement)bindTx);
        ldap->events.RvLdapStateChangedEv(ldap->hLdapApp, (HLDAP)ldap, ldap->eState);
        RvEmaReturnFromCallback((EMAElement)bindTx, timesLocked);
    }

    //DeleteSaslConnectCtx(pSaslCtx);
    RvLdapEndTx((HLDAPRequest)bindTx);
    RvEmaUnlock((EMAElement)bindTx);
    return RV_OK;
}

 

/******************************************************************************
 * RvLdapBindAnsweredEv
 * ----------------------------------------------------------------------------
 * General: This is the event reporting an answer on the LDAP bind request.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hAppRequest       - The application handle to the request
 *         hRequest          - The LDAP request handle
 *         arrHMesssage      - The array of messages received
 *         numResults        - The number of results received
 *         bError            - RV_TRUE if an error occured
 * Output: None.
 *****************************************************************************/
RvStatus RvLdapBindAnsweredEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest,
    IN HLDAPMessage * arrHMesssage,
    IN RvInt32 numResults,
    IN RvLdapErrorType errorCode)
{
    ldapTransaction_T * tx = (ldapTransaction_T *)hRequest;

    RV_UNUSED_ARG(hAppRequest);
    RV_UNUSED_ARG(numResults);
    RV_UNUSED_ARG(arrHMesssage);

    if (RvEmaLock((EMAElement)tx) == RV_OK)
    {
        if (errorCode != RvLdapErrorTypeSuccess)
        {
            /* handle transaction error */
            tx->ldap->eState = RvLdapStateError;

            RvLogInfo(tx->ldap->pLdapLog, (tx->ldap->pLdapLog,
                "RvLdapBindAnsweredEv: RvLdapStateChangedEv(ldap=%p, state=%d)", tx->ldap, RvLdapStateError));

            if (tx->ldap->events.RvLdapStateChangedEv != NULL)
            {
                int timesLocked = RvEmaPrepareForCallback((EMAElement)tx);
                tx->ldap->events.RvLdapStateChangedEv(tx->ldap->hLdapApp, (HLDAP)tx->ldap, RvLdapStateError);
                RvEmaReturnFromCallback((EMAElement)tx, timesLocked);
            }
        }
        else
        {
            tx->ldap->eState = RvLdapStateConnected;

            RvLogInfo(tx->ldap->pLdapLog, (tx->ldap->pLdapLog,
                "RvLdapBindAnsweredEv: RvLdapStateChangedEv(ldap=%p, state=%d)", tx->ldap, RvLdapStateConnected));

            if (tx->ldap->events.RvLdapStateChangedEv != NULL)
            {
                int timesLocked = RvEmaPrepareForCallback((EMAElement)tx);
                tx->ldap->events.RvLdapStateChangedEv(tx->ldap->hLdapApp, (HLDAP)tx->ldap, RvLdapStateConnected);
                RvEmaReturnFromCallback((EMAElement)tx, timesLocked);
            }
        }
        RvLdapEndTx((HLDAPRequest)tx);
        RvEmaUnlock((EMAElement)tx);
    }
    return RV_OK;
}

RvStatus RvLdapSaslBindAbandonedEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest) {
    
    ldapTransaction_T *bindTx = (ldapTransaction_T *)hRequest;

    RV_UNUSED_ARG(hAppRequest);
    RvEmaLock((EMAElement)bindTx);
    /* handle transaction error */
    bindTx->ldap->eState = RvLdapStateError;
    if (bindTx->ldap->events.RvLdapStateChangedEv != NULL)
    {
        int timesLocked = RvEmaPrepareForCallback((EMAElement)bindTx);
        bindTx->ldap->events.RvLdapStateChangedEv(bindTx->ldap->hLdapApp, (HLDAP)bindTx->ldap, RvLdapStateError);
        RvEmaReturnFromCallback((EMAElement)bindTx, timesLocked);
    }

    RvLdapEndTx((HLDAPRequest)bindTx);
    RvEmaUnlock((EMAElement)bindTx);
    return RV_OK;
}


/******************************************************************************
 * RvLdapBindAbandonedEv
 * ----------------------------------------------------------------------------
 * General: This is the event reporting timer expiry on the bind transaction.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hAppRequest       - The application handle to the request
 *         hRequest          - The LDAP request handle
 * Output: None.
 *****************************************************************************/
RvStatus RvLdapBindAbandonedEv(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest)
{
    ldapTransaction_T * tx = (ldapTransaction_T *)hRequest;

    RV_UNUSED_ARG(hAppRequest);

    if (RvEmaLock((EMAElement)tx) == RV_OK)
    {
        /* handle transaction error */
        tx->ldap->eState = RvLdapStateError;
        if (tx->ldap->events.RvLdapStateChangedEv != NULL)
        {
            int timesLocked = RvEmaPrepareForCallback((EMAElement)tx);
            tx->ldap->events.RvLdapStateChangedEv(tx->ldap->hLdapApp, (HLDAP)tx->ldap, RvLdapStateError);
            RvEmaReturnFromCallback((EMAElement)tx, timesLocked);
        }

        RvLdapEndTx((HLDAPRequest)tx);
        RvEmaUnlock((EMAElement)tx);
    }
    return RV_OK;
}


#if defined(__cplusplus)
}
#endif

#else
int prevent_warning_of_ranlib_has_no_symbols_rvldap=0;
#endif /* RV_LDAP_TYPE != RV_LDAP_NONE */

