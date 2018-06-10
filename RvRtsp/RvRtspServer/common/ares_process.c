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
	(RV_OS_TYPE == RV_OS_TYPE_MAC)      || (RV_OS_TYPE == RV_OS_TYPE_NETBSD) || \
	(RV_OS_TYPE == RV_OS_TYPE_IPHONE)
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
static void process_timeouts(RvDnsEngine *channel, RvInt64 now);
static void process_answer(RvDnsEngine *channel, unsigned char *abuf,
                           int alen, int whichserver, int tcp, RvInt64 now);
static void handle_error(RvDnsEngine *channel, int whichserver, RvInt64 now);
static void next_server(RvDnsEngine *channel, rvQuery *query, RvInt64 now);
static void aresNextNonPriortyServer(RvDnsEngine *channel, rvQuery *query, RvInt64 now);
static void aresNextPriorityServer(RvDnsEngine *channel, rvQuery *query, RvInt64 now);
static int open_socket(RvDnsEngine *channel, int whichserver, RvSocketProtocol protocol);
static int same_questions(const unsigned char *qbuf, int qlen,
                          const unsigned char *abuf, int alen);
static void end_query(RvDnsEngine *channel, rvQuery *query, int status,
                      unsigned char *abuf, int alen);

#define LOG_SRC (channel->dnsSource)
#define LOG_FUNC LOG_SRC, FUNC ": "

#define DnsLogDebug(p) RvLogDebug(LOG_SRC, p)
#define DnsLogExcep(p) RvLogExcep(LOG_SRC, p)
#define DnsLogError(p) RvLogError(LOG_SRC, p)

#if RV_DNS_SANITY_CHECK

/* Assumes lock hold on 'channel' object */
RvBool rvDnsEngineSanityCheck(RvDnsEngine *channel) {
#undef FUNC
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
}

#endif

#ifndef RV_DNS_STATISTICS

#  define RV_DNS_STATISTICS 1

#endif

#if RV_DNS_STATISTICS
#  define STATS RvInt32 _nCompares = 0
#  define INCR_CMP(ch) (_nCompares++)
#  define INCR_SRCH(ch) ((ch)->nSearches++)
#  define CALC_AVG(ch)  \
    if(_nCompares > ch->nMaxCompares) {ch->nMaxCompares = _nCompares;} \
    ch->nCompares += _nCompares; \
    ((ch)->nAvgCompares = (RvInt32)((ch)->nCompares / (ch)->nSearches))
#else
#  define STATS
#  define INCR_CMP(ch) 
#  define INCR_SRCH(ch) 
#  define CALC_AVG(ch)  
#endif

static 
rvQuery *findQueryByQid(RvDnsEngine *channel, int id) {
    rvQuery *query;
    STATS;

    INCR_SRCH(channel);

    for (query = channel->queries; query && query->qid != id; query = query->qnext)
    {
        INCR_CMP(channel);
    }

    CALC_AVG(channel);
    return query;
}

rvQuery* findQueryByUqid(RvDnsEngine *channel, unsigned int qid) {
    rvQuery *cur;
    STATS;

    INCR_SRCH(channel);
    for(cur = channel->queries; cur && cur->user_qid != qid; cur = cur->qnext) {
        INCR_CMP(channel);
    }

    CALC_AVG(channel);
    return cur;
}

static
void removeTcpQuery(RvDnsEngine *channel, rvQuery *q) {
#undef FUNC
#define FUNC "removeTcpQuery"

    rvServerState *server = &channel->servers[q->server];
    rvQuery *cur;
    rvQuery *prev;

    if(q->tcp_len == 0) {
        /* This query was fully sent */
        return;
    }

    cur = server->qhead;
    if(cur == 0) {
        /* No queries were successfully sent */
        DnsLogDebug((LOG_FUNC "Tcp queries list is empty"));
        return;
    }
    /* query at the head may be partially send. If this is the case
     * we should continue sending it
     */
    if(cur == q) {
        /* Query at the head is partially sent */
        if(q->tcp_data != q->qbuf - 2) {
            int qlen = q->tcp_len;

            if(qlen > RV_DNS_MAX_SENDBUF_SIZE) {
                /* shouldn't happen */
                DnsLogExcep((LOG_FUNC "Query is too long: %d (maximum expected %d)", q->tcp_len, RV_DNS_MAX_SENDBUF_SIZE));
                qlen = RV_DNS_MAX_SENDBUF_SIZE;
            }
            memcpy(server->deletedQueryBuf, q->tcp_data, qlen);
            server->pDeletedQuery = server->deletedQueryBuf;
            server->nDeletedQueryLen = qlen;
        }

        server->qhead = q->tcp_next;
        if(server->qhead == 0) {
            server->qtail = 0;
        }

        return;
    }

    for(prev = cur, cur = cur->tcp_next; cur != 0 && cur != q; prev = cur, cur = cur->tcp_next) {

    }

    if(cur == 0) {
        /* It may happen if, for example, this query wasn't send yet */
        DnsLogDebug((LOG_FUNC "TCP query %d wasn't found in the server query list: ", q->user_qid));
        return;
    }

    prev->tcp_next = cur->tcp_next;
    if(cur == server->qtail) {
        server->qtail = prev;
    }
}

rvQuery* removeQueryByUqid(RvDnsEngine *channel, unsigned int qid) {
    rvQuery *cur;
    rvQuery *prev;
    STATS;

    INCR_SRCH(channel);
    for(prev = 0, cur = channel->queries; cur && cur->user_qid != qid; prev = cur, cur = cur->qnext) {
        INCR_CMP(channel);
    }

    CALC_AVG(channel);

    if(cur == 0) {
        return cur;
    }

    if(prev == 0) {
        channel->queries = cur->qnext;
    } else {
        prev->qnext = cur->qnext;
    }

    if(cur == channel->lastQuery) {
        channel->lastQuery = prev;
    }

    if(!cur->using_tcp) {
        RvTimerCancel(&cur->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
    } else {
        /* It's tcp query - remove from the list of server tcp queries */
        removeTcpQuery(channel, cur);
    }

	if (cur->isHeartbeat == RV_TRUE)
	{
		channel->bHbWasProcessed = RV_TRUE;
	}
    return cur;
}

static
rvQuery* findExpiredQuery(RvDnsEngine *channel, RvInt64 now) {
    rvQuery *cur;
    STATS;

    INCR_SRCH(channel);

    for(cur = channel->queries; cur != 0; cur = cur->qnext) {
        INCR_CMP(channel);
        if ((RvInt64IsNotEqual(cur->timeout, RvInt64Const(0,0,0)) &&
            RvInt64IsGreaterThanOrEqual(now, cur->timeout))) {
                break;
        }
    }

    CALC_AVG(channel);

    return cur;
    
}

static
rvQuery* findQueryByServer(RvDnsEngine *channel, RvInt server) {
    rvQuery *cur;
    STATS;

    INCR_SRCH(channel);

    for(cur = channel->queries; cur != 0 && cur->server != server; cur = cur->qnext) {
        INCR_CMP(channel);
    }

    CALC_AVG(channel);
    return cur;
}

void rvAresAddQuery(RvDnsEngine *channel, rvQuery *query) {
    query->qnext = 0;
    if(channel->lastQuery != 0) {
        channel->lastQuery->qnext = query;
    } else {
        channel->queries = query;
    }

    channel->lastQuery = query;
}


int rv_ares__send_query(RvDnsEngine *channel, rvQuery *query, RvInt64 now)
{
#undef FUNC
#define FUNC "rv_ares__send_query"

    RvStatus status;
    rvServerState *server;
    RvSize_t count = 0;
    RvInt64 timeout;

    if (channel->nservers <= 0) {
        end_query(channel, query, ARES_ENOSERVERS, 0, 0);
        return ARES_ENOSERVERS;
    }

    DnsLogDebug((LOG_FUNC "Sending query qid=%x to net", query->user_qid));

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
                /*query->skip_server[query->server] = 1;*/
                SKIP_SERVER_SET(query, query->server);
                next_server(channel, query, now);
                return ARES_SUCCESS;
            }
        }

        /* qbuf points to the UDP-style query
         * TCP-style queries are prepended by 2-bytes query length
         */
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
                DnsLogError((LOG_FUNC "Opening UDP socket for server %d failed (qid=%x)", query->server, query->user_qid));
                SKIP_SERVER_SET(query, query->server);
                /*query->skip_server[query->server] = 1;*/
                next_server(channel, query, now);
                return ARES_SUCCESS;
            }
        }

        /* calculate timeout and start a timer BEFORE sending the query
         * only to make sure that the we have available timer
         */
        {

            RvRandom rf;
            RvInt64  rfs;

            RvRandomGeneratorGetInRange(&channel->rnd, 500, &rf);
            rfs = rf * 1000000;
            timeout = channel->timeout + rfs;
        }

        status = RvTimerStart(&query->timer, channel->timerQueue, RV_TIMER_TYPE_ONESHOT,
                              timeout, (RvTimerFunc)timerCb, &server->udp_socket);
        if (status != RV_OK)
        {
            DnsLogError((LOG_FUNC "Starting timer failed for qid=%x", query->user_qid));
            end_query(channel, query, ARES_ESERVICE, NULL, 0);
            return ARES_ESERVICE;
        }

        query->timeout = RvInt64Add(now, timeout);
        DnsLogDebug((LOG_FUNC "Timer for %d secs was started for qid=%x", (RvInt)(timeout /  RV_TIME64_NSECPERSEC), query->user_qid));

        status = RvSocketSendBuffer(&server->udp_socket.fd.fd, (RvUint8*)query->qbuf,
                                    query->qlen, &server->addr, channel->logMgr, &count);
        if (status != RV_OK)
        {
            DnsLogError((LOG_FUNC "Sending query for qid=%x to %s failed", query->user_qid, server->sAddr));
            RvTimerCancel(&query->timer, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
            query->timeout = RvInt64Const(0,0,0);
            SKIP_SERVER_SET(query, query->server);
            /*query->skip_server[query->server] = 1;*/
            next_server(channel, query, now);
            return ARES_SUCCESS;
        }
        DnsLogDebug((LOG_FUNC "Query with qid=%x sent using UDP to %s", query->user_qid, server->sAddr));
    }

    return ARES_SUCCESS;
}

void rv_ares__close_sockets(RvDnsEngine *channel, int i, RvBool waitForCallbacks)
{
    rvServerState *server = &channel->servers[i];

    /* Close the TCP and UDP sockets. */
    if (server->tcp_socket.fd.fd != RV_INVALID_SOCKET)
    {
        RvSelectRemoveEx(channel->selectEngine, &server->tcp_socket.fd, waitForCallbacks, 0);
        RvSocketDestruct(&server->tcp_socket.fd.fd, RV_FALSE, NULL, NULL);
        server->tcp_socket.fd.fd = RV_INVALID_SOCKET;
    }

    if (server->udp_socket.fd.fd != RV_INVALID_SOCKET)
    {
        RvSelectRemoveEx(channel->selectEngine, &server->udp_socket.fd, waitForCallbacks, 0);
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
        } else {
            handle_error(channel, fd_args->server, now);
        }

        /* process_timeouts(channel, now); */

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
        process_timeouts(fd_args->channel, now);

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
    RvInt bytesSent;

    /* Make sure server has data to send and is selected in write_fds. */
    server = &channel->servers[fd_args->server];
    if (!server->qhead || &server->tcp_socket != fd_args)
        return;

#if !defined(NEED_MANUAL_WRITEV)
    {
        IOVEC vec[DEFAULT_IOVEC_LEN];
        int i;
        int bytesNotSent;
        RvBool continueWriting;

        do {
            /* If next query to be sent is deleted in the middle of send process,
            * we still have to continue to send it's data
            */
            if(server->nDeletedQueryLen) {
                i = 1;
                vec[0].iov_base = (char *)server->pDeletedQuery;
                vec[0].iov_len = bytesNotSent = server->nDeletedQueryLen;
            } else {
                i = 0;
                bytesNotSent = 0;
            }

            /* Fill in the iovecs and send. */
            for (query = server->qhead; query && i < DEFAULT_IOVEC_LEN; query = query->tcp_next, i++) {
                vec[i].iov_base = query->tcp_data;
                vec[i].iov_len  = query->tcp_len;
                bytesNotSent += query->tcp_len;
            }

            bytesSent = (int)writev((int)(server->tcp_socket.fd.fd), vec, i);
            if (bytesSent < 0)
            {
                if(errno == EAGAIN) {
                    bytesSent = 0;
                } else {
                    RvLogError(channel->dnsSource, (channel->dnsSource,
                        "write_tcp_data: Error in writev(errno=%d)", errno));
                    handle_error(channel, fd_args->server, now);
                    return;
                }
            }

            bytesNotSent -= bytesSent;

            if(bytesSent >= server->nDeletedQueryLen) {
                bytesSent -= server->nDeletedQueryLen;
                server->nDeletedQueryLen = 0;
                server->pDeletedQuery = server->deletedQueryBuf;
            } else {
                server->nDeletedQueryLen -= bytesSent;
                server->pDeletedQuery += bytesSent;
                bytesSent = 0;
            }

            /* Advance the send queue by as many bytes as we sent. */
            while (bytesSent)
            {
                query = server->qhead;
                if (bytesSent >= query->tcp_len)
                {
                    bytesSent -= query->tcp_len;
                    query->tcp_len = 0;
                    server->qhead = query->tcp_next;
                    if (server->qhead == NULL)
                        server->qtail = NULL;
                }
                else
                {
                    query->tcp_data += bytesSent;
                    query->tcp_len  -= bytesSent;
                    break;
                }
            }

            /* We can continue writing if we succeeded to sent all the data we had and 
             * there 
             */
            continueWriting = (bytesNotSent == 0) && ((server->nDeletedQueryLen > 0) || (server->qhead != 0));

        } while(continueWriting);
    } 
#else
    {
        RvSize_t count = 0;
        RvSocket *sock = &server->tcp_socket.fd.fd;
        RvBool continueWriting = RV_TRUE;

        if(server->nDeletedQueryLen) {
            status = RvSocketSendBuffer(sock, server->pDeletedQuery, server->nDeletedQueryLen, 0, 
                channel->logMgr, &count);

            if (status != RV_OK)
            {
                RvLogError(channel->dnsSource, (channel->dnsSource,
                    "write_tcp_data: Error in RvSocketSendBuffer(status=%d)", status));
                handle_error(channel, fd_args->server, now);
                return;
            }

            server->nDeletedQueryLen -= (RvInt)count;
            if(server->nDeletedQueryLen) {
                continueWriting = RV_FALSE;
                server->pDeletedQuery += count;
            } else {
                server->pDeletedQuery = server->deletedQueryBuf;
            }
        }

        while(continueWriting && ((query = server->qhead) != 0)) {

            status = RvSocketSendBuffer(sock, (RvUint8*)query->tcp_data,
                query->tcp_len, NULL, channel->logMgr, &count);

            if (status != RV_OK)
            {
                RvLogError(channel->dnsSource, (channel->dnsSource,
                    "write_tcp_data: Error in RvSocketSendBuffer(status=%d)", status));
                handle_error(channel, fd_args->server, now);
                return;
            }

            bytesSent = (RvInt)count;

            /* Advance the send queue by as many bytes as we sent. */
            if (bytesSent == query->tcp_len)
            {
                server->qhead = query->tcp_next;
                if (server->qhead == NULL)
                    server->qtail = NULL;
                continueWriting = RV_TRUE;
            }
            else
            {
                query->tcp_data += bytesSent;
                query->tcp_len  -= bytesSent;
                continueWriting = RV_FALSE;
            }
        }
    }

#endif
    
    if (server->qhead == NULL && server->nDeletedQueryLen == 0)
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
    RvSize_t count;

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
                                       channel->logMgr, &count, NULL);
        if (status != RV_OK)
        {
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "read_tcp_data: Error in RvSocketReceiveBuffer(status=%d)", status));
            handle_error(channel, fd_args->server, now);
            return;
        }

        server->tcp_lenbuf_pos += (RvInt)count;
        if (server->tcp_lenbuf_pos < TCP_LENWORD_SIZE)
            return;

        /* We finished reading the length word so decode the length. */
        server->tcp_length = (server->tcp_lenbuf[0] << 8) | server->tcp_lenbuf[1];
        if (server->tcp_length > server->tcp_bufflen)
        {
            RvUint8 *newbuf = 0;
            RvInt32 newLen = server->tcp_bufflen;
            RvInt32 reqLen = server->tcp_length;

            while(newLen <= reqLen) {
                newLen <<= 1;
            }

            if(newLen > 65536) {
                newLen = 65536;
            }

            RvLogDebug(channel->dnsSource, (channel->dnsSource,
                "read_tcp_data: increasing tcp buffer to %d", newLen));

            status = RvMemoryAlloc(0, newLen, channel->logMgr, (void **)&newbuf);
            if(status != RV_OK) {
                RvLogError(channel->dnsSource, (channel->dnsSource, 
                    "read_tcp_data: failed to increase tcp buffer to %d", newLen));
                handle_error(channel, fd_args->server, now);
                return;
            }

            RvMemoryFree(server->tcp_buffer, channel->logMgr);
            server->tcp_buffer = newbuf;
            server->tcp_bufflen = newLen;
            
        }
        server->tcp_buffer_pos = 0;
    }

    /* Read data into the allocated buffer. */
    status = RvSocketReceiveBuffer(&server->tcp_socket.fd.fd,
                                   server->tcp_buffer + server->tcp_buffer_pos,
                                   server->tcp_length - server->tcp_buffer_pos,
                                   channel->logMgr, &count, NULL);
    if (status != RV_OK)
    {
        RvLogError(channel->dnsSource, (channel->dnsSource,
            "read_tcp_data: Error in RvSocketReceiveBuffer(status=%d)", status));
        handle_error(channel, fd_args->server, now);
        return;
    }

    server->tcp_buffer_pos += (RvInt)count;
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
    RvSocket sock;

    /* Make sure the server has a socket and is selected in read_fds. */
    server = &channel->servers[fd_args->server];
    if (&server->udp_socket != fd_args)
            return;
    sock = server->udp_socket.fd.fd;

    for(;;) {
        count = 0;
        status = RvSocketReceiveBuffer(&sock, buf, sizeof(buf),
            channel->logMgr, &count, &remoteAddress);

        if(count == 0) {
            break;
        }

        if (status != RV_OK)
        {
            /* IP address exist but doesn't have a DNS server */
            RvLogError(channel->dnsSource, (channel->dnsSource,
                "read_udp_packets: Error in RvSocketReceiveBuffer(status=%d)", status));
            handle_error(channel, fd_args->server, now);
            break;   
        }

        process_answer(channel, buf, (int)count, fd_args->server, 0, now);
        if(sock != server->udp_socket.fd.fd) {
            /* Sanity check: as the result of process_answer original socket may be destroyed */
            break;
        }
    }
}

/* If any queries have timed out, note the timeout and move them on. */
static void process_timeouts(RvDnsEngine *channel, RvInt64 now)
{
#undef FUNC
#define FUNC "process_timeouts"

    rvQuery *query;

    (void)rvDnsEngineSanityCheck(channel);

    query = findExpiredQuery(channel, now);

    if(query == 0) {
        return;
    }

#ifndef RV_TIMER_CLEAR
    query->timer.event = 0;
#else
    RV_TIMER_CLEAR(&query->timer);
#endif
    query->error_status = ARES_ETIMEOUT;

    DnsLogDebug((LOG_FUNC "Timeout on query qid=%x", query->user_qid));

    /* next_server function eventually may unlock 'channel', so after this function
    *  we may not assume nothing about query queue, so we start from the beginning 
    */
    next_server(channel, query, now);
}

/* Handle an answer from a server. */
static void process_answer(RvDnsEngine *channel, unsigned char *abuf,
                           int alen, int whichserver, int tcp, RvInt64 now)
{
#undef FUNC
#define FUNC "process_answer"
    int tc, rcode;
    rvQuery *query;
    int id;

    /* If there's no room in the answer for a header, we can't do much with it. */
    if (alen < HFIXEDSZ)
        return;

    /* Find the query corresponding to this packet. */
    id = DNS_HEADER_QID(abuf);
    query = findQueryByQid(channel, id);


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
            rv_ares__send_query(channel, query, now);
        }
        return;
    }

    /* Limit alen to PACKETSZ if we aren't using TCP (only relevant if we
     * are ignoring truncation.
     */
    if (alen > PACKETSZ && !tcp)
        alen = PACKETSZ;

	if (channel->bEnableDnsPriority == RV_TRUE)
	{
		/* Server responded, so we increase server priority and change correct highest priority if needed */
		if(channel->servers[whichserver].priority != ARES_MAX_PRIORITY && (rcode == NOERROR || rcode == NXDOMAIN))
		{
			channel->servers[whichserver].priority--;
			DnsLogDebug((LOG_FUNC " Changing current priority of server #%d value to %d (engine=%p, qid=%x)", whichserver, channel->servers[whichserver].priority, channel,query->user_qid));
		}
		if(channel->servers[whichserver].priority <= channel->chpValue)
		{
			channel->chpValue	= channel->servers[whichserver].priority;
			channel->chpServer	= whichserver;
			DnsLogDebug((LOG_FUNC " Changing current highest priority server to server #%d, value to %d (engine=%p, qid=%x)", channel->chpServer, channel->chpValue, channel,query->user_qid));
		}
	}
    /* If we aren't passing through all error packets, discard packets
     * with SERVFAIL, NOTIMP, or REFUSED response codes.
     */
    if(rcode == SERVFAIL || rcode == NOTIMP || rcode == REFUSED)
    {
        SKIP_SERVER_SET(query, whichserver);
        /*query->skip_server[whichserver] = 1;*/
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
        q.namelen = rv_ares_expand_name(q.p, qbuf, qlen, q.name, sizeof(q.name));
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
            a.namelen = rv_ares_expand_name(a.p, abuf, alen, a.name, sizeof(a.name));
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
    rv_ares__close_sockets(channel, whichserver, RV_FALSE);

    /* Tell all queries talking to this server to move on and not try
     * this server again.
     */
    for(;;) {

        query = findQueryByServer(channel, whichserver);
        if(query == 0) {
            break;
        }

        SKIP_SERVER_SET(query, whichserver);
        /*query->skip_server[whichserver] = 1;*/
        next_server(channel, query, now);
    }
}

static 
RvStatus bind_socket(RvDnsEngine *channel, RvSocket *sock, RvInt addrType) {
    RvAddress localAddress;
    RvAddress *pLocalAddress;
    RvStatus  s;
    RvInt channelAddrType;

    static RvBool bindNeeded = 
#ifdef RV_SOCKET_BIND_NEEDED
        RV_TRUE
#else   
        RV_FALSE
#endif
        ;

    pLocalAddress = &channel->localAddr;
    channelAddrType = RvAddressGetType(pLocalAddress);
    if(channelAddrType == RV_ADDRESS_TYPE_NONE) {
        if(bindNeeded) {
            RvAddressConstruct(addrType, &localAddress);
            pLocalAddress = &localAddress;
        }
    } else {
        bindNeeded = RV_TRUE;
    }

    if(bindNeeded) {
        s = RvSocketBind(sock, pLocalAddress, NULL,  channel->logMgr);
        if(pLocalAddress == &localAddress) {
            RvAddressDestruct(&localAddress);
        }

        if (s != RV_OK)	{
            RvLogError(channel->dnsSource, (channel->dnsSource, "open_socket: Error in RvSocketBind(%d)", s));
            return ARES_ESERVICE;
        }
    }

    return RV_OK;
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
    if (server->addr.addrtype == RV_ADDRESS_TYPE_IPV6) {
        status = RvSocketConstruct(RV_ADDRESS_TYPE_IPV6, protocol, channel->logMgr, &s);
    } else
#endif
    {
        status = RvSocketConstruct(RV_ADDRESS_TYPE_IPV4, protocol, channel->logMgr, &s);
    }

    if (status != RV_OK) {
        return ARES_ESERVICE;
    }

    status = bind_socket(channel, &s, server->addr.addrtype);
    if(status != RV_OK) {
        RvSocketDestruct(&s, RV_FALSE, NULL, channel->logMgr);
        return status;
    }

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
        fd_args->fd.fd = RV_INVALID_SOCKET;
        return ARES_ESERVICE;
    }


    return ARES_SUCCESS;
}

#ifndef RVDNS_RETRIES_STRATEGY
#  define RVDNS_RETRIES_STRATEGY(configTries, nServers) ((configTries) < (nServers) ? (nServers) : (configTries))
#else

typedef int (*RvDnsRetryStrategy)(int configTries, int nServers);
int defaultRetryStrategy(int tries, int nservers) {
    return tries < nservers ? nservers : tries;
}

static RvDnsRetryStrategy sDnsRetryStrategy = defaultRetryStrategy;
RVCOREAPI void RvAresSetRetryStrategy(RvDnsRetryStrategy s) {
    sDnsRetryStrategy = s;
}

#  undef  RVDNS_RETRIES_STRATEGY
#  define RVDNS_RETRIES_STRATEGY(configTries, nServers) sDnsRetryStrategy(configTries, nServers)
#endif

static void next_server(RvDnsEngine *channel, rvQuery *query, RvInt64 now)
{
	if (channel->bEnableDnsPriority == RV_TRUE)
	{
		aresNextPriorityServer(channel, query, now);
	}
	else
	{
		aresNextNonPriortyServer(channel,query,now);
	}
}
static void aresNextNonPriortyServer(RvDnsEngine *channel, rvQuery *query, RvInt64 now)
{
	RvInt foundServer = -1;
    RvInt isTcpQuery = query->using_tcp;

	RvLogDebug(channel->dnsSource, (channel->dnsSource,	"aresNextNonPriortyServer: engine=%p, current server %d, isHeartbeat=%d, heartbeatType=%d",
		channel, query->server, query->isHeartbeat, channel->heartbeatType));

    if(query->serversGeneration != channel->serversGeneration) {
#if defined(RV_DNS_RESTART_QUERIES_ON_SERVERCHANGE)
        RvInt i;

        /* Probably, new servers where set on dnsEngine, restart this query */
        query->serversGeneration = channel->serversGeneration;
        query->try_index = 0;
        for(i = 0; i < channel->skipServerWords; i++) {
            query->skip_server[i] = 0;
        }

        foundServer = 0;
        RvLogDebug(channel->dnsSource, (channel->dnsSource,
            "engine=%p, New servers installed for dnsEngine, restarting query qid=%x", channel,query->user_qid));
#else
        RvLogDebug(channel->dnsSource, (channel->dnsSource,
			"aresNextNonPriortyServer: channel=%p, New servers installed for dnsEngine, ending query qid=%x", channel,query->user_qid));
        foundServer = -1;
#endif /*#if defined(RV_DNS_RESTART_QUERIES_ON_SERVERCHANGE)*/
       
    } else {
        int ntries;

        ntries = RVDNS_RETRIES_STRATEGY(channel->tries, channel->nservers);
        query->try_index++;
        if(query->try_index < ntries) {
            /* Another attempts left */

            RvInt nServers;
            RvInt lastServer;
            RvInt curServer;

            nServers = channel->nservers;
            curServer = lastServer = query->server;

            do {
                /* Advance to the next server and wrap around if last server reached */
                curServer++;
                if(curServer == nServers) {
                    curServer = 0;
                    if(isTcpQuery) {
                        /* No more than 1 attempt per server for TCP queries */
                        break;
                    }
                }
                if(!SKIP_SERVER(query, curServer)) {
                    /* If server wasn't marked as 'skip' - choose it */
                    foundServer = curServer;
                    break;
                }
            } while(curServer != lastServer);
        }
    }

	if(foundServer >= 0) {
        /* TCP queries are held in the server linked-list, so if it's a tcp query, it
        * should be removed from server linked list first
        */
        if(isTcpQuery) {
            removeTcpQuery(channel, query);
        }

		query->server = foundServer;
		RvLogDebug(channel->dnsSource, (channel->dnsSource,
			"aresNextPriorityServer: Attempt #%d for qid=%x server %d (engine=%p), %d attempts left", query->try_index, query->user_qid, query->server, channel,channel->tries - query->try_index - 1));
		rv_ares__send_query(channel, query, now);
	} else {
		end_query(channel, query, ARES_ENDOFSERVERS, NULL, 0);
	}
}
/* lower the priority of the current server and chooses current highest priority server */
static void aresNextPriorityServer(RvDnsEngine *channel, rvQuery *query, RvInt64 now)
{
    /* current server failed, so decrease priority for the server */
	RvLogDebug(channel->dnsSource, (channel->dnsSource,	"aresNextPriorityServer: Current priority for engine=%p, server no. %d is %d, isHeartbeat=%d, heartbeatType=%d"
			, channel, query->server, channel->servers[query->server].priority, query->isHeartbeat, channel->heartbeatType));
	if(channel->servers[query->server].priority != channel->minPriority)
	{
		channel->servers[query->server].priority++;
	}
	if(channel->chpValue == channel->minPriority)
	{
		channel->chpServer = (channel->chpServer == channel->nservers - 1)?0:(channel->chpServer + 1);
	}
	else
	{
		if(channel->chpServer == query->server)
		{
			int i;
			channel->chpValue = channel->servers[query->server].priority;
			for(i = channel->nservers - 1; i >= 0 ; i--)
			{
				if(channel->servers[i].priority <= channel->chpValue && i != query->server)
				{
					channel->chpServer	= i;
					channel->chpValue	= channel->servers[i].priority;
				}
			}
		}
	}
	/* Advance to the next server or try. */
	RvLogDebug(channel->dnsSource, (channel->dnsSource,
		"aresNextPriorityServer: Processed priority for engine=%p, server no. %d is %d, isHeartbeat=%d"
		, channel,query->server, channel->servers[query->server].priority, query->isHeartbeat));
    query->server = channel->chpServer;
	RvLogDebug(channel->dnsSource, (channel->dnsSource,
		"aresNextPriorityServer: engine=%p, Using server no. %d for the next try, isHeartbeat=%d", channel,query->server, query->isHeartbeat));
	/* Only one try if we're using TCP. */
    if((query->try_index < channel->tries) && !query->using_tcp)
    {
        query->try_index++;  /* Eli.N: another try */
        rv_ares__send_query(channel, query, now); 
		return; 
	} 
    end_query(channel, query, ARES_ENDOFSERVERS, NULL, 0);
}

static void end_query(RvDnsEngine *channel, rvQuery *query, int queryStatus,
                      unsigned char *abuf, int alen)
{
#undef FUNC
#define FUNC "end_query"

    rvQuery *q;
    int rcode, ancount, i;
    rvAresCallback user_callback;
    void *user_arg;
    unsigned int user_qid;
    RvDnsNewRecordCB newRecordCB;

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
                       "end_query(qid=%x): Erroneous response code (%d)",
                       query->user_qid, queryStatus));
        }

        if(queryStatus == ARES_ENOTFOUND || queryStatus == ARES_ENODATA) {
            rvDnsTreatNxdomain(channel, abuf, alen);
        }

    }


    /* First, remove the query from the chain in the channel */
    q = removeQueryByUqid(channel, query->user_qid);

    if(q != query) {
        DnsLogExcep((LOG_FUNC "Unexpected: wrong query found for qid=%x (expected %p, got %p)", query, q));
    }

    /* Simple cleanup policy: if no queries are remaining, close all
     * network sockets unless STAYOPEN is set.
     */
    if (!channel->queries && !(channel->flags & ARES_FLAG_STAYOPEN))
    {
        for (i = 0; i < channel->nservers; i++)
            rv_ares__close_sockets(channel, i, RV_FALSE);
    }

    /* Next, call the user callback if needed,
     * but only after releasing the channel lock to prevent deadlock
     */

	if(query->isHeartbeat == RV_FALSE)
	{
    user_callback = query->user_callback;
    user_arg = query->user_arg;
    user_qid = query->user_qid;
    newRecordCB = query->newRecordCB;
   
    RvLogDebug(channel->dnsSource, (channel->dnsSource,
			"ARES: calling internal CB (engine=%p,context=%p,qid=%x,status=%d)",
               channel, user_arg, user_qid, queryStatus));

    /* Only internal callbacks are called here - no unlock on DNS engine: DNS will be unlocked
		* inside this callback before calling "real" user callback
		*/
    user_callback(channel, newRecordCB, user_arg, queryStatus, user_qid, abuf, alen);
    
    RvLogDebug(channel->dnsSource, (channel->dnsSource,
			"ARES: internal CB returned (engine=%p,context=%p,qid=%x)",
               channel, user_arg, user_qid));
	} /* if !isHeartbeat */
}
#endif /* RV_DNS_ARES */
