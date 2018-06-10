#ifndef _FRAME_BUFFER_H__INCLUDE_
#define _FRAME_BUFFER_H__INCLUDE_
#pragma once

#include "SyncLock.h"
#include "ThreadPulse.h"

enum EBufferMode
{
    Mode_OnlyW = 1,     //ֻд
    Mode_OnlyR = 2,     //ֻ��
    Mode_BothRW = 3,    //ͬʱ��д
};

enum EMemoryMode
{
    Memory_RAM,
    Memory_ROM,
};

typedef unsigned (__stdcall *fpOutputData)(void* objIndex, PBYTE pData, unsigned nSize, PBYTE pInfo, unsigned nInfoSize);

typedef struct _SShareInfo
{
    char    bResetWrite;	//д���ñ�־
    char    bResetRead;		//�����ñ�־
    char    bRes1[2];

    unsigned    nROffset;   //��ƫ����
    unsigned    nWOffset;   //дƫ����

    char    bRLayer;        //��ָ�����ڲ�
    char    bWLayer;        //дָ�����ڲ�
    char    bRes2[2];
}SShareInfo;

typedef struct _SFrameBuffer
{
    //����������
    BYTE m_eIOMode;
    BYTE m_eMemoryMode;         //0-RAM, 1-ROM
    BYTE m_bCanWrite;
    BYTE m_bReadPause;
    void* m_objIndex;

    // ����IO��Ϣ
    HANDLE m_hShareInfo;
    SShareInfo *m_pShareInfo;

    // ������Ϣ
    HANDLE m_hMapBuffer;
    PBYTE m_pBufferBegin;

    //�������
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
// �෽ʽ����
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
