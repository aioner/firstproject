#ifndef _CLIN32_SYNC_LOCK_H__INCLUDE_
#define _CLIN32_SYNC_LOCK_H__INCLUDE_
#pragma once

#ifdef LINUX32
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#endif//LINUX32

#define USED_SEMAPHORE 0
#define USED_MUTEX 1
#define USED_TYPE USED_SEMAPHORE

#ifdef WIN32
typedef void* LOCK_HANDLE;
typedef void* EVENT_HANDLE; 
#endif//WIN32

#ifdef LINUX32

#if(USED_TYPE == USED_SEMAPHORE)
typedef sem_t LOCK_HANDLE;
#else//if(USED_TYPE == USED_MUTEX)
typedef pthread_mutex_t LOCK_HANDLE;
#endif

struct SEventHandle 
{
    pthread_mutex_t m_hCondLock;
    pthread_cond_t m_hCond;
};
typedef struct SEventHandle EVENT_HANDLE;

#endif//LINUX32

#ifdef __cplusplus
extern "C" {
#endif

// 返回0异常 返回1等待完成 返回2等待超时
typedef enum _ELockResult
{
    LOCK_FAILD = 0,
    LOCK_WAITOK,
    LOCK_OUTTIME,
}ELockResult;

#define SyncLock_Create(hLock) SyncLock_CreateWithNamed(hLock, NULL)
    void SyncLock_CreateWithNamed(LOCK_HANDLE *hLock, const char *szName);
    void SyncLock_Destroy(LOCK_HANDLE *hLock);
    // 返回0异常 返回1等待完成 返回2等待超时
    int SyncLock_Lock(LOCK_HANDLE *hLock, unsigned nWaitTime);
    BOOL SyncLock_Unlock(LOCK_HANDLE *hLock);

#define SyncEvent_Create(hEvent) SyncEvent_CreateWithNamed(hEvent, NULL)
    void SyncEvent_CreateWithNamed(EVENT_HANDLE *hEvent, const char *szName);
    void SyncEvent_Destroy(EVENT_HANDLE *hEvent);
    int SyncEvent_Wait(EVENT_HANDLE *hEvent, unsigned nMSec);
    BOOL SyncEvent_Set(EVENT_HANDLE *hEvent);

#ifdef __cplusplus
}

class CSyncLock
{
    LOCK_HANDLE m_hLock;
public:
    CSyncLock();
    CSyncLock(const char *szName);
    ~CSyncLock();
    // 返回0异常 返回1等待完成 返回2等待超时
    int Lock(unsigned nWaitTime = -1);
    BOOL Unlock();
};

class CSyncEvent
{
    EVENT_HANDLE m_hEvent;
public:
    CSyncEvent();
    CSyncEvent(const char *szName);
    ~CSyncEvent();
    int WaitEvent(unsigned nWaitTime = -1);
    BOOL SetEvent();
};


#endif

#endif//_CLIN32_SYNC_LOCK_H__INCLUDE_
