/***********************************************************************
Filename   : rvaddress.c
Description: Network address manipulation
************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/

#include "rvaddress.h"
#include "rvansi.h"
#include <string.h>

#if (RV_NET_TYPE != RV_NET_NONE)

/* This whole module could eventually be done with plug-ins but we'll */
/* keep it simple and fast for now. */

static RvChar *Rv_inet_ntop4(const RvUint8 *src, RvChar *dst, RvSize_t size);
static RvBool Rv_inet_pton4(const RvChar *src, RvUint8 *dst);
#if (RV_NET_TYPE & RV_NET_IPV6)
static RvChar *Rv_inet_ntop6(const RvUint8 *src, RvChar *dst, RvSize_t size);
static RvBool Rv_inet_pton6(const RvChar *src, RvUint8 *dst);
#endif


/**********************************************************************************
 * RvAddressConstruct
 * creates default address structure.
 * INPUT:
 *  addr        - pointer to a pre-located address structure.
 *  addrtype    - type of address.
 * RETURN:
 *  RvStatus    - pointer to the given address struct, or NULL on failure.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressConstruct(
    IN    RvInt addrtype,
    INOUT RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((addrtype != RV_ADDRESS_TYPE_NONE) && (addrtype != RV_ADDRESS_TYPE_IPV4) && (addrtype != RV_ADDRESS_TYPE_IPV6))
        return NULL;
#endif

    addr->addrtype = addrtype;
    switch(addrtype)
    {
    case RV_ADDRESS_TYPE_NONE: return addr; /* No data for address type NONE. */
    case RV_ADDRESS_TYPE_IPV4: if(RvAddressIpv4Construct(&addr->data.ipv4, RV_ADDRESS_IPV4_ANYADDRESS, RV_ADDRESS_IPV4_ANYPORT) != NULL)
                                   return addr;
        break;
#if (RV_NET_TYPE & RV_NET_IPV6)
    case RV_ADDRESS_TYPE_IPV6:
        {
            RvUint8 ip6anyaddress[RV_ADDRESS_MAXADDRSIZE];

            memset(ip6anyaddress, 0, RV_ADDRESS_IPV6_ADDRSIZE);
            if(RvAddressIpv6Construct(ip6anyaddress,RV_ADDRESS_IPV6_ANYPORT, 0,&addr->data.ipv6) != NULL)
                return addr;
            break;
        }
#endif
    }
    return NULL;
}


/**********************************************************************************
 * RvAddressGetType
 * return the address type.
 * INPUT:
 *  addr        - pointer to a pre-located address structure.
 * RETURN:
 *  RvInt       - address type.
 */
RVCOREAPI RvInt RVCALLCONV RvAddressGetType(
    IN const RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvIntConst(-1);
#endif
    return addr->addrtype;
}


/**********************************************************************************
 * RvAddressCopy
 * copy an address structure.
 * INPUT:
 *  source - pointer to the source address
 *  result - pointer to a pre-located target address structure.
 * RETURN:
 *  RvStatus    - pointer to a pre-located target address structure
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressCopy(
    IN    const RvAddress *source,
    INOUT RvAddress *result)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((result == NULL) || (source == NULL))
        return NULL;
#endif

    memcpy(result, source, sizeof(RvAddress));
    return result;
}

/**********************************************************************************
 * RvAddressCompare
 * compare 2 address-structures.
 * INPUT:
 *  addr1 - pointer to address 1.
 *  addr2 - pointer to address 2.
 *  comparetype - RV_ADDRESS_FULLADDRESS - checks also the port,
 *                RV_ADDRESS_BASEADDRESS - checks only ip
 * RETURN:
 *  RvStatus    - RV_TRUE/RV_FALSE
 */
RVCOREAPI RvBool RVCALLCONV RvAddressCompare(
    IN const RvAddress *addr1,
    IN const RvAddress *addr2,
    IN RvInt comparetype)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr1 == NULL) || (addr2 == NULL))
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((comparetype != RV_ADDRESS_FULLADDRESS) && (comparetype != RV_ADDRESS_BASEADDRESS))
        return RV_FALSE;
#endif

    if(addr1->addrtype != addr2->addrtype)
    {
        /* ToDo: it is possible to convert a v4 address to a v6 address for the sake of comparison */
        return RV_FALSE;
    }
    switch(addr1->addrtype)
    {
        case RV_ADDRESS_TYPE_NONE: return RV_TRUE;
        case RV_ADDRESS_TYPE_IPV4: return RvAddressIpv4Compare(&addr1->data.ipv4, &addr2->data.ipv4, comparetype);
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: return RvAddressIpv6Compare(&addr1->data.ipv6, &addr2->data.ipv6, comparetype);
#endif
    }
    return RV_FALSE;
}


/**********************************************************************************
 * RvAddressGetString
 * converts the IP address to string.
 * INPUT:
 *  addr - pointer to address to get the ip from.
 *  bufsize - size of string buffer.
 *  buf - the buffer in which to put the converted ip in.
 * RETURN:
 *  RvStatus    - the converted ip string.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressGetString(
    IN    const RvAddress *addr,
    IN    RvSize_t bufsize,
    INOUT RvChar *buf)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (buf == NULL))
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(bufsize < 1)
        return NULL;
#endif

    switch(addr->addrtype) {
        case RV_ADDRESS_TYPE_NONE: *buf = '\0';
                                   return buf;
        case RV_ADDRESS_TYPE_IPV4: return RvAddressIpv4GetString(&addr->data.ipv4, bufsize, buf);
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: return RvAddressIpv6GetString(&addr->data.ipv6, bufsize, buf);
#endif
    }
    return NULL;
}


/**********************************************************************************
 * RvAddressSetString
 * sets an IP in string format to core-format structure.
 * INPUT:
 *  buf - the source string format ip
 *  addr - the address structure to set the ip into.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressSetString(
    IN    const RvChar *buf,
    INOUT RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (buf == NULL))
        return RV_FALSE;
#endif

    switch(addr->addrtype)
    {
        case RV_ADDRESS_TYPE_NONE: return NULL;
        case RV_ADDRESS_TYPE_IPV4: if(RvAddressIpv4SetString(buf, &addr->data.ipv4) != NULL)
                                       return addr;
                                   break;
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: if(RvAddressIpv6SetString(buf, &addr->data.ipv6) != NULL)
                                       return addr;
                                   break;
#endif
    }
    return NULL;
}


/* Functions for IPV4 and IPV6 */

/**********************************************************************************
 * RvAddressGetIpPort
 * extract the port from an rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the port from.
 * RETURN:
 *  RvStatus - the port.
 */
RVCOREAPI RvUint16 RVCALLCONV RvAddressGetIpPort(
    IN const RvAddress *addr)
{
    if(addr == NULL)
        return RvUint16Const(0);

    switch(addr->addrtype)
    {
        case RV_ADDRESS_TYPE_IPV4: return RvAddressIpv4GetPort(&addr->data.ipv4);
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: return RvAddressIpv6GetPort(&addr->data.ipv6);
#endif
    }
    return RvUint16Const(0);
}

/**********************************************************************************
 * RvAddressIsNull
 * check if an all the bits of an address are 0
 * INPUT:
 *  addr - the address structure
 * RETURN:
 *  RvBool - RV_TRUE if address is null, RV_FALSE o/w
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIsNull(
    IN const RvAddress *addr)
{
    if(addr == NULL)
        return RV_TRUE;

    switch(addr->addrtype)
    {
        case RV_ADDRESS_TYPE_IPV4: return RvAddressIpv4IsNull(&addr->data.ipv4);
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: return RvAddressIpv6IsNull(&addr->data.ipv6);
#endif
    }
    return RV_TRUE;
}


/**********************************************************************************
 * RvAddressSetIpPort
 * sets the port in an rvAddress structure.
 * INPUT:
 *  addr - the address structure to ser the port into.
 *  port - the port.
 * RETURN:
 *  RvStatus - the port.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressSetIpPort(
    INOUT RvAddress *addr,
    IN    RvUint16 port)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif
    switch(addr->addrtype)
    {
        case RV_ADDRESS_TYPE_IPV4: if(RvAddressIpv4SetPort(&addr->data.ipv4, port) != NULL)
                                       return addr;
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: if(RvAddressIpv6SetPort(&addr->data.ipv6, port) != NULL)
                                       return addr;
#endif
    }
    return NULL;
}


/**********************************************************************************
 * RvAddressIsMulticastIp
 * Return RV_TRUE if addr is a multicast address.
 * INPUT:
 *  addr - the address structure to check.
 * RETURN:
 *  RvStatus - RV_TRUE if addr is a multicast address.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIsMulticastIp(
    IN const RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RV_FALSE;
#endif

    switch(addr->addrtype)
    {
        case RV_ADDRESS_TYPE_IPV4: return RvAddressIpv4IsMulticast(&addr->data.ipv4);
#if (RV_NET_TYPE & RV_NET_IPV6)
        case RV_ADDRESS_TYPE_IPV6: return RvAddressIpv6IsMulticast(&addr->data.ipv6);
#endif
    }
    return RV_FALSE;
}


/* IPV4 specific functions. */

/**********************************************************************************
 * RvAddressConstructIpv4
 * creates a core-format address structure.
 * INPUT:
 *  addr - the address structure to check.
 *  ip - ip value to set in the new address structure
 *  port - port value to set in the new address structure
 * RETURN:
 *  RvStatus - the new address.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressConstructIpv4(
    INOUT RvAddress *addr,
    IN    RvUint32 ip,
    IN    RvUint16 port)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    addr->addrtype = RV_ADDRESS_TYPE_IPV4;
    if(RvAddressIpv4Construct(&addr->data.ipv4, ip, port) == NULL)
        return NULL;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv4ToString
 * converts an hexa-format ip to a string format ip.
 * INPUT:
 *  buf - pointer to a buffer in which to place the string.
 *  bufsize - buffer size.
 *  ip - the ip value in hexa format
 * RETURN:
 *  RvStatus - the ip in string format.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv4ToString(
    INOUT RvChar *buf,
    IN    RvSize_t bufsize,
    IN    RvUint32 ip)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(buf == NULL)
        return NULL;
#endif
    return Rv_inet_ntop4((RvUint8 *)&ip, buf, bufsize);
}


/**********************************************************************************
 * RvAddressIpv4ToString
 * converts a string format ip address to an integer format ip.
 * INPUT:
 *  ipbuf - pointer to a place in which to store the IP value.
 *  buf - pointer to the pi string to convert.
 * RETURN:
 *  RvStatus - RV_TRUE on success.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressStringToIpv4(
    INOUT RvUint32 *ip,
    IN    const RvChar *buf)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((ip == NULL) || (buf == NULL))
        return RV_FALSE;
#endif
    return Rv_inet_pton4(buf, (RvUint8 *)ip);
}


/**********************************************************************************
 * RvAddressGetIpv4
 * extracts the ip value from an address-structure.
 * INPUT:
 *  addr - pointer to the structure from which to get the ip from.
 * RETURN:
 *  RvStatus - the ipv4 structure.
 */
RVCOREAPI const RvAddressIpv4 * RVCALLCONV RvAddressGetIpv4(
    IN const RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(addr->addrtype != RV_ADDRESS_TYPE_IPV4)
        return NULL;
#endif

    return &addr->data.ipv4;
}


/**********************************************************************************
 * RvAddressIpv4Construct
 * creates an ipv4 core format address structure.
 * INPUT:
 *  addr - pointer to the structure to "fill in".
 *  ip - ip address in integer format.
 *  port - port to use in the new address structure
 * RETURN:
 *  RvStatus - the new address structure
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4Construct(
    INOUT RvAddressIpv4 *addr,
    IN    RvUint32 ip,
    IN    RvUint16 port)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    addr->ip = ip;
    addr->port = port;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv4Compare
 * compare 2 ipv4 address-structures.
 * INPUT:
 *  addr1 - pointer to address 1.
 *  addr2 - pointer to address 2.
 *  comparetype - RV_ADDRESS_FULLADDRESS - checks also the port,
 *                RV_ADDRESS_BASEADDRESS - checks only ip
 * RETURN:
 *  RvStatus    - RV_TRUE/RV_FALSE
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv4Compare(
    IN const RvAddressIpv4 *addr1,
    IN const RvAddressIpv4 *addr2,
    IN RvInt comparetype)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr1 == NULL) || (addr2 == NULL))
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((comparetype != RV_ADDRESS_FULLADDRESS) && (comparetype != RV_ADDRESS_BASEADDRESS))
        return RV_FALSE;
#endif

    /* Check base IP address. */
    if(addr1->ip != addr2->ip)
        return RV_FALSE;

    /* Check full address, which means the port too. */
    if((comparetype == RV_ADDRESS_FULLADDRESS) && (addr1->port != addr2->port))
        return RV_FALSE;

    return RV_TRUE;
}


/**********************************************************************************
 * RvAddressGetString
 * converts the IPv4 address to string.
 * INPUT:
 *  addr - pointer to address to get the ip from.
 *  bufsize - size of string buffer.
 *  buf - the buffer in which to put the converted ip in.
 * RETURN:
 *  RvStatus    - the converted ip string.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv4GetString(
    IN const RvAddressIpv4 *addr,
    IN RvSize_t bufsize,
    INOUT RvChar *buf)
{

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (buf == NULL))
        return NULL;
#endif

    return Rv_inet_ntop4((RvUint8 *)&addr->ip, buf, bufsize);
}


/**********************************************************************************
 * RvAddressIpv4SetString
 * sets an IPv4 in string format to core-format structure.
 * INPUT:
 *  buf - the source string format ip
 *  addr - the address structure to set the ip into.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4SetString(
    IN const RvChar *buf,
    INOUT RvAddressIpv4 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (buf == NULL))
        return NULL;
#endif

    if(Rv_inet_pton4(buf, (RvUint8 *)&addr->ip) == RV_FALSE)
        return NULL;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv4GetIp
 * extract the ip value from an ipv4 rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the ip from.
 * RETURN:
 *  RvStatus - the ip value.
 */
RVCOREAPI RvUint32 RVCALLCONV RvAddressIpv4GetIp(
    IN const RvAddressIpv4 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint32Const(0);
#endif

    return addr->ip;
}


/**********************************************************************************
 * RvAddressIpv4SetIp
 * sets the ip value in an ipv4 rvAddress structure.
 * INPUT:
 *  addr - the address structure to set the ip in.
 *  ip   - the ip value
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4SetIp(
    INOUT RvAddressIpv4 *addr,
    IN RvUint32 ip)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    addr->ip = ip;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv4GetPort
 * extract the port from an ipv4 rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the port from.
 * RETURN:
 *  RvStatus - the port.
 */
RVCOREAPI RvUint16 RVCALLCONV RvAddressIpv4GetPort(
    IN const RvAddressIpv4 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint16Const(0);
#endif

    return addr->port;
}

/**********************************************************************************
 * RvAddressIpv4IsNull
 * check if an all the bits of an address are 0
 * INPUT:
 *  addr - the address structure
 * RETURN:
 *  RvBool - RV_TRUE if address is null, RV_FALSE o/w
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv4IsNull(
    IN const RvAddressIpv4 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint16Const(0);
#endif

    if (0 == addr->ip)
        return RV_TRUE;
    else
        return RV_FALSE;
}

/**********************************************************************************
 * RvAddressIpv4SetPort
 * sets the port from in ipv4 rvAddress structure.
 * INPUT:
 *  addr - the address structure to set the port in.
 *  port - the port value.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4SetPort(
    INOUT RvAddressIpv4 *addr,
    IN RvUint16 port)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    addr->port = port;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv4IsMulticast
 * Return RV_TRUE if ipv4 address is a multicast address.
 * INPUT:
 *  addr - the address structure to check.
 * RETURN:
 *  RvStatus - RV_TRUE if addr is a multicast address.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv4IsMulticast(
    IN const RvAddressIpv4 *addr)
{
    RvUint8 *addrbytes;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RV_FALSE;
#endif

    addrbytes = (RvUint8 *)&addr->ip;
    if((addrbytes[0] & RvUint8Const(0xF0)) == RvUint8Const(0xE0)) /* 224..239 */
       return RV_TRUE;
    return RV_FALSE;
}

#if (RV_NET_TYPE & RV_NET_IPV6)
/* IPV6 specific functions */

/**********************************************************************************
 * RvAddressGetIpv6Scope
 * extract the scope number from an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the scope from.
 * RETURN:
 *  RvStatus - the scope value.
 */
RVCOREAPI RvUint32 RVCALLCONV RvAddressGetIpv6Scope(
    IN const RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint32Const(0);
#endif
    switch(addr->addrtype)
    {
        case RV_ADDRESS_TYPE_IPV6: return RvAddressIpv6GetScope(&addr->data.ipv6);
    }
    return RvUint32Const(0);
}


/**********************************************************************************
 * RvAddressSetIpv6Scope
 * sets the scope number in an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to set the scope in.
 *  scopeId - scope number to set.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressSetIpv6Scope(
    INOUT RvAddress *addr,
    IN    RvUint32 scopeId)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif
    switch(addr->addrtype) {
        case RV_ADDRESS_TYPE_IPV6: if(RvAddressIpv6SetScope(&addr->data.ipv6, scopeId) != NULL)
                                       return addr;
    }
    return NULL;
}


/* The ip address points to a 16 byte IP6 address. */

/**********************************************************************************
 * RvAddressConstructIpv6
 * creates a new core-format IPv6 address structure.
 * INPUT:
 *  addr - the address structure to "fill in".
 *  ip - pointer to a 16byte ipv6 string.
 *  port - port number to set.
 *  scopeId - scope number to set.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressConstructIpv6(
    INOUT RvAddress *addr,
    IN const RvUint8 *ip,
    IN RvUint16 port,
    IN RvUint32 scopeId)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (ip == NULL))
        return NULL;
#endif

    addr->addrtype = RV_ADDRESS_TYPE_IPV6;
    if(RvAddressIpv6Construct(ip,port, scopeId,&addr->data.ipv6) == NULL)
        return NULL;
    return addr;
}

RVCOREAPI RvAddress * RVCALLCONV RvAddressConstructIpv6FromIpv4(
    INOUT RvAddress *addr6,
    IN RvAddress *addr4)
{
    RvUint32 iip4;
    RvUint8 *ip4;
    RvUint8 *ip6;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr6 == NULL) || (addr4 == NULL))
        return NULL;
#endif
    iip4 = addr4->data.ipv4.ip;
    addr6->data.ipv6.port = addr4->data.ipv4.port;
    ip4 = (RvUint8 *)&iip4;
    ip6 = addr6->data.ipv6.ip;
    addr6->addrtype = RV_ADDRESS_TYPE_IPV6;
    memset(ip6, 0, 10);
    ip6 += 10;
    *ip6++ = 0xff;
    *ip6++ = 0xff;
    *ip6++ = *ip4++;
    *ip6++ = *ip4++;
    *ip6++ = *ip4++;
    *ip6++ = *ip4++;
    
    return addr6;
}


/**********************************************************************************
 * RvAddressIpv6ToString
 * converts a hexa-format string ipv6 to a string format ip.
 * INPUT:
 *  buf - pointer to a buffer in which to place the string.
 *  bufsize - buffer size.
 *  ip - the ip value in hexa format
 * RETURN:
 *  RvStatus - the ip in string format.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv6ToString(
    INOUT RvChar *buf,
    IN RvSize_t bufsize,
    IN const RvUint8 *ip)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((buf == NULL) || (ip == NULL))
        return NULL;
#endif
    return Rv_inet_ntop6(ip, buf, bufsize);
}


/**********************************************************************************
 * RvAddressStringToIpv6
 * converts a string format ipv6 address to an hexa-format string ip.
 * INPUT:
 *  ip - pointer to a place in which to store the IP value.
 *  buf - pointer to the ip string to convert.
 * RETURN:
 *  RvStatus - RV_TRUE on success.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressStringToIpv6(
    INOUT RvUint8 *ip,
    IN const RvChar *buf)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((ip == NULL) || (buf == NULL))
        return RV_FALSE;
#endif
    return Rv_inet_pton6(buf, ip);
}


/**********************************************************************************
 * RvAddressGetIpv6
 * extracts the ip value from an ipv6 address-structure.
 * INPUT:
 *  addr - pointer to the structure from which to get the ip from.
 * RETURN:
 *  RvStatus - the ipv6 structure.
 */
RVCOREAPI const RvAddressIpv6 * RVCALLCONV RvAddressGetIpv6(
    IN const RvAddress *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    if(addr->addrtype != RV_ADDRESS_TYPE_IPV6)
        return NULL;
#endif

    return &addr->data.ipv6;
}


/**********************************************************************************
 * RvAddressIpv6Construct
 * creates an ipv6 core format address structure.
 * INPUT:
 *  ip - ip address in string format.
 *  port - port to use in the new address structure
 *  scopeId - scope number to set.
 *  addr - pointer to the structure to "fill in".
 * RETURN:
 *  RvStatus - the new address structure
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6Construct(
    IN const RvUint8 *ip,
    IN RvUint16 port,
    IN RvUint32 scopeId,
    INOUT RvAddressIpv6 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (ip == NULL))
        return NULL;
#endif

    memcpy(addr->ip, ip, RV_ADDRESS_IPV6_ADDRSIZE);
    addr->port = port;
    addr->scopeId = scopeId;

    return addr;
}


/**********************************************************************************
 * RvAddressIpv6Compare
 * compare 2 ipv4 address-structures.
 * INPUT:
 *  addr1 - pointer to address 1.
 *  addr2 - pointer to address 2.
 *  comparetype - RV_ADDRESS_FULLADDRESS - checks also the port and scopeId
 *                RV_ADDRESS_BASEADDRESS - checks only ip
 * RETURN:
 *  RvStatus    - RV_TRUE/RV_FALSE
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv6Compare(
    IN const RvAddressIpv6 *addr1,
    IN const RvAddressIpv6 *addr2,
    IN RvInt comparetype)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr1 == NULL) || (addr2 == NULL))
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((comparetype != RV_ADDRESS_FULLADDRESS) && (comparetype != RV_ADDRESS_BASEADDRESS))
        return RV_FALSE;
#endif

    /* Check base IP address. */
    if(memcmp(addr1->ip,addr2->ip, RV_ADDRESS_IPV6_ADDRSIZE) != 0)
        return RV_FALSE;

    /* Check full address, which means the port too. */
    if((comparetype == RV_ADDRESS_FULLADDRESS) && ((addr1->port != addr2->port) || (addr1->scopeId != addr2->scopeId)))
        return RV_FALSE;

    return RV_TRUE;
}


/**********************************************************************************
 * RvAddressIpv6GetString
 * converts the IPv6 address to string.
 * INPUT:
 *  addr - pointer to address to get the ip from.
 *  bufsize - size of string buffer.
 *  buf - the buffer in which to put the converted ip in.
 * RETURN:
 *  RvStatus    - the converted ip string.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv6GetString(
    IN const RvAddressIpv6 *addr,
    IN RvSize_t bufsize,
    INOUT RvChar *buf)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (buf == NULL))
        return RV_FALSE;
#endif

    return Rv_inet_ntop6(addr->ip, buf, bufsize);
}


/**********************************************************************************
 * RvAddressIpv6SetString
 * sets an IPv6 in string format to core-format structure.
 * INPUT:
 *  addr - the address structure to set the ip into.
 *  buf - the source string format ip
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetString(
    IN const RvChar *buf,
    INOUT RvAddressIpv6 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((addr == NULL) || (buf == NULL))
        return NULL;
#endif

    if(Rv_inet_pton6(buf, addr->ip) == RV_FALSE)
        return NULL;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv6GetIp
 * extract the ip value from an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the ip from.
 * RETURN:
 *  RvStatus - the ip value.
 */
RVCOREAPI const RvUint8 * RVCALLCONV RvAddressIpv6GetIp(
    IN const RvAddressIpv6 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    return addr->ip;
}


/**********************************************************************************
 * RvAddressIpv6SetIp
 * sets the ip value in an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to set the ip in.
 *  ip   - the ip value
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetIp(
    INOUT RvAddressIpv6 *addr,
    IN const RvUint8 *ip)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    memcpy(addr->ip, ip, RV_ADDRESS_IPV6_ADDRSIZE);
    return addr;
}


/**********************************************************************************
 * RvAddressIpv6GetPort
 * extract the port from an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the port from.
 * RETURN:
 *  RvStatus - the port.
 */
RVCOREAPI RvUint16 RVCALLCONV RvAddressIpv6GetPort(
    IN const RvAddressIpv6 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint16Const(0);
#endif

    return addr->port;
}

/**********************************************************************************
 * RvAddressIpv6IsNull
 * check if an all the bits of an address are 0
 * INPUT:
 *  addr - the address structure
 * RETURN:
 *  RvBool - RV_TRUE if address is null, RV_FALSE o/w
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv6IsNull(
    IN const RvAddressIpv6 *addr)
{
    static RvChar nullIp6[16] = {0};
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint16Const(0);
#endif
    if (0 == memcmp(nullIp6,addr->ip,16))
        return RV_TRUE;
    else
        return RV_FALSE;
}

/**********************************************************************************
 * RvAddressIpv6SetPort
 * sets the port from in ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to set the port in.
 *  port - the port value.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetPort(
    INOUT RvAddressIpv6 *addr,
    IN RvUint16 port)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    addr->port = port;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv6GetScope
 * extract the scope number from an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to get the scope from.
 * RETURN:
 *  RvStatus - the scope value.
 */
RVCOREAPI RvUint32 RVCALLCONV RvAddressIpv6GetScope(
    IN const RvAddressIpv6 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RvUint32Const(0);
#endif

    return addr->scopeId;
}


/**********************************************************************************
 * RvAddressIpv6SetScope
 * sets the scope number in an ipv6 rvAddress structure.
 * INPUT:
 *  addr - the address structure to set the scope in.
 *  scopeId - scope number to set.
 * RETURN:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetScope(
    INOUT RvAddressIpv6 *addr,
    IN RvUint32 scopeId)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return NULL;
#endif

    addr->scopeId = scopeId;
    return addr;
}


/**********************************************************************************
 * RvAddressIpv6IsMulticast
 * Return RV_TRUE if ipv6 address is a multicast address.
 * INPUT:
 *  addr - the address structure to check.
 * RETURN:
 *  RvStatus - RV_TRUE if addr is a multicast address.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv6IsMulticast(
    IN const RvAddressIpv6 *addr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(addr == NULL)
        return RV_FALSE;
#endif

    if(addr->ip[0] == RvUint8Const(0xFF))
        return RV_TRUE;
    return RV_FALSE;
}

#endif /* (RV_NET_TYPE & RV_NET_IPV6) */



/*********************************************************************************/
/* The software in this section is from the Internet Software Consortium */
/* with only minor changes: */

/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/* const char *
 * inet_ntop4(src, dst, size)
 *  format an IPv4 address
 * return:
 *  `dst' (as a const)
 * notes:
 *  (1) uses no statics
 *  (2) takes a u_char* not an in_addr as input
 * author:
 *  Paul Vixie, 1996.
 */
static RvChar *Rv_inet_ntop4(const RvUint8 *src, RvChar *dst, RvSize_t size)
{
    RvChar tmp[sizeof "255.255.255.255"];

    RvSprintf(tmp, "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
    if (strlen(tmp) >= size)
        return (NULL);
    strcpy(dst, tmp);
    return (dst);
}


/* int
 * inet_pton4(src, dst)
 *  like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *  1 if `src' is a valid dotted quad, else 0.
 * notice:
 *  does not touch `dst' unless it's returning 1.
 * author:
 *  Paul Vixie, 1996.
 */
static RvBool Rv_inet_pton4(const RvChar *src, RvUint8 *dst)
{
    static const RvChar digits[] = "0123456789";
    RvInt saw_digit, octets, ch;
    RvUint8 tmp[RV_ADDRESS_IPV4_ADDRSIZE], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0' && (ch != ':')) {
        RvChar *pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            RvUint newValue = (RvUint)(*tp * 10 + (pch - digits));

            if (newValue > 255)
                return RV_FALSE;
            *tp = (RvUint8)newValue;
            if (! saw_digit) {
                if (++octets > 4)
                    return RV_FALSE;
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return RV_FALSE;
            *++tp = 0;
            saw_digit = 0;
        } else
            return RV_FALSE;
    }
    if (octets < 4)
        return RV_FALSE;
    memcpy(dst, tmp, RV_ADDRESS_IPV4_ADDRSIZE);
    return RV_TRUE;
}


#if (RV_NET_TYPE & RV_NET_IPV6)

/* const char *
 * inet_ntop6(src, dst, size)
 *  convert IPv6 binary address into presentation (printable) format
 * author:
 *  Paul Vixie, 1996.
 */
static RvChar *Rv_inet_ntop6(const RvUint8 *src, RvChar *dst, RvSize_t size)
{
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    RvChar tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct { RvInt base, len; } best, cur;
    RvUint words[RV_ADDRESS_IPV6_ADDRSIZE / 2];
    RvInt i;

    /*
     * Preprocess:
     *  Copy the input (bytewise) array into a wordwise array.
     *  Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    for (i = 0; i < RV_ADDRESS_IPV6_ADDRSIZE; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;
    for (i = 0; i < (RV_ADDRESS_IPV6_ADDRSIZE / 2); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (RV_ADDRESS_IPV6_ADDRSIZE / 2); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base &&
              i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 &&
              (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
            if (Rv_inet_ntop4(src+12, tp, (RvSize_t)(sizeof tmp - (tp - tmp))) == RV_FALSE)
                return (NULL);
            tp += strlen(tp);
            break;
        }
        RvSprintf(tp, "%x", words[i]);
        tp += strlen(tp);
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) ==
          (RV_ADDRESS_IPV6_ADDRSIZE / 2))
        *tp++ = ':';
    *tp++ = '\0';

    /*
     * Check for overflow, copy, and we're done.
     */
    if ((size_t)(tp - tmp) > size)
        return (NULL);
    strcpy(dst, tmp);
    return (dst);
}


/* int
 * inet_pton6(src, dst)
 *  convert presentation level address to network order binary form.
 * return:
 *  1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *  (1) does not touch `dst' unless it's returning 1.
 *  (2) :: in a full address is silently ignored.
 * credit:
 *  inspired by Mark Andrews.
 * author:
 *  Paul Vixie, 1996.
 */
static RvBool Rv_inet_pton6(const RvChar *src, RvUint8 *dst)
{
    static RvChar xdigits_l[] = "0123456789abcdef",
    xdigits_u[] = "0123456789ABCDEF";
    RvUint8 tmp[RV_ADDRESS_IPV6_ADDRSIZE], *tp, *endp, *colonp;
    RvChar *xdigits;
    const RvChar *curtok;
    RvInt ch, saw_xdigit;
    RvUint val;

    memset((tp = tmp), '\0', RV_ADDRESS_IPV6_ADDRSIZE);
    endp = tp + RV_ADDRESS_IPV6_ADDRSIZE;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return RV_FALSE;
    curtok = src;
    saw_xdigit = 0;
    val = 0;
    while ((ch = *src++) != '\0') {
        RvChar *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return RV_FALSE;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':') {
            curtok = src;
            if (!saw_xdigit) {
                if (colonp)
                    return RV_FALSE;
                colonp = tp;
                continue;
            } else if (*src == '\0') {
                return RV_FALSE;
            }
            if (tp + 2 > endp)
                return RV_FALSE;
            *tp++ = (RvUint8)((val >> 8) & 0xff);
            *tp++ = (RvUint8)(val & 0xff);
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + RV_ADDRESS_IPV4_ADDRSIZE) <= endp) &&
              (Rv_inet_pton4(curtok, tp) == RV_TRUE)) {
            tp += RV_ADDRESS_IPV4_ADDRSIZE;
            saw_xdigit = 0;
            break;  /* '\0' was seen by inet_pton4(). */
        }
        return RV_FALSE;
    }
    if (saw_xdigit) {
        if (tp + 2 > endp)
            return (0);
        *tp++ = (RvUint8)((val >> 8) & 0xff);
        *tp++ = (RvUint8)(val & 0xff);
    }
    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        RvPtrdiff_t n = tp - colonp;
        RvInt i;

        if (tp == endp)
            return RV_FALSE;
        for (i = 1; i <= n; i++) {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return RV_FALSE;
    memcpy(dst, tmp, RV_ADDRESS_IPV6_ADDRSIZE);
    return RV_TRUE;
}

#endif


/*********************************************************************************/
/* End of code from Internet Software Consortium */

#endif /* (RV_NET_TYPE != RV_NET_NONE) */
