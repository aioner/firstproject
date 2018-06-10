//#include "StdAfx.h"
#include "DevLinkHandle.h"
#include "LinkInfo.h"
#include "Sdp.h"

#ifdef WIN32
#else
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#define __stdcall
#endif

#define MAX_LINK_INDEX 10240

char szLogBuf[10240];
int logLevel = 1;
void WriteLog(int level, const char* title, const char* content)
{
    if (level < 1)
    {
        return;
    }

    char timeBuf[100];
    char fileName[100];
    char titleBuf[100]={0};

#ifdef WIN32
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);
    sprintf(fileName, "d:\\Log\\MediaDeviceLog\\Log_%d%02d%02d.txt", sysTime.wYear, sysTime.wMonth,sysTime.wDay);
    sprintf(timeBuf, "[%d-%02d-%02d %02d:%02d:%02d.%03d]   ", sysTime.wYear, sysTime.wMonth,sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
#else
    struct timeval tv;
    struct timezone tz;
    struct tm *ptm = NULL;

    gettimeofday(&tv, &tz);
    //ptm = gmtime((const time_t*)&tv.tv_sec);
	ptm = localtime((const time_t*)&tv.tv_sec);
    sprintf(fileName, "/var/log/MediaDeviceLog/Log_%d%02d%02d.txt", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
    sprintf(timeBuf, "%d-%02d-%02d %02d:%02d:%02d  ", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
#endif

    FILE *fl = fopen(fileName, "a+b");
    if (fl == NULL)
    {
        return;
    }

    _snprintf(titleBuf, 27, "%- 30s", title);

    fwrite(timeBuf, sizeof(char), strlen(timeBuf), fl);
    fwrite(titleBuf, sizeof(char), strlen(titleBuf), fl);
    fwrite(content, sizeof(char), strlen(content), fl);
#ifdef WIN32
    fwrite("\r\n", sizeof(char), 2, fl);
#else
    fwrite("\n", sizeof(char), 1, fl);
#endif

    fclose(fl);
}

using namespace sdp;


//回调函数
long __stdcall RealDataCallback(long nLinkHandle, long nFrameType, unsigned char* pDataBuf, long nDataLength,  long nDataType, void* pUserData, long nTimeStamp, unsigned long nSSRC)
{
    CBaseDevice* pBaseDevie = (CBaseDevice*)pUserData;
    if (!pBaseDevie)
    {
        return -1;
    }

    if(nFrameType == 128)
    {		
        sprintf(szLogBuf ,  "Head data length=%ld", nDataLength);
        WriteLog(logLevel, "[RealDataCallback]",  szLogBuf);

        pBaseDevie->NotifyHeadData(nLinkHandle, pDataBuf, nDataLength);
    }

    LINKINFO *link;//for loop 512x(512+1)/2=13w 次
    int ret = pBaseDevie->GetLinkInfo(nLinkHandle, &link);
    if(ret>=0)
    {
		//数据监测
		if (link->monitorThreadFlag  == 1)
		{
			if (nFrameType == 0)
			{
				link->videoNumber++;
			}
			else if (nFrameType != 128)
			{
				link->audioNumber++;
			}
		}

		//抛出数据给用户
		if ( link->pOutputFun != NULL)
		{
			return link->pOutputFun(link->operHandle,  pDataBuf, nDataLength,nFrameType, nDataType, (void*)link->userData, nTimeStamp, nSSRC);
		}
    }

    return -1;
}

#ifdef WIN32
DWORD __stdcall ThreadMonitorData(void* param)
{
	if (param == NULL)
	{
		return -1;
	}

	LINKINFO* p = (LINKINFO*)param;

	sprintf(szLogBuf ,  "Begin the data monitor thread. handle: %ld", p->operHandle);
	WriteLog(logLevel, "[DataMonitorThread]",  szLogBuf);

	while (p != NULL && p->monitorThreadFlag == 1)
	{
		for (int i=0; i<p->monitorIntervalTime*2; i++)
		{
			Sleep(500);
			if (p->monitorThreadFlag == 0)
			{
				break;
			}
		}
		if (p->monitorThreadFlag == 0)
		{
			break;
		}
		if (p->monitorFunc != NULL)
		{
			((DataMonitorCB)p->monitorFunc)(p->operHandle, p->videoNumber, p->audioNumber, p->monitorContext);
		}

		p->videoNumber = 0;
		p->audioNumber = 0;
	}

	sprintf(szLogBuf ,  "Exit the data monitor thread. handle: %ld", p->operHandle);
	WriteLog(logLevel, "[DataMonitorThread]",  szLogBuf);

	return 0;
}
#else
void* ThreadMonitorData(void* param)
{
	if (param == NULL)
	{
		return NULL;
	}
	LINKINFO* p = (LINKINFO*)param;

	sprintf(szLogBuf ,  "Begin the data monitor thread. handle: %ld", p->operHandle);
	WriteLog(logLevel, "[DataMonitorThread]",  szLogBuf);

	while (p != NULL && p->monitorThreadFlag == 1)
	{
		for (int i=0; i<p->monitorIntervalTime*2; i++)
		{
			usleep(500*1000);
			if (p->monitorThreadFlag == 0)
			{
				break;
			}
		}
		if (p->monitorThreadFlag == 0)
		{
			break;
		}
		if (p->monitorFunc != NULL)
		{
			((DataMonitorCB)p->monitorFunc)(p->operHandle, p->videoNumber, p->audioNumber, p->monitorContext);
		}

		p->videoNumber = 0;
		p->audioNumber = 0;
	}

	sprintf(szLogBuf ,  "Exit the data monitor thread. handle: %ld", p->operHandle);
	WriteLog(logLevel, "[DataMonitorThread]",  szLogBuf);

	return NULL;
}
#endif

CDevLinkHandle::CDevLinkHandle(void)
{
}

CDevLinkHandle::~CDevLinkHandle(void)
{
}

long CDevLinkHandle::get_free_linkindex()
{
	for (long i=0;i<MAX_LINK_INDEX;++i)
	{
		if (m_mapLinkHandle.find(i) == m_mapLinkHandle.end())
		{
			return i;
		}
	}

	return -1;
}

//初始化设备
long CDevLinkHandle::InitializeDevice(long  nDeviceType, void* pParam)
{
    sprintf(szLogBuf, "device type:%ld", nDeviceType);
    WriteLog(logLevel,"[InitializeDevice] ", szLogBuf);

    //如果nDeviceType<0，就初始化配置文件中的所有设备类型
    if (nDeviceType < 0)
    {
        for (std::map<long, CBaseDevice*>::iterator iter=m_DeviceCollect.begin(); iter != m_DeviceCollect.end(); iter++)
        {
            if (iter->second == NULL)
            {
                continue;
            }
            else
            {
                ((CBaseDevice*)(iter->second))->InitDevice(pParam);
            }
        }

        return 0;
    }
    else
    {
        //通过设备类型获取设备指针 
        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(nDeviceType);

        if(pBaseDevice == NULL)
        {
            return -1;
        }

        return pBaseDevice->InitDevice(pParam);
    }
}

//初始化设备
long CDevLinkHandle::InitializeDeviceEx(long  nDeviceType, xt_client_cfg_t xtCfg)
{
    sprintf(szLogBuf, "device type:%ld\r\n", nDeviceType);
    WriteLog(logLevel,"[InitializeDeviceEx] ", szLogBuf);

    //如果nDeviceType<0，就初始化配置文件中的所有设备类型
    if (nDeviceType < 0)
    {
        for (std::map<long, CBaseDevice*>::iterator iter=m_DeviceCollect.begin(); iter != m_DeviceCollect.end(); iter++)
        {
            if (iter->second == NULL)
            {
                continue;
            }
            else
            {
                ((CBaseDevice*)(iter->second))->InitDevice((void*)&xtCfg);
            }
        }

        return 0;
    }
    else
    {
        //通过设备类型获取设备指针 
        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(nDeviceType);

        if(pBaseDevice == NULL)
        {
            return -1;
        }

        return pBaseDevice->InitDevice((void*)&xtCfg);
    }
}

//反初始化设备
long CDevLinkHandle::UnInitializeDevice(long  nDeviceType, void* pParam)
{
    sprintf(szLogBuf, "device type:%ld", nDeviceType);
    WriteLog(logLevel,"[UnInitializeDevice] ", szLogBuf);

    if (nDeviceType < 0)
    {
        for (std::map<long, CBaseDevice*>::iterator iter=m_DeviceCollect.begin(); iter != m_DeviceCollect.end(); iter++)
        {
            if (iter->second == NULL)
            {
                continue;
            }
            else
            {
                ((CBaseDevice*)(iter->second))->UnInitDevice(pParam);
            }
        }

        return 0;
    }
    else
    {
        //通过设备类型获取设备指针 
        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(nDeviceType);

        if(!pBaseDevice)
        {
            return -1;
        }

        return pBaseDevice->UnInitDevice(pParam);
    }
}

//开始采集
long CDevLinkHandle::StartDeviceCapture(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,
										char* szUser, char* szPassword,long nNetLinkType, long nMediaType, long sockethandle, 
										const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP,void *hmp)
{
	sprintf(szLogBuf, "IP:%s, port:%ld, devType:%ld, chan:%ld, userID:%ld, netType:%ld, mediaType:%ld, user:%s, pwd:%s, multiIP:%s, multiPort:%d, localIP:%s", szIP, nPort,nDeviceType,nChannel,pUserData,nNetLinkType, nMediaType, szUser, szPassword, szMulticastIp, nMulticastPort, szLocalIP);
    WriteLog(logLevel, "[StartDeviceCapture]", szLogBuf);

    char szSrcIp[256];

	//先读取netlinktype
	long linktypea = m_DeviceCollect.GetNetlinkType(nDeviceType);
	if (linktypea != -1)
	{
		nNetLinkType = linktypea;
	}
	//

    //port <1，从配置文件中去读port
    if(nPort <1)
    {
        m_DeviceCollect.QueryPort(nDeviceType, nNetLinkType, nPort);
    }

    // LT_TCP_TCP_PRI = 0,          //私有tcp会话以及私有tcp流
    //	LT_TCP_UDP_PRI = 1,          //私有tcp会话以及私有udp流
    //	LT_TCP_UDP_MULTI = 2,        //私有tcp会话以及私有udp流多播
    //	LT_TCP_RTP_PRI = 3,          //私有tcp会话以及私有rtp混合流
    //	LT_TCP_RTP_MULTI = 4,        //私有tcp会话以及私有rtp混合流多播
    //	LT_TCP_RTP_DEMUX_PRI = 5,    //私有tcp会话以及私有rtp混合端口复用流
    //	LT_RTSP_RTP_STD = 6,         //标准rtsp会话以及标准rtp流
    //	LT_RTSP_RTP_DEMUX = 7,       //标准rtsp会话以及复用rtp流
    //	LT_RTSP_RTP_PRI = 8,         //标准rtsp会话以及私有rtp流
    //	LT_TCP_RTP_STD = 9,          //私有tcp会话以及标准rtp流

    //	LT_XMPP_RTP_STD =10,         //私有XMPP会话以及标准rtp流
    //	LT_XMPP_RTP_PRI = 11,        //私有XMPP会话以及私有rtp流
    //	LT_XMPP_RTP_DEMUX_PRI =12,   //私有XMPP会话以及私有rtp混合端口复用流

    //	LT_UDP_RTP_PRI  = 13,        //私有udp会话以及私有rtp混合流

    if (nDeviceType == 172 || nDeviceType == 181)
    {
        nNetLinkType = 6; 
    }
    if(nNetLinkType == 6)
    {
        if (nDeviceType == 172)
        {
            int  nCtlPort = (nMediaType==1?8556:8557);
            sprintf(szSrcIp, "rtsp://%s:%d/ch%02ld/PSIA/Streaming/channels/2?videoCodecType=H.264",szIP, nCtlPort, nChannel); //264
            //sprintf(szSrcIp, "rtsp://%s:%d/stream:h265",szIP, nCtlPort); //265
        }
        else if (nDeviceType == 2)
        {
            sprintf(szSrcIp, "%s",szIP);
        }
        else
        {
            if(nPort <1)
            {
                m_DeviceCollect.QueryPort(nDeviceType, nNetLinkType, nPort);
            }

            sprintf(szSrcIp, "rtsp://%s:%ld/%ld",szIP, nPort, nChannel);
        }
    }
    else
    {
        //memcpy(szSrcIp,szIP, 128);
        strcpy(szSrcIp, szIP); //liujin
    }

    //异步
    //if(m_pThreadPool)
    //{
    //	return -1;
    //return StartDeviceCaptureAsync(szSrcIp, nPort,  nDeviceType, nChannel, lUserData, pOutputRealData, szUser, szPassword,nNetLinkType, nMediaType, sockethandle);
    //}
    //else//同步
    {
        return StartDeviceCaptureSync(szSrcIp, nPort,  nDeviceType, nChannel, 
			pUserData, pOutputRealData, szUser, szPassword,nNetLinkType, nMediaType, sockethandle, 
			szMulticastIp, nMulticastPort, szLocalIP,hmp);
    }
}

//开启采集 RTP
long CDevLinkHandle::StartDeviceCapture(long nDeviceType, const char* szURL, void* pUserData, POUTPUTREALDATA  pOutputRealData)
{
    //异步
    //if(m_pThreadPool)
    //{
    //return -1;
    //return StartDeviceCaptureAsync((char*)szURL,  0,  nDeviceType,  -1, lUserData, pOutputRealData);
    //}
    //else//同步
    {
        return StartDeviceCaptureSync((char*)szURL,  0,  nDeviceType,  -1, pUserData, pOutputRealData);
    }
}


//开启采集
long CDevLinkHandle::StartDeviceCaptureSync(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,
											char* szUser, char* szPassword,long nNetLinkType, long nMediaType, long sockethandle, 
											const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP,void *hmp)
{  
    char szMsg[1024] = "";

#ifdef WIN32
    sprintf(szMsg, "开始设备连接");
#else
    sprintf(szMsg, "Begin to link device");
#endif
    WriteLog(logLevel,"[StartDeviceCaptureSync]", szMsg);

    //通过设备类型获取设备指针 
    CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(nDeviceType);

    if(!pBaseDevice)
    {
#ifdef WIN32
        sprintf(szMsg,"在配置中未找到%d类型的设备，请检查参数和配置文件", nDeviceType);
#else
        sprintf(szMsg,"Not find the type %ld device, please check the configuration file", nDeviceType);
#endif
        WriteLog(logLevel,  "[StartDeviceCaptureSync]", szMsg);

        return -1;
    }

    sprintf(szMsg, "ip=%s, port=%ld, devtype=%ld ,channel=%ld, mediatype=%ld, netLinktype=%ld,userdata=%ld, user=%s, pwd=%s", szIP, nPort, nDeviceType, nChannel, nMediaType, nNetLinkType, (long)pUserData, szUser, szPassword);
    WriteLog(logLevel, "[StartDeviceCaptureSync]", szMsg);

    //判断设备是否登录
    long nLoginHandle = m_DeviceCollect.GetLoginHandle(szIP, nDeviceType);

	if (nChannel <0)
	{
		//rtp不登录
	}
	else
	{
		//如果没登录则登录,获取登录句柄
		if(nLoginHandle < 0)
		{
			nLoginHandle = pBaseDevice->LoginDevice(szIP, nPort, szUser, szPassword);
		}

		if(nLoginHandle < 0 && nLoginHandle != 0xefffffff)
		{
	#ifdef WIN32
			sprintf(szMsg,"登录失败. LoginHandle:%ld",nLoginHandle);
			WriteLog(logLevel, "[StartDeviceCaptureSync]", szMsg);
	#else
			sprintf(szMsg,"Login failed. LoginHandle:%ld",nLoginHandle);
			WriteLog(logLevel,  "[StartDeviceCaptureSync]", szMsg);	
	#endif

			return -2;
		}
	}

    LINKINFO * pLinkinfo = new LINKINFO();
    pLinkinfo->nLoginHandle =  nLoginHandle;
    pLinkinfo->nChannel        = nChannel;
    pLinkinfo->nMediaType  = nMediaType;
    pLinkinfo->nDevType      = nDeviceType;
    pLinkinfo->userData        = pUserData;
    pLinkinfo->pOutputFun = pOutputRealData;
    strcpy(pLinkinfo->szIp,  szIP);

    //是RTP则传URL
    if(pLinkinfo->nChannel < 0)
    {
        pLinkinfo->nLinkHandle = pBaseDevice->StartLinkDevice(szIP);
    }
    else
    {
        //需要登录的设备，传入登录句柄字符串
        if(nLoginHandle != 0xefffffff && nDeviceType == 2)
        {
            char szip[5] = {0};
            sprintf(szip, "%ld", nLoginHandle);

            //开启链接
            pLinkinfo->nLinkHandle = pBaseDevice->StartLinkDevice(szip, nPort , nChannel, nNetLinkType, nMediaType, sockethandle, szMulticastIp, nMulticastPort,szLocalIP);

        }
        else //不需要登录的设备，传IP
        {
            pLinkinfo->nLinkHandle = pBaseDevice->StartLinkDevice(szIP, nPort , nChannel, nNetLinkType, nMediaType, sockethandle, szMulticastIp, nMulticastPort,szLocalIP);
        }
    }

    if(pLinkinfo->nLinkHandle < 0)
    {
#ifdef WIN32
        sprintf(szMsg,"设备连接失败，设备异常或%ld类型的设备库不正确。返回的设备连接句柄为: %d", nDeviceType, pLinkinfo->nLinkHandle);
#else
        sprintf(szMsg,"Link device failed, maybe the device fault or the type %ld device library error. Error code:%ld",nDeviceType, pLinkinfo->nLinkHandle);
#endif	
        WriteLog(logLevel, "[StartDeviceCaptureSync]", szMsg);

        pBaseDevice->LogoutDevice(nLoginHandle);
        delete pLinkinfo; 
        pLinkinfo = NULL;

        return -3;
    }

    long nLinkHandle;
    nLinkHandle = pLinkinfo->nLinkHandle;

    LINKHANDLEINFO linkhdlInfo;
    linkhdlInfo.handle = pLinkinfo;

    //采集
    long ret = pBaseDevice->StartLinkCapture(nLinkHandle, RealDataCallback, pBaseDevice);
    if (ret < 0)
    {
        pBaseDevice->StopLinkDevice(nLinkHandle);
        pBaseDevice->LogoutDevice(nLoginHandle);
        delete pLinkinfo; 
        pLinkinfo = NULL;
#ifdef WIN32
        sprintf(szMsg, "StartLinkCapture失败");
#else
        sprintf(szMsg, "StartLinkCapture failed");
#endif	
        WriteLog(logLevel, "[StartDeviceCaptureSync]", szMsg);

        return -4;
    }

	long link_index = -1;
	{
		boost::unique_lock<boost::shared_mutex> mutex(m_mutex);
		link_index = get_free_linkindex();
		pLinkinfo->operHandle = link_index;
		linkhdlInfo.handle = pBaseDevice->AddLinkInfo(pLinkinfo);  //返回新的指针地址来替换已有的地址

		//AddLinkInfo中拷贝了一份数据，要删掉重复的
		delete pLinkinfo;
		pLinkinfo = NULL;

		m_mapLinkHandle.insert(pair<long, LINKHANDLEINFO>(link_index, linkhdlInfo));

#ifdef WIN32
		sprintf(szMsg,"链接成功，操作句柄为: %d", link_index);
#else
		sprintf(szMsg,"Link success. The operation handle: %ld",link_index);
#endif	
		WriteLog(logLevel, "[StartDeviceCaptureSync]", szMsg);
	}

    return link_index;
}

long CDevLinkHandle::StartDeviceCaptureAsync(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,char* szUser, char* szPassword,long nNetLinkType, long nMediaType, long sockethandle)
{
    return -1;
}

PlayRetStatus CDevLinkHandle::GetPlayRetStatus(long hOperHandle)
{
	boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
    map<long, LINKHANDLEINFO>::iterator itor;
    itor = m_mapLinkHandle.find(hOperHandle);
    if (itor == m_mapLinkHandle.end())
    {
        return STATUS_LOGIN_FAIL;
    }

    LINKINFO* pLinkInfo = (LINKINFO*)itor->second.handle;

    if(pLinkInfo->nLoginHandle != 0xefffffff && pLinkInfo->nLoginHandle < 0)
    {
        return STATUS_LOGIN_FAIL;
    }
    if(pLinkInfo->nLinkHandle == -1/* || pLinkInfo->nLinkHandle == hOperHandle*/)
    {
        return STATUS_LINK_FAIL;
    }
    if(pLinkInfo->nSize == 0)
    {
        return STATUS_GETHEAD_FAIL;
    }

    return STATUS_SUCESS;
}

//停止采集
long CDevLinkHandle::StopDeviceCapture(long nOperHandle, int nThreadIndex)
{
    char szBuf[512];
#ifdef WIN32
    sprintf(szBuf, "开始停止数据连接。操作句柄: %d.",nOperHandle);
#else
    sprintf(szBuf, "Begin to stop the device link. operation handle:%ld", nOperHandle);
#endif	 
    WriteLog(logLevel, "[StopDeviceCapture]", szBuf);

    //如果该链接是最后一路,停止采集，断开链接，如果是最后一路连接，则断开登录
    if(nOperHandle < 0)
    {
#ifdef WIN32
        WriteLog(logLevel, "[StopDeviceCapture]", "Error: 非法的操作句柄");
#else
        WriteLog(logLevel, "[StopDeviceCapture]", "Error:  Invalid handle\n");
#endif	 		 
        return -1;
    }

	LINKINFO* Linkinfo = NULL;
	do 
	{
		boost::unique_lock<boost::shared_mutex> mutex(m_mutex);
		map<long, LINKHANDLEINFO>::iterator itor;
		itor = m_mapLinkHandle.find(nOperHandle);
		if (itor == m_mapLinkHandle.end())
		{
#ifdef WIN32
			WriteLog(logLevel, "[StopDeviceCapture]", "Error: 传入的句柄未找到\r\n");
#else
			WriteLog(logLevel, "[StopDeviceCapture]", "Error: Not find the handle\n");
#endif	 		
			return -1;
		}

		Linkinfo = (LINKINFO*)itor->second.handle;
		if(Linkinfo == NULL)
		{
			WriteLog(logLevel, "[StopDeviceCapture]",  "Error: pLinkinfo invalid");
			return -1;
		}

		Linkinfo->monitorFunc = NULL;
		Linkinfo->pOutputFun = NULL;

		//等待监测线程退出
		if (Linkinfo->monitorThreadFlag != 0)
		{
			Linkinfo->monitorThreadFlag = 0;

		#ifdef WIN32
			if (Linkinfo->monitorHandle != NULL)
			{
				WaitForSingleObject(Linkinfo->monitorHandle, 1000);
				CloseHandle(Linkinfo->monitorHandle);
				Linkinfo->monitorHandle = NULL;
			}
		#else
			if (Linkinfo->monitorHandle != -1)
			{
				struct timespec ts;
				clock_gettime(CLOCK_REALTIME, &ts);
				ts.tv_sec += 1;
				int joinRet = pthread_timedjoin_np(Linkinfo->monitorHandle, NULL, &ts);
				if (joinRet != 0)
				{
					pthread_cancel(Linkinfo->monitorHandle);
				}
				Linkinfo->monitorHandle  = -1;
			}
		#endif
		}

		//::memcpy(&Linkinfo, l, sizeof(LINKINFO));
		m_mapLinkHandle.erase(itor);

	} while (false);

#ifdef WIN32
    sprintf(szBuf, "设备类型: %d, 操作句柄: %d", Linkinfo->nDevType, nOperHandle);
#else
    sprintf(szBuf, "device type: %d, operation handle: %ld", Linkinfo->nDevType, nOperHandle);
#endif
    WriteLog(logLevel, "[StopDeviceCapture]", szBuf);

    CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(Linkinfo->nDevType);
    if(NULL==pBaseDevice)
    {
        WriteLog(0, "[StopDeviceCapture]",   "Error: pBaseDevice==NULL\n");

        return -1;
    }

    int lLoginHandle = 0;
    lLoginHandle = Linkinfo->nLoginHandle;

    if(Linkinfo->nLinkHandle >= 0)
    {
        //断开链接
#ifdef WIN32
        sprintf(szBuf, "开始StopLinkCapture,操作句柄: %ld.", nOperHandle);
#else
        sprintf(szBuf, "Begin StopLinkCapture, operation handle: %ld.", nOperHandle);
#endif
        WriteLog(logLevel, "[StopDeviceCapture]", szBuf);

        pBaseDevice->StopLinkCapture(lLoginHandle);

#ifdef WIN32
        sprintf(szBuf, "结束StopLinkCapture,操作句柄: %ld.", nOperHandle);
#else
        sprintf(szBuf, "End StopLinkCapture, operation handle: %ld.", nOperHandle);
#endif
        WriteLog(logLevel, "[StopDeviceCapture]", szBuf);


#ifdef WIN32
        sprintf(szBuf, "开始StopLinkDevice,操作句柄: %ld.", nOperHandle);
#else
        sprintf(szBuf, "Begin StopLinkDevice, operation handle: %ld.", nOperHandle);
#endif
        WriteLog(logLevel, "[StopDeviceCapture]", szBuf);

        pBaseDevice->StopLinkDevice(Linkinfo->nLinkHandle);

#ifdef WIN32
        sprintf(szBuf, "结束StopLinkDevice,操作句柄: %ld.", nOperHandle);
#else
        sprintf(szBuf, "End StopLinkDevice, operation handle: %ld.", nOperHandle);
#endif
        WriteLog(logLevel, "[StopDeviceCapture]", szBuf);
    }


    //删除链接
    pBaseDevice->DelLinkInfo(Linkinfo->nLinkHandle);

    //是否该IP的全部链接都断开了，RTP没有login也不用logout
    if(Linkinfo->nChannel >=0 && !m_DeviceCollect.IsDeviceLink(lLoginHandle, pBaseDevice)) 
    {
#ifdef WIN32
        sprintf(szBuf, "登出设备, 操作句柄: %ld.", nOperHandle);
#else
        sprintf(szBuf, "Log out device. operation handle: %ld.", nOperHandle);
#endif
        WriteLog(logLevel, "[StopDeviceCapture]", szBuf);

        pBaseDevice->LogoutDevice(lLoginHandle);
    }

#ifdef WIN32
    sprintf(szBuf, "链接停止完成, 操作句柄: %ld\r\n", nOperHandle);
#else
    sprintf(szBuf, "Stop success. operation handle:%ld\n", nOperHandle);
#endif	 
    WriteLog(logLevel, "[StopDeviceCapture]", szBuf);

    return 0;
}

//停止采集
long CDevLinkHandle::StopDeviceCaptureAsyn(long nOperHandle)
{
    return 0;
}

//初始化设备集合
void CDevLinkHandle::InitDeviceCollect(char* szXmlPath, long nStartPort, long nEndPort)
{
    //加载设备
    //	m_DeviceCollect.LoadDevices(szXmlPath, nStartPort, nEndPort);
}

long  CDevLinkHandle::GetDataType(long nOperHandle)
{
	boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
    if(nOperHandle < 0)
        return -1;

    map<long, LINKHANDLEINFO>::iterator itor;
    itor = m_mapLinkHandle.find(nOperHandle);
    if (itor == m_mapLinkHandle.end())
    {
        return -1;
    }

    LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;

    if(!pLinkinfo)
        return -1;

    CBaseDevice* pDevice = m_DeviceCollect.GetDevice(pLinkinfo->nDevType);

    if(!pDevice)
        return -1;

    //如果是纯音频
    if(pLinkinfo->nMediaType >1)
        return pDevice->m_nDataType + 64;
    else
        return pDevice->m_nDataType;
}

//获取设备的状态
long  CDevLinkHandle::GetDeviceType(long nOperHandle)
{
	boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
    map<long, LINKHANDLEINFO>::iterator itor;
    itor = m_mapLinkHandle.find(nOperHandle);
    if (itor == m_mapLinkHandle.end())
    {
        return -1;
    }

    LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;
    return pLinkinfo->nDevType;
}

//获取设备头
long  CDevLinkHandle::GetHeadData(long nOperHandle,char* szHeadData)
{
	boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
    if(szHeadData == NULL)
        return -1;

    map<long, LINKHANDLEINFO>::iterator itor;
    itor = m_mapLinkHandle.find(nOperHandle);
    if (itor == m_mapLinkHandle.end())
    {
        return -1;
    }

    LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;

    int   nSize = 0;

    if(!pLinkinfo)
        return -1;

    memcpy(szHeadData, pLinkinfo->szHead ,  pLinkinfo->nSize);
    nSize = pLinkinfo->nSize;

    return nSize;
}

//XMPP承载RTSP会话接口
////////////////////////////////////////////////////////////////////////////////////////////////////
//开启RTP链路
long CDevLinkHandle::StartLinkDevice(const char *ip, const char* sdp,long sdp_len,long channel,
                                     long link_type, long media_type, long device_type,
                                     void* user_data,POUTPUTREALDATA  p_output_real_data)
{
    //通过设备类型获取设备指针 
    CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(device_type);

    if(!pBaseDevice)
        return -1;

#ifdef WIN32
    WriteLog(logLevel, "[StartLinkDevice]","开始点播");
#else
    WriteLog(logLevel, "[StartLinkDevice]","Begin to link device");
#endif	

    LINKINFO * pLinkinfo = new LINKINFO();
    //memcpy(pLinkinfo->szIp,  ip, 128);
    pLinkinfo->nLoginHandle =  0;
    pLinkinfo->nChannel     = channel;
    pLinkinfo->nMediaType   = media_type;
    pLinkinfo->nDevType     = device_type;
    pLinkinfo->userData = user_data;
    pLinkinfo->pOutputFun = p_output_real_data;
    strcpy(pLinkinfo->szIp, ip);    


    //开启链路
    pLinkinfo->nLinkHandle = pBaseDevice->StartLinkDevice(ip,sdp,sdp_len,channel,link_type,media_type);

    if(pLinkinfo->nLinkHandle < 0)
    {
        delete pLinkinfo; pLinkinfo = NULL;
#ifdef WIN32
        WriteLog(logLevel, "[StartLinkDevice]",    "设备连接失败");
#else
        WriteLog(logLevel, "[StartLinkDevice]",    "Link device failed");
#endif
        return -3;
    }

	LINKHANDLEINFO linkhdlInfo;
    linkhdlInfo.handle = pBaseDevice->AddLinkInfo(pLinkinfo);

    long nLinkHandle;
    nLinkHandle = pLinkinfo->nLinkHandle;
    
    //采集
    int ret = pBaseDevice->StartLinkCapture(nLinkHandle, RealDataCallback, pBaseDevice);

	long link_index = -1;
	{
		boost::unique_lock<boost::shared_mutex> mutex(m_mutex);
		link_index = get_free_linkindex();
		((LINKINFO*)linkhdlInfo.handle)->operHandle = link_index;
		m_mapLinkHandle.insert(pair<long, LINKHANDLEINFO>(link_index, linkhdlInfo));
	}

	delete pLinkinfo;

    return link_index;
}

//开启采集
long CDevLinkHandle::StartLinkCapture(long nOperHandle)
{
    int ret_code = 0;

    do 
    {
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;

        }

        map<long, LINKHANDLEINFO>::iterator itor;
        itor = m_mapLinkHandle.find(nOperHandle);
        if (itor == m_mapLinkHandle.end())
        {
            return -1;
        }

        LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;

        if (!pLinkinfo)
        {
            ret_code =-1;
            break;
        }

        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(pLinkinfo->nDevType);
        if(!pBaseDevice)
        {
            ret_code = -1;
            break;		
        }

        if (!pBaseDevice->StartLinkCapture(pLinkinfo->nLinkHandle, RealDataCallback, pBaseDevice))
        {
            ret_code = -1;
            break;
        }		

    } while (false);

    return ret_code;
}

long CDevLinkHandle::StopLinkDevice(long nOperHandle)
{
    int ret_code = 0;
    do 
    {
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO* pLinkinfo;
		do 
		{
			boost::unique_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(nOperHandle);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			pLinkinfo = (LINKINFO*)itor->second.handle;

			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			pLinkinfo->monitorFunc = NULL;
			pLinkinfo->pOutputFun = NULL;

			//等待监测线程退出
			if (pLinkinfo->monitorThreadFlag != 0)
			{
				pLinkinfo->monitorThreadFlag = 0;

		#ifdef WIN32
				if (pLinkinfo->monitorHandle != NULL)
				{
					WaitForSingleObject(pLinkinfo->monitorHandle, 1000);
					CloseHandle(pLinkinfo->monitorHandle);
					pLinkinfo->monitorHandle = NULL;
				}
		#else
				if (pLinkinfo->monitorHandle != -1)
				{
					struct timespec ts;
					clock_gettime(CLOCK_REALTIME, &ts);
					ts.tv_sec += 1;
					int joinRet = pthread_timedjoin_np(pLinkinfo->monitorHandle, NULL, &ts);
					if (joinRet != 0)
					{
						pthread_cancel(pLinkinfo->monitorHandle);
					}
					pLinkinfo->monitorHandle  = -1;
				}
		#endif
			}

			m_mapLinkHandle.erase(itor);
		} while (false);


        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(pLinkinfo->nDevType);
        if(!pBaseDevice)
        {
            ret_code = -1;
            break;		
        }

        pBaseDevice->StopLinkDevice(pLinkinfo->nLinkHandle);	

    } while (false);

    return ret_code;

}

//关闭采集
long CDevLinkHandle::StopLinkCapture(long nOperHandle)
{
    int ret_code = 0;
    do 
    {
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(nOperHandle);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;
			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);


        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(!pBaseDevice)
        {
            ret_code = -1;
            break;		
        }

        pBaseDevice->StopLinkCapture(link_info.nLinkHandle);	

    } while (false);

    return ret_code;

}
long CDevLinkHandle::GetSDP(long nOperHandle,unsigned char *szSDP, long& nLength)
{
    nLength = 0;

    char szBuf[260];
	sprintf(szBuf, "Begin GetSDP:%ld", nOperHandle);
	WriteLog(logLevel, "[GetSDP]", szBuf);

    int ret_code = 0;
    do 
    {
		boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;
        }

        map<long, LINKHANDLEINFO>::iterator itor;
        itor = m_mapLinkHandle.find(nOperHandle);
        if (itor == m_mapLinkHandle.end())
        {
            return -1;
        }

        LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;
        if (!pLinkinfo)
        {
            ret_code =-1;
            break;
        }

        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(pLinkinfo->nDevType);
        if(!pBaseDevice)
        {
            ret_code = -1;
            break;		
        }

        pBaseDevice->GetSDP(pLinkinfo->nLinkHandle,szSDP,nLength);	

		if (nLength > 0)
		{
			sprintf(szBuf, "devtype=%d, sdpLen=%ld, handle=%d\r\n", pLinkinfo->nDevType, nLength, nOperHandle);
		}
		else
		{
			sprintf(szBuf, "devtype=%d, sdpLen=%ld, handle=%d", pLinkinfo->nDevType, nLength, nOperHandle);
		}
        WriteLog(logLevel, "[GetSDP]", szBuf);

    } while (false);

    return ret_code;
}

long CDevLinkHandle::GetClientInfo(long nOperHandle, long& RtpRecvPort, long& RtcpRecvPort, bool& MultiplexR, long& MultidR)
{
    int ret_code = 0;

    do 
    {
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(nOperHandle);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;
			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);

        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(pBaseDevice == NULL)
        {
            ret_code = -1;
            break;		
        }

        int ret = pBaseDevice->GetClientInfo(link_info.nLinkHandle,RtpRecvPort,RtcpRecvPort,MultiplexR,MultidR);
        if (ret < 0)
        {
            ret_code = -1;
            break;
        }

    } while (false);

    return ret_code;
}

long CDevLinkHandle::create_recv(int track_num, bool demux)
{
    long ret = -1;
    do 
    { 
        const int dev_type = 9;

        //通过设备类型获取设备指针 
        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(dev_type);

        if( NULL == pBaseDevice )
        {
            ret = -1;
            break;
        }

        LINKINFO* pLinkinfo = new LINKINFO();
        if (NULL == pLinkinfo)
        {
            ret = -2;
            break;
        }

        pLinkinfo->nDevType = dev_type;
        pLinkinfo->nLinkHandle = pBaseDevice->create_recv(track_num, demux);

        if ( pLinkinfo->nLinkHandle < 0)
        {
            delete pLinkinfo;
            pLinkinfo = NULL;
            ret = -3;
            break;
        }

		LINKHANDLEINFO linkhdlInfo;

        //增加设备管理  	
        linkhdlInfo.handle = pBaseDevice->AddLinkInfo(pLinkinfo);

		long link_index = -1;
		{
			boost::unique_lock<boost::shared_mutex> mutex(m_mutex);
			link_index = get_free_linkindex();
			((LINKINFO*)linkhdlInfo.handle) ->operHandle = link_index;
			m_mapLinkHandle.insert(pair<long, LINKHANDLEINFO>(link_index, linkhdlInfo));
		}

		delete pLinkinfo;
        //返回设备管理句柄
        ret = link_index;

    } while (false);

    return ret; 
}

long CDevLinkHandle::create_recvinfo(int dev_type, int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
{
    char szMsg[128];
#ifdef WIN32
    sprintf(szMsg,"开始进入create_recvinfo");
#else
    sprintf(szMsg,"Begin create_recvinfo");
#endif	
    WriteLog(logLevel, "[create_recvinfo]", szMsg);

    //通过设备类型获取设备指针 
    CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(dev_type);

    if( NULL == pBaseDevice )
    {
        WriteLog(logLevel, "[create_recvinfo]", "Error, pBaseDevice==NULL");
        return -1;
    }

    LINKINFO* pLinkinfo = new LINKINFO();
    if (NULL == pLinkinfo)
    {
        WriteLog(logLevel, "[create_recvinfo]", "Error, pLinkinfo==NULL");
        return -1;
    }

    pLinkinfo->nDevType = dev_type;
    pLinkinfo->nLinkHandle = pBaseDevice->create_recvinfo(track_num, demux, multicast, multicastip, multiports);

#ifdef WIN32
    sprintf(szMsg,"设备返回的连接句柄: %d", pLinkinfo->nLinkHandle);
#else
    sprintf(szMsg,"The device link handle: %ld",pLinkinfo->nLinkHandle);
#endif	
    WriteLog(logLevel, "[create_recvinfo]", szMsg);

    if ( pLinkinfo->nLinkHandle < 0)
    {
        WriteLog(logLevel, "[create_recvinfo]", "Error, Link device failed");
        delete pLinkinfo;
        pLinkinfo = NULL;
        return -1;
    }

	LINKHANDLEINFO linkhdlInfo;

    //增加设备管理  	
    linkhdlInfo.handle = pBaseDevice->AddLinkInfo(pLinkinfo);
	delete pLinkinfo;

	long link_index = -1;
	{
		boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
		link_index = get_free_linkindex();
		((LINKINFO*)linkhdlInfo.handle)->operHandle = link_index;
		m_mapLinkHandle.insert(pair<long, LINKHANDLEINFO>(link_index, linkhdlInfo));

#ifdef WIN32
		sprintf(szMsg,"链接成功，操作句柄为: %ld", link_index);
#else
		sprintf(szMsg,"Link success. The operation handle: %ld",link_index);
#endif	
		WriteLog(logLevel, "[create_recvinfo]", szMsg);
	} 

    //返回设备管理句柄
    return link_index;
}

long CDevLinkHandle::get_rcvinfo(long link, RCVINFO *infos, int &num)
{
    char szMsg[512];
    sprintf(szMsg,  "link handle: %ld", link);
    WriteLog(logLevel,  "[get_rcvinfo]", szMsg);

    int ret_code = 0;

    do 
    {
        if (link < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(link);
			if (itor == m_mapLinkHandle.end())
			{
				WriteLog(logLevel,  "[get_rcvinfo]", "Not find the link handle");
				return -1;
			}

			LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;
			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false); 

        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(pBaseDevice == NULL)
        {
            ret_code = -1;
            break;		
        }

        int ret = pBaseDevice->get_rcvinfo(link_info.nLinkHandle,infos, num);
        if (ret < 0)
        {
            ret_code = -1;
            break;
        }

    } while (false);

    if (num > 0)
    {
        sprintf(szMsg,  "ret_code:%d, num:%d, rtp: %d, rtcp: %d", ret_code, num, infos[0].port_rtp, infos[0].port_rtcp);
        WriteLog(logLevel,  "[get_rcvinfo]", szMsg);
    }
    else
    {
        sprintf(szMsg,  "error, ret_code:%d, num:%d", ret_code, num);
        WriteLog(logLevel,  "[get_rcvinfo]", szMsg);
    }


    return ret_code;
}
long CDevLinkHandle::request_iframe(long link)
{
    int ret_code = 0;

    do 
    {
        if (link < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(link);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			LINKINFO* pLinkinfo = static_cast<LINKINFO*>(itor->second.handle);
			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);

        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(pBaseDevice == NULL)
        {
            ret_code = -1;
            break;		
        }

        int ret = pBaseDevice->request_iframe(link_info.nLinkHandle);
        if (ret < 0)
        {
            ret_code = -1;
            break;
        }

    } while (false);

    return ret_code;

}

long CDevLinkHandle::set_sdp(long link, const char *sdp, unsigned int len_sdp)
{
    char szMsg[128];
    sprintf(szMsg,  "Start set_sdp. link:%ld, sdplen: %u", link, len_sdp);
    WriteLog(logLevel,  "[set_sdp]", szMsg);

    int ret_code = 0;

    do 
    {
        if (link < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(link);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			LINKINFO* pLinkinfo = static_cast<LINKINFO*>(itor->second.handle);
			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			//缓存系统头
			pLinkinfo->nSize = len_sdp;
			::memcpy(pLinkinfo->szHead,sdp,pLinkinfo->nSize);

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);

        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(pBaseDevice == NULL)
        {
            ret_code = -1;
            break;		
        }

        int ret = pBaseDevice->set_sdp(link_info.nLinkHandle, sdp, len_sdp);
        sprintf(szMsg,  "The setting result: %d", ret);
        WriteLog(logLevel,  "[set_sdp]", szMsg);
        if (ret < 0)
        {
            ret_code = -1;
            break;
        }

    } while (false);

    return ret_code;
}

long CDevLinkHandle::start_link_captuer(const long link_handel,POUTPUTREALDATA data_out_cb,void* user_data)
{
    char szMsg[128];
    sprintf(szMsg,  "Start start_link_captuer. link:%ld, userdata: %p", link_handel, user_data);
    WriteLog(logLevel,  "[start_link_captuer]", szMsg);

    long ret_code=0;

    do 
    {
        if (link_handel < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			std::map<long, LINKHANDLEINFO>::iterator itr = m_mapLinkHandle.find(link_handel);
			if (itr == m_mapLinkHandle.end())
			{
				ret_code = -2;
				break;
			}

			LINKINFO* pLinkinfo = static_cast<LINKINFO*>(itr->second.handle);
			if (!pLinkinfo)
			{
				ret_code =-4;
				break;
			}

			//设置用户数据与回调
			pLinkinfo->userData = user_data;
			pLinkinfo->pOutputFun = (POUTPUTREALDATA)data_out_cb;

			//不用登录
			pLinkinfo->nLoginHandle = 0xefffffff;

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);


        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(pBaseDevice == NULL)
        {
            ret_code = -5;
            break;		
        }

        //捕获数据
        ret_code = pBaseDevice->StartLinkCapture(link_info.nLinkHandle, RealDataCallback, pBaseDevice);
        if (ret_code < 0)
        {
            WriteLog(logLevel, "[start_link_captuer]",  "StartLinkCapture fail!!"); 

            ret_code = -6;
            break;
        }

		pBaseDevice->DelLinkInfo(link_info.nLinkHandle);
		pBaseDevice->AddLinkInfo(&link_info);

    } while (false);

    return ret_code;
}

long CDevLinkHandle::regist_call_back(long device_type,regist_call_back_t func)
{
    CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(device_type);
    if (pBaseDevice != NULL)
    {
        pBaseDevice->set_regist_callback(func);
        return 0;
    }

    return -1;
}

long CDevLinkHandle::GetTrack(long nOperHandle,  _TRACKINFO * pTrackInfo)
{
	boost::shared_lock<boost::shared_mutex> mutex(m_mutex);

    if (nOperHandle < 0 || pTrackInfo == NULL)
    {
        return -1;
    }

    map<long, LINKHANDLEINFO>::iterator itor;
    itor = m_mapLinkHandle.find(nOperHandle);
    if (itor == m_mapLinkHandle.end())
    {
        return -1;
    }

    LINKINFO* pLinkInfo = (LINKINFO*)itor->second.handle;
    if (NULL == pLinkInfo)
    {
        return -1;
    }

    if(pLinkInfo->nSize == 0)
    {
        return 0;
    }

    SessionDescription  sd;
    if(!sd.parse((char *)pLinkInfo->szHead, pLinkInfo->nSize))
    {
        return -3;
    }

    int	nCount = sd.getMediaCount();
    MediaDescription md;
    for(int i = 0 ; i< nCount; i++)
    {
        md = sd.getMedia(i);

        string sControl =  md.getControl();
        string::size_type t =sControl.rfind("trackID=");

        if(t == string::npos)
        {
            //track2情况
            t =sControl.rfind("track");
            if(t == string::npos)
            {
                continue;
            }
            else
            {
                t += 5;
            }
        }
        else
        {
            t += 8;
        }

        string s = sControl.substr(t);

        pTrackInfo->trackId = atoi(s.c_str());

        ::memset(pTrackInfo->trackname, 0, 64);
        if(md.media == "video")
        {
            pTrackInfo->trackType = 0; 
            ::strcpy(pTrackInfo->trackname,"video");
        }			
        else if(md.media == "audio")
        {
            pTrackInfo->trackType = 1;
            ::strcpy(pTrackInfo->trackname,"audio");
        }	
        else
        {
            pTrackInfo->trackType = -1;
            ::strcpy(pTrackInfo->trackname,"na");
        }

        pTrackInfo++;
    }
    return nCount;
}

long  CDevLinkHandle::TcpPlayCtrl(const long nOperHandle,double npt,float scale, unsigned long *rtp_pkt_timestamp)
{
    int ret_code = 0;
    do 
    {
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(nOperHandle);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;

			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);

        
        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(!pBaseDevice)
        {
            ret_code = -1;
            break;		
        }

        ret_code = pBaseDevice->TcpPlayCtrl(link_info.nLinkHandle,npt,scale, rtp_pkt_timestamp);

    } while (false);

    return ret_code;
}

long  CDevLinkHandle::TcpPauseCtrl(const long nOperHandle)
{
    int ret_code = 0;
    do 
    {
        if (nOperHandle < 0)
        {
            ret_code = -1;
            break;
        }

		LINKINFO link_info;
		do 
		{
			boost::shared_lock<boost::shared_mutex> mutex(m_mutex);
			map<long, LINKHANDLEINFO>::iterator itor;
			itor = m_mapLinkHandle.find(nOperHandle);
			if (itor == m_mapLinkHandle.end())
			{
				return -1;
			}

			LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;

			if (!pLinkinfo)
			{
				ret_code =-1;
				break;
			}

			::memcpy(&link_info, pLinkinfo, sizeof(LINKINFO));
		} while (false);        


        CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(link_info.nDevType);
        if(!pBaseDevice)
        {
            ret_code = -1;
            break;		
        }

        ret_code = pBaseDevice->TcpPauseCtrl(link_info.nLinkHandle);

    } while (false);

    return ret_code;

}

long CDevLinkHandle::SetDataMonitor(long nOperHandle, DataMonitorCB monitorFunc, int intervalTime, void* pContext)
{
	map<long, LINKHANDLEINFO>::iterator itor;
	itor = m_mapLinkHandle.find(nOperHandle);
	if (itor == m_mapLinkHandle.end())
	{
		return -1;
	}

	LINKINFO* pLinkinfo = (LINKINFO*)itor->second.handle;
	if(pLinkinfo == NULL)
	{
		return -2;
	}

	//如果不设监测间隔时间，就采用默认值
	if (intervalTime <=0)
	{
		intervalTime = 20;
	}

	//每个连接对应一个数据监测
	pLinkinfo->monitorFunc = (void*)monitorFunc;
	pLinkinfo->monitorIntervalTime = intervalTime;
	pLinkinfo->monitorContext = pContext;

	if (pLinkinfo->monitorFunc != NULL)
	{
		pLinkinfo->monitorThreadFlag = 1;
#ifdef WIN32
		pLinkinfo->monitorHandle = CreateThread(NULL, 0, ThreadMonitorData, pLinkinfo, 0, NULL);
#else
		int ret = pthread_create(&pLinkinfo->monitorHandle, NULL, &ThreadMonitorData, pLinkinfo);
		if (ret != 0)
		{
			pLinkinfo->monitorThreadFlag = 0;
			pLinkinfo->monitorHandle = -1;
		}
#endif
	}
	else
	{
		pLinkinfo->monitorThreadFlag = 0;
	}

	return 0;
}

long CDevLinkHandle::SetSSRCReport(long nOperHandle, rtcp_report_callback_t reportFunc, void* pContext)
{
	map<long, LINKHANDLEINFO>::iterator itor;
	itor = m_mapLinkHandle.find(nOperHandle);
	if (itor == m_mapLinkHandle.end())
	{
		return -1;
	}

	LINKINFO* pLinkInfo = (LINKINFO*)itor->second.handle;
	if (NULL == pLinkInfo)
	{
		return -1;
	}

	CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(pLinkInfo->nDevType);
	if(pBaseDevice == NULL)
	{
		return -2;
	}

	return pBaseDevice->SetSSRCReport(pLinkInfo->nLinkHandle, reportFunc, pContext);
}

long CDevLinkHandle::SetResend(long device_type,int resend,int wait_resend, int max_resend, int vga_order)
{
	CBaseDevice* pBaseDevice = m_DeviceCollect.GetDevice(device_type);
	if (pBaseDevice != NULL)
	{
		return pBaseDevice->SetResend(resend,wait_resend,max_resend,vga_order);
	}
	return -1;
}