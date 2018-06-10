
#ifndef _MEDIADEVINTERFACE_H_
#define _MEDIADEVINTERFACE_H_

#ifdef _WIN32
#ifndef   DEVICELINK_EXPORTS
#define   MEDIADEVICEAPI	__declspec(dllimport)
#else
#define   MEDIADEVICEAPI	__declspec(dllexport)
#endif
#else
#include <string.h>
#define __stdcall
#define MEDIADEVICEAPI __attribute__((visibility("default")))
#endif

#include <stddef.h>

struct TRACKINFO
{
    int trackId;        //数据ID
    int trackType;  //数据类型。0：视频；1：音频；－1：其他
    char trackname[64];
};

struct _RCVINFO 
{
    int index;
    unsigned short port_rtp;
    unsigned short port_rtcp;
    bool demux;
    unsigned int demuxid;
};

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

/*
函数功能：媒体数据回调函数定义
参数说明：
nOperHandle：媒体设备操作句柄
pDataBuf：       数据缓冲区。返回的实际数据
nDataLength：数据缓冲区的长度
nFrameType： 数据帧类型
nDataType：    媒体数据类型
pUserData：    用户自定义的标识
nTimeStamp：所获媒体数据的时间戳
返回值：成功：0；失败：-1
*/
typedef long (__stdcall *POUTPUTREALDATA)(long nOperHandle, unsigned char* pDataBuf, long nDataLength, long nFrameType,long nDataType,void* pUserData, long nTimeStamp, unsigned long nSSRC);

typedef long (__stdcall *regist_call_back_t)(const char *ip, unsigned short port, const unsigned char *data, unsigned int length);

typedef void (__stdcall *rtcp_report_callback_t)(void *ctx, unsigned int ssrc, const RTCP_SR *sr, const RTCP_RR *rr);

/*
函数功能：数据监测回调函数定义
参数说明：
operHandle： 操作句柄
videoNum：监测时间段内接收到的视频包数量
audioNum：监测时间段内接收到的视频包数量
pContext：用户上下文
返回值：0
*/
typedef long (__stdcall *DataMonitorCB)(long operHandle, long videoNum, long audioNum, void* pContext);

#endif//POUTPUTREALDATA__

#ifdef __cplusplus
extern "C" {
#endif

    /*
    函数功能：初始化MediaDevice资源
    参数说明：
    返回值：成功：0；失败：其他值
    */
    long MEDIADEVICEAPI StartMediadevice();

    /*
    函数功能：释放MediaDevice资源
    参数说明：
    返回值：成功：0；失败：其他值
    */
    long MEDIADEVICEAPI EndMediadevice();

    /*
    函数功能：初始化设备
    参数说明：
    nDeviceType：设备类型。 如果nDeviceType<0，将初始化配置文件中所有类型的设备
    pParam：          其他需要的参数，没有填NULL
    返回值：成功：0；失败：其他值
    */
    long MEDIADEVICEAPI InitializeDevice(long  nDeviceType, void* pParam);

    /*
    函数功能：初始化设备
    参数说明：
    nDeviceType：设备类型。 如果nDeviceType<0，将初始化配置文件中所有类型的设备
    xtCfg：        初始化参数
    startPort:    开始的端口号
    portNum:   如果portNum<1，默认startPort=16000，portNum=1000;
    返回值：成功：0；失败：其他值
    */
    long MEDIADEVICEAPI InitializeDeviceEx(long  nDeviceType, xt_client_cfg_t xtCfg, long startPort, long portNum);

    /*
    函数功能：反初始化设备，释放资源
    参数说明：
    nDeviceType：设备类型。如果nDeviceType<0，将反初始化配置文件中所有类型的设备
    pParam：          其他需要的参数，没有填NULL
    返回值：成功：0；失败：其他值
    */
    long MEDIADEVICEAPI UnInitializeDevice(long  nDeviceType, void* pParam);


    //////   媒体点播接口  ////////////////////////////////////////////
    /*
    函数功能：点播媒体数据
    参数说明：
    szIP：    要点播设备的IP
    nPort： 要点播设备的端口
    nDeviceType：要点播设备的类型
    nChannel：       点播的通道号
    pUserData：      用户自定义的标识
    pOutputRealData：获取实时的媒体数据
    szUser：              设备登录用户名
    szPassword：     设备登录密码
    nNetLinkType： 连接网络的协议类型
    nMediaType：   码流类型
    sockethandle： 保留，填0
	szMulticastIp: 组播接收地址
	nMulticastPort: 组播接收端口
	szLocalIP: 接收bind本地地址
    返回值：媒体设备操作句柄。成功：大于等于0；失败：小于0
    */
    long MEDIADEVICEAPI StartDeviceCapture(char* szIP, long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,
		char* szUser, char* szPassword, long nNetLinkType, long nMediaType, long sockethandle,
		const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP="0.0.0.0",void *hmp=NULL);

    /*
    函数功能：点播媒体数据
    参数说明：
    szIP：    要点播设备的IP
    nPort： 要点播设备的端口
    nDeviceType：要点播设备的类型
    nChannel：       点播的通道号
    pUserData：      用户自定义的标识
    pOutputRealData：实时数据回调函数
    szUser：              设备登录用户名
    szPassword：     设备登录密码
    nNetLinkType： 连接网络的协议类型
    nMediaType：   码流类型
    sockethandle： 保留，填0
	szMulticastIp: 组播接收地址
	nMulticastPort: 组播接收端口
    返回值：媒体设备操作句柄。成功：大于等于0；失败：小于0
    */
    long MEDIADEVICEAPI StartDeviceCaptureEx(char* szIP, long nPort, long  nDeviceType, long nChannel, void* pUserData, POUTPUTREALDATA  pOutputRealData,char* szUser, char* szPassword, long nNetLinkType, long nMediaType, long sockethandle, const char *szMulticastIp, unsigned short nMulticastPort);

    /*
    函数功能：rtp点播
    参数说明：
    nDeviceType：设备类型
    szURL：             设备的IP
    pUserData：      用户自定义的标识
    pOutputRealData：获取实时的媒体数据
    nNetLinkType：连接网络的协议类型
    nMediaType：  码流类型
    返回值：媒体设备操作句柄。成功：大于等于0；失败：小于0
    */
    long MEDIADEVICEAPI StartDeviceCaptureRTP(long  nDeviceType, const char* szURL, void* pUserData, POUTPUTREALDATA  pOutputRealData,long nNetLinkType, long nMediaType);

    /*
    函数功能：停止点播
    参数说明：
    nOperHandle：媒体设备操作句柄
    返回值：成功：0；失败：非0
    */
    long MEDIADEVICEAPI StopDeviceCapture(long nOperHandle);

    /*
    函数功能：获取数据类型
    参数说明：
    nOperHandle：媒体设备操作句柄
    返回值：成功：>0；失败：<0
    */
    long MEDIADEVICEAPI GetDataType(long nOperHandle);

    /*
    函数功能：获取状态
    参数说明：
    nOperHandle：媒体设备操作句柄
    返回值：播放状态
    */
    long MEDIADEVICEAPI GetPlayRetStatus(long nOperHandle);

    /*
    函数功能：获取媒体数据信息
    参数说明：
    nOperHandle：媒体设备操作句柄
    pTrackInfo：[out] 返回媒体数据的信息
    返回值：>0/=0/-1/-2 ---- 成功/未获取数据头/不存在该链接/传入缓存为空
    */
    long MEDIADEVICEAPI GetTrack(long nOperHandle,  TRACKINFO * pTrackInfo);

    /*
    函数功能：获取数据头
    参数说明：
    nOperHandle：媒体设备操作句柄
    nHeadLen：[out] 数据头的长度。=-1
    返回值：数据头的内容
    */
    char MEDIADEVICEAPI *GetHeadData(long nOperHandle, long& nHeadLen);

	/*
    函数功能：设置数据的断线监测功能
    参数说明：
	nOperHandle：操作句柄
    monitorFunc : 断线信息回调函数。 若设置为NULL，则取消监测
    intervalTime :  断线监测时间间隔。 单位 秒
	pContext：用户上下文
    返回值：
    */
	long MEDIADEVICEAPI SetDataMonitor(long nOperHandle, DataMonitorCB monitorFunc, int intervalTime, void* pContext);

	/*
    函数功能：设置SSRC报告功能
    参数说明：
	nOperHandle：操作句柄
    reportFunc :    报告回调函数
	pContext：用户上下文
    返回值：
    */
	long MEDIADEVICEAPI SetSSRCReport(long nOperHandle, rtcp_report_callback_t reportFunc, void* pContext);


    ////////    文件流化控制接口    ///////////////////////////////////////////////////
    /*
    函数功能：控制文件播放、拖动、播放速率
    参数说明：
    nOperHandle：文件播放句柄
    npt：   客户端seek之后的npt时间 
    scale：客户端seek之后的播放倍率
    rtp_pkt_timestamp：seek之后的rtp包的时戳
    返回值：成功：大于等于0；失败：小于0
    */
    long  MEDIADEVICEAPI TcpPlayCtrl(const long nOperHandle,double npt,float scale, unsigned long *rtp_pkt_timestamp);

    /*
    函数功能：暂停文件播放
    参数说明：
    nOperHandle：文件播放句柄
    返回值：成功：大于等于0；失败：小于0
    */
    long  MEDIADEVICEAPI TcpPauseCtrl(const long nOperHandle);



    ///////    XMPP应用层协商传输接口    ///////////////////////////////////
    //启动链路
    long MEDIADEVICEAPI StartLinkDevice(const char *szIP, const char* szSdp, long nSdpLen,long nChannel,long nLinkType, long nMediaType, long nDeviceType,void* pUesrData,POUTPUTREALDATA  POutPutRealDataFunc);

    //开启采集
    long MEDIADEVICEAPI StartLinkCapture(long nOperHandle);

    //关闭采集
    long MEDIADEVICEAPI StopLinkCapture(long nOperHandle);

    //关闭链路
    long MEDIADEVICEAPI StopLinkDevice(long nOperHandle);

    //获取SDP
    long MEDIADEVICEAPI GetSDP(long nOperHandle,unsigned char *szSDP, long& nLength);

    long MEDIADEVICEAPI GetClientInfo(long lDeviceLinkHandle,long& RtpRecvPort,long& RtcpRecvPort,bool& MultiplexR, long& MultidR);
	
	//设置丢包重传参数
	long MEDIADEVICEAPI SetResend(long device_type, int resend,int wait_resend, int max_resend, int vga_order);

    //////////////////////////   交换特别使用   //////////////////////////////////
    long MEDIADEVICEAPI md_create_recv(int track_num, bool demux);

    long MEDIADEVICEAPI md_create_recvinfo(int dev_type, int track_num, bool demux, bool multicast, const char* multicastip, int* multiports);

    long MEDIADEVICEAPI md_get_rcvinfo(long link, _RCVINFO *infos, int &num);

    long MEDIADEVICEAPI md_set_sdp(long link, const char *sdp, unsigned int len_sdp);

    long MEDIADEVICEAPI md_start_link_captuer(const long link_handle,POUTPUTREALDATA data_out_cb,void* user_data);

    long MEDIADEVICEAPI md_set_regist_callback(long device_type,regist_call_back_t func);

    long MEDIADEVICEAPI md_request_iframe(long link);
    ///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
};
#endif

#endif //_MEDIADEVINTERFACE_H_