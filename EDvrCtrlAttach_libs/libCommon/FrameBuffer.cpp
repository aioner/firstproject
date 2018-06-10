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
        //TRACE("����[%s]�����ڴ����ʧ��", szName);
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

    if(!szName || !szName[0])//�ڴ�ģʽ
    {
        hBuffer->m_eMemoryMode = Memory_RAM;
        nRet = FrameBuffer_AllocRAM(hBuffer);
    }
    else//ӳ���ڴ�ģʽ
    {
        hBuffer->m_eMemoryMode = Memory_ROM;
        nRet = FrameBuffer_AllocROM(hBuffer, szName, eBufferMode);
    }
    hBuffer->m_pBufferEnd = hBuffer->m_pBufferBegin + nBuffSize;

    if(nRet < 0)
        return -1;

    if(eBufferMode & Mode_OnlyR)    //��ʼ��������
    {
        FrameBuffer_ResetReader(hBuffer, FALSE);
        FrameBuffer_CreateReadThread(hBuffer, szName);
    }
    if(eBufferMode & Mode_OnlyW)    //��ʼ��������
    {
        FrameBuffer_ResetWriter(hBuffer, FALSE);
        hBuffer->m_bCanWrite = TRUE;
    }

    if(!hBuffer->m_pWrite)   //��ʼдָ��
        hBuffer->m_pWrite = hBuffer->m_pBufferBegin+hBuffer->m_pShareInfo->nWOffset;
    if(!hBuffer->m_pRead)    //��ʼ��ָ��
        hBuffer->m_pRead = hBuffer->m_pBufferBegin+hBuffer->m_pShareInfo->nROffset;
    return 0;
}

void FrameBuffer_Release(HFrameBuffer hBuffer)
{
    SyncLock_Lock(&hBuffer->m_lockWrite, -1);
    hBuffer->m_bCanWrite = FALSE;
    SyncLock_Unlock(&hBuffer->m_lockWrite);

    //ֹͣ����̣߳��ͷ���������¼�
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

//��ʼ��ϢΪһ���ֽ���������
//  (0XF0) �������ԭ�ȵ�������Ϣ
//	(0XF1) ԭ�ȵ�������Ϣ�ڻ���������ʼλ��(�Ѿ�ѭ����)

//�жϻ�������ʣ��ռ��С�Ƿ���, �������������д������
//�Ѿ�д����λ��+��Ҫд������ݵĳ���: (�������ݳ��ȵĿռ�(4�ֽ�)+���ݳ���+������Ϣ���ȵĿռ�(4�ֽ�)+��Ϣ����)
//���Ƿ񳬳�����������Ĵ�С
//�������д��������ʱ�����ڵ�ǰλ��дһ��ѭ����־��Ȼ�����ƶ���������ͷ��ʼд������

/*	m_nWritePos
		|
|-------------------------------------------------------------------------------|
		|��ʼ��Ϣ(1�ֽ�)|4�ֽ�|���ݳ���|4�ֽ�|��Ϣ����|ѭ����־(1�ֽ�)|

|<-----------------------------   �����������Ĵ�С ---------------------------->|
*/
int FrameBuffer_Write(HFrameBuffer hBuffer, PBYTE pData, unsigned nSize, PBYTE pInfoData, unsigned nInfoSize)
{
    PBYTE pCurPos;

    SyncLock_Lock(&hBuffer->m_lockWrite, -1);

	if((!hBuffer->m_bCanWrite)||((signed)nSize >= hBuffer->m_nSize))
    {
        //TRACE("��ǰ������������д��\n");
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
            //��дָ�벻��ͬһ��ʱ�����д���µ����ݺ�ᳬ����ָ�룬��ǰ��д�����Ͳ��ٽ���
			SyncLock_Lock(&hBuffer->m_lockSeek, -1);

			if( (hBuffer->m_pShareInfo->bWLayer != hBuffer->m_pShareInfo->bRLayer)		//��дָ�벻��ͬһ��
				&&(pCurPos > (hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nROffset)))		//д�������ݺ�ᳬ����ָ��
			{
                //TRACE("д���ٶ�̫��,�Ѿ�׷���˶�ָ��, д�����ݺ�дָ��λ��:%d, дָ��λ��:%d, ��ָ��λ��:%d, ��ǰҪд�������:%x\n",
				//	pCurPos, hBuffer->m_pWrite, (hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nROffset), pData);
				SyncLock_Unlock(&hBuffer->m_lockSeek);

				return -1;
			}
			SyncLock_Unlock(&hBuffer->m_lockSeek);

			//д����ʼ��ʶ
			*(hBuffer->m_pWrite) = 0XF0;			//������ʼλ��
			hBuffer->m_pWrite += 1;

			//д�����ݵĳ���ֵ
			memcpy(hBuffer->m_pWrite, (void*)&nSize, sizeof(unsigned));//���˺�4���ֽ����ã���������Ҫд�����ݵĳ���
			hBuffer->m_pWrite += sizeof(unsigned);

			//д������
			if (nSize>0)
			{
				memcpy(hBuffer->m_pWrite, pData, nSize);
				hBuffer->m_pWrite += nSize;
			}

			//д����Ϣ����ֵ
            memcpy(hBuffer->m_pWrite, (void*)&nInfoSize, sizeof(unsigned));//���˺�4���ֽ����ã���������Ҫ������Ϣ�ĳ���
            hBuffer->m_pWrite += sizeof(unsigned);

			//д����Ϣ����
			if(nInfoSize>0)
			{
				memcpy(hBuffer->m_pWrite, pInfoData, nInfoSize);
				hBuffer->m_pWrite += nInfoSize;
			}

			//����дƫ��ֵ
			SyncLock_Lock(&hBuffer->m_lockSeek, -1);
			hBuffer->m_pShareInfo->nWOffset = hBuffer->m_pWrite - hBuffer->m_pBufferBegin;
            SyncLock_Unlock(&hBuffer->m_lockSeek);

			FrameBuffer_ResetWriter(hBuffer, TRUE);
		}
		else	//��ͷ��ʼд������
        {
            //TRACE("ʣ��ռ䲻��, ��ͷ��ʼд������, дָ��λ��:%d, ��ָ��λ��:%d, д�����ݺ�дָ��λ��:%d, ������:%d~%d, ��ǰҪд������:%x\n",
			//	hBuffer->m_pWrite, hBuffer->m_pRead, pCurPos, hBuffer->m_pBufferBegin, hBuffer->m_pBufferEnd, pData);

			*(hBuffer->m_pWrite) = 0XF1;			//��ĩβ����һ�����õ�����ͷ�ı�־
			hBuffer->m_pWrite = hBuffer->m_pBufferBegin;

			SyncLock_Lock(&hBuffer->m_lockSeek, -1);
			hBuffer->m_pShareInfo->nWOffset = 0;
			hBuffer->m_pShareInfo->bWLayer = !hBuffer->m_pShareInfo->bWLayer;
			SyncLock_Unlock(&hBuffer->m_lockSeek);

			goto WriteData;
		}
	}

	//���¼�֪ͨ������
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

	//��ͬһ���������ָ��׷����дָ�룬���ټ�����ȡ����
	SyncLock_Lock(&hBuffer->m_lockSeek, -1);
	if(hBuffer->m_pShareInfo->bRLayer == hBuffer->m_pShareInfo->bWLayer)					//��дָ����ͬһ��
	{
		pWriteBuffer = hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nWOffset;

		//��ָ�����дָ��,˵���������������Ѿ�����,�˴β�������,�ȴ���һ���������¼�
		if(pWriteBuffer == hBuffer->m_pRead)
		{
			SyncLock_Unlock(&hBuffer->m_lockSeek);
			return -1;
		}

		//��ͬһ���ָ�볬����дָ�룬˵���ڶ��Ĺ�����дָ�뱻�޸��ˣ�һ�������ò������£���
		//��ʱ����ָ��������ò���
		if(hBuffer->m_pRead > pWriteBuffer)
		{
			//TRACE("��ͬһ���ָ�볬����дָ�� ��ָ��λ��:%d, дָ��λ��:%d\n", hBuffer->m_pRead, pWriteBuffer);
			hBuffer->m_pRead = hBuffer->m_pBufferBegin;
		}
	}
	SyncLock_Unlock(&hBuffer->m_lockSeek);

	switch(*hBuffer->m_pRead)	//�жϵ�һ���ֽڵ�����
	{
	case 0XF0:	//��������ȡ����
		pAlReadPos = hBuffer->m_pRead;	//�õ���ǰ��ָ�����ʼλ��(���ں�������ƫ����)
		hBuffer->m_pRead += 1;

		//�õ������Լ����ݳ���
		memcpy(&nDataSize, hBuffer->m_pRead, sizeof(unsigned));
		hBuffer->m_pRead += sizeof(unsigned);

		pData = hBuffer->m_pRead;
		hBuffer->m_pRead += nDataSize;

        //�õ�������Ϣ�Լ�������Ϣ����
		memcpy(&nInfoSize, hBuffer->m_pRead, sizeof(unsigned));
		hBuffer->m_pRead += sizeof(unsigned);

		pInfo = hBuffer->m_pRead;
		hBuffer->m_pRead += nInfoSize;

		//�������Լ���������Ϣͨ���ص��������
		if(fCallback)
		{
			fCallback(hBuffer->m_objIndex, pData, nDataSize, pInfo, nInfoSize);
		}

		//TRACE("��ȡ����, ��������:%x, ��ʼλ��:%d, ����λ��:%d, ��ǰдָ��λ��:%d, ��������Χ:%d~%d\n",
		//	pData, pAlReadPos, hBuffer->m_pRead, (hBuffer->m_pBufferBegin + hBuffer->m_pShareInfo->nWOffset),\
		//	hBuffer->m_pBufferBegin, hBuffer->m_pBufferEnd);

		//�����ƫ����
		SyncLock_Lock(&hBuffer->m_lockSeek, -1);
		hBuffer->m_pShareInfo->nROffset = hBuffer->m_pRead - hBuffer->m_pBufferBegin;
		SyncLock_Unlock(&hBuffer->m_lockSeek);
		return nDataSize;

	case 0XF1:	//������������ʼ����ȡ����
		hBuffer->m_pRead = hBuffer->m_pBufferBegin;

		SyncLock_Lock(&hBuffer->m_lockSeek, -1);
		hBuffer->m_pShareInfo->bRLayer = !hBuffer->m_pShareInfo->bRLayer;	//���л�����һ��
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
    // ������Ϣҳ��
    if(!hBuffer->m_pShareInfo)
    {
        //TRACE("�����ڴ����ʧ��\n");
        return -1;
    }

    // ��������ҳ��
    hBuffer->m_pBufferBegin = (PBYTE)malloc(hBuffer->m_nSize);
    if(!hBuffer->m_pBufferBegin)
    {
        //TRACE("�����ڴ����ʧ��\n");
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

    // ������Ϣҳ��ӳ��
    sprintf(strObjName, "%s_ShareInfo", szName);
    hBuffer->m_pShareInfo = (SShareInfo*)CreateMapBuffer(INVALID_HANDLE_VALUE, sizeof(SShareInfo), strObjName, &hBuffer->m_hShareInfo);
    if(!hBuffer->m_pShareInfo)
    {
        //TRACE("����[%s]�����ڴ����ʧ��\n", szName);
        return -1;
    }

    // ��������ҳ��ӳ��
    sprintf(strObjName, "%s_ShareBuff", szName);
    hBuffer->m_pBufferBegin = (PBYTE)CreateMapBuffer(INVALID_HANDLE_VALUE, hBuffer->m_nSize, strObjName, &hBuffer->m_hMapBuffer);
    if(!hBuffer->m_pBufferBegin)
    {
        //TRACE("����[%s]�����ڴ����ʧ��\n", szName);
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
    //�ͷŻ�����
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

    // �ͷ���
    SyncLock_Destroy(&hBuffer->m_lockSeek);
    SyncLock_Destroy(&hBuffer->m_lockWrite);
    SyncLock_Destroy(&hBuffer->m_lockReset);

    return 0;
}

int FrameBuffer_CreateReadThread(HFrameBuffer hBuffer, const char *szName)
{
    if(hBuffer->m_eIOMode != Mode_OnlyW)
    {
        sprintf(hBuffer->m_sThreadReading.m_szFlag, "֡������");
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

        //�����δ����дƫ�ƣ����Ͷ˻�δ�������ݣ����Ƚ�������

        //����дָ�붼�ƶ�����������ʼ��
        if(!bSyncRW)	//ֻ����������ʱ�Ŷ�дָ������
        {
            hBuffer->m_pShareInfo->nWOffset = 0;
            hBuffer->m_pWrite = hBuffer->m_pBufferBegin;
        }

        hBuffer->m_pShareInfo->nROffset = 0;
        hBuffer->m_pRead = hBuffer->m_pBufferBegin;

        //����дָ������Ϊͬһ��
        hBuffer->m_pShareInfo->bWLayer = hBuffer->m_pShareInfo->bRLayer;

        //TRACE("������� дͬ����־:%d дָ��λ��:%d дƫ��:%d ��ָ��λ��:%d ��ƫ��:%d\n",
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

        //����дָ�붼�ƶ�����������ʼ��
        hBuffer->m_pShareInfo->nWOffset = 0;
        hBuffer->m_pWrite = hBuffer->m_pBufferBegin;

        //ֻ����������ʱ�ŶԶ�������������
        if(!bSyncRW)
        {
            hBuffer->m_pShareInfo->nROffset = 0;
            hBuffer->m_pRead = hBuffer->m_pBufferBegin;
        }

        //����дָ������Ϊͬһ��
        hBuffer->m_pShareInfo->bWLayer = hBuffer->m_pShareInfo->bRLayer;

        //TRACE("�������� ��ͬ����־:%d дָ��λ��:%d дƫ��:%d ��ָ��λ��:%d ��ƫ��:%d\n",
        //    bSyncRW, hBuffer->m_pWrite, hBuffer->m_pShareInfo->nWOffset, hBuffer->m_pRead, hBuffer->m_pShareInfo->nROffset);

        SyncLock_Unlock(&hBuffer->m_lockSeek);
    }
    SyncLock_Unlock(&hBuffer->m_lockReset);
    return 0;
}

BOOL CALLBACK ReadPulse(UINT_PTR m_nThreadFlag, void* objUser)
{
	HFrameBuffer hBuffer = (HFrameBuffer)objUser;
	//�Ӷ�ָ��ָ��Ļ�������ͷ�����ж������ݲ���
	//ֱ�������е����ݶ���Ϊֹ
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
