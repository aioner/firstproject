/******************************************************************************
Filename    :rvsdpmedia.c
Description : media descriptor manipulation routines.

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
#include <stdlib.h>
#include <stdio.h>

#include "rvsdpprivate.h"

/***************************************************************************
 * rvSdpMediaDescrConstructEx
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpMediaDescr object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'descr' is NULL the media descriptor will be allocated from
 *            the 'msg' pool of media descriptors. If 'msg' is not NULL the constructed
 *            media will be appended to 'msg' list of media descriptors. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      descr - a pointer to valid RvSdpMediaDescr object or NULL.
 *      mediaType - the type of media.
 *      port - the media's port.
 *      protocol - the media's protocol.
 *      badSyn - the proprietary formatted media field or NULL if standard media is 
 *               constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'descr' must point to valid & constructed
 *                      RvSdpMediaDescr object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrConstructEx(RvSdpMsg* msg, RvSdpMediaDescr* descr, 
                                           RvSdpMediaType mediaType,
                                           RvUint32 port, RvSdpProtocol protocol,
                                           const char* badSyn, 
                                           RvAlloc* a, RvBool dontConstruct)
{        
    if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
        msg = (RvSdpMsg*)a;
    if (!dontConstruct)
    {
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the RvSdpMsg context */
        {            
            if (descr)
                /* the 'descr' can't be set it has to be allocated from the msg media descr pool */
                return NULL;
        
            descr = rvSdpPoolTake(&msg->iMediasPool);
            if (!descr)
                /* failed to allocate from the msg emails pool */
                return NULL;
        
            memset(descr,0,sizeof(RvSdpMediaDescr));
        
            descr->iObj = msg;
        }
        else 
        {        
            if (!descr)
                /* the RvSdpMediaDescr instance has to be supplied */
                return NULL;

            memset(descr,0,sizeof(RvSdpMediaDescr));        
        
            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            descr->iObsMsg = rvSdpMsgConstruct2(NULL,a);
            if (descr->iObsMsg == NULL)
                return NULL;

            /* save the allocator used */
            descr->iObj = descr->iObsMsg;
            msg = descr->iObsMsg;
        }
    }

    descr->iSdpMsg = msg;

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&descr->iBadSyntaxField,descr->iObj,badSyn) != RV_SDPSTATUS_OK)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iMediasPool,descr);
            return NULL;
        }
    }
    else
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    {        
        if (badSyn)
            return NULL;

        /* media type and protocol will not exceed this */
        rvSdpMsgPromiseBuffer(msg,200);
        
            /* allocate the strings from the allocator */
        rvSdpMediaDescrSetMediaType(descr,mediaType);
		rvSdpMediaDescrSetProtocol(descr,protocol);
        descr->iPort = port;
    }

    if (!dontConstruct)
        rvSdpCommonFieldsInitialize(&descr->iCommonFields);

    if (msg && !dontConstruct && !descr->iObsMsg)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_MEDIA,&descr->iLineObj,
            RV_OFFSETOF(RvSdpMediaDescr,iLineObj));

    return descr;   
}

/*
 *	To allocate the memory for RvSdpMediaDescr object (called by the pool)
 *  Return: 
 *      valid RvSdpMediaDescr pointer
 *      or NULL if fails
 */
RvSdpMediaDescr* rvSdpMediaDescrCreateByPool(RvSdpMsg* msg)
{
    RvSdpMediaDescr* md;
    
    md = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpMediaDescr));
    if (!md)
        return NULL;
    
    memset(md,0,sizeof(RvSdpMediaDescr));   
    md->iObj = msg;
    
    return md;
}

/*
 *	Sets the internal RvSdpMediaDescr fields of 'dest' from the 'src'
 *  RvSdpMediaDescr instance.   
 *  Returns the new constructed RvSdpMediaDescr object.
 */
RvSdpMediaDescr* rvSdpMediaDescrFill(RvSdpMediaDescr* dest, const RvSdpMediaDescr* src)
{
	int cnt;
    dest->iPort = src->iPort;
    dest->iNumOfPorts = src->iNumOfPorts;

    rvSdpMediaDescrClearFormat(dest);
    for (cnt = 0; cnt < src->iMediaFormatsNum; cnt++)
        if (rvSdpMediaDescrAddFormat(dest,src->iMediaFormats[cnt]) != RV_SDPSTATUS_OK)
            return NULL;
                
    if (rvSdpSetTextField(&dest->iMediaTypeStr,dest->iObj,src->iMediaTypeStr) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&dest->iProtocolStr,dest->iObj,src->iProtocolStr) != RV_SDPSTATUS_OK)
        return NULL;
    return dest;
}


/***************************************************************************
 * rvSdpMediaDescrCopyEx
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpMediaDescr from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpMediaDescr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMediaDescr object or NULL. 
 *      src - a pointer to the source object.
 *      obj - the RvSdpMsg instance that will own the destination object or the
 *            allocator pointer.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 *      mediaOnly - if set to true only fields regarding media line itself will
 *                  be copied, otherwise also object owned by media descriptors
 *                  will be copied as well.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrCopyEx(RvSdpMediaDescr* dest, const RvSdpMediaDescr* src, 
                                      void* obj, RvBool dontConstruct, RvBool mediaOnly)
{
    if (!obj && dest && dontConstruct)
    {
        if (dest->iSdpMsg)
            obj = dest->iSdpMsg;
        else if (dest->iObj)
            obj = dest->iObj;
    }
    dest = rvSdpMediaDescrConstructEx(NULL,dest,RV_SDPMEDIATYPE_NOTSET,src->iPort,RV_SDPPROTOCOL_NOTSET,
                        RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj,dontConstruct);
    if (dest == NULL)
        return NULL;

    if (!rvSdpMediaDescrFill(dest,src))
        goto failure;

    if (!mediaOnly)
    {
        rvSdpCommonFieldsDestruct(&dest->iCommonFields,obj);
        if (rvSdpCommonFieldCopy2(&dest->iCommonFields,&src->iCommonFields,obj) != RV_SDPSTATUS_OK)
            goto failure;
    }
            
    return dest;

failure:
    rvSdpMediaDescrDestruct(dest);
    return NULL;
}

/*
 *	To free the memory for RvSdpMediaDescr object (called by the pool)
 */
void rvSdpMediaDescrDestroyByPool(RvSdpMediaDescr* media)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(media->iObj))->iAllocator,sizeof(RvSdpMediaDescr),media);
}

/***************************************************************************
 * rvSdpMediaDescrDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the media descriptor object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
***************************************************************************/
void rvSdpMediaDescrDestruct(RvSdpMediaDescr* descr)
{
    if (!descr->iObj)
        /* cannot deallocate memory */
        return;
 
    rvSdpCommonFieldsDestruct(&descr->iCommonFields,descr->iObj);

    rvSdpMediaDescrClearFormat(descr);
/*
    rvSdpListClear(&descr->iVcidValue);
    rvSdpUnsetTextField(&descr->iControlMethod,descr->iObj);
*/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&descr->iBadSyntaxField,descr->iObj);
#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/
    rvSdpUnsetTextField(&descr->iMediaTypeStr,descr->iObj);
    rvSdpUnsetTextField(&descr->iProtocolStr,descr->iObj);

    rvSdpMediaDescrDestroyInformation(descr);
    rvSdpKeyDestruct(&descr->iCommonFields.iKey);
        
    rvSdpLineObjsListRemove(descr->iSdpMsg,&descr->iLineObj);

    if (descr->iObsMsg)
    {
        rvSdpMsgDestruct(descr->iObsMsg);
        descr->iObsMsg = NULL;
    }
    else if (RV_SDP_OBJ_IS_MESSAGE(descr))
        /* RvSdpMsg context was used */
        rvSdpPoolReturn(&((RvSdpMsg*)(descr->iObj))->iMediasPool,descr);        
        
}

/***************************************************************************
* rvSdpMediaDescrGetPayload
* ------------------------------------------------------------------------
* General: 
*      Gets a media descriptor payload by index. 
*          
* Return Value: 
*      The requested media descriptor payload.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfPayloads() call. 
***************************************************************************/
int rvSdpMediaDescrGetPayload(RvSdpMediaDescr* descr, int index)
{
    const char *p;
    p = rvSdpMediaDescrGetFormat(descr,index);
    if (!p)
        return 0;
    return atoi(p);
}

/***************************************************************************
 * rvSdpMediaDescrAddPayloadNumber
 * ------------------------------------------------------------------------
 * General: 
 *      Adds another payload number to the media descriptor object.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      payload - the payload to be added.
***************************************************************************/
RvSdpStatus rvSdpMediaDescrAddPayloadNumber(RvSdpMediaDescr* descr, int payload)
{
    char txt[20];
    return rvSdpMediaDescrAddFormat(descr,rvSdpItoa(txt,payload));
}

/***************************************************************************
 * rvSdpMediaDescrSetConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new connection object to the media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP 
 *             address for an IP network, and so on.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetConnection(RvSdpMediaDescr* descr, RvSdpNetType type, 
                                         RvSdpAddrType addr_type, const char* addr)
{    
    return (rvSdpMediaDescrAddConnection(descr,type,addr_type,addr)) ? 
                                    RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
} 

/***************************************************************************
 * rvSdpMediaDescrSetBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new bandwidth object at media descriptor level.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetBandwidth(RvSdpMediaDescr* descr, const char* bwtype, int b)
{
    return (rvSdpMediaDescrAddBandwidth(descr,bwtype,b)) ? RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

/***************************************************************************
 * rvSdpMediaDescrSetKey
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP key field in media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      em - the key encryption method.
 *      key - the key value
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetKey(RvSdpMediaDescr* descr, RvSdpEncrMethod em, 
                                  const char* key)
{
    return (rvSdpKeyConstruct2(descr->iSdpMsg,&descr->iCommonFields.iKey,em,key,
                               NULL,NULL,RV_FALSE))
                            ? RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}


#ifdef RV_SDP_CHECK_BAD_SYNTAX

/***************************************************************************
 * rvSdpMediaDescrSetBadSyntaxKey
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP key field with a proprietary format for a specific
 *      media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - The proprietary formatted key field to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetBadSyntaxKey(RvSdpMediaDescr* descr, const char* key)
{
    if (rvSdpKeyConstruct2(descr->iSdpMsg,&descr->iCommonFields.iKey,
                            RV_SDPENCRMTHD_NOTSET,NULL,key,NULL,RV_FALSE))
        return RV_SDPSTATUS_OK;
    else
        return RV_SDPSTATUS_ALLOCFAIL;
}


/* 
 * tests whether the line object 'lo' is bad syntax and
 * belongs to media descriptor 'media'
 */
RvBool rvSdpLineObjectBelongsToMediaAndBad(RvSdpLineObject *lo, RvSdpMediaDescr *media)
{
    char *p = (char*)lo - lo->iOffset, *pp, **ppp;
    RvSdpList *l;
    RvSdpListIter i;
    int offs;
    
    if (lo->iFieldType == SDP_FIELDTYPE_KEY && media->iCommonFields.iKey.iBadSyntaxField && 
        (char*)&media->iCommonFields.iKey.iBadSyntaxField == p)
        return RV_TRUE;
    else if (lo->iFieldType == SDP_FIELDTYPE_CONNECTION)
    {
        l = &media->iCommonFields.iConnectionList;
        offs = RV_OFFSETOF(RvSdpConnection,iBadSyntaxField);
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_BANDWIDTH)
    {
        l = &media->iCommonFields.iBandwidthList;
        offs = RV_OFFSETOF(RvSdpBandwidth,iBadSyntaxField);
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_UNKNOWN_TAG)
    {
        l = &media->iCommonFields.iOtherList;
        offs = -1;
    }
    else if (lo->iFieldType == SDP_FIELDTYPE_ATTRIBUTE)
    {
        RvSdpAttribute *attr = (RvSdpAttribute*)p;

        if (attr->iSpecAttrData && attr->iSpecAttrData->iBadValueOffset >= 0)
            offs = attr->iSpecAttrData->iBadValueOffset;
		else
            return RV_FALSE;		
        l = &media->iCommonFields.iAttrList;
    }
    else
        return RV_FALSE;
    
    
    for (pp = (char*)rvSdpListGetFirst(l,&i); pp; pp = (char*)rvSdpListGetNext(&i))
    {
        if (p != pp)
            continue;
        if (offs < 0)
            return RV_TRUE;
        ppp = (char**)(pp + offs);
        if (*ppp)
            return RV_TRUE;
    }    
    
    return RV_FALSE;
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor bad syntax objects.
 *          
 * Return Value: 
 *      Number of bad syntax objects defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfBadSyntax(const RvSdpMediaDescr* descr)
{
    RvSdpLineObject *lo;
    RvSize_t cnt;
    
    for (lo = descr->iSdpMsg->iHeadLO, cnt = 0; lo; lo = lo->iNext)
    {
        if (rvSdpLineObjectBelongsToMediaAndBad(lo,(RvSdpMediaDescr*)descr))
            cnt++;
    }
    return cnt; 
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first bad syntax object defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBadSyntax  object or the NULL pointer if there are no
 *      bad syntax objects defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextBadSyntax calls
 ***************************************************************************/
RvSdpBadSyntax* rvSdpMediaDescrGetFirstBadSyntax(RvSdpMediaDescr* descr, RvSdpLineObjIter* iter)
{
    RvSdpLineObject *lo;
    for (lo = descr->iSdpMsg->iHeadLO; lo; lo = lo->iNext)
    {
        if (rvSdpLineObjectBelongsToMediaAndBad(lo,(RvSdpMediaDescr*)descr))
        {
            iter->iCurrentLO = lo->iNext;
            iter->iMediaDescr = descr;
            return (RvSdpBadSyntax*) lo;
        }
    }
    return NULL;
}

/***************************************************************************
 * rvSdpMediaDescrGetNextBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next bad syntax object defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBadSyntax object or the NULL pointer if there is no
 *      more BadSyntaxs defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)BadSyntax function. 
 ***************************************************************************/
RvSdpBadSyntax* rvSdpMediaDescrGetNextBadSyntax(RvSdpLineObjIter* iter)
{
    RvSdpLineObject *lo =  iter->iCurrentLO;
    
    for ( ; lo; lo = lo->iNext)
    {
        if (rvSdpLineObjectBelongsToMediaAndBad(lo,iter->iMediaDescr))
        {
            iter->iCurrentLO = lo->iNext;
            return (RvSdpBadSyntax*) lo;
        }
    }
    return NULL;
}

/***************************************************************************
* rvSdpMediaDescrGetBadSyntax
* ------------------------------------------------------------------------
* General: 
*      Gets a bad syntax object by index (in media descriptor context). 
*          
* Return Value: 
*      The requested RvSdpBadSyntax object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfBadSyntax() call. 
***************************************************************************/
RvSdpBadSyntax* rvSdpMediaDescrGetBadSyntax(const RvSdpMediaDescr* descr, int index)
{
    RvSdpLineObject *lo;
    RvSize_t cnt;
    
    for (lo = descr->iSdpMsg->iHeadLO, cnt = 0; lo; lo = lo->iNext)
    {
        if (rvSdpLineObjectBelongsToMediaAndBad(lo,(RvSdpMediaDescr*)descr))
        {
            if (cnt == (unsigned)index)
                return (RvSdpBadSyntax*) lo;
            cnt++;            
        }
        
    }
    return NULL;
}

#endif /*#ifdef RV_SDP_CHECK_BAD_SYNTAX*/


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMediaDescrSetBadSyntaxConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the SDP connection field with a proprietary format for a specific
 *      media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - The proprietary formatted connection field to be set.
 ***************************************************************************/
RvSdpStatus 
rvSdpMediaDescrSetBadSyntaxConnection(RvSdpMediaDescr* descr, const char* badSyn)
{
    return (rvSdpAddConnection2(descr,RV_SDPNETTYPE_NOTSET,RV_SDPADDRTYPE_NOTSET,
                        NULL,badSyn) != NULL) ? RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL; 
}

/***************************************************************************
 * rvSdpMediaDescrSetBadSyntaxBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the SDP bandwidth field with a proprietary format for a specific
 *      media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      bandwidth - The proprietary formatted bandwidth field to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetBadSyntaxBandwidth(RvSdpMediaDescr* descr, 
                                                 const char* bandwidth)
{
    return (rvSdpAddBandwidth2(descr,NULL,0,bandwidth) != NULL) ? 
                                RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL;
}

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/


/***************************************************************************
 * rvSdpMediaDescrConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a media descriptor and copies the values from a source media
 *      descriptor. 
 *      This  function is obsolete. The 'rvSdpMsgInsertMediaDescr' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to a media descriptor to be constructed. Must point 
 *             to valid memory.
 *      src - a source media descriptor.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrConstructCopyA(RvSdpMediaDescr* dest, const RvSdpMediaDescr* src, RvAlloc* a)
{
    return rvSdpMediaDescrCopyEx(dest,src,a,RV_FALSE,RV_FALSE);
}

/***************************************************************************
 * rvSdpMediaDescrAddFormatN
 * ------------------------------------------------------------------------
 * General: 
 *      Adds another codec format to the media descriptor object.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      fmt - the name of the format.
 *      len - the 'fmt' length
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrAddFormatN(RvSdpMediaDescr* descr, const char* fmt, int len)
{
	char val[32];
	if (len > (int) sizeof(val)-1)
		return RV_SDPSTATUS_ALLOCFAIL;
	strncpy(val,fmt,len);
	val[len] = 0;
	return rvSdpMediaDescrAddFormat(descr,val);
}

#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpMediaDescrAddFormat
 * ------------------------------------------------------------------------
 * General: 
 *      Adds another codec format to the media descriptor object.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      fmt - the name of the format.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrAddFormat(RvSdpMediaDescr* descr, const char* fmt)
{
    return rvSdpAddTextToArray(&(descr->iMediaFormatsNum),RV_SDP_MEDIA_FORMATS_MAX,
                    descr->iMediaFormats,descr->iObj,fmt);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveFormat
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and deallocates) the codec format name by index in the
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
 *              rvSdpMediaDescrGetNumOfConnections call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveFormat(RvSdpMediaDescr* descr,RvSize_t index)
{
    rvSdpRemoveTextFromArray(&(descr->iMediaFormatsNum),descr->iMediaFormats,descr->iObj,index);
    
}

/***************************************************************************
 * rvSdpMediaDescrClearFormat
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all codec formats set in the media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearFormat(RvSdpMediaDescr* descr)
{
    rvSdpClearTxtArray(&(descr->iMediaFormatsNum),descr->iMediaFormats,descr->iObj);
}

/***************************************************************************
 * rvSdpMediaDescrSetInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the information field of the media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      info - the new information value.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetInformation(RvSdpMediaDescr* descr, const char* info)
{ 
    return rvSdpSetSdpInformation(&descr->iCommonFields.iInfo,info,descr->iSdpMsg);
}

/***************************************************************************
 * rvSdpMediaDescrDestroyInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the information field of the media descriptor.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrDestroyInformation(RvSdpMediaDescr* descr)
{
    rvSdpDestroySdpInformation(&descr->iCommonFields.iInfo,descr->iSdpMsg);
}

/***************************************************************************
 * rvSdpMediaDescrAddRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Adds a new RTP map to the media descriptor's RTP map list.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpRtpMap object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      payload - an RTP dynamic payload number.
 *      encoding_name - the name of the codec.
 *      rate - the clock rate.
 ***************************************************************************/
RvSdpRtpMap* rvSdpMediaDescrAddRtpMap(RvSdpMediaDescr* descr, int payload, const char* encoding_name, int rate)
{
    return rvSdpAddRtpMap2(descr->iSdpMsg,&descr->iCommonFields,payload,encoding_name,rate,NULL);
}

#ifdef RV_SDP_CHECK_BAD_SYNTAX
/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpRtpMap object at media 
 *      descriptor level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpRtpMap object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of RTP map special attribute field
 ***************************************************************************/
RvSdpRtpMap* rvSdpMediaDescrAddBadSyntaxRtpMap(RvSdpMediaDescr* descr, const char* rtpmap)
{
    return rvSdpAddRtpMap2(descr->iSdpMsg,&descr->iCommonFields,0,NULL,0,rtpmap);
}
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */


/***************************************************************************
 * rvSdpMediaDescrGetNumOfRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor RTP map attributes.
 *          
 * Return Value: 
 *      Number of codec RTP maps defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfRtpMap(const RvSdpMediaDescr* descr)
{
    return rvSdpGetNumOfSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the RTP map special attribute object pointed 
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
void rvSdpMediaDescrRemoveCurrentRtpMap(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the RTP map special attribute object by index in the
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
 *              rvSdpMediaDescrGetNumOfRtpMap call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveRtpMap(RvSdpMediaDescr* descr, RvSize_t index)
{   
    rvSdpRemoveSpecialAttr(&descr->iCommonFields,index,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMediaDescrClearRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all RTP map special attributes set in the 
 *      media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearRtpMap(RvSdpMediaDescr* descr)
{
    rvSdpClearSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first RTP map special attribute defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRtpMap  object or the NULL pointer if there are no
 *      RTP maps defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextRtpMap calls.
 ***************************************************************************/
RvSdpRtpMap* rvSdpMediaDescrGetFirstRtpMap(RvSdpMediaDescr* descr, RvSdpListIter* iter)
{
    return (RvSdpRtpMap*)rvSdpGetFirstSpecialAttr(&descr->iCommonFields,iter,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
 * rvSdpMediaDescrGetNextRtpMap
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next RtpMap special attribute defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRtpMap object or the NULL pointer if there are no
 *      more RtpMap special attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescrGetFirstRtpMap function. 
 ***************************************************************************/
RvSdpRtpMap* rvSdpMediaDescrGetNextRtpMap(RvSdpListIter* iter)
{
    return (RvSdpRtpMap*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_RTP_MAP);
}

/***************************************************************************
* rvSdpMediaDescrGetRtpMap
* ------------------------------------------------------------------------
* General: 
*      Gets an RTP map special attribute object by index (in media descriptor 
*      context). 
*          
* Return Value: 
*      The requested RvSdpRtpMap pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfRtpMap() call. 
***************************************************************************/
RvSdpRtpMap* rvSdpMediaDescrGetRtpMap(const RvSdpMediaDescr* descr, RvSize_t index)
{
    return (RvSdpRtpMap*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&descr->iCommonFields,
                    index,SDP_FIELDTYPE_RTP_MAP);
}

#ifdef RV_SDP_KEY_MGMT_ATTR

/***************************************************************************
 * rvSdpMediaDescrAddKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new key management attribute to the media descriptor.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMiaDescr object.
 *      prtclId - the protocol ID.
 *      keyData - the encryption key data.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMediaDescrAddKeyMgmt(RvSdpMediaDescr* descr, const char* prtclId, const char* keyData)
{
    return rvSdpAddKeyMgmt2(descr->iSdpMsg,&descr->iCommonFields,prtclId,keyData,NULL);
}

#ifdef RV_SDP_CHECK_BAD_SYNTAX
/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpKeyMgmtAttr object at media 
 *      descriptor level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of KeyMgmt attribute field
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMediaDescrAddBadSyntaxKeyMgmt(RvSdpMediaDescr* descr, 
                                                     const char* keyMgmt)
{
    return rvSdpAddKeyMgmt2(descr->iSdpMsg,&descr->iCommonFields,NULL,NULL,keyMgmt);
}
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor key-mgmt attributes.
 *          
 * Return Value: 
 *      Number of codec key-mgmt attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfKeyMgmt(const RvSdpMediaDescr* descr)
{
    return rvSdpGetNumOfSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the key-mgmt special attribute object pointed 
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
void rvSdpMediaDescrRemoveCurrentKeyMgmt(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the key-mgmt special attribute object by index in the
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
 *              rvSdpMediaDescrGetNumOfKeyMgmt call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveKeyMgmt(RvSdpMediaDescr* descr, RvSize_t index)
{   
    rvSdpRemoveSpecialAttr(&descr->iCommonFields,index,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMediaDescrClearKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all key-mgmt special attributes set in media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearKeyMgmt(RvSdpMediaDescr* descr)
{
    rvSdpClearSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first key-mgmt attribute defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpKeyMgmtAttr  object or the NULL pointer if there are no
 *      key-mgmt attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextKeyMgmt calls.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMediaDescrGetFirstKeyMgmt(RvSdpMediaDescr* descr, 
                                                 RvSdpListIter* iter)
{
    return (RvSdpKeyMgmtAttr*)rvSdpGetFirstSpecialAttr(&descr->iCommonFields,
            iter,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
 * rvSdpMediaDescrGetNextKeyMgmt
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next KeyMgmt attribute defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpKeyMgmtAttr object or the NULL pointer if there 
 *      are no more KeyMgmt attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)KeyMgmt function. 
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMediaDescrGetNextKeyMgmt(RvSdpListIter* iter)
{
    return (RvSdpKeyMgmtAttr*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_KEY_MGMT);
}

/***************************************************************************
* rvSdpMediaDescrGetKeyMgmt
* ------------------------------------------------------------------------
* General: 
*      Gets a key management special attribute object by index (in media 
*      descriptor context). 
*          
* Return Value: 
*      The requested RvSdpKeyMgmtAttr pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfKeyMgmt() call. 
***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpMediaDescrGetKeyMgmt(const RvSdpMediaDescr* descr, RvSize_t index)
{
    return (RvSdpKeyMgmtAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&descr->iCommonFields,index,SDP_FIELDTYPE_KEY_MGMT);
}

#endif /*RV_SDP_KEY_MGMT_ATTR*/


#ifdef RV_SDP_CRYPTO_ATTR

/***************************************************************************
 * rvSdpMediaDescrAddCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new crypto special attribute object to the media descriptor.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      tag - the crypto attribute tag number.
 *      suite - the crypto attribute suite value.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMediaDescrAddCrypto(RvSdpMediaDescr* descr, RvUint tag, const char* suite)
{
    return rvSdpAddCrypto2(descr->iSdpMsg,&descr->iCommonFields,tag,suite,NULL);
}

#ifdef RV_SDP_CHECK_BAD_SYNTAX
/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpCryptoAttr object at media 
 *      descriptor level.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of crypto special attribute field
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMediaDescrAddBadSyntaxCrypto(RvSdpMediaDescr* descr, const char* crypto)
{
    return rvSdpAddCrypto2(descr->iSdpMsg,&descr->iCommonFields,0,NULL,crypto);
}
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor crypto special attributes.
 *          
 * Return Value: 
 *      Number of crypto attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfCrypto(const RvSdpMediaDescr* descr)
{
    return rvSdpGetNumOfSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the crypto special attribute object pointed 
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
void rvSdpMediaDescrRemoveCurrentCrypto(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the crypto special attribute object by index in the
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
 *              rvSdpMediaDescrGetNumOfCrypto call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveCrypto(RvSdpMediaDescr* descr, RvSize_t index)
{   
    rvSdpRemoveSpecialAttr(&descr->iCommonFields,index,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMediaDescrClearCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all crypto special attributes set in media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearCrypto(RvSdpMediaDescr* descr)
{
    rvSdpClearSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first crypto special attribute defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpCryptoAttr  object or the NULL pointer if there are no
 *      crypto attriubutes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextCrypto calls.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMediaDescrGetFirstCrypto(RvSdpMediaDescr* descr, 
                                               RvSdpListIter* iter)
{
    return (RvSdpCryptoAttr*)rvSdpGetFirstSpecialAttr(&descr->iCommonFields,
        iter,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMediaDescrGetNextCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next Crypto attribute defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpCryptoAttr object or the NULL pointer if there are no
 *      more Crypto attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescrGetFirstCrypto function. 
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMediaDescrGetNextCrypto(RvSdpListIter* iter)
{
    return (RvSdpCryptoAttr*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_CRYPTO);
}

/***************************************************************************
 * rvSdpMediaDescrGetCrypto
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a crypto attribute object by index (in media descriptor context). 
 *          
 * Return Value: 
 *      The requested RvSdpCryptoAttr pointer.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMediaDescrGetNumOfCrypto() call. 
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpMediaDescrGetCrypto(const RvSdpMediaDescr* descr, RvSize_t index)
{
    return (RvSdpCryptoAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&descr->iCommonFields,
        index,SDP_FIELDTYPE_CRYPTO);
}

#endif /*RV_SDP_CRYPTO_ATTR*/

/***************************************************************************
 * rvSdpMediaDescrConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpMediaDescr object using default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddMediaDescr' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      mediaType - the type of media.
 *      port - the media's port.
 *      protocol - the media's protocol.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrConstruct(RvSdpMediaDescr* descr, RvSdpMediaType mediaType,
                                          RvUint32 port, RvSdpProtocol protocol)
{
    return rvSdpMediaDescrConstructEx(NULL,descr,mediaType,port,protocol,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpMediaDescrConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpMediaDescr object.
 *      This function is obsolete. The 'rvSdpMsgAddMediaDescr' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      mediaType - the type of media.
 *      port - the media's port.
 *      protocol - the media's protocol.
 *      badSyn - the proprietary formatted media field or NULL if standard media is 
 *               constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrConstructA(RvSdpMediaDescr* descr, 
                                           RvSdpMediaType mediaType, RvUint32 port,
                                           RvSdpProtocol protocol, RvAlloc* a)
{
    return rvSdpMediaDescrConstructEx(NULL,descr,mediaType,port,protocol,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpMediaDescrConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a media descriptor and copies the values from a source media
 *      descriptor.
 *      This  function is obsolete. The 'rvSdpMsgInsertMediaDescr' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to a media descriptor to be constructed. Must point 
 *             to valid memory.
 *      src - a source media descriptor.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrConstructCopy(RvSdpMediaDescr* dest, 
                                              const RvSdpMediaDescr* src)
{
    return rvSdpMediaDescrCopyEx(dest,src,NULL,RV_FALSE,RV_FALSE);
}

/***************************************************************************
 * rvSdpMediaDescrCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source media descriptor to destination.
 *      This function is obsolete. 
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination media descriptor. Must point 
 *             to constructed RvSdpMediaDescr object.
 *      src - a source media descriptor.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrCopy(RvSdpMediaDescr* dest, const RvSdpMediaDescr* src)
{
    return rvSdpMediaDescrCopyEx(dest,src,NULL,RV_TRUE,RV_FALSE);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxMediaDescrConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpMediaDescr object with proprietary format using 
 *      default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxMediaDescr' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - the proprietary format of media descriptor.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpBadSyntaxMediaDescrConstruct(RvSdpMediaDescr* descr, 
                                                   const char* badSyn)
{
    return rvSdpMediaDescrConstructEx(NULL,descr,RV_SDPMEDIATYPE_NOTSET,0,
        RV_SDPPROTOCOL_NOTSET,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxMediaDescrConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpMediaDescr object with proprietary format using 
 *      provided allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxMediaDescr' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - the proprietary format of media descriptor.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpBadSyntaxMediaDescrConstructA(RvSdpMediaDescr* descr, 
                                                    const char* badSyn, RvAlloc* a)
{
    return rvSdpMediaDescrConstructEx(NULL,descr,RV_SDPMEDIATYPE_NOTSET,0,
        RV_SDPPROTOCOL_NOTSET,badSyn,a,RV_FALSE);
}

#endif /*#if defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/***************************************************************************
 * rvSdpMediaDescrAddConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new connection object to the media descriptor object.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpConnection object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP 
 *             address for an IP network, and so on.
 ***************************************************************************/
RvSdpConnection* rvSdpMediaDescrAddConnection(RvSdpMediaDescr* descr,RvSdpNetType type,
                                              RvSdpAddrType addr_type, const char* addr)
{
    return rvSdpAddConnection2(descr,type,addr_type,addr,NULL);
}

/***************************************************************************
 * rvSdpMediaDescrAddBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new bandwidth object at media descriptor level.
 *          
 * Return Value: 
 *      Pointer to the added RvSdpBandwidth  object or the NULL pointer if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
RvSdpBandwidth* rvSdpMediaDescrAddBandwidth(RvSdpMediaDescr* descr, 
                                            const char* bwtype, int b)
{
    return rvSdpAddBandwidth2(descr,bwtype,b,NULL);
}


/***************************************************************************
* rvSdpMediaDescrAddAttr
* ------------------------------------------------------------------------
* General: 
*      Adds new generic attribute to the media descriptor.
*          
* Return Value: 
*      The pointer to added RvSdpAttribute object or NULL if the function fails.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      name - the name of the new generic attribute.
*      value - the value of the new generic attribute.
**************************************************************************/
RvSdpAttribute* rvSdpMediaDescrAddAttr(RvSdpMediaDescr* descr, const char* name, 
                                       const char* value)
{
    return rvSdpAddAttr2(descr->iSdpMsg,&descr->iCommonFields,NULL,name,value);
}

/***************************************************************************
 * rvSdpMediaDescrGetMediaType
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the media type of media descriptor.
 *          
 * Return Value: 
 *      Media type value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSdpMediaType rvSdpMediaDescrGetMediaType(const RvSdpMediaDescr* descr)
{
	return rvSdpMediaTypeTxt2Val(descr->iMediaTypeStr);
}

/***************************************************************************
 * rvSdpMediaDescrSetMediaType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the media type of media descriptor.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      type - the new media type.
 ***************************************************************************/
void rvSdpMediaDescrSetMediaType(RvSdpMediaDescr* descr, RvSdpMediaType type)
{
	rvSdpSetTextField(&descr->iMediaTypeStr,descr->iObj,rvSdpMediaTypeVal2Txt(type));
}


/***************************************************************************
 * rvSdpMediaDescrGetProtocol
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the protocol of media descriptor.
 *          
 * Return Value: 
 *      Media protocol value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSdpProtocol rvSdpMediaDescrGetProtocol(const RvSdpMediaDescr* descr)
{
	return rvSdpMediaProtoTxt2Val(descr->iProtocolStr);
}

/***************************************************************************
 * rvSdpMediaDescrSetProtocol
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the protocol type of the media descriptor.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      protocol - the new media protocol value.
 ***************************************************************************/
void rvSdpMediaDescrSetProtocol(RvSdpMediaDescr* descr, RvSdpProtocol protocol)
{
	rvSdpSetTextField(&descr->iProtocolStr,descr->iObj,rvSdpMediaProtoVal2Txt(protocol));
}

/***************************************************************************
 * rvSdpMediaDescrGetMediaTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the media type string of media descriptor.
 *          
 * Return Value: 
 *      Media type text value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
const char* rvSdpMediaDescrGetMediaTypeStr(RvSdpMediaDescr* descr)
{
	return RV_SDP_EMPTY_STRING(descr->iMediaTypeStr);
}

/***************************************************************************
 * rvSdpMediaDescrSetMediaTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the media type string of media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      type - the new value of media type string.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetMediaTypeStr(RvSdpMediaDescr* descr, const char* type)
{
	return rvSdpSetTextField(&descr->iMediaTypeStr,descr->iObj,type);
}

/***************************************************************************
 * rvSdpMediaDescrGetProtocolStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the media protocol name string of the media descriptor.
 *          
 * Return Value: 
 *      Media protocol text value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
const char* rvSdpMediaDescrGetProtocolStr(RvSdpMediaDescr* descr)
{
	return RV_SDP_EMPTY_STRING(descr->iProtocolStr);
}

/***************************************************************************
 * rvSdpMediaDescrSetProtocolStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the media protcol name string of the media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      protocol - the new value of the media type string.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetProtocolStr(RvSdpMediaDescr* descr, const char* protocol)
{
	return rvSdpSetTextField(&descr->iProtocolStr,descr->iObj,protocol);
}


/*
const char* rvSdpMediaDescrGetNASCtrlMethod(const RvSdpMediaDescr* descr)
{
    return RV_SDP_EMPTY_STRING(descr->iControlMethod);
}
RvSdpStatus rvSdpMediaDescrSetNASCtrlMethod(RvSdpMediaDescr* descr, const char* cm)
{
    return rvSdpSetTextField(&descr->iControlMethod,descr->iObj,cm);
}
*/

/***************************************************************************
 * rvSdpMediaDescrGetPort
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the media port number.
 *          
 * Return Value: 
 *      The port number.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvUint32 rvSdpMediaDescrGetPort(const RvSdpMediaDescr* descr)
{
    return descr->iPort;
}

/***************************************************************************
 * rvSdpMediaDescrSetPort
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the media port number.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      port - the new value of the media port number.
 ***************************************************************************/
void rvSdpMediaDescrSetPort(RvSdpMediaDescr* descr, RvUint32 port)
{
    descr->iPort = port;
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfPorts
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of subsequent ports defined for the media descriptor.
 *          
 * Return Value: 
 *      Number of subsequent ports defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
int rvSdpMediaDescrGetNumOfPorts(const RvSdpMediaDescr* descr)
{
    return descr->iNumOfPorts;
}

/***************************************************************************
 * rvSdpMediaDescrSetNumOfPorts
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the number of subsequent ports defined for the media descriptor.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      numPorts - the new number of subsequent ports.
 ***************************************************************************/
void rvSdpMediaDescrSetNumOfPorts(RvSdpMediaDescr* descr, int numPorts)
{
    descr->iNumOfPorts = numPorts;
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfFormats
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor codec formats.
 *          
 * Return Value: 
 *      Number of codec formats defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfFormats(const RvSdpMediaDescr* descr)
{
    return descr->iMediaFormatsNum;
}

/***************************************************************************
* rvSdpMediaDescrGetFormat
* ------------------------------------------------------------------------
* General: 
*      Gets a media descriptor format by index. 
*          
* Return Value: 
*      The requested codec format name object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfFormats() call. 
***************************************************************************/
const char* rvSdpMediaDescrGetFormat(const RvSdpMediaDescr* descr, RvSize_t index)
{
    return RV_SDP_EMPTY_STRING((const char*) descr->iMediaFormats[index]);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfPayloads
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor payloads.
 *          
 * Return Value: 
 *      Number of payloads defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfPayloads(RvSdpMediaDescr* descr)
{
    return descr->iMediaFormatsNum;
}

/***************************************************************************
 * rvSdpMediaDescrRemovePayloadNumber
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the codec payload number by index in the media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMediaDescrGetNumOfPayloads call. 
 ***************************************************************************/
void rvSdpMediaDescrRemovePayloadNumber(RvSdpMediaDescr* descr, RvSize_t index)
{
    rvSdpMediaDescrRemoveFormat(descr,index);
}

/***************************************************************************
 * rvSdpMediaDescrClearPayloads
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all codec payload numbers set in the media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearPayloads(RvSdpMediaDescr* descr)
{
    rvSdpMediaDescrClearFormat(descr);
}

/***************************************************************************
 * rvSdpMediaDescrGetInformation
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the information field of the media descriptor.
 *          
 * Return Value: 
 *      Returns the media descriptor information field text of empty string
 *      if the information field is not set.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
const char* rvSdpMediaDescrGetInformation(const RvSdpMediaDescr* descr)
{
    return RV_SDP_EMPTY_STRING(descr->iCommonFields.iInfo.iInfoTxt);
}

/***************************************************************************
* rvSdpMediaDescrGetConnection
* ------------------------------------------------------------------------
* General: 
*      Gets a first connection object (in the media descriptor context). 
*          
* Return Value: 
*      The first connection object or NULL if there are no connections.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
***************************************************************************/
RvSdpConnection* rvSdpMediaDescrGetConnection(const RvSdpMediaDescr* descr)
{
    return (RvSdpConnection*) rvSdpListGetByIndex(&descr->iCommonFields.iConnectionList,0);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfConnections
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor connections fields.
 *          
 * Return Value: 
 *      Number of connections defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfConnections(const RvSdpMediaDescr* descr)
{
    return descr->iCommonFields.iConnectionList.iListSize;
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first connection object defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpConnection  object or the NULL pointer if there are no
 *      connections defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextConnection calls
 ***************************************************************************/
RvSdpConnection* rvSdpMediaDescrGetFirstConnection(RvSdpMediaDescr* descr, RvSdpListIter* iter)
{
    return (RvSdpConnection*) rvSdpListGetFirst(&descr->iCommonFields.iConnectionList,iter);    
}

/***************************************************************************
 * rvSdpMediaDescrGetNextConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next connection object defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpConnection object or the NULL pointer if there are no
 *      more connections defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Connection function. 
 ***************************************************************************/
RvSdpConnection* rvSdpMediaDescrGetNextConnection(RvSdpListIter* iter)
{
    return (RvSdpConnection*) rvSdpListGetNext(iter);    
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the connection object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveCurrentConnection(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMediaDescrGetConnectionByIndex
* ------------------------------------------------------------------------
* General: 
*      Gets a connection object by index (in the media descriptor context). 
*          
* Return Value: 
*      The requested connection object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfConnections call. 
***************************************************************************/
RvSdpConnection* rvSdpMediaDescrGetConnectionByIndex(const RvSdpMediaDescr* descr,
                                                     RvSize_t index)
{
    return (RvSdpConnection*) rvSdpListGetByIndex(&descr->iCommonFields.iConnectionList,
        index);
    
}

/***************************************************************************
 * rvSdpMediaDescrRemoveConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the connection object by index in the
 *      context of a media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMediaDescrGetNumOfConnections call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveConnection(RvSdpMediaDescr* descr, RvSize_t index)
{
    rvSdpListRemoveByIndex(&descr->iCommonFields.iConnectionList,index);
}

/***************************************************************************
 * rvSdpMediaDescrClearConnection
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all connections set in the media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearConnection(RvSdpMediaDescr* descr)
{
    rvSdpListClear(&descr->iCommonFields.iConnectionList);
}


/***************************************************************************
* rvSdpMediaDescrGetBandwidth
* ------------------------------------------------------------------------
* General: 
*      Gets a first bandwidth object at the media descriptor level. 
*          
* Return Value: 
*      The first bandwidth object or NULL if there are no bandwidths.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
***************************************************************************/
RvSdpBandwidth* rvSdpMediaDescrGetBandwidth(const RvSdpMediaDescr* descr)
{
    return (RvSdpBandwidth*) rvSdpListGetByIndex(&descr->iCommonFields.iBandwidthList,0);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor bandwidth fields.
 *          
 * Return Value: 
 *      Number of bandwidths defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfBandwidth(const RvSdpMediaDescr* descr)
{
    return descr->iCommonFields.iBandwidthList.iListSize;
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first bandwidth object defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBandwidth  object or the NULL pointer if there are no
 *      bandwidths defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextBandwidth calls.
 ***************************************************************************/
RvSdpBandwidth* rvSdpMediaDescrGetFirstBandwidth(RvSdpMediaDescr* descr, RvSdpListIter* iter)
{
    return (RvSdpBandwidth*) rvSdpListGetFirst(&descr->iCommonFields.iBandwidthList,iter);    
}

/***************************************************************************
 * rvSdpMediaDescrGetNextBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next bandwidth object defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBandwidth object or the NULL pointer if there is no
 *      more bandwidths defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescrGetFirstBandwidth/rvSdpMediaDescrGetNextBandwidth 
 *             functions. 
 ***************************************************************************/
RvSdpBandwidth* rvSdpMediaDescrGetNextBandwidth(RvSdpListIter* iter)
{
    return (RvSdpBandwidth*) rvSdpListGetNext(iter);    
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentBandwidth
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
void rvSdpMediaDescrRemoveCurrentBandwidth(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMediaDescrGetBandwidthByIndex
* ------------------------------------------------------------------------
* General: 
*      Gets a bandwidth object by index at the media descriptor level. 
*          
* Return Value: 
*      The requested bandwidth object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfBandwidths() call. 
***************************************************************************/
RvSdpBandwidth* rvSdpMediaDescrGetBandwidthByIndex(const RvSdpMediaDescr* descr,RvSize_t index)
{
    return (RvSdpBandwidth*) rvSdpListGetByIndex(&descr->iCommonFields.iBandwidthList,index);
    
}

/***************************************************************************
 * rvSdpMediaDescrRemoveBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the bandwidth object by index in the
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
 *              rvSdpMediaDescrGetNumOfBandwidths call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveBandwidth(RvSdpMediaDescr* descr, RvSize_t index)
{
    rvSdpListRemoveByIndex(&descr->iCommonFields.iBandwidthList,index);
}

/***************************************************************************
 * rvSdpMediaDescrClearBandwidth
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all bandwidths set in media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearBandwidth(RvSdpMediaDescr* descr)
{
    rvSdpListClear(&descr->iCommonFields.iBandwidthList);
}


/***************************************************************************
 * rvSdpMediaDescrGetKey
 * ------------------------------------------------------------------------
 * General: 
 *      Gets a pointer to the key field of media descriptor.
 *          
 * Return Value: 
 *      A pointer to the key field, or NULL if the key field is not 
 *      set in the media descriptor
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 ***************************************************************************/
RvSdpKey* rvSdpMediaDescrGetKey(const RvSdpMediaDescr* descr)
{
    return (RV_SDP_KEY_IS_USED(&descr->iCommonFields.iKey) ? 
		((RvSdpKey*) &descr->iCommonFields.iKey) : NULL);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor generic attributes.
 *          
 * Return Value: 
 *      Number of generic attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfAttr(const RvSdpMediaDescr* descr)
{
	return  rvSdpGetNumOfSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
* rvSdpMediaDescrGetAttribute
* ------------------------------------------------------------------------
* General: 
*      Gets a generic attribute object by index (in media descriptor context). 
*          
* Return Value: 
*      The requested RvSdpAttribute pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfAttr() call. 
**************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetAttribute(const RvSdpMediaDescr* descr, RvSize_t index)
{
	return rvSdpGetSpecialAttr((RvSdpCommonFields*)&descr->iCommonFields,\
		index,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first generic attribute object defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute  object or the NULL pointer if there are no
 *      generic attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextAttribute calls.
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetFirstAttribute(RvSdpMediaDescr* descr, 
                                                 RvSdpListIter* iter)
{
	return (RvSdpAttribute*)rvSdpGetFirstSpecialAttr(&descr->iCommonFields,
		iter,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMediaDescrGetNextAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next generic attribute defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more generic attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Attribute function. 
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetNextAttribute(RvSdpListIter* iter)
{
	return (RvSdpAttribute*)rvSdpGetNextSpecialAttr(iter,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the attribute object pointed by list iterator
 *      in the context of media descriptor.
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveCurrentAttribute(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveAttribute
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the generic attribute object by index in the
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
 *              rvSdpMediaDescrGetNumOfAttr call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveAttribute(RvSdpMediaDescr* descr,  RvSize_t index)
{
	rvSdpRemoveSpecialAttr(&descr->iCommonFields,index,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMediaDescrClearAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all generic attributes set in media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearAttr(RvSdpMediaDescr* descr)
{			
	rvSdpClearSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_NOT_SET);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor attributes (generic and special).
 *          
 * Return Value: 
 *      Number of attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfAttr2(const RvSdpMediaDescr* descr)
{
    return descr->iCommonFields.iAttrList.iListSize;
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first attribute object (generic or special) defined in the
 *      media descriptor. Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute  object or the NULL pointer if there are no
 *      attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextAttribute2 calls.
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetFirstAttribute2(RvSdpMediaDescr* descr, 
                                                  RvSdpListIter* iter)
{
    return (RvSdpAttribute*) rvSdpListGetFirst(&descr->iCommonFields.iAttrList,iter);

}

/***************************************************************************
 * rvSdpMediaDescrGetNextAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next attribute (generic or special) defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more generic attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Attribute2 function. 
 ***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetNextAttribute2(RvSdpListIter* iter)
{
    return (RvSdpAttribute*) rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentAttribute2
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
void rvSdpMediaDescrRemoveCurrentAttribute2(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMediaDescrGetAttribute2
* ------------------------------------------------------------------------
* General: 
*      Gets an attribute (generic or special) object by index (in media 
*      descriptor context). 
*          
* Return Value: 
*      The requested RvSdpAttribute pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfAttribute2() call. 
***************************************************************************/
RvSdpAttribute* rvSdpMediaDescrGetAttribute2(const RvSdpMediaDescr* descr, RvSize_t index)
{
    return (RvSdpAttribute*) rvSdpListGetByIndex(&descr->iCommonFields.iAttrList,index);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveAttribute2
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the attribute (generic or special) object by 
 *      index in the context of media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMediaDescrGetNumOfAttr2 call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveAttribute2(RvSdpMediaDescr* descr, RvSize_t index)
{
    rvSdpListRemoveByIndex(&descr->iCommonFields.iAttrList,index);
}

/***************************************************************************
 * rvSdpMediaDescrClearAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all attributes (generic and special) set in 
 *      the media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearAttr2(RvSdpMediaDescr* descr)
{
    rvSdpListClear(&descr->iCommonFields.iAttrList);
}

/***************************************************************************
 * rvSdpMediaDescrGetConnectionMode
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the connection mode of the media descriptor or RV_SDPCONNECTMODE_NOTSET
 *      if the correspondent attribute is not set.
 *          
 * Return Value: 
 *      Returns the connection mode.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSdpConnectionMode rvSdpMediaDescrGetConnectionMode(const RvSdpMediaDescr* descr)
{
    return rvSdpGetConnectionMode(&descr->iCommonFields);
}

/***************************************************************************
 * rvSdpMediaDescrSetConnectionMode
 * ------------------------------------------------------------------------
 * General: 
 *      Sets/modifies the connection mode of the media descriptor.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      mode - the new value of connection mode.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetConnectionMode(RvSdpMediaDescr* descr, 
                                             RvSdpConnectionMode mode)
{
    return rvSdpSetConnectionMode(descr->iSdpMsg,&descr->iCommonFields,mode);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMediaDescrGetBadSyntaxValue
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted media descriptor field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
const char* rvSdpMediaDescrGetBadSyntaxValue(RvSdpMediaDescr* descr)
{
    return RV_SDP_EMPTY_STRING(descr->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpMediaDescrIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the media descriptor field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvBool rvSdpMediaDescrIsBadSyntax(RvSdpMediaDescr* descr)
{
    return (descr->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpMediaDescrAddOther
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary tag SDP object to the media descriptor's list of
 *      RvSdpOther objects.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpOther object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      media - a pointer to the RvSdpMediaDescr object.
 *      tag - the tag letter of the line.
 *      value - the proprietary text of the line.
 ***************************************************************************/
RvSdpOther* rvSdpMediaDescrAddOther(RvSdpMediaDescr* descr, 
                                    const char tag, const char *value)
{    
    return rvSdpAddOther(descr->iSdpMsg,&descr->iCommonFields,tag,value);
}

/***************************************************************************
 * rvSdpMediaDescrGetNumOfOther
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of media descriptor RvSdpOther objects.
 *          
 * Return Value: 
 *      Number of RvSdpOther objects defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RvSize_t rvSdpMediaDescrGetNumOfOther(const RvSdpMediaDescr* descr)
{
    return descr->iCommonFields.iOtherList.iListSize;
}

/***************************************************************************
 * rvSdpMediaDescrGetFirstOther
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first RvSdpOther object defined in the media descriptor. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpOther  object or the NULL pointer if there are no
 *      'other's defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpMediaDescrGetNextOther calls.
 ***************************************************************************/
RvSdpOther* rvSdpMediaDescrGetFirstOther(RvSdpMediaDescr* descr, RvSdpListIter* iter)
{
    return (RvSdpOther*)(rvSdpListGetFirst(&descr->iCommonFields.iOtherList,iter));
}

/***************************************************************************
 * rvSdpMediaDescrGetNextOther      
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next 'other' object defined in the media descriptor. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpOther object or the NULL pointer if there are no
 *      more 'other' objects defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Other function. 
 ***************************************************************************/
RvSdpOther* rvSdpMediaDescrGetNextOther(RvSdpListIter* iter)
{
    return (RvSdpOther*)rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentOther
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
void rvSdpMediaDescrRemoveCurrentOther(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
* rvSdpMediaDescrGetOther
* ------------------------------------------------------------------------
* General: 
*      Gets a 'other' object by index (in media descriptor context). 
*          
* Return Value: 
*      The requested RvSdpOther object.
* ------------------------------------------------------------------------
* Arguments:
*      media - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpMediaDescrGetNumOfOther() call. 
***************************************************************************/
RvSdpOther* rvSdpMediaDescrGetOther(RvSdpMediaDescr* descr, RvSize_t i)
{
    return (RvSdpOther*)rvSdpListGetByIndex(&descr->iCommonFields.iOtherList,i);
}

/***************************************************************************
 * rvSdpMediaDescrRemoveOther
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the 'other' object by index in the
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
 *              rvSdpMediaDescrGetNumOfOther call. 
 ***************************************************************************/
void rvSdpMediaDescrRemoveOther(RvSdpMediaDescr* descr, RvSize_t index)
{   
    rvSdpListRemoveByIndex(&descr->iCommonFields.iOtherList,index);
}

/***************************************************************************
 * rvSdpMediaDescrClearOther
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all RvSdpOther objects set in media descriptor.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
void rvSdpMediaDescrClearOther(RvSdpMediaDescr* descr)
{
    rvSdpListClear(&descr->iCommonFields.iOtherList);
}

/***************************************************************************
 * rvSdpMediaDescrSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP media descriptor field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpMediaDescr object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMediaDescrSetBadSyntax(RvSdpMediaDescr* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

#endif /*#ifndef RV_SDP_USE_MACROS*/

