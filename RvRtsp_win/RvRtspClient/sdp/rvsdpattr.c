/******************************************************************************
Filename    :rvsdpattr.c
Description :attributes manipulation routines.

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD..
  
    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include <string.h>
#include "rvstrutils.h"
#include "rvsdpprivate.h"
#include "rvsdpprsutils.h"

#include "rvsdpapiprivate.h"


/*
 *	To allocate the memory for RvSdpAttribute object (called by the pool)
 *  Return: 
 *      valid RvSdpAttribute pointer
 *      or NULL if fails
 */
RvSdpAttribute* rvSdpAttributeCreateByPool(RvSdpMsg* msg)
{
    RvSdpAttribute *attr;
    attr = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpAttribute));
    if (!attr)
        return NULL;
    memset(attr,0,sizeof(RvSdpAttribute));
    attr->iObj = msg;
    return attr;
}

/***************************************************************************
 * rvSdpAttributeConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpAttribute object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'attr' is NULL the attribute will be allocated from
 *            the 'msg' pool of attributes. If 'msg' is not NULL the constructed
 *            attribute will be appended to 'msg' list of attributes. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      attr - a pointer to valid RvSdpAttribute object or NULL.
 *      name - the name of the attribute.
 *      value - the value of the attribute.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'attr' must point to valid & constructed
 *                      RvSdpAttribute object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpAttribute* 
rvSdpAttributeConstruct2(RvSdpMsg* msg, RvSdpAttribute* attr, const char* name, 
                         const char* value, RvAlloc* a, RvBool dontConstruct)
{    
    RvBool takenFromPool = RV_FALSE;
	if(strcmp(name,"rtpmap")==0)
	{
		printf("name=%s value =%s\n",name,value);
	}
    if (!dontConstruct || !attr)
    {
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the msg context */
        {
            if (!attr)
            {
                /* the 'attr' can't be set it has to be allocated from the msg attributes pool */
                attr = rvSdpPoolTake(&msg->iAttrsPool);
                if (!attr)
                    /* failed to allocate from the msg attributes pool */
                    return NULL;            
                takenFromPool = RV_TRUE;
            }
            attr->iObj = msg;
        }
        else 
        {
            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!attr)
                /* attr must be supplied */
                return NULL;
            memset(attr,0,sizeof(RvSdpAttribute));
            if (!a)
                /* the default allocator will be used */
                a = rvSdpGetDefaultAllocator();
            /* save the allocator used */
            attr->iObj = a;
        }
    }

    if (msg)
        rvSdpMsgPromiseBuffer(msg,((name)?(int)strlen(name):0)+((value)?(int)strlen(value):0));

    if (rvSdpSetTextField(&attr->iAttrName,attr->iObj,name) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&attr->iAttrValue,attr->iObj,value) != RV_SDPSTATUS_OK)
    {
        rvSdpUnsetTextField(&attr->iAttrName,attr->iObj);
        rvSdpUnsetTextField(&attr->iAttrValue,attr->iObj);
        if (takenFromPool && msg)
            rvSdpPoolReturn(&msg->iAttrsPool,attr);
        return NULL;
    }

    if (!dontConstruct)
    {
        if (msg)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_ATTRIBUTE,&attr->iLineObj,
                            RV_OFFSETOF(RvSdpAttribute,iLineObj));
    }

    return attr;   
}

/* Deallocates RvSdpAttribute netto memory	*/
void rvSdpAttributeDestructNetto(
                     RvSdpAttribute* attr,   /* the attribute pointer */ 
                     RvBool retToPool)       /* whether to return to attributes pool */
{
    if (!attr->iObj)
        /* cannot deallocate memory */
        return;
        
    rvSdpUnsetTextField(&attr->iAttrName,attr->iObj);
    rvSdpUnsetTextField(&attr->iAttrValue,attr->iObj);

	attr->iSpecAttrData = NULL;
    
    if (RV_SDP_OBJ_IS_MESSAGE(attr))
    {
        rvSdpLineObjsListRemove(attr->iObj,&attr->iLineObj);
        /* RvSdpMsg context was used */
        if (retToPool)
            rvSdpPoolReturn(&((RvSdpMsg*)(attr->iObj))->iAttrsPool,attr);
    }
}   

/***************************************************************************
 * rvSdpAttributeDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the media descriptor object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to the RvSdpAttribute object.
***************************************************************************/
void rvSdpAttributeDestruct(RvSdpAttribute* attr)
{
	if (attr->iSpecAttrData && attr->iSpecAttrData->iDestructFunc)
		(*attr->iSpecAttrData->iDestructFunc)(attr);
	else
		rvSdpAttributeDestructNetto(attr,RV_TRUE);
}

/*
 *	To free the memory for RvSdpAttribute object (called by the pool)
 */
void rvSdpAttributeDestroyByPool(RvSdpAttribute* p)
{    
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpAttribute),p);
}

/***************************************************************************
 * rvSdpAttributeCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpAttribute from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpAttribute object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpAttribute object or NULL. 
 *      src - a pointer to the source object
 *      dstMsg - the RvSdpMsg instance that will own the destination object
 ***************************************************************************/
RvSdpAttribute* rvSdpAttributeCopy2(RvSdpAttribute* dest, 
                                    const RvSdpAttribute* src, RvSdpMsg* dstMsg)
{    

	if (src->iSpecAttrData && src->iSpecAttrData->iCopyFunc)
		return (*src->iSpecAttrData->iCopyFunc)(dest,src,dstMsg);
	else
	{
        dest = rvSdpAttributeConstruct2(dstMsg,dest,src->iAttrName,src->iAttrValue,NULL,RV_FALSE);   
		if (dest && src->iSpecAttrData)
			/*
			 * treat the case when the special attribute does not have the copy function (conn-mode)	
			 */
			dest->iSpecAttrData = src->iSpecAttrData;
		return dest;
	}
}

/***************************************************************************
 * rvSdpSpecAttrGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the value of a special attribute;
 *      The value is first calculated using the appropriate callback 
 *      (iGetValueFunc) in RvSdpSpecAttributeData structure and then stored 
 *      in iAttrValue field of the RvSdpAttribute instance.
 *      The iAttrValue is returned 
 *          
 * Return Value: 
 *      The value of iAttrValue of RvSdpAttribute is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - pointer to RvSdpAttribute instance.
 ***************************************************************************/
const char* rvSdpSpecAttrGetValue(RvSdpAttribute *attr)
{

	if (attr->iSpecAttrData && attr->iSpecAttrData->iGetValueFunc)
	{
		char txt[2048], *p = NULL;
		p = (*attr->iSpecAttrData->iGetValueFunc)(attr,txt);
		rvSdpSetTextField(&attr->iAttrValue,attr->iObj,p);
	}
    return RV_SDP_EMPTY_STRING(attr->iAttrValue);
}

/*
 *	Special attribute  parse function for the framerate and fmtp special attributes. 
 *  The constructed object is added to current context message or media 
 *  descriptor.
 *  If parsing or construction fails the correspondent status is returned.
 */
RvSdpSpecAttrParseSts rvSdpSpecialAttrParsing(
				const RvSdpSpecAttributeData* sad,  /* the special attribute data */
                RvBool createAsBadSyntax,           /* not used */
                RvSdpParserData* pD,    /* the parser data instance */        
                RvSdpCommonFields *cm,  /* the current context common fields instance,
                                           here the special attribute will be added */
				char* n,                /* the attribute name */
                char* v,                /* the attribute value to parse */
                REPLACING_DECL)         /* used for zero-substitutions in input buffer */
{
	UNUSE_REPLACING_DECL;
	RV_UNUSED_ARG(n);
    RV_UNUSED_ARG(createAsBadSyntax);
	if (!rvSdpSpecialAttrSetValue(pD->iMsg,cm,sad->iFieldType,v,sad->iIsMultiple))
            return rvSdpSpAttrPrsAllocFail;
	return rvSdpSpAttrPrsOK;
}

/*
 *	Special attribute  parse function for the connection mode special attributes. 
 *  The constructed object is added to current context message or media 
 *  descriptor.
 *  If parsing or construction fails the correspondent status is returned.
 */
RvSdpSpecAttrParseSts rvSdpConnModeAttrParsing(
				const RvSdpSpecAttributeData* sad,  /* the special attribute data */
                RvBool createAsBadSyntax,           /* not used */
                RvSdpParserData* pD,    /* the parser data instance */        
                RvSdpCommonFields *cm,  /* the current context common fields instance,
                                           here the special attribute will be added */
				char* n,                /* the attribute name */
                char* v,                /* the attribute value to parse */
                REPLACING_DECL)         /* used for zero-substitutions in input buffer */
{	
	RvSdpConnectionMode  m;
	UNUSE_REPLACING_DECL;
	RV_UNUSED_ARG(createAsBadSyntax);
	RV_UNUSED_ARG(sad);

	m = rvSdpConnModeTxt2Val(n);
	if (m == RV_SDPCONNECTMODE_NOTSET || v != NULL)
		return rvSdpSpAttrPrsCrtRegular; 

	if (rvSdpGetConnectionMode(cm) != RV_SDPCONNECTMODE_NOTSET)
	{
		RV_SDP_WARNING_SET_AND_CREATE_WARNING(RvSdpWarnConnModeSetAlready,pD);    
	}                
	rvSdpSetConnectionMode(pD->iMsg,cm,m);
	return rvSdpSpAttrPrsOK;
}

/*
 *	This array contains data for all types of special attributes supported.
 */
static const RvSdpSpecAttributeData gcSpecAttributesData[] = 
{
    {"rtpmap",
		SDP_FIELDTYPE_RTP_MAP,
			(rvAttrDestructCB)rvSdpRtpMapDestruct,
				(rvAttrCopyCB)rvSdpRtpMapCopy2,
					(rvAttrGetValueCB)rvSdpRtpMapGetValue,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
						RV_OFFSETOF(RvSdpRtpMap,iBadSyntaxField),
#endif
							(rvAttrReshuffleCB)rvSdpRtpMapReshuffle,
								rvSdpRtpMapParsing,
									RV_TRUE},
    {"sendonly",
		SDP_FIELDTYPE_CONNECTION_MODE,
			NULL,
				NULL,
					NULL,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						-1,
#endif
							NULL,
								rvSdpConnModeAttrParsing,
									RV_FALSE},
    {"recvonly",
		SDP_FIELDTYPE_CONNECTION_MODE,
			NULL,
				NULL,
					NULL,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						-1,
#endif
							NULL,
								rvSdpConnModeAttrParsing,
									RV_FALSE},
    {"sendrecv",
		SDP_FIELDTYPE_CONNECTION_MODE,
			NULL,
				NULL,
					NULL,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						-1,
#endif
							NULL,
								rvSdpConnModeAttrParsing,
									RV_FALSE},
    {"inactive",
		SDP_FIELDTYPE_CONNECTION_MODE,
			NULL,
				NULL,
					NULL,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						-1,
#endif
							NULL,
								rvSdpConnModeAttrParsing,
									RV_FALSE},
#ifdef RV_SDP_KEY_MGMT_ATTR
    {"key-mgmt",
		SDP_FIELDTYPE_KEY_MGMT,
			(rvAttrDestructCB)rvSdpKeyMgmtDestruct,
				(rvAttrCopyCB)rvSdpKeyMgmtCopy2,
					(rvAttrGetValueCB)rvSdpKeyMgmtGetValue,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						RV_OFFSETOF(RvSdpKeyMgmtAttr,iBadSyntaxField),
#endif
							(rvAttrReshuffleCB)rvSdpKeyMgmtReshuffle,
								rvSdpKeyMgmtParsing,
									RV_TRUE},
#endif /*RV_SDP_KEY_MGMT_ATTR*/
#ifdef RV_SDP_CRYPTO_ATTR
    {"crypto",
		SDP_FIELDTYPE_CRYPTO,
			(rvAttrDestructCB)rvSdpCryptoDestruct,
				(rvAttrCopyCB)rvSdpCryptoCopy2,
					(rvAttrGetValueCB)rvSdpCryptoGetValue,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						RV_OFFSETOF(RvSdpCryptoAttr,iBadSyntaxField),
#endif
							(rvAttrReshuffleCB)rvSdpCryptoReshuffle,
								rvSdpCryptoParsing,
									RV_TRUE},
#endif /*RV_SDP_CRYPTO_ATTR*/

#ifdef RV_SDP_FRAMERATE_ATTR
    {"framerate",
		SDP_FIELDTYPE_FRAMERATE,
			NULL,
				NULL,
					NULL,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						-1,
#endif
							NULL,
								rvSdpSpecialAttrParsing,
									RV_FALSE},
#endif /*RV_SDP_FRAMERATE_ATTR*/
#ifdef RV_SDP_FMTP_ATTR
    {"fmtp",
		SDP_FIELDTYPE_FMTP,
			NULL,
				NULL,
					NULL,
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						-1,
#endif
							NULL,
								rvSdpSpecialAttrParsing,
									RV_TRUE},
#endif /*RV_SDP_FMTP_ATTR*/
    {NULL,	
		SDP_FIELDTYPE_NOT_SET,	
			NULL,	
				NULL,	
					NULL,	
#if defined(RV_SDP_CHECK_BAD_SYNTAX)	
						0,	
#endif
							NULL,	
								NULL,
                                    RV_FALSE},
};


/* this function help in getting spec attr data from 
   the global array gcSpecAttributesData */
const RvSdpSpecAttributeData* 
rvSdpFindSpecAttrDataByFieldType(RvSdpFieldTypes fieldType)
{
	const RvSdpSpecAttributeData* attrData;

	attrData = gcSpecAttributesData;
	while (attrData->iName)
	{
		if (attrData->iFieldType == (RvSdpFieldTypes)fieldType)
			return attrData;
		attrData++;
	}
	return NULL;
}

/* this function help in getting spec attr data from 
   the global array gcSpecAttributesData */
const RvSdpSpecAttributeData* rvSdpFindSpecAttrDataByName(const char* name)
{
	const RvSdpSpecAttributeData* attrData;
	
	attrData = gcSpecAttributesData;
	while (attrData->iName)
	{
		if (!strcasecmp(attrData->iName,name))
			return attrData;
		attrData++;
	}
	return NULL;
}

#define RV_SDP_IS_SPEC_ATTR(_attr,_t) \
	(((_t) == SDP_FIELDTYPE_NOT_SET \
            && (_attr)->iSpecAttrData == NULL) ||\
               ((_attr)->iSpecAttrData && \
               (_attr)->iSpecAttrData->iFieldType == (_t)))

/***************************************************************************
 * rvSdpGetNumOfSpecialAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets  the number of special attributes of specific type in message 
 *      or media descrtiptor.
 *          
 * Return Value: 
 *      Returns the number of special attributes  of specific type in message
 *      or media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *          commF - common fields pointer of message or media where number of 
 *                  special attributes is needed.
 *          specAttrType - the type of special attribute is question.
 ***************************************************************************/
RvSize_t rvSdpGetNumOfSpecialAttr(
            const RvSdpCommonFields* commF, 
            RvSdpFieldTypes specAttrType)
{
    RvSize_t cnt = 0;
    RvSdpAttribute *attr;
    RvSdpListIter iter;

    for (attr = (RvSdpAttribute*)rvSdpListGetFirst(&commF->iAttrList,&iter); attr; attr = (RvSdpAttribute*)rvSdpListGetNext(&iter))
    {
		printf("attr->iAttrName=%s\n",attr->iAttrName);
		if (RV_SDP_IS_SPEC_ATTR(attr,specAttrType))
            cnt++;
    }
    return cnt;
}

/***************************************************************************
 * rvSdpGetFirstSpecialAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first special attribute object of specific type defined 
 *      in the message or  media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute  object or the NULL pointer if there are no
 *      special attributes of given type defined in the message or media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *          commF - common fields pointer of message or media descriptor.
 *          iter - pointer to RvSdpListIter to be used for subsequent 
 *                 rvSdpGetNextSpecialAttr calls.
 *          specAttrType - the type of special attribute is question.
 ***************************************************************************/
RvSdpAttribute* rvSdpGetFirstSpecialAttr(
            RvSdpCommonFields* commF, 
            RvSdpListIter* iter, 
            RvSdpFieldTypes specAttrType)
{
    RvSdpAttribute *attr;
    for (attr = (RvSdpAttribute*)rvSdpListGetFirst(&commF->iAttrList,iter); attr; attr = (RvSdpAttribute*)rvSdpListGetNext(iter))
    {
        if (RV_SDP_IS_SPEC_ATTR(attr,specAttrType))
            return attr;
    }
    return NULL;
}

/***************************************************************************
 * rvSdpGetNextSpecialAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next special attribute object of specific type defined 
 *      in the message or  media descriptor. 
 *      Also modifies the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute  object or the NULL pointer if there are no
 *      more special attributes of given type defined in the message or media 
 *      descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *          iter - pointer to RvSdpListIter to be used for subsequent 
 *                 rvSdpGetNextSpecialAttr calls.
 *          specAttrType - the type of special attribute is question.
 ***************************************************************************/
RvSdpAttribute* rvSdpGetNextSpecialAttr(
            RvSdpListIter* iter, 
            RvSdpFieldTypes specAttrType)
{
    RvSdpAttribute *attr;
    for (attr = (RvSdpAttribute*)rvSdpListGetNext(iter);attr;
                        attr = (RvSdpAttribute*)rvSdpListGetNext(iter))
    {
        if (RV_SDP_IS_SPEC_ATTR(attr,specAttrType))
            return  attr;
    }
    return NULL;
}

/***************************************************************************
 * rvSdpGetSpecialAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the special attribute object of specific type defined 
 *      in the message or media descriptor by zero-based index. 
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute  object.
 * ------------------------------------------------------------------------
 * Arguments:
 *          commF - common fields pointer of message or media descriptor.
 *          index - zero-based index of special attribute. 
 *          specAttrType - the type of special attribute is question.
 ***************************************************************************/
RvSdpAttribute* rvSdpGetSpecialAttr(
            RvSdpCommonFields* commF, 
            RvSize_t index, 
            RvSdpFieldTypes specAttrType)
{
    RvSdpAttribute *sa;
    RvSdpListIter i;
    for (sa = rvSdpGetFirstSpecialAttr(commF,&i,specAttrType); sa; 
                            sa = rvSdpGetNextSpecialAttr(&i,specAttrType), index--)
        if (index == 0)
            return sa;
        return NULL;
}


/***************************************************************************
 * rvSdpRemoveSpecialAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes and destroys the special attribute object of specific type defined 
 *      in the message or media descriptor by zero-based index. 
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *          commF - common fields pointer of message or media descriptor.
 *          index - zero-based index of special attribute. 
 *          specAttrType - the type of special attribute is question.
 ***************************************************************************/
void rvSdpRemoveSpecialAttr(
            RvSdpCommonFields* commF, 
            RvSize_t index, 
            RvSdpFieldTypes specAttrType)
{
    RvSdpAttribute* sa;
    sa = rvSdpGetSpecialAttr(commF,index,specAttrType);
    if (!sa)
        return;
    rvSdpListRemoveByValue(&commF->iAttrList,sa);
}

/***************************************************************************
 * rvSdpClearSpecialAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes and destroys all special attribute object of specific type defined 
 *      in the message or media descriptor. 
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *          commF - common fields pointer of message or media descriptor.
 *          specAttrType - the type of special attributes is question.
 ***************************************************************************/
void rvSdpClearSpecialAttr(RvSdpCommonFields* commF, RvSdpFieldTypes specAttrType)
{
    RvSdpListIter i;
    void *sa;
    for (;;)
    {
        sa = rvSdpGetFirstSpecialAttr(commF,&i,specAttrType);
        if (!sa)
            break;
        rvSdpListRemoveCurrent(&i);
    }
}

/***************************************************************************
 * rvSdpGetConnectionMode
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the connection mode of the message or media
 *      descriptor.
 *          
 * Return Value: 
 *      Returns the connection mode or RV_SDPCONNECTMODE_NOTSET if the 
 *      SDP_FIELDTYPE_CONNECTION_MODE special attribute is not set in the message
 *      or media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      commF - the RvSdpCommonFields instance of message or media.
 ***************************************************************************/
RvSdpConnectionMode rvSdpGetConnectionMode(const RvSdpCommonFields* commF)
{
    RvSdpAttribute *attr;
    RvSdpConnectionMode m = RV_SDPCONNECTMODE_NOTSET;

	attr = rvSdpGetSpecialAttr((RvSdpCommonFields*)commF,0,SDP_FIELDTYPE_CONNECTION_MODE);
	if (!attr)
		return RV_SDPCONNECTMODE_NOTSET;

	m = rvSdpConnModeTxt2Val(attr->iAttrName);	if (m != RV_SDPCONNECTMODE_NOTSET)
		return m;
	attr->iSpecAttrData = NULL;
	return RV_SDPCONNECTMODE_NOTSET;
}

/***************************************************************************
 * rvSdpSetConnectionMode
 * ------------------------------------------------------------------------
 * General: 
 *      Sets/modifies/removes the connection mode of the message or media
 *      descriptor.
 *      If the SDP_FIELDTYPE_CONNECTION_MODE special attribute was not set
 *      in a message or media it will be added. In case the 'mode' is 
 *      RV_SDPCONNECTMODE_NOTSET the special attribute will be removed.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - the RvSdpMsg instance.
 *      commF - the RvSdpCommonFields instance of message or media.
 *      mode - the desired connection mode.
 ***************************************************************************/
RvSdpStatus 
rvSdpSetConnectionMode(RvSdpMsg* msg, RvSdpCommonFields* commF, RvSdpConnectionMode m)
{
    RvSdpAttribute *attr;

	attr = rvSdpGetSpecialAttr(commF,0,SDP_FIELDTYPE_CONNECTION_MODE);
	
	if (m == RV_SDPCONNECTMODE_NOTSET && attr)
	{
		rvSdpRemoveSpecialAttr(commF,0,SDP_FIELDTYPE_CONNECTION_MODE);
		return RV_SDPSTATUS_OK;
	}
	else if (attr)
		return rvSdpSetTextField(&attr->iAttrName,attr->iObj,rvSdpConnModeVal2Txt(m));
	else if (!attr && m != RV_SDPCONNECTMODE_NOTSET)
	{
		attr = rvSdpAddAttr2(msg,commF,NULL,rvSdpConnModeVal2Txt(m),NULL);
		if (attr)
			attr->iSpecAttrData = 
                    rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_CONNECTION_MODE);
        if (!attr)
            return RV_SDPSTATUS_ALLOCFAIL;
	}
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpSpecialAttrSetValue
 * ------------------------------------------------------------------------
 * General: 
 *      For special attributes allowing multiple appearances adds another
 *      instance of given type special attribute with given name.
 *      For special attributes with at most single appearance modifies the
 *      value of attribute if it was set before or creates the attribute 
 *      if it was not set before.
 *          
 * Return Value: 
 *      Returns the pointer to the RvSdpAttribute of created/modified object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - the RvSdpMsg where change happens.
 *      commF - common fields pointer of message or media descriptor.
 *      specAttrType - the type of special attributes is question.
 *      value - the desired value of special attribute.
 *      isMultiple - whether the special attribute allows multiple appearances.
 ***************************************************************************/
RvSdpAttribute* rvSdpSpecialAttrSetValue(RvSdpMsg *msg, RvSdpCommonFields* commF,
										 RvSdpFieldTypes specAttrType, 
                                         const char* value, RvBool isMultiple)
{
	RvSdpAttribute *attr;
	const RvSdpSpecAttributeData* sad;

	sad = rvSdpFindSpecAttrDataByFieldType(specAttrType);
	if (!sad)
		return NULL;

	if (!isMultiple)
	{
		attr = rvSdpGetSpecialAttr(commF,0,specAttrType);
		if (attr)
		{
			if (rvSdpSetTextField(&attr->iAttrValue,attr->iObj,value) != RV_SDPSTATUS_OK)
				return NULL;
			else
				return attr;
		}
	}

	attr = rvSdpAddAttr2(msg,commF,NULL,sad->iName,value);
    if (!attr)
		return NULL;

    attr->iSpecAttrData = sad;
	return attr;
}

/***************************************************************************
 * rvSdpAttributeCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source attribute to destination.
 *      Only generic attributes can be copied in this way. If one of the 
 *      arguments is special attribute NULL will be returned.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination generic attribute. Must point 
 *             to constructed RvSdpAttribute object.
 *      src - a source generic attribute.
 ***************************************************************************/
RvSdpAttribute* rvSdpAttributeCopy(RvSdpAttribute* dest, const RvSdpAttribute* src)
{
    RvSdpMsg *msg;

    if (!dest || !src)
        return NULL;

    if (dest && dest->iSpecAttrData)
        return NULL;
    if (src && src->iSpecAttrData)
        return NULL;
    
    if (dest && RV_SDP_OBJ_IS_MESSAGE(dest))
        msg = dest->iObj;
    else
        msg = NULL;
    
    return rvSdpAttributeConstruct2(msg,dest,src->iAttrName,
        src->iAttrValue,dest->iObj,RV_TRUE);
}


#ifdef RV_SDP_FRAMERATE_ATTR
/***************************************************************************
 * rvSdpMediaDescrGetFrameRate
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the frame-rate special attribute value of the media descriptor.
 *          
 * Return Value: 
 *      Returns the attributes value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
const char* rvSdpMediaDescrGetFrameRate(RvSdpMediaDescr* descr)
{
    const char* p;
    RvSdpAttribute *attr;
    attr = rvSdpGetSpecialAttr(&descr->iCommonFields,0,SDP_FIELDTYPE_FRAMERATE);
    p = (attr) ? attr->iAttrValue : NULL;
    return RV_SDP_EMPTY_STRING(p);
}
#endif /*RV_SDP_FRAMERATE_ATTR*/


#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpAttributeSetName
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the attribute name.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails. 
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object. 
 *      name - the new attribute's name. 
 ***************************************************************************/
RvSdpStatus
rvSdpAttributeSetName(RvSdpAttribute* attr, const char* name)
{
    return (attr->iSpecAttrData != NULL) ? 
        RV_SDPSTATUS_ILLEGAL_SET : rvSdpSetTextField(&attr->iAttrName,attr->iObj,name);
}

/***************************************************************************
 * rvSdpAttributeGetName
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the attribute name.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object. 
 ***************************************************************************/
const char* rvSdpAttributeGetName(RvSdpAttribute* attr)
{
    return RV_SDP_EMPTY_STRING(attr->iAttrName);
}

/***************************************************************************
 * rvSdpAttributeGetValue
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the attribute value or the empty string if the value is not set.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object. 
 ***************************************************************************/
const char* rvSdpAttributeGetValue(RvSdpAttribute* attr)
{
    return (attr->iSpecAttrData == NULL) ? 
        RV_SDP_EMPTY_STRING(attr->iAttrValue) : rvSdpSpecAttrGetValue(attr);
}

/***************************************************************************
 * rvSdpAttributeSetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the attribute value.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails. 
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object. 
 *      name - the new attribute's value. 
 ***************************************************************************/
RvSdpStatus 
rvSdpAttributeSetValue(RvSdpAttribute* attr, const char* value)
{
    return (attr->iSpecAttrData != NULL) ? 
            RV_SDPSTATUS_ILLEGAL_SET : 
            rvSdpSetTextField(&attr->iAttrValue,attr->iObj,value);
}                               

/***************************************************************************
 * rvSdpAttributeConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpAttribute object.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr' 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to valid RvSdpAttribute object.
 *      name - the name of the attribute.
 *      value - the value of the attribute.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpAttribute* 
rvSdpAttributeConstructA(RvSdpAttribute* attr, const char* name, 
                         const char* value, RvAlloc* a)
{
    return rvSdpAttributeConstruct2(NULL,attr,name,value,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpAttributeConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpAttribute object using default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr' 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to valid RvSdpAttribute object.
 *      name - the name of the attribute.
 *      value - the value of the attribute.
 ***************************************************************************/
RvSdpAttribute* 
rvSdpAttributeConstruct(RvSdpAttribute* attr, const char* name, const char* value)
{
    return rvSdpAttributeConstruct2(NULL,attr,name,value,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpAttributeConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs an attribute  object and copies the values from a source 
 *      attribute. 
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr' 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to attribute to be constructed. Must point 
 *             to valid memory.
 *      src - a source attribute.
 ***************************************************************************/
RvSdpAttribute* rvSdpAttributeConstructCopy(RvSdpAttribute* dest,  
                                            const RvSdpAttribute* src)
{
    return rvSdpAttributeConstruct2(NULL,dest,src->iAttrName,
        src->iAttrValue,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpAttributeConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs an attribute  object and copies the values from a source 
 *      attribute.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr' 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to attribute to be constructed. Must point 
 *             to valid memory.
 *      src - a source attribute.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpAttribute* rvSdpAttributeConstructCopyA(RvSdpAttribute* dest, 
                                             const RvSdpAttribute* src, RvAlloc* a)
{
    return rvSdpAttributeConstruct2(NULL,dest,src->iAttrName,
        src->iAttrValue,a,RV_FALSE);
}


#ifdef RV_SDP_FRAMERATE_ATTR

/* framerate attribute */


/***************************************************************************
 * rvSdpMediaDescrSetFrameRate
 * ------------------------------------------------------------------------
 * General: 
 *      Sets/modifies the frame-rate special attribute of the media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      val - the frame rate attribute value.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetFrameRate(RvSdpMediaDescr* descr, const char* val)
{
	return (rvSdpSpecialAttrSetValue(descr->iSdpMsg,&descr->iCommonFields,
						SDP_FIELDTYPE_FRAMERATE,val,RV_FALSE)) ? 
						RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

/***************************************************************************
 * rvSdpMediaDescrGetFrameRate
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the frame-rate special attribute value of the media descriptor.
 *          
 * Return Value: 
 *      Returns the attributes value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrDestroyFrameRate(RvSdpMediaDescr* descr)
{
	rvSdpRemoveSpecialAttr(&descr->iCommonFields,0,SDP_FIELDTYPE_FRAMERATE);
}

#endif /*RV_SDP_FRAMERATE_ATTR*/


#ifdef RV_SDP_FMTP_ATTR

/* fmtp attribute */

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the fmtp special attribute object pointed 
 *      by list iterator in the context  of media descriptor. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveCurrentFmtp(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}


/***************************************************************************
 * rvSdpMediaDescrRemoveFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the fmtp special attribute object by index in the
 *      context of media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMediaDescrGetNumOfFmtp call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveFmtp(RvSdpMediaDescr* descr, RvSize_t index)
{
	rvSdpRemoveSpecialAttr(&descr->iCommonFields,index,SDP_FIELDTYPE_FMTP);
}

/***************************************************************************
 * rvSdpMediaDescrClearFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all fmtp special attributes set in media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearFmtp(RvSdpMediaDescr* descr)
{
	rvSdpClearSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_FMTP);
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first fmtp special attribute defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute (of fmtp)  object or the NULL pointer if there 
 *      are no fmtp attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextFmtp calls
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetFirstFmtp(RvSdpMediaDescr* descr, RvSdpListIter* iter)
{
	return rvSdpGetFirstSpecialAttr(&descr->iCommonFields,iter,SDP_FIELDTYPE_FMTP);
}

/***************************************************************************
 * rvSdpMediaDescrGetNextFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next fmtp special attribute defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute (fmtp) object or the NULL pointer if there is no
 *      more fmtp attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Fmtp function. 
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetNextFmtp(RvSdpListIter* iter)
{
	return rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_FMTP);
}

/***************************************************************************
* rvSdpMediaDescrGetFmtp
* ------------------------------------------------------------------------
* General: 
*      Gets an fmtp special attribute object by index (in media descriptor context). 
*          
* Return Value: 
*      The requested RvSdpAttribute (of fmtp special attribute) pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfFmtp() call. 
***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetFmtp(const RvSdpMediaDescr* descr, RvSize_t index)
{
	return rvSdpGetSpecialAttr((RvSdpCommonFields*)&descr->iCommonFields,
							   index,SDP_FIELDTYPE_FMTP);
}

/***************************************************************************
 * rvSdpMediaDescrAddFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new fmtp special attribute object to the media descriptor.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpAttribute object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      val - the value of new fmtp special attribute.
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrAddFmtp(RvSdpMediaDescr* descr, const char* val)
{
	return rvSdpSpecialAttrSetValue(descr->iSdpMsg,&descr->iCommonFields,
									SDP_FIELDTYPE_FMTP,val,RV_TRUE);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfFmtp
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor fmtp special attributes.
 *          
 * Return Value: 
 *      Number of fmtp attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfFmtp(const RvSdpMediaDescr* descr)
{
	return rvSdpGetNumOfSpecialAttr(&descr->iCommonFields,
									SDP_FIELDTYPE_FMTP);
}

#endif /*RV_SDP_FMTP_ATTR*/

#endif /*#ifndef RV_SDP_USE_MACROS*/

