#pragma once
#ifndef _QUEUEFRAME_H_
#define _QUEUEFRAME_H_

#ifndef  LINKCOMM_EXPORTS
#define  LINKCOMMAPI	__declspec(dllimport)
#else
#define  LINKCOMMAPI	__declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

class CriticalSection
{
public:
    CriticalSection()
    {
        InitializeCriticalSection( &m_cs );
    }

    ~CriticalSection()
    {
        DeleteCriticalSection( &m_cs );
    }

    void lock()
    {
        EnterCriticalSection( &m_cs );
    }

    void unlock()
    {
        LeaveCriticalSection( &m_cs );
    }

	CRITICAL_SECTION &getLock()
	{
		return m_cs;
	}

#if(_WIN32_WINNT >= 0x0400)
    bool trylock()
    {
        return ( TryEnterCriticalSection( &m_cs ) != 0 );
    }
#endif /* _WIN32_WINNT >= 0x0400 */

private:
    CRITICAL_SECTION m_cs;
};

#define MAX_FRAME_SIZE 1920*1080

typedef struct tag_EncodeOut
{
	long			m_lDataLength;
	DWORD            nFrameType;
	BYTE			m_Data[MAX_FRAME_SIZE];
	tag_EncodeOut()
	{
		nFrameType    = 0;
		m_lDataLength = 0;
		memset(m_Data, 0, sizeof(BYTE)*MAX_FRAME_SIZE);
	}

}EncodeOut;

/******************************************************
����:         �����ڴ�ʵ��ѭ������
                ��������������
*******************************************************/

class LINKCOMMAPI CQueueFrame
{
public:
	CQueueFrame(void);
	~CQueueFrame(void);

	//���ö��г�ֵ
	void InitQueue(int max_len);

	//���
	bool PushQueue(BYTE* szData , long lLen , DWORD nFrameType);

	//����
	EncodeOut* PopQueue();

	//������
	void CopyFrame(int nPos, BYTE* szData , long lLen, DWORD nFrameType);

	int GetSize();

	CriticalSection m_cs;
private:
	int         m_front;  //��ͷָ��
	int         m_rear;   //��βָ��
	int         m_len;    //���鳤��
	EncodeOut*	m_pData;  //���е�ֵ
	int         m_maxlen; //��󳤶�

};

#ifdef __cplusplus
};
#endif

#ifndef  LINKCOMM_EXPORTS	
#pragma comment(lib, "LinkComm2.0.lib")
#pragma message("�Զ����� LinkComm2.0.lib") 
#endif

#endif