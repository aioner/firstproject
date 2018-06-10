/* Copyright 1998 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#include "rvccore.h"

#if (RV_DNS_TYPE == RV_DNS_ARES)
#include "ares.h"
#include "ares_private.h"
#include "rvmemory.h"
#include "rvclock.h"
#include "rvrandomgenerator.h"
#include "rvstrutils.h"
#include "rvstdio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if (RV_OS_TYPE != RV_OS_TYPE_WINCE)
#include <time.h>
#include <errno.h>
#endif
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && (RV_OS_VERSION > RV_OS_VXWORKS_3_1)
#include <ipcom_sysvar.h>
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_OSE)
/* Currently, only OSE supports adapters for ARES module */
#include "rvadares.h"
#endif


#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)


/* Setting RV_DNS_W2K_MODE to 0 will simulate WinNT behavior on Win2K and later machines
 * for debugging purposes
 */
/*#define RV_DNS_W2K_MODE 0 */


#pragma message("WINVER = " Macro2String(WINVER))

#if !defined(RV_DNS_W2K_MODE)
#  if WINVER > 0x400
#    define RV_DNS_W2K_MODE 1
#  else
#    define RV_DNS_W2K_MODE 0
#  endif
#endif


#if RV_DNS_W2K_MODE  /* Platform SDK is installed */
#pragma warning (push,3)
#include <IPHlpApi.h>
#include <WinDns.h>
#pragma warning (pop)

typedef DWORD (WINAPI* GET_ADAPTERS_INFO)(PIP_ADAPTER_INFO,PULONG);
typedef DWORD (WINAPI* GET_PER_ADAPTERS_INFO)(ULONG,PIP_PER_ADAPTER_INFO,PULONG);
typedef DNS_STATUS (WINAPI* DNS_QUERY_CONFIG)(DNS_CONFIG_TYPE,DWORD,PWSTR,PVOID,PVOID,PDWORD);
typedef DWORD (WINAPI* GET_ADAPTERS_ADDRESSES)(ULONG,DWORD,PVOID,PIP_ADAPTER_ADDRESSES,PULONG);

static HMODULE iphlpapiDll, dnsapiDll;
static GET_ADAPTERS_INFO gaiFunc = NULL;
static GET_PER_ADAPTERS_INFO gpaiFunc = NULL;
static DNS_QUERY_CONFIG dqcFunc = NULL;
static GET_ADAPTERS_ADDRESSES gaaFunc = NULL;

#define init_win32 init_win2k

#else  /* !RV_DNS_W2K_MODE */ /* Platform SDK isn't installed */

#pragma message(RvWarning "For correct functioning,  it's recommended to compile this module with Platform SDK installed")

#define init_win32 init_winNT

#endif

#endif /* RV_OS_TYPE == RV_OS_TYPE_WIN32 */

#if defined(_WIN32_WCE) && (_WIN32_WCE >= 300)
#pragma warning (push,3)
#include <iphlpapi.h>
#pragma warning(pop)
#endif


#define PATH_RESOLV_CONF    "/etc/resolv.conf"
#ifndef INADDR_NONE
#define INADDR_NONE         0xffffffff
#endif


static int init_by_defaults(RvDnsEngine *channel);
#if (RV_OS_TYPE != RV_OS_TYPE_INTEGRITY) && (RV_OS_TYPE != RV_OS_TYPE_OSE) && \
    (RV_OS_TYPE != RV_OS_TYPE_NUCLEUS)   && (RV_OS_TYPE != RV_OS_TYPE_MOPI) && \
    (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN)
static int config_nameserver_by_string(RvDnsEngine *channel, const char *str);
#endif
#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX)     || (RV_OS_TYPE == RV_OS_TYPE_NETBSD) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD)	|| (RV_OS_TYPE == RV_OS_TYPE_MAC) || \
    (RV_OS_TYPE == RV_OS_TYPE_IPHONE)
static char *try_config(char *s, char *opt);
static int set_options(RvDnsEngine *channel, const char *str);
static const char *try_option(const char *p, const char *q, const char *opt);
#endif
static int init_server(RvDnsEngine *channel, RvAddress *addr);
static int set_search(RvDnsEngine *channel, const char *str, int ndomains);

#if 0
static int config_lookup(RvDnsEngine *channel, const char *str);
static int config_sortlist(struct apattern **sortlist, int *nsort, const char *str);
static int ip_addr(const char *s, int len, struct in_addr *addr);
static void natural_mask(struct apattern *pat);
#endif

static
RvStatus RvAresConfigureEhd(RvDnsEngine *dnsEngine);

int rv_ares_init(void)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) && RV_DNS_W2K_MODE
    iphlpapiDll = LoadLibrary("IPHlpApi.dll");
    dnsapiDll   = LoadLibrary("DnsApi.dll");

    if (iphlpapiDll != NULL && dnsapiDll != NULL)
    {
        gaiFunc  = (GET_ADAPTERS_INFO)GetProcAddress(iphlpapiDll,"GetAdaptersInfo");
        gpaiFunc = (GET_PER_ADAPTERS_INFO)GetProcAddress(iphlpapiDll,"GetPerAdapterInfo");
        dqcFunc  = (DNS_QUERY_CONFIG)GetProcAddress(dnsapiDll,"DnsQueryConfig");
        gaaFunc = (GET_ADAPTERS_ADDRESSES)GetProcAddress(iphlpapiDll,"GetAdaptersAddresses");
    } else {
        return RV_ERROR_UNKNOWN;
    }

#endif

    return RV_OK;
}

void rv_ares_end(void)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) && RV_DNS_W2K_MODE

    if (iphlpapiDll != NULL)
    {
        FreeLibrary(iphlpapiDll);
        iphlpapiDll = NULL;
    }
    if (dnsapiDll != NULL)
    {
        FreeLibrary(dnsapiDll);
        dnsapiDll = NULL;
    }

#endif
}

static
RvStatus initServer(rvServerState *server, int tcp_bufflen, RvLogMgr *logMgr) {
    RvStatus s;

    s = RvMemoryAlloc(NULL, tcp_bufflen, logMgr, (void**)&server->tcp_buffer);
    if(s != RV_OK) {
        return ARES_ESERVICE;
    }
    server->tcp_socket.fd.fd = RV_INVALID_SOCKET;
    server->udp_socket.fd.fd = RV_INVALID_SOCKET;
    server->tcp_bufflen = tcp_bufflen;
    server->tcp_buffer_pos = -1;

    server->pDeletedQuery = server->deletedQueryBuf;
    server->nDeletedQueryLen = 0;

    server->qhead = 0;
    server->qtail = 0;
    return RV_OK;
}

int rv_ares_construct(RvDnsEngine *channel, int max_servers, int max_domains,
                   int tcp_bufflen, struct rv_ares_options *options, int optmask)
{

    int status, i;
    RvRandomGenerator randGenObj;
    RvRandom randNum;

    /* Set everything to distinguished values so we know they haven't
     * been set yet.
     */
    channel->flags = -1;
    channel->timeout = RvInt64Const(-1,0,1);
    channel->tries = -1;
    channel->ndots = -1;
    channel->udp_port = (RvUint16)-1;
    channel->tcp_port = (RvUint16)-1;
    channel->max_servers = 0;
    channel->serversGeneration = 0;
    channel->max_domains = 0;
	channel->ehdOrder = RV_EHD_SEARCH_HOSTS_FIRST;
	channel->minPriority = 0;
	channel->heartbeatTimeout  = RvInt64Const(-1,0,1);
	channel->heartbeatServer   = -1;
	channel->heartbeatType     = RV_DNS_UNDEFINED;
	channel->strHeartbeatQuery = NULL;
	channel->hHeartbeatPage    = NULL;
	channel->heartbeatTimer.event = NULL;
	channel->heartbeatTimer.id    = 0;

    /* create a lock for protecting the channel (DNS engine) data */
    status = RvLockConstruct(channel->logMgr, &channel->lock);
    if (status != RV_OK)
        return ARES_ESERVICE;

    /* allocate array for "server state" structures */
    status = RvMemoryAlloc(NULL, max_servers * sizeof(rvServerState),
                           channel->logMgr, (void**)&channel->servers);
    if (status != RV_OK)
        return ARES_ESERVICE;
    channel->max_servers = max_servers;
    channel->skipServerWords = (max_servers / INTBITS) + (max_servers % INTBITS != 0);
    channel->nservers    = 0;
    channel->autoconfigMask = RV_DNS_AUTOCONFIG;
    channel->autoConfig = 0;
	channel->minPriority = (RvUint8)max_servers;

    /* Initialize per-server structure*/
    for (i=0; i < max_servers; ++i)
    {
        rvServerState *server = &channel->servers[i];
        status = initServer(server, tcp_bufflen, channel->logMgr);
        if(status != RV_OK) {
            return status;
        }
    }
    channel->tcp_bufflen = tcp_bufflen;

    /* allocate array for domain name pointers */
    channel->domains = NULL;
    status = RvMemoryAlloc(NULL, max_domains * sizeof(char*),
                           channel->logMgr, (void**)&channel->domains);
    if (status != RV_OK)
        return ARES_ESERVICE;

    channel->max_domains      = max_domains;
    channel->domainsGeneration = 0;
    channel->ndomains         = 0;
    channel->longest_domain_i = -1;
	channel->chpValue		  = 0;
	channel->chpServer		  = 0;

    /* allocate a buffer for each domain name */
    for (i=0; i < max_domains; ++i)
    {
        status = RvMemoryAlloc(NULL, RV_DNS_MAX_NAME_LEN+1,
                               channel->logMgr, (void**)&channel->domains[i]);
        if (status != RV_OK)
            return ARES_ESERVICE;
    }

    /* configure 'hosts' file support */
    RvAresConfigureEhd(channel);

    status = rv_ares_set_options(channel, options, optmask);
    if (status == ARES_SUCCESS)
        status = init_by_defaults(channel);
    if (status != ARES_SUCCESS)
    {
        rv_ares_destruct(channel, RV_FALSE);
        return status;
    }

    /* Trim to one server if ARES_FLAG_PRIMARY is set. */
    if ((channel->flags & ARES_FLAG_PRIMARY) && channel->nservers > 1)
        channel->nservers = 1;

    /* Choose a somewhat random query ID.  The main point is to avoid
     * collisions with stale queries.  An attacker trying to spoof a DNS
     * answer also has to guess the query ID, but it's only a 16-bit
     * field, so there's not much to be done about that.
     */
    {
		RvTime currentTime;
		RvClockGet(NULL, &currentTime);
        RvRandomGeneratorConstruct(&randGenObj, RvTimeGetNsecs(&currentTime));
    }

    RvRandomGeneratorGetInRange(&randGenObj, 0xffff, &randNum);
    channel->next_id = (unsigned short)randNum;

    channel->queries = 0;
    channel->lastQuery = 0;

    status = RvCondvarConstruct(&channel->inCallbackCond, channel->logMgr);
    if(status != RV_OK) {
        return status;
    }

    channel->inCallbackQid = 0;
    channel->inCallbackTid = 0;

    channel->nCompares = RvInt64ShortConst(0);
    channel->nSearches = RvInt64ShortConst(0);
    channel->nAvgCompares = 0;
    channel->nMaxCompares = 0;
    RvRandomGeneratorConstruct(&channel->rnd, 17300);

    return ARES_SUCCESS;
}

void rv_ares_destruct(RvDnsEngine *channel, RvBool waitForCallbacks)
{
    int i;

    for (i = 0; i < channel->nservers; i++)
        rv_ares__close_sockets(channel, i, waitForCallbacks);

    for (i = 0; i < channel->max_servers; i++)
        RvMemoryFree(channel->servers[i].tcp_buffer, channel->logMgr);

    if (channel->max_servers > 0)
        RvMemoryFree(channel->servers, channel->logMgr);

    for (i = 0; i < channel->max_domains; i++)
        RvMemoryFree(channel->domains[i], channel->logMgr);

    if (channel->domains)
        RvMemoryFree(channel->domains, channel->logMgr);

    RvCondvarDestruct(&channel->inCallbackCond, channel->logMgr);

    RvLockDestruct(&channel->lock, channel->logMgr);
}

int rv_ares_set_options(RvDnsEngine *channel, struct rv_ares_options *options, int optmask)
{
    int status, i;

    /* Easy stuff. */
    if (optmask & ARES_OPT_FLAGS)
        channel->flags = options->flags;
    if (optmask & ARES_OPT_TIMEOUT)
        channel->timeout = options->timeout;
    if (optmask & ARES_OPT_TRIES)
        channel->tries = options->tries;
    if (optmask & ARES_OPT_NDOTS)
        channel->ndots = options->ndots;
    if (optmask & ARES_OPT_UDP_PORT)
        channel->udp_port = options->udp_port;
    if (optmask & ARES_OPT_TCP_PORT)
        channel->tcp_port = options->tcp_port;

    /* Copy the servers, if given. */
    if ((optmask & ARES_OPT_SERVERS) && (options->nservers > 0))
    {
        for (i = 0; i < channel->nservers; i++) {
            rv_ares__close_sockets(channel, i, RV_FALSE);
        }

        channel->nservers = 0;

        for (i = 0; i < options->nservers; i++)
        {
            status = init_server(channel, &options->servers[i]);
            if (status != ARES_SUCCESS)
            {
                RvLogError(channel->dnsSource, (channel->dnsSource,
                           "rv_ares_set_options: init_server failed (status=%d)", status));
                return status;
            }
        }

        channel->serversGeneration++;
    }

    /* Copy the domains, if given.  Keep channel->ndomains consistent so
     * we can clean up in case of error.
     */
    if ((optmask & ARES_OPT_DOMAINS) && (options->ndomains > 0))
    {
        channel->ndomains = 0;
        channel->longest_domain_i = -1;
        channel->domainsGeneration++;

        for (i = 0; i < options->ndomains; i++)
        {
            status = set_search(channel, options->domains[i], 1);
            if (status != ARES_SUCCESS)
            {
                RvLogError(channel->dnsSource, (channel->dnsSource,
                           "rv_ares_set_options: set_search failed (status=%d)", status));
                return status;
            }
        }
    }

    return ARES_SUCCESS;
}

void rv_ares_get_options(RvDnsEngine *channel, RvInt64 *timeout, int *tries, RvAddress *servers,
                 int *nservers, char **domains, int *ndomains)
{
    int i;

    if (timeout != NULL)
        *timeout = channel->timeout;

    if (tries != NULL)
        *tries = channel->tries;

    if (servers != NULL && nservers != NULL)
    {
        int nRealServers = channel->nservers;
        int nServersOut;
        rvServerState *serverStates = channel->servers;

        nServersOut = RvMin(nRealServers, *nservers);
        for(i = 0; i < nServersOut; i++)
        {
            RvAddressCopy(&serverStates->addr, &servers[i]);
            serverStates++;
        }
        *nservers = nRealServers;
    }

    if (domains != NULL && ndomains != NULL)
    {
        for (i=0; i < RvMin(*ndomains,channel->ndomains); ++i)
        {
            domains[i] = channel->domains[i];
        }
        *ndomains = channel->ndomains;
    }
}


/* Converts error codes to meaningful string */
const char* rv_ares_get_error_string(int error)
{
    switch(error)
    {
        case ARES_ENODATA:      return "ARES_ENODATA";
        case ARES_EFORMERR:     return "ARES_EFORMERR";
        case ARES_ESERVFAIL:    return "ARES_ESERVFAIL";
        case ARES_ENOTFOUND:    return "ARES_ENOTFOUND";
        case ARES_ENOTIMP:      return "ARES_ENOTIMP";
        case ARES_EREFUSED:     return "ARES_EREFUSED";
        case ARES_EBADQUERY:    return "ARES_EBADQUERY";
        case ARES_EBADNAME:     return "ARES_EBADNAME";
        case ARES_EBADFAMILY:   return "ARES_EBADFAMILY";
        case ARES_EBADRESP:     return "ARES_EBADRESP";
        case ARES_ECONNREFUSED: return "ARES_ECONNREFUSED";
        case ARES_ETIMEOUT:     return "ARES_ETIMEOUT";
        case ARES_EOF:          return "ARES_EOF";
        case ARES_EFILE:        return "ARES_EFILE";
        case ARES_ENOMEM:       return "ARES_ENOMEM";
        case ARES_EDESTRUCTION: return "ARES_EDESTRUCTION";
        case ARES_ESERVICE:     return "ARES_ESERVICE";
#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX)     || (RV_OS_TYPE == RV_OS_TYPE_NETBSD) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD)	|| (RV_OS_TYPE == RV_OS_TYPE_MAC) || \
    (RV_OS_TYPE == RV_OS_TYPE_IPHONE)
        case ARES_ENOSERVERS:   return "DNS servers were neither configured nor found in " PATH_RESOLV_CONF;
#else
        case ARES_ENOSERVERS:   return "DNS servers were not found";
#endif
        case ARES_ENDOFSERVERS: return "all servers in the list were searched";
    }
    return "";
}

#if defined(_WIN32_WCE) && (_WIN32_WCE >= 300)

static int init_wce300(RvDnsEngine *channel, int config_type) {
    union {
        FIXED_INFO netParams;
        unsigned char buf[2000];
    } buf;

    FIXED_INFO *pNetParams;
    DWORD errCode = ERROR_SUCCESS;
    ULONG bufSize;
    IP_ADDR_STRING *pDnsAddr;

    pNetParams = &buf.netParams;
    bufSize = sizeof(buf);

    /* Auto configuration of DNS suffixes under WCE < 420 isn't supported
     * log warning and return
     */
    if(config_type & RV_DNS_SUFFIXES) {
        RvLogWarning(channel->dnsSource, (channel->dnsSource, "RvAresConfigure: suffix autoconfiguration isn't supported under WCE"));
    }

    if(!(config_type & RV_DNS_SERVERS)) {
        return ERROR_SUCCESS;
    }

    errCode = GetNetworkParams(pNetParams, &bufSize);
    /* Not enough space for network parameters structure, try to allocate on heap */
    if(errCode == ERROR_BUFFER_OVERFLOW) {
        pNetParams = (FIXED_INFO *)LocalAlloc(LMEM_FIXED, bufSize);
        /* Allocation from heap failed */
        if(pNetParams == 0) {
            return ERROR_BUFFER_OVERFLOW;
        }

        errCode = GetNetworkParams(pNetParams, &bufSize);
    }

    /* Unrecoverable error - return */
    if(errCode != ERROR_SUCCESS) {
        goto finish;
    }


    for(pDnsAddr = &pNetParams->DnsServerList; pDnsAddr != 0; pDnsAddr = pDnsAddr->Next) {
        char *saddr;

        saddr = pDnsAddr->IpAddress.String;
        if(saddr[0] != 0) {
            errCode = config_nameserver_by_string(channel, saddr);
            if(errCode) goto finish;
        }
    }

finish:
    /* Memory was allocated on heap - free it */
    if(pNetParams != &buf.netParams) {
        LocalFree((HLOCAL)pNetParams);
    }

    return errCode;
}

#endif /* _WIN32_WCE > 300 */

#if RV_OS_TYPE == RV_OS_TYPE_WIN32

#if RV_DNS_W2K_MODE

static int init_win2k(RvDnsEngine *channel, int config_type)
{
    DWORD status = ERROR_SUCCESS;
    RvBool keepRunning = RV_TRUE;
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    PIP_ADDR_STRING pAddrStr;
    ULONG bufSize;

    RvLogDebug(channel->dnsSource, (channel->dnsSource,
               "RvAresConstruct (init_win2k): Compiled with Platform SDK"));

    if (config_type & RV_DNS_SUFFIXES)
    {
        UCHAR buffer[_MAX_PATH];

        if (gaaFunc)
        {
            union
            {
                RvChar buf[4*4096];
                IP_ADAPTER_ADDRESSES addr;
            } puAddr;
            int len;

            bufSize = sizeof(puAddr);
            memset(puAddr.buf,0,bufSize);
            status = gaaFunc(AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                            NULL,&puAddr.addr,&bufSize);
            if (status == ERROR_SUCCESS)
            {
                IP_ADAPTER_ADDRESSES* adaptAddr;

                adaptAddr = &puAddr.addr;
                while (adaptAddr)
                {
                    len = (int)wcslen(adaptAddr->DnsSuffix);
                    if (len)
                    {
                        len = WideCharToMultiByte(CP_ACP, 0, adaptAddr->DnsSuffix, len,
                            (char*)buffer,sizeof(buffer),NULL,NULL);
                        if (len)
                        {
                            buffer[len] = 0;
                            set_search(channel,(const char*)buffer,1);
                        }
                    }
                    adaptAddr = adaptAddr->Next;
                }
            }
        }

        /* call DnsQueryConfig to get the primary DNS suffix */
        bufSize = sizeof(buffer);
        status = dqcFunc(DnsConfigPrimaryDomainName_A, FALSE,
                         NULL, NULL, buffer, &bufSize);

        if (status != ERROR_SUCCESS)
        {
            switch (status)
            {
            case ERROR_OUTOFMEMORY:
                RvLogInfo(channel->dnsSource, (channel->dnsSource,
                    "RvAresConstruct (init_win2k): DnsQueryConfig: no primary domain name found"));
                break;
            default:
                RvLogError(channel->dnsSource, (channel->dnsSource,
                    "RvAresConstruct (init_win2k): DnsQueryConfig: failure (status=%d)", status));
            }
        }
        else if ((status = set_search(channel, (char*)buffer, 1)) != ARES_SUCCESS)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "RvAresConstruct (init_win2k): set_search  failed (status %d)", status));
        }
    }

    if (!(config_type & RV_DNS_SERVERS))
        return ERROR_SUCCESS;

    /* call GetAdaptersInfo to retrieve adapters information for the local computer */
    status = gaiFunc(NULL, &bufSize);
    if (status != ERROR_BUFFER_OVERFLOW)
        return status;

    pAdapterInfo = (PIP_ADAPTER_INFO)malloc(bufSize);
    status = gaiFunc(pAdapterInfo, &bufSize);
    if (status != ERROR_SUCCESS)
    {
        RvLogError(channel->dnsSource, (channel->dnsSource,
                   "RvAresConstruct (init_win2k): gaiFunc (status=%d)", status));
        free(pAdapterInfo);
        return status;
    }


    /* for each adapter - get the per-adapter info to get the DNS server list */
    for (pAdapter = pAdapterInfo;
         pAdapter != NULL  &&  keepRunning == RV_TRUE;  pAdapter = pAdapter->Next)
    {
        /* call GetPerAdapterInfo to retrieve the DNS server list
           for the current interface.

           NOTE: GetPerAdapterInfo is called for getting the DNS server list while
           DnsQueryConfig is called for getting the Domain name although it provides
           the DNS server list as well. This is because DnsQueryConfig has sometimes
           a bug by means of listing DNS servers which are not really defined in the registry */
        status = gpaiFunc(pAdapter->Index, NULL, &bufSize);
        if (status != ERROR_BUFFER_OVERFLOW)
            break;

        pPerAdapterInfo = (PIP_PER_ADAPTER_INFO)malloc(bufSize);
        if (pPerAdapterInfo == NULL)
            break;

        status = gpaiFunc(pAdapter->Index, pPerAdapterInfo, &bufSize);
        if (status != ERROR_SUCCESS)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                       "RvAresConstruct (init_win2k): gpaiFunc (status=%d)", status));
            free(pPerAdapterInfo);
            break;
        }

        /* we have per-adapter info */

        for (pAddrStr = &pPerAdapterInfo->DnsServerList;
             pAddrStr != NULL  &&  keepRunning == RV_TRUE;  pAddrStr = pAddrStr->Next)
        {
            if (strlen(pAddrStr->IpAddress.String) <= 0)
                break;

            status = config_nameserver_by_string(channel, pAddrStr->IpAddress.String);
            if (status != ARES_SUCCESS)
            {
                RvLogError(channel->dnsSource, (channel->dnsSource,
                           "RvAresConstruct (init_win2k): config_nameserver_by_string (status=%d)", status));
                keepRunning = RV_FALSE;
            }
        }

        free(pPerAdapterInfo);
    }

    free(pAdapterInfo);

    return status;
}

#else

static int init_winNT(RvDnsEngine *channel, int configType)
{
    DWORD status = ERROR_SUCCESS;
    HKEY hKey;
    ULONG bufSize;
    UCHAR buffer[_MAX_PATH];
    char *p0, *p1;

    if(configType == 0) {
        /* Nothing to config */
        return ARES_SUCCESS;
    }

    RvLogDebug(channel->dnsSource, (channel->dnsSource,
               "RvAresConstruct (init_winNT): Compiled without Platform SDK"));

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                          0, KEY_QUERY_VALUE, &hKey);
    if (status != ERROR_SUCCESS)
	{
		/* if the host is Win98, than check in a different location in the registry list */
	    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Vxd\\MSTCP",
		                      0, KEY_QUERY_VALUE, &hKey);
		if (status != ERROR_SUCCESS)
			return ARES_SUCCESS;  /* no DNS servers were found;  initialize defaults */
	}

    if(configType & RV_DNS_SERVERS) {
        bufSize = sizeof(buffer);
        status = RegQueryValueEx(hKey, "NameServer", NULL, NULL, buffer, &bufSize);

        /* bufSize = 1 means 1 byte set to '\0' */
        if (status != ERROR_SUCCESS || bufSize <= 1)
        {
            bufSize = sizeof(buffer);
            status = RegQueryValueEx(hKey, "DhcpNameServer", NULL, NULL, buffer, &bufSize);
        }

        if (status == ERROR_SUCCESS && bufSize > 1)
        {
            for (p0 = p1 = (char*)buffer;  p1 != NULL;  p0 = p1)
            {
                /* Registry values can be separated in two ways:
                    by ',' - for manual configuration, and
                    by ' ' - for DHCP retrieved configuration */

                p1 = strchr(p0, ',');
                if (p1 == NULL)
                    p1 = strchr(p0, ' ');
                if (p1 != NULL)
                {
                    *p1 = 0;
                    p1++;
                }
                status = config_nameserver_by_string(channel, p0);
                if (status != ARES_SUCCESS)
                {
                    RegCloseKey(hKey);
                    return status;
                }
            }
        }
    }


    if(configType & RV_DNS_SUFFIXES) {

        bufSize = sizeof(buffer);
        status = RegQueryValueEx(hKey, "Domain", NULL, NULL, buffer, &bufSize);

        if (status != ERROR_SUCCESS || bufSize <= 1)
        {
            RegCloseKey(hKey);
            return ARES_SUCCESS;  /* no domain suffix was found;  initialize defaults */
        }

        status = set_search(channel, (char*)buffer, RV_INT32_MAX);
    }

    RegCloseKey(hKey);

    return status;
}
#endif
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_OSE)

/* callback to set a server address */
void rv_ares_configure_server_ose(RvUint32 addr, void *data)
{
	RvAddress rvaddr;
	RvDnsEngine *channel;

	channel = (RvDnsEngine *)data;

	/* Set DNS server in DnsEngine structure */
	RvAddressConstructIpv4(&rvaddr, addr, RV_ADDRESS_IPV4_ANYPORT);
	init_server(channel, &rvaddr);
	RvAddressDestruct(&rvaddr);
}

/* callback to set domain names from a search string */
void rv_ares_configure_search_ose(RvChar *str, RvInt maxDomains, void *data)
{
	RvDnsEngine *channel;

	channel = (RvDnsEngine *)data;

	/* Set DNS name in DnsEngine structure */
	set_search(channel, str, maxDomains);
}
#endif


#if RV_ANDROID
#define MAX_DNS_SERVERS 8

RvStatus rvOsGetDnsServers(RvAddress *addrs, RvSize_t *pNaddrs)
{
	return RV_ERROR_LIBCODE_CCORE;
}

int rvAdAresConfigure(RvDnsEngine *dnsEngine, int configType) {
  RvAddress addrs[MAX_DNS_SERVERS];
  RvSize_t naddrs = MAX_DNS_SERVERS;
  RvStatus s;
  int i;
  int si = ARES_SUCCESS;

  if(configType == RV_DNS_SUFFIXES) {
    return si;
  }

  s = rvOsGetDnsServers(addrs, &naddrs);
  if(s != RV_OK || naddrs == 0) {
    return si;
  }
  

  for(i = 0; i < naddrs; i++) {
    init_server(dnsEngine, &addrs[i]);
  }

  return si;
}
#endif
int rv_ares_configure(RvDnsEngine *channel, int config_type)
{
    int status = ARES_SUCCESS;
#if RV_ANDROID
    status = rvAdAresConfigure(channel, config_type);
    return status;

#elif (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX)     || (RV_OS_TYPE == RV_OS_TYPE_NETBSD) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD)	|| (RV_OS_TYPE == RV_OS_TYPE_MAC) || \
    (RV_OS_TYPE == RV_OS_TYPE_IPHONE)

    FILE *fp;
    char *line = NULL, *p;
    int linesize;
    RvBool configServers;
    RvBool configDomains;

    configServers = config_type & RV_DNS_SERVERS;
    configDomains = config_type & RV_DNS_SUFFIXES;

    RV_UNUSED_ARG(config_type);

    fp = fopen(PATH_RESOLV_CONF, "r");
    if (fp == NULL)
    {
        if (errno == ENOENT)  /* no such file */
            return ARES_SUCCESS;
        RvLogError(channel->dnsSource, (channel->dnsSource,
                   "rv_ares_configure: Unable to open %s file", PATH_RESOLV_CONF));
        return ARES_EFILE;
    }
    while ((status = rv_ares__read_line(fp, &line, &linesize)) == ARES_SUCCESS)
    {
        if (configServers && (p = try_config(line, "nameserver")) != NULL) {
            status = config_nameserver_by_string(channel, p);
        } else if (configDomains && (p = try_config(line, "domain")) != NULL) {
            status = set_search(channel, p, 1);
        } else if (configDomains && (p = try_config(line, "search")) != NULL) {
            status = set_search(channel, p, RV_INT32_MAX);
        } else if ((p = try_config(line, "options")) != NULL) {
            status = set_options(channel, p);
        } else {
            status = ARES_SUCCESS;
        }

        if (status != ARES_SUCCESS)
            break;
    }

    if (status == ARES_EOF)
        status = ARES_SUCCESS;

    free(line);
    fclose(fp);

#elif (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    status = init_win32(channel, config_type);
#elif defined(_WIN32_WCE) && _WIN32_WCE >= 300
    status = init_wce300(channel, config_type);

#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#if (RV_OS_VERSION <= RV_OS_VXWORKS_3_1)
    int i;
    RESOLV_PARAMS_S resolvParams;

    resolvParamsGet(&resolvParams);

    if(config_type & RV_DNS_SERVERS) {
        for (i=0; i < MAXNS; ++i)
        {
            if (strlen(resolvParams.nameServersAddr[i]) > 0)
            {
                status = config_nameserver_by_string(channel, resolvParams.nameServersAddr[i]);
                if (status != ARES_SUCCESS)
                    break;
            }
        }
    }

    if(config_type & RV_DNS_SUFFIXES) {
        set_search(channel, resolvParams.domainName, RV_INT32_MAX);
    }
#else
	RvChar buff[256];
	RvSize_t len;

    if(config_type & RV_DNS_SERVERS) {
		memset(buff,0,sizeof(buff));
		ipcom_sysvar_get("ipdnsc.primaryns",buff,&len);
		if (buff != NULL)
			config_nameserver_by_string(channel, buff);

		memset(buff,0,sizeof(buff));
		ipcom_sysvar_get("ipdnsc.secondaryns",buff,&len);
		if (buff != NULL)
			config_nameserver_by_string(channel, buff);

		memset(buff,0,sizeof(buff));
		ipcom_sysvar_get("ipdnsc.tertiaryns",buff,&len);
		if (buff != NULL)
			config_nameserver_by_string(channel, buff);

		memset(buff,0,sizeof(buff));
		ipcom_sysvar_get("ipdnsc.quaternaryns ",buff,&len);
		if (buff != NULL)
			config_nameserver_by_string(channel, buff);
	}
    if(config_type & RV_DNS_SUFFIXES) {
		memset(buff,0,sizeof(buff));
		ipcom_sysvar_get("ipdnsc.domainname",buff,&len);
		if (buff != NULL)
			set_search(channel, buff, RV_INT32_MAX);
	}
#endif /* (RV_OS_VERSION <= RV_OS_VXWORKS_3_1) */

#elif (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)

    RvUint32    dns_list[10];   /* list of 10 possible DNS servers */
    RvUint32    dnsServerIp, i = 0;
    int         dnsNum;
    RvAddress   addr;

    memset(dns_list, 0, sizeof(dns_list));

    dnsNum = NU_Get_DNS_Servers((RvUint8*)dns_list, sizeof(dns_list));
    if (dnsNum <= 0)
        return ARES_ENODATA;

    if(config_type & RV_DNS_SERVERS) {
        for (dnsServerIp = dns_list[0]; ((dnsServerIp != 0) && ((int)i < dnsNum)); i++, dnsServerIp = dns_list[i])
        {
            RvAddressConstructIpv4(&addr, dnsServerIp, RV_ADDRESS_IPV4_ANYPORT);
            status = init_server(channel, &addr);
            if (status != ARES_SUCCESS)
                break;
        }
    }
#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)
	/* Need to call configure in another file to prevent header conflicts. */
	if(RvAdAresConfigure((config_type & RV_DNS_SERVERS), (config_type & RV_DNS_SUFFIXES), rv_ares_configure_server_ose, rv_ares_configure_search_ose, (void *)channel) != RV_OK)
		status = ARES_ENOTFOUND; /* Only error is not finding the DNS service */
#else
    RV_UNUSED_ARG(channel);
    RV_UNUSED_ARG(config_type);
#endif

    if (status != ARES_SUCCESS)
	{
        RvLogError(channel->dnsSource, (channel->dnsSource,
                   "rv_ares_configure: Configuration failed"));
		return status;
	}

    return status;
}

static int init_by_defaults(RvDnsEngine *channel)
{
    if (channel->flags == -1)
        channel->flags = 0;
    if (RvInt64IsEqual(channel->timeout, RvInt64Const(-1,0,1)))
        channel->timeout = DEFAULT_TIMEOUT;
    if (channel->tries == -1)
        channel->tries = DEFAULT_TRIES;
    if (channel->ndots == -1)
        channel->ndots = 1;
    if (channel->udp_port == (RvUint16)-1)
        channel->udp_port = NAMESERVER_PORT;
    if (channel->tcp_port == (RvUint16)-1)
        channel->tcp_port = NAMESERVER_PORT;
    if (RvInt64IsEqual(channel->heartbeatTimeout, RvInt64Const(-1,0,1)))
        channel->heartbeatTimeout = DEFAULT_HEARTBEAT_TIMEOUT;
	if(channel->heartbeatServer == -1)
		channel->heartbeatServer = 0;
	if(channel->heartbeatType == -1)
		channel->heartbeatType = RV_DNS_HOST_IPV4_TYPE;
    return ARES_SUCCESS;
}

#if (RV_OS_TYPE != RV_OS_TYPE_INTEGRITY) && (RV_OS_TYPE != RV_OS_TYPE_OSE) && \
    (RV_OS_TYPE != RV_OS_TYPE_NUCLEUS)   && (RV_OS_TYPE != RV_OS_TYPE_MOPI)  && \
    (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN)
static int config_nameserver_by_string(RvDnsEngine *channel, const char *str)
{
    RvAddress addr;

#if (RV_NET_TYPE & RV_NET_IPV6)
    RvAddressConstruct(RV_ADDRESS_TYPE_IPV6, &addr);
    if (RvAddressSetString(str, &addr) == NULL)
#endif
    {
        RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &addr);
        if (RvAddressSetString(str, &addr) == NULL)
            return ARES_SUCCESS;  /* ignore invalid addresses */
    }

    return init_server(channel, &addr);
}
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX)     || (RV_OS_TYPE == RV_OS_TYPE_FREEBSD) || \
    (RV_OS_TYPE == RV_OS_TYPE_MAC)      || (RV_OS_TYPE == RV_OS_TYPE_NETBSD) || \
    (RV_OS_TYPE == RV_OS_TYPE_IPHONE)

static char *try_config(char *s, char *opt)
{
    int len;

    len = strlen(opt);
    if (strncmp(s, opt, len) != 0 || !isspace((unsigned char)s[len]))
        return NULL;
    s += len;
    while (isspace((unsigned char)*s))
        s++;
    return s;
}

static int set_options(RvDnsEngine *channel, const char *str)
{
    const char *p, *q, *val;

    p = str;
    while (*p)
    {
        q = p;
        while (*q && !isspace((unsigned char)*q))
            q++;
        val = try_option(p, q, "ndots:");
        if (val && channel->ndots == -1)
            channel->ndots = atoi(val);
        val = try_option(p, q, "retrans:");
        if (val && RvInt64IsEqual(channel->timeout, RvInt64Const(-1,0,1)))
            channel->timeout = RvInt64Mul(RvInt64FromRvInt(atoi(val)), RV_TIME64_NSECPERSEC);
        val = try_option(p, q, "retry:");
        if (val && channel->tries == -1)
            channel->tries = atoi(val);
        p = q;
        while (isspace((unsigned char)*p))
            p++;
    }

    return ARES_SUCCESS;
}

static const char *try_option(const char *p, const char *q, const char *opt)
{
    int len;

    len = strlen(opt);
    return (q - p > len && strncmp(p, opt, len) == 0) ? p + len : NULL;
}
#endif

static int init_server(RvDnsEngine *channel, RvAddress *addr)
{
    int i;
    rvServerState *server;

    for(i = 0, server = &channel->servers[0]; i < channel->nservers; ++i, server++)  {
        if (RvAddressCompare(&server->addr, addr, RV_ADDRESS_BASEADDRESS) == RV_TRUE)
            return ARES_SUCCESS;
    }

    if (i >= channel->max_servers)
        return ARES_ENOMEM;

    server->addr = *addr;
    server->udp_socket.fd.fd = (RvSocket)-1;
    server->tcp_socket.fd.fd = (RvSocket)-1;
	server->priority		 = ARES_MAX_PRIORITY;
    channel->nservers++;
    /* Convert server address to string, mostly for logging */
    RvAddressGetString(addr, sizeof(server->sAddr), server->sAddr);
    return ARES_SUCCESS;
}

static int set_search(RvDnsEngine *channel, const char *str, int ndomains)
{
    int cnt,n, status = ARES_SUCCESS;
    RvBool found;
    const char *p, *q;
    char *s;
    RvSize_t len;

    for (n = 0, p = q = str; *p && n < ndomains; ++n)
    {
        while (*q && !isspace((unsigned char)*q))
            q++;
        /* q points now to the end of the current string */

        if (channel->ndomains < channel->max_domains)
        {
            len = q - p;
            if (len > RV_DNS_MAX_NAME_LEN)
            /* this is name is too long */
                goto findNext;

            s = channel->domains[channel->ndomains];
            memcpy(s, p, len);
            s[len] = 0;

            /* test that this suffix was not set before */
            found = RV_FALSE;
            for (cnt = 0; cnt < channel->ndomains; cnt++)
            {
                if (RvStrcasecmp(s,channel->domains[cnt]) == 0)
                {
                    found = RV_TRUE;
                    break;
                }
            }

            if (found)
                goto findNext;

            /* set the index of the longest domain name */
            if ((channel->longest_domain_i == -1) ||
                (strlen(s) > strlen(channel->domains[channel->longest_domain_i])))
                channel->longest_domain_i = channel->ndomains;
            RvLogDebug(channel->dnsSource, (channel->dnsSource, "set_search: Domain %s added to search", s));
            channel->ndomains++;
        }
        else
            status = ARES_ENOMEM;

findNext:
        while (isspace((unsigned char)*q))
            q++;

        /* q points now to the beginning of the next string */
        p = q;
    }

    return status;
}


#if 0
static int config_lookup(RvDnsEngine *channel, const char *str)
{
    char lookups[3], *l;
    const char *p;

    /* Set the lookup order.  Only the first letter of each work
     * is relevant, and it has to be "b" for DNS or "f" for the
     * host file.  Ignore everything else.
     */
    l = lookups;
    p = str;
    while (*p)
    {
        if ((*p == 'b' || *p == 'f') && l < lookups + 2)
            *l++ = *p;
        while (*p && !isspace((unsigned char)*p))
            p++;
        while (isspace((unsigned char)*p))
            p++;
    }
    *l = 0;
    channel->lookups = strdup(lookups);
    return (channel->lookups) ? ARES_SUCCESS : ARES_ENOMEM;
}

static int config_sortlist(struct apattern **sortlist, int *nsort, const char *str)
{
    struct apattern pat, *newsort;
    const char *q;

    /* Add sortlist entries. */
    while (*str && *str != ';')
    {
        q = str;
        while (*q && *q != '/' && *q != ';' && !isspace((unsigned char)*q))
            q++;
        if (ip_addr(str, q - str, &pat.addr) == 0)
        {
            /* We have a pattern address; now determine the mask. */
            if (*q == '/')
            {
                str = q + 1;
                while (*q && *q != ';' && !isspace((unsigned char)*q))
                    q++;
                if (ip_addr(str, q - str, &pat.mask) != 0)
                    natural_mask(&pat);
            }
            else
                natural_mask(&pat);

            /* Add this pattern to our list. */
            newsort = realloc(*sortlist, (*nsort + 1) * sizeof(struct apattern));
            if (!newsort)
                return ARES_ENOMEM;
            newsort[*nsort] = pat;
            *sortlist = newsort;
            (*nsort)++;
        }
        else
        {
            while (*q && *q != ';' && !isspace((unsigned char)*q))
                q++;
        }
        str = q;
        while (isspace((unsigned char)*str))
            str++;
    }

    return ARES_SUCCESS;
}

static int ip_addr(const char *s, int len, struct in_addr *addr)
{
    char ipbuf[16];

    /* Four octets and three periods yields at most 15 characters. */
    if (len > 15)
        return -1;
    memcpy(ipbuf, s, len);
    ipbuf[len] = 0;

    addr->s_addr = inet_addr(ipbuf);
    if (addr->s_addr == INADDR_NONE && strcmp(ipbuf, "255.255.255.255") != 0)
        return -1;
    return 0;
}

static void natural_mask(struct apattern *pat)
{
    struct in_addr addr;

    /* Store a host-byte-order copy of pat in a struct in_addr.  Icky,
     * but portable.
     */
    addr.s_addr = RvConvertNetowrkToHost32(pat->addr.s_addr);

    /* This is out of date in the CIDR world, but some people might
     * still rely on it.
     */
    if (IN_CLASSA(addr.s_addr))
        pat->mask.s_addr = RvConvertHostToNetwork32(IN_CLASSA_NET);
    else if (IN_CLASSB(addr.s_addr))
        pat->mask.s_addr = RvConvertHostToNetwork32(IN_CLASSB_NET);
    else
        pat->mask.s_addr = RvConvertHostToNetwork32(IN_CLASSC_NET);
}


#endif


static
RvStatus RvAresConfigureEhd(RvDnsEngine *dnsEngine) {
    dnsEngine->ehdOrder = RV_EHD_SEARCH_HOSTS_FIRST;
    return RV_OK;
}


#endif /* RV_DNS_ARES */
