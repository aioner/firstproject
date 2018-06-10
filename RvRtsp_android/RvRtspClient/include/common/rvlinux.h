/***********************************************************************
Filename   : rvwin32.h
Description: config file for Win32
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
#ifndef RV_LINUX_H
#define RV_LINUX_H

#ifdef _RV_LINUX_API_HIDDEN
#define RVAPI __attribute__((visibility("hidden")))
#define RVINTAPI  __attribute__((visibility("hidden")))
#define RVCOREAPI  __attribute__((visibility("hidden")))
#elif defined(_RV_LINUX_API_DEFAULT)
#define RVAPI __attribute__((visibility("default")))
#define RVINTAPI  __attribute__((visibility("default")))
#define RVCOREAPI  __attribute__((visibility("default")))
#else
#define RVAPI
#define RVINTAPI
#define RVCOREAPI
#endif
#define RVCALLCONV

/* rvtime: Select timestamp interface to use */
#define RV_TIMESTAMP_TYPE RV_TIMESTAMP_LINUX

/* rvtime: Select clock interface to use */
#define RV_CLOCK_TYPE RV_CLOCK_LINUX

/* rvtm: Select tm (calendar time) interface to use */
#define RV_TM_TYPE RV_TM_POSIX

/* rv64ascii: Select 64 bit conversions to use */
#define RV_64TOASCII_TYPE RV_64TOASCII_STANDARD

/* rvsemaphore: Select semaphore interface to use */
#define RV_SEMAPHORE_TYPE RV_SEMAPHORE_POSIX
#define RV_SEMAPHORE_ATTRIBUTE_DEFAULT 0 /* not used */

/* rvmutex: Select mutex interface to use */
#define RV_MUTEX_TYPE RV_MUTEX_POSIX
//#define RV_MUTEX_ATTRIBUTE_DEFAULT 0 /* spin count (used only on Win2000 and newer) */
#define RV_MUTEX_ATTRIBUTE_DEFAULT { 0, 0 } /* not used */

#undef  PTHREAD_EXPLICIT_SCHED
#define PTHREAD_EXPLICIT_SCHED 1

#undef  PTHREAD_MUTEX_FAST_NP
#define PTHREAD_MUTEX_FAST_NP 0

/* rvthread: Select thread interface to use and set parameters */
/* rvthread: Select thread interface to use and set parameters */
#define RV_THREAD_TYPE RV_THREAD_POSIX
#define RV_THREAD_PRIORITY_DEFAULT 10
#define RV_THREAD_STACKSIZE_DEFAULT 0 /* Allow OS to allocate */
#define RV_THREAD_STACKSIZE_USEDEFAULT 0x100000 /* Under this stack size use default stack size */
#define RV_THREAD_ATTRIBUTE_DEFAULT { PTHREAD_SCOPE_SYSTEM, SCHED_OTHER, PTHREAD_EXPLICIT_SCHED } /* scope, schedpolicy, inheritsched */
#define RV_THREAD_AUTODELETE 1

/* rvlock: Select lock interface to use */
#define RV_LOCK_TYPE RV_LOCK_POSIX
#define RV_LOCK_ATTRIBUTE_DEFAULT { PTHREAD_MUTEX_FAST_NP } /* set to FAST or ERRORCHECK only */

/* rvmemory: Select memory interface to use */
#define RV_MEMORY_TYPE RV_MEMORY_STANDARD

/* rvosmem: Select OS dynamic memory driver to use */
#define RV_OSMEM_TYPE RV_OSMEM_MALLOC

/* rvhost: Select network host interface to use */
#define RV_HOST_TYPE RV_HOST_POSIX

/* rvfdevent: File-descriptor events interface to use */
#define RV_SELECT_TYPE RV_SELECT_SELECT

/* rvsockets: Type of Sockets used in the system */
#define RV_SOCKET_TYPE RV_SOCKET_BSD

/* rvportrange: Type of Port-range used in the system */
#define RV_PORTRANGE_TYPE RV_PORTRANGE_FAST

/* rvloglistener: Type of log listeners used in the system */
#define RV_LOGLISTENER_TYPE RV_LOGLISTENER_FILE_AND_TERMINAL

/* rvstdio: Select stdio interface to use */
#define RV_STDIO_TYPE RV_STDIO_ANSI

/* rvassert: Select stdio interface to use */
#define RV_ASSERT_TYPE RV_ASSERT_ANSI




#endif /* RV_LINUX_H */

