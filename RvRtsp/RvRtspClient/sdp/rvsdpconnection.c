/******************************************************************************
Filename    :rvsdpconnection.c
Description :connection manipulation routines.

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
 *	To allocate the memory for RvSdpConnection object (called by the pool)
 *  Return: 
 *      valid RvSdpConnection pointer
 *      or NULL if fails
 */
RvSdpConnection* rvSdpConnectionCreateByPool(RvSdpMsg* msg)
{
    RvSdpConnection* conn;
    
    conn = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpConnection));
    if (!conn)
        return NULL;
    
    memset(conn,0,sizeof(RvSdpConnection));   
    conn->iObj = msg;
    
    return conn;
}

/*
 *	To free the memory for RvSdpConnection object (called by the pool)
 */
void rvSdpConnectionDestroyByPool(RvSdpConnection* p)
{    
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpConnection),p);
}

/*
 *	Sets the internal RvSdpConnection fields to the supplied values.
 *  Returns the 'conn' of NULL if fails.
 */
RvSdpConnection* 
rvSdpConnectionFill(RvSdpConnection* conn, const char* addr, const char* addrType, 
                    const char* netType, int numAddr, int ttl)
{
    conn->iTtl = ttl; 
    conn->iNumAddr = numAddr;       
    
    if (rvSdpSetTextField(&conn->iAddress,conn->iObj,addr) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&conn->iAddrTypeStr,conn->iObj,addrType) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&conn->iNetTypeStr,conn->iObj,netType) != RV_SDPSTATUS_OK)
        return NULL;
    return conn; 
}

/***************************************************************************
 * rvSdpConnectionConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpConnection object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'conn' is NULL the connection will be allocated from
 *            the 'msg' pool of connections. If 'msg' is not NULL the constructed
 *            connection will be appended to 'msg' list of connections. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      conn - a pointer to valid RvSdpConnection object or NULL.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      badSyn - the proprietary formatted connection field or NULL if standard 
 *               connection is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'conn' must point to valid & constructed
 *                      RvSdpConnection object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpConnection* 
rvSdpConnectionConstruct2(RvSdpMsg* msg, RvSdpConnection* conn, RvSdpNetType nettype,
                          RvSdpAddrType addrtype, const char* address, const char* badSyn, 
                          RvAlloc* a, RvBool dontConstruct)
{

    if (!dontConstruct)
    {
        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*) a;
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed in the RvSdpMsg context */
        {            
        
            if (conn)
                /* the 'conn' can't be set it has to be allocated from the msg connections pool */
                return NULL;
        
            conn = rvSdpPoolTake(&msg->iConnectionsPool);
            if (!conn)
                /* failed to allocate from the msg connections pool */
                return NULL;
        
            memset(conn,0,sizeof(RvSdpConnection));
        
            conn->iObj = msg;
        }
        else 
        {
            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!conn)
                /* the RvSdpConnection instance has to be supplied */
                return NULL;
        
            memset(conn,0,sizeof(RvSdpConnection));      
            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            /* save the allocator used */
            conn->iObj = a;
        }
    }

        
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&conn->iBadSyntaxField,conn->iObj,badSyn) != RV_SDPSTATUS_OK)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iConnectionsPool,conn);
            return NULL;
        }
        if (msg && !dontConstruct)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_CONNECTION,&conn->iLineObj,
                    RV_OFFSETOF(RvSdpConnection,iLineObj));                    
        return conn;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;

    if (msg)
    {
        int l = 0;
        l += (address) ? strlen(address):0;
        l += 60; /* addrtype and nettype cannot exceed it */
        rvSdpMsgPromiseBuffer(msg,l);
    }

    if (!rvSdpConnectionFill(conn,address,rvSdpAddrTypeVal2Txt(addrtype),
        rvSdpNetTypeVal2Txt(nettype),RV_SDP_INT_NOT_SET,RV_SDP_INT_NOT_SET))
    {
        if (msg && !dontConstruct)
            rvSdpPoolReturn(&msg->iConnectionsPool,conn);
        return NULL;
    }
    
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_CONNECTION,&conn->iLineObj,
            RV_OFFSETOF(RvSdpConnection,iLineObj));                    
    return conn;   
}

/***************************************************************************
 * rvSdpConnectionConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a connection object and copies the values from a source 
 *      connection field. 
 *      This function is obsolete. The rvSdpMsgAddConnection or  
 *      rvSdpMediaDescrAddConnection should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to connection to be constructed. Must point 
 *             to valid memory.
 *      src - a source connection object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpConnection* rvSdpConnectionConstructCopyA(RvSdpConnection* dest, const RvSdpConnection* src, RvAlloc* a)
{
    return rvSdpConnectionCopy2(dest,src,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpConnectionCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpConnection from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpConnection object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpConnection object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpConnection* rvSdpConnectionCopy2(RvSdpConnection* dest, const RvSdpConnection* src, 
                                      void* obj, RvBool dontConstruct)
{
    if (!obj && dest && dontConstruct)
        obj = dest->iObj;

    dest = rvSdpConnectionConstruct2(NULL,dest,RV_SDPNETTYPE_NOTSET,RV_SDPADDRTYPE_NOTSET,
                            src->iAddress,RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),
                            obj,dontConstruct);
    if (dest == NULL)
        return NULL;
    
    if (rvSdpSetTextField(&dest->iNetTypeStr,dest->iObj,src->iNetTypeStr) != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&dest->iAddrTypeStr,dest->iObj,src->iAddrTypeStr) != RV_SDPSTATUS_OK)
    {
        rvSdpConnectionDestruct(dest);
        return NULL;
    }
    dest->iTtl = src->iTtl;
    dest->iNumAddr = src->iNumAddr;
    
    return dest;
}

/***************************************************************************
 * rvSdpConnectionDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the connection object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
***************************************************************************/
void rvSdpConnectionDestruct(RvSdpConnection* conn)
{
    if (!conn->iObj)
        /* cannot deallocate memory */
        return;

    rvSdpUnsetTextField(&conn->iAddress,conn->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&conn->iBadSyntaxField,conn->iObj);
#endif
    rvSdpUnsetTextField(&conn->iNetTypeStr,conn->iObj);
    rvSdpUnsetTextField(&conn->iAddrTypeStr,conn->iObj);
    
    rvSdpLineObjsListRemove(conn->iObj,&conn->iLineObj);
    
    if (RV_SDP_OBJ_IS_MESSAGE(conn))
        rvSdpPoolReturn(&((RvSdpMsg*)(conn->iObj))->iConnectionsPool,conn);
}


#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpConnectionConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpConnection object.
 *      This function is obsolete. The rvSdpMsgAddConnection or  
 *      rvSdpMediaDescrAddConnection should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 ***************************************************************************/
RvSdpConnection* 
rvSdpConnectionConstruct(RvSdpConnection* conn, RvSdpNetType nettype, RvSdpAddrType addrtype, const char* address)
{
    return rvSdpConnectionConstruct2(NULL,conn,nettype,addrtype,address,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpConnectionConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpConnection object.
 *      This function is obsolete. The rvSdpMsgAddConnection or  
 *      rvSdpMediaDescrAddConnection should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpConnection* rvSdpConnectionConstructA(RvSdpConnection* conn, RvSdpNetType nettype, RvSdpAddrType addrtype,
                                           const char* address, RvAlloc* a)
{
    return rvSdpConnectionConstruct2(NULL,conn,nettype,addrtype,address,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpConnectionConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a connection object and copies the values from a source 
 *      connection field. 
 *      This function is obsolete. The rvSdpMsgAddConnection or  
 *      rvSdpMediaDescrAddConnection should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to connection to be constructed. Must point 
 *             to valid memory.
 *      src - a source connection object.
 ***************************************************************************/
RvSdpConnection* rvSdpConnectionConstructCopy(RvSdpConnection* dest, const RvSdpConnection* src)
{
    return rvSdpConnectionCopy2(dest,src,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpConnectionCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source connection object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination connection. Must point 
 *             to constructed RvSdpConnection object.
 *      src - a source connection object.
 ***************************************************************************/
RvSdpConnection* rvSdpConnectionCopy(RvSdpConnection* dest, const RvSdpConnection* src)
{
    return rvSdpConnectionCopy2(dest,src,NULL,RV_TRUE);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxConnectionConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpConnection object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxConnection or  
 *      rvSdpMediaDescrSetBadSyntaxConnection should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      badSyn - the proprietary format of connection.
 ***************************************************************************/
RvSdpConnection* rvSdpBadSyntaxConnectionConstruct(RvSdpConnection* conn, 
                                                   const char* badSyn)
{
    return rvSdpConnectionConstruct2(NULL,conn,RV_SDPNETTYPE_NOTSET,
        RV_SDPADDRTYPE_NOTSET,NULL,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxConnectionConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpConnection object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxConnection or  
 *      rvSdpMediaDescrSetBadSyntaxConnection should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      badSyn - the proprietary format of connection.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpConnection* rvSdpBadSyntaxConnectionConstructA(RvSdpConnection* conn, 
                                                    const char* badSyn, RvAlloc* a)
{
    return rvSdpConnectionConstruct2(NULL,conn,RV_SDPNETTYPE_NOTSET,
        RV_SDPADDRTYPE_NOTSET,NULL,badSyn,a,RV_FALSE);
}

#endif /*#if defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/***************************************************************************
 * rvSdpConnectionGetAddress
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
const char* rvSdpConnectionGetAddress(const RvSdpConnection* conn)
{
    return RV_SDP_EMPTY_STRING(conn->iAddress);
}

/***************************************************************************
 * rvSdpConnectionGetAddressTTL
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's address TTL.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
int rvSdpConnectionGetAddressTTL(const RvSdpConnection* conn)
{
    return conn->iTtl;
}

/***************************************************************************
 * rvSdpConnectionSetAddressTTL
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's address TTL.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      ttl - the connection's address TTL.
 ***************************************************************************/
void rvSdpConnectionSetAddressTTL(RvSdpConnection* conn, int ttl)
{
    conn->iTtl = ttl;
}

/***************************************************************************
 * rvSdpConnectionGetAddressNum
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's number of subsequent addresses.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
int rvSdpConnectionGetAddressNum(const RvSdpConnection* conn)
{
    return conn->iNumAddr;
}

/***************************************************************************
 * rvSdpConnectionSetAddressNum
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's number of subsequent addresses.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      num - the connection's number of subsequent addresses.
 ***************************************************************************/
void rvSdpConnectionSetAddressNum(RvSdpConnection* conn, int num)
{
    conn->iNumAddr = num;
}

/***************************************************************************
 * rvSdpConnectionSetAddress
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's address.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      addr - the connection's address
 ***************************************************************************/
RvSdpStatus rvSdpConnectionSetAddress(RvSdpConnection* conn, const char* addr)
{
    return rvSdpSetTextField(&conn->iAddress,conn->iObj,addr);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpConnectionIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the connection field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
RvBool rvSdpConnectionIsBadSyntax(const RvSdpConnection* conn)
{
    return (conn->iBadSyntaxField != NULL);
}
               
/***************************************************************************
 * rvSdpConnectionGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted connection field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
const char* rvSdpConnectionGetBadSyntax(const RvSdpConnection* conn)
{
    return RV_SDP_EMPTY_STRING(conn->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpConnectionSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP connection field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpConnection object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpConnectionSetBadSyntax(RvSdpConnection* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/***************************************************************************
 * rvSdpConnectionGetNetType
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's network type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
RvSdpNetType rvSdpConnectionGetNetType(const RvSdpConnection* conn)
{
	return rvSdpNetTypeTxt2Val(conn->iNetTypeStr);
}

/***************************************************************************
 * rvSdpConnectionSetNetType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's network type.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the network type.
 ***************************************************************************/
void rvSdpConnectionSetNetType(RvSdpConnection* conn, RvSdpNetType type)
{
	rvSdpSetTextField(&conn->iNetTypeStr,conn->iObj,rvSdpNetTypeVal2Txt(type));
}

/***************************************************************************
 * rvSdpConnectionGetAddrType
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's address type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
RvSdpAddrType rvSdpConnectionGetAddrType(const RvSdpConnection* conn)
{
	return rvSdpAddrTypeTxt2Val(conn->iAddrTypeStr);
}

/***************************************************************************
 * rvSdpConnectionSetAddrType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's address type.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the address type.
 ***************************************************************************/
void rvSdpConnectionSetAddrType(RvSdpConnection* conn, RvSdpAddrType type)
{
	rvSdpSetTextField(&conn->iAddrTypeStr,conn->iObj,rvSdpAddrTypeVal2Txt(type));
}

/***************************************************************************
 * rvSdpConnectionGetNetTypeStr
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's network type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
const char* rvSdpConnectionGetNetTypeStr(RvSdpConnection* conn)
{
	return RV_SDP_EMPTY_STRING(conn->iNetTypeStr);
}

/***************************************************************************
 * rvSdpConnectionGetAddrTypeStr
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the connection's address type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
const char* rvSdpConnectionGetAddrTypeStr(RvSdpConnection* conn)
{
	return RV_SDP_EMPTY_STRING(conn->iAddrTypeStr);
}

/***************************************************************************
 * rvSdpConnectionSetNetTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's network type string.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the network type.
 ***************************************************************************/
RvSdpStatus rvSdpConnectionSetNetTypeStr(RvSdpConnection* conn, const char* type)
{
	return rvSdpSetTextField(&conn->iNetTypeStr,conn->iObj,type);
}

/***************************************************************************
 * rvSdpConnectionSetAddrTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the connection's address type string.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the address type string.
 ***************************************************************************/
RvSdpStatus rvSdpConnectionSetAddrTypeStr(RvSdpConnection* c, const char* t)
{
	return rvSdpSetTextField(&c->iAddrTypeStr,c->iObj,t);
}

#endif /*#ifndef RV_SDP_USE_MACROS*/

