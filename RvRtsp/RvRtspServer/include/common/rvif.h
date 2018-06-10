#ifndef _rvif_h
#define _rvif_h

#include "rvtypes.h"
#include "rvaddress.h"
#include "rvlog.h"


#define RV_IF_NONE 0
#define RV_IF_WSAIOCTL 1
#define RV_IF_GETIFADDRS 2

#ifndef RV_IF_TYPE
#  if RV_OS_TYPE == RV_OS_TYPE_WIN32
#    define RV_IF_TYPE RV_IF_WSAIOCTL
#  else
#    define RV_IF_TYPE RV_IF_NONE
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Flags */
#define RV_IF_DOWN 1
#define RV_IF_UP 2
#define RV_IF_ALL (RV_IF_DOWN | RV_IF_UP)

#define RV_IPV4_LOOPBACK 4
#define RV_IPV4_PRIVATE 8
#define RV_IPV4_GLOBAL 16
#define RV_IPV4_ALL_UP (RV_IPV4_LOOPBACK | RV_IPV4_PRIVATE | RV_IPV4_GLOBAL | RV_IF_UP)
#define RV_IPV4_ALL (RV_IPV4_LOOPBACK | RV_IPV4_PRIVATE | RV_IPV4_GLOBAL | RV_IF_ALL)

#define RV_IPV6_LOOPBACK 32
#define RV_IPV6_LINKLOCAL 64
#define RV_IPV6_SITELOCAL 128
#define RV_IPV6_GLOBAL 256
#define RV_IPV6_ALL_UP (RV_IPV6_LOOPBACK | RV_IPV6_LINKLOCAL | RV_IPV6_SITELOCAL | RV_IPV6_GLOBAL | RV_IF_UP)
#define RV_IPV6_ALL (RV_IPV6_LOOPBACK | RV_IPV6_LINKLOCAL | RV_IPV6_SITELOCAL | RV_IPV6_GLOBAL | RV_IF_ALL)


#define RV_IP_ALL_UP (RV_IPV4_ALL_UP | RV_IPV6_ALL_UP)
#define RV_IP_ALL (RV_IPV4_ALL | RV_IPV6_ALL)



typedef struct _RvIfInfo {
    RvUint32  flags;         /* OR'ed combination of various flags
                              * If flags & RV_IF_UP - appropriate interface is up and running
                              * If flags & RV_IPV4_PRIVATE - described addresses are IPv4 private addresses
                              */
    RvAddress unicastAddr;   /* unicast address */
    RvAddress subnetMask;    /* subnet mask */
    RvAddress broadcastAddr; /* broadcast address */
} RvIfInfo;



#if RV_IF_TYPE != RV_IF_NONE

/* RvStatus RvIfInfoGet(INOUT RvIfInfo *pIfinfo,
 *                      INOUT RvSize_t *pSize,
 *                      IN    RvUint32 flags,
 *                      IN    RvLogMgr *logMgr)
 * Parameters:
 *  pIfinfo - points to the vector of size *pSize of RvIfInfo structures.
 *            On successful return, those structures will contain information about various available
 *            IP addresses
 *  *pSize  -  As input parameter - size of pIfinfo vector.
 *             As output parameter:
 *              if return value is RV_OK - the number of filled RvIfInfo structures
 *              if return value is RV_ERROR_INSUFFICIENT_BUFFER - the actual number of available addresses
 *               in this case *pSize(in) < *pSize(out) and the number of filled RvIfInfo will be *pSize(in).
 *              To fetch all available addresses caller may call this function again with pIfinfo of
 *              appropriate size. Pay attention, there is no obligation that this second call will succeed
 *              due to dynamic nature of host network configuration.
 *  flags  -  OR'ed combination of flags. Only addresses that satisfies flags will be returned.
 *            if flags == RV_IPV4_GLOBAL | RV_IF_UP - only global IPv4 addresses on running interfaces
 *               will be returned.
 *            if flags == RV_IPV4_ALL - all IPv4 addresses will be returned
 *            if flags == RV_IPV4_GLOBAL | RV_IPV4_PRIVATE | RV_IF_ALL - all IPV4 adresses, except loopback
 *               will be returned

 *
 * Return value:
 * RV_OK - success
 * RV_ERROR_INSUFFICIENT_BUFFER - there is more addresses than the size of pIfinfo
 * any other - error
 *
 */

RVCOREAPI
RvStatus RvIfInfoGet(IN RvIfInfo *pIfinfo, INOUT RvSize_t *pSize, IN RvUint32 flags, IN RvLogMgr *logMgr);

/* Return
 * RV_OK - on the same network
 * > 0 - on differenet network
 * < 0 - error
 */
RVCOREAPI
RvStatus RvIfSameSubnet(RvAddress *localAddress, RvAddress *remoteAddress, RvLogMgr *logMgr);


#else

#define RvIfInfoGet(pinfo, psize, flags, log) ((void)(pinfo), (void)(psize), (void)(flags0(void)(log), RV_ERROR_NOTSUPPORTED)

#define RvIfSameSubnet(localAddress, remoteAddress, logMgr) \
    ((void)(localAddress), (void)(remoteAddress), (void)(logMgr), RV_ERROR_NOTSUPPORTED)



#endif

RVCOREAPI
RvStatus RvIfSameSubnetA(const RvChar *localAddress, const RvChar *remoteAddress, RvLogMgr *logMgr);


#ifdef __cplusplus
}
#endif

#endif
