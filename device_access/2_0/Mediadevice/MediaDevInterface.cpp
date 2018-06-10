//#include "StdAfx.h"
#include "MediaDevInterface.h"

//#include "MediaLog.h"
#include "DevLinkHandle.h"
#include "LinkInfo.h"

#ifndef WIN32
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#endif

extern int logLevel;
extern void WriteLog(int level, const char* title, const char* content);

//#define MAX_PACKETS 1920*1080*2
const char* XT_LIB_INFO = "XT_Lib_Version: V1.16.0505.0";

CDevLinkHandle* g_pDeviceLink = NULL;
char headData[1024];

//初始化
long StartMediadevice()
{
#ifdef WIN32
	CreateDirectory("d:\\Log\\MediaDeviceLog", NULL);
	WriteLog(logLevel, "[StartMediadevice]" ,  "****启动MediaDevice2.0********");
#else
	mkdir("/var/log/MediaDeviceLog", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	WriteLog(logLevel, "[StartMediadevice]" ,  "****Start MediaDevice2.0********");
#endif

	if (g_pDeviceLink != NULL)
	{
		EndMediadevice();
	}
	if (g_pDeviceLink == NULL)
	{
		g_pDeviceLink = new CDevLinkHandle();
		if (g_pDeviceLink == NULL)
		{
			return -1;
		}
	}

	return 0;
}

//反初始化
long EndMediadevice()
{
	if (g_pDeviceLink != NULL)
	{
		delete g_pDeviceLink;
		g_pDeviceLink = NULL;
	}

#ifdef WIN32
	WriteLog(logLevel, "[EndMediadevice]" ,   "****退出MediaDevice2.0******\r\n");
#else
	WriteLog(logLevel, "[EndMediadevice]" ,   "****Exit MediaDevice2.0******\r\n");
#endif	

	return 0;
}

long MEDIADEVICEAPI InitializeDevice(long  nDeviceType, void* pParam)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}

	g_pDeviceLink->m_DeviceCollect.SetPortRange(0, 0);
	return g_pDeviceLink->InitializeDevice(nDeviceType, pParam);
}

long MEDIADEVICEAPI InitializeDeviceEx(long  nDeviceType, xt_client_cfg_t xtCfg, long startPort, long portNum)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	g_pDeviceLink->m_DeviceCollect.SetPortRange(startPort, portNum);

	return g_pDeviceLink->InitializeDeviceEx(nDeviceType, xtCfg);
}

long MEDIADEVICEAPI UnInitializeDevice(long  nDeviceType, void* pParam)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}

	return g_pDeviceLink->UnInitializeDevice(nDeviceType, pParam);
}

//XMPP应用层协商传输接口
///////////////////////////////////////////////////////////////////////////////
//启动链路
long StartLinkDevice(const char *szIP, const char* szSdp,long nSdpLen,long nChannel,long nLinkType, long nMediaType,
									   long nDeviceType, void* lUesrData,POUTPUTREALDATA  POutPutRealDataFunc)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StartLinkDevice(szIP,szSdp,nSdpLen,nChannel,nLinkType,nMediaType,nDeviceType,lUesrData,POutPutRealDataFunc);
}

//开启采集
long StartLinkCapture(long nOperHandle)
{	
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StartLinkCapture(nOperHandle);
}

//关闭采集
long  StopLinkCapture(long nOperHandle)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StopLinkCapture(nOperHandle);
}

long GetSDP(long nOperHandle,unsigned char *szSDP, long& nLength)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->GetSDP(nOperHandle,szSDP,nLength);
}
//关闭链路
long  StopLinkDevice(long nOperHandle)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StopLinkDevice(nOperHandle);
}

long GetClientInfo(long nOperHandle,long& RtpRecvPort,long& RtcpRecvPort,bool& MultiplexR, long& MultidR)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->GetClientInfo(nOperHandle,RtpRecvPort,RtcpRecvPort,MultiplexR,MultidR);
}

long md_create_recv(int track_num, bool demux)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->create_recv(track_num, demux);
}

long md_create_recvinfo(int dev_type, int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->create_recvinfo(dev_type, track_num, demux, multicast, multicastip, multiports);
}

long md_get_rcvinfo(long link, _RCVINFO *infos, int &num)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->get_rcvinfo(link, (RCVINFO*)infos, num);
}

long md_set_sdp(long link, const char *sdp, unsigned int len_sdp)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->set_sdp(link, sdp, len_sdp);
}

long md_request_iframe(long link)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->request_iframe(link);

}
long  md_start_link_captuer(const long link_handel,POUTPUTREALDATA data_out_cb,void* user_data)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}

	return g_pDeviceLink->start_link_captuer(link_handel,data_out_cb,user_data);

}

long  md_set_regist_callback(long device_type,regist_call_back_t func)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}

	return g_pDeviceLink->regist_call_back(device_type,func);

}
///////////////////////////////////////////////////////////////////////////////

long StartDeviceCapture(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,char* szUser, char* szPassword, long nNetLinkType, long nMediaType, long sockethandle,
						const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP,void *hmp)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StartDeviceCapture(szIP, nPort, nDeviceType, nChannel, pUserData, pOutputRealData,szUser,szPassword,nNetLinkType,nMediaType, sockethandle, szMulticastIp, nMulticastPort, szLocalIP,hmp);
}

long StartDeviceCaptureEx(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,char* szUser, char* szPassword, long nNetLinkType, long nMediaType, long sockethandle, const char *szMulticastIp, unsigned short nMulticastPort)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StartDeviceCapture(szIP, nPort, nDeviceType, nChannel, pUserData, pOutputRealData,szUser,szPassword,nNetLinkType,nMediaType, sockethandle, szMulticastIp, nMulticastPort);
}

 
long  StartDeviceCaptureRTP(long  nDeviceType, const char* szURL,  void* pUserData, POUTPUTREALDATA  pOutputRealData, long nNetLinkType, long nMediaType)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StartDeviceCapture(nDeviceType, szURL, pUserData,pOutputRealData);
}

long StopDeviceCapture(long nOperHandle)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->StopDeviceCapture(nOperHandle);	
}

long  GetDataType(long nOperHandle)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->GetDataType(nOperHandle);
}

char* GetHeadData(long nOperHandle, long& nHeadLen)
{	 
	if (g_pDeviceLink == NULL)
	{
		return NULL;
	}
	nHeadLen = g_pDeviceLink->GetHeadData(nOperHandle,headData);
	return headData;
}

long  GetPlayRetStatus(long nOperHandle)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->GetPlayRetStatus(nOperHandle);
}


long GetTrack(long nOperHandle, TRACKINFO * pTrackInfo)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}

	return g_pDeviceLink->GetTrack(nOperHandle, (_TRACKINFO*)pTrackInfo);
}

long  TcpPlayCtrl(const long nOperHandle,double npt,float scale, unsigned long *rtp_pkt_timestamp)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->TcpPlayCtrl(nOperHandle,npt,scale,rtp_pkt_timestamp);
}

long  TcpPauseCtrl(const long nOperHandle)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->TcpPauseCtrl(nOperHandle);
}

long SetDataMonitor(long nOperHandle, DataMonitorCB monitorFunc, int intervalTime, void* pContext)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->SetDataMonitor(nOperHandle,monitorFunc, intervalTime, pContext);
}

long MEDIADEVICEAPI SetSSRCReport(long nOperHandle, rtcp_report_callback_t reportFunc, void* pContext)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->SetSSRCReport(nOperHandle,reportFunc, pContext);
}

long MEDIADEVICEAPI SetResend(long device_type, int resend, int wait_resend, int max_resend, int vga_order)
{
	if (g_pDeviceLink == NULL)
	{
		return -10;
	}
	return g_pDeviceLink->SetResend(device_type,resend,wait_resend, max_resend,vga_order);
}

/*
//开始播放
bool    MC_Start(long nOperHandle)
{
	return true;
}

//停止
bool    MC_Stop(long nOperHandle)
{
	return true;
}

//暂停
bool    MC_Pause(int nOperHandle)
{
	return true;
}

//快进
bool    MC_Speed(long nOperHandle, float fNum)
{
	return true;
}

//拖放
unsigned int   MC_SeekTo(long nOperHandle, unsigned long dTimeStamp)
{
	return 0;
}
*/