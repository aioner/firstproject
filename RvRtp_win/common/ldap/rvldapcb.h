/* rvldapcb.h - LDAP Callback header file */
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
#ifndef RV_LDAPCB_H
#define RV_LDAPCB_H

#if RV_LDAP_TYPE != RV_LDAP_NONE
#include <ldap.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include "rverror.h"

/* These are the callbacks relating to the application's LDAP Interface, and they
 should be implemented at the application level. The callbacks presented here
 were written with the Open-LDAP package in mind. In order to use another package,
 some re-parsing may be in order. */

/****************/
/* The Handles: */
/****************/

RV_DECLARE_HANDLE(HLDAP);     /* Handle to the LDAP module */

RV_DECLARE_HANDLE(HLDAPCb);     /* Handle to the implementation's LDAP object */
RV_DECLARE_HANDLE(HLDAPRequestCb); /* Handle to the implementation's request object */
RV_DECLARE_HANDLE(HLDAPMessageCb); /* Handle to the implementation's message object */
RV_DECLARE_HANDLE(HLDAPEntryCb); /* Handle to the implementation's entry object */
RV_DECLARE_HANDLE(HLDAPAttributeCb); /* Handle to the implementation's result attribute */
RV_DECLARE_HANDLE(HLDAPValuesCb); /* Handle to the implementation's attribute value */

#define RV_LDAP_ERROR_SASL_BIND_IN_PROGRESS RvLdapError(LDAP_SASL_BIND_IN_PROGRESS)
#define RV_LDAP_ERROR_NOMORERESULTS RvLdapError(-1)


/*********************/
/* The Enumerations: */
/*********************/

typedef enum {
    ldapConnectionMethodSimple,
    ldapConnectionMethodKRBV41,
    ldapConnectionMethodKRBV42,
    ldapConnectionMethodSasl,
} RvLdapConnectionMethod;

typedef enum {
    ldapOptApiInfo,
    ldapOptProtocolVersion
} RvLdapOption;

typedef enum {
    ldapScope_MIN,      /* range check */
    ldapScopeBase,      /* read operation */
    ldapScopeOneLevel,  /* list operation */
    ldapScopeSubTree,   /* sub-tree */
    ldapScope_MAX       /* range check */
} RvLdapScope;

typedef enum {
    RvLdapErrorTypeSuccess = 0,
    RvLdapErrorTypeOperationsError = RvLdapError(LDAP_OPERATIONS_ERROR),
    RvLdapErrorTypeProtocolError = RvLdapError(LDAP_PROTOCOL_ERROR),
    RvLdapErrorTypeTimeLimitExceeded = RvLdapError(LDAP_TIMELIMIT_EXCEEDED),
    RvLdapErrorTypeSizeLimitExceeded = RvLdapError(LDAP_SIZELIMIT_EXCEEDED),
    RvLdapErrorTypeCompareFalse = RvLdapError(LDAP_COMPARE_FALSE),
    RvLdapErrorTypeCompareTrue = RvLdapError(LDAP_COMPARE_TRUE),
    RvLdapErrorTypeAuthMethodNotSupported = RvLdapError(LDAP_AUTH_METHOD_NOT_SUPPORTED),
    RvLdapErrorTypeStrongAuthRequired = RvLdapError(LDAP_STRONG_AUTH_REQUIRED),
    RvLdapErrorTypePartialResults = RvLdapError(LDAP_PARTIAL_RESULTS),
    RvLdapErrorTypeNoSuchAttribute = RvLdapError(LDAP_NO_SUCH_ATTRIBUTE),
    RvLdapErrorTypeUndefinedType = RvLdapError(LDAP_UNDEFINED_TYPE),
    RvLdapErrorTypeInappropriateMatching = RvLdapError(LDAP_INAPPROPRIATE_MATCHING),
    RvLdapErrorTypeConstraintViolation = RvLdapError(LDAP_CONSTRAINT_VIOLATION),
    RvLdapErrorTypeTypeOrValueExists = RvLdapError(LDAP_TYPE_OR_VALUE_EXISTS),
    RvLdapErrorTypeInvalidSyntax = RvLdapError(LDAP_INVALID_SYNTAX),
    RvLdapErrorTypeNoSuchObject = RvLdapError(LDAP_NO_SUCH_OBJECT),
    RvLdapErrorTypeAliasProblem = RvLdapError(LDAP_ALIAS_PROBLEM),
    RvLdapErrorTypeInvalidDnSyntax = RvLdapError(LDAP_INVALID_DN_SYNTAX),
    RvLdapErrorTypeIsLeaf = RvLdapError(LDAP_IS_LEAF),
    RvLdapErrorTypeAliasDerefProblem = RvLdapError(LDAP_ALIAS_DEREF_PROBLEM),
    RvLdapErrorTypeInappropriateAuth = RvLdapError(LDAP_INAPPROPRIATE_AUTH),
    RvLdapErrorTypeInappropriate_auth = RvLdapError(LDAP_INAPPROPRIATE_AUTH),
    RvLdapErrorTypeInvalidCredentials = RvLdapError(LDAP_INVALID_CREDENTIALS),
    RvLdapErrorTypeInsufficientAccess = RvLdapError(LDAP_INSUFFICIENT_ACCESS),
    RvLdapErrorTypeBusy = RvLdapError(LDAP_BUSY),
    RvLdapErrorTypeUnavailable = RvLdapError(LDAP_UNAVAILABLE),
    RvLdapErrorTypeUnwillingToPerform = RvLdapError(LDAP_UNWILLING_TO_PERFORM),
    RvLdapErrorTypeLoopDetect = RvLdapError(LDAP_LOOP_DETECT),
    RvLdapErrorTypeNamingViolation = RvLdapError(LDAP_NAMING_VIOLATION),
    RvLdapErrorTypeObjectClassViolation = RvLdapError(LDAP_OBJECT_CLASS_VIOLATION),
    RvLdapErrorTypeNotAllowedOnNonleaf = RvLdapError(LDAP_NOT_ALLOWED_ON_NONLEAF),
    RvLdapErrorTypeNotAllowedOnRdn = RvLdapError(LDAP_NOT_ALLOWED_ON_RDN),
    RvLdapErrorTypeAlreadyExists = RvLdapError(LDAP_ALREADY_EXISTS),
    RvLdapErrorTypeNoObjectClassMods = RvLdapError(LDAP_NO_OBJECT_CLASS_MODS),
    RvLdapErrorTypeOther = RvLdapError(LDAP_OTHER),
    RvLdapErrorTypeServerDown = RvLdapError(LDAP_SERVER_DOWN),
    RvLdapErrorTypeLocalError = RvLdapError(LDAP_LOCAL_ERROR),
    RvLdapErrorTypeEncodingError = RvLdapError(LDAP_ENCODING_ERROR),
    RvLdapErrorTypeDecodingError = RvLdapError(LDAP_DECODING_ERROR),
    RvLdapErrorTypeTimeout = RvLdapError(LDAP_TIMEOUT),
    RvLdapErrorTypeAuthUnknown = RvLdapError(LDAP_AUTH_UNKNOWN),
    RvLdapErrorTypeFilterError = RvLdapError(LDAP_FILTER_ERROR),
    RvLdapErrorTypeParamError = RvLdapError(LDAP_PARAM_ERROR),
    RvLdapErrorTypeNoMemory = RvLdapError(LDAP_NO_MEMORY),
    RvLdapErrorTypeSaslBindInProgress = RvLdapError(LDAP_SASL_BIND_IN_PROGRESS)
} RvLdapErrorType;

#if 0

typedef enum {
    RvLdapErrorTypeSuccess,
    RvLdapErrorTypeOperationsError,
    RvLdapErrorTypeProtocolError,
    RvLdapErrorTypeTimeLimitExceeded,
    RvLdapErrorTypeSizeLimitExceeded,
    RvLdapErrorTypeCompareFalse,
    RvLdapErrorTypeCompareTrue,
    RvLdapErrorTypeAuthMethodNotSupported,
    RvLdapErrorTypeStrongAuthRequired,
    RvLdapErrorTypePartialResults,
    RvLdapErrorTypeNoSuchAttribute,
    RvLdapErrorTypeUndefinedType,
    RvLdapErrorTypeInappropriateMatching,
    RvLdapErrorTypeConstraintViolation,
    RvLdapErrorTypeTypeOrValueExists,
    RvLdapErrorTypeInvalidSyntax,
    RvLdapErrorTypeNoSuchObject,
    RvLdapErrorTypeAliasProblem,
    RvLdapErrorTypeInvalidDnSyntax,
    RvLdapErrorTypeIsLeaf,
    RvLdapErrorTypeAliasDerefProblem,
    RvLdapErrorTypeInappropriate_auth,
    RvLdapErrorTypeInvalidCredentials,
    RvLdapErrorTypeInsufficientAccess,
    RvLdapErrorTypeBusy,
    RvLdapErrorTypeUnavailable,
    RvLdapErrorTypeUnwillingToPerform,
    RvLdapErrorTypeLoopDetect,
    RvLdapErrorTypeNamingViolation,
    RvLdapErrorTypeObjectClassViolation,
    RvLdapErrorTypeNotAllowedOnNonleaf,
    RvLdapErrorTypeNotAllowedOnRdn,
    RvLdapErrorTypeAlreadyExists,
    RvLdapErrorTypeNoObjectClassMods,
    RvLdapErrorTypeOther,
    RvLdapErrorTypeServerDown,
    RvLdapErrorTypeLocalError,
    RvLdapErrorTypeEncodingError,
    RvLdapErrorTypeDecodingError,
    RvLdapErrorTypeTimeout,
    RvLdapErrorTypeAuthUnknown,
    RvLdapErrorTypeFilterError,
    RvLdapErrorTypeParamError,
    RvLdapErrorTypeNoMemory,
    RvLdapErrorSaslBindInProgress
} RvLdapErrorType;

#endif

typedef enum {
    RvLdapAttrString,
    RvLdapAttrInt32
} RvLdapAttrType;


/*******************/
/* The Structures: */
/*******************/

typedef struct {
    RvChar*         attrName;
    RvLdapAttrType  attrType;
    void*           attrValues;
} LDAPAttrList;

typedef struct {
    const RvChar *mech;
    const RvChar *realm;
    const RvChar *authcid;
    const RvChar *passwd;
    const RvChar *authzid;
} RvLdapConnectParams;


typedef struct {
    /* Input parameters */
    RvLdapConnectParams *pSaslConnectParams;
    RvSize_t nSaslConnectParams;
    RvChar *mechs;
    /* Output parameters */
    RvChar *choosenMech;
    void   *resultMsg;
    RvInt   msgId;
} RvLdapSaslConnectCtx;

typedef struct {
    void *cookie;
    RvUlong pageSize;
    RvBool bAnswered;
} RvLdapPagedSearchCtx;


/******************/
/* The Callbacks: */
/******************/

/******************************************************************************
 * RvLdapInitializeCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement LDAP memory allocation and
 *          initialization.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : strServerAddress  - The LDAP server host-name
 * Output: pHLdapCb          - Handle to the LDAP instance
 *****************************************************************************/
typedef RvStatus (* RvLdapInitializeCb_T)(
    IN RvChar * strServerAddress,
    OUT HLDAPCb * pHLdapCb);


/******************************************************************************
 * RvLdapSetOptionCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement option setting for LDAP session
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : strServerAddress  - The LDAP server host-name
 * Output: pHLdapCb          - Handle to the LDAP instance
 *****************************************************************************/
typedef RvStatus (* RvLdapSetOptionCb_T)(
    IN HLDAPCb hLdapCb,
    IN RvLdapOption ldapOption,
    IN void* ldapData);


/******************************************************************************
 * RvLdapBindCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement the connection to the previously
 *          given server with the user name and password given, either by a
 *          secure connection or not, as directed.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         eConnectionType   - Type of connection, simple or secure
 *         strName           - Distinguished Name to give the server
 *         strPassword       - Password to give the server
 * Output: pHRequestCb       - Handle to the LDAP bind request
 *****************************************************************************/
typedef RvStatus (* RvLdapBindCb_T)(
    IN HLDAPCb hLdapCb,
    IN RvLdapConnectionMethod eConnectionType,
    IN const RvChar * strName,
    IN const RvChar * strPassword,
    OUT HLDAPRequestCb * pHRequestCb);


typedef RvStatus (*RvLdapSaslBindCb_T)(
    IN HLDAPCb hLdapCb,
    IN RvLdapSaslConnectCtx *pSaslConnectCtx);


/******************************************************************************
 * RvLdapUnbindCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement disconnection from the server and
 *          destruction of the LDAP element.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 * Output: none
 *****************************************************************************/
typedef RvStatus (* RvLdapUnbindCb_T)(
    IN HLDAPCb hLdapCb);


/******************************************************************************
 * RvLdapGetNextResultCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should get a result for a previous request. If
 *          pHResult points to NULL, the function should get the first result.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hResultCb         - Handle to a request result
 *         pHEntryCb         - Previous entry handle received
 * Output: pHEntryCb         - Handle to the next result entry
 *****************************************************************************/
typedef RvStatus (* RvLdapGetNextResultCb_T)(
    IN HLDAPCb hLdapCb,
    IN HLDAPMessageCb hResultCb,
    INOUT HLDAPEntryCb * pHEntryCb);


/******************************************************************************
 * RvLdapGetAllResultsCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should get all the results of a request, and the
 *          number of results received. If the request handle is NULL, it should
 *          also find a request for which results were received, or leave it as
 *          NULL if no full results were received yet.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hRequestCb        - Pointer to the handle of the request to check, or NULL
 * Output: hRequestCb        - Handle to the request for which results were received
 *         errorCode         - Error code, zero if ok
 *         resultCount       - Number of result messages
 *         pHResultCb        - Handle to the results
 *****************************************************************************/
typedef RvStatus (* RvLdapGetAllResultsCb_T)(
    IN HLDAPCb hLdapCb,
    INOUT HLDAPRequestCb * hRequestCb,
    OUT RvLdapErrorType * errorCode,
    OUT RvInt32 * resultCount,
    OUT HLDAPMessageCb * pHResultCb);


/******************************************************************************
 * RvLdapGetEntryDnCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should get Distinguished Name of a result entry.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hEntryCb          - Handle of the result entry
 *         length            - Length of the buffer given
 * Output: length            - Length of the string returned or the buffer needed
 *         strValue          - The DN string
 *****************************************************************************/
typedef RvStatus (* RvLdapGetEntryDnCb_T)(
    IN HLDAPCb hLdapCb,
    IN HLDAPEntryCb hEntryCb,
    INOUT RvInt32 * length,
    OUT RvChar * strValue);


/* Given a result handle, the function will get an attribute and its value(s). If pHAtribute points
 to NULL, it will get the first attribute. Each call to this function with non-NULL pointer
 pHAtribute expects to get the previously provided pHValues in order to free it. This function is
 very open-LDAP centric. */
/******************************************************************************
 * RvLdapResultGetNextAtributeCb_T
 * ----------------------------------------------------------------------------
 * General: Given a result handle, the function should get an attribute and
 *          its value(s). If pHAtribute points to NULL, it will get the first
 *          attribute. Each call to this function with non-NULL pointer
 *          pHAtribute may expects to get the previously provided pHValues in
 *          order to free it.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hEntryCb          - Handle of the result entry
 *         pHAtributeCb      - Pointer to the last attribute handle given
 *         pHValuesCb        - Pointer to the last values handle given
 *         lenAttribute      - Length of the attribute buffer given
 * Output: pHAtributeCb      - Handle to the next attribute
 *         pHValuesCb        - Handle to the values of the next attribute
 *         strAttribute      - The attribute string
 *         lenAttribute      - Length of the attribute or buffer needed
 *         numValues         - Ammount of values for the current attribute
 *****************************************************************************/
typedef RvStatus (* RvLdapResultGetNextAtributeCb_T)(
    IN HLDAPCb hLdapCb,
    IN HLDAPEntryCb hEntryCb,
    INOUT HLDAPAttributeCb * pHAtributeCb,
    INOUT HLDAPValuesCb * pHValuesCb,
    OUT RvChar * strAttribute,
    INOUT RvInt32 * lenAttribute,
    OUT RvInt32 * numValues);


/******************************************************************************
 * RvLdapGetAtributeValueCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should copy the indexed value, if it exists, to the
 *          given buffer.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hAtributeCb       - Handle of the attribute
 *         hValuesCb         - Handle of the attribute values
 *         index             - The requested value index
 *         length            - Length of the buffer given
 * Output: length            - Used length of the buffer or the buffer needed
 *         strValue          - The value
 *****************************************************************************/
typedef RvStatus (* RvLdapGetAtributeValueCb_T)(
    IN HLDAPCb hLdapCb,
    IN HLDAPAttributeCb hAtributeCb,
    IN HLDAPValuesCb hValuesCb,
    IN RvInt32 index,
    INOUT RvInt32 * length,
    OUT RvChar ** strValue);


/******************************************************************************
 * RvLdapFreeResultCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should free the parameters given.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hAtributeCb       - Handle of the results, may be NULL
 *         hAtributeCb       - Handle of the attribute, may be NULL
 *         hValuesCb         - Handle of the attribute values, may be NULL
 * Output: none
 *****************************************************************************/
typedef RvStatus (* RvLdapFreeResultCb_T)(
    IN HLDAPCb hLdapCb,
    IN HLDAPMessageCb hResultCb,
    IN HLDAPAttributeCb hAtributeCb,
    IN HLDAPValuesCb hValuesCb);


/******************************************************************************
 * RvLdapEndRequestCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement abandonment of a request.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         hRequestCb        - Handle of the request
 * Output: none
 *****************************************************************************/
typedef RvStatus (* RvLdapEndRequestCb_T)(
    IN HLDAPCb hLdapCb,
    IN HLDAPRequestCb hRequestCb);


/******************************************************************************
 * RvLdapStartSearchCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement sending of a search request. The
 *          strings are in Open-LDAP format.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         strBase           - String containing the search base
 *         eScope            - Scope of the search, read or list
 *         strFilter         - String containing the search filter
 *         strAttributesToReturn - Array of strings containing the attributes
 *                             to return
 * Output: pHRequestCb       - The handle to the request
 *****************************************************************************/
typedef RvStatus (* RvLdapStartSearchCb_T)(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase, 
    IN RvLdapScope eScope, 
    IN const RvChar * strFilter, 
    IN RvChar ** strAttributesToReturn,
    IN RvUlong pageSize,
    IN void *vcookie,
    OUT HLDAPRequestCb * pHRequestCb);

typedef RvStatus (*RvLdapGetPagedSearchInfoCb_T)(
    IN HLDAPCb hLdapCb, 
    IN HLDAPMessageCb hMessage, 
    INOUT void **pCookie);


typedef RvStatus (*RvLdapStartUrlSearchCb_T)(
    IN HLDAPCb hLdapCb,
    IN const char *url,
    OUT HLDAPRequestCb * pHRequestCb);

typedef RvStatus (*RvLdapStartTlsCb_T)(IN HLDAPCb hLdapSb, IN RvBool bAsync);

/******************************************************************************
 * RvLdapDeleteCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement sending of a delete request. The
 *          strings are in Open-LDAP format.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         strBase           - String containing the distinguished name
 * Output: pHRequestCb       - The handle to the request
 *****************************************************************************/
typedef RvStatus (* RvLdapDeleteCb_T)(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase, 
    OUT HLDAPRequestCb * pHRequestCb);


/******************************************************************************
 * RvLdapAddCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement sending of an add request. The
 *          strings are in Open-LDAP format.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         strBase           - String containing the distinguished name
 * Output: pHRequestCb       - The handle to the request
 *****************************************************************************/
typedef RvStatus (* RvLdapAddCb_T)(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase,
    IN LDAPAttrList* attrList,
    IN RvInt32 nAttrList,
    OUT HLDAPRequestCb * pHRequestCb);


/******************************************************************************
 * RvLdapModifyCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should implement sending of a modify request. The
 *          strings are in Open-LDAP format.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 *         strBase           - String containing the distinguished name
 * Output: pHRequestCb       - The handle to the request
 *****************************************************************************/
typedef RvStatus (* RvLdapModifyCb_T)(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase, 
    IN LDAPAttrList* attrList,
    IN RvInt32 nAttrList,
    OUT HLDAPRequestCb * pHRequestCb);


/******************************************************************************
 * RvLdapGetFdCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should get the LDAP file descriptor or socket, which
 * will be added to the stack select loop to wait for incoming server responses.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 * Output: fileDesc          - The socket or file descriptor
 *****************************************************************************/
typedef RvStatus (* RvLdapGetFdCb_T)(
    IN HLDAPCb hLdapCb,
    OUT void * fileDesc);


/******************************************************************************
 * RvLdapReceivedEventCb_T
 * ----------------------------------------------------------------------------
 * General: This callback is used to notify the LDAP as to incoming events. The
 *          bPoll output indicates whether the LDAP module should now poll the
 *          requests, otherwise the LDAP implementation will notify it as to
 *          the requests on which events were received using
 *          RvLdapReceivedEventForRqst().
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 * Output: bPoll             - true if the caller should now poll the requests
 *****************************************************************************/
typedef RvStatus (* RvLdapReceivedEventCb_T)(
    IN HLDAPCb hLdapCb,
    OUT RvBool *bPoll);


/******************************************************************************
 * RvLdapGetErrorNumberCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should return the last error code which occured
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP instance
 * Output: none
 *****************************************************************************/
typedef RvLdapErrorType (* RvLdapGetErrorNumberCb_T)(
    IN HLDAPCb hLdapCb);


/******************************************************************************
 * RvLdapGetErrorStringCb_T
 * ----------------------------------------------------------------------------
 * General: This callback should convert an error code to a printable string
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : errorCode         - The error code
 * Output: none
 *****************************************************************************/
typedef const RvChar * (* RvLdapGetErrorStringCb_T)(
    IN RvLdapErrorType errorCode);


/******************************************************************************
 * RvLdapReceivedEventForRqst
 * ----------------------------------------------------------------------------
 * General: Function is used to notify the LDAP module that an answer was
 *          received for a request.
 *
 * Return Value: Non-negative value on success, Negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * input : hLdapCb           - Handle to the LDAP module
 *         hRequestCb        - The request handle
 * Output: none
 *****************************************************************************/
RvStatus RvLdapReceivedEventForRqst(
    IN HLDAP hLdap,
    IN HLDAPRequestCb hRequestCb);


#if defined(__cplusplus)
}
#endif

#endif
#endif /* RV_LDAPCB_H */
