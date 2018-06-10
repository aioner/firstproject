/***********************************************************************
Copyright (c) 2002 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..
RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/

#include "rvtime.h"
#include "rvselect.h"
#include "rvtimer.h"
#include "rvstdio.h"
#include "rvra.h"
#include "rvrtpseli.h"
#include "rvlog.h"




#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                           TYPE DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

static  RvLogMgr  *rtpLogManager = NULL;
#define TMP_LOG rtpLogManager

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
/* Our Windows implementation doesn't support seliCallOn() at all. */
#define SELI_MAX_USER_FDS (0)

#else
/* (RV_OS_TYPE != RV_OS_TYPE_WIN32) */
/* Maximum number of user FDs that will be used.
   This number is the amount of seliCallOn() calls on concurrent fds the user
   is about to use. */
#define SELI_MAX_USER_FDS (10)
#endif


/* User defined SELI events on file descriptors */
typedef struct
{
    RvSelectFd           fd; /* Actual core fd to use */
    RvRtpSeliCallback    callback; /* Callback to use */
} RvRtpSeliUserFd;



/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/

/* Maximum number of user FDs that will be used.
   This number is the amount of seliCallOn() calls on concurrent fds the user
   is about to use. */
static RvSize_t seliMaxUserFds = SELI_MAX_USER_FDS;

/* Array holding the fd's memory allocations */
static HRA seliUserFds = NULL;

/* Indication how many times seliInit() was called. */
static RvInt seliInitCount = 0;


/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/

static void seliEventsCB(
        IN RvSelectEngine*  selectEngine,
        IN RvSelectFd*      fd,
        IN RvSelectEvents   selectEvent,
        IN RvBool           error);


/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/************************************************************************
 * RvRtpSeliInit
 * purpose: Initialize a SELI interface.
 * input  : none
 * output : none
 * return : RV_OK on success, negative value on failure
 ************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliInit(void)
{
    RvStatus status;
    RvSelectEngine *engine;

    status = RvSelectConstruct(2048, 10, TMP_LOG, &engine);

    /* Make sure we allocate some additional fd's for the user */
    if ((status == RV_OK) && (seliUserFds == NULL) && (seliMaxUserFds > 0) && (seliInitCount == 0))
    {
         status = RvRaConstruct(
             sizeof(RvRtpSeliUserFd), (int)seliMaxUserFds, RV_TRUE, "SELI USER FDs", TMP_LOG, &seliUserFds);
    }

    if (status == RV_OK)
        seliInitCount++;

    return status;
}

/************************************************************************
 * RvRtpSeliEnd
 * purpose: End a SELI interface.
 * input  : none
 * output : none
 * return : RV_OK on success, negative value on failure
 ************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliEnd(void)
{
    RvStatus status;
    RvSelectEngine *engine;

    status = RvSelectGetThreadEngine(TMP_LOG, &engine);
    if ((status != RV_OK) || (engine == NULL))
        return status;

    status = RvSelectDestruct(engine, 0);

    seliInitCount--;

    if ((seliUserFds != NULL) && (seliInitCount == 0))
    {
        RvRaDestruct(seliUserFds);
        seliUserFds = NULL;
    }
    return status;
}

/******************************************************************************
 * RvRtpSeliSelectUntil
 * ----------------------------------------------------------------------------
 * General: Block on the select() interface on some operating systems.
 *          Use parallel interfaces on other operating systems.
 *          This function also gives the application the ability to give a
 *          maximum delay to block for.
 *
 * Return Value: RV_OK on success, negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  delay    - Maximum time to block on milliseconds.
 * Output: None.
 *****************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliSelectUntil(IN RvUint32 delay)
{
    RvSelectEngine* selectEngine;
    RvStatus status;
    RvUint64 timeout;

    status = RvSelectGetThreadEngine(TMP_LOG, &selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL))
        return status;

    timeout = RvUint64Mul(RvUint64FromRvUint32(delay), RV_TIME64_NSECPERMSEC);

    if (status == RV_OK)
        status = RvSelectWaitAndBlock(selectEngine, timeout);

    return status;
}

/************************************************************************
 * RvRtpSeliCallOn
 * purpose: Ask the SELI interface for events on a given handle/file
 *          descriptor.
 * input  : fd              - Handle/File descriptor to wait on for events
 *          sEvents         - Events to wait for
 *          callbackFunc    - Function to call when the event occurs
 * output : none
 * return : RV_OK on success, negative value on failure
 ************************************************************************/
RVAPI RvStatus RVCALLCONV RvRtpSeliCallOn(
    IN int                   fd,
    IN RvRtpSeliEvents       sEvents,
    IN RvRtpSeliCallback     callbackFunc)
{
    RvSelectEngine* selectEngine;
    RvStatus status;
    RvSocket s;
    RvSelectFd* coreFd;
    RvRtpSeliUserFd* userFd;
    RvSelectEvents coreEvents = 0;
    int location = 0;

    /* Convert the events to the core's events */
    if (sEvents & RvRtpSeliEvRead) coreEvents |= RvSelectRead;
    if (sEvents & RvRtpSeliEvWrite) coreEvents |= RvSelectWrite;


    /* Find the select engine for this thread at first */
    status = RvSelectGetThreadEngine(TMP_LOG, &selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL))
        return status;

    /* Look for this fd if we're currently waiting for events on it */
    s = (RvSocket)fd;
    coreFd = RvSelectFindFd(selectEngine, &s);

    if ((coreFd == NULL) && ((int)sEvents != 0) && (callbackFunc != NULL))
    {
        /* This is a new fd we're waiting on - add it */

        /* Allocate an FD for the user */
        if (seliUserFds == NULL)
            return RV_ERROR_NOTSUPPORTED;
        RvRaAdd(seliUserFds, (RAElement*)&userFd, &location);
        if (location >= 0)
        {
            userFd->callback = callbackFunc;
            status = RvFdConstruct(&userFd->fd, &s, TMP_LOG);
            if (status == RV_OK)
            {
                status = RvSelectAdd(selectEngine, &userFd->fd, coreEvents, seliEventsCB);
                if (status != RV_OK) RvFdDestruct(&userFd->fd);
            }
            if (status != RV_OK)
                RvRaDelete(seliUserFds, (RAElement)userFd);
        }
        else
            return RV_ERROR_OUTOFRESOURCES; /* No more fd's to spare */
    }
    else if (coreFd != NULL)
    {
        userFd = RV_GET_STRUCT(RvRtpSeliUserFd, fd, coreFd);

        /* We already have it */
        if (((int)sEvents == 0) || (callbackFunc == NULL))
        {
            /* We should remove this fd */
            RvSelectRemove(selectEngine, &userFd->fd);
            RvFdDestruct(&userFd->fd);
            RvRaDelete(seliUserFds, (RAElement)userFd);
        }
        else
        {
            /* We should update this fd */
            status = RvSelectUpdate(selectEngine, &userFd->fd, coreEvents, seliEventsCB);
            if (status == RV_OK)
                userFd->callback = callbackFunc;
        }
    }

    return status;
}

#if 0 /* old for reference */
/***********************************************************************
 * RvRtpSeliSetMaxDescs
 * Description: Sets the maximum number of file descriptors that can be
 *              supported by the Events Loop interface.
 *
 *  input:
 *  maxDescs - Maximum number of file descriptors available.
 *  Returns Non-negative value on success, other on failure.
 *
 * Remarks
 * 1. This function must be called prior to any other call, as it determines
 *    the allocation sizes by the Stack.
 * 2. This function is only available in operating systems that support
 *    a select() interface.
 ************************************************************************/
int RvRtpSeliSetMaxDescs(IN int maxDescs)
{
    return RvSelectSetMaxFileDescriptors((RvUint32)maxDescs);
}

/************************************************************************
 * RvRtpSeliGetMaxDescs
 * Description : Gets the maximum number of file descriptors that are currently
 *               supported by the Events Loop interface.
 * input: none
 * output none
 * Returns Non-negative value on success, other on failure.
 ************************************************************************/
int RvRtpSeliGetMaxDescs(void)
{
    RvUint32 maxFds;

    maxFds = RvSelectGetMaxFileDescriptors();

    if (maxFds == 0)
        return RV_ERROR_UNKNOWN;
    else
        return (int)maxFds;
}




/* The following functions are only relevant for systems supporting the select() interface */
#if (RV_SELECT_TYPE == RV_SELECT_SELECT) && (RV_OS_TYPE != RV_OS_TYPE_WINCE)
/************************************************************************
 * RvRtpSeliSelectEventsRegistration
 * Description: Prepares for a call to select().
 * Parameters
 *    fdSetLen - The maximum length of the file descriptor bit masks.
 *    num      - Returns the highest file descriptor set in the bit masks.
 *               This is the first parameter that should be passed to the
 *               select() function (the nfds parameter).
 *    rdSet    - Bit masks of file descriptors waiting for a Read event.
 *               This is the second parameter that should be passed to the
 *               select() function (the readfds parameter).
 *    wrSet    - Bit masks of file descriptors waiting for a Write event.
 *               This is the third parameter that should be passed to the
 *               select() function (the writefds parameter).
 *    exSet    - Bit masks of file descriptors waiting for exception events.
 *               This is the fourth parameter that should be passed to the
 *               select() function (the errorfds parameter).
 *    timeOut  - Number of milliseconds that should be passed as the timeout
 *               to the select() function. This is the last parameter that
 *               should be passed to the select() function, after conversion
 *               (the timeout parameter).
 *    Returns    RV_OK on success, or a negative value on failure.
 *    Remarks
 * 1) This function is only available when the select() interface is used.
 *    It allows applications to call the select() function independently
 *    from the Stack.
 * 2) The names of the parameters of the select() functions were taken
 *    from the Solaris manual.
 * 3) After calling the select() function, the application should call
 *    RvRtpSeliSelectEventsHandling().
 ************************************************************************/
RvStatus RvRtpSeliSelectEventsRegistration(
    IN  int        fdSetLen,
    OUT int        *num,
    OUT fd_set     *rdSet,
    OUT fd_set     *wrSet,
    OUT fd_set     *exSet,
    OUT RvUint32   *timeOut)
{
    RvSelectEngine* selectEngine;
    RvTimerQueue* timerQueue;
    RvStatus status;

    RV_UNUSED_ARG(exSet);
    RV_UNUSED_ARG(fdSetLen);

    /* Find the select engine for this thread at first */
    status = RvSelectGetThreadEngine(TMP_LOG, &selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL))
        return status;

    /* Get fd_set bits from the select engine */
    status = RvSelectGetSelectFds(selectEngine, num, rdSet, wrSet);
    if (status != RV_OK)
        return status;

    status = RvSelectGetTimeoutInfo(selectEngine, NULL, &timerQueue);
    if (status != RV_OK)
        timerQueue = NULL;

    if (timerQueue != NULL)
    {
        RvInt64 nextEvent = RvInt64Const(0, 0, 0);

        status = RvTimerQueueNextEvent(timerQueue, &nextEvent);
        if (status == RV_OK)
        {
            if (RvInt64IsGreaterThan(nextEvent, RvInt64Const(0, 0, 0)))
            {
                /* Convert it to milliseconds, rounding up */
                *timeOut = RvInt64ToRvUint32(RvInt64Div(
                    RvInt64Sub(RvInt64Add(nextEvent, RV_TIME64_NSECPERMSEC), RvInt64Const(1, 0, 1)),
                        RV_TIME64_NSECPERMSEC));
            }
            else
            {
                /* Seems like we have timers to handle - enter select() without blocking there */
                *timeOut = 0;
            }
        }
        else if (RvErrorGetCode(status) == RV_TIMER_WARNING_QUEUEEMPTY)
        {
            /* Seems like we have no timers to handle - we should give some kind of an "infinite" timeout value */
            *timeOut = (RvUint32)-1;
        }

        status = RV_OK; /* Make sure we select() */
    }

    return status;
}

/************************************************************************
 * RvRtpSeliSelectEventsHandling
 * Description: Handles events after a call to select().
 * Parameters
 *    rdSet     - Bit masks of file descriptors that had Read events on
 *                them in the last call to select(). This is the second parameter
 *                that should be passed to the select() call (the readfds parameter).
 *    wrSet     - Bit masks of file descriptors that had Write events on them in
 *                the last call to select(). This is the third parameter that should
 *                be passed to the select() call (the writefds parameter).
 *    exSet     - Bit mask of file descriptors that had Exception events on them in
 *                the last call to select(). This is the fourth parameter that
 *                should be passed to the select() call (the errorfds parameter).
 *    num       - Highest file descriptor value that was passed to the select() function.
 *                This is the first parameter that should be passed to the select()
 *                call (the nfds parameter).
 *    numEvents - Number of events that occurred in the last call to select().
 *                The return value from the select() function should be placed here.
 *    Returns      RV_OK on success, or a negative value on failure.
 *    Remarks
 * 1) This function is only available when the select() interface is used.
 *    It allows applications to call the select() function independently from the Stack.
 * 2) The names of the parameters of the select() functions were taken from the Solaris manual.
 ************************************************************************/
RvStatus RvRtpSeliSelectEventsHandling(
    IN fd_set   *rdSet,
    IN fd_set   *wrSet,
    IN fd_set   *exSet,
    IN int      num,
    IN int      numEvents)
{
    RvSelectEngine* selectEngine;
    RvTimerQueue* timerQueue = NULL;
    RvStatus status;

    RV_UNUSED_ARG(exSet);

    /* Find the select engine for this thread at first */
    status = RvSelectGetThreadEngine(TMP_LOG, &selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL))
        return status;

    /* Handle the events we have */
    status = RvSelectHandleSelectFds(selectEngine, rdSet, wrSet, num, numEvents);

    /* We need to check the timeout as well... */
    status = RvSelectGetTimeoutInfo(selectEngine, NULL, &timerQueue);
    if ((status == RV_OK) && (timerQueue != NULL))
        RvTimerQueueService(timerQueue, 0, NULL, NULL, NULL);

    return status;
}

#endif  /* (RV_SELECT_TYPE == RV_SELECT_SELECT) && (RV_OS_TYPE != RV_OS_TYPE_WINCE) */



/* The following functions are only relevant for systems supporting the poll() interface */
#if (RV_SELECT_TYPE == RV_SELECT_POLL)
/************************************************************************
 * RvRtpSeliPollEventsRegistration
 * Description: Prepares for a call to poll().
 * Parameters
 *  len       - Maximum number of file descriptors that can be polled (should
 *              be high enough to handle all active calls in the Stack).
 *  pollFdSet - File descriptors array to be polled. This array will be filled
 *              in by the call to this function and should be passed to the poll()
 *              function (the fds parameter).
 *  num       - Returns the number of file descriptors to be polled.
 *              This is the second parameter that should be passed to the poll()
 *              function (the nfds parameter).
 *  timeOut   - Number of milliseconds that should be passed as the timeout to
 *              the poll() function. This is the last parameter that should be passed
 *              to the poll() function (the timeout parameter).
 *  Returns  RV_OK on success, or a negative value on failure.
 *  Remarks
 *   1. This function is only available when the poll() interface is used.
 *      It allows applications to call the poll() function independently from the Stack.
 *   2. The names of the parameters of the poll() functions were taken from the Solaris manual.
 *   3. After calling the poll() function, the application should call
 *      RvRtpSeliPollEventsHandling().
 ************************************************************************/
RvStatus RvRtpSeliPollEventsRegistration(
    IN  int             len,
    OUT struct pollfd   *pollFdSet,
    OUT int             *num,
    OUT RvUint32        *timeOut)
{
    RvSelectEngine* selectEngine;
    RvTimerQueue* timerQueue;
    RvUint32 numFds;
    RvStatus status;

    /* Find the select engine for this thread at first */
    status = RvSelectGetThreadEngine(TMP_LOG, &selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL))
        return status;

    /* Get pollfd array from the select engine */
    numFds = (RvUint32)len;
    status = RvSelectGetPollFds(selectEngine, &numFds, pollFdSet);
    if (status != RV_OK)
        return status;
    *num = (int)numFds;

    status = RvSelectGetTimeoutInfo(selectEngine, NULL, &timerQueue);
    if (status != RV_OK)
        timerQueue = NULL;

    if (timerQueue != NULL)
    {
        RvInt64 nextEvent = RvInt64Const(0, 0, 0);

        status = RvTimerQueueNextEvent(timerQueue, &nextEvent);
        if (status == RV_OK)
        {
            if (RvInt64IsGreaterThan(nextEvent, RvInt64Const(0, 0, 0)))
            {
                /* Convert it to milliseconds, rounding up */
                *timeOut = RvInt64ToRvUint32(RvInt64Div(
                    RvInt64Sub(RvInt64Add(nextEvent, RV_TIME64_NSECPERMSEC), RvInt64Const(1, 0, 1)),
                        RV_TIME64_NSECPERMSEC));
            }
            else
            {
                /* Seems like we have timers to handle - enter poll() without blocking there */
                *timeOut = 0;
            }
        }
        else if (RvErrorGetCode(status) == RV_TIMER_WARNING_QUEUEEMPTY)
        {
            /* Seems like we have no timers to handle - we should give some kind of an "infinite" timeout value */
            *timeOut = (RvUint32)-1;
        }

        status = RV_OK; /* Make sure we poll() */
    }

    return status;
}

/************************************************************************
 * RvRtpSeliPollEventsHandling
 * Description: Handles events after a call to poll().
 * Parameters
 *  pollFdSet  - The array of file descriptors that was passed in the last call to poll().
 *               This is the first parameter that the application has passed to the poll()
 *               call (the fds parameter).
 *  num        - Number of file descriptors that were passed to the poll() function.
 *               This is the second parameter that the application has passed to the
 *               poll() call (the nfds parameter).
 *  numEvents  - Number of events that occurred in the last call to poll(). The return
 *               value from the poll() function should be placed here.
 *  Returns
 *        RV_OK on success, or a negative value on failure.
 *  Remarks
 *  1. This function is only available when the poll() interface is used. It allows
 *     applications to call the poll() function independently from the Stack.
 *  2. The names of the parameters of the poll() functions were taken from the Solaris manual.
 ************************************************************************/
RvStatus RvRtpSeliPollEventsHandling(
    IN struct pollfd    *pollFdSet,
    IN int              num,
    IN int              numEvents)
{
    RvSelectEngine* selectEngine;
    RvTimerQueue* timerQueue = NULL;
    RvStatus status;

    /* Find the select engine for this thread at first */
    status = RvSelectGetThreadEngine(TMP_LOG, &selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL))
        return status;

    status = RvSelectHandlePollFds(selectEngine, pollFdSet, num, numEvents);

    /* We need to check the timeout as well... */
    status = RvSelectGetTimeoutInfo(selectEngine, NULL, &timerQueue);
    if ((status == RV_OK) && (timerQueue != NULL))
        RvTimerQueueService(timerQueue, 0, NULL, NULL, NULL);

    return status;
}

#endif  /* (RV_SELECT_TYPE == RV_SELECT_POLL) */

#endif /* #if 0 old for reference */
/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

static void seliEventsCB(
        IN RvSelectEngine*  selectEngine,
        IN RvSelectFd*      fd,
        IN RvSelectEvents   selectEvent,
        IN RvBool           error)
{
    RvRtpSeliUserFd* userFd;
    RvRtpSeliCallback cb;
    RvSocket* s;
    RvRtpSeliEvents seliEvent = (RvRtpSeliEvents)0;

    RV_UNUSED_ARG(selectEngine);

    userFd = RV_GET_STRUCT(RvRtpSeliUserFd, fd, fd);

    s = RvFdGetSocket(&userFd->fd);

    if (selectEvent & RvSelectRead) seliEvent = RvRtpSeliEvRead;
    else if (selectEvent & RvSelectWrite) seliEvent = RvRtpSeliEvWrite;
    else return;

    cb = userFd->callback;
    if (cb != NULL)
        cb((int)(*s), seliEvent, error);
}


#ifdef __cplusplus
}
#endif
