#ifndef _CTHREAD_PULSE_H__INCLUDE_
#define _CTHREAD_PULSE_H__INCLUDE_
#pragma once

#include "SyncLock.h"

typedef enum _EThreadStateID
{
    PULSE_THREAD_START,			//线程开启
    PULSE_THREAD_STOP,			//线程停止
    PULSE_THREAD_EXISTED,		//线程已存在
    PULSE_THREAD_WAITE_TIMEOUT,	//线程等待超时
}EThreadStateID;

//返回false将退出线程
typedef BOOL (CALLBACK* fThreadPulse)(UINT_PTR m_nThreadFlag, void* objUser);
typedef void (CALLBACK* fThreadStateEvent)(UINT_PTR m_nThreadFlag, unsigned nStateID, void* objUser);

#ifdef LINUX32
    typedef pthread_t THREAD_HANDLE;
    typedef pthread_mutex_t MUTEX_HANDLE;
#endif//LINUX32

#ifdef WIN32
    typedef void* THREAD_HANDLE;
    typedef void* MUTEX_HANDLE;
#endif//WIN32

typedef struct _SThreadPulse
{
    short		m_bNoStart;		//是否启动	(false)
    short       m_bIsPulse;		//脉冲处理	(true)
    unsigned    m_nWaitTime;	//等待时间	(0)
    UINT_PTR    m_nThreadFlag;	//标志		(0)
    void*       m_objUser;		//上下文	(0)

    THREAD_HANDLE m_hThread;    //句柄		(INVALID_HANDLE_VALUE)
    EVENT_HANDLE m_hEvent;

	char        m_szFlag[128];
    fThreadPulse	m_lpPluseFunc;	//脉冲函数	(NULL)
    fThreadStateEvent m_OnStateEvent;//状态事件 (NULL)
}SThreadPulse;

#ifdef __cplusplus
extern "C" {
#endif

int ThreadPulseInit(SThreadPulse *pInput);

#define ThreadPulse_Start(pThread, nThreadFlag, fPulse, pState, nWaitTime, objUser) \
    ThreadPulse_StartWithNamed(pThread, nThreadFlag, NULL, fPulse, pState, nWaitTime, objUser)
int ThreadPulse_StartWithNamed(SThreadPulse *pThread, UINT_PTR nThreadFlag, const char *szName, fThreadPulse fPulse,
                      fThreadStateEvent pState, unsigned nWaitTime, void* objUser);
void ThreadPulse_Stop(SThreadPulse *pThread);
BOOL ThreadPulse_IsPulseEnd(SThreadPulse *pThread);
void ThreadPulse_Impulse(SThreadPulse *pThread);
void ThreadPulse_WaitPulseEnd(SThreadPulse *pThread);//Block

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class CThreadPulse : public _SThreadPulse
{
public:
    CThreadPulse();
    ~CThreadPulse();
    int  Start(UINT_PTR nThreadFlag, fThreadPulse fPulse, fThreadStateEvent pState,\
        unsigned nWaitTime = 0, void* objUser = 0);
    int  Start(UINT_PTR nThreadFlag, const char *szName, fThreadPulse fPulse, fThreadStateEvent pState,\
        unsigned nWaitTime = 0, void* objUser = 0);
    void Stop();
    BOOL IsPulseEnd();
    void Impulse();
    void WaitPulseEnd();
};

#endif
#endif//_CTHREAD_PULSE_H__INCLUDE_
