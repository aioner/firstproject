#ifndef _DATAFILTER_H_
#define _DATAFILTER_H_

#ifdef _WIN32
#ifndef  LINKCOMM_EXPORTS
#define  LINKCOMMAPI	__declspec(dllimport)
#else
#define  LINKCOMMAPI	__declspec(dllexport)
#endif

#else
#include "xt_config.h"
#define LINKCOMMAPI __attribute__((visibility("default")))
#endif

#include <list>
#include <map>
using namespace std;

#define MAX_CHANNELS  32
#define MAX_HEAD_SIZE 2048
#define MAX_LINKS     1024

//回调函数  
typedef long (__stdcall *OV_PRealDataCallback)(long nLinkHandle, long nFrameType, unsigned char*	pDataBuf, long	nDataLength, long nDataType, void* objUser, long nTimeStamp, unsigned long nSSRC);

#ifndef POUTPUTREALDATA__
#define POUTPUTREALDATA__

struct xt_client_cfg_t
{
	char udp_session_bind_ip[32];
	unsigned short udp_session_bind_port;
	unsigned short udp_session_heartbit_proid; //udp会话保活心跳周期 单位：毫秒 默认配置为:20
	unsigned short udp_session_request_try_count;  //udp会话操作失败后重试次数 默认配置为:4
	unsigned short udp_session_request_one_timeout;  //udp会话操作等待时间 默认配置为:5000

	char tcp_session_bind_ip[32];
	unsigned short tcp_session_bind_port;
	unsigned short tcp_session_connect_timeout; 	//tcp会话连接超时时间，单位：毫秒 默认配置为:10000
	unsigned short tcp_session_login_timeout;	   	//tcp会话登录超时时间，单位：毫秒 默认配置为:10000
	unsigned short tcp_session_play_timeout;		//tcp会话点播超时时间，单位：毫秒 默认配置为:2000
	unsigned short tcp_session_stop_timeout;		//tcp会话停点超时时间，单位：毫秒 默认配置为:2000

	unsigned short rtsp_session_connect_timeout;		//rtsp会话连接超时时间，单位：毫秒 默认配置为:10000
	unsigned short rtsp_session_describe_timeout;		//rtsp会话describe超时时间，单位：毫秒 默认配置为:10000
	unsigned short rtsp_session_setup_timeout;		//rtsp会话setup超时时间，单位：毫秒 默认配置为:10000
	unsigned short rtsp_session_play_timeout;         //rtsp会话play超时时间，单位：毫秒 默认配置为:10000
	unsigned short rtsp_session_pause_timeout;        //rtsp会话pause超时时间，单位：毫秒 默认配置为:10000
	unsigned short rtsp_session_teardown_timeout;     //rtsp会话teardown超时时间，单位：毫秒 默认配置为:1000
} ;

struct RTCP_SR
{
	unsigned int    mNTPtimestamp;  /* Most significant 32bit of NTP timestamp */
	unsigned int    lNTPtimestamp;  /* Least significant 32bit of NTP timestamp */
	unsigned int    timestamp;      /* RTP timestamp */
	unsigned int    packets;        /* Total number of RTP data packets transmitted by the sender since transmission started and up until the time this SR packetwas generated. */
	unsigned int    octets;     /* The total number of payload octets (not including header or padding */
};

struct RTCP_RR
{
	unsigned int    fractionLost;   /* The fraction of RTP data packets from source specified by SSRC that were lost since previous SR/RR packet was sent. */
	unsigned int    cumulativeLost; /* Total number of RTP data packets from source specified by SSRC that have been lost since the beginning of reception. */
	unsigned int    sequenceNumber; /* Sequence number that was received from the source specified by SSRC. */
	unsigned int    jitter;         /* Estimate of the statistical variance of the RTP data packet inter arrival time. */
	unsigned int    lSR;            /* The middle 32 bits of the NTP timestamp received. */
	unsigned int    dlSR;         /* Delay since the last SR. */
};

//数据外抛回调函数
typedef long (__stdcall *POUTPUTREALDATA)(long nLinkHandle, unsigned char* pDataBuf, long nDataLength, long nFrameType, long nDataType,void* objUser, long nTimeStamp, unsigned long nSSRC);

//反向注册回调函数
typedef long (__stdcall *regist_call_back_t)(const char *ip, unsigned short port, const unsigned char *data, unsigned length);

//SSRC报告回调函数
typedef void (__stdcall *rtcp_report_callback_t)(void *ctx, unsigned int ssrc, const RTCP_SR *sr, const RTCP_RR *rr);
#endif//POUTPUTREALDATA__


struct LINKINFO;
//typedef map<long, LINKINFO*> LINKMAP;

struct PORTINFO
{
	int nDevType;
	int nNetLinkType;
	int nPort;
};

struct RCVINFO 
{
	int index;
	unsigned short port_rtp;
	unsigned short port_rtcp;
	bool demux;
	unsigned int demuxid;
};


//标识唯一的一路流链接信息

/************************************************************************************
                                     设备接入基类
*************************************************************************************/
class CDeviceCollect;

class LINKCOMMAPI CBaseDevice
{
public:
	CBaseDevice(void);
	virtual ~CBaseDevice(void);

	//初始化设备
	virtual long InitDevice(void* pParam){return -1;}

	//反初始化设备
	virtual long UnInitDevice(void* pParam){return -1;}

	//设备登录
	virtual long LoginDevice(const char* szDeviceIP, long  nPort, const char* szUserID, const char* szPassword){return 0xefffffff;}

	//设备登出
	virtual bool LogoutDevice(long lLoginHandle){return 0;}

	//开启设备链接，确定数据类型
	virtual long StartLinkDevice(char *szDeviceIP, long nNetPort , long nChannel, long nLinkType, long nMediaType, long sockethandle, 
		const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP){return 0;}

	//开启设备链接，确定数据类型 XMPP
	virtual long StartLinkDevice(const char *ip, const char* sdp, long sdp_len, long channel, long link_type,  long media_type){return 0;}

	//开启设备链接 -URL
	virtual long StartLinkDevice(const char *szURL){return 0;}

	//关闭设备链接
	virtual void StopLinkDevice(long lDeviceLinkHandle){}

	//开启采集
	virtual long StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext){return 0;}

	//关闭采集
	virtual long  StopLinkCapture(long lDeviceLinkHandle){return 0;}

	//取得设备状态
	virtual long  GetDeviceStatus(long lDeviceLinkHandle){return 0;}

	virtual long GetSDP(long lDeviceLinkHandle, unsigned char *msg, long& length){return 0;}

	//文件流化接口
	virtual long TcpPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp){return 0;}
	virtual long TcpPauseCtrl(long lDeviceLinkHandle){return 0;}

	//RTSP接口
	virtual long RtspPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp){return 0;}
	virtual long RtspPauseCtrl(long lDeviceLinkHandle){return 0;}

	//
	virtual long GetClientInfo(long lDeviceLinkHandle, long& rtp_recv_port, long& rtcp_recv_port, bool& multiplex_r, long & multid_r){return 0;}

	//设置SSRC报告
	virtual	long SetSSRCReport(long lDeviceLinkHandle, rtcp_report_callback_t reportFunc, void* pContext){return 0;}

	//设置丢包重传参数
	virtual long SetResend(int resend,int wait_resend, int max_resend, int vga_order){return 0;}

	//////////////////////////////////////////////////////////////////////////
	virtual long create_recv(int track_num, bool demux){return 0;}

	virtual long create_recvinfo(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports){return 0;}

	virtual long get_rcvinfo(long link, RCVINFO *infos, int &num){return 0;}

	virtual long set_sdp(long link, const char *sdp, unsigned int len_sdp){return 0;}

	virtual long request_iframe(long link_hadle){return 0;}
    virtual long set_regist_callback(regist_call_back_t func){return 0;}
	///////////////////////////////////////////////////////////////////////////////

	/********************************************以下由管理类统筹调度,子类不用实现**********************************************/
	//通知数据头
	long NotifyHeadData(long lDeviceLinkHandle, unsigned char* szHead, long nSize);

	//传递插件容器指针
	void  SetDeviceCollect(CDeviceCollect *pDeviceCollect);

	//通过IP获得登录句柄
	long  GetLoginHandle(char* szIp);

	//获取空闲的端口
	bool  GetPorts(long portnum, long *pPorts,  bool bIsSort = false);

	//释放端口
	void FreePorts(long *pPorts, long portnum);

	//获取媒体类型
	long  GetMediaType(long lLinkHandle);

	//设置属性
	void SetLinkType(long nLinkType, long nDataType);

	//添加链接
	void* AddLinkInfo(LINKINFO *pLinkinfo);

    //删除链接
    void DelLinkInfo(long linkhandle);

	//获取链接信息
	LINKINFO*   GetLinkMap();

	int GetLinkInfo(long hanlde, LINKINFO **link);

    //获取链接数
    long   GetLinkCount();

private:
	//AddLink Lock
	//CRITICAL_SECTION   m_lockAddLink;

	//链接类型,对应具体的DLL
	long  m_nLinkType;

	//网络链接类型(TCP/UDP/组播)
	//int  m_nNetLinkType;

	CDeviceCollect *m_pDeviceCollect;

	//链接列表
	//LINKMAP  m_mapLink; 
    LINKINFO *m_Links;
	
public:
	//对应具体的解码DLL
	long  m_nDataType;
};


struct DEVINFO
{
	long nDevLinkType; //链接类型
	long nNetLinkType; //网络链接类型 tcp/udp/rtp
};

/************************************************************************************
                                设备容器类
*************************************************************************************/
class LINKCOMMAPI CDeviceCollect : public map<long,CBaseDevice*>
{
public:
	CDeviceCollect(void);
	~CDeviceCollect(void);

	//加载设备列表
	bool   LoadDevices(char* szWorkPath);

	//注销设备
	bool   UnLoadDevices();

	//通过设备类型获取设备对象
	CBaseDevice* GetDevice(long nDeviceType);

	//获取加载的总设备类型数
	long   GetDeviceCount();

	//获取链接数
	long   GetLinkCount();

	//获取连接类型
	long GetNetlinkType(int devType);

	//判断是否已登录
	long   GetLoginHandle(char* szIP, long nDeviceType);

	//判断是否已登录
	long   GetLoginHandle(char* szIP, CBaseDevice *pDevice);

	//判断是否已链接
	long  GetLinkHandle(char* szIP, long nChannel, long nMediaType, CBaseDevice *pDevice);

	//获取链接信息
	int GetLinkInfo(long nLinkHandle, CBaseDevice *pDevice, LINKINFO **link);

	//是否还有指定登录句柄的设备处于链接中
	bool     IsDeviceLink(long lLoginHandle, CBaseDevice *pDevice);

	void     AddDevice(CBaseDevice *pDevice, long nDeviceLinkType);

	long SetPortRange(long startPort, long portNum);

	//申请端口
	bool  GetPorts(long portnum, long *pPorts,  bool bIsSort = false);

	//释放端口
	void FreePorts(long *pPorts, long portnum);

	//重置端口
	bool QueryPort(long nDevType, long nNetLinkType, long& nPort);

private:

	//设备类型与连接类型的映射
	map<long, DEVINFO> m_mapLinkType;

	//起始端口号
	long   m_nStartPort;
	long   m_nEndPort;

	//端口配置
	list<PORTINFO> m_PortInfo;

	//端口状态数组
	unsigned char* m_pPortStatus;
	
};

#ifdef __cplusplus
extern "C" {
#endif
//插件加载
CBaseDevice LINKCOMMAPI * AddA(CBaseDevice* pDevice,  CDeviceCollect *pDeviceCollect, long nDeviceLinkType, long nDataType);


#ifdef __cplusplus
};
#endif

#ifdef _WIN32
#ifndef  LINKCOMM_EXPORTS
#pragma comment(lib, "LinkComm2.0.lib")
#pragma message("Auto Link LinkComm2.0.lib")
#endif//#ifndef  LINKCOMM_EXPORTS
#endif//#ifdef _WIN32

#endif //_DATAFILTER_H_
