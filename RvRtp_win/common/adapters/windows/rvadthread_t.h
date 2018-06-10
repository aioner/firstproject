/* rvthread_t.h - rvthread_t header file */
/************************************************************************
      Copyright (c) 2001,2002 RADVISION Inc. and RADVISION Ltd.
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

#ifndef RV_ADTHREAD_T_H
#define RV_ADTHREAD_T_H


#include "rvtypes.h"

typedef HANDLE RvThreadBlock; /* use the tcb as the handle reference */

/********************************************************************************************
 * RvThreadId
 * An OS specific thread ID. Used to identify threads regardless
 * of whether or not a thread handle has been constructed for it.
 ********************************************************************************************/
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
typedef unsigned RvThreadId;
#else
typedef DWORD RvThreadId;
#endif


/********************************************************************************************
 * RvThreadAttr
 * OS specific attributes and options used for threads. See definitions in rvthread.h
 * along with the default values in rvccoreconfig.h for more information.
 ********************************************************************************************/
typedef struct {
    BOOL disablepriorityboost; /* value for SetThreadPriorityBoost (Win NT 4.0 or newer only) */
    DWORD affinitymask;        /* value for SetThreadAffinityMask */
    DWORD idealprocessor;      /* value for SetThreadIdealProcessor (Win NT 4.0 or newer only) */
} RvThreadAttr;

#define RV_THREAD_PRIORITY_MAX 2
#define RV_THREAD_PRIORITY_MIN (-2)
#define RV_THREAD_PRIORITY_INCREMENT 1
#define RV_THREAD_WRAPPER_STACK 64

/* OS specific options */
#define RV_THREAD_AUTO_DELETE       RV_YES  /* whether thread auto-delete is supported or not */
#define RV_THREAD_SETUP_STACK       RV_NO   /* whether to allocate stack if stackaddr == NULL */
#define RV_THREAD_STACK_ADDR_USER   RV_NO   /* whether user allocated stack is supported or not */
#define RV_THREAD_STACK_SIZE_OS     RV_YES  /* whether OS can determine stack size or not */


#endif
