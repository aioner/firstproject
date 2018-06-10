#include "HCDvrDevice.h"
#include "AnalyzeDataNewInterface.h"

#ifdef WIN32
#else
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define __declspec(dllexport)
#define CALLBACK
const char* HC_VER_INFO = "XT_Lib_Version: V_HC_1.00.1221.0";
#endif

CBaseDevice __declspec(dllexport) * AddMe(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType)
{
	return AddA(new CHCDvrDevice, pDeviceCollect, nDeviceLinkType, nDataType);
}

/*void __declspec(dllexport)  DeleteMe(CBaseDevice* p)
{
	delete  (CHCDvrDevice*)p;
}*/
#ifdef WIN32
DWORD __stdcall ThreadStopPlay(void* param)
{
	int devhandle = (int)(*((int*)param));
	NET_DVR_StopRealPlay(devhandle);
	return 0;
}
#else
void* ThreadStopPlay(void* param)
{
	int devhandle = (int)(*((int*)param));
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	NET_DVR_StopRealPlay(devhandle);
	printf("HC stop OK. handle=%d\n", devhandle);

	return NULL;
}
#endif

#ifdef WIN32
DWORD __stdcall ThreadLogOut(void* param)
{
	int devhandle = (int)(*((int*)param));
	NET_DVR_Logout_V30(devhandle);
	return 0;
}
#else
void* ThreadLogOut(void* param)
{
	int devhandle = (int)(*((int*)param));
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	NET_DVR_Logout_V30(devhandle);

	return NULL;
}
#endif


void CALLBACK RealDataCallBack(long nLinkHandle, long nFrameType, UCHAR *pPacketBuffer,long nPacketSize, long dwUser)
{
	CHCDvrDevice *pHCDvrDevice = (CHCDvrDevice *)dwUser;
	
	//pHCDvrDevice->m_LinkMap[nLinkHandle].cs->lock();

	/*if(pHCDvrDevice->m_LinkMap.count(nLinkHandle) == 0)
	{
		pHCDvrDevice->m_LinkMap[nLinkHandle]->unlock();
		return;
	}*/

	map<long, DEV_LINKMAPS>::iterator iter = pHCDvrDevice->m_LinkMap.find(nLinkHandle);

	if(iter == pHCDvrDevice->m_LinkMap.end())
	{
		//pHCDvrDevice->m_LinkMap[nLinkHandle].cs->unlock();
		return;
	}

	//数据头信息	
	if(nFrameType == 1)
	{
		nFrameType = 0x80;
		iter->second.headLen = nPacketSize;
		memcpy(iter->second.headData, pPacketBuffer, nPacketSize);
		//pHCDvrDevice->NotifyHeadData(nLinkHandle, pPacketBuffer, nPacketSize);
	}
	else
	{
		nFrameType = 0;
	}

	//正常音视频帧(主辅码流)
	if (pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.mediaType == 0)
	{
		if (pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.pRealDatacallback != NULL)
		{
			pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.pRealDatacallback(nLinkHandle, nFrameType, pPacketBuffer, nPacketSize, 1, pHCDvrDevice, 0, 0);
		}
	}
	else //纯音频
	{
		if (pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.hAnalyzerHandle == NULL)
		{
			pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.hAnalyzerHandle = HIKANA_CreateStreamEx(1024*1024*2, pPacketBuffer);
		}

		PACKET_INFO_EX pstPacket;

		HIKANA_InputData(pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.hAnalyzerHandle, pPacketBuffer, nPacketSize);
		int nRet = HIKANA_GetOnePacketEx(pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.hAnalyzerHandle, &pstPacket);
		while(ERROR_NO == nRet)
		{
			if((pstPacket.nPacketType == AUDIO_PACKET)&&(NULL != pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.pRealDatacallback))
			{
				pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.pRealDatacallback(nLinkHandle, 0x120000, (unsigned char*)pstPacket.pPacketBuffer, pstPacket.dwPacketSize, 65, pHCDvrDevice, 0, 0);
				//OV_RealDataCallBack(g_nDeviceType, nLinkIndex, PBYTE(pstPacket.pPacketBuffer), pstPacket.dwPacketSize, g_nDataType+64, 0, 0, nDataType, (void *)nUserData);
			}

			nRet = HIKANA_GetOnePacketEx(pHCDvrDevice->m_LinkMap[nLinkHandle].devLinkinfo.hAnalyzerHandle, &pstPacket);
		}
	}

	//pHCDvrDevice->m_LinkMap[nLinkHandle].cs->unlock();
}


void CALLBACK HikRealDataCallBack_V30(LONG nLinkHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize, void * dwUser)
{

   // RealDataCallBack(nLinkHandle, dwDataType, pBuffer, dwBufSize, (INT_PTR)dwUser);
}

void CALLBACK HikRealDataCallBack(LONG nLinkHandle, DWORD dwDataType, BYTE *pBuffer,DWORD dwBufSize, DWORD dwUser)
{
    RealDataCallBack(nLinkHandle, dwDataType, pBuffer, dwBufSize, (int)dwUser);
}


CHCDvrDevice::CHCDvrDevice(void)
{
	//m_lpRealDatacallback = NULL;
	//BOOL ret = NET_DVR_Init(); 
}

CHCDvrDevice::~CHCDvrDevice(void)
{
}

//初始化设备
long CHCDvrDevice::InitDevice(void* pParam)
{
	int iError = 0;
	BOOL bIsInit = false;
	bIsInit = NET_DVR_Init();
	if (!bIsInit)
	{
		iError = NET_DVR_GetLastError();
		return iError;
	}
	
	return 0;
}

//反初始化设备
long CHCDvrDevice::UnInitDevice(void* pParam)
{
	NET_DVR_Cleanup();
	return 0;
}

long CHCDvrDevice::LoginDevice(const char* szDeviceIP, long  nPort, const char* szUserID, const char* szPassword)
{
	NET_DVR_DEVICEINFO_V30 sDeviceInfo;
    memset(&sDeviceInfo, 0, sizeof(NET_DVR_DEVICEINFO_V30));
	long nLoginHandle = NET_DVR_Login_V30((char *)szDeviceIP, (WORD)nPort, (char *)szUserID, (char *)szPassword, &sDeviceInfo);
	
    int nError = 0;
    if(nLoginHandle<0)
	{
        nError = NET_DVR_GetLastError();
	}
	else
	{
		m_loginMap.insert(pair<long, NET_DVR_DEVICEINFO_V30>(nLoginHandle, sDeviceInfo));
	}

	return nLoginHandle;
}

bool CHCDvrDevice::LogoutDevice(long lLoginHandle)
{
	if(lLoginHandle<0)
        return false;

	m_loginMap.erase(lLoginHandle);

	//BOOL b = NET_DVR_Logout_V30(lLoginHandle);
#ifdef WIN32
	HANDLE logoutHand = CreateThread(NULL, 0, ThreadLogOut, (void*)&lLoginHandle, 0, NULL);
	DWORD waitFlag = WaitForSingleObject(logoutHand, 2000);
	if (waitFlag != WAIT_OBJECT_0)
	{
		TerminateThread(logoutHand, 0);
	}
	CloseHandle(logoutHand);
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;
	pthread_t  pth_t;
	pthread_create(&pth_t, NULL, &ThreadLogOut, &lLoginHandle);
	int joinRet = pthread_timedjoin_np(pth_t, NULL, &ts);
	if (joinRet != 0)
	{
		pthread_cancel(pth_t);
	}
#endif

	return true;
}

long CHCDvrDevice::StartLinkDevice(char *szDeviceIP,long nNetPort ,long nChannel, long nLinkType,long nMediaType, long sockethandle, const char *szMulticastIp, unsigned short nMulticastPort)
{
	NET_DVR_PREVIEWINFO info = {0};
    info.hPlayWnd   = NULL;
    info.dwLinkMode = nLinkType;

	//主辅码流
    info.dwStreamType = nMediaType; 
	if (nMediaType == 2)
	{
		info.dwStreamType = 0;
	}

	//szDeviceIP在该处为句柄，参见MediaDevice
	long lLoginHandle = atoi(szDeviceIP);//GetLoginHandle((char*)szDeviceIP);

	if(lLoginHandle < 0)
		return -1;

	//如果是IPC通道
	if(m_loginMap[lLoginHandle].byChanNum <(nChannel + 1))
	{
		info.lChannel   = nChannel - m_loginMap[lLoginHandle].byChanNum + 33;
	}
	else//编码器通道
	{
		info.lChannel   = nChannel + 1;
	}

	//添加回调函数
	long lLinkHandle = NET_DVR_RealPlay_V40(lLoginHandle, &info, NULL, this);
	if (lLinkHandle == -1)
	{
		return -2;
	}

	DEV_LINKMAPS linkMap;
	//linkMap.cs = new  CriticalSection;
	linkMap.devLinkinfo.port = nNetPort;
	linkMap.devLinkinfo.channel = nChannel;
	linkMap.devLinkinfo.mediaType = nMediaType;
	linkMap.devLinkinfo.linkType = nLinkType;

	m_LinkMap.insert(pair<int, DEV_LINKMAPS>(lLinkHandle, linkMap));

    return lLinkHandle;
}

void CHCDvrDevice::StopLinkDevice(long lDeviceLinkHandle)
{
    if(lDeviceLinkHandle<0)
        return;

	map<long, DEV_LINKMAPS>::iterator iter = m_LinkMap.find(lDeviceLinkHandle);
	if(iter == m_LinkMap.end())
	{
		return;
	}

	//CriticalSection * p = m_LinkMap[lDeviceLinkHandle].cs;

	//p->lock();

    BOOL bRet = NET_DVR_SetRealDataCallBack(lDeviceLinkHandle, NULL, 0);

#ifdef WIN32
	HANDLE stopHand = CreateThread(NULL, 0, ThreadStopPlay, (void*)&lDeviceLinkHandle, 0, NULL);
	DWORD waitFlag = WaitForSingleObject(stopHand, 2000);
	if (waitFlag != WAIT_OBJECT_0)
	{
		TerminateThread(stopHand, 0);
	}
	CloseHandle(stopHand);
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;
	pthread_t  pth_t;
	pthread_create(&pth_t, NULL, &ThreadStopPlay, &lDeviceLinkHandle);
	int joinRet = pthread_timedjoin_np(pth_t, NULL, &ts);
	if (joinRet != 0)
	{
		pthread_cancel(pth_t);
	}
#endif

	m_LinkMap.erase(lDeviceLinkHandle);
	
	//p->unlock();
	//delete p;
	//p = NULL;

}


long CHCDvrDevice::StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext)
{
	if(lpRealDatacallback == NULL)
	{
		return 0;
	}
	else
	{
		map<long, DEV_LINKMAPS>::iterator iter = m_LinkMap.find(lDeviceLinkHandle);

		if(iter == m_LinkMap.end())
		{
			return -1;
		}
		iter->second.devLinkinfo.pRealDatacallback = lpRealDatacallback;
	}

	BOOL ret = NET_DVR_SetRealDataCallBack(lDeviceLinkHandle, HikRealDataCallBack, (DWORD)this);
	if (ret == FALSE)
	{
		return -1;
	}
	
	return 0;
}

long  CHCDvrDevice::StopLinkCapture(long lDeviceLinkHandle)
{
	map<long, DEV_LINKMAPS>::iterator iter = m_LinkMap.find(lDeviceLinkHandle);
	if(iter == m_LinkMap.end())
	{
		return -1;
	}
	iter->second.devLinkinfo.pRealDatacallback = NULL;

	NET_DVR_SetRealDataCallBack(lDeviceLinkHandle, NULL, (DWORD)this);

	return 0; 
}

long CHCDvrDevice::GetSDP(long lDeviceLinkHandle, unsigned char *headData, long& length)
{
	map<long, DEV_LINKMAPS>::iterator iter = m_LinkMap.find(lDeviceLinkHandle);
	if(iter == m_LinkMap.end())
	{
		return -1;
	}
	
	length = iter->second.headLen;
	if (length == 0)
	{
		headData[0] = '\0';
	}
	else
	{
		memcpy(headData, iter->second.headData, length);
	}

	return 0;
}

long CHCDvrDevice::TcpPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp)
{
	return -1;
}

long CHCDvrDevice::TcpPauseCtrl(long lDeviceLinkHandle)
{
	return -1;
}
