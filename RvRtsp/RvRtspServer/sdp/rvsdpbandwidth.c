/******************************************************************************
Filename    :rvsdpbandwidth.c
Description :bandwidth  manipulation routines.

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

/*
 *	To allocate the memory for RvSdpBandwidth object (called by the pool)
 *  Return: 
 *      valid RvSdpBandwidth pointer
 *      or NULL if fails
 */
RvSdpBandwidth* rvSdpBandwidthCreateByPool(RvSdpMsg* msg)
{
    RvSdpBandwidth* bw;
    
    bw = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpBandwidth));
    if (!bw)
        return NULL;
    
    memset(bw,0,sizeof(RvSdpBandwidth));   
    bw->iObj = msg;
    
    return bw;
}

/*
 *	To free the memory for RvSdpBandwidth object (called by the pool)
 */
void rvSdpBandwidthDestroyByPool(RvSdpBandwidth* p)
{    
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpBandwidth),p);
}

/*
 *	Sets the internal RvSdpBandwidth fields to the supplied values.
 *  Returns the 'bw' of NULL if fails.
 */
RvSdpBandwidth* rvSdpBandwidthFill(RvSdpBandwidth* bw, const char* type, RvUint32 value)
{
    bw->iBWValue = value;
    if (rvSdpSetTextField(&bw->iBWType,bw->iObj,type) != RV_SDPSTATUS_OK)
        return NULL;
    return bw;
}

/***************************************************************************
 * rvSdpBandwidthConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpBandwidth object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'bw' is NULL the bandwidth will be allocated from
 *            the 'msg' pool of bandwidths. If 'msg' is not NULL the constructed
 *            bandwidth will be appended to 'msg' list of bandwidths. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      bw - a pointer to valid RvSdpBandwidth object or NULL.
 *      type - the bandwidth type name.
 *      value - the bandwith value (in Kbs).
 *      badSyn - the proprietary formatted bandwidth field or NULL if standard bandwidth
 *               is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'bw' must point to valid & constructed
 *                      RvSdpBandwidth object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpBandwidth* 
rvSdpBandwidthConstruct2(RvSdpMsg* msg, RvSdpBandwidth* bw, const char* type, 
                         RvUint32 value, const char* badSyn, RvAlloc* a, 
                         RvBool dontConstruct)
{
    if (!dontConstruct)
    {
        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*) a;
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the msg context */
        {
            if (bw)
                /* the 'bw' can't be set it has to be allocated from the msg attributes pool */
                return NULL;
        
            bw = rvSdpPoolTake(&msg->iBandwidthsPool);
            if (!bw)
                /* failed to allocate from the msg attributes pool */
                return NULL;

            memset(bw,0,sizeof(RvSdpBandwidth));       
        
            bw->iObj = msg;
        }
        else 
        {
            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!bw)
                return NULL;

            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            memset(bw,0,sizeof(RvSdpBandwidth));

            /* save the allocator used */
            bw->iObj = a;
        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        if (rvSdpSetTextField(&bw->iBadSyntaxField,bw->iObj,badSyn) != RV_SDPSTATUS_OK)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iBandwidthsPool,bw);
            return NULL;        
        }
        if (msg && !dontConstruct)
        {
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_BANDWIDTH,&bw->iLineObj,
                RV_OFFSETOF(RvSdpBandwidth,iLineObj));                
        }
        return bw;
    }
#endif
    if (badSyn)
        return NULL;

    if (!rvSdpBandwidthFill(bw,type,value))
    {
        if (msg && !dontConstruct)   
            rvSdpPoolReturn(&msg->iBandwidthsPool,bw);
        return NULL;
    }
    
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_BANDWIDTH,&bw->iLineObj,
            RV_OFFSETOF(RvSdpBandwidth,iLineObj));                
    return bw;   
}

/***************************************************************************
 * rvSdpBandwidthConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a bandwidth object and copies the values from a source 
 *      bandwidth field. 
 *      This function is obsolete. The rvSdpMsgAddBandwidth or  
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to bandwidth to be constructed. Must point 
 *             to valid memory.
 *      src - a source bandwidth object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpBandwidth* 
rvSdpBandwidthConstructCopyA(RvSdpBandwidth* dest, RvSdpBandwidth* src, RvAlloc* a)
{
    return rvSdpBandwidthCopy2(dest,src,a,RV_FALSE);
}


/***************************************************************************
 * rvSdpBandwidthCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpBandwidth from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpBandwidth object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpBandwidth object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpBandwidth* 
rvSdpBandwidthCopy2(RvSdpBandwidth* dest, const RvSdpBandwidth* src, 
                    void* obj, RvBool dontConstruct)
{
    if (!obj && dest && dontConstruct)
        obj = dest->iObj;
    return rvSdpBandwidthConstruct2(NULL,dest,src->iBWType,src->iBWValue,
                     RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj,dontConstruct);
}

/***************************************************************************
 * rvSdpBandwidthDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the bandwidth object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
***************************************************************************/
void rvSdpBandwidthDestruct(RvSdpBandwidth* bw)
{
    if (!bw->iObj)
        /* cannot deallocate memory */
        return;

    rvSdpUnsetTextField(&bw->iBWType,bw->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&bw->iBadSyntaxField,bw->iObj);
#endif

    rvSdpLineObjsListRemove(bw->iObj,&bw->iLineObj);
    
    if (RV_SDP_OBJ_IS_MESSAGE(bw))
        rvSdpPoolReturn(&((RvSdpMsg*)(bw->iObj))->iBandwidthsPool,bw);
}

#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpBandwidthConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpBandwidth object.
 *      This function is obsolete. The rvSdpMsgAddBandwidth or  
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      type - the bandwidth type name.
 *      value - the bandwith value (in Kbs).
 ***************************************************************************/
RvSdpBandwidth* rvSdpBandwidthConstruct(RvSdpBandwidth* bw, const char* type, RvUint32 value)
{
    return rvSdpBandwidthConstruct2(NULL,bw,type,value,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBandwidthConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpBandwidth object.
 *      This function is obsolete. The rvSdpMsgAddBandwidth or  
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      type - the bandwidth type name.
 *      value - the bandwith value (in Kbs).
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpBandwidth* rvSdpBandwidthConstructA(RvSdpBandwidth* bw, 
                                         const char* type, RvUint32 value, RvAlloc* a)
{
    return rvSdpBandwidthConstruct2(NULL,bw,type,value,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpBandwidthConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a bandwidth object and copies the values from a source 
 *      bandwidth field. 
 *      This function is obsolete. The rvSdpMsgAddBandwidth or  
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to bandwidth to be constructed. Must point 
 *             to valid memory.
 *      src - a source bandwidth object.
 ***************************************************************************/
RvSdpBandwidth* rvSdpBandwidthConstructCopy(RvSdpBandwidth* dest, RvSdpBandwidth* src)
{
    return rvSdpBandwidthCopy2(dest,src,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBandwidthCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source bandwidth object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination bandwidth. Must point 
 *             to constructed RvSdpBandwidth object.
 *      src - a source bandwidth object.
 ***************************************************************************/
RvSdpBandwidth* rvSdpBandwidthCopy(RvSdpBandwidth* dest, RvSdpBandwidth* src)
{
    return rvSdpBandwidthCopy2(dest,src,NULL,RV_TRUE);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxBandwidthConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpBandwidth object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxBandwidth or  
 *      rvSdpMediaDescrSetBadSyntaxBandwidth should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      badSyn - the proprietary format of bandwidth field.
 ***************************************************************************/
RvSdpBandwidth* rvSdpBadSyntaxBandwidthConstruct(RvSdpBandwidth* bw, const char* badSyn)
{
    return rvSdpBandwidthConstruct2(NULL,bw,NULL,0,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxBandwidthConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpBandwidth object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxBandwidth or  
 *      rvSdpMediaDescrSetBadSyntaxBandwidth should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      badSyn - the proprietary format of bandwidth field.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpBandwidth* rvSdpBadSyntaxBandwidthConstructA(RvSdpBandwidth* bw, const char* badSyn, RvAlloc* a)
{
    return rvSdpBandwidthConstruct2(NULL,bw,NULL,0,badSyn,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpBandwidthIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the bandwidth field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 ***************************************************************************/
RvBool rvSdpBandwidthIsBadSyntax(RvSdpBandwidth* bw)
{
    return (bw->iBadSyntaxField) ? RV_TRUE : RV_FALSE;
}

/***************************************************************************
 * rvSdpBandwidthGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted bandwidth field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 ***************************************************************************/
const char* rvSdpBandwidthGetBadSyntax(const RvSdpBandwidth* bw)
{
    return RV_SDP_EMPTY_STRING(bw->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpBandwidthSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP bandwidth field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpBandwidth object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpBandwidthSetBadSyntax(RvSdpBandwidth* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpBandwidthGetType
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the bandwidth type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 ***************************************************************************/
const char* rvSdpBandwidthGetType(const RvSdpBandwidth* bw)
{
    return RV_SDP_EMPTY_STRING(bw->iBWType);
}

/***************************************************************************
 * rvSdpBandwidthSetType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the bandwidth type.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 *      type - the bandwidth type.
 ***************************************************************************/
RvSdpStatus rvSdpBandwidthSetType(RvSdpBandwidth* bw, const char* type)
{
    return rvSdpSetTextField(&bw->iBWType,bw->iObj,type);
}

/***************************************************************************
 * rvSdpBandwidthGetValue
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the bandwidth value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
***************************************************************************/
RvUint32 rvSdpBandwidthGetValue(const RvSdpBandwidth* bw)
{
    return bw->iBWValue;
}

/***************************************************************************
 * rvSdpBandwidthSetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the bandwidth value.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 *      value - value bandwidth value.
 ***************************************************************************/
void rvSdpBandwidthSetValue(RvSdpBandwidth* bw, RvUint32 value)
{
    bw->iBWValue = value;
}

#endif /*#ifndef RV_SDP_USE_MACROS*/

