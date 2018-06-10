//#include "stdafx.h"

#include <vector>
#include <string.h>

#include "DvrOnvif.h"
#include "CommonDef.h"
#include "xt_media_client.h"
#include "xt_media_client_types.h"

extern struct soap tsoap;
extern char* g_szMediaAddress;
extern int g_nPort;
extern char g_szUserName[128];
extern char g_szPassword[128];

const char* XT_LIB_INFO = "XT_Lib_Version: V1.2016.0503.0";

#ifdef WIN32
#else
	#define __declspec(dllexport)
#endif

CBaseDevice __declspec(dllexport) * AddMe(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType)
{
    return AddA(new DvrOnvif(), pDeviceCollect, nDeviceLinkType, nDataType);
}

DvrOnvif::DvrOnvif(void)
{
    data_cb_ = NULL;
    regist_cb_ = NULL;
    data_ctx_ = NULL;
	index_map = -1;
}

DvrOnvif::~DvrOnvif(void)
{
}

long DvrOnvif::GetSDP(long link_hadle,unsigned char *msg, long& length)
{
	map<long, void*>::iterator itor = link_handle_map.find(link_hadle);

    if (link_handle_map.end() == itor)
    {
        WriteLog(c_szFlag, "GetSDP::Not find the handle");
        return -1;
    }
	xt_media_link_handle_t link = itor->second;

    xt_media_client_status_t stat = xt_media_client_get_header(link, (uint8_t *)msg, ((uint32_t*)&length));
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        WriteLog(c_szFlag, "DvrOnvif::GetSDP xt_media_client_get_header return fail");
        return -1;
    }
    WriteLog(c_szFlag, "DvrOnvif::GetSDP ok. length=%d", length);
    return 0;
}

int32_t DvrOnvif::s_ports_mgr_get_cb(void *ctx, uint32_t num, uint16_t *ports, int32_t opt)
{
    DvrOnvif *pThis = static_cast<DvrOnvif *>(ctx);
    if (NULL == pThis)
    {
        return -1;
    }
    std::vector<long> vports(num);
    if (!pThis->GetPorts(num, &vports[0], (1 == opt)))
    {
        return -2;
    }

    std::copy(vports.begin(), vports.end(), ports);
    return 0;
}

void DvrOnvif::s_ports_mgr_free_cb(void *ctx, uint32_t num, uint16_t *ports)
{
    DvrOnvif *pThis = static_cast<DvrOnvif *>(ctx);
    if (NULL != pThis)
    {
        std::vector<long> vports(num);
        std::copy(ports, ports + num, vports.begin());
        pThis->FreePorts(&vports[0], num);
    }
}

long DvrOnvif::InitDevice( void* pParam )
{
    EnableLog(true);
    return SoapInit();
}

long DvrOnvif::UnInitDevice( void* pParam )
{
    SoapUninit();
    return 0;
}

long DvrOnvif::LoginDevice( const char* szDeviceIP, long nPort, const char* szUserID, const char* szPassword )
{
    strcpy(g_szUserName, szUserID);
    strcpy(g_szPassword, szPassword);
    WriteLog(c_szFlag,"OV_LoginDevice用户名:%s 密码:%s", g_szUserName, g_szPassword);
    return 0;
}

bool DvrOnvif::LogoutDevice( long lLoginHandle )
{
    return 0;
}

long DvrOnvif::StartLinkDevice( char *szDeviceIP, long nNetPort ,long nChannel,long nLinkType,long nMediaType, long sockethandle /*= 0*/, const char *szMulticastIp/* = NULL*/, unsigned short nMulticastPort/* = 0*/)
{
    WriteLog(c_szFlag,"OV_StartLinkDevice开始连接设备 设备ip:%s 通道号:%d 码流类型:%d 端口号:%d",szDeviceIP,nChannel,nMediaType,nNetPort);
    char szDeviceUrl[512] = {0};
	char szRtspUrl[512] = {0};
	g_nPort = nNetPort;
    char* ret = Onvif_MakeDeviceUrl(&tsoap, szDeviceIP, szDeviceUrl);

    WriteLog(c_szFlag, "Onvif_MakeDeviceUrl http is =%s", ret);
    if (ret == NULL)
    {
        WriteLog(c_szFlag, "Onvif_MakeDeviceUrl http error");
        return -1;
    }
    ret = Onvif_MakeChannelUrl(&tsoap, szDeviceUrl, szDeviceIP, nChannel, nMediaType, szRtspUrl);

    WriteLog(c_szFlag, "Onvif_MakeDeviceUrl rtsp is =%s", ret);
    if (ret == NULL)
    {
        WriteLog(c_szFlag, "Onvif_MakeDeviceUrl rtsp error");
        return -1;
    }

    xt_media_link_handle_t link = NULL;
    xt_media_client_status_t stat = xt_media_client_rtsp_link(szRtspUrl, 0, false, &link);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        WriteLog(c_szFlag, "call xt_media_client_rtsp_link fail. stat=%d", stat);
        return -1;
    }

	index_map++;
	link_handle_map.insert(pair<long, xt_media_link_handle_t>(index_map, link));

    return index_map;
}

void DvrOnvif::StopLinkDevice( long lDeviceLinkHandle )
{
	map<long, void*>::iterator itor = link_handle_map.find(lDeviceLinkHandle);

	if (link_handle_map.end() == itor)
	{
		WriteLog(c_szFlag, "StopLinkDevice::Not find the handle");
		return;
	}
	xt_media_link_handle_t link = itor->second;

    if (NULL != link)
    {
        xt_media_client_close_link(link);
    }

	link_handle_map.erase(itor);
}

void XT_MEDIA_CLIENT_STDCALL DvrOnvif::s_media_client_frame_cb(void *ctx, xt_media_link_handle_t link, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
{
    DvrOnvif *pThis = static_cast<DvrOnvif *>(ctx);
	if (pThis == NULL)
	{
		WriteLog(c_szFlag, "s_media_client_frame_cb::pThis==NULL");
		return;
	}

	long handle = -1;
	for (map<long, void*>::iterator itor=pThis->link_handle_map.begin(); itor !=pThis->link_handle_map.end();itor++)
	{
		if (itor->second == link)
		{
			handle = itor->first;
			break;
		}
	}

	if (handle == -1)
	{
		WriteLog(c_szFlag, "s_media_client_frame_cb::Not find the handle:%d", (long)link);
		return;
	}

    if (NULL != pThis && pThis->data_cb_)
    {
        pThis->data_cb_(handle, frame_type, (unsigned char *)data, length, data_type, pThis->data_ctx_, timestamp,ssrc);
    }
}

long DvrOnvif::StartLinkCapture( long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext )
{
	map<long, void*>::iterator itor = link_handle_map.find(lDeviceLinkHandle);

	if (link_handle_map.end() == itor)
	{
		WriteLog(c_szFlag, "StartLinkCapture::Not find the handle");
		return -1;
	}
	xt_media_link_handle_t link = itor->second;

    if (NULL == link)
	{
		WriteLog(c_szFlag, "StartLinkCapture::link==NULL");
        return -1;
	}

    data_cb_ = lpRealDatacallback;
    data_ctx_ = UserContext;

    xt_media_client_status_t stat = xt_media_client_play(link, &s_media_client_frame_cb, this);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        return -1;
    }

    uint8_t header_msg[2048] = "";
    unsigned int  head_len = 2048;
    unsigned int frame_type = 128; 
    stat = xt_media_client_get_header(link,header_msg,&head_len);

    if (MEDIA_CLIENT_STATUS_OK != stat)
        return -1;

	if (data_cb_ != NULL)
	{
		data_cb_(reinterpret_cast<long>(link), frame_type, header_msg, head_len, 0, data_ctx_, 0, 0);
	}

    return 0;
}

long DvrOnvif::StopLinkCapture( long lDeviceLinkHandle )
{
    return 0;
}
