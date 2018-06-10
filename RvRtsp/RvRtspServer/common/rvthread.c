/***********************************************************************
Filename   : rvthread.c
Description: thread management functions
************************************************************************
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
#include "rvstdio.h"
#include "rvlock.h"
#include "rvthread.h"
#include "rvmemory.h"
#include "rvccoreglobals.h"
#include "rvinterfacesdefs.h"
#include <string.h>

#if RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL
#include "rvthreadtls.h"
#endif

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
static RvThread *RvThreadCurrentPtr = NULL; /* Used to track current thread ptr */
#define RV_THREAD_WRAPPER_STACK 0
#define RV_THREAD_PRIORITY_MIN 0
#define RV_THREAD_PRIORITY_MAX 0
#define RV_THREAD_DOSLEEP tm_wkafter
#endif

/* define the default attributes since they may be a structure */
/* static RvThreadAttr RvDefaultThreadAttr = RV_THREAD_ATTRIBUTE_DEFAULT; */

#if !defined(RV_THREAD_ADDITIONAL_STACK)
/* indicate any additional stack space that is needed for our own use */
/* but is not used by the OS for the actual stack */
#define RV_THREAD_ADDITIONAL_STACK 0
#endif

/*static RvLock RvThreadLock; / * RvThreadVars lock to prevent bizarre construct/destruct sequences */
/*static RvThreadVar RvThreadVars[RV_THREAD_MAX_VARS]; / * Thread specific variable info */

static RvStatus RvThreadDelete(RvThread *th);
static RvStatus RvThreadWaitOnExit(RvThread *th);
#if (RV_THREAD_TYPE != RV_THREAD_NONE)
static RvStatus RvThreadSetupStack(RvThread *th);
#endif
static RvStatus RvThreadCallExits(RvThread *th);
static RvStatus RvThreadGetOsPriority(RvThreadId id, RvInt32 *priority);
static RvStatus RvThreadSetupThreadPtr(RvThread *th);
static RvStatus RvThreadRemoveThreadPtr(RvThread *th);


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvThreadLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->threadSource, p);}
#else
#define RvThreadLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvThreadLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->threadSource, p);}
#else
#define RvThreadLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvThreadLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->threadSource, p);}
#else
#define RvThreadLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define RvThreadLogDebug(p) {if (logMgr != NULL) RvLogDebug(&logMgr->threadSource, p);}
#else
#define RvThreadLogDebug(p) {RV_UNUSED_ARG(logMgr);}
#endif


#define RvThreadReleaseLog(th) {th->logMgr = 0;}

#undef LOGSRC
#define LOGSRC &logMgr->threadSource

/* Must be called only once and before any other calls to rvthread. */
RvStatus RvThreadInit(void)
{
    RvStatus result;
    RvInt32 i;
    RV_USE_CCORE_GLOBALS;

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    /* to avoid warnings (ghs) */
    RV_UNUSED_ARG(RvThreadLock);
#endif

    result = RvLockConstruct(NULL, &RvThreadLock);
    if(result != RV_OK)
        return result;
    for(i = 0; i < RV_THREAD_MAX_VARS; i++) {
        RvThreadVars[i].active = RV_FALSE;
        RvThreadVars[i].name[0] = '\0';
    }

#if RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL
    RvThreadTLSInit();
#endif

    /* OS specific setup */

    result = RvAdThreadInit();

    if(result != RV_OK)
        RvLockDestruct(&RvThreadLock,NULL);
    return result;
}


/********************************************************************************************
 * RvThreadEnd
 * Shuts down the Thread module. Must be called once (and
 * only once) when no further calls to this module will be made.
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvThreadEnd(void)
{
    RV_USE_CCORE_GLOBALS;
    RvAdThreadEnd();

#if RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL
    RvThreadTLSEnd();
#endif



    RvLockDestruct(&RvThreadLock,NULL);

    return RV_OK;
}

RvStatus RvThreadSourceConstruct(RvLogMgr* logMgr)
{
    RvStatus result;

    result = RvLogSourceConstruct(logMgr, &logMgr->threadSource, "THREAD", "Threads interface");

    return result;
}



#if (RV_THREAD_TYPE != RV_THREAD_NONE)

void RvThreadWrapper(
    IN  void*           arg1,
    IN  RvBool          callDelete)
{
    RV_USE_CCORE_GLOBALS;
    RvThread *th;

    /* At this point arg1 should be the current thread ptr. */
    th = (RvThread *)arg1;

	if(th == 0) {
		return;
	}
    /* Save copy of the thread pointer */
    if(RvThreadSetupThreadPtr(th) != RV_OK) {
        /* We need to abort the best we can. */
        RvLockGet(&th->datalock,RvThreadGetLogManager(th));
        th->state = RV_THREAD_STATE_EXITING;
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvSemaphorePost(&th->exitsignal,RvThreadGetLogManager(th));
        return;
    }

    RvLockGet(&th->datalock,RvThreadGetLogManager(th));
    th->state = RV_THREAD_STATE_RUNNING;
    RvLockRelease(&th->datalock,RvThreadGetLogManager(th));

    th->func(th, th->data);

    RvLockGet(&th->datalock,RvThreadGetLogManager(th));
    th->state = RV_THREAD_STATE_EXITING;
    RvLockRelease(&th->datalock,RvThreadGetLogManager(th));

    RvLockGet(&RvThreadLock,RvThreadGetLogManager(th)); /* Prevent add/remove race conditions */
    RvThreadCallExits(th); /* Do thread specific variable cleanup */
    RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));

    RvThreadRemoveThreadPtr(th); /* Remove our thread pointer. */

    /* call the user exit function (if there is one) */
    if (th->exitfunc != NULL)
        th->exitfunc(th, th->exitdata);

    /* Posting this semaphore may cause thread logMgr destruction,
     * so NULL logMgr should be supplied to this call
     */
    RvSemaphorePost(&th->exitsignal, NULL);

    if (callDelete == RV_TRUE)
        RvThreadDelete(th);
}

#endif /* RV_THREAD_TYPE != RV_THREAD_NONE */

static void
rvDummyThreadFunc(RvThread *th, void *ctx) {
    (void)th, (void)ctx;
}

/********************************************************************************************
 * RvThreadConstruct
 * Constructs a thread object. No actual thread is created yet. All settings
 * for this thread use the defaults defined in rvccoreconfig.h so that it is
 * not necessary to set every parameter if the default ones are acceptable for
 * this thread.
 * INPUT   : func   - Pointer to function which will executed in the new thread.
 *           data   - User defined data which will be passed to func when it is executed.
 *           logMgr - log manager instance
 * OUTPUT  : th - Pointer to thread object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadConstruct(
    IN RvThreadFunc func,
    IN void *data,
    IN RvLogMgr* logMgr,
    OUT RvThread *th)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus result;
    RvUint32 i;
    static RvUint32 thrCount = 1;

    RvThreadLogEnter((&logMgr->threadSource, "RvThreadConstruct(thread=%p)", th));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((th == NULL) || (func == NULL))
    {
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct: NULL param(s)"));
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvLockGet(&RvThreadLock,logMgr);

    th->logMgr = logMgr;
    if (logMgr != NULL)
        th->threadSource = &logMgr->threadSource;
    else
        th->threadSource = NULL;

    result = RvLockConstruct(logMgr, &th->datalock);
    if(result != RV_OK) {
        RvLockRelease(&RvThreadLock,logMgr);
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct(thread=%p): RvLockConstruct", th));
        return result;
    }

    result = RvSemaphoreConstruct(RvUint32Const(0),logMgr, &th->exitsignal);
    if(result != RV_OK) {
        RvLockDestruct(&th->datalock,logMgr);
        RvLockRelease(&RvThreadLock,logMgr);
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct(thread=%p): RvSemaphoreConstruct", th));
        return result;
    }

    result = RvSemaphoreConstruct(RvUint32Const(0), logMgr, &th->waitListSem);
    if(result != RV_OK) {
        RvLockDestruct(&th->datalock,logMgr);
        RvLockRelease(&RvThreadLock,logMgr);
        RvSemaphoreDestruct(&th->exitsignal, logMgr);
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct(thread=%p): RvSemaphoreConstruct", th));
        return result;
    }


    th->state = RV_THREAD_STATE_INIT;
    th->stackallocated = RV_FALSE;
    th->autodelete = RV_FALSE;

    /* Use RvThreadSetStack to initialize stack info since that has */
    /* the logic for dealing with all the special cases. */
    result = RvThreadSetStack(th, NULL, RV_THREAD_STACKSIZE_DEFAULT);
    if(result != RV_OK) {
        RvLockDestruct(&th->datalock,logMgr);
        RvLockRelease(&RvThreadLock,logMgr);
        RvSemaphoreDestruct(&th->exitsignal, logMgr);
        RvSemaphoreDestruct(&th->waitListSem, logMgr);
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct(thread=%p): RvThreadSetStack", th));
        return result;
    }

    /* Use RvThreadSetStack to initialize stack info since that has */
    /* the logic for dealing with all the special cases. */
    result = RvSemaphoreConstruct(0,logMgr,&th->eventsSleepingCh.iSleepingChSema);
    if(result != RV_OK) {
        RvLockDestruct(&th->datalock,logMgr);
        RvLockRelease(&RvThreadLock,logMgr);
        RvSemaphoreDestruct(&th->exitsignal, logMgr);
        RvSemaphoreDestruct(&th->waitListSem, logMgr);
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct(thread=%p): RvSemaphoreConstruct", th));
        return result;
    }
    
    /* this is not thread safe, but usually threads created at the beginning of the application,
       and even if there is simulataneous '++' access nothing terrible will happen */
    {
        RvUint32 i = thrCount;
        thrCount ++;
        RvSprintf(th->name, "RvDefault_%d", i);
    }
    th->waitdestruct = RV_FALSE;
    th->func = func;
    th->data = data;
    th->priority = RV_THREAD_PRIORITY_DEFAULT;
    memcpy(&th->attr, &RvDefaultThreadAttr, sizeof(th->attr));
    for(i = 0; i < RV_THREAD_MAX_VARS; i++)
        th->vars[i] = NULL; /* Start with vars cleared */
    th->exitfunc = NULL;

    /* OS specific initializations */
    result = RvAdThreadConstruct(&th->tcb);
    if (result != RV_OK)
    {
        RvLockDestruct(&th->datalock,logMgr);
        RvLockRelease(&RvThreadLock,logMgr);
        RvSemaphoreDestruct(&th->exitsignal, logMgr);
        RvSemaphoreDestruct(&th->waitListSem, logMgr);
        RvSemaphoreDestruct(&th->eventsSleepingCh.iSleepingChSema, logMgr);
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstruct(thread=%p): RvAdThreadConstruct failed", th));
        return result;
    }

    RvLockRelease(&RvThreadLock,logMgr);

    RvThreadLogLeave((&logMgr->threadSource, "RvThreadConstruct(thread=%p)", th));

    return result;
}

/********************************************************************************************
 * RvThreadConstructFromUserThread
 * Constructs a thread object and associates it with an existing thread that
 * was created externally (not using RvThreadConstruct). It should be called
 * from the thread itself. Threads which have made this call must call
 * RvThreadDestruct before exiting.
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : th - Pointer to thread object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadConstructFromUserThread(
    IN RvLogMgr* logMgr,
    OUT RvThread *th)
{
/* Construct a thread which is actually a dummy thread used to attach */
/* information to the calling thread, which must be a thread that was not */
/* constructed with the RvThreadConstruct function. This function must be */
/* called from any thread not constructed with the RvThreadConstruct */
/* function before making any other calls to rvthread. Obviously, some */
/* rvthread calls can not be used on threads constructed in this manner. */
    RvStatus result;
    static RvUint32 thrCount = 1;
    RV_USE_CCORE_GLOBALS;

#if (RV_OS_TYPE == RV_OS_TYPE_OSA)
    if(RvAdThreadSetTls(RvThreadCurrentId(), 0, (void*)th) != RV_OK)
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
#endif
    RvThreadLogEnter((&logMgr->threadSource, "RvThreadConstructFromUserThread(thread=%p)", th));

    /* Use the regular construct with a dummy function to set up the basics. */
    result = RvThreadConstruct(rvDummyThreadFunc, NULL, logMgr, th);
    if(result != RV_OK) {
        RvThreadLogError((&logMgr->threadSource, "RvThreadConstructFromUserThread(thread=%p): RvThreadConstruct", th));
        return result;
    }

    /* Now create the special version for our purposes. */
    RvLockGet(&RvThreadLock,logMgr);
    th->state = RV_THREAD_STATE_SPECIAL;
        /* this is not thread safe, but usually threads created at the beginning of the application,
       and even if there is simulataneous '++' access nothing terrible will happen */
    {
        RvUint32 i = thrCount;
        thrCount ++;
        RvSprintf(th->name, "rv%d", i);
    }
    th->id = RvThreadCurrentId();
    RvThreadGetOsPriority(th->id, (RvInt32*)&th->priority);
    result = RvThreadSetupThreadPtr(th);
    RvLockRelease(&RvThreadLock,logMgr);

    RvThreadLogDebug((&logMgr->threadSource, "RvThreadConstructFromUserThread(thread=%p) - thread was constructed.", th));
    RvThreadLogLeave((&logMgr->threadSource, "RvThreadConstructFromUserThread(thread=%p)", th));

    return result;
}


/********************************************************************************************
 * RvThreadDestruct
 * Destruct a thread object. Threads can not be destructed until they have
 * exited so if the thread has not exited this function will wait until
 * it does before returning.
 * Threads created with RvThreadConstructFromUserThread are a special case
 * where RvThreadDestruct MUST be called from the thread itself before
 * exiting (and obviously won't wait for the thread to exit).
 * INPUT   : th - Pointer to thread object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : RvThreadDestruct may only be called once on each thread. Thus
 *           it may not be called simultaneously from multiple threads (with the
 *           same thread to destruct).
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadDestruct(
    IN RvThread *th)
{
    RV_USE_CCORE_GLOBALS;

/* Will wait for a thread to exit and then destroy it. Obviously only */
/* one destruct is allowed on each thread. */
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource, (th->threadSource, "RvThreadDestruct(thread=%p)", th));

    RvLockGet(&RvThreadLock,RvThreadGetLogManager(th));

    /* Check if thread already destroyed. Obviously can't lock */
    /* and could be a problem if the memory has been deallocated. */
    if(th->state == RV_THREAD_STATE_DESTROYED) {
        RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));
        RvLogWarning(th->threadSource,
            (th->threadSource, "RvThreadDestruct(thread=%p): Destroyed already", th));
        return RV_OK;
    }

    RvLockGet(&th->datalock,RvThreadGetLogManager(th));

    if(th->waitdestruct == RV_TRUE) {
        /* Someone is already destructing this thread */
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));
        RvLogWarning(th->threadSource,
            (th->threadSource, "RvThreadDestruct(thread=%p): Destructing already", th));
        return RvThreadErrorCode(RV_THREAD_ERROR_DESTRUCTING);
    }

    /* Deal with application threads (created with RvThreadConstructFromUserThread) */
    if(th->state == RV_THREAD_STATE_SPECIAL) {
        if(RvThreadIdEqual(th->id, RvThreadCurrentId()) == RV_FALSE) {
            /* May only be destructed from thread itself */
            RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));
            RvLogError(th->threadSource,
                (th->threadSource, "RvThreadDestruct(thread=%p): Application thread", th));
            return RvThreadErrorCode(RV_THREAD_ERROR_USERAPP);
        }

        /* Call exit functions (unlock thread lock to prevent dealocks) */
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvThreadCallExits(th);
        RvLockGet(&th->datalock,RvThreadGetLogManager(th));

        /* Remove thread pointer. */
        RvThreadRemoveThreadPtr(th);
   }

    if(th->state == RV_THREAD_STATE_CREATED) {
        /* Task created but never started, just kill it off */
        if(RvThreadDelete(th) != RV_OK) {
            RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
            RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));
            RvLogError(th->threadSource,
                (th->threadSource, "RvThreadDestruct(thread=%p): RvThreadDelete", th));
            return RvThreadErrorCode(RV_ERROR_UNKNOWN);
        }

        /* Wait to make sure thread no longer exists (from OS perspective). */
        /* If any thread specific cleanup has to be done than do it. */
        RvThreadWaitOnExit(th);
    }

    if((th->state == RV_THREAD_STATE_STARTING) || (th->state == RV_THREAD_STATE_RUNNING) ||
       (th->state == RV_THREAD_STATE_EXITING)) {
        /* Task is running, wait for thread wrapper to trigger exit semaphore */
        th->waitdestruct = RV_TRUE;
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));
        RvSemaphoreWait(&th->exitsignal,RvThreadGetLogManager(th));
        RvLockGet(&RvThreadLock,RvThreadGetLogManager(th));
        RvLockGet(&th->datalock,RvThreadGetLogManager(th));

        /* Wait to make sure thread no longer exists (from OS perspective). */
        /* If any thread specific cleanup has to be done than do it. */
        RvThreadWaitOnExit(th);
    }

    /* If the stack had been allocated, we need to free it */
    if(th->stackallocated == RV_TRUE)
        RvMemoryFree(th->stackaddr,RvThreadGetLogManager(th));

    /* OS specific destruction */
    RvAdThreadDestruct(&th->tcb);

    /* Set state and destroy components */
    th->state = RV_THREAD_STATE_DESTROYED;
    RvSemaphoreDestruct(&th->exitsignal,RvThreadGetLogManager(th));
    RvSemaphoreDestruct(&th->waitListSem, RvThreadGetLogManager(th));
    RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
    RvLockDestruct(&th->datalock,RvThreadGetLogManager(th));
    RvSemaphoreDestruct(&th->eventsSleepingCh.iSleepingChSema,RvThreadGetLogManager(th));

    RvLockRelease(&RvThreadLock,RvThreadGetLogManager(th));

    RvLogDebug(th->threadSource, (th->threadSource, "RvThreadDestruct(thread=%p) - thread was destructed", th));
    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadDestruct(thread=%p)", th));

    return RV_OK;
}

/* Physically tell the OS to delete a thread when it has been */
/* created but never started. Called internally only, so no */
/* locking is not needed (should be done by caller). */
static RvStatus RvThreadDelete(RvThread *th)
{
    /* Since POSIX threads (and variations) can't be created without */
    /* being started there is no thread before start is called. Thus, */
    /* we don't have to do anything here. */

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif


    RvLogEnter(th->threadSource, (th->threadSource, "RvThreadDelete(thread=%p)", th));

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    RV_UNUSED_ARG(th);

#else
    if (RvAdThreadDelete(&th->tcb, th->id) != RV_OK) {
        RvLogError(th->threadSource, (th->threadSource, "RvThreadDelete(thread=%p)", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }
#endif

    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadDelete(thread=%p)", th));

    return RV_OK;
}


/********************************************************************************************
 * RvThreadWaitOnExit
 * Wait to make sure thread has completely exited by finding out from
 * the OS. If anything has to be done to release the thread then do it.
 * Called internally only so locking is not needed (should be done
 * by the caller).
 * INPUT   : th - Pointer to thread object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
static RvStatus RvThreadWaitOnExit(
    IN RvThread *th)
{
#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    RV_UNUSED_ARG(th);

#else
    RvStatus result;

    for(;;) {
        result = RvAdThreadWaitOnExit(&th->tcb, th->id);
        if(result == RV_ERROR_UNKNOWN)
            return RvThreadErrorCode(RV_ERROR_UNKNOWN);
        if(result == RV_OK)
            break;
        RvThreadNanosleep(RvInt64FromRvInt32(100 * RV_TIME_NSECPERMSEC), RvThreadGetLogManager(th));
    }
#endif

    return RV_OK;
}


/********************************************************************************************
 * RvThreadCreate
 * Create the actual thread. The thread will be created by the OS and
 * allocate all needed resources but will not begin executing.
 * INPUT   : th - Pointer to thread object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Some operating systems do not allow threads to be started in the
 *           suspended state so the physical thread will not appear until
 *           RvThreadStart is called.}
 *           If the call fails, RvThreadDestruct should be called in order to properly clean up.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadCreate(
    IN RvThread *th)
{
/* If this function fails, the only function that should be called */
/* on the thread is RvThreadDestruct to insure proper cleanup. */
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource, (th->threadSource, "RvThreadCreate(thread=%p)", th));

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    RvLogError(th->threadSource,
        (th->threadSource, "RvThreadCreate(thread=%p): Not supported", th));
    result = RvThreadErrorCode(RV_ERROR_NOTSUPPORTED);

#else
    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadCreate(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    if(th->state != RV_THREAD_STATE_INIT){
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadCreate(thread=%p): Thread has not been constructed", th));
        return RvThreadErrorCode(RV_THREAD_ERROR_CREATED);
    }

    /* Set up the stack for the OS */
    result = RvThreadSetupStack(th);
    if(result != RV_OK) {
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadCreate(thread=%p): RvThreadSetupStack", th));
        return result;
    }

    /* Create the task in the OS (but don't start it). */
    result = RvAdThreadCreate(&th->tcb, th->name, th->priority, &th->attr, th->stackaddr, th->realstacksize, th, &th->id);
    if(result == RV_OK)
        th->state = RV_THREAD_STATE_CREATED;
    else
    {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadCreate(thread=%p): RvAdThreadCreate", th));
    }

    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        /* Deep trouble, do our best. */
        th->state = RV_THREAD_STATE_CREATED;
        RvThreadDelete(th); /* Undo the thread creation */
        /*RK: result = RvThreadErrorCode(RV_ERROR_UNKNOWN);*/
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadCreate(thread=%p): RvLockRelease", th));
    }
#endif  /* RV_THREAD_TYPE == RV_THREAD_NONE */

    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadCreate(thread=%p)", th));

    return result;
}


#if (RV_THREAD_TYPE != RV_THREAD_NONE)
/********************************************************************************************
 * RvThreadSetupStack
 * Do anything special required to set up a stack for the OS including
 * allocating it if needed. The elements stackaddr and
 * stackallocated should be dealt with as needed for the OS. Called
 * internally only so locking is not needed (should be done by caller).
 * INPUT   : th - Pointer to thread object to set the name of.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
static RvStatus RvThreadSetupStack(
    IN RvThread *th)
{
#if (RV_THREAD_SETUP_STACK == RV_NO)
    RV_UNUSED_ARG(th);

#else
    RvStatus result;

    /* For some OS's we must allocate the stack space if it hasn't been done for us */
    if(th->stackaddr == NULL) {
        result = RvMemoryAlloc(NULL, th->stacksize, RvThreadGetLogManager(th), &th->stackaddr);
        if(result != RV_OK)
            return result;
        th->stackallocated = RV_TRUE;
    }
#endif

    return RV_OK;
}
#endif  /* (RV_THREAD_TYPE != RV_THREAD_NONE) */


/********************************************************************************************
 * RvThreadStart
 * Start thread execution.
 * INPUT   : th - Pointer to thread object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : If the call fails, RvThreadDestruct should be called in order to properly clean up.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadStart(
    IN RvThread *th)
{
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    result = RvThreadErrorCode(RV_ERROR_NOTSUPPORTED);
    return result;

#else
    RvLogEnter(th->threadSource, (th->threadSource, "RvThreadStart(thread=%p)", th));

    result = RV_OK;
    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadStart(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    /* We can't start something that hasn't been created or has already been started */
    if(th->state != RV_THREAD_STATE_CREATED){
        if(th->state == RV_THREAD_STATE_INIT) {
            result = RvThreadErrorCode(RV_THREAD_ERROR_NOTCREATED);
            RvLogError(th->threadSource,
                (th->threadSource, "RvThreadStart(thread=%p): Thread has not been created", th));
        } else {
            result = RvThreadErrorCode(RV_THREAD_ERROR_RUNNING);
            RvLogError(th->threadSource,
                (th->threadSource, "RvThreadStart(thread=%p): Thread has not been constructed", th));
        }
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        return result;
    }

    /* Actually tell the OS to start running the task */
    result = RvAdThreadStart(&th->tcb, &th->id, &th->attr, th);
    if (result != RV_OK)
    {
        RvLogError(th->threadSource, (th->threadSource, "RvThreadStart(thread=%p): RvAdThreadStart failed", th));
    }
    else
    {
        th->state = RV_THREAD_STATE_STARTING;
	}


    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        result = RvThreadErrorCode(RV_ERROR_UNKNOWN);
        RvLogError(th->threadSource, (th->threadSource, "RvThreadStart(thread=%p): RvLockRelease", th));
    }

    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadStart(thread=%p)", th));

    return result;
#endif  /* RV_THREAD_TYPE == RV_THREAD_NONE */
}


/********************************************************************************************
 * RvThreadSetExitFunc
 * INPUT   : func - Pointer to function which will executed when thread exits.
 *           data - User defined data which will be passed to func when it is executed.
 *           th   - Pointer to thread object.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetExitFunc(
    IN RvThread *th,
    IN RvThreadFunc func,
    IN void *data)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource, (th->threadSource, "RvThreadSetExitFunc(thread=%p; func=%p)", th, func));

    th->exitfunc = func;
    th->exitdata = data;

    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadSetExitFunc(thread=%p; func=%p)", th, func));

    return RV_OK;
}


/********************************************************************************************
 * RvThreadSetupThreadPtr
 * Attach thread pointer to the task so that it can be retrieved by
 * the RvThreadCurrent function. If an OS can only attach to the
 * current thread (instead of that pointer to by th) then it should
 * do so but the caller needs to account for it. Called internally
 * only so locking is not needed (should be done by caller).
 * INPUT   : th - Pointer to thread object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
static RvStatus RvThreadSetupThreadPtr(
    IN RvThread *th)
{
#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    RvThreadCurrentPtr = th;

#elif  RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL
    return RvThreadTLSSetupThreadPtr(th->id, th);
#else
    /* Use thread local storage to save a copy of the thread pointer. */
    /**** Pointer is set for current task NOT the task pointer to by th. ****/
    if(RvAdThreadSetTls(th->id, th->state, th) != RV_OK)
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
#endif

    return RV_OK;
}


/********************************************************************************************
 * RvThreadRemoveThreadPtr
 * Remove thread pointer from a task. Obviously, if the pointer
 * can only be removed from the current task then that is what it
 * will do.
 * INPUT   : th - Pointer to thread object to be destructed.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
static RvStatus RvThreadRemoveThreadPtr(
    IN RvThread *th)
{
#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    RV_UNUSED_ARG(th);
    RvThreadCurrentPtr = NULL;
#elif  RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL
    return RvThreadTLSRemoveThreadPtr(th->id);
#else
    /* Used thread local storage to save a copy of the thread pointer, just clear it. */
    /**** Pointer is cleared for current task NOT the task pointer to by th. ****/
    if(RvAdThreadSetTls(th->id, th->state, NULL) != RV_OK)
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
#endif

    return RV_OK;
}


/********************************************************************************************
 * RvThreadCurrent
 * Get the thread handle of the current thread.
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : A pointer to the thread object of the current thread.
 * NOTE    : If the thread was not created with this Thread module or was
 *           not attach to a thread object with RvThreadConstructFromUserThread
 *           then this function will return NULL.
 */

RVCOREAPI
RvThread * RVCALLCONV RvThreadCurrentEx(RvBool autoCreate)
{
/* Can only be used by threads which have been created using this module */
/* or have used RvThreadConstructFromUserThread to attach to a user thread. */
    RvThread *th = 0;

    RV_UNUSED_ARG(autoCreate);

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    th = RvThreadCurrentPtr;
	return th;
#elif  RV_THREAD_TLS_TYPE == RV_THREAD_TLS_MANUAL
    {
        RvThreadId id;
        RvStatus s = RV_OK;

        id = RvThreadCurrentId();
        s = RvThreadTLSGetThreadPtr(id, &th);
        if(s != RV_OK) {
            return 0;
        }

        return th;
    }
#else
    th = (RvThread *)RvAdThreadGetTls();
    /* We need to check for tasks which have no variable set */
#if RV_THREAD_USE_AUTOMATIC_INTERNING

    if(th == 0 && autoCreate) {
        RvStatus s = RV_OK;

        s = RvMemoryAlloc(0, sizeof(RvThread), 0, (void **)&th);
        if(s != RV_OK) {
            return 0;
        }

        s = RvThreadConstructFromUserThread(0, th);
        if(s != RV_OK) {
            RvMemoryFree(th, 0);
            return 0;
        }

        s = RvThreadSetAutoDelete(th, RV_TRUE);

        if(s != RV_OK) {
            RvThreadDestruct(th);
            RvMemoryFree(th, 0);
            return 0;
        }

    }

#endif

	if(th != 0 && th->id != RvThreadCurrentId()) {
		th = 0;
	}



    return th;
#endif
}


/********************************************************************************************
 * RvThreadCurrentId
 * Get the OS specific thread ID of the current thread.
 * INPUT   : None.
 * OUTPUT  : None.
 * RETURN  : The thread ID of the current thread.
 * NOTE    : This works for all threads regardless of how they were created.
 */
RVCOREAPI
RvThreadId RVCALLCONV RvThreadCurrentId(void)
{
#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    return 1;
#else
    return RvAdThreadCurrentId();
#endif
}



/********************************************************************************************
 * RvThreadGetId
 * Get the OS specific thread ID of a given thread.
 * INPUT   : th - Pointer to thread object.
 * OUTPUT  : None.
 * RETURN  : The thread ID of the thread.
 * NOTE    : This works for all threads regardless of how they were created.
 */
RVCOREAPI
RvThreadId RVCALLCONV RvThreadGetId(
    IN RvThread *th)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return (RvThreadId)0;
#endif

    return th->id;
}

/********************************************************************************************
 * RvThreadIdEqual
 * Compares two thread IDs.
 * INPUT   : id1 - Thread ID.
 *           id2 - Thread ID.
 * OUTPUT  : None.
 * RETURN  : RV_TRUE if threads are the same, otherwise RV_FALSE.
 */
RVCOREAPI
RvBool RVCALLCONV RvThreadIdEqual(
    IN RvThreadId id1,
    IN RvThreadId id2)
{
#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    return id1 == id2;
#else

    return RvAdThreadIdEqual(id1, id2);
#endif
}


/********************************************************************************************
 * RvThreadSleep
 * Suspends the current thread for the requested amount of time.
 * INPUT   : t - Pointer to RvTime structure containing amount of time to sleep.
 *           logMgr - log manager instance
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : This works for all threads regardless of how they were created.
 *           The exact time of suspension is based on the operating system and the
 *           resolution of the system clock.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSleep(
    IN const RvTime *t,
    IN RvLogMgr* logMgr)
{
    RvStatus status = RV_OK;

    RvThreadLogEnter((&logMgr->threadSource, "RvThreadSleep(t=%d:%d)", t->sec, t->nsec));

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    /* Not supported... */
    RV_UNUSED_ARG(t);
    RvThreadLogError((&logMgr->threadSource, "RvThreadSleep: Not supported"));
	status = RV_ERROR_NOTSUPPORTED;

#else /* RV_THREAD_TYPE != RV_THREAD_NONE */

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(t == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    status = RvAdThreadSleep(t);
    if(status == RV_ERROR_NOTSUPPORTED) {
        /* It's easier to just convert to nanosecs */
        status = RvThreadNanosleep(RvTimeConvertTo64(t),logMgr);
    }

    RvThreadLogLeave((&logMgr->threadSource, "RvThreadSleep"));

#endif /* RV_THREAD_TYPE == RV_THREAD_NONE */

    return status;

}


/********************************************************************************************
 * RvThreadNanosleep
 * Suspends the current thread for the requested amount of time.
 * INPUT   : nsecs - Time to sleep in nanoseconds.
 *           logMgr - log manager structure
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : This works for all threads regardless of how they were created.
 *           The exact time of suspension is based on the operating system and the
 *           resolution of the system clock.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadNanosleep(
    IN RvInt64 nsecs,
    IN RvLogMgr* logMgr)
{
    RvThreadLogEnter((&logMgr->threadSource, "RvThreadNanosleep(nsecs=%ld)", nsecs));

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    /* Not supported... */
    RV_UNUSED_ARG(nsecs);
    RvThreadLogError((&logMgr->threadSource, "RvThreadNanosleep: Not supported"));
    return RvThreadErrorCode(RV_ERROR_NOTSUPPORTED);

#else

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(RvInt64IsLessThanOrEqual(nsecs, RvInt64Const(0,0,0))) {
        RvThreadLogError((&logMgr->threadSource, "RvThreadNanosleep: Range"));
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
    }
#endif

    RvAdThreadNanosleep(nsecs);

    RvThreadLogLeave((&logMgr->threadSource, "RvThreadNanosleep(nsecs=%ld)", nsecs));

    return RV_OK;
#endif
}


/********************************************************************************************
 * RvThreadGetOsName
 * Gets the name of a thread as seen by the operating system.
 * INPUT   : id - Thread ID.
 *           buf - Pointer to buffer where the name will be copied.
 *           size - Size of the buffer.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : This works for all threads regardless of how they were created.
 *           If the buffer is too small for the name the name will be truncated.
 *           Only works on threads that exist (are executing).
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadGetOsName(
    IN RvThreadId id,
    IN RvInt32 size,
    OUT RvChar *buf)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(buf == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(size < RvInt32Const(1))
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
#endif

#if RV_THREAD_TYPE == RV_THREAD_NONE
    RV_UNUSED_ARG(id);
    buf[0] = 0;
#else
    if(RvAdThreadGetName(id, size, buf) != RV_OK)
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
#endif

    return RV_OK;
}


/********************************************************************************************
 * RvThreadGetOsPriority
 * Gets the priority of a thread as seen by the operating system.
 * INPUT   : id - Thread ID.
 *           priority - Pointer to location where the priority value will be stored.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : This works for all threads regardless of how they were created.
 *           Only works on threads that exist (are executing).
 */
static RvStatus RvThreadGetOsPriority(
    IN RvThreadId id,
    IN RvInt32 *priority)
{
    RvStatus status = RV_OK;

#if (RV_THREAD_TYPE == RV_THREAD_NONE)
    RV_UNUSED_ARG(id);
    *priority = 0;

#else
    status = RvAdThreadGetPriority(id, priority);
#endif

    return status;
}

/********************************************************************************************
 * RvThreadGetName
 * Gets the name of a thread.
 * INPUT   : th - Pointer to thread object to get the name of.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : The pointer returned is only valid until the thread is destructed.
 */
RVCOREAPI
RvChar * RVCALLCONV RvThreadGetName(
    IN RvThread *th)
{
/* Result is only valid until thread is destructed */
/*
 * !!! Don't instrument this function with log calls !!!
 *     this function is called by the log itself
 */
    RvChar *result;
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return NULL;
#endif

#if 0
    /* why to bother - if the function returns a pointer and not the content itself ? */
    /* we (CC) prefer not to lock, since RvLogTextAny() calls this function,
       which may cause deadlock (when datalock is already locked by any RvThread function) */
    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK)
        return NULL;
#endif

    result = th->name;

#if 0
    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK)
        return NULL;
#endif

    return result;
}


/********************************************************************************************
 * RvThreadSetName
 * Sets the name of a thread.
 * INPUT   : th - Pointer to thread object to set the name of.
 *           name - name of the thread
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : A copy of the string passed in is made. The maximum size of the
 *           the string is RV_THREAD_MAX_NAMESIZE and names that are too long
 *           will be truncated.
 *           The name can not be changed once a thread has been created.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetName(
    IN RvThread *th,
    IN const RvChar *name)
{
    RvStatus result;
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((th == NULL) || (name == NULL))
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource,
        (th->threadSource, "RvThreadSetName(thread=%p; name=%s)", th, name));

    result = RV_OK;
    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetName(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }
    if ((th->state == RV_THREAD_STATE_INIT) || (th->state == RV_THREAD_STATE_SPECIAL))
    {
        strncpy(th->name, name, RV_THREAD_MAX_NAMESIZE);
        th->name[RV_THREAD_MAX_NAMESIZE - 1] = '\0';
    } else {
        result = RvThreadErrorCode(RV_THREAD_ERROR_CREATED);
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetName(thread=%p): Thread has not been constructed", th));
    }
    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetName(thread=%p): RvLockRelease", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadSetName(thread=%p)", th));

    return result;
}


/********************************************************************************************
 * RvThreadSetAutoDelete
 * Auto-deletion parameter cannot be changed after thread is created. This function should
 * only be called for threads constructed with RvThreadConstructFromUserThread().
 * INPUT   : th - Pointer to thread object to set the name of.
 *           autoDelete - RV_TRUE to allow autoDelete, otherwise RV_FALSE.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetAutoDelete(
    IN RvThread *th,
    IN RvBool autoDelete)
{
    RvStatus result = RV_OK;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource,
        (th->threadSource, "RvThreadSetAutoDelete(thread=%p; autoDelete=%d)", th, autoDelete));

#if (RV_THREAD_AUTO_DELETE == RV_NO)
    /* This OS's can't support this feature */
    RV_UNUSED_ARG(th);
    RV_UNUSED_ARG(autoDelete);
    RvLogError(th->threadSource,
        (th->threadSource, "RvThreadSetAutoDelete(thread=%p): Not supported", th));
	result = RV_ERROR_NOTSUPPORTED;
#else

    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetAutoDelete(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }
    if (th->state == RV_THREAD_STATE_SPECIAL)
    {
        th->autodelete = autoDelete;
    } else {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetAutoDelete(thread=%p): Thread has not been constructed", th));
        result = RvThreadErrorCode(RV_THREAD_ERROR_CREATED);
    }
    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetAutoDelete(thread=%p): RvLockRelease", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }
    RvLogLeave(th->threadSource, (th->threadSource, "RvThreadSetAutoDelete(thread=%p)", th));
#endif


    return result;
}


/********************************************************************************************
 * RvThreadSetStack
 * Sets the stack information for a thread.
 * INPUT   : th - Pointer to thread object to set the stack information for.
 *           stackaddr - Address of memory to use for stack (NULL means to allocate it).
 *           stacksize - Size of the stack.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : The stack information can not be changed once a thread has been created.
 *           If stackaddr is set to NULL, the amount of stack space used for internal overhead
 *           will be allocated in addition to the requested stack size.
 *           If stackaddr is set to NULL and the stacksize is set to 0, the default
 *           stack settings will be used (see rvccoreconfig.h).
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetStack(
    IN RvThread *th,
    IN void *stackaddr,
    IN RvInt32 stacksize)
{
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL) {
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvLogEnter(th->threadSource,
        (th->threadSource, "RvThreadSetStack(thread=%p; stacksize=%d)", th, stacksize));

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(stacksize < 0) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): Range(stacksize < 0)", th));
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
    }

    if((stacksize == 0) && (stackaddr != NULL)) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): Bad param(stackaddr != NULL)", th));
        return RvThreadErrorCode(RV_ERROR_BADPARAM);
    }

    if((stacksize > 0) && (stackaddr != NULL) &&
        (stacksize <= (RvInt32)(RV_THREAD_WRAPPER_STACK + RV_THREAD_ADDITIONAL_STACK))) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): Bad param(stackaddr != NULL)", th));
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
    }

#if (RV_THREAD_STACK_ADDR_USER == RV_NO)
    if(stackaddr != NULL) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): Not supported", th));
        return RvThreadErrorCode(RV_ERROR_NOTSUPPORTED);
    }
#endif
#endif /* RANGECHECK */

    result = RV_OK;
    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    /* Can only be changed before created */
    if(th->state != RV_THREAD_STATE_INIT) {
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): Thread has not been constructed", th));
        return RvThreadErrorCode(RV_THREAD_ERROR_CREATED);
    }

    th->stackaddr = stackaddr;
    if(stacksize == 0) {
        /* Set to default value */
        th->reqstacksize = RV_THREAD_STACKSIZE_DEFAULT;
    } else th->reqstacksize = stacksize;

    if(th->stackaddr == NULL) {
        /* Set default level if below configured threshold */
        if(th->reqstacksize < RV_THREAD_STACKSIZE_USEDEFAULT)
            th->reqstacksize = RV_THREAD_STACKSIZE_DEFAULT;

        /* Add our overhead to the requested stack size to get real stack size */
        th->realstacksize = th->reqstacksize + RV_THREAD_WRAPPER_STACK;

        /* Account for any extra memory required above and beyond that which will */
        /* be used for the stack itself. */
        th->stacksize = th->realstacksize + RV_THREAD_ADDITIONAL_STACK;
    } else {
        /* We've been given the memory so we have to make due with its size */
        th->stacksize = th->reqstacksize;
        th->realstacksize = th->reqstacksize - RV_THREAD_ADDITIONAL_STACK;
    }

    /* Deal with OS specific special cases */
#if (RV_THREAD_STACK_SIZE_OS == RV_YES)
    /* if requested size was 0, then we want to OS to determine size */
    if(th->reqstacksize == 0) {
        th->stacksize = 0;
        th->realstacksize = 0;
    }
#endif

    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetStack(thread=%p): RvLockRelease", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLogLeave(th->threadSource,
        (th->threadSource, "RvThreadSetStack(thread=%p)", th));

    return result;
}


/********************************************************************************************
 * RvThreadSetPriority
 * Sets the stack priority for a thread.
 * INPUT   : th - Pointer to thread object to set the stack priority of.
 *           priority - Priority to set thread to.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetPriority(
    IN RvThread *th,
    IN RvInt32 priority)
{
    RvStatus result;
    RvInt32 oldpri;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource,
        (th->threadSource, "RvThreadSetPriority(thread=%p; priority=%d)", th, priority));

    if(th->state == RV_THREAD_STATE_DESTROYED) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetPriority(thread=%p): Thread destructed", th));
        return RvThreadErrorCode(RV_ERROR_DESTRUCTED);
    }

    result = RV_OK;
    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetPriority(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    if(th->state == RV_THREAD_STATE_SPECIAL) { /* Don't change user apps */
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetPriority(thread=%p): Application thread", th));
        return RvThreadErrorCode(RV_THREAD_ERROR_USERAPP);
    }

    oldpri = th->priority;
    th->priority = priority;
    if((th->priority != oldpri) && ((th->state == RV_THREAD_STATE_CREATED) ||
                                   (th->state == RV_THREAD_STATE_STARTING) ||
                                   (th->state == RV_THREAD_STATE_RUNNING))) {
        /* Thread is currently running so tell the OS the new priority */
        RvStatus s = RvAdThreadSetPriority(&th->tcb, th->id, priority, th->state);
        if(s != RV_OK) {
            result = RvThreadErrorCode(RV_ERROR_UNKNOWN);
            RvLogError(th->threadSource,
                (th->threadSource, "RvThreadSetPriority(thread=%p): SetThreadPriority", th));
        }

        /* If the change failed go back */
        if(result != RV_OK)
            th->priority = oldpri;
    }
    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetPriority(thread=%p): RvLockRelease", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLogLeave(th->threadSource,
        (th->threadSource, "RvThreadSetPriority(thread=%p)", th));

    return result;
}


/********************************************************************************************
 * RvThreadGetAttr
 * Gets the current attributes for a thread.
 * INPUT   : th - Pointer to thread object to get the attributes of.
 *           attr - Pointer to OS specific thread attributes structure where the values will be copied.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Information about the attributes structure and its settings can be found in
 *           the rvccoreconfig.h and rvthread.h file.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadGetAttr(
    IN RvThread *th,
    OUT RvThreadAttr *attr)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((th == NULL) || (attr == NULL))
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource,
        (th->threadSource, "RvThreadGetAttr(thread=%p)", th));

    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadGetAttr(thread=%p): Unable to lock datalock", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    memcpy(attr, &th->attr, sizeof(*attr));

    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadGetAttr(thread=%p): RvLockRelease", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLogLeave(th->threadSource,
        (th->threadSource, "RvThreadGetAttr(thread=%p)", th));

    return RV_OK;
}


/********************************************************************************************
 * RvThreadSetAttr
 * Sets the attributes for a thread.
 * INPUT   : th - Pointer to thread object to set the attributes of.
 *           attr - Pointer to OS specific thread attributes to begin using (NULL = use defaults).
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : The attributes can not be changed once a thread has been created.
 *           The default values for these attributes are set in rvccoreconfig.h.
 *           Further information about these attributes can be found in that file and
 *           the rvthread.h file.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetAttr(
    IN RvThread *th,
    IN const RvThreadAttr *attr)
{
    RV_USE_CCORE_GLOBALS;
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(th == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvLogEnter(th->threadSource,
        (th->threadSource, "RvThreadSetAttr(thread=%p)", th));

    if(RvLockGet(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    if(th->state == RV_THREAD_STATE_SPECIAL) { /* Don't change user apps */
        RvLockRelease(&th->datalock,RvThreadGetLogManager(th));
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetAttr(thread=%p): Application thread", th));
        return RvThreadErrorCode(RV_THREAD_ERROR_USERAPP);
    }

    /* Do OS specific stuff here */
    if(attr != NULL) {
        memcpy(&th->attr, attr, sizeof(th->attr));
    } else memcpy(&th->attr, &RvDefaultThreadAttr, sizeof(th->attr));

    if(RvLockRelease(&th->datalock,RvThreadGetLogManager(th)) != RV_OK) {
        RvLogError(th->threadSource,
            (th->threadSource, "RvThreadSetAttr(thread=%p): RvLockRelease", th));
        return RvThreadErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLogLeave(th->threadSource,
        (th->threadSource, "RvThreadSetAttr(thread=%p)", th));

    return RV_OK;
}

static 
RvStatus rvThreadFindVarAux(const RvChar *name, RvUint32 *idx) {
    RvInt i;
    RV_USE_CCORE_GLOBALS;

    
    *idx = RV_THREAD_MAX_VARS;
    for(i = 0; i < RV_THREAD_MAX_VARS; i++) {
        RvThreadVar *cur = &RvThreadVars[i];
        
        if(!cur->active) {
            *idx = i;
        } else if(name && !strcmp(cur->name, name)) {
            *idx = i;
            return RV_OK;
        }
    }
    
    return RV_THREAD_WARNING_NOTFOUND;
}

/********************************************************************************************
 * RvThreadCreateVar
 * Creates a thread specific variable for all threads.
 * INPUT   : exitfunc - Pointer to a function which will be called for this variable when a thread exits (NULL = none).
 *           name - Pointer to a name string to identify the variable (NULL = no name).
 *           logMgr - log manager instance
 * OUTPUT  : index - Pointer to a location to store the index of the variable created.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Only threads constructed with RvThreadConstruct or RvThreadConstructFromUserThread
 *           may use thread specific variables.
 *           For threads created with RvThreadConstructFromUserThread, the exit function
 *           will be called when the thread calls RvThreadDestruct.
 *           The maximum number of thread specific variables is RV_THREAD_MAX_VARS and is
 *           configured in the rvccoreconfig.h file.
 *           A copy of the name string passed in is made. The maximum size of the
 *           the string is RV_THREAD_MAX_VARNAMESIZE and names that are too long
 *           will be truncated.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadCreateVar(
    IN RvThreadVarFunc exitfunc,
    IN const RvChar *name,
    IN RvLogMgr* logMgr,
    OUT RvUint32 *index)
{
    RvStatus s;

    RV_USE_CCORE_GLOBALS;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(index == NULL)
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

/* Create a thread specific variable. An id to reference the variable */
/* with is put into the index parameter. The exitfunc (if not NULL) */
/* will be called within the context exiting thread when it exits. */
/* The name is used by the find command to look up thread ids so names */
/* that are to be referenced should be unique (there is no check). The */
/* name can simply be NULL. In the case of a user application */
/* thread which has called RvThreadConstructFromUserThread, the exit */
/* function will be called when RvThreadDestruct is called. The exit */
/* function must not make any calls to RvThreadConstruct, RvThreadDestruct, */
/* RvThreadCreateVar, RvThreadDeleteVar, or any variation of them. The exit */
/* function will be passed the current thread pointer and the index of the */
/* variable to operate on. */

    RvThreadLogEnter((&logMgr->threadSource, "RvThreadCreateVar(name=%s)", name));

    RvLockGet(&RvThreadLock,logMgr);
    s = rvThreadFindVarAux(name, index);
    if(s != RV_OK) {
        if(*index < RV_THREAD_MAX_VARS) {
            RvThreadVar *cur = &RvThreadVars[*index];
            cur->active = RV_TRUE;
            cur->exitfunc = exitfunc;
            if(name) {
                size_t len = sizeof(cur->name);
                strncpy(cur->name, name, len);
                cur->name[len - 1] = 0;
            } else {
                cur->name[0] = 0;
            }

            s = RV_OK;
        } else {
            s = RvThreadErrorCode(RV_THREAD_ERROR_NOVARS);
        }
    }
    RvLockRelease(&RvThreadLock, logMgr);
    if(s == RV_OK) {
        RvThreadLogLeave((&logMgr->threadSource, "RvThreadCreateVar"));
        return RV_OK;
    }

    RvThreadLogError((&logMgr->threadSource, "RvThreadCreateVar: No variables"));
    return s;
}



/********************************************************************************************
 * RvThreadDeleteVar
 * Deletes a thread specific variable for all threads.
 * INPUT   : index - Index of thread specific variable to be deleted.
 *           logMgr - Pointer to the log manager
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Exit functions for the variable will NOT be called.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadDeleteVar(
    IN RvUint32 index,
    IN RvLogMgr* logMgr)
{
    RV_USE_CCORE_GLOBALS;
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(index >= RV_THREAD_MAX_VARS)
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
#endif

    RvThreadLogEnter((&logMgr->threadSource, "RvThreadDeleteVar(index=%d)", index));

    RvLockGet(&RvThreadLock,logMgr);
    RvThreadVars[index].active = RV_FALSE;
    RvLockRelease(&RvThreadLock,logMgr);

    RvThreadLogLeave((&logMgr->threadSource, "RvThreadDeleteVar"));

    return RV_OK;
}

/********************************************************************************************
 * RvThreadSetVar
 * Sets the value of a thread specific variable for the current thread.
 * INPUT   : index - Index of thread specific variable to be set.
 *           value - Value to set variable to.
 *           logMgr - Pointer to the log manager
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Only threads constructed with RvThreadConstruct or RvThreadConstructFromUserThread
 *           may use thread specific variables.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadSetVar(
    IN RvUint32 index,
    IN void *value,
    IN RvLogMgr* logMgr)
{
    RvThread *th;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(index >= RV_THREAD_MAX_VARS)
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
#endif

/* Note: for speed purposes, no check is made to insure that the variable */
/* has been created. This must be insured by the application. */

    if (logMgr != NULL)
        RvThreadLogEnter((&logMgr->threadSource, "RvThreadSetVar(index=%d)", index));

    th = RvThreadCurrent();
    if(th == NULL) {
        RvThreadLogError((&logMgr->threadSource, "RvThreadSetVar: Invalid thread"));
        return RvThreadErrorCode(RV_THREAD_ERROR_BADTHREAD); /* No thread structure for this thread */
    }
    th->vars[index] = value;

    if (logMgr != NULL)
        RvThreadLogLeave((&logMgr->threadSource, "RvThreadSetVar"));

    return RV_OK;
}

/********************************************************************************************
 * RvThreadGetVar
 * Gets the value of a thread specific variable for the current thread.
 * INPUT   : index - Index of thread specific variable to be retrieved.
 *           logMgr - log manager instance
 * OUTPUT  : value - Pointer to a location to store the current value of the variable.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Only threads constructed with RvThreadConstruct or RvThreadConstructFromUserThread
 *           may use thread specific variables.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadGetVar(
    IN RvUint32 index,
    IN RvLogMgr* logMgr,
    OUT void **value)
{
    RvThread *th;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(index >= RV_THREAD_MAX_VARS)
        return RvThreadErrorCode(RV_ERROR_OUTOFRANGE);
#endif

    if (logMgr != NULL)
        RvThreadLogEnter((&logMgr->threadSource, "RvThreadGetVar(index=%d)", index));

    th = RvThreadCurrent();
    if(th == NULL) {
        RvThreadLogError((&logMgr->threadSource, "RvThreadGetVar: Invalid thread"));
        return RvThreadErrorCode(RV_THREAD_ERROR_BADTHREAD); /* No thread structure for this thread */
    }
    *value = th->vars[index];

    if (logMgr != NULL)
        RvThreadLogLeave((&logMgr->threadSource, "RvThreadGetVar"));

    return RV_OK;
}


/********************************************************************************************
 * RvThreadFindVar
 * Finds the index of a thread specific variable by its name.
 * INPUT   : name - Pointer to a name string of the variable to find.
 *           logMgr - log manager instance
 * OUTPUT  : index - Pointer to a location to store the index of the variable found.
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : If multiple variables have the same name, the first one found
 *           will be returned.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadFindVar(
    IN const RvChar *name,
    IN RvLogMgr* logMgr,
    OUT RvUint32 *index)
{
    RvStatus s;
    RV_USE_CCORE_GLOBALS;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((name == NULL) || (index == NULL))
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif

    RvThreadLogEnter((&logMgr->threadSource, "RvThreadFindVar(name=%s)", name));

    RvLockGet(&RvThreadLock, logMgr);
    s = rvThreadFindVarAux(name, index);
    RvLockRelease(&RvThreadLock, logMgr);
    if(s == RV_OK) {
        RvThreadLogLeave((&logMgr->threadSource, "RvThreadFindVar"));
        return s;
    }
    RvThreadLogError((&logMgr->threadSource, "RvThreadFindVar: Not found"));
    return RvThreadErrorCode(RV_THREAD_WARNING_NOTFOUND);
}


/********************************************************************************************
 * RvThreadCallExits
 * Call the exit functions for any active thread specific variables.
 * Called internally only so locking is not needed (should be done by
 * caller). Caller's locking should take callbacks into account so don't
 * have the thread's individual lock locked or any calls back to rvthread
 * will deadlock.
 * INPUT   : th - Pointer to a thread instance.
 * OUTPUT  : None.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
static RvStatus RvThreadCallExits(RvThread *th)
{
    RvUint32 i;
    RV_USE_CCORE_GLOBALS;

    for(i = 0; i < RV_THREAD_MAX_VARS; i++) {
        if((RvThreadVars[i].active == RV_TRUE) && (RvThreadVars[i].exitfunc != NULL))
            RvThreadVars[i].exitfunc(th, th->vars[i], i); /* Send thread pointer, var value, and index */
    }
    return RV_OK;
}


/********************************************************************************************
 * RvThreadJoin
 * Finds the index of a thread specific variable by its name.
 * INPUT   : th - Pointer to thread object to be
 * OUTPUT  : *pResult FALSE if thread calls to RvThreadJoin to itself
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvThreadJoin(
								 IN RvThread *th,
								 OUT RvBool *pResult)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((th == 0) || (pResult == 0))
        return RvThreadErrorCode(RV_ERROR_NULLPTR);
#endif
	if (RvThreadCurrentId() == th->id)
	{
        *pResult = RV_FALSE;
		return RV_OK;
	}

    *pResult = RV_TRUE; /* THREAD is DEAD */

	/*	not needed RvLockGet(&RvThreadLock,RvThreadGetLogManager(th)); */
	if (th->state == RV_THREAD_STATE_INIT) {
        /* Nothing to join to */
        return RvThreadErrorCode(RV_THREAD_ERROR_NOTCREATED);
    }

	RvSemaphoreWait(&th->exitsignal, RvThreadGetLogManager(th));
	RvSemaphorePost(&th->exitsignal, RvThreadGetLogManager(th));
	*pResult = RV_TRUE;
	return RV_OK;
}


/* Make sure we delete threads automatically if necessary */
void RvThreadExitted(
    IN void*            ptr)
{
    RvThread* th = (RvThread *)ptr;

    if (th == NULL)
        th = RvThreadCurrentEx(RV_FALSE);

    if ((th != NULL) && (th->autodelete == RV_TRUE))
    {
        /* We need to delete this thread and it's variables... */
        RvThreadDestruct(th);

        /* Although this isn't clean, we assume the memory was allocated by
           the caller using RvMemoryAlloc() just before
           RvThreadConstructFromUserThread() */
        RvMemoryFree(th,RvThreadGetLogManager(th));
    }
}

#if 0

/* Thread wait list functions */

RVCOREAPI
RvStatus RVCALLCONV RvThreadWaitListConstruct(RvThreadWaitList *self, RvMutex *mtx, RvLogMgr *logMgr) {
    (void)logMgr;
    self->first = 0;
    self->last  = 0;
    self->mtx   = mtx;
    return RV_OK;
}

/* This function should be called in 'locked' state of associated mutex */
RVCOREAPI
RvStatus RVCALLCONV RvThreadWaitListRelease(RvThreadWaitList *self, /* RvInt mode, */ RvLogMgr *logMgr) {
    RvThread *cur;
    RvThread *next;
    RvInt32   lockCnt = 0;

    RV_UNUSED_ARG(lockCnt);

    RvThreadLogEnter((LOGSRC, "RvThreadWaitListRelease(waitList=%p)", self));

    for(cur = self->first; cur != 0; cur = next) {
        next = cur->waitListNext;
        cur->waitListNext = 0;
        cur->waitListPrev = 0;
        RvThreadLogDebug((LOGSRC, "RvThreadWaitListRelease(waitList=%p): releasing thread %p", self, cur));
        RvSemaphorePost(&cur->waitListSem, logMgr);
    }

    RvMutexRelease(self->mtx, logMgr, &lockCnt);
    RvThreadLogLeave((LOGSRC, "RvThreadWaitListRelease(waitList=%p)", self));
    return RV_OK;
}

/* This function should be called in 'locked' state of associated mutex */

RVCOREAPI
RvStatus RVCALLCONV RvThreadWait(RvThreadWaitList *self, RvLogMgr *logMgr) {
    RvThread *th = RvThreadCurrent();
    RvStatus s = RV_OK;
    RvInt32 lockCnt = 0;

    RV_UNUSED_ARG(lockCnt);

    if(th == 0) {
        RvThreadLogError((LOGSRC, "RvThreadWait(waitList = %p) failed - not a CC thread", self));
        return RV_THREAD_EBADTHREAD;
    }

    RvThreadLogEnter((LOGSRC, "RvThreadWait(waitList=%p, thread=%p)", self, th));

    th->waitListPrev = self->last;
    th->waitListNext = 0;

    if(self->last != 0) {
        self->last->waitListNext = th;
    } else {
        self->first = th;
    }

    self->last = th;
    s = RvMutexRelease(self->mtx, logMgr, &lockCnt);
    RvSemaphoreWait(&th->waitListSem, logMgr);

    RvThreadLogLeave((LOGSRC, "RvThreadWait(waitList=%p, thread=%p)", self, th));

    return s;
}
#endif

