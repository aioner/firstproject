#include "Device_DH.h"
#include "DHDvrDevice.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32

#else

#define __declspec(dllexport)
#define CALLBACK

#endif

const char* DH_VER_INFO = "XT_Lib_Version: V_DH_1.00.1208.0";

CBaseDevice __declspec(dllexport) * AddMe(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType)
{
	return AddA(new DHDvrDevice(new CDevice_DH), pDeviceCollect, nDeviceLinkType, nDataType);
}

DHDvrDevice::DHDvrDevice( CDevice_DH *pDevice_DH )
{
	m_pDevice_DH = pDevice_DH;
}

DHDvrDevice::~DHDvrDevice(void)
{
	delete m_pDevice_DH;
	m_pDevice_DH = NULL;
}

/////////////////////////////////////////////
//实现大华设备媒体接口
//返回值处理：成功返回0，失败返回-1

long DHDvrDevice::InitDevice( void* pParam )
{
	//获取设备类型 数据类型
	return m_pDevice_DH->New_InitDevice(5, 5);
}

long DHDvrDevice::UnInitDevice( void* pParam )
{
	m_pDevice_DH->New_ExitDevice();
	return 0;
}

long DHDvrDevice::LoginDevice( const char* szDeviceIP, long nPort, const char* szUserID, const char* szPassword )
{
	long lLoginHandle = m_pDevice_DH->EDvr_Login((char*)szDeviceIP, nPort, (char*)szUserID, (char*)szPassword, TRUE);
	WriteLog("DLL_DeviceDH", "登陆设备 IP:%s, Port:%d, Name:%s, Pass:%s, Ret:%d", szDeviceIP, nPort, szUserID, szPassword, lLoginHandle);

	return lLoginHandle;
}

bool DHDvrDevice::LogoutDevice(long lLoginHandle)
{
	m_pDevice_DH->New_LogoutDevice(lLoginHandle);
	return 0;
}

long DHDvrDevice::StartLinkDevice( char *szDeviceIP, long nNetPort ,long nChannel,long nLinkType,long nMediaType, long sockethandle /*= 0*/, const char *szMulticastIp, unsigned short nMulticastPort )
{
	return m_pDevice_DH->New_StartLinkDevice(szDeviceIP, nChannel, nLinkType, nNetPort, nMediaType);
}

void DHDvrDevice::StopLinkDevice( long lDeviceLinkHandle )
{
	m_pDevice_DH->New_StopLinkDevice(lDeviceLinkHandle);
}

long DHDvrDevice::StartLinkCapture( long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback, void* UserContext )
{
	return m_pDevice_DH->New_StartLinkCapture(lDeviceLinkHandle, lpRealDatacallback, UserContext);
}

long DHDvrDevice::StopLinkCapture( long lDeviceLinkHandle )
{
	m_pDevice_DH->New_StopLinkCapture(lDeviceLinkHandle);
	return 0;
}

long DHDvrDevice::GetSDP(long lDeviceLinkHandle, unsigned char *msg, long& length)
{
	m_pDevice_DH->New_GetHeadData(lDeviceLinkHandle, msg);
	length = strlen((char*)msg);

	return 0;
}
