#include "rvif.h"
#include "rvlog.h"
#include "rvmemory.h"

#if RV_IF_TYPE != RV_IF_NONE

static
RvBool IsEqualMask(RvUint8 *addr1, RvUint8 *addr2, RvUint8 *mask, RvInt size) {
    int i;

    for(i = 0; i < size; i++, addr1++, addr2++, mask++) {
        RvUint8 m1 = *addr1 & *mask;
        RvUint8 m2 = *addr2 & *mask;
        if(m1 != m2) {
            return RV_FALSE;
        }
    }

    return RV_TRUE;
}

#endif

#if RV_IF_TYPE == RV_IF_WSAIOCTL

#include <Winsock2.h>
#include <Mstcpip.h>
#include <ws2tcpip.h>

RvUint8 localMask[16] = {0xfe, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
RvUint8 linkLocal[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
RvUint8 siteLocal[16] = {0xfe, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
RvUint8 loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
RvUint8 loopbackMask[16] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


#define Ipv6IsLoopback(addr)  IsEqualMask(addr, loopback, loopbackMask, 16)
#define Ipv6IsLinkLocal(addr) IsEqualMask(addr, linkLocal, localMask, 16)
#define Ipv6IsSiteLocal(addr) IsEqualMask(addr, siteLocal, localMask, 16)

/* Returns Ipv4 address as RvUint32 from sockaddr_in */
#define Ipv4FromSockIn(si) ((si)->sin_addr.s_addr)

/* Returns Ipv4 address as RvUint32 from sockaddr_gen */
#define Ipv4FromSockGen(sg) Ipv4FromSockIn(&(sg)->AddressIn)

/* Returns Ipv6 address as pointer to 16 bytes from sockaddr_in6 */
#define Ipv6FromSockIn6(si) ((si)->sin6_addr.s6_addr)

/* Returns Ipv6 address as pointer to 16 bytes from sockaddr_gen */
#define Ipv6FromSockGen(sg) Ipv6FromSockIn6((struct sockaddr_in6*)(&(sg)->AddressIn))

static 
RvInt AddrType(struct sockaddr_in *addr) {
    if(addr->sin_family == AF_INET) {
        RvUint8 *baddr = (RvUint8 *)&addr->sin_addr.s_addr;

        if(baddr[0] == 127) {
            return RV_IPV4_LOOPBACK;
        }

        if(baddr[0] == 10 ||
           (baddr[0] == 172 && baddr[1] >= 16 && baddr[1] <= 31) ||
           (baddr[0] == 192 && baddr[1] == 168)) {
               return RV_IPV4_PRIVATE;
        }

        return RV_IPV4_GLOBAL;
    } else if(addr->sin_family == AF_INET6){
        RvUint8 *saddr = ((struct sockaddr_in6 *)addr)->sin6_addr.u.Byte;

        if(Ipv6IsLoopback(saddr)) {
            return RV_IPV6_LOOPBACK;
        }

        if(Ipv6IsLinkLocal(saddr)) {
            return RV_IPV6_LINKLOCAL;
        }

        if(Ipv6IsSiteLocal(saddr)) {
            return RV_IPV6_SITELOCAL;
        }

        return RV_IPV6_GLOBAL;
    }

    return 0;
}

static
RvStatus GetInterfaceInfo(INTERFACE_INFO **ppIfInfo, DWORD *pIfInfoSize, RvLogMgr *logMgr) {
#define SOCKET_CREATED 1

    SOCKET sock = INVALID_SOCKET;
    RvStatus s = RV_OK;
    int stage = 0;
    INTERFACE_INFO *localAddrs = *ppIfInfo;
    DWORD localAddrsSize = *pIfInfoSize;
    DWORD bytesReturned;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock == INVALID_SOCKET) {
        s = RV_ERROR_UNKNOWN;
        goto failure;
    }

    stage |= SOCKET_CREATED;

    for(;;) {
        int res;

        /* Ask for address list in IPv4 domain */
        res = WSAIoctl(sock, SIO_GET_INTERFACE_LIST, NULL, 0, (LPVOID)localAddrs,
            localAddrsSize, &bytesReturned, NULL, NULL);

        if(res != SOCKET_ERROR) {
            break;
        }

        res = WSAGetLastError();
        if(res != WSAEFAULT) {
            s = RV_ERROR_UNKNOWN;
            goto failure;
        }

        localAddrsSize <<= 1;
        if(localAddrsSize == 0) {
            localAddrsSize = sizeof(*localAddrs);
        }

        if(localAddrs != *ppIfInfo) {
            RvMemoryFree(localAddrs, logMgr);
            localAddrs = 0;
        }

        s = RvMemoryAlloc(0, localAddrsSize, logMgr, (void **)&localAddrs);
        if(s != RV_OK) {
            goto failure;
        }
    }

    *ppIfInfo = localAddrs;
    *pIfInfoSize = localAddrsSize;

failure:
    if(stage & SOCKET_CREATED) {
        closesocket(sock);
    }

    return s;
}

RvStatus IfInfoGet_WSAIOCTL(RvIfInfo *pIfinfo, RvSize_t *pIfInfoSize, RvUint32 flags, RvLogMgr *logMgr) {
#define MAX_LOCAL_ADDRS 16
    RvStatus s = RV_OK;
    RvUint32 numLocalAddr;
    RvUint32 i,j;
    INTERFACE_INFO localAddrsBuf[MAX_LOCAL_ADDRS];
    INTERFACE_INFO *localAddrs = localAddrsBuf;
    DWORD localAddrsSize = sizeof(localAddrsBuf);

    RvSize_t ifInfoSize = *pIfInfoSize;

    s = GetInterfaceInfo(&localAddrs, &localAddrsSize, logMgr);
    if(s != RV_OK) {
        return s;
    }
    
    numLocalAddr = (localAddrsSize / sizeof(*localAddrs));

    for(i = 0, j = 0; j < numLocalAddr; j++)
    {
        INTERFACE_INFO *cur = &localAddrs[j];
        RvUint32 mask = 0;
        RvUint32 addrType;
        RvIfInfo *curif;


        mask = cur->iiFlags & IFF_UP ? RV_IF_UP : RV_IF_DOWN;
        if((mask & flags) == 0) {
            continue;
        }
        
        addrType = AddrType(&cur->iiAddress.AddressIn);

        if(!(addrType & flags)) {
            continue;
        }
        
        if(i >= ifInfoSize) {
            i++;
            continue;
        }

        curif = &pIfinfo[i];
        curif->flags = mask | addrType;

        if(addrType & RV_IPV4_ALL) {
            RvUint32 iaddr = Ipv4FromSockGen(&cur->iiAddress);
            (void)RvAddressConstructIpv4(&curif->unicastAddr, iaddr, RV_ADDRESS_IPV4_ANYPORT);
            iaddr = Ipv4FromSockGen(&cur->iiNetmask);
            (void)RvAddressConstructIpv4(&curif->subnetMask, iaddr, RV_ADDRESS_IPV4_ANYPORT);
            iaddr = Ipv4FromSockGen(&cur->iiBroadcastAddress);
            (void)RvAddressConstructIpv4(&curif->broadcastAddr, iaddr, RV_ADDRESS_IPV4_ANYPORT);
        } else {
            RvUint8 *baddr = Ipv6FromSockGen(&cur->iiAddress);
            (void)RvAddressConstructIpv6(&curif->unicastAddr, baddr, 0, 0);
            baddr = Ipv6FromSockGen(&cur->iiNetmask);
            (void)RvAddressConstructIpv6(&curif->subnetMask, baddr, 0, 0);
            baddr = Ipv6FromSockGen(&cur->iiBroadcastAddress);
        }

        i++;
    }

    if(i >= ifInfoSize) {
        s = RV_ERROR_INSUFFICIENT_BUFFER;
    }

    if(localAddrs != localAddrsBuf) {
        RvMemoryFree(localAddrs, logMgr);
    }

    *pIfInfoSize = i;
    return s;
}


RVCOREAPI
RvStatus RvIfInfoGet(RvIfInfo *pIfinfo, RvSize_t *pIfInfoSize, RvUint32 flags, RvLogMgr *logMgr) {
    return IfInfoGet_WSAIOCTL(pIfinfo, pIfInfoSize, flags, logMgr);
}


RVCOREAPI
RvStatus RvIfSameSubnet(RvAddress *localAddress, RvAddress *remoteAddress, RvLogMgr *logMgr) {
    INTERFACE_INFO localAddrsBuf[MAX_LOCAL_ADDRS];
    INTERFACE_INFO *localAddrs = localAddrsBuf;
    DWORD localAddrsSize = sizeof(localAddrsBuf);
    RvStatus s = RV_OK;
    int nLocalAddrs;
    int i;
    RvInt addrType;
    const RvAddressIpv4 *ip4;
    RvUint32 lip;
    RvUint32 rip;
    RvUint32 mask = 0;

    addrType = RvAddressGetType(localAddress);
    if(addrType != RV_ADDRESS_TYPE_IPV4) {
        return RV_ERROR_NOTSUPPORTED;
    }


    s = GetInterfaceInfo(&localAddrs, &localAddrsSize, logMgr);
    if(s != RV_OK) {
        return s;
    }

    ip4 = RvAddressGetIpv4(localAddress);
    lip = RvAddressIpv4GetIp(ip4);

    nLocalAddrs = localAddrsSize / sizeof(*localAddrs);
    for(i = 0; i < nLocalAddrs; i++) {
        INTERFACE_INFO *cur = &localAddrs[i];
        RvUint32 curip = Ipv4FromSockGen(&cur->iiAddress);
        if(curip == lip) {
            mask = Ipv4FromSockGen(&cur->iiNetmask);
            break;
        }
    }

    if(localAddrs != localAddrsBuf) {
        RvMemoryFree(localAddrs, logMgr);
    }

    if(i >= nLocalAddrs) {
        return RV_ERROR_UNKNOWN;
    }

    ip4 = RvAddressGetIpv4(remoteAddress);
    rip = RvAddressIpv4GetIp(ip4);

    return (lip & mask) == (rip & mask) ? RV_OK : 1;
}




#endif

#if RV_IF_TYPE == RV_IF_GETIFADDRS

#include <sys/types.h>
#include <ifaddrs.h>

RVCOREAPI
RvStatus RvIfSameSubnet(const RvAddress *localAddress, const RvAddress *remoteAddress, RvLogMgr *logMgr) {
    struct ifaddrs *ifa;
    struct ifaddrs *cur;
    RvInt ltype = RvAddressGetType(localAddress);
    RvInt rtype = RvAddressGetType(remoteAddress);
    int res;
    RvUint32 lipv4;
    RvUint32 ripv4;
    RvUint32 *lipv6;
    RvUint32 *ripv6;
    int af;
    
    if(ltype != rtype) {
        return RV_ERROR_BADPARAM;
    }

    if(ltype == RV_ADDRESS_TYPE_IPV4) {
        RvAddressIpv4 *rvipv4 = RvAddressGetIpv4(localAddress);
        lipv4 = RvAddressIpv4GetIp(rvipv4);
        lipv4 = htonl(lipv4);
        rvipv4 = RvAddressGetIpv4(remoteAddress);
        ripv4  = RvAddressIpv4GetIp(rvipv4);
        ripv4  = htonl(ripv4);
        af == AF_INET;
    } else {
        RvAddressIpv6 *rvipv6 = RvAddressGetIpv6(localAddress);
        lipv6 = (RvUint32 *)RvAddressIpv6GetIp(rvipv6);

        rvipv6 = RvAddressGetIpv6(remoteAddress);
        ripv6 = (RvUint32 *)RvAddressIpv6GetIp(rvipv6);
        af = AF_INET6;
    }

    res = getifaddrs(&ifa);

    if(res != 0) {
        return RvOsError(errno);
    }

    for(cur = ifa; cur != 0; cur = cur->ifa_next) {
        struct sockaddr *sin = cur->ifa_addr;
        if(sin->sa_family != af) {
            continue;
        }

        if(af == AF_INET) {
            struct sockaddr_in *sin4 = (struct sockaddr_in *)sin;
            struct sockaddr_in *smask; 
            RvUint32 ipv4 = sin4->in_addr.saddr;
            RvUint32 mask;

            if(ipv4 != lipv4) {
                continue;
            }

            smask = (struct sockaddr_in *)cur->ifa_netmask;
            mask = smask->in_addr.saddr;
            return lipv4 & mask == ripv4 & mask ? RV_OK : 1;
        }

        if(af == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sin;
            struct sockaddr_in6 *smask6;
            RvUint32 *ipv6 = sin6->in6_addr.s6_addr32;
            RvUint32 *mask6;

            int iseq = !memcmp(ipv6, lipv6, 16);

            if(!iseq) {
                continue;
            }

            smask6 = = (struct sockaddr_in6 *)cur->ifa_netmask;
            mask6 = smask6->sin6_addr.s6_addr32;

            return IsEqualMask(lipv6, ripv6, mask6);
        }
    }

    return 1;
}


#endif


RVCOREAPI
RvStatus RvIfSameSubnetA(const RvChar *localAddress, const RvChar *remoteAddress, RvLogMgr *logMgr) {
    RvAddress la;
    RvAddress ra;
    RvAddress *ret;
    RvStatus s;

    RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &la);
    ret = RvAddressSetString(localAddress, &la);
    if(ret == 0) {
        return RV_ERROR_NOTSUPPORTED;
    }

    RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &ra);
    ret = RvAddressSetString(remoteAddress, &ra);
    if(ret == 0) {
        RvAddressDestruct(&la);
        return RV_ERROR_NOTSUPPORTED;
    }

    s = RvIfSameSubnet(&la, &ra, logMgr);
    RvAddressDestruct(&la);
    RvAddressDestruct(&ra);
    return s;
}




