#include "rvtypes.h"
#include "rverror.h"
#include "rvnettypes.h"
#include "rvselect.h"
#include "rvhost.h"


#include "rvlog.h"

#include "rvifmonitor.h"

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
#define LOG_ERR(f) if (logMgr) { RvLogError(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f));}
#define LOG_ERR_1(f,p1) if (logMgr) { RvLogError(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1));}

#define LOG_DBG(f) if (logMgr) { RvLogDebug(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f));}
#define LOG_DBG_1(f,p1) if (logMgr) { RvLogDebug(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1));}
#define LOG_DBG_2(f,p1,p2) if (logMgr) { RvLogDebug(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1,p2));}
#define LOG_DBG_3(f,p1,p2,p3) if (logMgr) { RvLogDebug(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1,p2,p3));}

#define LOG_ENTER(f) if (logMgr) { RvLogEnter(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f));}
#define LOG_ENTER_1(f,p1) if (logMgr) { RvLogEnter(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1));}
#define LOG_ENTER_2(f,p1,p2) if (logMgr) { RvLogEnter(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1,p2,));}
#define LOG_ENTER_3(f,p1,p2,p3) if (logMgr) { RvLogEnter(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1,p2,p3));}

#define LOG_LEAVE(f) if (logMgr) { RvLogLeave(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f));}
#define LOG_LEAVE_1(f,p1) if (logMgr) { RvLogLeave(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1));}
#define LOG_LEAVE_2(f,p1,p2) if (logMgr) { RvLogLeave(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1,p2));}
#define LOG_LEAVE_3(f,p1,p2,p3) if (logMgr) { RvLogLeave(&logMgr->selectSource, (&logMgr->selectSource, FUNCTION_NAME " " f, p1,p2,p3));}
#else
#define LOG_ERR(f) 
#define LOG_ERR_1(f,p1) 

#define LOG_DBG(f) 
#define LOG_DBG_1(f,p1) 
#define LOG_DBG_2(f,p1,p2) 
#define LOG_DBG_3(f,p1,p2,p3) 

#define LOG_ENTER(f) 
#define LOG_ENTER_1(f,p1) 
#define LOG_ENTER_2(f,p1,p2) 
#define LOG_ENTER_3(f,p1,p2,p3)

#define LOG_LEAVE(f) 
#define LOG_LEAVE_1(f,p1) 
#define LOG_LEAVE_2(f,p1,p2) 
#define LOG_LEAVE_3(f,p1,p2,p3) 
#endif


/* To enable interface monitoring, set this flag to '1' in rvusrconfig.h */
#if (RV_OS_TYPE == RV_OS_TYPE_LINUX)&& RV_USES_IFMONITOR

#include <ifaddrs.h>
#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>





typedef struct {
    RvInt                       udpsock;
    RvInt                       nlsock;
    RvSelectFd                  selFd;
    RvInterfaceState            currState;
    RvInterfaceMonitorOnEvent   appOnEvent;
    void*                       pAppCtx;
    char                        ifname[IFNAMSIZ+1];
    RvLogMgr*                   logMgr;
    RvSelectEngine*             selEngine;
} RvIfmonitorData;



static RvInterfaceState getInterfaceStatus(int udps, char *ifname)
{
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);

    if (ioctl(udps, SIOCGIFFLAGS, &ifr) == -1) {
        return RvInterfaceError;
    }

    return ifr.ifr_flags & IFF_RUNNING ? RvInterfaceUp : RvInterfaceDown;

}

static void nlSockCb(
    IN RvSelectEngine*  selectEngine,
    IN RvSelectFd*      fd,
    IN RvSelectEvents   selectEvent,
    IN RvBool           error)
{
#define FUNCTION_NAME "nlSockCb"    
    RvIfmonitorData *monData;
    RvLogMgr* logMgr;
    RvInt ret;
    RvChar buff[4096];
    RvInterfaceState istate;

    RV_UNUSED_ARG(selectEngine);


    monData = (RvIfmonitorData*)((RvChar*)fd-RV_OFFSETOF(RvIfmonitorData,selFd));
    logMgr = monData->logMgr;

    if (error)
    {
        LOG_ERR("Select report error");
        return;
    }

    if (selectEvent != RvSelectRead)
    {
        LOG_ERR_1("Unexpected event (%d) reported",selectEvent);
        return;
    }

    ret = recv(monData->nlsock,buff,sizeof(buff),0);
    if (ret < 0)
    {
        LOG_ERR_1("recv failed for NL socket with error %d",errno);
        return;
    }

    istate = getInterfaceStatus(monData->udpsock,monData->ifname);
    if (istate == RvInterfaceError)
    {
        LOG_ERR_1("interface %s reports error",monData->ifname);
        return;
    }

    if (istate == monData->currState)
        return;

    LOG_DBG_3("interface %s reports state change %d->%d",monData->ifname,monData->currState,istate);
    monData->currState = istate;

    (monData->appOnEvent)((RvInterfaceMonitorHandle)monData,monData->pAppCtx,istate);

#undef FUNCTION_NAME
}

/********************************************************************************************
 * RvInterfaceMonitorStart - Starts to monitor the network interface
 *
 * Application registers the callback to be called when the network interface state
 * changes.
 * This function must be called in the thread where the toolkit (SIP,ARTP etc) was
 * constructed.
 *
 * INPUT   : addr           - the address of the interface
 *           ifname         - the name of the interface. ("eth0" for example)
 *           One of addr or ifname must be provided.
 *           onEvent        - the callback to be called when the interface status changes
 *           pUsrCtx        - the application context
 *           logMgr         - the log manager (may be NULL)
 * OUTPUT  : phIfMon        - the monitor handle. It will be needed later for RvInterfaceMonitorStop
 *           pIfState       - if not NULL the current state of the interface will be returned.
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvInterfaceMonitorStart(
	IN RvAddress* addr, 
	IN RvChar* ifname, 
	IN RvInterfaceMonitorOnEvent onEvent,
	IN void* pUsrCtx,
	IN RvLogMgr *logMgr,
	OUT RvInterfaceMonitorHandle* phIfMon,
	OUT RvInterfaceState* pIfState)
{
#define FUNCTION_NAME "RvInterfaceMonitorStart"
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvChar addrTxt[256];
#endif
    RvSelectEngine* pSelEngine;
    RvChar ifnameStr[IFNAMSIZ+1], *i;
    RvInt udps, nlsock;
    RvInterfaceState iState;
    RvIfmonitorData* monData;
    RvStatus retv;

    LOG_ENTER("");

    if (RvSelectGetThreadEngine(logMgr,&pSelEngine) != RV_OK || pSelEngine == NULL)
    {
        LOG_ERR("RvSelectGetThreadEngine failed");
        return RV_ERROR_UNKNOWN;
    }

    if (!logMgr)
        logMgr = pSelEngine->logMgr;

    LOG_DBG_2("Called addr %s  ifname %s",
        (addr)?RvAddressGetString(addr,sizeof(addrTxt),addrTxt):"none",
        (ifname)?ifname:"");

    if (addr == NULL && (ifname == NULL || *ifname == 0))
    {
        LOG_ERR("No input data was provided");
        return RV_ERROR_UNKNOWN;
    }

    /* if no interface name was provided, get it */
    if (!ifname || !*ifname)
    {        
        struct ifaddrs *ifap, *ifa;
        union {
            struct sockaddr_in dummy;
            RvUint8 sockdata[RV_SOCKET_SOCKADDR_SIZE];
        } sockAddr;
        int sockAddrLen,len,offs;

        i = NULL;

        RvSocketAddressToSockAddr(addr,&sockAddr.sockdata,&sockAddrLen);

        if (getifaddrs(&ifap) != 0)
        {
            LOG_ERR_1("getifaddrs fails with error %d",errno);
            return RV_ERROR_UNKNOWN;
        }    

        len = (sockAddr.dummy.sin_family == AF_INET) ? 4:16;
        offs = (sockAddr.dummy.sin_family == AF_INET) ? 4:8;

        for (ifa = ifap; ifa; ifa = ifa->ifa_next) 
        {    
            if (sockAddr.dummy.sin_family == ifa->ifa_addr->sa_family &&
                memcmp((char*)&sockAddr+offs,(char*)ifa->ifa_addr+offs,len) == 0)
            {
                strcpy(ifnameStr,ifa->ifa_name);
                i = ifnameStr;
                break;
            }


        }
        freeifaddrs(ifap);

        if (i == NULL)
        {
            LOG_ERR_1("Could not find interface for address %s",RvAddressGetString(addr,sizeof(addrTxt),addrTxt));
            return RV_ERROR_UNKNOWN;
        }
        
        LOG_DBG_1("Could find the interface %s",i);
    }
    else
        i = ifname;


    /* test the  interface status */
    udps = socket(AF_INET,SOCK_DGRAM, 0);
    if (udps < 0)
    {
        LOG_ERR_1("failed to create the UDP socket, error is %d",errno);
        return RV_ERROR_UNKNOWN;
    }

    iState = getInterfaceStatus(udps,i);
    if (pIfState)    
        *pIfState = iState;


    LOG_DBG_2("The current state of intreface %s is %d",i,iState);

    /* now create the NETLINK socket */
    {
        struct sockaddr_nl nladdr;
    
        if ((nlsock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) 
        {
            LOG_ERR_1("failed to create the NETLINK socket, error is %d",errno);
            close(udps);
            return RV_ERROR_UNKNOWN;
        }
    
        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;
        nladdr.nl_groups = RTMGRP_LINK;
        nladdr.nl_pid = getpid();
    
        if (bind(nlsock, (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0) {
            LOG_ERR_1("failed to bind the NETLINK socket, error is %d",errno);
            close(nlsock);
            close(udps);
            return RV_ERROR_UNKNOWN;
        }

        LOG_DBG_1("Constructed bound NETLINK socket %d",nlsock);
    }

    /* allocate the memory */
    if (RvMemoryAlloc(NULL,sizeof(RvIfmonitorData),logMgr,(void**)&monData) != RV_OK)
    {
        LOG_ERR_1("RvMemoryAlloc failed",errno);
        return RV_ERROR_UNKNOWN;
    }
    memset(monData,0,sizeof(RvIfmonitorData));

    monData->currState = iState;
    monData->udpsock = udps;
    monData->nlsock = nlsock;  
    RvFdConstruct(&monData->selFd,&monData->nlsock,logMgr);
    monData->appOnEvent = onEvent;
    monData->pAppCtx = pUsrCtx;
    monData->logMgr = logMgr;
    monData->selEngine = pSelEngine;
    strcpy(monData->ifname,i);

    retv = RvSelectAdd(pSelEngine,&monData->selFd,RvSelectRead,nlSockCb);
    if (retv != RV_OK)
    {
        close(monData->nlsock);
        close(monData->udpsock);
        RvMemoryFree(monData,logMgr);
        LOG_ERR_1("RvSelectAdd failed for nl socket %d",monData->nlsock);
        return RV_ERROR_UNKNOWN;
    }

    *phIfMon = (RvInterfaceMonitorHandle) monData;

    LOG_LEAVE("");

    return RV_OK;
#undef FUNCTION_NAME
}

/********************************************************************************************
 * RvInterfaceMonitorStop - Stops to monitor the network interface
 *
 *
 * INPUT   : hIfMon         - the monitor handle created by RvInterfaceMonitorStart
 * OUTPUT  : None
 * RETURN  : None
 */
void RvInterfaceMonitorStop(
	IN RvInterfaceMonitorHandle hIfMon)
{
    RvIfmonitorData* monData = (RvIfmonitorData*) hIfMon;
    RvLogMgr* logMgr;

#define FUNCTION_NAME "RvInterfaceMonitorStop"

    LOG_ENTER_1("Called for %s",monData->ifname);

    RvSelectRemoveEx(monData->selEngine,&monData->selFd,RV_TRUE,NULL);

    close(monData->udpsock);
    close(monData->nlsock);

    RvFdDestruct(&monData->selFd);

    RvMemoryFree((void*)monData,logMgr);

    LOG_LEAVE("");

#undef FUNCTION_NAME
}

#elif (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <windows.h>

RvStatus RVCALLCONV RvHostWindowsGetIpV4List(
    IN      RvLogMgr*   logMgr,
    IN      RvUint32    numOfIpsAllocated,
    OUT     RvUint32*   numOfIps,
    OUT     RvAddress*  ipList);


RVCOREAPI RvStatus RVCALLCONV 
RvInterfaceMonitorRegister(
    IN  RvSelectEngine*     pSelEng, 
    IN  RvOnAddressChange   pListener,
    IN  void*               pAppCtx,
    OUT RvInt*              pListenerInd)
{
#define FUNCTION_NAME "RvInterfaceMonitorRegister"
    RvLogMgr* logMgr = pSelEng->logMgr;
    RvStatus retv = RV_OK;
    RvInt cnt;

    RvLockGet(&pSelEng->lock,pSelEng->logMgr);
    logMgr = pSelEng->logMgr;
    LOG_ENTER("");

    for (cnt = 0; cnt < RV_ADDR_MONITOR_MAX_LISTENERES; cnt++)
    {
        if (pSelEng->onAddressChangeListenerers[cnt] == NULL)
            break;
    }

    if (cnt == RV_ADDR_MONITOR_MAX_LISTENERES)
    {
        LOG_ERR("No more room for listeners");
        retv = RV_ERROR_INSUFFICIENT_BUFFER;
        goto finish;
    }

    if (pSelEng->interfaceMonitorEvent == NULL)
    {
        retv = RvHostWindowsGetIpV4List(pSelEng->logMgr,RV_ADDR_MONITOR_MAX_CURRADDRS,
            (RvUint32*)&pSelEng->currentAddrsNum,pSelEng->currentAddrs);
        if (retv != RV_OK)
        {
            LOG_ERR("Failed in RvInterfaceMonitorGetAddrs");
            retv = RV_ERROR_UNKNOWN;
            goto finish;
        }
        LOG_DBG_1("Could fetch %d addresses",pSelEng->currentAddrsNum);

        pSelEng->interfaceMonitorEvent = WSACreateEvent();
        if (pSelEng->interfaceMonitorEvent ==  WSA_INVALID_EVENT)
        {
            LOG_ERR_1("Failed in WSACreateEvent, error %d",WSAGetLastError());
            retv = RV_ERROR_UNKNOWN;
            goto finish;
        }
        LOG_DBG("Could create the event");
        pSelEng->interfaceChangeDetected = RV_TRUE;
    }

    pSelEng->onAddressChangeListenerers[cnt] = pListener;
    pSelEng->onAddressChangeListenerersCtx[cnt] = pAppCtx;
    pSelEng->onAddressChangeListenerersCnt ++;

    if (pListenerInd)
        *pListenerInd = cnt;

finish:
    LOG_LEAVE_2(" Leaving with %d, listeneresCnt %d",retv,pSelEng->onAddressChangeListenerersCnt);
    RvLockRelease(&pSelEng->lock,pSelEng->logMgr);

    return retv;
#undef FUNCTION_NAME
}

RVCOREAPI RvStatus RVCALLCONV 
RvInterfaceMonitorUnregister(
    IN  RvSelectEngine*         pSelEng, 
    IN  void*                   pAppCtx,
    IN  RvInt*                  pListenerInd)
{
#define FUNCTION_NAME "RvInterfaceMonitorUnregister"
    RvLogMgr* logMgr = pSelEng->logMgr;
    RvStatus retv = RV_OK;
    RvInt cnt;
    RvBool found = RV_FALSE;

    RvLockGet(&pSelEng->lock,pSelEng->logMgr);
    logMgr = pSelEng->logMgr;
    LOG_ENTER("");

    if (pAppCtx)
    {
        for (cnt = 0; cnt < RV_ADDR_MONITOR_MAX_LISTENERES; cnt++)
        {
            if (pSelEng->onAddressChangeListenerersCtx[cnt] == pAppCtx)
            {
                found = RV_TRUE;
                pSelEng->onAddressChangeListenerers[cnt] = NULL;
                pSelEng->onAddressChangeListenerersCtx[cnt] = NULL;
                break;
            }
        }
    }
    else if (*pListenerInd && pSelEng->onAddressChangeListenerers[*pListenerInd])
    {
        found = RV_TRUE;
        pSelEng->onAddressChangeListenerers[*pListenerInd] = NULL;
        pSelEng->onAddressChangeListenerersCtx[*pListenerInd] = NULL;
    }

    if (found)
    {
        pSelEng->onAddressChangeListenerersCnt --;
        LOG_DBG_1("Found the listerener to be removed, listeners now %d",
            pSelEng->onAddressChangeListenerersCnt);
    }
    else {
        LOG_DBG_1("Not found the listerener to be removed, listeners now %d",
            pSelEng->onAddressChangeListenerersCnt);
        retv = RV_ERROR_BADPARAM;
    }

    LOG_LEAVE_2(" Leaving with %d, listeneresCnt %d",retv,pSelEng->onAddressChangeListenerersCnt);
    RvLockRelease(&pSelEng->lock,pSelEng->logMgr);

    return retv;
#undef FUNCTION_NAME
}


/*
RvStatus RvInterfaceMonitorGetAddrs(
    IN      RvAddress *addrs,
    INOUT   RvInt      addrsNum,
    IN      RvLogMgr*  logMgr)
{   
#define FUNCTION_NAME "RvInterfaceMonitorGetAddrs"
    DWORD dwSize, dwRetVal;
    MIB_IPADDRTABLE IPAddrTable[10], *pIPAddrTable;
    IN_ADDR IPAddr;
    RvInt i;
    RvBool allocated = RV_FALSE;
    RvStatus retv;

    pIPAddrTable = IPAddrTable;

    dwSize = sizeof(IPAddrTable);
    dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) 
    {
        pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
        if (pIPAddrTable == NULL) 
        {
            LOG_ERR_1("Failed to allocate the %d bytes",dwSize);
            retv = RV_ERROR_OUTOFRESOURCES;
            goto finish;
        }
        allocated = RV_TRUE;
        dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 );
    }

    if (dwRetVal != NO_ERROR ) { 
        LOG_ERR_1("GetIpAddrTable failed with %d",dwRetVal);
        retv = RV_ERROR_UNKNOWN;
        goto finish;
    }

    for (i=0; i < (int) pIPAddrTable->dwNumEntries; i++) {
        printf("\n\tInterface Index[%d]:\t%ld\n", i, pIPAddrTable->table[i].dwIndex);
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwAddr;
        printf("\tIP Address[%d]:     \t%s\n", i, inet_ntoa(IPAddr) );
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwMask;
        printf("\tSubnet Mask[%d]:    \t%s\n", i, inet_ntoa(IPAddr) );
        IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwBCastAddr;
        printf("\tBroadCast[%d]:      \t%s (%ld%)\n", i, inet_ntoa(IPAddr), pIPAddrTable->table[i].dwBCastAddr);
        printf("\tReassembly size[%d]:\t%ld\n", i, pIPAddrTable->table[i].dwReasmSize);
        printf("\tType and State[%d]:", i);
        if (pIPAddrTable->table[i].wType & MIB_IPADDR_PRIMARY)
            printf("\tPrimary IP Address");
        if (pIPAddrTable->table[i].wType & MIB_IPADDR_DYNAMIC)
            printf("\tDynamic IP Address");
        if (pIPAddrTable->table[i].wType & MIB_IPADDR_DISCONNECTED)
            printf("\tAddress is on disconnected interface");
        if (pIPAddrTable->table[i].wType & MIB_IPADDR_DELETED)
            printf("\tAddress is being deleted");
        if (pIPAddrTable->table[i].wType & MIB_IPADDR_TRANSIENT)
            printf("\tTransient address");
        printf("\n");
    }

    retv = RV_OK;

finish:
    if (allocated)
        free(pIPAddrTable);

    return retv;
#undef FUNCTION_NAME
}
*/

void 
RvInterfaceMonitorChangeDetected(
    RvSelectEngine* pSelEng)
{
#define FUNCTION_NAME "RvInterfaceMonitorChangeDetected"
    RvAddress addrs[RV_ADDR_MONITOR_MAX_CURRADDRS];
    RvAddress addrsAdded[RV_ADDR_MONITOR_MAX_CURRADDRS];
    RvAddress addrsRemoved[RV_ADDR_MONITOR_MAX_CURRADDRS];
    RvInt added, removed, cnt1, cnt2, addrSz;
    RvStatus retv;
    RvBool found;
    RvOnAddressChange       currListeners[RV_ADDR_MONITOR_MAX_LISTENERES];
    void*                   currCtx[RV_ADDR_MONITOR_MAX_LISTENERES];
    RvInt                   currListCnt;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvChar txtAddr[256];
    RvLogMgr* logMgr = pSelEng->logMgr;
#endif

    RvLockGet(&pSelEng->lock,pSelEng->logMgr);

    LOG_ENTER("Called");
    added = removed = 0;

    retv = RvHostWindowsGetIpV4List(pSelEng->logMgr,RV_ADDR_MONITOR_MAX_CURRADDRS,
        (RvUint32*)&addrSz,addrs);
    if (retv != RV_OK)
    {
        LOG_ERR("Failed in RvInterfaceMonitorGetAddrs");
        RvLockRelease(&pSelEng->lock,pSelEng->logMgr);
        return;
    }

    LOG_DBG_2("Could get %d addresses, saved %d",addrSz,pSelEng->currentAddrsNum);


     /* find added */
    for (cnt1 = 0; cnt1 < addrSz; cnt1 ++)
    {
        found = RV_FALSE;
        for (cnt2 = 0; cnt2 < pSelEng->currentAddrsNum; cnt2++)
        {
            if (RvAddressCompare(addrs+cnt1,pSelEng->currentAddrs+cnt2,RV_ADDRESS_BASEADDRESS))
            {
                found = RV_TRUE;
                break;
            }
        }
        if (!found)
        {
            RvAddressCopy(addrs+cnt1,addrsAdded+added);
            added ++;
            LOG_DBG_2("Added address %s (total added %d)",
                RvAddressGetString(addrs+cnt1,sizeof(txtAddr),txtAddr),added);
        }
    }

    /* find removed */
    for (cnt1 = 0; cnt1 < pSelEng->currentAddrsNum; cnt1 ++)
    {
        found = RV_FALSE;
        for (cnt2 = 0; cnt2 < addrSz; cnt2++)
        {
            if (RvAddressCompare(addrs+cnt2,pSelEng->currentAddrs+cnt1,RV_ADDRESS_BASEADDRESS))
            {
                found = RV_TRUE;
                break;
            }
        }
        if (!found)
        {
            RvAddressCopy(pSelEng->currentAddrs+cnt1,addrsRemoved+removed);
            removed ++;
            LOG_DBG_2("Removed address %s (total removed %d)",
                RvAddressGetString(pSelEng->currentAddrs+cnt1,sizeof(txtAddr),txtAddr),removed);
        }
    }

    if (!added && !removed)
    {
        LOG_LEAVE("No added or removed");   
        RvLockRelease(&pSelEng->lock,pSelEng->logMgr);
        return;
    }

    /* copy all */
    memcpy(pSelEng->currentAddrs,addrs,addrSz*sizeof(RvAddress));
    pSelEng->currentAddrsNum = addrSz;

    memcpy(currListeners,pSelEng->onAddressChangeListenerers,pSelEng->onAddressChangeListenerersCnt*sizeof(void*));
    memcpy(currCtx,pSelEng->onAddressChangeListenerersCtx,pSelEng->onAddressChangeListenerersCnt*sizeof(void*));
    currListCnt = pSelEng->onAddressChangeListenerersCnt;
    RvLockRelease(&pSelEng->lock,pSelEng->logMgr);

    for (cnt1 = 0; cnt1 < currListCnt; cnt1++)
    {
        (currListeners[cnt1])(currCtx[cnt1],addrsAdded,added,addrsRemoved,removed);
    }

    LOG_LEAVE_2("Leaving added %d, removed %d",added,removed);   

#undef FUNCTION_NAME
}

#endif /*(RV_OS_TYPE == RV_OS_TYPE_LINUX)&& RV_USES_IFMONITOR */

RVCOREAPI RvStatus RVCALLCONV
RvInterfaceGetMacAddresses(
    INOUT   RvMacAddress*       macAddrs,
    INOUT   RvUint*             pNumOfMacs)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    RvStatus retV = RV_OK;
    DWORD s;
    RvChar buff[1024], *pBuff;
    DWORD buffSz;
    PIP_ADAPTER_INFO p;
    RvBool wasAllocated = RV_FALSE;
    RvUint         numOfMacs = *pNumOfMacs;

    *pNumOfMacs = 0;

    pBuff = buff;
    buffSz = sizeof(buff);
    s = GetAdaptersInfo((PIP_ADAPTER_INFO)pBuff,&buffSz);
    if (s != NO_ERROR)
    {
        if (s == ERROR_BUFFER_OVERFLOW)
        {
            pBuff = malloc(buffSz);
            if (pBuff == NULL)
                return RV_ERROR_UNKNOWN;
            wasAllocated = RV_TRUE;
            s = GetAdaptersInfo((PIP_ADAPTER_INFO)pBuff,&buffSz);
            if (s != NO_ERROR)
            {
                retV = RV_ERROR_UNKNOWN;
                goto atEnd;
            }
        }
    }
    p = (PIP_ADAPTER_INFO)pBuff;

    while (p)
    {
        static RvUint8 MacZeros[RV_MAC_ADDR_LENGTH] = {0,0,0,0,0,0};
        static RvUint8 MacFFs[RV_MAC_ADDR_LENGTH] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        if (p->AddressLength != RV_MAC_ADDR_LENGTH)
            goto next;

        if (memcmp(p->Address,MacZeros,RV_MAC_ADDR_LENGTH) == 0 || memcmp(p->Address,MacFFs,RV_MAC_ADDR_LENGTH) == 0)
            goto next;
 
        memcpy(macAddrs[*pNumOfMacs].Address,p->Address,RV_MAC_ADDR_LENGTH);
        macAddrs[*pNumOfMacs].AddressLen = RV_MAC_ADDR_LENGTH;
        (*pNumOfMacs)++;
        if (*pNumOfMacs == numOfMacs)
            break;

next:
        p = p->Next;
    }


atEnd:
    if (wasAllocated)
        free(pBuff);
    return retV;
#else
    RV_UNUSED_ARG(macAddrs);
    RV_UNUSED_ARG(pNumOfMacs);
    return RV_ERROR_NOTSUPPORTED;
#endif

}

RVCOREAPI RvStatus RVCALLCONV
RvInterfaceGetInterfaceInfoByDestination(
    IN      RvAddress*          pDestAddr,
    OUT     RvInterfaceData*    pInterfData)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    union {
        struct sockaddr_in dummy;
        RvUint8 sockdata[RV_SOCKET_SOCKADDR_SIZE];
    } sockAddr;
    int sockAddrLen;
    RvStatus status, retV = RV_OK;
    DWORD s,interfInd;
    RvChar buff[1024], *pBuff;
    DWORD buffSz;
    PIP_ADAPTER_INFO p;
    MIB_IFROW ifr;
    IP_ADDR_STRING *a;
    RvBool wasAllocated = RV_FALSE;

    memset(pInterfData,0,sizeof(RvInterfaceData));

    status = RvSocketAddressToSockAddr(pDestAddr,&sockAddr.sockdata,&sockAddrLen);
    if (status != RV_OK)
        return RV_ERROR_UNKNOWN;

    s = GetBestInterfaceEx((struct sockaddr*)&sockAddr.dummy,&interfInd);
    if (s != NO_ERROR)
    {
        return RV_ERROR_UNKNOWN;
    }

    pBuff = buff;
    buffSz = sizeof(buff);
    s = GetAdaptersInfo((PIP_ADAPTER_INFO)pBuff,&buffSz);
    if (s != NO_ERROR)
    {
        if (s == ERROR_BUFFER_OVERFLOW)
        {
            pBuff = malloc(buffSz);
            if (pBuff == NULL)
                return RV_ERROR_UNKNOWN;
            wasAllocated = RV_TRUE;
            s = GetAdaptersInfo((PIP_ADAPTER_INFO)pBuff,&buffSz);
            if (s != NO_ERROR)
            {
                retV = RV_ERROR_UNKNOWN;
                goto atEnd;
            }
        }
    }
    p = (PIP_ADAPTER_INFO)pBuff;

    while (p)
    {
        if (p->Index == interfInd)
            break;
        p = p->Next;
    }

    if (!p)
    {
        /* meanwhile we support IPv4 only here */
        static RvAddress loopBackAddr;
        static RvBool fTime = RV_TRUE;
        RvAddress locAddrs[5];
        RvUint32 numOfIps = 5;
        if (fTime)
        {
            fTime = RV_FALSE;
            RvAddressConstruct(RV_ADDRESS_TYPE_IPV4,&loopBackAddr);
            RvAddressSetString("127.0.0.1",&loopBackAddr);
        }
        if (RvAddressCompare(pDestAddr,&loopBackAddr,RV_ADDRESS_BASEADDRESS))
        {
            RvAddressCopy(&loopBackAddr,pInterfData->pAddrs);
            pInterfData->addrsNum = 1;
            goto afterAddrs;
        }

        retV = RvHostGetIpList(NULL,NULL,&numOfIps,locAddrs);
        if (retV == RV_OK)
        {
            RvUint32 cnt;
            for (cnt = 0; cnt < numOfIps; cnt++)
            {
                if (RvAddressCompare(pDestAddr,locAddrs+cnt,RV_ADDRESS_BASEADDRESS))
                { 
                    RvAddressCopy(locAddrs+cnt,pInterfData->pAddrs);
                    pInterfData->addrsNum = 1;
                    goto afterAddrs;
                }
            }
        }

        /* could not find the interface with the relevant index */
        retV = RV_ERROR_UNKNOWN;
        goto atEnd;
    }

    a = &p->IpAddressList;
    while (a && pInterfData->addrsNum < RV_INTERFACE_ADDRS_MAX_NUM)
    {
        RvAddress *addr;
        addr = pInterfData->pAddrs + pInterfData->addrsNum;

        /* skip the 0.0.0.0 address */
        if (strcmp(a->IpAddress.String,"0.0.0.0") == 0)
            goto next;

        /* define whether this is IPv4 or IPv6 */
        RvAddressConstruct(RV_ADDRESS_TYPE_IPV4,addr);

        if (RvAddressSetString(a->IpAddress.String,addr))
        {
            pInterfData->addrsNum++;
        }
#if (RV_NET_TYPE & RV_NET_IPV6)
        else {
            RvAddressConstruct(RV_ADDRESS_TYPE_IPV6,addr);
            if (RvAddressSetString(a->IpAddress.String,addr))
            {
                pInterfData->addrsNum++;
            }
        }
#endif
next:
        a = a->Next;
    }

    if (pInterfData->addrsNum == 0)
    {
        /* could not find any addresses */
        retV = RV_ERROR_UNKNOWN;
        goto atEnd;
    }

afterAddrs:
    ifr.dwIndex = interfInd;
    s = GetIfEntry(&ifr);
    if (s == NO_ERROR)
    {
        pInterfData->interfaceSpeed = ifr.dwSpeed;
        strncpy(pInterfData->intrfaceDescrription,(const char*) ifr.bDescr,RV_INTERFACE_DESCR_MAX_LEN);
    }

atEnd:
    if (wasAllocated)
        free(pBuff);
    return retV;

#else
    RV_UNUSED_ARG(pDestAddr);
    RV_UNUSED_ARG(pInterfData);
    return RV_ERROR_NOTSUPPORTED;
#endif
}


