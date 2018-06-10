#ifndef _CTHREAD_PULSE_H__INCLUDE_
#define _CTHREAD_PULSE_H__INCLUDE_
#pragma once

#include "SyncLock.h"

typedef enum _EThreadStateID
{
    PULSE_THREAD_START,			//�߳̿���
    PULSE_THREAD_STOP,			//�߳�ֹͣ
    PULSE_THREAD_EXISTED,		//�߳��Ѵ���
    PULSE_THREAD_WAITE_TIMEOUT,	//�̵߳ȴ���ʱ
}EThreadStateID;

//����false���˳��߳�
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
    short		m_bNoStart;		//�Ƿ�����	(false)
    short       m_bIsPulse;		//���崦��	(true)
    unsigned    m_nWaitTime;	//�ȴ�ʱ��	(0)
    UINT_PTR    m_nThreadFlag;	//��־		(0)
    void*       m_objUser;		//������	(0)

    THREAD_HANDLE m_hThread;    //���		(INVALID_HANDLE_VALUE)
    EVENT_HANDLE m_hEvent;

	char        m_szFlag[128];
    fThreadPulse	m_lpPluseFunc;	//���庯��	(NULL)
    fThreadStateEvent m_OnStateEvent;//״̬�¼� (NULL)
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
