#ifndef _FRAME_BUFFER_H__INCLUDE_
#define _FRAME_BUFFER_H__INCLUDE_
#pragma once

#include "SyncLock.h"
#include "ThreadPulse.h"

enum EBufferMode
{
    Mode_OnlyW = 1,     //只写
    Mode_OnlyR = 2,     //只读
    Mode_BothRW = 3,    //同时读写
};

enum EMemoryMode
{
    Memory_RAM,
    Memory_ROM,
};

typedef unsigned (__stdcall *fpOutputData)(void* objIndex, PBYTE pData, unsigned nSize, PBYTE pInfo, unsigned nInfoSize);

typedef struct _SShareInfo
{
    char    bResetWrite;	//写重置标志
    char    bResetRead;		//读重置标志
    char    bRes1[2];

    unsigned    nROffset;   //读偏移量
    unsigned    nWOffset;   //写偏移量

    char    bRLayer;        //读指针所在层
    char    bWLayer;        //写指针所在层
    char    bRes2[2];
}SShareInfo;

typedef struct _SFrameBuffer
{
    //缓冲区属性
    BYTE m_eIOMode;
    BYTE m_eMemoryMode;         //0-RAM, 1-ROM
    BYTE m_bCanWrite;
    BYTE m_bReadPause;
    void* m_objIndex;

    // 共享IO信息
    HANDLE m_hShareInfo;
    SShareInfo *m_pShareInfo;

    // 缓存信息
    HANDLE m_hMapBuffer;
    PBYTE m_pBufferBegin;

    //缓存操作
    PBYTE m_pBufferEnd;
    PBYTE m_pWrite;
    PBYTE m_pRead;
    int m_nSize;
    EVENT_HANDLE m_eventStartRead;

    LOCK_HANDLE m_lockSeek;
    LOCK_HANDLE m_lockWrite;
    LOCK_HANDLE m_lockReset;
    fpOutputData m_dgOutput;
    SThreadPulse m_sThreadReading;

}SFrameBuffer, *HFrameBuffer;

#ifdef __cplusplus
extern "C" {
#endif

int InitFrameBuffer(HFrameBuffer pInput);

#define FrameBuffer_Create(hBuffer, objIndex, nBuffSize, fCallback) \
        FrameBuffer_CreateWithNamed(hBuffer, objIndex, nBuffSize, fCallback, NULL, Mode_BothRW)
int FrameBuffer_CreateWithNamed(HFrameBuffer hBuffer, void* objIndex,\
        unsigned nBuffSize, fpOutputData fCallback, const char *szName, int eBufferMode);
void FrameBuffer_Release(HFrameBuffer hBuffer);
int FrameBuffer_Write(HFrameBuffer hBuffer, PBYTE pData, unsigned nSize, PBYTE pInfoData, unsigned nInfoSize);
int FrameBuffer_Read(HFrameBuffer hBuffer, fpOutputData fCallback);

BOOL FrameBuffer_ReSume(HFrameBuffer hBuffer);
void FrameBuffer_Pause(HFrameBuffer hBuffer);

#ifdef __cplusplus
};
#endif//__cplusplus


//////////////////////////////////////////////////////////////////////////
// 类方式描述
#ifdef __cplusplus

class CFrameBuffer : public SFrameBuffer
{
public:
    CFrameBuffer(void);
    ~CFrameBuffer(void);

    int InitBuffer(void* objIndex, unsigned nBuffSize, fpOutputData fCallback, const char *szName, int eBufferMode = Mode_BothRW);
    void UnInitBuffer();

    BOOL ReSume();
    void Pause();

    void WriteBuffer(PBYTE pData, unsigned nSize, PBYTE pInfoData, unsigned nInfoSize);
    void ReadBuffer(fpOutputData fCallback);
};

#endif//__cplusplus

#endif //_FRAME_BUFFER_H__INCLUDE_
