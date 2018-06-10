/***********************************************************************
Filename   : rvtimerengine.c
Description: engine which services timer queue in a seperate thread
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
#include "rvtimerengine.h"
#include "rvtimestamp.h"


/* Lets make error codes a little easier to type */
#define RvTimerEngineErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CBASE, RV_CBASE_MODULE_TIMERENGINE, (_e))

/********************************************************************************************
 * RvTimerEngineInit
 *
 * Initializes the TimerEngine module. Must be called once (and
 * only once) before any other functions in the module are called.
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimerEngineInit(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvTimerEngineEnd
 *
 * Shuts down the TimerEngine module. Must be called once (and
 * only once) when no further calls to this module will be made.
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimerEngineEnd(void)
{
    return RV_OK;
}


#if (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)
/* Single threaded compilations can't use the timer engine,
   so they don't actually "compile" this file */

static void RvTimerEngineThread(RvThread *th, void *data);

/********************************************************************************************
 * RvTimerEngineConstruct
 *
 * Constructs a timer engine based on the parameters passed in.
 * INPUT   : tengine - Pointer to timer engine object to be constructed.
 *           tqueue - Pointer to timer queue object that needs to be serviced.
 *           period - Rate (in nanoseconds) at which timer should be serviced.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : The configuration of the timer engine thread will use the RvThread default
 *           settings. To change the thread configuration, use the RvTimerEngineSetOptions
 *           function before starting the engine.
 *           The actual period at which the timer queue is serviced may not be precisely
 *           the period requested. The acutal period will depend on the resolution and
 *           acuracy of the operatings systems schedular.
 */
RVCOREAPI RvStatus RVCALLCONV RvTimerEngineConstruct(
    IN RvTimerEngine *tengine,
    IN RvTimerQueue *tqueue,
    IN RvInt64 period)
{
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((tengine == NULL) || (tqueue == NULL))
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if(RvInt64IsLessThan(period, RvInt64Const(0, 0, 0)))
        return RvTimerEngineErrorCode(RV_ERROR_OUTOFRANGE);
#endif

    /* Set up Engine information. */
    tengine->period = period;
    tengine->tqueue = tqueue;
    tengine->stopped = RV_FALSE;
    tengine->paused = RV_FALSE;

    /* Construct the thread. */
    result = RvThreadConstruct(RvTimerEngineThread, tengine, NULL, &tengine->thread);
    if(result != RV_OK)
        return result;

    return RV_OK;
}

/********************************************************************************************
 * RvTimerEngineDestruct
 *
 * Destructs a timer engine.
 * INPUT   : tengine - Pointer to timer engine object to be Destructed.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : RvTimerEngineDestruct may only be called once on each timer engine.
 *           Thus it may not be called simultaneously from multiple threads (with
 *           the same timer engine to destruct).}
 *           This function will suspend the calling thread until the timer engine
 *           completes any callbacks that are in progess and until the timer engine
 *           (and associated thread) is completely shut down.
 */
RVCOREAPI RvStatus RVCALLCONV RvTimerEngineDestruct(
    IN RvTimerEngine *tengine)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tengine == NULL)
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif
    RvTimerEngineStop(tengine);
    return RvThreadDestruct(&tengine->thread);
}

/* Actual Timer Engine thread. Set RV_TIMERENGINE_ADDITIONAL_STACK */
/* to the amount of additional stackspace required by this thread */
/* on top of those needed for the timer callbacks themselves. */
#define RV_TIMERENGINE_ADDITIONAL_STACK 1024 /* Estimate */
static void RvTimerEngineThread(RvThread *th, void *data)
{
    RvTimerEngine *tengine;
    RvInt64 nextservice, delay;

    RV_UNUSED_ARG(th);

    tengine = (RvTimerEngine *)data;
    nextservice = RvTimestampGet(NULL);
    while(tengine->stopped == RV_FALSE) {
        /* Determine when next service should be. */
        nextservice = RvInt64Add(nextservice, tengine->period);

        /* servce the timer queue unless we're paused. */
        if(tengine->paused == RV_FALSE) {
            if(RvTimerQueueService(tengine->tqueue, 0, NULL, NULL, NULL))
                break; /* Just exit on error, the timer queue was probably stopped. */
        }

        /* Calculate delay need to reach next service time. */
        delay = RvInt64Sub(nextservice, RvTimestampGet(NULL));

        /* Do the delay (if <= 0 then we try to catch up by not sleeping). */
        if(RvInt64IsGreaterThan(delay, RvInt64Const(0,0,0)))
            RvThreadNanosleep(delay,NULL);
    }
}

/********************************************************************************************
 * RvTimerEngineSetOptions
 *
 * Sets the thread configuration for the thread of a timer engine. For more information
 * about these options, refer to the documentation for the Thread module.
 * INPUT   : tengine - Pointer to timer engine object to set thread configuration for.
 *           name - Pointer to string which constains the thread name.
 *           stackaddr - Address of memory to use for stack (NULL means to allocate it).
 *           stacksize - Size of the stack.
 *           priority - Priority to set thread to.
 *           attr - Pointer to OS speicific thread attributes to use (NULL = use defaults).
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : A copy of the name string passed in is made. The maximum size of the
 *           the string is RV_THREAD_MAX_NAMESIZE and names that are too long
 *           will be truncated.
 *           If stackaddr is set to NULL, the amount of stack space used for internal overhead
 *           will be allocated in addition to the requested stack size.
 *           If stackaddr is set to NULL and the stacksize is set to 0, the default
 *           stack settings will be used (see rvccoreconfig.h).
 *           The default values for the thread attributes are set in rvccoreconfig.h.
 *           Further information about these attributes can be found in that file and
 *           the rvthread.h file.
 *           Once the timer engine is started, these options can not be changed.
 */
RVCOREAPI RvStatus RVCALLCONV RvTimerEngineSetOptions(
    IN RvTimerEngine *tengine,
    IN RvChar *name,
    IN void *stackaddr,
    IN RvInt32 stacksize,
    IN RvInt32 priority,
    IN RvThreadAttr *attr)
{
    RvStatus result;
    RvInt32 ourstacksize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tengine == NULL)
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif

    result = RvThreadSetName(&tengine->thread, name);
    if(result != RV_OK)
        return result;
    ourstacksize = stacksize;
    if(stacksize > 0)
        ourstacksize += RV_TIMERENGINE_ADDITIONAL_STACK; /* Account for our overhead. */
    result = RvThreadSetStack(&tengine->thread, stackaddr, ourstacksize);
    if(result != RV_OK)
        return result;
    result = RvThreadSetPriority(&tengine->thread, priority);
    if(result != RV_OK)
        return result;
    result = RvThreadSetAttr(&tengine->thread, attr);
    if(result != RV_OK)
        return result;

    return RV_OK;
}

/********************************************************************************************
 * RvTimerEngineStart
 *
 * Starts a timer engine.
 * INPUT   : tengine - Pointer to timer engine object to be started.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : This function can only be called once for each timer engine.
 */
RVCOREAPI RvStatus RVCALLCONV RvTimerEngineStart(
    IN RvTimerEngine *tengine)
{
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tengine == NULL)
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif

    /* Create and start the thread. */
    result = RvThreadCreate(&tengine->thread);
    if(result != RV_OK) {
        RvThreadDestruct(&tengine->thread);
        return result;
    }
    result = RvThreadStart(&tengine->thread);
    if(result != RV_OK) {
        RvThreadDestruct(&tengine->thread);
        return result;
    }

    return RV_OK;
}

/********************************************************************************************
 * RvTimerEngineStop
 *
 * Stops a timer engine.
 * INPUT   : tengine - Pointer to timer engine object to be stopped.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : This function can only be called once for each timer engine.
 *           Callbacks currently in progress will be completed and the timer
 *           engine thread will exit, however, the RvTimerEngineStop call
 *           will not wait for this to happen.
 */
RVCOREAPI RvStatus RVCALLCONV RvTimerEngineStop(
    IN RvTimerEngine *tengine)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tengine == NULL)
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif

    tengine->stopped = RV_TRUE;
    return RV_OK;
}

/********************************************************************************************
 * RvTimerEnginePause
 *
 * Pauses a timer engine.
 * INPUT   : tengine - Pointer to timer engine object to be paused.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : Callbacks currently in progress will be completed
 */
RvStatus RvTimerEnginePause(
    IN RvTimerEngine *tengine)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tengine == NULL)
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif

    tengine->paused = RV_TRUE;
    return RV_OK;
}

/********************************************************************************************
 * RvTimerEngineResume
 *
 * Resumes a paused timer engine.
 * INPUT   : tengine - Pointer to timer engine object to be resumed.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 * NOTE    : When a timer engine resumes, timer events that were supposed to
 *           happen while the timer engine was paused will not be lost. They
 *           will be triggered and soon as the timer engine resumes.
 */
RvStatus RvTimerEngineResume(
    IN RvTimerEngine *tengine)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tengine == NULL)
        return RvTimerEngineErrorCode(RV_ERROR_NULLPTR);
#endif

    tengine->paused = RV_FALSE;
    return RV_OK;
}
#endif /* (RV_SOCKET_TYPE == RV_SOCKET_NUCLEUS) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI) */
