/******************************************************************************
Filename    :rvsdpphone.c
Description :phone manipulation routines.

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
 *	Sets the internal RvSdpPhone fields to the supplied values.
 *  Returns the 'phone' of NULL if fails.
 */
RvSdpPhone* rvSdpPhoneFill(RvSdpPhone* phone, const char* phNumber, 
                           const char* text, RvChar separSymbol)
{
    phone->iSeparSymbol = separSymbol;
    if (rvSdpSetTextField(&phone->iPhoneNumber,phone->iObj,phNumber) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&phone->iText,phone->iObj,text) != RV_SDPSTATUS_OK)
        return NULL;
    return phone;
}


/***************************************************************************
 * rvSdpPhoneConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpPhone object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'phone' is NULL the phone will be allocated from
 *            the 'msg' pool of phones. If 'msg' is not NULL the constructed
 *            phone will be appended to 'msg' list of phones. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      phone - a pointer to valid RvSdpPhone object or NULL.
 *      number - the phone number.
 *      text - optional phone text.
 *      badSyn - the proprietary formatted phone field or NULL if standard 
 *               phone is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'phone' must point to valid & constructed
 *                      RvSdpPhone object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpPhone* 
rvSdpPhoneConstruct2(RvSdpMsg* msg, RvSdpPhone* phone, const char* number, 
                     const char* text, 
                     const char* badSyn,RvAlloc* a, RvBool dontConstruct)
{
    if (text && !*text)
        text = NULL;

    if (!dontConstruct)
    {

        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*) a;

        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the RvSdpMsg context */
        {            

            if (phone)
                /* the 'phone' can't be set it has to be allocated from the msg phones pool */
                return NULL;
        
            phone = rvSdpPoolTake(&msg->iPhonesPool);
            if (!phone)
                /* failed to allocate from the msg phones pool */
                return NULL;
        
            memset(phone,0,sizeof(RvSdpPhone));
        
            phone->iObj = msg;
        }
        else 
        {        
            if (!phone)
                /* the RvSdpPhone instance has to be supplied */
                return NULL;

            memset(phone,0,sizeof(RvSdpPhone));
                
            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            /* save the allocator used */
            phone->iObj = a;
        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&phone->iBadSyntaxField,phone->iObj,badSyn) 
                                                            != RV_SDPSTATUS_OK)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iPhonesPool,phone);
            return NULL;
        }
        if (msg && !dontConstruct)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_PHONE,&phone->iLineObj,
                RV_OFFSETOF(RvSdpPhone,iLineObj));                    
        return phone;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;

    {
        int expLen;
        expLen = ((number)?(int)strlen(number):0) + ((text)?(int)strlen(text):0+10);
        rvSdpMsgPromiseBuffer(msg,expLen);
    }
            
    /* allocate the strings on the msg strings buffer */

    if (!rvSdpPhoneFill(phone,number,text,'('))
    {
        rvSdpPhoneDestruct(phone);
        return NULL;
    }
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_PHONE,&phone->iLineObj,
                    RV_OFFSETOF(RvSdpPhone,iLineObj));                
    return phone;   
}

/***************************************************************************
 * rvSdpPhoneConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a phone object and copies the values from a source 
 *      phone field. 
 *      This function is obsolete. The rvSdpMsgAddPhone should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to phone to be constructed. Must point 
 *             to valid memory.
 *      src - a source phone object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneConstructCopyA(RvSdpPhone* dest, const RvSdpPhone* src, RvAlloc* a)
{
    return rvSdpPhoneCopy2(dest,src,a,RV_FALSE);
}


/***************************************************************************
 * rvSdpPhoneCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpPhone from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpPhone object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpPhone object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneCopy2(RvSdpPhone* dst, const RvSdpPhone* src, void* obj, 
                            RvBool dontConstruct)
{
    RvSdpPhone* p;
    if (!obj && dst && dontConstruct)
        obj = dst->iObj;
    p = rvSdpPhoneConstruct2(NULL,dst,src->iPhoneNumber,src->iText,
                        RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj,dontConstruct);
    if (p)
        p->iSeparSymbol = src->iSeparSymbol;
    return p;
}

/*
 *	To allocate the memory for RvSdpPhone object (called by the pool)
 *  Return: 
 *      valid RvSdpPhone pointer
 *      or NULL if fails
 */
RvSdpPhone* rvSdpPhoneCreateByPool(RvSdpMsg* msg)
{
    RvSdpPhone* phone;
    
    phone = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpPhone));
    if (!phone)
        return NULL;
    
    memset(phone,0,sizeof(RvSdpPhone));   
    phone->iObj = msg;
    
    return phone;
}

/*
 *	To free the memory for RvSdpPhone object (called by the pool)
 */
void rvSdpPhoneDestroyByPool(RvSdpPhone* p)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpPhone),p);
}

/***************************************************************************
 * rvSdpPhoneDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the phone object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
***************************************************************************/
void rvSdpPhoneDestruct(RvSdpPhone* phone)
{
    if (!phone->iObj)
        /* cannot deallocate memory */
        return;
    
    /* RvSdpMsg context was used */
    rvSdpUnsetTextField(&phone->iPhoneNumber,phone->iObj);
    rvSdpUnsetTextField(&phone->iText,phone->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&phone->iBadSyntaxField,phone->iObj);
#endif

    rvSdpLineObjsListRemove(phone->iObj,&phone->iLineObj);
    
    if (RV_SDP_OBJ_IS_MESSAGE(phone))
        rvSdpPoolReturn(&((RvSdpMsg*)(phone->iObj))->iPhonesPool,phone);
}

#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpPhoneConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpPhone object.
 *      This function is obsolete. The rvSdpMsgAddPhone should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      number - the phone number.
 *      text - optional phone text.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneConstruct(RvSdpPhone* phone, const char* number, const char* text)
{
    return rvSdpPhoneConstruct2(NULL,phone,number,text,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpPhoneConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpPhone object.
 *      This function is obsolete. The rvSdpMsgAddPhone should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      number - the phone number.
 *      text - optional phone text.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneConstructA(RvSdpPhone* phone, const char* number, 
                                 const char* text, RvAlloc* alloc)
{
    return rvSdpPhoneConstruct2(NULL,phone,number,text,NULL,alloc,RV_FALSE);
}

/***************************************************************************
 * rvSdpPhoneConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a phone object and copies the values from a source 
 *      phone field. 
 *      This function is obsolete. The rvSdpMsgAddPhone should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to phone to be constructed. Must point 
 *             to valid memory.
 *      src - a source phone object.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneConstructCopy(RvSdpPhone* dest, const RvSdpPhone* src)
{
    return rvSdpPhoneCopy2(dest,src,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpPhoneCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source phone object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination phone. Must point 
 *             to constructed RvSdpPhone object.
 *      src - a source phone object.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneCopy(RvSdpPhone* dest, const RvSdpPhone* src)
{
    return rvSdpPhoneCopy2(dest,src,NULL,RV_TRUE);
}


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxPhoneConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpPhone object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxPhone should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      badSyn - the proprietary format of phone.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpPhone* rvSdpBadSyntaxPhoneConstruct(RvSdpPhone* phone, const char* badSyn)
{
    return rvSdpPhoneConstruct2(NULL,phone,NULL,NULL,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxPhoneConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpPhone object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxPhone should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      badSyn - the proprietary format of phone.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpPhone* rvSdpBadSyntaxPhoneConstructA(RvSdpPhone* phone, const char* badSyn,
                                          RvAlloc* a)
{
    return rvSdpPhoneConstruct2(NULL,phone,NULL,NULL,badSyn,a,RV_FALSE);
}

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/


/***************************************************************************
 * rvSdpPhoneGetNumber
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the phone number.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
const char* rvSdpPhoneGetNumber(const RvSdpPhone* phone)
{
    return RV_SDP_EMPTY_STRING(phone->iPhoneNumber);
}

/***************************************************************************
 * rvSdpPhoneSetNumber
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the phone number.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 *      number - the phone number.
 ***************************************************************************/
RvSdpStatus rvSdpPhoneSetNumber(RvSdpPhone* phone, const char* number)
{
    return rvSdpSetTextField(&phone->iPhoneNumber,phone->iObj,number);
}

/***************************************************************************
 * rvSdpPhoneGetText
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the phone text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
const char* rvSdpPhoneGetText(const RvSdpPhone* phone)
{
    return RV_SDP_EMPTY_STRING(phone->iText);
}

/***************************************************************************
 * rvSdpPhoneSetText
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the phone text.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 *      text - the phone text.
 ***************************************************************************/
RvSdpStatus rvSdpPhoneSetText(RvSdpPhone* phone, const char* text)
{
    return rvSdpSetTextField(&phone->iText,phone->iObj,text);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpPhoneGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted phone field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
const char* rvSdpPhoneGetBadSyntax(const RvSdpPhone* phone)
{
    return RV_SDP_EMPTY_STRING(phone->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpPhoneIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the phone field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
RvBool rvSdpPhoneIsBadSyntax(RvSdpPhone* phone)
{
    return (phone->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpPhoneSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP phone field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpPhone object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpPhoneSetBadSyntax(RvSdpPhone* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

#endif /*#ifndef RV_SDP_USE_MACROS*/

