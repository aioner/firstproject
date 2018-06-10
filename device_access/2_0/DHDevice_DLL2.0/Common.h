// Common.h: interface for the CCommon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMON_H__7C21641F_7142_4EE3_9BE5_07DA963CCEBF__INCLUDED_)
#define AFX_COMMON_H__7C21641F_7142_4EE3_9BE5_07DA963CCEBF__INCLUDED_

#ifdef _WIN32
#include <Windows.h>
#include <WinSock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "toolsInsteadMFC.h"
#include "xt_config.h"
#endif//#ifdef _WIN32
#include "dhnetsdk.h"
#include "DHDvrDevice.h"


//////////////////////////////////////////////////////////////////////
// 系统宏定义
#define MAX_LOGON_AMOUNT			4069	//最大登陆设备数量
#define MAX_CHANNEL_AMOUNT			128 	//最大数据通道数量
#define MAX_USERNAME_LENGTH 		32		//最大用户名长度
#define MAX_PASSWORD_LENGTH 		64		//最大密码长度
#define MAX_BACKFILE_INFO_LENGTH	4096	//最大回放文件信息缓冲

#define REONLINE_WAIT_TIME			10		//重上线等待时间
#define DATA_TIME_OUT				9		//无数据超时
#define REONLINE_TIMER				1		//码率统计计时器号
#define RATE_CHECK_TIMER			2		//码率统计计时器号


const BYTE DH_FileHead[8] = {0x44,0x41,0x48,0x55,0x41,0x11,0x04,0x19};
const int c_nBaudRate[10] = {300,600,1200,2400,4800,9600,19200,38400,57600,115200};
const int c_nDataBit[4] = {5, 6, 7, 8};

//typedef long (__stdcall *OV_PRealDataCallback)(long nLinkHandle, long nFrameType, unsigned char*	pDataBuf, long	nDataLength, long nDataType, void* objUser, long nTimeStam);

//////////////////////////////////////////////////////////////////////
/*语音数据传输回调原型*/
typedef void(__stdcall *fAudioDataCallBack)(char *pstrIP, char *pDataBuf, DWORD dwBufSize, BYTE byAudioFlag);
/*普通报警消息回调*/
typedef long(__stdcall *fCommAlarmCB)(char *pstrIP, long nDVRPort, long alarminputcount, BYTE *pAlarm);
//移动侦测报警消息回调
typedef long(__stdcall *fMotionDectionCB)(char *pstrIP, long nDVRPort, long nChannelCount, BYTE *pMotionDection);
//视频丢失报警消息回调
typedef long(__stdcall *fVideoLostCB)(char *pstrIP, long nDVRPort, long nChannelCount, BYTE *pVideoLost);
/*视频遮挡消息回调*/
typedef long(__stdcall *fShelterAlarmCB)(char *pstrIP, long nDVRPort, BYTE *ShelterAlarm, long nArraySize);
/*硬盘满消息回调*/
typedef long(__stdcall *fDiskFullCB)(char *pstrIP, long nDVRPort, BYTE IsFull);
/*硬盘故障消息回调*/
typedef long(__stdcall *fDiskErrorCB)(char *pstrIP, long nDVRPort, BYTE *DiskError, long nArraySize);
/*音频监测消息回调*/
typedef long(__stdcall *fSoundDetectCB)(char *pstrIP, long nDVRPort, BYTE *SoundDetect, long nArraySize);
/*设备断线回调*/
typedef void(__stdcall *fDisConnectCB)(char *pstIP, long nDVRPort);
//图像叠加回调
typedef void(__stdcall *fDrawWndCB)(char *pstrIP, long nChID, HDC hDC,DWORD dwUser);
//新回放进度信息
typedef void(__stdcall *DHPlayBackPos)(char *pstrIP, long nChID, long hWnd, double dbPos, DWORD dwUser);
//新下载进度信息
typedef void(__stdcall *DHDownloadPos)(char *pstrIP, long nChID, DWORD dwTotalSize, DWORD dwDownLoadSize, double dbPos, DWORD dwUser);
//磁盘信息回调
//nDiskNum磁盘数量, nDiskID 磁盘序号，nFullSpace 容量，nFreeSpace 剩余容量，nDiskState 状态0-休眠,1-活动,2-故障等
typedef void(__stdcall *fDHDiskInfo)(char *szIP, long nDiskNum, long nDiskID, long nFullSpace, long nFreeSpace, long nDiskState);

typedef long (__stdcall *pOV_RealDataCallback)(DWORD nDeviceType, DWORD nLinkID,
                                               BYTE* pDataBuf, DWORD nDataLength, DWORD nImageType, DWORD nWidth, DWORD nHeight,
                                               long nFrameType, void* UserContext);

/*获取用户权限列表*/
typedef void (__stdcall *fUserRightCB)(
                                       char *pstrIP,                    //DVR IP地址
                                       long nRightID,                   //权限编号
                                       char *pstrRightName,				//权限名称,32字节
                                       char *pstrRightMemo 				//权限备注,32字符
                                       );
/*获取用户组列表*/
typedef void (__stdcall *fUserGroupCB)(
                                       char *pstrIP,					//DVR IP地址
                                       long nGroupID,					//用户组编号
                                       char *pstrGroupName,				//用户组名称
                                       long nRightNum, 					//权限数量
                                       long *pnRightList,				//权限编号数组,最大100长度
                                       char *pstrGroupMemo 				//用户组备注
                                       );
/*获取用户列表*/
typedef void (__stdcall *fUserInfoCB)(
                                      char *pstrIP,						//DVR IP地址
                                      long nUserID,						//用户编号
                                      long nGroupID,					//用户组编号
                                      char *pstrUserName, 				//用户名
                                      char *pstrPassword, 				//密码(加密)
                                      long nRightNum, 					//权限数量
                                      long *pnRightList,				//权限编号数组,最大100长度
                                      char *pstrUserMemo				//用户备注
                                      );

//////////////////////////////////////////////////////////////////////
// 系统结构定义

//串口基本配置
struct SComCfg
{
    BYTE nDataBit;
    BYTE nStopBit;
    BYTE nParity;
    BYTE nBaudRate;
};

//登陆设备信息
struct SDeviceInfo_DH
{
    DWORD m_dwIP;							//	设备IP地址		(默认0	空地址)
    WORD m_nID; 							//	设备ID编号		(默认-1 空设备)
    WORD m_wPost;							//	设备Post端口号	(默认0	空端口)
    char m_strUserName[MAX_USERNAME_LENGTH];//	登陆用户名		(默认NULL)
    char m_strPassWord[MAX_PASSWORD_LENGTH];//	登陆密码		(默认NULL)
    long m_nHandle; 						//	设备句柄		(默认值0,空)
    long m_nDevVer;                         //  设备版本，用于选择调用API版本
	long lLoginHandle;						//	登录句柄 add pan 8/11
    
    //语音对讲相关
    long m_nAudioHandle;					//	语音对讲句柄	(默认值0,空)
    void* m_hARecLock[2];                  //  对讲录音锁      ()
    void* m_hSpeakRec[2];                  //  对讲录音句柄    (默认值0,空)
    
    //串口配置
    void* m_objSerial;
    long m_hSerialTran[2];                  //  透传句柄        (0-232, 1-485)
    long m_nSerial485;                      //  当前使用的通道号(默认值0)
    SComCfg m_cfg232[DH_MAX_232_NUM];
    SComCfg m_cfg485[DH_MAX_CHANNUM];
};

/*数据通道信息*/
struct SChannelInfo_DH
{
    long m_nID; 							//	设备编号		(默认值-1,空闲)
    DWORD m_dwIP;							//	设备IP地址		(默认值0,空地址)
    short m_nChannelType;					//	数据通道状态	(默认值0,无：1即时数据；2回放数据；3回放下载)
    short m_nMediaType;                     //  码流序号        (默认值0,主码流)
    bool m_aPlayState[2];					//	播放状态		(0数据捕获状态,1数据保存状态)
    long m_nHandle; 						//	播放句柄		(默认值0,空)
    HWND m_hWnd;							//	显示句柄		(默认值NULL)
    long m_nData;							//	用户数据		(默认值-1)
	pOV_RealDataCallback m_pOVCallback; 	//	OV标准数据输出	(默认值NULL)	
	OV_PRealDataCallback m_pNewCallBack;
    NET_RECORDFILE_INFO* m_pRPFile; 		//	回放文件信息	(默认值NULL)
    DWORD m_nDataRate;						//	码率统计
    long m_nOutTime;						//	超时统计
    char* m_szSavePath;
};

struct SReOnlineInfo
{
    char szIP[16];
    long nWaitTime; 	//倒计时(秒)
};

struct SReLinkInfo
{
    DWORD nIPv4;    //设备IP
    long nChID;     //图像通道
};

//时间格式转换
void BitSet(DWORD &nInput, long nBitCount, BYTE bFlag);
void FixComCfgStruct(SComCfg& rOutput, const DH_COMM_PROP& rInput);
void FixComCfgStruct(DH_COMM_PROP& rOutput, const SComCfg& rInput);

DH_RealPlayType FixMediaType(int nMediaType);

//结构初始化
void StructInit(SDeviceInfo_DH& Struct);
void StructInit(SChannelInfo_DH& Struct);
void StructInit(SReOnlineInfo& SInput);
void StructInit(SReLinkInfo& SInput);

void WriteLog(const char* szFlag, const char* szLogFmt, ...);
void EnableLog(bool bEnable);

#endif // !defined(AFX_COMMON_H__7C21641F_7142_4EE3_9BE5_07DA963CCEBF__INCLUDED_)
