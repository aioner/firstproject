/******************************************************************************
Filename    :rvsdpemail.c
Description :email  manipulation routines.

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
 *	Sets the internal RvSdpEmail fields to the supplied values.
 *  Returns the 'email' of NULL if fails.
 */
RvSdpEmail* 
rvSdpEmailFill(RvSdpEmail* email, const char* address, 
               const char* text, RvChar separSymbol)
{
    email->iSeparSymbol = separSymbol;
    
    if (rvSdpSetTextField(&email->iAddress,email->iObj,address) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&email->iText,email->iObj,text) != RV_SDPSTATUS_OK)
        return NULL;
    return email;
}

/***************************************************************************
 * rvSdpEmailConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpEmail object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'email' is NULL the email will be allocated from
 *            the 'msg' pool of emails. If 'msg' is not NULL the constructed
 *            email will be appended to 'msg' list of emails. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      email - a pointer to valid RvSdpEmail object or NULL.
 *      address - the email address.
 *      text - optional email text.
 *      badSyn - the proprietary formatted email field or NULL if standard 
 *               email is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'email' must point to valid & constructed
 *                      RvSdpEmail object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpEmail* 
rvSdpEmailConstruct2(RvSdpMsg* msg, RvSdpEmail* email, const char* address, 
                     const char* text, const char* badSyn, RvAlloc* a, 
                     RvBool dontConstruct)
{        
    if (text && !*text)
        text = NULL;
    
    if (!dontConstruct)
    {
        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*)a;
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the RvSdpMsg context */
        {            
            if (email)
                /* the 'email' can't be set it has to be allocated from the msg emails pool */
                return NULL;
        
            email = rvSdpPoolTake(&msg->iEmailsPool);
            if (!email)
                /* failed to allocate from the msg emails pool */
                return NULL;
            memset(email,0,sizeof(RvSdpEmail));      
            email->iObj = msg;
        }
        else
        {
            if (!email)
                /* the RvSdpEmail instance has to be supplied */
                return NULL;

            memset(email,0,sizeof(RvSdpEmail));        
        
            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            /* save the allocator used */
            email->iObj = a;

        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&email->iBadSyntaxField,email->iObj,badSyn) != RV_SDPSTATUS_OK)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iEmailsPool,email);
            return NULL;
        }
        if (msg && !dontConstruct)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_EMAIL,&email->iLineObj,
                RV_OFFSETOF(RvSdpEmail,iLineObj));            
        return email;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;

    rvSdpMsgPromiseBuffer(msg,((address)?(int)strlen(address):0) + ((text)?(int)strlen(text):0)+10);


    if (!rvSdpEmailFill(email,address,text,'('))
    {
        rvSdpEmailDestruct(email);
        return NULL;
    }
    
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_EMAIL,&email->iLineObj,
                        RV_OFFSETOF(RvSdpEmail,iLineObj));            
    
    return email;   
}

/***************************************************************************
 * rvSdpEmailConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a email object and copies the values from a source 
 *      email field. 
 *      This function is obsolete. The rvSdpMsgAddEmail should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to email to be constructed. Must point 
 *             to valid memory.
 *      src - a source email object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailConstructCopyA(RvSdpEmail* dest, const RvSdpEmail* source, RvAlloc* a)
{
    return rvSdpEmailCopy2(dest,source,a,RV_FALSE);
}

/*
 *	To allocate the memory for RvSdpEmail object (called by the pool)
 *  Return: 
 *      valid RvSdpEmail pointer
 *      or NULL if fails
 */
RvSdpEmail* rvSdpEmailCreateByPool(RvSdpMsg* msg)
{
    RvSdpEmail* e;
    
    e = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpEmail));
    if (!e)
        return NULL;
    
    memset(e,0,sizeof(RvSdpEmail));   
    e->iObj = msg;
    
    return e;
}

/***************************************************************************
 * rvSdpEmailCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpEmail from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpEmail object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpEmail object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailCopy2(RvSdpEmail* dest, const RvSdpEmail* src, void* obj, RvBool dontConstruct)
{
    RvSdpEmail* e;
    if (!obj && dest && dontConstruct)
        obj = dest->iObj;
    e = rvSdpEmailConstruct2(NULL,dest,src->iAddress,src->iText,
                    RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj,dontConstruct);
    if (e)
        e->iSeparSymbol = src->iSeparSymbol;
    return e;
}


/*
 *	To free the memory for RvSdpEmail object (called by the pool)
 */
void rvSdpEmailDestroyByPool(RvSdpEmail* p)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpEmail),p);
}

/***************************************************************************
 * rvSdpEmailDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the email object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
void rvSdpEmailDestruct(RvSdpEmail *email)
{
    if (!email->iObj)
        /* cannot deallocate memory */
        return;

    rvSdpUnsetTextField(&email->iAddress,email->iObj);
    rvSdpUnsetTextField(&email->iText,email->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&email->iBadSyntaxField,email->iObj);
#endif

    rvSdpLineObjsListRemove(email->iObj,&email->iLineObj);
    
    if (RV_SDP_OBJ_IS_MESSAGE(email))
        rvSdpPoolReturn(&((RvSdpMsg*)(email->iObj))->iEmailsPool,email);
}

#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpEmailConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpEmail object.
 *      This function is obsolete. The rvSdpMsgAddEmail should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      address - the email address.
 *      text - optional email text.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailConstruct(RvSdpEmail* email, const char* address, const char* text)
{
    return rvSdpEmailConstruct2(NULL,email,address,text,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpEmailConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpEmail object.
 *      This function is obsolete. The rvSdpMsgAddEmail should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      address - the email address.
 *      text - optional email text.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailConstructA(RvSdpEmail* email, const char* address, const char* text, RvAlloc* a)
{
    return rvSdpEmailConstruct2(NULL,email,address,text,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpEmailConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a email object and copies the values from a source 
 *      email field. 
 *      This function is obsolete. The rvSdpMsgAddEmail should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to email to be constructed. Must point 
 *             to valid memory.
 *      src - a source email object.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailConstructCopy(RvSdpEmail* dest, const RvSdpEmail* source)
{
    return rvSdpEmailCopy2(dest,source,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpEmailCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source email object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination email. Must point 
 *             to constructed RvSdpEmail object.
 *      src - a source email object.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailCopy(RvSdpEmail* dest, const RvSdpEmail* source)
{
    return rvSdpEmailCopy2(dest,source,NULL,RV_TRUE);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxEmailConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpEmail object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxEmail should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      badSyn - the proprietary format of email.
 ***************************************************************************/
RvSdpEmail* rvSdpBadSyntaxEmailConstruct(RvSdpEmail* email, const char* badSyn)
{
    return rvSdpEmailConstruct2(NULL,email,NULL,NULL,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxEmailConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpEmail object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxEmail should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      badSyn - the proprietary format of email.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpEmail* rvSdpBadSyntaxEmailConstructA(RvSdpEmail* email, const char* badSyn, RvAlloc* a)
{
    return rvSdpEmailConstruct2(NULL,email,NULL,NULL,badSyn,a,RV_FALSE);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpEmailGetAddress
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the email address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
const char* rvSdpEmailGetAddress(const RvSdpEmail* email)
{
    return RV_SDP_EMPTY_STRING(email->iAddress);
}

/***************************************************************************
 * rvSdpEmailSetAddress
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the email address.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 *      addr - the email address.
 ***************************************************************************/
RvSdpStatus rvSdpEmailSetAddress(RvSdpEmail* email, const char* addr)
{
    return rvSdpSetTextField(&email->iAddress,email->iObj,addr);
}

/***************************************************************************
 * rvSdpEmailGetText
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the email optional text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
const char* rvSdpEmailGetText(const RvSdpEmail* email)
{
    return RV_SDP_EMPTY_STRING(email->iText);
}

/***************************************************************************
 * rvSdpEmailSetText
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the email text.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 *      text - the email text.
 ***************************************************************************/
RvSdpStatus rvSdpEmailSetText(RvSdpEmail* email, const char* text)
{
    return rvSdpSetTextField(&email->iText,email->iObj,text);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpEmailIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the email field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
RvBool rvSdpEmailIsBadSyntax(RvSdpEmail* email)
{
    return (email->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpEmailGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted email field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
const char* rvSdpEmailGetBadSyntax(const RvSdpEmail* email)
{
    return RV_SDP_EMPTY_STRING(email->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpEmailSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP email value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpEmail object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpEmailSetBadSyntax(RvSdpEmail* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

#endif /*#ifndef RV_SDP_USE_MACROS*/

