/***********************************************************************
Filename   : rv2waylock.h
Description: rv2waylock header file
This file contains utilities and definitions concerning the 2 way 
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

#ifndef RV_2_WAY_LOCK_H
#define RV_2_WAY_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------*/
/*                        INCLUDE HEADER FILES                           */
/*-----------------------------------------------------------------------*/
#include "rvccore.h"
#include "rvmutex.h"
#include "rvthread.h"
#include "rvcondvar.h"
#include "rvlog.h"

struct Rv2WayLock_s;      /* Dummy declaration, to be defined down below */ 
typedef struct Rv2WayLock_s Rv2WayLock;

/*-----------------------------------------------------------------------*/
/*								CALLBACKS                                */
/*-----------------------------------------------------------------------*/

/********************************************************************************************
* Rv2WayLockGetObjIdFunc
*
*	Defines a prototype for a function that retrieves an object's 
*   identifier. This identifier may be used to verify that the 
*   object identifier wasn't changed through the locking session.
*   
*   NOTE: Within the callback implementation there MUSTN'T be any locking.
*
* INPUT   : pObj    - Pointer to an object to be locked.
* OUTPUT  : none.
* RETURN  : Pointer to the object's identifier.
*/
typedef void* (*Rv2WayLockGetObjIdFunc)(IN  void*	obj);

/********************************************************************************************
* Rv2WayLockIsObjValidFunc - Prototype for validity check function (through locking)
*
*	Defines a prototype for an object's validity check function after the object
*   is locked. For example this function may call RLIST_IsElementVacant() 
*   for an RLIST object or may return with no further checks. 
*
* INPUT   : obj    - Pointer to an object to be locked.
*
* OUTPUT  : objId  - Pointer to the object unique id which was retrieved by 
*                    Rv2WayLockGetObjIdFunc callback
*
* RETURNS:
*  Indication if the object is valid (RV_TRUE) or not (RV_FALSE)
*/
typedef RvBool (*Rv2WayLockIsObjValidFunc)(IN void* obj,IN void* objId);

/*-----------------------------------------------------------------------*/
/*                          TYPE DEFINITIONS                             */
/*-----------------------------------------------------------------------*/

/* 2-Way Lock callback functions  */
typedef struct 
{
	Rv2WayLockGetObjIdFunc            getObjIdFunc;
	/* For the retrieval of an object identifier 
	   NOTE: This function may be NULL if there's no id */
	Rv2WayLockIsObjValidFunc          isValidObjFunc;
	/* For checking the validity of an object identifier through locking 
	   NOTE: This function may be NULL if no validity checks should
	         take place */
} Rv2WayLockCBs;

/* 2-Way Lock information of the current lock state */
typedef struct 
{
	RvUint32	  cbCnt;        /* Event nested locking counter */
	RvThreadId    threadId;     /* The id of the last successful locking thread */
	RvThreadId    cbThreadId;   /* The id of the thread in callback 
	                               NOTE: There may be no more than one thread in 
	                               callback since the lock mechanism allows
	                               only a single event locking concurrently */
} Rv2WayLockInfo;

/* 2-Way Lock information to be used through locked object callback */
typedef struct 
{
	RvInt32     lockCnt;  /* Counter of lock nesting */
	Rv2WayLock* lock;    /* Pointer to the lock which was released before callback - used for lock switching */
} Rv2WayLockCbInfo;

/* 2-Way Global Lock structure in order to enable lock replacement */
typedef struct 
{
    RvCondvar readCond;
    RvInt     readNo;        /* readers number */
    RvCondvar writeCond;
    RvInt     writeNo;       /* writers number */
    RvMutex   writeLock;
    RvMutex   readLock;
} Rv2WayLockGlobalLock;

/* 2-Way Lock configuration structure to be used through lock construction */
typedef struct 
{
    Rv2WayLockCBs*       lockCbs; /* Pointer to callbacks structure - to be used
                                     through lock procedure . See Rv2WayLockCBs.
                                     NOTE: 
                                      1. The 2WayLock keeps a pointer to this
                                         structure (but doesn't copy it) for 
                                         lower memory consumption. Thus, this 
                                         pointer should be valid as long as 
                                         the 2WayLock is not destructed.
                                      2. The pointer may be NULL. */                                          
    Rv2WayLockGlobalLock* gLock;  /* Pointer to a global structure to be used
                                     for an object's lock replacement.
                                     NOTE: 
                                     1. The 2WayLock keeps a pointer to this
                                        structure (but doesn't copy it) for 
                                        lower memory consumption. Thus, this 
                                        pointer should be valid as long as 
                                        the 2WayLock is valid (not destructed).
                                     2. The pointer may be NULL. 
                                     3. The global lock may be constructed by
                                        Rv2WayLockConsructGlobal() */ 
    
} Rv2WayLockCfg;

/* Rv2WayLock_s
* ------------------------
* Holds a 2-way lock data of any object.
*/
struct Rv2WayLock_s
{
#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)

	/* The following members are meaningless when the multi-thread mode is off */ 
	RvMutex               lock;     /* The basic object lock */ 
	RvCondvar		      condVar;  /* The object condition variable */
	Rv2WayLockInfo        lockInfo; /* Information about the current locking state */
	Rv2WayLockGlobalLock* gLock;    /* Pointer to global lock for lock replacement.
	                                   NOTE: The pointer may be NULL */

	/*------------------------ Module References ------------------------*/
	/* The following members are fixed per module. Consequently, 
	   these parameters may be initialized during the allocation  
	   of the whole module (but per object). The 2-Way Lock maintains
	   the Mutex methodology: A 2-Way Lock object is constructed 
	   and initialized once. Afterwards it may be locked/unlocked many 
	   times until it is destructed */ 
	Rv2WayLockCBs*   lockCbs;     /* Pointer to a set of locking callbacks */
#else 
    int dummy;
#endif /* #if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE) */ 
};


#if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE)

/*-----------------------------------------------------------------------*/
/*                          FUNCTIONS HEADERS                            */
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
                INOUT Rv2WayLock*	  twLock);

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
                IN RvLogMgr*   logMgr);


/********************************************************************************************
* Rv2WayLockConsructGlobal
*
* Constructs and initializes an 2WayLock global lock instance. The global lock would
* serve some objects which may exchange their locks pointers.
*
* INPUT   : logMgr	 - Pointer to a log manager.
*           lockCfg  - Pointer to a set of callbacks to be used through locking
*       
* OUTPUT  : twLock   - Pointer to a 2WayLock to be constructed and initialized
* RETURN  : RV_OK if successful otherwise an error code.
*/
RVCOREAPI
RvStatus Rv2WayLockConstructGlobal(
                      IN    RvLogMgr*		       logMgr,
                      INOUT Rv2WayLockGlobalLock*  gLock); 
              
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
                      IN RvLogMgr*		        logMgr);
                                                      
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
                        IN RvLogMgr*	  logMgr);


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
              IN RvLogMgr*   logMgr); 


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
                OUT Rv2WayLockCbInfo* cbInfo); 

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
               IN  RvLogMgr*	     logMgr); 


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
               INOUT Rv2WayLock**   repTwLock); 
                           
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
RvStatus Rv2WayLockSourceConstruct(IN RvLogMgr *logMgr);

#else /* #if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE) */ 

#define Rv2WayLockConstruct(logMgr,lockCfg,twLock)    (RV_OK)
#define Rv2WayLockDestruct(twLock,logMgr)             (RV_OK)
#define Rv2WayLockConstructGlobal(logMgr,gLock)       (RV_OK)
#define Rv2WayLockDestructGlobal(gLock,logMgr)        (RV_OK)
#define Rv2WayLockLock(twLock,cbCtx,isEvent,logMgr)	  (RV_OK)
#define Rv2WayLockUnlock(twLock,logMgr)
#define Rv2WayLockBeforeCb(twLock,logMgr,cbInfo)      (RV_OK)
#define Rv2WayLockAfterCb(twLock,cbInfo,logMgr)       (RV_OK)
#define Rv2WayLockReplace(newTwLock,logMgr,repTwLock) (RV_OK)
#define Rv2WayLockSourceConstruct(logMgr)             (RV_OK)

#endif /* #if (RV_THREADNESS_TYPE != RV_THREADNESS_SINGLE) */ 



#ifdef __cplusplus
}
#endif

#endif /* #ifndef RV_2_WAY_LOCK_H */
/* nl for Linux */
