/******************************************************************************
Filename    :rvsdpkey.c
Description : key manipulation routines.

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
 *	Sets the internal RvSdpKey fields to the supplied values.
 *  Returns the 'key' of NULL if fails.
 */
RvSdpKey* 
rvSdpKeyFill(RvSdpKey* key, const char* type, const char* data)
{
    if (rvSdpSetTextField(&key->iTypeStr,key->iObj,type) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&key->iData,key->iObj,data) != RV_SDPSTATUS_OK)
        return NULL;
    return key;
}

/***************************************************************************
 * rvSdpKeyConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpKey object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. 
 *            If NULL the 'a' allocator will be used.
 *      key - a pointer to valid RvSdpKey object.
 *      type - the encryption method type.
 *      data - the encryption data.
 *      badSyn - the proprietary formatted key field or NULL if standard key is 
 *               constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'key' must point to valid & constructed
 *                      RvSdpKey object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpKey* 
rvSdpKeyConstruct2(RvSdpMsg* msg, RvSdpKey* key, RvSdpEncrMethod type, const char* data,
                   const char* badSyn, RvAlloc* a, RvBool dontConstruct)
{
    if (!key)
        /* the RvSdpKey instance has to be supplied */
        return NULL;
    
    if (!dontConstruct && RV_SDP_KEY_IS_USED(key))
        rvSdpKeyDestruct(key);

    if (!dontConstruct)
    {

        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*)a;

        memset(key,0,sizeof(RvSdpKey));
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the RvSdpMsg context */
            key->iObj = msg;
        else
        {
            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();
        
            /* save the allocator used */
            key->iObj = a;
        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&key->iBadSyntaxField,key->iObj,badSyn) != RV_SDPSTATUS_OK)
            return NULL;
        if (msg && !dontConstruct)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_KEY,&key->iLineObj,
                RV_OFFSETOF(RvSdpTZA,iLineObj));
        
        RV_SDP_KEY_SET_USED(key);
        return key;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;

    if (!rvSdpKeyFill(key,rvSdpKeyTypeVal2Txt(type),data))
    {
        rvSdpKeyDestruct(key);
        return NULL;
    }        
    RV_SDP_KEY_SET_USED(key);
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_KEY,&key->iLineObj,
            RV_OFFSETOF(RvSdpTZA,iLineObj));
    return key;   
}

/***************************************************************************
 * rvSdpKeyCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpKey from 'src' to 'dest'. 
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpKey object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpKey object. 
 *      src - a pointer to the source object.
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpKey* rvSdpKeyCopy2(RvSdpKey* dest, const RvSdpKey* src, void* obj, RvBool dontConstruct)

{
    if (!obj && dest && dontConstruct)
        obj = dest->iObj;

    dest = rvSdpKeyConstruct2(obj,dest,RV_SDPENCRMTHD_NOTSET,src->iData,
                RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),NULL,dontConstruct);
    if (dest == NULL)
        return NULL;
    
    if (rvSdpSetTextField(&dest->iTypeStr,dest->iObj,src->iTypeStr) != RV_SDPSTATUS_OK)
    {
        rvSdpKeyDestruct(dest);
        return NULL;
    }
    return dest;
}

/***************************************************************************
 * rvSdpKeyDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the key object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
void rvSdpKeyDestruct(RvSdpKey* key)
{
    if (!key->iObj)
        /* cannot deallocate memory */
        return;

    rvSdpUnsetTextField(&key->iData,key->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&key->iBadSyntaxField,key->iObj);
#endif
    rvSdpUnsetTextField(&key->iTypeStr,key->iObj);
    rvSdpLineObjsListRemove(key->iObj,&key->iLineObj);
    RV_SDP_KEY_SET_NOT_USED(key);
}


#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpKeyConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpKey object.
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      type - the encryption method type.
 *      data - the encryption data.
 ***************************************************************************/
RvSdpKey* rvSdpKeyConstruct(RvSdpKey* key, RvSdpEncrMethod type, const char* data)
{
    return rvSdpKeyConstruct2(NULL,key,type,data,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpKeyConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpKey object.
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      type - the encryption method type.
 *      data - the encryption data.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpKey* rvSdpKeyConstructA(RvSdpKey* key, RvSdpEncrMethod type, const char* data, RvAlloc* a)
{
    return rvSdpKeyConstruct2(NULL,key,type,data,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpKeyConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a key object and copies the values from a source 
 *      key field. 
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to key to be constructed. Must point 
 *             to valid memory.
 *      src - a source key object.
 ***************************************************************************/
RvSdpKey* rvSdpKeyConstructCopy(RvSdpKey* dest, const RvSdpKey* src)
{
    return rvSdpKeyCopy2(dest,src,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpKeyConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a key object and copies the values from a source 
 *      key field. 
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to key to be constructed. Must point 
 *             to valid memory.
 *      src - a source key object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpKey* rvSdpKeyConstructCopyA(RvSdpKey* dest, const RvSdpKey* src, RvAlloc* a)
{
    return rvSdpKeyCopy2(dest,src,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpKeyCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source key object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination key. Must point 
 *             to constructed RvSdpKey object.
 *      src - a key object.
 ***************************************************************************/
RvSdpKey* rvSdpKeyCopy(RvSdpKey* dest, const RvSdpKey* src)
{
    return rvSdpKeyCopy2(dest, src, NULL, RV_TRUE);
}

/**************************************************************************
 * rvSdpKeyGetData
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the key encryption data.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
const char* rvSdpKeyGetData(const RvSdpKey* key)
{
    return RV_SDP_EMPTY_STRING(key->iData);
}

/**************************************************************************
 * rvSdpKeySetData
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the key encryption data.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 *      data - the encryption data.
 ***************************************************************************/
RvSdpStatus rvSdpKeySetData(RvSdpKey* key, const char* data)
{
    return rvSdpSetTextField(&key->iData,key->iObj,data);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpKeyIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the encryption key field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
RvBool rvSdpKeyIsBadSyntax(RvSdpKey* key)
{
    return (key->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpKeyGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted encryption key field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
const char*  rvSdpKeyGetBadSyntax(const RvSdpKey* key)
{
    return RV_SDP_EMPTY_STRING(key->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpBadSyntaxKeyConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpKey object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxKey or  
 *      rvSdpMediaDescrSetBadSyntaxKey should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      badSyn - the proprietary format of repeat interval.
 ***************************************************************************/
RvSdpKey* rvSdpBadSyntaxKeyConstruct(RvSdpKey* key, const char* badSyn)
{
    return rvSdpKeyConstruct2(NULL,key,RV_SDPENCRMTHD_NOTSET,NULL,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxKeyConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpKey object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxKey or  
 *      rvSdpMediaDescrSetBadSyntaxKey should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      badSyn - the proprietary format of key.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpKey* rvSdpBadSyntaxKeyConstructA(RvSdpKey* key, const char* badSyn, RvAlloc* a)
{
    return rvSdpKeyConstruct2(NULL,key,RV_SDPENCRMTHD_NOTSET,NULL,badSyn,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpKeySetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP encryption key field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpKey object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpKeySetBadSyntax(RvSdpKey* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */


/**************************************************************************
 * rvSdpKeyGetType
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the key encryption type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
RvSdpEncrMethod rvSdpKeyGetType(const RvSdpKey* key)
{
	return rvSdpKeyTypeTxt2Val(key->iTypeStr);
}

/**************************************************************************
 * rvSdpKeySetType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the key encryption type.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 *      type - the encryption type.
 ***************************************************************************/
void rvSdpKeySetType(RvSdpKey* key, RvSdpEncrMethod type)
{
	rvSdpSetTextField(&key->iTypeStr,key->iObj,rvSdpKeyTypeVal2Txt(type));
}

/**************************************************************************
 * rvSdpKeyGetTypeStr
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the key encryption type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
const char* rvSdpKeyGetTypeStr(RvSdpKey* key)
{
	return RV_SDP_EMPTY_STRING(key->iTypeStr);
}


/**************************************************************************
 * rvSdpKeySetTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the key encryption type string.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 *      typeStr - the encryption type string.
 ***************************************************************************/
RvSdpStatus rvSdpKeySetTypeStr(RvSdpKey* key, const char* typeStr)
{
	return rvSdpSetTextField(&key->iTypeStr,key->iObj,typeStr);
}


#endif /*#ifndef RV_SDP_USE_MACROS*/

