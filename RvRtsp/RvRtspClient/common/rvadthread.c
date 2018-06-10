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
#endif
 
#if (RV_OS_TYPE == RV_OS_TYPE_WINCE)
#  define RV_CREATE_THREAD CreateThread
#  define RV_DELETE_THREAD ExitThread
#endif

#if (RV_THREAD_TYPE == RV_THREAD_POSIX)  
#include <errno.h>
static pthread_key_t RvThreadCurrentKey; /* Used to track current thread ptr */
#endif

#if (RV_THREAD_TYPE == RV_THREAD_WINCE) 
static DWORD RvThreadCurrentKey; /* Used to track current thread ptr */

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WIN32)
static unsigned __stdcall RvAdThreadWrapper(void* arg1)
#endif

#endif
#if (RV_THREAD_TYPE == RV_THREAD_POSIX)  
static void* RvAdThreadWrapper(void* arg1)
#endif
{
    RvThreadWrapper(arg1, RV_FALSE);
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_WINCE)
    RV_DELETE_THREAD(0); /* does not return */
#endif

    return 0;
}


RvStatus RvAdThreadInit(void)
{
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    /* create a TLS variable for holding copies of thread pointers */
    RvThreadCurrentKey = TlsAlloc();
    if(RvThreadCurrentKey == 0xFFFFFFFF)
        return RV_ERROR_UNKNOWN;
#endif        

#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
    /* create a thread specific variable for holding copies of thread pointers */
    if(pthread_key_create(&RvThreadCurrentKey, RvThreadExitted) != 0)
        return RV_ERROR_UNKNOWN;
#endif

    return RV_OK;
}


void RvAdThreadEnd(void)
{
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    TlsFree(RvThreadCurrentKey); /* need to delete TLS variable */
#endif    

#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
    pthread_key_delete(RvThreadCurrentKey);
#endif

}


RvStatus RvAdThreadConstruct(
    IN  RvThreadBlock*  tcb)
{
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    *tcb = NULL; /* make sure its clear so we know when a handle has been created */
#endif

#if (RV_THREAD_TYPE == RV_THREAD_POSIX)
    if(pthread_attr_init(tcb) != 0)
        return (RV_ERROR_UNKNOWN);
#endif

    return RV_OK;
}


void RvAdThreadDestruct(
    IN  RvThreadBlock*  tcb)
{
#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
    pthread_attr_destroy(tcb);
#endif

#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    if(*tcb != NULL)
        CloseHandle(*tcb);

 #endif
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

#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
    struct sched_param params;
    int presult;
#endif

    RV_UNUSED_ARG(name);
    RV_UNUSED_ARG(priority);
    RV_UNUSED_ARG(attr);
    RV_UNUSED_ARG(stackaddr);
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    *tcb = (RvThreadBlock)RV_CREATE_THREAD(NULL, realstacksize, RvAdThreadWrapper,
                                           arg1, CREATE_SUSPENDED, id);
    
    if(*tcb == NULL)
        return RV_ERROR_UNKNOWN;
#endif        


#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
    /* We can't actually create the thread, but we can set up all the attributes. */
    /* We'll have to set a copy of the thread pointer in the ThreadWrapper. */
    presult = pthread_attr_getschedparam(tcb, &params);
    if(presult == 0) {
        params.sched_priority = priority;
        /* FIX THIS presult |= */ pthread_attr_setschedparam(tcb, &params);
    }
    presult |= pthread_attr_setdetachstate(tcb, PTHREAD_CREATE_DETACHED);
    if (realstacksize != 0)
        presult |= pthread_attr_setstacksize(tcb, realstacksize);

    /* Now set attributes we allow the user to set */
    presult |= pthread_attr_setscope(tcb, attr->contentionscope);
    presult |= pthread_attr_setschedpolicy(tcb, attr->policy);
    presult |= pthread_attr_setinheritsched(tcb, attr->inheritsched);

    if(presult != 0)
        return RV_ERROR_UNKNOWN;
#endif


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

#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    if(ResumeThread(*tcb) != 1)
        return RV_ERROR_UNKNOWN;
#endif   

#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
    if(pthread_create(id, tcb, RvThreadWrapper, arg1) != 0)
         return RV_ERROR_UNKNOWN;
#endif

    return RV_OK;
}


RvStatus RvAdThreadDelete(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId      id)
{
    RV_UNUSED_ARG(id);
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    if(TerminateThread(*tcb, 0) == 0)
        return RV_ERROR_UNKNOWN;
#endif
    return RV_OK;
}


RvStatus RvAdThreadWaitOnExit(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId      id)
{
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    DWORD exitcode;

    RV_UNUSED_ARG(id);

    if(GetExitCodeThread(*tcb, &exitcode) == 0)
        return RV_ERROR_UNKNOWN;

    if(exitcode == STILL_ACTIVE)
        return RV_ERROR_TRY_AGAIN;
#endif        

    return RV_OK;
}


RvStatus RvAdThreadSetTls(
    IN  RvThreadId      id,
    IN  RvInt32         state,
    IN  void*           tlsData)
{
    RV_UNUSED_ARG(id);
    RV_UNUSED_ARG(state);
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    if(TlsSetValue(RvThreadCurrentKey, tlsData) == 0)
        return RV_ERROR_UNKNOWN;
#endif

#if (RV_THREAD_TYPE == RV_THREAD_POSIX)
    /* Use a thread specific variable to store a copy of the thread pointer. */
    /* RvThreadCurrentKey must be set up in RvThreadInit. */
    /**** Pointer is set for current task NOT the task pointer to by th. ****/
    if(pthread_setspecific(RvThreadCurrentKey,tlsData) != 0)
        return 0;
#endif


    return RV_OK;
}


void* RvAdThreadGetTls(void)
{
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    return TlsGetValue(RvThreadCurrentKey);
#endif    

#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
   return pthread_getspecific(RvThreadCurrentKey);
#endif

}


RvThreadId RvAdThreadCurrentId(void)
{
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    return GetCurrentThreadId();
#endif    
#if (RV_THREAD_TYPE == RV_THREAD_POSIX)
    return pthread_self();
#endif
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
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    DWORD ticks;

    ticks = (DWORD)(nsecs / RV_TIME64_NSECPERMSEC);

    if(ticks == 0)
        ticks = 1;

    Sleep(ticks);
#endif    

#if (RV_THREAD_TYPE == RV_THREAD_POSIX)
    struct timespec delaytime, timeleft;

    delaytime.tv_sec = (RvInt32)(nsecs/RV_TIME64_NSECPERSEC);
    delaytime.tv_nsec = (RvInt32)(nsecs% RV_TIME64_NSECPERSEC);  
    for(;;) {
        if(nanosleep(&delaytime, &timeleft) == 0)
            return ;
        if(errno != EINTR)
            return ;
        delaytime.tv_sec = timeleft.tv_sec;
        delaytime.tv_nsec = timeleft.tv_nsec;
    }
#endif    
    
}


RvStatus RvAdThreadGetPriority(
    IN  RvThreadId      id,
    OUT RvInt32*        priority)
{
    RV_UNUSED_ARG(id);
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    *priority = 0;
#endif
#if (RV_THREAD_TYPE == RV_THREAD_POSIX)
    struct sched_param params;
    int policy;
    RvInt32 pri;
    if(pthread_getschedparam(id, &policy, &params) != 0)
        return (RV_ERROR_UNKNOWN);
     *priority = (RvInt32)params.sched_priority;
#endif

    return RV_OK;
}


RvStatus RvAdThreadSetPriority(
    IN  RvThreadBlock*  tcb,
    IN  RvThreadId      id,
    IN  RvInt32         priority,
    IN  RvInt32         state)
{
#if (RV_THREAD_TYPE == RV_THREAD_POSIX)
    struct sched_param params;
    int policy;
#endif
    RV_UNUSED_ARG(id);
    RV_UNUSED_ARG(state);
    
#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)
    if(SetThreadPriority(*tcb, priority) == 0)
        return RV_ERROR_UNKNOWN;
#endif      

#if (RV_THREAD_TYPE == RV_THREAD_POSIX) 
        /* Set out local copy */
        if(pthread_attr_getschedparam(tcb, &params) == 0) {
            params.sched_priority = priority;
            if(pthread_attr_setschedparam(tcb, &params) != 0)
                return RV_ERROR_UNKNOWN;
        } else return (RV_ERROR_UNKNOWN);

        /* If thread has been started so tell the scheduler about it */
        if(state != RV_THREAD_STATE_CREATED) {
            if(pthread_getschedparam(id, &policy, &params) == 0) {
                params.sched_priority = priority;
                if(pthread_setschedparam(id, policy, &params) != 0)
                    return (RV_ERROR_UNKNOWN);
            } else  return (RV_ERROR_UNKNOWN);
        }
#endif

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

#if (RV_THREAD_TYPE == RV_THREAD_WIN32) || (RV_THREAD_TYPE == RV_THREAD_WINCE)

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
#endif

#endif /* RV_THREAD_TYPE != RV_THREAD_NONE */
