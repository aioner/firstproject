/***********************************************************************
Filename   : rvsocket.c
Description: enables OS-independent BSD4.4 sockets operations.
************************************************************************
                Copyright (c) 2001 RADVISION Inc.
************************************************************************
NOTICE:
This document contains information that is proprietary to RADVISION LTD.
No part of this publication may be reproduced in any form whatsoever
without written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

#include "rvlock.h"
#include "rvstdio.h"
#include "rvaddress.h"
#include "rvthread.h"
#include "rvnet2host.h"
#include "rvsocket.h"
#include "rvccoreglobals.h"
#include "rvsocketshadow.h"


#if (RV_NET_TYPE != RV_NET_NONE)

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */

#define LOGSRC (&logMgr->socketSource)

#define SRCFUNC LOGSRC, FUNC

#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvSockLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->socketSource, p);}
#else
#define RvSockLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvSockLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->socketSource, p);}
#else
#define RvSockLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvSockLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->socketSource, p);}
#else
#define RvSockLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_EXCEP)
#define RvSockLogExcep(p) {if (logMgr != NULL) RvLogExcep(&logMgr->socketSource, p);}
#else
#define RvSockLogExcep(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define RvSockLogDebug(p) {if (logMgr != NULL) RvLogDebug(&logMgr->socketSource, p);}
#else
#define RvSockLogDebug(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_INFO)
#define RvSockLogInfo(p) {if (logMgr != NULL) RvLogInfo(&logMgr->socketSource, p);}
#else
#define RvSockLogInfo(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
#define RvSockLogWarning(p) {if (logMgr != NULL) RvLogWarning(&logMgr->socketSource, p);}
#else
#define RvSockLogWarning(p) {RV_UNUSED_ARG(logMgr);}
#endif


/* Include files required for each different Operating System */
#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA) || \
	((RV_OS_TYPE == RV_OS_TYPE_WINCE) && (_WIN32_WCE >= 400))
#pragma warning (push,3)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning (pop)

#elif ((RV_OS_TYPE == RV_OS_TYPE_WINCE) && (_WIN32_WCE < 400))
#include <winsock.h>

#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)
#include <inet.h>

#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#include <ioLib.h>
#include <iosLib.h>
#include <inetLib.h>
#include <sockLib.h>
#include "rvstdio.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <signal.h>
#if (RV_TOOL_TYPE == RV_TOOL_TYPE_DIAB)
#include <socklib.h>
#endif

#elif (RV_SOCKET_TYPE == RV_SOCKET_PSOS)
#include <pna.h>
#include <errno.h>

#elif (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
#include "rvselect.h"
#include "rvtimer.h"
#include "rvtimerengine.h"

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
#include "rvsymsock.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#elif (RV_OS_TYPE == RV_OS_TYPE_MOPI)

#elif (RV_SOCKET_TYPE == RV_SOCKET_BSD)

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)  || (RV_OS_TYPE == RV_OS_TYPE_LINUX)    || \
    (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE) || (RV_OS_TYPE == RV_OS_TYPE_TRU64)    || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX)     || (RV_OS_TYPE == RV_OS_TYPE_INTEGRITY) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD)	|| (RV_OS_TYPE == RV_OS_TYPE_MAC) || \
    (RV_OS_TYPE == RV_OS_TYPE_NETBSD)   || (RV_OS_TYPE == RV_OS_TYPE_IPHONE)

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE)
#include <sys/filio.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "rvstdio.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#else
/* VxWorks, OSE, WinCE */
#include "rvstdio.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <signal.h>
#endif

#endif  /* include files by OS */

#include "rvwindscp.h"

/* Some definitions which are different between operating systems */
#if (RV_SOCKET_TYPE == RV_SOCKET_PSOS)
#define FIONREAD FIOREAD            /* for ioctl()... */
#define imr_multiaddr imr_mcastaddr /* for multicasting */
#endif


#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)

#  ifndef HAS_BINDPORT0
#    if NET_VERSION_COMP >= NET_5_2
#      define HAS_BINDPORT0 1
#    else
#      define HAS_BINDPORT0 0
#    endif
#  endif /* HAS_BINDPORT0 */

/* If WOULD_BLOCK isn't defined, define it to something meaningless in Nucleus NET context */
# ifndef NU_WOULD_BLOCK
#   define NU_WOULD_BLOCK -501
# endif


#define MAX_TIMERS_PER_THREAD 100

#define AF_INET NU_FAMILY_IP                /* translate nucleus IP definition */
#define SOCK_STREAM NU_TYPE_STREAM          /* translate nucleus TCP definition */
#define SOCK_DGRAM  NU_TYPE_DGRAM           /* translate nucleus UDP definition */
#define SOCK_RAW    NU_TYPE_RAW             /* translate nucleus RAW(only IP) definition */
#define imr_multiaddr sck_multiaddr         /* for multicasting */
#define imr_interface sck_inaddr            /* for multicasting */

/* redefine structure names */
#define sockaddr_in addr_struct
#define sa_family family
#define sin_family family
#define sin_port port

#if !HAS_NONBLOCKEDCONNECT
/* RvSocketState: Socket life cycle - different states */
typedef enum
{
    RvSocketState_Idle,
    RvSocketState_Connecting,
    RvSocketState_Connected,
    RvSocketState_Closing,
    RvSocketState_Closed

} RvSocketState;

/* RvThreadState: thread's various states */
typedef enum
{
    RvThreadState_Idle,
    RvThreadState_Running,
    RvThreadState_Stopped

} RvThreadState;

/* connectionInfo: Array of information on each connection,
   used only by nucleus implementation */
typedef struct
{
    RvTimer         timer;          /* handle to the socket's associated timer (for limited connecting time) */
    RvBool          timerInitiated; /* a flag indicating if this socket has a timer for a connect/close task */
    RvSocketState   socketState;    /* socket state */
    RvThreadState   threadState;    /* thread state (thread for blocking connect()/close()) */
    RvThread        threadId;       /* thread's ID */
    RvAddress       sockAddress;    /* socket remote address */
    RvUint32        localPort;      /* local port */
    RvPortRange*    portRange;      /* port range that provides the port which this socket is bound to */
    RvSelectEngine* selectEngine;   /* select engine that probes this socket */
} connectionInfo;

/* Array of information on each connection, used only by nucleus implementation */
static connectionInfo* nuConInfo;

/* timer mechanism for nucleus is required, to time-frame the connecting/closing tasks */
static RvTimerQueue timerQueue;
static RvTimerEngine tengine;

/* locking is required in nucleus to 'thread-safe' the DB (nuConInfo),
   which is used from several tasks */
static RvLock nuLock;
#define RV_SOCKET_LOCK() RvLockGet(&nuLock, NULL);
#define RV_SOCKET_UNLOCK() RvLockRelease(&nuLock, NULL);



static void RvSocketNucleusEventsTaskConnect(
    IN RvThread *th,
    IN void* data);

static void RvSocketNucleusEventsTaskClose(
    IN RvThread *th,
    IN void* data);

static RvBool RvSocketNucleusEvTimerExp(
    IN void* argument);

static void RvSocketNucleusSocketClean(
    IN RvSocket socket);

#endif /* HAS_NONBLOCKEDCONNECT */


#if !HAS_BINDPORT0

/* NUCLEUS can't bind to 0. We have to find a specific port for NUCLEUS. If someone
tries to bind NUCLEUS to port 0, we just use this global port range object. */
static RvPortRange RvSocketPortRange;

#endif


#endif  /* NUCLEUS definitions */


#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)

#define sockaddr_in         sock_sockaddr_in
/*lint -save -e652 */
/* warning -- #define of symbol 'errno' declared previously */
#undef errno
#define errno       Mmb_errno
/*lint -restore */

#endif



/* errno specific definition - Get the error number of the last error in the system */
#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA)
#define RvSocketErrNo WSAGetLastError()
/* See if the socket's last error was "would block" */
#define RvSocketErrorWouldBlock(err) (err == WSAEWOULDBLOCK)
/* check if received network buffer is bigger than RV buffer size */
#define RvSocketErrorMsgSize(err)    (err == WSAEMSGSIZE)

#define ENOTCONN WSAENOTCONN

#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)
#define RvSocketErrNo GetLastError()
/* See if the socket's last error was "would block" */
#define RvSocketErrorWouldBlock(err) (err == WSAEWOULDBLOCK)
/* check if received network buffer is bigger than RV buffer size */
#define RvSocketErrorMsgSize(err)    (err == WSAEMSGSIZE)

#define ENOTCONN        WSAENOTCONN
#define EADDRINUSE      WSAEADDRINUSE
#define EADDRNOTAVAIL   WSAEADDRNOTAVAIL


#elif (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
#define RvSocketErrNo 777
#define RvSocketErrorMsgSize(err) RV_FALSE

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
#define RvSocketErrNo 777
/* See if the socket's last error was "would block" */
#define RvSocketErrorWouldBlock(err) (err == 52)
#define RvSocketErrorMsgSize(err) RV_FALSE

#elif (RV_SOCKET_TYPE == RV_SOCKET_BSD) || (RV_SOCKET_TYPE == RV_SOCKET_PSOS)
#define RvSocketErrNo errno
/* See if the socket's last error was "would block" */
#define RvSocketErrorWouldBlock(err) (err == EWOULDBLOCK)
#define RvSocketErrorMsgSize(err) RV_FALSE

#endif



/* Macro definitions for the actual OS function call for sockets */
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
#define TCP_SEND NU_Send
#define UDP_SEND NU_Send_To
#define TCP_RECV NU_Recv
#define UDP_RECV(a,b,c,d,e,f) NU_Recv_From(a,b,c,d,e,(RvInt16*)f)
#define SET_SOCK_OPT NU_Setsockopt
#define GET_SOCK_OPT NU_Getsockopt
#define BIND_SOCKET NU_Bind
#define LISTEN_SOCKET NU_Listen
#define CLOSE_SOCKET NU_Close_Socket
#define OPEN_SOCKET NU_Socket
#define ACCEPT_SOCKET(a, b, c) NU_Accept(a, b, 0)
#define SOCKLEN_TYPE int
#define RV_LINGER_VALUE_TYPE int
#define RV_MULTICASTTTL_VALUE_TYPE RvUint8
#define RV_MULTICAST_LOOP_VALUE_TYPE RvUint8
#define RV_SOCKADDR_PTR struct sockaddr_in *
#define RV_IP_MREQ IP_MREQ

#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)
#define TCP_SEND inet_send
#define UDP_SEND sendto
#define TCP_RECV recv
#define UDP_RECV recvfrom
#define SET_SOCK_OPT setsockopt
#define BIND_SOCKET bind
#define LISTEN_SOCKET listen
#define CLOSE_SOCKET close
#define OPEN_SOCKET socket
#define ACCEPT_SOCKET accept
#define SOCKLEN_TYPE int
#define RV_LINGER_VALUE_TYPE int
#define RV_MULTICASTTTL_VALUE_TYPE RvUint8
#define RV_MULTICAST_LOOP_VALUE_TYPE RvUint8
#define RV_SOCKADDR_PTR struct sockaddr *
#define RV_IP_MREQ struct ip_mreq

#elif (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_WINCE)
#define TCP_SEND send
#define UDP_SEND sendto
#define TCP_RECV recv
#define UDP_RECV recvfrom
#define SET_SOCK_OPT setsockopt
#define BIND_SOCKET bind
#define LISTEN_SOCKET listen
#define CLOSE_SOCKET closesocket
#define OPEN_SOCKET socket
#define ACCEPT_SOCKET accept
#define SOCKLEN_TYPE int
#define RV_LINGER_VALUE_TYPE u_short
#define RV_MULTICASTTTL_VALUE_TYPE int
#define RV_MULTICASTTTLIPV6_VALUE_TYPE int
#define RV_MULTICAST_LOOP_VALUE_TYPE u_char
#define RV_MULTICAST_LOOPIPV6_VALUE_TYPE u_int
#define RV_SOCKADDR_PTR struct sockaddr *
#define RV_IP_MREQ struct ip_mreq
#define RV_IPV6_MREQ IPV6_MREQ

#elif (RV_OS_TYPE == RV_OS_TYPE_MOPI)
#define TCP_SEND					Mmb_tcp_send
#define UDP_SEND					Mmb_udp_send
#define TCP_RECV					Mmb_tcp_recv
#define UDP_RECV					Mmb_udp_recv
#define SET_SOCK_OPT				Mmb_setsockopt
#define GET_SOCK_OPT sock_getsockopt
#define BIND_SOCKET					Mmb_bindSocket
#define LISTEN_SOCKET   			Mmb_listenSocket
#define CLOSE_SOCKET				Mmb_closeSocket
#define OPEN_SOCKET                 Mmb_RvSocketOpen
#define ACCEPT_SOCKET           	Mmb_acceptSocket
#define SOCKLEN_TYPE int
#define RV_SOCKADDR_PTR				struct sockaddr *
#define RV_LINGER_VALUE_TYPE        int
#define RV_MULTICASTTTL_VALUE_TYPE  int
#define RV_MULTICAST_LOOP_VALUE_TYPE unsigned char
#define RV_IP_MREQ                  struct ip_mreq

#else
#define TCP_SEND send
#define UDP_SEND sendto
#define TCP_RECV recv
#define UDP_RECV recvfrom
#define SET_SOCK_OPT setsockopt
#define BIND_SOCKET bind
#define LISTEN_SOCKET listen
#define CLOSE_SOCKET close
#define OPEN_SOCKET socket

/* For some reason, return value of accept under PSOS is long and not int */
#if RV_SOCKET_TYPE == RV_SOCKET_PSOS
#define ACCEPT_SOCKET(sock, paddr, paddrlen) ((RvSocket)accept(sock, paddr, paddrlen))
#else
#define ACCEPT_SOCKET accept
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_TRU64) || (RV_OS_TYPE == RV_OS_TYPE_HPUX) || \
  (RV_OS_TYPE == RV_OS_TYPE_INTEGRITY) || (RV_OS_TYPE == RV_OS_TYPE_PSOS) || \
  (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) || RV_ANDROID
#define SOCKLEN_TYPE int
#else
#define SOCKLEN_TYPE unsigned int
#endif
#define RV_LINGER_VALUE_TYPE int
#define RV_MULTICASTTTL_VALUE_TYPE RvUint8
#define RV_MULTICASTTTLIPV6_VALUE_TYPE unsigned int
#define RV_MULTICAST_LOOP_VALUE_TYPE RvUint8
#define RV_MULTICAST_LOOPIPV6_VALUE_TYPE unsigned int
#define RV_SOCKADDR_PTR struct sockaddr *
#define RV_IP_MREQ struct ip_mreq
#define RV_IPV6_MREQ struct ipv6_mreq

#endif

#ifndef GET_SOCK_OPT
#define GET_SOCK_OPT getsockopt
#endif

/* for ioctl() adaptation */
#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA) || (RV_OS_TYPE == RV_OS_TYPE_WINCE)
#define RV_IOCTL_ARGP_TYPE u_long*
#define ioctl ioctlsocket

#elif (RV_SOCKET_TYPE == RV_SOCKET_PSOS) || (RV_OS_TYPE == RV_OS_TYPE_OSE)
#define RV_IOCTL_ARGP_TYPE char*

#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#define RV_IOCTL_ARGP_TYPE int

#elif (RV_SOCKET_TYPE == RV_SOCKET_BSD) || (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
#define RV_IOCTL_ARGP_TYPE RvUint32*

#endif

/* On some OS (namely Linux) SIGPIPE signals cause some weird deadlock in libc
 * We'll try to prevent these signals by setting MSG_NOSIGNAL flag in 'send'
 * operation. On OS that doesn't provide this flag - define it as 0.
 */
#ifdef MSG_NOSIGNAL
#define RV_MSG_NOSIGNAL MSG_NOSIGNAL
#else
#define RV_MSG_NOSIGNAL 0
#endif

/********************************************************************************************
 * RvSocketProtocolData
 * Structure of a connection manager protocol. This holds information about the different
 * types of supported network protocols.
 ********************************************************************************************/
typedef struct
{
  int           protocol;     /* The protocol itself */
  int           protocolType; /* Type of protocol */
} RvSocketProtocolData;


#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif

#if !defined(SOL_IPV6) && defined(IPPROTO_IPV6)
#  define SOL_IPV6 IPPROTO_IPV6
#endif

#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
/* Information about the different types of network protocols supported */
static RvSocketProtocolData rvSocketProtocolInfo[] =
{
    {IPPROTO_UDP,  SOCK_DGRAM},   /* RvSocketProtocolUdp                              */
    {IPPROTO_TCP,  SOCK_STREAM},  /* RvSocketProtocolTcp                              */
    {IPPROTO_SCTP, SOCK_STREAM},  /* RvSocketProtocolSctp (1-to-1)                    */
    {IPPROTO_ICMP, SOCK_RAW},     /* RvSocketProtocolRawIcmp                          */
    {IPPROTO_SCTP, SOCK_RAW},     /* RvSocketProtocolRawSctp (for our in-house impl.) */
};
#endif



/* Make sure SD_SEND is defined as 1. We use this when calling shutdown() to notify the TCP
   stack that we don't intend to send anything more. */
#ifndef SD_SEND
#define SD_SEND 1
#endif

/* Aligned socket address
 * CC uses sockdata to contain IPv4 and IPv6 addresses (sockaddr_in, sockaddr_in6),
 * so it should be properly aligned to prevent nasty behavior on some architectures (bus error on Sparc, for example)
 *
 */
typedef union {
    struct sockaddr_in dummy;
    RvUint8 sockdata[RV_SOCKET_SOCKADDR_SIZE];
} rvASockAddr;

#define RVSOCKADDR(u) (RV_SOCKADDR_PTR)((u).sockdata)
#define RVSOCKDATA(u) ((u).sockdata)


static RvStatus RvSocketGetAddressType(IN RvSocket *s, IN RvLogMgr *logMgr, OUT RvInt *addrType);

/* static RvBool tosSupported = RV_FALSE;*/

void CheckTosSupport(void);

#define LOG_SRC (&logMgr->socketSource)
#define SRC_FUNC LOG_SRC, FUNC ": "

#define RV_LOG_ADDR(addr, buf) (addr ? RvAddressGetString(addr, sizeof(buf), buf) : "")



/********************************************************************************************
 *
 *               Private functions - Socket sharer for pSOS
 *
 *
 * The following code implements a socket sharing service required to
 * circumvent thread-local socket concepts like PSoS/PNA.  Other OS's
 * don't use this service at all.
 *
 * The service creates and destroys all sockets from it's thread and
 * allows other threads to request socket sharing.  Once a thread has
 * shared a given socket, the information is cached so future references
 * to the socket from that thread do not access the service.
 *
 ********************************************************************************************/

#if (RV_SOCKET_USE_SHARER == RV_YES)

static RvStatus RvSocketConstructHere(
    IN RvSocket*        sock,
    IN RvInt            addressType,
    IN RvSocketProtocol protocolType,
    IN RvLogMgr*        logMgr,
    IN RvInt            options);

typedef enum
{
    RvSocketSharerCommandExit,      /* Exit socket sharer thread */
    RvSocketSharerCommandCreate,    /* Create a new socket */
    RvSocketSharerCommandDestroy,   /* Close a socket */
    RvSocketSharerCommandShare,     /* Share an existing socket */
    RvSocketSharerCommandMultiShare /* Share an array of existing sockets */
} RvSocketSharerCommand;

typedef struct
{
    /* The following fields are used for the command the sharer thread should process */
    RvSocketSharerCommand   command;    /* Command to process */
    RvSocket*               socket;     /* Socket we're dealing with */
    int                     width;      /* width of array of sockets for multi sharer */

    /* Creation parameters */
    RvInt               addressType;    /* Address type for creation command */
    RvSocketProtocol    protocolType;   /* Protocol type for creation command */
    RvInt               options;        /* socket options (like LINGER, REUSE-ADDR and so on) */

    /* Sharing parameters */
    RvThreadId          task;           /* Task to share this socket with for sharing command */
    RvSocket*           sharableSocket; /* Resulting shared socket handle */

} RvSocketSharerCmd;


typedef struct
{
    RvLock              lock;           /* Lock for the sharer operations */
    RvThread            thread;         /* Socket sharer thread */
    RvSemaphore         requestSem;     /* Semaphore posted to the sharer thread */
    RvSemaphore         responseSem;    /* Semaphore posted from the sharer thread */
    RvSocketSharerCmd   cmd;            /* Command to process in sharer thread */
    int                 result;         /* Result of the processed command */
    RvInt               error;          /* the possible error of sharer operation */
    RvInt               errorFD;        /* the value of socket descriptor whose sharing
                                           operation failed during multi share command */
} RvSocketSharer;


static RvSocketSharer RvSharer;         /* Socket sharer to use */



/**********************************************************************************
 * RvSocketSharerThread
 * Process a command, and preform it on the shared socket.
 *
 * RETURN:
 *  Nothing
 */
static void RvSocketSharerThread(
    IN RvThread* th,
    IN void* data)
{
    RvSocketSharer* sharer = (RvSocketSharer *)data;
    RvStatus res;
    RvBool exitThread = RV_FALSE;

    RV_UNUSED_ARG(th);

    while (!exitThread)
    {
        /* Let's wait until we have a request to process */
        res = RvSemaphoreWait(&sharer->requestSem, RvThreadGetLogManager(th));
        if (res != RV_OK)
            break;

        /* Process the command */
        switch (sharer->cmd.command)
        {
        case RvSocketSharerCommandExit:
            exitThread = RV_TRUE;
            break;
        case RvSocketSharerCommandCreate:
            sharer->error = 0;
            sharer->result = RvSocketConstructHere(sharer->cmd.socket, sharer->cmd.addressType,
                sharer->cmd.protocolType, NULL,sharer->cmd.options);
            sharer->error = RvSocketErrNo;
            break;
        case RvSocketSharerCommandDestroy:
            sharer->error = 0;
            sharer->result = CLOSE_SOCKET(*(sharer->cmd.socket));
            sharer->error = RvSocketErrNo;
            break;
        case RvSocketSharerCommandShare:
            sharer->error = 0;
#if (RV_SOCKET_TYPE == RV_SOCKET_PSOS)
            *(sharer->cmd.sharableSocket) = shr_socket(*(sharer->cmd.socket), sharer->cmd.task);
            if (*(sharer->cmd.sharableSocket) < 0)
            {
                sharer->result = RV_ERROR_UNKNOWN;
                sharer->error = RvSocketErrNo;
            }
            else
                sharer->result = RV_OK;
#else
            *sharer->cmd.sharableSocket = *(sharer->cmd.socket);
            sharer->result = RV_OK;
#endif
            break;
        case RvSocketSharerCommandMultiShare:
            {
                int i;
                int ourwidth = -1;
                sharer->error = 0;
                for(i = 0; i < sharer->cmd.width; i++)
                {
                    if(sharer->cmd.socket[i] >= 0)
                    {
                         sharer->cmd.socket[i] = shr_socket((RvSocket)i, sharer->cmd.task);
                         if (sharer->cmd.socket[i] > ourwidth)
                             ourwidth = sharer->cmd.socket[i];
                         /* guarantee that the first failed socket's errno will be saved */
                         if (sharer->cmd.socket[i] < 0 && sharer->error == 0)
                         {
                             sharer->error = RvSocketErrNo;
                             sharer->errorFD = i;
                         }
                    }
                }
                ourwidth++; /* new width of shared sockets. */
                sharer->result = ourwidth;
            }
            break;
        }

        /* Let's make sure the client doesn't wait any longer - we're done with this command */
        res = RvSemaphorePost(&sharer->responseSem, RvThreadGetLogManager(th));
        if (res != RV_OK)
            break;
    }

    /* This sequence must be followed or pSOS will not clean everything up */
    fclose(0);
    close_f(0);
    close(0);
    free((void *)(-1));
    /* DO NOT PUT 't_delete(0)'! task must gracefully terminate through threadWrapper,
       otherwise the core will never shutdown */
}


/**********************************************************************************
 * RvSocketSharerInit
 * initiate the socket sharer module.
 *
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
static RvStatus RvSocketSharerInit(void)
{
    RvStatus res;

    res = RvLockConstruct(NULL, &RvSharer.lock);
    if (res != RV_OK)
        return res;

    res = RvSemaphoreConstruct(RvUint32Const(0), NULL, &RvSharer.requestSem);
    if (res != RV_OK)
    {
        RvLockDestruct(&RvSharer.lock, NULL);
        return res;
    }

    res = RvSemaphoreConstruct(RvUint32Const(0), NULL, &RvSharer.responseSem);
    if (res != RV_OK)
    {
        RvSemaphoreDestruct(&RvSharer.requestSem, NULL);
        RvLockDestruct(&RvSharer.lock, NULL);
        return res;
    }

    res = RvThreadConstruct(RvSocketSharerThread, &RvSharer, NULL, &RvSharer.thread);
    if (res != RV_OK)
    {
        RvSemaphoreDestruct(&RvSharer.responseSem, NULL);
        RvSemaphoreDestruct(&RvSharer.requestSem, NULL);
        RvLockDestruct(&RvSharer.lock, NULL);
        return res;
    }

    /* Create and start the sharer thread. */
    res = RvThreadCreate(&RvSharer.thread);
    if (res == RV_OK)
    {
        RvThreadSetName(&RvSharer.thread, "RvSharer");
        res = RvThreadStart(&RvSharer.thread);
    }

    if (res != RV_OK)
    {
        RvThreadDestruct(&RvSharer.thread);
        RvSemaphoreDestruct(&RvSharer.responseSem, NULL);
        RvSemaphoreDestruct(&RvSharer.requestSem, NULL);
        RvLockDestruct(&RvSharer.lock, NULL);
    }

    return res;
}


/**********************************************************************************
 * RvSocketSharerEnd
 * close the socket sharer module.
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
static RvStatus RvSocketSharerEnd(void)
{
    /* Make sure we exit the sharer thread */
    RvLockGet(&RvSharer.lock, NULL);
    RvSharer.cmd.command = RvSocketSharerCommandExit; /* Post an exit request */
    if (RvSemaphorePost(&RvSharer.requestSem, NULL) == RV_OK)
        RvSemaphoreWait(&RvSharer.responseSem, NULL);

    RvThreadDestruct(&RvSharer.thread);
    RvSemaphoreDestruct(&RvSharer.responseSem, NULL);
    RvSemaphoreDestruct(&RvSharer.requestSem, NULL);
    RvLockDestruct(&RvSharer.lock, NULL);
    return RV_OK;
}


/**********************************************************************************
 * RvSocketSharerConstruct
 * Construct a socket using the socket sharer module.
 * INPUT:
 *  sock   - Socket to construct
 *  addressType    - Type of address to create
 *  protocolType   - Type of protocol used by the socket
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
static RvStatus RvSocketSharerConstruct(
    IN RvLogMgr*        logMgr,
    IN RvSocket*        sock,
    IN RvInt            addressType,
    IN RvSocketProtocol protocolType,
    IN RvInt            options)
{
    RvStatus res,sharerRes;
    RvInt sharerErr;


    if (RvSharer.thread.id == RvThreadCurrentId())
        return RvSocketConstructHere(sock,addressType,protocolType, NULL,options);

    /* Send command to the sharer thread */
    res = RvLockGet(&RvSharer.lock, NULL);
    if (res != RV_OK)
        return res;

    RvSharer.cmd.command = RvSocketSharerCommandCreate;
    RvSharer.cmd.socket = sock;
    RvSharer.cmd.addressType = addressType;
    RvSharer.cmd.protocolType = protocolType;
    RvSharer.cmd.options = options;

    res = RvSemaphorePost(&RvSharer.requestSem, NULL);
    if (res == RV_OK)
        res = RvSemaphoreWait(&RvSharer.responseSem, NULL);

    if (res == RV_OK)
        res = (RvStatus)RvSharer.result;

    sharerRes = (RvStatus)RvSharer.result;
    sharerErr = RvSharer.error;

    RvLockRelease(&RvSharer.lock, NULL);

    if (sharerRes != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSharerConstruct: Failed to construct the socket, retVal=%d, errno=%d",
            sharerRes,sharerErr));
    }
    return res;
}


/**********************************************************************************
 * RvSocketSharerDestruct
 * Destruct the socket using the socket sharer module.
 * INPUT:
 *  sock   - Socket to destruct
 *  addressType    - Type of address to create
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
static RvStatus RvSocketSharerDestruct(
    IN RvSocket* sock)
{
    RvStatus res;

    if (RvSharer.thread.id == RvThreadCurrentId())
    {
        CLOSE_SOCKET(*sock);
        return RV_OK;
    }

    /* Send command to the sharer thread */
    res = RvLockGet(&RvSharer.lock, NULL);
    if (res != RV_OK)
        return res;
    RvSharer.cmd.command = RvSocketSharerCommandDestroy;
    RvSharer.cmd.socket = sock;

    res = RvSemaphorePost(&RvSharer.requestSem, NULL);
    if (res == RV_OK)
        res = RvSemaphoreWait(&RvSharer.responseSem, NULL);

    if (res == RV_OK)
        res = (RvStatus)RvSharer.result;

    RvLockRelease(&RvSharer.lock, NULL);
    return res;
}


/**********************************************************************************
 * RvSocketSharerShare
 * "share" a socket using the socket sharer module.
 * INPUT:
 *  sock   - Socket to share
 *  sharableSocket  - the counterpart shared socket
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
RvStatus RvSocketSharerShare(
    IN RvLogMgr* logMgr,
    IN RvSocket* sock,
    OUT RvSocket* sharableSocket)
{
    RvStatus res,sharerRes;
    RvInt sharerErr;

    if (RvSharer.thread.id == RvThreadCurrentId())
    {
        *sharableSocket = *sock;
        return RV_OK;
    }

    /* Send command to the sharer thread */
    res = RvLockGet(&RvSharer.lock, NULL);
    if (res != RV_OK)
        return res;

    RvSharer.cmd.command = RvSocketSharerCommandShare;
    RvSharer.cmd.socket = sock;
    RvSharer.cmd.task = RvThreadCurrentId();
    RvSharer.cmd.sharableSocket = sharableSocket;

    res = RvSemaphorePost(&RvSharer.requestSem, NULL);
    if (res == RV_OK)
        res = RvSemaphoreWait(&RvSharer.responseSem, NULL);

    if (res == RV_OK)
        res = (RvStatus)RvSharer.result;

    sharerRes = (RvStatus)RvSharer.result;
    sharerErr = RvSharer.error;

    RvLockRelease(&RvSharer.lock, NULL);

    if (sharerRes != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSharerShare: Failed to share the socket, retVal=%d, errno=%d",
            sharerRes,sharerErr));
    }

    return res;
}


/**********************************************************************************
 * RvSocketSharerMultiShare
 * "share" a set of sockets, using the socket sharer module. this is called right
 * before calling 'select()' function.
 * INPUT:
 *  sock   - a pointer to a set of sockets to share.
 *  width  - the length of the set.
 *  logMgr - the log manager
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
RvStatus RvSocketSharerMultiShare(
    IN OUT RvSocket* sock,
    IN int width,
    IN RvLogMgr* logMgr)
{
    RvStatus res;
    RvInt sharerErr,errFd;


    if (RvSharer.thread.id == RvThreadCurrentId())
        return width;

    /* Send command to the sharer thread */
    res = RvLockGet(&RvSharer.lock, NULL);
    if (res != RV_OK)
        return res;

    RvSharer.cmd.command = RvSocketSharerCommandMultiShare;
    RvSharer.cmd.socket = sock;
    RvSharer.cmd.width = width;
    RvSharer.cmd.task = RvThreadCurrentId();

    res = RvSemaphorePost(&RvSharer.requestSem, NULL);
    if (res == RV_OK)
        res = RvSemaphoreWait(&RvSharer.responseSem, NULL);

    if (res == RV_OK)
        res = (RvStatus)RvSharer.result;

    sharerErr = RvSharer.error;
    errFd = RvSharer.errorFD;

    RvLockRelease(&RvSharer.lock, NULL);

    if (sharerErr != 0)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSharerMultiShare: Failed to share the one of socket, socketFD=%d, errno=%d",
            errFd,sharerErr));
    }

    return res;
}


/**********************************************************************************
 * RvSocketSharerClose
 * close a socket in the socket sharer module.
 * INPUT:
 *  _sock   - Socket to close
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
void RvSocketSharerClose(IN RvSocket* sock)
{
    if (RvSharer.thread.id == RvThreadCurrentId())
        return;

    close(*sock);
}


/**********************************************************************************
 * RvSocketSetSockOpt
 * change a shared socket properties.
 * INPUT:
 *  sock   - Socket to construct.
 *  level  - level of property.
 *  optName - type of property.
 *  optVal  - value of property.
 *  optLen  - length of value.
 * RETURN:
 *  RvStatus - RV_OK on success, other on failure.
 */
static int RvSocketSetSockOpt(
    IN RvSocket*    socket,
    IN int          level,
    IN int          optName,
    IN const char*  optVal,
    IN int          optLen)
{
    RvSocket ourSocket;
    RvStatus status;
    RvThreadId threadId;
    int        res;

    /* If we are calling this function from inside the sharer thread, then
       we do not need to share the socket again */
    threadId = RvThreadCurrentId();
    if (threadId != RvSharer.thread.id)
    {
        status = RvSocketSharerShare(NULL, socket, &ourSocket);
        if (status != RV_OK)
            return -1;

        res = SET_SOCK_OPT(ourSocket, level, optName, optVal, optLen);

        RvSocketSharerClose(&ourSocket);
        return res;
    }

    res = SET_SOCK_OPT(*socket, level, optName, optVal, optLen);

    return res;
}


#else
/* (RV_SOCKET_USE_SHARER == RV_NO) */

#define RvSocketSharerEnd() RV_OK
#define RvSocketSharerConstruct(_log, _sock, _addressType, _protocolType) RV_OK
#define RvSocketSharerDestruct(_sock) RV_UNUSED_ARG(_sock)
#define RvSocketSharerShare(_logMgr, _sock, _sharableSock) (*(_sharableSock) = *(_sock)) ? RV_OK : RV_OK
#define RvSocketSharerClose(_sock) RV_UNUSED_ARG(_sock)
#define RvSocketSetSockOpt(_sock, _level, _optName, _optVal, _optLen) \
    SET_SOCK_OPT(*(_sock), (_level), (_optName), (_optVal), (_optLen))

#define RvSocketSharerMultiShare(_sock,_width,_logMgr) RV_OK


#endif  /* RV_SOCKET_USE_SHARER */




/********************************************************************************************
 *
 *                                      Private functions
 *
 ********************************************************************************************/


#if (RV_SOCKET_TYPE == RV_SOCKET_BSD)
/* Handler for SIGPIPE signals. We get these when TCP connections tear down in a non-graceful
   manner. Their default behavior in Unix is a core dump, so we put in place this signal
   handler that just ignores this signal. */
#if (RV_OS_TYPE != RV_OS_TYPE_WINCE) && (RV_OS_TYPE != RV_OS_TYPE_OSE) && \
    (RV_OS_TYPE != RV_OS_TYPE_MOPI)
static void RvSocketHandleSigPipe(IN int sigNo)
{
    RV_UNUSED_ARG(sigNo);

    /* Reset this signal */
    signal(SIGPIPE, RvSocketHandleSigPipe);
}
#endif /* (RV_OS_TYPE != RV_OS_TYPE_WINCE) */
#endif /* (RV_SOCKET_TYPE == RV_SOCKET_BSD) */


/********************************************************************************************
 * RvSocketConstructHere
 * Create a socket handle here. This function is internal, since some operating
 * systems such as pSOS needs to create the socket in a specified thread, other than
 * the one that called RvSocketConstruct()
 * This function is thread-safe.
 * INPUT:
 *  sock           - Socket to construct
 *  addressType    - Type of address to create
 *  protocolType   - Protocol to use with socket
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus RvSocketConstructHere(
    IN RvSocket*        sock,
    IN RvInt            addressType,
    IN RvSocketProtocol protocolType,
#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
    IN RvUint16         cid,
#endif
    IN RvLogMgr*        logMgr,
    RvInt               options)
{
    int addressFamily = 0;
    RvStatus status = RV_OK;

    /* default options */
    if (options == -1)
        options = RV_SOCKET_OPT_KEEP_ALIVE | RV_SOCKET_OPT_TCP_DELAY | RV_SOCKET_OPT_LINGER;

#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
	RV_UNUSED_ARG(addressFamily);
	status = RvSymSockConstruct(addressType,protocolType,logMgr,sock);
#else

    switch (addressType)
    {
    case RV_ADDRESS_TYPE_IPV4:
        addressFamily = AF_INET;
        break;

#if (RV_NET_TYPE & RV_NET_IPV6)
    case RV_ADDRESS_TYPE_IPV6:
        addressFamily = AF_INET6;
        break;
#endif

    default:
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA)

    *sock = WSASocket(addressFamily, rvSocketProtocolInfo[protocolType].protocolType,
        rvSocketProtocolInfo[protocolType].protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if ((*sock) == INVALID_SOCKET)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)

    status = RvSymSockConstruct(addressType,protocolType,logMgr,sock);

#else
    if ((*sock = OPEN_SOCKET(addressFamily,  rvSocketProtocolInfo[protocolType].protocolType,
        rvSocketProtocolInfo[protocolType].protocol)) < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS && !HAS_NONBLOCKEDCONNECT)
    /* make sure there's no old socket still open here */
    if (nuConInfo[*sock].socketState)
        RvSocketNucleusSocketClean(*sock);
#endif

#endif /* #else RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA */

#endif /*(RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN) */

#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
    /* associate the new socket with a connection ID */
    if (status == RV_OK)
    {
        if (RvSocketMopiSetConnectionId(*sock, cid) != 0)
        {
            /* association failed - close the new socket */
            Mmb_closeSocket(*sock);
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
    }
#endif

    if (status == RV_OK)
    {
        if (options & RV_SOCKET_OPT_REUSE_ADDR)
        {
            /* Make sure the TCP connection sends keep-alive messages once in a while */
            if (RvSocketReuseAddr(sock,logMgr) != RV_OK)
            {
                RvSockLogError((&logMgr->socketSource,
                    "RvSocketConstructHere: Failed in RvSocketReuseAddr errNo - %d", RvSocketErrNo));
            }
        }
        if (protocolType == RvSocketProtocolTcp)
        {
#if ((RV_SOCKET_TYPE != RV_SOCKET_NUCLEUS)  && (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN))
            /* no support for KEEP-ALIVE in Nucleus */
            /* NODELAY can only be set on a connected socket in Nucleus */

            int yes = RV_TRUE;

            if (options & RV_SOCKET_OPT_KEEP_ALIVE)
            {
                /* Make sure the TCP connection sends keep-alive messages once in a while */
                if (RvSocketSetSockOpt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&yes, sizeof(yes)) != 0)
                {
                    RvSockLogError((&logMgr->socketSource,
                        "RvSocketConstructHere: Failed to setsockopt(SO_KEEPALIVE) %d", RvSocketErrNo));
                }
            }

            if (options & RV_SOCKET_OPT_TCP_DELAY)
            {
            /* Disable Nagle algorithm - we want our messages to be sent as soon as possible */
                if (RvSocketSetSockOpt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(yes)) != 0)
                {
                    RvSockLogError((&logMgr->socketSource,
                        "RvSocketConstructHere: Failed to setsockopt(TCP_NODELAY) %d", RvSocketErrNo));
                }
            }
#endif

            if (options & RV_SOCKET_OPT_LINGER)
            {
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_WINCE)
            /* Set linger parameter of the socket. On Windows, we disable it. */
            RvSocketSetLinger(sock, -1, logMgr);
#else
            /* On other operating systems - we set the linger timeout to 0 */
            RvSocketSetLinger(sock, 0, logMgr);
#endif
            }

        }
#if (RV_NET_TYPE & RV_NET_SCTP)
        else if (protocolType == RvSocketProtocolSctp)
        {
            if (options & RV_SOCKET_OPT_TCP_DELAY)
            {
                int yes = RV_TRUE;
                status = RvSocketSctpSockOpt(sock, RvSctpOptionSet, RV_SCTP_NODELAY, &yes, sizeof(yes), logMgr);
            if (status != RV_OK)
                {
                    RvSockLogError((&logMgr->socketSource,
                        "RvSocketConstructHere: Failed to setsockopt(RV_SCTP_NODELAY) %d", RvSocketErrNo));
                }
            }
        }
#endif
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstructHere: Failed to call socket(), errno=%d(%p)", RvSocketErrNo, RvSocketErrNo));
    }

    return status;
}


/*********** Begin of IPv6 ToS (traffic class) support *************/

/* RV_SOCKET_USE_FAKED_IPV6_TCLASS semantic:
 *  if not defined - apply heuristics to define appropriately
 *  if 0 - don't use faked IPV6_TCLASS support
 *  if 1 - use faked support only if IPV6_TCLASS isn't defined
 *  if > 1 - use faked support always
 */

/* If we're not compiling under IPv6 - disable RV_SOCKET_ENABLE_IPV6_TOS
 *  compilation flag
 */
#if ((RV_NET_TYPE & RV_NET_IPV6) == 0)
#  undef RV_SOCKET_ENABLE_IPV6_TOS
#endif

/* If RV_SOCKET_ENABLE_IPV6_TOS wasn't defined - define it as '0',
 *  mostly to prevent warnings on some compilers that complains about
 *  using not-defined macros.
 */
#if !defined(RV_SOCKET_ENABLE_IPV6_TOS)
#  define RV_SOCKET_ENABLE_IPV6_TOS 0
#endif

/* If IPV6_TOS support wasn't enabled, prevent also using faked support for IPV6_TCLASS
 * socket option
 */
#if !RV_SOCKET_ENABLE_IPV6_TOS
#  undef RV_SOCKET_USE_FAKED_IPV6_TCLASS
#  define RV_SOCKET_USE_FAKED_IPV6_TCLASS 0
#elif !defined(RV_SOCKET_USE_FAKED_IPV6_TCLASS)
#  define RV_SOCKET_USE_FAKED_IPV6_TCLASS 1
#endif

/* If IPV6_TCLASS socket option is provided, we probably don't need to fake it,
 * unless we were explicitly asked to (RV_SOCKET_USE_FAKED_IPV6_TCLASS >= 2)
 *
 */
#if defined(IPV6_TCLASS)
#  if RV_SOCKET_USE_FAKED_IPV6_TCLASS <= 1
#    undef  RV_SOCKET_USE_FAKED_IPV6_TCLASS
#    define RV_SOCKET_USE_FAKED_IPV6_TCLASS 0
#  else
#    undef IPV6_TCLASS
#    if defined(__GNUC__)
#      warning "Using RV_SOCKET_USE_FAKED_IPV6_TCLASS option while IPV6_TCLASS is available is strongly discouraged"
#    endif

#  endif
#endif



#if !defined(RV_SOCKET_USE_FAKED_IPV6_TCLASS)
#  if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && !defined(IPV6_TCLASS)
#    define RV_SOCKET_USE_FAKED_IPV6_TCLASS 1
#  else
#    define RV_SOCKET_USE_FAKED_IPV6_TCLASS 0
#  endif
#endif


 /* If we were asked to provide support for IPv6 ToS, but IPV6_TCLASS socket optiont is not
  *  available and RV_SOCKET_USE_FAKED_IPV6_TCLASS wasn't set - issue appropriate warning
  *
  */
#if RV_SOCKET_ENABLE_IPV6_TOS && !defined(IPV6_TCLASS) && !RV_SOCKET_USE_FAKED_IPV6_TCLASS
#  if defined(__GNUC__)
#    warning "IPV6_TCLASS or RV_SOCKET_USE_FAKED_IPV6_TCLASS should be define to support IPV6 ToS"
#  elif defined(_MSC_VER)
#    pragma message(RvWarning "IPV6_TCLASS or RV_SOCKET_USE_FAKED_IPV6_TCLASS should be defined to support IPv6 ToS")
#  endif
#endif


#if RV_SOCKET_USE_FAKED_IPV6_TCLASS

#  if defined(__GNUC__)
#    warning "Uses experimental support for setting TCLASS on IPv6 connections"
#  endif

/* On Linux, OS disregards flowinfo setting, unless IPV6_FLOWINFO_SEND socket option is set on this socket */
#  if RV_OS_TYPE == RV_OS_TYPE_LINUX
#    define RV_IPV6_FLOWINFO_SEND_REQUIRED 1
#  else
#    define RV_IPV6_FLOWINFO_SEND_REQUIRED 0
#  endif


/* On Linux, IPV6_FLOWINFO_SEND socket option is missing from libc include files (<netinet/in.h>)
 * and appears only in kernel files (<linux/in6.h>). Moreover, this include contains also a lot of stuff
 * common with <netinet/in.h>, so directly including it together with later is a problem.
 *
 * So, I choose to define our own RV_IPV6_FLOWINFO_SEND constant as following:
 *  1. If IPV6_FLOWINFO_SEND defined - use it as a value for RV_IPV6_FLOWINFO_SEND
 *  2. Otherwise - use RV_IPV6_FLOWINFO_SEND_DEFAULT that should be defined in rvusrconfig.h
 *     We use '33' as the value for RV_IPV6_FLOWINFO_SEND_DEFAULT. Check your linux/in6.h and set
 *     RV_IPV6_FLOWINFO_SEND_DEFAULT to whatever is appropriate to your system
 */

#  if  RV_IPV6_FLOWINFO_SEND_REQUIRED
#    ifndef RV_IPV6_FLOWINFO_SEND
#      ifndef IPV6_FLOWINFO_SEND
#        define RV_IPV6_FLOWINFO_SEND 33
#        if defined(__GNUC__)
#          warning "IPV6_FLOWINFO_SEND required, but not found in your includes, using '33' instead"
#        endif
#      else
#        define RV_IPV6_FLOWINFO_SEND IPV6_FLOWINFO_SEND
#      endif
#    endif
#  endif


#  ifndef RV_SOCKET_SHADOWS_SIZE
#    define RV_SOCKET_SHADOWS_SIZE 256
#  endif

#  ifndef RV_SOCKET_SHADOWS_MAX
#    define RV_SOCKET_SHADOWS_MAX 8192
#  endif



typedef struct {
    RvUint8 enabled;
    RvUint8 ipv6TrafficClass;
} RvSocketShadow;

typedef struct {
    RvLock lock;
    RvSocketShadow *shadows;
    RvSize_t nShadows;
} RvSocketShadows;

static RvSocketShadow gsInitShadows[RV_SOCKET_SHADOWS_SIZE];
static RvSocketShadows gsShadows;


#define RvSocketToIndex(s) ((RvSize_t)*s)


static
RvStatus rvShadowConstruct(RvSocket *s, RvLogMgr *logMgr) {
#define FUNC "rvConstructShadow:"

    RvStatus st = RV_OK;
    RvSocketShadows *self = &gsShadows;
    RvSize_t n;
    RvSize_t idx = RvSocketToIndex(s);

    st = RvLockGet(&self->lock, logMgr);
    if(st != RV_OK) {
        return st;
    }

    n = self->nShadows;

    if(idx >= n) {
        RvSocketShadow *pShadows = 0;

        while(((n <<= 1) <= idx) && n > 0);
        if(n == 0 || n > RV_SOCKET_SHADOWS_MAX) {
            RvSockLogWarning((SRC_FUNC "Socket descriptor is too high:%d. Increase RV_SOCKET_SHADOWS_MAX (%d)",
                             idx, RV_SOCKET_SHADOWS_MAX));
            st = RV_ERROR_OUTOFRESOURCES;
            goto failure;
        }

        st = RvMemoryAlloc(0, sizeof(*pShadows) * n, logMgr, (void **)&pShadows);
        if(st != RV_OK) goto failure;
        memset(pShadows, 0, n * sizeof(*pShadows));
        memcpy(pShadows, self->shadows, self->nShadows * sizeof(*pShadows));
	if(self->shadows != gsInitShadows) {
	    RvMemoryFree(self->shadows, logMgr);
	}
        self->shadows = pShadows;
        self->nShadows = n;
    }

    self->shadows[idx].enabled = RV_TRUE;
    self->shadows[idx].ipv6TrafficClass = 0;

failure:
    RvLockRelease(&self->lock, logMgr);
    return st;
#undef FUNC
}

static
RvStatus rvShadowDestruct(RvSocket *s, RvLogMgr *logMgr) {
#define FUNC "rvDestructShadow:"

    RvStatus st = RV_OK;
    RvSocketShadows *self = &gsShadows;
    RvSize_t n;
    RvSize_t idx = RvSocketToIndex(s);

    st = RvLockGet(&self->lock, logMgr);
    if(st != RV_OK) {
        return st;
    }

    n = self->nShadows;

    if(idx >= n) {
        RvSockLogError((SRC_FUNC "No shadow for this socket descriptor: %d", idx));
        st = RV_ERROR_BADPARAM;
        goto failure;
    }

    self->shadows[idx].enabled = RV_FALSE;

failure:
    RvLockRelease(&self->lock, logMgr);
    return st;
#undef FUNC
}

/*
 int on = 1;
  int res;
  unsigned int tclass;
  struct sockaddr_in6 raddr;
  int s;
  int i;

  s = socket(PF_INET6, SOCK_DGRAM, 0);
  res = setsockopt(s, IPPROTO_IPV6, IPV6_FLOWINFO_SEND, (void *)&on, 4);
*/

static
RvStatus rvShadowSetTrafficClass(RvSocket *s, RvInt tc, RvLogMgr *logMgr) {
#define FUNC "rvShadowSetTrafficClass: "

    RvStatus st = RV_OK;
    RvSocketShadows *self = &gsShadows;
    RvSocketShadow *pShadow;
    RvSize_t idx = RvSocketToIndex(s);



    st = RvLockGet(&self->lock, logMgr);
    if(st != RV_OK) {
        return st;
    }

    if(idx >= self->nShadows) {
        RvSockLogError((SRC_FUNC "Socket descriptor is too high: %d, check RV_SOCKET_SHADOWS_MAX (%d)",
                        *s, RV_SOCKET_SHADOWS_MAX));
        st = RV_ERROR_BADPARAM;
        goto failure;
    }

    pShadow = &self->shadows[idx];
    if(pShadow->enabled != RV_TRUE) {
        RvSockLogError((SRC_FUNC "Shadow disabled for this socket: %d", *s));
        st = RV_ERROR_BADPARAM;
        goto failure;
    }


    /* set IPV6_FLOWINFO_SEND option on this socket, otherwise Linux disregards setting of flowinfo
     * in sockaddr
     */
#if RV_IPV6_FLOWINFO_SEND_REQUIRED
    {
        int res;
        int on = 1;


        res = RvSocketSetSockOpt(s, SOL_IPV6, RV_IPV6_FLOWINFO_SEND, (void *)&on, sizeof(on));
        if(res != 0) {
            /* If for some reason we failed to set this option, mark shadow as disabled */
            RvSockLogError((SRC_FUNC "Failed to set IPV6_FLOWINFO_SEND option on socket %d", *s));
            pShadow->enabled = RV_FALSE;
            st = RV_ERROR_UNKNOWN;
            goto failure;
	}
    }

#endif

    pShadow->ipv6TrafficClass = (RvUint8)tc;

failure:
    RvLockRelease(&self->lock, logMgr);
    return st;
#undef FUNC
}

static
RvStatus rvShadowGetTrafficClass(RvSocket *s, RvInt *ptc, RvLogMgr *logMgr) {
#define FUNC "rvShadowGetTrafficClass: "

    RvStatus st = RV_OK;
    RvSocketShadows *self = &gsShadows;
    RvSocketShadow *pShadow;
    RvSize_t idx = RvSocketToIndex(s);

    st = RvLockGet(&self->lock, logMgr);
    if(st != RV_OK) {
        return st;
    }

    if(idx >= self->nShadows) {
        RvSockLogError((SRC_FUNC "Socket descriptor is too high: %d, check RV_SOCKET_SHADOWS_MAX (%d)",
                        *s, RV_SOCKET_SHADOWS_MAX));
        st = RV_ERROR_BADPARAM;
        goto failure;
    }

    pShadow = &self->shadows[idx];
    if(pShadow->enabled != RV_TRUE) {
        RvSockLogError((SRC_FUNC "No shadow for this socket descriptor: %d", *s));
        st = RV_ERROR_BADPARAM;
        goto failure;
    }

    *ptc = pShadow->ipv6TrafficClass;

failure:
    RvLockRelease(&self->lock, logMgr);
    return st;
#undef FUNC
}

static
RvStatus rvShadowSetFlowinfo(RvSocket *s, void *sockaddrBuf, RvLogMgr *logMgr) {
    RvStatus st = RV_OK;
    RvInt tc;
    struct sockaddr_in6 *asin6 = (struct sockaddr_in6 *)sockaddrBuf;


    /* It's actually IPv4 address, no flowinfo setting is needed */
    if(asin6->sin6_family != AF_INET6) {
        return RV_OK;
    }

    st = rvShadowGetTrafficClass(s, &tc, logMgr);
    if(st != RV_OK) {
        return st;
    }

    tc = tc << 20;
    tc = htonl(tc);
    asin6->sin6_flowinfo = tc;
    return st;
}

static
RvStatus rvShadowsInit() {
    RvSize_t i;
    RvStatus s = RV_OK;

    s = RvLockConstruct(0, &gsShadows.lock);
    if(s != RV_OK) {
        return s;
    }

    gsShadows.shadows = gsInitShadows;
    i = gsShadows.nShadows = sizeof(gsInitShadows) / sizeof(gsInitShadows[0]);
    while(i) {
        gsInitShadows[--i].enabled = RV_FALSE;
    }

    return s;
}

static
void rvShadowsEnd() {
    RvLockDestruct(&gsShadows.lock, 0);
    if(gsShadows.shadows != gsInitShadows) {
        RvMemoryFree(gsShadows.shadows, 0);
    }
}



#else /* RV_SOCKET_USE_FAKED_IPV6_TCLASS */

#  define rvShadowsInit() (RV_OK)
#  define rvShadowsEnd()
#  define rvShadowConstruct(s, logMgr) ((void)(s), (void)(logMgr), RV_OK)
#  define rvShadowDestruct(s, logMgr) ((void)(s), (void)(logMgr), RV_OK)
#  define rvShadowGetTrafficClass(s, ptc, logMgr) ((void)(s), (void)(logMgr), ((*ptc) = 0), RV_ERROR_NOTSUPPORTED)
#  define rvShadowSetTrafficClass(s, tc, logMgr) ((void)(s), (void)(tc), (void)(logMgr), RV_ERROR_NOTSUPPORTED)
#  define rvShadowSetFlowinfo(s, sockaddrBuf, logMgr) ((void)(s), (void)(sockaddrBuf), (void)(logMgr), RV_OK)


#endif /* RV_SOCKET_USE_FAKED_IPV6_TCLASS */



/********************************************************************************************
 *                               Core - Public functions
 ********************************************************************************************/


#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
/********************************************************************************************
 * RvSocketSockAddrToAddress
 * Convert a sockaddr struct into an RvAddress struct.
 * This function is thread-safe.
 * INPUT:
 *  sockaddr   - Socket address
 *  socklen    - Socket address length
 * OUTPUT:
 *  address    - Address constructed
 * RETURN:
 *  RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSockAddrToAddress(
    IN  RvUint8*    sockaddr,
    IN  int         socklen,
    OUT RvAddress*  address)
{
    RvStatus status = RV_OK;

    RV_UNUSED_ARG(socklen);

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    /* since Nucleus's sockaddr doesn't have family field, casting it to sockaddr_in will leave
       'sin_family' field empty. */
    /* todo: a temporary solution (until nucleus will also support IPv6)... */
    ((struct sockaddr_in *)sockaddr)->sin_family = AF_INET;
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && !(RV_NET_TYPE & RV_NET_IPV6)
    /*  some vxworks BSPs will return a garbled sin_family field, so,
       if we are  not compiled with IPv6, and this is vxworks, the sin_family
       field must be AF_INET */
    ((struct sockaddr_in *)sockaddr)->sin_family = AF_INET;
#endif

    switch (((struct sockaddr_in *)sockaddr)->sin_family)
    {
    case AF_INET:
        {
            struct sockaddr_in* sin = (struct sockaddr_in *)sockaddr;

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
            RvAddressConstructIpv4(address,
                *((RvUint32*)(sin->id.is_ip_addrs)),
                sin->sin_port);
#else
            RvAddressConstructIpv4(address,
                (RvUint32)sin->sin_addr.s_addr,
                (RvUint16)ntohs(sin->sin_port)); /* We use ntohs and not our internal RvConvertNetworkToHost16()
                                          since Tru64 doesn't work properly otherwise */
#endif
        }
        break;

#if (RV_NET_TYPE & RV_NET_IPV6)
    case AF_INET6:
        {
            struct sockaddr_in6* sin = (struct sockaddr_in6*)sockaddr;

            RvAddressConstructIpv6(address,
                (RvUint8*)(sin->sin6_addr.s6_addr), RvConvertNetworkToHost16(sin->sin6_port), (RvUint32)sin->sin6_scope_id);
        }
        break;
#endif  /* RV_NET_TYPE & RV_NET_IPV6 */

    default:
        status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
    }

    return status;
}
#endif

#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
/********************************************************************************************
 * RvSocketAddressToSockAddr
 * Convert an RvAddress struct into a sockaddr struct.
 * This function is thread-safe.
 * INPUT:
 *  address    - Address to convert
 * OUTPUT:
 *  sockaddr   - Socket address constructed
 *  socklen    - Socket address length constructed
 * RETURN:
 *  RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketAddressToSockAddr(
    IN  RvAddress*      address,
    OUT void*           sockaddr,
    OUT int*            socklen)
{
    RvStatus res = RV_OK;

    switch (RvAddressGetType(address))
    {
    case RV_ADDRESS_TYPE_IPV4:
        {
            struct sockaddr_in *sin = (struct sockaddr_in*)sockaddr;
            const RvAddressIpv4* ipv4Addr;

            ipv4Addr = RvAddressGetIpv4(address);
            if(ipv4Addr == 0) {
                return RV_ERROR_BADPARAM;
            }

                memset(sin, 0, sizeof(struct sockaddr_in));
                sin->sin_family = AF_INET;
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
                sin->sin_port = ipv4Addr->port;
                {
                    RvUint32 addr = RvAddressIpv4GetIp(ipv4Addr);
                    memcpy(&(sin->id.is_ip_addrs), &addr, 4);
                }
#else
                /* We use htons and not our internal RvConvertHostToNetwork16() since Tru64
                   doesn't work properly otherwise */
                sin->sin_port = (RvUint16)htons(ipv4Addr->port);
                sin->sin_addr.s_addr = RvAddressIpv4GetIp(ipv4Addr);
#endif
                *socklen = sizeof(struct sockaddr_in);
            }
        break;

#if (RV_NET_TYPE & RV_NET_IPV6)
    case RV_ADDRESS_TYPE_IPV6:
        {
            struct sockaddr_in6 *sin= (struct sockaddr_in6 *)sockaddr;
            const RvAddressIpv6* ipv6Addr;

            ipv6Addr = RvAddressGetIpv6(address);
            if(ipv6Addr == 0) {
                return RV_ERROR_BADPARAM;
            }

            memset((char *)sin, 0, sizeof(sin));
            sin->sin6_family = AF_INET6;

                /* FLOW INFO - this field classify the packet to a certain flow, i.e.
                   priorities the flow of this session. it is a QOS issue, and since no
                   special requirements were given in the IPv6 QOS field, and the RFC does
                   not obligate it, we set it to default value - 0. */
                sin->sin6_flowinfo = 0;
                sin->sin6_port = RvConvertHostToNetwork16(ipv6Addr->port);
                sin->sin6_scope_id = ipv6Addr->scopeId;
                memcpy(&(sin->sin6_addr), RvAddressIpv6GetIp(ipv6Addr), sizeof(sin->sin6_addr));

                *socklen = sizeof(struct sockaddr_in6);
        }
        break;
#endif  /* RV_NET_TYPE & RV_NET_IPV6 */

    default:
        res = RvSocketErrorCode(RV_ERROR_BADPARAM);
        break;
    }

    return res;
}


/********************************************************************************************
 * RvSocketSockAddrSetPort
 * Set the port inside a sockaddr struct to a specified port value
 * This function is thread-safe.
 * INPUT:
 *  sockaddr   - Socket address to modify
 *  port       - Port to set in address
 * RETURN:
 *  RV_OK on success, other on failure
 */
static RvStatus RvSocketSockAddrSetPort(
    IN RV_SOCKADDR_PTR  sockaddr,
    IN RvUint           port)
{
    RvStatus status = RV_OK;

    switch (sockaddr->sa_family)
    {
    case AF_INET:
        {
            RvUint16 port16 = (RvUint16)port;
            struct sockaddr_in* sin = (struct sockaddr_in *)sockaddr;
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
            sin->sin_port = port16;
#else
            sin->sin_port = RvConvertHostToNetwork16(port16);
#endif
        }
        break;
#if (RV_NET_TYPE & RV_NET_IPV6)
    case AF_INET6:
        {
            RvUint16 port16 = (RvUint16)port;
            struct sockaddr_in6 *sin= (struct sockaddr_in6 *)sockaddr;
            sin->sin6_port= RvConvertHostToNetwork16(port16);
        }
        break;
#endif
    default:
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    return status;
}

#endif /* RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN */

#endif /* (RV_NET_TYPE != RV_NET_NONE) */


/********************************************************************************************
 * RvSocketInit
 * Open the Socket Module.
 * INPUT   : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSocketInit(void)
{
    RvStatus status = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    RvUint16 endianTest = 1;
    RvUint8* p = (RvUint8*)&endianTest;

    /* Make sure we compiled this with the right endian */
#if (RV_ARCH_ENDIAN == RV_ARCH_LITTLE_ENDIAN)
    if (p[0] != 1)
        return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#elif (RV_ARCH_ENDIAN == RV_ARCH_BIG_ENDIAN)
    if (p[1] != 1)
        return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif

#if (RV_SOCKET_USE_SHARER == RV_YES)
    status = RvSocketSharerInit();
    if (status != RV_OK)
        return status;
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && (RV_OS_VERSION > RV_OS_VXWORKS_2_2) && defined(AF_INET6)

    {
	/* we want to make sure we are compatible with the OS image *
     * that is we want to catch the situation when the image
     * was compiled with INET6 and we were not or vice-versa */
        RvSocket sock;
        sock = socket(AF_INET6,SOCK_DGRAM,0);
#ifdef INET6
        if (sock < 0)
        {
            printf("ERROR: INET6 incompatibility between RADVISION stack and OS image\n.The RADVISION stack was compiled with -DINET6 but the OS image does not support IPv6\n");
            return RV_ERROR_UNKNOWN;
        }
        else
        	close(sock);
#else
        if (sock >= 0)
        {
			close(sock);
            printf("ERROR: INET6 incompatibility between RADVISION stack and OS image\n.The RADVISION stack was compiled without -DINET6 but the OS image was compiled with -DINET6\n");
            return RV_ERROR_UNKNOWN;
        }
#endif

    }
#endif /*(RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && (RV_OS_VERSION > RV_OS_VXWORKS_2_2)*/

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)

#  if !HAS_BINDPORT0

    /* Create the default ephemeral port range for NUCLEUS */
    status = RvPortRangeConstruct(RV_PORTRANGE_DEFAULT_START, RV_PORTRANGE_DEFAULT_FINISH, NULL, &RvSocketPortRange);
    if (status != RV_OK)
    {
        RvSocketSharerEnd();
        return status;
    }
#  else
#   define RvPortRangeDestruct(range)
#  endif

#  if !HAS_NONBLOCKEDCONNECT

    status = RvTimerQueueConstruct(RV_TIMER_QTYPE_FIXED, MAX_TIMERS_PER_THREAD, 0, 0, 0, 40, NULL, NULL, NULL, &timerQueue);
    if (status != RV_OK)
    {
        RvPortRangeDestruct(&RvSocketPortRange);
        RvSocketSharerEnd();
        return status;
    }

    status = RvMemoryAlloc(NULL, (RvSize_t)(sizeof(connectionInfo)*RvSelectGetMaxFileDescriptors()), NULL, (void **)&nuConInfo);
    if (status != RV_OK)
    {
        RvTimerQueueDestruct(&timerQueue);
        RvPortRangeDestruct(&RvSocketPortRange);
        RvSocketSharerEnd();
        return status;
    }

    /* zero the sockets dataBase */
    memset(nuConInfo, 0, (RvSize_t)(sizeof(connectionInfo) * RvSelectGetMaxFileDescriptors()));

    status = RvLockConstruct(NULL, &nuLock);
    if (status != RV_OK)
    {
        RvMemoryFree(nuConInfo, NULL);
        RvTimerQueueDestruct(&timerQueue);
        RvPortRangeDestruct(&RvSocketPortRange);
        RvSocketSharerEnd();
        return status;
    }

#if (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)
    status = RvTimerEngineConstruct(&tengine, &timerQueue,
        RV_TIME64_NSECPERMSEC * RvInt64Const(1,0,100));
    if (status != RV_OK)
    {
        RvLockDestruct(&nuLock, NULL);
        RvMemoryFree(nuConInfo, NULL);
        RvTimerQueueDestruct(&timerQueue);
        RvPortRangeDestruct(&RvSocketPortRange);
        RvSocketSharerEnd();
        return status;
    }

    RvTimerEngineSetOptions(&tengine, "RvNuTimers", NULL, 0, RV_THREAD_PRIORITY_DEFAULT, NULL);
    status = RvTimerEngineStart(&tengine);
    if(status != RV_OK)
    {
        RvTimerEngineDestruct(&tengine);
        RvLockDestruct(&nuLock, NULL);
        RvMemoryFree(nuConInfo, NULL);
        RvTimerQueueDestruct(&timerQueue);
        RvPortRangeDestruct(&RvSocketPortRange);
        RvSocketSharerEnd();
        return status;
    }
#endif
#  endif /* HAS_NONBLOCKEDCONNECT */

#elif (RV_SOCKET_TYPE == RV_SOCKET_BSD)

#if (RV_OS_TYPE != RV_OS_TYPE_WINCE) && (RV_OS_TYPE != RV_OS_TYPE_OSE) && \
    (RV_OS_TYPE != RV_OS_TYPE_MOPI)
    /* Catch SIGPIPE signals and ignore them instead of core-dumping */
    if (signal(SIGPIPE, RvSocketHandleSigPipe) == SIG_ERR)
    {
        status = RvSocketSharerEnd();
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        return status;
    }
#endif

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN) /* (RV_SOCKET_TYPE == RV_SOCKET_BSD) */
    if (RvSymSocketInit() != RV_OK)
        return RV_ERROR_UNKNOWN;
#endif /* (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN) */

#if RV_SOCKET_USE_SHADOWS
    status = RvSocketShadowInit();
#endif
    CheckTosSupport();

    status = rvShadowsInit();
    if(status != RV_OK) {
        return status;
    }

#endif /* (RV_NET_TYPE != RV_NET_NONE) */

    return status;
}


/********************************************************************************************
 * RvSocketEnd
 * Close the Socket module.
 * INPUT   : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSocketEnd(void)
{
    RvStatus status = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    status = RvSocketSharerEnd();

#  if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)

#    if !HAS_NONBLOCKEDCONNECT

#      if (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)
         RvTimerEngineDestruct(&tengine);
#      endif

      RvLockDestruct(&nuLock, NULL);

      RvMemoryFree(nuConInfo, NULL);

      RvTimerQueueDestruct(&timerQueue);
#   endif /* HAS_NONBLOCKEDCONNECT */

#   if !HAS_BINDPORT0
      RvPortRangeDestruct(&RvSocketPortRange);
#   endif


#  endif

#  if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    RvSymSocketEnd();
#  endif

    rvShadowsEnd();

#endif
    (void)RvWinDscpEnd();
    RvSocketShadowEnd();

    return status;
}

/********************************************************************************************
 * RvSocketSetUDPMaxLength
 * Set For Symbian The UDP Maximum Send And receive Buffer Length
 * INPUT   : Length Of The Buffers
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI void RvSocketSetUDPMaxLength(RvInt numLenght)
{
    RV_UNUSED_ARG(numLenght);
    return;
}
/********************************************************************************************
 * RvSocketSourceConstruct
 * Create a log source for the socket module.
 * INPUT   : logMgr - log manager associated with the log source.
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSocketSourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus result = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->socketSource, "SOCKET", "Sockets interface");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}



#if (RV_NET_TYPE != RV_NET_NONE)

        /* ================================================================ */
        /* ==== General functions (used by both TCP and UDP protocols) ==== */
        /* ================================================================ */


/********************************************************************************************
 * RvSocketConstruct
 * Construct a socket object. During construction time, the socket's type of address
 * and protocol must be supplied.
 * A call to RvSocketSetBlocking() must follow the call to this function to set
 * this socket as a blocking or a non-blocking socket.
 * This function is thread-safe.
 * INPUT   : addressType    - Address type of the created socket.
 *           protocolType   - The type of protocol to use (TCP, UDP, etc).
 *           logMgr         - log manager instance
 * OUTPUT:   sock           - Socket to construct
 * RETURN  : RV_OK on success, other on failure
 */
#if (RV_OS_TYPE != RV_OS_TYPE_MOPI)
RVCOREAPI
RvStatus RVCALLCONV RvSocketConstruct(
    IN  RvInt             addressType,
    IN  RvSocketProtocol  protocolType,
    IN  RvLogMgr*         logMgr,
    OUT RvSocket*         sock)
#else
RVCOREAPI
RvStatus RVCALLCONV RvSocketConstructWithCid(
    IN  RvInt             addressType,
    IN  RvSocketProtocol  protocolType,
    IN  RvLogMgr*         logMgr,
    IN  RvUint16     	  cid,
    OUT RvSocket*         sock)
#endif
{
    RvStatus status;

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketConstruct(sock=%p,addressType=%d,protocolType=%d)",
        sock, addressType, protocolType));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstruct(type=%d,protocol=%d, errno=%d)=%d",
            addressType, protocolType, RvSocketErrNo, status));
        return status;
    }
#endif

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if (protocolType >= RvSocketProtocolEND)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstruct(type=%d,protocol=%d, errno=%d)=%d",
            addressType, protocolType, RvSocketErrNo, status));
        return status;
    }
#endif

#if (RV_SOCKET_USE_SHARER == RV_YES)
    status = RvSocketSharerConstruct(logMgr, sock, addressType, protocolType,-1);

#elif (RV_OS_TYPE == RV_OS_TYPE_MOPI)
    status = RvSocketConstructHere(sock, addressType, protocolType, cid, logMgr,-1);

#else
    status = RvSocketConstructHere(sock, addressType, protocolType, logMgr,-1);

#endif

#if RV_SOCKET_USE_FAKED_IPV6_TCLASS
    /* Currently, we use shadows only to support setting Traffic class for IPv6 sockets
     *  on systems that doesn't support IPV6_TCLASS socket option, for example - various Linux systems,
     *  except those Linux that have USAGI patch installed
     */
    if(addressType == RV_ADDRESS_TYPE_IPV6) {
        rvShadowConstruct(sock, logMgr);
    }

#endif

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketConstruct(socket=%d,type=%d,protocol=%d)=%d",
            *sock, addressType, protocolType, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstruct(socket=%d,type=%d,protocol=%d, errno=%d)=%d",
            *sock, addressType, protocolType, RvSocketErrNo, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketConstruct2
 * Construct a socket object. During construction time, the socket's type of address
 * and protocol must be supplied.
 * A call to RvSocketSetBlocking() must follow the call to this function to set
 * this socket as a blocking or a non-blocking socket.
 * This function is thread-safe.
 * INPUT   : addressType    - Address type of the created socket.
 *           protocolType   - The type of protocol to use (TCP, UDP, etc).
 *           options        - Combination of socket options like RV_SOCKET_OPT_KEEP_ALIVE,
 *                            RV_SOCKET_OPT_TCP_DELAY etc.
 *                            If set to -1 the default options will be set. The deafult options are:
 *                              RV_SOCKET_OPT_KEEP_ALIVE,RV_SOCKET_OPT_TCP_DELAY and RV_SOCKET_OPT_LINGER.
 *                             That is when called with options set to -1 the result is identical
 *                             to calling RvSocketConstruct function.
 *           logMgr         - log manager instance
 * OUTPUT:   sock           - Socket to construct
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketConstruct2(
    IN  RvInt             addressType,
    IN  RvSocketProtocol  protocolType,
    IN  RvInt             options,
    IN  RvLogMgr*         logMgr,
    OUT RvSocket*         sock)
{
    RvStatus status;

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketConstruct2(sock=%p,addressType=%d,protocolType=%d,options=%d)",
        sock, addressType, protocolType,options));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstruct2(type=%d,protocol=%d, errno=%d,options=%d)=%d",
            addressType, protocolType, options,RvSocketErrNo, status));
        return status;
    }
#endif

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if (protocolType >= RvSocketProtocolEND)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstruct2(type=%d,protocol=%d, errno=%d,options=%d)=%d",
            addressType, protocolType, options,RvSocketErrNo, status));
        return status;
    }
#endif

#if (RV_SOCKET_USE_SHARER == RV_YES)
    status = RvSocketSharerConstruct(logMgr, sock, addressType, protocolType,options);

#elif (RV_OS_TYPE == RV_OS_TYPE_MOPI)
    status = RvSocketConstructHere(sock, addressType, protocolType, cid, logMgr,options);

#else
    status = RvSocketConstructHere(sock, addressType, protocolType, logMgr,options);

#endif

#if RV_SOCKET_USE_FAKED_IPV6_TCLASS
    /* Currently, we use shadows only to support setting Traffic class for IPv6 sockets
     *  on systems that doesn't support IPV6_TCLASS socket option, for example - various Linux systems,
     *  except those Linux that have USAGI patch installed
     */
    if(addressType == RV_ADDRESS_TYPE_IPV6) {
        rvShadowConstruct(sock, logMgr);
    }

#endif

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketConstruct(socket=%d,type=%d,protocol=%d)=%d",
            *sock, addressType, protocolType, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConstruct(socket=%d,type=%d,protocol=%d, errno=%d)=%d",
            *sock, addressType, protocolType, RvSocketErrNo, status));
    }

    return status;
}



/********************************************************************************************
 * RvSocketShutdown
 * Shutdown a TCP socket. This function should be called before calling
 * RvSocketDestruct() for TCP sockets.
 * In blocking mode, this function blocks until RvSocketDestruct() can be called in
 * a graceful manner to close the connection.
 * In non-blocking mode, when the remote side will close its connection, this socket
 * will receive the RvSocketClose event in the select module and RvSocketDestruct()
 * should be called at that point.
 * This function needs to be ported only if TCP is used by the stack.
 * This function is NOT thread-safe.
 * INPUT   : socket         - Socket to shutdown
 *           cleanSocket    - RV_TRUE if you want to clean the socket before shutting it down.
 *                            this will try to receive from the socket some buffers.
 *                            It is suggested to set this value to RV_TRUE for non-blocking
 *                            sockets.
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketShutdown(
    IN RvSocket*    sock,
    IN RvBool       cleanSocket,
    IN RvLogMgr*    logMgr)
{
    RvSocket ourSocket;
    RvStatus status;
    RvInt errorNum = 0;

    (void)errorNum;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketShutdown(socket=NULL,clean=%d, errno=%d)=%d",
            cleanSocket, RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketShutdown(socket=%d,cleanSocket=%d)", *sock, cleanSocket));

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((cleanSocket != RV_TRUE) && (cleanSocket != RV_FALSE))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d, errno=%d)=%d",
            *sock, cleanSocket, RvSocketErrNo, status));
        return status;
    }
#endif

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d, errno=%d)=%d",
            *sock, cleanSocket, RvSocketErrNo, status));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)

    /* In Nucleus, select() returns an error on an unconnected-socket that was
       set to wait on an event. so in case we are on TCP, and the peer has disconnected,
       and the user has decided to shutdown this socket - avoid this error, and just
       let him destruct it later. no need to shut it down. */
    if (NU_Is_Connected(*sock) != RV_TRUE)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d)=%d", *sock, cleanSocket, status));
        return RV_OK;
    }

#  if HAS_NONBLOCKEDCONNECT
    {
        STATUS nus;

        nus = NU_Shutdown(*sock, SHUT_WR);
        if(nus != OS_SUCCESS) {
            RvSockLogError((&logMgr->socketSource, "RvSocketShutdown(socket=%d) failed with %d", nus));
            return RvOsError(nus);
        }

        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d)=%d", *sock, cleanSocket, status));

        return RV_OK;
    }


#  else

    RV_SOCKET_LOCK();

    /* check socket state */
    if (nuConInfo[ourSocket].socketState == RvSocketState_Connecting)
        NU_Abort(ourSocket);

    /* check thread state */
    if (nuConInfo[ourSocket].threadState == RvThreadState_Stopped)
    {
        RV_SOCKET_UNLOCK();
        RvSocketNucleusTaskClean(&ourSocket);
        RV_SOCKET_LOCK();
    }

    /* clean timer if such exists */
    if (nuConInfo[ourSocket].timerInitiated)
    {
        RvTimerCancel(&(nuConInfo[ourSocket].timer), RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        nuConInfo[ourSocket].timerInitiated = RV_FALSE;
    }

    /* Set timer for expiration of close() command - 1 second */
    status = RvTimerStart(&(nuConInfo[ourSocket].timer), &timerQueue,
                 RV_TIMER_TYPE_ONESHOT, RV_TIME64_NSECPERSEC /* 1 second */,
                 RvSocketNucleusEvTimerExp, (void*)ourSocket);
    if (status != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d, errno=%d)=%d",
            *sock, cleanSocket, RvSocketErrNo, status));

        RV_SOCKET_UNLOCK();

        return status;
    }

    nuConInfo[ourSocket].timerInitiated = RV_TRUE;

    if (nuConInfo[ourSocket].threadState == RvThreadState_Idle)
    {
        /* Since the close() call on nucleus is blocking, we construct a thread
           for closing this socket */
        status = RvThreadConstruct(RvSocketNucleusEventsTaskClose, (void*)ourSocket, logMgr, &(nuConInfo[ourSocket].threadId));
        status = RvThreadSetName(&(nuConInfo[ourSocket].threadId), "RvNuShutdown");
        status = RvThreadSetPriority(&(nuConInfo[ourSocket].threadId), RV_THREAD_SOCKET_PRIORITY_DEFAULT);

        nuConInfo[ourSocket].socketState = RvSocketState_Closing;

        status = RvThreadCreate(&(nuConInfo[ourSocket].threadId));
        if (status == RV_OK)
            status = RvThreadStart(&(nuConInfo[ourSocket].threadId));

        if (status != RV_OK)
            nuConInfo[ourSocket].socketState = RvSocketState_Idle;
    }

    RV_SOCKET_UNLOCK();

    /* enable task-switching for our new thread */
    RvThreadNanosleep(RV_TIME64_NSECPERMSEC * 10, logMgr);

#  endif /* HAS_NONBLOCKEDCONNECT */

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
		RV_UNUSED_ARG(errorNum);
    status = RvSymSockShutdown(sock,cleanSocket);

#else
    if (shutdown(ourSocket, SD_SEND) < 0)
    {
        /* If the error is the fact that we're not connected anymore, we should treat it as a successful
           call to this function */
        errorNum = RvSocketErrNo;
#if (RV_OS_TYPE != RV_OS_TYPE_MOPI) /* MOPI doesn't provide this error */
        if (errorNum != ENOTCONN)
#endif
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    RvSocketSharerClose(&ourSocket);

    if (cleanSocket)
    {
        RvSocketClean(sock,logMgr);
    }

#endif  /* (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS) */

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d)=%d", *sock, cleanSocket, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketShutdown(socket=%d,clean=%d, errno=%d)=%d",
            *sock, cleanSocket, errorNum, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketClean
 *
 * Read & drop all waiting on the socket data.
 *
 * INPUT   : sock           - Socket to shutdown
 *           logMgr         - log manager pointer
 * OUTPUT  : None
 * RETURN  : none
 */
RVCOREAPI
void RVCALLCONV RvSocketClean(
    IN RvSocket*    sock,
    IN RvLogMgr*    logMgr)
{
    RvUint8 tmpBuffer[128];
    RvInt safetyCounter = 1000;
    RvSize_t bytesReceived = 1;
    RvStatus res = RV_OK;

    /* Clean the socket as much as we can */
    while ((safetyCounter > 0) && (res == RV_OK) && (bytesReceived > 0))
    {
        res = RvSocketReceiveBuffer(sock, tmpBuffer, sizeof(tmpBuffer), logMgr, &bytesReceived, NULL);
        safetyCounter--;
    }
}

/********************************************************************************************
 * RvSocketDestruct
 * Close a socket.
 * This function is NOT thread-safe.
 * INPUT   : sock           - Socket to shutdown
 *           cleanSocket    - RV_TRUE if you want to clean the socket before shutting it down.
 *                            this will try to receive from the socket some buffers.
 *                            It is suggested to set this value to RV_TRUE for TCP sockets.
 *                            It should be set to RV_FALSE for UDP sockets.
 *           portRange      - Port range to return this socket's port to. If NULL, the
 *                            socket's port will not be added to any port range object.
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketDestruct(
    IN RvSocket*    sock,
    IN RvBool       cleanSocket,
    IN RvPortRange* portRange,
    IN RvLogMgr*    logMgr)
{
    RvStatus res = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        res = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketDestruct(socket=NULL,clean=%d,range=%p)=%d",
            cleanSocket, portRange, res));
        return res;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketDestruct(socket=%d,clean=%d,range=%p)", *sock, cleanSocket, portRange));

    RvSocketShadowRemoveAll(sock);
    (void)RvWinDscpRemoveSocket(*sock, logMgr);
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((cleanSocket != RV_TRUE) && (cleanSocket != RV_FALSE))
    {
        res = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketDestruct(socket=%d,clean=%d,range=%p)=%d",
            *sock, cleanSocket, portRange, res));
        return res;
    }
#endif

#if RV_SOCKET_USE_FAKED_IPV6_TCLASS
    rvShadowDestruct(sock, logMgr);
#endif

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS && !HAS_BINDPORT0)
    /* In NUCLEUS, we make sure to have a portRange. If none is supplied, we just use
       the port range specified when bind was called. */
    if (portRange == NULL)
        portRange = nuConInfo[*sock].portRange;
#endif

    if (portRange != NULL)
    {
        /* We should try and return this socket's port to the port-range object */
        RvUint16 port;
        res = RvSocketErrorCode(RV_ERROR_UNKNOWN);

#if (RV_OS_TYPE != RV_OS_TYPE_NUCLEUS || HAS_NONBLOCKEDCONNECT)
        {
            RvAddress localAddress;

            if (RvSocketGetLocalAddress(sock, logMgr, &localAddress) == RV_OK)
            {
                port = RvAddressGetIpPort(&localAddress);
            }
            else
            {
#if ((RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && !defined(IPPROTO_IPV6))
                struct socket *so;             /* only when IPv6 stack is not installed */
                so = (struct socket *) iosFdValue (*sock);
                if (so != (struct socket *)ERROR)
                    port = so->so_userArg;
                else
#endif
                port = 0;
            }
        }
#else  /* RV_OS_TYPE_NUCLEUS */

        port = nuConInfo[*sock].localPort;

        if (port == 0)
            /* we can only reach this point if a user has constructed a socket, but has
               decided not to bind, connect or accept it, just destruct it....
               in which case no local port has been assigned to it yet */
            res = RV_OK;
#endif

        if (port > 0)
        {
            RvUint fromPort, toPort;

            /* Make sure this one is within the range we're dealing with */
            res = RvPortRangeGetRange(portRange, &fromPort, &toPort);
            if (res == RV_OK)
            {
                /* Add this port to list of available ports */
                res = RvPortRangeReleasePort(portRange, (RvUint)port);
            } else
            {
                RvSockLogWarning((&logMgr->socketSource, "RvSocketDestruct: inconsistent port range (%d..%d) for port %d", fromPort, toPort, port));
            }
        }
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS && !HAS_NONBLOCKEDCONNECT)

    RV_SOCKET_LOCK();

    if (nuConInfo[*sock].threadState == RvThreadState_Stopped)
    {
        RV_SOCKET_UNLOCK();
        RvSocketNucleusTaskClean(sock);
        RV_SOCKET_LOCK();
    }

    if (nuConInfo[*sock].socketState != RvSocketState_Closed)
    {
        CLOSE_SOCKET(*sock);
        /* we don't really have to update the socket state, since we're going to zero
          it anyway later, and the lock is ours here... */
    }
    if (nuConInfo[*sock].timerInitiated)
        RvTimerCancel(&(nuConInfo[*sock].timer), RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);

    memset(&nuConInfo[*sock], 0, sizeof(connectionInfo));

    RV_SOCKET_UNLOCK();

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)

    res = RvSymSockDestruct(sock,cleanSocket,portRange,logMgr);


#else

    if (cleanSocket)
    {
        RvSocketClean(sock,logMgr);
    }

#if (RV_SOCKET_USE_SHARER == RV_YES)
    RvSocketSharerDestruct(sock);

#else
    if (CLOSE_SOCKET(*sock) < 0)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketDestruct: Error closing socket=%d (%d)", *sock, RvSocketErrNo));
        res = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#endif


#endif

    if (res == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketDestruct(socket=%d,clean=%d,range=%p)=%d",
            *sock, cleanSocket, portRange, res));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketDestruct: Error closing socket=%d (%d)", *sock, RvSocketErrNo));
        res = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    return res;
}


/********************************************************************************************
 * RvSocketBind
 * Bind a socket to a local address
 * This function is NOT thread-safe.
 * INPUT   : sock       - Socket to bind
 *           address    - Address to bind socket to
 *           portRange  - Port range to use if address states an ANY port.
 *                        NULL if this parameter should be ignored.
 *           logMgr         - log manager instance
 * RETURN  : RV_OK on success, other on failure.
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketBind(
    IN RvSocket*         sock,
    IN RvAddress*        address,
    IN RvPortRange*      portRange,
    IN RvLogMgr*         logMgr)
{
    RvStatus status;
    RvSocket ourSocket = 0;
    RvInt socklen = 0;
    RvUint port = 0;
    RvBool usePortRange = RV_FALSE;

    RV_UNUSED_ARG(ourSocket);
    RV_UNUSED_ARG(usePortRange);

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (address == NULL))
    {
		status = RvSocketErrorCode(RV_ERROR_BADPARAM);
		RvSockLogError((&logMgr->socketSource,"RvSocketBind: required parameter is NULL"));
        return status;
    }
#endif

    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogEnter((&logMgr->socketSource,
            "RvSocketBind(socket=%d,address=%s:%d,scopeId=%d,range=%p)",
            *sock, addr, RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address), portRange));
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    RV_UNUSED_ARG(socklen);
    RV_UNUSED_ARG(port);
    status = RvSymSockBind(sock,address,portRange,logMgr);
#else
    {
    rvASockAddr u;
    port = RvAddressGetIpPort(address);

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS && !HAS_BINDPORT0)
    /* In NUCLEUS, we make sure to have a portRange. If none is supplied, we just use
       the static one declared and used in this module. */
    if (port != RV_ADDRESS_IPV4_ANYPORT)
        portRange = NULL;  /* don't allow port-range when an explicit port is specified */
    else if (portRange == NULL)
        portRange = &RvSocketPortRange;
#endif

#if (RV_NET_TYPE & RV_NET_IPV6) && defined(IPV6_V6ONLY)
	if (address->addrtype == RV_ADDRESS_TYPE_IPV6)
	{
#if (RV_SOCKET_IPV6_BIND_TO_IPV6_ONLY==RV_YES)
		RvInt32 val = 1;
#else
		RvInt32 val = 0;
#endif
		if (RvSocketSetSockOpt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&val, sizeof(val)) != 0)
		{
			RvSockLogWarning((&logMgr->socketSource,
				"RvSocketBind(sock=%d): failed to switch %s IPV6_V6ONLY option",
				*sock, (val==1?"on":"off")));
		}
	}
#endif /* #if (RV_NET_TYPE & RV_NET_IPV6) && defined(IPV6_V6ONLY) */

    usePortRange = (portRange != NULL && port == RV_ADDRESS_IPV4_ANYPORT);  /* same for IPV6... */

    status = RvSocketAddressToSockAddr(address, RVSOCKADDR(u), &socklen);
    if (status == RV_OK)
        status = RvSocketSharerShare(logMgr, sock, &ourSocket);

    if (status == RV_OK)
    {
        if (usePortRange)
        {
            RvUint numberOfTries;

            status = RvPortRangeGetNumberOfFreePorts(portRange, &numberOfTries);
            if (numberOfTries == 0)
                status = RvSocketErrorCode(RV_ERROR_OUTOFRESOURCES);

            /* Try until we went through all available ports or until we find a vacant port */
            while (status == RV_OK)
            {
                /* Get a port */
                status = RvPortRangeGetPort(portRange, &port);
                if (status != RV_OK)
                    break;

                /* Fix port in address struct */
                status = RvSocketSockAddrSetPort(RVSOCKADDR(u), port);
                if (status != RV_OK)
                    break;

                /* Try to bind */
                if ((BIND_SOCKET(ourSocket, RVSOCKADDR(u), socklen)) >= 0)
                    break; /* We found a good one - exit the while loop */

                /* Check the error code to see if we want to resume our check */
#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA) || (RV_OS_TYPE == RV_OS_TYPE_WINCE)
                switch (WSAGetLastError())
                {
                case WSAEACCES:
                case WSAEADDRINUSE:
                case WSAEADDRNOTAVAIL:
                    /* We assume that address is in use even if it's unavailable or we have an access violation */
                    break;
                default:
                    status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
                    break;
                }

#elif (RV_SOCKET_TYPE == RV_SOCKET_BSD) || (RV_SOCKET_TYPE == RV_SOCKET_PSOS)
                switch (RvSocketErrNo)
                {
                case EADDRINUSE:
                case EADDRNOTAVAIL:
#ifdef EACCES
                case EACCES: /* Not supported in WinCE or PSOS */
#endif
                    /* We assume that address is in use even if it's unavailable or we have an access violation */
                    break;
                default:
                    status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
                    break;
                }
#endif

                /* No good - this port is taken by someone other than this port range. Release
                   this port back to pool of free ports - it might get free next time. */
                RvPortRangeReleasePort(portRange, port);
                numberOfTries--;
                if (numberOfTries == 0)
                    status = RvSocketErrorCode(RV_ERROR_OUTOFRESOURCES);
            }
        }
        else
        {   /* Just bind and get it over with */
            if ((BIND_SOCKET(ourSocket, RVSOCKADDR(u), socklen)) < 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }

#if ((RV_OS_TYPE == RV_OS_TYPE_VXWORKS) && !defined(IPPROTO_IPV6))
        {                                  /* only when IPv6 stack is not installed */
            struct socket *so;
            so = (struct socket *) iosFdValue (ourSocket);
            if (so != (struct socket *)ERROR)
                so->so_userArg = port;
        }

#elif (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS && !HAS_BINDPORT0)
        if (status == RV_OK)
        {   /* set local port that was binded, in the nucleus database */
            RvAddress localAddress;
            RV_SOCKET_LOCK();
            if (RvSocketGetLocalAddress(sock, logMgr, &localAddress) == RV_OK)
                nuConInfo[ourSocket].localPort = RvAddressGetIpPort(&localAddress);
            nuConInfo[ourSocket].portRange = portRange;
            RV_SOCKET_UNLOCK();
        }
#endif
        RvSocketSharerClose(&ourSocket);
    }
    }
#endif /* #if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN) */

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddress boundAddr;
        RvSocketGetLocalAddress(sock, logMgr, &boundAddr);
        RvAddressGetString(&boundAddr, sizeof(addr), addr);
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketBind(sock=%d,addr=%s:%d,scopeId=%d,range=%p,useRange=%d)=0",
            *sock, addr, RvAddressGetIpPort(&boundAddr), RvAddressGetIpv6Scope(&boundAddr),
            portRange, usePortRange));
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
    if (status != RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketBind(sock=%d,addr=%s:%d,scopeId=%d,range=%p,useRange=%d,errno=%d)=%d",
            *sock, addr, RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address),
            portRange, usePortRange, RvSocketErrNo, status));
    }
#endif

    return status;
}



/* =========================== */
/* ==== functions for TCP ==== */
/* =========================== */

/********************************************************************************************
 * RvSocketConnect
 * Starts a connection between the specified socket and the specified destination.
 * The destination must be running the RvSocketListen() function to receive the
 * incoming connection.
 * In blocking mode, this function returns only when the socket got connected, or
 * on a timeout with a failure return value.
 * In non-blocking mode, this function returns immediately, and when the remote side
 * accepts this connection, this socket will receive the RvSelectConnect event in the
 * select module.
 * This function needs to be ported only if TCP is used by the stack.
 * This function is thread-safe.
 * INPUT   : socket     - Socket to connect
 *           logMgr     - log manager instance
 * OUTPUT  : address    - Remote address to connect this socket to
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketConnect(
        IN RvSocket*    sock,
        IN RvAddress*   address,
        IN RvLogMgr*    logMgr)
{
    RvSocket ourSocket;
    RvStatus status;
    int socklen = 0;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (address == NULL))
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,addr=%s:%d,scopeId=%d,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), addr, RvAddressGetIpPort(address),
            RvAddressGetIpv6Scope(address), RvSocketErrNo, status));
        return status;
    }
#endif

    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogEnter((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,address=%s:%d,scopeId=%d)", *sock, addr,
            RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address)));
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    RV_UNUSED_ARG(socklen);
        status = RvSymSockConnect(sock,address,logMgr);
    ourSocket = *sock;
#else
    {
    rvASockAddr u;
    status = RvSocketAddressToSockAddr(address,  RVSOCKDATA(u), &socklen);
    if (status != RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,addr=%s:%d,scopeId=%d,errno=%d)=%d",
            *sock, addr, RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address), RvSocketErrNo, status));
        return status;
    }

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,addr=%s:%d,scopeId=%d,errno=%d)=%d",
            *sock, addr, RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address), RvSocketErrNo, status));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD) || (RV_SOCKET_TYPE == RV_SOCKET_PSOS)

    if (connect(ourSocket, RVSOCKADDR(u), socklen) != 0)
    {
        /* Make sure the error is not due to the non-blocking nature of this socket */
#if (RV_OS_TYPE == RV_OS_TYPE_WINCE)
        /* Would-Block error in WinCE is just fine with us... (same as Win32) */
			int wsaError = RvSocketErrNo;
        if (wsaError != WSAEWOULDBLOCK)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#else
        /* Any BSD implementation other than WinCE checks for in-progress */
        if (RvSocketErrNo != EINPROGRESS)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#endif
    }

#elif (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)

#  if HAS_NONBLOCKEDCONNECT
    {
        STATUS nus = NU_Connect(ourSocket, RVSOCKADDR(u), socklen);
        if(nus != NU_IS_CONNECTING) {
            status = RvOsError(nus);
        }
    }
#  else

#if (RV_CHECK_MASK & RV_CHECK_OTHER)
    /* Make sure the user called RvSelectAdd() before calling RvSocketConnect... */
    if (nuConInfo[ourSocket].selectEngine == NULL)
    {
        RvSockLogExcep((&logMgr->socketSource,
            "RvSocketConnect(sock=%d): Called before RvSelectAdd()!", *sock));
        return RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

#endif
    RV_SOCKET_LOCK();

    /* clean timer if such exists */
    if (nuConInfo[ourSocket].timerInitiated)
    {
        RvTimerCancel(&(nuConInfo[ourSocket].timer), RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        nuConInfo[ourSocket].timerInitiated = RV_FALSE;
    }

    /* Set timer to 45s connection time */
    status = RvTimerStart(&(nuConInfo[ourSocket].timer), &timerQueue,
        RV_TIMER_TYPE_ONESHOT, (45*RV_TIME64_NSECPERSEC) /* 45 seconds */,
        RvSocketNucleusEvTimerExp, (void*)ourSocket);
    if (status != RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,addr=%s:%d,scopeId=%d,errno=%d)=%d",
            *sock, addr, RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address), RvSocketErrNo, status));

        RV_SOCKET_UNLOCK();

        return status;
    }

    nuConInfo[ourSocket].timerInitiated = RV_TRUE;

    RvAddressCopy(address, &(nuConInfo[ourSocket].sockAddress));

    /* Since the connect() call on nucleus is blocking, we construct a thread
       for closing this socket */
    status = RvThreadConstruct(RvSocketNucleusEventsTaskConnect, (void*)ourSocket, logMgr, &(nuConInfo[ourSocket].threadId));
    status = RvThreadSetName(&(nuConInfo[ourSocket].threadId), "RvNuConnect");
    status = RvThreadSetPriority(&(nuConInfo[ourSocket].threadId), RV_THREAD_SOCKET_PRIORITY_DEFAULT);

    nuConInfo[ourSocket].socketState = RvSocketState_Connecting;

    RV_SOCKET_UNLOCK();

    status = RvThreadCreate(&(nuConInfo[ourSocket].threadId));
    if (status == RV_OK)
        status = RvThreadStart(&(nuConInfo[ourSocket].threadId));

    if (status != RV_OK)
    {
        RV_SOCKET_LOCK();
        nuConInfo[ourSocket].socketState = RvSocketState_Idle;
        RV_SOCKET_UNLOCK();
    }

    /* enable task-switching for our new thread */
    RvThreadNanosleep(RV_TIME64_NSECPERMSEC * 10, logMgr);

#  endif
#elif (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA)

    if (WSAConnect(ourSocket, RVSOCKADDR(u), socklen, NULL, NULL, NULL, NULL) == SOCKET_ERROR)
    {
        /* Would-Block error is just fine with us... */
        int wsaError = WSAGetLastError();
        if (wsaError != WSAEWOULDBLOCK)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#endif

    RvSocketSharerClose(&ourSocket);

    }
#endif /* (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN) */

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddress localaddr;
        RvAddressGetString(address, sizeof(addr), addr);
        RvSocketGetLocalAddress(&ourSocket,NULL,&localaddr);
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,address=%s:%d,scopeId=%d, new local port=%d)=0", *sock, addr,
			RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address),RvAddressGetIpPort(&localaddr)));
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
    if (status != RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketConnect(sock=%d,address=%s:%d,scopeId=%d,errno=%d)=%d",
            *sock, addr, RvAddressGetIpPort(address), RvAddressGetIpv6Scope(address), RvSocketErrNo, status));
    }
#endif

    return status;
}


/********************************************************************************************
 * RvSocketAccept
 * Accept an incoming socket connect request, creating a new socket object.
 * In blocking mode, this function blocks until an incoming connect request to this
 * socket is made.
 * In non-blocking mode, this function will exit immediately and when an incoming
 * connection request to this socket is made, this socket will receive the
 * RvSocketAccept event in the select module.
 * The newSocket object should be regarded as if it was created using
 * RvSocketConstruct().
 * This function needs to be ported only if TCP is used by the stack.
 * This function is thread-safe.
 * INPUT   : socket         - Listening socket receiving the incoming connection
 *           logMgr         - log manager instance
 * OUTPUT  : newSocket      - Accepted socket information
 *           remoteAddress  - Address of remote side of connection
 *                            Can be passed as NULL if not needed
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketAccept(
    IN  RvSocket*    sock,
    IN  RvLogMgr*    logMgr,
    OUT RvSocket*    newSocket,
    OUT RvAddress*   remoteAddress)
{
    RvSocket ourSocket;
    RvStatus status;
#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
    rvASockAddr u;
    SOCKLEN_TYPE socklen = RV_SOCKET_SOCKADDR_SIZE;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (newSocket == NULL))
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        if (remoteAddress != NULL)
            RvAddressGetString(remoteAddress, sizeof(addr), addr);
        else
            addr[0] = '\0';
        RvSockLogError((&logMgr->socketSource,
            "RvSocketAccept(sock=%d,new=%d,addr=%s:%d,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), ((newSocket == NULL) ? 0 : *newSocket),
			addr, RvAddressGetIpPort(remoteAddress), RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketAccept(socket=%d,newSocket=%p,remoteAddress=%p)",
        *sock, newSocket, remoteAddress));

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
        return status;

#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA)

    *newSocket = WSAAccept(ourSocket, RVSOCKADDR(u), &socklen, NULL, 0);
    if ((*newSocket) == INVALID_SOCKET)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
        status = RvSymSockAccept(sock,logMgr,newSocket,remoteAddress);

#else
    if ((*newSocket = ACCEPT_SOCKET(ourSocket, RVSOCKADDR(u), &socklen)) < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

    RvSocketSharerClose(&ourSocket);
    /* Sharing the new socket accepted with the sharer thread because this socket
       was not constructed in the sharer thread as all usual sockets were.
       This action will allow us sharer sockets from other threads. */
#if (RV_SOCKET_USE_SHARER == 1)
#if (RV_SOCKET_TYPE == RV_SOCKET_PSOS)
    if (*newSocket >= 0)
    {
        RvSocket sharedAcceptSocket;
        sharedAcceptSocket = shr_socket(*newSocket, RvSharer.thread.id);
        RvSocketSharerClose(newSocket);
        *newSocket = sharedAcceptSocket;
    }
#endif /* RV_SOCKET_TYPE */
#endif /* RV_SOCKET_USE_SHARER */


#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS && !HAS_NONBLOCKEDCONNECT)
    {
        RvAddress localAddress;

        /* make sure there's no old socket still open here */
        if (nuConInfo[*newSocket].socketState)
            RvSocketNucleusSocketClean(*newSocket);

        RV_SOCKET_LOCK();

        nuConInfo[*newSocket].socketState = RvSocketState_Connected;

        if (RvSocketGetLocalAddress(newSocket, logMgr, &localAddress) == RV_OK)
            nuConInfo[*newSocket].localPort = RvAddressGetIpPort(&localAddress);

        RV_SOCKET_UNLOCK();
    }

#endif

#endif

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD) || \
    (RV_SOCKET_TYPE == RV_SOCKET_PSOS) || \
    (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    /* This part is here since it's added to most of the OS's we support. In this part,
       we're just setting some socket options on the newly accepted socket. */
    if (status == RV_OK)
    {
        /* Some Unix systems don't like setting the options of the listening socket in the
           accepted one, so we have to change the defaults to fit the regular needs. */
        int yes = RV_TRUE;

        /* Disable Nagle algorithm - we want our messages to be sent as soon as possible */
        if (RvSocketSetSockOpt(newSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(yes)) != 0)
        {
            RvSockLogWarning((&logMgr->socketSource,
                "RvSocketAccept: (sock=%d) (listenSocket=%d) Failed to setsockopt(TCP_NODELAY) %d",
                *newSocket,*sock,RvSocketErrNo));
        }

        /* We're working with non-blocking sockets */
        RvSocketSetBlocking(newSocket, RV_FALSE, logMgr);
    }
#endif


#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
    if ((status == RV_OK) && (remoteAddress != NULL))
        status = RvSocketSockAddrToAddress(RVSOCKDATA(u), socklen, remoteAddress);
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        if (remoteAddress != NULL)
            RvAddressGetString(remoteAddress, sizeof(addr), addr);
        else
            addr[0] = '\0';
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketAccept(sock=%d,new=%d,addr=%s:%d)=0",
            *sock, *newSocket, addr, RvAddressGetIpPort(remoteAddress)));
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
    if (status != RV_OK)
    {
        RvSockLogWarning((&logMgr->socketSource,
            "RvSocketAccept(sock=%d,new=%d,errno=%d)=%d", *sock, *newSocket, RvSocketErrNo, status));
    }
#endif

    return status;
}



/********************************************************************************************
 * RvSocketDontAccept
 *
 * Rejects an incoming socket connect request.
 * This function needs to be ported only if TCP is used by the stack.
 * This function is thread-safe.
 * INPUT   : socket         - Listening socket receiving the incoming connection
 *           logMgr         - log manager
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketDontAccept(
    IN  RvSocket*    sock,
    IN  RvLogMgr*    logMgr)
{
    RvSocket newSock;
    RvStatus retCode;


    RvSockLogEnter((&logMgr->socketSource, "RvSocketDontAccept(sock=%d)",*sock));

    retCode = RvSocketAccept(sock,logMgr,&newSock,NULL);
    if (retCode != RV_OK) {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketDontAccept(sock=%d), RvSocketAccept failure", *sock));
        return retCode;
    }

    RvSockLogLeave((&logMgr->socketSource, "RvSocketDontAccept(sock=%d)",*sock));

    RvSocketDestruct(&newSock,RV_FALSE,NULL,logMgr);

    return RV_OK;
}



/********************************************************************************************
 * RvSocketListen
 * Listens on the specified socket for connections from other locations.
 * In non-blocking mode, when a connection request is received, this socket will receive
 * the RvSocketAccept event in the select module.
 * This function needs to be ported only if TCP is used by the stack.
 * This function is thread-safe.
 * INPUT   : sock           - Listening socket receiving the incoming connection
 *           queuelen       - Length of queue of pending connections
 *           logMgr         - log manager instance
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketListen(
        IN  RvSocket    *sock,
        IN  RvUint32    queuelen,
        IN  RvLogMgr*   logMgr)
{
    RvSocket ourSocket;
    RvStatus status;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketListen(sock=NULL,len=%d,errno=%d)=%d",
            queuelen, RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketListen(socket=%d,queuelen=%d)",
        *sock, queuelen));

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketListen(sock=%d,len=%d,errno=%d)=%d",
            *sock, queuelen, RvSocketErrNo, status));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    status = RvSymSockListen(sock,queuelen,logMgr);
#else
    if (LISTEN_SOCKET(ourSocket, (int)queuelen) < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#endif

    RvSocketSharerClose(&ourSocket);

#if RV_LOGMASK & RV_LOGLEVEL_DEBUG
    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketListen(sock=%d,len=%d)=0",
            *sock, queuelen));
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
    if (status != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketListen(sock=%d,len=%d,errno=%d)=%d",
            *sock, queuelen, RvSocketErrNo, status));
    }
#endif

    return status;
}


/********************************************************************************************
 * RvSocketSendBuffer
 * Send a buffer on a socket.
 * This function actually copies the given buffer to the operating system's memory
 * for later sending.
 * In blocking mode, this function will block until the buffer is sent to the remote
 * side or upon failure.
 * In non-blocking mode, this function will return immediately, indicating the exact
 * amount of bytes the operating system has processed and sent.
 * This function handles both TCP and UDP sockets. You might need to port only parts
 * of this function if you need only one of these protocols.
 * This function is thread-safe.
 * INPUT   : socket         - Socket to send the buffer on
 *           buffer         - Buffer to send
 *           bytesToSend    - Number of bytes to send from buffer
 *           remoteAddress  - Address to send the buffer to.
 *                            Should be set to NULL on connection-oriented sockets (TCP).
 *           logMgr         - log manager instance
 * OUTPUT  : bytesSent      - Number of bytes we sent
 *                            If less than bytesToSend, then we would block if we tried on a
 *                            blocking socket. The application in this case should wait for the
 *                            RvSelectWrite event in the select module before trying to send
 *                            the buffer again.
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSendBuffer(
    IN  RvSocket*   sock,
    IN  RvUint8*    buffer,
    IN  RvSize_t    bytesToSend,
    IN  RvAddress*  remoteAddress,
    IN  RvLogMgr*   logMgr,
    OUT RvSize_t*   bytesSent)
{
#define FUNC "RvSocketSendBuffer"
    RvSocket ourSocket;
    RvStatus status;
    int errLen = 0;
    int socketError = 0;

#if RV_LOGMASK
    RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
#endif

    (void)socketError;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (buffer == NULL))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSendBuffer(sock=%d,addr=%s:%d,buf=%p,len=%d,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), RV_LOG_ADDR(remoteAddress, addr), RvAddressGetIpPort(remoteAddress), buffer, bytesToSend, socketError, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSendBuffer(sock=%d,addr=%s:%d,buf=%p,len=%d)",
        *sock, RV_LOG_ADDR(remoteAddress, addr), RvAddressGetIpPort(remoteAddress), buffer, bytesToSend));
/*#define RV_SOCKET_EXTRA_LOGGING*/
#ifdef RV_SOCKET_EXTRA_LOGGING
    {
        RvChar txt[256], *p, *a;
        RvInt n = 40,cnt;
        if (n > (RvInt)bytesToSend)
            n = bytesToSend;
        p = txt;
        txt[0] = 0;
        for (cnt = 0; cnt < n; cnt++)
        {
            sprintf(p," %x",(RvUint) buffer[cnt]);
            p += strlen(p);
        }
        a = RV_LOG_ADDR(remoteAddress, addr);
        if (a == NULL)
            a = "";
        RvSockLogDebug((&logMgr->socketSource,
            "SendBuff: to %s:%d, len %d",a, RvAddressGetIpPort(remoteAddress),bytesToSend));
        RvSockLogDebug((&logMgr->socketSource,
            "SendBuff: %s",txt));
    }
#endif

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSendBuffer(sock=%d,addr=%s:%d,buf=%p,len=%d,errno=%d)=%d",
            *sock, RV_LOG_ADDR(remoteAddress, addr), RvAddressGetIpPort(remoteAddress), buffer, bytesToSend, socketError, status));
        return status;
    }

    (void)RvWinDscpCheckTos(ourSocket, logMgr);
#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    RV_UNUSED_ARG(errLen);
    RV_UNUSED_ARG(socketError);
    status = RvSymSockSendBuffer(sock,buffer,bytesToSend,remoteAddress,logMgr,bytesSent);
#else
    if (remoteAddress == NULL) {
        errLen = TCP_SEND(ourSocket, (char *)buffer, (int)bytesToSend, RV_MSG_NOSIGNAL);
    }
    else
    {
        rvASockAddr u;

        int socklen = 0;

        status = RvSocketAddressToSockAddr(remoteAddress, RVSOCKADDR(u), &socklen);
        if(status != RV_OK) {
            RvSockLogError((SRCFUNC " failed to convert remote RvAddress (%p) to sock_addr", remoteAddress));
            return status;
        }

        (void)rvShadowSetFlowinfo(&ourSocket, RVSOCKDATA(u), logMgr);
        errLen = UDP_SEND(ourSocket, (char *)buffer, (int)bytesToSend, RV_MSG_NOSIGNAL, RVSOCKADDR(u), socklen);
    }

    if (status == RV_OK)
    {
        /* We should make sure that the send()|sendto() functions actually succeeded, since a
           negative value means an error... */
        if (errLen > 0 || (bytesToSend == 0 && errLen == 0))
        {
            if (bytesSent != NULL)
                *bytesSent = errLen;
        }
        else
        {
            /* Update the number of bytes we sent to none */
            if (bytesSent != NULL)
                *bytesSent = 0;

#if (RV_SOCKET_TYPE != RV_SOCKET_NUCLEUS)  /* (nucleus does not have WOULDBLOCK event) */
            /* We had an error - check to see if it's WOULDBLOCK */
            socketError = RvSocketErrNo;
            if (!RvSocketErrorWouldBlock(socketError))
            {
                /* An error which is not "Would block" - return an error status */
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
            }
#else
            if(errLen != NU_WOULD_BLOCK) {
                status = RvOsError(errLen);
            }

#endif /* (RV_SOCKET_TYPE != RV_SOCKET_NUCLEUS) */
        }
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSendBuffer(sock=%d,addr=%s:%d,buf=%p,len=%d,sent=%d)=0",
            *sock, RV_LOG_ADDR(remoteAddress, addr), RvAddressGetIpPort(remoteAddress), buffer, bytesToSend, errLen));
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
    if (status != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSendBuffer(sock=%d,addr=%s:%d,buf=%p,len=%d,errno=%d)=%d",
            *sock, RV_LOG_ADDR(remoteAddress, addr), RvAddressGetIpPort(remoteAddress), buffer, bytesToSend, socketError, status));
    }
#endif

    RvSocketSharerClose(&ourSocket);
    return status;
#undef FUNC
}

#define MAX_CHUNKS_NUM  10

#if (RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA || RV_OS_TYPE == RV_OS_TYPE_WINCE)
typedef WSABUF bufstr;
#define BUF(_t) (_t).buf
#define LEN(_t) (_t).len
#elif (RV_SOCKET_TYPE == RV_SOCKET_BSD && RV_OS_TYPE != RV_OS_TYPE_OSE && RV_OS_TYPE != RV_OS_TYPE_INTEGRITY)
typedef struct iovec bufstr;
#define BUF(_t) (_t).iov_base
#define LEN(_t) (_t).iov_len
#else /*RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA || RV_OS_TYPE == RV_OS_TYPE_WINCE || RV_SOCKET_TYPE == RV_SOCKET_BSD)*/
typedef struct {
    RvChar* buf;
    RvUint32 len;
} bufstr;
#define BUF(_t) (_t).buf
#define LEN(_t) (_t).len
#endif


#if ((RV_SOCKET_TYPE == RV_SOCKET_BSD && RV_OS_TYPE != RV_OS_TYPE_OSE && RV_OS_TYPE != RV_OS_TYPE_INTEGRITY && RV_OS_TYPE != RV_OS_TYPE_VXWORKS) || \
	  RV_SOCKET_TYPE == RV_SOCKET_WIN32_WSA || RV_OS_TYPE == RV_OS_TYPE_WINCE)
#define RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED RV_YES
#else
#define RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED RV_NO
#endif

/********************************************************************************************
 * RvSocketSendManyBuffersEx
 * Send a buffer on a socket (scatter gather)
 * This function actually copies the given buffer to the operating system's memory
 * for later sending.
 * In blocking mode, this function will block until the buffer is sent to the remote
 * side or upon failure.
 * In non-blocking mode, this function will return immediately, indicating the exact
 * amount of bytes the operating system has processed and sent.
 * This function handles both TCP and UDP sockets. You might need to port only parts
 * of this function if you need only one of these protocols.
 * This function is thread-safe.
 * INPUT   : sock       - Socket to be used for buffer sending
 *           numOfBuffs - Number of buffers to be sent.
 *                        This parameter defines size of arrays pointed
 *                        by the 'ppBuff' and 'pLen' parameters.
 *           ppBuff     - Array of pointers of the buffers to be sent.
 *                        Size of this array is defined by the 'numOfBuffs'.
 *           pLen       - Array of lengths of the buffers to be sent.
 *                        The length should match buffers, pointed by the ppBuff
 *           destAddr   - Address where to send the buffers to.
 *                        This parameter is ignored for non-UDP sockets.
 *           logMgr     - Log manager
 * OUTPUT  : bytesSent  - Total number of bytes that were sent.
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSendManyBuffers(
    IN  RvSocket*   sock,
    IN  RvUint32    numOfBuffs,
    IN  RvUint8**   ppBuff,
    IN  RvSize_t*   pLen,
    IN  RvAddress*  remoteAddress,
    IN  RvLogMgr*   logMgr,
    OUT RvSize_t*   bytesSent)
{
    bufstr bufs[MAX_CHUNKS_NUM];
    RvStatus retv = RV_OK;
    RvSize_t totalLen = 0;
    RvUint32 i;

#if (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_NO)
    RvUint8   bbuff[4096], *pBuf, *pAlloc;
    RvSize_t  accLen;
#endif

    if (numOfBuffs == 1)
    {
        return RvSocketSendBuffer(sock,ppBuff[0],pLen[0],remoteAddress,logMgr,bytesSent);
    }

    if (numOfBuffs > MAX_CHUNKS_NUM)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSendManyBuffers(sock=%d): too many buffers (%d), no more than %d is allowed",
            *sock, numOfBuffs, MAX_CHUNKS_NUM));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }

    memset(bufs,0,sizeof(bufs));

    for (i=0; i<numOfBuffs; i++)
    {
        BUF(bufs[i]) = (RvChar*) ppBuff[i];
        LEN(bufs[i]) = (RvUint32) pLen[i];
        totalLen += pLen[i];
    }

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSendManyBuffers(sock=%d,buffer=%p,buffLen=%d,totalLen=%d,numOfBuffs=%d)",
        *sock,ppBuff[0],pLen[0],totalLen,numOfBuffs));

#if (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_YES)
    {
        int ret, socketError=0;

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)
        struct msghdr msg;
#else   /* (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)*/
        DWORD sent;
#endif  /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) else */

        rvASockAddr u;
        int socklen = RV_SOCKET_SOCKADDR_SIZE;

        if (remoteAddress)
        {
            if (RvSocketAddressToSockAddr(remoteAddress, RVSOCKADDR(u), &socklen) != RV_OK)
            {
                RvSockLogError((&logMgr->socketSource,
                    "RvSocketSendManyBuffers(sock=%d) failed in RvSocketAddressToSockAddr",*sock));
                return RV_ERROR_BADPARAM;
            }
            (void)rvShadowSetFlowinfo(sock,RVSOCKDATA(u),logMgr);
        }

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)
        memset(&msg,0,sizeof(msg));
        msg.msg_iov = bufs;
        msg.msg_iovlen = (RvInt)numOfBuffs;
        if (remoteAddress)
        {
            msg.msg_name = (void*) &u;
            msg.msg_namelen = socklen;
        }
        ret = sendmsg(*sock,&msg,0);
#else   /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) */
        if (remoteAddress)
        {
            ret = WSASendTo(*sock,bufs,(DWORD)numOfBuffs,&sent,0,RVSOCKADDR(u),socklen,NULL,NULL);
        }
        else
            ret = WSASend(*sock,bufs,(DWORD)numOfBuffs,&sent,0,NULL,NULL);

        if (ret == SOCKET_ERROR)
            ret = -1;
        else
            ret = sent;
#endif  /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) else */

        if (ret > 0)
        {
            if (bytesSent != NULL)
                *bytesSent = ret;
        }
        else
        {
            /* Update the number of bytes we sent to none */
            if (bytesSent != NULL)
                *bytesSent = 0;
            /* We had an error - check to see if it's WOULDBLOCK */
            socketError = RvSocketErrNo;
            if (!RvSocketErrorWouldBlock(socketError))
            {
                /* An error which is not "Would block" - return an error status */
                retv = RvSocketErrorCode(RV_ERROR_UNKNOWN);
            }
        }


    }

#else /*  (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_YES) */

    pAlloc = NULL;
    if (totalLen > sizeof(bbuff))
    {
        retv = RvMemoryAlloc(0,totalLen,logMgr,(void**)&pAlloc);
        if (!pAlloc)
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketSendManyBuffers (%d) failed in RvMemoryAlloc for big-buffer (%d bytes)",
                *sock,totalLen));
            return RV_ERROR_OUTOFRESOURCES;
        }
        pBuf = pAlloc;
    }
    else
        pBuf = bbuff;

    accLen = 0;
    for (i=0; accLen < totalLen; i++)
    {
        memcpy(pBuf+accLen,BUF(bufs[i]),LEN(bufs[i]));
        accLen += LEN(bufs[i]);
    }

    retv = RvSocketSendBuffer(sock,pBuf,totalLen,remoteAddress,logMgr,bytesSent);
    if (pAlloc)
        RvMemoryFree(pAlloc,logMgr);
#endif /*  (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_YES) */

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketSendManyBuffers(sock=%d,buffer=%p,buffLen=%d,totalLen=%d,numOfBuffs=%d)=%d",
        *sock, ppBuff[0], pLen[0], totalLen, numOfBuffs, retv));

    return retv;
}

/********************************************************************************************
 * RvSocketReceiveBuffer
 * Receive a buffer from a socket.
 * In blocking mode, this function will block until a buffer is received on the
 * socket.
 * In non-blocking mode, this function returns immediately, even if there is nothing
 * to be received on this socket at the moment. This function is usually called after
 * RvSelectRead event is received in the select module on this socket, indicating that
 * there is information to receive on this socket.
 * This function handles both TCP and UDP sockets. You might need to port only parts
 * of this function if you need only one of these protocols.
 * This function is thread-safe.
 * INPUT   : socket         - Socket to receive the buffer from
 *           buffer         - Buffer to use for received data
 *           bytesToReceive - Number of bytes available on the given buffer
 *                            For UDP sockets, this value must be higher than the incoming datagram.
 *           logMgr         - log manager instance
 * OUTPUT  : bytesReceived  - Number of bytes that were actually received
 *           remoteAddress  - Address buffer was received from
 *                            Should be given as NULL on connection-oriented sockets (TCP).
 *                            This address is constructed by this function and should be
 *                            destructed by the caller to this function
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketReceiveBuffer(
    IN  RvSocket*   sock,
    IN  RvUint8*    buffer,
    IN  RvSize_t    bytesToReceive,
    IN  RvLogMgr*   logMgr,
    OUT RvSize_t*   bytesReceived,
    OUT RvAddress*  remoteAddress)
{
    RvSocket ourSocket;
    RvStatus status;
    int errLen;
    int socketError = 0;
    /* remoteAddr == NULL serves as indication of TCP socket */
    RvBool isTcpSocket = (remoteAddress == NULL);

    (void)socketError;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (buffer == NULL))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketReceiveBuffer(sock=%d,buf=%p,len=%d,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), buffer, bytesToReceive, socketError, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
         "RvSocketReceiveBuffer(sock=%d,buffer=%p,bytesToReceive=%d,logMgr=%p,bytesReceived=%p,remoteAddress=%p)",
         *sock,buffer,bytesToReceive,logMgr,bytesReceived,remoteAddress));

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketReceiveBuffer(sock=%d,buf=%p,len=%d,errno=%d)=%d",
            *sock, buffer, bytesToReceive, socketError, status));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    RV_UNUSED_ARG(socketError);
    RV_UNUSED_ARG(isTcpSocket);

	status = RvSymSockGetBytesAvailable(sock,bytesReceived);
	if(status != RV_OK)
	{
		return status;
	}
	if (*bytesReceived == 0)
	{
		return RV_OK;
	}
    status = RvSymSockReceiveBuffer(sock,buffer,bytesToReceive,logMgr,bytesReceived,remoteAddress);
    errLen = *bytesReceived;
#else
    if (isTcpSocket)
        errLen = TCP_RECV(ourSocket, (char *)buffer, (int)bytesToReceive, 0);
    else
    {
        rvASockAddr u;
        int socklen = RV_SOCKET_SOCKADDR_SIZE;

        memset(&u, 0, sizeof(u));
        errLen = UDP_RECV(ourSocket, (char *)buffer, (int)bytesToReceive, 0, RVSOCKADDR(u), (SOCKLEN_TYPE *) &socklen);
        RvSocketSockAddrToAddress(RVSOCKDATA(u), socklen, remoteAddress);
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    /* nucleus return error when no RX data present... */
    if (errLen == NU_NO_DATA_TRANSFER)
        errLen = 0;
#endif

    if (errLen >= 0)
    {
        if (bytesReceived)
            *bytesReceived = (RvSize_t)errLen;
        /* FIN got on TCP connection, return EOF error */
        if(errLen == 0 && isTcpSocket && bytesToReceive > 0)
        {
            status = RV_SOCKET_ERROR_EOF;
        }
    }
    else
    {
        /* We had an error - check to see if it's WOULDBLOCK */
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
        /* Nucleus does not have WOULDBLOCK event, so we just assume it's an error */
        if (errLen != NU_WOULD_BLOCK)
        {
            status = RvOsError(errLen);
        }
#else
        socketError = RvSocketErrNo;
        if (RvSocketErrorMsgSize(socketError))
        {
            status = RvSocketErrorCode(RV_ERROR_INSUFFICIENT_BUFFER);
        }
        else if (!RvSocketErrorWouldBlock(socketError))
        {
            /* An error which is not "Would block" - return an error status */
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
#endif /*(RV_SOCKET_TYPE != RV_SOCKET_NUCLEUS)*/

        if (bytesReceived)
            *bytesReceived = 0;
    }
#endif

    RvSocketSharerClose(&ourSocket);

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvChar* pAddr = "";
        if (remoteAddress != NULL)
            pAddr = (RvAddressGetString(remoteAddress, sizeof(addr), addr)) ? addr:"";
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketReceiveBuffer(sock=%d,addr=%s:%d,buf=%p,len=%d,received=%d)=0",
            *sock, pAddr, RvAddressGetIpPort(remoteAddress), buffer, bytesToReceive, errLen));
#ifdef RV_SOCKET_EXTRA_LOGGING
        {
            RvChar txt[256], *p;
            RvInt n = 40,cnt;
            if (n > (RvInt)errLen)
                n = errLen;
            p = txt;
            txt[0] = 0;
            for (cnt = 0; cnt < n; cnt++)
            {
                sprintf(p," %x",(RvUint) buffer[cnt]);
                p += strlen(p);
            }
            RvSockLogDebug((&logMgr->socketSource,
                "RecvBuff: from %s:%d, len %d",pAddr,RvAddressGetIpPort(remoteAddress),errLen));
            RvSockLogDebug((&logMgr->socketSource,
                "RecvBuff: %s",txt));
        }
#endif
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status != RV_OK)
    {
        RvSockLogDebug((&logMgr->socketSource,
            "RvSocketReceiveBuffer(sock=%d,buf=%p,len=%d,errno=%d)=%d",
            *sock, buffer, bytesToReceive, socketError, status));
    }
#endif

    return status;
}

/********************************************************************************************
 * RvSocketReceiveManyBuffers
 * Receive a buffer from a socket (scatter/gather variant).
 * In blocking mode, this function will block until a buffer is received on the
 * socket.
 * In non-blocking mode, this function returns immediately, even if there is nothing
 * to be received on this socket at the moment. This function is usually called after
 * RvSelectRead event is received in the select module on this socket, indicating that
 * there is information to receive on this socket.
 * This function handles both TCP and UDP sockets. You might need to port only parts
 * of this function if you need only one of these protocols.
 * This function is thread-safe.
 * INPUT   : sock       - Socket to be used for receiving
 *           numOfBuffs - Number of buffers to be received.
 *                        This parameter defines size of arrays pointed
 *                        by the 'ppBuff' and 'pLen' parameters.
 *           ppBuff     - Array of pointers of the buffers to store the received
 *                        bytes.
 *                        Size of this array is defined by the 'numOfBuffs'.
 *           pLen       - Array of lengths of the buffers.
 *                        The length should match buffers, pointed by the ppBuff
 *           logMgr     - Log manager
 * OUTPUT  : bytesReceived  - Total number of bytes that were actually received
 *           remoteAddress  - Address where from the buffers were received.
 *                            This parameter is ignored for non-UDP sockets.
 *                            This address is constructed by this function and
 *                            should be destructed by the user
 *                            using the RvAddressDestruct function.
 * RETURN  : RV_OK on success, other on failure
 *
 * Notes:
 *   * Return value of RV_SOCKET_EOF_ERROR on TCP socket indicates that peer tries
 *     to close this connection. 'bytesReceived' in this case will be 0
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketReceiveManyBuffers(
    IN  RvSocket*   sock,
    IN  RvUint32    numOfBuffs,
    IN  RvUint8**   ppBuff,
    IN  RvSize_t*   pLen,
    IN  RvLogMgr*   logMgr,
    OUT RvSize_t*   bytesReceived,
    OUT RvAddress*  remoteAddress)
{
    bufstr   bufs[MAX_CHUNKS_NUM];
    RvStatus retv = RV_OK;
    RvSize_t totalLen = 0;
    RvUint32 i;
#if  (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_NO)
    RvUint8 bbuff[4096], *pBuf, *pAlloc;
#endif

    if (numOfBuffs == 1)
    {
        return RvSocketReceiveBuffer(sock,ppBuff[0],pLen[0],logMgr,bytesReceived,remoteAddress);
    }

    if (numOfBuffs > MAX_CHUNKS_NUM)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketReceiveManyBuffers(sock=%d): too many buffers (%d), no more than %d is allowed",
            *sock, numOfBuffs, MAX_CHUNKS_NUM));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }

    for (i=0; i<numOfBuffs; i++)
    {
        BUF(bufs[i]) = (RvChar*) ppBuff[i];
        LEN(bufs[i]) = (RvUint32) pLen[i];
        totalLen += pLen[i];
    }

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketReceiveManyBuffers(sock=%d,buffer=%p,buffLen=%d,totalLen=%d,numOfBuffs=%d)",
        *sock, ppBuff[0], pLen[0], totalLen, numOfBuffs));

#if  (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_YES)
    {
        int ret, socketError=0;

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)
        struct msghdr msg;
#else   /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) */
        DWORD flags = 0, received;
#endif  /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) else */

        rvASockAddr u;
        int socklen = RV_SOCKET_SOCKADDR_SIZE;

#if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)
        memset(&msg,0,sizeof(msg));
        msg.msg_iov = bufs;
        msg.msg_iovlen = (RvInt)numOfBuffs;
        if (remoteAddress)
        {
            msg.msg_name = (void*) &u;
            msg.msg_namelen = RV_SOCKET_SOCKADDR_SIZE;
        }
        ret = recvmsg(*sock,&msg,0);
        socklen = msg.msg_namelen;
#else   /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)*/
        if (remoteAddress)
        {
            memset(&u, 0, sizeof(u));
            ret = WSARecvFrom(*sock,bufs,numOfBuffs,&received,&flags,RVSOCKADDR(u),&socklen,NULL,NULL);
        }
        else
            ret = WSARecv(*sock,bufs,numOfBuffs,&received,&flags,NULL,NULL);

        if (ret == SOCKET_ERROR)
            ret = -1;
        else
            ret = received;
#endif  /* if (RV_SOCKET_TYPE == RV_SOCKET_BSD) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) else */
        RvSocketSockAddrToAddress(RVSOCKDATA(u), socklen, remoteAddress);

        if (ret >= 0)
        {
            if (bytesReceived)
                *bytesReceived = (RvSize_t)ret;
            /* FIN got on TCP connection, return EOF error */
            if(ret == 0 && remoteAddress && totalLen > 0)
                retv = RV_SOCKET_ERROR_EOF;
        }
        else {
            socketError = RvSocketErrNo;
            if (RvSocketErrorMsgSize(socketError))
            {
                retv = RvSocketErrorCode(RV_ERROR_INSUFFICIENT_BUFFER);
            }
            else if (!RvSocketErrorWouldBlock(socketError))
            {
                /* An error which is not "Would block" - return an error status */
                retv = RvSocketErrorCode(RV_ERROR_UNKNOWN);
            }
        }
        if (bytesReceived)
            *bytesReceived = 0;
    }

#else /* (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_YES)*/
    pAlloc = NULL;
    if (totalLen > sizeof(bbuff))
    {
        retv = RvMemoryAlloc(0,totalLen,logMgr,(void**)&pAlloc);
        if (!pAlloc)
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketReceiveManyBuffers (%d) failed in RvMemoryAlloc for big-buffer (%d bytes)",
                *sock,totalLen));
            return RV_ERROR_OUTOFRESOURCES;
        }
        pBuf = pAlloc;
    }
    else
        pBuf = bbuff;

    retv = RvSocketReceiveBuffer(sock,pBuf,totalLen,logMgr,bytesReceived,remoteAddress);
    if (retv == RV_OK && *bytesReceived > 0)
    {
        RvSize_t  accLen;
        accLen = 0;
        for (i=0; accLen < totalLen; i++)
        {
            memcpy(BUF(bufs[i]),pBuf+accLen,LEN(bufs[i]));
            accLen += LEN(bufs[i]);
        }
    }
    if (pAlloc)
        RvMemoryFree(pAlloc,logMgr);
#endif /* (RV_SEND_RECEIVE_MANY_BUFFERS_SUPPORTED == RV_YES)*/

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketReceiveManyBuffers(sock=%d,buffer=%p,buffLen=%d,totalLen=%d,numOfBuffs=%d)=%d",
        *sock, ppBuff[0], pLen[0], totalLen, numOfBuffs, retv));

    return retv;
}

/******************************************************************************
 * rvSocketWasShutdownByPeer
 * Checks, if remote peer shutdown connection to the socket.
 * This function is meaningless for not connection oriented sockets.
 * It is responsibility of caller not to use this function for UDP sockets.
 * This function is thread-safe.
 * INPUT   : socket      - Socket to be checked
 *           logMgr      - log manager instance
 * OUTPUT  : wasShutdown - RV_TRUE, if remote peer shutdown connection
 * RETURN  : RV_OK on success, other on failure
 *
 * Notes:
 *   * Any error on socket except WOULD_BLOCK causes erroneous return value
 */
RvStatus RVCALLCONV rvSocketWasShutdownByPeer(
    IN  RvSocket*  sock,
    IN  RvLogMgr*  logMgr,
    OUT RvBool*    wasShutdown)
{
    RvSocket ourSocket;
    RvStatus status;
    int      socketError;

    socketError = 0;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (wasShutdown == NULL))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "rvSocketWasShutdownByPeer(sock=%d)=%d",
            ((sock == NULL) ? 0 : *sock), status));
        return status;
    }
#endif

    *wasShutdown = RV_FALSE;

    RvSockLogEnter((&logMgr->socketSource,
         "rvSocketWasShutdownByPeer(sock=%d,logMgr=%p,wasShutdown=%p)",
         *sock,logMgr,wasShutdown));

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "rvSocketWasShutdownByPeer(sock=%d)=%d - socket sharing failed",
            *sock, status));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    if (NU_Is_Connected(ourSocket) == NU_FALSE)
    {
        *wasShutdown = RV_TRUE;
    }
#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    {
        RvSize_t bytesAvailable;
        status = RvSymSockGetBytesAvailable(sock, &bytesAvailable);
        if (status == RV_OK  &&  bytesAvailable == 0)
        {
            *wasShutdown = RV_TRUE;
        }
    }
#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)
    status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#else /* all OSs except WinCE, Nucleus and Symbian */
	{
		RvChar   byte;
		int      retVal;

        /* For SCTP socket there is no need in rvSocketWasShutdownByPeer
           functionality, since SCTP stack provides SHUTDOWN notifications.
           Therefore we disable this redundant functionality in order to
           prevent problems on systems, where MSG_PEEK for SCTP is not supported
           (e.g. Solaris 10).
        */
#if (RV_NET_TYPE & RV_NET_SCTP)
#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)
        {
            struct sctp_rtoinfo sctpOpt;
            RvSize_t            sctpOptLen = sizeof(struct sctp_rtoinfo);
            RvInt               sctpStatus;
            sctpStatus = getsockopt(ourSocket, IPPROTO_SCTP, SCTP_RTOINFO,
                                    (void*)&sctpOpt, &sctpOptLen);
            if (sctpStatus == 0)
            {
                /* getsockopt(IPPROTO_SCTP) succeeded => the socket is SCTP */
                status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
                RvSockLogDebug((&logMgr->socketSource,
                    "rvSocketWasShutdownByPeer(sock=%d,logMgr=%p) is not supported for SCTP",
                    *sock, logMgr));
                return status;
            }
        }
#endif /*#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)*/
#endif /*#if (RV_NET_TYPE & RV_NET_SCTP)*/

		retVal = recv(ourSocket, &byte, 1,  MSG_PEEK);
		if(retVal == 0)
		{
			*wasShutdown = RV_TRUE;
		}
		else
		/* Handle error: if it is not WOULDBLOCK, return error,
						 since socket is not usable any more */
		if (retVal < 0)
		{
			socketError = RvSocketErrNo;
			if (!RvSocketErrorWouldBlock(socketError))
			{
				status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
			}
		}
	}
#endif

    RvSocketSharerClose(&ourSocket);

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "rvSocketWasShutdownByPeer(sock=%d,logMgr=%p,wasShutdown=%p,wasShutdown=%d)=0",
            *sock, logMgr, wasShutdown, *wasShutdown));
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (status != RV_OK)
    {
        RvSockLogDebug((&logMgr->socketSource,
            "rvSocketWasShutdownByPeer(sock=%d,logMgr=%p,wasShutdown=%p,errno=%d)=%d",
            *sock, logMgr, wasShutdown, socketError, status));
    }
#endif

    return status;
}

/********************************************************************************************
 * RvSocketSetBuffers
 * Sets the send and receive buffer sizes for the specified socket. The default sizes
 * of send and receive buffers are platform dependent. This function may be used
 * to increase the default sizes if larger packets are expected, such as video packets,
 * or to decrease the default size if smaller packets are expected.
 * This function is thread-safe.
 * INPUT   : socket     - Socket whose buffer sizes are to be changed.
 *           sendSize   - The new size of the send buffer. If the size is negative, the existing
 *                        value remains.
 *           recvSize   - The new size of the receive buffer. If the size is negative, the existing
 *                        value remains.
 *           logMgr         - log manager instance
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetBuffers(
        IN RvSocket*    sock,
        IN RvInt32      sendSize,
        IN RvInt32      recvSize,
        IN RvLogMgr*    logMgr)
{
    RvStatus status = RV_OK;
#if (RV_SOCKET_TYPE != RV_SOCKET_NUCLEUS)  && (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
    /* can't set buffer sizes in nucleus ,symbian or MOPI */
    int buflen=0;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBuffers(sock=NULL,sendSize=%d,recvSize=%d,errno=%d)=%d",
            sendSize, recvSize, RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetBuffers(socket=%d,sendSize=%d,recvSize=%d)",
        *sock, sendSize, recvSize));

#if (RV_SOCKET_TYPE != RV_SOCKET_NUCLEUS) && (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)

#if (RV_OS_TYPE != RV_OS_TYPE_WINCE) /* SO_RCVBUF isn't supported in WinCE */
    if (recvSize >= 0)
    {
        buflen = (int)recvSize;
        if (RvSocketSetSockOpt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&buflen, sizeof(buflen)) != 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#else
	RV_UNUSED_ARG(recvSize);
#endif  /* (RV_OS_TYPE != RV_OS_TYPE_WINCE) */

    if (sendSize >= 0)
    {
        buflen = (int)sendSize;
        if (RvSocketSetSockOpt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&buflen, sizeof(buflen)) != 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)

    status = RvSymSockSetBuffers(sock,sendSize,recvSize,logMgr);

#endif

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetBuffers(socket=%d,sendSize=%d,recvSize=%d)=%d",
            *sock, sendSize, recvSize, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBuffers(sock=%d,sendSize=%d,recvSize=%d,errno=%d)=%d",
            *sock, sendSize, recvSize, RvSocketErrNo, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketSetLinger
 * Set the linger time after socket is closed.
 * This function can only be called only on UDP sockets.
 * This function is thread-safe.
 * INPUT   : sock       - Socket to modify
 *           lingerTime - Time to linger in seconds
 *                        Setting this parameter to -1 sets linger off for this socket
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetLinger(
        IN  RvSocket*   sock,
        IN  RvInt32     lingerTime,
        IN  RvLogMgr*   logMgr)
{
#if (RV_OS_TYPE == RV_OS_TYPE_OSE) || (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS) || \
    (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
    /* Linger parameter of a socket is not supported in OSE & NUCLEUS */
    RV_UNUSED_ARG(sock);
    RV_UNUSED_ARG(lingerTime);
    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketSetLinger is not supported by this operating system"));
    return RV_OK;

#else
    RvStatus status = RV_OK;
    struct linger lingerParam;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetLinger(sock=<NULL>,linger=%d,errno=%d)=%d",
            lingerTime, RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetLinger(sock=%d,linger=%d)",
        *sock, lingerTime));

    if (lingerTime >= 0)
    {
        lingerParam.l_onoff = RV_TRUE;
        lingerParam.l_linger = (RV_LINGER_VALUE_TYPE)lingerTime;
    }
    else
    {
        lingerParam.l_onoff = RV_FALSE;
        lingerParam.l_linger = 0;
    }

    if (RvSocketSetSockOpt(sock, SOL_SOCKET, SO_LINGER, (char *)&lingerParam, sizeof(lingerParam)) != 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetLinger(sock=%d,linger=%d)=%d",
            *sock, lingerTime, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetLinger(sock=%d,linger=%d,errno=%d)=%d",
            *sock, lingerTime, RvSocketErrNo, status));
    }

    return status;
#endif
}


/********************************************************************************************
 * RvSocketReuseAddr
 * Set the socket as a reusable one (in terms of its address)
 * This allows a TCP server socket and UDP multicast addresses to be used by
 * other processes on the same machine as well.
 * This function has to be called before RvSocketBind().
 * This function is thread-safe.
 * INPUT   : sock     - Socket to modify
 *           logMgr   - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketReuseAddr(
        IN  RvSocket*   sock,
        IN  RvLogMgr*   logMgr)
{
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS) || (RV_SOCKET_TYPE== RV_SOCKET_SYMBIAN)
    /* not supported in NUCLEUS, Symbian */
    RV_UNUSED_ARG(sock);
    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketReuseAddr is not supported by this operating system"));
    return RV_OK;

#else
    RvStatus status = RV_OK;
    int      yes    = RV_TRUE;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketReuseAddr(sock=<NULL>, errno=%d)=%d",
            RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketReuseAddr(sock=%d)",
        *sock));

    if (RvSocketSetSockOpt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) != 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketReuseAddr(sock=%d)=%d",
            *sock, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketReuseAddr(sock=%d, errno=%d)=%d",
            *sock, RvSocketErrNo, status));
    }

    return status;
#endif
}


/********************************************************************************************
 * RvSocketSetBlocking
 * Set blocking/non-blocking mode on a socket.
 * This function must be called after calling RvSocketConstruct(), otherwise, the
 * socket's mode of operation will be determined by each operating system separately,
 * causing applications and stacks to be non-portable.
 * This function is thread-safe.
 * INPUT   : sock       - Socket to modify
 *           isBlocking - RV_TRUE for a blocking socket
 *                        RV_FALSE for a non-blocking socket
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetBlocking(
        IN RvSocket*    sock,
        IN RvBool       isBlocking,
        IN RvLogMgr*    logMgr)
{
    RvSocket ourSocket;
    RvStatus status;
    int res;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBlocking(sock=NULL, isBlocking=%d, errno=%d)=%d",
            isBlocking, RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetBlocking(sock=%d, isBlocking=%d)",
        *sock, isBlocking));

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((isBlocking != RV_TRUE) && (isBlocking != RV_FALSE))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBlocking(sock=%d, isBlocking=%d, errno=%d)=%d",
            *sock, isBlocking, RvSocketErrNo, status));
        return status;
    }
#endif

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBlocking(sock=%d, isBlocking=%d, errno=%d)=%d",
            *sock, isBlocking, RvSocketErrNo, status));
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    if (isBlocking)
        res = NU_Fcntl(ourSocket, NU_SETFLAG, NU_BLOCK);
    else
        res = NU_Fcntl(ourSocket, NU_SETFLAG, NU_FALSE);
#elif (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
    res = RvSymSocketSetBlocking(sock,isBlocking);
#else   /* UNIX, VxWorks, pSOS, OSE, WINCE and WIN32 */
    {
        int on = !isBlocking;
        res = ioctl(ourSocket, FIONBIO, (RV_IOCTL_ARGP_TYPE)&on);
    }
#endif

    if (res < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

    RvSocketSharerClose(&ourSocket);

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetBlocking(sock=%d, isBlocking=%d)=%d",
            *sock, isBlocking, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBlocking(sock=%d, isBlocking=%d, errno=%d)=%d",
            *sock, isBlocking, RvSocketErrNo, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketSetBroadcast
 * Set permission for sending broadcast datagrams on a socket
 * This function is only needed if the stack being ported supports multicast sending.
 * This function is thread-safe.
 * INPUT   : sock           - Socket to modify
 *           canBroadcast   - RV_TRUE for permitting broadcast
 *                            RV_FALSE for not permitting broadcast
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetBroadcast(
        IN RvSocket*    sock,
        IN RvBool       canBroadcast,
        IN RvLogMgr*    logMgr)
{
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    /* not supported in NUCLEUS */
    RV_UNUSED_ARG(sock);
    RV_UNUSED_ARG(canBroadcast);
    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketSetBroadcast is not supported by this operating system"));
    return RV_OK;

#else
    RvStatus status = RV_OK;
    int      on     = (int)canBroadcast;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBroadcast(sock=NULL, canBroadcast=%d, errno=%d)=%d",
            canBroadcast, RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetBroadcast(sock=%d, canBroadcast=%d)=%d",
        *sock, canBroadcast, status));

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((canBroadcast != RV_TRUE) && (canBroadcast != RV_FALSE))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBroadcast(sock=%d, canBroadcast=%d, errno=%d)=%d",
            *sock, canBroadcast, RvSocketErrNo, status));
        return status;
    }
#endif

#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    RV_UNUSED_ARG(on);
    status = RV_OK;
    if(RvSymSocketSupportBroadCast())
    {
       status = RvSymSockSetBroadcast(sock,canBroadcast,logMgr);
       if(status != RV_OK)
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketSetBroadcast(sock=%d, canBroadcast=%d, errno=%d)=%d",
                *sock, canBroadcast, RvSocketErrNo, status));
	    }
    }




#else
    if (RvSocketSetSockOpt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on)) != 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#endif
    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetBroadcast(sock=%d, canBroadcast=%d)=%d",
            *sock, canBroadcast, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetBroadcast(sock=%d, canBroadcast=%d, errno=%d)=%d",
            *sock, canBroadcast, RvSocketErrNo, status));
    }

    return status;
#endif
}

#if RV_SOCKET_ENABLE_IPV6_TOS

RVCOREAPI
RvStatus RVCALLCONV RvSocketSetToS6( IN RvSocket*    sock,
                                    IN RvInt        tos,
                                    IN RvLogMgr*    logMgr) {
#define FUNC "RvSocketSetTos6"

    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource, FUNC "(sock=0,tos=%d)=%d", tos, status));
        return status;
    }
#endif /* RV_CHECK_MASK & RV_CHECK_NULL */

    RvSockLogEnter((&logMgr->socketSource, FUNC "(sock=%d, tos=%d)", *sock, tos));

#if defined(IPV6_TCLASS)
    if(RvSocketSetSockOpt(sock, SOL_IPV6, IPV6_TCLASS, (char *)&tos, sizeof(tos)) != 0) {
        RvSockLogError((&logMgr->socketSource, FUNC "failed setting dscp=%d on socket %d", tos, *sock));
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

#elif RV_SOCKET_USE_FAKED_IPV6_TCLASS
    status = rvShadowSetTrafficClass(sock, tos, logMgr);

#else
    RvSockLogError((&logMgr->socketSource, FUNC "isn't supported"));
    return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif /* IPV6_TCLASS */


    RvSockLogLeave((&logMgr->socketSource, FUNC "(sock=%d, trafficClass=%d)", *sock, tos));

    return status;

#undef FUNC
}

RVCOREAPI
RvStatus RVCALLCONV RvSocketGetToS6(IN RvSocket*    sock,
                                    IN RvLogMgr*    logMgr,
                                    IN RvInt        *tos) {
#define FUNC "RvSocketGetToS6"


    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource, FUNC "(sock=0)=%d", status));
        return status;
    }
#endif /* RV_CHECK_MASK & RV_CHECK_NULL */


    RvSockLogEnter((&logMgr->socketSource, FUNC "(sock=%d)",  *sock));

#if defined(IPV6_TCLASS)
    {
        int optLen = (int)sizeof(*tos);

        if(GET_SOCK_OPT(*sock, SOL_IPV6, IPV6_TCLASS, (char *)tos, &optLen) != 0) {
            int sockerr = RvSocketErrNo;
            RvSockLogError((&logMgr->socketSource, FUNC "failed on socket %d (oserror = %d)", *sock, sockerr));
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
    }

#elif RV_SOCKET_USE_FAKED_IPV6_TCLASS
    *tos = 0;
    status = rvShadowGetTrafficClass(sock, tos, logMgr);

#else
    RvSockLogError((&logMgr->socketSource, FUNC "isn't supported"));
    return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif /* IPV6_TCLASS */


    RvSockLogLeave((&logMgr->socketSource, FUNC "(sock=%d, tos=%d)", *sock, *tos));

    return status;

#undef FUNC
}

#endif /*RV_SOCKET_ENABLE_IPV6_TOS*/

RVCOREAPI
RvStatus RVCALLCONV RvSocketSetToS4(IN RvSocket*    sock,
                                    IN RvInt        tos,
                                    IN RvLogMgr*    logMgr) {
#define FUNC "RvSocketSetToS4"

    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((&logMgr->socketSource, FUNC "(sock=0,tos=%d)=%d", tos, status));
        return status;
    }
#endif /* RV_CHECK_MASK & RV_CHECK_NULL */

    if(!RvSocketIsTypeOfServiceSupported()) {
        RvSockLogError((&logMgr->socketSource, FUNC "is not supported"));
        return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
    }

#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)

#  if defined(IP_TOS)

    if (RvSocketSetSockOpt(sock, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(tos)) != 0) {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

#  else
     status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);

#  endif /* defined(IP_TOS) */

#else
    status = RvSymSockSetTypeOfService(sock, tos, NULL);
    if(status != RV_OK)
    {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#endif
    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetTypeOfService(sock=%d, tos=%d)=%d",
            *sock, tos, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            FUNC "(sock=%d, tos=%d, errno=%d)=%d", *sock, tos, RvSocketErrNo, status));
    }

    return status;

#undef FUNC
}



RVCOREAPI
RvStatus RVCALLCONV RvSocketGetToS4(IN RvSocket*    sock,
                                    IN RvLogMgr*    logMgr,
                                    IN RvInt        *tos) {
#define FUNC "RvSocketGetToS4"

    RvStatus status = RV_OK;
    RvSocket ourSocket;
#if defined(IP_TOS)
    SOCKLEN_TYPE optSize = (SOCKLEN_TYPE)sizeof(*tos);
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((SRC_FUNC "(sock=0)=%d", status));
        return status;
    }
#endif /* RV_CHECK_MASK & RV_CHECK_NULL */

    if(!RvSocketIsTypeOfServiceSupported()) {
        RvSockLogError((SRC_FUNC "is not supported"));
        return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
    }

#if defined(IP_TOS)
    status = RvSocketSharerShare(logMgr, sock, &ourSocket);

    if(status != RV_OK)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((SRC_FUNC "(sock=%d)=%d", *sock, status));
        return status;
    }

    if(GET_SOCK_OPT(ourSocket, IPPROTO_IP, IP_TOS, (char *)tos, &optSize) != 0) {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    RvSocketSharerClose(&ourSocket);
#else

    status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);

#endif /*IP_TOS*/

    if (status == RV_OK)
    {
        RvSockLogLeave((SRC_FUNC "(sock=%d, tos=%d)=%d", *sock, *tos, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            FUNC "(sock=%d, errno=%d)=%d", *sock, RvSocketErrNo, status));
    }

    return status;

#undef FUNC
}



/********************************************************************************************
 * RvSocketSetTypeOfService
 * Set the type of service (DiffServ Code Point) of the socket (IP_TOS)
 * This function is supported by few operating systems.
 * IPV6 does not support type of service.
 * This function is thread-safe.
 * INPUT   : sock           - Socket to modify
 *           typeOfService  - type of service to set
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */


RVCOREAPI
RvStatus RVCALLCONV RvSocketSetTypeOfService(
                                             IN RvSocket*    sock,
                                             IN RvInt        typeOfService,
                                             IN RvLogMgr*    logMgr)
{
#define FUNC "RvSocketSetTypeOfService"

    RvStatus status = RV_OK;
    RvInt addrType;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((SRC_FUNC "(sock=0,typeOfService=%d)=%d",
             typeOfService, status));
        return status;
    }
#endif /* RV_CHECK_MASK & RV_CHECK_NULL */


    RvSockLogEnter((SRC_FUNC "(sock=%d, typeOfService=%d)", *sock, typeOfService));

#if (RV_WIN_DSCP == RV_WIN_DSCP_TC) || (RV_WIN_DSCP == RV_WIN_DSCP_GQOS)

#if (RV_WIN_DSCP == RV_WIN_DSCP_TC)
    /* the IPTOS and TC (Traffic Control) mechanisms implement different semantics
       IP TOS sets the whole byte (in the IP header) to the 'typeOfService' value
       whereas the TC mechanism sets the relevant ToS bits within the byte.
       So we need to scale down the value.
    */
     typeOfService >>= 2;   
#endif


    (void)addrType; 
    status = RvWinDscpSetTos(*sock, typeOfService, logMgr);

#else /* RV_WIN_DSCP */
    status = RvSocketGetAddressType(sock, logMgr, &addrType);
    if(status != RV_OK) {
        return status;
    }

    if(status != RV_OK) {
        /* Failed to get address type - probably, socket is unbound
         * Try to set as if socket is IPv4 socket and next - as IPv6
         */
        status = RvSocketSetToS4(sock, typeOfService, logMgr);
#if RV_SOCKET_ENABLE_IPV6_TOS
        if(status != RV_OK) {
            status = RvSocketSetToS6(sock, typeOfService, logMgr);
        }
#endif
    } else if(addrType == RV_ADDRESS_TYPE_IPV4) {
        status = RvSocketSetToS4(sock, typeOfService, logMgr);
    } else if(addrType == RV_ADDRESS_TYPE_IPV6) {
#if RV_SOCKET_ENABLE_IPV6_TOS
        status = RvSocketSetToS6(sock, typeOfService, logMgr);
#else
        RvSockLogExcep((SRC_FUNC "(sock=%d, tos=%d) - unexpected address type: IPV6", *sock, typeOfService));
        return RV_ERROR_UNKNOWN;
#endif
    }
#endif /* RV_WIN_DSCP */

    if (status == RV_OK)
    {
        RvSockLogLeave((SRC_FUNC "(sock=%d, typeOfService=%d)=%d", *sock, typeOfService, status));
    }
    else
    {
        RvSockLogError((SRC_FUNC "(sock=%d, typeOfService=%d, errno=%d)=%d",
            *sock, typeOfService, RvSocketErrNo, status));
    }

    return status;
#undef FUNC
}



/********************************************************************************************
 * RvSocketGetTypeOfService
 * Get the type of service (DiffServ Code Point) of the socket (IP_TOS)
 * This function is supported by few operating systems.
 * IPV6 does not support type of service.
 * This function is thread-safe.
 * INPUT   : sock           - socket to modify
 *           logMgr         - log manager instance
 * OUTPUT  : typeOfService  - type of service to set
 * RETURN  : RV_OK on success, other on failure
 */

RVCOREAPI
RvStatus RVCALLCONV RvSocketGetTypeOfService(
     IN  RvSocket*    sock,
     IN  RvLogMgr*    logMgr,
     OUT RvInt32*     typeOfService)
{
#define FUNC "RvSocketGetTypeOfService"

    RvStatus status;
    RvInt addrType;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (typeOfService == NULL))
    {
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvSockLogError((SRC_FUNC "(sock=%d,typeOfService=%p)=%d",
            ((sock == NULL) ? 0 : *sock), typeOfService, status));
        return status;
    }
#endif /* RV_CHECK_MASK & RV_CHECK_NULL */

    RvSockLogEnter((SRC_FUNC "(sock=%d)", *sock));

    if(!RvSocketIsTypeOfServiceSupported()) {
        RvSockLogError((SRC_FUNC "is not supported on this OS"));
        return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
    }

#if (RV_WIN_DSCP == RV_WIN_DSCP_TC) || (RV_WIN_DSCP == RV_WIN_DSCP_GQOS)
    (void)addrType;
    status = RvWinDscpGetTos(*sock, typeOfService, logMgr);

#if (RV_WIN_DSCP == RV_WIN_DSCP_TC)

    if (status == RV_OK)
    /* the IPTOS and TC (Traffic Control) mechanisms implement different semantics
    IP TOS sets the whole byte (in the IP header) to the 'typeOfService' value
    whereas the TC mechanism sets the relevant ToS bits within the byte.
    So we need to scale up the value.
    */
        *typeOfService <<= 2;   
#endif
#else /* RV_WIN_DSCP */
    status = RvSocketGetAddressType(sock, logMgr, &addrType);
    if(status != RV_OK) {
        status = RvSocketGetToS4(sock, logMgr, typeOfService);
#if RV_SOCKET_ENABLE_IPV6_TOS
        if(status != 0) {
            status = RvSocketGetToS6(sock, logMgr, typeOfService);
        }
#endif
    } else if(addrType == RV_ADDRESS_TYPE_IPV4) {
        status = RvSocketGetToS4(sock, logMgr, typeOfService);
    } else if(addrType == RV_ADDRESS_TYPE_IPV6) {
#if RV_SOCKET_ENABLE_IPV6_TOS
        status = RvSocketGetToS6(sock, logMgr, typeOfService);
#else
        RvSockLogExcep((SRC_FUNC "(sock=%d) - unexpected address type: IPV6", *sock));
        return RV_ERROR_UNKNOWN;
#endif
    } else {
        RvSockLogExcep((SRC_FUNC "(sock=%d) - unexpected address type: %d", *sock, addrType));
        return RV_ERROR_UNKNOWN;
    }
#endif /* RV_WIN_DSCP */


    if (status == RV_OK)
    {
        RvSockLogLeave((SRC_FUNC "(sock=%d,typeOfService=%d)", *sock,*typeOfService));
    }
    else
    {
        RvSockLogError((SRC_FUNC "(sock=%d, errno=%d)=%d", *sock, RvSocketErrNo, status));
    }

    return status;

#undef FUNC
}



/********************************************************************************************
 * RvSocketSetMulticastTtl
 * Set the TTL to use for multicast sockets (UDP)
 * This function is only needed if the stack being ported supports multicast sending.
 * This function is thread-safe.
 * INPUT   : sock       - Socket to modify
 *           ttl        - TTL to set
 *           logMgr     - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetMulticastTtl(
        IN RvSocket*    sock,
        IN RvInt32      ttl,
        IN RvLogMgr*    logMgr)
{
    RvStatus status = RV_OK;
    RvAddress locAddr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastTtl(sock=<NULL>, ttl=%d, errno=%d)=%d",
            ttl, RvSocketErrNo, RvSocketErrorCode(RV_ERROR_BADPARAM)));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetMulticastTtl(sock=%d,ttl=%d)",
        *sock, ttl));

    RvSocketGetLocalAddress(sock, NULL, &locAddr);

/* SYMBIAN's standard C (libc) does not support these defines
   but if Nokia's Open C SDK is installed and used it does support these defines */
#if (RV_NET_TYPE & RV_NET_IPV6 && defined(IPPROTO_IPV6) && defined(IPV6_MULTICAST_HOPS))
    if (locAddr.addrtype == RV_ADDRESS_TYPE_IPV6)
    {
        RV_MULTICASTTTLIPV6_VALUE_TYPE ttlValue = (RV_MULTICASTTTLIPV6_VALUE_TYPE)ttl;

        if (RvSocketSetSockOpt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char*)&ttlValue, sizeof(ttlValue)) != 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
    else
#endif
    if (locAddr.addrtype == RV_ADDRESS_TYPE_IPV4)
    {
        RV_MULTICASTTTL_VALUE_TYPE ttlValue = (RV_MULTICASTTTL_VALUE_TYPE)ttl;

        if (RvSocketSetSockOpt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttlValue, sizeof(ttlValue)) != 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    RvAddressDestruct(&locAddr)

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetMulticastTtl(sock=%d, ttl=%d)=%d",
            *sock, ttl, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastTtl(sock=%d, ttl=%d, errno=%d)=%d",
            *sock, ttl, RvSocketErrNo, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketSetMulticastInterface
 * Set the interface to use for multicast packets (UDP)
 * This function is only needed if the stack being ported supports multicast receiving.
 * This function is thread-safe.
 * INPUT   : sock       - Socket to modify
 *           address    - Local address to use for the interface
 *           logMgr     - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetMulticastInterface(
        IN RvSocket*    sock,
        IN RvAddress*   address,
        IN RvLogMgr*    logMgr)
{
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    RV_UNUSED_ARG(sock);
    RV_UNUSED_ARG(address);
    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketSetMulticastInterface is not supported by this operating system"));
    return RV_OK;
#else
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (address == NULL))
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastInterface(sock=%d,addr=%s:%d,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), addr, RvSocketErrNo, RvSocketErrorCode(RV_ERROR_BADPARAM)));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogEnter((&logMgr->socketSource,
            "RvSocketSetMulticastInterface(sock=%d,addr=%s:%d)",
            *sock, addr, RvAddressGetIpPort(address)));
    }

    switch (address->addrtype)
    {
    case RV_ADDRESS_TYPE_IPV4:
        {
            struct in_addr addr;
            memset(&addr, 0, sizeof(addr));

            addr.s_addr = RvAddressIpv4GetIp(RvAddressGetIpv4(address));

            if (RvSocketSetSockOpt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr, sizeof(addr)) != 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
        break;

/* SYMBIAN's standard C (libc) does not support these defines
   but if Nokia's Open C SDK is installed and used it does support these defines */
#if (RV_NET_TYPE & RV_NET_IPV6  /*&& defined(IPPROTO_IPV6)*/ && defined(IPV6_MULTICAST_IF))
    /* On windows Vista (and older) the IPPROTO_IPV6 became enum (and not define) thus we cannot
       leave 'defined(IPPROTO_IPV6) here. The assumption is that if IPV6_MULTICAST_IF is defined
       the IPPROTO_IPV6 will also be defined somehow */


    case RV_ADDRESS_TYPE_IPV6:
        {
            unsigned int outif = address->data.ipv6.scopeId;
            /* ToDo: maybe use if_nameindex() to get the interface names, or find some way
            to get the interface from the socket
            outif = if_nametoindex("hme0");*/

            if (RvSocketSetSockOpt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char*)&outif, sizeof(outif)) != 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

            RvSockLogDebug((&logMgr->socketSource,
                "RvSocketSetMulticastInterface(sock=%d) interface-index=%d, status=%d",
                *sock, outif,status));
        }
        break;
#endif

    default:
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }

    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetMulticastInterface(sock=%d,addr=%s)=0",
            *sock, addr));
    }
    else
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(address, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastInterface(sock=%d,addr=%s,errno=%d)=%d",
            *sock, addr, RvSocketErrNo, status));
    }

    return status;
#endif
}

/********************************************************************************************
* RvSocketSetMulticastLoop
* Enable or disable local loopback of multicast datagrams.
* By default, loopback is enabled: A copy of each multicast datagram sent by a process
* on the host will also be looped back and processed as a received datagram by that host,
* if the host belongs to that multicast group on the outgoing interface.
* This function is thread-safe.
* INPUT   : sock       - Socket to modify
*           bEnable    - Indication if the loopback is enabled or not
*           logMgr     - log manager instance
* OUTPUT  : None
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI
RvStatus RVCALLCONV RvSocketSetMulticastLoop(
    IN RvSocket*    sock,
    IN RvBool       bEnable,
    IN RvLogMgr*    logMgr)
{
    RvStatus  status = RV_OK;
    RvAddress locAddr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastLoop(sock=<NULL>, enable=%d, errno=%d)=%d",
            bEnable, RvSocketErrNo, RvSocketErrorCode(RV_ERROR_BADPARAM)));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetMulticastLoop(sock=%d,enable=%d)",
        *sock, bEnable));

    status = RvSocketGetLocalAddress(sock, NULL, &locAddr);
    if (status != RV_OK)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastLoop: RvSocketGetLocalAddress failed with %d",
            status));
        return RV_ERROR_UNKNOWN;
    }

#if (RV_NET_TYPE & RV_NET_IPV6 && defined(IPPROTO_IPV6) && defined(IPV6_MULTICAST_HOPS))
    if (locAddr.addrtype == RV_ADDRESS_TYPE_IPV6)
    {
        RV_MULTICAST_LOOPIPV6_VALUE_TYPE enableValue = (RV_MULTICAST_LOOPIPV6_VALUE_TYPE)bEnable;

        if (RvSocketSetSockOpt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char*)&enableValue, sizeof(enableValue)) != 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
    else
#endif
    if (locAddr.addrtype == RV_ADDRESS_TYPE_IPV4)
    {
        RV_MULTICAST_LOOP_VALUE_TYPE enableValue = (RV_MULTICAST_LOOP_VALUE_TYPE)bEnable;
        RvInt ret;

        ret = RvSocketSetSockOpt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&enableValue, sizeof(enableValue));
        if (ret != 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    RvAddressDestruct(&locAddr)

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetMulticastLoop(sock=%d, enable=%d)=%d",
            *sock, bEnable, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetMulticastLoop(sock=%d, enable=%d, errno=%d)=%d",
            *sock, bEnable, RvSocketErrNo, status));
    }

    return status;
}

/********************************************************************************************
 * RvSocketJoinMulticastGroup
 * Join a multicast group
 * This function is only needed if the stack being ported supports multicast receiving.
 * This function is thread-safe.
 * INPUT   : sock             - Socket to modify
 *           multicastAddress - Multicast address to join
 *           interfaceAddress - Interface address to use on the local host.
 *                              Setting this to NULL chooses an arbitrary interface
 *           logMgr           - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketJoinMulticastGroup(
    IN RvSocket*    sock,
    IN RvAddress*   multicastAddress,
    IN RvAddress*   interfaceAddress,
    IN RvLogMgr*    logMgr)
{
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (multicastAddress == NULL) || (interfaceAddress == NULL))
    {
        RvChar addr1[RV_ADDRESS_MAXSTRINGSIZE], addr2[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(multicastAddress, sizeof(addr1), addr1);
        RvAddressGetString(interfaceAddress, sizeof(addr2), addr2);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketJoinMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), addr1, addr2, RvSocketErrNo, RvSocketErrorCode(RV_ERROR_BADPARAM)));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    {
        RvChar addr1[RV_ADDRESS_MAXSTRINGSIZE], addr2[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(multicastAddress, sizeof(addr1), addr1);
        RvAddressGetString(interfaceAddress, sizeof(addr2), addr2);
        RvSockLogEnter((&logMgr->socketSource,
                "RvSocketJoinMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s)",
                *sock, addr1, addr2));
    }

    switch (multicastAddress->addrtype)
    {
    case RV_ADDRESS_TYPE_IPV4:
        {
            RV_IP_MREQ mreq;
            const RvAddressIpv4* multicastIpv4Addr;
            const RvAddressIpv4* interfaceIpv4Addr;
            RvUint32 multicastIp;
            RvUint32 interfaceIp = 0;

            /* Get the IP address of each one of the addresses */
            multicastIpv4Addr = RvAddressGetIpv4(multicastAddress);
            if (multicastIpv4Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            multicastIp = RvAddressIpv4GetIp(multicastIpv4Addr);

            interfaceIpv4Addr = RvAddressGetIpv4(interfaceAddress);
            if (interfaceIpv4Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            interfaceIp = RvAddressIpv4GetIp(interfaceIpv4Addr);

            /* Set the parameter for the OS */
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
            mreq.imr_multiaddr = multicastIp;
            mreq.imr_interface = interfaceIp;
#else
            mreq.imr_multiaddr.s_addr = multicastIp;
            mreq.imr_interface.s_addr = interfaceIp;
#endif
            if (RvSocketSetSockOpt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) != 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
        break;

/* SYMBIAN's standard C (libc) does not support these defines
   but if Nokia's Open C SDK is installed and used it does support these defines */
#if (RV_NET_TYPE&RV_NET_IPV6 && defined(IPPROTO_IPV6) && defined(IPV6_JOIN_GROUP))
    case RV_ADDRESS_TYPE_IPV6:
        {
            RV_IPV6_MREQ mreq;
            const RvAddressIpv6* multicastIpv6Addr;
            const RvUint8* multicastIp;

            /* Get the IP address of each one of the addresses */
            multicastIpv6Addr = RvAddressGetIpv6(multicastAddress);
            if (multicastIpv6Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            multicastIp = RvAddressIpv6GetIp(multicastIpv6Addr);


            /* Set the parameter for the OS */
            memcpy(&(mreq.ipv6mr_multiaddr), multicastIp, sizeof(mreq.ipv6mr_multiaddr));
            mreq.ipv6mr_interface = interfaceAddress->data.ipv6.scopeId;

            if (RvSocketSetSockOpt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *)&mreq, sizeof(mreq)) != 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

            RvSockLogDebug((&logMgr->socketSource,
                "RvSocketJoinMulticastGroup(sock=%d) mreq.ipv6mr_interface=%d, status=%d",
                *sock,mreq.ipv6mr_interface,status));

        }
        break;
#endif

    default:
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
    }

    {
        RvChar addr1[RV_ADDRESS_MAXSTRINGSIZE], addr2[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(multicastAddress, sizeof(addr1), addr1);
        RvAddressGetString(interfaceAddress, sizeof(addr2), addr2);
        if (status == RV_OK)
        {
            RvSockLogLeave((&logMgr->socketSource,
                "RvSocketJoinMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s)=0",
                *sock, addr1, addr2));
        }
        else
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketJoinMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s,errno=%d)=%d",
                *sock, addr1, addr2, RvSocketErrNo, status));
        }
    }

    return status;
}


/********************************************************************************************
 * RvSocketLeaveMulticastGroup
 * Leave a multicast group
 * This function is only needed if the stack being ported supports multicast receiving.
 * This function is thread-safe.
 * INPUT   : sock             - Socket to modify
 *           multicastAddress - Multicast address to leave
 *           interfaceAddress - Interface address to use on the local host.
 *                              Setting this to NULL chooses an arbitrary interface
 *           logMgr           - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketLeaveMulticastGroup(
    IN RvSocket*    sock,
    IN RvAddress*   multicastAddress,
    IN RvAddress*   interfaceAddress,
    IN RvLogMgr*    logMgr)
{
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (multicastAddress == NULL) || (interfaceAddress == NULL))
    {
        RvChar addr1[RV_ADDRESS_MAXSTRINGSIZE], addr2[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(multicastAddress, sizeof(addr1), addr1);
        RvAddressGetString(interfaceAddress, sizeof(addr2), addr2);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketLeaveMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s)=%d",
            ((sock == NULL) ? 0 : *sock), addr1, addr2, RvSocketErrorCode(RV_ERROR_BADPARAM)));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    {
        RvChar addr1[RV_ADDRESS_MAXSTRINGSIZE], addr2[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(multicastAddress, sizeof(addr1), addr1);
        RvAddressGetString(interfaceAddress, sizeof(addr2), addr2);
        RvSockLogEnter((&logMgr->socketSource,
                "RvSocketLeaveMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s)",
                *sock, addr1, addr2));
    }

    switch (multicastAddress->addrtype)
    {
    case RV_ADDRESS_TYPE_IPV4:
        {
            RV_IP_MREQ mreq;
            const RvAddressIpv4* multicastIpv4Addr;
            const RvAddressIpv4* interfaceIpv4Addr;
            RvUint32 multicastIp;
            RvUint32 interfaceIp = 0;

            /* Get the IP address of each one of the addresses */
            multicastIpv4Addr = RvAddressGetIpv4(multicastAddress);
            if (multicastIpv4Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            multicastIp = RvAddressIpv4GetIp(multicastIpv4Addr);

            interfaceIpv4Addr = RvAddressGetIpv4(interfaceAddress);
            if (interfaceIpv4Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            interfaceIp = RvAddressIpv4GetIp(interfaceIpv4Addr);

            /* Set the parameter for the OS */
#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
            mreq.imr_multiaddr = multicastIp;
            mreq.imr_interface = interfaceIp;
#else
            mreq.imr_multiaddr.s_addr = multicastIp;
            mreq.imr_interface.s_addr = interfaceIp;
#endif

            if (RvSocketSetSockOpt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) != 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
        break;

/* SYMBIAN's standard C (libc) does not support these defines
   but if Nokia's Open C SDK is installed and used it does support these defines */
#if (RV_NET_TYPE&RV_NET_IPV6 && defined(IPPROTO_IPV6) && defined(IPV6_LEAVE_GROUP))
    case RV_ADDRESS_TYPE_IPV6:
        {
            RV_IPV6_MREQ mreq;
            const RvAddressIpv6* multicastIpv6Addr;
            const RvAddressIpv6* interfaceIpv6Addr;
            const RvUint8* multicastIp;
            const RvUint8* interfaceIp = 0;

            /* Get the IP address of each one of the addresses */
            multicastIpv6Addr = RvAddressGetIpv6(multicastAddress);
            if (multicastIpv6Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            multicastIp = RvAddressIpv6GetIp(multicastIpv6Addr);

            interfaceIpv6Addr = RvAddressGetIpv6(interfaceAddress);
            if (interfaceIpv6Addr == NULL)
            {
                status = RvSocketErrorCode(RV_ERROR_BADPARAM);
                break;
            }
            interfaceIp = RvAddressIpv6GetIp(interfaceIpv6Addr);

            /* Set the parameter for the OS */
            memcpy(&(mreq.ipv6mr_multiaddr), multicastIp, sizeof(mreq.ipv6mr_multiaddr));
            mreq.ipv6mr_interface = *interfaceIp;

            if (RvSocketSetSockOpt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char *)&mreq, sizeof(mreq)) != 0)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
        break;
#endif

    default:
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
    }

    {
        RvChar addr1[RV_ADDRESS_MAXSTRINGSIZE], addr2[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(multicastAddress, sizeof(addr1), addr1);
        RvAddressGetString(interfaceAddress, sizeof(addr2), addr2);
        if (status == RV_OK)
        {
            RvSockLogLeave((&logMgr->socketSource,
                "RvSocketLeaveMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s)=0",
                *sock, addr1, addr2));
        }
        else
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketLeaveMulticastGroup(sock=%d,multicastAddress=%s,interfaceAddress=%s,errno=%d)=%d",
                *sock, addr1, addr2, RvSocketErrNo, status));
        }
    }

    return status;
}



/* =========================== */
/* ==== Utility functions ==== */
/* =========================== */


/********************************************************************************************
 * RvSocketGetBytesAvailable
 * Get the number of bytes that are in the receive buffer of the socket.
 * This number might be larger than the return result of a single recv() operation.
 * This is important when dealing with data-gram types of connections (such as UDP).
 * This function is not available by some of the operating systems.
 * This function is NOT thread-safe.
 * INPUT   : socket         - Socket to check
 *           logMgr         - log manager instance
 * OUTPUT  : bytesAvailable - Number of bytes in the receive buffer of the socket
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketGetBytesAvailable(
    IN RvSocket*  sock,
    IN RvLogMgr*  logMgr,
    OUT RvSize_t* bytesAvailable)
{
    RvSocket ourSocket;
    RvStatus status = RV_OK;
    size_t numBytes = 0;


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (bytesAvailable == NULL))
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketGetBytesAvailable(sock=%d,logMgr=%p,bytesAvailable=%p)",
        *sock, logMgr, bytesAvailable));

    *bytesAvailable = 0;

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK) {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketGetBytesAvailable RvSocketSharerShare error"));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    if (NU_Is_Connected(ourSocket) == NU_TRUE)
         /* Assume we have a byte waiting on this socket */
        numBytes = 1;

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    status = RvSymSockGetBytesAvailable(sock,bytesAvailable);
#elif (RV_OS_TYPE != RV_OS_TYPE_INTEGRITY)
    if (ioctl(ourSocket, FIONREAD, (RV_IOCTL_ARGP_TYPE) &numBytes) < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#else
    status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif

    if (status == RV_OK)
        *bytesAvailable = (RvSize_t)numBytes;

    RvSocketSharerClose(&ourSocket);

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketGetBytesAvailable, %d bytes available",*bytesAvailable));

    return status;
}

/********************************************************************************************
* RvSocketGetSendQueueSize
* Get the number of bytes in the send queue of the socket.
* At the moment the function is supported for Linux and NetBSD only.
* INPUT   : socket         - Socket to check
*           logMgr         - log manager instance
* OUTPUT  : sendQueueSz    - Number of bytes in the send queue of the socket
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI
RvStatus RVCALLCONV RvSocketGetSendQueueSize(
    IN RvSocket*  sock,
    IN RvLogMgr*  logMgr,
    OUT RvSize_t* sendQueueSz)
{
    RvStatus status = RV_OK;
    size_t numBytes = 0;


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (sendQueueSz == NULL))
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketGetSendQueueSize(sock=%d,logMgr=%p,bytesAvailable=%p)",
        *sock, logMgr, sendQueueSz));

    *sendQueueSz = 0;

#if defined(TIOCOUTQ) || defined(FIONWRITE)

#   ifdef TIOCOUTQ
#   define MY_WRITE_OPT TIOCOUTQ
#else
#   define MY_WRITE_OPT FIONWRITE
#   endif      

    if (ioctl(*sock, MY_WRITE_OPT, (RV_IOCTL_ARGP_TYPE) &numBytes) < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#else
    status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif

    if (status == RV_OK)
        *sendQueueSz = (RvSize_t)numBytes;

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketGetSendQueueSize, %d bytes available, status %d",*sendQueueSz,status));

    return status;

}



/********************************************************************************************
 * RvSocketGetLastError
 * Get the last error that occured on a socket.
 * This function works for synchronous sockets only.
 * The return value is OS-specific. A value of 0 means no error occured.
 * This function is thread-safe.
 * INPUT   : sock     - Socket to check
 *           logMgr         - log manager instance
 * OUTPUT  : lastError  - Error that occured. 0 means no error.
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketGetLastError(
    IN RvSocket* sock,
    IN RvLogMgr* logMgr,
    OUT RvInt32* lastError)
{
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (lastError == NULL))
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketGetLastError(sock=%d,logMgr=%p,lastError=%p)",
        *sock, logMgr, lastError));

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    status = RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    status = RvSymSockGetLastError(sock,lastError);

#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)
    /* WinCE does not support SO_ERROR */
    {
        RvInt8 Name[RV_SOCKET_SOCKADDR_SIZE];
        int NameSize = RV_SOCKET_SOCKADDR_SIZE;

        if (getpeername(*sock, (struct sockaddr *)&Name, &NameSize) < 0)
        {
            *lastError = RvSocketErrNo;

            /* If the socket is not yet connected - it is a valid error */
            if (*lastError != WSAENOTCONN)
                status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }

        *lastError = 0;
    }
#else
    {
        int len;
        int socketError = 0;
        RvSocket ourSocket;

        len = sizeof(socketError);

        status = RvSocketSharerShare(logMgr, sock, &ourSocket);
        if (status != RV_OK)
        {
            *lastError = 0;
            RvSockLogError((&logMgr->socketSource,
                "RvSocketGetLastError - RvSocketSharerShare failure"));
            return status;
        }

        if (GET_SOCK_OPT(ourSocket, SOL_SOCKET, SO_ERROR, (char*)&socketError, (SOCKLEN_TYPE *)&len) < 0)
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

        *lastError = (RvInt32)socketError;
        RvSocketSharerClose(&ourSocket);
    }
#endif

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketGetLastError, last error = %d",*lastError));

    return status;
}


/********************************************************************************************
 * RvSocketGetLocalAddress
 * Get the local address used by this socket.
 * This function is thread-safe.
 * INPUT   : sock     - Socket to check
 *           logMgr   - log manager instance
 * OUTPUT  : address    - Local address
 *                        Must be destructed by the caller to this function
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketGetLocalAddress(
    IN RvSocket  *sock,
    IN RvLogMgr  *logMgr,
    OUT RvAddress* address)
{
    RvSocket ourSocket;
    RvStatus status;
#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
    rvASockAddr u;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    if(address == NULL)
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketGetLocalAddress(sock=%d,logMgr=%p,address=%p)",
        *sock, logMgr, address));

#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
    memset(&u, 0, sizeof(u));
#endif
    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK) {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketGetLocalAddress - RvSocketSharerShare failure sock=%d not shared", *sock));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    {
        RvInt16 socklen = RV_SOCKET_SOCKADDR_SIZE;
        struct sockaddr_struct *temp = (struct sockaddr_struct *)u.sockdata;

        if((NU_Get_Sock_Name(ourSocket, temp, &socklen)) == 0)
        {
            /* we cant use RvSocketSockAddrToAddress() here, since NU_Get_Sock_Name()
               does not return address family type */
            /* nucleus doesn't support IPv6 */
            address->addrtype = RV_ADDRESS_TYPE_IPV4;
            address->data.ipv4.port = temp->port_num;
            memcpy(&address->data.ipv4.ip, temp->ip_num.is_ip_addrs, 4);
        }
        else
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    status = RvSymSockGetLocalAddress(sock,address);

#else
    {
        int socklen = RV_SOCKET_SOCKADDR_SIZE;

        if ((getsockname(ourSocket, RVSOCKADDR(u), (SOCKLEN_TYPE *) &socklen)) == 0)
            status = RvSocketSockAddrToAddress(RVSOCKDATA(u), socklen, address);
        else
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#endif

    RvSocketSharerClose(&ourSocket);

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketGetLocalAddress(sock=%d,logMgr=%p,address=%p)",
        *sock, logMgr, address));

    return status;
}


/********************************************************************************************
 * RvSocketGetAddressType
 * Get the address type used by this socket.
 * This function is thread-safe.
 * INPUT   : sock     - Socket to check
 *           logMgr   - log manager instance
 * OUTPUT  : address    - Local address
 *                        Must be destructed by the caller to this function
 * RETURN  : RV_OK on success, other on failure
 */

static
RvStatus RvSocketGetAddressType(IN RvSocket *s, IN RvLogMgr *logMgr, OUT RvInt *addrType)
{
    RvAddress sockAddr;
    RvStatus status;

    status = RvSocketGetLocalAddress(s, logMgr, &sockAddr);
    if(status != RV_OK)
    {
        return status;
    }

    *addrType = sockAddr.addrtype;
    return RV_OK;
}


/********************************************************************************************
 * RvSocketGetRemoteAddress
 * Get the remote address used by this socket.
 * This function can only be called only on TCP sockets.
 * This function is thread-safe.
 * INPUT   : sock     - Socket to check
 *           logMgr   - log manager instance
 * OUTPUT  : address  - Remote address
 *                      Must be destructed by the caller to this function
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketGetRemoteAddress(
    IN RvSocket  *sock,
    IN RvLogMgr  *logMgr,
    OUT RvAddress* address)
{
    RvSocket ourSocket;
    RvStatus status;
#if (RV_SOCKET_TYPE != RV_SOCKET_SYMBIAN)
    rvASockAddr u;

    memset(&u, 0, sizeof(u));
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(sock == NULL)
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    if(address == NULL)
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketGetRemoteAddress(sock=%d,logMgr=%p,address=%p)",
        *sock, logMgr, address));

    status = RvSocketSharerShare(logMgr, sock, &ourSocket);
    if (status != RV_OK) {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketGetRemoteAddress - RvSocketSharerShare failure"));
        return status;
    }

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS)
    {
        RvInt16 socklen = RV_SOCKET_SOCKADDR_SIZE;
        struct sockaddr_struct *temp = (struct sockaddr_struct *)u.sockdata;

        if(NU_Get_Peer_Name(ourSocket, temp, (RvInt16*)&socklen) == 0)
        {
            /* we cant use RvSocketSockAddrToAddress() here, since NU_Get_Peer_Name()
               does not return address family type */
            /* nucleus doesn't support IPv6 */
            address->addrtype = RV_ADDRESS_TYPE_IPV4;
            address->data.ipv4.port = temp->port_num;
            memcpy(&address->data.ipv4.ip, temp->ip_num.is_ip_addrs, 4);
        }
        else
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

#elif (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    status = RvSymSockGetRemoteAddress(sock,address);

#else
    {
        SOCKLEN_TYPE socklen = RV_SOCKET_SOCKADDR_SIZE;

        if (getpeername(ourSocket, RVSOCKADDR(u), &socklen) == 0)
            status = RvSocketSockAddrToAddress(RVSOCKDATA(u), socklen, address);
        else
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
#endif

    RvSocketSharerClose(&ourSocket);

    RvSockLogLeave((&logMgr->socketSource,
        "RvSocketGetRemoteAddress(sock=%d,logMgr=%p,address=%p)",
        *sock, logMgr, address));

    return status;
}




/* ====== Nucleus specific functions ====== */

#if (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS && !HAS_NONBLOCKEDCONNECT)

void RvSocketNucleusTaskDelete(IN RvSocket* socket)
{
    if (nuConInfo[*socket].threadState == RvThreadState_Stopped)
    {
        RvThreadDestruct(&(nuConInfo[*socket].threadId));
        nuConInfo[*socket].threadState = RvThreadState_Idle;
    }
    /* enable task-switching for our new thread */
    RvThreadNanosleep(RV_TIME64_NSECPERMSEC * 50, NULL);
}

/********************************************************************************************
 * RvSocketNucleusTaskClean
 * destruct task TCP socket connecting/closing task
 * This function is thread-safe.
 * INPUT   : SocketHandle  - handle to socket, managed by the task.
 * OUTPUT  : none
 * RETURN  : none
 */
void RvSocketNucleusTaskClean(
    IN RvSocket *socket)
{
    RV_SOCKET_LOCK();

    /* check thread state; destruct the thread if:
       1) the thread has been created, and executed succesfully, in which case clean the object.*/
    if (nuConInfo[*socket].threadState == RvThreadState_Stopped)
        RvThreadDestruct(&(nuConInfo[*socket].threadId));

    /* 2) the thread has been executed, but hangs on the blocking call to NU_connect() or
          NU_close(), in which case the timer expiration call should clean this task. */
    if (nuConInfo[*socket].threadState == RvThreadState_Running)
    {
        /* check if this connection hangs - occurs in the second option, when a call to NU_connect()
           hangs... - close socket and destruct that thread */
        if ((   (nuConInfo[*socket].socketState == RvSocketState_Connecting) &&
                /* just to make sure context switching hasn't occured in a nasty critical timming */
                /* if connect() has succeded after all - get outa here */
                (NU_Is_Connected(*socket) != RV_TRUE)) ||
            (nuConInfo[*socket].socketState == RvSocketState_Closing))
        {
            NU_Abort(*socket);
        }

        RvThreadDestruct(&(nuConInfo[*socket].threadId));
    }

    nuConInfo[*socket].threadState = RvThreadState_Idle;

    RV_SOCKET_UNLOCK();
}


/********************************************************************************************
 * RvSocketNucleusEventsTaskConnect
 * used as socket management thread function for NUCLEUS.
 * In NUCLEUS, connect and close operation on TCP socket may
 * block caller for long time interval.
 * This task is thread-safe.
 * INPUT   : data - socket handler
 * OUTPUT  : none
 * RETURN  : none
 */
static void RvSocketNucleusEventsTaskConnect(
    IN RvThread *th,
    IN void* data)
{
    RvStatus status;
    RvSocket socket = (RvSocket)data;
    RvSelectEngine* selectEngine;
    RvSelectFd* selectFd = NULL;
    RvLogMgr* logMgr = NULL;

    selectEngine = nuConInfo[socket].selectEngine;
    if (selectEngine != NULL)
    {
        selectFd = RvSelectFindFd(selectEngine, &socket);
        logMgr = selectEngine->logMgr;
    }

    RV_SOCKET_LOCK();

    nuConInfo[socket].threadState = RvThreadState_Running;

    if(nuConInfo[socket].timerInitiated)
    {
        RvTimerCancel(&(nuConInfo[socket].timer), RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        nuConInfo[socket].timerInitiated = RV_FALSE;
    }

    if (nuConInfo[socket].socketState == RvSocketState_Connecting)
    {
        struct sockaddr_in saddr;
        int saddrLen;

        RvSocketAddressToSockAddr(&(nuConInfo[socket].sockAddress),
            (RV_SOCKADDR_PTR)&saddr, &saddrLen);

        RV_SOCKET_UNLOCK();
        RvSocketSetBlocking(&socket, RV_TRUE, logMgr);

        /* Connect... */
        status = NU_Connect(socket, &saddr, saddrLen);

        RvSocketSetBlocking(&socket, RV_FALSE, logMgr);

        RV_SOCKET_LOCK();
        if (status >= 0)
        {
            /* We're connected - set the NODELAY option on */
            int yes = RV_TRUE;
            if (RvSocketSetSockOpt(&socket, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(yes)) != 0)
            {
                RvSockLogError((&logMgr->socketSource,
                    "RvSocketNucleusEventsTaskConnect: Failed to setsockopt(TCP_NODELAY) %d",
                    RvSocketErrNo));
            }

            nuConInfo[socket].socketState = RvSocketState_Connected;
        }
        else
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketNucleusEventsTaskConnect: Failed to connect with status %d",
                status));

            nuConInfo[socket].socketState = RvSocketState_Idle;
        }

        RvSelectSimulateEvent(selectEngine, selectFd, RV_SELECT_WRITE);
    }

    nuConInfo[socket].threadState = RvThreadState_Stopped;

    RV_SOCKET_UNLOCK();
}


/********************************************************************************************
 * RvSocketNucleusEventsTaskClose
 * used as socket management thread function for NUCLEUS.
 * In NUCLEUS, connect and close operation on TCP socket may
 * block caller for long time interval.
 * This task is thread-safe.
 * INPUT   : data - socket handler
 * OUTPUT  : none
 * RETURN  : none
 */
static void RvSocketNucleusEventsTaskClose(
    IN RvThread *th,
    IN void* data)
{
    RvSocket socket = (RvSocket)data;
    RvSelectEngine* selectEngine;
    RvSelectFd* selectFd = NULL;
    RvLogMgr* logMgr = NULL;

    selectEngine = nuConInfo[socket].selectEngine;
    if (selectEngine != NULL)
    {
        selectFd = RvSelectFindFd(selectEngine, &socket);
        logMgr = selectEngine->logMgr;
    }

    RV_SOCKET_LOCK();

    nuConInfo[socket].threadState = RvThreadState_Running;

    if(nuConInfo[socket].timerInitiated)
    {
        RvTimerCancel(&(nuConInfo[socket].timer), RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        nuConInfo[socket].timerInitiated = RV_FALSE;
    }

    if (nuConInfo[socket].socketState == RvSocketState_Closing)
    {
        RV_SOCKET_UNLOCK();

        RvSocketSetBlocking(&socket, RV_TRUE, logMgr);
        CLOSE_SOCKET(socket);

        if (selectEngine != NULL)
        {
            RV_SOCKET_LOCK();
            nuConInfo[socket].socketState = RvSocketState_Closed;
            RV_SOCKET_UNLOCK();

            RvSelectSimulateEvent(selectEngine, selectFd, RV_SELECT_READ);
        }

        RV_SOCKET_LOCK();
    }

    nuConInfo[socket].threadState = RvThreadState_Stopped;

    RV_SOCKET_UNLOCK();
}


/********************************************************************************************
 * RvSocketSetSelectEngine
 * Update the socket database - associate a socket with it's select-engine.
 * This function is thread-safe.
 * INPUT   : socket       - Socket to check
 *           selectEngine - select-engine that probes the socket.
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSocketSetSelectEngine(
    IN RvSocket *socket,
    IN void* selectEngine)
{
    RvLogMgr *logMgr;

    RV_SOCKET_LOCK();
    nuConInfo[*socket].selectEngine = (RvSelectEngine *)selectEngine;
    logMgr = nuConInfo[*socket].selectEngine->logMgr;
    RV_SOCKET_UNLOCK();

    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketSetSelectEngine(socket=%d, selectEngine=%d).", *socket, (RvSelectEngine *)selectEngine));

    return RV_OK;
}


/********************************************************************************************
 * RvSocketNucleusEvTimerExp
 * If the connection or closing timer has expired, just cancel the timer,
 * and close the socket.
 * This function is thread-safe.
 * INPUT   : argument       - Socket to check
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other on failure
 */
static RvBool RvSocketNucleusEvTimerExp(
    IN void* argument)
{
    RvSocket socket = (RvSocket)argument;
    RvSocketState sockState = nuConInfo[socket].socketState;
    RvLogMgr *logMgr;

    (void)sockState;
    RV_SOCKET_LOCK();

    /* cancel this timer. we don't want it to repeat */
    if(nuConInfo[socket].timerInitiated)
    {
        RvTimerCancel(&(nuConInfo[socket].timer), RV_TIMER_CANCEL_DONT_WAIT_FOR_CB);
        nuConInfo[socket].timerInitiated = RV_FALSE;
    }

    logMgr = nuConInfo[socket].selectEngine->logMgr;

    RV_SOCKET_UNLOCK();

    /* If the event-handling thread hasn't reached normal execution - kill it,
       and discard the socket */
    if(nuConInfo[socket].threadState != RvThreadState_Stopped)
    {
        RvSocketNucleusTaskClean(&socket);
        NU_Abort(socket);
    }

    RvSockLogDebug((&logMgr->socketSource,
        "RvSocketNucleusEvTimerExp(socket=%d): %s timer has expired",
        socket, (sockState == RvSocketState_Connecting) ? "Connect-Socket" : "Close-Socket"));

    return RV_TRUE;
}


/********************************************************************************************
 * RvSocketNucleusSocketClean
 * Clean any objects of an old socket.
 * This function is thread-safe.
 * INPUT   : socket       - Socket to check
 * OUTPUT  : None.
 * RETURN  : None.
 */
static void RvSocketNucleusSocketClean(
    IN RvSocket socket)
{
    /* even though it is a new socket - if this socket is not idle, there is a
       selectEngine that has already been assigned to wait on this socket before, so
       lets clean all its old objects */

    RvSocketNucleusTaskClean(&socket);

    RV_SOCKET_LOCK();

    memset(&nuConInfo[socket], 0, sizeof(connectionInfo));

    RV_SOCKET_UNLOCK();
}


STATUS RvNuIsConnected(IN RvSocket socket)
{
    return nuConInfo[socket].socketState == RvSocketState_Connected ? NU_TRUE : NU_FALSE;
}

#endif /* (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS) */




RVCOREAPI
RvBool RVCALLCONV  RvSocketIsTypeOfServiceSupported(void)
{
    RV_USE_CCORE_GLOBALS;
    return tosSupported;
}

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)


#else

#define CheckTosSupportOther CheckTosSupport

/*static*/
void CheckTosSupportOther(void)
{
    RV_USE_CCORE_GLOBALS;
#if (RV_SOCKET_TYPE == RV_SOCKET_SYMBIAN)
    tosSupported = CheckTosSupportOtherSym();
/*#elif defined(IP_TOS) && defined(IPPROTO_IP) 
         IPPROTO_IP is enum on some systems and this causes 
         to UNSUPPORT TOS decision
 */
#elif defined(IP_TOS)
    {
        RvSocket dummy;
        RvStatus s;
        int tosVal = 0;

        RvSocketConstruct(RV_ADDRESS_TYPE_IPV4, RvSocketProtocolUdp, 0, &dummy);
        s = RvSocketSetSockOpt(&dummy, IPPROTO_IP, IP_TOS, (char *)&tosVal, sizeof(tosVal));
        tosSupported = (s == RV_OK);
        RvSocketDestruct(&dummy, RV_FALSE, NULL, NULL);
    }
#else /* defined(IP_TOS) && defined(IPPROTO_IP) */
    tosSupported = RV_FALSE;
#endif
}

#endif /* RV_OS_TYPE == RV_OS_TYPE_WIN32 */

#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)

int RvSocketMopiSetConnectionId(RvSocket socket, RvUint16 cid)
{
    return Mmb_SocketSetConnectionId(socket, cid);
}

#endif

#if (RV_NET_TYPE & RV_NET_SCTP)

/********************************************************************************************
 * RvSocketSctpBind
 * Bind a SCTP socket to a series of local IP/port addresses.
 * The port number must be the same to all addresses.
 * This function can be used also to remove some addresses from the socket list
 * INPUT   : sock       - Socket to bind
 *           addresses  - List of addresses to bind socket to
 *           nAddresses - Number of addresses in the list
 *           operation  - Must be either RvSctpBindAdd or RvSctpBindRemove
 *           logMgr     - log manager instance
 * RETURN  : RV_OK on success, other on failure.
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSctpBind(
    IN RvSocket*         sock,
    IN RvAddress*        addresses,
    IN RvSize_t          nAddresses,
    IN RvSctpBindOp      operation,
    IN RvLogMgr*         logMgr)

#undef FUNC
#define FUNC "RvSocketSctpBind"
{
    RvStatus status = RV_OK;
    int socklen;
    RvSize_t addrIdx = 0;
    int dsock;   /* holds socket descriptor, *sock */
    union {
        struct sockaddr_in dummy;
        RvUint8 sockdata[RV_SOCKET_MAX_ADDRESSES * RV_SOCKET_SOCKADDR_SIZE];
    } u;
    RvUint8* sockdataPtr = u.sockdata;
    RvAddress *curAddr;


#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (sock == NULL || addresses == NULL)
    {
        RvSockLogError((SRCFUNC "(sock=%d,addr=%p): unexpected null pointer",
			((sock == NULL) ? 0 : *sock), addresses));
        return RV_ERROR_NULLPTR;
    }
#endif

#if RV_CHECK_MASK & RV_CHECK_RANGE
    if(nAddresses > RV_SOCKET_MAX_ADDRESSES) {

        RvSockLogError((SRCFUNC "(sock=%d): number of addresses (%d) exceeds RV_SOCKET_MAX_ADDRESSES (%d)",
			((sock == NULL) ? 0 : *sock), nAddresses, RV_SOCKET_MAX_ADDRESSES));
        return RV_ERROR_OUTOFRANGE;
    }
#endif

#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(addresses, sizeof(addr), addr);
        RvSockLogEnter((SRCFUNC "(socket=%d,address=%s:%d,scopeId=%d)",
            *sock, addr, RvAddressGetIpPort(addresses), RvAddressGetIpv6Scope(addresses)));
    }
#endif

    dsock = *sock;

    if(nAddresses == 0) {
        RvSockLogError((SRCFUNC "(sock=%d): no addresses were specified", dsock));
        return RV_ERROR_OUTOFRANGE;
    }


    if(operation == RvSctpBind) {
        /* According to specification regular 'bind' is needed before calling sctp_bindx,
         * but at least on linux 'bind' on sctp socket fails, while on solaris 'sctp_bindx'
         * fails if 'bind' wasn't called. So, RV_SCTP_BIND_NEEDED is used to indicate whether or not
         * we should call 'bind' before calling 'sctp_bindx'
         */
#ifdef RV_SCTP_BIND_NEEDED
        status = RvSocketBind(sock, &addresses[0], 0, logMgr);
        if(status != RV_OK) {
            return status;
        }
        addrIdx = 1;
        nAddresses--;
#endif
        operation = RvSctpBindAdd;

    }


    nAddresses = RvMin(nAddresses,RV_SOCKET_MAX_ADDRESSES);

    for(curAddr = &addresses[addrIdx]; addrIdx < nAddresses; addrIdx++, curAddr++)
    {
        status = RvSocketAddressToSockAddr(curAddr, (RV_SOCKADDR_PTR)sockdataPtr, &socklen);
        if(status != RV_OK) {
            RvSockLogError((SRCFUNC "(%d) failed to convert RvAddress to sock_addr", dsock));
            return status;
        }


#if (RV_NET_TYPE & RV_NET_IPV6)
        if (curAddr->addrtype == RV_ADDRESS_TYPE_IPV4) {
            sockdataPtr += RV_SOCKET_SOCKADDR4_SIZE;
        } else {
            sockdataPtr += RV_SOCKET_SOCKADDR_SIZE;
        }
#else
        sockdataPtr += RV_SOCKET_SOCKADDR_SIZE;
#endif
    }

    if((nAddresses > 0) && (sctp_bindx(*sock, (RV_SOCKADDR_PTR)u.sockdata, nAddresses, (int)operation) != 0)) {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
        {
            RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
            RvAddressGetString(addresses, sizeof(addr), addr);
            RvSockLogError((SRCFUNC "(sock=%d,addr=%s:%d,scopeId=%d), sctp_bindx failed, errno=%d",
                            dsock, addr, RvAddressGetIpPort(addresses), RvAddressGetIpv6Scope(addresses),
                            RvSocketErrNo));
        }
#endif
        return status;
    }

#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(addresses, sizeof(addr), addr);
        RvSockLogLeave((SRCFUNC "(sock=%d,addr=%s:%d,scopeId=%d)=0",
            *sock, addr, RvAddressGetIpPort(addresses), RvAddressGetIpv6Scope(addresses)));
    }
#endif


    return status;
}


/********************************************************************************************
 * RvSocketSctpAccept
 * Accept an incoming socket connect request, creating a new socket object.
 * In blocking mode, this function blocks until an incoming connect request to this
 * socket is made.
 * In non-blocking mode, this function will exit immediately and when an incoming
 * connection request to this socket is made, this socket will receive the
 * RvSocketAccept event in the select module.
 * The newSocket object should be regarded as if it was created using
 * RvSocketConstruct().
 * This function needs to be ported only if TCP is used by the stack.
 * This function is thread-safe.
 * INPUT   : socket         - Listening socket receiving the incoming connection
 *           logMgr         - log manager instance
 * OUTPUT  : newSocket      - Accepted socket information
 *           remoteAddress  - Address of remote side of connection
 *                            Can be passed as NULL if not needed
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSctpAccept(
    IN  RvSocket*    sock,
    IN  RvLogMgr*    logMgr,
    OUT RvSocket*    newSocket,
    OUT RvAddress*   remoteAddress)
{
    RvStatus status;
    rvASockAddr u;
    SOCKLEN_TYPE socklen = RV_SOCKET_SOCKADDR_SIZE;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((sock == NULL) || (newSocket == NULL))
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        status = RvSocketErrorCode(RV_ERROR_BADPARAM);
        RvAddressGetString(remoteAddress, sizeof(addr), addr);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpAccept(sock=%d,new=%d,addr=%s:%d,errno=%d)=%d",
            ((sock == NULL) ? 0 : *sock), ((newSocket == NULL) ? 0 : *newSocket),
			addr, RvAddressGetIpPort(remoteAddress), RvSocketErrNo, status));
        return status;
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSctpAccept(socket=%d,newSocket=%p,remoteAddress=%p)",
        *sock, newSocket, remoteAddress));

    if ((*newSocket = accept(*sock, RVSOCKADDR(u), &socklen)) < 0)
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);

    else
    {
        int yes = RV_TRUE;

        /* Disable Nagle algorithm - we want our messages to be sent as soon as possible */
        status = RvSocketSctpSockOpt(sock, RvSctpOptionSet, RV_SCTP_NODELAY, &yes, sizeof(yes), logMgr);
        if (status != RV_OK)
        {
            RvSockLogError((&logMgr->socketSource,
                "RvSocketSctpAccept: Failed to setsockopt(RV_SCTP_NODELAY) %d on socket %d", RvSocketErrNo, *sock));
        }

        /* We're working with non-blocking sockets */
        RvSocketSetBlocking(newSocket, RV_FALSE, logMgr);

        status = RvSocketSockAddrToAddress(RVSOCKDATA(u), socklen, remoteAddress);
    }


#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(remoteAddress, sizeof(addr), addr);
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSctpAccept(sock=%d,new=%d,addr=%s:%d)=0",
            *sock, *newSocket, addr, RvAddressGetIpPort(remoteAddress)));
    }
#endif

    if (status != RV_OK)
    {
        RvSockLogWarning((&logMgr->socketSource,
            "RvSocketSctpAccept(sock=%d,new=%d,errno=%d)=%d", *sock, *newSocket, RvSocketErrNo, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketSctpSend
 * Send a buffer on a SCTP socket.
 * This function actually copies the given buffer (pointed to by the messageInfo param)
 * to the operating system's memory for later sending.
 * In blocking mode, this function will block until the buffer is sent to the remote
 * side or upon failure.
 * In non-blocking mode, this function will return immediately, indicating the exact
 * amount of bytes the operating system has processed and sent.
 * INPUT   : socket         - Socket to send the buffer on
 *           messageInfo    - Of type RvSctpMessageInfo indicating the buffer to send,
 *                            its length, the stream number to use and flags
 *           logMgr         - log manager instance
 * OUTPUT  : bytesSent      - Number of bytes the have been sent
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSctpSend(
    IN  RvSocket*           sock,
    IN  RvSctpMessageInfo*  messageInfo,
    IN  RvLogMgr*           logMgr,
    OUT RvSize_t*           bytesSent)
{
    RvStatus status = RV_OK;
    int errLen;
    int socketError = 0;
    struct msghdr outmessage;
    struct iovec iov;
    union {
        struct cmsghdr _cmsg;
        char _outcmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
    } u_outcmsg;
    struct cmsghdr* cmsg;
    struct sctp_sndrcvinfo *sinfo;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (sock == NULL || messageInfo == NULL)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpSend(sock=%d,messageInfo=%p)=%d",
            ((sock == NULL) ? 0 : *sock), messageInfo, status));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSctpSend(sock=%d,messageInfo=%p,logMgr=%p,bytesSent=%p)",
        *sock,messageInfo,logMgr,bytesSent));

    /* First, initialize the iov struct */
    iov.iov_base = messageInfo->buffer;
    iov.iov_len  = messageInfo->bufferLength;

    cmsg = (struct cmsghdr *)(u_outcmsg._outcmsg);
    cmsg->cmsg_level = IPPROTO_SCTP;
    cmsg->cmsg_type = SCTP_SNDRCV;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

    sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
    memset(sinfo, 0, sizeof(struct sctp_sndrcvinfo));
    sinfo->sinfo_stream = messageInfo->streamNum;
    sinfo->sinfo_flags  = messageInfo->flags;

    memset(&outmessage, 0, sizeof(outmessage));
    outmessage.msg_iov        = &iov;
    outmessage.msg_iovlen     = 1;
    outmessage.msg_control    = cmsg;
    outmessage.msg_controllen = cmsg->cmsg_len;

#ifdef MSG_NOSIGNAL
    errLen = sendmsg(*sock, &outmessage, MSG_NOSIGNAL);
#else
    errLen = sendmsg(*sock, &outmessage, 0);
#endif

    if (errLen <= 0)
    {
        /* We had an error - check to see if it's WOULDBLOCK */
        socketError = RvSocketErrNo;
        if (socketError == EWOULDBLOCK)
            status = RvSocketErrorCode(RV_ERROR_TRY_AGAIN);
        else if (socketError == ECONNRESET)
            status = RvSocketErrorCode(RV_ERROR_NETWORK_PROBLEM);
        else
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        errLen = 0;  /* Update the number of bytes we sent to none */
    }

    if (bytesSent != NULL)
        *bytesSent = errLen;


    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSctpSend(sock=%d,buf=%p,len=%d,sent=%d)=0",
            *sock, messageInfo->buffer, messageInfo->bufferLength, errLen));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpSend(sock=%d,buf=%p,len=%d,errno=%d)=%d",
            *sock, messageInfo->buffer, messageInfo->bufferLength, RvSocketErrNo, status));
    }

    return status;
}


/********************************************************************************************
 * RvSocketSctpReceive
 * Receive a buffer from a SCTP socket.
 * In blocking mode, this function will block until a buffer is received on the
 * socket.
 * In non-blocking mode, this function returns immediately, even if there is nothing
 * to be received on this socket at the moment. This function is usually called after
 * RvSelectRead event is received in the select module on this socket, indicating that
 * there is information to receive on this socket.
 * INPUT   : socket         - Socket to receive the buffer from
 *           messageInfo    - Of type RvSctpMessageInfo indicating the buffer to receive
 *                            the data and the buffer length.
 *                            Stream and flags will be filled in by the function
 *           logMgr         - log manager instance
 * OUTPUT  : bytesReceived  - Number of bytes that were actually received
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSctpReceive(
    IN  RvSocket*           sock,
    IN  RvSctpMessageInfo*  messageInfo,
    IN  RvLogMgr*           logMgr,
    OUT RvSize_t*           bytesReceived)
{
    RvStatus status = RV_OK;
    int errLen;
    int socketError = 0;
    rvASockAddr u;
    struct msghdr inmessage;
    struct iovec iov;
    union {
        struct cmsghdr _cmsg;
        char _incmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
    } u_incmsg;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (sock == NULL || messageInfo == NULL)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpReceive(sock=%d,messageInfo=%p): BADPARAM",
            ((sock == NULL) ? 0 : *sock), messageInfo));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
         "RvSocketSctpReceive(sock=%d,messageInfo=%p,logMgr=%p,bytesReceived=%p)",
         *sock,messageInfo,logMgr,bytesReceived));

    iov.iov_base = messageInfo->buffer;
    iov.iov_len  = messageInfo->bufferLength;

    inmessage.msg_name       = RVSOCKDATA(u);
    inmessage.msg_namelen    = sizeof(RVSOCKDATA(u));
    inmessage.msg_iov        = &iov;
    inmessage.msg_iovlen     = 1;
    inmessage.msg_control    = u_incmsg._incmsg;
    inmessage.msg_controllen = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

    errLen = recvmsg(*sock, &inmessage, MSG_WAITALL);

    if (errLen >= 0)
    {
        if (bytesReceived != NULL)
            *bytesReceived = (RvSize_t)errLen;

        if (errLen > 0)
        {
            if (inmessage.msg_flags & MSG_NOTIFICATION)
            {
                messageInfo->messageType = RvSctpMsgNotification;
            }
            else
            {
                /* Inspect the ancillary data (exists only when errLen > 0) */
                struct cmsghdr* cmsg;
                struct sctp_sndrcvinfo* sinfo;

                messageInfo->messageType = RvSctpMsgNormal;

                cmsg = CMSG_FIRSTHDR(&inmessage);
                while (NULL != cmsg)
                {
                    if (SCTP_SNDRCV == cmsg->cmsg_type)
                    {
                        sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
                        messageInfo->streamNum = sinfo->sinfo_stream;
                        messageInfo->flags     = sinfo->sinfo_flags;
                        break;
                    }
                    cmsg = CMSG_NXTHDR(&inmessage, cmsg);
                }

                /* translate sockaddr_in into RvAddress struct */
                RvSocketSockAddrToAddress(RVSOCKDATA(u), sizeof(RVSOCKDATA(u)), &messageInfo->remoteAddress);
            }
        }
        else if (messageInfo->bufferLength > 0)
        {
            /* FIN got on SCTP connection, return EOF error */
            status = RvSocketErrorCode(RV_SOCKET_ERROR_EOF);
        }
    }
    else
    {
        /* We had an error - check to see if it's WOULDBLOCK */
        socketError = RvSocketErrNo;
        if (socketError == EWOULDBLOCK)
            status = RvSocketErrorCode(RV_ERROR_TRY_AGAIN);
        else if (socketError == ECONNRESET)
            status = RvSocketErrorCode(RV_ERROR_NETWORK_PROBLEM);
        else
            status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
    if (status == RV_OK)
    {
        RvChar addr[RV_ADDRESS_MAXSTRINGSIZE];
        RvAddressGetString(&messageInfo->remoteAddress, sizeof(addr), addr);
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSctpReceive(sock=%d,addr=%s:%d,messageInfo=%p,received=%d)=0",
            *sock, addr, RvAddressGetIpPort(&messageInfo->remoteAddress), messageInfo, errLen));
    }
#endif

    if (status != RV_OK)
    {
        RvSockLogDebug((&logMgr->socketSource,
            "RvSocketSctpReceive(sock=%d,messageInfo=%p,errno=%d)=%d",
            *sock, messageInfo, socketError, status));
    }

    return status;
}


/**********************************************************************************
 * RvSocketSctpSockOpt
 * Change the options associated with a SCTP socket.
 * INPUT   : socket     - Socket to change its options
 *           optName    - Option id
 *           optVal     - Pointer to the option properties
 *           optLen     - Length of the data pointed to by optVal
 *           logMgr     - log manager instance
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSocketSctpSockOpt(
    IN  RvSocket*           sock,
    IN  RvSctpOptionOper    operation,
    IN  RvInt               optName,
    IN  void*               optVal,
    IN  RvSize_t            optLen,
    IN  RvLogMgr*           logMgr)
{
    RvStatus status = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (sock == NULL || optVal == NULL)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpSockOpt(sock=%d,optVal=%p): BADPARAM",
            ((sock == NULL) ? 0 : *sock), optVal));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS)
    if (operation==RvSctpOptionGet && optName==RV_SCTP_PRIMARY_ADDR)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpSockOpt(sock=%d): Get Primary Address is not supported on Solaris",
            ((sock == NULL) ? 0 : *sock)));
        return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSctpSockOpt(sock=%d,option=%d,value=%p,length=%d)",
        *sock,optName,optVal,optLen));

    if (operation == RvSctpOptionGet)
    {
        RvUint intlen;
        intlen = (RvUint)optLen;
        status = getsockopt(*sock, IPPROTO_SCTP, optName, optVal, &intlen);
        optLen = intlen;
    }
    else
    {
        status = setsockopt(*sock, IPPROTO_SCTP, optName, optVal, optLen);
    }

    if (status == 0)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSctpSockOpt(sock=%d,option=%d,value=%p,length=%d)",
            *sock,optName,optVal,optLen));
    }
    else
    {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpSockOpt(sock=%d,option=%d,value=%p,length=%d,errno=%d)",
            *sock,optName,optVal,optLen,RvSocketErrNo));
    }

    return status;
}


/**********************************************************************************
 * RvSocketSctpGetAddresses
 * Get all the locally bound IP addresses on a SCTP socket or all the peer addresses
 * in an association represented by a socket.
 * INPUT   : socket     - Socket to get its associated addresses
 *           assocSide  - Which side of the connection is required:
 *                        RvSctpAssocLocal or RvSctpAssocPeer
 *           addresses  - Array of addresses to get the results
 *           logMgr     - log manager instance
 * INOUT   : nAddresses - Pointer to variable indicating the number of addresses in the array
 *                        Will be used to report the actual number of addresses
 * RETURN  : RV_OK on success, other on failure
 */


#ifdef RV_SCTP_ADDR_CAST_NEEDED
#define SCTP_ADDR_CAST(x) (void **)(x)
#else
#define SCTP_ADDR_CAST(x) x
#endif



RVCOREAPI
RvStatus RVCALLCONV RvSocketSctpGetAddresses(
   IN    RvSocket*          sock,
   IN    RvSctpAssocSide    assocSide,
   IN    RvAddress*         addresses,
   INOUT RvSize_t*          nAddresses,
   IN    RvLogMgr*          logMgr)
{
    RvStatus status = RV_OK;
    struct sockaddr *addrs;
    RvUint8* sockaddrPtr;
    int nAddrs, i;
    int socketError = 0;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (sock == NULL || addresses == NULL || nAddresses == NULL)
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpGetAddresses(sock=%d,addrs=%p,nAddrs=%p): BADPARAM",
            ((sock == NULL) ? 0 : *sock), addresses, nAddresses));
        return RvSocketErrorCode(RV_ERROR_BADPARAM);
    }
#endif

    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSctpGetAddresses(sock=%d,side=%d,addrs=%p,nAddrs=%d)",
        *sock,assocSide,addresses,*nAddresses));

    if (assocSide == RvSctpAssocLocal)
        nAddrs = sctp_getladdrs(*sock, 0, SCTP_ADDR_CAST(&addrs));
    else
        nAddrs = sctp_getpaddrs(*sock, 0, SCTP_ADDR_CAST(&addrs));

    if (nAddrs < 0)
    {
        socketError = RvSocketErrNo;
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }
    else
    {
        sockaddrPtr = (RvUint8*)addrs;

        for (i=0; i < (RvInt)RvMin(*nAddresses,(RvSize_t)nAddrs); ++i)
        {
            status = RvSocketSockAddrToAddress(sockaddrPtr, RV_SOCKET_SOCKADDR_SIZE,
                                               &addresses[i]);
            if (status != RV_OK)
                break;

#if (RV_NET_TYPE & RV_NET_IPV6)
            if (RvAddressGetType(&addresses[i]) == RV_ADDRESS_TYPE_IPV4)
                sockaddrPtr += RV_SOCKET_SOCKADDR4_SIZE;
            else
#endif
                sockaddrPtr += RV_SOCKET_SOCKADDR_SIZE;
        }

        *nAddresses = nAddrs;

        if (nAddrs > 0)
        {
            if (assocSide == RvSctpAssocLocal)
                sctp_freeladdrs(addrs);
            else
                sctp_freepaddrs(addrs);
        }
    }

    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSctpGetAddresses(sock=%d,side=%d,addrs=%p,nAddrs=%d)",
            *sock,assocSide,addresses,*nAddresses));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSctpGetAddresses(sock=%d,side=%d,addrs=%p,nAddrs=%d,errno=%d)=%d",
            *sock,assocSide,addresses,*nAddresses,socketError,status));
    }

    return status;
}

#endif /* RV_NET_TYPE & RV_NET_SCTP */



/********************************************************************************************
 * RvSocketSetTCPSynRetries
 *
 *
 *
 * INPUT   : sock           - Socket to shutdown
 *           retries        - Number of TCP SYN retries to be sent.
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI RvStatus RvSocketSetTCPSynRetries(
	IN RvSocket*        sock,
	IN RvInt 	        retries,
	IN RvLogMgr*        logMgr)
{
#if (RV_OS_TYPE == RV_OS_TYPE_LINUX)
    RvInt res;
    RvStatus status = RV_OK;


    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketSetTCPSynRetries(sock=%d, retries=%d)",
        *sock, retries));

    res = setsockopt(*sock, IPPROTO_TCP, TCP_SYNCNT, (char*)&retries,
                         sizeof (retries));


    if (res < 0)
    {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }



    if (status == RV_OK)
    {
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketSetTCPSynRetries(sock=%d, retries=%d)=%d",
            *sock, retries, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketSetTCPSynRetries(sock=%d, retries=%d, errno=%d)=%d",
            *sock, retries, RvSocketErrNo, status));
    }

    return status;

#else  /* (RV_OS_TYPE == RV_OS_TYPE_LINUX) */
    RV_UNUSED_ARG(sock);
    RV_UNUSED_ARG(retries);
    RV_UNUSED_ARG(logMgr);
    return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif /* (RV_OS_TYPE == RV_OS_TYPE_LINUX) */
}


/********************************************************************************************
* RvSocketGetTCPSynRetries
*
*
* INPUT   : sock           - Socket to shutdown
*           logMgr         - log manager instance
* OUTPUT  : 
*           pRetries       - Number of TCP SYN retries to be sent.
* RETURN  : RV_OK on success, other on failure
*/
RVCOREAPI RvStatus RvSocketGetTCPSynRetries(
    IN  RvSocket*        sock,
    OUT RvInt 	         *pRetries,
    IN  RvLogMgr*        logMgr)
{
#if (RV_OS_TYPE == RV_OS_TYPE_LINUX)
    RvInt res;
    RvStatus status = RV_OK;
    RvInt retries;
    RvUint intlen;

    *pRetries = 0;


    RvSockLogEnter((&logMgr->socketSource,
        "RvSocketGetTCPSynRetries(sock=%d, retries=%d)",
        *sock, retries));

    intlen = sizeof (retries);
    res = getsockopt(*sock, IPPROTO_TCP, TCP_SYNCNT, (char*)&retries,&intlen);


    if (res < 0)
    {
        status = RvSocketErrorCode(RV_ERROR_UNKNOWN);
    }

    if (status == RV_OK)
    {
        *pRetries = retries;
        RvSockLogLeave((&logMgr->socketSource,
            "RvSocketGetTCPSynRetries(sock=%d, retries=%d)=%d",
            *sock, retries, status));
    }
    else
    {
        RvSockLogError((&logMgr->socketSource,
            "RvSocketGetTCPSynRetries(sock=%d, retries=%d, errno=%d)=%d",
            *sock, retries, RvSocketErrNo, status));
    }

    return status;

#else  /* (RV_OS_TYPE == RV_OS_TYPE_LINUX) */
    RV_UNUSED_ARG(sock);
    RV_UNUSED_ARG(pRetries);
    RV_UNUSED_ARG(logMgr);
    return RvSocketErrorCode(RV_ERROR_NOTSUPPORTED);
#endif /* (RV_OS_TYPE == RV_OS_TYPE_LINUX) */
}




#if defined(__cplusplus)
}
#endif

#endif /* (RV_NET_TYPE != RV_NET_NONE) */
