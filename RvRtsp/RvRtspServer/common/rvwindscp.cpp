#include "rvmemory.h"
#include "rvlock.h"
#include "rvlog.h"
#include "rvwindscp.h"
#include "rvccoreglobals.h"

#pragma warning(push)
#pragma warning(disable:4995)  // Traffic Control was deprecated in Vista timeframe
#pragma warning(disable:4127)  // conditional expression is constant
#pragma warning(disable:4201)  // nameless struct/union

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#if RV_WIN_DSCP == RV_WIN_DSCP_TC
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ntddndis.h>
#include <traffic.h>


#include "rvsocket.h"

/* Use Traffic Control API */
#define NDSCPS 64


typedef struct _DscpPerIfcInfo DscpPerIfcInfo;

/* For each possible DSCP value */
typedef struct {
    RvInt refCount;    /* number of sockets with this DSCP set at this moment */
    DscpPerIfcInfo *first; /* for each TC-enabled interface we keep flow handle associated with this DSCP */
} DscpInfo;

struct _DscpPerIfcInfo {
	DscpPerIfcInfo *next;
	HANDLE hIfc;      /* interface handle */
	HANDLE hFlow;     /* flow handle */
};

typedef struct {
    DscpInfo dscps[NDSCPS];
} DscpDB;

typedef struct _SocketPerIfcInfo {
	struct _SocketPerIfcInfo *next;
    HANDLE hIfc;
    HANDLE hFilter;
} SocketPerIfcInfo;

typedef struct _IpFilter {
    TC_GEN_FILTER filter;
    IP_PATTERN pattern;
    IP_PATTERN mask;
} IpFilter;

typedef struct {
    SOCKET sock;
    int type;
    bool filterConstructed;
    int dscp;
    IpFilter filter;
    SocketPerIfcInfo *first;
} SocketInfo;

#include <hash_map>

using namespace stdext;
using namespace std;

typedef hash_map<SOCKET, SocketInfo*> SocketInfoMap;

typedef struct _SocketInfoDB {
    SocketInfoMap smap;
} SocketInfoDB;

typedef struct _Ifc {
	struct _Ifc *next;
	LPWSTR name;
	HANDLE hIfc;
} Ifc;

typedef struct {
	Ifc *first;
    HANDLE hClt;
} IfcDB;

typedef struct {
    RvLock lock;
    HANDLE hClt;
    IfcDB ifcs;            /* Interface-related information */
    SocketInfoDB socks;    /* Socket-related information */
    DscpDB dscps;          /* DSCP-related information */
    RvLogMgr *logMgr;
} RvDscpM;

static RvDscpM sDscpM, *spDscpM = &sDscpM;

#define ModLock() RvLockGet(&spDscpM->lock, logMgr)
#define ModUnlock() RvLockRelease(&spDscpM->lock, logMgr)
#define ModLogMgr spDscpM->logMgr

#define SRC &ModLogMgr->socketSource, __FUNCTION__ ": "

#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define LogEnter(p) {if (ModLogMgr != NULL) RvLogEnter(&ModLogMgr->socketSource, p);}
#else
#define LogEnter(p) {RV_UNUSED_ARG(ModLogMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define LogLeave(p) {if (ModLogMgr != NULL) RvLogLeave(&ModLogMgr->socketSource, p);}
#else
#define LogLeave(p) {RV_UNUSED_ARG(ModLogMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define LogError(p) {if (ModLogMgr != NULL) RvLogError(&ModLogMgr->socketSource, p);}
#else
#define LogError(p) {RV_UNUSED_ARG(ModLogMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_EXCEP)
#define LogExcep(p) {if (ModLogMgr != NULL) RvLogExcep(&ModLogMgr->socketSource, p);}
#else
#define LogExcep(p) {RV_UNUSED_ARG(ModLogMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define LogDebug(p) {if (ModLogMgr != NULL) RvLogDebug(&ModLogMgr->socketSource, p);}
#else
#define LogDebug(p) {RV_UNUSED_ARG(ModLogMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
#define LogWarning(p) {if (ModLogMgr != NULL) RvLogWarning(&ModLogMgr->socketSource, p);}
#else
#define LogWarning(p) {RV_UNUSED_ARG(ModLogMgr);}
#endif


static
RvStatus OnIfcDeleted(HANDLE) {
	return RV_OK;
}

typedef void (*IfcVisitor)(HANDLE hIfc, void *ctx);

static
RvStatus IfcNew(Ifc **ppIfc, LPWSTR name, HANDLE hIfc) {
	RvStatus s;
	Ifc *pIfc;

	s = RvMemoryAlloc(0, sizeof(*pIfc), 0, (void **)&pIfc);
	if(s != RV_OK) {
		return s;
	}

	pIfc->hIfc = hIfc;
	pIfc->name = _wcsdup(name);
	if(pIfc->name == 0) {
		RvMemoryFree(pIfc, 0);
		return RV_ERROR_OUTOFRESOURCES;
	}

	*ppIfc = pIfc;
	return RV_OK;
}

static 
RvStatus IfcDBAdd(IfcDB *self, LPWSTR name, RvBool bNotify) {
	Ifc *cur;
	ULONG retval;
	HANDLE hIfc;
	Ifc *newIfc;
	RvStatus s = RV_OK;

	retval = TcOpenInterfaceW(name, self->hClt, 0, &hIfc);
	if(s != NO_ERROR) {
		return RvOsError(s);
	}

	for(cur = self->first; cur != 0; cur = cur->next) {
		if(wcscmp(name, cur->name) == 0) {
			/* Interface with the same name detected */
			if(bNotify) {
				OnIfcDeleted(cur->hIfc);
			}

			TcCloseInterface(cur->hIfc);
			cur->hIfc = hIfc;
			return RV_OK;
		}
	}

	/* Interface with the same name wasn't found */
	s = IfcNew(&newIfc, name, hIfc);
	if(s != RV_OK) {
		return s;
	}
	newIfc->next = self->first;
	self->first = newIfc;
	return RV_OK;
}

static 
RvStatus IfcDbConstruct(IfcDB *self, HANDLE hClt, RvLogMgr *logMgr) {
#define TCBUFSIZE 1024

	union {
		UCHAR buf[TCBUFSIZE];
		TC_IFC_DESCRIPTOR descriptors[1];
	} u;
	ULONG curSize = sizeof(u);
	TC_IFC_DESCRIPTOR *pDescriptors;
	TC_IFC_DESCRIPTOR *curDescr;
	RvUint8 *cur;
	RvUint8 *last;
	ULONG retval;
	RvStatus s;

	self->first = 0;
    self->hClt = hClt;
	
	pDescriptors = &u.descriptors[0];
	retval = TcEnumerateInterfaces(hClt, &curSize, pDescriptors);

	if(retval == ERROR_NOT_ENOUGH_MEMORY || retval == ERROR_INSUFFICIENT_BUFFER) {
		for(;;) {
			s = RvMemoryAlloc(0, curSize, logMgr, (void **)&pDescriptors);
			if(s != RV_OK) {
				return s;
			}

			retval = TcEnumerateInterfaces(self->hClt, &curSize, pDescriptors);
			if(retval != ERROR_NOT_ENOUGH_MEMORY) {
				break;
			}

			RvMemoryFree(pDescriptors, logMgr);
		}
	}

 	if(retval != NO_ERROR) {
		return RvOsError(retval);
	}

	cur = (RvUint8 *)pDescriptors;
	last = cur + curSize;

	while(cur < last) {
		curDescr = (TC_IFC_DESCRIPTOR *)cur;
		IfcDBAdd(self, curDescr->pInterfaceName, RV_FALSE);
		cur += curDescr->Length;
	}
	
	return RV_OK;
}

static 
void IfcDbDestruct(IfcDB *self, RvLogMgr *logMgr) {
    Ifc *cur, *next;

    for(cur = self->first; cur; cur = next) {
        next = cur->next;
        free(cur->name);
        TcCloseInterface(cur->hIfc);
        RvMemoryFree(cur, logMgr);
    }

    self->first = 0;

}

static TC_GEN_FLOW gsFlowTemplate = {
	{
        QOS_NOT_SPECIFIED,
		QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        SERVICETYPE_BESTEFFORT,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
	},

    {
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
        SERVICETYPE_BESTEFFORT,
        QOS_NOT_SPECIFIED,
        QOS_NOT_SPECIFIED,
     },

     0
};


static
RvStatus SocketFilterConstruct(SOCKET sock, int socktype, IpFilter *ipfilter, RvBool force) {
    SOCKADDR_IN localAddr;
    SOCKADDR_IN destAddr = {0};
    UCHAR protoid;
    TC_GEN_FILTER *filter;
    IP_PATTERN *pattern;
    IP_PATTERN *mask;
    short localPort;
    
    int namelen = sizeof(localAddr);
    int res;

    localAddr.sin_port = 0;
    res = getsockname(sock, (struct sockaddr *)&localAddr, &namelen);
    localPort = localAddr.sin_port;
    if(localPort == 0 && socktype == SOCK_DGRAM && force == RV_TRUE) {
        /* Try forced bind */
        LogDebug((SRC "UDP socket=%d seems to be unbound, trying to bind", sock));
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = 0;
        localAddr.sin_port = 0;
        res = bind(sock, (struct sockaddr *)&localAddr, sizeof(localAddr));
        if(res == SOCKET_ERROR) {
            res = WSAGetLastError();
            LogError((SRC "Forced bind on socket=%d failed (%d)", sock, res));
            return RvOsError(res);
        }

        LogDebug((SRC "Forced bind on socket=%d succeeded", sock));
        res = getsockname(sock, (struct sockaddr *)&localAddr, &namelen);
        if(res == SOCKET_ERROR) {
            res = WSAGetLastError();
            LogError((SRC "getsockname on sock=%d failed (%d)", sock, res));
            return RvOsError(res);
        }

        localPort = localAddr.sin_port;
        LogDebug((SRC "Forced bind on socket=%d yield addr=%s:%d", sock, inet_ntoa(localAddr.sin_addr), htons(localPort)));
    }

    if(localPort == 0)  {
        /* We need at least local port for filter */
        LogError((SRC "Filter Creation for socket=%d failed (probably unbound socket)", sock));
        return RV_ERROR_UNKNOWN;
    }

    if(socktype == SOCK_STREAM) {
        /* For TCP filters we need peer also */
        res = getpeername(sock, (struct sockaddr *)&destAddr, &namelen);
        if(res == SOCKET_ERROR) {
            res = WSAGetLastError();
            LogError((SRC "getpeername for TCP socket=%d failed (%d) (probably not connected)", sock, res));
            return RvOsError(res);
        }
        protoid = IPPROTO_TCP;
    } else {
        protoid = IPPROTO_UDP;
    }

    filter = &ipfilter->filter;
    pattern = &ipfilter->pattern;
    mask = &ipfilter->mask;
    filter->Mask = mask;
    filter->Pattern = pattern;

    filter->AddressType = NDIS_PROTOCOL_ID_TCP_IP;
    filter->PatternSize = sizeof(*pattern);
    memset(pattern, 0, sizeof(*pattern));
    pattern->ProtocolId = protoid;

    pattern->DstAddr = destAddr.sin_addr.s_addr;
    pattern->tcDstPort = destAddr.sin_port;
    

    pattern->SrcAddr = localAddr.sin_addr.s_addr;
    pattern->tcSrcPort = localPort;

    if(protoid == IPPROTO_TCP && (pattern->tcDstPort == 0)) {
        /* TCP socket isn't connected */
        LogError((SRC "Filter Creation for socket=%d failed (probably not connected TCP socket)", sock));
        return RV_ERROR_UNKNOWN;
    }

    memset(mask, -1, sizeof(*mask));

    if(pattern->SrcAddr == 0) {
        mask->SrcAddr = 0;
    }

    if(pattern->DstAddr == 0) {
        mask->DstAddr = 0;
    }

    if(pattern->tcDstPort == 0) {
        mask->tcDstPort = 0;
    }

    LogDebug((SRC "Filter for socket=%d created (srcAddr=%s:%d, destAddr=%s:%d)", sock,
        inet_ntoa(localAddr.sin_addr), htons(localPort), inet_ntoa(destAddr.sin_addr), htons(destAddr.sin_port)));

    return RV_OK;
}

static
RvStatus SocketGetType(SOCKET sock, int *type, int *af) {
    WSAPROTOCOL_INFO proto;
    int optlen = sizeof(proto);
    int res;

    res = getsockopt(sock, SOL_SOCKET, SO_PROTOCOL_INFO, (char *)&proto, &optlen);;
    if(res == SOCKET_ERROR) {
        res = WSAGetLastError();
        LogError((SRC "getting protocol info for socket=%d failed", sock));
        return RvOsError(res);
    }

    *type = proto.iSocketType;
    *af = proto.iAddressFamily;
    return RV_OK;
}


inline 
SocketInfo* SocketInfoDBFind(SocketInfoDB *self, SOCKET s) {
    SocketInfoMap::iterator iter;

    iter = self->smap.find(s);
    if(iter == self->smap.end()) {
        return 0;
    }

    return iter->second;
}

inline 
RvStatus SocketInfoDBAdd(SocketInfoDB *self, SOCKET s, SocketInfo *si) {
    pair<SocketInfoMap::iterator, bool> iterb = self->smap.insert(make_pair(s, si));
    return iterb.second ? RV_OK : RV_ERROR_UNKNOWN;
}

inline
SocketInfo* SocketInfoDBAny(SocketInfoDB *self) {
    SocketInfoMap::iterator iter;

    iter = self->smap.begin();
    if(iter == self->smap.end()) {
        return 0;
    }

    return iter->second;
}

inline
SocketInfo* SocketInfoDBRemove(SocketInfoDB *self, SOCKET s) {
    SocketInfoMap::iterator iter;
    SocketInfo *si;

    iter = self->smap.find(s);
    if(iter == self->smap.end()) {
        return 0;
    }

    si = iter->second;
    self->smap.erase(iter);
    return si;
}


static
RvStatus DscpInfoRelease(int dscp) {
    DscpInfo *di;
    DscpPerIfcInfo *cur, *next;

    di = &spDscpM->dscps.dscps[dscp];
    if(di->refCount <= 0) {
        /* Shouldn't happen */
        return RV_ERROR_UNKNOWN;
    }

    di->refCount--;
    if(di->refCount != 0) {
        return RV_OK;
    }

     for(cur = di->first; cur != 0; cur = next) {
        TcDeleteFlow(cur->hFlow);
        next = cur->next;
        RvMemoryFree(cur, ModLogMgr);
    }

   
    di->first = 0;
    return RV_OK;
}

static
RvStatus DscpInfoAcquire(int dscp) {
    DscpInfo *di;
    Ifc *cur;
    RvStatus s;
    DscpPerIfcInfo *pi;


    LogDebug((SRC "acquiring tos=%d", dscp));
    di = &spDscpM->dscps.dscps[dscp];
    di->refCount++;
   
    if(di->refCount > 1) {
        LogDebug((SRC "already acquired (refCount=%d)", di->refCount));
        return RV_OK;
    }

    di->first = 0;

    for(cur = spDscpM->ifcs.first; cur != 0; cur = cur->next) {
        s = RvMemoryAlloc(0, sizeof(*pi), ModLogMgr, (void **)&pi);
        if(s != RV_OK) {
            DscpPerIfcInfo *nextpi;

            for(pi = di->first; pi; pi = nextpi) {
                nextpi = pi->next;
                RvMemoryFree(pi, ModLogMgr);
            }

            di->first = 0;
            return s;
        }

        pi->hFlow = INVALID_HANDLE_VALUE;
        pi->hIfc = cur->hIfc;
        pi->next = di->first;
        di->first = pi;
    }


    for(pi = di->first; pi != 0; pi = pi->next) {
        union {
            TC_GEN_FLOW genFlow;
            struct {
                FLOWSPEC SendingFlowspec;  
                FLOWSPEC ReceivingFlowspec;  
                ULONG TcObjectsLength;
                QOS_DS_CLASS ds;
            } dsFlow;
        } u;
        ULONG res;

        QOS_DS_CLASS *pds = &u.dsFlow.ds;


        u.genFlow = gsFlowTemplate;
        pds->ObjectHdr.ObjectType = QOS_OBJECT_DS_CLASS;
        pds->ObjectHdr.ObjectLength = sizeof(*pds);
        pds->DSField = dscp;
        u.dsFlow.TcObjectsLength = sizeof(*pds);

        res = TcAddFlow(pi->hIfc, 0, 0, &u.genFlow, &pi->hFlow);
        if(res != NO_ERROR) {
            LogError((SRC "Add flow failed for dscp=%d", dscp))
        }
    }

    return RV_OK;
}

static
RvStatus DscpDBConstruct(DscpDB *self) {
    int i;
    DscpInfo *cur;

    for(i = 0; i < NDSCPS; i++) {
        cur = &self->dscps[i];
        cur->refCount = 0;
        cur->first = 0;
    }

    return RV_OK;
}

extern "C"
RvStatus RvWinDscpGetTos(SOCKET sock, int *tos, RvLogMgr *logMgr) {
    SocketInfo *si;
    RV_USE_CCORE_GLOBALS;

    if(!tosSupported) {
        return RV_ERROR_NOTSUPPORTED;
    }

    ModLock();
    spDscpM->logMgr = logMgr;
    si = SocketInfoDBFind(&spDscpM->socks, sock);
    *tos = si == 0 ? 0 : si->dscp;
    ModUnlock();
    return  RV_OK;
}


static
RvStatus DscpSetTos(SOCKET sock, int tos, RvBool forced) {
#define SI_ALLOCED 1
#define SI_CONSTRUCTED 2

    SocketInfo *si;
    RvStatus s = RV_OK;
    RvInt stage = 0;
    DscpInfo *di;
    DscpPerIfcInfo *pi;


    LogDebug((SRC "socket=%d, tos=%d, forced=%d", sock, tos, (int)forced));
    si = SocketInfoDBFind(&spDscpM->socks, sock);

    if(si == 0) {
        int af;
        int stype;

        LogDebug((SRC "socket=%d not found", sock));
        if(tos <= 0) {
            /* tos == 0 is default, nothing have to be done 
             * tos < 0 we're using to try and set TOS again in case of 
             * non-bound/not-connected socket. 
             */
            LogDebug((SRC "nothing to do for socket=%d", sock));
            return RV_OK;
        }

        s = SocketGetType(sock, &stype, &af);
        if(s != RV_OK) {
            goto failure;
        }

        if(af != AF_INET) {
            s = RV_ERROR_NOTSUPPORTED;
            LogError((SRC "setting tos isn't supported for this address family: %d", af));
            goto failure;
        }

        s = RvMemoryAlloc(0, sizeof(SocketInfo), ModLogMgr, (void **)&si); 
        if(s != RV_OK) {
            goto failure;
        }

        stage |= SI_ALLOCED;

        si->sock = sock;
        si->dscp = tos;
        si->first = 0;
        si->type = stype;
        si->filterConstructed = false;

        s = SocketInfoDBAdd(&spDscpM->socks, sock, si);
        if(s != RV_OK) {
            LogError((SRC "adding socket=%d to DB failed", sock));
            goto failure;
        }

        stage |= SI_CONSTRUCTED;
    }

    if(tos < 0) {
        tos = si->dscp;
    }

    
    if(si->filterConstructed) {
        SocketPerIfcInfo *cur, *next;

        /* Nothing to do, the dscp didn't change */
        if(si->dscp == tos) {
            LogDebug((SRC "tos for socket=%d didn't change (%d), nothing to do", sock, tos));

            return RV_OK;
        }

        LogDebug((SRC "tos changed on socket=%d, releasing previously established filters", sock));
        DscpInfoRelease(si->dscp);

        for(cur = si->first; cur; cur = next) {
            TcDeleteFilter(cur->hFilter);
            cur->hFilter = INVALID_HANDLE_VALUE;
            next = cur->next;
            RvMemoryFree(cur, ModLogMgr);
        }

        si->first = 0;
    } 

    /* If new tos == 0 */
    if(tos == 0) {
        LogDebug((SRC "new tos==0, forgetting socket=%d", sock));
        SocketInfoDBRemove(&spDscpM->socks, sock);
        RvMemoryFree(si, ModLogMgr);
        return RV_OK;
    }

    si->dscp = tos;
    s = SocketFilterConstruct(sock, si->type, &si->filter, forced);
    if(s != RV_OK) {
        return RV_OK;
    }

    LogDebug((SRC "establishing filters for socket=%d", sock));
    si->filterConstructed = true;
    di = &spDscpM->dscps.dscps[si->dscp];
    DscpInfoAcquire(tos);
    si->first = 0;

    for(pi = di->first; pi; pi = pi->next) {
        SocketPerIfcInfo *spi;
        ULONG res;
        
        s = RvMemoryAlloc(0, sizeof(*spi), ModLogMgr, (void **)&spi);
        spi->hFilter = INVALID_HANDLE_VALUE;
        spi->hIfc = pi->hIfc;
        spi->next = si->first;
        si->first = spi;
        res = TcAddFilter(pi->hFlow, &si->filter.filter, &spi->hFilter);
        if(res != NO_ERROR) {
            LogError((SRC "adding filter failed, sock=%d, tos=%d, ifc=%d", sock, tos, spi->hIfc));
        }
    }

failure:
    if(s != RV_OK) {
        if(stage & SI_ALLOCED) {
            RvMemoryFree(si, ModLogMgr);
        }
    }

    return s;
}

extern "C"
RvStatus RvWinDscpSetTos(SOCKET sock, int tos, RvLogMgr *logMgr) {
    RvStatus s;
    RV_USE_CCORE_GLOBALS;

    if(!tosSupported) {
        return RV_ERROR_NOTSUPPORTED;
    }

    if(tos < 0 || tos >= NDSCPS) {
        return RV_ERROR_BADPARAM;
    }

    ModLock();
    spDscpM->logMgr = logMgr;
    s = DscpSetTos(sock, tos, RV_FALSE);
    ModUnlock();

    return s;
}

extern "C"
RvStatus RvWinDscpCheckTos(SOCKET sock, RvLogMgr *logMgr) {
    RvStatus s;
    RV_USE_CCORE_GLOBALS;
    
    if(!tosSupported) {
        return RV_ERROR_NOTSUPPORTED;
    }

    ModLock();
    spDscpM->logMgr = logMgr;
    s = DscpSetTos(sock, -1, RV_TRUE);
    ModUnlock();
    return s;
}

extern "C"
RvStatus RvWinDscpRemoveSocket(SOCKET sock, RvLogMgr *logMgr) {
    return RvWinDscpSetTos(sock, 0, logMgr);
}

void CALLBACK ClNotifyHandler(HANDLE ClRegCtx,
                              HANDLE ClIfcCtx,
                              ULONG Event,
                              HANDLE SubCode,
                              ULONG BufSize,
                              PVOID Buffer) {

    (void)Buffer;
    (void)BufSize;
    (void)SubCode;
    (void)Event;
    (void)ClIfcCtx;
    (void)ClRegCtx;

}

TCI_CLIENT_FUNC_LIST sCallbacks = {
    ClNotifyHandler,
    0,
    0,
    0
};

static
RvStatus RvDscpMConstruct(RvDscpM *self) {
#define CLT_REGISTERED 1
#define LOCK_CONSTRUCTED 2
#define IFCDB_CONSTRUCTED 4

    RvStatus s = RV_OK;
    ULONG res;
    RvUint32 stage = 0;


    res = TcRegisterClient(CURRENT_TCI_VERSION, (HANDLE)self, &sCallbacks, &self->hClt);
    if(res != NO_ERROR) {
        return RvOsError(res);
    }

    stage |= CLT_REGISTERED;
    s = RvLockConstruct(0, &self->lock);
    if(s != RV_OK) goto failure;
    stage |= LOCK_CONSTRUCTED;

    s = IfcDbConstruct(&self->ifcs, self->hClt, 0);
    if(s != RV_OK) goto failure;
    stage |= IFCDB_CONSTRUCTED;

    DscpDBConstruct(&self->dscps);

    return RV_OK;
failure:


    if(s & LOCK_CONSTRUCTED) {
        RvLockDestruct(&self->lock, 0);
    }

    if(s & IFCDB_CONSTRUCTED) {
        IfcDbDestruct(&self->ifcs, 0);
    }

    if(s & CLT_REGISTERED) {
        TcDeregisterClient(self->hClt);
    }

    return s;
#undef CLT_REGISTERED 
#undef LOCK_CONSTRUCTED 
#undef IFCDB_CONSTRUCTED
}

static
RvStatus RvDscpMDestruct(RvDscpM *self) {
    SocketInfo *si;

    while((si = SocketInfoDBAny(&self->socks)) != 0) {
        /* Actually, removes all the information related to this socket */
        DscpSetTos(si->sock, 0, RV_FALSE);
    }

    IfcDbDestruct(&self->ifcs, 0);
    RvLockDestruct(&self->lock, 0);

    return RV_OK;
}


extern "C"
RvStatus RvWinDscpEnd() {
    RV_USE_CCORE_GLOBALS;
    
    if(tosSupported) {
        RvDscpMDestruct(spDscpM);
    }
    return RV_OK;
}

extern "C"
void CheckTosSupport(void) {
    RvStatus s;
    RV_USE_CCORE_GLOBALS;

    s = RvDscpMConstruct(spDscpM);
    tosSupported = (s == RV_OK);
}

/* Use GQoS API */
#elif RV_WIN_DSCP == RV_WIN_DSCP_GQOS

static FLOWSPEC gsFlowSpecTemplate = {
    QOS_NOT_SPECIFIED,
    QOS_NOT_SPECIFIED,
    QOS_NOT_SPECIFIED,
    QOS_NOT_SPECIFIED,
    QOS_NOT_SPECIFIED,
    SERVICETYPE_BESTEFFORT,
    QOS_NOT_SPECIFIED,
    QOS_NOT_SPECIFIED,
};

extern "C"
RvStatus RvWinDscpSetTos(SOCKET sock, int tos, RvLogMgr *) {
    QOS qos;
    QOS_DS_CLASS ds;
    int res;
    DWORD retBytes;

    qos.SendingFlowspec = gsFlowSpecTemplate;
    qos.ReceivingFlowspec = gsFlowSpecTemplate;
    ds.DSField = tos;
    ds.ObjectHdr.ObjectType = QOS_OBJECT_DS_CLASS;
    ds.ObjectHdr.ObjectLength = sizeof(ds);
    qos.ProviderSpecific.buf = (char *)&ds;
    qos.ProviderSpecific.len = sizeof(ds);

    res = WSAIoctl(sock, SIO_SET_QOS, &qos, sizeof(qos), 0, 0, &retBytes, 0, 0);
    if(res != 0) {
        res = WSAGetLastError();
        return RvOsError(res);
    }

    return RV_OK;
}

extern "C"
RvStatus RvWinDscpInit() {
    return RV_OK;
}

extern "C"
RvStatus RvWinDscpEnd() {
    return RV_OK;
}

extern "C"
RvStatus RvWinDscpRemoveSocket(SOCKET, RvLogMgr*) {
	return RV_OK;
}

extern "C"
RvStatus RvWinDscpGetTos(SOCKET, int *, RvLogMgr *) {
	return RV_ERROR_UNKNOWN;
}
 
#elif RV_WIN_DSCP == RV_WIN_DSCP_IPTOS

#define TCPIP_PARAMS_KEY "System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\"
#define DISABLE_TOS_VALUE "DisableUserTOSSetting"

extern "C"
void CheckTosSupport(void)
{
    RV_USE_CCORE_GLOBALS;
    RvUint winver = RvWindowsVersion();

    if(winver == RV_OS_WIN32_NT4)
    {
        tosSupported = RV_TRUE;
        return;
    }

    /* In Win2000 and XP tos setting on socket was replaced by DCSP
    *  setting using Tc (traffic control) library. To support backward
    *  compatibility with NT these OS have 'DisableUserTOSSetting' value
    *  under HKLM\System\CurrentControlSet\Services\Tcpip\Parameters\
    *  key in the system registry. If this value set to 0, Win2000 and XP
    *  behaves like NT.
    */
    if(winver == RV_OS_WIN32_2000 || winver == RV_OS_WIN32_XP || winver == RV_OS_WIN32_2003)
    {
        HKEY key;
        LONG res = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TCPIP_PARAMS_KEY,
            0,
            KEY_QUERY_VALUE,
            &key);
        DWORD val;
        DWORD retSize;

        if(res != ERROR_SUCCESS)
        {
            tosSupported = RV_FALSE;
            return;
        }

        retSize = sizeof(val);
        res = RegQueryValueEx(key, DISABLE_TOS_VALUE, 0, 0, (LPBYTE)&val, &retSize);
        tosSupported = (res == ERROR_SUCCESS && val == 0);
        RegCloseKey(key);
        return;
    }

}


#endif

#pragma warning(pop)
