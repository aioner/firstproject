/******************************************************************************
Filename    :rvsdporigin.c
Description :origin manipulation routines.

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
 *	Sets the internal RvSdpOrigin fields to the supplied values.
 *  Returns the 'origin' of NULL if fails.
 */
RvSdpOrigin* rvSdpOriginFill(   
                RvSdpOrigin *origin,    /* RvSdpOrigin instance */  
                const char* userName,   /* the origin field user name */ 
                const char* sessionId,  /* the origin field session ID */ 
                const char* version,    /* the origin field version */ 
                const char* netType,    /* the origin field network type */ 
                const char* addrType,   /* the origin field address type */ 
                const char* address)    /* the origin field address */              
{
    if (rvSdpSetTextField(&origin->iUserName,origin->iObj,userName) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&origin->iSessionId,origin->iObj,sessionId) != RV_SDPSTATUS_OK || 
        rvSdpSetTextField(&origin->iVersion,origin->iObj,version) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&origin->iAddress,origin->iObj,address) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&origin->iAddrTypeStr,origin->iObj,addrType) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&origin->iNetTypeStr,origin->iObj,netType) != RV_SDPSTATUS_OK)
        return NULL;
    return origin;
}

/***************************************************************************
 * rvSdpOriginConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOrigin object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object.
 *      origin - a pointer to valid RvSdpOrigin object.
 *      username - the origin field user name.
 *      session_id - the origin field session ID.
 *      version - the origin field version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      badSyn - the proprietary formatted origin field or NULL if standard 
 *               origin is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'origin' must point to valid & constructed
 *                      RvSdpOrigin object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpOrigin* 
rvSdpOriginConstruct2(RvSdpMsg *msg, RvSdpOrigin *origin, const char* userName, 
                      const char* sessionId, const char* version, RvSdpNetType netType, 
                      RvSdpAddrType addrType, const char* address, 
                      const char* badSyn, RvAlloc* a, RvBool dontConstruct)
{
    if (!origin)
        /* the RvSdpOrigin instance has to be supplied */
        return NULL;
    
    if (!dontConstruct && RV_SDP_ORIGIN_IS_USED(origin))
        rvSdpOriginDestruct(origin);

    if (!dontConstruct)
    {        
        memset(origin,0,sizeof(RvSdpOrigin));

        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*) a;

        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed 
            in the RvSdpMsg context */
            origin->iObj = msg;
        else
        {
            /* obsolete API usage:
                no msg context given, will be using allocator */
            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            /* save the allocator used */
            origin->iObj = a;
        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&origin->iBadSyntaxField,origin->iObj,badSyn) 
                                                                != RV_SDPSTATUS_OK)
            return NULL;
        goto realReturn;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;
            	
    if (!rvSdpOriginFill(origin,userName,sessionId,version,
                rvSdpNetTypeVal2Txt(netType),rvSdpAddrTypeVal2Txt(addrType),address))
    {
        rvSdpOriginDestruct(origin);
        return NULL;
    }

    goto realReturn; /*keep compiler happy*/

realReturn:
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_ORIGIN,&msg->iOrigin.iLineObj,
            RV_OFFSETOF(RvSdpOrigin,iLineObj));        
    RV_SDP_ORIGIN_SET_USED(origin);

    return origin;   
}

/***************************************************************************
 * rvSdpOriginDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the origin object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
***************************************************************************/
void rvSdpOriginDestruct(RvSdpOrigin* origin)
{
    if (!origin->iObj)
        /* cannot deallocate memory */
        return;
    
    rvSdpUnsetTextField(&origin->iUserName,origin->iObj);
    rvSdpUnsetTextField(&origin->iSessionId,origin->iObj);
    rvSdpUnsetTextField(&origin->iVersion,origin->iObj);
    rvSdpUnsetTextField(&origin->iUserName,origin->iObj);
    rvSdpUnsetTextField(&origin->iAddress,origin->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&origin->iBadSyntaxField,origin->iObj);
#endif
    rvSdpUnsetTextField(&origin->iNetTypeStr,origin->iObj);
    rvSdpUnsetTextField(&origin->iAddrTypeStr,origin->iObj);    
    RV_SDP_ORIGIN_SET_NOT_USED(origin);

    rvSdpLineObjsListRemove(origin->iObj,&origin->iLineObj);
}

/***************************************************************************
 * rvSdpOriginCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpOrigin from 'src' to 'dest'. 
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpOrigin object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpOrigin object. 
 *      src - a pointer to the source object.
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpOrigin* rvSdpOriginCopy2(RvSdpOrigin* dest, const RvSdpOrigin* src, 
                              void* obj, RvBool dontConstruct)
{
    if (!obj && dest && dontConstruct)
        obj = dest->iObj;

    dest = rvSdpOriginConstruct2(NULL,dest,src->iUserName,src->iSessionId,src->iVersion,
        RV_SDPNETTYPE_NOTSET,RV_SDPADDRTYPE_NOTSET,
        src->iAddress,RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj,dontConstruct);
    
    if (dest == NULL)
        return NULL;
    
    
    if (rvSdpSetTextField(&dest->iNetTypeStr,dest->iObj,src->iNetTypeStr) 
                                                            != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&dest->iAddrTypeStr,dest->iObj,src->iAddrTypeStr) 
                                                            != RV_SDPSTATUS_OK)
    {
        rvSdpOriginDestruct(dest);
        return NULL;
    }

    return dest;
}


#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpOriginConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOrigin object.
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      username - the origin field user name.
 *      session_id - the origin field session ID.
 *      version - the origin field version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 ***************************************************************************/
RvSdpOrigin* 
rvSdpOriginConstruct(RvSdpOrigin *origin, const char* username, const char* session_id, 
                     const char* version, RvSdpNetType nettype, RvSdpAddrType addrtype, 
                     const char* address)
{
    return rvSdpOriginConstruct2(NULL,origin,username,session_id,version,nettype,
        addrtype,address,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpOriginConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOrigin object.
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      username - the origin field user name.
 *      session_id - the origin field session ID.
 *      version - the origin field version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpOrigin* 
rvSdpOriginConstructA(RvSdpOrigin *origin, const char* username, const char* session_id, 
                      const char* version, RvSdpNetType nettype, RvSdpAddrType addrtype, 
                      const char* address, RvAlloc* a)
{
    return rvSdpOriginConstruct2(NULL,origin,username,session_id,version,nettype,
        addrtype,address,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpOriginConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a origin object and copies the values from a source 
 *      origin field. 
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to origin to be constructed. Must point 
 *             to valid memory.
 *      src - a source origin object.
 ***************************************************************************/
RvSdpOrigin* rvSdpOriginConstructCopy(RvSdpOrigin* dest, const RvSdpOrigin* src)
{
    return rvSdpOriginCopy2(dest,src,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpOriginConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a origin object and copies the values from a source 
 *      origin field. 
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to origin to be constructed. Must point 
 *             to valid memory.
 *      src - a source origin object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpOrigin* rvSdpOriginConstructCopyA(RvSdpOrigin* dest, const RvSdpOrigin* src,  
                                       RvAlloc* a)
{
    return rvSdpOriginCopy2(dest,src,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpOriginCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source origin object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination origin. Must point 
 *             to constructed RvSdpOrigin object.
 *      src - a source origin object.
 ***************************************************************************/
RvSdpOrigin* rvSdpOriginCopy(RvSdpOrigin* dest, const RvSdpOrigin* src)
{
    return rvSdpOriginCopy2(dest,src,NULL,RV_TRUE);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxOriginConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOrigin object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxOrigin should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      badSyn - the proprietary format of origin field.
 ***************************************************************************/
RvSdpOrigin* rvSdpBadSyntaxOriginConstruct(RvSdpOrigin* origin, const char* badSyn)
{
    return rvSdpOriginConstruct2(NULL,origin,NULL,NULL,NULL,RV_SDPNETTYPE_NOTSET,
        RV_SDPADDRTYPE_NOTSET,NULL,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxOriginConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpOrigin object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxOrigin should be 
 *      used instead.  
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      badSyn - the proprietary format of origin field.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpOrigin* rvSdpBadSyntaxOriginConstructA(RvSdpOrigin* origin, const char* badSyn, 
                                            RvAlloc* a)
{
    return rvSdpOriginConstruct2(NULL,origin,NULL,NULL,NULL,RV_SDPNETTYPE_NOTSET,
        RV_SDPADDRTYPE_NOTSET,NULL,badSyn,a,RV_FALSE);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpOriginGetUsername
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's user name.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetUsername(const RvSdpOrigin* origin)
{
    return RV_SDP_EMPTY_STRING(origin->iUserName);
}

/***************************************************************************
 * rvSdpOriginSetUsername
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field user name.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      username - the origin's field user name.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetUsername(RvSdpOrigin* origin, const char* userName)
{
    return rvSdpSetTextField(&origin->iUserName,origin->iObj,userName);
}

/***************************************************************************
 * rvSdpOriginGetVersion
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's version.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetVersion(const RvSdpOrigin* origin)
{
    return RV_SDP_EMPTY_STRING(origin->iVersion);
}

/***************************************************************************
 * rvSdpOriginSetVersion
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field version.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      version - the origin's field version.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetVersion(RvSdpOrigin* origin, const char* version)
{
    return rvSdpSetTextField(&origin->iVersion,origin->iObj,version);
}


/***************************************************************************
 * rvSdpOriginGetSessionId
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's session ID.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetSessionId(const RvSdpOrigin* origin)
{
    return RV_SDP_EMPTY_STRING(origin->iSessionId);
}

/***************************************************************************
 * rvSdpOriginSetSessionId
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field session ID.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      id - the origin's field session  ID.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetSessionId(RvSdpOrigin* origin, const char* sessionId)
{
    return rvSdpSetTextField(&origin->iSessionId,origin->iObj,sessionId);
}


/***************************************************************************
 * rvSdpOriginGetAddress
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetAddress(const RvSdpOrigin* origin)
{
    return RV_SDP_EMPTY_STRING(origin->iAddress);
}

/***************************************************************************
 * rvSdpOriginSetAddress
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field address.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      addr - the origin's field address.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetAddress(RvSdpOrigin* origin, const char* address)
{
    return rvSdpSetTextField(&origin->iAddress,origin->iObj,address);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpOriginGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted origin field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetBadSyntax(const RvSdpOrigin* origin)
{
    return RV_SDP_EMPTY_STRING(origin->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpOriginIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the origin field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
RvBool rvSdpOriginIsBadSyntax(const RvSdpOrigin* origin)
{
    return (origin->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpOriginSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP origin field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpOrigin object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetBadSyntax(RvSdpOrigin* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpOriginGetNetType
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's network type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
RvSdpNetType rvSdpOriginGetNetType(const RvSdpOrigin* origin)
{
	return rvSdpNetTypeTxt2Val(origin->iNetTypeStr);
}

/***************************************************************************
 * rvSdpOriginSetNetType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field network type.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      netType - the origin's field network type.
 ***************************************************************************/
void rvSdpOriginSetNetType(RvSdpOrigin* origin, RvSdpNetType type)
{
	rvSdpSetTextField(&origin->iNetTypeStr,origin->iObj,rvSdpNetTypeVal2Txt(type));
}

/***************************************************************************
 * rvSdpOriginGetAddressType
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's address type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
RvSdpAddrType rvSdpOriginGetAddressType(const RvSdpOrigin* origin)
{
	return rvSdpAddrTypeTxt2Val(origin->iAddrTypeStr);
}

/***************************************************************************
 * rvSdpOriginSetAddressType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field address type.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      addrType - the origin's field address type.
 ***************************************************************************/
void rvSdpOriginSetAddressType(RvSdpOrigin* origin, RvSdpAddrType type)
{
	rvSdpSetTextField(&origin->iAddrTypeStr,origin->iObj,rvSdpAddrTypeVal2Txt(type));
}

/***************************************************************************
 * rvSdpOriginGetNetTypeStr
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's network type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetNetTypeStr(RvSdpOrigin* origin)
{
	return RV_SDP_EMPTY_STRING(origin->iNetTypeStr);
}

/***************************************************************************
 * rvSdpOriginGetAddressTypeStr
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the origin's address type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
const char* rvSdpOriginGetAddressTypeStr(RvSdpOrigin* origin)
{
	return RV_SDP_EMPTY_STRING(origin->iAddrTypeStr);
}

/***************************************************************************
 * rvSdpOriginSetNetTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field network type string.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      type - the origin's field network type string.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetNetTypeStr(RvSdpOrigin* origin, const char* type)
{
	return rvSdpSetTextField(&origin->iNetTypeStr,origin->iObj,type);
}

/***************************************************************************
 * rvSdpOriginSetAddressTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the origin's field address type string.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      t - the origin's field address type string.
 ***************************************************************************/
RvSdpStatus rvSdpOriginSetAddressTypeStr(RvSdpOrigin* o, const char* t)
{
	return rvSdpSetTextField(&o->iAddrTypeStr,o->iObj,t);
}

#endif /*#ifndef RV_SDP_USE_MACROS*/

