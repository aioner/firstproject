#ifndef _LINKINFO_H_
#define _LINKINFO_H_

#ifdef _WIN32
#include <Windows.h>
#ifndef  LINKCOMM_EXPORTS
#define  LINKINFOAPI	__declspec(dllimport)
#else
#define  LINKINFOAPI	__declspec(dllexport)
#endif
//#include "QueueFrame.h"

#else
#include <string.h>
#include <pthread.h>
#define  LINKINFOAPI
#endif



struct LINKINFOAPI LINKINFO
{
	//typedef void (__stdcall *ThreadWorkFunc)(void* pdata);
	//typedef unsigned int (__stdcall *PTHREAD_FUN)( LPVOID lpThreadParameter);

    long             nLinkHandle;               //���Ӿ��
	long		     nLoginHandle;            //�ϲ㴦��ṹ����¼���
	int                nDevType;                   //�豸����
	int		         nMediaType;              //ý������ ��/��/����/���� ---0/1/2/3
	void*           userData;
	char             szIp[1024];                   //IP��ַ
	int                nChannel;                    //ͨ����
	unsigned char    szHead[MAX_HEAD_SIZE];        //����ͷ
	int              nSize;                            //����ͷ��С
	POUTPUTREALDATA  pOutputFun;
	//CriticalSection  m_Lock;            //������
	HANDLE           hEventCaptrue;
	long   operHandle;
	bool use;

	//���ݼ��
	int monitorIntervalTime;   //�����ʱ��
	int monitorThreadFlag;     //����̱߳�ʶ
	void* monitorFunc;            //���ص�����
	void* monitorContext;      //����û�������
	long videoNumber;           //������ε���Ƶ����
	long audioNumber;           //������ε���Ƶ����
#ifdef WIN32
	void* monitorHandle;       //����߳̾��
#else
	pthread_t monitorHandle;
#endif
	
	//CQueueFrame      cQueqeData;     
	LINKINFO() 
	{
		nLinkHandle       = -1;
		nLoginHandle    = -1; //Ĭ��-1δ��¼
		nDevType           = 0;
		nMediaType       = -1;
		userData              = NULL;
		nSize                    = 0;
		pOutputFun        = NULL;
        use = false;

		videoNumber = 0;
		audioNumber = 0;
#ifdef WIN32
		monitorHandle = NULL;
#else
		monitorHandle = -1;
#endif

		hEventCaptrue     = NULL;
		memset(szHead, 0, MAX_HEAD_SIZE);
		memset(szIp, 0, 1024);
	//	hEventTreadExit   = ::CreateEvent(NULL,FALSE,true,NULL);
	}

};

struct THDATA
{
	LINKINFO *pLinkinfo;
	long  lOper;
};

// #ifndef  LINKCOMM_EXPORTS	
// #pragma comment(lib, "LinkComm.lib")
// #pragma message("�Զ����� LinkComm.lib") 
// #endif

#endif