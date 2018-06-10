/* rvevent.c - event object functions */
/************************************************************************
        Copyright (c) 2001-2006 RADVISION Inc. and RADVISION Ltd.
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
#include "rvevent.h"
#include "rvlock.h"
#include "rvsemaphore.h"
#include "rvthread.h"
#include "rvassert.h"

/********************************************************************************************
 * RvEventConstruct - Constructs the event object
 *
 * INOUT   : event	- Pointer to event object to be constructed.
 *                    The event object does not need to be destructed.
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI RvStatus RvEventConstruct(IN RvEvent* event)
{
    event->iSleepingChannel = NULL;
#ifdef RV_DEBUG_EVENT
    event->iChCounter = 0;
#endif        
    return RV_OK;
}

/********************************************************************************************
 * RvEventPulse - Pulses the event object causing all threads waiting on this event object to 
 *                continue. The access to event object has to be protected b 'lock'. If the 'lock'
 *                argument points to valid lock object this lock will be used to protect the access
 *                to event. If 'lock' is NULL it is responsibility of the caller to protect the event
 *                object.
 *
 * IN   : event	- Pointer to event object to be constructed.
 *        lock - Points to valid lock object or NULL.
 *        logMgr - the log manager
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI RvStatus RvEventPulse(IN RvEvent* event, IN RvLock* lock, IN RvLogMgr *logMgr)
{
    RvEventSleepingChannel* ch;

    if (lock)
        RvLockGet(lock,logMgr);
    
    /* wake up all waiting threads who was trying to cancel this event */
    for (ch = event->iSleepingChannel; ch; ch = ch->iSleepingChNext)
        RvSemaphorePost(&ch->iSleepingChSema,logMgr);

    if (lock)
        RvLockRelease(lock,logMgr);    
    
    return RV_OK;
}


/********************************************************************************************
 * RvEventWait - Blocks the calling thread until the event is pulsed by some other thread.
 *               It is supposed that RvEventWait is called when the 'lock' was locked. 
 *               The function will set up waiting thread and then unlock the 'lock' for 
 *               actual waiting. When the function returns it is gurantedd that the 'lock' is locked
 *               again.
 *
 * IN   : event	- Pointer to event object to be constructed.
 *        lock - Points to valid lock object.
 *        logMgr - the log manager
 * RETURN  : RV_OK if successful otherwise an error code.
 */
RVCOREAPI RvStatus RvEventWait(IN RvEvent* event, IN RvLock* lock, IN RvLogMgr *logMgr)
{
    RvThread *th;
    RvEventSleepingChannel *ch, *prevCh;

    th = RvThreadCurrentEx(RV_FALSE);
    if (!th)
        return RV_ERROR_UNKNOWN; 

    if (event->iSleepingChannel)
        th->eventsSleepingCh.iSleepingChNext = event->iSleepingChannel;
    event->iSleepingChannel = &th->eventsSleepingCh;
#ifdef RV_DEBUG_EVENT
    event->iChCounter++;
#endif        
   

    if (lock)
        RvLockRelease(lock,logMgr);
    RvSemaphoreWait(&th->eventsSleepingCh.iSleepingChSema,logMgr);        
    if (lock)
        RvLockGet(lock,logMgr);        

    /* now remove the this thread from the list of threads' sleeping channels */
    ch = event->iSleepingChannel; 
    prevCh = NULL;
    while (ch && ch != &th->eventsSleepingCh)
    {
        prevCh = ch;
        ch = ch->iSleepingChNext;
    }     

    RvAssert(ch);

    /* ch can't be zero here */
    if (prevCh)
        prevCh->iSleepingChNext = ch->iSleepingChNext;
    else
        event->iSleepingChannel = event->iSleepingChannel->iSleepingChNext;
    th->eventsSleepingCh.iSleepingChNext = NULL;
#ifdef RV_DEBUG_EVENT
    event->iChCounter--;
#endif
        
    return RV_OK;
}
