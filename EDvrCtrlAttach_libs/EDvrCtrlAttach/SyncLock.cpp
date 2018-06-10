#include "stdafx.h"
#include "SyncLock.h"

#ifdef LINUX32
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SyncLock_CreateWithNamed(LOCK_HANDLE *hLock, const char *szName)
{
#ifdef LINUX32
#if(USED_TYPE == USED_SEMAPHORE)
    sem_init(hLock, 0, 1);
#else//if(USED_TYPE == USED_MUTEX)
    pthread_mutex_init(hLock, szName);
#endif
#endif//LINUX32

#ifdef WIN32
#if(USED_TYPE == USED_SEMAPHORE)
    *hLock = CreateSemaphore(NULL, 1, 1, szName);
#else//if(USED_TYPE == USED_MUTEX)
    *hLock = CreateMutex(NULL, FALSE, szName);
#endif
#endif//WIN32
}

void SyncLock_Destroy(LOCK_HANDLE *hLock)
{
    if (!SyncLock_Unlock(hLock))
        return;
#ifdef LINUX32
#if(USED_TYPE == USED_SEMAPHORE)
    sem_destroy(hLock);
#else//if(USED_TYPE == USED_MUTEX)
    pthread_mutex_destroy(hLock);
#endif
#endif//LINUX32

#ifdef WIN32
    CloseHandle(*hLock);
    *hLock = NULL;
#endif//WIN32
}

int SyncLock_Lock(LOCK_HANDLE *hLock, unsigned nWaitTime)
{
#ifdef LINUX32
#if(USED_TYPE == USED_SEMAPHORE)
    if (!sem_wait(hLock))
#else//if(USED_TYPE == USED_MUTEX)
    if (!pthread_mutex_lock(hLock))
#endif
        return TRUE;
    else
        return FALSE;
#endif//LINUX32

#ifdef WIN32
    if (*hLock != NULL)
    {
        DWORD nResult = WaitForSingleObject(*hLock, nWaitTime);
        if(WAIT_TIMEOUT == nResult)
        {
            return LOCK_OUTTIME;
        }
        else if (WAIT_FAILED != nResult)
        {
            return LOCK_WAITOK;
        }
    }
    return LOCK_FAILD;
#endif//WIN32
}

BOOL SyncLock_Unlock(LOCK_HANDLE *hLock)
{
    int nError = 0;
#ifdef LINUX32
#if(USED_TYPE == USED_SEMAPHORE)
    if (!sem_post(hLock))
#else//if(USED_TYPE == USED_MUTEX)
    if (!pthread_mutex_unlock(hLock))
#endif
        return TRUE;
    else
        return FALSE;
#endif//LINUX32

#ifdef WIN32
    if (*hLock != NULL)
    {
#if(USED_TYPE == USED_SEMAPHORE)
        nError = ReleaseSemaphore(*hLock, 1, NULL);
#else//if(USED_TYPE == USED_MUTEX)
        nError = ReleaseMutex(*hLock);
#endif
        if(nError)
            return TRUE;
        nError = GetLastError();
        if(nError != 6)
            return TRUE;
    }
    return FALSE;
#endif//WIN32
}

//////////////////////////////////////////////////////////////////////

void SyncEvent_CreateWithNamed(EVENT_HANDLE *hEvent, const char *szName)
{
#ifdef LINUX32
    pthread_mutex_init(&hEvent->m_hCondLock, NULL/* szName*/);
    pthread_cond_init(&hEvent->m_hCond, NULL/*, szName*/);
#endif//LINUX32

#ifdef WIN32
    *hEvent = CreateEvent(NULL, FALSE, FALSE, szName);
#endif//WIN32
}

void SyncEvent_Destroy(EVENT_HANDLE *hEvent)
{
    if(hEvent == NULL)
        return;
#ifdef LINUX32
    pthread_mutex_unlock(&hEvent->m_hCondLock);
    pthread_cond_destroy(&hEvent->m_hCond);
    pthread_mutex_destroy(&hEvent->m_hCondLock);
#endif//LINUX32
#ifdef WIN32
    CloseHandle(*hEvent);
    *hEvent = NULL;
#endif//WIN32
}

int SyncEvent_Wait(EVENT_HANDLE *hEvent, unsigned nMSec)
{
    int nRet = -1;
#ifdef LINUX32
    timespec dtWait;
    clock_gettime(CLOCK_REALTIME, &dtWait);

    nRet = pthread_mutex_lock(&hEvent->m_hCondLock);
    if (nRet < 0)
        return FALSE;
    if (nMSec == (unsigned)-1)
    {
        nRet = pthread_cond_wait(&hEvent->m_hCond, &hEvent->m_hCondLock);
    }
    else
    {
        dtWait.tv_sec += (nMSec/1000);
        dtWait.tv_nsec += (nMSec%1000*1000);
        nRet = pthread_cond_timedwait(&hEvent->m_hCond, &hEvent->m_hCondLock, &dtWait);
    }

    pthread_mutex_unlock(&hEvent->m_hCondLock);
#endif//LINUX32

#ifdef WIN32
    HANDLE hEventHandle = NULL;
    if (hEvent != NULL)
    {
        hEventHandle = *hEvent;
        nRet = WaitForSingleObject(hEventHandle, nMSec);
        switch(nRet)
        {
        case WAIT_ABANDONED: return FALSE;
        case WAIT_FAILED: return FALSE;
        case WAIT_OBJECT_0: return TRUE;
        case WAIT_TIMEOUT: return 110;
        }
        
    }
#endif//WIN32
    return nRet;
}

BOOL SyncEvent_Set(EVENT_HANDLE *hEvent)
{
#ifdef LINUX32
    pthread_cond_signal(&hEvent->m_hCond);
    return TRUE;
#endif//LINUX32

#ifdef WIN32
    return (SetEvent(*hEvent) == TRUE);
#endif//WIN32
}

#ifdef __cplusplus
}
#endif
//////////////////////////////////////////////////////////////////////////
// class CSyncLock
CSyncLock::CSyncLock()
{
	SyncLock_Create(&m_hLock);
}

CSyncLock::CSyncLock(const char *szName)
{
	SyncLock_CreateWithNamed(&m_hLock, szName);
}

CSyncLock::~CSyncLock()
{
	SyncLock_Destroy(&m_hLock);
}

int CSyncLock::Lock(unsigned nWaitTime/*= -1*/)
{
	return SyncLock_Lock(&m_hLock, nWaitTime);
}

BOOL CSyncLock::Unlock()
{
	return SyncLock_Unlock(&m_hLock);
}

//////////////////////////////////////////////////////////////////////////
// class CSyncEvent
CSyncEvent::CSyncEvent()
{
	SyncEvent_Create(&m_hEvent);
}

CSyncEvent::CSyncEvent(const char *szName)
{
	SyncEvent_CreateWithNamed(&m_hEvent, szName);
}

CSyncEvent::~CSyncEvent()
{
	SyncEvent_Destroy(&m_hEvent);
}

int CSyncEvent::WaitEvent(unsigned nWaitTime/*= -1*/)
{
	return SyncEvent_Wait(&m_hEvent, nWaitTime);
}

BOOL CSyncEvent::SetEvent()
{
	return SyncEvent_Set(&m_hEvent);
}
