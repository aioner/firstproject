//封装支持Onvif协议的VGA接口
//该类主要调用其他操作接口，实现抽象层需要的接口
#ifndef DVRONVIF_H__
#define DVRONVIF_H__
#include "BaseDevice.h"
#include "xt_media_client.h"
#include <map>

using namespace std;

#ifdef WIN32
#ifndef  ONVifDVRDEVICE_EXPORTS
#define  ONVifDVRDEVICEAPI	__declspec(dllimport)
#else
#define  ONVifDVRDEVICEAPI	__declspec(dllexport)
#endif

#else
#include "xt_config.h"
#include <pthread.h>
#define ONVifDVRDEVICEAPI __attribute__((visibility("default")))
#endif

class DvrOnvif : public CBaseDevice
{
public:
	DvrOnvif(void);
	~DvrOnvif(void);

public:
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

	long GetSDP(long link_hadle,unsigned char *msg, long& length);

	static void __stdcall s_media_client_frame_cb(void *ctx, xt_media_link_handle_t link, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc);
	static int32_t  __stdcall  s_ports_mgr_get_cb(void *ctx, uint32_t num, uint16_t *ports, int32_t opt);
	static void  __stdcall s_ports_mgr_free_cb(void *ctx, uint32_t num, uint16_t *ports);
private:
	OV_PRealDataCallback data_cb_;
	regist_call_back_t  regist_cb_;
	map<long, xt_media_link_handle_t> link_handle_map;
	long index_map;
	void *data_ctx_;
};

#ifdef __cplusplus
extern "C"{
#endif
ONVifDVRDEVICEAPI CBaseDevice* AddMe(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType);
#ifdef __cplusplus
};
#endif
#endif //#ifndef DVRONVIF_H__
