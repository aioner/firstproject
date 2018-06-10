/* rvnettypes.h - Common Core public types networking types and APIs*/

/************************************************************************
                Copyright (c) 2006 RADVISION Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVISION LTD.
No part of this publication may be reproduced in any form whatsoever
without written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
************************************************************************/


#ifndef _RV_NETTYPES_H
#define _RV_NETTYPES_H

#include "rvtypes.h"
#include "rverror.h"

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#include <winsock2.h>

#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)


#if (_WIN32_WCE >= 400)
#include <winsock2.h>
#else
#include <winsock.h>
#endif

#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)
#include <inet.h>
#elif (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
#include <inc/nu_net.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#include <sockLib.h>
#endif
#endif

/*@****************************************************************************
* Module: RvPublicNetworking (root)
* ----------------------------------------------------------------------------
* Title: RADVISION Toolkits Public Networking API
*
* This section describes the public networking API of the RADVISION Toolkits.
***************************************************************************@*/


/*@****************************************************************************
* Module: RvNetworkTypes (RvPublicNetworking)
* ----------------------------------------------------------------------------
* Title: RADVISION Public Networking API Data Definitions
*
* This section describes the public networking API data definitions of the
* RADVISION Toolkits.
***************************************************************************@*/

/*@****************************************************************************
* Module: RvNetworkAPI (RvPublicNetworking)
* ----------------------------------------------------------------------------
* Title: RADVISION Public Networking API Functions
*
* This section describes the public networking API functions of the RADVISION Toolkits.
***************************************************************************@*/


/*@****************************************************************************
* Enum: RvTLSMethod (RvNetworkTypes)
* ----------------------------------------------------------------------------
* Defines the supported SSL versions.
***************************************************************************@*/
typedef enum {
	RV_TLS_SSL_V2 = 1,
        /* SSL version 1. */
	RV_TLS_SSL_V3,
        /* SSL version 2. */
	RV_TLS_TLS_V1
        /* SSL version 3. */
} RvTLSMethod;



/*@****************************************************************************
* Type: RV_TLS_DEFAULT_CERT_DEPTH (RvNetworkTypes)
* ----------------------------------------------------------------------------
* Default certificates chain length.
***************************************************************************@*/
#define RV_TLS_DEFAULT_CERT_DEPTH	(-1)


/*@****************************************************************************
* Enum: RvPrivKeyType (RvNetworkTypes)
* ----------------------------------------------------------------------------
* Defines the supported private key types.
***************************************************************************@*/
typedef enum {
	RV_TLS_RSA_KEY = 1
} RvPrivKeyType;


/*@****************************************************************************
* Enum: RvHashFuncEnum (RvNetworkTypes)
* ----------------------------------------------------------------------------
* Defines the hash function for the certificate fingerprint.
* The values are as in RFC 4572.
***************************************************************************@*/
typedef enum {
    RvHashFuncUndefined,
    RvHashFuncSha1,
    RvHashFuncSha224,
    RvHashFuncSha256,
    RvHashFuncSha384,
    RvHashFuncSha512,
    RvHashFuncMd5,
    RvHashFuncMd2
} RvHashFuncEnum;


#if (RV_TLS_TYPE == RV_TLS_OPENSSL)

/*@****************************************************************************
* Type: RvTLSEngineCfg (RvNetworkTypes)
* ----------------------------------------------------------------------------
* Defines the configuration of the TLS engine.
***************************************************************************@*/
typedef struct {
    RvTLSMethod             method;
    /* The SSL version. The default is RV_TLS_TLS_V1. */

    RvChar                 *privKey;
    /* The application can provide the private key. */

    RvInt                   privKeyLen;
    /* Length of 'privKey'. */

    RvChar                 *privKeyFileName;
    /* The application can provide the name of the file that contains the private key.
    This field is considered only if 'privKey' is NULL. */

    RvChar                  *cert;
    /* The application can provide the local certificate to be used for SSL connections. */

    RvInt                   certLen;
    /* The length of the local certificate provided in 'cert'. */

    RvChar                  *certFileName;
    /* The application can provide the name of the file that contains the local certificate.
    This field is considered only if 'cert' is NULL. */

    RvInt                   certDepth;
    /*  Maximum allowed depth of the certificate chains.
        Use the RV_TLS_DEFAULT_CERT_DEPTH (-1) value for the OpenSSL set default.
    */

    void*                   pRvLogMgr;
    /* Common RADVISION log manager instance pointer. Can be retrieved from all RADVISION Toolkits with the
    corresponding API call. For example, for the SIP Toolkit, use RvSipStackGetLogHandle. */
} RvTLSEngineCfg;

/*@****************************************************************************
* Type: RvTLSEngineEx (RvNetworkTypes)
* ----------------------------------------------------------------------------
* Defines the TLS engine.
***************************************************************************@*/
typedef struct _RvTLSEngineEx RvTLSEngineEx;

#if defined(__cplusplus)
extern "C" {
#endif



/*@**********************************************************************************
* RvTLSEngineCfgInit (RvNetworkAPI)
* ----------------------------------------------------------------------------------
* General:
* Initializes the RvTLSEngineCfg type instance to its default values.
*
* Arguments:
* Output: pCfg - The configuration structure instance pointer.
*
* Return Value: None.
*********************************************************************************@*/
RVCOREAPI
void RvTLSEngineCfgInit(
    INOUT RvTLSEngineCfg *pCfg);


/*@**********************************************************************************
* RvTLSEngineConstructEx (RvNetworkAPI)
* ----------------------------------------------------------------------------------
* General:
* Constructs the SSL context according to the input parameters.
*
* Arguments:
* Input:  pTlsEngCfg  - The configuration of the SSL engine.
* Output: ppTlsEng    - The TLS engine to be constructed.
*
* Return Value: RvStatus - success or failure.
***************************************************************************@*/
RVCOREAPI
RvStatus RvTLSEngineConstructEx(
    IN  RvTLSEngineCfg*         pTlsEngCfg,
    OUT RvTLSEngineEx**         ppTlsEng);

/*@**********************************************************************************
* RvTLSEngineDestructEx (RvNetworkAPI)
* ----------------------------------------------------------------------------------
* General:
*  Destructs the TLS engine instance constructed by the application.
*
* Arguments:
* Input:  pTlsEng - TLS engine to be destructed.
* Output: None.
*
* Return Value: None.
***************************************************************************@*/
RVCOREAPI
void RvTLSEngineDestructEx(
    IN RvTLSEngineEx*         pTlsEng);

/*@**********************************************************************************
* RvTLSGetCertificateFingerprint (RvNetworkAPI)
* ----------------------------------------------------------------------------------
* General:
* Calculates the fingerprint of the certificate according to the
* hash algorithm specified by 'hash'.
* The certificate can be given through the 'cert' or as the filename
* containing the certificate.
*
* Arguments:
* Input:  cert              - The certificate.
*         certLen           - The length of the certificate.
*         certFilename      - The file name containing the certificate.
*                             If 'cert' is not NULL, 'certFilename' is ignored.
* Output: fingerprint       - Must point to the buffer, provided by the application, in which
*                             the calculated fingerprint will be stored.
*         fingerprintLen    - On input: The size of the 'fingerprint' buffer.
*                             On output: The size of the calculated fingerprint.
*
* Return Value: RvStatus - success or failure.
***************************************************************************@*/
RVCOREAPI
RvStatus RvTLSGetCertificateFingerprint(
    IN    RvChar*                cert,
    IN    RvInt                  certLen,
    IN    RvChar*                certFilename,
    IN    RvHashFuncEnum         hash,
    INOUT RvChar*                fingerprint,
    INOUT RvInt*                 fingerprintLen);



#ifdef __cplusplus
}
#endif


#endif /* (RV_TLS_TYPE != RV_TLS_OPENSSL) */



#if (RV_NET_TYPE != RV_NET_NONE)

/* Type declaration of events we can wait for on file descriptors */
typedef RvUint16 RvSelectEvents;


#if ((RV_SELECT_TYPE == RV_SELECT_WIN32_WSA) || (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION))
#include <winsock2.h>
/* We use Windows specific values so we don't have to convert them */
#define RV_SELECT_READ       FD_READ
#define RV_SELECT_WRITE      FD_WRITE
#define RV_SELECT_ACCEPT     FD_ACCEPT
#define RV_SELECT_CONNECT    FD_CONNECT
#define RV_SELECT_CLOSE      FD_CLOSE
#else

#define RV_SELECT_READ       RvUint16Const(0x01)
#define RV_SELECT_WRITE      RvUint16Const(0x02)
#define RV_SELECT_ACCEPT     RvUint16Const(0x04)
#define RV_SELECT_CONNECT    RvUint16Const(0x08)
#define RV_SELECT_CLOSE      RvUint16Const(0x10)


#endif

/*
* Possible fd events
* Not all operating systems support all of these events. In these cases, it's up to the
* user of this module to resolve the actual event from the events he got.
* The events that are supported by all are read and write events.
* Note the fact that CLOSE events might not be returned by some OS's (mostly Unix). In such
* cases, receiving a READ event, and actually trying to read will cause an error, which can
* be interpreted as close event.
*/
#define RvSelectRead        RV_SELECT_READ
#define RvSelectWrite       RV_SELECT_WRITE
#define RvSelectAccept      RV_SELECT_ACCEPT
#define RvSelectConnect     RV_SELECT_CONNECT
#define RvSelectClose       RV_SELECT_CLOSE

/* Support address types. */
#define RV_ADDRESS_TYPE_NONE RvIntConst(0)
#define RV_ADDRESS_TYPE_IPV4 RvIntConst(1)
#define RV_ADDRESS_TYPE_IPV6 RvIntConst(2)

/* IPV4 definitions */
#define RV_ADDRESS_IPV4_STRINGSIZE 16               /* Max size of IPV4 address string */
#define RV_ADDRESS_IPV4_ADDRSIZE 4                  /* Size of IPV4 address */
#define RV_ADDRESS_IPV4_ANYADDRESS RvUint32Const(0) /* for dynamic addresss binding */
#define RV_ADDRESS_IPV4_BROADCAST RvUint32Const(0xFFFFFFFF)
#define RV_ADDRESS_IPV4_ANYPORT RvUint16Const(0)    /* for dynamic port binding */

/* abstract IPv4 address structure used internally inside the core */
typedef struct {
    RvUint32 ip;    /* 4 byte IP address, network format */
    RvUint16 port;  /* 2 byte port number, host format */
} RvAddressIpv4;


#if (RV_NET_TYPE & RV_NET_IPV6)
/* IPV6 definitions */
#define RV_ADDRESS_IPV6_STRINGSIZE 46               /* Max size of IPV6 address string */
#define RV_ADDRESS_IPV6_ADDRSIZE 16                 /* Size of IPV6 address */
#define RV_ADDRESS_IPV6_ANYPORT RvUint16Const(0)    /* for dynamic port binding */

/* abstract IPv6 address structure used internally inside the core */
typedef struct {
    RvUint8 ip[16];     /* 16 byte IP address, network format */
    RvUint16 port;      /* 2 byte port number, host format */
    RvUint32 scopeId;   /* 4 bytes of interface for a scope. todo: also could
                        be implemented as a list of interfaces */
} RvAddressIpv6;

#endif


/* abstract IP address structure used internally inside the core */
typedef struct
{
    RvInt   addrtype; /* Address type */
    union
    {
        /* One for each address type supported. */
        RvAddressIpv4 ipv4;
#if (RV_NET_TYPE & RV_NET_IPV6)
        RvAddressIpv6 ipv6;
#endif
    } data;
} RvAddress;

/* For comparisons */
#define RV_ADDRESS_FULLADDRESS RvIntConst(0) /* Compare entire address. */
#define RV_ADDRESS_BASEADDRESS RvIntConst(1) /* Compare only the base address */

/* Combined Constants */
#if (RV_NET_TYPE & RV_NET_IPV6)
#define RV_ADDRESS_MAXSTRINGSIZE RV_ADDRESS_IPV6_STRINGSIZE /* Max size of any address string */
#define RV_ADDRESS_MAXADDRSIZE RV_ADDRESS_IPV6_ADDRSIZE /* Max address size of any address. */
#else
#define RV_ADDRESS_MAXSTRINGSIZE RV_ADDRESS_IPV4_STRINGSIZE /* Max size of any address string */
#define RV_ADDRESS_MAXADDRSIZE RV_ADDRESS_IPV4_ADDRSIZE /* Max address size of any address. */
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/*@**********************************************************************************
 * RvAddressConstruct (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Initializes the default address structure.
 *
 * Arguments:
 * Input:
 *  addr        - Pointer to a pre-located address structure.
 *  addrtype    - Type of address.
 *
 * Return Value:
 *  RvStatus - pointer to the given address struct, or NULL on failure.
 *********************************************************************************@*/
RVCOREAPI RvAddress * RVCALLCONV RvAddressConstruct(
    IN    RvInt addrtype,
	INOUT RvAddress *addr);


/*@**********************************************************************************
 * RvAddressGetType (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Returns the address type.
 *
 * Arguments:
 * Input:
 *  addr        - Pointer to a pre-located address structure.
 *
 * Return Value:
 *  RvInt - address type.
 *********************************************************************************@*/
RVCOREAPI RvInt RVCALLCONV RvAddressGetType(
    IN const RvAddress *addr);

/*@**********************************************************************************
 * RvAddressCopy (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Copies an address structure.
 *
 * Arguments:
 * Input:
 *  source - Pointer to the source address.
 *  result - Pointer to a pre-located target address structure.
 *
 * Return Value:
 *  RvStatus - Pointer to a pre-located target address structure.
 *********************************************************************************@*/
RVCOREAPI RvAddress * RVCALLCONV RvAddressCopy(
    IN const RvAddress *source,
    INOUT RvAddress *result);

/*@**********************************************************************************
 * RvAddressCompare (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Compares two address structures.
 *
 * Arguments:
 * Input:
 *  addr1 - Pointer to address 1.
 *  addr2 - Pointer to address 2.
 *  comparetype - RV_ADDRESS_FULLADDRESS - also checks the port.
 *                RV_ADDRESS_BASEADDRESS - checks only the IP.
 *
 * Return Value:
 *  RvStatus - RV_TRUE/RV_FALSE.
 *********************************************************************************@*/
RVCOREAPI RvBool RVCALLCONV RvAddressCompare(
    IN const RvAddress *addr1,
    IN const RvAddress *addr2,
    IN RvInt comparetype);

/*@**********************************************************************************
 * RvAddressGetString (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Converts the IP address to a string.
 *
 * Arguments:
 * Input:
 *  addr - Pointer to the address from which to get the IP.
 *  bufsize - Size of the string buffer.
 *  buf - The buffer in which to put the converted IP.
 *
 * Return Value:
 *  RvStatus    - The converted IP string.
 *********************************************************************************@*/
RVCOREAPI RvChar * RVCALLCONV RvAddressGetString(
    IN const RvAddress *addr,
    IN RvSize_t bufsize,
    INOUT RvChar *buf);

/*@**********************************************************************************
 * RvAddressGetString (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Sets an IP in string format to the core-format structure.
 *
 * Arguments:
 * Input:
 *  buf - The source IP, in string format.
 *  addr - The address structure into which to set the IP.
 *
 * Return Value:
 *  RvStatus - The new address structure.
 *********************************************************************************@*/
RVCOREAPI RvAddress * RVCALLCONV RvAddressSetString(
    IN const RvChar *buf,
    INOUT RvAddress *addr);

#define RvAddressDestruct(_a)
#define RvAddressChangeType(_a, _t) RvAddressConstruct((_a), (_t))

/* IPV4 Utility functions. */

/*@**********************************************************************************
 * RvAddressGetIpPort (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Extracts the port from an rvAddress structure.
 *
 * Arguments:
 * Input:
 *  addr - The address structure from which to get the port.
 *
 * Return Value:
 *   result - The port.
 *********************************************************************************@*/
RVCOREAPI RvUint16 RVCALLCONV RvAddressGetIpPort(
    IN const RvAddress *addr);

/*@**********************************************************************************
 * RvAddressIsNull (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Checks if all the bits of an address are 0.
 *
 * Arguments:
 * Input:
 *  addr - The address structure.
 *
 * Return Value:
 *  RvBool - RV_TRUE if address is null, RV_FALSE otherwise.
 *********************************************************************************@*/
RVCOREAPI RvBool RVCALLCONV RvAddressIsNull(
    IN const RvAddress *addr);

/*@**********************************************************************************
 * RvAddressSetIpPort (RvNetworkAPI)
 * ----------------------------------------------------------------------------------
 * General:
 * Sets the port in an RvAddress structure.
 *
 * Arguments:
 * Input:
 *  addr - The address structure into which to set the port.
 *  port - The port.
 *
 * Return Value:
 *  RvStatus - The port.
 *********************************************************************************@*/
RVCOREAPI RvAddress * RVCALLCONV RvAddressSetIpPort(
    INOUT RvAddress *addr,
    IN RvUint16 port);

/**********************************************************************************
 * RvAddressIsMulticastIp
 * Return RV_TRUE if addr is a multicast address.
 * Input:
 *  addr - the address structure to check.
 * Return Value:
 *  RvStatus - RV_TRUE if addr is a multicast address.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIsMulticastIp(
    IN const RvAddress *addr);

/**********************************************************************************
 * RvAddressConstructIpv4
 * creates a core-format address structure.
 * Input:
 *  addr - the address structure to check.
 *  ip - ip value to set in the new address structure
 *  port - port value to set in the new address structure
 * Return Value:
 *  RvStatus - the new address.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressConstructIpv4(
    INOUT RvAddress *addr,
    IN RvUint32 ip,
    IN RvUint16 port);

/**********************************************************************************
 * RvAddressIpv4ToString
 * converts an hexa-format ip to a string format ip.
 * Input:
 *  buf - pointer to a buffer in which to place the string.
 *  bufsize - buffer size.
 *  ip - the ip value in hexa format
 * Return Value:
 *  RvStatus - the ip in string format.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv4ToString(
    INOUT RvChar *buf,
    IN RvSize_t bufsize,
    IN RvUint32 ip);

/**********************************************************************************
 * RvAddressIpv4ToString
 * converts a string format ip address to an integer format ip.
 * Input:
 *  ipbuf - pointer to a place in which to store the IP value.
 *  buf - pointer to the pi string to convert.
 * Return Value:
 *  RvStatus - RV_TRUE on success.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressStringToIpv4(
    INOUT  RvUint32 *ip,
    IN const RvChar *buf);

#define RvAddressChangeTypeIpv4(_a, _i, _p) RvAddressConstructIpv4((_a), (_i), (_p))

/**********************************************************************************
 * RvAddressGetIpv4
 * extracts the ip value from an address-structure.
 * Input:
 *  addr - pointer to the structure from which to get the ip from.
 * Return Value:
 *  RvStatus - the ip value.
 */
RVCOREAPI const RvAddressIpv4 * RVCALLCONV RvAddressGetIpv4(
    IN const RvAddress *addr);

/* IPV4 specific functions. Only use if absolutely necessary. */

/**********************************************************************************
 * RvAddressIpv4Construct
 * creates an ipv4 core format address structure.
 * Input:
 *  addr - pointer to the structure to "fill in".
 *  ip - ip address in integer format.
 *  port - port to use in the new address structure
 * Return Value:
 *  RvStatus - the new address structure
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4Construct(
    INOUT RvAddressIpv4 *addr,
    IN RvUint32 ip,
    IN RvUint16 port);

/**********************************************************************************
 * RvAddressIpv4Compare
 * compare 2 ipv4 address-structures.
 * Input:
 *  addr1 - pointer to address 1.
 *  addr2 - pointer to address 2.
 *  comparetype - RV_ADDRESS_FULLADDRESS - checks also the port,
 *                RV_ADDRESS_BASEADDRESS - checks only ip
 * Return Value:
 *  RvStatus    - RV_TRUE/RV_FALSE
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv4Compare(
    IN const RvAddressIpv4 *addr1,
    IN const RvAddressIpv4 *addr2,
    IN RvInt comparetype);

/**********************************************************************************
 * RvAddressIpv4GetString
 * converts the IPv4 address to string.
 * Input:
 *  addr - pointer to address to get the ip from.
 *  bufsize - size of string buffer.
 *  buf - the buffer in which to put the converted ip in.
 * Return Value:
 *  RvStatus    - the converted ip string.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv4GetString(
    IN const RvAddressIpv4 *addr,
    IN RvSize_t bufsize,
    INOUT RvChar *buf);

/**********************************************************************************
 * RvAddressIpv4SetString
 * sets an IPv4 in string format to core-format structure.
 * Input:
 *  buf - the source string format ip
 *  addr - the address structure to set the ip into.
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4SetString(
    IN const RvChar *buf,
    INOUT RvAddressIpv4 *addr);

/**********************************************************************************
 * RvAddressIpv4GetIp
 * extract the ip value from an ipv4 rvAddress structure.
 * Input:
 *  addr - the address structure to get the ip from.
 * Return Value:
 *  RvStatus - the ip value.
 */
RVCOREAPI RvUint32 RVCALLCONV RvAddressIpv4GetIp(
    IN const RvAddressIpv4 *addr);

/**********************************************************************************
 * RvAddressIpv4SetIp
 * sets the ip value in an ipv4 rvAddress structure.
 * Input:
 *  addr - the address structure to set the ip in.
 *  ip   - the ip value
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4SetIp(
    INOUT RvAddressIpv4 *addr,
    IN RvUint32 ip);

/**********************************************************************************
 * RvAddressIpv4GetPort
 * extract the port from an ipv4 rvAddress structure.
 * Input:
 *  addr - the address structure to get the port from.
 * Return Value:
 *  RvStatus - the port.
 */
RVCOREAPI RvUint16 RVCALLCONV RvAddressIpv4GetPort(
    IN const RvAddressIpv4 *addr);

/**********************************************************************************
 * RvAddressIpv4IsNull
 * check if an all the bits of an address are 0
 * Input:
 *  addr - the address structure
 * Return Value:
 *  RvBool - RV_TRUE if address is null, RV_FALSE o/w
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv4IsNull(
    IN const RvAddressIpv4 *addr);

/**********************************************************************************
 * RvAddressIpv4SetPort
 * sets the port from in ipv4 rvAddress structure.
 * Input:
 *  addr - the address structure to set the port in.
 *  port - the port value.
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv4 * RVCALLCONV RvAddressIpv4SetPort(
    INOUT RvAddressIpv4 *addr,
    IN RvUint16 port);

/**********************************************************************************
 * RvAddressIpv4IsMulticast
 * Return RV_TRUE if ipv4 address is a multicast address.
 * Input:
 *  addr - the address structure to check.
 * Return Value:
 *  RvStatus - RV_TRUE if addr is a multicast address.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv4IsMulticast(
    IN const RvAddressIpv4 *addr);

#if (RV_NET_TYPE & RV_NET_IPV6)
/* IPV6 specific functions */

/**********************************************************************************
 * RvAddressGetIpv6Scope
 * extract the scope number from an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to get the scope from.
 * Return Value:
 *  RvStatus - the scope value.
 */
RVCOREAPI RvUint32 RVCALLCONV RvAddressGetIpv6Scope(
    IN const RvAddress *addr);

#else
#define RvAddressGetIpv6Scope(a) (-1)
#endif



#if (RV_NET_TYPE & RV_NET_IPV6)
/* IPV6 Utility functions. */


/**********************************************************************************
 * RvAddressSetIpv6Scope
 * sets the scope number in an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to set the scope in.
 *  scopeId - scope dumber to set.
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressSetIpv6Scope(
    INOUT RvAddress *addr,
    IN RvUint32 scopeId);

/**********************************************************************************
 * RvAddressConstructIpv6
 * creates a new core-format IPv6 address structure.
 * Input:
 *  addr - the address structure to "fill in".
 *  ip - pointer to a 16byte ipv6 string.
 *  port - port number to set.
 *  scopeId - scope number to set.
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddress * RVCALLCONV RvAddressConstructIpv6(
    INOUT RvAddress *addr,
    IN const RvUint8 *ip,
    IN RvUint16 port,
    IN RvUint32 scopeId);

RVCOREAPI RvAddress * RVCALLCONV RvAddressConstructIpv6FromIpv4(
    INOUT RvAddress *addr,
    IN    RvAddress *ipv4addr);

/**********************************************************************************
 * RvAddressIpv6ToString
 * converts a hexa-format string ipv6 to a string format ip.
 * Input:
 *  buf - pointer to a buffer in which to place the string.
 *  bufsize - buffer size.
 *  ip - the ip value in hexa format
 * Return Value:
 *  RvStatus - the ip in string format.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv6ToString(
    INOUT RvChar *buf,
    IN RvSize_t bufsize,
    IN const RvUint8 *ip);

/**********************************************************************************
 * RvAddressStringToIpv6
 * converts a string format ipv6 address to an hexa-format string ip.
 * Input:
 *  ip - pointer to a place in which to store the IP value.
 *  buf - pointer to the ip string to convert.
 * Return Value:
 *  RvStatus - RV_TRUE on success.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressStringToIpv6(
    INOUT RvUint8 *ip,
    IN const RvChar *buf);

#define RvAddressChangeTypeIpv6(_a, _i, _p, _s) RvAddressConstructIpv6((_a), (_i), (_p), (_s))

/* IPV6 specific functions. Only use if absolutely necessary. */

/**********************************************************************************
 * RvAddressGetIpv6
 * extracts the ip value from an ipv6 address-structure.
 * Input:
 *  addr - pointer to the structure from which to get the ip from.
 * Return Value:
 *  RvStatus - the ipv6 structure.
 */
RVCOREAPI const RvAddressIpv6 * RVCALLCONV RvAddressGetIpv6(
    IN const RvAddress *addr);

/**********************************************************************************
 * RvAddressIpv6Construct
 * creates an ipv6 core format address structure.
 * Input:
 *  ip - ip address in string format.
 *  port - port to use in the new address structure
 *  scopeId - scope number to set.
 *  addr - pointer to the structure to "fill in".
 * Return Value:
 *  RvStatus - the new address structure
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6Construct(
    IN const RvUint8 *ip,
    IN RvUint16 port,
    IN RvUint32 scopeId,
	INOUT RvAddressIpv6 *addr);

/**********************************************************************************
 * RvAddressIpv6Compare
 * compare 2 ipv4 address-structures.
 * Input:
 *  addr1 - pointer to address 1.
 *  addr2 - pointer to address 2.
 *  comparetype - RV_ADDRESS_FULLADDRESS - checks also the port and scopeId
 *                RV_ADDRESS_BASEADDRESS - checks only ip
 * Return Value:
 *  RvStatus    - RV_TRUE/RV_FALSE
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv6Compare(
    IN const RvAddressIpv6 *addr1,
    IN const RvAddressIpv6 *addr2,
    IN RvInt comparetype);

/**********************************************************************************
 * RvAddressIpv6GetString
 * converts the IPv6 address to string.
 * Input:
 *  addr - pointer to address to get the ip from.
 *  bufsize - size of string buffer.
 *  buf - the buffer in which to put the converted ip in.
 * Return Value:
 *  RvStatus    - the converted ip string.
 */
RVCOREAPI RvChar * RVCALLCONV RvAddressIpv6GetString(
    IN const RvAddressIpv6 *addr,
    IN RvSize_t bufsize,
    INOUT RvChar *buf);

/**********************************************************************************
 * RvAddressIpv6SetString
 * sets an IPv6 in string format to core-format structure.
 * Input:
 *  addr - the address structure to set the ip into.
 *  buf - the source string format ip
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetString(
    IN const RvChar *buf,
    INOUT RvAddressIpv6 *addr);

/**********************************************************************************
 * RvAddressIpv6GetIp
 * extract the ip value from an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to get the ip from.
 * Return Value:
 *  RvStatus - the ip value.
 */
RVCOREAPI const RvUint8 * RVCALLCONV RvAddressIpv6GetIp(
    IN const RvAddressIpv6 *addr);

/**********************************************************************************
 * RvAddressIpv6SetIp
 * sets the ip value in an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to set the ip in.
 *  ip   - the ip value
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetIp(
    INOUT RvAddressIpv6 *addr,
    IN const RvUint8 *ip);

/**********************************************************************************
 * RvAddressIpv6GetPort
 * extract the port from an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to get the port from.
 * Return Value:
 *  RvStatus - the port.
 */
RVCOREAPI RvUint16 RVCALLCONV RvAddressIpv6GetPort(
    IN const RvAddressIpv6 *addr);

/**********************************************************************************
 * RvAddressIpv6IsNull
 * check if an all the bits of an address are 0
 * Input:
 *  addr - the address structure
 * Return Value:
 *  RvBool - RV_TRUE if address is null, RV_FALSE o/w
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv6IsNull(
    IN const RvAddressIpv6 *addr);

/**********************************************************************************
 * RvAddressIpv6SetPort
 * sets the port from in ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to set the port in.
 *  port - the port value.
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetPort(
    INOUT RvAddressIpv6 *addr,
    IN RvUint16 port);

/**********************************************************************************
 * RvAddressIpv6GetScope
 * extract the scope number from an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to get the scope from.
 * Return Value:
 *  RvStatus - the scope value.
 */
RVCOREAPI RvUint32 RVCALLCONV RvAddressIpv6GetScope(
    IN const RvAddressIpv6 *addr);

/**********************************************************************************
 * RvAddressIpv6SetScope
 * sets the scope number in an ipv6 rvAddress structure.
 * Input:
 *  addr - the address structure to set the scope in.
 *  scopeId - scope number to set.
 * Return Value:
 *  RvStatus - the new address structure.
 */
RVCOREAPI RvAddressIpv6 * RVCALLCONV RvAddressIpv6SetScope(
    INOUT RvAddressIpv6 *addr,
    IN RvUint32 scopeId);

/**********************************************************************************
 * RvAddressIpv6IsMulticast
 * Return RV_TRUE if ipv6 address is a multicast address.
 * Input:
 *  addr - the address structure to check.
 * Return Value:
 *  RvStatus - RV_TRUE if addr is a multicast address.
 */
RVCOREAPI RvBool RVCALLCONV RvAddressIpv6IsMulticast(
    IN const RvAddressIpv6 *addr);

#else /* (RV_NET_TYPE & RV_NET_IPV6) */

#define RvAddressConstructIpv6(addr, ip, port, scopeId) NULL
#define RvAddressSetIpv6Scope(addr,scopeId) (0)

#endif /* (RV_NET_TYPE & RV_NET_IPV6) */


/*@****************************************************************************
* RvProcessEventsUntil (RvNetworkAPI)
* ----------------------------------------------------------------------------
* General:
* The common RADVISION events-processing function should be run in a loop
* the entire time that at least one RADVSION Toolkit is active.
* The event-processing should be run in the same thread in which the Toolkit
* was initialized.
* This function blocks for a maximum of 'delay' milliseconds, or until
* one of the internal Stack events occurs.
* Note: This function is not used if the BFCP Stack was configured to run in
* its own thread (that is, if the bRunInOwnThread field of the BFCP Stack
* configuration structure was set to RV_TRUE).
*
* Arguments:
* Input:  delay -  The maximum time to block, in milliseconds.
* Output: None.
*
* Return Value: RV_OK on success, or a negative value on failure.
***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvProcessEventsUntil(
    IN  RvUint32                delay);

/*@****************************************************************************
* RvProcessEvents (RvNetworkAPI)
* ----------------------------------------------------------------------------
* General:
* The common RADVISION events-processing function should be run in a loop
* the entire time that at least one RADVSION Toolkit is active.
* The event-processing should be run in the same thread in which the Toolkit
* was initialized. This function blocks until an internal Stack event occurs.
* Note: This function is not used if the BFCP Stack was configured to run in
* its own thread (that is, if the bRunInOwnThread field of the BFCP Stack
* configuration structure was set to RV_TRUE).
*
* Arguments:
* Input:  None.
* Output: None.
*
* Return Value: RV_OK on success, or a negative value on failure.
***************************************************************************@*/
RVAPI RvStatus RVCALLCONV RvProcessEvents(void);


/*@**********************************************************************************
* RvHashFuncStr2Enum (RvNetworkAPI)
* ----------------------------------------------------------------------------------
* General:
* Utility function. Converts the textual representation of the hash algorithm into
* a value within RvHashFuncEnum.
*
* Arguments:
* Input:  txt - The textual name of the hash function.
*               Must be one of the following: "sha-1", "sha-224", "sha-256", "sha-384","sha-512", "md2" or "md5".
*               The value can also be in upper case. Note that the hyphen character after 'sha' may not appear.
* Output: None.
*
* Return Value: The corresponding value in the RvHashFuncEnum enumeration.
*********************************************************************************@*/
RVCOREAPI
RvHashFuncEnum RvHashFuncStr2Enum(
    IN const RvChar* txt);

/*@**********************************************************************************
* RvHashFuncEnum2Str (RvNetworkAPI)
* ----------------------------------------------------------------------------------
* General:
* Converts the enumeration RvHashFuncEnum value into its textual representation.
*
* Arguments:
* Input:  v         - The hash algorithm value.
*         upperCase - Whether the hash algorithm names should be returned in upper case.
*         useHyphen - Whether the hyphen character ('-') must be set between the
*                     algorithm name (e.g., "sha") and its number (512).
*                     The hyphen is used only for the 'sha' family.
*
* Output: None.
*
* Return Value: The name of the hash algorithm.
*********************************************************************************@*/
RVCOREAPI
const RvChar* RvHashFuncEnum2Str(
    IN RvHashFuncEnum v,
    IN RvBool upperCase,
    IN RvBool useHyphen);




#ifdef __cplusplus
}
#endif

#endif /* (RV_NET_TYPE != RV_NET_NONE) */


#endif /* #ifndef _RV_NETTYPES_H  */
