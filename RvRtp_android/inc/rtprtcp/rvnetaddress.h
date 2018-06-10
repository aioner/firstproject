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

#ifndef __RV_NET_ADDRESS_H
#define __RV_NET_ADDRESS_H

#include "rvtypes.h"
#include "rverror.h"
#include "rvnettypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	RVNET_ADDRESS_NONE = 0,
	RVNET_ADDRESS_IPV4,
	RVNET_ADDRESS_IPV6

} RvNetAddressType;

typedef struct  {
	RvUint32 ip;		/* 4 byte IP address, network format */
	RvUint16 port;		/* 2 byte port number, host format   */
} RvNetIpv4;

typedef struct {
    RvUint8 ip[16];     /* 16 byte IP address, network format */
    RvUint16 port;      /* 2 byte port number, host format    */
    RvUint32 scopeId;   /* 4 bytes of interface for a scope   */
} RvNetIpv6;

typedef struct  {
	RvUint8 address[32]; 
} RvNetAddress;

/************************************************************************************
 * RvNetCreateIpv4
 * description: RvNetCreateIpv4 constructs RvNetAddress from RvNetIpv4 (IPV4 network address)
 * input:  IpV4        - pointer to IPV4 address
 * output: netAddress  - pointer to RvNetAddress.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/    
RVAPI
RvStatus RVCALLCONV RvNetCreateIpv4(INOUT RvNetAddress* netAddress,
                         IN RvNetIpv4* IpV4);

/************************************************************************************
 * RvNetCreateIpv6
 * description: RvNetCreateIpv6 constructs RvNetAddress from RvNetIpv6 (IPV6 network address)
 * input:  IpV6        - pointer to IPV6 address
 * output: netAddress  - pointer to RvNetAddress.
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/   
RVAPI
RvStatus RVCALLCONV RvNetCreateIpv6(INOUT RvNetAddress* netAddress,
                        IN RvNetIpv6* IpV6);

/************************************************************************************
 * RvNetGetIpv4
 * description: RvNetGetIpv4 gets RvNetIpv4 (IPV4 network address) from filled RvNetAddress
 * input:  netAddress  - pointer to RvNetAddress.
 * output: IpV4        - pointer to IPV4 address
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvNetGetIpv4(OUT RvNetIpv4* IpV4,
                        IN RvNetAddress* netAddress);

/************************************************************************************
 * RvNetGetIpv6
 * description: RvNetGetIpv6 gets RvNetIpv6 (IPV6 network address) from filled RvNetAddress
 * input:  netAddress  - pointer to RvNetAddress.
 * output: IpV6        - pointer to IPV6 address
 * return value: If an error occurs, the function returns a negative value.
 *               If no error occurs, the function returns a RV_OK value.
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvNetGetIpv6(OUT RvNetIpv6* IpV6,
                        IN RvNetAddress* netAddress);

/************************************************************************************
 * RvNetGetAddressType
 * description: RvNetGetAddressType gets RvNetAddressType from filled RvNetAddress
 * input:  netAddress  - pointer to RvNetAddress.
 * return RvNetAddressType: 
 * If an error occurs, the function returns RVNET_ADDRESS_NONE.
 ***********************************************************************************/
RVAPI
RvNetAddressType RVCALLCONV RvNetGetAddressType(IN RvNetAddress* netAddress);

/************************************************************************************
 * RvNetAddressToRvAddress
 * description: converts RvNetAddress structure to RvAddress structure 
 * input:  netAddress  - pointer to RvNetAddress.
 * output  rvAddress    - pointer to filled RvAddress 
 * RETURN  : RV_OK on success, other on failure
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvNetAddressToRvAddress(
         IN         RvNetAddress*  netAddress,
         INOUT      RvAddress*     rvAddress);
 
/************************************************************************************
 * RvAddressToRvNetAddress
 * description: converts RvAddress to RvNetAddress structure used
 *                     in ARTP Toolkit
 * input: rvAddress      - pointer to RvAddress
 * output  netAddress  - pointer to RvNetAddress. 
 * RETURN  : RV_OK on success, other on failure
 ***********************************************************************************/
RVAPI
RvStatus RVCALLCONV RvAddressToRvNetAddress(
         IN          RvAddress*     rvAddress,
         INOUT       RvNetAddress*  netAddress);
 
	
#ifdef __cplusplus
}
#endif

#endif /* __RV_NET_ADDRESS_H */



