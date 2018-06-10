/* rvadthread.c - Windows adapter thread management functions */
/************************************************************************
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
#include "rvadthread.h"

#if (RV_THREAD_TYPE != RV_THREAD_NONE)

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#  include <process.h>
#  define RV_CREATE_THREAD _beginthreadex
#  define RV_DELETE_THREAD _endthreadex
#else
#  define RV_CREATE_THREAD CreateThread
#  define RV_DELETE_THREAD ExitThread
#endif

static DWORD RvThreadCurrentKey; /* Used to track current thread ptr */

#if RV_OS_TYPE == RV_OS_TYPE_WIN32
static unsigned __stdcall RvAdThreadWrapper(void* arg1)
#else
static DWORD WINAPI RvAdThreadWrapper(void* arg1)
#endif
{
    RvThreadWrapper(arg1, RV_FALSE);

    RV_DELETE_THREAD(0); /* does not return */

    return 0;
}


RvStatus RvAdThreadInit(void)
{
    /* create a TLS variable for holding copies of thread pointers */
    RvThreadCurrentKey = TlsAlloc();
    if(RvThreadCurrentKey == 0xFFFFFFFF)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


void RvAdThreadEnd(void)
{
    TlsFree(RvThreadCurrentKey); /* need to delete TLS variable */
}


RvStatus RvAdThreadConstruct(
    IN  RvThreadBlock*  tcb)
{
    *tcb = NULL; /* make sure its clear so we know when a handle has been created */

    return RV_OK;
}


void RvAdThreadDestruct(
    IN  RvThreadBlock*  tcb)
{
    if(*tcb != NULL)
        CloseHandle(*tcb);
}


RvStatus RvAdThreadCreate(
    IN  RvThreadBlock*  tcb,
    IN  RvChar*         name,
    IN  RvInt32         priority,
    IN  RvThreadAttr*   attr,
    IN  void*           stackaddr,
    IN  RvInt32         realstacksize,
    IN  void*           arg1,
    OUT RvThreadId*     id)
{
    RV_UNUSED_ARG(name);
    RV_UNUSED_ARG(priority);
    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(stackaddr);

    *tcb = (RvThreadBlock)RV_CREATE_THREAD(NULL, realstacksize, RvAdThreadWrapper,
                                           arg1, CREATE_SUSPENDED, id);
    
    if(*tcb == NULL)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


RvStatus RvAdThreadStart(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId*     id,
    IN  RvThreadAttr*   attr,
    IN  void*           arg1)
{
    RV_UNUSED_ARG(id);
    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(arg1);

    if(ResumeThread(*tcb) != 1)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


RvStatus RvAdThreadDelete(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId      id)
{
    RV_UNUSED_ARG(id);

    if(TerminateThread(*tcb, 0) == 0)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


RvStatus RvAdThreadWaitOnExit(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId      id)
{
    DWORD exitcode;

    RV_UNUSED_ARG(id);

    if(GetExitCodeThread(*tcb, &exitcode) == 0)
        return RV_ERROR_UNKNOWN;

    if(exitcode == STILL_ACTIVE)
        return RV_ERROR_TRY_AGAIN;

    return RV_OK;
}


RvStatus RvAdThreadSetTls(
    IN  RvThreadId      id,
    IN  RvInt32         state,
    IN  void*           tlsData)
{
    RV_UNUSED_ARG(id);
    RV_UNUSED_ARG(state);

    if(TlsSetValue(RvThreadCurrentKey, tlsData) == 0)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


void* RvAdThreadGetTls(void)
{
    return TlsGetValue(RvThreadCurrentKey);
}


RvThreadId RvAdThreadCurrentId(void)
{
    return GetCurrentThreadId();
}


RvBool RvAdThreadIdEqual(
    IN  RvThreadId      id1,
    IN  RvThreadId      id2)
{
    return (id1 == id2);
}


RvStatus RvAdThreadSleep(
    IN  const RvTime*   t)
{
    RV_UNUSED_ARG(t);

    return RV_ERROR_NOTSUPPORTED;
}


void RvAdThreadNanosleep(
    IN  RvInt64         nsecs)
{
    DWORD ticks;

    ticks = (DWORD)(nsecs / RV_TIME64_NSECPERMSEC);

    if(ticks == 0)
        ticks = 1;

    Sleep(ticks);
}


RvStatus RvAdThreadGetPriority(
    IN  RvThreadId      id,
    OUT RvInt32*        priority)
{
    RV_UNUSED_ARG(id);

    *priority = 0;

    return RV_OK;
}


RvStatus RvAdThreadSetPriority(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId      id,
    IN  RvInt32         priority,
    IN  RvInt32         state)
{
    RV_UNUSED_ARG(id);
    RV_UNUSED_ARG(state);

    if(SetThreadPriority(*tcb, priority) == 0)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


RvStatus RvAdThreadGetName(
    IN  RvThreadId      id,
    IN  RvInt32         size,
    OUT RvChar*         buf)
{
    RV_UNUSED_ARG(id);
    RV_UNUSED_ARG(size);

    /* Win32 don't keep names of threads so we'll just return an empty string. */
    buf[0] = '\0';

    return RV_OK;
}


/* Make sure we delete threads automatically if necessary */
BOOL WINAPI DllMain(IN HINSTANCE    hinstDLL,
                    IN DWORD        fdwReason,
                    IN LPVOID       lpvReserved)
{
    RV_UNUSED_ARG(hinstDLL);
    RV_UNUSED_ARG(lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:			
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            RvThreadExitted(NULL);
            break;
    }

	
    return TRUE;
}

#endif /* RV_THREAD_TYPE != RV_THREAD_NONE */
