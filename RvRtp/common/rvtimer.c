/* rvtimer.c - timer functions */
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
#include "rvtimer.h"
#include "rvtimestamp.h"
#include "rvselect.h"
#include "rvmemory.h"
#include "rvccoreglobals.h"
#include "rv64ascii.h"
#include <string.h>

#define RV_TIMER_TRAVERSED 0x1234abcd

#define RvLogTimestamp(time) RvTimestampGetSecs(time), RvTimestampGetNsecs(time)

/* Sleep channel interface. Used for cancelling timer in WAIT_FOR_CB mode */

typedef struct RvTimerSleepChannel_s RvTimerSleepChannel;
typedef RvStatus (*RvTimerSleepCB)(RvTimerSleepChannel *channel, RvLogMgr *logMgr);
typedef RvStatus (*RvTimerWakeupCB)(RvTimerSleepChannel *channel, RvLogMgr *logMgr);

/* Virtual function table of sleep channel interface */
typedef struct RvTimerSleepChannelVT_s {
  RvTimerSleepCB sleep;
  RvTimerWakeupCB wakeup;
} RvTimerSleepChannelVT;

struct RvTimerSleepChannel_s {
  const RvTimerSleepChannelVT *vt;
  RvTimerSleepChannel *next;
};

typedef struct {
    RvTimerSleepChannel base;
    RvSemaphore         sem;
} RvTimerSleepChannelImpl;

/*static RvUint32 gsSleepChannelTLSIndex;*/

static RvStatus RvTimerSleepCBImpl(RvTimerSleepChannel *baseCh, RvLogMgr *logMgr) {
    RvTimerSleepChannelImpl *ch = (RvTimerSleepChannelImpl *)baseCh;
    return RvSemaphoreWait(&ch->sem, logMgr);
}

static RvStatus RvTimerWakeupCBImpl(RvTimerSleepChannel *baseCh, RvLogMgr *logMgr) {
    RvTimerSleepChannelImpl *ch = (RvTimerSleepChannelImpl *)baseCh;
    return RvSemaphorePost(&ch->sem, logMgr);
}

const RvTimerSleepChannelVT gsSleepChannelVT = {
    RvTimerSleepCBImpl,
    RvTimerWakeupCBImpl
};

/* typedef void (*RvThreadVarFunc)(RvThread *, void *, RvUint32 index); */

/* TLS destroy callback */
static void RvTimerDestructSleepChannelTLS(RvThread *th, void *data, RvUint32 idx) {
    RvTimerSleepChannelImpl *sleepChannel = (RvTimerSleepChannelImpl *)data;
    RvLogMgr *logMgr = RvThreadGetLogManager(th);

    RV_UNUSED_ARG(idx);
    /* No sleep channel was created for this thread */
    if(sleepChannel == 0) {
        return;
    }

    RvSemaphoreDestruct(&sleepChannel->sem, logMgr);
    RvMemoryFree((void *)sleepChannel, logMgr);
}

static 
RvStatus RvTimerGetSleepChannel(RvTimerSleepChannel **pch, RvLogMgr *logMgr) {
    RvTimerSleepChannelImpl *ch = 0;
    RvStatus s;
    RV_USE_CCORE_GLOBALS;

    s = RvThreadGetVar(gsSleepChannelTLSIndex, logMgr, (void **)&ch);
    if(s != RV_OK) {
        return s;
    }

    if(ch != 0) {
        *pch = (RvTimerSleepChannel *)ch;
        return RV_OK;
    }
    
    s = RvMemoryAlloc(0, sizeof(RvTimerSleepChannelImpl), logMgr, (void **)&ch);

    if(s != RV_OK) {
        return s;
    }

    s = RvSemaphoreConstruct(0, logMgr, &ch->sem);
    if(s != RV_OK) {
        RvMemoryFree(ch, logMgr);
        return s;
    }

    ch->base.next = 0;
    ch->base.vt   = &gsSleepChannelVT;

    RvThreadSetVar(gsSleepChannelTLSIndex, ch, logMgr);
    *pch = (RvTimerSleepChannel *)ch;
    return RV_OK;
}


/***********************************************************************
 * Internal timer structure containing information for each event.
 ***********************************************************************/
struct  RvTimerEvent_s {
    RvInt timertype;          /* Type of timer event (ONESHOT or PERIODIC). */
    RvTimerQueue *tqueue;     /* Timer queue that event was placed on. */
    RvInt state;              /* Current state. */
    RvUint id;                /* id number of event, used for sanity check. */
    RvInt64 triggerlength;    /* How long from starttime until timer should trigger. */
    RvInt64 triggertime;      /* Time at which event should be triggered. */
    RvBool canceled;          /* Set to RV_TRUE when canceled so PERIODIC timers won't repeat. */
#if 0
    RvSemaphore wait;         /* Used to block tasks waiting to cancel the event. */
    RvSize_t waitcount;       /* Number of tasks waiting to cancel the event. */
#endif

    RvTimerSleepChannel *sleepChannel;
    RvObjPoolElement element; /* Required for using object in pool. */
    RvSize_t index;           /* Index of event in Priority Queue. */
    void *userdata;           /* User data to be passed into event callback. */
    RvTimerFuncEx callback;     /* User callback to be called upon event. */
#if RV_TIMER_TEST_TIMERS
    RvInt traversed;
    RvInt64 starttime;        /* timestamp at start time, used for sanity check. */
#endif
}; /* typedef RvTimerEvent in header file */

/* Timer event states. */
#define RV_TIMER_EVENTSTATE_NOTINUSE RvIntConst(0)
#define RV_TIMER_EVENTSTATE_QUEUED RvIntConst(1)
#define RV_TIMER_EVENTSTATE_TRIGGERED RvIntConst(2)

/* Lets make error codes a little easier to type */
#define RvTimerErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CBASE, RV_CBASE_MODULE_TIMER, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvTimerLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->timerSource, p);}
#else
#define RvTimerLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvTimerLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->timerSource, p);}
#else
#define RvTimerLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvTimerLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->timerSource, p);}
#else
#define RvTimerLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
#define RvTimerLogWarning(p) {if (logMgr != NULL) RvLogWarning(&logMgr->timerSource, p);}
#else
#define RvTimerLogWarning(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define RvTimerLogDebug(p) {if (logMgr != NULL) RvLogDebug(&logMgr->timerSource, p);}
#else
#define RvTimerLogDebug(p) {RV_UNUSED_ARG(logMgr);}
#endif


static void *RvTimerEventConstruct(void *objptr, void *data);
static void RvTimerEventDestruct(void *objptr, void *data);
static void *RvTimerMemAlloc(RvSize_t size, void *data);
static void RvTimerMemFree(void *ptr, void *data);
static RvBool RvTimerPQueueItemCmp(void *ptr1, void *ptr2);
static void RvTimerPQueueNewIndex(void *item, RvSize_t index);

#if RV_TIMER_TEST_TIMERS
static RvInt64 startingTime;
#endif


/********************************************************************************************
 * RvTimerInit - Initializes the Timer module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimerInit(void)
{
    RvStatus s = RV_OK;
    RV_USE_CCORE_GLOBALS;

#if RV_TIMER_TEST_TIMERS
    startingTime = RvTimestampGet(0);
#endif

    /* Create TLS variable to hold sleep channel used to block this thread waiting for cancel timer */
    s = RvThreadCreateVar(RvTimerDestructSleepChannelTLS, "TimerSleepCannel", 0, &gsSleepChannelTLSIndex);
    return s;
}

#if RV_TIMER_TEST_TIMERS

static RvInt RvTimerRelativeSecs() {
    RvInt64 relTime = RvTimestampGet(0) - startingTime;
    RvTime t;

    RvTimeConstructFrom64(relTime, &t);
    return t.sec;
}

#endif


/********************************************************************************************
 * RvTimerEnd - Shuts down the Timer module.
 *
 * Must be called once (and only once) when no further calls to this module will be made.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimerEnd(void)
{
    RvStatus s;
    RV_USE_CCORE_GLOBALS;
    
    s = RvThreadDeleteVar(gsSleepChannelTLSIndex, 0);
    return s;
}


/********************************************************************************************
 * RvTimerSourceConstruct - Constructs timer module log source.
 *
 * Constructs log source to be used by common core when printing log from the
 * timer module. This function is applied per instance of log manager.
 *
 * INPUT   : logMgr - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RvStatus RvTimerSourceConstruct(
    IN RvLogMgr *logMgr)
{
    RvStatus result = RV_OK;

    result = RvLogSourceConstruct(logMgr, &logMgr->timerSource, "TIMER", "Timer functions");

    return result;
}



/********************************************************************************************
 * RvTimerQueueConstruct - Constructs a timer queue object based on the parameters passed in.
 *
 * There are two parts to each timer queue, the timer pool and the priority queue, both
 * of which require memory. There are three types of pools:
 *          FIXED:  This creates a fixed size timer queue (which contains
 *                  a fixed size timer pool and a fixed size priority queue) with
 *                  all memory pre-allocated based on the number of starttimers
 *                  requested. The number of blocks can be increased with the
 *                  RvTimerQueueSetSize and RvTimerQueueAddSize calls.}
 *          EXPANDING: This creates a timer queue pool which expands (by adding
 *                  pages and doubling the priority queue size) as needed. The additional
 *                  memory is not released until the timer queue is destructed.}
 *          DYNAMIC: This creates a timer queue which expands exactly like
 *                  and EXPANDING timer queue but also has the ability to remove
 *                  unused pages and reduce the size of the priority queue.
 *                  The freelevel value determines when a page should be released.
 *                  The priority queue is reduced by 50% when 25% or less of it is
 *                  in use.}
 *
 *
 * INPUT   : tqtype      - Type of timer queue: RV_TIMER_QTYPE_FIXED,
 *                  RV_TIMER_QTYPE_EXPANDING, or RV_TIMER_QTYPE_DYNAMIC.
 *           starttimers - Number of timers to start with.
 *           maxtimers   - Never exceed this number of timers.
 *           mintimers   - Never go below this number of timers.
 *           freelevel   - The minimum number of free timers per 100 to maintain
 *                  in the pool when shrinking a DYNAMIC timer pool (0 to 100).
 *                  A value of 0 always releases empty pages and a value of 100
 *                  never releases empty pages (which is the same as an EXPANDING
 *                  pool).
 *           pagetimers - Number of timers per memory allocation page in th pool.
 *           memregion  - Memory region to allocate memory from (NULL = default region).
 *           selEng     - select engine, used to set/cancel timer
 *           logMgr     - log manager
 * OUTPUT  : tqueue - Pointer to timer queue object to be constructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerQueueConstruct(
    IN  RvInt           tqtype,
    IN  RvSize_t        starttimers,
    IN  RvSize_t        maxtimers,
    IN  RvSize_t        mintimers,
    IN  RvSize_t        freelevel,
    IN  RvSize_t        pagetimers,
    IN  RvMemory        *memregion,
    IN  void            *selEng,
    IN  RvLogMgr        *logMgr,
    OUT RvTimerQueue    *tqueue)
{
    RvStatus result;
    RvTimerEvent timerevent;
    RvObjPoolFuncs poolcallbacks;
    RvPQueueFuncs pqueuecallbacks;
    RvSize_t startevents;
    RvInt32 pooltype;
    RvInt pqueuetype;
    RvBool salvage;
    RvSize_t numtimers;

    RvTimerLogEnter((&logMgr->timerSource, "RvTimerQueueConstruct(tqueue=%p,start=%d,max=%d,min=%d,page=%d)",
        tqueue, starttimers, maxtimers, mintimers, pagetimers));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL) {
        RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: NULL param(s)"));
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    tqueue->logMgr = logMgr;
    if (logMgr != NULL)
        tqueue->timerSource = &logMgr->timerSource;
    else
        tqueue->timerSource = NULL;

    result = RvLockConstruct(logMgr, &tqueue->lock);
    if(result != RV_OK) {
        RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: RvLockConstruct"));
        return result;
    }

    result = RvSemaphoreConstruct(0,logMgr, &tqueue->wait);
    if(result != RV_OK){
        RvLockDestruct(&tqueue->lock,logMgr);
        RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: RvSemaphoreConstruct"));
        return result;
    }

    /* Set up pool and pqueue types based on timer queue type */
    switch(tqtype) {
        case RV_TIMER_QTYPE_FIXED: pooltype = RV_OBJPOOL_TYPE_FIXED;
                                   pqueuetype = RV_PQUEUE_TYPE_FIXED;
                                   salvage = RV_OBJPOOL_SALVAGE_NEVER; /* save space */
                                   break;
        case RV_TIMER_QTYPE_EXPANDING: pooltype = RV_OBJPOOL_TYPE_EXPANDING;
                                       pqueuetype = RV_PQUEUE_TYPE_EXPANDING;
                                       salvage = RV_OBJPOOL_SALVAGE_NEVER; /* save space */
                                       break;
        case RV_TIMER_QTYPE_DYNAMIC: pooltype = RV_OBJPOOL_TYPE_DYNAMIC;
                                      pqueuetype = RV_PQUEUE_TYPE_DYNAMIC;
                                      salvage = RV_OBJPOOL_SALVAGE_ALLOWED; /* required */
                                      break;
        default: RvSemaphoreDestruct(&tqueue->wait,logMgr);
                 RvLockDestruct(&tqueue->lock,logMgr);
                 RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: Invalid type id"));
                 return RvTimerErrorCode(RV_ERROR_OUTOFRANGE);
    }

    /* Contruct pool of events */
    memset(&poolcallbacks, 0, sizeof(RvObjPoolFuncs));
    poolcallbacks.objconstruct = RvTimerEventConstruct;
    poolcallbacks.objdestruct = RvTimerEventDestruct;
    poolcallbacks.pagealloc = RvTimerMemAlloc;
    poolcallbacks.pagefree = RvTimerMemFree;
    poolcallbacks.objconstructdata = tqueue; /* tqueue Never changes so set it upon construction. */
    poolcallbacks.objdestructdata = NULL;
    poolcallbacks.pageallocdata = memregion;
    poolcallbacks.pagefreedata = memregion;
    if(RvObjPoolConstruct(&timerevent, &timerevent.element, &poolcallbacks, sizeof(RvTimerEvent), pagetimers, 0, pooltype, salvage, maxtimers, mintimers, freelevel, &tqueue->pool) == NULL) {
        RvSemaphoreDestruct(&tqueue->wait,logMgr);
        RvLockDestruct(&tqueue->lock,logMgr);
        RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: RvObjPoolConstruct"));
        return RvTimerErrorCode(RV_TIMER_ERROR_POOL);
    }

    /* Create starting number of events in pool */
    if(starttimers > RvObjPoolTotalItems(&tqueue->pool)) {
        numtimers = starttimers - RvObjPoolTotalItems(&tqueue->pool);
        if(RvObjPoolAddItems(&tqueue->pool, numtimers) < numtimers) {
            RvObjPoolDestruct(&tqueue->pool);
            RvSemaphoreDestruct(&tqueue->wait,logMgr);
            RvLockDestruct(&tqueue->lock,logMgr);
            RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: RvObjPoolAddItems"));
            return RvTimerErrorCode(RV_TIMER_ERROR_POOL);
        }
    }

    /* Construct Priority Queue for events (minimum size is 2). */
    startevents = RvObjPoolTotalItems(&tqueue->pool); /* start with pool & queue in sync */
    if(startevents < 2)
        startevents = 2;
    pqueuecallbacks.memalloc = RvTimerMemAlloc;
    pqueuecallbacks.memfree = RvTimerMemFree;
    pqueuecallbacks.itemcmp = RvTimerPQueueItemCmp;
    pqueuecallbacks.newindex = RvTimerPQueueNewIndex;
    pqueuecallbacks.memallocdata = memregion;
    pqueuecallbacks.memfreedata = memregion;
    if(RvPQueueConstruct(pqueuetype, startevents, &pqueuecallbacks, &tqueue->pqueue) == NULL) {
        RvObjPoolDestruct(&tqueue->pool);
        RvSemaphoreDestruct(&tqueue->wait,logMgr);
        RvLockDestruct(&tqueue->lock,logMgr);
        RvTimerLogError((&logMgr->timerSource, "RvTimerQueueConstruct: RvPQueueConstruct"));
        return RvTimerErrorCode(RV_TIMER_ERROR_PQUEUE);
    }

    /* clear id number counter */
    tqueue->callcount = 0;
    tqueue->qState = RV_TIMERQUEUE_ENABLED;
    tqueue->selEng = selEng;
    tqueue->prevCurtime = RvTimestampGet(logMgr);

#if RV_TIMER_TEST_TIMERS
    tqueue->lastServiceTime = RvTimerRelativeSecs();
    tqueue->lastDisabled = 0;
    tqueue->lastEnabled = 0;
    tqueue->lastSId = 0;
    tqueue->lastDId = 0;
    tqueue->lastEId = 0;
#endif

    RvTimerLogLeave((&logMgr->timerSource, "RvTimerQueueConstruct(tqueue=%p)", tqueue));

    return RV_OK;
}


/********************************************************************************************
 * RvTimerQueueControl - Stops/Resumes a timer queue.
 *
 * Stops or resumes processing of the timer queue.
 *
 * INPUT   : tqueue - Pointer to timer queue object to be stopped.
 *           qState - Indicates whether this is a permanent or temporary stop.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerQueueControl(
    IN RvTimerQueue         *tqueue,
    IN RvTimerQueueState    qState)
{
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    result = RvLockGet(&tqueue->lock,tqueue->logMgr);
    if(result != RV_OK)
        return result;

    tqueue->qState = qState;

    /* First see if we have to wait for callbacks to complete. */
    if(qState == RV_TIMERQUEUE_DELETED && tqueue->callcount > 0) {
        /* Wait until callbacks complete. */
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        result = RvSemaphoreWait(&tqueue->wait,tqueue->logMgr);
        return RV_OK; /* We don't need to do anything else. */
    }

#if RV_TIMER_TEST_TIMERS
    if(qState == RV_TIMERQUEUE_ENABLED) {
        tqueue->lastEnabled = RvTimerRelativeSecs();
        tqueue->lastEId = (void *)RvThreadCurrentId();
    } else if(qState == RV_TIMERQUEUE_DISABLED) {
        tqueue->lastDisabled = RvTimerRelativeSecs();
        tqueue->lastDId = (void *)RvThreadCurrentId();
    }

#endif

    RvLockRelease(&tqueue->lock,tqueue->logMgr);

#if (RV_NET_TYPE != RV_NET_NONE)
    if(qState == RV_TIMERQUEUE_ENABLED && tqueue->selEng != NULL) {
        /* We should set select to trigger when timeout will be expired */
        RvInt64 nextevent = RV_INT64_MAX;

        RvTimerQueueNextEvent(tqueue,&nextevent);

		if (RvInt64IsLessThanOrEqual(nextevent, RvInt64ShortConst(0)))
            nextevent = RvInt64ShortConst(1); /* Timeout must be higher than 0 to work at all */
        RvSelectSetTimeOut((RvSelectEngine *)tqueue->selEng, RvTimestampGet(tqueue->logMgr), nextevent, tqueue->logMgr);
    }
#endif

    return RV_OK;
}


/********************************************************************************************
 * RvTimerQueueSetLogMgr - Update the log manager of a timer queue.
 *
 * INPUT   : tqueue - Pointer to the timer queue object.
 *           logMgr - log manager
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerQueueSetLogMgr(
    IN RvTimerQueue     *tqueue,
    IN  RvLogMgr        *logMgr)
{
    RvStatus result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    result = RvLockGet(&tqueue->lock,logMgr);
    if(result != RV_OK)
        return result;

    tqueue->logMgr = logMgr;

    if (logMgr != NULL)
        tqueue->timerSource = &logMgr->timerSource;
    else
        tqueue->timerSource = NULL;

    RvLockRelease(&tqueue->lock,logMgr);

    return RV_OK;
}


/********************************************************************************************
 * RvTimerQueueNumEvents - Find out how many events are in the timer queue.
 *
 *
 * INPUT   : tqueue - Pointer to timer queue object to be checked.
 * OUTPUT  : none
 * RETURN  : Number of events currently in the timer queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueNumEvents(
    IN RvTimerQueue *tqueue)
{
    RvSize_t qsize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;
    qsize = RvPQueueNumItems(&tqueue->pqueue);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return qsize;
}


/********************************************************************************************
 * RvTimerQueueNumEvents - Find out the maximum concurrent timers in the timer queue.
 *
 *
 * INPUT   : tqueue - Pointer to timer queue object to be checked.
 * OUTPUT  : none
 * RETURN  : Number of maximum concurrent events in the timer queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueMaxConcurrentEvents(
    IN RvTimerQueue *tqueue)
{
    RvSize_t qsize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;
    qsize = RvPQueueMaxConcurrentItems(&tqueue->pqueue);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return qsize;
}


/********************************************************************************************
 * RvTimerQueueTimersCreated - Find out how many timers were 
 *                             created in the timer queue.
 *
 * INPUT   : tqueue - Pointer to timer queue object to be checked.
 * OUTPUT  : none
 * RETURN  : Number of timer created in the timer queue.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueTimersCreated(
    IN RvTimerQueue *tqueue)
{
    RvSize_t qsize;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;
    qsize = RvPQueueAllocatedItems(&tqueue->pqueue);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return qsize;
}



/********************************************************************************************
 * RvTimerQueueGetSize - Find the current total size of the timer queue.
 *
 *
 * INPUT   : tqueue - Pointer to timer queue object to be checked.
 * OUTPUT  : none
 * RETURN  : Number of timers in the timer queue pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueGetSize(
    IN RvTimerQueue *tqueue)
{
    RvSize_t poolsize;
    RvLogMgr *logMgr = NULL;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerQueueGetSize(tqueue=%p)", tqueue));

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueGetSize(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return 0;
    }
    poolsize = RvObjPoolTotalItems(&tqueue->pool);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    RvTimerLogLeave((tqueue->timerSource, "RvTimerQueueGetSize(tqueue=%p)", tqueue));

    return poolsize;
}


/********************************************************************************************
 * RvTimerQueueSetSize - Set the total size of the timer queue.
 *
 * The size may only be increased over its current value.
 * The actual number of timers may be larger than that requested since
 * the amount added will be a multiple of the number of timers per
 * page that was set when the timer queue was constructed. The value
 * returned will be the actual new number of timers available.
 * Changes are subject to the limits of the maxtimers and mintimers settings.
 *
 * INPUT   : tqueue  - Pointer to timer queue object to be set.
 *           newsize - New size that the timer queue should be set to.
 * OUTPUT  : none
 * RETURN  : Number of timers in the timer queue pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueSetSize(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     newsize)
{
    RvSize_t poolsize, newqsize, newitems, result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;

    poolsize = RvObjPoolTotalItems(&tqueue->pool);
    if(newsize <= poolsize) {
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        return poolsize;
    }
    newitems = newsize - poolsize;

    newqsize = newsize;
    if(newqsize < 2)
        newqsize = 2; /* minimum size of priority queue. */

    /* Ajdust the size of the priority queue. */
    if(RvPQueueChangeSize(&tqueue->pqueue, newqsize) != newqsize) {
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        return poolsize;
    }

    /* Add the items to the timer pool. */
    RvObjPoolAddItems(&tqueue->pool, newitems);
    result = RvObjPoolTotalItems(&tqueue->pool);

    RvLockRelease(&tqueue->lock,tqueue->logMgr);
    return result;
}


/********************************************************************************************
 * RvTimerQueueAddSize - Adds to the total size of the timer queue.
 *
 * The actual number of timers may be larger than that requested since
 * the amount added will be a multiple of the number of timers per
 * page that was set when the timer queue was constructed. The value
 * returned will be the actual new number of timers that was added.
 * Changes are subject to the limits of the maxtimers setting.
 *
 * INPUT   : tqueue  - Pointer to timer queue object to be set.
 *           newsize - New size that the timer queue should be set to.
 * OUTPUT  : none
 * RETURN  : Actual number of timers added to the timer queue pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueAddSize(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     addsize)
{
    RvSize_t poolsize, newqsize, result;
    RvLogMgr *logMgr = NULL;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerQueueAddSize(tqueue=%p,add=%d)", tqueue, addsize));

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueAddSize(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return 0;
    }

    poolsize = RvObjPoolTotalItems(&tqueue->pool);
    newqsize = poolsize + addsize;
    if(newqsize < 2)
        newqsize = 2; /* minimum size of priority queue. */

    /* Ajdust the size of the priority queue. */
    if(RvPQueueChangeSize(&tqueue->pqueue, newqsize) != newqsize) {
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        return 0;
    }

    /* Add the items to the timer pool. */
    RvObjPoolAddItems(&tqueue->pool, addsize);
    result = RvObjPoolTotalItems(&tqueue->pool) - poolsize;

    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    RvTimerLogLeave((tqueue->timerSource, "RvTimerQueueAddSize(tqueue=%p)", tqueue));

    return result;
}


/********************************************************************************************
 * RvTimerQueueGetMaxtimers - Returns current value for maxtimers (not used by FIXED queues)
 *
 * INPUT   : tqueue  - Pointer to timer queue object.
 * OUTPUT  : none
 * RETURN  : current value for maxtimers.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueGetMaxtimers(
    IN RvTimerQueue *tqueue)
{
    RvSize_t maxtimers;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;
    maxtimers = RvObjPoolGetMaxitems(&tqueue->pool);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return maxtimers;
}


/********************************************************************************************
 * RvTimerQueueSetMaxtimers - Sets the value for maxtimers (not used by FIXED queues).
 *
 * INPUT   : tqueue    - Pointer to timer queue object.
 *           maxtimers - new value for maxtimers
 * OUTPUT  : none
 * RETURN  : Returns RV_TRUE upon success (otherwise RV_FALSE).
 */
RVCOREAPI
RvBool RVCALLCONV RvTimerQueueSetMaxtimers(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     maxtimers)
{
    RvBool result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RV_FALSE;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return RV_FALSE;
    result = RvObjPoolSetMaxitems(&tqueue->pool, maxtimers);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return result;
}


/********************************************************************************************
 * RvTimerQueueChangeMaxtimers - Changes the value for maxtimers (not used by FIXED queues)
 *
 * INPUT   : tqueue    - Pointer to timer queue object.
 *           delta     - change value
 *           direction - RV_TIMER_VALUE_DECREASE or RV_TIMER_VALUE_INCREASE
 * OUTPUT  : none
 * RETURN  : Returns RV_TRUE upon success (otherwise RV_FALSE).
 */
RVCOREAPI
RvBool RVCALLCONV RvTimerQueueChangeMaxtimers(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     delta,
    IN RvBool       direction)
{
    RvBool result;
    RvSize_t maxtimers;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((direction != RV_TIMER_VALUE_INCREASE) && (direction != RV_TIMER_VALUE_DECREASE))
        return RV_FALSE;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return RV_FALSE;
    maxtimers = RvObjPoolGetMaxitems(&tqueue->pool);
    if(direction == RV_TIMER_VALUE_DECREASE) {
        if(delta > maxtimers) {
            /* Can't go negative. */
            RvLockRelease(&tqueue->lock,tqueue->logMgr);
            return RV_FALSE;
        }
        maxtimers -= delta;
    } else maxtimers += delta;
    result = RvObjPoolSetMaxitems(&tqueue->pool, maxtimers);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return result;
}


/********************************************************************************************
 * RvTimerQueueGetMintimers - Returns current value for mintimers (not used by FIXED).
 *
 * INPUT   : tqueue    - Pointer to timer queue object.
 * OUTPUT  : none
 * RETURN  : current value for mintimers
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueGetMintimers(
    IN RvTimerQueue *tqueue)
{
    RvSize_t mintimers;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;
    mintimers = RvObjPoolGetMinitems(&tqueue->pool);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return mintimers;
}



/********************************************************************************************
 * RvTimerQueueSetMinTimers - Sets the minimum size of the timer queue.
 *
 * This setting is not ususally used by FIXED pools since they are limited
 * by their actual size. Increasing a fixed pool minimum beyond its current
 * size will use more memory but not add any timers.
 *
 * INPUT   : tqueue    - Pointer to timer queue object to be set.
 *           mintimers - New setting for minimum number of timers.
 * OUTPUT  : none
 * RETURN  : Returns RV_TRUE upon success (otherwise RV_FALSE).
 */
RVCOREAPI
RvBool RVCALLCONV RvTimerQueueSetMintimers(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     mintimers)
{
    RvBool result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RV_FALSE;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return RV_FALSE;
    result = RvObjPoolSetMinitems(&tqueue->pool, mintimers);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return result;
}



/********************************************************************************************
 * RvTimerQueueChangeMinTimers - Changes the minimum size of the timer queue.
 *
 * This setting is not ususally used by FIXED pools since they are limited
 * by their actual size. Increasing a fixed pool minimum beyond its current
 * size will use more memory but not add any timers.
 *
 * INPUT   : tqueue    - Pointer to timer queue object to be changed.
 *           delta     - The amount to change the mintimer value by.
 *           direction - irection to change value, either RV_TIMER_VALUE_INCREASE or
 *                  RV_TIMER_VALUE_DECREASE
 * OUTPUT  : none
 * RETURN  : Returns RV_TRUE upon success (otherwise RV_FALSE).
 */
RVCOREAPI
RvBool RVCALLCONV RvTimerQueueChangeMintimers(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     delta,
    IN RvBool       direction)
{
    RvBool result;
    RvSize_t mintimers;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RV_FALSE;
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((direction != RV_TIMER_VALUE_INCREASE) && (direction != RV_TIMER_VALUE_DECREASE))
        return RV_FALSE;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return RV_FALSE;
    mintimers = RvObjPoolGetMinitems(&tqueue->pool);
    if(direction == RV_TIMER_VALUE_DECREASE) {
        if(delta > mintimers) {
            /* Can't go negative. */
            RvLockRelease(&tqueue->lock,tqueue->logMgr);
            return RV_FALSE;
        }
        mintimers -= delta;
    } else mintimers += delta;
    result = RvObjPoolSetMinitems(&tqueue->pool, mintimers);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return result;
}


/********************************************************************************************
 * RvTimerQueueGetFreelevel - Find the current freelevel of the timer queue.
 *
 * This setting is only used by DYNAMIC timer queues.
 *
 * INPUT   : tqueue    - Pointer to timer queue object to be checked.
 * OUTPUT  : none
 * RETURN  : Value of the freevalue setting for timer queue pool.
 */
RVCOREAPI
RvSize_t RVCALLCONV RvTimerQueueGetFreelevel(
    IN RvTimerQueue *tqueue)
{
    RvSize_t freelevel;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return 0;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return 0;
    freelevel = RvObjPoolGetFreelevel(&tqueue->pool);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return freelevel;
}

/********************************************************************************************
 * RvTimerQueueSetFreelevel - Sets the current freelevel of the timer queue.
 *
 * This is the minimum number
 * of free timers per 100 to maintain in the pool when shrinking a DYNAMIC
 * timer pool (0 to 100). A value of 0 always releases empty pages and a
 * value of 100 never releases empty pages (which is the same as an EXPANDING
 * pool).
 * This setting is only used by DYNAMIC timer queues.
 *
 * INPUT   : tqueue    - Pointer to timer queue object to be set.
 *           freelevel - New freelevel value to set (0 to 100).
 * OUTPUT  : none
 * RETURN  : Returns RV_TRUE if successful, otherwise RV_FALSE.
 */
RVCOREAPI
RvBool RVCALLCONV RvTimerQueueSetFreelevel(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     freelevel)
{
    RvBool result;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RV_FALSE;
#endif

    if(RvLockGet(&tqueue->lock,tqueue->logMgr) != RV_OK)
        return RV_FALSE;
    result = RvObjPoolSetFreelevel(&tqueue->pool, freelevel);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    return result;
}


/********************************************************************************************
 * RvTimerQueueNextEvent - Returns the number of nanoseconds until the next timer is supposed
 * to be triggered.
 *
 * This is the minimum number
 * of free timers per 100 to maintain in the pool when shrinking a DYNAMIC
 * timer pool (0 to 100). A value of 0 always releases empty pages and a
 * value of 100 never releases empty pages (which is the same as an EXPANDING
 * pool).
 * This setting is only used by DYNAMIC timer queues.
 * The value of nextevent may be negative if the next event is overdue.
 *
 * INPUT   : tqueue    - Pointer to timer queue object to be checked.
 * OUTPUT  : nextevent - Pointer to location to store the nanoseconds until the next event.
 * RETURN  : RV_OK if successful otherwise an error code. A warning of RV_TIMER_WARNING_QUEUEEMPTY
 *          will be returned if there are no events in the timer queue.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerQueueNextEvent(
    IN  RvTimerQueue    *tqueue,
    OUT RvInt64         *nextevent)
{
    RvStatus result;
    RvInt64 curtime;
    RvTimerEvent *event;
    RvLogMgr     *logMgr = NULL;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerQueueNextEvent(tqueue=%p)", tqueue));

    curtime = RvTimestampGet(tqueue->logMgr);

    result = RvLockGet(&tqueue->lock,tqueue->logMgr);
    if(result != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueNextEvent(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return result;
    }

    if (tqueue->qState != RV_TIMERQUEUE_ENABLED)
        *nextevent = RV_INT64_MAX;
    else
    {
        event = (RvTimerEvent *)RvPQueuePeek(&tqueue->pqueue);
        if(event != NULL) {
            *nextevent = RvInt64Sub(event->triggertime, curtime);
        } else {
            *nextevent = RV_INT64_MAX;
            result = RvTimerErrorCode(RV_TIMER_WARNING_QUEUEEMPTY);
            RvLogInfo(tqueue->timerSource,
                (tqueue->timerSource, "RvTimerQueueNextEvent(tqueue=%p): PQueue is empty", tqueue));
        }
    }

    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    RvTimerLogLeave((tqueue->timerSource, "RvTimerQueueNextEvent(tqueue=%p)", tqueue));

    return result;
}


/********************************************************************************************
 * RvTimerQueueDestruct - Destructs a timer queue.
 *
 * All timers in the queue are lost when the timer queue is destructed.
 * Insure that no threads are adding timers or servicing timers on the
 * timer queue when it is destructing since not all operating systems
 * handle the situation gracefully.
 * This function may only be called once on each timer queue. Thus
 * it may not be called simultaneously from multiple threads (with the
 * same timer queue to destruct).
 *
 * INPUT   : tqueue    - Pointer to timer queue object to be Destructed.
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerQueueDestruct(
    IN RvTimerQueue *tqueue)
{
    RvStatus result;
    RvTimerEvent *event;
    RvLogMgr *logMgr = NULL;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    logMgr = tqueue->logMgr;
    RvTimerLogEnter((tqueue->timerSource, "RvTimerQueueDestruct(tqueue=%p)", tqueue));

    RvTimerQueueControl(tqueue, RV_TIMERQUEUE_DELETED); /* Doesn't hurt to make sure. */

    result = RvLockGet(&tqueue->lock,tqueue->logMgr);
    if(result != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueDestruct(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return result;
    }

    /* Return all events back to pool. */
    for(;;) {
        event = (RvTimerEvent *)RvPQueueGet(&tqueue->pqueue);
        if(event == NULL)
            break;
        RvObjPoolReleaseItem(&tqueue->pool, event);
    }

    RvPQueueDestruct(&tqueue->pqueue);
    if(RvObjPoolDestruct(&tqueue->pool) != RV_TRUE) {
        result = RvTimerErrorCode(RV_TIMER_ERROR_POOL);
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueDestruct(tqueue=%p): RvObjPoolDestruct", tqueue));
    }
    RvSemaphoreDestruct(&tqueue->wait,tqueue->logMgr);
    RvLockRelease(&tqueue->lock,tqueue->logMgr);
    RvLockDestruct(&tqueue->lock,tqueue->logMgr);

    RvTimerLogLeave((tqueue->timerSource, "RvTimerQueueDestruct(tqueue=%p)", tqueue));

    return result;
}


/********************************************************************************************
 * RvTimerEventConstruct - constructs timer event.
 *
 * INPUT   : objptr    - memory where event should be constructed
 *           data      - timer queue
 * OUTPUT  : none
 * RETURN  : none
 */
static void *RvTimerEventConstruct(
    IN void *objptr,
    IN void *data)
{
    RvTimerEvent *timerevent;
    RvTimerQueue *tqueue;

    timerevent = (RvTimerEvent *)objptr;
    tqueue = (RvTimerQueue *)data;
    timerevent->sleepChannel = 0;
    timerevent->tqueue = tqueue; /* Never changes. */
    timerevent->id = 0;          /* Timer generation number. It's incremented every time this timer is reused */
    return objptr;
}


/********************************************************************************************
 * RvTimerEventDestruct - Destroy individual event for pool.
 *
 * INPUT   : objptr    - event to destruct
 *           data      - timer queue
 * OUTPUT  : none
 * RETURN  : none
 */
static void RvTimerEventDestruct(
    IN void *objptr,
    IN void *data)
{
    RV_UNUSED_ARG(data);
    RV_UNUSED_ARG(objptr);
}


/********************************************************************************************
 * RvTimerMemAlloc - Allocate memory page for event pool and Priority Queue.
 *
 * INPUT   : size      - size of memory to be allocated
 *           data      - RvMemory object
 * OUTPUT  : none
 * RETURN  : pointer to alocated memory
 */
static void *RvTimerMemAlloc(
    IN RvSize_t size,
    IN void     *data)
{
    void *result;
    RvStatus status;

    status = RvMemoryAlloc((RvMemory *)data, size, NULL, &result);
    if(status != RV_OK)
        return NULL;
    return result;
}



/********************************************************************************************
 * RvTimerMemFree - Free memory page for event pool and Priority Queue.
 *
 * INPUT   : ptr       - pointer to allocated memory
 *           data      - RvMemory object
 * OUTPUT  : none
 * RETURN  : none
 */
static void RvTimerMemFree(
    IN void *ptr,
    IN void *data)
{
    RV_UNUSED_ARG(data);
    RvMemoryFree(ptr, NULL);
}


/********************************************************************************************
 * RvTimerPQueueItemCmp - For Priority queue, return RV_TRUE if ptr1 higher priority than ptr2.
 *
 * INPUT   : ptr1      - pointer to first element
 *           ptr2      - pointer to second element
 * OUTPUT  : none
 * RETURN  : RV_TRUE if ptr1 higher priority than ptr2 or RV_FALSE otherways
 */
static RvBool RvTimerPQueueItemCmp(
    IN void *ptr1,
    IN void *ptr2)
{
    if(RvInt64IsLessThan(((RvTimerEvent *)ptr1)->triggertime, ((RvTimerEvent *)ptr2)->triggertime))
        return RV_TRUE;
    return RV_FALSE;
}


/********************************************************************************************
 * RvTimerPQueueNewIndex - For Priority Queue, save index of specified item (used for deletion).
 *
 * INPUT   : item      - queue item
 *           index     - item index in the queue
 * OUTPUT  : none
 * RETURN  : none
 */
static void RvTimerPQueueNewIndex(
    IN void     *item,
    IN RvSize_t index)
{
    ((RvTimerEvent *)item)->index = index;
}


/********************************************************************************************
 * RvTimerStartEx - Creates and schedules a timer event.
 *
 * There are two type of timers, ONESHOT and PERIDOC.
 * A ONESHOT timer will trigger only once, at the requested time. A PERIODIC timer will
 * triggered repeatedly until it is canceled with a call to RvTimerCancel or by the
 * callback returning a value of RV_FALSE.
 * note:  There is no construct or destruct calls for the RvTimer type. It is simply used as
 *      an identifier for the timer event so that it may be canceled. The RvTimer structure
 *      is not required in order for the timer event to trigger and not keeping that
 *      structure around in no way effects the operation of the timer other then leaving
 *      no way to cancel the timer event. Also, the timer parameter may be NULL, which
 *      simply starts a timer without returning the information needed to cancel it.
 * note:  The actual accuracy of the timer is dependent upon a number of things including
 *      the resolution of the timestamp clock, the rate and priority of the thread
 *      servicing the timer queue, and the cpu load of the system.
 *
 * INPUT  : timer       - Pointer to timer object which will be filled in with information
 *                        needed to identify the event.
 *          tqueue      - Pointer to timer queue object where timer should be scheduled.
 *          timertype   - Type of timer: RV_TIMER_TYPE_ONESHOT or RV_TIMER_TYPE_PERIODIC.
 *          delay       - Time until timer should be triggered, in nanoseconds.
 *          callback    - Function to be called when the timer is triggered.
 *          userdata    - Pointer that will be passed to the callback function when triggered.
 * OUTPUT  : none
 * RETURN  : returns: RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerStartEx(
    IN RvTimer      *timer,
    IN RvTimerQueue *tqueue,
    IN RvInt        timertype,
    IN RvInt64      delay,
    IN RvTimerFuncEx  callback,
    IN void         *userdata)
{
    RvStatus result;
    RvInt64 starttime;
    RvTimerEvent *event;
    RvLogMgr     *logMgr = NULL;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if((tqueue == NULL) || (callback == NULL))
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerStart(tqueue=%p; timer=%p)", tqueue, timer));

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if((timertype != RV_TIMER_TYPE_ONESHOT) && (timertype != RV_TIMER_TYPE_PERIODIC)) {
        RvTimerLogError((tqueue->timerSource, "RvTimerStart(tqueue=%p): Range", tqueue));
        return RvTimerErrorCode(RV_ERROR_OUTOFRANGE);
    }
#endif

    /* Get start time as soon as possible for best accuracy. */
    starttime = RvTimestampGet(tqueue->logMgr);

    result = RvLockGet(&tqueue->lock,tqueue->logMgr);
    if(result != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerStart(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return result;
    }

    /* Get a timer event from the pool. */
    event = (RvTimerEvent *)RvObjPoolGetItem(&tqueue->pool);
    if(event == NULL) {
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        RvTimerLogError((tqueue->timerSource, "RvTimerStart(tqueue=%p): RvObjPoolGetItem", tqueue));
        if(timer != 0) {
            timer->event = 0;
            timer->id = 0;
        }
        return RvTimerErrorCode(RV_TIMER_ERROR_QUEUEFULL);
    }

    /* Initialize event; tqueue, the wait semaphore, and waitcount are already set. */
    event->timertype = timertype;
    event->state = RV_TIMER_EVENTSTATE_QUEUED;
    event->id++;  /* Increment generation number */
    event->triggerlength = delay;
    event->triggertime = RvInt64Add(starttime, delay);
    event->canceled = RV_FALSE;
    event->userdata = userdata;
    event->callback = callback;

#if RV_TIMER_TEST_TIMERS
    event->starttime = starttime;
#endif


    /* Add the new event to the Priority Queue. */
    if(RvPQueuePut(&tqueue->pqueue, event) == NULL) {
        RvObjPoolReleaseItem(&tqueue->pool, event);
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        RvTimerLogError((tqueue->timerSource, "RvTimerStart(tqueue=%p): RvPQueuePut", tqueue));
        return RvTimerErrorCode(RV_TIMER_ERROR_QUEUEFULL);
    }

    /* Fill in the timer identification information. */
    if(timer != NULL) {
        timer->event = event;
        timer->id = event->id;
    }

#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
    if (tqueue->timerSource != NULL &&
        RvLogIsSelected(tqueue->timerSource, RV_LOGLEVEL_DEBUG))
    {
        RvChar strStart[RV_64TOASCII_BUFSIZE];
        RvChar strDelay[RV_64TOASCII_BUFSIZE];
        RvChar strFinish[RV_64TOASCII_BUFSIZE];
        Rv64toA(starttime, strStart);
        Rv64toA(delay, strDelay);
        Rv64toA(event->triggertime, strFinish);

        RvTimerLogDebug((tqueue->timerSource, "RvTimerStart(tqueue=%p,timer=%p):event=%p,delay=%s,start=%s,end=%s",
            tqueue, timer, event, strDelay, strStart, strFinish));
    }
#endif /*#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)*/

    RvLockRelease(&tqueue->lock,tqueue->logMgr);

#if (RV_NET_TYPE != RV_NET_NONE)
    if (tqueue->selEng != NULL) {
        /* We should set select to trigger when timeout will be expired */
        RvInt64 nextevent = RV_INT64_MAX;

        RvTimerQueueNextEvent(tqueue,&nextevent);

        if (RvInt64IsLessThanOrEqual(nextevent, RvInt64Const(0,0,0)))
            nextevent = RvInt64Const(1,0,1); /* Timeout must be higher than 0 to work at all */
        RvSelectSetTimeOut((RvSelectEngine *)tqueue->selEng, starttime, nextevent, tqueue->logMgr);
    }
#endif

    RvTimerLogLeave((tqueue->timerSource, "RvTimerStart(tqueue=%p; timer=%p)", tqueue, timer));

    return(RV_OK);
}

RvStatus RvTimerCancelEx(RvTimer *timer, RvTimerSleepChannel *ch) {
    RvStatus result;
    RvTimerEvent *event;
    RvTimerQueue *tqueue;
    RvLogMgr     *logMgr = NULL;
    RvTimerSleepChannel *cur, *prev;

    event = timer->event;
    /* Timer was never started */
    if(event == 0) {
        return RV_OK;
    }

    tqueue = event->tqueue;
    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerCancel(tqueue=%p)", tqueue));

    result = RvLockGet(&tqueue->lock, logMgr);
    if(result != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerCancel(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return result;
    }

    /* Check to insure timer event is still valid. */
    if((event->id != timer->id) || (event->state == RV_TIMER_EVENTSTATE_NOTINUSE)) {
        /* Timer isn't valid so just say that cancel worked. */
        RvTimerLogWarning((tqueue->timerSource,
            "RvTimerCancel(tqueue=%p): Timer %p is not valid (event->id=%u, timer->id=%u, event->state=%d)",
            tqueue, timer, event->id, timer->id, event->state));
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        return RV_OK;
    }

    if(event->state == RV_TIMER_EVENTSTATE_QUEUED) {
        /* Event is in the Priority Queue, just remove it and return it to the pool. */
        RvPQueueRemove(&tqueue->pqueue, event->index);
        event->state = RV_TIMER_EVENTSTATE_NOTINUSE;
        RvObjPoolReleaseItem(&tqueue->pool, event);
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        timer->id = 0;
        RvTimerLogDebug((tqueue->timerSource, "RvTimerCancelEx: event was removed(tqueue=%p; event=%p)", tqueue, event));
        return RV_OK;
    }

    /* The event is in the middle of its callback (TRIGGERED state). */

    event->canceled = RV_TRUE; /* Tell periodic timers not to repeat. */

    /* No sleep channel was specified */
    if(ch == 0) {
        /* Caller does not want to wait for callback completion. */
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        /* RvTimerLogWarning((tqueue->timerSource, "RvTimerCancel(tqueue=%p): Event is handled by CB", tqueue)); */
        return RvTimerErrorCode(RV_TIMER_WARNING_BUSY);
    }

    /* Caller wants to wait for timer callback to complete. */

    /* Push new sleep channel at the head of sleep channels list */
    ch->next = event->sleepChannel;
    event->sleepChannel = ch;

    RvLockRelease(&tqueue->lock,tqueue->logMgr);
    result = ch->vt->sleep(ch, tqueue->logMgr);
    RvLockGet(&tqueue->lock,tqueue->logMgr);

    /* Pop one of the sleep channels */
    for(cur = event->sleepChannel, prev = 0; cur && cur != ch; prev = cur, cur = cur->next)
    {}
    
    if(cur == ch && cur != 0) {
        if(prev == 0) {
            event->sleepChannel = cur->next;
        } else {
            prev->next = cur->next;
        }
        cur->next = 0;
    }
    
    if(event->sleepChannel == 0) {
        /* We're the last one that was waiting so return the event to the pool. */
        event->state = RV_TIMER_EVENTSTATE_NOTINUSE;
        RvObjPoolReleaseItem(&tqueue->pool, event);

        RvTimerLogDebug((tqueue->timerSource, "RvTimerCancelEx: event was removed(tqueue=%p; event=%p)", tqueue, event));

        /* If the timer queue is stopped and we were waiting for the */
        /* last callback, wake up the task that has been waiting for us. */
        if((tqueue->qState == RV_TIMERQUEUE_DELETED) && (tqueue->callcount == 0)) {
            RvSemaphorePost(&tqueue->wait,tqueue->logMgr);
        }
    }

    RvLockRelease(&tqueue->lock,tqueue->logMgr);

    timer->id = 0;
    RvTimerLogLeave((tqueue->timerSource, "RvTimerCancel(tqueue=%p)", tqueue));

    return result;
}

/********************************************************************************************
 * RvTimerCancel - Cancels a timer event.
 *
 * This function has two slightly different behaviours
 * depending on how the blocking parameter is set:
 *          RV_TIMER_CANCEL_WAIT_FOR_CB: The timer event is garanteed to be canceled
 *                  when the function returns. If the timer event was is the middle of
 *                  being triggered, this function will suspend the calling thread until
 *                  the callback is completed.
 *          RV_TIMER_CANCEL_DONT_WAIT_FOR_CB: The timer event is canceled if it is
 *                  not currently in the middle of being triggered. If the timer event
 *                  was is the middle of being triggered a return warning value of
 *                  RV_TIMER_WARNING_BUSY is returned. PERIODIC timers will have future
 *                  triggers canceled so that the trigger currently in progress will be
 *                  the last one.
 *  note:   Do not cancel a timer that was scheduled on a timer queue that has been destructed.
 *          Destructing the timer queue automatically cancels and destroys all timers.
 *  note:   Never call RvTimerCancel with behaviour set to RV_TIMER_CANCEL_WAIT_FOR_CB from
 *          inside of that timers callback. It really should not be necessary to call it
 *          at all from within its callback.
 *
 * INPUT   : timer      - Pointer to timer object which identifies the event to cancel.
 *           behaviour  - Behaviour of cancel: RV_TIMER_CANCEL_WAIT_FOR_CB, RV_TIMER_CANCEL_DONT_WAIT_FOR_CB.
 *                        or RV_TIMER_CANCEL_IGNORE_CB
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code. May also return RV_TIMER_WARNING_BUSY
 *          if blocking is set to RV_TIMER_CANCEL_DONT_WAIT_FOR_CB.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerCancel(
    IN RvTimer                  *timer,
    IN RvTimerCancelBehavior    behaviour)
{
    RvStatus s;
    RvTimerSleepChannel *sleepChannel;
    RvTimerEvent *event;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(timer == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if( (behaviour != RV_TIMER_CANCEL_WAIT_FOR_CB) &&
        (behaviour != RV_TIMER_CANCEL_DONT_WAIT_FOR_CB) )
        return RvTimerErrorCode(RV_ERROR_OUTOFRANGE);
#endif

    if(behaviour == RV_TIMER_CANCEL_DONT_WAIT_FOR_CB) {
        return RvTimerCancelEx(timer, 0);
    }

    event = timer->event;
    if(event == 0) {
        return RV_OK;
    }

    s = RvTimerGetSleepChannel(&sleepChannel, event->tqueue->logMgr);
    if(s != RV_OK) {
        return s;
    }

    s = RvTimerCancelEx(timer, sleepChannel);
    return s;
}


/********************************************************************************************
 * RvTimerGetRemainingTime - Returns the time left until the timer is expired.
 *
 * INPUT   : timer      - Pointer to timer object.
 * OUTPUT  : delay      - Pointer to a 64-bit variable where the remaining time value will
 *                        be stored.
 * RETURN  : returns: RV_OK if successful otherwise an error code.
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerGetRemainingTime(
    IN  RvTimer     *timer,
    OUT RvInt64     *delay)
{
    RvTimerEvent *event;
    RvLogMgr     *logMgr = NULL;
    RvTimerQueue *tqueue;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(timer == NULL || delay == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    event = timer->event;
    tqueue = event->tqueue;
    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerGetRemainingTime(tqueue=%p)", tqueue));

    *delay = RvInt64Sub(event->triggertime, RvTimestampGet(logMgr));

    RvTimerLogLeave((tqueue->timerSource, "RvTimerGetRemainingTime(tqueue=%p)", tqueue));

    return RV_OK;
}



/********************************************************************************************
 * RvTimerResolution - Gets the resolution of the clock used by timers, in nanoseconds.
 *
 * INPUT   : none
 * OUTPUT  : logMgr - log manager instance
 * RETURN  : Resolution of the timer clock, in nanoseconds.
 */
RVCOREAPI
RvInt64 RVCALLCONV RvTimerResolution(
    RvLogMgr *logMgr)
{
    RvInt64 result;

    RvTimerLogEnter((&logMgr->timerSource, "RvTimerResolution(logMgr=%p)", logMgr));

    result = RvTimestampResolution(logMgr);

    RvTimerLogLeave((&logMgr->timerSource, "RvTimerResolution(logMgr=%p)", logMgr));

    return result;
}


/********************************************************************************************
 * RvTimerQueueService - Checks a timer queue and executes events that should be triggered.
 *
 * INPUT   : tqueue     - Pointer to timer queue object to be serviced.
 *           maxevents  - Maximum number of events that should be executed.
 *           numevents  - Pointer to variable where the number of events
 *                  actually executed will be stored.
 *           alternativeCb - used in stacks where some actions should be applied
 *                  before executing defined for the time-out callbacks. Specifically
 *                  if stack wishes to notify other thread about time-out event instead
 *                  of processing time-out events by the current thread. Note that this callback
 *                  is applied when tqueue->lock is locked.
 *           alternativeCbContext - alternative callback user data
 * OUTPUT  : none
 * RETURN  : RV_OK if successful otherwise an error code. An error of RV_TIMER_ERROR_QUEUESTOPPED
 *          will be returned if the queue hass been stopped (with the RvTimerQueueStop function).
 */
RVCOREAPI
RvStatus RVCALLCONV RvTimerQueueService(
    IN RvTimerQueue *tqueue,
    IN RvSize_t     maxevents,
    IN RvSize_t     *numevents,
    IN RvTimerFunc  alternativeCb,
    IN void         *alternativeCbContext)
{
    RvStatus result;
    RvTimerEvent *event;
    RvBool callbackresult;
    RvSize_t *eventcount, defaultcount;
    RvInt64 curtime;
    RvLogMgr  *logMgr = NULL;
    RvTimer timerInfo;
    RvSize_t qSz;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if(tqueue == NULL)
        return RvTimerErrorCode(RV_ERROR_NULLPTR);
#endif

    logMgr = tqueue->logMgr;

    RvTimerLogEnter((tqueue->timerSource, "RvTimerQueueService(tqueue=%p,maxevents=%d)", tqueue, maxevents));

    /* If numevents is NULL, we still need a counter. */
    if(numevents != NULL) {
        eventcount = numevents;
    } else eventcount = &defaultcount;

    *eventcount = 0;

    result = RvLockGet(&tqueue->lock,tqueue->logMgr);
    if(result != RV_OK) {
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueService(tqueue=%p): Unable to lock tqueue lock", tqueue));
        return result;
    }

    if((tqueue->qState == RV_TIMERQUEUE_DELETED) ||
       (tqueue->qState == RV_TIMERQUEUE_DISABLED && alternativeCb != NULL)) {
        /* Queue has been stopped. */
        RvLockRelease(&tqueue->lock,tqueue->logMgr);
        return RvTimerErrorCode(RV_TIMER_ERROR_QUEUESTOPPED);
    }

    curtime = RvTimestampGet(tqueue->logMgr);
    if (RvInt64IsGreaterThan(tqueue->prevCurtime, curtime))
    {
        RvTimerLogError((tqueue->timerSource, "RvTimerQueueService(tqueue=%p): non monotonic timestamp (curtime=<%d:%9.9d> prevCurtime=<%d:%9.9d>",
            tqueue, RvLogTimestamp(curtime),RvLogTimestamp(tqueue->prevCurtime)));
    }

    qSz = RvPQueueNumItems(&tqueue->pqueue);
    /* protection measure: we will remember the size of the queue before we start to process it
       and we will not process more events than qSz */
    RvTimerLogDebug((tqueue->timerSource, "RvTimerQueueService(tqueue=%p): queue size = %d",
        tqueue,qSz));

    /* Event processing loop. */
    do {
        /* Check to see if the event at top of the queue should be triggered. */
        event = (RvTimerEvent *)RvPQueuePeek(&tqueue->pqueue);
        if((event == NULL) || (qSz == 0) || (RvInt64IsGreaterThan(event->triggertime, curtime)) ||
           (tqueue->qState == RV_TIMERQUEUE_DISABLED && alternativeCb != NULL))
        {
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
            if (tqueue->timerSource != NULL &&
                RvLogIsSelected(tqueue->timerSource, RV_LOGLEVEL_DEBUG))
            {
                RvChar strCurtime[RV_64TOASCII_BUFSIZE];
                RvChar strTriggertime[RV_64TOASCII_BUFSIZE];
                RvChar strDelta[RV_64TOASCII_BUFSIZE];
                RvInt64 delta;

                Rv64toA(curtime, strCurtime);
                if (event != NULL)
                {
                    Rv64toA(event->triggertime, strTriggertime);
                    delta = RvInt64Sub(event->triggertime, curtime);
                    Rv64toA(delta, strDelta);
                }
                else
                {
                    strTriggertime[0] = '\0';
                    strDelta[0] = '\0';
                }
                RvTimerLogDebug((tqueue->timerSource, "RvTimerQueueService(tqueue=%p):qState=%s,event=%p,delta=%s,curtime=%s,triggertime=%s,qSz=%d",
                    tqueue, ((tqueue->qState==RV_TIMERQUEUE_DISABLED)?"DISABLED":"ENABLED"),
                    event, strDelta, strCurtime, strTriggertime,qSz));
            }
#endif /*#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)*/

            break; /* No events to trigger. */
        }
        qSz --;

        if (alternativeCb != NULL)
        {
            RvTimerLogDebug((tqueue->timerSource, "RvTimerQueueService: timer expired(tqueue=%p; event=%p)", tqueue, event));

            RvLockRelease(&tqueue->lock,tqueue->logMgr);
            callbackresult = alternativeCb(alternativeCbContext);
            result = RvLockGet(&tqueue->lock,tqueue->logMgr);
        }
        else
        {
            RvTimerLogDebug((tqueue->timerSource, "RvTimerQueueService: timer is being removed(tqueue=%p; event=%p)", tqueue, event));

            /* Remove that first event and mark it as triggered. */
            RvPQueueRemove(&tqueue->pqueue, event->index); /* Faster than PQueueueGet */
            event->state = RV_TIMER_EVENTSTATE_TRIGGERED;
            *eventcount += 1;
#if RV_TIMER_TEST_TIMERS
            tqueue->lastServiceTime = RvTimerRelativeSecs();
            tqueue->lastSId = (void *)RvThreadCurrentId();
#endif

            /* Call the users callback. */
            tqueue->callcount += 1;
            RvLockRelease(&tqueue->lock,tqueue->logMgr);
            timerInfo.event = event;
            timerInfo.id = event->id;
            callbackresult = event->callback(event->userdata, &timerInfo);
            result = RvLockGet(&tqueue->lock,tqueue->logMgr);
            tqueue->callcount -= 1;

            /* Check to see if event is to be rescheduled. */
            if((event->timertype == RV_TIMER_TYPE_PERIODIC) && (event->canceled == RV_FALSE) &&
               (callbackresult == RV_TRUE) && (tqueue->qState != RV_TIMERQUEUE_DELETED)) {
                /* Reschedule the event */
                event->state = RV_TIMER_EVENTSTATE_QUEUED;
                event->triggertime = RvInt64Add(event->triggertime, event->triggerlength);
                if(RvPQueuePut(&tqueue->pqueue, event) == NULL) {
                    /* We can't reschedule, so just clean up the event. */
                    event->state = RV_TIMER_EVENTSTATE_NOTINUSE;
                    RvObjPoolReleaseItem(&tqueue->pool, event);
                    result = RvTimerErrorCode(RV_TIMER_ERROR_QUEUEFULL);
                    RvTimerLogError((tqueue->timerSource, "RvTimerQueueService(tqueue=%p): Timer queue is full", tqueue));
                }
            } else {
                /* Deal with events that are not to be rescheduled. */
                if(event->sleepChannel != 0) {
                    /* Tasks are waiting to cancel this event, notify them. */
                    /* They'll have to clean up the event. */
                    RvTimerSleepChannel *sleepChannel;

                    for(sleepChannel = event->sleepChannel; sleepChannel != 0; sleepChannel = sleepChannel->next) {
                        sleepChannel->vt->wakeup(sleepChannel, tqueue->logMgr);
                    }

                } else {
                    /* No one waiting so we have to clean up the event. */
                    event->state = RV_TIMER_EVENTSTATE_NOTINUSE;
                    RvObjPoolReleaseItem(&tqueue->pool, event);

                    /* If the timer queue is stopped and we are the last callback, */
                    /* wake up the task that might be waiting for us. */
                    if(tqueue->qState == RV_TIMERQUEUE_DELETED && tqueue->callcount == 0)
                        RvSemaphorePost(&tqueue->wait,tqueue->logMgr);
                }
            }
        }
    } while((result == RV_OK) && (tqueue->qState != RV_TIMERQUEUE_DELETED) &&
            ((maxevents == 0) || (*eventcount < maxevents))); /* Do more events? */

    RvLockRelease(&tqueue->lock,tqueue->logMgr);

#if (RV_NET_TYPE != RV_NET_NONE)
    if (tqueue->selEng != NULL) {
        /* We should set select to trigger when timeout will be expired */
        RvInt64 nextevent = RV_INT64_MAX;

        RvTimerQueueNextEvent(tqueue,&nextevent);

        if (RvInt64IsLessThanOrEqual(nextevent, RvInt64Const(0,0,0)))
            nextevent = RvInt64Const(1,0,1); /* Timeout must be higher than 0 to work at all */
        RvSelectSetTimeOut((RvSelectEngine *)tqueue->selEng, curtime, nextevent, tqueue->logMgr);
    }
#endif

    RvTimerLogLeave((tqueue->timerSource, "RvTimerQueueService(tqueue=%p,events=%d)", tqueue, *eventcount));

    return result;
}

/* Timer debugging utilities */

#if RV_TIMER_TEST_TIMERS

#define GetSecSinceLastService(q) (RvTimerRelativeSecs() - (q)->lastServiceTime)

static RvInt32 RvTimeRelative(RvInt32 *sec, RvInt32 *nsec) {
    RvInt64 now = RvTimestampGet(0);
    RvInt64 delta = now - startingTime;
    RvTime t;

    RvTimeConstructFrom64(delta, &t);
    if(sec) {
        *sec = t.sec;
    }

    if(nsec) {
        *sec = t.nsec;
    }

    return t.sec;
}


typedef struct {
    RvInt64  curTime;
    RvInt    nExpired;
    RvInt    nTimers;
    RvBool   isConsistent;
    RvInt    badTimer;
    RvInt    badTimerParent;
    RvFprint pf;
    void    *pfCtx;
    RvInt    numItems;
} ATCtx;

RvInt AnalyzeTimersCB(RvPQueue *pq, RvPQueueEnumOp op, void *pitem, void *item, void *ctx) {
    ATCtx *atctx = (ATCtx *)ctx;
    RvTimerEvent *son = (RvTimerEvent *)item;
    RvTimerEvent *parent = (RvTimerEvent *)pitem;
    RvFprint pf = atctx->pf;
    void *pfCtx = atctx->pfCtx;
    
    RV_UNUSED_ARG(pq);

    if(op != RV_PQUEUE_NODE) {
        return 1;
    }
    
    atctx->nTimers++;
    if(son->triggertime < atctx->curTime) {
        atctx->nExpired++;
    }
    
    atctx->numItems++;

    if(parent == 0) {
        son->traversed = RV_TIMER_TRAVERSED;
        return 2;
    }
    
    if(son->timertype != RV_TIMER_TYPE_ONESHOT && son->timertype != RV_TIMER_TYPE_PERIODIC) {
        pf(pfCtx, "Illegal timer type (%d) for timer %d\n", son->timertype, son->id);
        return -4;
    }

    if(son->triggertime < parent->triggertime) {
        pf(pfCtx, "Timers is inconsistent: son->triggertime < parent->triggertime (son - %d), (parent - %d)\n",
            son->id, parent->id);
        return -1;
    }

    if(son->triggertime != son->starttime + son->triggerlength) {
        pf(pfCtx, "Timer %d is inconsistent: son->triggertime != son->starttime + son->triggerlength\n", son->id);
        return -2;
    }

    if(son->state <= RV_TIMER_EVENTSTATE_NOTINUSE || son->state > RV_TIMER_EVENTSTATE_TRIGGERED) {
        pf(pfCtx, "Illegal state: %d for timer %d\n", son->state, son->id);
        return -3;
    }

   
    son->traversed = RV_TIMER_TRAVERSED;
    return 0;   
}

static RvInt AnalyzeTimers(RvTimerQueue *tq, RvFprint pf, void *pfCtx) {
    ATCtx ctx;
    RvInt nitems;
    RvInt res;
    
    ctx.curTime = RvTimestampGet(0);
    ctx.isConsistent = RV_TRUE;
    ctx.nTimers = 0;
    ctx.nExpired = 0;
    ctx.pf = pf;
    ctx.pfCtx = pfCtx;
    ctx.nTimers = 0;

    
    pf(pfCtx, "Starting timer analysis...\n");
    res = RvPQueueEnumerate(&tq->pqueue, AnalyzeTimersCB, &ctx);
    if(res < 0) {
        return res;
    } 
    
    nitems = RvPQueueNumItems(&tq->pqueue);
    if(ctx.nTimers != nitems) {
        pf(pfCtx, "Number of timers as returned by RvPQueueNumItems (%d) dffers from scanned during analysis (%d)\n", 
            nitems, ctx.nTimers);
        return -1;
    }

    pf(pfCtx, "Timers seems to be consistent...\n");
    if(ctx.nExpired) {
        pf(pfCtx, "There is %d expired timers...\n", ctx.nExpired);
    } else {
        pf(pfCtx, "There is no expired timers...\n");
    }

        
    return 0;
}


typedef struct {
    RvInt indentLevel;
    RvInt64 startTime;
    RvFprint pf;
    void *pfCtx;
} PQEnumCtx;


static RvInt PQEnumerator(RvPQueue *pq, RvPQueueEnumOp op, void *pitem, void *item, void *ctx) {
    PQEnumCtx *pqCtx = (PQEnumCtx *)ctx;
    RvTimerEvent *ev = (RvTimerEvent *)item;
    RvTimerEvent *parent = (RvTimerEvent *)pitem;
    char indent[80];
    RvFprint pf = pqCtx->pf;
    void *pfCtx = pqCtx->pfCtx;
    RvInt64 relTimeNow, relTimeStart;
    RvChar *canceled[] = {"", "Canceled,"};
    RvChar *state[] = {"NotInuse", "Queued", "Triggered"};
    RvChar *type[] = {"OneShot", "Periodic"};
    RvTime tn, ts;
    

    RV_UNUSED_ARG(pq);
    
    if(op == RV_PQUEUE_LEFT || op == RV_PQUEUE_RIGHT) {
        return 1;
    }
    
    if(op == RV_PQUEUE_UP) {
        pqCtx->indentLevel--;
        return 2;
    }
    
    pqCtx->indentLevel++;
    memset(indent, ' ', pqCtx->indentLevel);
    indent[pqCtx->indentLevel] = 0;
    
    if(parent != 0) {
        if(parent->triggertime > ev->triggertime) {
            pf(pfCtx, "**** Error ****\n");
            return -1;
        }
    }
    
   
    relTimeNow = ev->triggertime - pqCtx->startTime;
    RvTimeConstructFrom64(relTimeNow, &tn);
    relTimeStart = ev->triggertime - startingTime;
    RvTimeConstructFrom64(relTimeStart, &ts);
    
    pf(pfCtx, "%s<%s TT from now - %d, TT from start - %d, id - %d, index - %d, state - %s, type - %s>\n", 
        indent, 
        canceled[ev->canceled],
        tn.sec, 
        ts.sec,
        ev->id,
        ev->index,
        state[ev->state],
        type[ev->timertype]
        );
    return 1;
}

typedef struct {
    RvInt nitems;
    RvInt nfree;
    RvFprint pf;
    void *pfCtx;
    RvTimerQueue *tq;
} OPEnumCtx;

static RvBool ObjPoolEnumerator(RvObjPool *op, void *ctx, void *item, RvBool isFree) {
    OPEnumCtx *ectx = (OPEnumCtx *)ctx;
    RvTimerEvent *t = (RvTimerEvent *)item;
    RvFprint pf = ectx->pf;
    void *pfCtx = ectx->pfCtx;

    RV_UNUSED_ARG(op);

    if(isFree) {
        ectx->nfree++;
        t->traversed = 0;
        return RV_TRUE;
    }

    ectx->nitems++;

    if(t->traversed != RV_TIMER_TRAVERSED) {
        pf(pfCtx, "ObjPool inconsistent: found not traversed item: %d\n", t->id);
        return RV_FALSE;
    }

    t->traversed = 0;
    if(ectx->tq->pqueue.heap[t->index] != t) {
        pf(pfCtx, "Found inconsistency: heap[id] != t (id - %d, index - %d)\n", t->id, t->index);
        return RV_FALSE;
    }

    return RV_TRUE;
}

static int RvAnalyzeTimerPool(RvTimerQueue *tq, RvFprint pf, void *pfCtx) {
    RvInt ntq;
    OPEnumCtx ctx;
    RvInt res;
    RvInt totalItems;

    pf(pfCtx, "Starting ObjPool analysis...\n");
    ntq = tq->pqueue.numitems;
    ctx.nitems = 0;
    ctx.nfree = 0;
    ctx.pf = pf;
    ctx.pfCtx = pfCtx;
    ctx.tq = tq;
    res = RvObjPoolEnumerate(&tq->pool, ObjPoolEnumerator, &ctx);
    if(res != 0) {
        return res;
    }

    if(ctx.nitems != (RvInt)tq->pqueue.numitems) {
        pf(pfCtx, "Number of items in pool (%d) and heap differs (%d)\n", ctx.nitems, tq->pqueue.numitems);
        return -1;
    }

    ctx.nfree = RvObjPoolFreeItems(&tq->pool);
    totalItems = (RvInt)RvObjPoolTotalItems(&tq->pool);
    if(ctx.nitems + ctx.nfree != totalItems) {
        pf(pfCtx, "Total items in pool inconsistent...\n");
        return -1;
    }
    pf(pfCtx, "There is %d items in pool and %d free timers\n", totalItems, ctx.nfree);
    pf(pfCtx, "ObjPool seems OK...\n");
    return 0;
}

RVCOREAPI 
void RvTimerTestTimers(RvTimerQueue *tq, RvFprint pf, void *pfCtx) {
    PQEnumCtx ctx;
    RvChar *states[] = {"Enabled", "Disabled", "Deleted"};
    RvInt res;
    RvInt lsDelta;
    RvInt relTime;

    RvLockGet(&tq->lock, 0);
    relTime = RvTimeRelative(0, 0);

    lsDelta = GetSecSinceLastService(tq);
    pf(pfCtx, "\n\n===== Timer queue status at %d relative time=====\n", relTime);
    pf(pfCtx, "Current state: %s\n", states[tq->qState]);
    pf(pfCtx, "Last service was at %d (%d seconds passed) from thread %p\n", tq->lastServiceTime, lsDelta, tq->lastSId);
    pf(pfCtx, "Last disable was at %d from thread %p\n", tq->lastDisabled, tq->lastDId);
    pf(pfCtx, "Last enable was at %d from thread %p\n", tq->lastEnabled, tq->lastEId);

   
    res = AnalyzeTimers(tq, pf, pfCtx);
    if(res < 0) {
        goto end;
    }

        
    pf(pfCtx, "Timers state:\n");
    ctx.indentLevel = 2;
    ctx.pf = pf;
    ctx.pfCtx = pfCtx;
    ctx.startTime = RvTimestampGet(0);

    RvPQueueEnumerate(&tq->pqueue, PQEnumerator, &ctx);

    RvAnalyzeTimerPool(tq, pf, pfCtx);

    
end:
    RvLockRelease(&tq->lock, 0);
}

#endif /*RV_TIMER_TEST_TIMERS */

