// Device_DH.h: longerface for the
// SDeviceInfo struct
// SChannelInfo struct
// CDevice_DH class
// CDeviceList class
// CChannelList class
// 使用时需要加入静态库<ws2_32.lib>
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICE_DH_H__8CB9885D_F5AD_4813_8C61_6C6016578886__INCLUDED_)
#define AFX_DEVICE_DH_H__8CB9885D_F5AD_4813_8C61_6C6016578886__INCLUDED_

#include "InfoMrg.h"

//////////////////////////////////////////////////////////////////////
// 系统类定义
class CDevice_DH  
{
public:
    CDeviceList_DH m_DeviceList;			//连接设备列表
    CChannelList_DH m_ChannelList;			//数据通道列表
    CBackFileList_DH* m_BackFileList;		//回放录像文件信息列表
    bool m_bNoInit; 						//系统化状态(默认:true)
    fAudioDataCallBack m_pFunAudioCB;		//语音数据回调函数指针
    fCommAlarmCB m_pFunCommAlarmCB; 		//普通报警消息回调指针
    fMotionDectionCB m_pFuncMotionDectionCB;//移动侦测消息回调指针
    fVideoLostCB m_pFuncVideoLostCB;		//视频丢失消息回调指针
    fShelterAlarmCB m_pFunShelterAlarmCB;	//视频遮挡消息回调指针
    fDiskFullCB m_pFunDiskFullCB;			//硬盘满消息回调指针
    fDiskErrorCB m_pFunDiskErrorCB; 		//硬盘错误消息回调指针
    fSoundDetectCB m_pFunSoundDetectCB; 	//音频监测消息回调指针
    fDisConnectCB m_pFunDisConnectCB;		//断线回调指针
    fDrawWndCB m_pDrawWndCB;				//字符叠加回调指针
    
    DHPlayBackPos m_pPlayBackPos;
    DWORD m_nPBUser;
    DHDownloadPos m_pDownloadPos;
    DWORD m_nDLUser;
    
    static DHDEV_CHANNEL_CFG m_ChannelCfg[DH_MAX_CHANNUM];
    static DHDEV_ALARM_SCHEDULE m_AlarmCfg;
    static CDevice_DH* m_pThis;
    static char g_szFlag[];
    
    void* m_hRelinkEvent;
    vector<SReOnlineInfo> m_lstOnline;
    vector<SReLinkInfo> m_lstRelink;
    
protected:
    /****************************************************************/
    /*内部接口 及 回调函数											*/
    /****************************************************************/
    /*设备断开时回调函数，可以用来处理断开后设备列表的更新及设备的删除操作*/
    static void CALLBACK DisConnectFunc(long lLoginID, char *pchDVRIP, long nDVRPort, DWORD dwUser);
    /*设备重连时回调函数，可以用来处理断开后设备列表的更新及设备的删除操作*/
    static void CALLBACK ReOnlineFunc(long lLoginID, char *pchDVRIP, long nDVRPort, DWORD dwUser);
      /*消息回调处理函数,是对整个sdk应用的回调*/
    static BOOL CALLBACK MessCallBack(long lCommand, long lLoginID, char *pBuf, DWORD dwBufLen, char *pchDVRIP, LONG nDVRPort, DWORD dwUser);

    static bool CALLBACK MessCallBackV26(long lCommand, long lLoginID, char *pBuf, DWORD dwBufLen,
                                       char *pchDVRIP, long nDVRPort, DWORD dwUser);
    /*数据回调处理函数*/
    static void CALLBACK DataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize,LONG param, DWORD dwUser);
    /*对讲音频回调处理函数*/
    static void CALLBACK AudioCallBack(long nTalkHandle, char *pDataBuf, DWORD dwBufSize, BYTE byAudioFlag,
        DWORD dwUser);
    //字符叠加回调
    static void CALLBACK DrawWndCallBack(long lLoginID, long lRealHandle, HDC hDC, DWORD dwUser);
    //远程文件播放、下载进度POS回调
    static void CALLBACK FileDownloadPosCallBack(long lPlayBackRet, DWORD dwTotalSize, DWORD dwDownLoadSize, DWORD dwUser);
    
    /*停止指定设备所有数据链接*/
    void EDvr_StopDeviceDataLink(char* strIP);	
    //设备重登操作
    long EDvr_ReLogin(long nLoginID);
    //设备连接重连操作
    long EDvr_ReLink(long nLoginID);
    long EDvr_ReLink(DWORD nDevIPv4, long nChID);
    
public:
    CDevice_DH();
    virtual ~CDevice_DH();
    
    //////////////////////////////////////////////////////////////////////////
    // 初始化与清理退出
    //////////////////////////////////////////////////////////////////////////
    /*初始化客户端*/
    long EDvr_Init();//失败返回-1；成功返回0
    
    /*停止当前的所有操作，退出释放所有资源*/
    void EDvr_Exit();
    
    //设备断线回调设置
    void EDvr_SetDisConnectCallBackFunc(fDisConnectCB lpFuncCB);
    
    //////////////////////////////////////////////////////////////////////////
    // DVR设备登陆与注销
    //////////////////////////////////////////////////////////////////////////
    /*登录到指定的设备上*/
    long EDvr_Login(
        char*pstrIP,		//DVR IP地址
        long nPort, 		//DVR 端口号
        char*pstrLogName,	//DVR 登陆账号
        char *pstrLogPass,	//DVR 登陆密码
        bool bCheckUser = FALSE
        );//失败返回-1；成功返回设备编号，是以后操作的参数

    //开启告警侦听
    long EDvr_Listen(SDeviceInfo_DH &rDevice);
    
    long EDvr_ReLogin(
        char*pstrIP 	//DVR IP地址
        );
    
    /*从指定的设备上注销*/
    void EDvr_LogOut(
        char *pstrIP		//DVR IP地址
        );

	void EDvr_LogOut(
		long lLoginHandle	//dvr登录句柄
		);
    
    /*从所有已登录的设备上注销*/
    void EDvr_LogOutAll();
    
    /*远程重启设备*/
    void EDvr_Reboot(
        char *pstrIP		//DVR IP地址
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 设备通道及时数据链接
    //////////////////////////////////////////////////////////////////////////
    /*建立与指定设备的指定通道的数据链接*/
    long EDvr_StartDataLink(
        char*pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        long nLinkType, 	//连接方式 TCP:0；UDP:1
        long nMediaType,    //码流序号 0-录像流，1~
        HWND hWin			//播放窗体句柄，检测用
        );//失败返回-1；成功返回数据通道号，是以后操作的参数
    
    /*停止与指定设备的指定通道的数据链接*/
    void EDvr_StopDataLink(
        long nRDataLink 	//EDvr_StartDataLink 返回值
        );
    void EDvr_StopDataLink(
        char *pstrIP,		//DVR IP地址
        long nChID			//DVR 采集终端通道
        );
    
    /*停止当前的所有数据链接*/
    void EDvr_StopAllDataLink();
    
    /*获取采集通道对应操作句柄*/
    long EDvr_GetChannelHandle(char *pstrIP, long nChID);
    
    //////////////////////////////////////////////////////////////////////////
    // 设备通道及时数据捕获
    //////////////////////////////////////////////////////////////////////////
    
    /*启动指定设备的指定通道的数据链接的数据捕获*/
    long EDvr_StartDataCapture(
        long nRDataLink 	//EDvr_StartDataLink 返回值
        );//失败返回-1；成功返回0
    long EDvr_StartDataCapture(
        char *pstrIP,		//DVR IP地址
        long nChID			//DVR 采集终端通道
        );//失败返回-1；成功返回0
    
    /*停止指定设备的指定通道的数据链接的数据捕获*/
    void EDvr_StopDataCapture(
        long nRDataLink 	//EDvr_StartDataLink 返回值
        );
    void EDvr_StopDataCapture(
        char *pstrIP,		//DVR IP地址
        long nChID
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 采集录像/截图控制
    //////////////////////////////////////////////////////////////////////////
    /*开始指定设备指定通道的数据连接的数据保存文件(录像)*/
    long EDvr_StartDataRecorder(
        char *pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        char *pstrFileName	//本地录像文件名
        );//失败返回-1；成功返回0
    long EDvr_StartDataRecorder(
        long nRDataLink,	//EDvr_StartDataLink 返回值
        char *pstrFileName	//本地录像文件名
        );//失败返回-1；成功返回0
    
    /*停止指定设备指定通道的数据连接的数据保存文件(录像)*/
    void EDvr_StopDataRecorder(
        char *pstrIP,		//DVR IP地址
        long nChID			//DVR 采集终端通道
        );
    void EDvr_StopDataRecorder(
        long nRDataLink 	//EDvr_StartDataLink 返回值
        );
    
    /*对指定设备指定通道的御览画面进行截图*/
    long EDvr_ConvertPicture(
        char *pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        char *pstrFileName	//本地录像文件名
        );//失败返回-1；成功返回0
    long EDvr_ConvertPicture(
        long nRDataLink,	//EDvr_StartDataLink 返回值
        char *pstrFileName	//本地录像文件名
        );//失败返回-1；成功返回0
    
    //////////////////////////////////////////////////////////////////////////
    // 云台控制相关
    //////////////////////////////////////////////////////////////////////////
    /*云台控制*/
    long EDvr_PTZControl(
        char *pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        long nPTZCommand,	//云台控制命令
        bool bStop, 		//开始/停止运动
        long nStep			//步进/速度 1-8
        );//失败返回-1；成功返回0
    
    /*云台控制扩展*/
    long EDvr_PTZControlEx(
        char *pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        long nPTZCommandEx, //云台控制命令
        bool bStop, 		//开始/停止运动
        unsigned char param1, unsigned char param2, unsigned char param3
        );//失败返回-1；成功返回0
    
    //////////////////////////////////////////////////////////////////////////
    // 设备录像控制
    //////////////////////////////////////////////////////////////////////////
    
    /*获取远程录像配置*/
    long EDvr_GetRecordBackState(
        char *pstrIP,		//DVR IP地址
        char *pRState,		//录像状态数组：0不录像，1手动录像，2自动录像
        long nMaxLen,		//状态数组长度最大值，不能小于通道数
        long *pnStateLen	//状态数组长度，每个通道一个字节
        );//失败返回-1；成功返回0
    /*设置远程录像配置*/
    long EDvr_SetRecordBackState(
        char *pstrIP,		//DVR IP地址
        char *pRState,		//录像状态数组：0不录像，1手动录像，2自动录像
        long nStateLen		//状态数组长度，每个通道一个字节
        );//失败返回-1；成功返回0
    
    /*关闭查询句柄*/
    void EDvr_FindClose(
        long lFileFindHandle
        );
    /*查找下一条记录*/
    long EDvr_FindNextFile(
        long lFileFindHandle
        );
    
    /*开始播放录像文件*/
    long EDvr_StartPlayBack(
        char *pstrIP,		//DVR IP地址
        long nFileHandle,	//查找文件操作序号
        HWND hWnd			//播放窗体句柄
        );
    long EDvr_StartPlayBack(
        char *pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        char *pstrFileName, //录像文件名
        HWND hWnd			//播放窗体句柄
        );
    
    /*停止播放录像文件*/
    void EDvr_StopPlayBack(
        char *pstrIP,		//DVR IP地址
        long nChID			//DVR 采集终端通道
        );
    void EDvr_StopPlayBack(
        long nBDataLink 	//文件播放操作序号
        );
    void EDvr_StopPlayBack(HWND hWnd);
    
    /*开始下载录像文件*/
    long EDvr_StartFileDownload(
        char *pstrIP,		//DVR IP地址
        long nFileHandle,	//查找文件操作序号
        char *sSavedFileName
        );
    long EDvr_StartFileDownload(
        char *pstrIP,		//DVR IP地址
        long nChID, 		//DVR 采集终端通道
        char *pstrFileName, //录像文件名
        char *sSavedFileName//保存文件名
        );
    
    /*停止下载录像文件*/
    void EDvr_StopFileDownload(
        long nFileHandle	//查找文件操作序号
        );
    //停止下载文件
    // long nChID 通道号
    // char *pstrFileName 被下载文件名
    void EDvr_StopFileDownload(long nChID, char *pstrFileName);
    
    /*获得下载录像的当前位置*/
    long EDvr_GetDownloadPos(
        long nDownHandle,	//文件下载句柄
        long *nTotalSize,	//现在下载进度
        long *nDownLoadSize //总下载进度
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 语音对讲控制
    //////////////////////////////////////////////////////////////////////////
    /*设置语音回调函数*/
    void EDvr_SetAudioDataCallBackFunc(fAudioDataCallBack CallBackFunc);
    
    /*开启语音*/
    long EDvr_OpenSound(char *pstrIP, long nChID);
    void EDvr_CloseSound();
    
    //开启语音数据捕获
    long EDvr_StartAudioDataCapture();
    
    //停止语音数据捕获
    void EDvr_StopAudioDataCapture();
    
    /*发送语音到设备*/
    long EDvr_AudioDataSend(char *pstrIP, char *pSendBuff, long nBuffSize);
    
    /*解码从设备得到的语音数据*/
    void EDvr_AudioDecode(char *pSendBuff, long nBuffSize);
    
    /*设置音量*/
    long EDvr_SetAudioVolume(char *pstrIP, unsigned short wVolume);
    
    //////////////////////////////////////////////////////////////////////////
    // 报警消息回调设置
    //////////////////////////////////////////////////////////////////////////
    
    /*设置普通报警消息回调*/
    void EDvr_SetCommAlarmCallBackFunc(fCommAlarmCB CallBackFunc);
    
    //设置移动侦测报警回调
    void EDvr_SetMotionDectionCallBackFunc(fMotionDectionCB CallBackFunc);
    
    //设置视频丢失报警回调
    void EDvr_SetVideoLostCallBackFunc(fVideoLostCB CallBackFunc);
    
    /*设置视频遮挡消息回调*/
    void EDvr_SetShelterAlarmCallBackFunc(fShelterAlarmCB CallBackFunc);
    
    /*设置硬盘满消息回调*/
    void EDvr_SetDiskFullCallBackFunc(fDiskFullCB CallBackFunc);
    
    /*设置硬盘错误消息回调*/
    void EDvr_SetDiskErrorCallBackFunc(fDiskErrorCB CallBackFunc);
    
    /*设置音频监测消息回调*/
    void EDvr_SetSoundDetectCallBackFunc(fSoundDetectCB CallBackFunc);
    
    
    //////////////////////////////////////////////////////////////////////////
    // 报警配置
    //////////////////////////////////////////////////////////////////////////
    
    /*报警常规设置*/
    long EDvr_SetNormalAlarmCFG(
        char *pstrIP,						//DVR IP地址
        long nAlarmKind,					//报警种类:0-报警输入,1-移动侦测,2-视频丢失,3-视频遮挡,4-磁盘报警
        long nAlarmChannel, 				//报警输入-报警点序号；移动侦测.视频丢失.视频遮挡-视频通道序号；磁盘报警--1-无磁盘,2-磁盘满,3-磁盘故障
        long nAlarmInfo,					//报警输入-0常开,1常闭；移动侦测.视频丢失.视频遮挡-灵敏度(1~6)；磁盘报警--无意义,使用时填0
        long nAlarmEnable					//布防状态:0-撤防,1-布防
        );									//返回值:成功0，失败-1
    
    /*获取外部报警布防状态*/
    long EDvr_GetMsgAlarmInEnable(
        char *pstrIP,						//DVR IP地址
        bool *pIsEnable,					//布防状态数组，true布防，false撤防
        long *pAlarmInCount 				//数组长度
        );
    /*设置外部报警布防状态*/
    long EDvr_SetMsgAlarmInEnable(
        char *pstrIP,						//DVR IP地址
        const bool *pIsEnable,				//布防状态数组，true布防，false撤防
        long nAlarmInCount = DH_MAX_ALARM_IN_NUM
        );
    
    /*获取报警输出状态*/
    long EDvr_GetMsgAlarmOutEnable(
        char *pstrIP,						//DVR IP地址
        bool *pIsEnable,					//布防状态数组，true布防，false撤防
        long *pAlarmOutCount				//数组长度
        );
    /*设置报警输出状态*/
    long EDvr_SetMsgAlarmOutEnable(
        char *pstrIP,						//DVR IP地址
        const bool *pIsEnable,				//布防状态数组，true布防，false撤防
        long nAlarmOutCount = DH_MAX_ALARMOUT_NUM
        );
    
    /*获取动态监测报警布防状态*/
    long EDvr_GetMsgMotionDectionEnable(
        char *pstrIP,						//DVR IP地址
        bool *pIsEnable,					//布防状态数组，true布防，false撤防
        long *pVideoCount
        );
    /*设置动态监测报警布防状态*/
    long EDvr_SetMsgMotionDectionEnable(
        char *pstrIP,						//DVR IP地址
        const bool *pIsEnable,				//布防状态数组，true布防，false撤防
        long nVideoCount = DH_MAX_VIDEO_IN_NUM
        );
    
    /*获取视频丢失布防状态*/
    long EDvr_GetMsgVedioLostEnable(
        char *pstrIP,						//DVR IP地址
        bool *pIsEnable,					//布防状态数组，true布防，false撤防
        long *pVideoCount
        );
    /*设置视频丢失布防状态*/
    long EDvr_SetMsgVedioLostEnable(
        char *pstrIP,						//DVR IP地址
        const bool *pIsEnable,				//布防状态数组，true布防，false撤防
        long nVideoCount = DH_MAX_VIDEO_IN_NUM
        );
    
    /*获取图像遮挡布防状态*/
    long EDvr_GetMsgBlindEnable(
        char *pstrIP,						//DVR IP地址
        bool *pIsEnable,				//布防状态数组，true布防，false撤防
        long *pVideoCount
        );
    /*设置图像遮挡布防状态*/
    long EDvr_SetMsgBlindEnable(
        char *pstrIP,						//DVR IP地址
        const bool *pIsEnable,				//布防状态数组，true布防，false撤防
        long nVideoCount = DH_MAX_VIDEO_IN_NUM
        );
    
    /*获取硬盘消息布防状态*/
    long EDvr_GetMsgHardDiskEnable(
        char *pstrIP,						//DVR IP地址
        long nConfigType,					//设置类型，0无硬盘报警；1硬盘容量不足报警；2硬盘故障报警
        bool *pIsEnable 					//布防状态，true布防，false撤防
        );
    /*设置硬盘消息布防状态*/
    long EDvr_SetMsgHardDiskEnable(
        char *pstrIP,						//DVR IP地址
        long nConfigType,					//设置类型，0无硬盘报警；1硬盘容量不足报警；2硬盘故障报警
        bool bIsEnable						//布防状态，true布防，false撤防
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 拼接御览连接
    
    long EDvr_StartMultiLink(
        char*pstrIP,						//DVR IP地址
        long nLinkType, 					//连接方式 TCP:0；UDP:1
        HWND hWin							//播放窗体句柄，检测用
        );
    void EDvr_StopMultiLink(char *pstrIP);
    void EDvr_StopMultiLink(long nRDataLink);
    
    //////////////////////////////////////////////////////////////////////////
    // 视频通道设置
    
    /*获取通道画质*/
    long EDvr_GetVideoEffect(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        BYTE *nBright,						//亮度(0-100)
        BYTE *nContrast,					//对比度(0-100)
        BYTE *nSaturation,					//饱和度(0-100)
        BYTE *nHue, 						//色度(0-100)
        bool *bGainEnable,					//增益开关
        BYTE *nGain 						//增益(0-100)
        );
    /*设置通道画质*/
    long EDvr_SetVideoEffect(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        BYTE nBright,						//亮度(0-100)
        BYTE nContrast, 					//对比度(0-100)
        BYTE nSaturation,					//饱和度(0-100)
        BYTE nHue,							//色度(0-100)
        bool bGainEnable,					//增益开关
        BYTE nGain							//增益(0-100)
        );
    /*视频画面设置*/
    long EDvr_SetImageEffect(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEffecType,					//画面选项:0-亮度,1-对比度,2-饱和度,3-色度,4-增益
        long nEffectValue					//选项调解值(0~100)
        );
    
    /*获取通道视频配置*/
    long EDvr_GetChannelVideoInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
                                            long nEncOpt,						/*通道选项(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            4-Assistant_1;5-Assistant_2;6-Assistant_3)*/
                                            char *pstrChannelName,				//通道名
                                            bool *bVideoEnable, 				//图像使能
                                            BYTE *nBitRate, 					//码流(码率)控制(1-BRC_CBR;2-BRC_VBR)
                                            BYTE *nFPS, 						//帧率(1-30??)
                                            BYTE *nEncodeMode,					//编码模式:参见枚举 eEncodeMode
                                            BYTE *nImageSize,					//分辨率:参见枚举	eImageSize
                                            BYTE *nImageQlty					//画质(1-6)
                                            );
    /*设置通道视频配置*/
    long EDvr_SetChannelVideoInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEncOpt,						//通道选项(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
        //4-Assistant_1;5-Assistant_2;6-Assistant_3)
        char *pstrChannelName,				//通道名
        bool bVideoEnable,					//图像使能
        BYTE nBitRate,						//码流(码率)控制(1-BRC_CBR;2-BRC_VBR)
        BYTE nFPS,							//帧率(1-30??)
        BYTE nEncodeMode,					//编码模式:参见枚举 eEncodeMode
        BYTE nImageSize,					//分辨率:参见枚举	eImageSize
        BYTE nImageQlty 					//画质(1-6)
        ); 
    
    /*视频分辨率设置*/
    long EDvr_SetImageZoom(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEncKind,						//通道选项(1-主码流-常规;2-主码流-移动侦测;3-主码流-报警联动;
                                            //4-从码流1;5-从码流2;6-从码流3)
        long nImageZoom,					//分辨率(0-D1,1-HD1,2-BCIF,3-CIF,4-QCIF,5-VGA,6-QVGA,7-SVCD)
        long nZoonWidth,					//宽度(保留)
        long nZoomHeigh,					//高度(保留)
        long nFPS							//帧率(0-1,1-2,2-4,3-6,4-12,5-25)
        );
    /*画面清晰度设置*/
    long EDvr_SetImageDefine(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEncKind,						//通道选项(1-主码流-常规;2-主码流-移动侦测;3-主码流-报警联动;
        //4-从码流1;5-从码流2;6-从码流3)
        long nBitRateType,					//码流控制(0-定码率,1-变码率,2-限码率(保留))
        long nBitRate,						//码率限制(保留)
        long nImageQlty,					//画质(1-6)
        long nEncodeMode					//编码模式(0-DIVX_MPEG4;1-MS_MPEG4;2-MPEG2;3-MPEG1;4-H263;
        //5-MJPG;6-FCC_MPEG4;7-H264)
        );
    /*设置通道名称*/
    long EDvr_SetChannelName(
        char *pstrIP,						//DVR IP地址
        long nChType,						//通道类型(0-视频通道,1-报警输入通道(保留),2-报警输出通道(保留))
        long nChID, 						//通道序号
        char *pstrName						//输入的名字
        );
    
    /*获取通道音频配置*/
    long EDvr_GetChannelAudioInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEncOpt,						///通道选项(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            //4-Assistant_1;5-Assistant_2;6-Assistant_3)
        bool *bAudioEnable, 				//音频使能
        BYTE *nFormatTag,					//编码(保留,填0)
        WORD *nTrackCount,					//声道数(保留,填0)
        WORD *nBitsPerSample,				//采样深度(保留,填0)
        DWORD *nSamplesPerSec				//采样率(保留,填0)
        );
    
    /*获取通道OSD配置*/
    long EDvr_GetChannelOsdInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long OsdOpt,						//Osd类型:1-时间显示；2-通道名现实；3-遮挡显示
        DWORD *nFrontColor, 				//表色:4BYTE 分别为R、G、B、透明度
        DWORD *nBackColor,					//底色:4BYTE 分别为R、G、B、透明度
        RECT *rcRecr,						//位置:4LONG 分别为上、下、左、右
        bool *bOsdShow						//Osd使能
        );
    /*设置通道OSD配置*/
    long EDvr_SetChannelOsdInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long OsdOpt,						//Osd类型:1-时间显示；2-通道名现实；3-遮挡显示
        DWORD nFrontColor,					//表色:4BYTE 分别为R、G、B、透明度
        DWORD nBackColor,					//底色:4BYTE 分别为R、G、B、透明度
        const RECT *rcRecr, 				//位置:4LONG 分别为上、下、左、右
        bool bOsdShow						//Osd使能
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 串口配置
    
    /*获取485串口协议列表*/
    long EDvr_Get485PorList(
        char *pstrIP,						//DVR IP地址
        DWORD *n485PorCount,				//485协议数量
        char *str485PorList 			//协议名称列表
        );
    
    /*获取232串口功能列表*/
    long EDvr_Get232FuncList(
        char *pstrIP,						//DVR IP地址
        DWORD *n232FuncCount,				//232功能数量
        char *str232FuncList				//功能名称列表
        );
    
    /*获取458串口配置*/
    long EDvr_Get485Info(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long *n485Por,						//485协议序号，对应协议列表序号
        DWORD *nCOMInfo,					//通用串口配置：4BYTE 分别为
                                            //数据位：  0-5；1-6；2-7；3-8
                                            //停止位：  0-1位, 1-1.5位, 2-2位
                                            //校验：	0-no, 1-odd, 2-even
                                            //波特率：  {0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}*/
        BYTE *n485Address					//485口地址(0-255)
        );
    /*设置458串口配置*/
    long EDvr_Set485Info(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long n485Por,						//485协议序号，对应协议列表序号
        DWORD nCOMInfo, 					//通用串口配置：4BYTE 分别为
                                            //数据位：  0-5；1-6；2-7；3-8
                                            //停止位：  0-1位, 1-1.5位, 2-2位
                                            //校验：    0-no, 1-odd, 2-even
                                            //波特率：  {0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}
        BYTE n485Address					//485口地址(0-255)
    );
    
    /*获取232串口配置*/
    long EDvr_Get232Info(
        char *pstrIP,						//DVR IP地址
        long n232Index, 					//232串口序号
        long *n232Func, 					//232功能序号，对应功能列表序号
        DWORD *nCOMInfo 					//通用串口配置：4BYTE 分别为
                                            //数据位：0-5；1-6；2-7；3-8
                                            //停止位：0-1位, 1-1.5位, 2-2位
                                            //校验：	0-no, 1-odd, 2-even
                                            //波特率：{0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}
        );
    /*设置232串口配置*/
    long EDvr_Set232Info(
        char *pstrIP,						//DVR IP地址
        long n232Index, 					//232串口序号
        long n232Func,						//232功能序号，对应功能列表序号
        DWORD nCOMInfo						//通用串口配置：4BYTE 分别为
                                            //数据位：0-5；1-6；2-7；3-8
                                            //停止位：0-1位, 1-1.5位, 2-2位
                                            //校验：    0-no, 1-odd, 2-even
                                            //波特率：{0-300,1-600,2-1200,3-2400,4-4800,5-9600,6-19200,7-38400,8-57600,9-115200}
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 网络设置
    
    /*获取网络端口配置*/
    long EDvr_GetNetPortInfo(
        char *pstrIP,						//DVR IP地址
        char *pstrDeviceName,				//设备名称
        WORD *nTCPCount,					//TCP连接数量
        WORD *nTCPPort, 					//TCP端口号
        WORD *nUDPPort, 					//UDP端口号
        WORD *nHTTPPort,					//HTTP端口号
        WORD *HTTPSPort,					//HTTPS端口号
        WORD *nSSLPort						//SSL端口号
        );
    /*设置网络端口配置*/
    long EDvr_SetNetPortInfo(
        char *pstrIP,						//DVR IP地址
        const char *pstrDeviceName, 		//设备名称
        WORD nTCPCount, 					//TCP连接数量
        WORD nTCPPort,						//TCP端口号
        WORD nUDPPort,						//UDP端口号
        WORD nHTTPPort, 					//HTTP端口号
        WORD HTTPSPort, 					//HTTPS端口号
        WORD nSSLPort						//SSL端口号
        );
    
    /*获取以太网口配置*/
    long EDvr_GetEthernetInfo(
        char *pstrIP,						//DVR IP地址
        long nEthernetIndex,				//以太网卡序号
        char *pstrEthernetIP,				//网卡IP
        char *EthernetMask, 				//网卡子网掩码
        char *GatewayIP,					//网卡网关
        long *nNetMode, 					//接口类型：1-10MBase-T;2-10MBase-T全双工;3-100MBase-TX;4-100M全双工;5-10M/100M自适应
        char *pstrMAC						//网卡MAC编号
        );
    /*设置以太网口配置*/
    long EDvr_SetEthernetInfo(
        char *pstrIP,						//DVR IP地址
        long nEthernetIndex,				//以太网卡序号
        const char *pstrEthernetIP, 		//网卡IP
        const char *EthernetMask,			//网卡子网掩码
        const char *GatewayIP,				//网卡网关
        long nNetMode						//接口类型：1-10MBase-T;2-10MBase-T全双工;3-100MBase-TX;4-100M全双工;5-10M/100M自适应
        //const char *pstrMAC				//网卡MAC编号
        );
    
    /*获取远程服务器配置*/
    long EDvr_GetRemoteHostInfo(
        char *pstrIP,						//DVR IP地址
        long nHostType, 					//服务器种类: 1-报警服务器;2-日志服务器;3- SMTP服务器;4-多播组;5-NFS服务器;
                                            //6-远程Ftp服务器;7-PPPoE服务器;8-DDNS服务器;9-DNS服务器
        char *pstrHostIP,					//服务器IP
        WORD *nHostPort,					//服务器端口
        bool *bIsEnable,					//连接是否开启
        char *pstrUserName, 				//用户名
        char *pstrPassWord, 				//密码
        char *pstrHostName					//服务器信息：PPPoE=>PPPoE注册返回的IP，DDNS=>DDNS主机名，其他无效
        );
    /*设置远程服务器配置*/
    long EDvr_SetRemoteHostInfo(
        char *pstrIP,						//DVR IP地址
        long nHostType, 					//服务器种类: 1-报警服务器;2-日志服务器;3- SMTP服务器;4-多播组;5-NFS服务器;
                                            //6-远程Ftp服务器;7-PPPoE服务器;8-DDNS服务器;9-DNS服务器
        const char *pstrHostIP, 			//服务器IP
        WORD nHostPort, 					//服务器端口
        bool bIsEnable, 					//连接是否开启
        const char *pstrUserName,			//用户名
        const char *pstrPassWord,			//密码
        const char *pstrHostName			//服务器信息：PPPoE=>PPPoE注册返回的IP，DDNS=>DDNS主机名，其他无效
        );
    
    /*获取邮件服务器配置*/
    long EDvr_GetMailHostInfo(
        char *pstrIP,						//DVR IP地址
        char *pstrHostIP,					//服务器IP
        WORD *nHostPort,					//服务器端口
        char *pstrUserName, 				//用户名
        char *pstrPassWord, 				//密码
        char *pstrDestAddr, 				//邮件地址
        char *pstrCcAddr,					//操送地址
        char *pstrBccAddr,					//暗抄地址
        char *pstrSubject					//邮件标题
        );
    /*设置邮件服务器配置*/
    long EDvr_SetMailHostInfo(
        char *pstrIP,						//DVR IP地址
        char *pstrHostIP,					//服务器IP
        WORD nHostPort, 					//服务器端口
        const char *pstrUserName,			//用户名
        const char *pstrPassWord,			//密码
        const char *pstrDestAddr,			//邮件地址
        const char *pstrCcAddr, 			//操送地址
        const char *pstrBccAddr,			//暗抄地址
        const char *pstrSubject 			//邮件标题
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 配置系统参数
    
    /*获取设备属性配置*/
    long EDvr_GetDeviceAttribute(
        char *pstrIP,						//DVR IP地址
        BYTE *pDeviceAttributes,			//视频口,音频口,报警输入,报警输出,网络口,USB口,IDE口,串口,并口
        long *pnAttributeSize				//属性数组长度
        );
    
    /*捕获I帧*/
    long EDvr_CaptureIFrame(
        char *pstrIP,						//DVR IP地址
        long nChID							//视频通道号
        );
    
    //////////////////////////////////////////////////////////////////////////
    // 用户信息管理
    
    /*获取用户权限信息列表*/
    long EDvr_GetUserRightList(char *pstrIP, fUserRightCB lpFunCB);
    
    /*用户组信息查找*/
    long EDvr_GetUserGroupList(char *pstrIP, fUserGroupCB lpFunCB);
    long EDvr_GetUserGroup(
        char *pstrIP,						//DVR IP地址
        char *pstrGroupName,				//用户组名称(输入)
        long *nGroupID, 					//用户组编号
        long *nRightNum,					//权限数量
        long *pnRightList,					//权限编号数组,最大100长度
        char *pstrGroupMemo 				//用户组备注
        );
    
    /*用户信息查找*/
    long EDvr_GetUserInfoList(char *pstrIP, fUserInfoCB lpFunCB);
    long EDvr_GetUserInfo(
        char *pstrIP,						//DVR IP地址
        char *pstrUserName, 				//用户名(输入)
        long *nUserID,						//用户编号
        long *nGroupID, 					//用户组编号
        char *pstrPassword, 				//密码(加密)
        long *nRightNum,					//权限数量
        long *pnRightList,					//权限编号数组,最大100长度
        char *pstrUserMemo					//用户备注
        );
    
    /*增加用户组*/
    long EDvr_AddUserGroup(
        char *pstrIP,						//DVR IP地址
        long nGroupID,						//用户组编号
        char *pstrGroupName,				//用户组名称
        long nRightNum, 					//权限数量
        long *pnRightList,					//权限编号数组,最大100长度
        char *pstrGroupMemo 				//用户组备注
        );
    
    /*增加用户*/
    long EDvr_AddUser(
        char *pstrIP,						//DVR IP地址
        long nUserID,						//用户编号
        char *pstrUserName, 				//用户名
        long nGroupID,						//用户组编号
        char *pstrPassword, 				//密码
        long nRightNum, 					//权限数量
        long *pnRightList,					//权限编号数组,最大100长度
        char *pstrUserMemo					//用户备注
        );
    
    
    /*删除用户组*/
    long EDvr_DeletUserGroup(char *pstrIP, char *pstrGroupName);
    /*删除用户*/
    long EDvr_DeleteUser(
        char *pstrIP,
        char *pstrUserName,
        char *pstrPassword					//(暂时保留)
        );
    
    /*修改用户组*/
    long EDvr_EditUserGroup(
        char *pstrIP,						//DVR IP地址
        char *pstrGroupName,				//用户组名(输入)
        long nGroupID,						//用户组编号
        char *pstrNewName,					//新用户组名
        long nRightNum, 					//权限数量
        long *pnRightList,					//权限编号数组,最大100长度
        char *pstrGroupMemo 				//用户组备注
        );
    /*修改用户*/
    long EDvr_EditUser(
        char *pstrIP,						//DVR IP地址
        char *pstrUserName, 				//用户名(输入)
        long nUserID,						//用户编号
        char *pstrNewName,					//新用户名
        long nGroupID,						//用户组编号
        char *pstrPassword, 				//(暂时保留)
        long nRightNum, 					//权限数量
        long *pnRightList,					//权限编号数组,最大100长度
        char *pstrUserMemo					//用户备注
        );
    
    /*修改用户密码*/
    long EDvr_ModifyUserPassword(
        char *pstrIP,						//DVR IP地址
        char *pstrUserName, 				//用户名(输入)
        char *pstrOldPassword,				//旧密码
        char *pstrNewPassword				//新密码
        );
    
    //添加超级用户 20081206
    long New_AddPowerUser(char* szIP, char* szUser, char* szPwd = NULL);
    
    //////////////////////////////////////////////////////////////////////////
    // 报警接口详细分类封装
    
    //报警常规配置
    // pstrIP：DVR IP地址
    // nChID：对应设置通道序号，没有的填0
    // nAlarmType：报警类型 0信号报警 1移动侦测 2视频丢失 3视频遮挡 4无磁盘报警 5磁盘满报警 6磁盘故障报警
    // nAlarmEnable：报警布防状态
    // nValue1：配置参数1>>信号报警:0常开 1常闭；移动侦测,视频遮挡:灵敏度1-6；磁盘报警,视频丢失为保留参数
    // nValue2：配置参数2>>移动侦测:侦测区域 BYTE[12][16]；磁盘报警,信号报警,视频遮挡,视频丢失:保留参数
    // nValue3：配置参数3>>移动侦测:侦测数组长度12*16；磁盘报警,信号报警,视频遮挡,视频丢失:保留参数
    long EDvr_GetAlarmNormalInfo(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nAlarmEnable, BYTE *nValue1, BYTE *nValue2,long *nValue3);
        
    //报警联动输出配置
    // pstrIP：DVR IP地址
    // nChID：对应设置通道序号，没有的填0
    // nAlarmType：报警类型 0信号报警 1移动侦测 2视频丢失 3视频遮挡 4无磁盘报警 5磁盘满报警 6磁盘故障报警
    // nOutEnable：是否报警联动输出
    // pAlarmOut：报警输出通道号组，BYTE[16]
    // nAlarmOutSize：报警输出数组长度 16
    long EDvr_GetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nOutEnable, BYTE *pAlarmOut, long *nAlarmOutSize);
    long EDvr_SetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nOutEnable, BYTE *pAlarmOut, long nAlarmOutSize);
    
    //报警联动录像配置
    // pstrIP：DVR IP地址
    // nChID：对应设置通道序号，没有的填0
    // nAlarmType：报警类型 0信号报警 1移动侦测 2视频丢失 3视频遮挡 4无磁盘报警 5磁盘满报警 6磁盘故障报警
    // nRecordEnable：是否报警联动录像
    // nPreRecLen：预录时间(秒)
    // pAlarmRecord：录像通道号组，BYTE[16]
    // nAlarmRecordSize：录像通道数组长度 16
    long EDvr_GetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nRecordEnable, long *nPreRecLen, BYTE *pAlarmRecord, long *nAlarmRecordSize);
    long EDvr_SetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nRecordEnable, long nPreRecLen, BYTE *pAlarmRecord, long nAlarmRecordSize);
    
    //报警联动云台
    // pstrIP：DVR IP地址
    // nChID：对应设置通道序号，没有的填0
    // nAlarmType：报警类型 0信号报警 1移动侦测 2视频丢失 3视频遮挡 4无磁盘报警 5磁盘满报警 6磁盘故障报警
    // nPTZEnable：是否报警联动云台
    // nPTZType：云台动作数组 BYTE[16]，0不联动 1转到预置点 2巡航 3轨迹 4线扫
    // nPTZNo：云台动作参数组 BYTE[16]，预置点序号/巡航组号/轨迹序号/线扫序号
    // nPTZSize：云台数组长度 16
    long EDvr_GetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long *nPTZSize);
    long EDvr_SetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long nPTZSize);
    
    //报警联动其他配置开关
    long EDvr_GetAlarmOtherEn(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *pOtherEnable, long *nOtherSize);
    
    //设置字符叠加回调
    void EDvr_SetDrawWndCallBackFunc(fDrawWndCB CallBackFunc);
    //开启字符叠加
    void EDvr_OpenDrawWnd();
    
    //////////////////////////////////////////////////////////////////////
    // 远程文件操作
    //暂停回放
    long EDvr_PausePlayBack(HWND hWnd, long bPause);
    //单帧播放
    long EDvr_OneFramePlayBack(HWND hWnd, long bOneFrame);
    //设置播放位置
    long EDvr_SetPosPlayBack(HWND hWnd, long nPlayTime);
    //快放
    long EDvr_FastPlayBack(HWND hWnd);
    //慢放
    long EDvr_SlowPlayBack(HWND hWnd);
    //抓图
    long EDvr_ConvertPicture(HWND hWnd, char *pstrFileName);
    
    //开启回放文件
    long New_OpenPlayBack(char *pstrIP, long nFileHandle, HWND hWnd);
    //停止回放文件
    void New_StopPlayBack( HWND hWnd );
    //关闭回放文件
    void New_ClosePlayBack( HWND hWnd );
    //设置回放进度
    long New_SetPosPlayBack(HWND hWnd, double dbPos);
    //播放进度回调
    void New_RegPlayBackPos(DHPlayBackPos CallBackFunc, DWORD dwUser);
    //下载进度回调
    void New_RegDownloadPos(DHDownloadPos CallBackFunc, DWORD dwUser);
    
    //设置播放图像的色彩
    long New_GetColor(HWND hWnd, long* nBright, long* nContrast, long* nSaturation, long* nHue);
    long New_SetColor(HWND hWnd, long nBright, long nContrast, long nSaturation, long nHue);
    
    /*获取通道视频配置*/
    long New_GetChannelVideoInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEncOpt,						//通道选项(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            //         4-Assistant_1;5-Assistant_2;6-Assistant_3)
        char *pstrChannelName,				//通道名
        bool *bVideoEnable, 				//图像使能
        BYTE *nBitRateType, 				//码流(码率)控制(1-BRC_CBR;2-BRC_VBR)
        long *nBitRate,
        BYTE *nFPS, 						//帧率(1-30??)
        BYTE *nEncodeMode,					//编码模式:参见枚举 eEncodeMode
        BYTE *nImageSize,					//分辨率:参见枚举	eImageSize
        BYTE *nImageQlty					//画质(1-6)
        );
    /*设置通道视频配置*/
    long New_SetChannelVideoInfo(
        char *pstrIP,						//DVR IP地址
        long nChID, 						//视频通道号
        long nEncOpt,						//通道选项(1-Rec_Nomarl;2-Rec_Motion;3-RecAlarm;
                                            //         4-Assistant_1;5-Assistant_2;6-Assistant_3)
        char *pstrChannelName,				//通道名
        bool bVideoEnable,					//图像使能
        BYTE nBitRateType,					//码流(码率)控制(1-BRC_CBR;2-BRC_VBR)
        long nBitRate,
        BYTE nFPS,							//帧率(1-30??)
        BYTE nEncodeMode,					//编码模式:参见枚举 eEncodeMode
        BYTE nImageSize,					//分辨率:参见枚举	eImageSize
        BYTE nImageQlty 					//画质(1-6)
        ); 
    
    long New_GetFPSList(DWORD* nCount, char* pstrNameList);
    
    static const WORD nVideoRateList[12];
    long New_GetVideoRateList(DWORD* nCount, char* pstrNameList);
    
    //////////////////////////////////////////////////////////////////////////
    // OV标准连接接口
    static long m_nDeviceType;
    static long m_nDataType;
    
    long New_InitDevice(long nDeviceType, long nDataType);
    void New_ExitDevice();
    long New_LoginDevice(char* szDeviceIP, long nPort,char* szUserID, char* szPassword , long nModelType);
    void New_LogoutDevice(char* szDeviceIP);
	 void New_LogoutDevice(long index);//add pan
    long New_StartLinkDevice(char* szDeviceIP, long nChannel, long nLinkType, long nNetPort, long nMediaType, void* UserContext = NULL);
    void New_StopLinkDevice(long nLinkID);
    long New_StartLinkCapture(long nLinkID, OV_PRealDataCallback fCallBack, void* objUser);
    void New_StopLinkCapture(long nLinkID);
    long New_GetHeadData(long nLinkID, BYTE* pHeadData);    
    //透明传输接口
    void in_SaveComCfg(int nLoginID);
    long in_SerialSend(int nLoginID, int nSerialPort, void* pDataBuf, long nDataSize);
    
    /************************************************************************/
    /*	试验用接口															*/
    /************************************************************************/
    long T_InsertDevice(
        char *pstrIP,		//DVR IP地址
        long nPort, 		//DVR 端口号
        char *pstrLogName,	//DVR 登陆账号
        char *pstrLogPass,	//DVR 登陆密码
        long nIndex 		//设备保存序号
        );
    void T_UserAlarm(char *pstrIP, long nDVRPort, long nChID);
};

#endif // !defined(AFX_DEVICE_DH_H__8CB9885D_F5AD_4813_8C61_6C6016578886__INCLUDED_)
