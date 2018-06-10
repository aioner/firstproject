/* rvopenldap.h - Interface functions for OpenLDAP services */
/************************************************************************
        Copyright (c) 2001 RADVISION Inc. and RADVISION Ltd.
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
#ifndef RV_OPENLDAP_H
#define RV_OPENLDAP_H

#if RV_LDAP_TYPE != RV_LDAP_NONE
#if defined(__cplusplus)
extern "C" {
#endif

    
RVAPI RvStatus RvLdapInitialize(
    IN RvChar * strServerAddress,
    OUT HLDAPCb * pHLdapCb);


RVAPI RvStatus RvLdapSetOption(
    IN HLDAPCb hLdapCb,
    IN RvLdapOption ldapOption,
    IN void* ldapData);


RVAPI RvStatus RvLdapBind(
    IN HLDAPCb hLdapCb,
    IN RvLdapConnectionMethod eConnectionType,
    IN const RvChar * strName,
    IN const RvChar * strPassword,
    OUT HLDAPRequestCb * pHRequestCb);


RVAPI RvStatus RvLdapSaslBind(
    IN HLDAPCb hLdapCb,
    IN RvLdapSaslConnectCtx *pSaslConnectCtx);

RVAPI RvStatus RvLdapUnbind(
    IN HLDAPCb hLdapCb);


RVAPI RvStatus RvLdapGetAllResults(
    IN HLDAPCb hLdapCb,
    INOUT HLDAPRequestCb * pHRequestCb,
    OUT RvLdapErrorType * errorCode,
    OUT RvInt32 * resultCount,
    OUT HLDAPMessageCb * pHResultCb);


RVAPI RvStatus RvLdapGetNextResult(
    IN HLDAPCb hLdapCb,
    IN HLDAPMessageCb hResultCb,
    INOUT HLDAPEntryCb * pHEntryCb);


RVAPI RvStatus RvLdapResultGetNextAtribute(
    IN HLDAPCb hLdapCb,
    IN HLDAPEntryCb hEntryCb,
    INOUT HLDAPAttributeCb * pHAtributeCb,
    INOUT HLDAPValuesCb * pHValuesCb,
    OUT RvChar * strAttribute,
    INOUT RvInt32 * lenAttribute,
    OUT RvInt32 * numValues);


RVAPI RvStatus RvLdapGetAtributeValue(
    IN HLDAPCb hLdapCb,
    IN HLDAPAttributeCb hAtributeCb,
    IN HLDAPValuesCb hValuesCb,
    IN RvInt32 index, 
    INOUT RvInt32 * length,
    OUT RvChar ** strValue);


RVAPI RvStatus RvLdapFreeResult(
    IN HLDAPCb hLdapCb,
    IN HLDAPMessageCb hResultCb,
    IN HLDAPAttributeCb hAtributeCb,
    IN HLDAPValuesCb hValuesCb);


RVAPI RvStatus RvLdapEndRequest(
    IN HLDAPCb hLdapCb,
    IN HLDAPRequestCb hRequestCb);


RVAPI RvStatus RvLdapStartSearch(
     IN HLDAPCb hLdapCb,
     IN const RvChar * strBase, 
     IN RvLdapScope eScope, 
     IN const RvChar * strFilter, 
     IN RvChar ** strAttributesToReturn,
     IN RvUlong pageSize,
     IN void *vcookie,
     OUT HLDAPRequestCb * pHRequestCb);


RVAPI RvStatus RvLdapDelete(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase, 
    OUT HLDAPRequestCb * pHRequestCb);


RVAPI RvStatus RvLdapAdd(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase, 
    IN LDAPAttrList* attrList,
    IN RvInt32 nAttrList,
    OUT HLDAPRequestCb * pHRequestCb);


RVAPI RvStatus RvLdapModify(
    IN HLDAPCb hLdapCb,
    IN const RvChar * strBase, 
    IN LDAPAttrList* attrList,
    IN RvInt32 nAttrList,
    OUT HLDAPRequestCb * pHRequestCb);


RVAPI RvStatus RvLdapGetFd(
    IN HLDAPCb hLdapCb,
    OUT void * pFileDesc);


RVAPI RvStatus RvLdapReceivedEvent(
    IN HLDAPCb hLdapCb,
    OUT RvBool *bPoll);


RVAPI RvLdapErrorType RvLdapGetErrorNumber(
    IN HLDAPCb hLdapCb);


RVAPI const RvChar * RvLdapGetErrorString(
    IN RvLdapErrorType errorCode);

RVAPI RvStatus RvLdapStartUrlSearch(
    IN HLDAPCb hLdapCb, 
    IN const RvChar *url, 
    OUT HLDAPRequestCb *pHRequestCb);

RVAPI RvStatus RvLdapStartTls(    
    IN HLDAPCb hLdapCb, 
    IN RvBool  bAsync);

RVAPI RvStatus RvLdapGetPagedSearchInfo(
    IN HLDAPCb hLdapCb, 
    IN HLDAPMessageCb hMessage, 
    INOUT void **pCookie);


RVAPI RvStatus RvLdapGetEntryDn(
    IN HLDAPCb hLdapCb,
    IN HLDAPEntryCb hEntryCb,
    INOUT RvInt32 * length,
    OUT RvChar * strValue);


#if defined(__cplusplus)
}
#endif

#endif
#endif /* _RV_DNS_H */
