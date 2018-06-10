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
功能:         定长内存实现循环队列
                队列满后队首溢出
*******************************************************/

class LINKCOMMAPI CQueueFrame
{
public:
	CQueueFrame(void);
	~CQueueFrame(void);

	//设置队列初值
	void InitQueue(int max_len);

	//入队
	bool PushQueue(BYTE* szData , long lLen , DWORD nFrameType);

	//出队
	EncodeOut* PopQueue();

	//祯拷贝
	void CopyFrame(int nPos, BYTE* szData , long lLen, DWORD nFrameType);

	int GetSize();

	CriticalSection m_cs;
private:
	int         m_front;  //队头指针
	int         m_rear;   //队尾指针
	int         m_len;    //数组长度
	EncodeOut*	m_pData;  //队列的值
	int         m_maxlen; //最大长度

};

#ifdef __cplusplus
};
#endif

#ifndef  LINKCOMM_EXPORTS	
#pragma comment(lib, "LinkComm2.0.lib")
#pragma message("自动连接 LinkComm2.0.lib") 
#endif

#endif