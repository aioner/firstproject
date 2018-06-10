/* rvldap.h - LDAP header file */
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
#ifndef RV_LDAP_H
#define RV_LDAP_H

#if RV_LDAP_TYPE != RV_LDAP_NONE
#include "rvldapcb.h"
#include "rvlog.h"

#if defined(__cplusplus)
extern "C" {
#endif

/******************************************************************************
 * These are the evens raised from the LDAP add-on to the application and the
 * functions which will be provided to the application by the stack. It is
 * possible to create multiple instances of the  LDAP module, each with its
 * own set of callbacks and events, if the application would like to
 * communicate with more than one server.
 *****************************************************************************/


/****************/
/* The Handles: */
/****************/

RV_DECLARE_HANDLE(HLDAPAPP);
RV_DECLARE_HANDLE(HLDAPRequest);
RV_DECLARE_HANDLE(HLDAPAppRequest);
RV_DECLARE_HANDLE(HLDAPMessage);



/*********************/
/* The Enumerations: */
/*********************/

typedef enum {
    RvLdapStateInitialized,
    RvLdapStateConnected,
    RvLdapStateDisconnected,
    RvLdapStateError
} RvLdapState;

typedef enum {
    RvLdapParmState,
    RvLdapParmNumActiveTransactions,
    RvLdapParmDefaultTimeout,
    RvLdapParmDefaultEvents
} RvLdapParm;

typedef enum {
    RvLdapRequestTypeSearch,
    RvLdapRequestTypeUrlSearch,
    RvLdapRequestTypeDelete,
    RvLdapRequestTypeAdd,
    RvLdapRequestTypeModify,
    RvLdapRequestTypeConnect,
    RvLdapResquestTypeSaslConnect,
    RvLdapRequestTypePagedSearch
} RvLdapRequestType;

typedef enum {
    RvLdapRequestStateAllocated,
    RvLdapRequestStateSent,
    RvLdapRequestStateAnswered,
    RvLdapRequestStateError,
    RvLdapRequestStateTimeout
} RvLdapRequestState;

typedef enum {
    RvLdapRequestParamState,
    RvLdapRequestParamTimeout,
    RvLdapRequestParamCallback,
    RvLdapRequestParamRequestType,
    RvLdapRequestParamScope,
    RvLdapRequestParamSearchBase,
    RvLdapRequestParamSearchFilter,
    RvLdapRequestParamSearchUrl,
    RvLdapRequestParamPageSize,
    RvLdapRequestParamAttributesToReturn,
    RvLdapRequestParamAttrList,
    RvLdapRequestParamMessageDistinguishedName,         /* get only */
    RvLdapRequestParamMessageFirstAttribute,            /* get only */
    RvLdapRequestParamMessageNextAttribute,             /* get only */
    RvLdapRequestParamMessageAttributeNumberOfValues,   /* get only */
    RvLdapRequestParamMessageAttributeValue,            /* get only */
    RvLdapRequestParamMessageErrorString,               /* get only */
    RvLdapRequestHasMoreResults                         /* get only */ 
} RvLdapRequestParam;


/***************/
/* The Events: */
/***************/

/******************************************************************************
 * RvLdapStateChangedEv_T
 * ----------------------------------------------------------------------------
 * General: This is the event type reporting a change in the LDAP state.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapApp          - Application handle to the LDAP instance
 *         hLdap             - The LDAP instance handle
 *         eState            - The new LDAP state
 * Output: None.
 *****************************************************************************/
typedef RvStatus (* RvLdapStateChangedEv_T)(
    IN HLDAPAPP hLdapApp,
    IN HLDAP hLdap,
    IN RvLdapState eState);


/******************************************************************************
 * RvLdapTxAnsweredEv
 * ----------------------------------------------------------------------------
 * General: This is the event type reporting an answer on an LDAP query. It
 * will be called after all answers arrive, and will provide the number or
 * answers received. The answers are released on exit form this callback.
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
typedef RvStatus (* RvLdapTxAnsweredEv_T)(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest,
    IN HLDAPMessage * arrHMesssage,
    IN RvInt32 numResults,
    IN RvLdapErrorType errorType);


/******************************************************************************
 * RvLdapTxAbandonedEv_T
 * ----------------------------------------------------------------------------
 * General: This is the event type reporting timer expiry. The application is
 * expected to free the request at this time at the latest.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hAppRequest       - The application handle to the request
 *         hRequest          - The LDAP request handle
 * Output: None.
 *****************************************************************************/
typedef RvStatus (* RvLdapTxAbandonedEv_T)(
    IN HLDAPAppRequest hAppRequest,
    IN HLDAPRequest hRequest);

/*******************/
/* The Structures: */
/*******************/

typedef struct {
    RvChar * strLdapSeverAddress;           /* string containing the server IP address or host name */
    RvChar * strUsername;                   /* string containing the user's distinguished name */
    RvChar * strPassword;                   /* string containing the password */
    RvBool   bUseTls;
    RvLdapConnectionMethod eConnectionType; /* the connection type, simple or secure */
    RvSize_t maxTransactions;               /* set the maximum number of simultaneous transactions */
    RvInt32 transactionTimeout;             /* the transaction timeout in milliseconds */
    RvSize_t maxStringSize;                 /* the maximum attribute string size */
    RvLdapConnectParams *pSaslConnectParams; /* array of RvSaslConnectParams structures */
    RvSize_t nSaslConnectParams;
} LDAPConfiguration;


typedef struct {
    RvLdapInitializeCb_T              RvLdapInitialize;
    RvLdapSetOptionCb_T               RvLdapSetOption;
    RvLdapBindCb_T                    RvLdapBind;
    RvLdapUnbindCb_T                  RvLdapUnbind;
    RvLdapGetNextResultCb_T           RvLdapGetNextResult;
    RvLdapGetAllResultsCb_T           RvLdapGetAllResults;
    RvLdapGetEntryDnCb_T              RvLdapGetEntryDn;
    RvLdapResultGetNextAtributeCb_T   RvLdapResultGetNextAtribute;
    RvLdapGetAtributeValueCb_T        RvLdapGetAtributeValue;
    RvLdapFreeResultCb_T              RvLdapFreeResult;
    RvLdapEndRequestCb_T              RvLdapEndRequest;
    RvLdapStartSearchCb_T             RvLdapStartSearch;
    RvLdapDeleteCb_T                  RvLdapDelete;
    RvLdapAddCb_T                     RvLdapAdd;
    RvLdapModifyCb_T                  RvLdapModify;
    RvLdapGetFdCb_T                   RvLdapGetFd;
    RvLdapReceivedEventCb_T           RvLdapReceivedEvent;
    RvLdapGetErrorNumberCb_T          RvLdapGetErrorNumber;
    RvLdapGetErrorStringCb_T          RvLdapGetErrorString;
    RvLdapStartUrlSearchCb_T          RvLdapStartUrlSearch;
    RvLdapStartTlsCb_T                RvLdapStartTls;
    RvLdapSaslBindCb_T                RvLdapSaslBind;
    RvLdapGetPagedSearchInfoCb_T      RvLdapGetPagedSearchInfo;
} LDAPCallbacks;

typedef struct {
    RvLdapStateChangedEv_T RvLdapStateChangedEv;
    RvLdapTxAnsweredEv_T RvLdapTxAnsweredEv;
    RvLdapTxAbandonedEv_T RvLdapTxAbandonedEv;
} LDAPEvents;

typedef struct {
    RvLdapTxAnsweredEv_T RvLdapTxAnsweredEv;
    RvLdapTxAbandonedEv_T RvLdapTxAbandonedEv;
} LDAPTransactionEvents;

/******************/
/* The Functions: */
/******************/

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
    OUT HLDAP * pHLdap);


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
    IN void* ldapData);


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
 *
 * Description:
 *  RvLdapConnect supports few connection methods:simple, krbv41, krbv42 and sasl (preferred)
 *  To use sasl connects eConnectionType of pLdapConfig should be set to 'ldapConnectionMethodSasl'
 *   and pSaslConnectParams should point to valid RvLdapSaslConnectParams structure
 *
 *   
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvLdapConnect(

    IN HLDAP hLdap, 
    IN LDAPConfiguration *pLdapConfig);


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
    IN HLDAP hLdap);


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
    OUT RvChar * string);


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
    IN const RvChar * string);


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
    IN HLDAP hLdap);


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
    OUT HLDAPRequest * pHRequest);


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
    OUT RvChar * string);

/* Utility functions 
 * Returns:
 * -1 - unknown
 *  0 - no more results
 *  1 - has more results
 */

RVAPI RvInt RvLdapTxHasMoreResults(IN HLDAPRequest hRequest);


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
    IN const RvChar * string);



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
    IN HLDAPRequest hRequest);


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
    IN HLDAPRequest hRequest);


RVAPI RvStatus RVCALLCONV RvLdapSearchNext(IN HLDAPRequest hRequest);

#if defined(__cplusplus)
}
#endif

#endif
#endif /* RV_LDAP_H */
