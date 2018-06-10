/******************************************************************************
Filename    :rvsdpmsg.c
Description : message manipulation routines.

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
#include "rvsdpprivate.h"
#include "rvcbase.h"

/*
 *	This function is registered within the RvStringBuffer as reshuffle callback and 
 *  is called by strings buffer when it needs to reallocate its memory chunk.
 *  This function passes over all objects of SDP message and causes each message own 
 *  object to copy its textual strings to the '*ptr'.
 *  The message owned objects are iterated using 'line object' mechanism.
 */
void rvSdpMsgReshuffle(void* usrD, char** ptr)
{
    RvSdpMsg* msg = (RvSdpMsg*) usrD;
    RvSdpLineObject* lo;
    int cnt;
    char *p; 
	RvSdpAttribute* attr;

    for (lo = msg->iHeadLO; lo; lo = lo->iNext)
    {
        p = (char*)lo - lo->iOffset;
        switch (lo->iFieldType)
        {
        case SDP_FIELDTYPE_VERSION:
            rvSdpChangeText(ptr,&((RvSdpVersion*)p)->iVersionTxt);
            break;
        case SDP_FIELDTYPE_ORIGIN:
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iUserName);
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iSessionId);
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iVersion);
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iAddress);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iBadSyntaxField);
#endif
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iNetTypeStr);
            rvSdpChangeText(ptr,&((RvSdpOrigin*)p)->iAddrTypeStr);
            break;
        case SDP_FIELDTYPE_SESSION_ID:
            rvSdpChangeText(ptr,&((RvSdpSessId*)p)->iSessIdTxt);
            break;
        case SDP_FIELDTYPE_INFORMATION:
            rvSdpChangeText(ptr,&((RvSdpInformation*)p)->iInfoTxt);
            break;
        case SDP_FIELDTYPE_URI:
            rvSdpChangeText(ptr,&((RvSdpUri*)p)->iUriTxt);
            break;
        case SDP_FIELDTYPE_EMAIL:
            rvSdpChangeText(ptr,&((RvSdpEmail*)p)->iAddress);
            rvSdpChangeText(ptr,&((RvSdpEmail*)p)->iText);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpEmail*)p)->iBadSyntaxField);
#endif
            break;
        case SDP_FIELDTYPE_PHONE:
            rvSdpChangeText(ptr,&((RvSdpPhone*)p)->iPhoneNumber);
            rvSdpChangeText(ptr,&((RvSdpPhone*)p)->iText);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpPhone*)p)->iBadSyntaxField);
#endif
            break;
        case SDP_FIELDTYPE_CONNECTION:
            rvSdpChangeText(ptr,&((RvSdpConnection*)p)->iAddress);
            rvSdpChangeText(ptr,&((RvSdpConnection*)p)->iNetTypeStr);
            rvSdpChangeText(ptr,&((RvSdpConnection*)p)->iAddrTypeStr);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpConnection*)p)->iBadSyntaxField);
#endif
            break;
        case SDP_FIELDTYPE_BANDWIDTH:
            rvSdpChangeText(ptr,&((RvSdpBandwidth*)p)->iBWType);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpBandwidth*)p)->iBadSyntaxField);
#endif
            break;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
        case SDP_FIELDTYPE_TIME:
            rvSdpChangeText(ptr,&((RvSdpSessionTime*)p)->iBadSyntaxField);
            break;
        case SDP_FIELDTYPE_REPEAT:
            rvSdpChangeText(ptr,&((RvSdpRepeatInterval*)p)->iBadSyntaxField);
            break;
        case SDP_FIELDTYPE_TIME_ZONE_ADJUST:
            rvSdpChangeText(ptr,&((RvSdpTZA*)p)->iBadSyntaxField);
            break;
#endif
        case SDP_FIELDTYPE_KEY:
            rvSdpChangeText(ptr,&((RvSdpKey*)p)->iData);
            rvSdpChangeText(ptr,&((RvSdpKey*)p)->iTypeStr);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpKey*)p)->iBadSyntaxField);
#endif
            break;
        case SDP_FIELDTYPE_MEDIA:
            for (cnt = 0; cnt < ((RvSdpMediaDescr*)p)->iMediaFormatsNum; cnt++)
                rvSdpChangeText(ptr,&((RvSdpMediaDescr*)p)->iMediaFormats[cnt]);
            rvSdpChangeText(ptr,&((RvSdpMediaDescr*)p)->iProtocolStr);
            rvSdpChangeText(ptr,&((RvSdpMediaDescr*)p)->iMediaTypeStr);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
            rvSdpChangeText(ptr,&((RvSdpMediaDescr*)p)->iBadSyntaxField);
#endif
            break;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
        case SDP_FIELDTYPE_UNKNOWN_TAG:
            rvSdpChangeText(ptr,&((RvSdpOther*)p)->iValue);
            break;
#endif            
        case SDP_FIELDTYPE_ATTRIBUTE:
            rvSdpChangeText(ptr,&((RvSdpAttribute*)p)->iAttrName);
            rvSdpChangeText(ptr,&((RvSdpAttribute*)p)->iAttrValue);
			attr = (RvSdpAttribute*)p;
			if (attr->iSpecAttrData && attr->iSpecAttrData->iReshuffleFunc)
				(attr->iSpecAttrData->iReshuffleFunc)(attr,ptr);
        default:
            break;
        }
    }
}

/* guarantee that strings buffer will be big enough to fit 'len' more bytes; 
 * as a result reshuffle procedure may be performed */
void rvSdpMsgPromiseBuffer(
            RvSdpMsg* msg,  /* message owning strings buffer */
            int len)        /* number of bytes to guarantee */
{
    if (!msg)
        return;

    if (msg->iStrBuffer.iBufferOffset + len + 100 > msg->iStrBuffer.iBufferSz)
        rvSdpStringBufferReshuffle(&msg->iStrBuffer,len);
}

/***************************************************************************
 * RvSdpMsgConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the instance of RvSdpMsg, initializes all internal fields,
 *      allocates memory for the strings buffer and pools of reusable objects 
 *          
 * Return Value: 
 *      valid RvSdpMsg pointer on success or NULL on failure
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg instance to be constructed, if the value is 
 *            NULL the instance will be allocated within the function.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMsg* rvSdpMsgConstruct2(RvSdpMsg* msg, RvAlloc* a)
{
    RvBool isAllocated;
    if (!a)
        /* the default allocator will be used */
        a = rvSdpGetDefaultAllocator();

    if (!msg)
    {
        msg = rvSdpAllocAllocate(a,sizeof(RvSdpMsg));
        if (!msg)
            return NULL;
        isAllocated = RV_TRUE;
    }
    else
        isAllocated = RV_FALSE;

    memset(msg,0,sizeof(RvSdpMsg));

    msg->iMagicNumber = RV_SDP_MESSAGE_MAGIC_NUMBER;
    msg->iAllocator = a;
    msg->iIsAllocated = isAllocated;

    rvSdpListInitialize(&msg->iEmailList,RV_OFFSETOF(RvSdpEmail,iNextEmail),
        (rvListNodeDestructFunc)rvSdpEmailDestruct);
    rvSdpListInitialize(&msg->iPhoneList,RV_OFFSETOF(RvSdpPhone,iNextPhone), 
        (rvListNodeDestructFunc)rvSdpPhoneDestruct);
    rvSdpListInitialize(&msg->iSessionTimeList,
        RV_OFFSETOF(RvSdpSessionTime,iNextSessionTime),
        (rvListNodeDestructFunc)rvSdpSessionTimeDestruct);
    rvSdpListInitialize(&msg->iTZA.iTimeZoneList,
        RV_OFFSETOF(RvSdpTimeZoneAdjust,iNextTimeZone),
        (rvListNodeDestructFunc)rvSdpTimeZoneAdjustDestruct);    
    rvSdpListInitialize(&msg->iMediaDescriptors,
        RV_OFFSETOF(RvSdpMediaDescr,iNextMediaDescr),
        (rvListNodeDestructFunc)rvSdpMediaDescrDestruct);

    rvSdpCommonFieldsInitialize(&msg->iCommonFields);    

    if (rvSdpPoolInitialize(&msg->iAttrsPool,RV_OFFSETOF(RvSdpAttribute,iNextAttribute),
                RV_SDP_ATTRPOOL_INIT_SZ,RV_SDP_ATTRPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpAttributeCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpAttributeDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;

#ifdef RV_SDP_CHECK_BAD_SYNTAX
    if (rvSdpPoolInitialize(&msg->iOthersPool,RV_OFFSETOF(RvSdpOther,iNextOther),
                RV_SDP_OTHERPOOL_INIT_SZ,RV_SDP_OTHERPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpOtherCreateByPool,(void*)msg,
                ( rvListNodeDestructFunc)rvSdpOtherDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
#endif
    
    if (rvSdpPoolInitialize(&msg->iEmailsPool,RV_OFFSETOF(RvSdpEmail,iNextEmail),
                RV_SDP_EMAILPOOL_INIT_SZ,RV_SDP_EMAILPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpEmailCreateByPool,(void*)msg,
                ( rvListNodeDestructFunc)rvSdpEmailDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;

    if (rvSdpPoolInitialize(&msg->iPhonesPool,RV_OFFSETOF(RvSdpPhone,iNextPhone),
                RV_SDP_PHONEPOOL_INIT_SZ,RV_SDP_PHONEPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpPhoneCreateByPool,(void*)msg,
                ( rvListNodeDestructFunc)rvSdpPhoneDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;

    if (rvSdpPoolInitialize(&msg->iMediasPool,RV_OFFSETOF(RvSdpMediaDescr,iNextMediaDescr),
                RV_SDP_MEDIAPOOL_INIT_SZ,RV_SDP_MEDIAPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpMediaDescrCreateByPool,(void*)msg,
                ( rvListNodeDestructFunc)rvSdpMediaDescrDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
    
    if (rvSdpPoolInitialize(&msg->iRtpMapsPool,RV_OFFSETOF(RvSdpRtpMap,iNextRtpMap),
                RV_SDP_RTPMAPPOOL_INIT_SZ,RV_SDP_RTPMAPPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpRtpMapCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpRtpMapDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
    
    if (rvSdpPoolInitialize(&msg->iSessTimesPool,
                RV_OFFSETOF(RvSdpSessionTime,iNextSessionTime),
                RV_SDP_SESSTIMEPOOL_INIT_SZ,RV_SDP_SESSTIMEPOOL_INCR_SZ,                                    
                (rvCreatePoolObjFunc)rvSdpSessTimeCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpSessTimeDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
    
    if (rvSdpPoolInitialize(&msg->iTimeZonesPool,
                RV_OFFSETOF(RvSdpTimeZoneAdjust,iNextTimeZone),
                RV_SDP_TZAPOOL_INIT_SZ,RV_SDP_TZAPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpTimeZoneCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpTimeZoneAdjustDestroyByPool) 
                                                            != RV_SDPSTATUS_OK)
        goto failure;    
    
    if (rvSdpPoolInitialize(&msg->iConnectionsPool,
                RV_OFFSETOF(RvSdpConnection,iNextConnection),
                RV_SDP_CONNECTIONPOOL_INIT_SZ,RV_SDP_CONNECTIONPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpConnectionCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpConnectionDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
    
    if (rvSdpPoolInitialize(&msg->iBandwidthsPool,
                RV_OFFSETOF(RvSdpBandwidth,iNextBandwidth),
                RV_SDP_BANDWIDTHPOOL_INIT_SZ,RV_SDP_BANDWIDTHPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpBandwidthCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpBandwidthDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
    
    if (rvSdpPoolInitialize(&msg->iRepeatIntervalPool,
                RV_OFFSETOF(RvSdpRepeatInterval,iNextRepeatInterval),
                RV_SDP_REPEATINTERVALPOOL_INIT_SZ,RV_SDP_REPEATINTERVALPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpRepeatIntervalCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpRepeatIntervalDestroyByPool) 
                                                                    != RV_SDPSTATUS_OK)
        goto failure;

    if (rvSdpPoolInitialize(&msg->iTypedTimePool,
                RV_OFFSETOF(RvSdpTypedTime,iNextTypedTime),
                RV_SDP_TYPEDTIMEPOOL_INIT_SZ,RV_SDP_TYPEDTIMEPOOL_INCR_SZ,
                (rvCreatePoolObjFunc)rvSdpTypedTimeCreateByPool,(void*)msg,
                (rvListNodeDestructFunc)rvSdpTypedTimeDestroyByPool) != RV_SDPSTATUS_OK)
        goto failure;
    
    
    if (rvSdpStringBufferInitialize(&msg->iStrBuffer,msg->iAllocator,
					RV_SDP_MSG_BUFFER_SIZE,msg,rvSdpMsgReshuffle) != RV_SDPSTATUS_OK)
		goto failure;

    /*ALGR - version line should always be present */
    rvSdpMsgSetVersion2(msg, RV_SDP_SDPVERSION);
	msg->iVersion.iSetByMsgConstruct = RV_TRUE;
    
    return msg;

failure:

    rvSdpMsgDestruct(msg);
    return NULL;
}

/***************************************************************************
 * rvSdpMsgConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object will be constructed. 
 *      If the destination object is NULL pointer the destination
 *      object will be allocated within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMsg object or NULL. 
 *      src - a pointer to the source object
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMsg* rvSdpMsgConstructCopyA(RvSdpMsg* dest, const RvSdpMsg* src, RvAlloc* a)
{
    return rvSdpMsgCopy2(dest,src,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpSetSdpInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the information field of the SDP message or media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - the RvSdpInformation object of message or media.
 *      info - the new information value.
 *      msg - a pointer to the RvSdpMsg object.
***************************************************************************/
RvSdpStatus 
rvSdpSetSdpInformation(RvSdpInformation* dest, const char* info, RvSdpMsg *msg)
{
    rvSdpUnsetTextField(&dest->iInfoTxt,msg);
    if (rvSdpSetTextField(&dest->iInfoTxt,msg,info) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;

    if (msg)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_INFORMATION,&dest->iLineObj,
            RV_OFFSETOF(RvSdpInformation,iLineObj));    

    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpDestroySdpInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the information field of the SDP message or media descriptor.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - the RvSdpInformation object of message or media.
 *      msg - a pointer to the RvSdpMsg object.
***************************************************************************/
void rvSdpDestroySdpInformation(RvSdpInformation* dest, RvSdpMsg* msg)
{
    rvSdpUnsetTextField(&dest->iInfoTxt,msg);
    rvSdpLineObjsListRemove(msg,&dest->iLineObj);
}

/***************************************************************************
 * rvSdpMsgCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object can be constructed depending on the value of 'dontConstruct' 
 *      argument. If the destination object is NULL pointer, the destination
 *      object will be allocated within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMsg object or NULL. 
 *      src - a pointer to the source object
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - whether the 'dest' should be constructed.
 ***************************************************************************/
RvSdpMsg* rvSdpMsgCopy2(RvSdpMsg* dest, const RvSdpMsg* src, RvAlloc* a, 
                        RvBool dontConstruct)
{
    if (!dontConstruct)
    {
        dest = rvSdpMsgConstruct2(dest,a);
        if (!dest)
            return NULL;    
    }
    else
        rvSdpMsgDestructForReuse(dest);

    if (src->iVersion.iVersionTxt)
    {
        if (rvSdpMsgCopySdpVersion(&src->iVersion,dest) != RV_SDPSTATUS_OK)
            goto failure;
		dest->iVersion.iSetByMsgConstruct = src->iVersion.iSetByMsgConstruct;
    }

    if (RV_SDP_ORIGIN_IS_USED(&src->iOrigin))
    {
        if (rvSdpOriginCopy2(&dest->iOrigin,&src->iOrigin,dest,RV_FALSE) == NULL)
            goto failure;
    }
    
    if (src->iSessId.iSessIdTxt)
    {
        if (rvSdpMsgCopySdpSessionId(&src->iSessId,dest) != RV_SDPSTATUS_OK)
            goto failure;
    }
    
    if (src->iUri.iUriTxt)
    {
        if (rvSdpMsgCopyURI(&src->iUri,dest) != RV_SDPSTATUS_OK)
            goto failure;
    }
    
    if (rvSdpListCopy(&dest->iEmailList,(RvSdpList*)&src->iEmailList,
                    (rvSdpListCopyFunc)rvSdpEmailConstructCopyA,dest) != RV_SDPSTATUS_OK)
        goto failure;
    if (rvSdpListCopy(&dest->iPhoneList,(RvSdpList*)&src->iPhoneList,
                    (rvSdpListCopyFunc)rvSdpPhoneConstructCopyA,dest) != RV_SDPSTATUS_OK)
        goto failure;
    if (rvSdpListCopy(&dest->iSessionTimeList,(RvSdpList*)&src->iSessionTimeList,
             (rvSdpListCopyFunc)rvSdpSessionTimeConstructCopyA,dest) != RV_SDPSTATUS_OK)
        goto failure;
    if (rvSdpListCopy(&dest->iMediaDescriptors,(RvSdpList*)&src->iMediaDescriptors,
               (rvSdpListCopyFunc)rvSdpMediaDescrConstructCopyA,dest) != RV_SDPSTATUS_OK)
        goto failure;

    if (rvSdpMsgTZACopy(dest,src) != RV_SDPSTATUS_OK)
        goto failure;

    
    if (rvSdpCommonFieldCopy2(&dest->iCommonFields,&src->iCommonFields,dest) 
                                                            != RV_SDPSTATUS_OK)
        goto failure;
    
    return dest;
    
failure:
    rvSdpMsgDestruct(dest);
    return NULL;
}

/***************************************************************************
 * rvSdpCommonFieldCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the contents of 'src' RvSdpCommonFields instance to the 'dest'
 *      RvSdpCommonFields instance.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - the destination RvSdpCommonFields instance.
 *      src - the source RvSdpCommonFields instance.
 *      obj - the RvSdpMsg pointer owning common fields object of message or 
 *            media descriptor OR the RvAlloc allocator.
 ***************************************************************************/
RvSdpStatus rvSdpCommonFieldCopy2(RvSdpCommonFields* dest, 
                                  const RvSdpCommonFields* src, void* obj)
{
    if (src->iInfo.iInfoTxt)
    {
        if (rvSdpSetSdpInformation(&dest->iInfo,src->iInfo.iInfoTxt,obj) 
                                                                != RV_SDPSTATUS_OK)
            goto failure;
    }
    
    if (rvSdpListCopy(&dest->iBandwidthList,(RvSdpList*)&src->iBandwidthList,
            (rvSdpListCopyFunc)rvSdpBandwidthConstructCopyA,obj) != RV_SDPSTATUS_OK)
        goto failure;
    
    if (rvSdpListCopy(&dest->iConnectionList,(RvSdpList*)&src->iConnectionList,
            (rvSdpListCopyFunc)rvSdpConnectionConstructCopyA,obj) != RV_SDPSTATUS_OK)
        goto failure;

    if (RV_SDP_KEY_IS_USED(&src->iKey))
    {
        if (rvSdpKeyCopy2(&dest->iKey,&src->iKey,obj,RV_FALSE) == NULL)
            goto failure;
    }
    
    if (rvSdpListCopy(&dest->iAttrList,(RvSdpList*)&src->iAttrList,
                    (rvSdpListCopyFunc)rvSdpAttributeCopy2,obj) != RV_SDPSTATUS_OK)
        goto failure;
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (rvSdpListCopy(&dest->iOtherList,(RvSdpList*)&src->iOtherList,
                (rvSdpListCopyFunc)rvSdpOtherConstructCopyA,obj) != RV_SDPSTATUS_OK)
        goto failure;
#endif
        
    return RV_SDPSTATUS_OK;
    
failure:
    rvSdpCommonFieldsDestruct(dest,obj);
    return RV_SDPSTATUS_ALLOCFAIL;
}


/* resets the contents of SDP messages (pools are not freed) */
void rvSdpMsgDestructForReuse(RvSdpMsg* msg)
{
    rvSdpMsgDestroyVersion(msg);
    rvSdpOriginDestruct(&msg->iOrigin);
    rvSdpMsgDestroySessionName(msg);
    rvSdpMsgDestroyInformation(msg);
    rvSdpMsgDestroyUri(msg);
    rvSdpKeyDestruct(&msg->iCommonFields.iKey);
    rvSdpCommonFieldsDestruct(&msg->iCommonFields,msg);
    rvSdpMsgTZADestroy(msg);
    
    rvSdpListClear(&msg->iEmailList);
    rvSdpListClear(&msg->iPhoneList);
    rvSdpListClear(&msg->iSessionTimeList);
    rvSdpListClear(&msg->iMediaDescriptors);
        
    rvSdpStringBufferReset(&msg->iStrBuffer);
}

/***************************************************************************
 * rvSdpMsgDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the RvSdpMsg object and frees up all internal memory.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to a RvSdpMsg object to be destructed.
 ***************************************************************************/
void rvSdpMsgDestruct(RvSdpMsg* msg)
{
    rvSdpMsgDestructForReuse(msg);
    
    rvSdpPoolDestroy(&msg->iAttrsPool);
    rvSdpPoolDestroy(&msg->iOthersPool);
    rvSdpPoolDestroy(&msg->iEmailsPool);
    rvSdpPoolDestroy(&msg->iPhonesPool);
    rvSdpPoolDestroy(&msg->iMediasPool);
    rvSdpPoolDestroy(&msg->iRtpMapsPool);
    rvSdpPoolDestroy(&msg->iSessTimesPool);
    rvSdpPoolDestroy(&msg->iTimeZonesPool);
    rvSdpPoolDestroy(&msg->iConnectionsPool);
    rvSdpPoolDestroy(&msg->iBandwidthsPool);
    rvSdpPoolDestroy(&msg->iRepeatIntervalPool);
    rvSdpPoolDestroy(&msg->iTypedTimePool);
    
    rvSdpStringBufferDestruct(&msg->iStrBuffer,msg->iAllocator);
    
    if (msg->iIsAllocated)
        rvSdpAllocDeallocate(msg->iAllocator,sizeof(RvSdpMsg),msg);
}



/* initializes the common (to message and media descriptor) fields  */
void rvSdpCommonFieldsInitialize(RvSdpCommonFields* fields)
{
    memset(fields,0,sizeof(RvSdpCommonFields));
    rvSdpListInitialize(&fields->iAttrList,RV_OFFSETOF(RvSdpAttribute,iNextAttribute),
        (rvListNodeDestructFunc)rvSdpAttributeDestruct);
    rvSdpListInitialize(&fields->iConnectionList,
                RV_OFFSETOF(RvSdpConnection,iNextConnection),
                (rvListNodeDestructFunc)rvSdpConnectionDestruct);
    rvSdpListInitialize(&fields->iBandwidthList,
            RV_OFFSETOF(RvSdpBandwidth,iNextBandwidth),
            (rvListNodeDestructFunc)rvSdpBandwidthDestruct);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpListInitialize(&fields->iOtherList,RV_OFFSETOF(RvSdpOther,iNextOther),
        (rvListNodeDestructFunc)rvSdpOtherDestruct);
#endif
}

/* destructs the common (to message and media descriptor) fields  */
void rvSdpCommonFieldsDestruct(RvSdpCommonFields* fields, void* obj)
{    
    rvSdpListClear(&fields->iConnectionList);
    rvSdpListClear(&fields->iBandwidthList);    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpListClear(&fields->iOtherList);
#endif
    rvSdpListClear(&fields->iAttrList);

    rvSdpKeyDestruct(&fields->iKey);
    rvSdpDestroySdpInformation(&fields->iInfo,obj);
}

/***************************************************************************
 * rvSdpMsgSetOrigin
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP origin field. 
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code 
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      username - the user name.
 *      session_id - the session id.
 *      version - the version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      address - the address, depending on the network type. For example, an 
 *                IP address for an IP network, and so on.
 ***************************************************************************/
RvSdpStatus 
rvSdpMsgSetOrigin(RvSdpMsg* msg, const char* username, const char* session_id, 
                  const char* version, RvSdpNetType nettype, RvSdpAddrType addrtype, 
                  const char* address)
{
    if (rvSdpOriginConstruct2(msg,&msg->iOrigin,username,session_id,version,
                                nettype,addrtype,address,NULL,NULL,RV_FALSE) == NULL)
        return RV_SDPSTATUS_ALLOCFAIL;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgSetConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new connection object at the session level.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code 
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP 
 *             address for an IP network, and so on.
 ***************************************************************************/
RvSdpStatus 
rvSdpMsgSetConnection(RvSdpMsg* msg, RvSdpNetType type, RvSdpAddrType addr_type, 
                      const char* addr)
{
    return (rvSdpAddConnection2(msg,type,addr_type,addr,NULL)) ?
                RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

/***************************************************************************
 * rvSdpAddConnection2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the connection to the message or media.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpConnection object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      obj - the pointer to RvSdpMsg or RvSdpCommonFields object where the 
 *            addition will be performed.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      badSyn - the proprietary formatted connection field or NULL if standard 
 *               connection is added.
 ***************************************************************************/
RvSdpConnection* 
rvSdpAddConnection2(void* obj,RvSdpNetType type, RvSdpAddrType addr_type, 
                    const char* addr, const char* badSyn)
{
    RvSdpConnection* conn;
    RvSdpMsg *msg = NULL;
    RvSdpMediaDescr *media = NULL;
    RvSdpCommonFields *commF;

    if (*(RvUint32*)obj == RV_SDP_MESSAGE_MAGIC_NUMBER)
    {
        msg = (RvSdpMsg*) obj;
        commF = &msg->iCommonFields;
    }
    else
    {
        media = (RvSdpMediaDescr*) obj;
        msg = media->iSdpMsg;
        commF = &media->iCommonFields;
    }
    conn = rvSdpConnectionConstruct2(msg,NULL,type,addr_type,addr,badSyn,NULL,RV_FALSE);
    if (!conn)
        return NULL;
    rvSdpListTailAdd(&commF->iConnectionList,conn);
    return conn;
}

/***************************************************************************
 * rvSdpAddBandwidth2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the bandwidth to the message or media.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpBandwidth object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      obj - the pointer to RvSdpMsg or RvSdpCommonFields object where the 
 *            addition will be performed.
 *      type - the bandwidth type name.
 *      value - the bandwith value (in Kbs).
 *      badSyn - the proprietary formatted bandwidth field or NULL if standard 
 *               bandwidth is added.
 ***************************************************************************/
RvSdpBandwidth* 
rvSdpAddBandwidth2(void* obj,const char* bwtype, int b, const char* badSyn)
{
    RvSdpBandwidth* bw;
    RvSdpMsg *msg = NULL;
    RvSdpMediaDescr *media = NULL;
    RvSdpCommonFields *commF;
    
    if (*(RvUint32*)obj == RV_SDP_MESSAGE_MAGIC_NUMBER)
    {
        msg = (RvSdpMsg*) obj;
        commF = &msg->iCommonFields;
    }
    else
    {
        media = (RvSdpMediaDescr*) obj;
        msg = media->iSdpMsg;
        commF = &media->iCommonFields;
    }
    bw = rvSdpBandwidthConstruct2(msg,NULL,bwtype,b,badSyn,NULL,RV_FALSE);
    if (!bw)
        return NULL;
    rvSdpListTailAdd(&commF->iBandwidthList,bw);
    return bw;
}


/***************************************************************************
 * rvSdpMsgSetBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new bandwidth object at the session level.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBandwidth(RvSdpMsg* msg, const char* bwtype, int b)
{
    return (rvSdpAddBandwidth2(msg,bwtype,b,NULL)) ?
                RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

/***************************************************************************
 * rvSdpMsgSetKey
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP key field.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      em - the key encryption method.
 *      key - the key value
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetKey(RvSdpMsg* msg, RvSdpEncrMethod em, const char* key)
{
    if (rvSdpKeyConstruct2(msg,&msg->iCommonFields.iKey,em,
                                    key,NULL,NULL,RV_FALSE) == NULL)
        return RV_SDPSTATUS_ALLOCFAIL;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpAddEmail2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the email to the message object.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpEmail object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - the pointer to RvSdpMsg object where the 
 *            addition will be performed.
 *      address - the email address.
 *      text - optional email text.
 *      badSyn - the proprietary formatted email field or NULL if standard 
 *               email is added.
 ***************************************************************************/
RvSdpEmail* rvSdpMsgAddEmail2(RvSdpMsg* msg, const char* email_addr, 
                              const char* string, const char* badSyn)
{
    RvSdpEmail* e;
   
    e = rvSdpEmailConstruct2(msg,NULL,email_addr,string,badSyn,NULL,RV_FALSE);
    if (!e)
        return NULL;
    rvSdpListTailAdd(&msg->iEmailList,e);
    return e;
}    

/***************************************************************************
 * rvSdpAddPhone2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the phone to the message object.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpPhone object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - the pointer to RvSdpMsg object where the 
 *            addition will be performed.
 *      number - the phone number.
 *      text - optional phone text.
 *      badSyn - the proprietary formatted phone field or NULL if standard 
 *               phone is added.
 ***************************************************************************/
RvSdpPhone* rvSdpMsgAddPhone2(RvSdpMsg* msg, const char* phone, 
                              const char* string, const char* badSyn)
{
    RvSdpPhone* ph;
    ph = rvSdpPhoneConstruct2(msg,NULL,phone,string,badSyn,NULL,RV_FALSE);
    if (!ph)
        return NULL;
    rvSdpListTailAdd(&msg->iPhoneList,ph);
    return ph;
}

/***************************************************************************
 * rvSdpAddSessionTime2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the session time to the message.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpSessionTime object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - the pointer to RvSdpMsg object where the 
 *            addition will be performed.
 *      start - the start time of the session.
 *      end - the end time of the session.
 *      badSyn - the proprietary formatted session time field or NULL if standard 
 *               session time is added.
 ***************************************************************************/
RvSdpSessionTime* rvSdpMsgAddSessionTime2(RvSdpMsg* msg, RvUint32 start, 
                                          RvUint32 stop, const char* badSyn)
{
    RvSdpSessionTime* st;
    st = rvSdpSessionTimeConstruct2(msg,NULL,start,stop,badSyn,NULL,RV_FALSE);
    if (!st)
        return NULL;
    rvSdpListTailAdd(&msg->iSessionTimeList,st);
    return st;
}

/***************************************************************************
 * rvSdpAddRtpMap2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the RTP map atribute to the message or media.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpRtpMap object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          msg - message where addition will be performed.
 *          commF - common fields pointer of message or media.
 *          payload - an RTP dynamic payload number.
 *          encoding_name - the name of the codec.
 *          rate - the clock rate.
 *          badSyn - proprietary syntax RTP map attribute or NULL if
 *                   the  valid RTP map attribute is added.
 ***************************************************************************/
RvSdpRtpMap* rvSdpAddRtpMap2(RvSdpMsg* msg, RvSdpCommonFields* commF, int payload, 
                             const char* encoding_name, int rate, const char *badSyn)
{
    RvSdpRtpMap* rm;
    RvSdpAttribute *attr;
    rm = rvSdpRtpMapConstruct2(msg,NULL,payload,encoding_name,rate,badSyn,NULL);
    if (!rm)
        return NULL;
    if (msg)
    {        
        attr = rvSdpAddAttr2(msg,commF,&(rm->iRMAttribute),"rtpmap",NULL);                
		attr->iSpecAttrData = rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_RTP_MAP);
    }
    return rm; 
}

/***************************************************************************
 * rvSdpMsgAddMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new media descriptor object to the SDP message.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpMediaDescr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      media_type - the type of media (audio, video, data).
 *      port - the port number.
 *      protocol - the protocol used to transport the media, such as RTP/RTCP. 
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMsgAddMediaDescr(RvSdpMsg* msg, RvSdpMediaType media_type, 
                                       RvUint16 port, RvSdpProtocol protocol)
{
    RvSdpMediaDescr* md;
    md = rvSdpMediaDescrConstructEx(msg,NULL,media_type,port,protocol,NULL,NULL,RV_FALSE);
    if (!md)
        return NULL;
    rvSdpListTailAdd(&msg->iMediaDescriptors,md);
    return md;
}

/* 
 * Constructs the RvSdpMediaDescr object, copies the 'descr' instance to the
 * constructed one and adds the new RvSdpMediaDescr object to RvSdpMessage object 
 */
RvSdpMediaDescr* rvSdpMsgInsertMediaDescr2(RvSdpMsg* msg, RvSdpMediaDescr* descr)
{
    RvSdpMediaDescr *md;
    md = rvSdpMediaDescrCopyEx(NULL,descr,msg,RV_FALSE,RV_FALSE);
    if (!md)
        return NULL;
    rvSdpListTailAdd(&msg->iMediaDescriptors,md);
    return md;
}

/***************************************************************************
 * rvSdpMsgInsertMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new media descriptor object to the SDP message as the copy
 *      of 'descr'.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object where new media descriptor will 
 *            be added.
 *      descr - the new media descriptor will be copied from this one.
 ***************************************************************************/
RvSdpStatus rvSdpMsgInsertMediaDescr(RvSdpMsg* msg, RvSdpMediaDescr* descr)
{
    return rvSdpMsgInsertMediaDescr2(msg,descr) ? 
                    RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}    

/***************************************************************************
 * rvSdpMsgTimeAddZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new time zone adjustment to the list specified in 'z=' line.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpTimeZoneAdjust object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      time - the time at which the adjustment is applied.
 *      adjust_time - the time shift length.
 *      units - The units of the time shift.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpMsgTimeAddZoneAdjustment(RvSdpMsg* msg, RvUint32 time, 
                              RvInt32 adjust_time, RvSdpTimeUnit units)
{
    RvSdpTimeZoneAdjust* tza;
    tza = rvSdpTimeZoneAdjustConstruct2(msg,NULL,time,adjust_time,units,NULL,RV_FALSE);
    if (!tza)
        return NULL;
    rvSdpListTailAdd(&msg->iTZA.iTimeZoneList,tza);
    
    return tza;
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpAddOther
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the 'other' object to message or media descriptor.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpOther object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - the pointer to RvSdpMsg object.
 *      commF - the pointer to RvSdpCommonFields of message or media descriptor 
 *              where 'other' object has to be added.
 *      tag - the tag letter of the 'other' object.
 *      value - the value (after '=' sign) of the 'other' object.
 ***************************************************************************/
RvSdpOther* rvSdpAddOther(RvSdpMsg* msg, RvSdpCommonFields* commF, 
                          const char tag, const char *value)
{
    RvSdpOther* oth;
    oth = rvSdpOtherConstruct2(msg,NULL,tag,value,NULL,RV_FALSE);
    if (!oth)
        return NULL;
    rvSdpListTailAdd(&commF->iOtherList,oth);
    return oth;
}

/***************************************************************************
 * rvSdpMsgSetBadSyntaxOrigin
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP origin field with a proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code 
 *      if the function fails.
 * -------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      origin - The proprietary formatted origin to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBadSyntaxOrigin(RvSdpMsg* msg, const char* origin)
{
    if (rvSdpOriginConstruct2(msg,&msg->iOrigin,NULL,NULL,NULL,RV_SDPNETTYPE_NOTSET,
                                RV_SDPADDRTYPE_NOTSET,NULL,origin,NULL,RV_FALSE) == NULL)
        return RV_SDPSTATUS_ALLOCFAIL;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgSetBadSyntaxConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP connection field with a proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code  
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted connection to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBadSyntaxConnection (RvSdpMsg* msg, const char* connection)
{
    return (rvSdpAddConnection2(msg,RV_SDPNETTYPE_NOTSET,RV_SDPADDRTYPE_NOTSET,
                        NULL,connection)) ? RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

/***************************************************************************
 * rvSdpMsgSetBadSyntaxBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP bandwidth field with a proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted bandwidth to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBadSyntaxBandwidth(RvSdpMsg* msg, const char* bandwidth)
{
    return (rvSdpAddBandwidth2(msg,NULL,0,bandwidth)) 
                        ? RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

/***************************************************************************
 * rvSdpMsgSetBadSyntaxKey
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP key field with a proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted key to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBadSyntaxKey(RvSdpMsg* msg, const char* key)
{
    if (rvSdpKeyConstruct2(msg,&msg->iCommonFields.iKey,RV_SDPENCRMTHD_NOTSET,
                                                    NULL,key,NULL,RV_FALSE) == NULL)
        return RV_SDPSTATUS_ALLOCFAIL;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgAddBadSyntaxMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpMediaDescr object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpMediaDescr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of media descriptor field
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMsgAddBadSyntaxMediaDescr(RvSdpMsg* msg, const char *media)
{
    RvSdpMediaDescr* md;
    md = rvSdpMediaDescrConstructEx(msg,NULL,RV_SDPMEDIATYPE_NOTSET,0,
                                    RV_SDPPROTOCOL_NOTSET,media,NULL,RV_FALSE);
    if (!md)
        return NULL;
    rvSdpListTailAdd(&msg->iMediaDescriptors,md);
    return md;
}

/*
 *	Tests whether the object is proprietary formatted (bad syntax).
 */
RvBool rvSdpLineObjectIsBadSyntax(RvSdpLineObject *lo)
{
    char *p = (char*)lo - lo->iOffset;
	int offs;
    
    if (lo->iFieldType == SDP_FIELDTYPE_ORIGIN)
    {
        if (((RvSdpOrigin*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_URI)
    {
        if (((RvSdpUri*)p)->iUriBadSyntax)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_EMAIL)
    {
        if (((RvSdpEmail*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_PHONE)
    {
        if (((RvSdpPhone*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_CONNECTION)
    {
        if (((RvSdpConnection*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_BANDWIDTH)
    {
        if (((RvSdpBandwidth*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_TIME)
    {
        if (((RvSdpSessionTime*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_REPEAT)
    {
        if (((RvSdpRepeatInterval*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_TIME_ZONE_ADJUST)
    {
        if (((RvSdpTZA*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_KEY)
    {
        if (((RvSdpKey*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_MEDIA)
    {
        if (((RvSdpMediaDescr*)p)->iBadSyntaxField)
            return RV_TRUE;       
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_ATTRIBUTE)
    {
        RvSdpAttribute *attr = (RvSdpAttribute*) p;
		if (attr->iSpecAttrData == NULL || attr->iSpecAttrData->iBadValueOffset < 0)
			return RV_FALSE;

		offs = attr->iSpecAttrData->iBadValueOffset;
		if (*(char**)((char*)attr+offs))
			return RV_TRUE;
		else
			return RV_FALSE;
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_UNKNOWN_TAG)
        return RV_TRUE;       
    return RV_FALSE;
}
/*
void rvSdpDestroyBadSyntax(RvSdpMsg* msg, RvSdpBadSyntax* bs)
{
    RvSdpLineObject *lo = (RvSdpLineObject*) bs;
    char *p = (char*)lo - lo->iOffset;

    switch(lo->iFieldType) 
    {
    case SDP_FIELDTYPE_ORIGIN:
        rvSdpOriginDestruct((RvSdpOrigin*)p);
    	break;
    case SDP_FIELDTYPE_URI:
        rvSdpMsgDestroyUri(msg);
    	break;
    case SDP_FIELDTYPE_EMAIL:
        rvSdpEmailDestruct((RvSdpEmail*)p);
        break;
    case SDP_FIELDTYPE_PHONE:
        rvSdpPhoneDestruct((RvSdpPhone*)p);
        break;
    default:
        return;
    }

void rvSdpMsgRemoveBadSyntax(RvSdpMsg* msg, RvSize_t index)
{
    RvSdpLineObjIter li;
    RvSdpBadSyntax *bs;
    for (bs = rvSdpMsgGetFirstBadSyntax(msg,&li); bs; bs = rvSdpMsgGetNextBadSyntax(&li))
    {
        if (index == 0)
        {
            rvSdpDestroyBadSyntax(msg,msg->iCommonFields,bs);
            return;
        }
        index--;
    }
}
*/

/***************************************************************************
 * rvSdpMsgGetFirstBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first bad syntax object defined in the SDP message. Also sets
 *      the list iterator for the further use. 
 *      The bad syntax objects owned by media descriptors are not treated by 
 *      the function.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBadSyntax object or the NULL pointer if there not
 *      bad syntax objects defined for the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpBadSyntax* rvSdpMsgGetFirstBadSyntax(RvSdpMsg* msg2, RvSdpLineObjIter* iter)
{
    RvSdpLineObject *lo;
    RvSdpMediaDescr *md;
    RvSdpListIter iter1;
    RvSdpMsg* msg;

    if (msg2)
    {
        msg = msg2;
        lo = msg->iHeadLO;
    }
    else
    {
        lo = iter->iCurrentLO;
        msg = iter->iMsg;
    }

        
    for (; lo; lo = lo->iNext)
    {
        if (!rvSdpLineObjectIsBadSyntax(lo))
            continue;
        /* is bad syntax test whether belongs to some media */
        for (md = rvSdpMsgGetFirstMediaDescr(msg,&iter1); md; 
                                md = rvSdpMsgGetNextMediaDescr(&iter1))
        {
               if (rvSdpLineObjectBelongsToMediaAndBad(lo,md))
                   break;
        }
        if (md)
            continue;
        iter->iCurrentLO = lo->iNext;
        iter->iMsg = msg;
        return (RvSdpBadSyntax*) lo;        
    }
    return NULL;
}

/***************************************************************************
 * rvSdpMsgGetNextBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next bad syntax object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *      The bad syntax objects owned by media descriptors are not treated by 
 *      the function.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBadSyntax object or the NULL pointer if there is no
 *      more bad syntax objects defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpBadSyntax* rvSdpMsgGetNextBadSyntax(RvSdpLineObjIter* iter)
{
    return rvSdpMsgGetFirstBadSyntax(NULL,iter);
}

/***************************************************************************
 * rvSdpMsgGetNumOfBadSyntax2
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of proprietary formatted elements (of all types) in the
 *      SDP message including bad syntax elements set in the context of media
 *      descriptors.
 *           
 * Return Value: 
 *      Number of bad syntax objects of the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfBadSyntax2(const RvSdpMsg* msg)
{
    RvSize_t cnt = 0;
    RvSdpLineObject* lo;
    
    for (lo = msg->iHeadLO; lo; lo = lo->iNext)
        if (rvSdpLineObjectIsBadSyntax(lo))
            cnt++;
    return cnt;
}


 /***************************************************************************
 * rvSdpMsgGetNumOfBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of proprietary formatted elements (of all types) in the
 *      SDP message not-including bad syntax elements set in the context of media
 *      descriptors. If some of RvSdpMediaDescr object is proprietary formatted
 *      it is counted but none of bad syntax objects owned by this media descriptor
 *      is counted.
 *          
 * Return Value: 
 *      Number of bad syntax objects of the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfBadSyntax(const RvSdpMsg* msg)
{
    RvSdpLineObjIter iter;
    RvSize_t cnt = 0;
    RvSdpBadSyntax *bad;
    
    for (bad = rvSdpMsgGetFirstBadSyntax((RvSdpMsg*)msg,&iter); bad; 
                                        bad = rvSdpMsgGetNextBadSyntax(&iter))
        cnt++;
    return cnt;
}

/***************************************************************************
* rvSdpMsgGetBadSyntax
* ------------------------------------------------------------------------
* General: 
*      Gets a bad syntax object by index. 
*      The bad syntax objects owned by media descriptors are not treated by 
*      the function.
*          
* Return Value: 
*      The requested RvSdpBadSyntax object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfBadSyntax() call. 
***************************************************************************/
RvSdpBadSyntax* rvSdpMsgGetBadSyntax(const RvSdpMsg* msg, RvSize_t index)
{
    RvSdpLineObjIter iter;
    RvSize_t cnt = index;
    RvSdpBadSyntax *bad;
    RV_UNUSED_ARG(index)
    
    for (bad = rvSdpMsgGetFirstBadSyntax((RvSdpMsg*)msg,&iter); bad; 
                                        bad = rvSdpMsgGetNextBadSyntax(&iter))
    {
        if (cnt == 0)
            return bad;
        cnt--;
    }
    return NULL;
}

#endif /*#if defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/***************************************************************************
 * rvSdpAddAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the generic atribute to the message or media.
 *      If the 'attr' is not NULL it must point to constructed RvSdpAttribute
 *      whose name and value will be set. Otherwise the attribute will be 
 *      constructed.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpAttribute object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          msg - message where addition will be performed.
 *          commF - common fields pointer of message or media.
 *          attr - the pointer to constructed RvSdpAttribute or NULL.
 *          name - the generic attribute name.
 *          value - the generic attribute value.
 ***************************************************************************/
RvSdpAttribute* rvSdpAddAttr2(RvSdpMsg* msg, RvSdpCommonFields *commF, 
                              RvSdpAttribute* attr, const char* name, const char* value)
{
    attr = rvSdpAttributeConstruct2(msg,attr,name,value,NULL,RV_FALSE);
    if (!attr)
        return NULL;
    rvSdpListTailAdd(&commF->iAttrList,attr);

    return attr;
}

/***************************************************************************
 * rvSdpMsgSetVersionN
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the version field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      version - the new version value.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetVersionN(RvSdpMsg* msg, const char* version)
{
    return rvSdpMsgSetVersion2(msg,version);
}
    
/***************************************************************************
 * rvSdpMsgSetVersion2
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the version field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      version - the new version value.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetVersion2(RvSdpMsg* msg, const char* version)
{
    rvSdpUnsetTextField(&msg->iVersion.iVersionTxt,msg);
    if (rvSdpSetTextField(&msg->iVersion.iVersionTxt,msg,version) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
    rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_VERSION,&msg->iVersion.iLineObj,
        RV_OFFSETOF(RvSdpVersion,iLineObj));
	msg->iVersion.iSetByMsgConstruct = RV_FALSE;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgSetVersionN
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the version field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      version - the new version value.
 ***************************************************************************/
void rvSdpMsgDestroyVersion(RvSdpMsg* msg)
{
    rvSdpUnsetTextField(&msg->iVersion.iVersionTxt,msg);
    rvSdpLineObjsListRemove(msg,&msg->iVersion.iLineObj);
}


/***************************************************************************
 * rvSdpMsgSetSessionName
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session name field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      session_name - the new session name value.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetSessionName(RvSdpMsg* msg, const char* session_name)
{
    rvSdpUnsetTextField(&msg->iSessId.iSessIdTxt,msg);
    if (rvSdpSetTextField(&msg->iSessId.iSessIdTxt,msg,session_name) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
    rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_SESSION_ID,&msg->iSessId.iLineObj,
                    RV_OFFSETOF(RvSdpSessId,iLineObj));        
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgDestroySessionName
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the session name field of the SDP message.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgDestroySessionName(RvSdpMsg* msg)
{
    rvSdpUnsetTextField(&msg->iSessId.iSessIdTxt,msg);
    rvSdpLineObjsListRemove(msg,&msg->iSessId.iLineObj);
}

/*
 *	Sets the URI fields of the message.
 *  Returns RV_SDPSTATUS_OK on success or RV_SDPSTATUS_ALLOCFAIL if fails.
 */
RvSdpStatus rvSdpMsgFillURI(RvSdpMsg* msg, const char* uri, RvBool badSyntax)
{
    RV_UNUSED_ARG(badSyntax);
    rvSdpUnsetTextField(&msg->iUri.iUriTxt,msg);
    if (rvSdpSetTextField(&msg->iUri.iUriTxt,msg,uri) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
	
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    msg->iUri.iUriBadSyntax = badSyntax;
#endif
    return RV_SDPSTATUS_OK;
}	

/***************************************************************************
 * rvSdpMsgSetURI2
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the URI field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      uri - the new URI value.
 *      badSyntax - whether the URI value is proprietary formatted.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetURI2(RvSdpMsg* msg, const char* uri, RvBool badSyntax)
{
	RvSdpStatus ret;

	ret = rvSdpMsgFillURI(msg,uri,badSyntax);
	if (ret != RV_SDPSTATUS_OK)
		return ret;

    rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_URI,&msg->iUri.iLineObj,
        RV_OFFSETOF(RvSdpUri,iLineObj));    

    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgDestroyUri
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the URI field of the SDP message.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgDestroyUri(RvSdpMsg* msg)
{
#if defined(RV_SDP_CHECK_BAD_SYNTAX)    
    msg->iUri.iUriBadSyntax = RV_FALSE;
#endif
    rvSdpUnsetTextField(&msg->iUri.iUriTxt,msg);

    rvSdpLineObjsListRemove(msg,&msg->iUri.iLineObj);
}

/***************************************************************************
 * rvSdpMsgListConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the list of SDP messages.
 *          
 * Return Value: 
 *      The constructed RvSdpMsgList object or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - a pointer to valid RvSdpMsgList object.
 ***************************************************************************/
RvSdpMsgList* rvSdpMsgListConstruct(RvSdpMsgList* msgList)
{
    return rvSdpMsgListConstructA(msgList,NULL);
}

/***************************************************************************
 * rvSdpMsgListConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the list of SDP messages using user provided allocator.
 *          
 * Return Value: 
 *      The constructed RvSdpMsgList object or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - a pointer to valid RvSdpMsgList object.
 *      a - allocator to be used.
 ***************************************************************************/
RvSdpMsgList* rvSdpMsgListConstructA(RvSdpMsgList* msgList, RvAlloc* a)
{
    if (a == NULL)
        a = rvSdpGetDefaultAllocator();
    msgList->iAllocator = a;
    rvSdpListInitialize(&msgList->iList,RV_OFFSETOF(RvSdpMsg,iNextMessage),
        (rvListNodeDestructFunc)rvSdpMsgDestruct);
    return msgList;
}

/***************************************************************************
 * rvSdpMsgListConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the list of SDP messages 'dest' and copies all SDP messages 
 *      contained in 'src' to 'dest'. 
 *          
 * Return Value: 
 *      The constructed RvSdpMsgList object 'dest' or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      d - the destination RvSdpMsgList object.
 *      s - the source RvSdpMsgList object.
 *      a - allocator to be used.
 ***************************************************************************/
RvSdpMsgList* rvSdpMsgListConstructCopyA(RvSdpMsgList* d, const RvSdpMsgList* s, 
                                         RvAlloc* a)
{
    rvSdpMsgListConstructA(d,a);
    return rvSdpMsgListCopy(d,s);
}

/***************************************************************************
 * rvSdpMsgListDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs all SDP messages contained in msgList.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
void rvSdpMsgListDestruct(RvSdpMsgList* msgList)
{
    rvSdpListClear(&msgList->iList);
}

/***************************************************************************
 * rvSdpMsgListCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies all SDP messages contained in 'src' to 'dest'. 
 *          
 * Return Value: 
 *      The RvSdpMsgList object 'dest' or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      d - the destination RvSdpMsgList object.
 *      s - the source RvSdpMsgList object.
 ***************************************************************************/
RvSdpMsgList* rvSdpMsgListCopy(RvSdpMsgList* d, const RvSdpMsgList* s)
{
    if (rvSdpListCopy(&d->iList,(RvSdpList*)&s->iList,
                (rvSdpListCopyFunc)rvSdpMsgConstructCopyA,d->iAllocator)
                                                        != RV_SDPSTATUS_OK)
    {
        rvSdpListClear(&d->iList);
        return NULL;        
    }
    return d;
}

/***************************************************************************
 * rvSdpMsgListGetSize
 * ------------------------------------------------------------------------
 * General: 
 *          
 * Return Value: 
 *      Returns the number of SDP messages in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RvSize_t rvSdpMsgListGetSize(const RvSdpMsgList* msgList)
{
    return msgList->iList.iListSize;
}

/***************************************************************************
 * rvSdpMsgListGetElement
 * ------------------------------------------------------------------------
 * General: 
 *          
 * Return Value: 
 *      Returns the SDP message with index 'i' contained in the list msgList.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RvSdpMsg * rvSdpMsgListGetElement(RvSdpMsgList* msgList, RvSize_t i)
{
    return (RvSdpMsg*) rvSdpListGetByIndex(&msgList->iList,i);
}

/***************************************************************************
 * rvSdpMsgListGetFirstMsg
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first SDP message in the messages list. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpMsg  object or the NULL pointer if there are no
 *      messages in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - a pointer to the RvSdpMsgList object.
 *      li - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMsgListGetNextMsg calls.
 ***************************************************************************/
RvSdpMsg * rvSdpMsgListGetFirstMsg(RvSdpMsgList* msgList, RvSdpListIter* li)
{
    return (RvSdpMsg*) rvSdpListGetFirst(&msgList->iList,li);
}

/***************************************************************************
 * rvSdpMsgListGetNextMsg
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next RvSdpMsg object from the messages list. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpMsg object or the NULL pointer if there is no
 *      more messages in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      li - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMsgList(GetFirst/Next)Msg function. 
 ***************************************************************************/
RvSdpMsg * rvSdpMsgListGetNextMsg(RvSdpListIter* li)
{
    return (RvSdpMsg*) rvSdpListGetNext(li);
}

/***************************************************************************
 * rvSdpMsgListRemoveCurrentMsg
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the message object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgListRemoveCurrentMsg(RvSdpListIter* li)
{
    rvSdpListRemoveCurrent(li);
}


/***************************************************************************
 * rvSdpMsgListRemoveElement
 * ------------------------------------------------------------------------
 * General: 
 *      Removes and destructs the message object contained in the list by index.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 *      i - the index of the message to remove.
 ***************************************************************************/
void rvSdpMsgListRemoveElement(RvSdpMsgList* msgList, RvSize_t i)
{
    rvSdpListRemoveByIndex(&msgList->iList,i);
}

/***************************************************************************
 * rvSdpMsgListClear
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all RvSdpMsg objects set in messages list.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
void rvSdpMsgListClear(RvSdpMsgList* msgList)
{
    rvSdpListClear(&msgList->iList);
}

/***************************************************************************
 * rvSdpMsgListAddMsg
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs new SDP message and adds it to the list.
 *
 * Return Value: 
 *      Pointer to the constructed/added RvSdpMsg object or NULL if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RvSdpMsg* rvSdpMsgListAddMsg(RvSdpMsgList* msgList)
{
    RvSdpMsg* msg;
    msg = rvSdpMsgConstruct2(NULL,msgList->iAllocator);
    if (!msg)
        return NULL;
    if (rvSdpMsgListInsertMsg(msgList,msg) != RV_SDPSTATUS_OK)
    {
        rvSdpMsgDestruct(msg);
        return NULL;
    }
    return msg;
}

/***************************************************************************
 * rvSdpMsgListInsertMsg
 * ------------------------------------------------------------------------
 * General: 
 *      Appends the the SDP message to the list.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 *      msg - pointer to constructed RvSdpMsg object.
 ***************************************************************************/
RvSdpStatus rvSdpMsgListInsertMsg(RvSdpMsgList* msgList, const RvSdpMsg* msg)
{
    rvSdpListTailAdd(&msgList->iList,(void*)msg);
    return RV_SDPSTATUS_OK;
}

/* to remove the line object from the list */
void rvSdpLineObjsListRemove(RvSdpMsg* msg, RvSdpLineObject *lo)
{
    if (lo->iUsed == 0)
        return;

    if (*(RvUint32*)msg != RV_SDP_MESSAGE_MAGIC_NUMBER)
    /* this is not RvSdpMessage*/
        return;

    if (!msg->iHeadLO)
        return;
    if (msg->iTailLO == msg->iHeadLO)
    {
        if (msg->iTailLO != lo)
            return;
        msg->iTailLO = msg->iHeadLO = NULL;
    }
    else if (msg->iHeadLO == lo)
    {
        msg->iHeadLO = msg->iHeadLO->iNext;
        msg->iHeadLO->iPrev = NULL;
    }
    else if (msg->iTailLO == lo)
    {
        msg->iTailLO = msg->iTailLO->iPrev;
        msg->iTailLO->iNext = NULL;
    }
    else
    {
        lo->iPrev->iNext = lo->iNext;
        lo->iNext->iPrev = lo->iPrev;
    }
    lo->iNext = lo->iPrev = NULL;
    lo->iUsed = 0;
    msg->iLOCnt--;
}

/* to insert the line object within the list of line objects */
void rvSdpLineObjsListInsert(RvSdpMsg* msg, RvSdpFieldTypes tag, 
                             RvSdpLineObject *lo, int offs)
{
    if (lo->iUsed)
        rvSdpLineObjsListRemove(msg,lo);
    lo->iFieldType = tag;
    lo->iOffset = (RvUint16) offs;
    lo->iNext = lo->iPrev = NULL;
    if (!msg->iHeadLO)
        msg->iHeadLO = msg->iTailLO = lo;
    else if (msg->iHeadLO == msg->iTailLO)
    {
        msg->iTailLO = lo;
        msg->iTailLO->iPrev = msg->iHeadLO;
        msg->iHeadLO->iNext = msg->iTailLO;
    }
    else
    {
        msg->iTailLO->iNext = lo;
        lo->iPrev = msg->iTailLO;
        msg->iTailLO = lo;
    }
    msg->iLOCnt++;
    lo->iUsed = 1;
}

#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpMsgSetURI
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the URI field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      uri - the new URI value.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetURI(RvSdpMsg* msg,const char* uri)
{
    return rvSdpMsgSetURI2(msg,uri,RV_FALSE);
}

/***************************************************************************
 * rvSdpMsgCopySdpVersion
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the version field of 'dstMsg' SDP message as of 'src'.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dstMsg - a pointer to the RvSdpMsg object.
 *      src - the source SDP version.
 ***************************************************************************/
RvSdpStatus rvSdpMsgCopySdpVersion(const RvSdpVersion* src, RvSdpMsg* dstMsg)
{
    return rvSdpMsgSetVersion2(dstMsg,src->iVersionTxt);
}

/***************************************************************************
 * rvSdpMsgCopySdpSessionId
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session name field of 'dstMsg' SDP message as of 'src'.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dstMsg - a pointer to the RvSdpMsg object.
 *      src - the source SDP session name.
 ***************************************************************************/
RvSdpStatus rvSdpMsgCopySdpSessionId(const RvSdpSessId* src, RvSdpMsg* dstMsg)
{
    return rvSdpMsgSetSessionName(dstMsg,src->iSessIdTxt);
}

/***************************************************************************
 * rvSdpMsgCopyURI
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the URI field of 'dstMsg' SDP message as of 'src'.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dstMsg - a pointer to the RvSdpMsg object.
 *      src - the source SDP URI.
 ***************************************************************************/
RvSdpStatus rvSdpMsgCopyURI(const RvSdpUri* src, RvSdpMsg* dstMsg)
{
    return rvSdpMsgSetURI2(dstMsg,src->iUriTxt,
            RV_SDP_BAD_SYNTAX_PARAM(src->iUriBadSyntax));
}

/***************************************************************************
 * rvSdpMsgAddConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new connection object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpConnection object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP 
 *             address for an IP network, and so on.
 ***************************************************************************/
RvSdpConnection* rvSdpMsgAddConnection(RvSdpMsg* msg, RvSdpNetType type,
                                       RvSdpAddrType addr_type, const char* addr)
{
    return rvSdpAddConnection2(msg,type,addr_type,addr,NULL);
}

/***************************************************************************
 * rvSdpMsgAddBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new bandwidth object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpBandwidth object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - Bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
RvSdpBandwidth* rvSdpMsgAddBandwidth(RvSdpMsg* msg, const char* bwtype, int b)
{
    return rvSdpAddBandwidth2(msg,bwtype,b,NULL);
}

/***************************************************************************
 * rvSdpMsgAddEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new RvSdpEmail object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpEmail object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      email_addr - the new email address.
 *      string - Optional free text. Set to "" (string of zero (0) length) 
 *               if not required.
 ***************************************************************************/
RvSdpEmail* rvSdpMsgAddEmail(RvSdpMsg* msg, const char* email_addr, const char* string)
{
    return rvSdpMsgAddEmail2(msg,email_addr,string,NULL);
}

/***************************************************************************
 * rvSdpMsgAddPhone
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new phone object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpPhone object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      phone - the new phone number.
 *      string - Optional free text. Set to "" (string of zero (0) length) 
 *               if not required.
 ***************************************************************************/
RvSdpPhone* rvSdpMsgAddPhone(RvSdpMsg* msg, const char* phone, const char* string)
{
    return rvSdpMsgAddPhone2(msg,phone,string,NULL);
}

/***************************************************************************
 * rvSdpMsgAddSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new session time object.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpSessionTime object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      start - the start time of the SDP session.
 *      stop - the end time of the SDP session.
 ***************************************************************************/
RvSdpSessionTime* rvSdpMsgAddSessionTime(RvSdpMsg* msg, RvUint32 start, RvUint32 stop)
{
    return rvSdpMsgAddSessionTime2(msg,start,stop,NULL);
}

/***************************************************************************
 * rvSdpMsgAddAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new generic attribute object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpAttribute object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      name - the generic attribute name.
 *      value - the generic attribute value.
 ***************************************************************************/
RvSdpAttribute* rvSdpMsgAddAttr(RvSdpMsg* msg, const char* name, const char* value)
{
    return rvSdpAddAttr2(msg,&msg->iCommonFields,NULL,name,value);
}

/***************************************************************************
 * rvSdpMsgSetSessionInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the information field of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      info - the new information value.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetSessionInformation(RvSdpMsg* msg, const char* info)
{
    return rvSdpSetSdpInformation(&msg->iCommonFields.iInfo,info,msg);
}

/***************************************************************************
 * rvSdpMsgDestroyInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the information field of the SDP message.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgDestroyInformation(RvSdpMsg* msg)
{
    rvSdpDestroySdpInformation(&msg->iCommonFields.iInfo,msg);
}

/***************************************************************************
 * rvSdpMsgAddRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Adds a new RTP map to the session-level RTP map list.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpRtpMap object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      payload - an RTP dynamic payload number.
 *      encoding_name - the name of the codec.
 *      rate - the clock rate.
 ***************************************************************************/
RvSdpRtpMap* rvSdpMsgAddRtpMap(RvSdpMsg* msg, int payload, const char* encoding_name, 
                               int rate)
{
    return rvSdpAddRtpMap2(msg,&msg->iCommonFields,payload,encoding_name,rate,NULL);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the RTP map special attribute object pointed 
 *      by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentRtpMap(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}
/***************************************************************************
 * rvSdpMsgRemoveRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the RTP map special attribute object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfRtpMaps call. 
 ***************************************************************************/
void rvSdpMsgRemoveRtpMap(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpRemoveSpecialAttr(&msg->iCommonFields,index,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMsgClearRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all RTP map special attributes set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearRtpMap(RvSdpMsg* msg)
{
    rvSdpClearSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMsgGetFirstRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first RTP map object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRtpMap object or the NULL pointer if there are no
 *      RTP maps defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpRtpMap* rvSdpMsgGetFirstRtpMap(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpRtpMap*)rvSdpGetFirstSpecialAttr(&msg->iCommonFields,
        iter,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMsgGetNextRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next RTP map object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRtpMap object or the NULL pointer if there are no
 *      more RTP maps defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpRtpMap* rvSdpMsgGetNextRtpMap(RvSdpListIter* iter)
{
    return (RvSdpRtpMap*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
* rvSdpMsgGetRtpMap
* ------------------------------------------------------------------------
* General: 
*      Gets an RTP map special attribute object by index. 
*          
* Return Value: 
*      The requested RTP map attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfRtpMaps() call. 
***************************************************************************/
RvSdpRtpMap* rvSdpMsgGetRtpMap(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpRtpMap*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&msg->iCommonFields,
        index,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMsgGetNumOfRtpMaps
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of RTP map special attributes set in SDP message context.
 *          
 * Return Value: 
 *      Number of RTP map special attributes set in SDP message context.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfRtpMap(const RvSdpMsg* msg)
{
    return rvSdpGetNumOfSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_RTP_MAP);
}

#ifdef RV_SDP_CHECK_BAD_SYNTAX

/***************************************************************************
 * rvSdpMsgAddBadSyntaxRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpRtpMap object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpRtpMap object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of RTP map special attribute.
 ***************************************************************************/
RvSdpRtpMap* rvSdpMsgAddBadSyntaxRtpMap(RvSdpMsg* msg, const char* rtpmap)
{
    return rvSdpAddRtpMap2(msg,&msg->iCommonFields,0,NULL,0,rtpmap);
}

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

#ifdef RV_SDP_KEY_MGMT_ATTR

/***************************************************************************
 * rvSdpMsgAddKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new key management special attribute at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      prtclId - the protocol ID.
 *      keyData - the encryption key data.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMsgAddKeyMgmt(RvSdpMsg* msg, const char* prtclId, 
                                     const char* keyData)
{
    return rvSdpAddKeyMgmt2(msg,&msg->iCommonFields,prtclId,keyData,NULL);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the key-mgmt special attribute object pointed 
 *      by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentKeyMgmt(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the key-mgmt special attribute object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfKeyMgmt call. 
 ***************************************************************************/
void rvSdpMsgRemoveKeyMgmt(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpRemoveSpecialAttr(&msg->iCommonFields,index,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMsgClearKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all key-mgmt special attributes set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearKeyMgmt(RvSdpMsg* msg)
{
    rvSdpClearSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMsgGetFirstKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first key management object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpKeyMgmt object or the NULL pointer if there are no
 *      key management special attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMsgGetFirstKeyMgmt(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpKeyMgmtAttr*)rvSdpGetFirstSpecialAttr(&msg->iCommonFields,
        iter,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMsgGetNextKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next key management special attribute object defined in the SDP
 *      message. The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpKeyMgmtAttr object or the NULL pointer if there is no
 *      more key management special attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMsgGetNextKeyMgmt(RvSdpListIter* iter)
{
    return (RvSdpKeyMgmtAttr*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
* rvSdpMsgGetKeyMgmt
* ------------------------------------------------------------------------
* General: 
*      Gets a key management attribute object by index. 
*          
* Return Value: 
*      The requested key management attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfKeyMgmt() call. 
***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMsgGetKeyMgmt(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpKeyMgmtAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&msg->iCommonFields,
        index,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMsgGetNumOfKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of key mgmt special attributes set in SDP message context.
 *          
 * Return Value: 
 *      Number of key mgmt special attributes set in SDP message context.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfKeyMgmt(const RvSdpMsg* msg)
{
    return rvSdpGetNumOfSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_KEY_MGMT);
}

#ifdef RV_SDP_CHECK_BAD_SYNTAX

/***************************************************************************
 * rvSdpMsgAddBadSyntaxKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpKeyMgmtAttr object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of KeyMgmt special attribute field.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMsgAddBadSyntaxKeyMgmt(RvSdpMsg* msg, const char* keyMgmt)
{
    return rvSdpAddKeyMgmt2(msg,&msg->iCommonFields,NULL,NULL,keyMgmt);
}

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

#endif /*RV_SDP_KEY_MGMT_ATTR*/

#ifdef RV_SDP_CRYPTO_ATTR

/***************************************************************************
 * rvSdpMsgAddCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new crypto special attribute object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      tag - the crypto attribute tag number.
 *      suite - the crypto attribute suite value.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMsgAddCrypto(RvSdpMsg* msg, RvUint tag, const char* suite)
{
    return rvSdpAddCrypto2(msg,&msg->iCommonFields,tag,suite,NULL);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the crypto special attribute object pointed 
 *      by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentCrypto(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}
/***************************************************************************
 * rvSdpMsgRemoveCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the crypto special attribute object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfCrypto() call. 
 ***************************************************************************/
void rvSdpMsgRemoveCrypto(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpRemoveSpecialAttr(&msg->iCommonFields,index,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMsgClearCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all crypto special attributes set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearCrypto(RvSdpMsg* msg)
{
    rvSdpClearSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMsgGetFirstCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first crypto attribute object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpCryptoAttr object or the NULL pointer if there are no
 *      crypto special attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMsgGetFirstCrypto(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpCryptoAttr*)rvSdpGetFirstSpecialAttr(&msg->iCommonFields,
        iter,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMsgGetNextCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next crypto attribute object defined in the SDP message. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpCryptoAttr object or the NULL pointer if there are no
 *      more crypto special attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMsgGetNextCrypto(RvSdpListIter* iter)
{
    return (RvSdpCryptoAttr*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
* rvSdpMsgGetCrypto
* ------------------------------------------------------------------------
* General: 
*      Gets a crypto attribute object by index. 
*          
* Return Value: 
*      The requested crypto attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfCrypto() call. 
***************************************************************************/
RvSdpCryptoAttr* rvSdpMsgGetCrypto(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpCryptoAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&msg->iCommonFields,
        index,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
* rvSdpMsgGetCrypto
* ------------------------------------------------------------------------
* General: 
*      Gets a crypto attribute object by index. 
*          
* Return Value: 
*      The requested crypto attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfCrypto() call. 
***************************************************************************/
RvSize_t rvSdpMsgGetNumOfCrypto(const RvSdpMsg* msg)
{
    return rvSdpGetNumOfSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_CRYPTO);
}

#ifdef RV_SDP_CHECK_BAD_SYNTAX

/***************************************************************************
 * rvSdpMsgAddBadSyntaxCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpCryptoAttr object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of crypto special attribute field.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMsgAddBadSyntaxCrypto(RvSdpMsg* msg, const char* crypto)
{
    return rvSdpAddCrypto2(msg,&msg->iCommonFields,0,NULL,crypto);
}

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

#endif /*RV_SDP_CRYPTO_ATTR*/

/***************************************************************************
 * RvSdpMsgConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the instance of RvSdpMsg, initializes all internal fields,
 *      allocates memory for the strings buffer and pools of reusable objects.
 *          
 * Return Value: 
 *      Valid RvSdpMsg pointer on success or NULL on failure
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg instance to be constructed, if the value is 
 *            NULL the instance will be allocated within the function.
 ***************************************************************************/
RvSdpMsg* rvSdpMsgConstruct(RvSdpMsg* msg)
{
    return rvSdpMsgConstruct2(msg,NULL);
}

/***************************************************************************
 * RvSdpMsgConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the instance of RvSdpMsg, initializes all internal fields,
 *      allocates memory for the strings buffer and pools of reusable objects 
 *          
 * Return Value: 
 *      Valid RvSdpMsg pointer on success or NULL on failure
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg instance to be constructed, if the value is 
 *            NULL the instance will be allocated within the function.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMsg* rvSdpMsgConstructA(RvSdpMsg* msg, RvAlloc* a)
{
    return rvSdpMsgConstruct2(msg,a);
}

/***************************************************************************
 * rvSdpMsgConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object will be constructed. 
 *      If the destination object is NULL pointer the destination
 *      object will be allocated within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMsg object or NULL. 
 *      src - a pointer to the source object
 ***************************************************************************/
RvSdpMsg* rvSdpMsgConstructCopy(RvSdpMsg* dest, const RvSdpMsg* src)
{
    return rvSdpMsgCopy2(dest,src,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpMsgCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object must be constructed prior to function call. 
 *          
 * Return Value: 
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to constructed RvSdpMsg object. 
 *      src - a pointer to the source object
 ***************************************************************************/
RvSdpMsg* rvSdpMsgCopy(RvSdpMsg* dest, const RvSdpMsg* src)
{
    return rvSdpMsgCopy2(dest,src,NULL,RV_TRUE);
}

/***************************************************************************
 * rvSdpMsgGetVersion
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the version field value of SDP message.
 *          
 * Return Value: 
 *      Returns the version text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
const char* rvSdpMsgGetVersion(const RvSdpMsg* msg)
{
    return RV_SDP_EMPTY_STRING(msg->iVersion.iVersionTxt);
}

/***************************************************************************
 * rvSdpMsgGetOrigin
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a pointer to the origin field.
 *          
 * Return Value: 
 *      A pointer to the origin field, or NULL if the origin field is not 
 *      set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSdpOrigin* rvSdpMsgGetOrigin(const RvSdpMsg* msg)
{
    return (RV_SDP_ORIGIN_IS_USED(&msg->iOrigin) ? (RvSdpOrigin*) &msg->iOrigin : NULL);
}

/***************************************************************************
 * rvSdpMsgGetSessionName
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the session name field value of SDP message.
 *          
 * Return Value: 
 *      Returns the session name text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
const char* rvSdpMsgGetSessionName(const RvSdpMsg* msg)
{
    return RV_SDP_EMPTY_STRING(msg->iSessId.iSessIdTxt);
}

/***************************************************************************
 * rvSdpMsgGetSessionInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the session information field value of SDP message.
 *          
 * Return Value: 
 *      Returns the session information text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
const char* rvSdpMsgGetSessionInformation(const RvSdpMsg* msg)
{
    return RV_SDP_EMPTY_STRING(msg->iCommonFields.iInfo.iInfoTxt);
}

/***************************************************************************
 * rvSdpMsgGetURI
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the URI field value of SDP message.
 *          
 * Return Value: 
 *      Returns the URI text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
const char* rvSdpMsgGetURI(const RvSdpMsg* msg)
{
    return RV_SDP_EMPTY_STRING(msg->iUri.iUriTxt);
}

/***************************************************************************
 * rvSdpMsgGetConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a pointer to the first connection object set in the message.
 *          
 * Return Value: 
 *      A pointer to the connection field, or NULL if there are not connection
 *      fields set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSdpConnection* rvSdpMsgGetConnection(const RvSdpMsg* msg)
{
    return (RvSdpConnection*) rvSdpListGetByIndex(&msg->iCommonFields.iConnectionList,0);
}

/***************************************************************************
 * rvSdpMsgGetNumOfConnections
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level connections list.
 *          
 * Return Value: 
 *      Size of connections list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfConnections(const RvSdpMsg* msg)
{
    return msg->iCommonFields.iConnectionList.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetConnectionByIndex
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a connection object by index. 
 *          
 * Return Value: 
 *      The requested connection object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfConnections call. 
 ***************************************************************************/
RvSdpConnection* rvSdpMsgGetConnectionByIndex(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpConnection*) rvSdpListGetByIndex(&msg->iCommonFields.iConnectionList,
        index);
}

/***************************************************************************
 * rvSdpMsgRemoveConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the connection object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfConnections call. 
 ***************************************************************************/
void rvSdpMsgRemoveConnection(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iCommonFields.iConnectionList,index);
}

/***************************************************************************
 * rvSdpMsgGetFirstConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first connection object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpConnection object or the NULL pointer if there are no
 *      connections defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             rvSdpMsgGetNextConnection calls.
 ***************************************************************************/
RvSdpConnection* rvSdpMsgGetFirstConnection(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return rvSdpListGetFirst(&msg->iCommonFields.iConnectionList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next connection object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpConnection object or the NULL pointer if there are no
 *      more connections defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMsgGetFirstConnection/rvSdpMsgGetNextConnection function. 
 ***************************************************************************/
RvSdpConnection* rvSdpMsgGetNextConnection(RvSdpListIter* iter)
{
    return rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the connection object pointed by list iterator.
 *       The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentConnection(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMsgClearConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all connections set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearConnection(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iCommonFields.iConnectionList);
}

/***************************************************************************
 * rvSdpMsgGetBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a pointer to the first bandwidth object set in the message.
 *          
 * Return Value: 
 *      A pointer to the bandwidth field, or NULL if there are no bandwidth fields 
 *      set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSdpBandwidth* rvSdpMsgGetBandwidth(const RvSdpMsg* msg)
{
    return (RvSdpBandwidth*) rvSdpListGetByIndex(&msg->iCommonFields.iBandwidthList,0);
}

/***************************************************************************
 * rvSdpMsgGetNumOfBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level bandwiths list.
 *          
 * Return Value: 
 *      Size of bandwiths list of SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfBandwidth(const RvSdpMsg* msg)
{
    return msg->iCommonFields.iBandwidthList.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetBandwidthByIndex
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a bandwidth object by index. 
 *          
 * Return Value: 
 *      The requested bandwidth object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements in 
 *              the list is retrieved by correspondent rvSdp...GetNum..() call. 
 ***************************************************************************/
RvSdpBandwidth* rvSdpMsgGetBandwidthByIndex(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpBandwidth*) rvSdpListGetByIndex(&msg->iCommonFields.iBandwidthList,
                    index);
}

/***************************************************************************
 * rvSdpMsgRemoveBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the bandwidth object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfBandwidths call. 
 ***************************************************************************/
void rvSdpMsgRemoveBandwidth(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iCommonFields.iBandwidthList,index);
}

/***************************************************************************
 * rvSdpMsgGetFirstBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first bandwidth object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBandwidth object or the NULL pointer if there are no
 *      bandwidths defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls. 
 ***************************************************************************/
RvSdpBandwidth* rvSdpMsgGetFirstBandwidth(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return rvSdpListGetFirst(&msg->iCommonFields.iBandwidthList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next bandwidth object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBandwidth object or the NULL pointer if there are no
 *      more bandwidths defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpBandwidth* rvSdpMsgGetNextBandwidth(RvSdpListIter* iter)
{
    return rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the bandwidth object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentBandwidth(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMsgClearBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all bandwidths set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearBandwidth(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iCommonFields.iBandwidthList);
}

/***************************************************************************
 * rvSdpMsgGetKey
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a pointer to the key field.
 *          
 * Return Value: 
 *      A pointer to the key field, or NULL if the key field is not 
 *      set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSdpKey* rvSdpMsgGetKey(const RvSdpMsg* msg)
{
    return (RV_SDP_KEY_IS_USED(&msg->iCommonFields.iKey) ? 
				((RvSdpKey*) &msg->iCommonFields.iKey) : NULL);
}

/***************************************************************************
 * rvSdpMsgGetConnectionMode
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the connection mode of the SDP message or RV_SDPCONNECTMODE_NOTSET
 *      if the correspondent attribute is not set.
 *          
 * Return Value: 
 *      Returns the connection mode.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSdpConnectionMode rvSdpMsgGetConnectionMode(const RvSdpMsg* msg)
{
    return rvSdpGetConnectionMode(&msg->iCommonFields);
}

/***************************************************************************
 * rvSdpMsgSetConnectionMode
 * ------------------------------------------------------------------------
 * General: 
 *      Sets/modifies the connection mode of the SDP message.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      mode - the new value of connection mode.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetConnectionMode(RvSdpMsg* msg, RvSdpConnectionMode mode)
{
    return rvSdpSetConnectionMode(msg,&msg->iCommonFields,mode);
}

/***************************************************************************
 * rvSdpMsgGetNumOfEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level emails list.
 *          
 * Return Value: 
 *      Size of emails list of SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfEmail(const RvSdpMsg* msg)
{
    return msg->iEmailList.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetFirstEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first email object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpEmail object or the NULL pointer if there are no
 *      emails defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.  
 ***************************************************************************/
RvSdpEmail* rvSdpMsgGetFirstEmail(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpEmail*) rvSdpListGetFirst(&msg->iEmailList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next email object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpEmail object or the NULL pointer if there is no
 *      more emails defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst function. 
 ***************************************************************************/
RvSdpEmail* rvSdpMsgGetNextEmail(RvSdpListIter* iter)
{
    return (RvSdpEmail*) rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the email object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentEmail(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMsgGetEmail
* ------------------------------------------------------------------------
* General: 
*      Gets an email object by index. 
*          
* Return Value: 
*      The requested email object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfEmails() call. 
***************************************************************************/
RvSdpEmail* rvSdpMsgGetEmail(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpEmail*) rvSdpListGetByIndex(&msg->iEmailList,index);
}

/***************************************************************************
 * rvSdpMsgRemoveEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the email object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfEmails call. 
 ***************************************************************************/
void rvSdpMsgRemoveEmail(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iEmailList,index);
}

/***************************************************************************
 * rvSdpMsgClearEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all emails set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearEmail(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iEmailList);
}

/***************************************************************************
 * rvSdpMsgGetNumOfPhones
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level phones list.
 *          
 * Return Value: 
 *      Size of phones list of SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfPhones(const RvSdpMsg* msg)
{
    return msg->iPhoneList.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetFirstPhone
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first phone object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpPhone object or the NULL pointer if there are no
 *      phones defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpPhone* rvSdpMsgGetFirstPhone(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpPhone*) rvSdpListGetFirst(&msg->iPhoneList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextPhone
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next phone object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpPhone object or the NULL pointer if there is no
 *      more phones defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpPhone* rvSdpMsgGetNextPhone(RvSdpListIter* iter)
{
    return (RvSdpPhone*) rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentPhone
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the phone object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentPhone(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMsgGetPhone
* ------------------------------------------------------------------------
* General: 
*      Gets a phone object by index. 
*          
* Return Value: 
*      The requested phone object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfPhones() call. 
***************************************************************************/
RvSdpPhone* rvSdpMsgGetPhone(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpPhone*) rvSdpListGetByIndex(&msg->iPhoneList,index);
}

/***************************************************************************
 * rvSdpMsgRemovePhone
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the phone object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfPhones call. 
 ***************************************************************************/
void rvSdpMsgRemovePhone(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iPhoneList,index);
}

/***************************************************************************
 * rvSdpMsgClearPhones
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all phones set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearPhones(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iPhoneList);
}

/***************************************************************************
 * rvSdpMsgGetNumOfSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level times list.
 *          
 * Return Value: 
 *      Size of session times list of SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfSessionTime(const RvSdpMsg* msg)
{
    return msg->iSessionTimeList.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetFirstSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first session time object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpSessionTime object or the NULL pointer if there are
 *      no session times defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpSessionTime* rvSdpMsgGetFirstSessionTime(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpSessionTime*) rvSdpListGetFirst(&msg->iSessionTimeList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next session time object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpSessionTime object or the NULL pointer if there is no
 *      more session times defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpSessionTime* rvSdpMsgGetNextSessionTime(RvSdpListIter* iter)
{
    return (RvSdpSessionTime*) rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the session time object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentSessionTime(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMsgGetSessionTime
* ------------------------------------------------------------------------
* General: 
*      Gets a session time object by index. 
*          
* Return Value: 
*      The requested session time object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfSessionTime() call. 
***************************************************************************/
RvSdpSessionTime* rvSdpMsgGetSessionTime(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpSessionTime*) rvSdpListGetByIndex(&msg->iSessionTimeList,index);
}

/***************************************************************************
 * rvSdpMsgRemoveSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the session time object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfSessionTimes call. 
 ***************************************************************************/
void rvSdpMsgRemoveSessionTime(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iSessionTimeList,index);
}

/***************************************************************************
 * rvSdpMsgClearSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all session time objects set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearSessionTime(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iSessionTimeList);
}

/***************************************************************************
 * rvSdpMsgGetNumOfMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptors set in SDP message.
 *          
 * Return Value: 
 *      Number of media descriptors set in SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfMediaDescr(const RvSdpMsg* msg)
{
    return msg->iMediaDescriptors.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetFirstMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first media descriptor object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpMediaDescr object or the NULL pointer if there are no
 *      media descriptors defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMsgGetFirstMediaDescr(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpMediaDescr*) rvSdpListGetFirst(&msg->iMediaDescriptors,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next media descriptor object defined in the SDP message. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpMediaDescr object or the NULL pointer if there is no
 *      more media descriptors defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMsgGetNextMediaDescr(RvSdpListIter* iter)
{
    return (RvSdpMediaDescr*) rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the media descriptor object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentMediaDescr(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMsgGetMediaDescr
* ------------------------------------------------------------------------
* General: 
*      Gets a media descriptor object by index. 
*          
* Return Value: 
*      The requested media descriptor object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMsgGetNumOfMediaDescr call. 
***************************************************************************/
RvSdpMediaDescr* rvSdpMsgGetMediaDescr(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpMediaDescr*) rvSdpListGetByIndex(&msg->iMediaDescriptors,index);
}

/***************************************************************************
 * rvSdpMsgRemoveMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the media descriptor object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfMediaDescr call. 
 ***************************************************************************/
void rvSdpMsgRemoveMediaDescr(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iMediaDescriptors,index);
}

/***************************************************************************
 * rvSdpMsgClearMediaDescr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all media descriptors set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearMediaDescr(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iMediaDescriptors);
}

/***************************************************************************
 * rvSdpMsgGetNumOfZoneAdjustments
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level time zone adjustments list.
 *          
 * Return Value: 
 *      Size of time zone adjustments list of SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfZoneAdjustments(const RvSdpMsg* msg)
{
    return msg->iTZA.iTimeZoneList.iListSize;
}

/***************************************************************************
* rvSdpMsgGetZoneAdjustment
* ------------------------------------------------------------------------
* General: 
*      Gets a time zone adjustment object by index. 
*          
* Return Value: 
*      The requested time zone adjustment object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMsgGetNumOfZoneAdjustments call. 
***************************************************************************/
RvSdpTimeZoneAdjust* rvSdpMsgGetZoneAdjustment(RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpTimeZoneAdjust*) rvSdpListGetByIndex(&msg->iTZA.iTimeZoneList,index);
}

/***************************************************************************
 * rvSdpMsgGetFirstZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first time zone adjustment  object defined in the SDP 
 *      message. Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpZoneAdjustment object or the NULL pointer if there
 *      are no time zone adjustments defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpTimeZoneAdjust* rvSdpMsgGetFirstZoneAdjustment(RvSdpMsg* msg, RvSdpListIter *iter)
{
    return (RvSdpTimeZoneAdjust*) rvSdpListGetFirst(&msg->iTZA.iTimeZoneList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next time zone adjustment object defined in the SDP message. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpTimeZoneAdjust object or the NULL pointer if there is no
 *      more time zone adjustments defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpTimeZoneAdjust* rvSdpMsgGetNextZoneAdjustment(RvSdpListIter *iter)
{
    return (RvSdpTimeZoneAdjust*) rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the time zone adjustment object pointed by list
 *      iterator. The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentZoneAdjustment(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveTimeZoneAdjust
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the time zone adjustment object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfZoneAdjustments call. 
 ***************************************************************************/
void rvSdpMsgRemoveTimeZoneAdjust(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iTZA.iTimeZoneList,index);
}

/***************************************************************************
 * rvSdpMsgClearZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all time zone adjustments set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearZoneAdjustment(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iTZA.iTimeZoneList);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMsgAddOther
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary tag SDP object to the session level list of
 *      RvSdpOther objects.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpOther object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      tag - the tag letter of the line.
 *      value - the proprietary text of the line.
 ***************************************************************************/
RvSdpOther* rvSdpMsgAddOther(RvSdpMsg* msg, const char tag, const char *value)
{
    return rvSdpAddOther(msg,&msg->iCommonFields,tag,value);
}

/***************************************************************************
 * rvSdpMsgGetNumOfOther
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of proprietary tag objects set in SDP message.
 *          
 * Return Value: 
 *      Number of proprietary tag objects set in SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfOther(const RvSdpMsg* msg)
{
    return msg->iCommonFields.iOtherList.iListSize;
}

/***************************************************************************
 * rvSdpMsgGetFirstOther
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first proprietary-tag object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpOther object or the NULL pointer if there are no
 *      'other' objects defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpOther* rvSdpMsgGetFirstOther(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return (RvSdpOther*)(rvSdpListGetFirst(&msg->iCommonFields.iOtherList,iter));
}

/***************************************************************************
 * rvSdpMsgGetNextOther
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next 'other' object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpOther object or the NULL pointer if there is no
 *      more 'other' objects defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpOther* rvSdpMsgGetNextOther(RvSdpListIter* iter)
{
    return (RvSdpOther*)rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentOther
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the 'other' object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentOther(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMsgGetOther
* ------------------------------------------------------------------------
* General: 
*      Gets a RvSdpOther object by index. 
*          
* Return Value: 
*      The requested RvSdpOther object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMsgGetNumOfOther call. 
***************************************************************************/
RvSdpOther* rvSdpMsgGetOther(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpOther*)rvSdpListGetByIndex(&msg->iCommonFields.iOtherList,index);
}

/***************************************************************************
 * rvSdpMsgRemoveOther
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the 'other' object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent rvSdpMsgGetNumOfOther call. 
 ***************************************************************************/
void rvSdpMsgRemoveOther(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iCommonFields.iOtherList,index);
}

/***************************************************************************
 * rvSdpMsgClearOther
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all RvSdpOther objects set in SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearOther(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iCommonFields.iOtherList);
}

/***************************************************************************
 * rvSdpMsgUriIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the URI field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvBool rvSdpMsgUriIsBadSyntax(const RvSdpMsg* msg)
{
    return msg->iUri.iUriBadSyntax;
}

/***************************************************************************
 * rvSdpGetBadSyntaxUri
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted URI value 
 *      or empty string ("") if the value is legal or is not set. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
const char* rvSdpMsgGetBadSyntaxUri(const RvSdpMsg* msg)
{
    return RV_SDP_EMPTY_STRING(msg->iUri.iUriTxt);
}

/***************************************************************************
 * rvSdpMsgSetBadSyntaxURI
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP uri field with a proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      uri - The proprietary formatted URI to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBadSyntaxURI(RvSdpMsg* msg, const char* uri)
{
    return rvSdpMsgSetURI2(msg,uri,RV_TRUE);
}

/***************************************************************************
 * rvSdpMsgAddBadSyntaxEmail
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpEmail object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpEmail object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      text - the proprietary value of email field
 ***************************************************************************/
RvSdpEmail* rvSdpMsgAddBadSyntaxEmail(RvSdpMsg* msg, const char* text)
{
    return rvSdpMsgAddEmail2(msg,NULL,NULL,text);
}

/***************************************************************************
 * rvSdpMsgAddBadSyntaxPhone
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpPhone object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpPhone object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of the phone field
 ***************************************************************************/
RvSdpPhone* rvSdpMsgAddBadSyntaxPhone(RvSdpMsg* msg, const char* phone)
{
    return rvSdpMsgAddPhone2(msg,NULL,NULL,phone);
}

/***************************************************************************
 * rvSdpMsgAddBadSyntaxSessionTime
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpSessionTime object at the session level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpSessionTime object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of session time field
 ***************************************************************************/
RvSdpSessionTime* rvSdpMsgAddBadSyntaxSessionTime(RvSdpMsg* msg, const char *session)
{
    return rvSdpMsgAddSessionTime2(msg,0,0,session);
}

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/


/***************************************************************************
 * rvSdpMsgGetNumOfAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level attributes list (generic 
 *      attributes). Special attributes (RTP map, connection mode, key management, 
 *      crypto, frame-rate and fmtp)
 *      are not counted among the attributes treated by this function. Use
 *      rvSdpMsgGetNumOfAttr2 function for all attributes number.
 *          
 * Return Value: 
 *      Number of generic attributes (except for special) of the message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfAttr(const RvSdpMsg* msg)
{
	return  rvSdpGetNumOfSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
* rvSdpMsgGetAttribute
* ------------------------------------------------------------------------
* General: 
*      Gets a generic attribute object by index. Use rvSdpMsgGetAttribute2 to get
*      attribute of all (generic and special) attributes.
*          
* Return Value: 
*      The requested generic attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent rvSdpMsgGetNumOfAttr() call. 
***************************************************************************/
RvSdpAttribute* rvSdpMsgGetAttribute(const RvSdpMsg* msg, RvSize_t index)
{
	return rvSdpGetSpecialAttr((RvSdpCommonFields*)&msg->iCommonFields,\
                                                 index,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMsgGetFirstAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first generic attribute object defined in the SDP message. 
 *      Also sets the list iterator for the further use. Use rvSdpMsgGetFirstAttribute2
 *      for iterating on all (generic and special) attributes.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there are no
 *      generic attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpAttribute* rvSdpMsgGetFirstAttribute(RvSdpMsg* msg, RvSdpListIter* iter)
{
	return (RvSdpAttribute*)rvSdpGetFirstSpecialAttr(&msg->iCommonFields,
													 iter,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMsgGetNextAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next generic attribute object defined in the SDP message. 
 *      The 'next' object is defined based on the list iterator state. Use 
 *      rvSdpMsgGetNextAttribute2 for iterating on all (generic and special) attributes.
 *      
 * Return Value: 
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more generic attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpAttribute* rvSdpMsgGetNextAttribute(RvSdpListIter* iter)
{
	return (RvSdpAttribute*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_NOT_SET);
}
 
/***************************************************************************
 * rvSdpMsgRemoveCurrentAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the attribute object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentAttribute(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the generic attribute object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfAttr call. 
 ***************************************************************************/
void rvSdpMsgRemoveAttribute(RvSdpMsg* msg,  RvSize_t index)
{
	rvSdpRemoveSpecialAttr(&msg->iCommonFields,index,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMsgClearAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all generic attributes set in SDP message. 
 *      The special attributes will not be removed. Use rvSdpMsgClearAttr2 to
 *      remove all (generic and special) attributes.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearAttr(RvSdpMsg* msg)
{			
	rvSdpClearSpecialAttr(&msg->iCommonFields,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMsgGetNumOfAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of elements in the session level attributes list.
 *      Special attributes are counted as well as generic. 
 *          
 * Return Value: 
 *      Size of attributes list of SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvSize_t rvSdpMsgGetNumOfAttr2(const RvSdpMsg* msg)
{
    return msg->iCommonFields.iAttrList.iListSize;
}

/***************************************************************************
* rvSdpMsgGetAttribute2
* ------------------------------------------------------------------------
* General: 
*      Gets an attribute object by index. 
*          
* Return Value: 
*      The requested attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMsgGetNumOfAttr2() call. 
***************************************************************************/
RvSdpAttribute* rvSdpMsgGetAttribute2(const RvSdpMsg* msg, RvSize_t index)
{
    return (RvSdpAttribute*) rvSdpListGetByIndex(&msg->iCommonFields.iAttrList,index);
}

/***************************************************************************
 * rvSdpMsgRemoveAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the attribute object by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMsgGetNumOfAttr2 call. 
 ***************************************************************************/
void rvSdpMsgRemoveAttribute2(RvSdpMsg* msg, RvSize_t index)
{
    rvSdpListRemoveByIndex(&msg->iCommonFields.iAttrList,index);
}

/***************************************************************************
 * rvSdpMsgGetFirstAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first attribute object defined in the SDP message. Also sets
 *      the list iterator for the further use. 
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there are no
 *      attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further 
 *             GetNext calls.
 ***************************************************************************/
RvSdpAttribute* rvSdpMsgGetFirstAttribute2(RvSdpMsg* msg, RvSdpListIter* iter)
{
    return rvSdpListGetFirst(&msg->iCommonFields.iAttrList,iter);
}

/***************************************************************************
 * rvSdpMsgGetNextAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next attribute object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more attributes defined in the SDP message
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
RvSdpAttribute* rvSdpMsgGetNextAttribute2(RvSdpListIter* iter)
{
    return rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMsgRemoveCurrentAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the attribute object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMsgRemoveCurrentAttribute2(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMsgClearAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all (generic and special) attributes set in 
 *      SDP message.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgClearAttr2(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iCommonFields.iAttrList);
}

#endif /*#ifndef RV_SDP_USE_MACROS*/

