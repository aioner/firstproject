/******************************************************************************
Filename    :rvsdpbadsyntax.c
Description :bad syntax  manipulation routines.

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
#include "rvsdpprivate.h"
#include <string.h>

#ifdef RV_SDP_CHECK_BAD_SYNTAX

/***************************************************************************
 * rvSdpOtherConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOther object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'oth' is NULL the 'other' will be allocated from
 *            the 'msg' pool of 'other's. If 'msg' is not NULL the constructed
 *            'other' will be appended to 'msg' list of 'other's. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      oth - a pointer to valid RvSdpOther object or NULL.
 *      tag - the tag letter of the line.
 *      value - the text of the line.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'oth' must point to valid & constructed
 *                      RvSdpOther object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpOther* 
rvSdpOtherConstruct2(RvSdpMsg* msg, RvSdpOther* oth, const char tag, const char* value, 
                     RvAlloc* a, RvBool dontConstruct)
{

    if (!dontConstruct)
    {
        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*) a;

        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the RvSdpMsg context */
        {
            if (oth)
                /* the 'oth' can't be set it has to be allocated from the RvSdpMsg Others pool */
                return NULL;
            oth = rvSdpPoolTake(&msg->iOthersPool);
            if (!oth)
                /* failed to allocate from the RvSdpMsg Others pool */
                return NULL;            
            memset(oth,0,sizeof(RvSdpOther));
            oth->iObj = msg;
        }
        else 
        {
            /* obsolete API usage:
                no RvSdpMsg context given, will be using allocator */
            if (!oth)
                /* oth must be supplied */
                return NULL;
            memset(oth,0,sizeof(RvSdpOther));        
            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();
            /* save the allocator used */
            oth->iObj = a;
        }
    }
 
    oth->iOtherTag = tag;
    /* allocate the strings on the RvSdpMsg strings buffer */
    if (rvSdpSetTextField(&oth->iValue,oth->iObj,value) != RV_SDPSTATUS_OK)
    {
        if (msg && !dontConstruct)
            rvSdpPoolReturn(&msg->iOthersPool,oth);
        return NULL;
    }

    if (!dontConstruct)
    {
        if (msg)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_UNKNOWN_TAG,&oth->iLineObj,
                    RV_OFFSETOF(RvSdpOther,iLineObj));
    }    
    
    return oth;   
}

/*
 *	To allocate the memory for RvSdpOther object (called by the pool)
 *  Return: 
 *      valid RvSdpOther pointer
 *      or NULL if fails
 */
RvSdpOther* rvSdpOtherCreateByPool(RvSdpMsg* msg)
{
    RvSdpOther *oth;
    oth = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpOther));
    if (!oth)
        return NULL;
    memset(oth,0,sizeof(RvSdpOther));
    oth->iObj = msg;
    return oth;
}

/***************************************************************************
 * rvSdpOtherConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs an 'other' object and copies the values from a source 
 *      'other' field. 
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to 'other' object to be constructed. Must point 
 *             to valid memory.
 *      src - a source 'other' object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpOther* rvSdpOtherConstructCopyA(RvSdpOther* dest, const RvSdpOther* src, RvAlloc* a)
{
    return rvSdpOtherCopy2(dest,src,a,RV_FALSE);
}

/*
 *	To free the memory for RvSdpOther object (called by the pool)
 */
void rvSdpOtherDestroyByPool(RvSdpOther* oth)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(oth->iObj))->iAllocator,sizeof(RvSdpOther),oth);
}

/***************************************************************************
 * rvSdpOtherDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the 'other' object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
***************************************************************************/
void rvSdpOtherDestruct(RvSdpOther *oth)
{ 
    if (!oth->iObj)
        /* cannot deallocate memory */
        return;    
    
    rvSdpUnsetTextField(&oth->iValue,oth->iObj);
    if (RV_SDP_OBJ_IS_MESSAGE(oth))
    {
        rvSdpLineObjsListRemove(oth->iObj,&oth->iLineObj);
        rvSdpPoolReturn(&((RvSdpMsg*)(oth->iObj))->iOthersPool,oth);
    }
}

/***************************************************************************
 * rvSdpBadSyntaxGetField
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the SDP object contained in the BadSyntax object.
 *
 * Return Value: 
 *      Returns a handle to the contained object. This handle can be cast to 
 *      the contained SDP object according to its type. The type can be retrieved 
 *      using the rvSdpBadSyntaxGetFieldType() function.
 * ------------------------------------------------------------------------
 * Arguments:
 *      badS - a pointer to the RvSdpBadSyntax object.
 ***************************************************************************/
RvSdpGenericFieldPtr rvSdpBadSyntaxGetField(const RvSdpBadSyntax* badS)
{
    RvSdpLineObject *lo = (RvSdpLineObject*) badS;
    char *p;
    p = (char*)lo - lo->iOffset;
    return p;
}

/***************************************************************************
 * rvSdpBadSyntaxGetFieldType
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the type of SDP object contained in the BadSyntax object. The user
 *      may use this function to know which object is wrapped inside the BadSyntax 
 *      object.
 *
 * Return Value: 
 *      Returns the type of the wrapped object. This type is mapped by the
 *      RvSdpFieldTypes enumeration.
 * ------------------------------------------------------------------------
 * Arguments:
 *      badS - a pointer to the RvSdpBadSyntax object.
 ***************************************************************************/
RvSdpFieldTypes rvSdpBadSyntaxGetFieldType(const RvSdpBadSyntax* badS)
{
    RvSdpLineObject *lo = (RvSdpLineObject*) badS;
    if (lo->iFieldType == SDP_FIELDTYPE_ATTRIBUTE)
    {
        RvSdpAttribute *attr = (RvSdpAttribute*)((char*)lo-lo->iOffset);
        if (attr->iSpecAttrData)
            return attr->iSpecAttrData->iFieldType;
    }
    return lo->iFieldType;
}


#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpOtherCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpOther from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpOther object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpOther object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpOther* rvSdpOtherCopy2(RvSdpOther* dest, const RvSdpOther* src,  
                            void* obj, RvBool dontConstruct)
{
    return rvSdpOtherConstruct2(NULL,dest,src->iOtherTag,src->iValue,obj,dontConstruct);
}

/***************************************************************************
 * rvSdpOtherConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOther object.
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to valid RvSdpOther object.
 *      tag - the tag letter of the line.
 *      value - the text of the line.
 ***************************************************************************/
RvSdpOther* rvSdpOtherConstruct(RvSdpOther* oth, const char tag, const char* value)
{
    return rvSdpOtherConstruct2(NULL,oth,tag,value,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpOtherConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOther object.
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to valid RvSdpOther object.
 *      tag - the tag letter of the line.
 *      value - the text of the line.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpOther* rvSdpOtherConstructA(RvSdpOther* oth, const char tag, const char* value, RvAlloc* a)
{
    return rvSdpOtherConstruct2(NULL,oth,tag,value,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpOtherConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs an 'other' object and copies the values from a source 
 *      'other' field. 
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to 'other' object to be constructed. Must point 
 *             to valid memory.
 *      src - a source 'other' object.
 ***************************************************************************/
RvSdpOther* rvSdpOtherConstructCopy(RvSdpOther* dest, const RvSdpOther* src)
{
    return rvSdpOtherCopy2(dest,src,NULL,RV_FALSE);
}


/***************************************************************************
 * rvSdpOtherCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source 'other' object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination 'other' object. Must point 
 *             to constructed RvSdpOther object.
 *      src - a 'other' object.
 ***************************************************************************/
RvSdpOther* rvSdpOtherCopy(RvSdpOther* dest, const RvSdpOther* src)
{
    return rvSdpOtherCopy2(dest,src,NULL,RV_TRUE);
}

/***************************************************************************
 * rvSdpOtherGetTag
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the tag letter.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 ***************************************************************************/
char rvSdpOtherGetTag(const RvSdpOther* oth)
{
    return oth->iOtherTag;
}

/***************************************************************************
 * rvSdpOtherSetTag
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the tag letter of the line.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 *      tag - the line's tag letter.
 ***************************************************************************/
void rvSdpOtherSetTag(RvSdpOther* oth, const char tag)
{
    oth->iOtherTag = tag;
}

/***************************************************************************
 * rvSdpOtherGetValue
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the SDP line's value (after the '=' symbol).
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 ***************************************************************************/
const char* rvSdpOtherGetValue(const RvSdpOther* oth)
{
    return RV_SDP_EMPTY_STRING(oth->iValue);
}

/***************************************************************************
 * rvSdpOtherSetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the value of the line (after the '=' symbol).
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 *      value - the SDP line's value.
 ***************************************************************************/
RvSdpStatus rvSdpOtherSetValue(RvSdpOther* oth, const char *value)
{
    return rvSdpSetTextField(&oth->iValue,oth->iObj,value);
}

#endif /*#ifndef RV_SDP_USE_MACROS*/

#endif /*#ifdef RV_SDP_CHECK_BAD_SYNTAX*/

