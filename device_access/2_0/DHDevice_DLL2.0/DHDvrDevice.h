//�̳��豸����,ʵ�ִ��豸����ý��ӿ�
//����ϰ�DLL_DeviceDH�࣬��ô��豸���ϰ�ȫ������
//MediaDevice2.0�л���ָ��ָ��ͬ���豸���࣬����ͳһ��dll�ӿ�
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
	//ʵ�ֵĴ��豸�����ӿ�
	////////////////////////////////////////////
	//��ʼ���豸
	long InitDevice(void* pParam);

	//����ʼ���豸
	long UnInitDevice(void* pParam);

	//�豸��¼
	long LoginDevice(const char* szDeviceIP, long  nPort, const char* szUserID, const char* szPassword);

	//�豸�ǳ�
	bool LogoutDevice(long lLoginHandle);

	//�����豸����
	long StartLinkDevice(char *szDeviceIP, long nNetPort ,long nChannel,long nLinkType,long nMediaType, long sockethandle = 0, const char *szMulticastIp = NULL, unsigned short nMulticastPort = 0);

	//�ر��豸����
	void StopLinkDevice(long lDeviceLinkHandle);

	//�����ɼ�
	long StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext);

	//�رղɼ�
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
