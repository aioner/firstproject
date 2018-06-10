//继承设备基类,实现大华设备子类媒体接口
//组合老版DLL_DeviceDH类，获得大华设备类老版全部功能
//MediaDevice2.0中基类指针指向不同的设备子类，导出统一的dll接口
//add 2015/8/4

#ifndef DEVICE_API_H
#define DEVICE_API_H

#define NOT_XT_CONFIG_
#include "BaseDevice.h"

#ifdef _WIN32
#ifndef  DHDVRDEVICE_EXPORTS
#define  DHDVRDEVICEAPI	__declspec(dllimport)
#else
#define  DHDVRDEVICEAPI	__declspec(dllexport)
#endif

#else
#define DHDVRDEVICEAPI __attribute__((visibility("default")))
#endif

class CDevice_DH;
class DHDvrDevice : public CBaseDevice
{
public:
	DHDvrDevice(CDevice_DH *pDevice_DH);
	~DHDvrDevice(void);

public:
	//实现的大华设备操作接口
	////////////////////////////////////////////
	//初始化设备
	long InitDevice(void* pParam);

	//反初始化设备
	long UnInitDevice(void* pParam);

	//设备登录
	long LoginDevice(const char* szDeviceIP, long  nPort, const char* szUserID, const char* szPassword);

	//设备登出
	bool LogoutDevice(long lLoginHandle);

	//开启设备链接
	long StartLinkDevice(char *szDeviceIP, long nNetPort ,long nChannel,long nLinkType,long nMediaType, long sockethandle = 0, const char *szMulticastIp = NULL, unsigned short nMulticastPort = 0);

	//关闭设备链接
	void StopLinkDevice(long lDeviceLinkHandle);

	//开启采集
	long StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext);

	//关闭采集
	long  StopLinkCapture(long lDeviceLinkHandle);

	long GetSDP(long lDeviceLinkHandle, unsigned char *msg, long& length);

private:
	CDevice_DH *m_pDevice_DH;
};

#ifdef __cplusplus
extern "C"{
#endif
DHDVRDEVICEAPI CBaseDevice* AddMe(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType);
#ifdef __cplusplus
};
#endif

#endif
