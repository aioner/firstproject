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

    long             nLinkHandle;               //链接句柄
	long		     nLoginHandle;            //上层处理结构，登录句柄
	int                nDevType;                   //设备类型
	int		         nMediaType;              //媒体类型 主/子/主音/子音 ---0/1/2/3
	void*           userData;
	char             szIp[1024];                   //IP地址
	int                nChannel;                    //通道号
	unsigned char    szHead[MAX_HEAD_SIZE];        //数据头
	int              nSize;                            //数据头大小
	POUTPUTREALDATA  pOutputFun;
	//CriticalSection  m_Lock;            //操作锁
	HANDLE           hEventCaptrue;
	long   operHandle;
	bool use;

	//数据监测
	int monitorIntervalTime;   //监测间隔时间
	int monitorThreadFlag;     //监测线程标识
	void* monitorFunc;            //监测回调函数
	void* monitorContext;      //监测用户上下文
	long videoNumber;           //监测间隔段的视频数量
	long audioNumber;           //监测间隔段的音频数量
#ifdef WIN32
	void* monitorHandle;       //监测线程句柄
#else
	pthread_t monitorHandle;
#endif
	
	//CQueueFrame      cQueqeData;     
	LINKINFO() 
	{
		nLinkHandle       = -1;
		nLoginHandle    = -1; //默认-1未登录
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
// #pragma message("自动连接 LinkComm.lib") 
// #endif

#endif