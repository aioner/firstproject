#include "StdAfx.h"
#include "FrameBuffer.h"
#include "WriteLog.h"

#ifdef __cplusplus
extern "C" {
#endif

int InitFrameBuffer(HFrameBuffer pInput)
{
    memset(pInput, 0, sizeof(SFrameBuffer));
    ThreadPulseInit(&pInput->m_sThreadReading);
    return 0;
}

void* CreateMapBuffer(HANDLE hFile, int nBufferSize, const char *szName, HANDLE* hMapping)
{
    assert(hMapping != NULL);
    if(*hMapping == NULL)
        *hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, nBufferSize, szName);

    if(*hMapping == NULL)
    {
        //TRACE("创建[%s]共享内存操作失败", szName);
        return NULL;
    }

    return MapViewOfFile(hMapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
int FrameBuffer_AllocRAM(HFrameBuffer hBuffer);
int FrameBuffer_AllocROM(HFrameBuffer hBuffer, const char *szName, int eBufferMode);

int FrameBuffer_FreeMemory(HFrameBuffer hBuffer);

int FrameBuffer_CreateReadThread(HFrameBuffer hBuffer, const char *szName);

int FrameBuffer_ResetReader(HFrameBuffer hBuffer, BOOL bSyncRW);
int FrameBuffer_ResetWriter(HFrameBuffer hBuffer, BOOL bSyncRW);

BOOL CALLBACK ReadPulse(UINT_PTR m_nThreadFlag, void* objUser);
void CALLBACK ReadStateEvent(UINT_PTR m_nThreadFlag, unsigned nStateID, void* objUser);

//////////////////////////////////////////////////////////////////////////

int FrameBuffer_CreateWithNamed(HFrameBuffer hBuffer, void* objIndex,\
        unsigned nBuffSize, fpOutputData fCallback, const char *szName, int eBufferMode)
{
    int nRet = -1;

    hBuffer->m_objIndex = objIndex;
    hBuffer->m_dgOutput = fCallback;
    hBuffer->m_nSize = nBuffSize;
    hBuffer->m_eIOMode = eBufferMode;

    if(!szName || !szName[0])//内存模式
    {
        hBuffer->m_eMemoryMode = Memory_RAM;
        nRet = FrameBuffer_AllocRAM(hBuffer);
    }
    else//映射内存模式
    {
        hBuffer->m_eMemoryMode = Memory_ROM;
        nRet = FrameBuffer_AllocROM(hBuffer, szName, eBufferMode);
    }
    hBuffer->m_pBufferEnd = hBuffer->m_pBufferBegin + nBuffSize;

    if(nRet < 0)
        return -1;

    if(eBufferMode & Mode_OnlyR)    //初始化读缓存
    {
        FrameBuffer_ResetReader(hBuffer, FALSE);
        FrameBuffer_CreateReadThread(hBuffer, szName);
    }
    if(eBufferMode & Mode_OnlyW)    //初始化读缓存
    {
        FrameBuffer_ResetWriter(hBuffer, FALSE);
        hBuffer->m_bCanWrite = TRUE;
    }

    if(!hBuffer->m_pWrite)   //初始写指针
        hBuffer->m_pWrite = hBuffer->m_pBufferBegin+hBuffer->m_pShareInfo->nWOffset;
    if(!hBuffer->m_pRead)    //初始读指针
        hBuffer->m_pRead = hBuffer->m_pBufferBegin+hBuffer->m_pShareInfo->nROffset;
    return 0;
}

void FrameBuffer_Release(HFrameBuffer hBuffer)
{
    SyncLock_Lock(&hBuffer->m_lockWrite, -1);
    hBuffer->m_bCanWrite = FALSE;
    SyncLock_Unlock(&hBuffer->m_lockWrite);

    //停止输出线程，释放输出控制事件
    if(hBuffer->m_eventStartRead != hBuffer->m_sThreadReading.m_hEvent)
    {
        CloseHandle(hBuffer->m_eventStartRead);
        hBuffer->m_eventStartRead = NULL;
    }
    else
    {
        hBuffer->m_bReadPause = TRUE;
        ThreadPulse_Stop(&hBuffer->m_sThreadReading);
        hBuffer->m_eventStartRead = NULL;
    }

    FrameBuffer_FreeMemory(hBuffer);

	InitFrameBuffer(hBuffer);
}

//起始信息为一个字节内容如下
//  (0XF0) 后面紧跟原先的数据信息
//	(0XF1) 原先的数据信息在缓冲区的起始位置(已经循环了)

//判断缓冲区的剩余空间大小是否够用, 如果不够用则不再写入数据
//已经写到的位置+将要写入的内容的长度: (保存数据长度的空间(4字节)+数据长度+保存信息长度的空间(4字节)+信息长度)
//看是否超出了整个缓冲的大小
//如果不够写入新数据时，先在当前位置写一个循环标志，然后再移动到缓冲区头开始写入数据

/*	m_nWritePos
		|
|-------------------------------------------------------------------------------|
		|起始信息(1字节)|4字节|数据长度|4字节|信息长度|循环标志(1字节)|

|<-----------------------------   整个缓冲区的大小 ---------------------------->|
*/
int FrameBuffer_Write(HFrameBuffer hBuffer, PBYTE pData, unsigned nSize, PBYTE pInfoData, unsigned nInfoSize)
{
    PBYTE pCurPos;

    SyncLock_Lock(&hBuffer->m_lockWrite, -1);

	if((!hBuffer->m_bCanWrite)||((signed)nSize >= hBuffer->m_nSize))
    {
        //TRACE("当前缓冲区不允许写入\n");
        SyncLock_Unlock(&hBuffer->m_lockWrite);
		return -1;
    }

	if(NULL != hBuffer->m_pWrite)
	{
WriteData:
		FrameBuffer_ResetWriter(hBuffer, TRUE);

		pCurPos = hBuffer->m_pWrite+nSize+nInfoSize+9;
		if(pCurPos < hBuffer->m_pBufferEnd)
        {
            //读写指针不在同一层时，如果写入新的数据后会超过读指针，当前的写操作就不再进行
			SyncLock_Lock(&hBuffer->m_lockSeek, -1);

			if( (hBuffer->m_pShareInfo->bWLayer != hBuffer->m_pShareInfo->bRLayer)		//读写指针不在同一层
				&&(pCurPos > (hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nROffset)))		//写入新数据后会超过读指针
			{
                //TRACE("写入速度太快,已经追上了读指针, 写入数据后写指针位置:%d, 写指针位置:%d, 读指针位置:%d, 当前要写入的数据:%x\n",
				//	pCurPos, hBuffer->m_pWrite, (hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nROffset), pData);
				SyncLock_Unlock(&hBuffer->m_lockSeek);

				return -1;
			}
			SyncLock_Unlock(&hBuffer->m_lockSeek);

			//写入起始标识
			*(hBuffer->m_pWrite) = 0XF0;			//保存起始位置
			hBuffer->m_pWrite += 1;

			//写入数据的长度值
			memcpy(hBuffer->m_pWrite, (void*)&nSize, sizeof(unsigned));//将此后4个字节重置，用来保存要写入数据的长度
			hBuffer->m_pWrite += sizeof(unsigned);

			//写入数据
			if (nSize>0)
			{
				memcpy(hBuffer->m_pWrite, pData, nSize);
				hBuffer->m_pWrite += nSize;
			}

			//写入信息长度值
            memcpy(hBuffer->m_pWrite, (void*)&nInfoSize, sizeof(unsigned));//将此后4个字节重置，用来保存要附加信息的长度
            hBuffer->m_pWrite += sizeof(unsigned);

			//写入信息数据
			if(nInfoSize>0)
			{
				memcpy(hBuffer->m_pWrite, pInfoData, nInfoSize);
				hBuffer->m_pWrite += nInfoSize;
			}

			//保存写偏移值
			SyncLock_Lock(&hBuffer->m_lockSeek, -1);
			hBuffer->m_pShareInfo->nWOffset = hBuffer->m_pWrite - hBuffer->m_pBufferBegin;
            SyncLock_Unlock(&hBuffer->m_lockSeek);

			FrameBuffer_ResetWriter(hBuffer, TRUE);
		}
		else	//从头开始写新数据
        {
            //TRACE("剩余空间不够, 从头开始写新数据, 写指针位置:%d, 读指针位置:%d, 写入数据后写指针位置:%d, 缓冲区:%d~%d, 当前要写入数据:%x\n",
			//	hBuffer->m_pWrite, hBuffer->m_pRead, pCurPos, hBuffer->m_pBufferBegin, hBuffer->m_pBufferEnd, pData);

			*(hBuffer->m_pWrite) = 0XF1;			//在末尾保存一个重置到缓冲头的标志
			hBuffer->m_pWrite = hBuffer->m_pBufferBegin;

			SyncLock_Lock(&hBuffer->m_lockSeek, -1);
			hBuffer->m_pShareInfo->nWOffset = 0;
			hBuffer->m_pShareInfo->bWLayer = !hBuffer->m_pShareInfo->bWLayer;
			SyncLock_Unlock(&hBuffer->m_lockSeek);

			goto WriteData;
		}
	}

	//发事件通知读数据
	if(NULL != hBuffer->m_eventStartRead)
		SyncEvent_Set(&hBuffer->m_eventStartRead);
    //ThreadPulse_Impulse(&hBuffer->m_sThreadReading);

//WriteEnd:
    SyncLock_Unlock(&hBuffer->m_lockWrite);

    return nSize + nInfoSize;
}

int FrameBuffer_Read(HFrameBuffer hBuffer, fpOutputData fCallback)
{
	PBYTE pWriteBuffer = NULL;
	PBYTE pAlReadPos = NULL;
	PBYTE pData = NULL;
	PBYTE pInfo = NULL;
	unsigned nDataSize = 0;
	unsigned nInfoSize = 0;

	if(hBuffer->m_bReadPause)
		return 0;
	FrameBuffer_ResetReader(hBuffer, TRUE);

	//在同一层中如果读指针追上了写指针，则不再继续读取数据
	SyncLock_Lock(&hBuffer->m_lockSeek, -1);
	if(hBuffer->m_pShareInfo->bRLayer == hBuffer->m_pShareInfo->bWLayer)					//读写指针在同一层
	{
		pWriteBuffer = hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nWOffset;

		//读指针等于写指针,说明缓冲区的数据已经读完,此次操作作废,等待下一个读数据事件
		if(pWriteBuffer == hBuffer->m_pRead)
		{
			SyncLock_Unlock(&hBuffer->m_lockSeek);
			return -1;
		}

		//在同一层读指针超过了写指针，说明在读的过程中写指针被修改了（一般是重置操作导致），
		//此时将读指针进行重置操作
		if(hBuffer->m_pRead > pWriteBuffer)
		{
			//TRACE("在同一层读指针超过了写指针 读指针位置:%d, 写指针位置:%d\n", hBuffer->m_pRead, pWriteBuffer);
			hBuffer->m_pRead = hBuffer->m_pBufferBegin;
		}
	}
	SyncLock_Unlock(&hBuffer->m_lockSeek);

	switch(*hBuffer->m_pRead)	//判断第一个字节的内容
	{
	case 0XF0:	//继续向后读取数据
		pAlReadPos = hBuffer->m_pRead;	//得到当前读指针的起始位置(用于后面计算读偏移量)
		hBuffer->m_pRead += 1;

		//得到数据以及数据长度
		memcpy(&nDataSize, hBuffer->m_pRead, sizeof(unsigned));
		hBuffer->m_pRead += sizeof(unsigned);

		pData = hBuffer->m_pRead;
		hBuffer->m_pRead += nDataSize;

        //得到附属信息以及附属信息长度
		memcpy(&nInfoSize, hBuffer->m_pRead, sizeof(unsigned));
		hBuffer->m_pRead += sizeof(unsigned);

		pInfo = hBuffer->m_pRead;
		hBuffer->m_pRead += nInfoSize;

		//将数据以及附属的信息通过回调函数输出
		if(fCallback)
		{
			fCallback(hBuffer->m_objIndex, pData, nDataSize, pInfo, nInfoSize);
		}

		//TRACE("读取数据, 数据内容:%x, 起始位置:%d, 结束位置:%d, 当前写指针位置:%d, 缓冲区范围:%d~%d\n",
		//	pData, pAlReadPos, hBuffer->m_pRead, (hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nWOffset),\
		//	hBuffer->m_pBufferBegin, hBuffer->m_pBufferEnd);

		//保存读偏移量
		SyncLock_Lock(&hBuffer->m_lockSeek, -1);
		hBuffer->m_pShareInfo->nROffset = hBuffer->m_pRead - hBuffer->m_pBufferBegin;
		SyncLock_Unlock(&hBuffer->m_lockSeek);
		return nDataSize;

	case 0XF1:	//跳到缓冲区起始处读取数据
		hBuffer->m_pRead = hBuffer->m_pBufferBegin;

		SyncLock_Lock(&hBuffer->m_lockSeek, -1);
		hBuffer->m_pShareInfo->bRLayer = !hBuffer->m_pShareInfo->bRLayer;	//读切换到另一层
		hBuffer->m_pShareInfo->nROffset = 0;
		SyncLock_Unlock(&hBuffer->m_lockSeek);

		return 0;

	default: return -1;
	}
	return -1;
}

BOOL FrameBuffer_ReSume(HFrameBuffer hBuffer)
{
    return FALSE;
}

void FrameBuffer_Pause(HFrameBuffer hBuffer)
{

}

//////////////////////////////////////////////////////////////////////////
int FrameBuffer_AllocRAM(HFrameBuffer hBuffer)
{
    hBuffer->m_pShareInfo = (SShareInfo*)malloc(sizeof(SShareInfo));
    // 创建信息页面
    if(!hBuffer->m_pShareInfo)
    {
        //TRACE("申请内存操作失败\n");
        return -1;
    }

    // 创建缓存页面
    hBuffer->m_pBufferBegin = (PBYTE)malloc(hBuffer->m_nSize);
    if(!hBuffer->m_pBufferBegin)
    {
        //TRACE("申请内存操作失败\n");
        return -1;
    }

    SyncLock_Create(&hBuffer->m_lockSeek);
    SyncLock_Create(&hBuffer->m_lockWrite);
    SyncLock_Create(&hBuffer->m_lockReset);

    return 0;
}

int FrameBuffer_AllocROM(HFrameBuffer hBuffer, const char *szName, int eBufferMode)
{
    char strObjName[256] = {0};

    // 创建信息页面映射
    sprintf(strObjName, "%s_ShareInfo", szName);
    hBuffer->m_pShareInfo = (SShareInfo*)CreateMapBuffer(INVALID_HANDLE_VALUE, sizeof(SShareInfo), strObjName, &hBuffer->m_hShareInfo);
    if(!hBuffer->m_pShareInfo)
    {
        //TRACE("创建[%s]共享内存操作失败\n", szName);
        return -1;
    }

    // 创建缓存页面映射
    sprintf(strObjName, "%s_ShareBuff", szName);
    hBuffer->m_pBufferBegin = (PBYTE)CreateMapBuffer(INVALID_HANDLE_VALUE, hBuffer->m_nSize, strObjName, &hBuffer->m_hMapBuffer);
    if(!hBuffer->m_pBufferBegin)
    {
        //TRACE("创建[%s]共享内存操作失败\n", szName);
        return -1;
    }

    sprintf(strObjName, "%s_OffSet", szName);
    SyncLock_CreateWithNamed(&hBuffer->m_lockSeek, strObjName);

    sprintf(strObjName, "%s_Write", szName);
    SyncLock_CreateWithNamed(&hBuffer->m_lockWrite, strObjName);

    sprintf(strObjName, "%s_Reset", szName);
    SyncLock_CreateWithNamed(&hBuffer->m_lockReset, strObjName);

    return 0;
}

int FrameBuffer_FreeMemory(HFrameBuffer hBuffer)
{
    //释放缓冲区
    if(hBuffer->m_eMemoryMode == Memory_RAM)
    {
        free(hBuffer->m_pBufferBegin);
        free(hBuffer->m_pShareInfo);
    }
    else if(hBuffer->m_eMemoryMode == Memory_ROM)
    {
		UnmapViewOfFile(hBuffer->m_pBufferBegin);
		UnmapViewOfFile(hBuffer->m_pShareInfo);
		CloseHandle(hBuffer->m_hMapBuffer);
		CloseHandle(hBuffer->m_hShareInfo);
    }
	hBuffer->m_pShareInfo = NULL;
    hBuffer->m_pBufferBegin = NULL;

    // 释放锁
    SyncLock_Destroy(&hBuffer->m_lockSeek);
    SyncLock_Destroy(&hBuffer->m_lockWrite);
    SyncLock_Destroy(&hBuffer->m_lockReset);

    return 0;
}

int FrameBuffer_CreateReadThread(HFrameBuffer hBuffer, const char *szName)
{
    if(hBuffer->m_eIOMode != Mode_OnlyW)
    {
        sprintf(hBuffer->m_sThreadReading.m_szFlag, "帧缓冲区");
        ThreadPulse_StartWithNamed(&hBuffer->m_sThreadReading, (UINT_PTR)hBuffer->m_objIndex, szName, ReadPulse, ReadStateEvent, -1, hBuffer);
        hBuffer->m_eventStartRead = hBuffer->m_sThreadReading.m_hEvent;
    }
    else
    {
        if(hBuffer->m_eMemoryMode == Memory_RAM)
        {
           hBuffer->m_eventStartRead = hBuffer->m_sThreadReading.m_hEvent;
        }
        else if(hBuffer->m_eMemoryMode == Memory_ROM)
        {
            hBuffer->m_eventStartRead = CreateEvent(NULL, FALSE, FALSE, szName);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

int FrameBuffer_ResetReader(HFrameBuffer hBuffer, BOOL bSyncRW)
{
    BOOL bRet = FALSE;

    SyncLock_Lock(&hBuffer->m_lockReset, -1);
    if(hBuffer->m_pShareInfo->bResetRead)
    {
        hBuffer->m_pShareInfo->bResetRead = FALSE;

        SyncLock_Lock(&hBuffer->m_lockSeek, -1);

        //如果还未重置写偏移（发送端还未发送数据）则先将其重置

        //将读写指针都移动到缓冲区起始处
        if(!bSyncRW)	//只有主动操作时才对写指针重置
        {
            hBuffer->m_pShareInfo->nWOffset = 0;
            hBuffer->m_pWrite = hBuffer->m_pBufferBegin;
        }

        hBuffer->m_pShareInfo->nROffset = 0;
        hBuffer->m_pRead = hBuffer->m_pBufferBegin;

        //将读写指针设置为同一层
        hBuffer->m_pShareInfo->bWLayer = hBuffer->m_pShareInfo->bRLayer;

        //TRACE("输出重置 写同步标志:%d 写指针位置:%d 写偏移:%d 读指针位置:%d 读偏移:%d\n",
        //  bSyncRW, hBuffer->m_pWrite, hBuffer->m_pShareInfo->nWOffset, hBuffer->m_pRead, hBuffer->m_pShareInfo->nROffset);

        SyncLock_Unlock(&hBuffer->m_lockSeek);
        bRet = TRUE;
    }
    SyncLock_Unlock(&hBuffer->m_lockReset);

    return bRet;
}

int FrameBuffer_ResetWriter(HFrameBuffer hBuffer, BOOL bSyncRW)
{
    SyncLock_Lock(&hBuffer->m_lockReset, -1);
    if(hBuffer->m_pShareInfo->bResetWrite)
    {
        hBuffer->m_pShareInfo->bResetWrite = FALSE;

        SyncLock_Lock(&hBuffer->m_lockSeek, -1);

        //将读写指针都移动到缓冲区起始处
        hBuffer->m_pShareInfo->nWOffset = 0;
        hBuffer->m_pWrite = hBuffer->m_pBufferBegin;

        //只有主动操作时才对读操作进行重置
        if(!bSyncRW)
        {
            hBuffer->m_pShareInfo->nROffset = 0;
            hBuffer->m_pRead = hBuffer->m_pBufferBegin;
        }

        //将读写指针设置为同一层
        hBuffer->m_pShareInfo->bWLayer = hBuffer->m_pShareInfo->bRLayer;

        //TRACE("输入重置 读同步标志:%d 写指针位置:%d 写偏移:%d 读指针位置:%d 读偏移:%d\n",
        //    bSyncRW, hBuffer->m_pWrite, hBuffer->m_pShareInfo->nWOffset, hBuffer->m_pRead, hBuffer->m_pShareInfo->nROffset);

        SyncLock_Unlock(&hBuffer->m_lockSeek);
    }
    SyncLock_Unlock(&hBuffer->m_lockReset);
    return 0;
}

BOOL CALLBACK ReadPulse(UINT_PTR m_nThreadFlag, void* objUser)
{
	HFrameBuffer hBuffer = (HFrameBuffer)objUser;
	//从读指针指向的缓冲区的头部进行读出数据操作
	//直到将所有的数据读完为止
	while(hBuffer->m_pRead && !hBuffer->m_bReadPause)
	{
		if(FrameBuffer_Read(hBuffer, hBuffer->m_dgOutput) < 0)
			return TRUE;
		Sleep(0);
	}

	return TRUE;
}

void CALLBACK ReadStateEvent(UINT_PTR m_nThreadFlag, unsigned nStateID, void* objUser)
{

}

#ifdef __cplusplus
};
#endif//__cplusplus

//////////////////////////////////////////////////////////////////////////
CFrameBuffer::CFrameBuffer(void)
{
    InitFrameBuffer(this);
}

CFrameBuffer::~CFrameBuffer(void)
{
}

int CFrameBuffer::InitBuffer(void* objIndex, unsigned nBuffSize, fpOutputData fCallback, const char *szName, int eBufferMode)
{
    return FrameBuffer_CreateWithNamed(this, objIndex, nBuffSize, fCallback, szName, eBufferMode);
}

void CFrameBuffer::UnInitBuffer()
{
    FrameBuffer_Release(this);
}

BOOL CFrameBuffer::ReSume()
{
    return FrameBuffer_ReSume(this);
}

void CFrameBuffer::Pause()
{
    FrameBuffer_Pause(this);
}

void CFrameBuffer::WriteBuffer(PBYTE pData, unsigned nSize, PBYTE pInfoData, unsigned nInfoSize)
{
    FrameBuffer_Write(this, pData, nSize, pInfoData, nInfoSize);
}

void CFrameBuffer::ReadBuffer(fpOutputData fCallback)
{
    FrameBuffer_Read(this, fCallback);
}
