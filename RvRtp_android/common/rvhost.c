/***********************************************************************
Filename   : rvhost.c
Description: host related functions
************************************************************************
        Copyright (c) 2001 RADVISION Inc. and RADVISION Ltd.
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
#include "rvstdio.h"
#include "rvhost.h"
#include "rvccoreglobals.h"


#if (RV_NET_TYPE != RV_NET_NONE)

#if (RV_ARCH_ENDIAN == RV_ARCH_LITTLE_ENDIAN)
#define RV_NETONE 0x1000000
#else
#define RV_NETONE 1
#endif

#define IPV6_ISLOOPBACK(x) (\
    (*(RvUint32 *)(x) == 0) && ((*(((RvUint32 *)x) + 1)) == 0) && \
    ((*(((RvUint32 *)x) + 2)) == 0) && ((*(((RvUint32 *)x) + 3)) == RV_NETONE) \
)

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD)	|| (RV_OS_TYPE == RV_OS_TYPE_MAC) || \
    (RV_OS_TYPE == RV_OS_TYPE_NETBSD)   || (RV_OS_TYPE == RV_OS_TYPE_IPHONE)
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#if (RV_OS_TYPE == RV_OS_TYPE_LINUX)
#include <linux/sockios.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || (RV_OS_TYPE == RV_OS_TYPE_FREEBSD) || (RV_OS_TYPE == RV_OS_TYPE_NETBSD)
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/sockio.h>
#if (RV_OS_TYPE == RV_OS_TYPE_NETBSD)
#include <sys/ioctl.h>
#include <ifaddrs.h>
#endif
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_MAC) || (RV_OS_TYPE == RV_OS_TYPE_IPHONE)
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#endif
#elif (RV_OS_TYPE == RV_OS_TYPE_HPUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#elif (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#pragma warning (push,3)
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma warning (pop)
#include <Iphlpapi.h>

#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)
#pragma warning (push,3)
static RvUint8 LOCAL_IP_BUF[] = {127,0,0,1}; /* Loopback address */
#if (RV_OS_TYPE == RV_OS_TYPE_WINCE) && (_WIN32_WCE >= 400)
#include <winsock2.h>
#include <Ws2spi.h>
#include <ws2tcpip.h>
#include <Iptypes.h>
#include <Iphlpapi.h>
#else
#include <winsock.h>
#endif
#pragma warning (pop)

#elif (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
#include "rvsymsock.h"

#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#  include <hostLib.h>

#  if (RV_OS_VERSION <= RV_OS_VXWORKS_2_2)
#    include <netinet/in.h>
#    if defined(IPPROTO_IPV6)  /* if IPv6 stack is installed */
#      include <net/if.h>
#      include <common/utilslib/ifaddrs.h>
       extern struct in_ifaddr* in_ifaddrhead;
#      define IN_IFADDR in_ifaddrhead
#      define IA_NEXT   ia_link.tqe_next
#    else
#      define IN_IFADDR in_ifaddr
#      define IA_NEXT   ia_next
#    endif
#    include <netinet/in_var.h>
static RvUint8 LOCAL_IP_BUF[] = {127,0,0,1}; /* Loopback address */
#  else
#    include <net/ifaddrs.h>
#    if (RV_OS_VERSION <= RV_OS_VXWORKS_3_1)
#      include <ifIndexLib.h> /* removed in workbench 3.2 */
#    else
#      include <inetLib.h>
#      include <ipcom.h>
#	   include <sockLib.h>
#      include <ipioctl.h>
#    endif
#
#  include <ifLib.h>
#  endif


#elif (RV_OS_TYPE == RV_OS_TYPE_PSOS)
#include <pna.h>
#include <pnacfg.h>

extern pNA_CT PnaCfg;

#define satosin(sa)     ((struct sockaddr_in *)(sa))
static RvUint8 LOCAL_IP_BUF[] = {127,0,0,1}; /* Loopback address */

RvInt gethostname(RvChar *buf, int size)
{
    strncpy(buf, PnaCfg.nc_hostname, (RvSize_t)size);

    return 0;
}

#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>


#elif (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
#include <nucleus.h>
#include <externs.h>
#include <socketd.h>



#elif (RV_OS_TYPE == RV_OS_TYPE_INTEGRITY)
#include <unistd.h>
/*#include <netconfig.h>*/

/* In order to support RvHostUnixGetIpV4List instead of
   RvHostIntegrityGetIpV4List the following headers should be added.
*/
#include <netinet/in.h>
#include <sys/sockio.h>

/* The RvHostUnixGetIpV4List doesn't use LOCAL_IP_BUF -> remove it */
/*
static RvUint8 LOCAL_IP_BUF[] = {127,0,0,1}; *//* Loopback address */

#endif

/* Lets make error codes a little easier to type */
#define RvHostErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_HOST, (_e))


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvHostLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->hostSource, p);}
#else
#define RvHostLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvHostLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->hostSource, p);}
#else
#define RvHostLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvHostLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->hostSource, p);}
#else
#define RvHostLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
#define RvHostLogWarning(p) {if (logMgr != NULL) RvLogError(&logMgr->hostSource, p);}
#else
#define RvHostLogWarning(p) {RV_UNUSED_ARG(logMgr);}
#endif


/* Host's name - allocated statically */
/*static RvChar rvHostLocalName[RV_HOST_MAX_NAMESIZE];*/


#if (RV_HOST_HAS_STATIC_ADDR == RV_YES)

/* List of host addresses - allocated statically */
/*static RvAddress rvHostLocalIpList[RV_HOST_MAX_ADDRS];*/
/*static RvUint32  rvHostLocalNumOfIps = RV_HOST_MAX_ADDRS;*/
/*static RvBool    rvHostLocalListInitialized = RV_FALSE;*/

#endif

/********************************************************************************************
 *
 *                                  Private functions
 *
 ********************************************************************************************/

#if (RV_OS_TYPE == RV_OS_TYPE_OSE)
/* since there is no host name in OSE, get it from the environment. */
/* host name should be set in environment prior to stack init (check readme file) */
int gethostname(char *buf, size_t size)
{
    char *result;

    result = get_env(current_process(), "HOSTNAME");
    if(result == NULL) {
        result = get_env(get_bid(current_process()), "HOSTNAME"); /* try block environment */
        if(result == NULL) {
            *buf = '\0';
            return -1;
        }
    }
    strncpy(buf, result, size);
    free_buf((union SIGNAL **)&result);
    return 0;
}
#endif  /* (RV_OS_TYPE == RV_OS_TYPE_OSE) */


#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS) && !defined(NET_5_2)
#define     NET_5_2         7
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS && NET_VERSION_COMP < NET_5_2)


struct hostent {
    char* h_name;
    char** h_aliases;
    short h_addrtype;
    short h_length;
    char** h_addr_list;
};

/********************************************************************************************
 * gethostbyname
 * Get host address corresponding to the host name.
 * INPUT   : host       - host name.
 *           buf        - buffer workspace holds the memory of the result.
 *           size       - size of buf.
 * OUTPUT  : none
 * RETURN  : a pointer to the hostent sturcture with the host address.
 */
static struct hostent* gethostbyname(const RvChar* host, RvChar* buf, RvSize_t size)
{
    struct hostent *h;
    RvChar *resultbuf;
    RvSize_t hostbufsize, resultbufsize;


    NU_HOSTENT nuc_h;
    RvChar **data;


    /* Make sure eveything is aligned right */
    h = (struct hostent *)RvAlign(buf); /* aligned hostent */
    hostbufsize = size - (size_t)((char *)h - buf); /* size of buffer with hostent */
    resultbuf = (char *)h + sizeof(struct hostent); /* start of remaining buffer */
    resultbufsize = size - (size_t)(resultbuf - buf); /* size of buffer without hostent */


    /* Nucleus never uses h_aliases and only allows one address (h_addr) */
    /* Also, name and address are not re-entrant, they point to internal */
    /* structures which can change out from underneath you. */

    /* Set up target result structure and data tables */
    data = (char **)resultbuf;
    *data = NULL;                  /* Empty alias table, has 1 null item */
    h->h_aliases = data;           /* set alias pointer to the table */
    data++;
    h->h_addr_list = data;         /* set address pointer to address table */
    data++;
    *data = NULL;                  /* end of address pointer table */
    data++;

    if(NU_Get_Host_By_Name((char *)host, &nuc_h) != NU_SUCCESS)
    {
        return(NULL);
    }
    if(resultbufsize < (strlen(nuc_h.h_name) + 1 + (sizeof(char *) * 4))) /* make sure we have enough space */
    {
        return (NULL);
    }
    *h->h_addr_list = memcpy((char *)data, nuc_h.h_addr, nuc_h.h_length); /* copy address and set ptr */
    data++;
    h->h_name = strcpy((char *)data, nuc_h.h_name);  /* copy name string and set h_hname */
    h->h_addrtype = nuc_h.h_addrtype;
    h->h_length = nuc_h.h_length;
    return h;
}
#endif


#if (RV_OS_TYPE != RV_OS_TYPE_MOPI)
/********************************************************************************************
 * RvHostFindLocalName
 *
 * purpose : Get the name of the local host and initialize it. This function is called from
 *           RvHostInit and is not guaranteed to be thread-safe.
 * input   : nameLength - Length of the name buffer in bytes
 * output  : name       - Null terminated name of the local host
 * return  : RV_OK on success
 *           Other values on failure
 ********************************************************************************************/
static RvStatus RvHostFindLocalName(
    IN  RvSize_t    nameLength,
    OUT RvChar*     name)
{
    RvStatus ret = RV_OK;

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
    if (NU_Get_Host_Name(name, (int)nameLength) != 0)
        ret = RvHostErrorCode(RV_ERROR_UNKNOWN);
#elif (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
    if (RvSymGetHostName((int)nameLength,name) != RV_OK)
       ret = RvHostErrorCode(RV_ERROR_UNKNOWN);
#else
    if (gethostname(name, (int)nameLength) != 0)
        ret = RvHostErrorCode(RV_ERROR_UNKNOWN);
#endif

    return ret;
}
#endif

#endif /* (RV_NET_TYPE != RV_NET_NONE) */



/********************************************************************************************
 *
 *                                  Public functions
 *
 ********************************************************************************************/


RvStatus RvHostInit(void)
{
    RvStatus status = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    RV_USE_CCORE_GLOBALS;

#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
    /* MOPI will set the IP value when it is available */
    RvHostMopiSetIpAddress(NULL);

#else
    status = RvHostFindLocalName(sizeof(rvHostLocalName), rvHostLocalName);
    if (status != RV_OK)
    {
        strcpy(rvHostLocalName,"localhost");
        status = RV_OK;
    }

#endif

#endif

    return status;
}


RvStatus RvHostEnd(void)
{
#if (RV_NET_TYPE != RV_NET_NONE)
#if (RV_HOST_HAS_STATIC_ADDR == RV_YES)
    RV_USE_CCORE_GLOBALS;
    rvHostLocalListInitialized = RV_FALSE;
#endif
#endif
    return RV_OK;
}

/********************************************************************************************
 * RvHostSourceConstruct - Constructs host module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * host module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvHostSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->hostSource, "HOST", "Host information");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}


#if (RV_NET_TYPE != RV_NET_NONE)

/********************************************************************************************
 * RvHostLocalGetName
 * Get the name of the local host.
 * INPUT   : nameLength - Length of the name buffer in bytes
 *           logMgr     - log manager instance
 * OUTPUT  : name       - Null terminated name of the local host
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvHostLocalGetName(
    IN  RvSize_t    nameLength,
    IN  RvLogMgr*   logMgr,
    OUT RvChar*     name)
{
    RV_USE_CCORE_GLOBALS;
    RvHostLogEnter((&logMgr->hostSource, "RvHostLocalGetName"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (name == NULL) {
        RvHostLogError((&logMgr->hostSource, "RvHostLocalGetName: NULL param(s)"));
        return RvHostErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    if (strlen(rvHostLocalName) > nameLength) {
        RvHostLogError((&logMgr->hostSource, "RvHostLocalGetName: Name buffer is too short"));
        return RvHostErrorCode(RV_ERROR_OUTOFRANGE);
    }

    strncpy(name, rvHostLocalName, nameLength);

    RvHostLogLeave((&logMgr->hostSource, "RvHostLocalGetName"));

    return RV_OK;
}




#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
/********************************************************************************************
 * RvHostWindowsGetIpV4List
 * Get the IP list of a windows host. This is done for only IPv4 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */

#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
            RvHostWindowsGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)

RvStatus RVCALLCONV RvHostWindowsGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
#if 0
#define MAX_LOCAL_ADDRS 30
    SOCKET s;
    DWORD bytesReturned;
    RvInt res;
    RvUint32 numLocalAddr;
    RvUint32 i,j;
    INTERFACE_INFO localAddr[MAX_LOCAL_ADDRS];
    SOCKADDR_IN* pAddrInet;
    RvUint loopbackIdx = MAX_LOCAL_ADDRS;

    RV_UNUSED_ARG(logMgr);

    *numOfIps = 0;

    /* Create dummy socket in IPv4 address family */
    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s != INVALID_SOCKET)
    {
        /* Ask for address list in IPv4 domain */
        res = WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0, (LPVOID)&localAddr[0],
            sizeof(localAddr), &bytesReturned, NULL, NULL);
        if (res == SOCKET_ERROR)
            return RvHostErrorCode(RV_ERROR_UNKNOWN);

        numLocalAddr = (bytesReturned / sizeof(INTERFACE_INFO));
        for(i = 0, j = 0; j < numLocalAddr && i < numOfIpsAllocated; j++)
        {

/*
            if (onlyUp && !(localAddr[j].iiFlags & IFF_UP))
            {
                continue;
            }
*/

            /* Filter out loopback interface */
            if(localAddr[j].iiFlags & IFF_LOOPBACK)
            {
                loopbackIdx = j;
                continue;
            }

            pAddrInet = (SOCKADDR_IN*)&localAddr[j].iiAddress;
            (void)RvAddressConstructIpv4(ipList + i, pAddrInet->sin_addr.S_un.S_addr, RV_ADDRESS_IPV4_ANYPORT);
            i++;
        }

        if(loopbackIdx < MAX_LOCAL_ADDRS && i < numOfIpsAllocated)
        {
            pAddrInet = (SOCKADDR_IN*)&localAddr[loopbackIdx].iiAddress;
            (void)RvAddressConstructIpv4(ipList + i, pAddrInet->sin_addr.S_un.S_addr, RV_ADDRESS_IPV4_ANYPORT);
            i++;
        }

        closesocket(s);

        *numOfIps = i;

        return RV_OK;

    }
    else
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
#else

    RvChar buff[1024], *pBuff;
    RvBool bAllocated;
    DWORD dwRetVal = 0;
    ULONG outBuffLen;
    RvStatus retv = RV_OK;
    ULONG flags;
    RvUint32 i = 0;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast;
    RvBool bLoopBack = RV_FALSE;

    bAllocated = RV_FALSE;
    pBuff = buff;
    flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME |
            GAA_FLAG_SKIP_MULTICAST;
    flags = 0;
    outBuffLen = sizeof(buff);


again:
    dwRetVal = GetAdaptersAddresses(AF_INET, flags, NULL, (PIP_ADAPTER_ADDRESSES)pBuff, &outBuffLen);

    if (dwRetVal == ERROR_BUFFER_OVERFLOW) 
    { 
        if (bAllocated)
        {
            retv = RV_ERROR_OUTOFRESOURCES;
            goto finish;
        }
        
        retv = RvMemoryAlloc(NULL, outBuffLen,logMgr,&pBuff);
        if (retv != RV_OK)
        {
            RvHostLogError((&logMgr->hostSource, "RvHostWindowsGetIpV4List: failed to allocate %d bytes",outBuffLen));
            goto finish;
        }
        goto again;
    }
    else if (dwRetVal != NO_ERROR)
    {
        RvHostLogError((&logMgr->hostSource, "RvHostWindowsGetIpV4List: GetAdaptersAddresses failed with %d, err %d",
            dwRetVal,WSAGetLastError()));
        goto finish;
    }

    // If successful, output some information from the data we received
    pCurrAddresses = (PIP_ADAPTER_ADDRESSES)pBuff;

    while (pCurrAddresses) 
    {
        if (!(pCurrAddresses->Flags & IP_ADAPTER_DDNS_ENABLED && pCurrAddresses->OperStatus == IfOperStatusUp))
            goto nextAddr;

        pUnicast = pCurrAddresses->FirstUnicastAddress;
        while (pUnicast)
        {
            if (pUnicast->Address.lpSockaddr->sa_family = AF_INET)
            {
                PSOCKADDR_IN pAddrInet;
                pAddrInet = (SOCKADDR_IN*) (pUnicast->Address.lpSockaddr);

                if (pAddrInet->sin_addr.S_un.S_addr == INADDR_LOOPBACK)
                {
                    bLoopBack = RV_TRUE;
                }
                else if(i < numOfIpsAllocated)
                {
                    (void)RvAddressConstructIpv4(ipList + i, pAddrInet->sin_addr.S_un.S_addr, RV_ADDRESS_IPV4_ANYPORT);
                    i++;
                }
            }
            pUnicast = pUnicast->Next;
        }

nextAddr:
        pCurrAddresses = pCurrAddresses->Next;
    }

    if (i < numOfIpsAllocated && bLoopBack)
    {
        (void)RvAddressConstructIpv4(ipList + i, INADDR_LOOPBACK, RV_ADDRESS_IPV4_ANYPORT);
        i++;

    }

    *numOfIps = i;
    retv = RV_OK;

finish:
    if (bAllocated)
        RvMemoryFree(pBuff,logMgr);
    return retv;
#endif
}

#elif (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
      (RV_OS_TYPE == RV_OS_TYPE_FREEBSD) || (RV_OS_TYPE == RV_OS_TYPE_MAC)   || \
      (RV_OS_TYPE == RV_OS_TYPE_NETBSD)  || (RV_OS_TYPE == RV_OS_TYPE_INTEGRITY) || \
      (RV_OS_TYPE == RV_OS_TYPE_IPHONE)

/********************************************************************************************
 * rvHostUnixGetIpV4List
 * Get the IP list of a Solaris/Linux/FreeBSD host. This is done for only IPv4 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */

#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
            rvHostUnixGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)

static RvStatus RVCALLCONV rvHostUnixGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    RvUint32 i;
    RvInt s, remaining, current;
    struct ifconf ifc;
    struct ifreq *ifrp;
    struct sockaddr_in *sa;
    RvChar buff[2048];
    RvBool loopBackTreated = RV_FALSE;

    RV_UNUSED_ARG(logMgr);

    *numOfIps = 0;

    /* Create dummy socket in IPv4 address family */
    s = socket(AF_INET, SOCK_DGRAM, 0);

    if(s == -1)
        return RvHostErrorCode(RV_ERROR_UNKNOWN);

    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
    {
        close(s);
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
    }
    close(s);

    i = 0;

    remaining = ifc.ifc_len;
    ifrp = ifc.ifc_req;
    while (remaining)
    {
        sa = (struct sockaddr_in *)&ifrp->ifr_addr;

        if (sa->sin_family == AF_INET)
        {
            if (htonl(sa->sin_addr.s_addr) != INADDR_LOOPBACK)
            {
                RvAddressConstructIpv4(ipList+i,sa->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
                if (++i >= numOfIpsAllocated)
                    break;
            }
            else
            {
                /* in a meanwhile place the loopback address in the end of ipList */
                loopBackTreated = RV_TRUE;
                RvAddressConstructIpv4(ipList+(numOfIpsAllocated-1),
                                       sa->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
            }
        }

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS || RV_OS_TYPE == RV_OS_TYPE_LINUX)
        current = sizeof(struct ifreq);
#else
        current = sa->sin_len + IFNAMSIZ;
#endif
        ifrp = (struct ifreq *)(((char *)ifrp)+current);
        remaining -= current;
    }

    if (loopBackTreated && i < numOfIpsAllocated)
    {
        /* now place the loopback address as the last used address */
        memcpy(ipList+i,ipList+(numOfIpsAllocated-1),sizeof(RvAddress));
        i++;
    }


    *numOfIps = i;

    return RV_OK;
}


#elif (RV_OS_TYPE == RV_OS_TYPE_PSOS)

#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
                RvHostPSOSGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostPSOSGetIpV4List
 * Get the IP list of a PSOS host. This is done for only IPv4 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostPSOSGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    int s;
    RvUint32 rc = 0;
    struct ifreq ifr;
    RvUint32 i = 0;
    RvBool loopBackTreated = RV_FALSE;

    RV_UNUSED_ARG(logMgr);

    /* create any type of socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);

    for (ifr.ifr_ifno = 1, i = 0; !rc && (i < numOfIpsAllocated); ifr.ifr_ifno++)  /* Start from 1, not from 0 - from debugging */
    {
        satosin(&ifr.ifr_addr)->sin_family = AF_INET;
        rc = ioctl(s, SIOCGIFADDR, (RvChar *)&ifr);  /* Get the IP address of the pNA+ interface */
        if (!rc)
        {
            if ( satosin(&ifr.ifr_addr)->sin_addr.s_addr != (*(RvUint32*)LOCAL_IP_BUF))
            {
                /*            ipList[i]= satosin(&ifr.ifr_addr)->sin_addr.s_addr;*/
                RvAddressConstructIpv4(ipList+i, satosin(&ifr.ifr_addr)->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
                i++;
            }
            else
            {
                /* in a meanwhile place the loopback address in the end of ipList */
                loopBackTreated = RV_TRUE;
                RvAddressConstructIpv4(ipList+(numOfIpsAllocated-1), satosin(&ifr.ifr_addr)->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
            }
        }
    }

    if (loopBackTreated && i < numOfIpsAllocated)
    {
        /* now place the loopback address as the last used address */
        memcpy(ipList+i,ipList+(numOfIpsAllocated-1),sizeof(RvAddress));
        i++;
    }

    close(s);

    *numOfIps = i;

    return RV_OK;
}

#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)

#  define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#  define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
            RvHostVxWorksGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostVxWorksGetIpV4List
 * Get the IP list of a VxWorks host. This is done for only IPv4 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */

#if (RV_OS_VERSION <= RV_OS_VXWORKS_2_2)

RvStatus RVCALLCONV RvHostVxWorksGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    FAST struct in_ifaddr *ia = 0;
    struct sockaddr_in *tmpAddr;
    RvUint32 i = 0;
    RvBool loopBackTreated = RV_FALSE;

    RV_UNUSED_ARG(logMgr);

    for (ia=IN_IFADDR; (ia != NULL) && (i < numOfIpsAllocated); ia=ia->IA_NEXT)
    {
        tmpAddr = IA_SIN(ia);
        if ( (tmpAddr->sin_addr).s_addr != (*(UINT32*)LOCAL_IP_BUF))
        {
            RvAddressConstructIpv4(ipList+i, (tmpAddr->sin_addr).s_addr, RV_ADDRESS_IPV4_ANYPORT);
            i++;
        }
        else
        {
            /* in a meanwhile place the loopback address in the end of ipList */
            loopBackTreated = RV_TRUE;
            RvAddressConstructIpv4(ipList+(numOfIpsAllocated-1), (tmpAddr->sin_addr).s_addr, RV_ADDRESS_IPV4_ANYPORT);
        }
    }

    if (loopBackTreated && i < numOfIpsAllocated)
    {
        /* now place the loopback address as the last used address */
        memcpy(ipList+i,ipList+(numOfIpsAllocated-1),sizeof(RvAddress));
        i++;
    }

    *numOfIps = i;
    return RV_OK;
}
#elif  (RV_OS_VERSION < RV_OS_VXWORKS_3_2)
RvStatus RVCALLCONV RvHostVxWorksGetIpV4List(
     IN      RvLogMgr*   logMgr,
     IN      RvUint32    numOfIpsAllocated,
     OUT     RvUint32*   numOfIps,
     OUT     RvAddress*  ipList)
{
    RvUint32 ifIdx, i = 0;
    RvChar ifName[256];
    RvChar ipAddr[16];
    RvBool loopbackSeen = RV_FALSE;
    RvAddress* curAddr;

    RV_UNUSED_ARG(logMgr);

    for (ifIdx = 1, i = 0; i < numOfIpsAllocated && ifIndexTest(ifIdx); ifIdx++)
    {
        STATUS s = ifIndexToIfName(ifIdx, ifName);

        if (s != OK)
            continue;

        s = ifAddrGet(ifName, ipAddr);
        if (s != OK)
            continue;

        if ((loopbackSeen == RV_FALSE) && (strcmp(ifName, "lo0") == 0))
        {
            loopbackSeen = RV_TRUE;
            continue;
        }

        curAddr = &ipList[i++];
        RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, curAddr);
        RvAddressSetString(ipAddr, curAddr);
    }

    if (i < numOfIpsAllocated && loopbackSeen)
    {
        curAddr = &ipList[i++];
        RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, curAddr);
        RvAddressSetString("127.0.0.1", curAddr);
    }

    *numOfIps = i;
    return RV_OK;
}
#else
RvStatus RVCALLCONV RvHostVxWorksGetIpV4List(
	IN      RvLogMgr*   logMgr,
	IN      RvUint32    numOfIpsAllocated,
	OUT     RvUint32*   numOfIps,
	OUT     RvAddress*  ipList)
{
    RvUint32 i;
    RvInt s, remaining, current;
    struct ifconf ifc;
    struct ifreq *ifrp;
    struct sockaddr_in *sa;
    RvChar buff[2048];
    RvBool loopBackTreated = RV_FALSE;

    RV_UNUSED_ARG(logMgr);

    *numOfIps = 0;

    /* Create dummy socket in IPv4 address family */
    s = socket(AF_INET, SOCK_DGRAM, 0);

    if(s == -1)
        return RvHostErrorCode(RV_ERROR_UNKNOWN);

    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
    {
        close(s);
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
    }
    close(s);

    i = 0;

    remaining = ifc.ifc_len;
    ifrp = ifc.ifc_req;
    while (remaining)
    {
        sa = (struct sockaddr_in *)&ifrp->ifr_addr;

        if (sa->sin_family == AF_INET)
        {
            if (htonl(sa->sin_addr.s_addr) != INADDR_LOOPBACK)
            {
                RvAddressConstructIpv4(ipList+i,sa->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
                if (++i >= numOfIpsAllocated)
                    break;
            }
            else
            {
                /* in a meanwhile place the loopback address in the end of ipList */
                loopBackTreated = RV_TRUE;
                RvAddressConstructIpv4(ipList+(numOfIpsAllocated-1),
                                       sa->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
            }
        }

        current = sizeof(ifrp->ifr_name) + ifrp->ifr_addr.sa_len;
        ifrp = (struct ifreq *)(((char *)ifrp)+current);
        remaining -= current;
    }

    if (loopBackTreated && i < numOfIpsAllocated)
    {
        /* now place the loopback address as the last used address */
        memcpy(ipList+i,ipList+(numOfIpsAllocated-1),sizeof(RvAddress));
        i++;
    }

    *numOfIps = i;

    return RV_OK;
}
#endif  /* RV_OS_VERSION <= RV_OS_VXWORKS_2_2 */

/* Replace RvHostIntegrityGetIpV4List with RvHostUnixGetIpV4List
   to enable run on isimarm simulator. The isimarm doesn't support
   GetNumNetIfaces functionality.
*/
/*
#elif (RV_OS_TYPE == RV_OS_TYPE_INTEGRITY)
*/
#elif 0

#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvHostIntegrityGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostIntegrityGetIpV4List
 * Get the IP list of a VxWorks host. This is done for only IPv4 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostIntegrityGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    RvUint32 n, ipAddr;
    RvUint32 i = 0;
    RvBool loopBackTreated = false;

    n = GetNumNetIfaces();

    for (i=0;  n > 0 && i < numOfIpsAllocated;  --n)
    {
        if (GetNetIfaceConfig(IFACE_IPADDR, n, (Address)&ipAddr, sizeof(ipAddr)) == NETCONFIG_SUCCESS)
        {
            if (ipAddr != *(RvUint32*)LOCAL_IP_BUF)
            {
                RvAddressConstructIpv4(ipList+i, ipAddr, RV_ADDRESS_IPV4_ANYPORT);
                i++;
            }
            else
            {
                /* in a meanwhile place the loopback address in the end of ipList */
                loopBackTreated = true;
                RvAddressConstructIpv4(ipList+(numOfIpsAllocated-1), ipAddr, RV_ADDRESS_IPV4_ANYPORT);
            }
        }
        else
            RvHostLogError((&logMgr->hostSource, "RvHostGetIpList: GetNetIfaceConfig returned error"));
    }

    if (loopBackTreated && i < numOfIpsAllocated)
    {
        /* now place the loopback address as the last used address */
        memcpy(ipList+i,ipList+(numOfIpsAllocated-1),sizeof(RvAddress));
        i++;
    }

    *numOfIps = i;
    return RV_OK;
}

#elif (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)

#define RV_HOST_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
    RvHostNucleusGetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostNucleusGetIpV4List
 * Get the IP list of a Nucleus host. This is done for only IPv4 addresses.
 * INPUT   : hostName   - Name of host to look for
 *           logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
# if NET_VERSION_COMP >= NET_5_2

/* Backward compatibility issue - using undocumented variable */
extern UINT32 DEV_Next_Index;

#define NDEVS DEV_Next_Index

RvStatus RVCALLCONV RvHostNucleusGetIpV4List(
    IN      RvChar*     hostName,
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    CHAR devName[DEV_NAME_LENGTH];
    INT32 idx;
    RvUint32 nIps = 0;
    RvBool hasLoopback = RV_FALSE;
    RvUint32 ipv4;
    RvAddress *curAddr;
    INT32 nDevs;

    RV_UNUSED_ARG(logMgr);

#define SetByte(x, i, b) (((RvUint8 *)(&x))[i] = b)

    RV_UNUSED_ARG(hostName);
    RV_UNUSED_ARG(logMgr);

    *numOfIps = 0;
    nDevs = (INT32)NDEVS;

    for(idx = 0; idx < nDevs; idx++) {
        CHAR *ifname;
        SCK_IOCTL_OPTION option;
        STATUS nus;
        UINT8 *ipaddr;

        ifname = NU_IF_IndexToName(idx, devName);
        if(ifname == OS_NULL) {
            continue;
        }

        option.s_optval = devName;
        nus = NU_Ioctl_SIOCGIFADDR(&option, sizeof(option));
        if(nus != OS_SUCCESS) {
            return RvOsError(nus);
        }

        ipaddr = option.s_ret.s_ipaddr;
        if(ipaddr[0] == 127 && ipaddr[1] == 0 && ipaddr[2] == 0 && ipaddr[3] == 1) {
            hasLoopback = RV_TRUE;
            continue;
        }

        curAddr = &ipList[nIps];

        SetByte(ipv4, 0, ipaddr[0]);
        SetByte(ipv4, 1, ipaddr[1]);
        SetByte(ipv4, 2, ipaddr[2]);
        SetByte(ipv4, 3, ipaddr[3]);
        RvAddressConstructIpv4(curAddr, ipv4, 0);
        nIps++;
        if(nIps >= numOfIpsAllocated) {
            *numOfIps = nIps;
            return RV_OK;
        }
    }

    if(!hasLoopback) {
        return RV_OK;
    }

    SetByte(ipv4, 0, 127);
    SetByte(ipv4, 1, 0);
    SetByte(ipv4, 2, 0);
    SetByte(ipv4, 3, 1);
    curAddr = &ipList[nIps];
    RvAddressConstructIpv4(curAddr, ipv4, 0);
    *numOfIps = nIps + 1;
    return RV_OK;
}


#  else

RvStatus RVCALLCONV RvHostNucleusGetIpV4List(
    IN      RvChar*     hostName,
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    struct hostent* host = NULL;
    RvUint32** addrs, i = 0;
    RvChar nuHostBuf[256];


    host = (struct hostent*) gethostbyname(hostName, nuHostBuf, sizeof(nuHostBuf));

    if (host == NULL)
    {
        *numOfIps = 0;
        RvHostLogError((&logMgr->hostSource, "RvHostGetIpList: gethostbyname returned NULL"));
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
    }

    addrs = (RvUint32**)(host->h_addr_list);

    for (i = 0;  (i < numOfIpsAllocated) && (*addrs != NULL);  ++i)
        (void)RvAddressConstructIpv4(ipList+i, **addrs++, RV_ADDRESS_IPV4_ANYPORT);

    *numOfIps = i;
    return RV_OK;
}

#  endif

#elif (RV_OS_TYPE == RV_OS_TYPE_MOPI)

#define RV_HOST_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvHostMOPIGetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostMOPIGetIpV4List
 * Get the IP list of a MOPI host. This is done for only IPv4 addresses.
 * INPUT   : hostName   - Name of host to look for
 *           logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostMOPIGetIpV4List(
    IN      RvChar*     hostName,
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    struct hostent* host = NULL;
    RvUint32** addrs, i=0;

    /* convert 'strHostName' to INT32 and place into **hosts */
    /* since in MOPI the str is actually the IP address in dot notation, we use InetAddr */

    RvUint32    theIpAddress = 0;
    RvUint32*   hostIP[2] = {NULL, NULL};

    /*lint -save -e774 -e831 -e534 */
    /* warnings related to the unique structure of this macro */
    /*lint -restore */

    /*lint -save -e774 */
    /* informational, deliberated code of RV */
    RV_UNUSED_ARG(host);
    /*lint -restore */

    hostIP[0] = &theIpAddress;
    theIpAddress = (RvUint32)inet_addr(hostName);
    addrs = hostIP;
    for (i = 0;  (i < numOfIpsAllocated) && (*addrs != NULL);  ++i)
        (void)RvAddressConstructIpv4(ipList+i, **addrs++, RV_ADDRESS_IPV4_ANYPORT);
    *numOfIps = i;
    return RV_OK;
}

#elif (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvSymGetIpList(numOfIpsAllocated,numOfIps,ipList)

#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)

#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
	RvHostOseGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostOseGetIpV4List
 * Get the IP list of a Ose host. This is done for only IPv4 addresses.
 * INPUT   : hostName   - Name of host to look for
 *           logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
static RvStatus RVCALLCONV RvHostOseGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    RvUint32 i;
    RvInt s, remaining, ifCount,diff;
    struct ifconf ifc;
    struct ifreq *ifrp;
    struct sockaddr_in *sa;
    RvChar buff[2048];
    RvBool loopBackTreated = RV_FALSE;

    RV_UNUSED_ARG(logMgr);

    *numOfIps = 0;

    /* Create dummy socket in IPv4 address family */
    s = socket(AF_INET, SOCK_DGRAM, 0);

    if(s == -1)
        return RvHostErrorCode(RV_ERROR_UNKNOWN);

    ifc.ifc_len = sizeof(buff);
    ifc.ifc_buf = buff;
    if (ioctl(s, SIOCGIFCONF, (char*)&ifc) < 0)
    {
        close(s);
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
    }

    i = 0;

    remaining = ifc.ifc_len;
    ifCount = 0;

    while (remaining)
    {
		ifrp = (struct ifreq*) ((char*)ifc.ifc_req + SDL_IFREQSIZE*ifCount);
		ifCount ++;
		diff = (char*)ifrp - (char*)ifc.ifc_req;
		if (diff >= ifc.ifc_len)
		    break;

        sa = (struct sockaddr_in *)&ifrp->ifr_addr;

        if (ioctl(s, SIOCGIFADDR, (char*)ifrp) < 0)
        {
            close(s);
            return RvHostErrorCode(RV_ERROR_UNKNOWN);
       	}

	     sa = (struct sockaddr_in *)&ifrp->ifr_addr;

         if (sa->sin_family == AF_INET)
         {
             if (htonl(sa->sin_addr.s_addr) != INADDR_LOOPBACK)
             {
                RvAddressConstructIpv4(ipList+i,sa->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
                if (++i >= numOfIpsAllocated)
                    break;
             }
             else
             {
                /* in a meanwhile place the loopback address in the end of ipList */
                 loopBackTreated = RV_TRUE;
                 RvAddressConstructIpv4(ipList+(numOfIpsAllocated-1),
                                       sa->sin_addr.s_addr, RV_ADDRESS_IPV4_ANYPORT);
             }
         }
    }

    if (loopBackTreated && i < numOfIpsAllocated)
    {
        /* now place the loopback address as the last used address */
        memcpy(ipList+i,ipList+(numOfIpsAllocated-1),sizeof(RvAddress));
        i++;
    }

    *numOfIps = i;

    close(s);

    return RV_OK;
}
#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvHostWinCEGetIpV4List(logMgr,numOfIpsAllocated,numOfIps,ipList)
#define RV_HOST_UNUSED_ARG(hostName) RV_UNUSED_ARG(hostName)

RvStatus RVCALLCONV RvHostWinCEGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
#define MAX_LOCAL_ADDRS 30
    SOCKET s;
    DWORD bytesReturned;
    RvInt res;
    RvUint32 i,j;
    SOCKET_ADDRESS_LIST localAddr[MAX_LOCAL_ADDRS];
	SOCKADDR_IN* pAddrInet,tempAddr;
	SOCKET_ADDRESS_LIST *psl;
    RV_UNUSED_ARG(logMgr);
    *numOfIps = 0;

    /* Create dummy socket in IPv4 address family */
    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s != INVALID_SOCKET)
    {
        /* Ask for address list in IPv4 domain */
        res = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, &localAddr, sizeof(localAddr), &bytesReturned, NULL, NULL);
		if (res == SOCKET_ERROR)
            return RvHostErrorCode(RV_ERROR_UNKNOWN);

        psl = (SOCKET_ADDRESS_LIST *)localAddr;

        for(i = 0, j = 0; j < (RvUint32)psl->iAddressCount && i < numOfIpsAllocated; j++)
        {

			pAddrInet = (SOCKADDR_IN *)psl->Address[j].lpSockaddr;
            (void)RvAddressConstructIpv4(ipList + i, pAddrInet->sin_addr.S_un.S_addr, RV_ADDRESS_IPV4_ANYPORT);
            i++;
        }
		/* add LoopBack address to the list */
		if (i < numOfIpsAllocated)
		{
			pAddrInet = &tempAddr;
			pAddrInet->sin_addr.S_un.S_addr =  (*(UINT32*)LOCAL_IP_BUF);
			(void)RvAddressConstructIpv4(ipList + i, pAddrInet->sin_addr.S_un.S_addr, RV_ADDRESS_IPV4_ANYPORT);
			i++;
		}

        closesocket(s);
        *numOfIps = i;

    }
    else
        return RvHostErrorCode(RV_ERROR_UNKNOWN);

    return RV_OK;
}
#else

#define RV_HOST_UNUSED_ARG(hostName)

#define RvHost_OS_GetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvHostGeneralGetIpV4List(hostName,logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostGeneralGetIpV4List
 * Get the IP list of a host. This is done for only IPv4 addresses.
 * This function is used in case there is no OS specific function to fetch
 * the list of IP addresses.
 * INPUT   : hostName   - Name of host to look for
 *           logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostGeneralGetIpV4List(
    IN      RvChar*     hostName,
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    /* Other OS's implementation */
    struct hostent* host = NULL;
    RvUint32** addrs,i=0;

    host = (struct hostent*) gethostbyname(hostName);

    if (host == NULL)
    {
        *numOfIps = 0;
        RvHostLogError((&logMgr->hostSource, "RvHostGetIpList: gethostbyname returned NULL"));
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
    }

    addrs = (RvUint32**)(host->h_addr_list);
#if 0
#if (RV_OS_TYPE == RV_OS_TYPE_WINCE)
    /* COMPAQ iPAQ: real IP is *SECOND* on the list, retrieved from gethostbyname().
       so to make the fix as minor as possible, increment the array pointer.
       The first number on the list, is most likely the USB interface identifier,
       when the device is USB-connected to the desktop computer. */
    if (*(addrs+1) != NULL)
        addrs++;
#endif
#endif

    for (i = 0;  (i < numOfIpsAllocated) && (*addrs != NULL);  ++i)
        (void)RvAddressConstructIpv4(ipList+i, **addrs++, RV_ADDRESS_IPV4_ANYPORT);
    *numOfIps = i;
    return RV_OK;
}

#endif

#if (RV_NET_TYPE & RV_NET_IPV6)

#if ((RV_OS_TYPE == RV_OS_TYPE_WIN32) && (WINVER > 0x0400))

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvHostWindowsGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostWindowsGetIpV6List
 * Get the IP list of a Windows host. This is done for only IPv6 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostWindowsGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    SOCKET s;
    RvUint32 i,j;
    RvChar buf[2000];
    SOCKET_ADDRESS_LIST *psl;
    RvInt res;
    DWORD bytesReturned;
	SOCKADDR_IN6 *pAddrInet;
    *numOfIps = 0;
    RV_UNUSED_ARG(logMgr);


    /* Create dummy socket in IPv6 address family */
    s = socket(AF_INET6, SOCK_STREAM, 0);

    if(s != INVALID_SOCKET)
    {
        /* Ask for all available IPv6 addresses */
        res = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, 0, 0, buf, sizeof(buf), &bytesReturned, 0, 0);
        if(res != 0)
        {
            closesocket(s);
            return RvHostErrorCode(RV_ERROR_UNKNOWN);
        }

        psl = (SOCKET_ADDRESS_LIST *)buf;

        for (i=0,j=0;  (j < (RvUint32)psl->iAddressCount) && (i < numOfIpsAllocated);  ++j)
        {

			/* check if the address returned is of type ipv6 */
			if ( psl->Address[j].lpSockaddr->sa_family ==  AF_INET6 )
			{
				pAddrInet = (SOCKADDR_IN6 *)psl->Address[j].lpSockaddr;
				(void)RvAddressConstructIpv6(ipList+i, pAddrInet->sin6_addr.s6_addr, RV_ADDRESS_IPV6_ANYPORT, pAddrInet->sin6_scope_id);
				++i;
			}
        }
        closesocket(s);
        *numOfIps = i;

    }
    /* we do not return failure if the socket could be created,
    since IPv6 might not be installed on the particular host */

    return RV_OK;
}

#elif ((RV_OS_TYPE == RV_OS_TYPE_WINCE) && (_WIN32_WCE >= 401))

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  \
        RvHostWinMobileGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostWinMobileGetIpV6List
 * Get the IP list of a WindowsMobile host.
 * This is done for only IPv6 addresses, by retreiving the list of addresses of the adapters.
 *
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostWinMobileGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{

    union
    {
		RvChar buf[4096];
		IP_ADAPTER_ADDRESSES addr;
	} puAddr;


    RvUint32 i=0;
	SOCKADDR_IN6 *pAddrInet;
    IP_ADAPTER_ADDRESSES *pAdapter;
    RvUlong outBufLen;
    IP_ADAPTER_UNICAST_ADDRESS *pUniAddr;
	DWORD res,retUnicastAddr;
    RvBool dynamic = RV_FALSE;

    pAdapter = 0;
    *numOfIps = 0;
    RV_UNUSED_ARG(logMgr);
    outBufLen = sizeof(puAddr.buf);
    memset(puAddr.buf,0,sizeof(puAddr.buf));
    retUnicastAddr = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_FRIENDLY_NAME | \
                     GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    res = GetAdaptersAddresses(AF_INET6,retUnicastAddr,NULL,\
                               (PIP_ADAPTER_ADDRESSES )puAddr.buf,&outBufLen);

    if ( res == ERROR_BUFFER_OVERFLOW )
    {
        while ( res == ERROR_BUFFER_OVERFLOW )
        {
            /* if we fail to retreive addresses because the buffer is too small
               dynamically allocate the buffer
            */
            pAdapter = (IP_ADAPTER_ADDRESSES *) realloc(pAdapter,outBufLen);
            if (pAdapter)
            {
                res = GetAdaptersAddresses(AF_INET6,retUnicastAddr,NULL,\
                                           (PIP_ADAPTER_ADDRESSES )pAdapter,&outBufLen);
                dynamic = RV_TRUE;
            }
            else
            {
                return RvHostErrorCode(RV_ERROR_UNKNOWN);
            }
        }
    }
    else
    {
        pAdapter = (IP_ADAPTER_ADDRESSES *)puAddr.buf;
    }
    /* Retrieve the list of addresses */
    if ( res == NO_ERROR )
    {
        while (pAdapter)
        {
            /* get the unicast address for the adapter and loop till we
               get all the address or till we exceed the number of ips
               allocated
            */
            pUniAddr = pAdapter->FirstUnicastAddress;
            while ((pUniAddr) && (i < numOfIpsAllocated))
            {
                /* check if the address returned is of type ipv6 */
                if (pUniAddr->Address.lpSockaddr->sa_family == AF_INET6 )
                {
                    pAddrInet = (struct sockaddr_in6 *)pUniAddr->Address.lpSockaddr;
                    (void)RvAddressConstructIpv6(ipList+i, pAddrInet->sin6_addr.s6_addr, RV_ADDRESS_IPV6_ANYPORT, pAddrInet->sin6_scope_id);
                    ++i;

                }
                pUniAddr = pUniAddr->Next;
            }
            pAdapter = pAdapter->Next;
       }

    }
    else
    {
        if ( dynamic == RV_TRUE )
        {
            free (pAdapter);
        }
        return RvHostErrorCode(RV_ERROR_UNKNOWN);
    }
    if ( dynamic == RV_TRUE )
    {
        free (pAdapter);
    }
    *numOfIps = i;
    return RV_OK;


} /* end of RvHostWinMobileGetIpV6List */

#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) || (RV_OS_TYPE == RV_OS_TYPE_MAC) || (RV_OS_TYPE == RV_OS_TYPE_IPHONE)

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  \
                RvHostVxWorksGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostVxWorksGetIpV6List
 * Get the IP list of a VxWorks host. This is done for only IPv6 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostVxWorksGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    struct ifaddrs *ifp;
    struct ifaddrs *addrList;
    int retVal;
    RvUint32 i = 0;

    RV_UNUSED_ARG(logMgr);

    retVal = getifaddrs(&ifp);

    if (retVal == 0)
    {
        for (addrList = ifp; addrList != NULL && i<numOfIpsAllocated; addrList = addrList->ifa_next)
        {
            if (addrList->ifa_addr->sa_family == AF_INET6)
            {
                struct sockaddr_in6 *pAddrInet = ((struct sockaddr_in6*)addrList->ifa_addr);
                RvUint8 *s6addr = pAddrInet->sin6_addr.s6_addr;
                RvAddressConstructIpv6(ipList+i, s6addr, RV_ADDRESS_IPV6_ANYPORT, pAddrInet->sin6_scope_id);
                i++;
            }
        }
    }

    freeifaddrs(ifp);
    *numOfIps = i;
    return RV_OK;
}

#elif (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  \
                RvHostSolarisGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostSolarisGetIpV6List
 * Get the IP list of a Solaris host. This is done for only IPv6 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostSolarisGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    RvInt s, j, nIfreqs;
    struct lifconf lifc;
    RvChar buff[2048];
    RvUint32 i = 0;

    RV_UNUSED_ARG(logMgr);

    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
      return RV_OK;

    memset(&lifc, 0, sizeof(lifc));
    lifc.lifc_family = AF_INET6;
    lifc.lifc_len    = sizeof(buff);
    lifc.lifc_req    = (void*)buff;

    if (ioctl(s, SIOCGLIFCONF, &lifc) < 0)
    {
        close(s);
        return RV_OK;
    }

    close(s);

    nIfreqs = lifc.lifc_len / sizeof(struct lifreq);

    for (j=0;  (j < nIfreqs) && (i < numOfIpsAllocated);  ++j, ++i)
    {
        struct sockaddr_in6 *pAddrInet = ((struct sockaddr_in6 *)&lifc.lifc_req[j].lifr_addr);
        RvUint8 *s6addr = pAddrInet->sin6_addr.s6_addr;
        RvAddressConstructIpv6(ipList+i, s6addr, RV_ADDRESS_IPV6_ANYPORT, pAddrInet->sin6_scope_id);
    }
    *numOfIps = i;
    return RV_OK;
}

#elif (RV_OS_TYPE == RV_OS_TYPE_LINUX)

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  \
                RvHostLinuxGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostLinuxGetIpV6List
 * Get the IP list of a Linux host. This is done for only IPv6 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostLinuxGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
#define ASCII_TO_HEX(b,i) ((((b[i]  <='9') ? b[i]-'0'   : tolower(b[i])-'a'+10)<<4) + \
    ( (b[i+1]<='9') ? b[i+1]-'0' : tolower(b[i+1])-'a'+10))

    FILE* ifInet6;
    RvChar buff[100], ipv6Addr[16];
    RvInt j;
    RvUint32 i = 0;

    RV_UNUSED_ARG(logMgr);

    ifInet6 = fopen("/proc/net/if_inet6","r");
    if (ifInet6 == NULL)
        return RV_OK;

    for (i = 0; (i < numOfIpsAllocated) && (fgets(buff, sizeof(buff), ifInet6) != NULL);  ++i)
    {
        for (j = 0; (j < 16); j++)
            ipv6Addr[j] = ASCII_TO_HEX(buff,j*2);

        RvAddressConstructIpv6(ipList+i, (const RvUint8*)ipv6Addr,
                               RV_ADDRESS_IPV6_ANYPORT, 0);
    }

    fclose(ifInet6);

    *numOfIps = i;
    return RV_OK;
}
#elif (RV_OS_TYPE == RV_OS_TYPE_NETBSD)

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  \
                RvHostNetBSDGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

/********************************************************************************************
 * RvHostNetBSDGetIpV6List
 * Get the IP list for NetBSD hosts using getifaddrs system call
 * Note we get not only IPv6 addresses but IPv4 addresses also, thus
 * we have to filter out IPv4 addresses.
 * INPUT   : logMgr     - log manager instance
 *           numOfIpsAllocated   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RvStatus RVCALLCONV RvHostNetBSDGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    RvUint32 i = 0;
    struct ifaddrs *ifaddrs, *ifa;
    struct sockaddr_in6 *addr;

    RV_UNUSED_ARG(logMgr);

    *numOfIps = 0;

    if (getifaddrs(&ifaddrs) != 0)
        return RV_OK;

    ifa = ifaddrs;
    while (ifa && i < numOfIpsAllocated)
    {
        addr = (struct sockaddr_in6*) ifa->ifa_addr;
        if (addr && addr->sin6_family == AF_INET6)
        {
            RvUint32 scopeId = 0;
            if (addr->sin6_addr.s6_addr[0] == 0xFE && addr->sin6_addr.s6_addr[1] == 0x80)
            {
               /* for link-local addresses (starting with 0xFE80) the NETBsd sets the in the fourth byte
                  the interface number. We nullify the fourth byte while keeping its value as the
                  scope id 
                */
                scopeId = addr->sin6_addr.s6_addr[3];
                addr->sin6_addr.s6_addr[3] = 0;
            }
            else
                scopeId = addr->sin6_scope_id;
            RvAddressConstructIpv6(ipList+i, addr->sin6_addr.s6_addr, RV_ADDRESS_IPV6_ANYPORT, scopeId);
            i++;
        }
        ifa = ifa->ifa_next;
    }

    freeifaddrs(ifaddrs);

    *numOfIps = i;
    return RV_OK;
}

#elif RV_OS_TYPE == RV_OS_TYPE_INTEGRITY

#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList) \
    RvHostIntegrityGetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>


RvStatus RVCALLCONV RvHostIntegrityGetIpV6List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList) {

    union {
	char buf[1024];
	struct ifreq dummy;
    } u;

    struct ifreq *cur;
    char *curb;
    char *lastb;
    struct ifconf ifconfs;
    int s;
    int r;
    int n;
    RvAddress *curAddr;

    ifconfs.ifc_len = sizeof(u.buf);
    ifconfs.ifc_req = (struct ifreq *)u.buf;

    s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if(s < 0) {
	return RvOsError(errno);
    }

    r = ioctl(s, SIOCGIFCONF, &ifconfs);
    if(r != 0) {
	return RvOsError(errno);
    }

    curb = u.buf;
    lastb = u.buf + ifconfs.ifc_len;
    n = 0;
    curAddr = ipList;

    while(curb < lastb && n < numOfIpsAllocated) {
	struct sockaddr *saddr;
	
	cur = (struct ifreq *)curb;
	saddr = &cur->ifr_addr;
	if(saddr->sa_family == AF_INET6) {
	    struct sockaddr_in6 *saddr6 = (struct sockaddr_in6 *)saddr;
	    RvAddressConstructIpv6(curAddr, saddr6->sin6_addr.s6_addr, 
				   RV_ADDRESS_IPV6_ANYPORT, saddr6->sin6_scope_id);
	    curAddr++;
	    n++;
	    
	}

	curb = (char *)saddr + saddr->sa_len;
    }
    
    *numOfIps = n;
    return RV_OK;
}



#else /* (RV_OS_TYPE == RV_OS_TYPE_NETBSD)*/
#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  RV_OK
#endif /* ((RV_OS_TYPE == RV_OS_TYPE_WIN32) && ....*/
#else  /* (RV_NET_TYPE & RV_NET_IPV6) */
#define RvHost_OS_GetIpV6List(logMgr,numOfIpsAllocated,numOfIps,ipList)  RV_OK
#endif /* (RV_NET_TYPE & RV_NET_IPV6) */

/********************************************************************************************
 * RvHostGetIpList
 * Get the IP list of a host. This is done for both IPv4 and IPv6 addresses.
 * INPUT   : hostName   - Name of host to look for
 *           logMgr     - log manager instance
 *           numOfIps   - Maximum number of IPs to get
 * OUTPUT  : numOfIps   - Number of IPs we got
 *           ipList     - List of IPs we got
 * RETURN  : RV_OK on success
 *           Other values on failure
 */

RVCOREAPI
RvStatus RVCALLCONV RvHostGetIpList(
    IN      RvChar*     hostName,
    IN      RvLogMgr*   logMgr,
    INOUT   RvUint32*   numOfIps,
    OUT     RvAddress*  ipList)
{
    RvUint32 i = 0;
    RvStatus rvSts;

    RV_HOST_UNUSED_ARG(hostName);

    RvHostLogEnter((&logMgr->hostSource, "RvHostGetIpList"));

    rvSts = RvHost_OS_GetIpV4List(hostName,logMgr,*numOfIps,&i,ipList);
    if (rvSts != RV_OK)
        return rvSts;

#if (RV_NET_TYPE & RV_NET_IPV6)
/* note that the IPv6 address are consecutively inserted to the ipList structure
   right after the IPv4 addresses, since 'i' is being used from the last value
   it reached in the last for loop */
    if (i < *numOfIps)
    {
        RvUint32 jIn, jOut;
        jIn = *numOfIps-i;
		jOut = 0;
        rvSts = RvHost_OS_GetIpV6List(logMgr,jIn,&jOut,ipList+i);
        if (rvSts != RV_OK)
            return rvSts;
        i += jOut;
    }
#endif

    *numOfIps = i;

    RvHostLogLeave((&logMgr->hostSource, "RvHostGetIpList"));

    return RV_OK;
}


/********************************************************************************************
 * RvHostLocalGetAddress
 * Get the addresses of the local host. This is done for IPv4 addresses only.
 * INPUT   : logMgr             - log manager instance
 *           numberOfAddresses  - Maximum number of addresses to get
 *           addresses          - Array of addresses to fill in
 * OUTPUT  : numberOfAddresses  - Number of addresses filled in
 *           addresses          - Constructed addresses. These addresses should be destructed
 *                                using RvAddressDestruct() when not needed anymore.
 * RETURN  : RV_OK on success
 *           Other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvHostLocalGetAddress(
    IN      RvLogMgr*   logMgr,
    INOUT   RvUint32*   numberOfAddresses,
    INOUT   RvAddress*  addresses)
{
    RvStatus status = RV_OK;
    RV_USE_CCORE_GLOBALS;

    RvHostLogEnter((&logMgr->hostSource, "RvHostLocalGetAddress"));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((numberOfAddresses == NULL) || (addresses == NULL)) {
        RvHostLogError((&logMgr->hostSource, "RvHostLocalGetAddress: NULL param(s)"));
        return RvHostErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    {
#if (RV_HOST_HAS_STATIC_ADDR == RV_NO)
        RvUint32 rvHostLocalNumOfIps = RvMin(RV_HOST_MAX_ADDRS, *numberOfAddresses);
        RvAddress* rvHostLocalIpList = addresses;
#else
        if (!rvHostLocalListInitialized)
#endif
            /* Get the list of addresses for the local host */
            status = RvHostGetIpList(rvHostLocalName, logMgr, &rvHostLocalNumOfIps, rvHostLocalIpList);

        *numberOfAddresses = RvMin(rvHostLocalNumOfIps, *numberOfAddresses);
    }

#if (RV_HOST_HAS_STATIC_ADDR == RV_YES)
    if (status == RV_OK)
    {
        RvUint32 i;

        rvHostLocalListInitialized = RV_TRUE;

        /* We have a *LOT* of addresses - copy them into the user's buffer */
        for (i = 0; i < (*numberOfAddresses); i++)
            RvAddressCopy(rvHostLocalIpList+i, addresses+i);
    }
    else
        RvHostLogError((&logMgr->hostSource, "RvHostLocalGetAddress: RvHostGetIpList"));
#endif

    RvHostLogLeave((&logMgr->hostSource, "RvHostLocalGetAddress"));

    return status;
}


#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
/********************************************************************************************
 * RvHostMopiSetIpAddress
 * Dynamically sets the IP address of RTP or SIP (they have the same IP address) client.
 * INPUT   : address - address in dot notation, null terminated.
 * OUTPUT  : None.
 * RETURN  : RV_OK on success
 */
RVCOREAPI
RvStatus RVCALLCONV RvHostMopiSetIpAddress(RvChar *address)
{
    RvInt length;

    if (NULL == address)
    {
        return RV_ERROR_NULLPTR;
    }

    length = (NULL == address)? 0 : strlen(address);

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if (RV_HOST_MAX_NAMESIZE <= length)
        return RV_ERROR_UNKNOWN;
#endif

    memcpy(rvHostLocalName, address, length);

    return RV_OK;
}
#endif

#endif /* (RV_NET_TYPE != RV_NET_NONE) */

