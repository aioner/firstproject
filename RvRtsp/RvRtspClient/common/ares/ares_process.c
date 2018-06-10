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
#include "ares_dns.h"
#include "rvoscomp.h"
#include "rvstrutils.h"
#include "rvassert.h"

#include <stdlib.h>
#include <string.h>
#if !defined(NEED_MANUAL_WRITEV)
#include <errno.h>
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE)
/* UnixWare hides strcasecmp() in strings.h */
#include <strings.h>
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX)     || (RV_OS_TYPE == RV_OS_TYPE_FREEBSD) || \
	(RV_OS_TYPE == RV_OS_TYPE_MAC)      || (RV_OS_TYPE == RV_OS_TYPE_NETBSD)
#include <sys/uio.h>
#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#include <ioLib.h>
#endif

static void selectCb(RvSelectEngine *selectEngine, RvSelectFd *fd,
                     RvSelectEvents selectEvent, RvBool error);
static RvBool timerCb(select_fd_args *fd_args);
static void write_tcp_data(RvDnsEngine *channel, select_fd_args *fd_args, RvInt64 now);
static void read_tcp_data(RvDnsEngine *channel, select_fd_args *fd_args, RvInt64 now);
static void read_udp_packets(RvDnsEngine *channel, select_fd_args *fd_args, RvInt64 now);
static void process_timeouts(RvDnsEngine *channel, RvInt64 now, RvBool error);
static void process_answer(RvDnsEngine *channel, unsigned char *abuf,
                           int alen, int whichserver, int tcp, RvInt64 now);
static rvQuery *find_query(RvDnsEngine *channel, unsigned char *abuf);
static void handle_error(RvDnsEngine *channel, int whichserver, RvInt64 now);
static void next_server(RvDnsEngine *channel, rvQuery *query, RvInt64 now);
static int open_socket(RvDnsEngine *channel, int whichserver, RvSocketProtocol protocol);
static int same_questions(const unsigned char *qbuf, int qlen,
                          const unsigned char *abuf, int alen);
static void end_query(RvDnsEngine *channel, rvQuery *query, int status,
                      unsigned char *abuf, int alen);

#define LOG_SRC (channel->dnsSource)
#define LOG_FUNC LOG_SRC, FUNC

#define DnsLogDebug(p) RvLogDebug(LOG_SRC, p)
#define DnsLogExcep(p) RvLogExcep(LOG_SRC, p)
#define DnsLogError(p) RvLogError(LOG_SRC, p)

#if RV_DNS_SANITY_CHECK

/* Assumes lock hold on 'channel' object */
RvBool rvDnsEngineSanityCheck(RvDnsEngine *channel) {
#define FUNC "RvDnsEngineSanityCheck"
    rvQuery *cur;

    for(cur = channel->queries; cur; cur = cur->qnext) {
        if(cur->mark) {
            DnsLogExcep((LOG_FUNC "List of queries includes cycle at query %d", cur->user_qid));
            RvAssert(0);
            /*lint -e{527} */ /*suppressing 'unreachable code' error*/
            return RV_FALSE;
        }

        cur->mark = RV_TRUE;
    }

    for(cur = channel->queries; cur; cur = cur->qnext) {
        cur->mark = RV_FALSE;
    }

    return RV_TRUE;
#undef FUNC
}

#endif


int ares__send_query(RvDnsEngine *channel, rvQuery *query, RvInt64 now)
{
    RvStatus status;
    rvServerState *server;
    RvSize_t count = 0;
    RvInt64 timeout;

    if (channel->nservers <= 0)
        return ARES_ENOSERVERS;

    server = &channel->servers[query->server];
    if (query->using_tcp)
    {
        /* Make sure the TCP socket for this server is set up and queue
         * a send request.
         */
        if (server->tcp_socket.fd.fd == RV_INVALID_SOCKET)
        {
            if (open_socket(channel, query->server, RvSocketProtocolTcp) != ARES_SUCCESS)
            {
                query->skip_server[query->server] = 1;
                next_server(channel, query, now);
                return ARES_SUCCESS;
            }
        }

        query->tcp_data  = query->qbuf - 2;
        query->tcp_len   = query->qlen + 2;
        query->tcp_next  = NULL;
        if (server->qtail)
            server->qtail->tcp_next = query;
        else
        {
            server->qhead = query;
            status = RvSelectUpdate(channel->selectEngine, &server->tcp_socket.fd,
                                    RV_SELECT_READ | RV_SELECT_WRITE | RV_SELECT_CONNECT, selectCb);
            if (status != RV_OK)
            {
                end_query(channel, query, ARES_ESERVICE, NULL, 0);
                return ARES_ESERVICE;
            }
        }
        server->qtail = query;
        query->timeout = RvInt64Const(0,0,0);
    }
    else  /* Send using UDP */
    {
        if (server->udp_socket.fd.fd == RV_INVALID_SOCKET)
        {
            if (open_socket(channel, query->server, RvSocketProtocolUdp) != ARES_SUCCESS)
            {
                query->skip_server[query->server] = 1;
                next_server(channel, query, now);
                return ARES_SUCCESS;
            }
        }

        /* calculate timeout and start a timer BEFORE sending the query
         * only to make sure that the we have available timer
         */
#if 0
        timeout = ((query->tryIndex == 0) ? channel->timeout :
                                       channel->timeout << query->tryIndex / channel->nservers);
#else
        timeout = channel->timeout;
#endif
        status = RvTimerStart(&query->timer, channel->timerQueue, RV_TIMER_TYPE_ONESHOT,
                              timeout, (RvTimerFunc)timerCb, &server->udp_socket);
        if (status != RV_OK)
        {
            end_query(channel, query, ARES_ESERVICE, NULL, 0);
            return ARES_ESERVICE;
        }

        query->timeout = RvInt64Add(now, timeout);

        status = RvSocketSendBuffer(&server->udp_socket.fd.fd, (RvUint8*)query->qbuf,
                                    query->qlen, &server->addr, channel->logMgr,
                                    (RvSize_t*)&count);
        if (status != RV_OK)
        {
            RvTimerCancel(&query->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
            query->timeout = RvInt64Const(0,0,0);

            query->skip_server[query->server] = 1;
            next_server(channel, query, now);
            return ARES_SUCCESS;
        }
    }

    return ARES_SUCCESS;
}

void ares__close_sockets(RvDnsEngine *channel, int i)
{
    rvServerState *server = &channel->servers[i];

    /* Close the TCP and UDP sockets. */
    if (server->tcp_socket.fd.fd != RV_INVALID_SOCKET)
    {
        RvSelectRemove(channel->selectEngine, &server->tcp_socket.fd);
        RvSocketDestruct(&server->tcp_socket.fd.fd, RV_FALSE, NULL, NULL);
        server->tcp_socket.fd.fd = RV_INVALID_SOCKET;
    }

    if (server->udp_socket.fd.fd != RV_INVALID_SOCKET)
    {
        RvSelectRemove(channel->selectEngine, &server->udp_socket.fd);
        RvSocketDestruct(&server->udp_socket.fd.fd, RV_FALSE, NULL, NULL);
        server->udp_socket.fd.fd = RV_INVALID_SOCKET;
    }
}

/* Something interesting happened on the wire, or there was a timeout.
 * See what's up and respond accordingly.
 */
static void selectCb(RvSelectEngine *selectEngine, RvSelectFd *fd,
                     RvSelectEvents selectEvent, RvBool error)
{
    RvStatus status;
    select_fd_args *fd_args = (select_fd_args*)fd;
    RvDnsEngine *channel = fd_args->channel;
    RvInt64 now = RvTimestampGet(channel->logMgr);

    RV_UNUSED_ARG(selectEngine);
    RV_UNUSED_ARG(selectEvent);

    RvLogEnter(channel->dnsSource,
        (channel->dnsSource, "selectCb(engine=%p)", channel));

    status = RvLockGet(&channel->lock, channel->logMgr);
    if (status == RV_OK)
    {
        if (!error)  /* Note: error is not really boolean but integer.
                        error != 0 is normally TCP connection problem */
        {
            if (fd_args->protocol == RvSocketProtocolTcp)
            {
                write_tcp_data(channel, fd_args, now);
                read_tcp_data(channel, fd_args, now);
            }
            if (fd_args->protocol == RvSocketProtocolUdp)
            {
                read_udp_packets(channel, fd_args, now);
            }
        }
        process_timeouts(channel, now, error);

        RvLockRelease(&channel->lock, channel->logMgr);
    }

    RvLogLeave(channel->dnsSource,
        (channel->dnsSource, "selectCb(engine=%p)", channel));
}

static RvBool timerCb(select_fd_args *fd_args)
{
    RvStatus status;
    RvDnsEngine *channel = fd_args->channel;
    RvInt64 now = RvTimestampGet(channel->logMgr);

    RvLogEnter(channel->dnsSource,
        (channel->dnsSource, "timerCb(engine=%p)", channel));

    status = RvLockGet(&channel->lock, channel->logMgr);
    if (status == RV_OK)
    {
        process_timeouts(fd_args->channel, now, RV_FALSE);

        RvLockRelease(&channel->lock, channel->logMgr);
    }

    RvLogLeave(channel->dnsSource,
        (channel->dnsSource, "timerCb(engine=%p)", channel));

    return RV_FALSE;  /* don't reschedule */
}

/* If any TCP sockets select true for writing, write out queued data
 * we have for them.
 */
static void write_tcp_data(RvDnsEngine *channel, select_fd_args *fd_args, RvInt64 now)
{
    RvStatus status;
    rvServerState *server;
    rvQuery *query;
    int count;
#if !defined(NEED_MANUAL_WRITEV)
    IOVEC vec[DEFAULT_IOVEC_LEN];
    int n;
#endif

    /* Make sure server has data to send and is selected in write_fds. */
    server = &channel->servers[fd_args->server];
    if (!server->qhead || &server->tcp_socket != fd_args)
        return;

#if !defined(NEED_MANUAL_WRITEV)
    /* Count the number of send queue items. */
    n = 0;
    for (query = server->qhead; query; query = query->tcp_next)
        n++;

    if (n <= DEFAULT_IOVEC_LEN)
    {
        /* Fill in the iovecs and send. */
        n = 0;
        for (query = server->qhead; query; query = query->tcp_next)
        {
            vec[n].iov_base = query->tcp_data;
            vec[n].iov_len  = query->tcp_len;
            n++;
        }
        count = writev((int)(server->tcp_socket.fd.fd), vec, n);
        if (count < 0)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "write_tcp_data: Error in writev(errno=%d)", errno));
            handle_error(channel, fd_args->server, now);
            return;
        }

        /* Advance the send queue by as many bytes as we sent. */
        while (count)
        {
            query = server->qhead;
            if (count >= query->tcp_len)
            {
                count -= query->tcp_len;
                server->qhead = query->tcp_next;
                if (server->qhead == NULL)
                    server->qtail = NULL;
            }
            else
            {
                query->tcp_data += count;
                query->tcp_len  -= count;
                break;
            }
        }
    }
    else
#endif
    {
        /* Can't use writev; just send the first request. */
        query = server->qhead;
        status = RvSocketSendBuffer(&server->tcp_socket.fd.fd, (RvUint8*)query->tcp_data,
                                    query->tcp_len, NULL, channel->logMgr,
                                    (RvSize_t*)&count);
        if (status != RV_OK)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "write_tcp_data: Error in RvSocketSendBuffer(status=%d)", status));
            handle_error(channel, fd_args->server, now);
            return;
        }

        /* Advance the send queue by as many bytes as we sent. */
        if (count == query->tcp_len)
        {
            server->qhead = query->tcp_next;
            if (server->qhead == NULL)
                server->qtail = NULL;
        }
        else
        {
            query->tcp_data += count;
            query->tcp_len  -= count;
        }
    }
    
    if (server->qhead == NULL)
    {
        status = RvSelectUpdate(channel->selectEngine, &server->tcp_socket.fd,
                                RV_SELECT_READ, selectCb);
        if (status != RV_OK)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "write_tcp_data: Error in RvSelectUpdate(status=%d)", status));
            handle_error(channel, fd_args->server, now);
            return;
        }
    }
}

/* If any TCP socket selects true for reading, read some data,
 * allocate a buffer if we finish reading the length word, and process
 * a packet if we finish reading one.
 */
static void read_tcp_data(RvDnsEngine *channel, select_fd_args *fd_args, RvInt64 now)
{
    RvStatus status;
    rvServerState *server;
    int count;

    /* Make sure the server has a socket and is selected in read_fds. */
    server = &channel->servers[fd_args->server];
    if (&server->tcp_socket != fd_args)
        return;

    if (server->tcp_lenbuf_pos < TCP_LENWORD_SIZE)
    {
        /* We haven't yet read a length word, so read that (or what's left to read of it). */
        status = RvSocketReceiveBuffer(&server->tcp_socket.fd.fd,
                                       server->tcp_lenbuf + server->tcp_lenbuf_pos,
                                       TCP_LENWORD_SIZE - server->tcp_lenbuf_pos,
                                       channel->logMgr, (RvSize_t*)&count, NULL);
        if (status != RV_OK)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "read_tcp_data: Error in RvSocketReceiveBuffer(status=%d)", status));
            handle_error(channel, fd_args->server, now);
            return;
        }

        server->tcp_lenbuf_pos += count;
        if (server->tcp_lenbuf_pos < TCP_LENWORD_SIZE)
            return;

        /* We finished reading the length word so decode the length. */
        server->tcp_length = (server->tcp_lenbuf[0] << 8) | server->tcp_lenbuf[1];
        if (server->tcp_length > channel->tcp_bufflen)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "read_tcp_data: Invalid length word(%d)", server->tcp_length));
            handle_error(channel, fd_args->server, now);
            return;
        }
        server->tcp_buffer_pos = 0;
    }

    /* Read data into the allocated buffer. */
    status = RvSocketReceiveBuffer(&server->tcp_socket.fd.fd,
                                   server->tcp_buffer + server->tcp_buffer_pos,
                                   server->tcp_length - server->tcp_buffer_pos,
                                   channel->logMgr, (RvSize_t*)&count, NULL);
    if (status != RV_OK)
    {
        RvLogError(channel->dnsSource, (channel->dnsSource,
            "read_tcp_data: Error in RvSocketReceiveBuffer(status=%d)", status));
        handle_error(channel, fd_args->server, now);
        return;
    }

    server->tcp_buffer_pos += count;
    if (server->tcp_buffer_pos < server->tcp_length)
        return;

    /* We finished reading this answer; process it and
     * prepare to read another length word.
     */
    process_answer(channel, server->tcp_buffer, server->tcp_length,
                   fd_args->server, 1, now);

    server->tcp_lenbuf_pos = 0;
}

/* If any UDP sockets select true for reading, process them. */
static void read_udp_packets(RvDnsEngine *channel, select_fd_args *fd_args, RvInt64 now)
{
    RvStatus status;
    rvServerState *server;
    RvSize_t count;
    unsigned char buf[PACKETSZ + 1];
    RvAddress remoteAddress; /* just to make sure that RvSocketReceiveBuffer will use UDP (on Nucleus) */

    /* Make sure the server has a socket and is selected in read_fds. */
    server = &channel->servers[fd_args->server];
    if (&server->udp_socket != fd_args)
        return;

    status = RvSocketReceiveBuffer(&server->udp_socket.fd.fd, buf, sizeof(buf),
                                   channel->logMgr, (RvSize_t*)&count, &remoteAddress);
    if (status != RV_OK)
    {
        /* IP address exist but doesn't have a DNS server */
        RvLogError(channel->dnsSource, (channel->dnsSource,
            "read_udp_packets: Error in RvSocketReceiveBuffer(status=%d)", status));
        handle_error(channel, fd_args->server, now);
        return;   /* Eli.N */
    }

    process_answer(channel, buf, (int)count, fd_args->server, 0, now);
}

/* If any queries have timed out, note the timeout and move them on. */
static void process_timeouts(RvDnsEngine *channel, RvInt64 now, RvBool error)
{
#define FUNC "process_timeouts"

    rvQuery *query;

    (void)rvDnsEngineSanityCheck(channel);

    for(;;) {
        for(query = channel->queries; query; query = query->qnext)
        {
            if ((RvInt64IsNotEqual(query->timeout, RvInt64Const(0,0,0)) &&
                RvInt64IsGreaterThanOrEqual(now, query->timeout)) || error)
            {
                
                query->error_status = error ? ARES_ECONNREFUSED : ARES_ETIMEOUT;
                break;
            }
        }

        if(query == 0) {
            break;
        }

        DnsLogDebug((LOG_FUNC "Timeout on query %d", query->qid));

        /* next_server function eventually may unlock 'channel', so after this function
         *  we may not assume nothing about query queue, so we start from the beginning 
         */
        next_server(channel, query, now);
    } 
#undef FUNC 
}

/* Handle an answer from a server. */
static void process_answer(RvDnsEngine *channel, unsigned char *abuf,
                           int alen, int whichserver, int tcp, RvInt64 now)
{
    int tc, rcode;
    rvQuery *query;

    /* If there's no room in the answer for a header, we can't do much with it. */
    if (alen < HFIXEDSZ)
        return;

    /* Find the query corresponding to this packet. */
    query = find_query(channel, abuf);

    if (!query)
        return;

    /* Grab the truncate bit and response code from the packet. */
    tc = DNS_HEADER_TC(abuf);
    rcode = DNS_HEADER_RCODE(abuf);

    /* If we got a truncated UDP packet and are not ignoring truncation,
     * don't accept the packet, and switch the query to TCP if we hadn't
     * done so already.
     */
    if ((tc || alen > PACKETSZ) && !tcp && !(channel->flags & ARES_FLAG_IGNTC))
    {
        if (!query->using_tcp)
        {
            query->using_tcp = 1;
            ares__send_query(channel, query, now);
        }
        return;
    }

    /* Limit alen to PACKETSZ if we aren't using TCP (only relevant if we
     * are ignoring truncation.
     */
    if (alen > PACKETSZ && !tcp)
        alen = PACKETSZ;

    /* If we aren't passing through all error packets, discard packets
     * with SERVFAIL, NOTIMP, or REFUSED response codes.
     */
    if(rcode == SERVFAIL || rcode == NOTIMP || rcode == REFUSED)
    {
        query->skip_server[whichserver] = 1;
        if (query->server == whichserver)
            next_server(channel, query, now);
        return;
    }
    
    if (!same_questions((const unsigned char*)query->qbuf, query->qlen, abuf, alen))
    {
        if (query->server == whichserver)
            next_server(channel, query, now);
        return;
    }

    end_query(channel, query, ARES_SUCCESS, abuf, alen);
}

static rvQuery *find_query(RvDnsEngine *channel, unsigned char *abuf)
{
    int id;
    rvQuery *query;

    id = DNS_HEADER_QID(abuf);

    for (query = channel->queries; query; query = query->qnext)
    {
        if (query->qid == id)
            break;
    }

    return query;
}

static int same_questions(const unsigned char *qbuf, int qlen,
                          const unsigned char *abuf, int alen)
{
    struct {
        const unsigned char *p;
        int qdcount;
        char name[RV_DNS_MAX_NAME_LEN+1];
        int namelen;
        int type;
        int dnsclass;
    } q, a;
    int i, j;

    if (qlen < HFIXEDSZ || alen < HFIXEDSZ)
        return 0;

    /* Extract qdcount from the request and reply buffers and compare them. */
    q.qdcount = DNS_HEADER_QDCOUNT(qbuf);
    a.qdcount = DNS_HEADER_QDCOUNT(abuf);
    if (q.qdcount != a.qdcount)
        return 0;

    /* For each question in qbuf, find it in abuf. */
    q.p = qbuf + HFIXEDSZ;
    for (i = 0; i < q.qdcount; i++)
    {
        /* Decode the question in the query. */
        q.namelen = ares_expand_name(q.p, qbuf, qlen, q.name, sizeof(q.name));
        if (q.namelen < 0)
            return 0;
        q.p += q.namelen;
        if (q.p + QFIXEDSZ > qbuf + qlen)
            return 0;
        q.type = DNS_QUESTION_TYPE(q.p);
        q.dnsclass = DNS_QUESTION_CLASS(q.p);
        q.p += QFIXEDSZ;

        /* Search for this question in the answer. */
        a.p = abuf + HFIXEDSZ;
        for (j = 0; j < a.qdcount; j++)
        {
            /* Decode the question in the answer. */
            a.namelen = ares_expand_name(a.p, abuf, alen, a.name, sizeof(a.name));
            if (a.namelen < 0)
                return 0;
            a.p += a.namelen;
            if (a.p + QFIXEDSZ > abuf + alen)
                return 0;
            a.type = DNS_QUESTION_TYPE(a.p);
            a.dnsclass = DNS_QUESTION_CLASS(a.p);
            a.p += QFIXEDSZ;

            /* Compare the decoded questions. */
            if (strcasecmp(q.name, a.name) == 0 &&
                q.type == a.type && q.dnsclass == a.dnsclass)
                break;
        }

        if (j == a.qdcount)
            return 0;
    }
    return 1;
}

static void handle_error(RvDnsEngine *channel, int whichserver, RvInt64 now)
{
    rvQuery *query;

    RvLogDebug(channel->dnsSource, (channel->dnsSource,
        "handle_error: Closing socket for server no. %d", whichserver));

    /* Reset communications with this server. */
    ares__close_sockets(channel, whichserver);

    /* Tell all queries talking to this server to move on and not try
     * this server again.
     */
    for(;;) {

        for (query = channel->queries; query; query = query->qnext) {
            if (query->server == whichserver) {
                break;
            }
        }

        if(query == 0) {
            break;
        }

        query->skip_server[whichserver] = 1;
        next_server(channel, query, now);
    }
}

static int open_socket(RvDnsEngine *channel, int whichserver, RvSocketProtocol protocol)
{
    RvStatus status;
    rvServerState *server;
    RvUint16 port;
    RvUint16 origPort;
    select_fd_args *fd_args;
    RvSocket s;

    server = &channel->servers[whichserver];

    if (protocol == RvSocketProtocolTcp)
    {
        port = channel->tcp_port;
        fd_args = &server->tcp_socket;
    }
    else
    {
        port = channel->udp_port;
        fd_args = &server->udp_socket;
    }

    /* Acquire a socket. */
#if (RV_NET_TYPE & RV_NET_IPV6)
    if (server->addr.addrtype == RV_ADDRESS_TYPE_IPV6)
        status = RvSocketConstruct(RV_ADDRESS_TYPE_IPV6, protocol, channel->logMgr, &s);
    else
#endif
        status = RvSocketConstruct(RV_ADDRESS_TYPE_IPV4, protocol, channel->logMgr, &s);
    if (status != RV_OK)
        return ARES_ESERVICE;

    /* Set the server port number. */
    origPort = RvAddressGetIpPort(&server->addr);
    if(origPort == 0) {
        RvAddressSetIpPort(&server->addr, port);
    }

    /* Set the socket non-blocking. */
    status = RvSocketSetBlocking(&s, RV_FALSE, channel->logMgr);
    if (status != RV_OK)
    {
        RvLogError(channel->dnsSource, (channel->dnsSource,
            "open_socket: Error in RvSocketSetBlocking(%d)", status));
        RvSocketDestruct(&s, RV_FALSE, NULL, channel->logMgr);
        return ARES_ESERVICE;
    }

    /* If using TCP connect to the server. */
    if (protocol == RvSocketProtocolTcp)
    {
        status = RvSocketConnect(&s, &server->addr, channel->logMgr);
        if (status != RV_OK)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "open_socket: Error in RvSocketConnect(%d)", status));
            RvSocketDestruct(&s, RV_FALSE, NULL, channel->logMgr);
            return ARES_ESERVICE;
        }

        server->tcp_lenbuf_pos = 0;
        server->qhead          = NULL;
        server->qtail          = NULL;
    }

    RvFdConstruct(&fd_args->fd, &s, channel->logMgr);
    /* save fd_args data */
    fd_args->protocol = protocol;
    fd_args->server   = whichserver;
    fd_args->channel  = channel;

    /* Register the selectCB function for this socket in the select engine */
    status = RvSelectAdd(channel->selectEngine, &fd_args->fd, RV_SELECT_READ, selectCb);
    if (status != RV_OK)
    {
        RvLogError(channel->dnsSource, (channel->dnsSource,
            "open_socket: Error in RvSelectAdd(%d)", status));
        RvSocketDestruct(&s, RV_FALSE, NULL, channel->logMgr);
        return ARES_ESERVICE;
    }

    return ARES_SUCCESS;
}



static void next_server(RvDnsEngine *channel, rvQuery *query, RvInt64 now)
{
    /* Advance to the next server or try. */
    query->server++;
    for (; query->try_index < channel->tries; query->try_index++)
    {
        for (; query->server < channel->nservers; query->server++)
        {
            if (!query->skip_server[query->server])
            {
                RvLogDebug(channel->dnsSource, (channel->dnsSource,
                           "next_server: Start working with server no. %d", query->server));
                query->try_index++;  /* Eli.N: another try */
                ares__send_query(channel, query, now);
                return;
            }
        }
        query->server = 0;
        
        /* Only one try if we're using TCP. */
        if (query->using_tcp)
            break;
    }
    end_query(channel, query, ARES_ENDOFSERVERS, NULL, 0);
}

static void end_query(RvDnsEngine *channel, rvQuery *query, int queryStatus,
                      unsigned char *abuf, int alen)
{
    rvQuery **q;
    int rcode, ancount, i;

    if (RvInt64IsNotEqual(query->timeout, RvInt64Const(0,0,0)))
        RvTimerCancel(&query->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);

    if (queryStatus == ARES_SUCCESS && abuf != NULL)
    {
        /* Pull the response code and answer count from the packet. */
        rcode = DNS_HEADER_RCODE(abuf);
        ancount = DNS_HEADER_ANCOUNT(abuf);

        /* Convert errors. */
        switch (rcode)
        {
        case NOERROR:
            queryStatus = (ancount > 0) ? ARES_SUCCESS : ARES_ENODATA;
            break;
        case FORMERR:
            queryStatus = ARES_EFORMERR;
            break;
        case SERVFAIL:
            queryStatus = ARES_ESERVFAIL;
            break;
        case NXDOMAIN:
            queryStatus = ARES_ENOTFOUND;
            break;
        case NOTIMP:
            queryStatus = ARES_ENOTIMP;
            break;
        case REFUSED:
            queryStatus = ARES_EREFUSED;
            break;
        }

        if (queryStatus != ARES_SUCCESS)
        {
            RvLogDebug(channel->dnsSource, (channel->dnsSource,
                       "end_query(queryId=%d): Erroneous response code (%d)",
                       query->user_qid, queryStatus));
        }

        if(queryStatus == ARES_ENOTFOUND || queryStatus == ARES_ENODATA) {
            rvDnsTreatNxdomain(channel, abuf, alen);
        }

    }


    /* First, remove the query from the chain in the channel */
    for (q = &channel->queries; *q; q = &(*q)->qnext)
    {
        if (*q == query)
            break;
    }
    *q = query->qnext;

    /* Simple cleanup policy: if no queries are remaining, close all
     * network sockets unless STAYOPEN is set.
     */
    if (!channel->queries && !(channel->flags & ARES_FLAG_STAYOPEN))
    {
        for (i = 0; i < channel->nservers; i++)
            ares__close_sockets(channel, i);
    }

    /* Next, call the user callback,
     * but only after releasing the channel lock to prevent deadlock
     */
    RvLockRelease(&channel->lock, channel->logMgr);
    
    RvLogDebug(channel->dnsSource, (channel->dnsSource,
               "ARES: calling internal CB (engine=%p,context=%d,queryId=%d,status=%d)",
               channel, query->user_arg, query->user_qid, queryStatus));

    query->user_callback(channel, query->newRecordCB, query->user_arg, queryStatus, query->user_qid, abuf, alen);
    
    RvLogDebug(channel->dnsSource, (channel->dnsSource,
               "ARES: internal CB returned (engine=%p,context=%d,queryId=%d)",
               channel, query->user_arg, query->user_qid));

    RvLockGet(&channel->lock, channel->logMgr);
}
#endif /* RV_DNS_ARES */
