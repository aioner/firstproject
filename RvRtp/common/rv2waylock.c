/***********************************************************************
Filename   : rvtriplelock.h
Description: rvtriplelock implementation file
This file contains utilities and definitions concerning the triple
lock mechanism which enables simultaneous API/Event locking
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

#include "rv2waylock.h"

#define RV_2_WAY_LOCK_UNDEFINED (-1)

/* Lets make error codes a little easier to type */
#define Rv2WayLockErrorCode(_e) \
		  RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_2WAYLOCK, (_e))

/* Make sure we don't crash if we pass a NULL log manager to these module's functions */
#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
#define Rv2WayLockLogEnter(p) {if (logMgr != NULL) RvLogEnter(&logMgr->twowaylockSource, p);}
#else
#define Rv2WayLockLogEnter(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
#define Rv2WayLockLogLeave(p) {if (logMgr != NULL) RvLogLeave(&logMgr->twowaylockSource, p);}
#else
#define Rv2WayLockLogLeave(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_SYNC)
#define Rv2WayLockLogSync(p) {if (logMgr != NULL) RvLogSync(&logMgr->twowaylockSource, p);}
#else
#define Rv2WayLockLogSync(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
#define Rv2WayLockLogWarning(p) {if (logMgr != NULL) RvLogWarning(&logMgr->twowaylockSource, p);}
#else
#define Rv2WayLockLogWarning(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
#define Rv2WayLockLogError(p) {if (logMgr != NULL) RvLogError(&logMgr->twowaylockSource, p);}
#else
#define Rv2WayLockLogError(p) {RV_UNUSED_ARG(logMgr);}
#endif
#if (RV_LOGMASK & RV_LOGLEVEL_EXCEP)
#define Rv2WayLockLogExcep(p) {if (logMgr != NULL) RvLogExcep(&logMgr->twowaylockSource, p);}
#else
#define Rv2WayLockLogExcep(p) {RV_UNUSED_ARG(logMgr);}
#endif

/* get locking information without crashing */
#define RV_2WAYLOCK_GET_ID(twLock,cbCtx)                                 \
	(twLock->lockCbs != NULL && twLock->lockCbs->getObjIdFunc != NULL ?  \
	 twLock->lockCbs->getObjIdFunc(cbCtx) : NULL)

#define RV_2WAYLOCK_IS_VALID(twLock,cbCtx,objId)                          \
	(twLock->lockCbs != NULL && twLock->lockCbs->isValidObjFunc != NULL ? \
	 twLock->lockCbs->isValidObjFunc(cbCtx,objId) : RV_TRUE)

#define RV_2WAYLOCK_START_PRINT(twLock,param,logMgr,func)   \
	Rv2WayLockLogSync((&logMgr->twowaylockSource,           \
	"%s(twLock=%p,cbCnt=%d,threadId=%p,%p,%p)",             \
    func,twLock,twLock->lockInfo.cbCnt,                     \
    twLock->lockInfo.threadId,twLock->lockInfo.cbThreadId,param));

#define RV_2WAYLOCK_END_PRINT(twLock,param,logMgr,func)   \
	Rv2WayLockLogSync((&logMgr->twowaylockSource,         \
	"%s(twLock=%p,cbCnt=%d,threadId=%p,%p,%p) Completed", \
    func,twLock,twLock->lockInfo.cbCnt,                   \
    twLock->lockInfo.threadId,twLock->lockInfo.cbThreadId,param));

#define RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,param,logMgr,func)  \
if (twLock == NULL || (void*)param == NULL)            \
{				                                       \
	Rv2WayLockLogError((&logMgr->twowaylockSource,     \
			  "%s(twLock=%p,%p) NULL pointer input",   \
			  func,twLock,param));                     \
	return Rv2WayLockErrorCode(RV_ERROR_NULLPTR);	   \
}

#define RV_2WAYLOCK_CHECK_AND_RETURN(twLock,logMgr,func) \
if (twLock == NULL)                                  \
{                                                    \
	Rv2WayLockLogError((&logMgr->twowaylockSource,   \
			  "%s(twLock=%p) NULL pointer input",    \
	          func,twLock));                         \
	return;                                          \
}

#define RV_2WAYLOCK_GLOBAL_LOCK_IF_NEEDED(gLock,logMgr)       \
                if (gLock != NULL)                            \
                {                                             \
                    RvMutexLock(&gLock->writeLock,logMgr);    \
                    RV_CONDVAR_WAITM(                         \
                        (gLock->writeNo == 0),                \
                         &gLock->writeCond,                   \
                         &gLock->writeLock,logMgr);           \
                    RvMutexLock(&gLock->readLock,logMgr);     \
                    gLock->readNo++;                          \
                    RvMutexUnlock(&gLock->readLock,logMgr);   \
                    RvMutexUnlock(&gLock->writeLock,logMgr);  \
                }

#define RV_2WAYLOCK_GLOBAL_UNLOCK_IF_NEEDED(gLock,logMgr)     \
                if (gLock != NULL)                            \
                {                                             \
                    RvMutexLock(&gLock->readLock,logMgr);     \
                    gLock->readNo--;                          \
                    if(gLock->readNo == 0)                    \
                    {                                         \
                     RvCondvarBroadcast(&gLock->readCond,logMgr);\
                    }                                         \
                    RvMutexUnlock(&gLock->readLock,logMgr);   \
                }

#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)

/*-----------------------------------------------------------------------*/
/*                          STATIC PROTOTYPES                            */
/*-----------------------------------------------------------------------*/
static RvStatus TwoWayLockUnlock(IN Rv2WayLock* twLock,
                                 IN RvLogMgr*   logMgr);

static void ReduceReadNo(IN  Rv2WayLock* twLock,
                         IN  RvLogMgr*   logMgr,
                         OUT RvInt32*    lockAgain);


/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS IMPLEMENTATION                     */
/*-----------------------------------------------------------------------*/


/********************************************************************************************
* Rv2WayLockConsruct
*
* Constructs and initializes an object local 2WayLock instance.
*
* INPUT   : logMgr	 - Pointer to a log manager.
*           lockCfg  - Pointer to a set of callbacks to be used through locking
*
* OUTPUT  : twLock   - Pointer to a 2WayLock to be constructed and initialized
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockConstruct(
					IN    RvLogMgr*		  logMgr,
					IN    Rv2WayLockCfg*  lockCfg,
					INOUT Rv2WayLock*	  twLock)
{
	RvStatus      rv;

	RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,lockCfg,logMgr,"Rv2WayLockConsruct");
	RV_2WAYLOCK_START_PRINT(twLock,NULL,logMgr,"Rv2WayLockConsruct");

	rv = RvCondvarConstruct(&twLock->condVar, logMgr);
    if (rv != RV_OK)
    {
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockConsruct(twLock=%p) failed to construct condvar (rv=%d)",twLock,rv));
        return Rv2WayLockErrorCode(rv);
    }

    rv = RvMutexConstruct(logMgr, &twLock->lock);
    if (rv != RV_OK)
    {
        RvCondvarDestruct(&twLock->condVar,logMgr);
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockConsruct(twLock=%p) failed to contruct mutex (rv=%d)",twLock,rv));
        return Rv2WayLockErrorCode(rv);
    }

	twLock->lockInfo.threadId   = 0;
	twLock->lockInfo.cbCnt      = 0;
	twLock->lockInfo.cbThreadId = 0;
	twLock->lockCbs             = lockCfg->lockCbs;
	twLock->gLock               = lockCfg->gLock;

	RV_2WAYLOCK_END_PRINT(twLock,NULL,logMgr,"Rv2WayLockConsruct");

	return RV_OK;
}

/********************************************************************************************
* Rv2WayLockDestruct
*
* Destroys a 2-way lock instance.
*
* INPUT   : twLock - Pointer to an object's local 2-way lock to be destructed
*           logMgr - Pointer to a log manager.
*
* OUTPUT  : none.
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockDestruct(
                IN Rv2WayLock* twLock,
                IN RvLogMgr*   logMgr)
{
    RvInt32  lockCnt = 0;
    RvStatus rv;

	RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,"NULL",logMgr,"Rv2WayLockDestruct");
	RV_2WAYLOCK_START_PRINT(twLock,NULL,logMgr,"Rv2WayLockDestruct");

    rv = RvMutexGetLockCounter(&twLock->lock,logMgr,&lockCnt);
	if (rv != RV_OK || lockCnt > 0)
    {
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockDestruct(twLock=%p) error rv (%d) or reference count is not zero (%d)",
            twLock,rv,lockCnt));
        return Rv2WayLockErrorCode(RV_ERROR_UNKNOWN);
    }

    RvCondvarDestruct(&twLock->condVar, logMgr);
	RvMutexDestruct(&twLock->lock,logMgr);

	RV_2WAYLOCK_END_PRINT(twLock,NULL,logMgr,"Rv2WayLockDestruct");
	return RV_OK;
}

/********************************************************************************************
* Rv2WayLockConsructGlobal
*
* Constructs and initializes an 2WayLock global lock instance. The global lock would
* serve some objects which may exchange their locks pointers.
*
* INPUT   : logMgr	 - Pointer to a log manager.
*
* OUTPUT  : gLock    - Pointer to a global lock to be constructed and initialized
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockConstructGlobal(
                IN    RvLogMgr*		         logMgr,
                INOUT Rv2WayLockGlobalLock*  gLock)
{
    RvStatus rv;

    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(gLock,"NULL",logMgr,"Rv2WayLockConsructGlobal");
    Rv2WayLockLogSync((&logMgr->twowaylockSource,"Rv2WayLockConsructGlobal(gLock=%p)",gLock));

    rv = RvCondvarConstruct(&gLock->readCond, logMgr);
    if (rv != RV_OK)
    {
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockConsructGlobal(gLock=%p) failed to construct read condvar (rv=%d)",gLock,rv));
        return Rv2WayLockErrorCode(rv);
    }

    rv = RvCondvarConstruct(&gLock->writeCond, logMgr);
    if (rv != RV_OK)
    {
        RvCondvarDestruct(&gLock->readCond,logMgr);
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockConsructGlobal(gLock=%p) failed to construct write condvar (rv=%d)",gLock,rv));
        return Rv2WayLockErrorCode(rv);
    }

    rv = RvMutexConstruct(logMgr,&gLock->readLock);
    if (rv != RV_OK)
    {
        RvCondvarDestruct(&gLock->readCond,logMgr);
        RvCondvarDestruct(&gLock->writeCond,logMgr);
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockConsructGlobal(gLock=%p) failed to construct read lock (rv=%d)",gLock,rv));
        return Rv2WayLockErrorCode(rv);
    }

    rv = RvMutexConstruct(logMgr,&gLock->writeLock);
    if (rv != RV_OK)
    {
        RvCondvarDestruct(&gLock->readCond,logMgr);
        RvCondvarDestruct(&gLock->writeCond,logMgr);
        RvMutexDestruct(&gLock->readLock,logMgr);
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockConsructGlobal(gLock=%p) failed to construct write lock (rv=%d)",gLock,rv));
        return Rv2WayLockErrorCode(rv);
    }

    /* Initializes the number of readers and writers */
    gLock->readNo  = 0;
    gLock->writeNo = 0;

    Rv2WayLockLogSync((&logMgr->twowaylockSource,"Rv2WayLockConsructGlobal(gLock=%p) Succeeded",gLock));
    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockDestructGlobal
*
* Destruct and release resources of an 2WayLock global lock instance. The global
* lock would serve some objects which may exchange their locks pointers.
*
* INPUT   : gLock   - Pointer to a global lock to be destructed.
*           logMgr	- Pointer to a log manager.
* OUTPUT  : none
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockDestructGlobal(
                  IN Rv2WayLockGlobalLock*  gLock,
                  IN RvLogMgr*		        logMgr)
{
    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(gLock,"NULL",logMgr,"Rv2WayLockDestructGlobal");
    Rv2WayLockLogSync((&logMgr->twowaylockSource,"Rv2WayLockDestructGlobal(gLock=%p)",gLock));

    RvCondvarDestruct(&gLock->readCond,logMgr);
    RvCondvarDestruct(&gLock->writeCond,logMgr);
    RvMutexDestruct(&gLock->readLock,logMgr);
    RvMutexDestruct(&gLock->writeLock,logMgr);

    Rv2WayLockLogSync((&logMgr->twowaylockSource,"Rv2WayLockDestructGlobal(gLock=%p) Succeeded",gLock));
    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockLock
*
* Locks a 2-way lock instance due to an internal event or due to an API call
*
* INPUT   : twLock  - Pointer to an 2-way lock instance.
*           cbCtx   - Context pointer to be referred by the locking callbacks (if any).
*					  NOTE: This context pointer may be NULL. In this case the locking
*							callbacks will not be triggered.
*			isEvent - Indication if the locking is due to internal event (RV_TRUE) or
*					  due to API call (RV_FALSE).
*			logMgr  - Pointer to a log manager.
* OUTPUT  : none.
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockLock(
            IN Rv2WayLock*    twLock,
            IN void*          cbCtx,
            IN RvBool		  isEvent,
            IN RvLogMgr*	  logMgr)
{
    void* objId;

    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,"NULL",logMgr,"Rv2WayLockLock");
    RV_2WAYLOCK_START_PRINT(twLock,cbCtx,logMgr,"Rv2WayLockLock");

    RV_2WAYLOCK_GLOBAL_LOCK_IF_NEEDED(twLock->gLock,logMgr);

    RvMutexLock(&twLock->lock, logMgr);

    /* Retrieve the locking object ID by the given context */
    objId  = RV_2WAYLOCK_GET_ID(twLock,cbCtx);

    Rv2WayLockLogSync((&logMgr->twowaylockSource,
        "Rv2WayLockLock(twLock=%p,%p) going to wait on cond var for %s locking",
        twLock,cbCtx,isEvent == RV_TRUE ?  "Event" : "API"));

    /*We are looping while the "threadId == 0 || threadId == RvThreadCurrentId()"
      condition is false. If it is false the macro would call the condVar wait
      function and wait for notification.
      The isEvent means: If we are trying to lock event from lower layer we want
      to prevent its processing until the the call back is completed */
    RV_CONDVAR_WAITM(twLock->lockInfo.threadId == RvThreadCurrentId() ||
          twLock->lockInfo.cbThreadId == RvThreadCurrentId() ||
          (twLock->lockInfo.threadId  == 0 &&
           (isEvent == RV_TRUE ? twLock->lockInfo.cbCnt == 0 : RV_TRUE)), /*added to prevent processing events while in CB cbCount>0*/
          &twLock->condVar, &twLock->lock, logMgr);


    Rv2WayLockLogSync((&logMgr->twowaylockSource,
        "Rv2WayLockLock(twLock=%p) After cond var waiting", twLock));

    /* Now we verify if the object that acquires this lock was changed while waiting for locking*/
    if (RV_2WAYLOCK_IS_VALID(twLock,cbCtx,objId) != RV_TRUE)
    {
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockLock(twLock=%p,%p,%p) the locked object seems to be invalid",
            twLock,cbCtx,objId));

        TwoWayLockUnlock(twLock,logMgr);

        return Rv2WayLockErrorCode(RV_ERROR_DESTRUCTED);
    }

    /* Update the lock information */
    twLock->lockInfo.threadId = RvThreadCurrentId();

    RV_2WAYLOCK_END_PRINT(twLock,cbCtx,logMgr,"Rv2WayLockLock");
    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockUnlock
*
* Unlocks a 2-way lock instance after an internal event or at the end of an API call
*
* INPUT   : twLock - Pointer to an object's local 2-way lock
*			logMgr - Pointer to a log manager.
* OUTPUT  : none.
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockUnlock(
          IN Rv2WayLock* twLock,
          IN RvLogMgr*   logMgr)
{
    RvStatus rv;

    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,"NULL",logMgr,"Rv2WayLockUnlock");
    RV_2WAYLOCK_START_PRINT(twLock,NULL,logMgr,"Rv2WayLockUnlock");

    rv = TwoWayLockUnlock(twLock,logMgr);
    if (rv != RV_OK)
    {
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockLock(twLock=%p) failed to unlock (%d)", twLock,rv));
        return Rv2WayLockErrorCode(RV_ERROR_DESTRUCTED);
    }

    RV_2WAYLOCK_END_PRINT(twLock,NULL,logMgr,"Rv2WayLockUnlock");
    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockBeforeCb
*
* Unlock a 2-way lock instance before triggering a callback.
* This function will reset the 2-Way Lock thread id (since it unlocks the 2-way lock).
* Rv2WayLockAfterCb() should be invoked with the same output parameter of this
* function after the callback function returns.
*
*
* INPUT   : twLock - Pointer to a 2-way lock instance.
*			logMgr - Pointer to a log manager.
* OUTPUT  : cbInfo - Information, describes the locking state before the callback triggering
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockBeforeCb(
            IN  Rv2WayLock*       twLock,
            IN  RvLogMgr*	      logMgr,
            OUT Rv2WayLockCbInfo* cbInfo)
{
    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,cbInfo,logMgr,"Rv2WayLockBeforeCb");
    RV_2WAYLOCK_START_PRINT(twLock,NULL,logMgr,"Rv2WayLockBeforeCb");

    /* Increment the callback count since we are about to raise a callback */
    twLock->lockInfo.cbCnt++;
    twLock->lockInfo.threadId   = 0;
    twLock->lockInfo.cbThreadId = RvThreadCurrentId();
    cbInfo->lock                = twLock; /* Keep the released lock */
    RvMutexRelease(&twLock->lock, logMgr, &cbInfo->lockCnt);


    /* Notify others that are waiting on the same conditional variable that the mutex is free to use.
      Events won't be able to lock since the cbCount is greater than 0 (unless the event lockings are
      in the same thread of the first event locking), API calls can lock the locker*/
    RvCondvarBroadcast(&twLock->condVar, logMgr);

    RV_2WAYLOCK_GLOBAL_UNLOCK_IF_NEEDED(twLock->gLock,logMgr);

    RV_2WAYLOCK_END_PRINT(twLock,NULL,logMgr,"Rv2WayLockBeforeCb");
    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockAfterCb
*
* Re-locks a 2-way lock after returning from a callback.
*
*
* INPUT   : cbInfo - Information, describes the locking state before the callback triggering.
*			twLock - Pointer to a 2-way lock instance.
*           logMgr - Pointer to a log manager.
* OUTPUT  : none.
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockAfterCb(
           IN  Rv2WayLock*       twLock,
           IN  Rv2WayLockCbInfo* cbInfo,
           IN  RvLogMgr*	     logMgr)
{
    RvBool bDecreaseCb = RV_TRUE;
    
    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,cbInfo,logMgr,"Rv2WayLockAfterCb");
    RV_2WAYLOCK_START_PRINT(twLock,cbInfo->lockCnt,logMgr,"Rv2WayLockAfterCb");

    RV_2WAYLOCK_GLOBAL_LOCK_IF_NEEDED(twLock->gLock,logMgr);

    if (cbInfo->lock != twLock)
    {
        Rv2WayLock* oldLock = cbInfo->lock;

        Rv2WayLockLogSync((&logMgr->twowaylockSource,
            "Rv2WayLockAfterCb(twLock=%p) the 2-way-lock was changed through callback. Decreasing the old cbCnt",
            twLock));
        RvMutexLock(&oldLock->lock,logMgr);
        oldLock->lockInfo.cbCnt--;
        RvMutexUnlock(&oldLock->lock,logMgr);
        bDecreaseCb = RV_FALSE;
    }
    
    RvMutexRestore(&twLock->lock, logMgr, cbInfo->lockCnt);
    twLock->lockInfo.threadId = RvThreadCurrentId();
    if (bDecreaseCb == RV_TRUE)
    {
        twLock->lockInfo.cbCnt--;
    }
    /* Restore the cbThreadId if this is the last cb level
    */
    if (twLock->lockInfo.cbCnt == 0) 
    {
        twLock->lockInfo.cbThreadId = 0;
    }
    

    RV_2WAYLOCK_END_PRINT(twLock,cbInfo->lockCnt,logMgr,"Rv2WayLockAfterCb");

    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockReplace
*
* Replaces a given two-way lock with a new two-way lock. This function may be called in any
* locking state, even if any of the two-ways locks is locked.
*
* NOTE:
*   1. Both of the two-way locks should be have been constructed with the same global lock
*      (see lockCfg.gLock in Rv2WayLockConstruct() API).
*   2. If the replaced lock object is in the middle of callback, then it's callback counter would
*      be reduced back upon returning from the callback. In other words, new events concerned
*      with the replaced (old) lock would not be handled until all the callbacks of the
*      replaced lock object return.
*
* INPUT   : newTwLock - Pointer to the new two-way lock to replace the first twLock
*           logMgr    - Pointer to a log manager.
* OUTPUT  : repTwLock - Pointer to a 2-way lock instance to be replaced
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockReplace(
               IN    Rv2WayLock*    newTwLock,
               IN    RvLogMgr*	    logMgr,
               INOUT Rv2WayLock**   repTwLock)
{
    Rv2WayLockGlobalLock* gLock;
    Rv2WayLock*           twLock = (repTwLock == NULL) ? NULL : *repTwLock;
    RvInt32               lockAgain = 0;

    RV_2WAYLOCK_CHECK_AND_RETURN_ERROR(twLock,newTwLock,logMgr,"Rv2WayLockReplace");
    RV_2WAYLOCK_START_PRINT(twLock,newTwLock,logMgr,"Rv2WayLockReplace");

    if (twLock->gLock == NULL || twLock->gLock != newTwLock->gLock)
    {
        Rv2WayLockLogError((&logMgr->twowaylockSource,
            "Rv2WayLockReplace(twLock=%p,%p) the replaced(%p)/replacing(%p) globals are different (or NULL)",
            twLock,newTwLock,twLock->gLock,newTwLock->gLock));
        return Rv2WayLockErrorCode(RV_ERROR_BADPARAM);
    }

    gLock = twLock->gLock;
    RvMutexLock(&gLock->writeLock,logMgr);
    gLock->writeNo++;
    RvMutexUnlock(&gLock->writeLock,logMgr);

    ReduceReadNo(twLock,logMgr,&lockAgain);

    RvMutexLock(&gLock->readLock,logMgr);
    RV_CONDVAR_WAITM((gLock->readNo != 0), &gLock->readCond, &gLock->readLock,logMgr);
    *repTwLock = newTwLock;
    if (lockAgain > 0)
    {
        RvMutexRestore(&newTwLock->lock,logMgr,lockAgain);
    }
    RvMutexUnlock(&gLock->readLock,logMgr);

    RvMutexLock(&gLock->writeLock,logMgr);
    gLock->writeNo--;
    if (gLock->writeNo == 0)
    {
        RvCondvarBroadcast(&gLock->writeCond,logMgr);
    }
    RvMutexUnlock(&gLock->writeLock,logMgr);

    RV_2WAYLOCK_END_PRINT(twLock,newTwLock,logMgr,"Rv2WayLockReplace");

    return RV_OK;
}

/********************************************************************************************
* Rv2WayLockSourceConstruct - Constructs 2-way lock module log source.
*
* Constructs log source to be used by common core when printing log from the
* 2-way lock module. This function is applied per instance of log manager.
*
* INPUT   : logMgr - log manager instance
* OUTPUT  : none
* RETURN  : RV_OK if successful otherwise an error code.
*/
RvStatus Rv2WayLockSourceConstruct(IN RvLogMgr *logMgr)
{
    RvStatus rv = RV_OK;

#if (RV_LOCK_TYPE != RV_LOCK_NONE)
    rv = RvLogSourceConstruct(logMgr, &logMgr->twowaylockSource, "2_WAY_LOCK", "2-Way Lock Interface");
#else
    RV_UNUSED_ARG(logMgr);
#endif

    return rv;

}

/*-----------------------------------------------------------------------*/
/*                          STATIC IMPLEMENTATION                        */
/*-----------------------------------------------------------------------*/

/********************************************************************************************
* TwoWayLockUnlock
*
* Unlocks a 2-way lock instance after an internal event or at the end of an API call
*
* INPUT   : twLock    - Pointer to an object's local 2-way lock
*           logMgr    - Pointer to a log manager.
*           nestLevel - The number of recursive unlocks
* OUTPUT  : none.
* RETURN  : RV_OK if successful otherwise an error code.
*/
static RvStatus TwoWayLockUnlock(IN Rv2WayLock* twLock,
                                 IN RvLogMgr*   logMgr)
{
    RvInt32  lockCnt = 0;
    RvStatus rv;

    rv = RvMutexGetLockCounter(&twLock->lock,logMgr,&lockCnt);
    if (rv != RV_OK || lockCnt == 0)
    {
        Rv2WayLockLogExcep((&logMgr->twowaylockSource,
            "TwoWayLockUnlock(twLock=%p) error rv (%d) or reference count is 0 BEFORE Unlock", twLock, rv));
        return RV_ERROR_DESTRUCTED;
    }

    /* Check if the lock is not nested. If it's not nested the thread id is reset */
    if (lockCnt == 1)
    {
        Rv2WayLockLogSync((&logMgr->twowaylockSource,
            "TwoWayLockUnlock(twLock=%p) reference count is 0, reseting thread Id.", twLock));
        twLock->lockInfo.threadId = 0;
        RvMutexRelease(&twLock->lock, logMgr,&lockCnt);
        /* Notify others that are waiting on the same conditional variable that the mutex is free to use.*/
        RvCondvarBroadcast(&twLock->condVar, logMgr);
    }
    else
    {
        RvMutexUnlock(&twLock->lock, logMgr);
    }

    RV_2WAYLOCK_GLOBAL_UNLOCK_IF_NEEDED(twLock->gLock,logMgr);

    return RV_OK;
}

/********************************************************************************************
* ReduceReadNo
*
* Reduces the number of current readers by the following actions:
*   1. Unlock the 2waylock locally ff the current thread has already locked it.
*   2. Waits until all of readers would release their lock. Note that in order
*      further avoid readers locks, the writers counter should have been increased
*      before invoking this function.
*
* INPUT   : twLock    - Pointer to an object's local 2-way lock
*           logMgr    - Pointer to a log manager.
* OUTPUT  : lockAgain - Specifies the nesting level of unlocks which took place internally
*                       in this function and thus should be re-locked (through the replacing
*                       lock later on.)
* RETURN  : RV_OK if successful otherwise an error code.
*/
static void ReduceReadNo(IN  Rv2WayLock* twLock,
                         IN  RvLogMgr*   logMgr,
                         OUT RvInt32*    lockAgain)
{
    /* find if the current thread locks the replaced two-way lock */
    RvMutexLock(&twLock->lock,logMgr);
    if (twLock->lockInfo.threadId == 0 || twLock->lockInfo.threadId == RvThreadCurrentId())
    {
        RvMutexRelease(&twLock->lock,logMgr,lockAgain);
        *lockAgain = *lockAgain - 1; /* Reduce the counter by 1 due to the prior internal lock above */
    }
    else
    {
        RvMutexUnlock(&twLock->lock,logMgr);
        *lockAgain = 0;
    }
}
#else /* (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE) */

int prevent_warning_of_ranlib_has_no_symbols_rv2waylock=0;

#endif /* (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE) */
