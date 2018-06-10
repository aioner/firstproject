/***********************************************************************
        Copyright (c) 2002 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

#include "rvstdio.h"
#include "rvnetaddress.h"
#include "rvaddress.h"

#ifdef __cplusplus
extern "C" {
#endif

RVAPI    
RvStatus RVCALLCONV RvNetCreateIpv4(INOUT RvNetAddress* pNetAddress,
                         IN RvNetIpv4* IpV4)
{
    if (NULL!=pNetAddress)
    {
      RvAddressConstructIpv4((RvAddress*)pNetAddress->address, IpV4->ip, IpV4->port);
      return RV_OK;
    }
    return RV_ERROR_NULLPTR;
}


RVAPI
RvStatus RVCALLCONV RvNetCreateIpv6(INOUT RvNetAddress* pNetAddress,
                         IN RvNetIpv6* IpV6)
{
    if (NULL!=pNetAddress)
    {
#if (RV_NET_TYPE & RV_NET_IPV6)
        RvAddressConstructIpv6((RvAddress*)pNetAddress->address, IpV6->ip, IpV6->port, IpV6->scopeId);
        return RV_OK;
#else
		RV_UNUSED_ARG(IpV6);
        return RV_ERROR_NOTSUPPORTED;		
#endif
    }
    return RV_ERROR_NULLPTR;
}

RVAPI
RvStatus RVCALLCONV RvNetGetIpv4(OUT RvNetIpv4* IpV4,
                      IN RvNetAddress* pNetAddress)
{
    if (NULL!=pNetAddress&&NULL!=IpV4)
    {
       RvAddressIpv4* pIpv4 = (RvAddressIpv4*)RvAddressGetIpv4((RvAddress*)pNetAddress->address);
	
	   //20150210 modify by songlei 
	   if (NULL != pIpv4)
		{
			IpV4->ip = pIpv4->ip;
			IpV4->port = pIpv4->port;
			return RV_OK; 
		}

    }
    
    return RV_ERROR_NULLPTR;
}

RVAPI
RvStatus RVCALLCONV RvNetGetIpv6(OUT RvNetIpv6* IpV6,
                      IN RvNetAddress* pNetAddress)
{
    if (NULL!=pNetAddress&&NULL!=IpV6)
    {
#if (RV_NET_TYPE & RV_NET_IPV6)
        RvAddressIpv6* pIpv6 = (RvAddressIpv6 *) RvAddressGetIpv6((RvAddress*)pNetAddress->address);
        memcpy(IpV6->ip, pIpv6->ip, sizeof(IpV6->ip));
        IpV6->port = pIpv6->port;
        IpV6->scopeId = pIpv6->scopeId;
        return RV_OK;
#else
        return RV_ERROR_NOTSUPPORTED;
#endif
    }
    return RV_ERROR_NULLPTR;
}

RVAPI
RvNetAddressType RVCALLCONV RvNetGetAddressType(IN RvNetAddress* pNetAddress)
{
    if (NULL!=pNetAddress)
    { 
        switch(RvAddressGetType((RvAddress*)pNetAddress->address))
        {
        case RV_ADDRESS_TYPE_IPV4:
            return RVNET_ADDRESS_IPV4;
        case RV_ADDRESS_TYPE_IPV6:
            return RVNET_ADDRESS_IPV6;
        default:
            ;
        }
    }
    return RVNET_ADDRESS_NONE;
}

/************************************************************************************
 * RvNetAddressToRvAddress
 * description: converts RvNetAddress structure to RvAddress structure 
 * input:  netAddress  - pointer to RvNetAddress.
 * output  rvAddress    - pointer to filled RvAddress
 * return RvNetAddressType: 
 * RETURN  : RV_OK on success, other on failure
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvNetAddressToRvAddress(
         IN         RvNetAddress*  netAddress,
         INOUT      RvAddress*     rvAddress)
{
    if (NULL == netAddress || NULL == rvAddress)
    {        
        return RV_ERROR_NULLPTR;
    }
    if (RvNetGetAddressType(netAddress) == RVNET_ADDRESS_IPV4)
    {
        RvNetIpv4 IpV4;
                       
        RvNetGetIpv4(&IpV4, netAddress);
        RvAddressConstructIpv4(rvAddress, IpV4.ip, IpV4.port);
    }
#if (RV_NET_TYPE & RV_NET_IPV6)
    else if (RvNetGetAddressType(netAddress) == RVNET_ADDRESS_IPV6)
    {
        RvNetIpv6 IpV6;
                       
        RvNetGetIpv6(&IpV6, netAddress);   
        RvAddressConstructIpv6(rvAddress, IpV6.ip, IpV6.port, IpV6.scopeId);        
    }
#endif
    return RV_OK;
}
 
/************************************************************************************
 * RvAddressToRvNetAddress
 * description: converts RvAddress to RvNetAddress structure used
 *                     in ARTP Toolkit
 * input: rvAddress      - pointer to RvAddress
 * output  netAddress  - pointer to RvNetAddress.
 * return RvNetAddressType: 
 * RETURN  : RV_OK on success, other on failure
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvAddressToRvNetAddress(
         IN          RvAddress*     rvAddress,
         INOUT       RvNetAddress*  netAddress)
{
    if (NULL == netAddress || NULL == rvAddress)
    {        
        return RV_ERROR_NULLPTR;
    }
    if (RvAddressGetType(rvAddress) == RV_ADDRESS_TYPE_IPV4)
    {
        const RvAddressIpv4* ipv4 = RvAddressGetIpv4(rvAddress);
        RvNetIpv4     IpV4;
        IpV4.ip   = ipv4->ip;
        IpV4.port = ipv4->port;        
        RvNetCreateIpv4(netAddress, &IpV4);
    }
#if (RV_NET_TYPE & RV_NET_IPV6)
    else if (RvAddressGetType(rvAddress) == RV_ADDRESS_TYPE_IPV6)
    {
        const RvAddressIpv6* ipv6 = RvAddressGetIpv6(rvAddress);
        RvNetIpv6     IpV6;
        memcpy(IpV6.ip, ipv6->ip, sizeof(ipv6->ip));
        IpV6.port    = ipv6->port;        
        IpV6.scopeId = ipv6->scopeId;  
        RvNetCreateIpv6(netAddress, &IpV6);       
    }
#endif
    return RV_OK;
}

#ifdef __cplusplus
}
#endif


