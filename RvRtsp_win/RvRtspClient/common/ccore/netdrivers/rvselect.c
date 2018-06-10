/* rvselect.c - select interface (select, poll, /dev/poll, etc.) */
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

/**********************************************************************
                        THREAD-SAFENESS GENERAL
 MODULE INITIALIZATION:
                    Select module should be initialized/shutdown
                    by the same thread. It is user responsibility
                    to ensure that during initialization/shutdown
                    no other thread uses the select module. The select
                    module initialization/shutdown is part of
                    CCoreInit/End procedures.
 MULTIPLE INSTANCES:
                    Multiple stack instances are supported by defining
                    select engine. it is user responsibility to synchronize
                    threads during initialization/shutdown of a select engine.
                    It is also assumed that once constructed, some of the
                    select engine parameters will not being changed (see
                    select engine structure definition for exact list).
 **********************************************************************/

#include "rvmemory.h"
#include "rvstdio.h"
#include "rvtime.h"
#include "rvlog.h"
#include "rvsocket.h"
#include "rvselect.h"
#include "rvtimer.h"
#include "rvtimestamp.h"
#include "rv64ascii.h"
#include "rvccoreglobals.h"
#include "rvansi.h"
#include "rvassert.h"

#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS && RV_OS_VERSION < RV_OS_VXWORKS_2_2)
#include <iosLib.h>
#include <msgQLib.h>
#endif
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#pragma warning(disable: 4127)
#endif

#if (RV_NET_TYPE != RV_NET_NONE)

#define RvLogTimestamp(time) RvTimestampGetSecs(time), RvTimestampGetNsecs(time)

/* Defines size of preemption queue 
 * Used only under VxWorks pipe preemption and SMQ preemption
 * mechanism
 */
#ifndef RV_SELECT_PREEMPTION_QUEUE_SIZE
#define RV_SELECT_PREEMPTION_QUEUE_SIZE  3000
#endif

/* Maximum select timeout in seconds 
 * Actually, only solaris is picky about this, but anyway - 10^8 sec (> 3 years) is enough
 */
#define RV_MAX_SELECT_SECTIMEOUT 100000000


/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define RvSelectLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->selectSource, p);}
#else
#define RvSelectLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define RvSelectLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->selectSource, p);}
#else
#define RvSelectLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define RvSelectLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->selectSource, p);}
#else
#define RvSelectLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_EXCEP)
#define RvSelectLogExcep(p) {if (logMgr != NULL) RvLogExcep(&logMgr->selectSource, p);}
#else
#define RvSelectLogExcep(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
#define RvSelectLogDebug(p) {if (logMgr != NULL) RvLogDebug(&logMgr->selectSource, p);}
#else
#define RvSelectLogDebug(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
#define RvSelectLogWarning(p) {if (logMgr != NULL) RvLogWarning(&logMgr->selectSource, p);}
#else
#define RvSelectLogWarning(p) {RV_UNUSED_ARG(logMgr);}
#endif

#define LOGSRC   (&logMgr->selectSource)
#define SRC_FUNC &logMgr->selectSource, FUNC


#if ((RV_SELECT_TYPE == RV_SELECT_WIN32_WSA) || (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION))
/* ------------------------------------ WIN32 SOCKETS --------------------------------- */
#define RvSelectErrNo WSAGetLastError()

/* --------------------------------------- select() ----------------------------------- */
#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)

#if ((RV_OS_TYPE == RV_OS_TYPE_WINCE) || (RV_OS_TYPE == RV_OS_TYPE_WIN32))
#define RvSelectErrNo GetLastError()
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
#include <inc/nu_net.h>
#include <time.h>

#define RvSelectErrNo "error"

#elif (RV_OS_TYPE == RV_OS_TYPE_OSE)
#include <inet.h>

#define RvSelectErrNo errno

#endif

/* --------------------------------------- select() ----------------------------------- */

#define SELECT_FD_SETSIZE FD_SETSIZE        /* Maximum number of file descriptors in fd_set */

/* --------------------------------------- poll() ----------------------------------- */
/* sys/resource.h is needed for getrlimit call to figure out the maximal amount
 * of file descriptors posible. It's used by poll, /dev/poll and kqueue engines
 * but I prefer to put the same #include in conditional compilation section for
 * each of the select engines
 */
#elif (RV_SELECT_TYPE == RV_SELECT_POLL)

#include <sys/resource.h>

/* --------------------------------------- /dev/poll ----------------------------------- */
#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)

#include <sys/resource.h>

/* -------------------------------------- kqueue -------------------------------------*/
#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)

#include <sys/resource.h>

#endif


#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || \
    (RV_OS_TYPE == RV_OS_TYPE_TRU64) || \
    (RV_OS_TYPE == RV_OS_TYPE_LINUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_HPUX) || \
    (RV_OS_TYPE == RV_OS_TYPE_INTEGRITY) || \
    (RV_OS_TYPE == RV_OS_TYPE_FREEBSD) || \
    (RV_OS_TYPE == RV_OS_TYPE_MAC) || \
    (RV_OS_TYPE == RV_OS_TYPE_NETBSD)

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Handle OS error numbers */
#define RvSelectStrError(_error) strerror(_error)
#define RvSelectErrNo errno



#elif (RV_OS_TYPE == RV_OS_TYPE_UNIXWARE)

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

/* Handle OS error numbers */
#define RvSelectStrError(_error) strerror(_error)
#define RvSelectErrNo errno



#elif (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
#include <sysLib.h>
#include <ioLib.h>
#include <errnoLib.h>
#include <pipeDrv.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define RV_PREEMPTION_PIPE_NAME "/pipe/rvPreemptPipe"
#define RV_MAX_ADDR_SIZE sizeof("0xffffffffffffffff")
#define RV_PREEMPTION_PIPE_NAME_SIZE (sizeof(RV_PREEMPTION_PIPE_NAME) + RV_MAX_ADDR_SIZE)

/* Handle OS error numbers */
#define RvSelectStrError(_error) strerror(_error)
#define RvSelectErrNo errnoGet()



#elif (RV_OS_TYPE == RV_OS_TYPE_PSOS)
#include <errno.h>
#include <pna.h>

/* Handle OS error numbers */
#define RvSelectStrError(_error) strerror(_error)
#define RvSelectErrNo errno

#if (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)
/* call pSOS designated function (not posix) that can handle events
   even on sockets called on different threads */
static RvInt RvSelectPsosSelect(
    IN      RvInt numFds,
    INOUT   fd_set *tmpRdSet,
    INOUT   fd_set *tmpWrSet,
    INOUT   fd_set *tmpExSet,
    IN      struct timeval *tm,
    IN      RvLogMgr* logMgr);

#endif



#elif (RV_OS_TYPE == RV_OS_TYPE_MOPI)

/* Handle OS error numbers */
/*lint -save -e652 -e830 -e750 */
/* Warning -> #define of symbol 'malloc(unsigned int)' declared previously in windows's stdlib.h */
/* Info -> local macro 'strerror' not referenced */
#define RvSelectStrError(_error) strerror(_error)
#define RvSelectErrNo               Mmb_errno
/*lint -restore */


#elif (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN)
#include "rvsymselect.h"
#define RvSelectErrNo  0
#endif  /* RV_OS_TYPE */


/* Lets make error codes a little easier to type */
#define RvSelectErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_SELECT, (_e))


#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)

#if (RV_TOOL_TYPE == RV_TOOL_TYPE_ADS)
/* struct timeval is not defined in ads headers (but it is defined in diab) */
struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};

#endif

static RvUint32 RvNucleusSelect(
    IN      RvUint32 nfds,
    INOUT   fd_set *readfds,
    INOUT   fd_set *writefds,
    INOUT   fd_set *exceptfds,
    IN      struct timeval *timeout);

#define select(a, b, c, d, e) RvNucleusSelect(a, b, c, d, e)

#endif

/* Index of TLS variable where select engine of the current thread is stored */
/*static RvUint32 rvSelectTlsVarIndex = 0;*/

#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || \
    (RV_SELECT_TYPE == RV_SELECT_POLL) || \
    (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || \
    (RV_SELECT_TYPE == RV_SELECT_KQUEUE) || \
    (RV_SELECT_TYPE == RV_SELECT_SYMBIAN))
/* Maximum number of file descriptors to allocate operating systems */
/*static RvUint32 rvSelectMaxDescriptors=0;*/
#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE && RV_SELECT_KQUEUE_GROUPS == RV_YES)
/* forward declaration */
RvStatus rvSelectKqueueGroupsConstruct(
    IN RvSelectEngine*  selectEngine);
#endif

#endif


/* Minimum timeout resolution that can be handled. This one on some of the embedded operating
   systems should be modified to make the system work correctly, without exiting the select()
   loop too many times for each timeout. The value is in nanoseconds, */
/*static RvUint64 rvSelectMinTimeoutResolution;*/

#if RV_SELECT_SYNC_REMOVAL
static RvUint32 gsSyncRemovalTLSIndex;

static RvStatus rvFdPreempt(RvSelectEngine   *selectEngine, RvUint8 message);
 
static 
void rvSelectSyncRemovalDestruct(RvThread *th, void *data, RvUint32 idx) {
    RvLogMgr *logMgr = RvThreadGetLogManager(th);
    RvSemaphore *syncSem = (RvSemaphore *)data;

    RV_UNUSED_ARG(idx);
    if(syncSem == 0) {
        return;
    }
    RvSemaphoreDestruct(syncSem, logMgr);
    RvMemoryFree(syncSem, logMgr);
}

static 
RvStatus rvSelectSyncRemovalSemaphoreGet(RvSelectEngine *seli, RvSemaphore **pSem) {
    RvSemaphore *syncSem = 0;
    RvStatus s = RV_OK;
    RvLogMgr *logMgr = seli->logMgr;
    
    s = RvThreadGetVar(gsSyncRemovalTLSIndex, logMgr, (void **)&syncSem);
    if(s != RV_OK) {
        RvSelectLogError((&logMgr->selectSource, "Failed to fetch removal sema"));
        return s;
    }

    if(syncSem == 0) {
        s = RvMemoryAlloc(0, sizeof(RvSemaphore), logMgr, (void **)&syncSem);
        if(s != RV_OK) {
            RvSelectLogError((&logMgr->selectSource, "Failed to allocate removal sema"));
            return s;
        }

        s = RvSemaphoreConstruct(0, logMgr, syncSem);
        if(s != RV_OK) {
            RvSelectLogError((&logMgr->selectSource, "Failed to construct removal sema"));
            RvMemoryFree(syncSem, logMgr);
            return s;
        }

        RvThreadSetVar(gsSyncRemovalTLSIndex, syncSem, logMgr);
    }

    *pSem = syncSem;
    return s;
}
    


/* Wait for removal notifications from the thread running RvSelectWaitAndBlock 
 * This function is called in the 'locked' state of seli and releases it
 *
 */
static
RvStatus rvSelectRemoveWait(RvSelectEngine *selectEngine) {
#define FUNC "rvSelectRemoveWait"

    RvSelectSyncNode curNode;
    RvSemaphore *syncSem;
    RvStatus s = RV_OK;
    RvLogMgr *logMgr = selectEngine->logMgr;

    /* We're in the same thread as RvSelectWaitAndBlock or seli doesn't in the waiting
     * state - just return
     */
    if((selectEngine->selectThread == RvThreadCurrent()) || !selectEngine->waiting) {
        RvLockRelease(&selectEngine->lock, logMgr);
        return s;
    }

    s = rvSelectSyncRemovalSemaphoreGet(selectEngine, &syncSem);

    if(s != RV_OK) {
        RvSelectLogExcep((SRC_FUNC " - syncSem wasn't found, probably calling from non-common core thread"));
        RvLockRelease(&selectEngine->lock, logMgr);
        return s;
    }

    curNode.syncSemaphore = syncSem;

    RvLockGet(&selectEngine->removalLock, logMgr);
    curNode.next = selectEngine->removalSyncList;
    selectEngine->removalSyncList = &curNode;
    RvLockRelease(&selectEngine->removalLock, logMgr);

    rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);
    RvLockRelease(&selectEngine->lock, logMgr);
    RvSemaphoreWait(syncSem, logMgr);
    /* At this stage, seli thread should already remove curNode from the list */
    return s;
#undef FUNC
}

static 
void rvSelectRemoveNotify(RvSelectEngine *selectEngine) {
  RvLogMgr *logMgr = selectEngine->logMgr;
    RvSelectSyncNode *curNode;

    for(;;) {
        RvLockGet(&selectEngine->removalLock, logMgr);
        curNode = selectEngine->removalSyncList;
        if(curNode == 0) {
            RvLockRelease(&selectEngine->removalLock, logMgr);
            break;
        }
        selectEngine->removalSyncList = curNode->next;
        RvLockRelease(&selectEngine->removalLock, logMgr);
        RvSemaphorePost(curNode->syncSemaphore, logMgr);
    }

}


#else
#define rvSelectRemoveNotify(seli)
#endif


/********************************************************************************************
 * fdGetEventStr - Return the string value of a given event
 * This function is not declared as static to remove warnings on release compilations
 * Not thread-safe function.
 *
 * INPUT   : ev   - event which name should be returned
 * OUTPUT  : none
 * RETURN  : pointer to constant string that contains name of the event
 */
const char * fdGetEventStr(
    IN RvSelectEvents ev)
{
    switch (ev)
    {
        case RvSelectRead:     return "Read ";
        case RvSelectWrite:    return "Write ";
        case RvSelectAccept:   return "Accept ";
        case RvSelectConnect:  return "Connect ";
        case RvSelectClose:    return "Close ";
    }

    return "";
}



/********************************************************************************************
 *
 *                  Bucket Hash Functions
 *
 ********************************************************************************************/


/********************************************************************************************
 * fdBucketHashConstruct
 *
 * Create the bucket hash for our use
 * Not thread-safe function
 * INPUT   : selectEngine   - Events engine to use
 *           maxHashSize    - Size of bucket hash to create
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus fdBucketHashConstruct(
    IN RvSelectEngine*      selectEngine,
    IN RvUint32             maxHashSize)
{
    RvStatus result;

    /* Allocate a bucket hash for use later on */
    selectEngine->maxHashSize = maxHashSize;
    result =  RvMemoryAlloc(NULL, sizeof(RvSelectBucket) * maxHashSize,
        selectEngine->logMgr, (void**)&selectEngine->hash);

    if (result == RV_OK)
    {
        memset(selectEngine->hash, 0, sizeof(RvSelectBucket) * maxHashSize);
    }

    selectEngine->firstBucket = NULL;

    /* there are no FD's yet */
    selectEngine->currentFDsCount = 0;

    return result;
}


/********************************************************************************************
 * fdBucketHashDestruct
 * Kills the bucket hash
 * Not thread-safe function
 * INPUT   : selectEngine - Events engine to use
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus fdBucketHashDestruct(
    IN RvSelectEngine*    selectEngine)
{
    selectEngine->firstBucket = NULL;
    selectEngine->currentFDsCount = 0; 
    return RvMemoryFree(selectEngine->hash,selectEngine->logMgr);
}


/********************************************************************************************
 * fdBucketHashFind
 *  Find the file descriptor struct from its OS value
 *  This function looks inside the bucket hash for the specific OS value.
 *  Not thread-safe function
 * INPUT   : selectEngine - Events engine to use
 *           osFd           - OS fd value to search for
 * OUTPUT  : rBucket - the bucket containing the fd
 *           prevFd - the FD struct previous to the found one in the bucket
 * RETURN  : File descriptor struct on success, NULL on failure
 */
static RvSelectFd* fdBucketHashFind(
    IN  RvSelectEngine* selectEngine,
    IN  RvSocket        osFd,
    OUT RvSelectBucket** rBucket,
    OUT RvSelectFd**     rPrevFd)
{
    RvSelectFd *currFd, *prevFd;
    RvSelectBucket* bucket;

    bucket = &(selectEngine->hash[osFd % selectEngine->maxHashSize]);

    currFd = bucket->selectFD;
    prevFd = NULL;

    while ((currFd != NULL) && (currFd->fd != osFd))
    {
        prevFd = currFd;
        currFd = currFd->nextInBucket; /* Search on */
    }

    /* set the values of the bucket and the previous FD 
    only if were asked */

    if (rBucket)
        *rBucket = bucket;

    if (rPrevFd)
        *rPrevFd = prevFd;

    return currFd;
}


/********************************************************************************************
 * fdBucketHashAdd
 * Add the file descriptor to the bucket hash. This function checks if
 * input FD was not already added into hush table.
 * Not thread-safe function, should be used under select engine lock.
 * INPUT   : selectEngine - Events engine to use
 *           fd             - fd to add
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus fdBucketHashAdd(
    IN RvSelectEngine* selectEngine,
    IN RvSelectFd* fd)
{
    RvSelectBucket* bucket;

    /* Check if the fd already in the backet */
    if (fdBucketHashFind(selectEngine,fd->fd,&bucket,NULL) != NULL) {
        /* The FD is already in the bucket.*/
        return RV_OK;
    }
    if (bucket->selectFD == NULL)
    /* this bucket is empty */
    {
        fd->nextInBucket = NULL;
        /* initialize it and insert it in the as the head of buckets list */
        RV_SELECT_INIT_BUCKET(bucket,fd,NULL,selectEngine->firstBucket);   
        if (selectEngine->firstBucket)
            selectEngine->firstBucket->prevBucket = bucket;
        selectEngine->firstBucket = bucket;
    }
    else
    {
        /* Put this fd first in this bucket */        
        fd->nextInBucket = bucket->selectFD;
        bucket->selectFD = fd;
    }
    
    selectEngine->currentFDsCount++; 

    return RV_OK;
}


/********************************************************************************************
 * fdBucketHashRemove
 * Remove the file descriptor from the bucket hash
 * Not thread-safe function
 * INPUT   : selectEngine - Events engine to use
 *           fd             - fd to remove
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus fdBucketHashRemove(
    IN RvSelectEngine* selectEngine,
    IN RvSelectFd* fd)
{
    RvStatus ret = RV_OK;
    RvSelectBucket* bucket;
    RvSelectFd* prevFd;  /* Previous fd in the bucket - NULL if we found the first one */
    RvSelectFd* currFd;  /* Current checked fd in the bucket */

    /* Check if the fd already was deleted */
    if ((currFd = fdBucketHashFind(selectEngine,fd->fd,&bucket,&prevFd)) == NULL) 
    {
        /* The FD is not in the backet.*/
        return RV_OK;
    }

    /* Remove this fd from the bucket hash */
    if (prevFd != NULL)
        prevFd->nextInBucket = currFd->nextInBucket; /* Not first in bucket */
    else
    {
        /* First in bucket - update the first one */ 
        bucket->selectFD = currFd->nextInBucket; 
        if (bucket->selectFD == NULL)        
        {
            /* this bucket is not needed anymore */
            if (selectEngine->firstBucket == bucket)
            {
                /* the first bucket in this engine */
                selectEngine->firstBucket = bucket->nextBucket;
                if (selectEngine->firstBucket)
                    selectEngine->firstBucket->prevBucket = NULL;
            }
            else
            {
                bucket->prevBucket->nextBucket = bucket->nextBucket;
                if (bucket->nextBucket != NULL)
                    bucket->nextBucket->prevBucket = bucket->prevBucket;
            }
            memset(bucket,0,sizeof(RvSelectBucket));
        }
    }

    selectEngine->currentFDsCount--;

    return ret;
}

/********************************************************************************************
 *
 *                  TLS (Thread Local Storage) support Functions
 *
 ********************************************************************************************/
static void rvSelectThreadExit(
    IN RvThread *th,
    IN void     *var,
    IN RvUint32 index)
{
    RvSelectEngine *selectEngine = (RvSelectEngine *)var;

    RV_UNUSED_ARG(index);
    RV_UNUSED_ARG(th);

    if (selectEngine != NULL) {
        /* make sure that selectEngine will be destructed */
        selectEngine->usageCnt = 1;
        RvSelectDestruct(selectEngine,1 /*number of timers is irrelevant here*/);
    }
}

/********************************************************************************************
 *
 *                  Preemption Functions
 *
 ********************************************************************************************/

#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET) || \
    (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)
/********************************************************************************************
 * rvFdPreemptionCallback
 * function, registered to be applied upon any event on the preemption file
 * descriptor (pipe or socket).
 * Thread-safe functions, under assumptions specified in the header of the file
 * INPUT   : selectEngine - Events engine to use
 *           fd           - preemption file descriptor
 *           selectEvent  - event
 *           error        - select error
 * OUTPUT  : none
 * RETURN  : none
 */
static void rvFdPreemptionCallback(
    IN RvSelectEngine*      selectEngine,
    IN RvSelectFd*          fd,
    IN RvSelectEvents       selectEvent,
    IN RvBool               error)
{
#define PREEMPTION_MSG_SIZE 512
    RvUint8     message[PREEMPTION_MSG_SIZE];
    RvInt       i;
    RvLogMgr    *logMgr;
    RvSize_t    bytesRead = 1;

    RV_UNUSED_ARG(selectEvent);
    RV_UNUSED_ARG(error);

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL)
    {
        return;
    }
#endif

    logMgr = selectEngine->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "rvFdPreemptionCallback(eng=%p,fd=%p,event=%p,error=%d)",
        selectEngine,fd,selectEvent,error));

    /* Note that since fd represents select engine pipe/select it
    may be changed only during select engine initialization
    or shutdown. We don't apply lock here to protect the fd.*/
    {
#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET)
        RvAddress dummyAddress;
        RvStatus rvSts = RV_OK;
        RvSize_t read;
        bytesRead = 0;
        /* on this socket usually one-byte datagrams are sent 
        thus we have to perform multiple read operations since 
        on UDP on each recv call only one datagram can be read */
        while (bytesRead < sizeof(message))
        {
            rvSts = RvSocketReceiveBuffer(&fd->fd, message+bytesRead, 
                            sizeof(message)-bytesRead, NULL, &read, &dummyAddress);
            if (rvSts != RV_OK || read == 0)
                break;
            bytesRead += read;
        }
        
        if (rvSts == RV_OK)

#elif (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
        if (ioctl(fd->fd, FIONMSGS, (int)&bytesRead) != ERROR && bytesRead > 0)
#else
        if ((RvInt)(bytesRead = read(fd->fd, &message, sizeof(message))) > 0)
#endif
#endif
        {
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
            if(bytesRead > PREEMPTION_MSG_SIZE) {
                bytesRead = PREEMPTION_MSG_SIZE;
            }
            for (i = 0; i < (RvInt)bytesRead; i++)
                read(fd->fd, &message[i], sizeof(RvUint8));
#endif
            /* In case preemption message was not RvSelectEmptyPreemptMsg,
            and stack registered it's preemption callback, apply the stack
            callback. Note that we should avoid possible dead-lock when two
            mutexes/semaphores are locked in different order by separate threads.
            To ensure this, we 'unlock' the select engine before call to the stack callback */
            if (selectEngine->preemptionCb != NULL)
            {
                for (i = 0; i < (RvInt)bytesRead; i++)
                {
                    RvSelectLogDebug((&logMgr->selectSource,
                        "rvFdPreemptionCallback, received %d preemption message", message[i]));
                    if (message[i] != RvSelectEmptyPreemptMsg)
                    {
                        selectEngine->preemptionCb(selectEngine, message[i], selectEngine->preemptionUsrCtx);
                    }
                }
            }
        }
        else
        {
            /* Seems like nothing was read properly */
            RvSelectLogExcep((&logMgr->selectSource, "rvFdPreemptionCallback read failure"));
        }
    }

    RvSelectLogLeave((&logMgr->selectSource,
        "rvFdPreemptionCallback(eng=%p,fd=%p,event=%p,error=%d)",
        selectEngine, fd, selectEvent, error));
}

/********************************************************************************************
 * rvFdPreemptionConstruct
 * Construct a preemption fd to use when we want to stop a select() loop from
 * blocking.
 * Not thread-safe function.
 * INPUT   : selectEngine - Events engine to use
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus rvFdPreemptionConstruct(
    IN RvSelectEngine   *selectEngine)
{
    RvStatus status;
    RvLogMgr *logMgr = selectEngine->logMgr;

#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET)
    {
        RvUint8 localIp[4] = {127,0,0,1};
        RvUint32* ipPtr = (RvUint32*)localIp;

        RvAddressConstructIpv4(&selectEngine->localAddress, *ipPtr, RV_ADDRESS_IPV4_ANYPORT);

        status = RvSocketConstruct(RV_ADDRESS_TYPE_IPV4, RvSocketProtocolUdp, logMgr, &selectEngine->preemptionSocket);
        if (status != RV_OK)
            return status;

        status = RvSocketSetBlocking(&selectEngine->preemptionSocket, RV_FALSE, logMgr);
        if (status == RV_OK)
            status = RvSocketBind(&selectEngine->preemptionSocket, &selectEngine->localAddress, NULL, logMgr);
        if (status == RV_OK)
            status = RvSocketGetLocalAddress(&selectEngine->preemptionSocket, logMgr, &selectEngine->localAddress);

        if (status != RV_OK)
        {
            RvSocketDestruct(&selectEngine->preemptionSocket, RV_FALSE, NULL, logMgr);
            return status;
        }

        status = RvFdConstruct(&selectEngine->preemptionFd, &selectEngine->preemptionSocket, logMgr);
    }

#elif (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)

#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
    {
        RvChar preemptionPipeName[RV_PREEMPTION_PIPE_NAME_SIZE];

        RvSprintf(preemptionPipeName, "%s%p", RV_PREEMPTION_PIPE_NAME, selectEngine);

        selectEngine->preemptionPipeFd[0] = open(preemptionPipeName, O_WRONLY, 0);
        if (selectEngine->preemptionPipeFd[0] == ERROR)
        {
            if (pipeDevCreate(preemptionPipeName, 
                              RV_SELECT_PREEMPTION_QUEUE_SIZE, sizeof(RvUint8)) == ERROR)
            {
                RvSelectLogError((&logMgr->selectSource,
                    "rvFdPreemptionConstruct: Error creating pipe (%d:%s)",
                    RvSelectErrNo, RvSelectStrError(RvSelectErrNo)));
                return RvSelectErrorCode(RV_SELECT_ERROR_PIPE);
            }
            else
            {
                selectEngine->preemptionPipeFd[0] = open(preemptionPipeName, O_WRONLY, 0);
                if (selectEngine->preemptionPipeFd[0] == ERROR)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "rvFdPreemptionConstruct: Error creating fd 0 (%d:%s)",
                        RvSelectErrNo, RvSelectStrError(RvSelectErrNo)));
                    return RvSelectErrorCode(RV_SELECT_ERROR_PIPE);
                }
            }
        }
        selectEngine->preemptionPipeFd[1] = open(preemptionPipeName, O_RDONLY, 0);
        if (selectEngine->preemptionPipeFd[1] == ERROR)
        {
            close(selectEngine->preemptionPipeFd[0]);
            selectEngine->preemptionPipeFd[0] = -1;
            RvSelectLogError((&logMgr->selectSource,
                "rvFdPreemptionConstruct: Error creating fd 1 (%d:%s)",
                RvSelectErrNo, RvSelectStrError(RvSelectErrNo)));
            return RvSelectErrorCode(RV_SELECT_ERROR_PIPE);
        }
    }
#else  /* RV_OS_TYPE == RV_OS_TYPE_VXWORKS */
    /* Create the preemption pipe if needed */
    if (pipe(selectEngine->preemptionPipeFd) != 0)
    {
        RvSelectLogError((&logMgr->selectSource,
            "rvFdPreemptionConstruct: Error creating pipe (%d:%s)",
            RvSelectErrNo, RvSelectStrError(RvSelectErrNo)));
        return RvSelectErrorCode(RV_SELECT_ERROR_PIPE);
    }
    else
    {
        /* Set the pipe's file descriptors to non-blocking */
        if (fcntl(selectEngine->preemptionPipeFd[0], F_SETFL, O_NONBLOCK) ||
            fcntl(selectEngine->preemptionPipeFd[1], F_SETFL, O_NONBLOCK))
        {
            RvSelectLogError((&logMgr->selectSource,
                "rvFdPreemptionConstruct: Error calling fcntl on pipe(%d:%s)",
                RvSelectStrError(RvSelectErrNo), RvSelectErrNo));
            close(selectEngine->preemptionPipeFd[0]);
            close(selectEngine->preemptionPipeFd[1]);
            return RvSelectErrorCode(RV_SELECT_ERROR_PIPE);
        }
    }
#endif  /* RV_OS_TYPE == RV_OS_TYPE_VXWORKS */

    status = RvFdConstruct(&selectEngine->preemptionFd, &selectEngine->preemptionPipeFd[0], logMgr);
#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE && RV_SELECT_KQUEUE_GROUPS == RV_YES)
    RvFdSetGroup(&selectEngine->preemptionFd, RV_SELECT_KQUEUE_GROUP_CNTL);
#endif

#endif  /* RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET */

    if (status == RV_OK)
    {
        /* Make sure we're waiting for READ events on this preemption fd */
        status = RvSelectAdd(selectEngine, &selectEngine->preemptionFd, RV_SELECT_READ, rvFdPreemptionCallback);
    }

    if (status != RV_OK)
    {
        RvFdDestruct(&selectEngine->preemptionFd);

#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET)
        RvSocketDestruct(&selectEngine->preemptionSocket, RV_FALSE, NULL, logMgr);
#elif (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)
        close(selectEngine->preemptionPipeFd[0]);
        close(selectEngine->preemptionPipeFd[1]);
#endif /* RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET */
    }

    return status;
}



#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS && RV_OS_VERSION < RV_OS_VXWORKS_2_2)
int pipeDevDelete(char *name)
{
    DEV_HDR *pDevHdr;
    char    *pNameTail;

    /* lookup the device */
    if ((pDevHdr = iosDevFind (name, &pNameTail)) == NULL) {
        return ERROR;
    }

    /* consistency check */
    if (strcmp(name, pDevHdr->name)) {
        return ERROR;
    }

    /* destroy the pipe */
    iosDevDelete (pDevHdr); /* delete the device */
    free (pDevHdr);  /* free up the memory allocated to the pipe */
    return OK;

}
#endif

#if (RV_OS_VERSION >= RV_OS_VXWORKS_2_2)
#define RvPipeDevDelete(name,force) pipeDevDelete(name,force)
#else
#define RvPipeDevDelete(name,force) pipeDevDelete(name)
#endif

/********************************************************************************************
 * rvFdPreemptionDestruct
 * Destruct a preemption fd to use when we want to stop a select() loop from
 * blocking.
 * Not thread-safe function.
 * INPUT   : selectEngine - Events engine to use
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus rvFdPreemptionDestruct(
    IN RvSelectEngine   *selectEngine)
{
    RvFdDestruct(&selectEngine->preemptionFd);

#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET)
    RvSocketDestruct(&selectEngine->preemptionSocket, RV_FALSE, NULL, selectEngine->logMgr);
    RvAddressDestruct(&selectEngine->localAddress);
#elif (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)
    close(selectEngine->preemptionPipeFd[0]);
    close(selectEngine->preemptionPipeFd[1]);
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
{
    RvChar preemptionPipeName[RV_PREEMPTION_PIPE_NAME_SIZE];
    RvSprintf(preemptionPipeName, "%s%p", RV_PREEMPTION_PIPE_NAME, selectEngine);
    RvPipeDevDelete(preemptionPipeName, 1);
}
#endif
#endif
    return RV_OK;
}


/********************************************************************************************
 * rvFdPreempt
 * Make sure we're out of the select() loop.
 * Thread-safe functions, under assumptions specified in the header of the file
 * INPUT   : selectEngine   - Events engine to use
 *           message        - message to be send
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus rvFdPreempt(
    IN RvSelectEngine   *selectEngine,
    IN RvUint8          message)
{
    RvStatus status = RV_OK;
    RvLogMgr *logMgr = selectEngine->logMgr;
    RvBool needToWrite = RV_FALSE;

    if (message == RvSelectEmptyPreemptMsg)
    {
        /* We need to verify that we're doing this from the right thread.
           The selectEngine->waiting parameter specifies if application
           entered the select loop from different then the current thread.
           When only FD list or timeout parameter update is required
           (message == RvSelectEmptyPreemptMsg) we can avoid unnecessary
           writing/reading into preemption pipe/socket. */
        if ((selectEngine->selectThread != RvThreadCurrent()) && selectEngine->waiting)
            needToWrite = RV_TRUE;
    }
    else
    {
        /* Preemption with a non-empty message is always written...
           In case (message != RvSelectEmptyPreemptMsg), the message specifies stack
           specific information and then, selectEngine->waiting value should
           not being verified. */
        needToWrite = RV_TRUE;
    }

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
    if (selectEngine->needToPreempt)
    {
        needToWrite = RV_TRUE;
        selectEngine->needToPreempt = RV_FALSE;
    }
#endif

    if (needToWrite)
    {
        /* Note that since fd represents select engine pipe it
        may be changed only during select engine initialization
        or shutdown. We don't apply lock here to protect the fd.*/
        /* We're waiting - let's preempt */
#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET)
        status = RvSocketSendBuffer(&selectEngine->preemptionSocket, &message, sizeof(RvUint8),
                                    &selectEngine->localAddress, logMgr, NULL);
        if (status != RV_OK)
        {
            RvSelectLogError((&logMgr->selectSource,
                "rvFdPreempt: Error calling RvSocketSendBuffer"));
        }

#elif (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)
        if (write(selectEngine->preemptionPipeFd[1], &message, sizeof(RvUint8)) < 0)
        {
            RvSelectLogError((&logMgr->selectSource,
                "rvFdPreempt: Error calling write on pipe(%d:%s)",
                RvSelectErrNo,RvSelectStrError(RvSelectErrNo)));
            status = RvSelectErrorCode(RV_SELECT_ERROR_PIPE);
        }
#endif
    }

    return status;
}

#elif (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_MOPI)

#define rvFdPreemptionConstruct(_selectEngine) RV_OK
#define rvFdPreemptionDestruct(_selectEngine) RV_OK


RvStatus rvFdPreempt(
    IN const RvSelectEngine   *selectEngine,
    IN RvUint8                message)
{
    return Mmb_fdPreempt(selectEngine, message);
}

#elif RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SMQ

static 
RvStatus RvSelectConstructSMQPreemption(RvSelectEngine *seli, RvInt size);

static 
void RvSelectDestructSMQPreemption(RvSelectEngine *seli);

static 
RvStatus rvFdPreemptionConstruct(IN RvSelectEngine *seli) {
  RvStatus s;

  s = RvSelectConstructSMQPreemption(seli, RV_SELECT_PREEMPTION_PIPE_SIZE);
  return s;
}


static
RvStatus rvFdPreemptionDestruct(RvSelectEngine *seli) {

  RvSelectDestructSMQPreemption(seli);
  return RV_OK;
}

static 
RvStatus rvFdPreempt(const RvSelectEngine *seli, IN RvUint8 message) {
  RvStatus s;
  
  s = RvSMQPost(seli->smq, seli->smqMsgTargetId, (RvSMQMsgId)message, 0, 0, seli->logMgr);
  return s;
}

#else    /* No need for preemption functions - we leave them empty */

#define rvFdPreemptionConstruct(_selectEngine) RV_OK
#define rvFdPreemptionDestruct(_selectEngine) RV_OK
#define rvFdPreempt(_selectEngine,_message) (_selectEngine || _message) ? RV_OK : RV_OK

#endif  /* (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET) || \
           (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE) */

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
/********************************************************************************************
 *
 *                  Windows (WSA) Specific internal functions
 *
 ********************************************************************************************/


#define RV_FD_EVENTS_CLASS "RVRTSPCLIENTSELECT"


/********************************************************************************************
 * Window message types for message windows
 *
 * The actual values of these messages is not important and can be changed if they interfere
 * with other messages in the application.
 ********************************************************************************************/
#define FD_NET_MESSAGE      (WM_USER + 100)  /* Network event occured */
#define FD_EXIT_MESSAGE     (WM_USER + 101)  /* Indication to exit the message loop */
#define FD_ENABLE_MESSAGE   (WM_USER + 102)  /* Enables FD */





/********************************************************************************************
 * rvSelectWinFunc
 * Network window message handler
 * This is the routine that is called by Windows when an event occurs.
 * It checks the nature of the event and act upon it.
 * Thread-safe functions, under assumptions specified in the header of the file.
 * INPUT   : hWnd       - Window handle
 *           uMsg       - Window message to handle
 *           wParam     - first 32-bit windows callback parameter
 *           lParam     - second 32-bit windows callback parameter
 * OUTPUT  : none
 * RETURN  : Success
 */
static LRESULT CALLBACK rvSelectWinFunc(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
#define FUNC "rvSelectWinFunc"
{
    RvSelectEngine* selectEngine;
    RvSelectFd* fd;
    RvLogMgr *logMgr = NULL;

#ifdef GetWindowLongPtr
    /* 64bit support in Windows */
    selectEngine = (RvSelectEngine *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
#else
    selectEngine = (RvSelectEngine *)GetWindowLong(hWnd, GWL_USERDATA);
#endif

 
    /* On some messages, selectEngine may be 0, for example: WM_CREATE */
    if(selectEngine != 0) {
        logMgr = selectEngine->logMgr;
    }

    /* analyze the events */
    switch (uMsg)
    {
        /* Window close event - kill the window */
        case WM_CLOSE:
        {
            DestroyWindow(hWnd);
            break;
        }

        /* Someone asked to exit the message loop */
        case FD_EXIT_MESSAGE:
        {
            RvAssert(selectEngine != 0);

            /* Indicate we're done - the message loop checks this one */
            selectEngine->exitMessageLoop = RV_TRUE;
            if(selectEngine->preemptionCb != NULL)
                selectEngine->preemptionCb(selectEngine,(RvUint8)wParam,selectEngine->preemptionUsrCtx);

            break;
        }

        case FD_ENABLE_MESSAGE: 
            RvAssert(selectEngine != 0);
            RvLockGet(&selectEngine->lock,selectEngine->logMgr);
            fd = fdBucketHashFind(selectEngine, (RvSocket)wParam, NULL, NULL);
            RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
            if(fd == 0)  {
                break;
            }
            
            if(fd->fdId != lParam) {
                RvSelectLogDebug((SRC_FUNC ": Ignoring enable event on socket %d (%d)", fd->fd));
                break;
            }
            
            fd->bEnabled = RV_TRUE;
            break;

        /* Network event received - send callback about it */
        case FD_NET_MESSAGE:
        {
            RvSize_t numevents;

            RvAssert(selectEngine != 0);
            RvLockGet(&selectEngine->lock,selectEngine->logMgr);

            /* Find the fd struct for this socket through the use of the bucket hash */
            fd = fdBucketHashFind(selectEngine, (RvSocket)wParam, NULL, NULL);

            /* Make sure we're waiting for events on this fd */
            if (fd != NULL)
            {
                RvSelectCb cb;
                RvSelectEvents events, originalEvents = (RvSelectEvents)0;
                RvBool error, enableRead = RV_FALSE;

                events = (RvSelectEvents)WSAGETSELECTEVENT(lParam);
                error = (RvBool)WSAGETSELECTERROR(lParam);

                if(!fd->bEnabled) {
                    /* We're probably dealing with old event, just ignore it */
                    RvSelectLogDebug((SRC_FUNC ": Ignoring old events on socket %d (%d)", fd->fd, events));
                    goto finish;  
                }

                cb = fd->callback;

                /* Let's make sure we're actually waiting for this event */
                if ((fd->events & events) != 0)
                {
                /* Save the original events on this callback - they might be changed from
                    within the callback, and then, adding back the read event or re-registering
                    write event won't be wise */
                    originalEvents = fd->events;
                    if ((events == RvSelectRead) && ((fd->events & RvSelectClose) != 0))
                    {
                        /* We know it's a TCP connection if we're waiting for close events... */
                        /* On read events of TCP connections, we had better remove the read
                           events until we're done with the callback, since Windows might send
                           us some more read events if we're calling recv() more than once from
                           the callback */
                        WSAAsyncSelect(fd->fd, hWnd, FD_NET_MESSAGE, fd->events & ~RvSelectRead);
                        enableRead = RV_TRUE;
                    }

                    if (events == RvSelectClose)
                    {
                        /* Mark the FD as closed to avoid situation when,
                        in case of multi-threaded application, the FD will
                        be registered for any event after close event was received. */
                        fd->closedByTcpPeer = RV_TRUE;
                    }

                    RvSelectLogDebug((&logMgr->selectSource,
                        "Occured event: fd=%d,event=%s,error=%d", fd->fd, fdGetEventStr(events), error));

                    if (cb != NULL) {
                        /* Releases lock to prevent dead lock. */
                        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
                        cb(selectEngine, fd, events, error);
                        RvLockGet(&selectEngine->lock,selectEngine->logMgr);
                    }

                    if ((enableRead || (events == RvSelectWrite)) &&
                        (originalEvents == fd->events))
                    {
                        /*
                        select events should be re-registered when user callback made no changes in
                        list of events and following conditions exist:
                        1. in case we disabled read event earlier in this function (see details above)
                        2. in case event was write event. This done to ensure that stack will get
                        write event again. In windows, write event will be issued only after writing
                        to the socket is available after WOULDBLOCK error was returned by function
                        like 'sendto' or if WSAAsyncSelect was applied again.
                        */
                        WSAAsyncSelect(fd->fd, hWnd, FD_NET_MESSAGE, fd->events);
                    }

                }
            }
finish:

            RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);

            break;
        }

        case WM_TIMER:
        {
            /* Get start time as soon as possible for best accuracy. */
            RvUint64        currentTime;
            RvSize_t numevents;

            RvAssert(selectEngine != 0);

            selectEngine->wakeupTime = RvUint64Const(0,0);
            currentTime = RvTimestampGet(selectEngine->logMgr);

            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);
        }

        break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0l;
#undef FUNC
}


#endif  /* RV_SELECT_TYPE == RV_SELECT_WIN32_WSA */




#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL) || \
    (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || (RV_SELECT_TYPE == RV_SELECT_KQUEUE))
/********************************************************************************************
 *
 *                  Non-Windows events translation
 *
 ********************************************************************************************/

/* Event Translation table
   User events:

   Rd - read 
   Wr - write 
   Ac - accept
   Cl - close
   Cn - connect

   OS events

   Or - read
   Ow - write
   Ox - exception (exception event is relevant only for WinCE, for all other OSes it's 0 always

  Rd | Wr | Ac | Cl | Cn || Or | Ow | Ox |
  ----------------------------------------
  1  | 0  | 0  | 0  | 0  || 1  | 0  | 0  |
  0  | 1  | 0  | 0  | 0  || 0  | 1  | 0  |
  1  | 1  | 0  | 0  | 0  || 1  | 1  | 0  |
  0  | 0  | 1  | 0  | 0  || 1  | 0  | 0  |
  1  | 0  | 1  | 0  | 0  || 1  | 0  | 0  |
  0  | 1  | 1  | 0  | 0  || 1  | 1  | 0  |
  1  | 1  | 1  | 0  | 0  || 1  | 1  | 0  |
  0  | 0  | 0  | 1  | 0  || 1  | 0  | 0  |
  1  | 0  | 0  | 1  | 0  || 1  | 0  | 0  |
  0  | 1  | 0  | 1  | 0  || 1  | 1  | 0  |
  1  | 1  | 0  | 1  | 0  || 1  | 1  | 0  |
  0  | 0  | 1  | 1  | 0  || 1  | 0  | 0  |
  1  | 0  | 1  | 1  | 0  || 1  | 0  | 0  |
  0  | 1  | 1  | 1  | 0  || 1  | 1  | 0  |
  1  | 1  | 1  | 1  | 0  || 1  | 1  | 0  |
  0  | 0  | 0  | 0  | 1  || 0  | 1  | 1  |
  1  | 0  | 0  | 0  | 1  || 1  | 1  | 1  |
  0  | 1  | 0  | 0  | 1  || 0  | 1  | 1  |
  1  | 1  | 0  | 0  | 1  || 1  | 1  | 1  |
  0  | 0  | 1  | 0  | 1  || 1  | 1  | 1  |
  1  | 0  | 1  | 0  | 1  || 1  | 1  | 1  |
  0  | 1  | 1  | 0  | 1  || 1  | 1  | 1  |
  1  | 1  | 1  | 0  | 1  || 1  | 1  | 1  |
  0  | 0  | 0  | 1  | 1  || 1  | 1  | 1  |
  1  | 0  | 0  | 1  | 1  || 1  | 1  | 1  |
  0  | 1  | 0  | 1  | 1  || 1  | 1  | 1  |
  1  | 1  | 0  | 1  | 1  || 1  | 1  | 1  |
  0  | 0  | 1  | 1  | 1  || 1  | 1  | 1  |
  1  | 0  | 1  | 1  | 1  || 1  | 1  | 1  |
  0  | 1  | 1  | 1  | 1  || 1  | 1  | 1  |
  1  | 1  | 1  | 1  | 1  || 1  | 1  | 1  |
*/  

/* This macro translates events into read/write events so they can work for Unix style events */
#if 0
#define rvSelectToOS(events) \
    ((( (events) & (RV_SELECT_READ | RV_SELECT_ACCEPT | RV_SELECT_CLOSE) ) != 0) ?   \
     ( ( (events) & (RV_SELECT_WRITE | RV_SELECT_CONNECT) ) ?                        \
        (RV_SELECT_READ | RV_SELECT_WRITE) : RV_SELECT_READ ) :                      \
        RV_SELECT_WRITE)
#endif

RvSelectEvents rvSelectToOS(RvSelectEvents events) {
    RvSelectEvents osevents = 0;
    
    if(events & (RV_SELECT_READ | RV_SELECT_ACCEPT | RV_SELECT_CLOSE)) {
        osevents |= RV_SELECT_READ;
    }
    
    if(events & (RV_SELECT_WRITE | RV_SELECT_CONNECT)) {
        osevents |= RV_SELECT_WRITE;
    }
#if 0
#ifdef UNDER_CE 
    
    if(events & RV_SELECT_CONNECT) {
        osevents |= RV_SELECT_EXCPT;
    }
    
#endif
#endif
    
    return osevents;
}

#if (RV_SELECT_TYPE != RV_SELECT_KQUEUE)
static RvStatus rvSocketAtEof(IN RvSocket *sock, 
                              OUT RvBool *atEof) 
{
    
#if RV_OS_TYPE == RV_OS_TYPE_INTEGRITY
    {
        char byte;
        ssize_t retVal;
        
        retVal = recv(*sock, &byte, 1,  MSG_PEEK);
        if(retVal < 0) {
            return RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
        
        *atEof = (retVal == 0);
        return RV_OK;        
    }
#else
    {
        RvStatus s;
        RvSize_t bytesAvailable = 0;
#if RV_OS_TYPE == RV_OS_TYPE_WINCE        
        {
            /* this patch comes to solve the following (on WinCE):
               if the socket has some unread bytes but also 
               the RESET was already received it does
               says there are unread bytes (through RvSocketGetBytesAvailable)
               but attempt to read them out fails.
               Thus we'll just test whether the socket has some error.
               If it received the RESET the error will be reported 
            */
            size_t num,len;
            int ret;        
            int err;
            len = sizeof(num);
            ret = getsockopt(*sock,SOL_SOCKET,SO_ERROR,(char*)&num,&len);
			if (ret < 0 )
				err = GetLastError();
            else
                err = 0;
            /* WinCE 4.2 does not support the getsockopt for SO_ERROR
               thus in case getsockopt fails with NOT SUPPORTED proceed
               with normal treating to RvSocketGetBytesAvailable
            */
            if ((ret < 0 && err != WSAENOPROTOOPT) || (ret && num))
                return RvSocketErrorCode(RV_ERROR_UNKNOWN);
        }
#endif
        s = RvSocketGetBytesAvailable(sock, 0, &bytesAvailable);
        if(s != RV_OK) {
            return s;
        } 
        
        *atEof = !bytesAvailable;
        return RV_OK;
    }
#endif
}
#endif


/********************************************************************************************
 * RvSelectEventsToUser
 * Convert events that occured in the network to those asked by the user.
 * It is needed when we're using Unix systems, that only handle read/write events.
 * Not a thread-safe function (due to "RvSocketGetLastError").
 * INPUT   : osFd           - fd in the OS
 *           wantedEvents   - Events the user asked for
 *           networkEvents  - Events that occured on the network
 *           error          - RV_TRUE if there was any error until now
 * OUTPUT  : networkEvents  - Events that occured on the network, translated to user events
 *           error          - RV_TRUE if there was an error on the network side
 * RETURN  : RV_OK on success, other on failure
 */
static RvStatus RvSelectEventsToUser(
    IN    RvSocket*         osFd,
    IN    RvSelectEvents    wantedEvents,
    INOUT RvSelectEvents*   networkEvents,
    INOUT RvBool*           error)
{
    RvSelectEvents translatedEvents = 0;
    RvBool isCandidateForTCP;

    /* heuristic: fd is candidate for tcp if it's registered for events other then 
     *  WRITE and READ
     */
    isCandidateForTCP = ((wantedEvents | (RV_SELECT_WRITE | RV_SELECT_READ)) != (RV_SELECT_WRITE | RV_SELECT_READ));

    if (*error)
    {
        if (wantedEvents & RV_SELECT_CONNECT)
        {
            *networkEvents = RV_SELECT_CONNECT;
            return RV_OK;
        }
        else if (wantedEvents & RV_SELECT_CLOSE)
        {
            *networkEvents = RV_SELECT_CLOSE;
            return RV_OK;
        }
    }

    if ((*networkEvents) & RV_SELECT_WRITE)
    {
        /* First see about network write events */
        if (wantedEvents & RV_SELECT_CONNECT)
            translatedEvents = RV_SELECT_CONNECT;
        else if (wantedEvents & RV_SELECT_WRITE)
            translatedEvents = RV_SELECT_WRITE;
    }

    else if ((*networkEvents) & RV_SELECT_READ)
    {
        /* See what to do with read events - a little bit more tricky here */
        if (wantedEvents & RV_SELECT_ACCEPT)
            translatedEvents = RV_SELECT_ACCEPT;
        else
        {
            if (wantedEvents & RV_SELECT_CLOSE)
            {
#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
                if ((*networkEvents) & RV_SELECT_CLOSE)
                    translatedEvents = RV_SELECT_CLOSE;
#else
                /* Get the amount of bytes pending - this will indicate if the read event
                   occured because we're about to close this socket */
                RvBool atEof;
                RvStatus ret;

                ret = rvSocketAtEof(osFd,&atEof);
                if (((ret == RV_OK) && atEof) ||
                    ((ret != RV_OK) && (RvErrorGetCode(ret) != RV_ERROR_NOTSUPPORTED)))
                {
                    /* Error getting bytes on this socket */
                    translatedEvents = RV_SELECT_CLOSE;
                }
                else
                {
                    RvAddress remoteAddress;

                    /* Make sure we've got a remote address to work with, otherwise - the
                       connection closed */
                    ret = RvSocketGetRemoteAddress(osFd, NULL, &remoteAddress);
                    if (ret == RV_OK)
                    {
                        RvAddressDestruct(&remoteAddress);
                    }
                    else
                    {
                        /* Can't get remote address? We're closed for the day. */
                        translatedEvents = RV_SELECT_CLOSE;
                    }
                }
#endif
            }

            /* It's not a close event and we're looking for read events */
            if ((translatedEvents == 0) && (wantedEvents & RV_SELECT_READ))
                translatedEvents = RV_SELECT_READ;
        }
    }

    *networkEvents = translatedEvents;

    /* See if we had an error on this socket - we do that only for TCP sockets */
    if ((*error == RV_FALSE) && (isCandidateForTCP == RV_TRUE))
    {
#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
        if ((translatedEvents != RV_SELECT_ACCEPT) && (translatedEvents != RV_SELECT_CLOSE))
            *error = (RvNuIsConnected(*osFd) != NU_TRUE);   /* this is a closed connection ... */
#else
        RvInt32 lastError;

        if (RvSocketGetLastError(osFd, NULL, &lastError) != RV_OK)
            *error = RV_TRUE;

        if (lastError != 0)
            *error = RV_TRUE;
#if RV_OS_TYPE == RV_OS_TYPE_INTEGRITY

        /* On connection failure Integrity doesn't return any error, so we'll
         *  try to check existence of peer address
         */
        if(*error != RV_TRUE && (translatedEvents & RV_SELECT_CONNECT))
        {
            RvAddress dummy;
            RvStatus ret;

            ret = RvSocketGetRemoteAddress(osFd, NULL, &dummy);
            
            if(ret == RV_OK) {
                RvAddressDestruct(&dummy);
            } else {
                *error = RV_TRUE;
            }
            
        }

#endif

#endif
    }

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
    if ((translatedEvents == RV_SELECT_CONNECT) ||
        (translatedEvents == RV_SELECT_CLOSE ))
        RvSocketNucleusTaskDelete(osFd);
#endif

    return RV_OK;
}



#endif  /* select(), poll(), /dev/poll */



#if (RV_SELECT_TYPE == RV_SELECT_SELECT)

/********************************************************************************************
 * RvSelectGetSelectFds
 * Copies fd_set's to temporary fd_set's. This required to prevent deadlock by
 * unlocking select engine before appling user callback. Since before applying
 * the callback we must unlock the engine, it is impossible to use engine internal
 * parameters that may be changed by other threads.
 * Not thread-safe
 * INPUT   : selectEngine   - select engine from which fd_set's will be copied
 *           numFds         - maximum number of file descriptors
 *           rdSet          - memory where to copy read file descriptors set
 *           wrSet          - memory where to copy write file descriptors set
 * OUTPUT  : none
 * RETURN  : none
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectGetSelectFds(
    IN  RvSelectEngine* selectEngine,
    OUT int*            numFds,
    OUT fd_set*         rdSet,
    OUT fd_set*         wrSet)
{
    *numFds = selectEngine->maxFdInSelect + 1;

    /* todo: these memcpy might not work in all operating systems! */
    if (rdSet != NULL)
        memcpy(rdSet, &selectEngine->rdSet, sizeof(fd_set));
    if (wrSet != NULL)
        memcpy(wrSet, &selectEngine->wrSet, sizeof(fd_set));

    return RV_OK;
}


/********************************************************************************************
 * RvSelectHandleSingleSelectFd - tests & applies user callback for one OS fd after select exits
 * INPUT   : selectEngine   - select engine
 *           rdSet          - read file descriptors set
 *           wrSet          - write file descriptors set
 *           fd             - the OS fd to test
 *           numEvents      - the number of events returned by select call (if the fd is handled 
 *                            this number is decremented)
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
static
RvStatus  RvSelectHandleSingleSelectFd(
    IN RvSelectEngine*  selectEngine,
    IN RvBool           hasRead,
    IN RvBool           hasWrite,
    IN RvSocket         fd,
    IN OUT int*         numEvents)
{    
#define FUNC "RvSelectHandleSingleSelectFd"

    if (!hasRead && !hasWrite)
        return RV_OK;

        {
            RvLogMgr  *logMgr = selectEngine->logMgr;
            RvSelectEvents selectEvent = 0;
            RvSelectFd* fdNode;

        /* We've got an event */
            RvLockGet(&selectEngine->lock,selectEngine->logMgr);

            fdNode = fdBucketHashFind(selectEngine, fd, NULL, NULL);

            if ((fdNode != NULL) && (fdNode->callback != NULL))
            {
                RvBool error = RV_FALSE;

#if RV_SELECT_USE_SOCKET_INCARNATION_HEURISTIC
                /* Under this heuristic on each call to RvSelectWaitAndBlock we're handling only
                 *  those FDs that were registered to 'select' BEFORE calling RvSelectWaitAndBlock.
                 *  This treatment assures that new incarnations of old sockets will not get events from
                 *  old incarnations. This heuristic assumes that multiplexing mechanism supports 
                 *  level-triggering semantic. On Nucleus, for simulated 'connect' events we provide
                 *  edge-triggering semantic, so on Nucleus we don't use this heuristic.
                 */
                
                if(RvUint64IsGreaterThanOrEqual(fdNode->timestamp, selectEngine->timestamp)) {
                    RvSelectLogDebug((SRC_FUNC "Ignoring events on socket %d, probably refer to previous incarnation", fdNode->fd));
                    RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
                    return  RV_OK;
                }
#endif

                if (hasRead)
                    selectEvent = RV_SELECT_READ;
                if (hasWrite)
                    selectEvent = RV_SELECT_WRITE;

                /* Translate events to the richer set of events */
                RvSelectEventsToUser(&fdNode->fd, fdNode->events, &selectEvent, &error);

                /* Make sure we're actually waiting for this event */
                if ((fdNode->events & selectEvent) != 0)
                {
                    RvSelectCb callback;

                    RvSelectLogDebug((SRC_FUNC, "Occured event: fd=%d,event=%s,error=%d",
                        fdNode->fd, fdGetEventStr(selectEvent), error));

                    if (selectEvent == RvSelectClose) {
                    /* Mark the FD as closed to avoid situation when,
                    in case of multi-threaded application, the FD will
                        be registered for any event after close event was received. */
                        fdNode->closedByTcpPeer = RV_TRUE;
                    }

                    callback = fdNode->callback;

                    /* Unlock before applying callback to avoid possible deadlock */
                    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
                    /* Tell the user about this event */
                    callback(selectEngine, fdNode, selectEvent, error);
                }
                else {
                    /* ensure that we unlock the select engine */
                    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
                }
            }
            else {
                /* ensure that we unlock the select engine */
                RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        }

        if (hasRead) 
            (*numEvents)--;
        if (hasWrite) 
            (*numEvents)--;
    }
    return RV_OK;
#undef FUNC
}



/********************************************************************************************
 * RvSelectHandleSelectFds - applies user callback after select exits.
 * Thread-safe functions, under assumptions specified in the header of the file
 * INPUT   : selectEngine   - select engine
 *           rdSet          - read file descriptors set
 *           wrSet          - write file descriptors set
 *           numFds         - maximum number of file descriptors
 *           numEvents      - number of occured events
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectHandleSelectFds(
    IN RvSelectEngine*  selectEngine,
    IN fd_set*          rdSet,
    IN fd_set*          wrSet,
    IN int              numFds,
    IN int              numEvents)
{
    RV_UNUSED_ARG(numFds);

    if (numEvents < 0)
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);

    if (numEvents == 0)
        return RV_OK;

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32 || RV_OS_TYPE == RV_OS_TYPE_WINCE)
    {
        u_int cnt;
        for (cnt = 0; cnt < rdSet->fd_count; cnt++)
            RvSelectHandleSingleSelectFd(selectEngine,RV_TRUE,RV_FALSE,
                                         rdSet->fd_array[cnt],&numEvents);
        for (cnt = 0; cnt < wrSet->fd_count; cnt++)
            RvSelectHandleSingleSelectFd(selectEngine,RV_FALSE,RV_TRUE,
                                        wrSet->fd_array[cnt],&numEvents);        
    }
#else
    {
        RvSocket runningSocket;
        RvBool hasRead,hasWrite;
        for (runningSocket = 0; (RvUint32) runningSocket <= selectEngine->maxFdInSelect && numEvents; runningSocket++)
        {
            hasRead = RV_FD_ISSET(runningSocket, rdSet);
            hasWrite = RV_FD_ISSET(runningSocket, wrSet);
            if (hasRead || hasWrite)
                RvSelectHandleSingleSelectFd(selectEngine,hasRead,hasWrite,
                    runningSocket,&numEvents);        
        }
    }
#endif

/*
    / * go over all entries within the hash and find non-empty* /
    currBucket = selectEngine->firstBucket;
    while (currBucket)
    {
        / * iterate on the linked list of non-empty FD's and find the biggest* /
        currFd = currBucket->selectFD;
        while (currFd)
        {
            nextFd = currFd->nextInBucket;
            RvSelectHandleSingleSelectFd(selectEngine,rdSet,wrSet,RV_TRUE,RV_TRUE,currFd->fd,&numEvents);
            if (numEvents == 0)
                goto finished;
            currFd = nextFd; / *currFd->nextInBucket;* /
        }
        currBucket = currBucket->nextBucket;
    }
*/

    return RV_OK;
}


#endif  /* (RV_SELECT_TYPE == RV_SELECT_SELECT) */


#if ((RV_SELECT_TYPE == RV_SELECT_POLL) || (RV_SELECT_TYPE == RV_SELECT_DEVPOLL))

/********************************************************************************************
 * RvSelectGetPollFds
 * Copies select pollfd structure array to temporary pollfd structure array.
 * This required to prevent deadlock by unlocking select engine before applying
 * user callback. Since before applying the callback we must unlock the engine,
 * it is impossible to use engine internal parameters that may be changed by
 * other threads.
 * Not thread-safe
 * INPUT   : selectEngine   - select engine from which fd_set's will be copied
 *           numFds         - maximum number of file descriptors
 *           rdSet          - memory where to copy read file descriptors set
 *           wrSet          - memory where to copy write file descriptors set
 * OUTPUT  : none
 * RETURN  : none
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectGetPollFds(
    IN    RvSelectEngine*   selectEngine,
    INOUT RvUint32*         maxFds,
    OUT   struct pollfd*    pollFdSet)
{
    RvUint32 engineMaxFds;
    RvLogMgr *logMgr;

#if (RV_SELECT_TYPE == RV_SELECT_POLL)
    engineMaxFds = selectEngine->maxFdInPoll;
#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    engineMaxFds = selectEngine->maxFdInPoll;
#endif

    logMgr = selectEngine->logMgr;

    if (*maxFds < engineMaxFds) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectGetPollFds FD is out of range"));
        return RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
    }

    *maxFds = engineMaxFds;
#if RV_SELECT_TYPE == RV_SELECT_POLL
    memcpy(pollFdSet, selectEngine->fdArray, sizeof(struct pollfd) * engineMaxFds);
    return RV_OK;
#else
    return RV_ERROR_NOTSUPPORTED;
#endif
}


/********************************************************************************************
 * RvSelectHandlePollFds - applies user callback after poll exits.
 * Thread-safe functions, under assumptions specified in the header of the file
 * INPUT   : selectEngine   - select engine
 *           pollFdSet      - list of poll file descriptor structures
 *           numFds         - maximum number of file descriptors
 *           numEvents      - number of occured events
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectHandlePollFds(
    IN RvSelectEngine*  selectEngine,
    IN struct pollfd*   pollFdSet,
    IN int              numFds,
    IN int              numEvents)
{
#define FUNC "RvSelectHandlePollFds"

    struct pollfd *pollFdElem;
    RvUint32      i;
    RvSelectFd    *fdNode;
    RvLogMgr      *logMgr = selectEngine->logMgr;

    if (numEvents < 0)
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);

    if (numEvents == 0)
        return RV_OK;

    pollFdElem = pollFdSet;
    for (i = 0; i < (RvUint32)numFds; i++)
    {
        if (pollFdElem->revents != 0)
        {
            RvLockGet(&selectEngine->lock, selectEngine->logMgr);

            /* Find out which fd are we dealing with */
            fdNode = fdBucketHashFind(selectEngine, pollFdElem->fd, NULL, NULL);

            if ((fdNode != NULL) && (fdNode->callback != NULL))
            {
                RvSelectEvents selectEvent = 0;
                RvBool error = RV_FALSE;

                if(RvUint64IsGreaterThanOrEqual(fdNode->timestamp, selectEngine->timestamp)) {
                    RvSelectLogDebug((SRC_FUNC "Ignoring events on socket %d, probably refers to previous incarnation", fdNode->fd));
                    RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
                    return  RV_OK;
                }

                /* Get read/write/error from the returned events */
                if ((pollFdElem->revents & POLLIN) || (pollFdElem->revents & POLLHUP))
                    selectEvent |= RV_SELECT_READ;
                if (pollFdElem->revents & POLLOUT)
                    selectEvent |= RV_SELECT_WRITE;
                if (pollFdElem->revents & POLLERR)
                    error = RV_TRUE;

                /* Translate events to the richer set of events */
                RvSelectEventsToUser(&fdNode->fd, fdNode->events, &selectEvent, &error);

                /* Make sure we're actually waiting for this event */
                if ((fdNode->events & selectEvent) != 0)
                {
                    RvInt32     lastError = 0;
                    RvSelectCb  callback;

                    if (error == RV_TRUE)
                    {
                        RvSocketGetLastError(&fdNode->fd, NULL, &lastError);
                        if (lastError == 0)
                            lastError = 1;
                    }

                    RvSelectLogDebug((&logMgr->selectSource,
                        "Occured event: fd=%d,event=%s,error=%d",
                        fdNode->fd, fdGetEventStr(selectEvent), lastError));

                    if (selectEvent == RvSelectClose) {
                    /* Mark the FD as closed to avoid situation when,
                    in case of multi-threaded application, the FD will
                        be registered for any event after close event was received. */
                        fdNode->closedByTcpPeer = RV_TRUE;
                    }

                    callback = fdNode->callback;
                    /* prevent deadlock */
                    RvLockRelease(&selectEngine->lock, selectEngine->logMgr);

                    /* Tell the user about this event */
                    callback(selectEngine, fdNode, selectEvent, error);
                }
                else
                    RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
            }
            else
                RvLockRelease(&selectEngine->lock, selectEngine->logMgr);

            /* We've got one less result to look for */
            numEvents--;
            if (numEvents == 0)
                break;
        }

        /* Get the next one in array */
        pollFdElem++;
    }

    return RV_OK;
#undef FUNC
}


#endif  /* ((RV_SELECT_TYPE == RV_SELECT_POLL) || (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)) */

#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
/********************************************************************************************
 * RvSelectHandleKqueueFds - applies user callback after kevent() exits.
 * Thread-safe functions, under assumptions specified in the header of the file
 * INPUT   : selectEngine   - select engine
 *           pollFdSet      - list of poll file descriptor structures
 *           numFds         - maximum number of file descriptors
 *           numEvents      - number of occured events
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectHandleKqueueFds(
    IN RvSelectEngine*  selectEngine,
    IN struct kevent*   keventFdSet,
    IN int              numFds,
    IN int              numEvents)
{
    struct kevent *keventFdElem;
    RvSelectFd    *fdNode;
    RvLogMgr      *logMgr = selectEngine->logMgr;
    RV_UNUSED_ARG(numFds);
    if (numEvents < 0)
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);

    if (numEvents == 0)
        return RV_OK;
    for (keventFdElem = keventFdSet; keventFdElem < &keventFdSet[numEvents]; keventFdElem++)
    {
        RvLockGet(&selectEngine->lock, selectEngine->logMgr);

        /* Find out which fd are we dealing with */
        fdNode = fdBucketHashFind(selectEngine, keventFdElem->ident, NULL, NULL);

        if ((fdNode != NULL) && (fdNode->callback != NULL))
        {
            RvSelectEvents selectEvent = 0;
            RvBool error = RV_FALSE;

            if(RvUint64IsGreaterThanOrEqual(fdNode->timestamp, selectEngine->timestamp)) {
                RvSelectLogDebug((SRC_FUNC "Ignoring events on socket %d, probably refers to previous incarnation", fdNode->fd));
                RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
                return  RV_OK;
            }

            /* Get read/write/error from the returned events */
            if (keventFdElem->filter == EVFILT_READ)
                selectEvent |= RV_SELECT_READ;
            if (keventFdElem->filter == EVFILT_WRITE)
                selectEvent |= RV_SELECT_WRITE;
            if (keventFdElem->flags & EV_EOF)
                selectEvent |= RV_SELECT_CLOSE;

            /* Translate events to the richer set of events */
            RvSelectEventsToUser(&fdNode->fd, fdNode->events, &selectEvent, &error);
            /* Make sure we're actually waiting for this event */
            if ((fdNode->events & selectEvent) != 0)
            {
                RvInt32     lastError = 0;
                RvSelectCb  callback;

                RvSelectLogDebug((&logMgr->selectSource,
                    "Occured event: fd=%d,event=%s,error=%d",
                    fdNode->fd, fdGetEventStr(selectEvent), lastError));

                if (selectEvent == RvSelectClose) {
                /* Mark the FD as closed to avoid situation when,
                in case of multi-threaded application, the FD will
                    be registered for any event after close event was received. */
                    fdNode->closedByTcpPeer = RV_TRUE;
                }

                callback = fdNode->callback;
                /* prevent deadlock */
                RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
                /* Tell the user about this event */
                callback(selectEngine, fdNode, selectEvent, error);
            }
            else
                RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
        }
        else
            RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
    }

    return RV_OK;
}


#endif  /* (RV_SELECT_TYPE == RV_SELECT_KQUEUE) */
#endif /* (RV_NET_TYPE != RV_NET_NONE) */


/********************************************************************************************
 *
 *                          Public functions
 *
 ********************************************************************************************/



/********************************************************************************************
 * RvSelectInit - Initiates select module.
 * This function must be applied only once when initiating process that uses the common core.
 * Even in case there are multiple stacks that uses common core, this function must be applied
 * only once. The function is registered in the rvccore.c file in RvCCoreModules structure and
 * is applied automatically by function RvCCoreInit.
 * Not thread-safe functions.
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSelectInit(void)
{
#if (RV_NET_TYPE != RV_NET_NONE)

    RvStatus retCode;
    RV_USE_CCORE_GLOBALS;

    retCode = RvThreadCreateVar(rvSelectThreadExit,
        "RvSelectEngine",NULL,&rvSelectTlsVarIndex);

    if (retCode != RV_OK) {
        return retCode;
    }

    /* Default value for minimal timeout resolution */
    rvSelectMinTimeoutResolution =  RvUint64Const(0,0);

    /* By operating system */
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
    {
        int ticksPerSecond = sysClkRateGet();
        if (ticksPerSecond <= 0)
            return RvSelectErrorCode(RV_ERROR_UNKNOWN);

        /* VxWorks likes to reduce the timeout given to select() to the closest rate value it
           can handle in a given DSP. To make sure we don't go anywhere below this value, we
           should calculate this one here */
        rvSelectMinTimeoutResolution = RvInt64Div(RV_TIME64_NSECPERSEC, RvInt64FromRvInt(ticksPerSecond));
    }
#endif

    /* By interface */
#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    {
        WNDCLASS wc;
        WSADATA wsaData;

        /* Start Windows Sockets */
        WORD winsockVersion = MAKEWORD(2, 2);
        if (WSAStartup(winsockVersion, &wsaData) != 0)
            return RvSelectErrorCode(RV_ERROR_UNKNOWN);

        /* Create a window class for network events */
        memset(&wc, 0, sizeof(WNDCLASS));
        wc.lpfnWndProc   = rvSelectWinFunc;
        wc.hInstance     = NULL;
        wc.lpszClassName = RV_FD_EVENTS_CLASS;
        if (!RegisterClass(&wc))
        {
            DWORD winerr = GetLastError();
            return RvSelectErrorCode(winerr);
        }

    }


#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)
    {
        WSADATA wsaData;

        /* Start Windows Sockets */
        WORD winsockVersion = MAKEWORD(2, 2);
        if (WSAStartup(winsockVersion, &wsaData) != 0)
            return RvSelectErrorCode(RV_ERROR_UNKNOWN);

    }


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)
#if ((RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_WINCE))
    {
        WSADATA wsaData;
        
        /* Start Windows Sockets */
        WORD winsockVersion = MAKEWORD(2, 2);
        if (WSAStartup(winsockVersion, &wsaData) != 0)
            return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }
#endif
    
    /* set rvSelectMaxDescriptors if it was not set yet */
    if (!rvSelectMaxDescriptors)
        rvSelectMaxDescriptors = SELECT_FD_SETSIZE;


#elif ((RV_SELECT_TYPE == RV_SELECT_POLL) || (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || \
       (RV_SELECT_TYPE == RV_SELECT_KQUEUE))
    /* Find out the limits posed by the OS on the amount of file descriptors */
    {
        struct rlimit r;

        if (getrlimit(RLIMIT_NOFILE, &r) < 0)
            return RvSelectErrorCode(RV_SELECT_ERROR_GETRLIMIT);

#ifdef RV_SELECT_MAX_DESCRIPTORS
        rvSelectMaxDescriptors = RvMin(r.rlim_cur, RV_SELECT_MAX_DESCRIPTORS);
#else
        rvSelectMaxDescriptors = r.rlim_cur;
#endif
    }

#endif

#if RV_SELECT_SYNC_REMOVAL
    /* Allocate TLS index for synchronization semaphore */
    RvThreadCreateVar(rvSelectSyncRemovalDestruct, "SelectSync", 0, &gsSyncRemovalTLSIndex);
#endif


#endif




    return RV_OK;
}


/********************************************************************************************
 * RvSelectEnd - Shutdown select module.
 * This function must be applied only once during shutdown of process that uses the common core.
 * Even in case there are multiple stacks that uses common core, this function must be applied
 * only once. The function is registered in the rvccore.c file in RvCCoreModules structure and
 * is applied automatically by function RvCCoreEnd.
 * Not thread-safe functions.
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSelectEnd(void)
{
    RvStatus ret = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    if (WSACleanup() != 0)
        ret = RvSelectErrorCode(RV_ERROR_UNKNOWN);

    if (UnregisterClass(RV_FD_EVENTS_CLASS, NULL/*hInstance*/) == 0)
    {
        DWORD err = GetLastError();
        ret = RvSelectErrorCode(err);
    }

#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)
    if (WSACleanup() != 0)
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)

#if ((RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_WINCE))    
    if (WSACleanup() != 0)
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
#endif

#elif (RV_SELECT_TYPE == RV_SELECT_POLL)


#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)

#elif (RV_SELECT_TYPE == RV_SELECT_SYMBIAN)

#endif

#endif

    return ret;
}


/********************************************************************************************
 * RvSelectSourceConstruct - initiates log source, used for select module logging.
 * This function must be applied once per instance of log manager. This function
 * is registered in list of RvLogModules and automatically applied by function
 * RvLogConstruct. It's assumed that this function is applied in main thread
 * and only once for specific select engine. Note that log source destruct applied
 * automatically when stack applied log manager destruct function.
 * Not thread-safe functions.
 * INPUT   : logMgr     - log manager instance
 * OUTPUT  : none
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvSelectSourceConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus result = RV_OK;

#if (RV_NET_TYPE != RV_NET_NONE)
    result = RvLogSourceConstruct(logMgr, &logMgr->selectSource, "SELECT", "File Descriptors Events Engine");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return result;
}



#if (RV_NET_TYPE != RV_NET_NONE)

/********************************************************************************************
 * RvSelectSetMaxFileDescriptors
 *
 * Set the amount of file descriptors that the Select module can handle in a single
 * select engine. This is also the value of the highest file descriptor possible.
 * Check if your stack and application uses this function before porting it.
 * This function should be called before calling RvSelectConstruct() and after RvSelectInit().
 * Not thread-safe
 * INPUT   : maxFileDescriptors - Maximum value of file descriptor possible
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectSetMaxFileDescriptors(
    IN RvUint32 maxFileDescriptors)
{

#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || \
    (RV_SELECT_TYPE == RV_SELECT_POLL)    || \
    (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || \
    (RV_SELECT_TYPE == RV_SELECT_KQUEUE)  || \
    (RV_SELECT_TYPE == RV_SELECT_SYMBIAN))
    RV_USE_CCORE_GLOBALS;
    rvSelectMaxDescriptors = maxFileDescriptors;
    return RV_OK;
#else
    RV_UNUSED_ARG(maxFileDescriptors);
    return RvSelectErrorCode(RV_ERROR_NOTSUPPORTED);
#endif
}


/********************************************************************************************
 * RvSelectGetMaxFileDescriptors
 *
 * Get the current value used as the maximum value for a file descriptor by the select engine.
 * Thread-safe functions, under assumptions specified in the header of the file.
 *
 * input   : None
 * output  : None
 * return  : Maximum value for a file descriptor select engines are going to support.
 */
RVCOREAPI
RvUint32 RVCALLCONV RvSelectGetMaxFileDescriptors(void)
{
#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || \
    (RV_SELECT_TYPE == RV_SELECT_POLL)    || \
    (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || \
    (RV_SELECT_TYPE == RV_SELECT_KQUEUE)  || \
    (RV_SELECT_TYPE == RV_SELECT_SYMBIAN))
    RV_USE_CCORE_GLOBALS;
    return rvSelectMaxDescriptors;
#else
    return 0;
#endif
}


/********************************************************************************************
 * RvSelectSetPreemptionCb
 * Sets stack preemption callback that later used to notify stack about
 * non-empty preemption messages.
 * Thread-safe functions, under assumptions specified in the header of the file.
 * INPUT   : selectEngine     - Events engine to construct
 *           maxHashSize        - Hash size used by the engine
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectSetPreemptionCb(
    IN  RvSelectEngine          *selectEngine,
    IN  RvSelectPreemptionCb    preemptionCb,
    IN  void                    *preemptionCtx)
{
    RvLogMgr *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    logMgr = selectEngine->logMgr;

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);
    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectSetPreemptionCb(eng=%p,callback=%p,ctx=%p)",
        selectEngine,preemptionCb,preemptionCtx));
    selectEngine->preemptionCb = preemptionCb;
    selectEngine->preemptionUsrCtx = preemptionCtx;
    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectSetPreemptionCb(eng=%p,callback=%p,ctx=%p)=%d",
        selectEngine,preemptionCb,preemptionCtx,RV_OK));
    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
    return RV_OK;
}

/********************************************************************************************
 * RvSelectSetTimeoutInfo
 * Sets stack timeout processing callback. Useful when stack wish to process timeouts
 * differently than simply applying timer service routine.
 * Thread-safe functions, under assumptions specified in the header of the file.
 * INPUT   : selectEngine     - select engine object
 *           timeOutCb        - timeout callback
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectSetTimeoutInfo(
    IN  RvSelectEngine          *selectEngine,
    IN  RvTimerFunc             timeOutCb,
    IN  void                    *cbContext)
{
    RvLogMgr *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);
    logMgr = selectEngine->logMgr;
    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectSetTimeoutInfo(eng=%p,callback=%p)",
        selectEngine,timeOutCb));
    selectEngine->timeOutCb = timeOutCb;
    selectEngine->timeOutCbContext = cbContext;
    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectSetTimeoutInfo(eng=%p,callback=%p)=%d",
        selectEngine,timeOutCb,RV_OK));
    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
    return RV_OK;
}

#if RV_SELECT_TYPE == RV_SELECT_POLL || RV_SELECT_TYPE == RV_SELECT_DEVPOLL

#if RV_SELECT_TYPE == RV_SELECT_POLL
#  define NFDARRAYS 2
#else
#  define NFDARRAYS 1
#endif

static RvStatus ExpandFdArrays(RvSelectEngine *seli, RvSize_t newFds) {
#define FUNC "ExpandFdArrays"
#define logMgr (seli->logMgr)
    
    RvSize_t newFdArraySize;
    RvStatus s = RV_OK;
    void *tmp;
    struct pollfd *newTmpFdArray;
    RvInt nArrays;

    if(newFds >= seli->maxFd) {
        return RV_ERROR_BADPARAM;
    }

    if(newFds <= seli->fdArraySize) {
        return RV_OK;
    }

    if(seli->fdArraySize == 0) {
        newFdArraySize = newFds;
    } else {
        newFdArraySize = seli->fdArraySize;
        while(newFdArraySize < newFds) {
            newFdArraySize <<= 1;
        }
    }



    s = RvMemoryAlloc(0, NFDARRAYS * sizeof(seli->tmpFdArray[0]) * newFdArraySize, logMgr, &tmp);
    if(s != RV_OK) {
        RvSelectLogError((SRC_FUNC " - seli %p: expanding fd arrays to %d elems failed", seli, newFdArraySize));
        return s;
    }

    newTmpFdArray = (struct pollfd *)tmp;
    RvSelectLogDebug((SRC_FUNC " - seli %p: fd arrays expanded to %d elems", seli, newFdArraySize));


#if RV_SELECT_TYPE == RV_SELECT_POLL 
    {
        struct pollfd *newFdArray;

        newFdArray = newTmpFdArray + newFdArraySize;

        if(seli->maxFdInPoll > 0) {
            memcpy(newFdArray, seli->fdArray, sizeof(seli->fdArray[0]) * seli->fdArraySize);
        }
        seli->fdArray = newFdArray;
    }
    
#endif /* RV_SELECT_TYPE == RV_SELECT_POLL */


    if(seli->tmpFdArray) {
        RvMemoryFree(seli->tmpFdArray, logMgr);
    }
    seli->tmpFdArray = newTmpFdArray;
    seli->fdArraySize = newFdArraySize;
    return s;
#undef FUNC
#undef logMgr
}

#endif /*  RV_SELECT_TYPE == RV_SELECT_POLL || RV_SELECT_TYPE == RV_SELECT_DEVPOLL */

/********************************************************************************************
 * RvSelectConstruct
 *
 * Allocates and initiates a new select engine.
 * In case select engine was already constructed for the current thread,
 * pointer to the existing agent will be returned.
 *
 *
 * INPUT   : maxHashSize    - Hash size used by the engine
 *           maxTimers      - maximum initial value for number of timers in the
 *                            timer queue, associated with the select engine
 *           logMgr         - log manager
 * OUTPUT  : engine         - Events engine to construct.
 * RETURN  : RV_OK on success, other on failure
 */

RVCOREAPI
RvStatus RVCALLCONV RvSelectConstruct(
    IN    RvUint32         maxHashSize,
    IN    RvUint32         maxTimers,
    IN    RvLogMgr         *logMgr,
    OUT   RvSelectEngine   **engine)
{
    RvStatus         status = RV_OK;
    RvSelectEngine   *selectEngine;
    RvThread         *th = NULL;
    RvThread         *constructedThread = 0;
    RV_USE_CCORE_GLOBALS;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectConstruct(maxHashSize=%d,maxTimers=%d,logMgr=%p,eng=%p)",
        maxHashSize,maxTimers,logMgr,engine));

    /* Get/Create common core structure, associated with the current thread */

    th = RvThreadCurrent();
    if (th == NULL) {
        status = RvMemoryAlloc(NULL,sizeof(RvThread),logMgr,(void **)&th);
        if (status != RV_OK) {
            RvSelectLogError((&logMgr->selectSource,
                "RvSelectConstruct failed to allocate thread wrapper"));
            return status;
        }
        memset(th,0,sizeof(RvThread));

        status = RvThreadConstructFromUserThread(logMgr,th);
        if (status != RV_OK) {
            RvSelectLogError((&logMgr->selectSource,
                "RvSelectConstruct failed to associate thread wrapper to thread"));
        }

        constructedThread = th;
    }

    /* Retrieve select engine from the TLS */
    status = RvThreadGetVar(rvSelectTlsVarIndex,
        logMgr,(void **)&selectEngine);
    if (status != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectConstruct RvThreadGetVar failed"));
        return status;
    }

    if (selectEngine != NULL) {
        RvSize_t  qAddedLen;
        RvInt     needToAdd;

        /* Select engine for this thread already created */
        *engine = selectEngine;
        /* Update usage counter */
        selectEngine->usageCnt++;
        needToAdd = (RvInt)((selectEngine->maxTimers + maxTimers) - RvTimerQueueGetSize(&selectEngine->tqueue));
        if (needToAdd > 0) {
            /* Increase maximum number of timers in the timer queue */
            qAddedLen  = RvTimerQueueAddSize(&selectEngine->tqueue, needToAdd);
            if ((RvSize_t)needToAdd > qAddedLen) {
                RvSelectLogError((&logMgr->selectSource,
                    "RvTimerQueueAddSize failed"));
                return RvSelectErrorCode(RV_ERROR_UNKNOWN);
            }
        }

        selectEngine->maxTimers += maxTimers;

        /* Set the log manager if we don't have any yet */
        if (selectEngine->logMgr == NULL)
        {
            selectEngine->logMgr = logMgr;
            RvTimerQueueSetLogMgr(&selectEngine->tqueue,logMgr);
            status = RvLogSelectUsageInc(logMgr);
        }

        RvSelectLogLeave((&logMgr->selectSource,
            "RvSelectConstruct engine exists (status=%d", status));
        return status;
    }


    /* No select engine was created for this thread */
    status = RvMemoryAlloc(NULL,sizeof(RvSelectEngine),logMgr,(void **)&selectEngine);
    if ((status != RV_OK) || (selectEngine == NULL)) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectConstruct failed to allocate select engine"));
        return status;
    }

    memset(selectEngine, 0, sizeof(RvSelectEngine));

    /* Log is a very elementary facility, let's set logMgr first.
       store the Log Manager of the current CC instance           */
    selectEngine->logMgr = logMgr;

    selectEngine->constructedThread = constructedThread;

    /* We also must protect stack from situation when log was destructed
    before destructing the select engine */
    status = RvLogSelectUsageInc(logMgr);
    if (status != RV_OK) {
        RvMemoryFree((void *)selectEngine,logMgr);
        return status;
    }

    status = RvLockConstruct(selectEngine->logMgr, &selectEngine->lock);
    if (status != RV_OK) {
        RvMemoryFree((void *)selectEngine,logMgr);
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectConstruct lock construct failure"));
        /* May cause logMgr destruction, so no logging should be done after this */
        RvLogSelectUsageDec(logMgr);
        return status;
    }

    /* Create a bucket hash */
    status = fdBucketHashConstruct(selectEngine, maxHashSize);
    if (status != RV_OK)
    {
        RvLockDestruct(&selectEngine->lock,selectEngine->logMgr);
        RvSelectLogError((&logMgr->selectSource,
            "fdBucketHashConstruct failure"));
        RvMemoryFree((void *)selectEngine,logMgr);
        /* May cause logMgr destruction, so no logging should be done after this */
        RvLogSelectUsageDec(logMgr);
        return status;
    }

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    {
        /* Create a window to use for network events */
        //modified by lichao, 20140814 修改64位系统上面 sprintf越界的问题
        char windowName[sizeof(RV_FD_EVENTS_CLASS) + sizeof(selectEngine) * 2];

        if (DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
            GetCurrentProcess(), &selectEngine->threadHandle,
            0, FALSE, DUPLICATE_SAME_ACCESS) == 0)
        {
            /* Got an error */
            status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
        }
        else
        {
            RvSprintf(windowName, "%s%p", RV_FD_EVENTS_CLASS, selectEngine);
            selectEngine->hNetEventsWnd =
                CreateWindow(RV_FD_EVENTS_CLASS, windowName,
                    WS_OVERLAPPED | WS_MINIMIZE,
                    0, 0, 0, 0, NULL, NULL, NULL/*hInstance*/, NULL);
            if (selectEngine->hNetEventsWnd == NULL)
            {
                CloseHandle(selectEngine->threadHandle);
                status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
            }
        }

        if (status == RV_OK)
        {
            selectEngine->threadId = GetCurrentThreadId();

#ifdef SetWindowLongPtr
            /* 64bit support for Windows */
            SetWindowLongPtr(selectEngine->hNetEventsWnd, GWLP_USERDATA, (LONG_PTR)selectEngine);
#else
            SetWindowLong(selectEngine->hNetEventsWnd, GWL_USERDATA, (LONG)selectEngine);
#endif
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)
    selectEngine->completionPort =
        CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)0, 0);
    if (selectEngine->completionPort == NULL) {
        RvSelectLogError((&logMgr->selectSource,
            "CreateIoCompletionPort failure"));
        status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)
    selectEngine->maxFd = rvSelectMaxDescriptors;
    selectEngine->maxFdInSelect = 0;

    /* Clear file descriptor sets */
    RV_FD_ZERO(&selectEngine->rdSet);
    RV_FD_ZERO(&selectEngine->wrSet);

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
    RV_FD_ZERO(&selectEngine->simulatedRdSet);
    RV_FD_ZERO(&selectEngine->simulatedWrSet);
    selectEngine->simulatedNumFd = 0;
    selectEngine->simulatedMaxFd = 0;
#endif

#elif (RV_SELECT_TYPE == RV_SELECT_POLL)
    selectEngine->maxFd = rvSelectMaxDescriptors;
    selectEngine->maxFdInPoll = 0;

    status = ExpandFdArrays(selectEngine, maxHashSize);

#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    selectEngine->maxFd = rvSelectMaxDescriptors;
    selectEngine->maxFdInPoll = 0;

    /* Open /dev/poll file descriptor we'll use */
    if ((selectEngine->fdDevPoll = open("/dev/poll", O_RDWR)) < 0) {
        status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
        RvSelectLogError((&logMgr->selectSource,
            "open /dev/poll failure"));
    }
    else
    {
        status = ExpandFdArrays(selectEngine, maxHashSize);

        if (status != RV_OK) {
            close(selectEngine->fdDevPoll);
        }
        else
        {
            status = RvSemaphoreConstruct(1, selectEngine->logMgr, &selectEngine->devPollWrite);
            if (status != RV_OK)
                RvSelectLogError((&logMgr->selectSource,
                "RvSemaphoreConstruct for devPollWrite failed"));
        }
    }

#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
    selectEngine->maxFd = rvSelectMaxDescriptors;

    /* Create a 'kqueue' event queue */
    if ((selectEngine->kqueueId = kqueue()) == -1)
    {
        status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
        RvSelectLogError((&logMgr->selectSource, "creating kqueue failure"));
    }
    else
    {
#if (RV_SELECT_KQUEUE_GROUPS == RV_YES)
        /* construct the low and high priority Kqueues and register the fd's
         * of these Kqueues on the "control" Kqueue which we have already created.
         * (we don't need the fdArray at this moment).
         */
        rvSelectKqueueGroupsConstruct(selectEngine);
        /* if "Groups" policy is enabled then we can have only 3 events at the most:
           2 for the low and high priority Kqueues and 1 for the preemption socket/pipe */
        selectEngine->nFdArray = 4;
#else
        /* otherwise we can expect for 2 events (Read & Write) for each fd */
        selectEngine->nFdArray = rvSelectMaxDescriptors * 2;
#endif
        status = RvMemoryAlloc(NULL, selectEngine->nFdArray * sizeof(struct kevent),
                               selectEngine->logMgr, (void**)&selectEngine->fdArray);
        if (status != RV_OK)
            RvSelectLogError((&logMgr->selectSource, "RvMemoryAlloc failure"));
    }
#elif (RV_SELECT_TYPE == RV_SELECT_SYMBIAN)

    status = RvSymSelectConstruct(selectEngine);
    if (status != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSymSelectConstruct failure"));
        RvMemoryFree((void *)selectEngine,logMgr);
        RvLogSelectUsageDec(logMgr);
        return status;
    }

#endif


    if(status == RV_OK) {
        status = rvFdPreemptionConstruct(selectEngine);

#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_MOPI)

    status = Mmb_RvSelectConstruct(selectEngine);

#endif
    } else {
        RvSelectLogError((&logMgr->selectSource, "rvFdPreemptionConstruct failure"));
        fdBucketHashDestruct(selectEngine);
        RvLockDestruct(&selectEngine->lock,selectEngine->logMgr);
        RvMemoryFree((void *)selectEngine,logMgr);
        RvLogSelectUsageDec(logMgr);
        return status;
    }

    /* initiate usage counter */
    selectEngine->usageCnt = 1;

    /* In multi-threaded environment, add the select engine to TLS */
    status = RvThreadSetVar(rvSelectTlsVarIndex,(void *)selectEngine,logMgr);
    if (status != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "Adding select engine to TLS failed"));
        fdBucketHashDestruct(selectEngine);
        RvLockDestruct(&selectEngine->lock,selectEngine->logMgr);
        RvMemoryFree((void *)selectEngine,logMgr);
        RvLogSelectUsageDec(logMgr);
        return status;
    }

    /* Construct timer queue for the select engine */
    status = RvTimerQueueConstruct(RV_TIMER_QTYPE_FIXED, maxTimers, 0, 0, 0,
    RV_SELECT_TIMERS_PER_PAGE, NULL, selectEngine, logMgr, &selectEngine->tqueue);

    if (status != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvTimerQueueConstruct failure"));
        fdBucketHashDestruct(selectEngine);
        RvLockDestruct(&selectEngine->lock,selectEngine->logMgr);
        RvMemoryFree((void *)selectEngine,logMgr);
        RvLogSelectUsageDec(logMgr);
        return status;
    }

    selectEngine->maxTimers = maxTimers;

    /* Construct fd removal sync objects*/
#if RV_SELECT_SYNC_REMOVAL
    RvLockConstruct(logMgr, &selectEngine->removalLock);
    selectEngine->removalSyncList = 0;
#endif
    
    *engine = selectEngine;

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectConstruct(eng=%p,maxHashSize=%d,logMgr=%p)",
        selectEngine,maxHashSize,logMgr));

    if (status != RV_OK) {
        RvLogSelectUsageDec(logMgr);
    }
    return status;
}



/********************************************************************************************
 * RvSelectDestruct
 *
 * Destruct a select engine
 * Not thread-safe function.
 *
 * INPUT   : selectEngine - Events engine to destruct
 *           maxTimers    - number of timers, added to the
 *                          select engine's tqueue for this
 *                          select engine instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectDestruct(
    IN RvSelectEngine   *selectEngine,
    IN RvUint32         maxTimers)
{
    RvStatus        ret;
    RvLogMgr        *logMgr = NULL;
    RvSelectEngine  *localThreadEngine = NULL;
    RV_USE_CCORE_GLOBALS;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    logMgr = selectEngine->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectDestruct(eng=%p)",selectEngine));

    ret = RvThreadGetVar(rvSelectTlsVarIndex,logMgr,
        (void **)&localThreadEngine);
    if (ret != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectDestruct, RvThreadGetVar failure"));
        return ret;
    }

    if (localThreadEngine != selectEngine) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectDestruct, trying to destruct select engine from thread other than thread where it was constructed"));
        return RvSelectErrorCode(RV_ERROR_BADPARAM);
    }

    /* The usage counter is used always by the same thread.
    That because it is changed/verified only during select
    construct or destruct. According to all stacks requirement
    select MUST be constructed and destructed by the same thread
    as well as Wait & Block MUST be applied by the same thread
    that constructed the select engine.
    Because of specified above, there is no need to 'lock' select
    engine when addressing usage counter */
    /* decrease usage counter */
    selectEngine->usageCnt--;

    if(selectEngine->usageCnt > 0) {
        /* the select engine is still in use
        by other stack(s). */
        RvLockGet(&selectEngine->lock,selectEngine->logMgr);
        if (selectEngine->maxTimers > maxTimers) {
            selectEngine->maxTimers -= maxTimers;
        }
        else {
            selectEngine->maxTimers = 1;
        }
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        RvSelectLogLeave((&logMgr->selectSource,
            "RvSelectDestruct(eng=%p)",selectEngine));
        return RV_OK;
    }

    ret = RvThreadSetVar(rvSelectTlsVarIndex,NULL,logMgr);
    if (ret != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectDestruct, RvThreadSetVar failure"));
        return ret;
    }

    /* Decrement log manager usage counter.
    In case select engine is the last of the log manager users,
    and RvLogDestruct already was applied, log manager will be destroied */
    selectEngine->logMgr = NULL;
    selectEngine->tqueue.logMgr = NULL;
    ret = RvLogSelectUsageDec(logMgr);
    if (ret != RV_OK) {
        /* there are more select engines that still are using the log manager */
        return ret;
    }
    logMgr = NULL;

    ret = RvTimerQueueDestruct(&selectEngine->tqueue);
    if (ret != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectDestruct, RvThreadSetVar failure"));
        return ret;
    }

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    DestroyWindow(selectEngine->hNetEventsWnd);
    CloseHandle(selectEngine->threadHandle);


#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)
    CloseHandle(selectEngine->completionPort);


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)

#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
    ret = Mmb_RvSelectDestruct((void*)selectEngine);
#endif
#elif (RV_SELECT_TYPE == RV_SELECT_POLL)

#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    close(selectEngine->fdDevPoll);
    if(selectEngine->tmpFdArray) {
        RvMemoryFree(selectEngine->tmpFdArray, logMgr);
    }
    RvSemaphoreDestruct(&selectEngine->devPollWrite, logMgr);
#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)

    RvMemoryFree(selectEngine->fdArray, logMgr);
    close(selectEngine->kqueueId);

#if (RV_SELECT_KQUEUE_GROUPS == RV_YES)
    RvMemoryFree(selectEngine->fdArrayLow, logMgr);
    RvMemoryFree(selectEngine->fdArrayHigh, logMgr);
    close(selectEngine->kqueueIdLow);
    close(selectEngine->kqueueIdHigh);
#endif

#elif (RV_SELECT_TYPE == RV_SELECT_SYMBIAN)

    ret = RvSymSelectDestruct(selectEngine);
    if (ret != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSymSelectConstruct failure"));
        RvLogSelectUsageDec(logMgr);
        RvMemoryFree((void *)selectEngine,logMgr);
        return ret;
    }
#endif

    ret = rvFdPreemptionDestruct(selectEngine);

    /* Deallocate the bucket hash we're using */
    fdBucketHashDestruct(selectEngine);

    /* Kill the lock */
    RvLockDestruct(&selectEngine->lock, logMgr);

    /* free the resource allocated for RvThread structure due to
    RvThreadConstructFromUserThread call */
    if (selectEngine->constructedThread)
    {
        RvThreadDestruct(selectEngine->constructedThread);
        RvMemoryFree(selectEngine->constructedThread, logMgr);
        selectEngine->constructedThread = NULL;
    }


    /* free the select engine memory */
    RvMemoryFree(selectEngine, logMgr);

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectDestruct(eng=%p)=%d",
        selectEngine,RV_OK));

    return ret;
}


/********************************************************************************************
 * RvSelectGetThreadEngine
 *
 * Get a select-engine structure that belongs to the current thread.
 * Not thread-safe function.
 *
 * INPUT   : logMgr       - log manager instance
 * OUTPUT  : selectEngine - Select engine associated with the current thread
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectGetThreadEngine(
    IN  RvLogMgr         *logMgr,
    OUT RvSelectEngine   **selectEngine)
{
    RvStatus retCode = RV_OK;
    RV_USE_CCORE_GLOBALS;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectGetThreadEngine(logMgr=%p,selectEngine=%p)",logMgr,selectEngine));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectGetThreadEngine, wrong input parameter, selectEngine == NULL"));
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    /* Retrieve select engine from the TLS */
    retCode = RvThreadGetVar(rvSelectTlsVarIndex,
        logMgr,(void **)selectEngine);
    if (retCode != RV_OK) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectGetThreadEngine RvThreadGetVar failed"));
        return retCode;
    }

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectGetThreadEngine(logMgr=%p,selectEngine=%p)",logMgr,selectEngine));

    return RV_OK;
}


/********************************************************************************************
 * RvSelectGetTimeoutInfo
 * Retrieves stack timeout processing information, including callback and timeout events queue
 * Thread-safe functions, under assumptions specified in the header of the file.
 * INPUT   : selectEngine     - select engine object
 * OUTPUT  : timeOutCb        - timeout callback
 *           tqueue           - timeout events queue
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectGetTimeoutInfo(
    IN  RvSelectEngine          *selectEngine,
    OUT RvTimerFunc             *timeOutCb,
    OUT RvTimerQueue            **tqueue)
{
    RvLogMgr *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);
    logMgr = selectEngine->logMgr;
    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectGetTimeoutInfo(eng=%p,callback=%p,tqueue=%p)",
        selectEngine,timeOutCb,tqueue));
    if (tqueue != NULL) {
        *tqueue = &selectEngine->tqueue;
    }
    if (timeOutCb != NULL) {
        *timeOutCb = selectEngine->timeOutCb;
    }
    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectGetTimeoutInfo(eng=%p,callback=%p,tqueue=%p)=%d",
        selectEngine,timeOutCb,tqueue,RV_OK));
    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
    return RV_OK;
}


/********************************************************************************************
 * RvFdConstruct
 *
 * Construct a file descriptor for a given socket.
 * This function must not be applied on fd structure after
 * it was added to RvSelect waiting loop (or before it was removed
 * from there)
 * Not thread-safe function
 * INPUT   : fd     - File descriptor to construct
 *           s      - Socket to use for this file descriptor
 *           logMgr - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvFdConstruct(
    IN RvSelectFd   *fd,
    IN RvSocket     *s,
    IN RvLogMgr     *logMgr)
{
    RvSelectLogEnter((&logMgr->selectSource,
        "RvFdConstruct(fd=%p,socket=%d,logMgr=%p)",
        fd,*s,logMgr));

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((fd == NULL) || (s == NULL)) {
        RvSelectLogError((&logMgr->selectSource,
            "RvFdConstruct, wrong input parameters"));
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif

    fd->fd = *s;
    fd->closedByTcpPeer = RV_FALSE;
    fd->logMgr = logMgr;
    fd->timestamp = RV_UINT64_ZERO;

#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
#if (RV_SELECT_KQUEUE_GROUPS == RV_YES)
    fd->group = RV_SELECT_KQUEUE_GROUP_LOW;
#endif
#endif

    RvSelectLogLeave((&logMgr->selectSource,
        "RvFdConstruct(fd=%p,socket=%d,logMgr=%p)",
        fd,*s,logMgr));

    return RV_OK;
}


/********************************************************************************************
 * RvFdDestruct
 *
 * Destruct a file descriptor of a given socket.
 * It is assumed that this function will not be called if the given socket is still
 * waiting for events in the select engine. This means that RvSelectRemove() has
 * been called before calling this function.
 * Not thread-safe function
 * INPUT   : fd - File descriptor to destruct
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvFdDestruct(
    IN RvSelectFd*        fd)
{
    RvLogMgr *logMgr;

    RV_UNUSED_ARG(fd);

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (fd == NULL)
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
#endif
    logMgr = fd->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvFdDestruct(fd=%p)",fd));
    RvSelectLogLeave((&logMgr->selectSource,
        "RvFdDestruct(fd=%p)",fd));
    return RV_OK;
}


/********************************************************************************************
 * RvFdGetSocket
 *
 * Get the socket associated with the file descriptor struct.
 * Not thread-safe
 * INPUT   : fd - File descriptor
 * RETURN  : Socket associated with the file descriptor on success
 *           NULL on failure
 */
RVCOREAPI
RvSocket* RVCALLCONV RvFdGetSocket(
    IN RvSelectFd* fd)
{
    RvLogMgr *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (fd == NULL)
        return NULL;
#endif

    logMgr = fd->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvFdGetSocket(fd=%p)",fd));
    RvSelectLogLeave((&logMgr->selectSource,
        "RvFdGetSocket(fd=%p)",fd));

    return &fd->fd;
}


/********************************************************************************************
 * RvSelectGetEvents
 *
 * Return the list of events we're waiting on for a given file descriptor
 * Not thread-safe
 * INPUT   : fd - File descriptor
 * OUTPUT  : None
 * RETURN  : Events we're waiting on
 */
RVCOREAPI
RvSelectEvents RVCALLCONV RvSelectGetEvents(
    IN RvSelectFd* fd)
{
    RvLogMgr *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (fd == NULL)
        return (RvSelectEvents)0;
#endif
    logMgr = fd->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectGetEvents(fd=%p)",fd));
    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectGetEvents(fd=%p)",fd));

    return fd->events;
}


/********************************************************************************************
 * RvSelectFindFd - Find the RvSelectFd object by the socket/file descriptor it's connected to.
 *
 * This function is used by the H.323 Stack to allow calls to seliCallOn() from the
 * stack's API. You don't need to port it if you are using other Stacks or if you
 * are not using the seliCallOn() function from your H.323 application.
 * Thread-safe functions, under assumptions specified in the header of the file.
 * Note that it still may be problem if you relay on output of the function since
 * after it called, list of file descriptor structure may be changed by other thread.
 *
 * INPUT   : selectEngine   - Events engine to look in
 *           s              - Socket/file descriptor object to find
 * OUTPUT  : None
 * RETURN  : RvSelectFd object if one exists, NULL otherwise.
 */
RVCOREAPI
RvSelectFd* RVCALLCONV RvSelectFindFd(
    IN RvSelectEngine*  selectEngine,
    IN RvSocket*        s)
{
    RvSelectFd *fd;
    RvLogMgr   *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((selectEngine == NULL) || (s == NULL))
        return NULL;
#endif

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);
    logMgr = selectEngine->logMgr;
    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectFindFd(eng=%p,socket=%d)",selectEngine,*s));
    fd = fdBucketHashFind(selectEngine, *s, NULL, NULL);
    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectFindFd(eng=%p,socket=%d)=%p",
        selectEngine,*s,fd));
    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

    return fd;
}


/********************************************************************************************
 * RvSelectAdd - Add a new file descriptor to those being checked
 *
 * It's not suggested to call this function on FD that was already added. Once added, use
 * RvSelectUpdate() to update the events we're waiting for, or RvSelectRemove()
 * to remove this file descriptor from being handled by the select engine.
 * Thread-safe functions, under assumptions specified in the header of the file.
 *
 * INPUT   : selectEngine   - Events engine of this fd
 *           fd             - File descriptor to check
 *           selectEvents   - Events to check
 *           eventsCb       - Callback to use when these events occur
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectAdd(
    IN RvSelectEngine*      selectEngine,
    IN RvSelectFd*          fd,
    IN RvSelectEvents       selectEvents,
    IN RvSelectCb           eventsCb)
{
    RvBool   allocatedBucketHash = RV_FALSE;
    RvStatus ret = RV_OK;
    RvLogMgr *logMgr;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((selectEngine == NULL) || (fd == NULL))
        return RvSelectErrorCode(RV_ERROR_BADPARAM);
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
    /* Since RvSocketSetSelectEngine() is going to protect its data structures
     * using a lock - we better call this function before we lock the select engine
     * in order to prevent deadlocks
     */
    RvSocketSetSelectEngine(RvFdGetSocket(fd), (void*)selectEngine);
#endif

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);

    logMgr = fd->logMgr;
    fd->timestamp = selectEngine->timestamp;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectAdd(fd=%d,events=%s%s%s%s%s,cb=%p)", fd->fd,
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectRead)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectWrite)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectAccept)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectConnect)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectClose)),
        eventsCb));

    if (fd->closedByTcpPeer == RV_TRUE) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectAdd failure, FD was closed by the TCP peer"));
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }
#if RV_SELECT_TYPE == RV_SELECT_WIN32_WSA    
    fd->bEnabled = RV_FALSE;
    fd->fdId     = selectEngine->additionCounter++;
#endif

    /* Add the events to the bucket hash */
    ret = fdBucketHashAdd(selectEngine, fd);
    if (ret == RV_OK) {
        allocatedBucketHash = RV_TRUE;
    }
    else
    {
        RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectAdd: fdBucketHashAdd failed"));
        return ret;
    }
   
#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    if(!PostMessage(selectEngine->hNetEventsWnd, FD_ENABLE_MESSAGE, (WPARAM)fd->fd, fd->fdId)) {
        RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
        RvSelectLogError((LOGSRC, "RvSelectAdd: Unable to post ENABLE message, %d", GetLastError()));
        return RV_ERROR_UNKNOWN;
    }

    if (WSAAsyncSelect(fd->fd, selectEngine->hNetEventsWnd, FD_NET_MESSAGE, selectEvents) != 0) {
        RvLockRelease(&selectEngine->lock, selectEngine->logMgr);
        RvSelectLogError((LOGSRC, "RvSelectAdd: Unable to register socket with WSAAsyncSelect, %d", 
            WSAGetLastError()));
        return RV_ERROR_UNKNOWN;
    }
    

#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)
    {
        HANDLE ioCompletionPort;
        ioCompletionPort = CreateIoCompletionPort((HANDLE)fd->fd,
            selectEngine->completionPort, (ULONG_PTR)fd, 0);
        if (ioCompletionPort == NULL)
            ret = RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)
    if (ret == RV_OK)
    {
        RvSelectEvents translatedEvents;

#if (RV_OS_TYPE != RV_OS_TYPE_WINCE) && (RV_OS_TYPE != RV_OS_TYPE_WIN32) && (RV_CHECK_MASK & RV_CHECK_RANGE)
        /* Windows CE doesn't really care that the socket's fd value is within
           the range of FD_SETSIZE. This is because the fd_set struct is actually
           defined more like poll - a list of fd's to select and not a bits buffer.
           If we'll range check for WinCE we'll just get errors after several socket
           constructions. */
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
        {
            ret = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
            RvSelectLogError((&logMgr->selectSource,
                "RvSelectAdd: Range check"));
        }

        if (ret == RV_OK)
#endif
        {
            /* Update the maximum limit of select() if we have to */
            if (fd->fd > (RvSocket)selectEngine->maxFdInSelect)
                selectEngine->maxFdInSelect = fd->fd;

            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = (RvSelectEvents)rvSelectToOS(selectEvents);

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
            if (selectEvents != RvSelectConnect)
#endif
            {            
            if (translatedEvents & RV_SELECT_READ)
                RV_FD_SET(fd->fd, &selectEngine->rdSet);
            if (translatedEvents & RV_SELECT_WRITE)
                RV_FD_SET(fd->fd, &selectEngine->wrSet);
            }
#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
            /* Update hash-table on the new fd that is added to the select-engine */
            Mmb_RvSelectAdd((void*)selectEngine, fd->fd);
#endif
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_POLL)
    if (ret == RV_OK)
    {
        RvSelectEvents translatedEvents;
        struct pollfd* pollFdElem;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            ret = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if(ret == RV_OK && selectEngine->maxFdInPoll == selectEngine->fdArraySize) {
            ret = ExpandFdArrays(selectEngine, selectEngine->fdArraySize + 1);
        }

        if (ret == RV_OK)
        {
            /* Find the entry to fill in */
            pollFdElem = selectEngine->fdArray + selectEngine->maxFdInPoll;

            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = rvSelectToOS(selectEvents);

            /* Fill in the pollfd element */
            pollFdElem->fd = fd->fd;
            pollFdElem->events = 0;
            pollFdElem->revents = 0;
            if (translatedEvents & RV_SELECT_READ)
                pollFdElem->events |= POLLIN;
            if (translatedEvents & RV_SELECT_WRITE)
                pollFdElem->events |= POLLOUT;

            /* Make sure to link the fd to this one */
            fd->fdArrayIndex = selectEngine->maxFdInPoll;

            /* Update the array size for poll() */
            selectEngine->maxFdInPoll++;
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    if (ret == RV_OK)
    {
        RvSelectEvents translatedEvents;
        struct pollfd fdUpdate;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            ret = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if(ret == RV_OK && selectEngine->maxFdInPoll == selectEngine->fdArraySize) {
            ret = ExpandFdArrays(selectEngine, selectEngine->fdArraySize + 1);
        }


        if (ret == RV_OK)
        {
            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = rvSelectToOS(selectEvents);

            /* Fill in the pollfd element */
            fdUpdate.fd = fd->fd;
            fdUpdate.events = 0;
            fdUpdate.revents = 0;
            if (translatedEvents & RV_SELECT_READ)
                fdUpdate.events |= POLLIN;
            if (translatedEvents & RV_SELECT_WRITE)
                fdUpdate.events |= POLLOUT;

            /* in Solaris you can not write new data to /dev/poll unless the
               WaitAndBlock is out of the ioctl;  because of this we have to
               preempt the blocked ioctl first */
            ret = rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);

            /* Now wait until the WaitAndBlock function releases the semaphore
               to indicate that the function is not blocked on the ioctl() anymore.
               Note that WaitAndBlock will not be able to "steal" the semaphore
               because it must lock a mutex first */
            RvSemaphoreWait(&selectEngine->devPollWrite, selectEngine->logMgr);

            /* Write the change to /dev/poll */
            if (write(selectEngine->fdDevPoll, &fdUpdate, sizeof(fdUpdate)) == sizeof(fdUpdate))
            {
                fd->devpollEvents = fdUpdate.events; /* Know what we're waiting for */

                /* We've got one more fd in /dev/poll */
                selectEngine->maxFdInPoll++;
            }
            else
            {
                RvSelectLogError((&logMgr->selectSource,
                    "RvSelectAdd: Can't add fd=%d with %d", fd->fd, selectEvents));
                ret = RvSelectErrorCode(RV_ERROR_UNKNOWN);
            }

            RvSemaphorePost(&selectEngine->devPollWrite, selectEngine->logMgr);
        }
    }

#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
    if (ret == RV_OK)
    {
        RvSelectEvents translatedEvents;
        struct kevent keventFdElem;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            ret = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if (ret == RV_OK)
        {
            RvInt32 kqueueId = selectEngine->kqueueId;
#if (RV_SELECT_KQUEUE_GROUPS == RV_YES)
            if (fd->group == RV_SELECT_KQUEUE_GROUP_LOW)
                kqueueId = selectEngine->kqueueIdLow;
            else if (fd->group == RV_SELECT_KQUEUE_GROUP_HIGH)
                kqueueId = selectEngine->kqueueIdHigh;
#endif
            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = rvSelectToOS(selectEvents);
            /* Fill out the kevent struct */
            if (translatedEvents & RV_SELECT_READ)
            {
                memset(&keventFdElem, 0 , sizeof(keventFdElem));
                EV_SET(&keventFdElem, fd->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
                /* set the event */
                ret = kevent(kqueueId, &keventFdElem, 1, NULL, 0, NULL);
                if (ret == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectAdd(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
            if (translatedEvents & RV_SELECT_WRITE)
            {
                EV_SET(&keventFdElem, fd->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
                /* set the event */
                ret = kevent(kqueueId, &keventFdElem, 1, NULL, 0, NULL);
                if (ret == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectAdd(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
        }
    }
#endif

    if (ret == RV_OK)
    {
        /* Write down the events of this fd in its struct */
        fd->callback = eventsCb;
        fd->events = selectEvents;

#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL))
        ret = rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);
#endif
    }

    if ((ret != RV_OK) && allocatedBucketHash)
        fdBucketHashRemove(selectEngine, fd);

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectAdd(fd=%d)=%d", fd->fd, ret));

    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

    return ret;
}


/********************************************************************************************
 * RvSelectRemove - Remove a file descriptor that is being checked by this engine
 *
 * Should only be called for file descriptors that RvSelectAdd() was called for them.
 * Thread-safe functions, under assumptions specified in the header of the file.
 *
 * INPUT   : selectEngine   - Events engine of this fd
 *           fd             - File descriptor to remove
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectRemove(
    IN RvSelectEngine*      selectEngine,
    IN RvSelectFd*          fd)
{
    RvStatus status = RV_OK;
    RvLogMgr *logMgr;
    RvBool selectLockReleased = RV_FALSE;

#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
        RvSelectEvents translatedEvents;

        /* Find out what was the requested OS-event for this fd */
        translatedEvents = rvSelectToOS(fd->events);
#endif
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((selectEngine == NULL) || (fd == NULL))
        return RvSelectErrorCode(RV_ERROR_BADPARAM);
#endif

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);

    logMgr = fd->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectRemove(fd=%d)", fd->fd));

/*
    if (fd->events != (RvSelectEvents)0)
*/
    {
        /* Remove the events from the bucket hash */
        fd->events = (RvSelectEvents)0;
        status = fdBucketHashRemove(selectEngine, fd);
    }

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    if (status == RV_OK)
    {
        /* Make sure the network sends no events on this fd */
        if (WSAAsyncSelect(fd->fd, selectEngine->hNetEventsWnd, 0, 0) != 0) {
            status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)
    if (status == RV_OK)
    {
#if (RV_OS_TYPE != RV_OS_TYPE_WINCE) && (RV_OS_TYPE != RV_OS_TYPE_WIN32) && (RV_CHECK_MASK & RV_CHECK_RANGE)
        /* Windows CE doesn't really care that the socket's fd value is within
           the range of FD_SETSIZE. This is because the fd_set struct is actually
           defined more like poll - a list of fd's to select and not a bits buffer.
           If we'll range check for WinCE we'll just get errors after several socket
           constructions. */
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif

        if (status == RV_OK)
        {
#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
            /* Update hash-table on the fd that is deleted from the select-engine */
            Mmb_RvSelectRemove((void*)selectEngine, fd->fd);
#endif
            /* Remove this client */
            RV_FD_CLR(fd->fd, &selectEngine->rdSet);
            RV_FD_CLR(fd->fd, &selectEngine->wrSet);

            /* Update the maximum limit of select() if we have to */
            if (fd->fd == (RvSocket)selectEngine->maxFdInSelect)
            {
                /* find the biggest OS descriptor set in the select engine */
                RvSocket maxFd = 0;
                RvSelectFd* currFd;
                RvSelectBucket* currBucket;

                /* go over all entries within the hash and find non-empty*/
                currBucket = selectEngine->firstBucket;
                while (currBucket)
                {
                    /* iterate on the linked list of non-empty FD's and find the biggest*/
                    currFd = currBucket->selectFD;
                    while (currFd)
                    {
                        if (currFd->fd > maxFd)
                            maxFd = currFd->fd;
                        currFd = currFd->nextInBucket;
                    }
                    currBucket = currBucket->nextBucket;
                }
                selectEngine->maxFdInSelect = maxFd;
            }
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_POLL)
    if (status == RV_OK)
    {
#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if (status == RV_OK)
        {
            selectEngine->maxFdInPoll--; /* We have one less */

            /* Remove this client from fdArray - we don't want holes in our array, so we'll
               just move the last one to fill in the gap */
            if (selectEngine->maxFdInPoll != (RvUint32)fd->fdArrayIndex)
            {
                RvSelectFd* movedFd;

                /* It's not the last - fill it in */
                selectEngine->fdArray[fd->fdArrayIndex] = selectEngine->fdArray[selectEngine->maxFdInPoll];

                /* We'll also need to move the fd itself... */
                movedFd = fdBucketHashFind(selectEngine, selectEngine->fdArray[fd->fdArrayIndex].fd, NULL, NULL);
                if (movedFd != NULL)
                {
                    movedFd->fdArrayIndex = fd->fdArrayIndex;
                }
                /* todo: handle errors */
            }

        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    if (status == RV_OK)
    {
        struct pollfd fdRemove;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if (status == RV_OK)
        {
            fdRemove.fd = fd->fd;
            fdRemove.events = POLLREMOVE;
            fdRemove.revents = 0;

            /* in Solaris you can not write new data to /dev/poll unless the
               WaitAndBlock is out of the ioctl;  because of this we have to
               preempt the blocked ioctl first */
            status = rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);

            /* Now wait until the WaitAndBlock function releases the semaphore
               to indicate that the function is not blocked on the ioctl() anymore.
               Note that WaitAndBlock will not be able to "steal" the semaphore
               because it must lock a mutex first */
            RvSemaphoreWait(&selectEngine->devPollWrite, selectEngine->logMgr);

            /* Write the change to /dev/poll */
            if (write(selectEngine->fdDevPoll, &fdRemove, sizeof(fdRemove)) == sizeof(fdRemove))
            {
                /* We've got one less fd in /dev/poll */
                selectEngine->maxFdInPoll--;
            }
            else
            {
                RvSelectLogError((&logMgr->selectSource,
                    "RvSelectRemove: Can't remove fd=%d", fd->fd));
                status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
            }

            RvSemaphorePost(&selectEngine->devPollWrite, selectEngine->logMgr);
        }
    }
#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
    if (status == RV_OK)
    {
        struct kevent keventFdElem;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */
        if (status == RV_OK)
        {
            RvInt32 kqueueId = selectEngine->kqueueId;
#if (RV_SELECT_KQUEUE_GROUPS == RV_YES)
            if (fd->group == RV_SELECT_KQUEUE_GROUP_LOW)
                kqueueId = selectEngine->kqueueIdLow;
            else if (fd->group == RV_SELECT_KQUEUE_GROUP_HIGH)
                kqueueId = selectEngine->kqueueIdHigh;
#endif
            /* Remove the kevent struct */
            if (translatedEvents & RV_SELECT_READ)
            {
                EV_SET(&keventFdElem, fd->fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
                /* set the event */
                status = kevent(kqueueId, &keventFdElem, 1, NULL, 0, NULL);
                if (status == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectRemove(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
            if (translatedEvents & RV_SELECT_WRITE)
            {
                EV_SET(&keventFdElem, fd->fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
                /* set the event */
                status = kevent(kqueueId, &keventFdElem, 1, NULL, 0, NULL);
                if (status == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectRemove(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
        }
    }

#endif

#if RV_SELECT_SYNC_REMOVAL
    rvSelectRemoveWait(selectEngine);
    selectLockReleased = RV_TRUE;
#elif ((RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL))
    status = rvFdPreempt(selectEngine,RvSelectEmptyPreemptMsg);
#endif


    if(!selectLockReleased) {
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
    }

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectRemove(fd=%d)=%d", fd->fd, status));

    return status;
}


/********************************************************************************************
 * RvSelectUpdate - Update the events of callback used for a given file descriptor
 * Should only be called for file descriptors that RvSelectAdd() was called for them.
 * Thread-safe functions, under assumptions specified in the header of the file.
 * INPUT   : selectEngine   - Events engine of this fd
 *           fd             - File descriptor to update
 *           selectEvents   - Events to check
 *           eventsCb       - Callback to use when these events occur
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectUpdate(
    IN RvSelectEngine*      selectEngine,
    IN RvSelectFd*          fd,
    IN RvSelectEvents       selectEvents,
    IN RvSelectCb           eventsCb)
{
    RvStatus status = RV_OK;
    RvLogMgr *logMgr;
#if (RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL) || \
    (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    RvBool   anyChanges = RV_FALSE;
#endif

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((selectEngine == NULL) || (fd == NULL))
        return RvSelectErrorCode(RV_ERROR_BADPARAM);
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
    /* Since RvSocketSetSelectEngine() is going to protect its data structures
     * using a lock - we better call this function before we lock the select engine
     * in order to prevent deadlocks
     */
    RvSocketSetSelectEngine(RvFdGetSocket(fd), (void*)selectEngine);
#endif

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);

    logMgr = selectEngine->logMgr;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectUpdate(fd=%d,events=%s%s%s%s%s,cb=%p)", fd->fd,
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectRead)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectWrite)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectAccept)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectConnect)),
        fdGetEventStr((RvSelectEvents)(selectEvents & RvSelectClose)),
        eventsCb));

    if (fd->closedByTcpPeer == RV_TRUE) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectUpdate failure, FD was closed by the TCP peer"));
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }

    if (fdBucketHashFind(selectEngine, fd->fd, NULL, NULL) == NULL) {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectUpdate failure: fd was not previously registered"));
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    {
        if (WSAAsyncSelect(fd->fd, selectEngine->hNetEventsWnd, FD_NET_MESSAGE, selectEvents) == 0)
            anyChanges = RV_TRUE;
        else
            status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }

#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)
    {
        RvSelectEvents translatedEvents;

#if (RV_OS_TYPE != RV_OS_TYPE_WINCE) && (RV_OS_TYPE != RV_OS_TYPE_WIN32)  && (RV_CHECK_MASK & RV_CHECK_RANGE)
        /* Windows CE doesn't really care that the socket's fd value is within
           the range of FD_SETSIZE. This is because the fd_set struct is actually
           defined more like poll - a list of fd's to select and not a bits buffer.
           If we'll range check for WinCE we'll just get errors after several socket
           constructions. */
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif

        if (status == RV_OK)
        {
            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = (RvSelectEvents)rvSelectToOS(selectEvents);
            anyChanges = RV_TRUE; /* Always think we have changes - simplify matters */

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
            if (selectEvents != RvSelectConnect)
#endif
            {            
            if (translatedEvents & RV_SELECT_READ)
                RV_FD_SET(fd->fd, &selectEngine->rdSet);
            else
                RV_FD_CLR(fd->fd, &selectEngine->rdSet);
            if (translatedEvents & RV_SELECT_WRITE)
                RV_FD_SET(fd->fd, &selectEngine->wrSet);
            else
                RV_FD_CLR(fd->fd, &selectEngine->wrSet);
            }
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_POLL)
    {
        RvSelectEvents translatedEvents;
        struct pollfd* pollFdElem;
        short newEvents;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if (status == RV_OK)
        {
            /* Find the entry to fill in */
            pollFdElem = selectEngine->fdArray + fd->fdArrayIndex;

#if (RV_CHECK_MASK & RV_CHECK_OTHER)
            /* Make sure we're updating the right fd */
            if (pollFdElem->fd != fd->fd)
            {
                RvSelectLogExcep((&logMgr->selectSource,
                    "RvSelectUpdate: Updating fd=%d with fd=%d.", pollFdElem->fd, fd->fd));
                status = RV_ERROR_BADPARAM;
            }
#endif

            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = rvSelectToOS(selectEvents);
            newEvents = 0;
            if (translatedEvents & RV_SELECT_READ)
                newEvents |= POLLIN;
            if (translatedEvents & RV_SELECT_WRITE)
                newEvents |= POLLOUT;

            if (newEvents != pollFdElem->events)
            {
                /* Fill in the pollfd element - it's updated now */
                pollFdElem->revents = 0;
                pollFdElem->events = newEvents;
                anyChanges = RV_TRUE;
            }
        }
    }


#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    if (status == RV_OK)
    {
        RvSelectEvents translatedEvents;
        struct pollfd fdUpdate[2];
        RvSize_t writeSize;
        int updateIndex;
        short newEvents;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if (status == RV_OK)
        {
            /* Since Unix systems support only Read/Write events, we have to translate
               the richer set of events we have */
            translatedEvents = rvSelectToOS(selectEvents);
            newEvents = 0;
            if (translatedEvents & RV_SELECT_READ)
                newEvents |= POLLIN;
            if (translatedEvents & RV_SELECT_WRITE)
                newEvents |= POLLOUT;

            /* See if we need any changes to make. We do it here so we won't have to call write()
               if we don't really have to - this reduces time in the kernel. */
            if (newEvents != fd->devpollEvents)
            {
                /* Fill in the pollfd element */
#if (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || (RV_OS_TYPE == RV_OS_TYPE_HPUX)
                /* On Solaris, we should remove existing mask and put the new one,
                   since adding the new one as is will only update the existing mask. */
                updateIndex = 1;
                writeSize = sizeof(struct pollfd) * 2;
                fdUpdate[0].fd = fd->fd;
                fdUpdate[0].events = POLLREMOVE;
                fdUpdate[0].revents = 0;
#else
                updateIndex = 0;
                writeSize = sizeof(struct pollfd);
#endif
                fdUpdate[updateIndex].fd = fd->fd;
                fdUpdate[updateIndex].revents = 0;
                fdUpdate[updateIndex].events = newEvents;

                /* in Solaris you can not write new data to /dev/poll unless the
                   WaitAndBlock is out of the ioctl;  because of this we have to
                   preempt the blocked ioctl first */
                status = rvFdPreempt(selectEngine,RvSelectEmptyPreemptMsg);

                /* Now wait until the WaitAndBlock function releases the semaphore
                   to indicate that the function is not blocked on the ioctl() anymore.
                   Note that WaitAndBlock will not be able to "steal" the semaphore
                   because it must lock a mutex first */
                RvSemaphoreWait(&selectEngine->devPollWrite, selectEngine->logMgr);

                /* Write the change to /dev/poll */
                if (write(selectEngine->fdDevPoll, fdUpdate, writeSize) == (int)writeSize)
                {
                    /* Succeeded - update what we're waiting for */
                    fd->devpollEvents = newEvents;
                }
                else
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectUpdate: Can't update fd=%d with %d", fd->fd, selectEvents));
                    status = RvSelectErrorCode(RV_ERROR_UNKNOWN);
                }

                RvSemaphorePost(&selectEngine->devPollWrite, selectEngine->logMgr);
            }
        }
    }
#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
    if (status == RV_OK)
    {
        RvSelectEvents translatedEvents;
        struct kevent  keventFdElem[2];

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
        if ((fd->fd < 0) || (fd->fd >= (RvSocket)selectEngine->maxFd))
            status = RvSelectErrorCode(RV_ERROR_OUTOFRANGE);
#endif  /* RV_RANGECHANGE */

        if (status == RV_OK)
        {
            RvInt32 kqueueId = selectEngine->kqueueId;
#if (RV_SELECT_KQUEUE_GROUPS == RV_YES)
            if (fd->group == RV_SELECT_KQUEUE_GROUP_LOW)
                kqueueId = selectEngine->kqueueIdLow;
            else if (fd->group == RV_SELECT_KQUEUE_GROUP_HIGH)
                kqueueId = selectEngine->kqueueIdHigh;
#endif
            /* we have to remove the old kevent from the queue, and put in the new one.
               since a kevent is identified by the (ident, filter) pair, we first have to
               find out what was the old event for this fd */

            /* Find out what was the requested OS-event for this fd */
            translatedEvents = rvSelectToOS(fd->events);
            /* Remove the kevent struct */
            if (translatedEvents & RV_SELECT_READ)
            {
                EV_SET(&keventFdElem[0], fd->fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
                /* set the events */
                status = kevent(kqueueId, &keventFdElem[0], 1, NULL, 0, NULL);
                if (status == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectUpdate(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
            if (translatedEvents & RV_SELECT_WRITE)
            {
                EV_SET(&keventFdElem[0], fd->fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
                /* set the events */
                status = kevent(kqueueId, &keventFdElem[0], 1, NULL, 0, NULL);
                if (status == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectUpdate(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
            translatedEvents = rvSelectToOS(selectEvents);
            /* Fill out the kevent struct */
            if (translatedEvents & RV_SELECT_READ)
            {
                EV_SET(&keventFdElem[1], fd->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
                /* set the events */
                status = kevent(kqueueId, &keventFdElem[1], 1, NULL, 0, NULL);
                if (status == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectUpdate(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
            if (translatedEvents & RV_SELECT_WRITE)
            {
                EV_SET(&keventFdElem[1], fd->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
                /* set the events */
                status = kevent(kqueueId, &keventFdElem[1], 1, NULL, 0, NULL);
                if (status == -1)
                {
                    RvSelectLogError((&logMgr->selectSource,
                        "RvSelectUpdate(fd=%d), error %d - %s", fd->fd, errno, strerror(errno)));
                }
            }
        }
    }

#endif

    if (status == RV_OK)
    {
        /* Write down the events of this fd in its struct */
        fd->callback = eventsCb;
        fd->events = selectEvents;

#if ((RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL))
        if (anyChanges)
            status = rvFdPreempt(selectEngine,RvSelectEmptyPreemptMsg);
#endif
    }

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectUpdate(fd=%d)=%d", fd->fd, status));

    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

    return status;
}


/********************************************************************************************
 * RvSelectWaitAndBlock
 *
 * Wait for events to occur on this engine and block the current running thread
 * for a given amount of time, or until events occur.
 * This function must be called from the thread that called RvSelectConstruct().
 * The timeout in this function is used by some of the stacks to implement their
 * timers mechanism.
 * Thread-safe functions, under assumptions specified in the header of the file.
 *
 * INPUT   : selectEngine   - Events engine to wait for
 *           nsecTimeout    - Timeout to wait in nanoseconds
 *                            A 0-value will not block - just check for events
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectWaitAndBlock(
    IN RvSelectEngine*      selectEngine,
    IN RvUint64             nsecTimeout)
{
    RvStatus result = RV_OK;
    RvChar   timestr[RV_64TOASCII_BUFSIZE];
    RvLogMgr *logMgr;
    RV_USE_CCORE_GLOBALS;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL)
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
#endif

    logMgr = selectEngine->logMgr;

    if (logMgr != NULL) {
        Rv64toA(nsecTimeout, timestr);
        RvSelectLogEnter((&logMgr->selectSource,
            "RvSelectWaitAndBlock(eng=%p,time_out=%s)",
            selectEngine,timestr));
    }

    if (RvUint64IsLessThan(nsecTimeout, rvSelectMinTimeoutResolution))
        nsecTimeout = rvSelectMinTimeoutResolution;

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    {
        MSG msg;
        DWORD winRes;
        DWORD timeout;
        RvUint64 msecTimeout;
        RvUint64 remainingTime;
        RvSize_t numevents;

        /* Recalculate timeout in milliseconds, making sure we don't overflow */
        if (RvUint64IsGreaterThanOrEqual(nsecTimeout, RV_INT64_MAX))
            timeout = INFINITE;
        else
        {
            msecTimeout = RvUint64Div(RvUint64Sub(RvUint64Add(nsecTimeout, RV_TIME64_NSECPERMSEC),
                                                  RvUint64Const(0,1)),
                                      RV_TIME64_NSECPERMSEC);
            if (RvUint64IsGreaterThan(msecTimeout, RvUint64FromRvInt32(RV_INT32_MAX)))
                timeout = INFINITE;
            else
                timeout = (DWORD)RvUint64ToRvUint32(msecTimeout);
        }

        /* Find out the remaining time we have */
        if (RvTimerQueueNextEvent(&selectEngine->tqueue, (RvInt64 *)&remainingTime) == RV_OK) {
            /* Make sure that remaining time is not negative */
            if (RvInt64IsLessThan(RvInt64FromRvUint64(remainingTime), RvInt64Const(0,0,0)))
                remainingTime = RvUint64Const(0,0);
            /* Set timeout */
            if (RvUint64IsLessThan(remainingTime, nsecTimeout)) {
                msecTimeout = RvUint64Div(RvUint64Sub(RvUint64Add(remainingTime, RV_TIME64_NSECPERMSEC),
                                                      RvUint64Const(0,1)),
                                          RV_TIME64_NSECPERMSEC);
                if (RvUint64IsGreaterThan(msecTimeout, RvUint64Const(0,RV_INT32_MAX)))
                    timeout = INFINITE;
                else
                    timeout = (DWORD)RvUint64ToRvUint32(msecTimeout);
            }
        }

        /* We wait for messages on this thread, to a given amount of milliseconds */
        winRes = MsgWaitForMultipleObjects(1, &selectEngine->threadHandle, FALSE,
            timeout, QS_ALLEVENTS|QS_ALLINPUT|QS_ALLPOSTMESSAGE|QS_SENDMESSAGE);
        if (winRes == WAIT_FAILED)
        {
            RvSelectLogExcep((&logMgr->selectSource,
                "RvSelectWaitAndBlock: Got error %d", GetLastError()));
            return RvSelectErrorCode(RV_ERROR_UNKNOWN);
        }

        if (winRes == WAIT_OBJECT_0 + 1)
        {
            /* Dispatch all messages that we can find for this thread before we exit this function */
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
                DispatchMessage(&msg);
        }

        RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                            selectEngine->timeOutCb, selectEngine->timeOutCbContext);
    }


#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)


#elif (RV_SELECT_TYPE == RV_SELECT_SELECT)
    {
        int numResults, numFds;
        struct timeval  timeout;
        struct timeval  *tm;
        fd_set          tmpWrSet;
        fd_set          tmpRdSet;
        RvUint64        nsecSelectTimeout;

        RV_FD_ZERO(&tmpRdSet);
        RV_FD_ZERO(&tmpWrSet);

        RvLockGet(&selectEngine->lock, selectEngine->logMgr);
        selectEngine->waiting = RV_TRUE;
        selectEngine->timestamp = RvUint64Incr(selectEngine->timestamp);

        /* wait for minimum of nsecTimeout and selectEngine->nsecTimeout */
        if (RvUint64IsNotEqual(selectEngine->nsecTimeout, RvUint64Const(0, 0)))
            nsecSelectTimeout = RvUint64Min(nsecTimeout, selectEngine->nsecTimeout);
        else
            nsecSelectTimeout = nsecTimeout;

        /* Set the timeout from this blocking function */
        if (RvUint64IsGreaterThanOrEqual(nsecSelectTimeout, RV_UINT64_MAX))
        {
            /* Blocking mode - no timeout to wait for */
            tm = NULL;
            timeout.tv_sec = -1; /* Used for logging */
            timeout.tv_usec = 0; /* Used for logging */
        }
        else if (RvUint64IsGreaterThan(nsecSelectTimeout, RvUint64Mul(RvUint64FromRvUint32(RV_UINT32_MAX), RvUint64FromRvInt64(RV_TIME64_NSECPERSEC))))
        {
            /* Time to wait is too log. Try reducing it to something logical */
            timeout.tv_sec = (long)RV_MAX_SELECT_SECTIMEOUT;
            timeout.tv_usec = 0;
            tm = &timeout;
        }
        else
        {
            /* Make sure the timeout we give isn't too small */
            if (RvUint64IsLessThan(nsecSelectTimeout, rvSelectMinTimeoutResolution))
                nsecSelectTimeout = rvSelectMinTimeoutResolution;

            /* Calculate the timeout for real */
            timeout.tv_sec = RvInt64ToRvInt32(RvInt64Div(nsecSelectTimeout, RV_TIME64_NSECPERSEC));
            timeout.tv_usec = (long)(RvInt64ToRvInt32(RvInt64Mod(nsecSelectTimeout, RV_TIME64_NSECPERSEC)) / RV_TIME_NSECPERUSEC);
            tm = &timeout;
        }

        /* Reset the file descriptors set */
        RvSelectGetSelectFds(selectEngine, &numFds, &tmpRdSet, &tmpWrSet);

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
        {
        /* == TODO: impermanent solution ==
           In nucleus OS, the following behaviour occurs:
           nu_select() only pends for read events, and the other 2 FD buffers (write and
           exception) are ignored. so marking a socket for being 'writable' is meaningless -
           the event will never occur.
           However, since we mark our file-descriptors for write event, when we enter
           select, they will be 'writable' when we exit select (the buffers are kept "as is").

           The problem starts, when no read events are received - than you will never get
           that write event...
           To avoid this we do constant preemption, when write event is pended for.

           possible fallbacks to this solution: the 'selecting' task could constantly use
           the CPU.
           A more clever solution can be - use an additional task, that on timed-occasion
           that select indeed suspends, it will preempt it. */

            RvInt32 compareBuffer[sizeof(fd_set)] = {0};

            /* check if write event was requested */
            if ( memcmp(tmpWrSet.words, compareBuffer, sizeof(fd_set)) )
            {
                selectEngine->needToPreempt = RV_TRUE;
                rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);
            }
        }
#endif

        /* we should know in which thread select was used. That to avoid
        unnecessary select preemption */
        selectEngine->selectThread = RvThreadCurrent();
        selectEngine->suspectTimeout = RvUint64IsNotEqual(selectEngine->nsecTimeout, RvUint64Const(0, 0));
        selectEngine->nsecTimeout = RvUint64Const(0, 0);
        result = RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

        if (result != RV_OK)
        {
            RvSelectLogExcep((&logMgr->selectSource,
                "RvSelectWaitAndBlock: RvLockRelease error"));
            return result;
        }

        RvSelectLogDebug((&logMgr->selectSource,
            "RvSelectWaitAndBlock(select): engine=%p,numFds=%d,timeout=%d.%d sec",
            selectEngine, numFds, timeout.tv_sec, timeout.tv_usec));

        /* Run the select() function in blocking mode */
#if (RV_OS_TYPE == RV_OS_TYPE_PSOS) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)
        numResults = RvSelectPsosSelect(numFds, &tmpRdSet, &tmpWrSet, NULL, tm, logMgr);
#else
        numResults = select(numFds, &tmpRdSet, &tmpWrSet, NULL, tm);
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)
        /* we don't check the events here - MOPI's sock_select() is non-blocking. when event or
           timer expiration occurs, an ENIP_SOCK_SELECT event is dispatched to the embedded
           process, and the actual events will be processed after the second
           (no timeout this time-)select() inside the event-process-callback */
        return RV_OK;
#endif

        /* the following code section is for non-MOPI OSs */
        /*lint --e{527}*/
        RvLockGet(&selectEngine->lock,selectEngine->logMgr);

        selectEngine->waiting = RV_FALSE;
        rvSelectRemoveNotify(selectEngine);

        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

        RvSelectLogDebug((&logMgr->selectSource,
            "RvSelectWaitAndBlock: numResults=%d", numResults));

/*
 *	Note: It is important that RvTimerQueueService is called before 
 *        calling socket callbacks.
 */

        /* Check if select() was, potentially, 'awaken' because of a timer */
        if(selectEngine->suspectTimeout)
        {
            RvSize_t numevents;

            /* We must not change this value after call to any callback */
            selectEngine->wakeupTime = RvUint64Const(0, 0);

            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);
        }

        /* Handle the result from the select() call */
        result = RvSelectHandleSelectFds(selectEngine,
            &tmpRdSet, &tmpWrSet, numFds, numResults);

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
        /* RvSelectHandleSelectFds() will probably call user callback, which 
         * in turn may call other select functions.  So we must release all
         * locks before calling this function.
         */
        RvLockGet(&selectEngine->lock ,selectEngine->logMgr);
        
        tmpRdSet   = selectEngine->simulatedRdSet;
        tmpWrSet   = selectEngine->simulatedWrSet;
        numFds     = selectEngine->simulatedMaxFd;
        numResults = selectEngine->simulatedNumFd;

        /* Handle simulated events from rvsocket.c, and then clean them up. */
        RV_FD_ZERO(&selectEngine->simulatedRdSet);
        RV_FD_ZERO(&selectEngine->simulatedWrSet);
        selectEngine->simulatedNumFd = 0;
        selectEngine->simulatedMaxFd = 0;

        RvLockRelease(&selectEngine->lock ,selectEngine->logMgr);

        /* Handle simulated events from rvsocket.c */
        result += RvSelectHandleSelectFds(selectEngine,
            &tmpRdSet, &tmpWrSet, numFds+1, numResults);
#endif
    }


#elif (RV_SELECT_TYPE == RV_SELECT_POLL)
    {
        int         numResults;
        RvInt32     msecTimeout;
        RvUint32    maxFdInPoll;
        RvUint64    nsecSelectTimeout;

        RvLockGet(&selectEngine->lock, selectEngine->logMgr);
        selectEngine->waiting = RV_TRUE;
        selectEngine->timestamp = RvUint64Incr(selectEngine->timestamp);

        /* wait for minimum of nsecTimeout and selectEngine->nsecTimeout */
        if (selectEngine->nsecTimeout != 0)
            nsecSelectTimeout = (nsecTimeout >= selectEngine->nsecTimeout)?selectEngine->nsecTimeout:nsecTimeout;
        else
            nsecSelectTimeout = nsecTimeout;

        /* Set the timeout from this blocking function */
        if (nsecSelectTimeout >= RV_INT64_MAX)
        {
            /* Blocking mode - no timeout to wait for */
            msecTimeout = -1;
        }
        else if (nsecSelectTimeout > ((RvInt64)RV_INT32_MAX) * RV_TIME64_NSECPERMSEC)
            msecTimeout = RV_INT32_MAX;
        else
            msecTimeout = (RvInt32)RvUint64Div(nsecSelectTimeout+RV_TIME64_NSECPERMSEC-1, RV_TIME64_NSECPERMSEC);

        RvSelectLogDebug((&logMgr->selectSource,
            "RvSelectWaitAndBlock(poll): engine=%p,numFds=%d,timeout=%d msec",
            selectEngine, selectEngine->maxFdInPoll, msecTimeout));

        RvSelectGetPollFds(selectEngine, &selectEngine->maxFdInPoll, selectEngine->tmpFdArray);
        maxFdInPoll = selectEngine->maxFdInPoll;

        /* we should know in which thread select was used. That to avoid
        unnecessary select preemption */
        selectEngine->selectThread = RvThreadCurrent();
        selectEngine->suspectTimeout = selectEngine->nsecTimeout != 0;
        selectEngine->nsecTimeout = 0;

        RvLockRelease(&selectEngine->lock, selectEngine->logMgr);

        /* poll for events */
        numResults = poll(selectEngine->tmpFdArray, maxFdInPoll, msecTimeout);

        RvLockGet(&selectEngine->lock,selectEngine->logMgr);

        selectEngine->waiting = RV_FALSE;
        rvSelectRemoveNotify(selectEngine);


        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

/*
 *	Note: It is important that RvTimerQueueService is called before 
 *        calling socket callbacks.
 */

        /* Check if select() was 'awaken' because of a timer */
        if (selectEngine->suspectTimeout)
        {
            RvSize_t numevents;

            selectEngine->wakeupTime = 0;

            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);
        }

        /* handle any network events that occured */
        result = RvSelectHandlePollFds(selectEngine, selectEngine->tmpFdArray,
            maxFdInPoll, numResults);

        RvSelectLogLeave((&logMgr->selectSource,
            "RvSelectWaitAndBlock: numResults=%d", numResults));

    }


#elif (RV_SELECT_TYPE == RV_SELECT_DEVPOLL)
    {
        struct dvpoll   dopoll;
        int             numResults;
        int             msecTimeout;
        RvUint32        maxFdInPoll;
        RvUint64        nsecSelectTimeout;

        RvLockGet(&selectEngine->lock,selectEngine->logMgr);

        selectEngine->waiting = RV_TRUE;
        selectEngine->timestamp = RvUint64Incr(selectEngine->timestamp);

        /* wait for minimum of nsecTimeout and selectEngine->nsecTimeout */
        if (selectEngine->nsecTimeout != 0)
            nsecSelectTimeout = (nsecTimeout >= selectEngine->nsecTimeout)?selectEngine->nsecTimeout:nsecTimeout;
        else
            nsecSelectTimeout = nsecTimeout;


        /* Set the timeout from this blocking function */
        if (nsecSelectTimeout >= RV_INT64_MAX)
        {
            /* Blocking mode - no timeout to wait for */
            msecTimeout = -1;
        }
        else if (nsecSelectTimeout > ((RvInt64)RV_INT32_MAX) * RV_TIME64_NSECPERMSEC)
            msecTimeout = (int)RV_INT32_MAX;
        else
            msecTimeout = (int)RvUint64Div(nsecSelectTimeout+RV_TIME64_NSECPERMSEC-1, RV_TIME64_NSECPERMSEC);

        RvSelectGetPollFds(selectEngine, &selectEngine->maxFdInPoll, selectEngine->tmpFdArray);

        /* Create polling command for /dev/poll */
        dopoll.dp_timeout = msecTimeout;
        dopoll.dp_nfds = selectEngine->maxFdInPoll;
        dopoll.dp_fds = selectEngine->tmpFdArray;

        maxFdInPoll = selectEngine->maxFdInPoll;

        RvSelectLogDebug((&logMgr->selectSource,
            "RvSelectWaitAndBlock(/dev/poll): engine=%p,numFds=%d,timeout=%d msec",
            selectEngine, selectEngine->maxFdInPoll, msecTimeout));

        /* we should know in which thread select was used. That to avoid
        unnecessary select preemption */
        selectEngine->selectThread = RvThreadCurrent();
        selectEngine->suspectTimeout = selectEngine->nsecTimeout != 0;
        selectEngine->nsecTimeout = 0;
        
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

        RvSemaphoreWait(&selectEngine->devPollWrite, selectEngine->logMgr);

        /* Wait for I/O events that we're interested in */
        numResults = ioctl(selectEngine->fdDevPoll, DP_POLL, &dopoll);

        RvSemaphorePost(&selectEngine->devPollWrite, selectEngine->logMgr);

        RvLockGet(&selectEngine->lock,selectEngine->logMgr);

        selectEngine->waiting = RV_FALSE;

        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

        RvSelectLogDebug((&logMgr->selectSource,
            "RvSelectWaitAndBlock: numResults=%d", numResults));

/*
 *	Note: It is important that RvTimerQueueService is called before 
 *        calling socket callbacks.
 */

        /* Check if select() was 'awaken' because of a timer */
        if (selectEngine->suspectTimeout)
        {
            RvSize_t numevents;

            selectEngine->wakeupTime = 0;
            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);
        }

        /* handle any network events that occured */
        result = RvSelectHandlePollFds(selectEngine, selectEngine->tmpFdArray,
            maxFdInPoll, numResults);
    }
#elif (RV_SELECT_TYPE == RV_SELECT_KQUEUE)
    {
        int         numResults;
        struct timespec msecTimeout = {0, 0}, *PmsecTimeout;
        RvUint64    nsecSelectTimeout;

        RvLockGet(&selectEngine->lock, selectEngine->logMgr);
        selectEngine->waiting = RV_TRUE;
        selectEngine->timestamp = RvUint64Incr(selectEngine->timestamp);


        /* wait for minimum of nsecTimeout and selectEngine->nsecTimeout */
        if (selectEngine->nsecTimeout != 0)
            nsecSelectTimeout = (nsecTimeout >= selectEngine->nsecTimeout)?selectEngine->nsecTimeout:nsecTimeout;
        else
            nsecSelectTimeout = nsecTimeout;

        /* Set the timeout from this blocking function */
        if (nsecSelectTimeout >= RV_INT64_MAX)
        {
            /* Blocking mode - no timeout to wait for */
            PmsecTimeout = NULL;
        }
        else
        {
            msecTimeout.tv_sec = (unsigned int) RvUint64Div(nsecSelectTimeout, RV_TIME64_NSECPERSEC); 
            msecTimeout.tv_nsec = RvUint64Sub(nsecSelectTimeout,
                            RvUint64Mul(RvInt64FromRvInt(msecTimeout.tv_sec),RV_TIME64_NSECPERSEC));
            PmsecTimeout = &msecTimeout;
        }
/*
        else if (nsecSelectTimeout > ((RvInt64)RV_INT32_MAX) * RV_TIME64_NSECPERMSEC)
        {        
            msecTimeout.tv_sec = RV_INT32_MAX;
            PmsecTimeout = &msecTimeout;
        }
        else
        {
            msecTimeout.tv_nsec = (RvInt32)RvUint64Div(nsecSelectTimeout+RV_TIME64_NSECPERMSEC-1, RV_TIME64_NSECPERMSEC);
            PmsecTimeout = &msecTimeout;
        }
*/

        RvSelectLogDebug((&logMgr->selectSource,
            "RvSelectWaitAndBlock(kqueue): engine=%p,timeout=%d msec", selectEngine, msecTimeout));

        /* we should know in which thread select was used. That to avoid
        unnecessary select preemption */
        selectEngine->selectThread = RvThreadCurrent();

        RvLockRelease(&selectEngine->lock, selectEngine->logMgr);

        /* poll for events */
        numResults = kevent(selectEngine->kqueueId, NULL, 0, selectEngine->fdArray,
                            selectEngine->nFdArray, PmsecTimeout);

        RvLockGet(&selectEngine->lock,selectEngine->logMgr);

        selectEngine->waiting = RV_FALSE;

        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

        /* Check if select() was 'awaken' because of a timer */
        if (selectEngine->nsecTimeout != 0)
        {
            RvSize_t numevents;

            selectEngine->nsecTimeout = 0;
            selectEngine->wakeupTime = 0;

            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);
        }

        /* handle any network events that occurred */
        result = RvSelectHandleKqueueFds(selectEngine, selectEngine->fdArray,
                                         selectEngine->nFdArray, numResults);

        RvSelectLogLeave((&logMgr->selectSource,
            "RvSelectWaitAndBlock: numResults=%d", numResults));

    }


#elif (RV_SELECT_TYPE == RV_SELECT_SYMBIAN)
    {
        RvUint64        nsecSelectTimeout;

        RvLockGet(&selectEngine->lock, selectEngine->logMgr);
        selectEngine->waiting = RV_TRUE;
        selectEngine->timestamp = RvUint64Incr(selectEngine->timestamp);

        /* wait for minimum of nsecTimeout and selectEngine->nsecTimeout */
        if (RvUint64IsNotEqual(selectEngine->nsecTimeout, RvUint64Const(0, 0)))
            nsecSelectTimeout = RvUint64Min(nsecTimeout, selectEngine->nsecTimeout);
        else
            nsecSelectTimeout = nsecTimeout;

        /* we should know in which thread select was used. That to avoid
        unnecessary select preemption */
        selectEngine->selectThread = RvThreadCurrent();
        selectEngine->suspectTimeout = RvUint64IsNotEqual(selectEngine->nsecTimeout, RvUint64Const(0, 0));
        selectEngine->nsecTimeout = RvUint64Const(0, 0);
        result = RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        if (result != RV_OK)
        {
            RvSelectLogExcep((&logMgr->selectSource,
                "RvSelectWaitAndBlock: RvLockRelease error"));
            return result;
        }


    result = RvSymSelectWaitAndBlock(selectEngine,nsecTimeout);

        RvLockGet(&selectEngine->lock,selectEngine->logMgr);
        selectEngine->waiting = RV_FALSE;
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

/*
 *	Note: It is important that RvTimerQueueService is called before 
 *        calling socket callbacks.
 */

        /* Check if select() was, potentially, 'awaken' because of a timer */
        if(selectEngine->suspectTimeout)
        {
            RvSize_t numevents;

            /* We must not change this value after call to any callback */
            selectEngine->wakeupTime = RvUint64Const(0, 0);
            RvTimerQueueService(&selectEngine->tqueue, 0, &numevents,
                                selectEngine->timeOutCb, selectEngine->timeOutCbContext);
        }

        result = RvSymSelectCallCallbacks(selectEngine);
    }

#endif

	if (result == RV_OK)
	{
		RvSelectLogLeave((&logMgr->selectSource,
			"RvSelectWaitAndBlock(eng=%p,time_out=%s)=%d",
			selectEngine,timestr,result));
	}
	else
	{
		RvSelectLogError((&logMgr->selectSource,
			"RvSelectWaitAndBlock(eng=%p,time_out=%s,errno=%d)=%d",
			selectEngine,timestr,RvSelectErrNo,result));
	}

    return result;
}


/********************************************************************************************
 * RvSelectStopWaiting
 *
 * Stop waiting for events on the given events engine. This function is called
 * from threads other than the one that called RvSelectWaitAndBlock() in order
 * to preempt it.This function doesn't wait for the other thread to stop executing, it just
 * makes sure the other thread returns from the call to RvSelectWaitAndBlock().
 * This function doesn't need to be ported for applications that are single-threaded
 * on top of stacks that are single threaded.
 * Thread-safe functions, under assumptions specified in the header of the file.
 *
 * INPUT   : selectEngine   - Events engine to stop from blocking
 *           message        - message to be send. Can be used by the receiver
 *                            for stack specific messages
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectStopWaiting(
    IN RvSelectEngine       *selectEngine,
    IN RvUint8              message,
    IN RvLogMgr             *logMgr)
{
    RvStatus ret = RV_OK;

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectStopWaiting(eng=%p,message=%d)",
        selectEngine,(RvInt)message));

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA) && (RV_SELECT_PREEMPTION_TYPE != RV_SELECT_PREEMPTION_SMQ)
    /* Post an exiting message to this window */
    if (PostMessage(selectEngine->hNetEventsWnd, FD_EXIT_MESSAGE, message, 0) == 0) {
        ret = RvSelectErrorCode(RV_ERROR_UNKNOWN);
        RvSelectLogExcep((&logMgr->selectSource,
            "RvSelectStopWaiting(eng=%p,message=%d)=%d",
            selectEngine,(RvInt)message,ret));
        return ret;
    }
#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)


#elif ((RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL) || \
       (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || (RV_SELECT_TYPE == RV_SELECT_KQUEUE) || \
       (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SMQ))
    ret = rvFdPreempt(selectEngine,message);

#elif (RV_SELECT_TYPE == RV_SELECT_SYMBIAN)
    ret = RvSymSelectStopWaiting(selectEngine,message);
#endif

    RvSelectLogLeave((&logMgr->selectSource,
        "RvSelectStopWaiting(eng=%p,message=%d)=%d",
        selectEngine,(RvInt)message,ret));

    return ret;
}


/********************************************************************************************
 * RvSelectSetTimeOut
 *
 * Sets timeout for the select. This function used by the rvtimer module to set timeout
 * according to shortest user specified timeout.
 *
 * INPUT   : selectEngine   - Events engine to set timer on it
 *           nsecTimeout    - Timeout to wait in nanoseconds
 *                            A 0-value will not block - just check for events
 *           currentTime    - current time in nanoseconds
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RVCALLCONV RvSelectSetTimeOut(
    IN RvSelectEngine       *selectEngine,
    IN RvUint64             currentTime,
    IN RvUint64             nsecTimeout,
    IN RvLogMgr             *logMgr)
{
    RvStatus ret = RV_OK;
#define FUNC RvSelectSetTimeOut

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    RvUint64 msecTimeout;
    UINT timeout;
#endif

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectSetTimeOut(eng=%p,currentTime=<%d:%9.9d>, nsecTimeout=<%d:%9.9d>, logMgr=%p)",
        selectEngine, RvLogTimestamp(currentTime),  RvLogTimestamp(nsecTimeout),logMgr));

    RvLockGet(&selectEngine->lock,selectEngine->logMgr);
    
    if (RvUint64IsNotEqual(selectEngine->wakeupTime, RvUint64Const(0,0)) &&
        (RvUint64IsGreaterThan(RvUint64Add(currentTime, nsecTimeout), selectEngine->wakeupTime))) {
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        RvSelectLogLeave((&logMgr->selectSource,
            "RvSelectSetTimeOut(eng=%p,currentTime=<%d:%9.9d>, nsecTimeout=<%d:%9.9d>, logMgr=%p) No update",
            selectEngine, RvLogTimestamp(currentTime), RvLogTimestamp(nsecTimeout), logMgr));
        return RV_OK;
    }

    selectEngine->wakeupTime = RvUint64Add(currentTime, nsecTimeout);

#if (RV_SELECT_TYPE == RV_SELECT_WIN32_WSA)
    /* Recalculate timeout in milliseconds, making sure we don't overflow */
    if (RvUint64IsEqual(nsecTimeout, RV_INT64_MAX))
        timeout = INFINITE;
    else
    {
        msecTimeout = RvUint64Div(RvUint64Sub(RvUint64Add(nsecTimeout, RV_TIME64_NSECPERMSEC),
                                              RvUint64Const(0,1)),
                                  RV_TIME64_NSECPERMSEC);
        if (RvUint64IsGreaterThan(msecTimeout, RvUint64FromRvInt32(RV_INT32_MAX)))
            timeout = INFINITE;
        else
            timeout = (UINT)RvUint64ToRvUint(msecTimeout);
    }

    if (timeout == INFINITE) {
        KillTimer(selectEngine->hNetEventsWnd, 1 /* we have only one timer */);
    }
    else if (SetTimer(selectEngine->hNetEventsWnd, 1, timeout, NULL) == 0) {
        RvSelectLogError((&logMgr->selectSource, "SetTimer failure"));
        RvLockRelease(&selectEngine->lock,selectEngine->logMgr);
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }

    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

#elif (RV_SELECT_TYPE == RV_SELECT_WIN32_COMPLETION)
    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

#elif ((RV_SELECT_TYPE == RV_SELECT_SELECT) || (RV_SELECT_TYPE == RV_SELECT_POLL) || \
       (RV_SELECT_TYPE == RV_SELECT_DEVPOLL) || (RV_SELECT_TYPE == RV_SELECT_KQUEUE) || \
       (RV_SELECT_TYPE == RV_SELECT_SYMBIAN))

    selectEngine->nsecTimeout = nsecTimeout;
    
    ret = rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);
    RvLockRelease(&selectEngine->lock,selectEngine->logMgr);

#endif

    if (ret == RV_OK) {
        RvSelectLogLeave((&logMgr->selectSource,
            "RvSelectSetTimeOut(eng=%p,currentTime=<%d:%9.9d>, nsecTimeout=<%d:%9.9d>, logMgr=%p)",
            selectEngine, RvLogTimestamp(currentTime), RvLogTimestamp(nsecTimeout),logMgr));
    }
    else {
        RvSelectLogError((&logMgr->selectSource,
            "RvSelectSetTimeOut - General error"));
    }

    return ret;
#undef FUNC
}


/********************************************************************************************
 * RvSelectRemoveLogMgr
 *
 * Resets the pointer to the log manager if it owned by the caller.
 *
 * INPUT   : selectEngine   - Select engine
 *           logMgr         - log manager instance
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectRemoveLogMgr(
    IN RvSelectEngine       *selectEngine,
    IN RvLogMgr             *logMgr)
{
    RvStatus ret = RV_OK;

    if(logMgr == NULL)
    {
        return ret;
    }

    RvSelectLogEnter((&logMgr->selectSource,
        "RvSelectRemoveLogMgr(eng=%p,logMgr=%p)",selectEngine,logMgr));

    RvLockGet(&selectEngine->lock,logMgr);

    if(selectEngine->logMgr == logMgr)
    {
        RvThread *th = selectEngine->constructedThread;

        selectEngine->logMgr = NULL;
        RvLogSelectUsageDec(logMgr);
        RvTimerQueueSetLogMgr(&selectEngine->tqueue, NULL);
        /* Select engine created this thread itself, using this log manager,
         * so it's responsible for removing it from thread structure
         */
        if(th != 0) {
            th->logMgr = 0;
            th->threadSource = 0;
        }
    }
    

#if (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SOCKET) || \
    (RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_PIPE)
    /* Clear logMgr of preemptionFd */
    if(selectEngine->preemptionFd.logMgr == logMgr)
    {
        selectEngine->preemptionFd.logMgr = NULL;
    }
#endif
    
    /* At this moment, logMgr might be already destroyed, so logMgr shouldn't
     *  be used from now on
     */

    RvLockRelease(&selectEngine->lock, NULL);

    /*
     *RvSelectLogLeave((&logMgr->selectSource,
     *   "RvSelectRemoveLogMgr(eng=%p,logMgr=%p)",selectEngine,logMgr));
     */

    return ret;
}


#if (RV_OS_TYPE == RV_OS_TYPE_PSOS) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)


static RvInt RvSelectPsosSelect(RvInt numFds, fd_set *tmpRdSet, fd_set *tmpWrSet, 
                                fd_set *tmpExSet, struct timeval*tm, RvLogMgr* logMgr)
{
    RvInt i, numResults, ourwidth;
    RvSocket sockets[FD_SETSIZE];
    RvSocket oursockets[FD_SETSIZE];
    fd_set ourreadset, ourwriteset, exceptset;
    long result, events, eventflag;

    /* Find out what sockets we need. */
    for(i = 0; i < numFds; i++)
    {
        if((FD_ISSET(i, tmpRdSet) != 0) || (FD_ISSET(i, tmpWrSet) != 0))
            sockets[i] = i;
        else
            sockets[i] = -1;
    }

    /* Get shared sockets and map them in sockets array. */
    ourwidth=RvSocketSharerMultiShare(sockets, numFds, logMgr);

    /* Clear our versions of the data. */
    for(i = 0; i < ourwidth; i++)
        oursockets[i] = -1;

    FD_ZERO(&ourreadset);
    FD_ZERO(&ourwriteset);
    FD_ZERO(&exceptset);

    /* Translate from passed in socket lists to our socket lists. */
    for(i = 0; i < numFds; i++)
    {
        if(sockets[i] != -1)
            oursockets[sockets[i]] = i;

        if (FD_ISSET(i, tmpRdSet))
        {
            if (sockets[i]>=0)
                FD_SET(sockets[i], &ourreadset);
            else
                RvSelectLogError((&logMgr->selectSource,
                    "RvSelectPsosSelect: not setting in the read mask fd %d (its sharing failed)",i));
            
        }
        if (FD_ISSET(i, tmpWrSet))
        {
            if (sockets[i]>=0)
                FD_SET(sockets[i], &ourwriteset);
            else
                RvSelectLogError((&logMgr->selectSource,
                   "RvSelectPsosSelect: not setting in the write mask fd %d (its sharing failed)",i));            
        }
    }

    /* Now we can finally call select. */
    numResults = select(ourwidth, &ourreadset, &ourwriteset, &exceptset, tm);

    if(numResults >= 0)
    {
        FD_ZERO(tmpRdSet);
        FD_ZERO(tmpWrSet);

        for(i = 0; i < ourwidth; i++)
        {
            if(FD_ISSET(i, &ourreadset) != 0)
                FD_SET(oursockets[i], tmpRdSet);
            if(FD_ISSET(i, &ourwriteset) != 0)
                FD_SET(oursockets[i], tmpWrSet);
        }
    }

    /* Unshare all of those shared sockets. */
    for (i=0; i<numFds; i++)
    {
        if (sockets[i] >= 0)
            RvSocketSharerClose(&sockets[i]);
    }

    return numResults;
}
#endif


#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
/********************************************************************************************
 * RvSelectSimulateEvent
 * Simulates event on file descriptor by updating relevant file descriptors
 * set and writing to the SELI pipe.
 * Thread-safe functions, under assumptions specified in the header of the file.
 *
 * INPUT   : hSeli          - SELI application handle to use
 *           fdHandle       - File Descriptor on which event will be simulated
 *           event          - SELI event to be simulated
 * OUTPUT  : none
 * RETURN  : none
 */
void RvSelectSimulateEvent(
    IN RvSelectEngine*      selectEngine,
    IN RvSelectFd*          fd,
    IN RvSelectEvents       selectEvents)
{
    /* What we actually do here, is fool around with the fd_set structs we gave to the
       select() function and then make sure the select() will exit. This will cause a
       "simulated" event on the given socket, which is what we want to achieve. */

    RvLockGet(&selectEngine->lock, selectEngine->logMgr); /* we must protect the FD parameter */

    if (selectEvents & RV_SELECT_READ)
        RV_FD_SET(fd->fd, &selectEngine->simulatedRdSet);
    else
        RV_FD_CLR(fd->fd, &selectEngine->simulatedRdSet);

    if (selectEvents & RV_SELECT_WRITE)
        RV_FD_SET(fd->fd, &selectEngine->simulatedWrSet);
    else
        RV_FD_CLR(fd->fd, &selectEngine->simulatedWrSet);

    if (fd->fd > selectEngine->simulatedMaxFd)
        selectEngine->simulatedMaxFd = fd->fd;
    selectEngine->simulatedNumFd++;
    selectEngine->needToPreempt = RV_TRUE;

    RvLockRelease(&selectEngine->lock, selectEngine->logMgr);

    /* Make sure the select() loop exits - if indeed select() is still pending for events */
    rvFdPreempt(selectEngine, RvSelectEmptyPreemptMsg);
}


/********************************************************************************************
 * RvNucleusSelect - implements nucleus specific 'select' functionality.
 * Not thread-safe function.
 *
 * INPUT   : nfds          - maximum file descriptor number
 *           timeout       - maximum time period to wait for any event
 * OUTPUT  :
 *           readfds       - set of file descriptors waiting/received for read event
 *           writefds      - set of file descriptors waiting/received for write event
 *           exceptfds     - set of file descriptors waiting/received for exception
 * RETURN  : number of descriptors in resulting fd sets
 */
static RvUint32 RvNucleusSelect(
    IN      RvUint32 nfds,
    INOUT   fd_set *readfds,
    INOUT   fd_set *writefds,
    INOUT   fd_set *exceptfds,
    IN      struct timeval *timeout)
{
    RvUint t; /* Nucleus time is in ticks !!! */
    RvUint32 result;

    if (timeout != NULL)
    {
        /* Convert timeout requested to time format used by select */
        t = timeout->tv_sec * TICKS_PER_SECOND +
            ((RvInt64)timeout->tv_usec * TICKS_PER_SECOND) / RV_TIME64_NSECPERMSEC;

        if (t == 0)
            t = 1;
        result = NU_Select(nfds, readfds, writefds, exceptfds, t);
    }
    else
        result = NU_Select(nfds, readfds, writefds, exceptfds, NU_SUSPEND);

    /* Nucleus doesn't return the count */
    if (result == NU_SUCCESS)
    {
		result = nfds;
	}
	else if (result == NU_NO_DATA )
	{
		result = 0;
	}
    return result;
}

#endif  /* RV_OS_TYPE_NUCLEUS */


#if (RV_OS_TYPE == RV_OS_TYPE_MOPI)

RvStatus RvSelectMmbHandleSelectFds(RvSelectEngine *selectEngine)
{
    RvStatus status = RV_OK;
    int numResults, numFds;
    fd_set          tmpWrSet;
    fd_set          tmpRdSet;
    struct timeval  timeout = {0,0};
    int             iRead, iWrite;
    RvLogMgr*       logMgr = NULL;

    logMgr = selectEngine->logMgr;

    RV_FD_ZERO(&tmpRdSet);
    RV_FD_ZERO(&tmpWrSet);

    /* Reset the file descriptors set */
    status = RvSelectGetSelectFds(selectEngine, &numFds, &tmpRdSet, &tmpWrSet);

    /* 'sock_select' some-why deletes the fd_count field, so we have
    to 'remember' it manually */
    iRead  = (int)tmpRdSet.fd_count;
    iWrite = (int)tmpWrSet.fd_count;

    /* Run the select() function in blocking mode */
    numResults = sock_select(numFds, &tmpRdSet, &tmpWrSet, NULL, &timeout, SOCK_BLOCK);

    tmpRdSet.fd_count = (unsigned short)iRead;
    tmpWrSet.fd_count = (unsigned short)iWrite;

    RvSelectLogDebug((&logMgr->selectSource,
        "RvSelectMopiLogDebug: numResults=%d", numResults));

    /* Handle the result from the select() call */
    status = RvSelectHandleSelectFds(selectEngine,
        &tmpRdSet, &tmpWrSet, numFds, numResults);

    return status;
}

#endif  /* RV_OS_TYPE_MOPI */
#if (RV_SELECT_TYPE == RV_SELECT_KQUEUE && RV_SELECT_KQUEUE_GROUPS == RV_YES)

/************************** Private Functions *****************************/

/* 
 * The following 2 functions are callback functions assigned to the fd's
 * of the low and high priority Kqueues.
 * These functions are called by the main WaitAndBlock loop and they
 * call the real callback functions assigned by the user using a simple
 * priority policy: the low priority function handles only N1 events and
 * then the high priority function handles N2 events.
 */

void rvSelectKqueueLowCB(RvSelectEngine* selectEngine, RvSelectFd* fd,
                         RvSelectEvents selectEvent, RvUint error)
{
    int numResults;
    struct timespec msecTimeout = {0, 0};
    RvLogMgr* logMgr = selectEngine->logMgr;
    RV_USE_CCORE_GLOBALS;
    
    RV_UNUSED_ARG(fd);
    RV_UNUSED_ARG(selectEvent);
    RV_UNUSED_ARG(error);
    
    
    RvSelectLogEnter((&logMgr->selectSource, "rvSelectKqueueLowCB(eng=%p)",selectEngine));
    
    numResults = kevent(selectEngine->kqueueIdLow, NULL, 0, selectEngine->fdArrayLow,
                        selectEngine->ratioLow, &msecTimeout);
    
    /* handle any network events that occurred */
    RvSelectHandleKqueueFds(selectEngine, selectEngine->fdArrayLow,
                            rvSelectMaxDescriptors * 2, numResults);
    
    RvSelectLogLeave((&logMgr->selectSource, "rvSelectKqueueLowCB(eng=%p)",selectEngine));
}


void rvSelectKqueueHighCB(RvSelectEngine* selectEngine, RvSelectFd* fd,
                          RvSelectEvents selectEvent, RvUint error)
{
    int numResults;
    struct timespec msecTimeout = {0, 0};
    RvLogMgr* logMgr = selectEngine->logMgr;
    RV_USE_CCORE_GLOBALS;
    
    RV_UNUSED_ARG(fd);
    RV_UNUSED_ARG(selectEvent);
    RV_UNUSED_ARG(error);
    
    
    RvSelectLogEnter((&logMgr->selectSource, "rvSelectKqueueHighCB(eng=%p)",selectEngine));

    numResults = kevent(selectEngine->kqueueIdHigh, NULL, 0, selectEngine->fdArrayHigh,
                        selectEngine->ratioHigh, &msecTimeout);
    
    /* handle any network events that occurred */
    RvSelectHandleKqueueFds(selectEngine, selectEngine->fdArrayHigh,
                            rvSelectMaxDescriptors * 2, numResults);

    RvSelectLogLeave((&logMgr->selectSource, "rvSelectKqueueHighCB(eng=%p)",selectEngine));
}


/********************************************************************************************
 * rvSelectKqueueGroupsConstruct
 *
 * Create two Kqueues for low and high priority events.
 * The low priority Kqueue will be used for regular fd's (normally by the stack
 * toolkit, i.e. SIP) while the high priority Kqueue will be used directly by the
 * high level application for privileged fd's.
 *
 *
 * INPUT   : selectEngine - Events engine constructed previously.
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus rvSelectKqueueGroupsConstruct(
    IN RvSelectEngine*  selectEngine)
{
    RvLogMgr* logMgr;
    RvStatus status;
    RV_USE_CCORE_GLOBALS;
    
    
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif
    
    logMgr = selectEngine->logMgr;
    
    RvSelectLogEnter((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct(eng=%p)",selectEngine));
    
    
    /* Create the low & high priority 'kqueue' event queues */
    
    if ((selectEngine->kqueueIdLow = kqueue()) == -1)
    {
        RvSelectLogError((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct: creating low priority kqueue failure"));
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }
    
    if ((selectEngine->kqueueIdHigh = kqueue()) == -1)
    {
        RvSelectLogError((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct: creating high priority kqueue failure"));
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }
    
    RvSelectLogDebug((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct(eng=%p;kqueueIdLow=%d;kqueueIdHigh=%d)",
                      selectEngine, selectEngine->kqueueIdLow, selectEngine->kqueueIdHigh));


    /* Allocate memory for kevent structures to receive pending events from the Kqueue */

    status = RvMemoryAlloc(NULL, rvSelectMaxDescriptors * 2 * sizeof(struct kevent),
                           selectEngine->logMgr, (void**)&selectEngine->fdArrayLow);
    if (status != RV_OK)
    {
        RvSelectLogError((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct: RvMemoryAlloc failure"));
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }
    
    status = RvMemoryAlloc(NULL, rvSelectMaxDescriptors * 2 * sizeof(struct kevent),
                           selectEngine->logMgr, (void**)&selectEngine->fdArrayHigh);
    if (status != RV_OK)
    {
        RvSelectLogError((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct: RvMemoryAlloc failure"));
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }
    
    
    /* Register the Kqueues identifiers in the "Control" Kqueue */
    
    RvFdConstruct(&selectEngine->kqueueFdLow, &selectEngine->kqueueIdLow, logMgr);
    RvFdSetGroup(&selectEngine->kqueueFdLow, RV_SELECT_KQUEUE_GROUP_CNTL);
    RvSelectAdd(selectEngine, &selectEngine->kqueueFdLow, RvSelectRead, rvSelectKqueueLowCB);
    
    RvFdConstruct(&selectEngine->kqueueFdHigh, &selectEngine->kqueueIdHigh, logMgr);
    RvFdSetGroup(&selectEngine->kqueueFdHigh, RV_SELECT_KQUEUE_GROUP_CNTL);
    RvSelectAdd(selectEngine, &selectEngine->kqueueFdHigh, RvSelectRead, rvSelectKqueueHighCB);
    

    /* set arbitrary default values to the low and high priority events ratio */
    selectEngine->ratioLow  = 20;
    selectEngine->ratioHigh = 20;
    
    RvSelectLogLeave((&logMgr->selectSource, "rvSelectKqueueGroupsConstruct(eng=%p)",selectEngine));
    
    return RV_OK;
}


/********************************************************************************************
 * RvSelectKqueueSetRatio
 *
 * Set the proportion of events handling between low and high priority fd's.
 * Events that are detected by the WaitAndBlock loop are handled according
 * to a simple priority policy, i.e. for each 'ratioLow' events from the
 * low priority queue a 'ratioHigh' events from the high priority queue are handled.
 *
 *
 * INPUT   : selectEngine - Events engine constructed previously.
 *           ratioLow     - Ratio for low priority events
 *           ratioHigh    - Ratio for high priority events
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvSelectKqueueSetRatio(
    IN RvSelectEngine*      selectEngine,
    IN RvUint32             ratioLow,
    IN RvUint32             ratioHigh)
{
    RvLogMgr* logMgr;
    
    
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (selectEngine == NULL) {
        return RvSelectErrorCode(RV_ERROR_NULLPTR);
    }
#endif
    
    logMgr = selectEngine->logMgr;
    
    RvSelectLogEnter((&logMgr->selectSource, "RvSelectKqueueSetRatio(eng=%p;ratioLow=%d;ratioHigh=%d)",
                      selectEngine, ratioLow, ratioHigh));

    if (ratioLow == 0 || ratioHigh == 0)
    {
        RvSelectLogError((&logMgr->selectSource, "RvSelectKqueueSetRatio: ratioLow or ratioHigh equal 0"));
        return RvSelectErrorCode(RV_ERROR_UNKNOWN);
    }

    selectEngine->ratioLow  = ratioLow;
    selectEngine->ratioHigh = ratioHigh;
    
    RvSelectLogLeave((&logMgr->selectSource, "RvSelectKqueueSetRatio(eng=%p;ratioLow=%d;ratioHigh=%d)",
                      selectEngine, ratioLow, ratioHigh));

    return RV_OK;
}


/********************************************************************************************
 * RvFdSetGroup
 *
 * Set a group id to a socket (fd struct).
 *
 *
 * INPUT   : fd     - The fd struct constructed by RvFdConstruct.
 *           group  - The group id to assign
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvFdSetGroup(
    IN RvSelectFd*              fd,
    IN RvSelectKqueueGroupId    group)
{
    fd->group = group;
    
    return RV_OK;
}

#endif  /* RV_SELECT_TYPE == RV_SELECT_KQUEUE && RV_SELECT_KQUEUE_GROUPS == RV_YES */
/* Message queue support functions and data structures */

#if  RV_SELECT_PREEMPTION_TYPE == RV_SELECT_PREEMPTION_SMQ

static void RvSelectSMQNotifyCB(HRvSMQ self, void *ctx, RvUint32 signal) {
  RvSelectEngine *seli = (RvSelectEngine *)ctx;
  RvSocket *s = &seli->smqFd.fd;
  RvUint8 dummy = (RvUint8)signal;
  RvSize_t sent;

  RV_UNUSED_ARG(self);

  seli->smqNotifications++;
  RvSocketSendBuffer(s, &dummy, 1, &seli->smqAddr, seli->logMgr, &sent);
}

static void RvSelectSMQSelectCb(
                          IN RvSelectEngine*  seli,
                          IN RvSelectFd*      fd,
                          IN RvSelectEvents   selectEvent,
                          IN RvBool           error) {
  RvSize_t recvd = 0;
  RvLogMgr *logMgr = seli->logMgr;
  HRvSMQ  smq;
  
  if(error || selectEvent != RV_SELECT_READ) {
    return;
  }

  smq = seli->smq;
  
  do {
    RvUint8 dummy;
    
    RvSocketReceiveBuffer(&fd->fd, &dummy, 1, seli->logMgr, &recvd, 0);
    
    RvSMQDispatch(smq, logMgr);
    
  } while(recvd);
}

static
RvStatus RvSelectInitSMQSocket(RvSelectEngine *seli) {
  RvSocket ms;
  RvStatus s;
  RvUint32 ipv4;
  RvUint8 *pipv4 = (RvUint8 *)&ipv4;
  RvLogMgr *logMgr = seli->logMgr;
  
  pipv4[0] = 127;
  pipv4[1] = 0;
  pipv4[2] = 0;
  pipv4[3] = 1;
  
  RvAddressConstructIpv4(&seli->smqAddr, ipv4, 0);
  s = RvSocketConstruct(RV_ADDRESS_TYPE_IPV4, RvSocketProtocolUdp, logMgr, &ms);
  if(s != RV_OK) {
    return s;
  }

  s = RvSocketSetBlocking(&ms, RV_FALSE, logMgr);
  if(s != RV_OK) goto failure;
  s = RvSocketBind(&ms, &seli->smqAddr, 0, logMgr);
  if(s != RV_OK) goto failure;
  RvSocketGetLocalAddress(&ms, logMgr, &seli->smqAddr);
  RvFdConstruct(&seli->smqFd, &ms, logMgr);
  RvSelectAdd(seli, &seli->smqFd, RV_SELECT_READ, RvSelectSMQSelectCb);
  return RV_OK;
failure:
  RvSocketDestruct(&ms, RV_FALSE, 0, logMgr);
  return s;
}

static 
void RvSelectDestructSMQSocket(RvSelectEngine *seli) {
  RvAddressDestruct(&seli->smqAddr);
  RvSocketDestruct(&seli->smqFd.fd, RV_FALSE, 0, seli->logMgr);
  RvFdDestruct(&seli->smqFd);
}

/* typedef void (*RvSMQCb)(RvSMQMsgTargetId id, RvSMQMsgId message, RvSMQMsgParam p1, RvSMQMsgParam p2, void *ctx);
*/

void RvSelectSMQTargetCallback(RvSMQMsgTargetId id, RvSMQMsgId msg, RvSMQMsgParam p1, RvSMQMsgParam p2, void *ctx) {
  RvSelectEngine *seli = (RvSelectEngine *)ctx;
  RvUint8 u8msg;

  RV_UNUSED_ARG(id);
  RV_UNUSED_ARG(p1);
  RV_UNUSED_ARG(p2);

  u8msg = (RvUint8)msg;
  
  if(u8msg != RvSelectEmptyPreemptMsg && seli->preemptionCb != 0) {
    seli->preemptionCb(seli, u8msg, seli->preemptionUsrCtx);
  }

}

static 
void RvSelectDestructSMQPreemption(RvSelectEngine *seli) {
  RvSMQDestruct(seli->smq, seli->logMgr);
  RvSelectDestructSMQSocket(seli);
}

static 
RvStatus RvSelectConstructSMQPreemption(RvSelectEngine *seli, RvInt size) {
  RvStatus s;
  HRvSMQ smq;
  RvLogMgr *logMgr = seli->logMgr;

  s = RvSMQConstruct(&smq, size, RvSelectSMQNotifyCB, (void *)seli, logMgr);
  if(s != RV_OK) {
    return s;
  }

  seli->smq = smq;
  seli->smqNotifications = 0;

  s = RvSelectInitSMQSocket(seli);

  if(s != RV_OK) {
    RvSMQDestruct(smq, logMgr);
    return s;
  }

  s = RvSMQRegisterTarget(smq, RvSelectSMQTargetCallback, (void *)seli, &seli->smqMsgTargetId, seli->logMgr);
  if(s != RV_OK) {
      RvSMQDestruct(smq, seli->logMgr);
      RvSelectDestructSMQSocket(seli);
  }

  return s;
}


#endif


/* End message queue support */

#endif /* (RV_NET_TYPE != RV_NET_NONE) */
