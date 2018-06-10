#ifndef _DEVLINKHANDLE_H_
#define _DEVLINKHANDLE_H_

#ifdef WIN32

#ifndef  DEVICELINK_EXPORTS
#define  DEVICELINKAPI	__declspec(dllimport)
#else
#define  DEVICELINKAPI	__declspec(dllexport)
#endif

#else

#define DEVICELINKAPI __attribute__ ((visibility("default")))
#include "xt_config.h"
#include <string.h>
#endif

#include "BaseDevice.h"
#include "boost/thread.hpp"
#include "boost/thread/shared_mutex.hpp"

typedef long (__stdcall *DataMonitorCB)(long operHandle, long videoNum, long audioNum, void* pContext);

using namespace std;

struct CAPTUREINFO
{
	char          szIp[256];
	int             nPort;
	int             nDeviceType;
	int             nChannel;
	void*        lUserData;
	char          szUser[32];
	char          szPassword[32];
	int             nNetLinkType;
	int             nMediaType;
	POUTPUTREALDATA pOutputRealData;
	LINKINFO*  pLinkInfo;
};

enum PlayRetStatus 
{
    STATUS_NONE,
	STATUS_LOGIN_FAIL,
	STATUS_LINK_FAIL,
	STATUS_GETHEAD_FAIL,
	STATUS_SUCESS
};

struct _TRACKINFO
{
	int trackId;        //����ID
	int trackType;  //�������͡�0����Ƶ��1����Ƶ����1������
	char trackname[64];
};


typedef struct _UserContext
{
	long index;
	CBaseDevice *plink;
}UserContext_t;

//�ⲿ����ص�����

//#define MAKEWORD_RET(plinkhandle,index) (((__int64)(((__int64)plinkhandle) & 0xffffffff)) | ((__int64)((((__int64)index) & 0xffffffff) << 32)))
//#define GET_LINKHANDLE(RET)      ((int)(((__int64)RET) & 0xffffffff))
//#define GET_INDEX(RET)           ((int)((((__int64)RET) >> 32) & 0xffffffff))

#ifdef __cplusplus
extern "C" {
#endif

struct LINKHANDLEINFO
{
	void * handle;  //�豸���Ӿ����ָ��

	LINKHANDLEINFO()
	{
		handle = NULL;
	}
};

class  CDevLinkHandle
{
 //  friend class ThreadPool;
public:
	CDevLinkHandle(void);
	~CDevLinkHandle(void);

	//��ʼ���豸
	long InitializeDevice(long  nDeviceType, void* pParam=NULL);


	long InitializeDeviceEx(long  nDeviceType,xt_client_cfg_t xtCfg);

	//����ʼ���豸
	long UnInitializeDevice(long  nDeviceType, void* pParam=NULL);

	//XMPP����RTSP�Ự�ӿ�
	////////////////////////////////////////////////////////////////////////////////////////////////////
	//����RTP��·
	long StartLinkDevice(const char *ip, const char* sdp,long sdp_len,long channel,
		long link_type, long media_type, long device_type,void* user_data,POUTPUTREALDATA  p_output_real_data);
	
	//�ر�RTP��·
	long StopLinkDevice(long lDeviceLinkHandle);
										
	//�����ɼ�
	long StartLinkCapture(long hOperHandle);

	//�رղɼ�
	long  StopLinkCapture(long hOperHandle);

	long GetSDP(long hOperHandle,unsigned char *szSDP, long& nLength);

	long GetClientInfo(long lDeviceLinkHandle,long& RtpRecvPort,long& RtcpRecvPort,bool& MultiplexR, long& MultidR);
	
	long create_recv(int track_num, bool demux);

	long create_recvinfo(int dev_type, int track_num, bool demux, bool multicast, const char* multicastip, int* multiports);

	long get_rcvinfo(long link, RCVINFO *infos, int &num);

	long set_sdp(long link, const char *sdp, unsigned int len_sdp);

	//����I֡
	long request_iframe(long link);
	long start_link_captuer(const long link_handel,POUTPUTREALDATA data_out_cb,void* user_data);
    
    long regist_call_back(long device_type,regist_call_back_t func);

	////////////////////////////////////////////////////////////////////////////////////////////////////

	//��ͳ������Լ�����Э�̽ӿ�
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//��ʼ�ɼ�
	long StartDeviceCapture(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,
		char* szUser, char* szPassword,long nNetLinkType, long nMediaType, long sockethandle, 
		const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP="0.0.0.0",
		void *hmp=NULL);

	//�����ɼ� RTP
	long StartDeviceCapture(long  nDeviceType, const char* szURL, void* pUserData, POUTPUTREALDATA  pOutputRealData);

	//ֹͣ�ɼ�
	long StopDeviceCapture(long nOperHandle , int nThreadIndex = 10);
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	//ֹͣ�ɼ�
	long StopDeviceCaptureAsyn(long nOperHandle);

	//��ʼ���豸��
	void InitDeviceCollect(char* szXmlPath, long nStartPort = 16000, long nEndPort = 17000);

	//��ȡ�豸ͷ
	long  GetHeadData(long nOperHandle,char* szHeadData);

	//��ȡ������������
	long  GetDataType(long nOperHandle);

    //��ȡ�豸��״̬
	long  GetDeviceType(long nOperHandle);

	//��ȡý��������Ϣ
	long GetTrack(long nOperHandle,  _TRACKINFO * pTrackInfo);

	long  TcpPlayCtrl(const long OperHandle,double npt,float scale, unsigned long *rtp_pkt_timestamp);

	long  TcpPauseCtrl(const long OperHandle);

	//���ö��߼��
	long SetDataMonitor(long nOperHandle, DataMonitorCB monitorFunc, int intervalTime, void* pContext);

	long SetSSRCReport(long nOperHandle, rtcp_report_callback_t reportFunc, void* pContext);

	//���ö����ش�����
	long SetResend(long device_type, int resend, int wait_resend, int max_resend, int vga_order);

	//��ȡ��������״̬
	PlayRetStatus GetPlayRetStatus(long hOperHandle);

private:

	//��ӵ����в�����һ���¼�
	void AddListEvent();

	//�����ɼ�
	long StartDeviceCaptureAsync(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,char* szUser = NULL, char* szPassword = NULL,long nNetLinkType = 0, long nMediaType = 0, long sockethandle = 0);
	
    //ͬ��
	long StartDeviceCaptureSync(char* szIP,long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,char* szUser = NULL, char* szPassword = NULL,long nNetLinkType = 0, long nMediaType = 0,  long sockethandle = 0, 
		const char *szMulticastIp = NULL, unsigned short nMulticastPort = 0,const char *szLocalIP="0.0.0.0",
		void *hmp=NULL);

public:
	//�豸����
	CDeviceCollect    m_DeviceCollect;

private:
	long get_free_linkindex();

	map<long, LINKHANDLEINFO> m_mapLinkHandle;

	boost::shared_mutex m_mutex;
};

#ifdef __cplusplus
};
#endif

#ifdef WIN32
#ifndef  DEVICELINK_EXPORTS	
#pragma comment(lib, "DeviceLink.lib")
#pragma message("�Զ����� DeviceLink.lib") 
#endif
#endif

#endif //_DEVLINKHANDLE_H_