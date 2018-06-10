/***********************************************************************
Filename   : rvusrconfig.h
Description: let the user to put his own choices
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
#ifndef RV_USRCONFIG_H
#define RV_USRCONFIG_H

/* Logging options - use make line when not on Win32 */
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)

#undef  RV_LOGMASK

#if defined(_DEBUG)

#define RV_LOGMASK RV_LOGLEVEL_ALL

#else /* _DEBUG */

#define RV_LOGMASK RV_LOGLEVEL_NONE

#endif /* _DEBUG */

#endif /* (RV_OS_TYPE == RV_OS_TYPE_WIN32) */

/* Log listener */
#undef  RV_LOGLISTENER_TYPE

#if (RV_OS_TYPE == RV_OS_TYPE_PSOS)
#define RV_LOGLISTENER_TYPE  RV_LOGLISTENER_TERMINAL/*RV_LOGLISTENER_FILE_AND_TERMINAL RV_LOGLISTENER_UDP*/
#else
#define RV_LOGLISTENER_TYPE  RV_LOGLISTENER_FILE_AND_TERMINAL
#endif

/* Threadness mode: Single / Multi */
#undef  RV_THREADNESS_TYPE
#define RV_THREADNESS_TYPE RV_THREADNESS_MULTI

/* Networking support: None, IPv4, IPv6 */
#undef  RV_NET_TYPE
#define RV_NET_TYPE RV_NET_IPV4 /*+ RV_NET_IPV6 */ /*+ RV_NET_SCTP*/

/* Select method: Select, Poll, Devpoll, Win32 WSA */
#undef  RV_SELECT_TYPE
#define RV_SELECT_TYPE RV_SELECT_DEFAULT

/* Socket Definitions */

/* IPv6 TOS support
 * Setting to '1' enables IPv6 ToS (traffic class support) on OS that
 * provide IPV6_TCLASS socket option
 * On OS that doesn't provide IPV6_TCLASS socket option,
 * you may set RV_SOCKET_USE_FAKED_IPV6_TCLASS compile time option
 * that provide IPv6 ToS support for UDP sockets.
 *
 */
#undef RV_SOCKET_ENABLE_IPV6_TOS
#define RV_SOCKET_ENABLE_IPV6_TOS 1

#undef RV_SOCKET_USE_FAKED_IPV6_TCLASS
#define RV_SOCKET_USE_FAKED_IPV6_TCLASS 2

/* End of Socket Definitions */


/* IPsec support for IMS: RV_NO, RV_YES */
#ifdef RV_IMS_IPSEC_ENABLED
#undef RV_IMS_IPSEC_ENABLED
#endif
#define RV_IMS_IPSEC_ENABLED RV_NO

/* TLS support: None, OpenSSL */
#undef  RV_TLS_TYPE
#define RV_TLS_TYPE RV_TLS_NONE


/* 'Hosts' file resolution settings */

/* If defined as 1 - enables 'hosts' file based name resolution */
#undef  RV_DNS_USES_HOSTS
#define RV_DNS_USES_HOSTS RV_NO




/*============== DNS caching related constants ==============
    Note: for embedded linux only
*/

/* Set RV_DNS_USES_CACHING to enable caching */
#define RV_DNS_USES_CACHING RV_YES

/* Size of page used to hold cache data. Should be >= 512 */
#define RV_DNS_CACHE_PAGE_SIZE 2048

/* Number of preallocated pages */
#define RV_DNS_CACHE_PAGES_NUMBER 512

/* Lower limit on the number of buckets in hash table
 * Actual number will be calculated by cache module itself as some prime
 * greater or equal RV_DNS_CACHE_HASH_SIZE
 */
#define RV_DNS_CACHE_HASH_SIZE 1024

/* Maximal number of active queries to cache module */
#define RV_DNS_CACHE_MAX_ACTIVE_QUERIES 512

/* Compiles in support for sanity checks in DNS caching */
#define RV_DNS_CACHE_SANITY RV_NO

/* For debugging purposes only! Uses faked notion of time */
#define RV_DNS_CACHE_DEBUG 0

/*-----------------------------------------------------*/
/*      FreeBSD type definitions                       */
/*-----------------------------------------------------*/
/*The following type definisions are only for FreeBSD platforms*/

/* Select if socket priority should be implemented with kqueue*/
#undef  RV_SELECT_KQUEUE_GROUPS
#define RV_SELECT_KQUEUE_GROUPS RV_NO




/*-----------------------------------------------------*/
/*      Nucleus type definitions                       */
/*-----------------------------------------------------*/
/*The following type definisions are only for Nucleus platforms*/

/* Set the range of Nucleus default portRange (otherwise, default range is 5000-65535) */
/*#undef RV_PORTRANGE_DEFAULT_START
#define RV_PORTRANGE_DEFAULT_START 10000
#undef RV_PORTRANGE_DEFAULT_FINISH
#define RV_PORTRANGE_DEFAULT_FINISH 65534*/


/* Set the priority of the blocked commands for Nucleus (connect, close)
   RV_THREAD_SOCKET_PRIORITY_DEFAULT by default is 3 */
/*
#undef RV_THREAD_SOCKET_PRIORITY_DEFAULT
#define RV_THREAD_SOCKET_PRIORITY_DEFAULT  (2)
*/


/*-----------------------------------------------------*/
/*      SIP Stack fixed definitions                    */
/*-----------------------------------------------------*/
/* The following definisions must not be changed by the application*/
/* Common core compatibility mode
 * Used for overcoming some backward incompatibilities
 * Value of this parameter defines CC version to emulate
 */
#define RV_CCORE_COMP_MODE 0x1100
#define RV_USE_RESOURCE_COUNTING 0
#ifndef RV_ANSI_FORCE_OUR
#define RV_ANSI_FORCE_OUR 0
#endif

/*
#ifndef _WIN32_WCE
#define _WIN32_WCE 0
#endif
*/
#ifndef RV_SQRT_TYPE
#define RV_SQRT_TYPE 0
#endif
#ifndef RV_THREAD_TLS_TYPE
#define RV_THREAD_TLS_TYPE 0
#endif
#ifndef RV_THREAD_USE_AUTOMATIC_INTERNING
#define RV_THREAD_USE_AUTOMATIC_INTERNING 0
#endif

/* Validation checking options */
#undef  RV_CHECK_MASK
#define RV_CHECK_MASK RV_CHECK_ALL


/* DNS support: None, Ares */
#undef  RV_DNS_TYPE
#define RV_DNS_TYPE RV_DNS_ARES


/* Defines 'hosts' file search order
 * If set as
 *  RV_EHD_SEARCH_HOSTS_PLATFORM - platform specific algorithm will be used
 *  RV_EHD_SEARCH_HOSTS_FIRST    - hosts file will be searched before DNS servers
 *  RV_EHD_SEARCH_HOSTS_LAST     - hosts file will be searched after DNS servers
 */
#undef  RV_EHD_SEARCH_ORDER
#define RV_EHD_SEARCH_ORDER RV_EHD_SEARCH_HOSTS_PLATFORM

#define RV_EHD_HOSTFILE "hosts.txt"


/* Does local host have a static list of addresses */
#undef RV_HOST_HAS_STATIC_ADDR
#define RV_HOST_HAS_STATIC_ADDR RV_YES

/* Select engine preemption is usually used in multithreading scenarios
 *  to interrupt RvSelectWaitAndBlock call. As a side effect user defined
 *  callback is called. In single-threaded scenarios the main effect (interrupt
 *  of RvSelectWaitAndBlock) is meaningless, so this mechanism is disabled by default.
 *  On the other hand, some application may find usefull the above mentioned
 *  side effect. To enable preemption in single-threaded scenarios also - set
 *  uncomment the following lines
 */
#undef RV_SINGLE_THREADED_PREEMPTION
#define RV_SINGLE_THREADED_PREEMPTION RV_YES

/*
#undef RV_SYMBIAN_MULTI_ONE_TH
#define RV_SYMBIAN_MULTI_ONE_TH RV_YES
*/

#define RV_QUEUE_TYPE RV_QUEUE_STANDARD

/* enabling session renegotiation for TLS  - for internal use only*/
/*
#undef RV_TLS_ENABLE_RENEGOTIATION
#define RV_TLS_ENABLE_RENEGOTIATION RV_YES
*/
#ifndef RV_TEST_ALL
#define RV_TEST_ALL 0
#endif

#ifndef RV_ANSI_FORCE_OUR
#define RV_ANSI_FORCE_OUR 0
#endif

/*
#ifndef _WIN32_WCE
#define _WIN32_WCE 0
#endif
*/


#ifndef RV_SQRT_TYPE
#define RV_SQRT_TYPE 0
#endif

#ifndef RV_THREAD_TLS_TYPE
#define RV_THREAD_TLS_TYPE 0
#endif

#ifndef RV_THREAD_USE_AUTOMATIC_INTERNING
#define RV_THREAD_USE_AUTOMATIC_INTERNING 0
#endif

/*-----------------------------------------------------*/
/*      SIP Stack deprecated definitions               */
/*-----------------------------------------------------*/
/*Do not delete the following type definitions. They are defined
  for backwards compatabilities and enable sip applications
  to use deprecated type definitions.*/

#if defined(RV_IPV6)
#undef  RV_CFLAG_IPV6
#define RV_CFLAG_IPV6
#endif  /* defined(RV_IPV6) */

#if defined(RV_TLS_ON)
#undef  RV_CFLAG_TLS
#define RV_CFLAG_TLS
#endif  /* defined(RV_TLS_ON) */

#if defined(RV_NOLOG)
#undef RV_CFLAG_NOLOG
#define RV_CFLAG_NOLOG
#endif  /* defined(RV_NOLOG) */

#if defined(RV_NOTHREADS)
#undef  RV_CFLAG_NOTHREADS
#define RV_CFLAG_NOTHREADS
#endif /* defined(RV_NOTHREADS) */

#undef RV_SELECT_PREEMPTION_PIPE_SIZE
#define RV_SELECT_PREEMPTION_PIPE_SIZE 1024

/*
#define RV_THREAD_USE_AUTOMATIC_INTERNING 1
*/




#endif /* RV_USRCONFIG_H */
