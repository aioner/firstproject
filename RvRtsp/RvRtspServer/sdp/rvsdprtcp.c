/******************************************************************************
Filename    :rvsdprtcp.c
Description : rtcp attribute manipulation routines.

  ******************************************************************************
  Copyright (c) 2006 RADVision Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include "rvsdpprivate.h"
#include "rvstrutils.h"
#include "rvsdpprsutils.h"
#include "rvsdpdatastruct.h"
#include "rvsdp.h"

#ifdef RV_SDP_RTCP_ATTR

    
/***************************************************************************
 * rvSdpRtcpAttrConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *	Allocates and constructs the object using RvAlloc allocator.
 *  Returns the constructed object or NULL if it was failed.
 .
 * Return Value: 
 *      A pointer to the new added RvSdpRtcpAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          obj - points to RvSdpMsg or RvAlloc instance.
 *          port - the port number of rtcp stream
 *          netType - the network type
 *          addrType - the address type
 *          netType - the address of the rtcp stream
 *          badSyn - proprietary syntax rtcp attribute or NULL.
 ***************************************************************************/
RVSDPCOREAPI RvSdpRtcp* rvSdpRtcpAttrConstruct2(
            void*                   obj,      
            RvInt32                 port,
            const char              *netType,
            const char              *addrType,
            const char              *address)
{
    RvSdpRtcp   *rtcp;
    RvAlloc     *alloc;
    
    /*
     *	unlike other SDP data structures this one does not allocate itself
     *  through dedicated objects pool but directly
     */
    
    /*check which alloactor to use: in the old way or the new way*/
    if (RV_SDP_OBJ_IS_MESSAGE2(obj))
        alloc = ((RvSdpMsg*)obj)->iAllocator;
    else /*old way*/
        alloc = (RvAlloc*) obj;

    if (!alloc)
        alloc = rvSdpGetDefaultAllocator();
    
    /*create the "a=rtcp" attribute*/
    rtcp = rvSdpAllocAllocate(alloc,sizeof(RvSdpRtcp));
    if (!rtcp)
        return NULL;
    
    memset(rtcp,0,sizeof(RvSdpRtcp));
    
    rtcp->iAlloc = alloc;
    
    if (!obj)
        obj = alloc;
    
    rtcp->iObj = obj;
    rtcp->iPort = port;

    if (rvSdpSetTextField(&rtcp->iNetTypeStr,obj,netType) != RV_SDPSTATUS_OK )
        goto failed;
    if (rvSdpSetTextField(&rtcp->iAddrTypeStr,obj,addrType) != RV_SDPSTATUS_OK )
        goto failed;
    if (rvSdpSetTextField(&rtcp->iAddress,obj,address) != RV_SDPSTATUS_OK )
        goto failed;
    
    rtcp->iRtcpAttribute.iSpecAttrData = rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_RTCP);
    return rtcp;
    
failed:
    if (rtcp)
        rvSdpRtcpAttrDestruct(rtcp);
    return NULL;
}


/***************************************************************************
 * rvSdpAddRtcpAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the rtcp atribute to msg obj.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpRtcp object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          msg - message where addition will be performed.
 *          commF - common fields pointer of media descriptor.
 *          port - the port number of rtcp stream
 *          netType - the network type
 *          addrType - the address type
 *          netType - the address of the rtcp stream
 ***************************************************************************/
RVSDPCOREAPI RvSdpRtcp* rvSdpAddRtcpAttr2(
                                   RvSdpMsg                *msg,  
                                   RvSdpCommonFields       *commF,
                                   RvInt32                 port,
                                   const char              *netType,
                                   const char              *addrType,
                                   const char              *address)
{
    RvSdpRtcp* rtcp;
    int buffSize;
    
    buffSize = 0;
    buffSize += (netType)?(int)strlen(netType+2):0;
    buffSize += (addrType)?(int)strlen(addrType+2):0;
    buffSize += (address)?(int)strlen(address+2):0;
    
    rvSdpMsgPromiseBuffer(msg,buffSize);
    
    rtcp = rvSdpRtcpAttrConstruct2(msg,port,netType,addrType,address);
    if (!rtcp)
        return NULL;    
    
    rvSdpAddAttr2(msg,commF,&(rtcp->iRtcpAttribute),"rtcp",NULL);    
    return rtcp;
}


/***************************************************************************
 * rvSdpRtcpAttrCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpRtcp from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpRtcp object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpRtcp object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 ***************************************************************************/
RVSDPCOREAPI RvSdpRtcp* rvSdpRtcpAttrCopy2(
        RvSdpRtcp      *dest, 
        const RvSdpRtcp *src, 
        void *obj)
{
    dest = rvSdpRtcpAttrConstruct2(obj,
                         src->iPort,
                         src->iNetTypeStr,
                         src->iAddrTypeStr,
                         src->iAddress);
    if (dest == NULL)
        return NULL;

        
    if (obj != NULL)
    {
        RvSdpMsg *msg;        
        
        msg = (RV_SDP_OBJ_IS_MESSAGE2(obj))?(RvSdpMsg*)obj:NULL;
        
        (void)rvSdpAttributeConstruct2( msg,
                                       &dest->iRtcpAttribute,
                                        src->iRtcpAttribute.iAttrName,
                                        src->iRtcpAttribute.iAttrValue,
                                        obj,
                                        RV_FALSE);
    }

    return dest;
}

/***************************************************************************
 * rvSdpRtcpAttrDestruct
 * ------------------------------------------------------------------------
 * General: Destroys the instance of RvSdpRtcp.
 *          
 * Return Value:  None.
 *     
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - object to be destroyed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpRtcpAttrDestruct(RvSdpRtcp* rtcp)
{
    rvSdpUnsetTextField(&rtcp->iNetTypeStr,rtcp->iObj);
    rvSdpUnsetTextField(&rtcp->iAddrTypeStr,rtcp->iObj);
    rvSdpUnsetTextField(&rtcp->iAddress,rtcp->iObj);
    
    /* Deallocates RvSdpAttribute netto memory	*/
    rvSdpAttributeDestructNetto(&rtcp->iRtcpAttribute,RV_FALSE);    
    rvSdpAllocDeallocate(rtcp->iAlloc,sizeof(RvSdpRtcp),rtcp);
}

/***************************************************************************
 * rvSdpRtcpAttrGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Prints the textual value of rtcp attribute  into provided 
 *      buffer (all the fields values following "a=rtcp:_________________ ".
 *          
 * Return Value: 
 *      Returns the buffer pointer on success or NULL if fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          rtcp - instance of RvSdpRtcp.
 *          txtVal - the buffer for the value.
 ***************************************************************************/
RVSDPCOREAPI char* rvSdpRtcpAttrGetValue(
    RvSdpRtcp* rtcp, 
    char *txtVal)
{
    if (rtcp->iNetTypeStr && rtcp->iAddrTypeStr && rtcp->iAddress)
        sprintf(txtVal,"%d %s %s %s",rtcp->iPort,rtcp->iNetTypeStr,rtcp->iAddrTypeStr,rtcp->iAddress);    
    else
        sprintf(txtVal,"%d",rtcp->iPort);        
    return txtVal;

}

/***************************************************************************
 * rvSdpRtcpAttrReshuffle
 * ------------------------------------------------------------------------
 * General: 
 *      Called during message strings buffer reshuffle.
 *      Copies all string fields of rtcp attribute into new '*ptr' while
 *      increasing '*ptr' value by the length of copied strings. 
 *      reshuffle- is called when 
 * Return Value: void
 ***************************************************************************/
void rvSdpRtcpAttrReshuffle(RvSdpRtcp* rtcp, char** ptr)
{
	rvSdpChangeText(ptr,&rtcp->iNetTypeStr);
    rvSdpChangeText(ptr,&rtcp->iAddrTypeStr);
    rvSdpChangeText(ptr,&rtcp->iAddress);
}


extern RvBool rvSdpParseNetType_AddrType_Addr(char** ptr, char** netTypeTxt, char** addrTypeTxt, char** addr, 
                                       RvSdpParseWarning* w, RvSdpParserData* pD, REPLACING_DECL);
/***************************************************************************
 * rvSdpRtcpAttrParsing
 * ------------------------------------------------------------------------
 *	   Special attribute  parse function for the rtcp attribute. 
 *     Parses rtcp attribute field and constructs the RvSdpRtcp object.
 *     The constructed object is added to current context media descriptor
 *
 *
 * Return Value: 
 *     If parsing or construction fails the correspondent status is returned.
 ***************************************************************************/
RvSdpSpecAttrParseSts rvSdpRtcpAttrParsing(
				const RvSdpSpecAttributeData* specAttrData, /* the special attribute data */
                RvBool              createAsBadSyntax, /* if set to RV_TRUE the bad 
                                                          syntax rtcp attribute  
                                                          will be created */
                RvSdpParserData     *pD,   /* the parser data instance */        
                RvSdpCommonFields   *commonFields, /* the current context common fields instance,
                                                     here the special attribute will be added */
				char                *name, /* the attribute name */
                char                *val,  /* the attribute value to parse */
                REPLACING_DECL)           /* used for zero-substitutions in 
                                             input buffer */
{
    RvSdpSpecAttrParseSts retv;
	RV_UNUSED_ARG(name);
	RV_UNUSED_ARG(specAttrData);


	if (createAsBadSyntax)
	{
        return rvSdpSpAttrPrsCrtRegular;
	}
	else 
	{
        RvUint port;
        RvChar *p, *pp, *netType=NULL, *addrType=NULL, *address=NULL;
        RvSdpParseWarning w;
        
        retv = rvSdpSpAttrPrsCrtRegular;
        p = val;

        if (!rvSdpParseInt(0,0,&port,RV_TRUE,RV_FALSE,&p,NULL))
            return retv;

        if (p && *p)
        {
            pp = p;    
            if (!rvSdpParseSpace(&p))
                return retv;

            if (!rvSdpParseNetType_AddrType_Addr(&p,&netType,&addrType,&address,&w,pD,REPLACING_ARGS2))
                return retv;
        }
        
        rvSdpClearSpecialAttr(commonFields,SDP_FIELDTYPE_RTCP);

        /*add the rtcp attribute */
        if (rvSdpAddRtcpAttr2(pD->iMsg,
                                   commonFields,
                                   port,
                                   netType,
                                   addrType,
                                   address) == NULL)
        {
            return rvSdpSpAttrPrsAllocFail;
        }
        retv = rvSdpSpAttrPrsOK;
	}
	return retv;	
}


/***************************************************************************
 * rvSdpMediaDescrGetRtcp
 * ------------------------------------------------------------------------
 * General:
 *      Return the rtcp special attribute of the media descriptor or NULL if 
 *      it is not set.
 *
 * Return Value:
 *      Returns the rtcp special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpRtcp* rvSdpMediaDescrGetRtcp(
				RvSdpMediaDescr* descr)
{
    return (RvSdpRtcp*)rvSdpGetSpecialAttr(&descr->iCommonFields,0,SDP_FIELDTYPE_RTCP);
}

/***************************************************************************
 * rvSdpMediaDescrSetRtcp
 * ------------------------------------------------------------------------
 * General:
 *      Sets/modifies the rtcp special attribute of the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      port - the port number of the RTCP stream
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP
 *             address for an IP network, and so on.
***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetRtcp(
				RvSdpMediaDescr* descr,
                RvInt32 port,
                RvSdpNetType netType,
                RvSdpAddrType addrType,
                const char* addr)
{
    if (rvSdpAddRtcpAttr2(descr->iSdpMsg,&descr->iCommonFields,port,
         rvSdpNetTypeVal2Txt(netType),rvSdpAddrTypeVal2Txt(addrType),addr) == NULL)
         return RV_SDPSTATUS_ALLOCFAIL;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMediaDescrDestroyRtcp
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the rtcp special attribute of the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMediaDescrDestroyRtcp(
				RvSdpMediaDescr* descr)
{
    rvSdpClearSpecialAttr(&descr->iCommonFields,SDP_FIELDTYPE_RTCP);
}


/***************************************************************************
 * rvSdpRtcpGetPort
 * ------------------------------------------------------------------------
 * General:
 *      Gets the media port number.
 *
 * Return Value:
 *      The port number of rtcp stream
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 ***************************************************************************/
RVSDPCOREAPI RvInt32 rvSdpRtcpGetPort(
                    const RvSdpRtcp* rtcp)
{
    return rtcp->iPort;
}

/***************************************************************************
 * rvSdpRtcpSetPort
 * ------------------------------------------------------------------------
 * General:
 *      Sets the port number of the rtcp stream.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpRtcp object.
 *      port - the new value of the rtcp port number.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpRtcpSetPort(
                    RvSdpRtcp* rtcp,
                    RvInt32 port)
{
    rtcp->iPort = port;
}

/***************************************************************************
 * rvSdpRtcpGetNetType
 * ------------------------------------------------------------------------
 * General:
 *      Get the rtcp's network type.
 * Return Value:
 *      Returns the rtcp's network type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpNetType rvSdpRtcpGetNetType(
                const RvSdpRtcp* rtcp)
{
    return rvSdpNetTypeTxt2Val(rtcp->iNetTypeStr);
}

/***************************************************************************
 * rvSdpRtcpSetNetType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the rtcp's field network type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 *      netType - the rtcp's field network type.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpRtcpSetNetType(
                RvSdpRtcp* rtcp,
                RvSdpNetType netType)
{
    rvSdpSetTextField(&rtcp->iNetTypeStr,rtcp->iObj,rvSdpNetTypeVal2Txt(netType));
}


/***************************************************************************
 * rvSdpRtcpGetNetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the rtcp's network type string.
 * Return Value:
 *      Returns the rtcp's network type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpRtcpGetNetTypeStr(
                RvSdpRtcp* rtcp)
{
    return rtcp->iNetTypeStr;
}

/***************************************************************************
 * rvSdpRtcpSetNetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the rtcp's field network type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 *      type - the rtcp's field network type string.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpRtcpSetNetTypeStr(
                RvSdpRtcp* rtcp,
                const char* type)
{
    return rvSdpSetTextField(&rtcp->iNetTypeStr,rtcp->iObj,type);
}

/***************************************************************************
 * rvSdpRtcpGetAddressType
 * ------------------------------------------------------------------------
 * General:
 *      Get the rtcp's address type.
 * Return Value:
 *      Returns the rtcp's address type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpAddrType rvSdpRtcpGetAddressType(
                const RvSdpRtcp* rtcp)
{
    return rvSdpAddrTypeTxt2Val(rtcp->iAddrTypeStr);
}

/***************************************************************************
 * rvSdpRtcpSetAddressType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the rtcp's field address type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 *      addrType - the rtcp's field address type.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpRtcpSetAddressType(
                RvSdpRtcp* rtcp,
                RvSdpAddrType addrType)
{
    rvSdpSetTextField(&rtcp->iAddrTypeStr,rtcp->iObj,rvSdpAddrTypeVal2Txt(addrType));
}

/***************************************************************************
 * rvSdpRtcpGetAddressTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the rtcp's address type string.
 * Return Value:
 *      Returns the rtcp's address type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpRtcpGetAddressTypeStr(
				RvSdpRtcp* rtcp)
{
    return rtcp->iAddrTypeStr;
}

/***************************************************************************
 * rvSdpRtcpSetAddressTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the rtcp's field address type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 *      t - the rtcp's field address type string.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpRtcpSetAddressTypeStr(
                RvSdpRtcp* rtcp,
                const char* t)
{
    return rvSdpSetTextField(&rtcp->iAddrTypeStr,rtcp->iObj,t);
}

/***************************************************************************
 * rvSdpRtcpGetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Get the rtcp's address.
 * Return Value:
 *      Returns the rtcp's address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpRtcpGetAddress(
                const RvSdpRtcp* rtcp)
{
    return rtcp->iAddress;
}

/***************************************************************************
 * rvSdpRtcpSetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Sets the rtcp's field address.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtcp - a pointer to the RvSdpRtcp object.
 *      addr - the rtcp's field address.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpRtcpSetAddress(
                RvSdpRtcp* rtcp,
                const char* addr)
{
    return rvSdpSetTextField(&rtcp->iAddress,rtcp->iObj,addr);
}



#endif /*#ifdef RV_SDP_RTCP_ATTR*/
