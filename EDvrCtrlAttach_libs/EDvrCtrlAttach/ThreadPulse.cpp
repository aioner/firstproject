#include "stdafx.h"
#include "ThreadPulse.h"

#include <signal.h>

#ifdef LINUX32
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#define Sleep(a) usleep(a*1000)
#else
#endif//LINUX32


#ifdef __cplusplus
extern "C" {
#endif

int ThreadPulseInit(SThreadPulse * pInput)
{
    memset(pInput, 0, sizeof(SThreadPulse));
    pInput->m_bNoStart = TRUE;	//是否未启动    (TRUE)
    pInput->m_bIsPulse = TRUE;	//脉冲处理      (TRUE)
    return 0;//没有释放资源
}

void* CALLBACK ThreadFunction(void * pThreadinfo)
{
	SThreadPulse* pThread = (SThreadPulse*)pThreadinfo;
    int dwRet = 0;
    pThread->m_bNoStart = 0;
    //WriteLog(c_szFlag, "thread:%d start", pThread->m_nThreadFlag);
    //线程启动
    if (pThread->m_OnStateEvent)
    {
        pThread->m_OnStateEvent(pThread->m_nThreadFlag, PULSE_THREAD_START, pThread->m_objUser);
    }
    while (!pThread->m_bNoStart)
    {
        //WriteLog(c_szFlag, "thread:%d wait", pThread->m_nThreadFlag);
        dwRet = SyncEvent_Wait(&(pThread->m_hEvent), pThread->m_nWaitTime);
        if (pThread->m_bNoStart)//手动退出
        {
            //WriteLog(c_szFlag, "thread:%d quite by custom", pThread->m_nThreadFlag);
            if (pThread->m_OnStateEvent)
            {
                pThread->m_OnStateEvent(pThread->m_nThreadFlag, PULSE_THREAD_STOP, pThread->m_objUser);
            }
            continue;
        }
        if (dwRet == 110)//等待超时
        {
            //WriteLog(c_szFlag, "thread:%d wait timeout", pThread->m_nThreadFlag);
            if (pThread->m_OnStateEvent)
            {
                pThread->m_OnStateEvent(pThread->m_nThreadFlag, PULSE_THREAD_WAITE_TIMEOUT, pThread->m_objUser);
            }
            continue;
        }
        //手动触发
        if (!pThread->m_lpPluseFunc)
            continue;

        pThread->m_bIsPulse = FALSE;
        //WriteLog(c_szFlag, "thread:%d impluse", pThread->m_nThreadFlag);
        if (pThread->m_lpPluseFunc(pThread->m_nThreadFlag, pThread->m_objUser))
            pThread->m_bIsPulse = TRUE;
        else
            break;
    }
    //线程自动退出
    SyncEvent_Destroy(&pThread->m_hEvent);
    ThreadPulseInit(pThread);

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
//

int ThreadPulse_StartWithNamed(SThreadPulse *pThread, UINT_PTR nThreadFlag, const char *szName, fThreadPulse fPulse,
                             fThreadStateEvent pState, unsigned nWaitTime, void* objUser)
{
    //记录参数
    pThread->m_nThreadFlag = nThreadFlag;
    pThread->m_lpPluseFunc = fPulse;
    pThread->m_OnStateEvent = pState;
    if (!nWaitTime)
        nWaitTime = (unsigned)-1;
    pThread->m_nWaitTime = nWaitTime;
    pThread->m_objUser = objUser;

    //判断存在性
    if (pThread->m_hThread != 0)
    {
        if (pThread->m_OnStateEvent)
            pThread->m_OnStateEvent(nThreadFlag, PULSE_THREAD_EXISTED, objUser);
        return -1;
    }
    //脉冲事件、退出事件
    SyncEvent_CreateWithNamed(&pThread->m_hEvent, szName);

#ifdef LINUX32
    pthread_create(&pThread->m_hThread, NULL, &ThreadFunction, pThread);
    usleep(1000);
#endif//LINUX32
#ifdef WIN32
    pThread->m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunction, pThread, 0, NULL);
    Sleep(1);
#endif//WIN32
    return 0;

}

void ThreadPulse_Stop(SThreadPulse * pThread)
{
    if(pThread->m_hThread == 0)
    {
        return;
    }

    pThread->m_nWaitTime = (unsigned)-1;
    pThread->m_bNoStart = TRUE;
    SyncEvent_Set(&pThread->m_hEvent);

    //等待线程结束
#ifdef LINUX32
    pthread_join(pThread->m_hThread, NULL);
#endif//LINUX32
#ifdef WIN32
    WaitForSingleObject(pThread->m_hThread, (unsigned)-1);
#endif//WIN32

    //释放资源
    pThread->m_hThread = 0;
    SyncEvent_Destroy(&pThread->m_hEvent);
    ThreadPulseInit(pThread);
}

void ThreadPulse_Impulse(SThreadPulse * pThread)
{
    //WriteLog(c_szFlag, "thread:%d impulse:%d", pThread->m_nThreadFlag, pThread->m_bIsPulse);
    if (pThread->m_bIsPulse)
    {
        SyncEvent_Set(&pThread->m_hEvent);
    }
}

BOOL ThreadPulse_IsPulseEnd(SThreadPulse * pThread)
{
    return pThread->m_bIsPulse;
}

void ThreadPulse_WaitPulseEnd(SThreadPulse *pThread)
{
    int i;
    for(i=0; i<3000 && !pThread->m_bIsPulse; i++)
    {
        Sleep(1);
    }
    Sleep(10);
}

#ifdef __cplusplus
}
#endif

CThreadPulse::CThreadPulse()
{
    ThreadPulseInit(this);
}

CThreadPulse::~CThreadPulse()
{
    ThreadPulse_Stop(this);
    ThreadPulseInit(this);
}

int CThreadPulse::Start(UINT_PTR nThreadFlag, fThreadPulse fPulse, fThreadStateEvent pState, unsigned nWaitTime, void* objUser)
{
    return ThreadPulse_Start(this, nThreadFlag, fPulse, pState, nWaitTime, objUser);
}

int CThreadPulse::Start(UINT_PTR nThreadFlag, const char *szName, fThreadPulse fPulse, fThreadStateEvent pState, unsigned nWaitTime, void* objUser)
{
    return ThreadPulse_StartWithNamed(this, nThreadFlag, szName, fPulse, pState, nWaitTime, objUser);
}

void CThreadPulse::Stop()
{
    ThreadPulse_Stop(this);
}

void CThreadPulse::Impulse()
{
    ThreadPulse_Impulse(this);
}

BOOL CThreadPulse::IsPulseEnd()
{
    return ThreadPulse_IsPulseEnd(this);
}

void CThreadPulse::WaitPulseEnd()
{
    ThreadPulse_WaitPulseEnd(this);
}
