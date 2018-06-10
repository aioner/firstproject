// Device_DH.cpp: implementation of the
// CDevice_DH class
// SDeviceInfo struct
// SChannelInfo struct
//
//////////////////////////////////////////////////////////////////////
#include "Device_DH.h"

CDevice_DH* CDevice_DH::m_pThis = NULL;
DHDEV_ALARM_SCHEDULE CDevice_DH::m_AlarmCfg = {0};
DHDEV_CHANNEL_CFG CDevice_DH::m_ChannelCfg[DH_MAX_CHANNUM] = {{0}};

CDevice_DH::CDevice_DH()
{
    m_pFunAudioCB = NULL;
    m_pFunCommAlarmCB = NULL;
    m_pFuncMotionDectionCB = NULL;
    m_pFuncVideoLostCB = NULL;
    m_pFunShelterAlarmCB = NULL;
    m_pFunDiskFullCB = NULL;
    m_pFunDiskErrorCB = NULL;
    m_pFunSoundDetectCB = NULL;
    m_pFunDisConnectCB = NULL;
    m_pDrawWndCB = NULL;
    m_pPlayBackPos = NULL;
    m_pDownloadPos = NULL;
    m_nPBUser = 0;
    m_nDLUser = 0;
    m_BackFileList = NULL;
    
    m_bNoInit = true;
    m_pThis = this;
}

CDevice_DH::~CDevice_DH()
{
    EDvr_Exit();
}


//////////////////////////////////////////////////////////////////////
// 主系统类::静态函数
//////////////////////////////////////////////////////////////////////
void CALLBACK CDevice_DH::DisConnectFunc(LONG lLoginID, char *pchDVRIP, LONG nDVRPort, DWORD dwUser)
{
    WriteLog(g_szFlag, "收到设备下线 IP:%s", pchDVRIP);
    if(!dwUser) return;
    CDevice_DH *pThis = (CDevice_DH*)dwUser;
    //增加了自动上线
    //pThis->EDvr_LogOut(pchDVRIP);
    if(NULL != pThis->m_pFunDisConnectCB)
    {
        pThis->m_pFunDisConnectCB(pchDVRIP,0);
    }
}

void CALLBACK CDevice_DH::ReOnlineFunc(long lLoginID, char *pchDVRIP, long nDVRPort, DWORD dwUser)
{
    WriteLog(g_szFlag, "收到设备上线 IP:%s", pchDVRIP);
    if(!dwUser) return;
    CDevice_DH *pThis = (CDevice_DH*)dwUser;
    
    // 需要上线延迟10秒
    //    pThis->EDvr_ReLogin(lLoginID);
    //    if(NULL != pThis->m_pFunDisConnectCB)
    //    {
    //        pThis->m_pFunDisConnectCB(pchDVRIP,1);
    //    }
    unsigned i=0;
    SReOnlineInfo theInfo;
    strcpy(theInfo.szIP, pchDVRIP);
    theInfo.nWaitTime = REONLINE_WAIT_TIME;
    
    //插入设备上线信息
    for(i=0; i<pThis->m_lstOnline.size(); i++)
    {
        if(pThis->m_lstOnline[i].szIP[0] == 0)
            break;
    }
    if(i >= pThis->m_lstOnline.size())
        pThis->m_lstOnline.push_back(theInfo);
    else
        pThis->m_lstOnline[i] = theInfo;
}

bool CALLBACK CDevice_DH::MessCallBackV26(long lCommand, long lLoginID, char *pBuf, DWORD dwBufLen,
                                       char *pchDVRIP, long nDVRPort, DWORD dwUser)
{
    unsigned ret = true, i;
    char cNoAlarm[32]={0};
    char szAlarmState[32] = {0};
    CDevice_DH * device = (CDevice_DH *)dwUser;

    switch(lCommand)
    {
    case DH_ALARM_ALARM_EX:			//0x2101	//External alarm 
        {
            for(i=0; i<dwBufLen; i++)
                szAlarmState[i] = pBuf[i] + 48;
            WriteLog(g_szFlag, "AlarmIn IP:%s Port:%d Signal:%s", pchDVRIP, nDVRPort, szAlarmState);

            if(NULL == device->m_pFunCommAlarmCB)
                ret = FALSE;
            if(memcmp(pBuf,cNoAlarm,DH_MAX_ALARM_IN_NUM))
                ret = device->m_pFunCommAlarmCB(pchDVRIP,nDVRPort,dwBufLen,(BYTE*)pBuf);
        }
        break;
    case DH_MOTION_ALARM_EX:		//	0x2102	//Motion detection alarm 
        {
            for(i=0; i<dwBufLen; i++)
                szAlarmState[i] = pBuf[i] + 48;
            WriteLog(g_szFlag, "Motion IP:%s Port:%d Signal:%s", pchDVRIP, nDVRPort, szAlarmState);

            if(NULL == device->m_pFuncMotionDectionCB)
                ret = FALSE;
            if(memcmp(pBuf,cNoAlarm,DH_MAX_CHANNUM))
                ret = device->m_pFuncMotionDectionCB(pchDVRIP,nDVRPort,dwBufLen,(BYTE*)pBuf);
        }
        break;
    case DH_VIDEOLOST_ALARM_EX:		//0x2103	//Video loss alarm 
        {
            for(i=0; i<dwBufLen; i++)
                szAlarmState[i] = pBuf[i] + 48;
            WriteLog(g_szFlag, "VideoLost IP:%s Port:%d Signal:%s", pchDVRIP, nDVRPort, szAlarmState);

            if(NULL == device->m_pFuncVideoLostCB)
                ret = FALSE;
            if(memcmp(pBuf,cNoAlarm,DH_MAX_CHANNUM))
                ret = device->m_pFuncVideoLostCB(pchDVRIP,nDVRPort,dwBufLen,(BYTE*)pBuf);
        }
        break;
    case DH_SHELTER_ALARM_EX:		//	0x2104	//Camera masking alarm 
        {
            for(i=0; i<dwBufLen; i++)
                szAlarmState[i] = pBuf[i] + 48;
            WriteLog(g_szFlag, "VideoShelter IP:%s Port:%d Signal:%s", pchDVRIP, nDVRPort, szAlarmState);

            if(NULL == device->m_pFunShelterAlarmCB)
                return FALSE;
            if(memcmp(pBuf,cNoAlarm,DH_MAX_CHANNUM))
                ret = device->m_pFunShelterAlarmCB(pchDVRIP,nDVRPort,(BYTE*)pBuf,dwBufLen);
        }
        break;
    case DH_SOUND_DETECT_ALARM_EX:	//0x2105	//Audio detection alarm 
        {
            if(NULL == device->m_pFunSoundDetectCB)
                return FALSE;
            if(memcmp(pBuf,cNoAlarm,DH_MAX_CHANNUM))
                ret = device->m_pFunSoundDetectCB(pchDVRIP,nDVRPort,(BYTE*)pBuf, dwBufLen);
        }
        break;
    case DH_DISKFULL_ALARM_EX:		//0x2106	//HDD full alarm 
        {
            if(NULL == device->m_pFunDiskFullCB)
                return FALSE;
            ret = device->m_pFunDiskFullCB(pchDVRIP,nDVRPort,*pBuf);
        }
        break;
    case DH_DISKERROR_ALARM_EX:		//0x2107	//HDD malfunction alarm 
        {
            if(NULL == device->m_pFunDiskErrorCB)
                return FALSE;
            if(memcmp(pBuf,cNoAlarm,DH_MAX_CHANNUM))
                ret = device->m_pFunDiskErrorCB(pchDVRIP,nDVRPort,(BYTE*)pBuf, dwBufLen);
        }
        break;
    case DH_ENCODER_ALARM_EX:		//	0x210A	//Encoder alarm 
        break;
    case DH_URGENCY_ALARM_EX:		//	0x210B	//Emergency alarm 
        break;
    case DH_WIRELESS_ALARM_EX:		//0x210C	//Wireless alarm 
        break;
    case DH_ALARM_DECODER_ALARM_EX: //0x210E	//Alarm decoder alarm 
    default:
        break;
    }
    return FALSE;
}

void CALLBACK CDevice_DH::DataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer,
                                       DWORD dwBufSize, LONG param, DWORD dwUser)
{
    if (!m_pThis) return;
    CDevice_DH * device = (CDevice_DH *)m_pThis;
    SChannelInfo_DH* pChannel = device->m_ChannelList.GetChannel(dwUser);
    if(pChannel == NULL) return;
    if(pChannel->m_nHandle != lRealHandle) return;
    
    //码率统计
    pChannel->m_nDataRate += dwBufSize;
    pChannel->m_nOutTime = DATA_TIME_OUT;
	
    if(pChannel->m_pNewCallBack){
        int frameType=0;
        if (param != 0)
        {
            tagVideoFrameParam *vedioInfo = (tagVideoFrameParam *)param;
            frameType = vedioInfo->frametype;
        }
        pChannel->m_pNewCallBack(dwUser, frameType, pBuffer, dwBufSize, 5, (void*)pChannel->m_nData, 0, 0);
    }
}

//////////////////////////////////////////////////////////////////////
// 主系统类::对外接口
//////////////////////////////////////////////////////////////////////

//静态字段声明
long CDevice_DH::m_nDeviceType = 0;
long CDevice_DH::m_nDataType = 0;
char CDevice_DH::g_szFlag[] = "Dll_DeviceDH";
unsigned char* pszStringBinding = NULL;

long CDevice_DH::EDvr_Init()
{	
    /*初始化*/
    if(!CLIENT_Init(DisConnectFunc,(unsigned long)this))
        return -1;
    m_bNoInit = false;
    
    //内部查看模式
    GetPrivateProfileInt("WriteLog", g_szFlag, 0, "d:\\netmcset\\底层配置.ini");
    EnableLog(true);

    /*设置消息回调*/
    CLIENT_SetDVRMessCallBack(MessCallBack,(unsigned long)this);
    //设置自动上线重连 20081121 HuangYJ
    CLIENT_SetAutoReconnect(ReOnlineFunc, (unsigned long)this);
    /*设置响应时间*/
    CLIENT_SetConnectTime(5000,3);

    return 0;
}

/*停止当前的所有操作，退出释放所有资源*/
void CDevice_DH::EDvr_Exit()
{
    if(m_bNoInit)
        return;
    m_bNoInit = true;

    EDvr_LogOutAll();
    CLIENT_Cleanup();
    if(m_BackFileList)
        delete m_BackFileList;
    m_BackFileList = NULL;
}

/*设置断线消息回调*/
void CDevice_DH::EDvr_SetDisConnectCallBackFunc(fDisConnectCB lpFuncCB)
{
    if(m_pFunDisConnectCB == NULL)
        m_pFunDisConnectCB = lpFuncCB;
}

/*登录到指定的设备上*/
long CDevice_DH::EDvr_Login(char *pstrIP, long nPort, char*pstrLogName, char *pstrLogPass, bool bCheckUser)
{
    /*判断是否初始化*/
    if(m_bNoInit)
    {
        int nRet = EDvr_Init();
        if(nRet < 0)
            return -1;
    }

    DWORD dwIP = inet_addr(pstrIP);
    /*判断输入参数合法*/
    if(dwIP == INADDR_NONE || nPort > 0x0000ffff)
        return -1;
    /*设备是否已经登陆*/
    long nLoginID = m_DeviceList.SelectLoginID(pstrIP);
    SDeviceInfo_DH* pDevice = m_DeviceList.GetDevice(nLoginID);
    if(nLoginID != -1 && pDevice->m_nHandle != 0)
        return nLoginID;
    /*申请临时空间保存信息*/
    SDeviceInfo_DH temp;
    StructInit(temp);

    temp.m_dwIP = dwIP;
    if(nPort != 0) temp.m_wPost = (WORD)nPort;
    if( pstrLogName && *pstrLogName != 0 ) strcpy(temp.m_strUserName, pstrLogName);
    if( pstrLogPass && *pstrLogPass != 0 ) strcpy(temp.m_strPassWord, pstrLogPass);
    temp.m_nID = 1;
    /*执行注册操作*/
    int nError;
    NET_DEVICEINFO tmpDev;

    {//权限验证登录,用户重用模式
        WriteLog(g_szFlag, "登录设备Ex IP:%s Port:%d Name:%s Pwd:%s", pstrIP, nPort, temp.m_strUserName, temp.m_strPassWord);
        temp.m_nHandle = CLIENT_Login(pstrIP,(WORD)nPort,temp.m_strUserName,temp.m_strPassWord,&tmpDev,&nError);
    }
    if(temp.m_nHandle == 0)
    {
        WriteLog(g_szFlag, "登录设备失败 API返回失败 IP:%s Error:0x%x", pstrIP, CLIENT_GetLastError());
        return -1;
    }
    /*打开设备监听*/
    // if(!EDvr_Listen(temp)) huangsq 2010.5.12
    if(EDvr_Listen(temp) < 0)
    {
        WriteLog(g_szFlag, "登录设备失败 开启监听失败 IP:%s", pstrIP);
        CLIENT_Logout(temp.m_nHandle);
        return -1;
    }
    if(pDevice == NULL)//新登录设备
        nLoginID = m_DeviceList.AddLogInDevice(temp);
    else//已存在设备
        memcpy(pDevice, &temp, sizeof(SDeviceInfo_DH));
    
    in_SaveComCfg(nLoginID);
    return nLoginID;
}

/*打开设备监听*/
long CDevice_DH::EDvr_Listen(SDeviceInfo_DH &rDevice)
{
    //得到设备的版本
    int nRet, bRet;
    if(m_pFunCommAlarmCB)
    {
		bRet = CLIENT_QueryDevState(rDevice.m_nHandle, DH_DEVSTATE_PROTOCAL_VER, (char*)&rDevice.m_nDevVer, sizeof(int), &nRet, 1000);
        if(bRet && rDevice.m_nDevVer >= 5)
        {
            bRet = CLIENT_StartListenEx(rDevice.m_nHandle);
        }
        else
        {
            bRet = CLIENT_StartListen(rDevice.m_nHandle);
        }
        if(!bRet) return -1;
    }
    return 0;
}

long CDevice_DH::EDvr_ReLogin(char *pstrIP)
{
    long nLoginID = m_DeviceList.SelectLoginID(pstrIP);
    if( -1 == nLoginID )
        return EDvr_Login(pstrIP, 0, NULL, NULL, true);
    else
        return EDvr_ReLogin(nLoginID);
}

/*从指定的设备上注销*/
void CDevice_DH::EDvr_LogOut(char *pstrIP)
{
    /*设备是否登陆过*/
    SDeviceInfo_DH *pTemp = m_DeviceList.SearchDevice(pstrIP);
    if(pTemp == NULL)
        return;
    
    /*获取登陆纪录*/
    long lDeviceHandle = pTemp->m_nHandle, nRet = -1;
    /*停止设备监听*/
    nRet = CLIENT_StopListen(lDeviceHandle);
    /*断开设备所有数据连接*/
    EDvr_StopDeviceDataLink(pstrIP);
    /*注销设备*/
    nRet = CLIENT_Logout(lDeviceHandle);
    
    /*释放设备信息*/
    m_DeviceList.DeletLogOutDevice(pstrIP);
}

/*从指定的设备上注销 add pan*/
void CDevice_DH::EDvr_LogOut(long lLoginHandle)
{
	/*设备是否登陆过*/
	SDeviceInfo_DH *pTemp = m_DeviceList.SearchDeviceByLogInID(lLoginHandle);
	if(pTemp == NULL)
		return;

	/*获取登陆纪录*/
	long lDeviceHandle = pTemp->m_nHandle, nRet = -1;
	/*停止设备监听*/
	nRet = CLIENT_StopListen(lDeviceHandle);
	/*注销设备*/
	nRet = CLIENT_Logout(lDeviceHandle);

	/*释放设备信息*/
	m_DeviceList.DeletLogOutDevice(lLoginHandle);
}

/*从所有已登录的设备上注销*/
void CDevice_DH::EDvr_LogOutAll()
{
    in_addr IP;
    for(long i=0; i<m_DeviceList.GetArraySize(); i++)
    {
        SDeviceInfo_DH *pTemp = m_DeviceList.GetDevice(i);
        if(pTemp != NULL)
        {
            IP.s_addr = pTemp->m_dwIP;
            char* strIP = inet_ntoa(IP);
            EDvr_LogOut(strIP);
        }
    }
}

/*远程重启设备*/
void CDevice_DH::EDvr_Reboot(char *pstrIP)
{
    /*设备是否登陆过*/
    SDeviceInfo_DH *pTemp = m_DeviceList.SearchDevice(pstrIP);
    if(pTemp == NULL)
        return;
    /*获取登陆纪录*/
    long lDeviceHandle = pTemp->m_nHandle;
    /*重启设备*/
    CLIENT_RebootDev(lDeviceHandle);
}

/*建立与指定设备的指定通道的数据链接*/
long CDevice_DH::EDvr_StartDataLink(char *pstrIP,long nChID,long nLinkType,long nMediaType,HWND hWin)
{
    //内部查看模式
    int nLog = GetPrivateProfileInt("WriteLog", "DLL_DeviceDH", 0, "D:\\NetMcSet\\底层配置.ini");
    EnableLog(nLog);

    WriteLog(g_szFlag, "开启连接 Begin IP:%s ChID:%d MediaType:%d HWND:0x%x", pstrIP, nChID, nMediaType, hWin);
    DWORD dwIP = inet_addr(pstrIP);
    long nLinkID = -1;
    /*判断输入参数合法*/
    if(dwIP == INADDR_NONE)
    {
        WriteLog(g_szFlag, "开启连接失败 IP非法错误 IP:%s", pstrIP);
        return -1;
    }
    /*通道是否已经开通*/
    if((nLinkID = m_ChannelList.GetChannelIndex(pstrIP,nChID)) != -1)
    {
        SChannelInfo_DH* theChannel = m_ChannelList.GetChannel(nLinkID);
        if(theChannel->m_nHandle != -1)
        {
            WriteLog(g_szFlag, "开启连接失败 连接已存在 IP:%s ChID:%d", pstrIP, nChID);
            return -1;
        }
        else
        {//增加重连方式调用
            return EDvr_ReLink(theChannel->m_dwIP, theChannel->m_nID);
        }
    }
    /*显示窗口是否被占用*/
    if(-1 != m_ChannelList.CheckFormUsed(hWin))
    {
        WriteLog(g_szFlag, "开启连接失败 显示窗口被占用 IP:%s ChID:%d HWND:0x%x", pstrIP, nChID, hWin);
        return -1;
    }
    /*申请临时空间保存信息*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = nChID;
    temp.m_nChannelType = 1;
    temp.m_nMediaType = nMediaType;
    temp.m_hWnd = hWin;
    /*设备是否登陆过*/
    SDeviceInfo_DH *pTemp = m_DeviceList.SearchDevice(pstrIP);
    if(pTemp == NULL)
    {
        WriteLog(g_szFlag, "开启连接失败 设备没有登陆 IP:%s", pstrIP);
        return -1;
    }
    /*建立指定数据连接*/
    temp.m_nHandle = CLIENT_RealPlayEx(pTemp->m_nHandle, nChID, hWin, FixMediaType(nMediaType));
    if (temp.m_nHandle == 0)
    {
        long nError = CLIENT_GetLastError();
        WriteLog(g_szFlag, "开启连接失败 API调用失败 开始重试 IP:%s ChID:%d HWND:0x%x Error:0x%x", pstrIP, nChID, hWin, nError);
        long nPlayID = m_ChannelList.AddPlayChannel(temp);
        if(EDvr_ReLogin(pstrIP) == -1)
        {
            WriteLog(g_szFlag, "开启连接失败 重登录失败 IP:%s ChID:%d HWND:0x%x", pstrIP, nChID, hWin);
            m_ChannelList.DeletStopChannel(nPlayID);
            return -1;
        }
        SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nPlayID);
        if(sChannel->m_nHandle != 0)
        {
            return nPlayID;
        }
        m_ChannelList.DeletStopChannel(nPlayID);
        return -1;
    }
    /*保存通道信息并返回通道号*/
    WriteLog(g_szFlag, "开启连接 End IP:%s ChID:%d HWND:0x%x", pstrIP, nChID, hWin);
    return m_ChannelList.AddPlayChannel(temp);
}

/*停止与指定设备的指定通道的数据链接*/
void CDevice_DH::EDvr_StopDataLink(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    EDvr_StopDataLink(nRDataLink);
}

void CDevice_DH::EDvr_StopDataLink(long nRDataLink)
{
    WriteLog(g_szFlag, "停止连接 LinkID:%d", nRDataLink);
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return;

    //begin 20090323 停止可能存在的重连动作
    for(unsigned int i=0; i<m_lstRelink.size(); i++)
    {
        if(m_lstRelink[i].nIPv4 == sChannel->m_dwIP && m_lstRelink[i].nChID == sChannel->m_nID)
        {
            StructInit(m_lstRelink[i]);
            break;
        }
    }
    sChannel->m_nOutTime = DATA_TIME_OUT;
    //end 20090323

    /*如果正在数据传输,就停止动作*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(nRDataLink);
    /*如果正在录像,就停止录像*/
    if(sChannel->m_aPlayState[1])
        EDvr_StopDataRecorder(nRDataLink);
    /*断开数据连接*/
    CLIENT_StopRealPlayEx(sChannel->m_nHandle);
    /*释放数据通道*/
    m_ChannelList.DeletStopChannel(nRDataLink);
}

/*停止当前的所有数据链接*/
void CDevice_DH::EDvr_StopAllDataLink()
{
    for(long i=0; i < m_ChannelList.GetArraySize(); i++)
    {
        if(m_ChannelList.GetChannel(i)->m_nID <= DH_MAX_CHANNUM)
            EDvr_StopDataLink(i);
        else
            EDvr_StopMultiLink(i);
    }
}

/*获取采集通道对应操作句柄*/
long CDevice_DH::EDvr_GetChannelHandle(char *pstrIP, long nChID)
{
    return m_ChannelList.GetChannelIndex(pstrIP,nChID);
}


/*启动指定设备的指定通道的数据链接的数据捕获*/
long CDevice_DH::EDvr_StartDataCapture(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    return EDvr_StartDataCapture(nRDataLink);
}

long CDevice_DH::EDvr_StartDataCapture(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return -1;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return -1;
    /*判断是否正在数据捕获*/
    if(sChannel->m_aPlayState[0])
        return -1;
    /*开始捕获数据*/
    sChannel->m_aPlayState[0] = true;
    sChannel->m_nData = nRDataLink;
    CLIENT_SetRealDataCallBackEx(sChannel->m_nHandle, DataCallBack, (DWORD)nRDataLink, 0x00000001);
    return 0;
}

/*停止指定设备的指定通道的数据链接的数据捕获*/
void CDevice_DH::EDvr_StopDataCapture(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    EDvr_StopDataCapture(nRDataLink);
}

void CDevice_DH::EDvr_StopDataCapture(long nLinkID)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nLinkID);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*判断是否正在数据捕获*/
    if(!sChannel->m_aPlayState[0])
        return;
    /*开始捕获数据*/
    CLIENT_SetRealDataCallBackEx(sChannel->m_nHandle,NULL,0,0);
    sChannel->m_aPlayState[0] = FALSE;
    return;
}

/*开始指定设备是定通道的数据连接的数据保存文件(录像)*/
long CDevice_DH::EDvr_StartDataRecorder(char *pstrIP, long nChID, char *pstrFileName)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    return EDvr_StartDataRecorder(nRDataLink, pstrFileName);
}

long CDevice_DH::EDvr_StartDataRecorder(long nRDataLink, char *pstrFileName)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return -1;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return -1;
    /*判断是否正在录像*/
    if(sChannel->m_aPlayState[1])
        return -1;
    /*判断文件名有效*/
    if(pstrFileName == NULL || pstrFileName[0] == '\0')
        return -1;
    /*开始录像*/
    if(!CLIENT_SaveRealData(sChannel->m_nHandle,pstrFileName))
        return -1;
    sChannel->m_aPlayState[1] = true;
    return 0;
}

/*停止指定设备指定通道的数据连接的数据保存文件(录像)*/
void CDevice_DH::EDvr_StopDataRecorder(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    EDvr_StopDataRecorder(nRDataLink);
}

void CDevice_DH::EDvr_StopDataRecorder(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*判断是否正在录像*/
    if(!sChannel->m_aPlayState[1])
        return;
    /*停止录像*/
    if(!CLIENT_StopSaveRealData(sChannel->m_nHandle))
        return;
    sChannel->m_aPlayState[1] = false;
}

/*对指定设备指定通道的御览画面进行截图*/
long CDevice_DH::EDvr_ConvertPicture(char *pstrIP, long nChID, char *pstrFileName)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    return EDvr_ConvertPicture(nRDataLink, pstrFileName);
}

long CDevice_DH::EDvr_ConvertPicture(long nRDataLink, char *pstrFileName)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return -1;
    /*开始截图*/
    if(!CLIENT_CapturePicture(sChannel->m_nHandle, pstrFileName))
        return -1;
    return 0;
}

/*云台控制*/
long CDevice_DH::EDvr_PTZControl(char *pstrIP, long nChID, long nPTZCommand, bool bStop, long nSpeed)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*判断设备登陆*/
    if(NULL == sDevice)
        return -1;
    /*判断参数有效*/
    //	if(0 >nSpeed || 8<nSpeed)
    //		return -1;
    /*开始云台控制*/
    if(!bStop)
    {
        CLIENT_PTZControl(sDevice->m_nHandle, nChID, nPTZCommand, nSpeed, 1);
    }
    if(!CLIENT_PTZControl(sDevice->m_nHandle, nChID, nPTZCommand, nSpeed, (bool)bStop))
    {
        /*
        if( -1 == EDvr_ReLogin(pstrIP) )
        {
            LeaveCriticalSection(&m_csPTZCtrl);
            return -1;
        }
        else
        {
            if(!CLIENT_PTZControl(sDevice->m_nHandle, nChID, nPTZCommand, nSpeed, (bool)bStop))
            {
                LeaveCriticalSection(&m_csPTZCtrl);
                return -1;
            }
        }
        */
        return -1;
    }
    return 0;
}

/*云台控制扩展*/
long CDevice_DH::EDvr_PTZControlEx(char *pstrIP, long nChID, long nPTZCommandEx, bool bStop,
                                   unsigned char param1, unsigned char param2, unsigned char param3)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*判断设备登陆*/
    if(NULL == sDevice)
        return -1;
    /*开始云台控制*/
    if(!bStop)
    {
        CLIENT_DHPTZControlEx(sDevice->m_nHandle, nChID, nPTZCommandEx, param1, param2, param3, 1);
    }
    if(!CLIENT_DHPTZControlEx(sDevice->m_nHandle, nChID, nPTZCommandEx, param1, param2, param3, bStop))
    {
        /*
        if( -1 == EDvr_ReLogin(pstrIP) )
        {
            LeaveCriticalSection(&m_csPTZCtrl);
            return -1;
        }
        else
        {
            if(!CLIENT_DHPTZControlEx(sDevice->m_nHandle, nChID, nPTZCommandEx, param1, param2, param3, bStop))
            {
                LeaveCriticalSection(&m_csPTZCtrl);
                return -1;
            }
        }
        */
        return -1;
    }
    return 0;
}

/*获取远程录像配置*/
long CDevice_DH::EDvr_GetRecordBackState(char *pstrIP, char *pRState, long nMaxLen, long *pnStateLen)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*判断设备登陆*/
    if(NULL == sDevice)
        return -1;
    if(!CLIENT_QueryRecordState(sDevice->m_nHandle, pRState, nMaxLen, (int *)pnStateLen, 2000))
    {
        if( -1 == EDvr_ReLogin(pstrIP) )
            return -1;
        else
            if(!CLIENT_QueryRecordState(sDevice->m_nHandle, pRState, nMaxLen, (int *)pnStateLen, 2000))
                return -1;
    }
    return 0;
}

/*设置远程录像配置*/
long CDevice_DH::EDvr_SetRecordBackState(char *pstrIP, char *pRState, long nStateLen)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*判断设备登陆*/
    if(NULL == sDevice)
        return -1;
    if(!CLIENT_SetupRecordState(sDevice->m_nHandle, pRState, nStateLen))
    {
        if( -1 == EDvr_ReLogin(pstrIP) )
            return -1;
        else
            if(!CLIENT_SetupRecordState(sDevice->m_nHandle, pRState, nStateLen))
                return -1;
    }
    return 0;
}

/*关闭查询句柄*/
void CDevice_DH::EDvr_FindClose(long lFileFindHandle)
{
    CLIENT_FindClose(lFileFindHandle);
}
NET_RECORDFILE_INFO temp;
/*查找下一条记录*/
long CDevice_DH::EDvr_FindNextFile(long lFileFindHandle)
{    
	long nRes = 0;
	nRes = CLIENT_FindNextFile(lFileFindHandle,&temp);
    m_BackFileList->AddFileInfo(temp);
    return nRes;
}

/*开始播放录像文件*/
long CDevice_DH::EDvr_StartPlayBack(char *pstrIP, long nFileHandle, HWND hWnd)
{
    DWORD dwIP = inet_addr(pstrIP);
    /*判断输入参数合法*/
    if(dwIP == INADDR_NONE)
        return -1;
    /*设备是否已登陆*/
    if(m_DeviceList.SearchDevice(pstrIP) == NULL)
        return -1;
    /*查询回放文件*/
    NET_RECORDFILE_INFO * lpRecordFile = m_BackFileList->GetFileInfo(nFileHandle);
    if(NULL == lpRecordFile)
        return -1;
    /*通道是否已经开通*/
    if(m_ChannelList.SearchChannel(pstrIP,lpRecordFile->ch) != NULL)
        return -2;
    /*显示窗口是否被占用*/
    if(-1 != m_ChannelList.CheckFormUsed(hWnd))
        return -1;
    /*申请临时空间保存信息*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = lpRecordFile->ch;
    temp.m_nChannelType = 2;
    temp.m_hWnd = hWnd;
    /*建立指定数据回放*/
//     temp.m_nHandle = CLIENT_PlayBackByRecordFile(m_DeviceList.SearchDevice(pstrIP)->m_nHandle,
//         lpRecordFile, hWnd, FileDownloadPosCallBack, (DWORD)this);
    if (temp.m_nHandle == 0)
        return -1;
    /*保存通道信息并返回通道号*/
    return m_ChannelList.AddPlayChannel(temp);
}
long CDevice_DH::EDvr_StartPlayBack(char *pstrIP, long nChID, char *pstrFileName, HWND hWnd)
{
    DWORD dwIP = inet_addr(pstrIP);
    /*判断输入参数合法*/
    if(dwIP == INADDR_NONE)
        return -1;
    /*设备是否已登陆*/
    if(m_DeviceList.SearchDevice(pstrIP) == NULL)
        return -1;
    /*通道是否已经开通*/
    if(m_ChannelList.SearchChannel(pstrIP,nChID) != NULL)
        return -2;
    /*申请临时空间保存信息*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = nChID;
    temp.m_nChannelType = 2;
    temp.m_hWnd = hWnd;
    /*构造文件信息结构*/
    NET_RECORDFILE_INFO lRecordFile;
    lRecordFile.ch = nChID;
    strcpy(lRecordFile.filename,pstrFileName);
    /*建立指定数据回放*/
    temp.m_nHandle = CLIENT_PlayBackByRecordFile(m_DeviceList.SearchDevice(pstrIP)->m_nHandle,
        &lRecordFile, hWnd, FileDownloadPosCallBack, (unsigned long)this);
    if (temp.m_nHandle == 0)
        return -1;
    /*保存通道信息并返回通道号*/
    return m_ChannelList.AddPlayChannel(temp);
}

BOOL CALLBACK CDevice_DH::MessCallBack(long lCommand, long lLoginID, char *pBuf, DWORD dwBufLen,
                                       char *pchDVRIP, long nDVRPort, DWORD dwUser)
{
    CDevice_DH * device = (CDevice_DH *)dwUser;
    int ret = true;
    char cNoAlarm[32]={0};
    //char szAlarmState[32] = {0};
    if (!dwUser) return FALSE;

    switch(lCommand)
    {
    case DH_COMM_ALARM:			//普通报警
        {
            NET_CLIENT_STATE *pMessage = (NET_CLIENT_STATE *)pBuf;

            if(NULL != device->m_pFunCommAlarmCB)
            {
                if(memcmp(pMessage->alarm,cNoAlarm,DH_MAX_ALARM_IN_NUM))
                    ret = device->m_pFunCommAlarmCB(pchDVRIP,nDVRPort,pMessage->alarminputcount,pMessage->alarm);
            }else
                ret = FALSE;
            if(NULL != device->m_pFuncMotionDectionCB)
            {
                if(memcmp(pMessage->motiondection,cNoAlarm,DH_MAX_CHANNUM))
                    ret = device->m_pFuncMotionDectionCB(pchDVRIP,nDVRPort,pMessage->channelcount,pMessage->motiondection);
            }else
                ret = FALSE;
            if(NULL != device->m_pFuncVideoLostCB)
            {
                if(memcmp(pMessage->videolost,cNoAlarm,DH_MAX_CHANNUM))
                    ret = device->m_pFuncVideoLostCB(pchDVRIP,nDVRPort,pMessage->channelcount,pMessage->videolost);
            }else
                ret = FALSE;
        }
        break;
    case DH_SHELTER_ALARM:		//遮挡报警
        {
            if(NULL == device->m_pFunShelterAlarmCB)
                return FALSE;
            ret = device->m_pFunShelterAlarmCB(pchDVRIP,nDVRPort,(BYTE*)pBuf,dwBufLen);
        }
        break;
    case DH_DISK_FULL_ALARM:	//磁盘满报警
        {
            if(NULL == device->m_pFunDiskFullCB)
                return FALSE;
            ret = device->m_pFunDiskFullCB(pchDVRIP,nDVRPort,(BYTE)*(DWORD*)pBuf);
        }
        break;
    case DH_DISK_ERROR_ALARM:	//磁盘错误报警
        {
            if(NULL == device->m_pFunDiskErrorCB)
                return FALSE;
            BYTE aDiskError[32];
            DWORD nTemp = *((DWORD*)pBuf);
            for(long i=0; i<32; i++)
            {
                aDiskError[i] = (BYTE)(nTemp%2);
                nTemp<<=1;
            }
            ret = device->m_pFunDiskErrorCB(pchDVRIP,nDVRPort,aDiskError,32);
        }
        break;
    case DH_SOUND_DETECT_ALARM:	//音频监控报警
        {
            if(NULL == device->m_pFunSoundDetectCB)
                return FALSE;
            ret = device->m_pFunSoundDetectCB(pchDVRIP,nDVRPort,(BYTE*)pBuf, dwBufLen);
        }
        break;
    default:
        return MessCallBackV26(lCommand, lLoginID, pBuf, dwBufLen, pchDVRIP, nDVRPort, dwUser);
    }
    return ret;
}

/*停止回放*/
void CDevice_DH::EDvr_StopPlayBack(char *pstrIP, long nChID)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,nChID);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 2)
        return;
    /*如果正在数据捕获,就停止动作*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(pstrIP,nChID);
    /*如果正在录像,就停止录像*/
    if(sChannel->m_aPlayState[1])
        EDvr_StopDataRecorder(pstrIP,nChID);
    /*断开数据连接*/
    CLIENT_StopPlayBack(sChannel->m_nHandle);
    /*释放数据通道*/
    m_ChannelList.DeletStopChannel(pstrIP,nChID);
}
void CDevice_DH::EDvr_StopPlayBack(long nBDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nBDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为回放数据连接*/
    if(sChannel->m_nChannelType != 2)
        return;
    /*如果正在数据传输,就停止动作*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(nBDataLink);
    /*断开数据连接*/
    CLIENT_StopPlayBack(sChannel->m_nHandle);
    /*释放数据通道*/
    m_ChannelList.DeletStopChannel(nBDataLink);
}
void CDevice_DH::EDvr_StopPlayBack(HWND hWnd)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
    if(!sChannel)
        return;
    /*判断是否为回放数据连接*/
    if(sChannel->m_nChannelType != 2)
        return;
    /*如果正在数据传输,就停止动作*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(m_ChannelList.GetChannelIndex(hWnd));
    /*断开数据连接*/
    CLIENT_StopPlayBack(sChannel->m_nHandle);
    /*释放数据通道*/
    m_ChannelList.DeletStopChannel(m_ChannelList.GetChannelIndex(hWnd));
}

long CDevice_DH::EDvr_StartFileDownload(char *pstrIP, long nFileHandle, char *sSavedFileName)
{
    return -1;
}
long CDevice_DH::EDvr_StartFileDownload(char *pstrIP, long nChID, char *pstrFileName, char *sSavedFileName)
{
    return -1;
}

/*停止下载录像文件*/
void CDevice_DH::EDvr_StopFileDownload(long nDownloadHandle)
{
    //修正数据头
    //if(sChannel->m_szSavePath != NULL)
    //{
    //    FILE* pFile = fopen(sChannel->m_szSavePath, "r+");
    //    if(pFile != NULL)
    //    {
    //        fwrite("DAHUA", 5, 1, pFile);
    //        fclose(pFile);
    //    }
    //    delete[] sChannel->m_szSavePath;
    //    sChannel->m_szSavePath = NULL;
    //}
}
void CDevice_DH::EDvr_StopFileDownload(long nChID, char *pstrFileName)
{
    /*判断数据连接通道是否打开*/
    long nFileHandle;
    if(NULL == m_BackFileList->SearchFileInfo(nChID,pstrFileName,&nFileHandle))
        return;
    /*断开数据连接*/
//    CLIENT_StopDownload(m_BackFileList->m_hDownloadHandle[nFileHandle]);
    //修正数据头
//    if(m_BackFileList->m_szSavePath[nFileHandle] != NULL)
//    {
//        FILE* pFile = fopen(m_BackFileList->m_szSavePath[nFileHandle], "wb");
//        if(pFile != NULL)
//        {
//            fwrite("DAHUA", 5, 1, pFile);
//            fclose(pFile);
//        }
//        delete[] m_BackFileList->m_szSavePath[nFileHandle];
//        m_BackFileList->m_szSavePath[nFileHandle] = NULL;
//    }
    /*释放数据通道*/
//    if(sChannel->m_pRPFile)
//    {
//        delete sChannel->m_pRPFile;
//    }
//    if(sChannel->m_szSavePath)
//    {
//        delete[] sChannel->m_szSavePath;
//    }
//    long nRealHandle = m_BackFileList->m_hDownloadHandle[nFileHandle];
//    m_ChannelList.DeletStopChannel(m_ChannelList.FixHandle(nRealHandle));
//    m_BackFileList->m_hDownloadHandle[nFileHandle] = NULL;
}

/*获得下载录像的当前位置*/
long CDevice_DH::EDvr_GetDownloadPos(long nDownHandle, long *nTotalSize, long *nDownLoadSize)
{
    /*判断数据连接通道是否打开*/
    if(NULL == m_BackFileList->GetFileInfo(nDownHandle))
        return -1;
//    long nRealHandle = m_BackFileList->m_hDownloadHandle[nDownHandle];
//    return CLIENT_GetDownloadPos(nRealHandle, (int*)nTotalSize, (int*)nDownLoadSize);
    return 0;
}

/*设置语音回调函数*/
void CDevice_DH::EDvr_SetAudioDataCallBackFunc(fAudioDataCallBack CallBackFunc)
{
    m_pFunAudioCB = CallBackFunc;
}

//开启录音采集出来的音频数据
long CDevice_DH::EDvr_StartAudioDataCapture()
{
    if (!CLIENT_RecordStart())
        return -1;
    return 0;
}

//关闭录音采集出来的音频数据
void CDevice_DH::EDvr_StopAudioDataCapture()
{
    CLIENT_RecordStop();
}

/*开启音频*/
long CDevice_DH::EDvr_OpenSound(char *pstrIP, long nChID)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,nChID);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return -1;
    /*开启音频*/
    if(!CLIENT_OpenSound(sChannel->m_nHandle))
        return -1;
    return 0;
}

/*关闭音频*/
void CDevice_DH::EDvr_CloseSound()
{
    CLIENT_CloseSound();
}

/*发送语音到设备*/
long CDevice_DH::EDvr_AudioDataSend(char *pstrIP, char *pSendBuff, long nBuffSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*语音对讲是否开启*/
    if (0 == temp->m_nAudioHandle)
        return -1;
    if(!CLIENT_TalkSendData(temp->m_nAudioHandle, pSendBuff, (DWORD)nBuffSize))
        return -1;
    return 0;
}

/*解码从设备得到的语音数据*/
void CDevice_DH::EDvr_AudioDecode(char *pSendBuff, long nBuffSize)
{
    CLIENT_AudioDec(pSendBuff, nBuffSize);
}

/*设置音量*/
long CDevice_DH::EDvr_SetAudioVolume(char *pstrIP, unsigned short wVolume)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*语音对讲是否开启*/
    if (0 == temp->m_nAudioHandle)
        return -1;
    /*设置音量*/
    CLIENT_SetAudioClientVolume(temp->m_nAudioHandle, wVolume);
    return 0;
}


/*设置普通报警消息回调*/
void CDevice_DH::EDvr_SetCommAlarmCallBackFunc(fCommAlarmCB CallBackFunc)
{
    m_pFunCommAlarmCB = CallBackFunc;
}

//设置移动侦测报警回调
void CDevice_DH::EDvr_SetMotionDectionCallBackFunc(fMotionDectionCB CallBackFunc)
{
    m_pFuncMotionDectionCB = CallBackFunc;
}

//设置视频丢失报警回调
void CDevice_DH::EDvr_SetVideoLostCallBackFunc(fVideoLostCB CallBackFunc)
{
    m_pFuncVideoLostCB = CallBackFunc;
}

/*设置视频遮挡消息回调*/
void CDevice_DH::EDvr_SetShelterAlarmCallBackFunc(fShelterAlarmCB CallBackFunc)
{
    m_pFunShelterAlarmCB = CallBackFunc;
}

/*设置硬盘满消息回调*/
void CDevice_DH::EDvr_SetDiskFullCallBackFunc(fDiskFullCB CallBackFunc)
{
    m_pFunDiskFullCB = CallBackFunc;
}

/*设置硬盘错误消息回调*/
void CDevice_DH::EDvr_SetDiskErrorCallBackFunc(fDiskErrorCB CallBackFunc)
{
    m_pFunDiskErrorCB = CallBackFunc;
}

/*设置音频监测消息回调*/
void CDevice_DH::EDvr_SetSoundDetectCallBackFunc(fSoundDetectCB CallBackFunc)
{
    m_pFunSoundDetectCB = CallBackFunc;
}

long CDevice_DH::EDvr_SetNormalAlarmCFG(char *pstrIP, long nAlarmKind, long nAlarmChannel, long nAlarmInfo, long nAlarmEnable)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
    {
        if( -1 == EDvr_ReLogin(pstrIP) )
            return -1;
        else
            if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
                return -1;
    }
    /*修改需要设置的参数*/
    switch(nAlarmKind)
    {
    case 0://报警输入
        almCFG->struLocalAlmIn[nAlarmChannel].byAlarmType = (BYTE)nAlarmInfo;
        almCFG->struLocalAlmIn[nAlarmChannel].byAlarmEn = (BYTE)nAlarmEnable;
        break;
    case 1://移动侦测
        almCFG->struMotion[nAlarmChannel].wSenseLevel = (WORD)nAlarmInfo;
        almCFG->struMotion[nAlarmChannel].byMotionEn = (BYTE)nAlarmEnable;
        break;
    case 2://视频丢失
        almCFG->struVideoLost[nAlarmChannel].byAlarmEn = (BYTE)nAlarmEnable;
        break;
    case 3://视频遮挡
        almCFG->struBlind[nAlarmChannel].byBlindLevel = (BYTE)nAlarmInfo;
        almCFG->struBlind[nAlarmChannel].byBlindEnable = (BYTE)nAlarmEnable;
        break;
    case 4://磁盘报警
        switch(nAlarmInfo)
        {
        case 1://无磁盘
            almCFG->struDiskAlarm.byNoDiskEn =  (BYTE)nAlarmEnable;
            break;
        case 2://磁盘满
            almCFG->struDiskAlarm.byLowCapEn =  (BYTE)nAlarmEnable;
            break;
        case 3://磁盘故障
            almCFG->struDiskAlarm.byDiskErrEn =  (BYTE)nAlarmEnable;
            break;
        default:
            break;
        }
        break;
        default:
            break;
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*获取外部报警布防状态*/
long CDevice_DH::EDvr_GetMsgAlarmInEnable(char *pstrIP, bool *pIsEnable, long *pAlarmInCount)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || pAlarmInCount == NULL)
        return -1;
    /*得到配置信息*/
    int retlen = 0;
    //DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    //if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
    ALARM_CONTROL almCfg[16];
    if(!CLIENT_QueryIOControlState(temp->m_nHandle,DH_ALARMINPUT,0,0,&retlen))
    {
        if( -1 == EDvr_ReLogin(pstrIP))
            return -1;
        else
            if(!CLIENT_QueryIOControlState(temp->m_nHandle,DH_ALARMINPUT,0,0,&retlen))
                return -1;
    }
    if(!CLIENT_QueryIOControlState(temp->m_nHandle,DH_ALARMINPUT,almCfg,sizeof(ALARM_CONTROL)*16,&retlen))
        return -1;
    /*返回配置结果*/
    *pAlarmInCount = retlen;
    for(long i=0; i<retlen; i++)
    {
        pIsEnable[i] = ((almCfg[i].state)?true:false);
    }
    return 0;
}

/*设置外部报警布防状态*/
long CDevice_DH::EDvr_SetMsgAlarmInEnable(char *pstrIP, const bool *pIsEnable, long nAlarmInCount /*= DH_MAX_ALARM_IN_NUM*/)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || nAlarmInCount == 0)
        return -1;
    ALARM_CONTROL almCfg[16];
    /*修改需要设置的参数*/
    for(long i=0; i<nAlarmInCount; i++)
    {
        almCfg[i].index = (WORD)i;
        almCfg[i].state = pIsEnable[i];
    }
    /*设置参数*/
    if(!CLIENT_IOControl(temp->m_nHandle,DH_ALARMINPUT,almCfg,sizeof(ALARM_CONTROL)*nAlarmInCount))
    {
        if( -1 == EDvr_ReLogin(pstrIP))
            return -1;
        else
            if(!CLIENT_IOControl(temp->m_nHandle,DH_ALARMINPUT,almCfg,sizeof(ALARM_CONTROL)*nAlarmInCount))
                return -1;
    }
    return 0;
}

/*获取报警输出状态*/
long CDevice_DH::EDvr_GetMsgAlarmOutEnable(char *pstrIP, bool *pIsEnable, long *pAlarmOutCount)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || pAlarmOutCount == 0)
        return -1;
    /*得到配置信息*/
    int retlen = 0;
    ALARM_CONTROL almCfg[16];
    if(!CLIENT_QueryIOControlState(temp->m_nHandle,DH_ALARMOUTPUT,0,0,&retlen))
    {
        if( -1 == EDvr_ReLogin(pstrIP) )
            return -1;
        else
            if(!CLIENT_QueryIOControlState(temp->m_nHandle,DH_ALARMOUTPUT,0,0,&retlen))
                return -1;
    }
    if(!CLIENT_QueryIOControlState(temp->m_nHandle,DH_ALARMOUTPUT,almCfg,sizeof(ALARM_CONTROL)*16,&retlen))
    {
        return -1;
    }
    /*返回配置结果*/
    *pAlarmOutCount = retlen;
    for(long i=0; i<retlen; i++)
    {
        pIsEnable[i] = ((almCfg[i].state)?true:false);
    }
    return 0;
}
/*设置报警输出状态*/
long CDevice_DH::EDvr_SetMsgAlarmOutEnable(char *pstrIP, const bool *pIsEnable,	long nAlarmOutCount)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || nAlarmOutCount == 0)
        return -1;
    /*得到配置信息*/
    ALARM_CONTROL almCfg[16];
    /*修改需要设置的参数*/
    for(long i=0; i<nAlarmOutCount; i++)
    {
        almCfg[i].index = (WORD)i;
        almCfg[i].state = pIsEnable[i];
    }
    /*设置参数*/
    if(!CLIENT_IOControl(temp->m_nHandle, DH_ALARMOUTPUT, almCfg, sizeof(ALARM_CONTROL)*nAlarmOutCount))
    {
        if( -1 == EDvr_ReLogin(pstrIP) )
            return -1;
        else
            if(!CLIENT_IOControl(temp->m_nHandle,DH_ALARMOUTPUT,almCfg, sizeof(ALARM_CONTROL)*nAlarmOutCount))
                return -1;
    }
    return 0;
}

/*获取动态监测报警布防状态*/
long CDevice_DH::EDvr_GetMsgMotionDectionEnable(char *pstrIP, bool *pIsEnable, long *pVideoCount)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || pVideoCount == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    *pVideoCount = DH_MAX_VIDEO_IN_NUM;
    for(long i=0; i<DH_MAX_VIDEO_IN_NUM; i++)
    {
        if(!almCFG->struMotion[i].byMotionEn)
            pIsEnable[i] = false;
        else
            pIsEnable[i] = true;
    }
    return 0;
}

/*设置动态监测报警布防状态*/
long CDevice_DH::EDvr_SetMsgMotionDectionEnable(char *pstrIP, const bool *pIsEnable, long nVideoCount /*= DH_MAX_VEDIO_IN_NUM*/)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || nVideoCount == 0)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    for(long i=0; i<nVideoCount; i++)
    {
        almCFG->struMotion[i].byMotionEn = pIsEnable[i];
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*获取视频丢失布防状态*/
long CDevice_DH::EDvr_GetMsgVedioLostEnable(char *pstrIP, bool *pIsEnable, long *pVideoCount)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || pVideoCount == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    *pVideoCount = DH_MAX_VIDEO_IN_NUM;
    for(long i=0; i<DH_MAX_VIDEO_IN_NUM; i++)
    {
        if(!almCFG->struVideoLost[i].byAlarmEn)
            pIsEnable[i] = false;
        else
            pIsEnable[i] = true;
    }
    return 0;
}
/*设置视频丢失布防状态*/
long CDevice_DH::EDvr_SetMsgVedioLostEnable(char *pstrIP, const bool *pIsEnable, long nVideoCount /*= DH_MAX_VIDEO_IN_NUM*/)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || nVideoCount == 0)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    for(long i=0; i<nVideoCount; i++)
    {
        almCFG->struVideoLost[i].byAlarmEn = pIsEnable[i];
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*获取图像遮挡布防状态*/
long CDevice_DH::EDvr_GetMsgBlindEnable(char *pstrIP, bool *pIsEnable, long *pVideoCount)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || pVideoCount == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    *pVideoCount = DH_MAX_VIDEO_IN_NUM;
    for(long i=0; i<DH_MAX_VIDEO_IN_NUM; i++)
    {
        if(!almCFG->struBlind[i].byBlindEnable)
            pIsEnable[i] = false;
        else
            pIsEnable[i] = true;
    }
    return 0;
}

/*设置图像遮挡布防状态*/
long CDevice_DH::EDvr_SetMsgBlindEnable(char *pstrIP, const bool *pIsEnable, long nVideoCount /*= DH_MAX_VIDEO_IN_NUM*/)
{	
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL || nVideoCount == 0)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    for(long i=0; i<nVideoCount; i++)
    {
        almCFG->struBlind[i].byBlindEnable = pIsEnable[i];
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*获取硬盘消息布防状态*/
long CDevice_DH::EDvr_GetMsgHardDiskEnable(char *pstrIP, long nConfigType, bool *pIsEnable)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(pIsEnable == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    switch(nConfigType)
    {
    case 0:	//无硬盘
        if(!almCFG->struDiskAlarm.byNoDiskEn)
            *pIsEnable = false;
        else
            *pIsEnable = true;
        break;
    case 1:	//容量不足
        if(!almCFG->struDiskAlarm.byLowCapEn)
            *pIsEnable = false;
        else
            *pIsEnable = true;
        break;
    case 2:	//硬盘错误
        if(!almCFG->struDiskAlarm.byDiskErrEn)
            *pIsEnable = false;
        else
            *pIsEnable = true;
        break;
    default:return -1;
    }
    return 0;
}

/*设置硬盘消息布防状态*/
long CDevice_DH::EDvr_SetMsgHardDiskEnable(char *pstrIP, long nConfigType, bool bIsEnable)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    switch(nConfigType)
    {
    case 0:	//无硬盘
        almCFG->struDiskAlarm.byNoDiskEn = bIsEnable;
        break;
    case 1:	//容量不足
        almCFG->struDiskAlarm.byLowCapEn = bIsEnable;
        break;
    case 2:	//硬盘错误
        almCFG->struDiskAlarm.byDiskErrEn = bIsEnable;
        break;
    default:return -1;
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*开启拼接御览连接*/
long CDevice_DH::EDvr_StartMultiLink(char*pstrIP, long nLinkType, HWND hWin)
{
    DWORD dwIP = inet_addr(pstrIP);
    /*判断输入参数合法*/
    if(dwIP == INADDR_NONE)
        return -1;
    /*通道是否已经开通*/
    if(m_ChannelList.SearchChannel(pstrIP,DH_MAX_CHANNUM+1) != NULL)
        return -2;
    /*申请临时空间保存信息*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = DH_MAX_CHANNUM+1;
    temp.m_nChannelType = 1;
    /*设备是否已登陆*/
    if(m_DeviceList.SearchDevice(pstrIP) == NULL)
        return -1;
    /*建立指定数据连接*/
    temp.m_nHandle = CLIENT_MultiPlay(m_DeviceList.SearchDevice(pstrIP)->m_nHandle, hWin);
    if (temp.m_nHandle == 0)
        return -1;
    /*保存通道信息并返回通道号*/
    return m_ChannelList.AddPlayChannel(temp);
}

/*停止拼接御览连接*/
void CDevice_DH::EDvr_StopMultiLink(char* pstrIP)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,DH_MAX_CHANNUM+1);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*断开数据连接*/
    CLIENT_StopMultiPlay(sChannel->m_nHandle);
    /*释放数据通道*/
    m_ChannelList.DeletStopChannel(pstrIP,DH_MAX_CHANNUM+1);
}
void CDevice_DH::EDvr_StopMultiLink(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return;
    /*判断是否为即时数据连接*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*断开数据连接*/
    CLIENT_StopMultiPlay(sChannel->m_nHandle);
    /*释放数据通道*/
    m_ChannelList.DeletStopChannel(nRDataLink);
}

/*获取通道画质*/
long CDevice_DH::EDvr_GetVideoEffect(char *pstrIP, long nChID, BYTE *nBright, BYTE *nContrast, BYTE *nSaturation, BYTE *nHue, bool *bGainEnable, BYTE *nGain)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==nBright || NULL==nContrast || NULL==nSaturation || NULL==nHue || NULL==bGainEnable || NULL==nGain)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*返回结果*/
    *nBright = ChannelCfg[nChID].stColorCfg[0].byBrightness;
    *nContrast = ChannelCfg[nChID].stColorCfg[0].byContrast;
    *nSaturation = ChannelCfg[nChID].stColorCfg[0].bySaturation;
    *nHue = ChannelCfg[nChID].stColorCfg[0].byHue;
    if(!ChannelCfg[nChID].stColorCfg[0].byGainEn)
        *bGainEnable = false;
    else
        *bGainEnable = true;
    *nGain = ChannelCfg[nChID].stColorCfg[0].byGain;
    return 0;
}
/*设置通道画质*/
long CDevice_DH::EDvr_SetVideoEffect(char *pstrIP, long nChID, BYTE nBright, BYTE nContrast, BYTE nSaturation, BYTE nHue, bool bGainEnable, BYTE nGain)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(100<nBright || 100<nContrast || 100<nSaturation || 100<nHue || 100<nGain)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    ChannelCfg[nChID].stColorCfg[0].byBrightness = nBright;
    ChannelCfg[nChID].stColorCfg[0].byContrast = nContrast;
    ChannelCfg[nChID].stColorCfg[0].bySaturation = nSaturation;
    ChannelCfg[nChID].stColorCfg[0].byHue = nHue;
    ChannelCfg[nChID].stColorCfg[0].byGainEn = bGainEnable;
    ChannelCfg[nChID].stColorCfg[0].byGain = nGain;
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}
/*视频画面设置*/
long CDevice_DH::EDvr_SetImageEffect(char *pstrIP, long nChID, long nEffecType, long nEffectValue)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(100<nEffectValue)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
    {
        if( -1 == EDvr_ReLogin(pstrIP))
            return -1;
        else
            if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
                return -1;
    }
    /*修改需要设置的参数*/
    switch(nEffecType)
    {
    case 0://亮度
        ChannelCfg[nChID].stColorCfg[0].byBrightness = (BYTE)nEffectValue;
        break;
    case 1://对比度
        ChannelCfg[nChID].stColorCfg[0].byContrast = (BYTE)nEffectValue;
        break;
    case 2://饱和度
        ChannelCfg[nChID].stColorCfg[0].bySaturation = (BYTE)nEffectValue;
        break;
    case 3://色度
        ChannelCfg[nChID].stColorCfg[0].byHue = (BYTE)nEffectValue;
        break;
    case 4://增益
        ChannelCfg[nChID].stColorCfg[0].byGain = (BYTE)nEffectValue;
        break;
    default:
        break;
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG, -1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}

/*获取通道视频配置*/
long CDevice_DH::EDvr_GetChannelVideoInfo(char *pstrIP, long nChID, long nEncOpt, char *pstrChannelName, bool *bVideoEnable, BYTE *nBitRate, BYTE *nFPS, BYTE *nEncodeMode, BYTE *nImageSize, BYTE *nImageQlty)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrChannelName || NULL==bVideoEnable || NULL==nBitRate || NULL==nFPS || NULL==nEncodeMode || NULL==nImageSize || NULL==nImageQlty)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*返回结果*/
    strcpy(pstrChannelName, ChannelCfg[nChID].szChannelName);
    DH_VIDEOENC_OPT* VideoEncOpt;
    switch(nEncOpt)
    {
    case 1:
        VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+0;
        break;
    case 2:
        VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1;
        break;
    case 3:
        VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2;
        break;
    case 4:
        VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+0;
        break;
    case 5:
        VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1;
        break;
    case 6:
        VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2;
        break;
    default:return -1;
    }
    if(!VideoEncOpt->byVideoEnable)
        *bVideoEnable = false;
    else
        *bVideoEnable = true;
    *nBitRate = VideoEncOpt->byBitRateControl;
    *nFPS = VideoEncOpt->byFramesPerSec;
    *nEncodeMode = VideoEncOpt->byEncodeMode;
    *nImageSize = VideoEncOpt->byImageSize;
    *nImageQlty = VideoEncOpt->byImageQlty;
    return 0;
}
/*设置通道视频配置*/
long CDevice_DH::EDvr_SetChannelVideoInfo(char *pstrIP, long nChID, long nEncOpt, char *pstrChannelName, bool bVideoEnable, BYTE nBitRate, BYTE nFPS, BYTE nEncodeMode, BYTE nImageSize, BYTE nImageQlty)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
    {
        return -1;
    }
    /*输入参数正确*/
    if(NULL==pstrChannelName)
    {
        return -1;
    }
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
    {
        return -1;
    }
    /*修改需要设置的参数*/
    strcpy(ChannelCfg[nChID].szChannelName, pstrChannelName);
    DH_VIDEOENC_OPT* VideoEncOpt;
    switch(nEncOpt)
    {
    case 1:
        VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+0;
        break;
    case 2:
        VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1;
        break;
    case 3:
        VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2;
        break;
    case 4:
        VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+0;
        break;
    case 5:
        VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1;
        break;
    case 6:
        VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2;
        break;
    default:return -1;
    }
    VideoEncOpt->byVideoEnable = bVideoEnable;
    VideoEncOpt->byBitRateControl = nBitRate;
    VideoEncOpt->byFramesPerSec = nFPS;
    VideoEncOpt->byEncodeMode = nEncodeMode;
    VideoEncOpt->byImageSize = nImageSize;
    VideoEncOpt->byImageQlty = nImageQlty;
    
    char strTemp[4]={0};
    sprintf(strTemp,"%d",VideoEncOpt->byVideoEnable);
    
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
    {
        return -1;
    }
    return 0;
}

/*视频分辨率设置*/
long CDevice_DH::EDvr_SetImageZoom(char *pstrIP, long nChID, long nEncKind, long nImageZoom, long nZoonWidth, long nZoomHeigh, long nFPS)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    DH_VIDEOENC_OPT *pVideoEncOpt = NULL;
    switch(nEncKind)
    {
    case 0:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt;
        break;
    case 1:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1;
        break;
    case 2:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2;
        break;
    case 3:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt;
        break;
    case 4:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1;
        break;
    case 5:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2;
        break;
    default:return -1;
    }
    pVideoEncOpt->byImageSize = (BYTE)nImageZoom;
    pVideoEncOpt->byFramesPerSec = (BYTE)nFPS;
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}
/*画面清晰度设置*/
long CDevice_DH::EDvr_SetImageDefine(char *pstrIP, long nChID, long nEncKind, long nBitRateType, long nBitRate, long nImageQlty, long nEncodeMode)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    DH_VIDEOENC_OPT *pVideoEncOpt = NULL;
    switch(nEncKind)
    {
    case 0:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt;
        break;
    case 1:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1;
        break;
    case 2:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2;
        break;
    case 3:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt;
        break;
    case 4:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1;
        break;
    case 5:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2;
        break;
    default:return -1;
    }
    pVideoEncOpt->byBitRateControl = (BYTE)nBitRateType;
    pVideoEncOpt->byImageQlty = (BYTE)nImageQlty;
    pVideoEncOpt->byEncodeMode = (BYTE)nEncodeMode;
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}

long CDevice_DH::EDvr_SetChannelName(char *pstrIP, long nChType, long nChID, char *pstrName)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL == pstrName)
        return -1;
    /*得到配置信息*/
    switch(nChType)
    {
    case 0:
        {
            DWORD retlen = 0;
            DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
            if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
                return -1;
            /*修改需要设置的参数*/
            memcpy(ChannelCfg[nChID].szChannelName,pstrName,DH_CHAN_NAME_LEN);
            /*设置参数*/
            if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
                return -1;
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/*获取通道音频配置*/
long CDevice_DH::EDvr_GetChannelAudioInfo(char *pstrIP, long nChID, long nEncOpt, bool *bAudioEnable, BYTE *nFormatTag, WORD *nTrackCount, WORD *nBitsPerSample, DWORD *nSamplesPerSec)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==bAudioEnable)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*返回结果*/
    DH_VIDEOENC_OPT* pVideoEncOpt = NULL;
    switch(nEncOpt)
    {
    case 0:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt;
        break;
    case 1:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1;
        break;
    case 2:pVideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2;
        break;
    case 3:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt;
        break;
    case 4:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1;
        break;
    case 5:pVideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2;
        break;
    default:return -1;
    }
    if(!pVideoEncOpt->byAudioEnable)
        *bAudioEnable = false;
    else
        *bAudioEnable = true;
    //保留部分
    if(NULL!=nFormatTag || NULL!=nTrackCount || NULL!=nBitsPerSample || NULL!=nSamplesPerSec)
    {
        *nFormatTag = pVideoEncOpt->wFormatTag;
        *nTrackCount = pVideoEncOpt->nChannels;
        *nBitsPerSample = pVideoEncOpt->wBitsPerSample;
        *nSamplesPerSec = pVideoEncOpt->nSamplesPerSec;
    }
    return 0;
}

/*获取通道OSD配置*/
long CDevice_DH::EDvr_GetChannelOsdInfo(char *pstrIP, long nChID, long OsdOpt, DWORD *nFrontColor, DWORD *nBackColor, RECT *rcRecr, bool *bOsdShow)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==nFrontColor || NULL==nBackColor || NULL==rcRecr || NULL==bOsdShow)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*返回结果*/
    DH_ENCODE_WIDGET* OsdEncCfg;
    switch(OsdOpt)
    {
    case 1:OsdEncCfg = &(ChannelCfg[nChID].stTimeOSD);
        break;
    case 2:OsdEncCfg = &(ChannelCfg[nChID].stChannelOSD);
        break;
    case 3:OsdEncCfg = ChannelCfg[nChID].stBlindCover;
        break;
    default:return -1;
    }
    *nFrontColor = OsdEncCfg->rgbaFrontground;
    *nBackColor = OsdEncCfg->rgbaBackground;
    memcpy(rcRecr, &(OsdEncCfg->rcRect), sizeof(RECT));
    if(!OsdEncCfg->bShow)
        *bOsdShow = false;
    else
        *bOsdShow = true;
    return 0;
}
/*设置通道OSD配置*/
long CDevice_DH::EDvr_SetChannelOsdInfo(char *pstrIP, long nChID, long OsdOpt, DWORD nFrontColor, DWORD nBackColor, const RECT *rcRecr, bool bOsdShow)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    DH_ENCODE_WIDGET* OsdEncCfg;
    switch(OsdOpt)
    {
    case 1:OsdEncCfg = &(ChannelCfg[nChID].stTimeOSD);
        break;
    case 2:OsdEncCfg = &(ChannelCfg[nChID].stChannelOSD);
        break;
    case 3:OsdEncCfg = ChannelCfg[nChID].stBlindCover;
        break;
    default:return -1;
    }
    OsdEncCfg->rgbaFrontground = nFrontColor;
    OsdEncCfg->rgbaBackground = nBackColor;
    memcpy(&(OsdEncCfg->rcRect), rcRecr, sizeof(RECT));
    OsdEncCfg->bShow = bOsdShow;
    
    char strTemp[4]={0};
    sprintf(strTemp,"%d",OsdEncCfg->bShow);
    
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}

/*获取485串口协议列表*/
long CDevice_DH::EDvr_Get485PorList(char *pstrIP, DWORD *n485PorCount, char *str485PorList)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==n485PorCount || NULL==str485PorList)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*返回结果*/
    *n485PorCount = ComCfg.dwDecProListNum;
    for(DWORD i=0; i<*n485PorCount; i++)
    {
        strcpy((str485PorList+i*DH_MAX_NAME_LEN), ComCfg.DecProName[i]);
    }
    return 0;
}

/*获取232串口功能列表*/
long CDevice_DH::EDvr_Get232FuncList(char *pstrIP, DWORD *n232FuncCount, char *str232FuncList)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==n232FuncCount || NULL==str232FuncList)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*返回结果*/
    *n232FuncCount = ComCfg.dw232FuncNameNum;
    for(DWORD i=0; i<*n232FuncCount; i++)
    {
        strcpy((str232FuncList+i*DH_MAX_NAME_LEN), ComCfg.s232FuncName[i]);
    }
    return 0;
}

/*获取458串口配置*/
long CDevice_DH::EDvr_Get485Info(char *pstrIP, long nChID, long *n485Por, DWORD *nCOMInfo, BYTE *n485Address)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==n485Por || NULL==nCOMInfo || NULL==n485Address || 0>nChID || DH_MAX_CHANNUM<nChID)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*返回结果*/
    *n485Por = ComCfg.stDecoder[nChID].wProtocol;
    //*nCOMInfo = ComCfg.stDecoder[nChID].struComm;
    //DH_COMM_PROP *pCommInfo = &(ComCfg.stDecoder[nChID].struComm);
    //*nCOMInfo = *((DWORD*)pCommInfo);
    memcpy(nCOMInfo,&(ComCfg.stDecoder[nChID].struComm),sizeof(DH_COMM_PROP));
    *n485Address = (BYTE)(ComCfg.stDecoder[nChID].wDecoderAddress);
    return 0;
}
/*设置458串口配置*/
long CDevice_DH::EDvr_Set485Info(char *pstrIP, long nChID, long n485Por, DWORD nCOMInfo, BYTE n485Address)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(0>n485Por || 0>nChID || DH_MAX_CHANNUM<nChID)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    ComCfg.stDecoder[nChID].wProtocol = (BYTE)n485Por;
    //DWORD *pCommInfo = &nCOMInfo;
    //ComCfg.stDecoder[nChID].struComm = *((DH_COMM_PROP*)pCommInfo);
    memcpy(&(ComCfg.stDecoder[nChID].struComm),&nCOMInfo,sizeof(DH_COMM_PROP));
    ComCfg.stDecoder[nChID].wDecoderAddress = n485Address;
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_COMMCFG,0,&ComCfg,sizeof(DHDEV_COMM_CFG)))
        return -1;
    return 0;
}

/*获取232串口配置*/
long CDevice_DH::EDvr_Get232Info(char *pstrIP, long n232Index, long *n232Func, DWORD *nCOMInfo)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==n232Func || NULL==nCOMInfo || 0>n232Index || DH_MAX_232_NUM<n232Index)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*返回结果*/
    *n232Func = ComCfg.st232[n232Index].byFunction;
    //DH_COMM_PROP *pCommInfo = &(ComCfg.st232[n232Index].struComm);
    //*nCOMInfo = *((DWORD*)pCommInfo);
    memcpy(nCOMInfo,&(ComCfg.st232[n232Index].struComm),sizeof(DH_COMM_PROP));
    return 0;
}
/*设置232串口配置*/
long CDevice_DH::EDvr_Set232Info(char *pstrIP, long n232Index, long n232Func, DWORD nCOMInfo)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(0>n232Func || 0>n232Index || DH_MAX_232_NUM<n232Index)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    ComCfg.st232[n232Index].byFunction = (BYTE)n232Func;
    //DWORD *pCommInfo = &nCOMInfo;
    //ComCfg.st232[n232Index].struComm = *((DH_COMM_PROP*)pCommInfo);
    memcpy(&(ComCfg.st232[n232Index].struComm),&nCOMInfo,sizeof(DH_COMM_PROP));
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_COMMCFG,0,&ComCfg,sizeof(DHDEV_COMM_CFG)))
        return -1;
    return 0;
}

/*获取网络端口配置*/
long CDevice_DH::EDvr_GetNetPortInfo(char *pstrIP, char *pstrDeviceName, WORD *nTCPCount, WORD *nTCPPort, WORD *nUDPPort, WORD *nHTTPPort, WORD *HTTPSPort, WORD *nSSLPort)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrDeviceName || NULL==nTCPCount || NULL==nTCPPort || NULL==nUDPPort || NULL==nHTTPPort ||
        NULL==HTTPSPort || NULL==nSSLPort)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*返回结果*/
    strcpy(pstrDeviceName, NetCfg.sDevName);
    *nTCPCount = NetCfg.wTcpMaxConnectNum;
    *nTCPPort = NetCfg.wTcpPort;
    *nUDPPort = NetCfg.wUdpPort;
    *nHTTPPort = NetCfg.wHttpPort;
    *HTTPSPort = NetCfg.wHttpsPort;
    *nSSLPort = NetCfg.wSslPort;
    return 0;
}
/*设置网络端口配置*/
long CDevice_DH::EDvr_SetNetPortInfo(char *pstrIP, const char *pstrDeviceName, WORD nTCPCount, WORD nTCPPort, WORD nUDPPort, WORD nHTTPPort, WORD HTTPSPort, WORD nSSLPort)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL == pstrDeviceName)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    strcpy(NetCfg.sDevName, pstrDeviceName);
    NetCfg.wTcpMaxConnectNum = nTCPCount;
    NetCfg.wTcpPort = nTCPPort;
    NetCfg.wUdpPort = nUDPPort;
    NetCfg.wHttpPort = nHTTPPort;
    NetCfg.wHttpsPort = HTTPSPort;
    NetCfg.wSslPort = nSSLPort;
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*获取以太网口配置*/
long CDevice_DH::EDvr_GetEthernetInfo(char *pstrIP, long nEthernetIndex, char *pstrEthernetIP, char *EthernetMask, char *GatewayIP, long *nNetMode, char *pstrMAC)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrEthernetIP || NULL==EthernetMask || NULL==GatewayIP || NULL==nNetMode || NULL==pstrMAC ||
        0>nEthernetIndex || DH_MAX_ETHERNET_NUM<nEthernetIndex)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*返回结果*/
    strcpy(pstrEthernetIP, NetCfg.stEtherNet[nEthernetIndex].sDevIPAddr);
    strcpy(EthernetMask, NetCfg.stEtherNet[nEthernetIndex].sDevIPMask);
    strcpy(GatewayIP, NetCfg.stEtherNet[nEthernetIndex].sGatewayIP);
    *nNetMode = (long)NetCfg.stEtherNet[nEthernetIndex].dwNetInterface;
    strcpy(pstrMAC, NetCfg.stEtherNet[nEthernetIndex].byMACAddr);
    return 0;
}
/*设置以太网口配置*/
long CDevice_DH::EDvr_SetEthernetInfo(char *pstrIP, long nEthernetIndex, const char *pstrEthernetIP, const char *EthernetMask, const char *GatewayIP, long nNetMode)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrEthernetIP || NULL==EthernetMask || NULL==GatewayIP || 0>=nNetMode || 0>nEthernetIndex ||
        DH_MAX_ETHERNET_NUM<nEthernetIndex)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    strcpy(NetCfg.stEtherNet[nEthernetIndex].sDevIPAddr, pstrEthernetIP);
    strcpy(NetCfg.stEtherNet[nEthernetIndex].sDevIPMask, EthernetMask);
    strcpy(NetCfg.stEtherNet[nEthernetIndex].sGatewayIP, GatewayIP);
    NetCfg.stEtherNet[nEthernetIndex].dwNetInterface = (BYTE)nNetMode;
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*获取远程服务器配置*/
long CDevice_DH::EDvr_GetRemoteHostInfo(char *pstrIP, long nHostType, char *pstrHostIP, WORD *nHostPort,bool *bIsEnable,
                                        char *pstrUserName, char *pstrPassWord, char *pstrHostName)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrHostIP || NULL==nHostPort || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrHostName ||
        NULL==bIsEnable)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*返回结果*/
    DH_REMOTE_HOST HostInfo;
    switch(nHostType)
    {
    case 1:
        memcpy(&HostInfo, &(NetCfg.struAlarmHost),sizeof(DH_REMOTE_HOST));
        break;
    case 2:
        memcpy(&HostInfo, &(NetCfg.struLogHost),sizeof(DH_REMOTE_HOST));
        break;
    case 3:
        memcpy(&HostInfo, &(NetCfg.struSmtpHost),sizeof(DH_REMOTE_HOST));
        break;
    case 4:
        memcpy(&HostInfo, &(NetCfg.struMultiCast),sizeof(DH_REMOTE_HOST));
        break;
    case 5:
        memcpy(&HostInfo, &(NetCfg.struNfs),sizeof(DH_REMOTE_HOST));
        break;
        //	case 6:
        //		memcpy(&HostInfo, &(NetCfg.struFtpServer),sizeof(DH_REMOTE_HOST));
        //		break;
    case 7:
        memcpy(&HostInfo, &(NetCfg.struPppoe),sizeof(DH_REMOTE_HOST));
        strcpy(pstrHostName, NetCfg.sPppoeIP);
        break;
    case 8:
        memcpy(&HostInfo, &(NetCfg.struDdns),sizeof(DH_REMOTE_HOST));
        strcpy(pstrHostName, NetCfg.sDdnsHostName);
        break;
    case 9:
        memcpy(&HostInfo, &(NetCfg.struDns),sizeof(DH_REMOTE_HOST));
        break;
    default:
        return -1;
    }
    strcpy(pstrHostIP,HostInfo.sHostIPAddr);
    *nHostPort = HostInfo.wHostPort;
    if(!HostInfo.byEnable)
        *bIsEnable = false;
    else
        *bIsEnable = true;
    strcpy(pstrUserName, HostInfo.sHostUser);
    strcpy(pstrPassWord, HostInfo.sHostPassword);
    return 0;
}
/*设置远程服务器配置*/
long CDevice_DH::EDvr_SetRemoteHostInfo(char *pstrIP, long nHostType, const char *pstrHostIP, WORD nHostPort,
                                        bool bIsEnable, const char *pstrUserName, const char *pstrPassWord, const char *pstrHostName)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrHostIP || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrHostName)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    DH_REMOTE_HOST HostInfo;
    HostInfo.byEnable = bIsEnable;
    strcpy(HostInfo.sHostIPAddr, pstrHostIP);
    HostInfo.wHostPort = nHostPort;
    strcpy(HostInfo.sHostUser, pstrUserName);
    strcpy(HostInfo.sHostPassword, pstrPassWord);
    switch(nHostType)
    {
    case 1:
        memcpy(&(NetCfg.struAlarmHost),&HostInfo, sizeof(DH_REMOTE_HOST));
        break;
    case 2:
        memcpy(&(NetCfg.struLogHost),&HostInfo, sizeof(DH_REMOTE_HOST));
        break;
    case 3:
        memcpy(&(NetCfg.struSmtpHost),&HostInfo, sizeof(DH_REMOTE_HOST));
        break;
    case 4:
        memcpy(&(NetCfg.struMultiCast),&HostInfo, sizeof(DH_REMOTE_HOST));
        break;
    case 5:
        memcpy(&(NetCfg.struNfs),&HostInfo, sizeof(DH_REMOTE_HOST));
        break;
        //	case 6:
        //		memcpy(&(NetCfg.struFtpServer),&HostInfo, sizeof(DH_REMOTE_HOST));
        //		break;
    case 7:
        memcpy(&(NetCfg.struPppoe),&HostInfo, sizeof(DH_REMOTE_HOST));
        strcpy(NetCfg.sPppoeIP, pstrHostName);
        break;
    case 8:
        memcpy(&(NetCfg.struDdns),&HostInfo, sizeof(DH_REMOTE_HOST));
        strcpy(NetCfg.sDdnsHostName, pstrHostName);
        break;
    case 9:
        memcpy(&(NetCfg.struDns),&HostInfo, sizeof(DH_REMOTE_HOST));
        break;
    default:
        return -1;
    }
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*获取邮件服务器配置*/
long CDevice_DH::EDvr_GetMailHostInfo(char *pstrIP, char *pstrHostIP, WORD *nHostPort, char *pstrUserName,
                                      char *pstrPassWord, char *pstrDestAddr, char *pstrCcAddr, char *pstrBccAddr, char *pstrSubject)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrHostIP || NULL==nHostPort || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrDestAddr ||
        NULL==pstrCcAddr || NULL==pstrBccAddr || NULL==pstrSubject)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*返回结果*/
    strcpy(pstrHostIP, NetCfg.struMail.sMailIPAddr);
    *nHostPort = NetCfg.struMail.wMailPort;
    strcpy(pstrUserName, NetCfg.struMail.sUserName);
    strcpy(pstrPassWord, NetCfg.struMail.sUserPsw);
    strcpy(pstrDestAddr, NetCfg.struMail.sDestAddr);
    strcpy(pstrCcAddr, NetCfg.struMail.sCcAddr);
    strcpy(pstrBccAddr, NetCfg.struMail.sBccAddr);
    strcpy(pstrSubject, NetCfg.struMail.sSubject);
    return 0;
}
/*设置邮件服务器配置*/
long CDevice_DH::EDvr_SetMailHostInfo(char *pstrIP,char *pstrHostIP,WORD nHostPort,const char *pstrUserName,
                                      const char *pstrPassWord,const char *pstrDestAddr,const char *pstrCcAddr,
                                      const char *pstrBccAddr,const char *pstrSubject)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrHostIP || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrDestAddr || NULL==pstrCcAddr ||
        NULL==pstrBccAddr || NULL==pstrSubject)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*修改需要设置的参数*/
    strcpy(NetCfg.struMail.sMailIPAddr, pstrHostIP);
    NetCfg.struMail.wMailPort = nHostPort;
    strcpy(NetCfg.struMail.sUserName, pstrUserName);
    strcpy(NetCfg.struMail.sUserPsw, pstrPassWord);
    strcpy(NetCfg.struMail.sDestAddr, pstrDestAddr);
    strcpy(NetCfg.struMail.sCcAddr, pstrCcAddr);
    strcpy(NetCfg.struMail.sBccAddr, pstrBccAddr);
    strcpy(NetCfg.struMail.sSubject, pstrSubject);
    /*设置配置*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*获取设备属性配置//视频口,音频口,报警输入,报警输出,网络口,USB口,IDE口,串口,并口*/
long CDevice_DH::EDvr_GetDeviceAttribute(char *pstrIP, BYTE *pDeviceAttributes, long *pnAttributeSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pDeviceAttributes || NULL==pnAttributeSize)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_SYSTEM_ATTR_CFG DeviceCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_DEVICECFG, 0, &DeviceCfg,sizeof(DHDEV_SYSTEM_ATTR_CFG),&retlen))
        return -1;
    /*返回结果*/
    pDeviceAttributes[0] = DeviceCfg.byVideoCaptureNum;
    pDeviceAttributes[1] = DeviceCfg.byAudioCaptureNum;
    pDeviceAttributes[2] = DeviceCfg.byAlarmInNum;
    pDeviceAttributes[3] = DeviceCfg.byAlarmOutNum;
    pDeviceAttributes[4] = DeviceCfg.byNetIONum;
    pDeviceAttributes[5] = DeviceCfg.byUsbIONum;
    pDeviceAttributes[6] = DeviceCfg.byIdeControlNum;
    pDeviceAttributes[7] = DeviceCfg.byComIONum;
    pDeviceAttributes[8] = DeviceCfg.byLPTIONum;
    pnAttributeSize[0] = 9;
    return 0;
}

/*捕获I帧*/
long CDevice_DH::EDvr_CaptureIFrame(char *pstrIP, long nChID)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
        /*
        //判断数据连接通道是否打开
        SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,nChID);
        if(sChannel == NULL)
        return -1;
        //判断是否为即时数据连接
        if(sChannel->m_nChannelType != 1)
        return -1;
    */
    //强制I帧
    if(!CLIENT_MakeKeyFrame(temp->m_nHandle,nChID))
        return -1;
    return 0;
}

/*获取用户权限列表*/
long CDevice_DH::EDvr_GetUserRightList(char *pstrIP, fUserRightCB lpFunCB)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL == lpFunCB)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*返回结果*/
    for(unsigned i=0; i<ManageInfo.dwRightNum; i++)
    {
        lpFunCB(pstrIP, ManageInfo.rightList[i].dwID, ManageInfo.rightList[i].name, ManageInfo.rightList[i].memo);
    }
    return 0;
}

/*用户组信息查找*/
long CDevice_DH::EDvr_GetUserGroupList(char *pstrIP, fUserGroupCB lpFunCB)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL == lpFunCB)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*返回结果*/
    for(unsigned i=0; i<ManageInfo.dwGroupNum; i++)
    {
        lpFunCB(pstrIP, ManageInfo.groupList[i].dwID, ManageInfo.groupList[i].name,ManageInfo.groupList[i].dwRightNum
            ,(long*)(ManageInfo.groupList[i].rights), ManageInfo.groupList[i].memo);
    }
    return 0;
}
long CDevice_DH::EDvr_GetUserGroup(char *pstrIP, char *pstrGroupName, long *nGroupID, long *nRightNum, long *pnRightList, char *pstrGroupMemo)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrGroupName || NULL==nGroupID || NULL==nRightNum || NULL==pnRightList || NULL==pstrGroupMemo)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*返回结果*/
    for(unsigned i=0; i<ManageInfo.dwGroupNum; i++)
    {
        if(!strcmp(ManageInfo.groupList[i].name,pstrGroupName))
        {
            *nGroupID = ManageInfo.groupList[i].dwID;
            *nRightNum = ManageInfo.groupList[i].dwRightNum;
            memcpy(pnRightList,ManageInfo.groupList[i].rights,sizeof(long)*ManageInfo.groupList[i].dwRightNum);
            strcpy(pstrGroupMemo,ManageInfo.groupList[i].memo);
            return 0;
        }
    }
    return -1;
}

/*获取用户信息*/
long CDevice_DH::EDvr_GetUserInfoList(char *pstrIP, fUserInfoCB lpFunCB)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL == lpFunCB)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*返回结果*/
    for(unsigned i=0; i<ManageInfo.dwUserNum; i++)
    {
        lpFunCB(pstrIP, ManageInfo.userList[i].dwID, ManageInfo.userList[i].dwGroupID, ManageInfo.userList[i].name
            , ManageInfo.userList[i].passWord, ManageInfo.userList[i].dwRightNum, (long*)(ManageInfo.userList[i].rights)
            , ManageInfo.userList[i].memo);
    }
    return 0;
}
long CDevice_DH::EDvr_GetUserInfo(char *pstrIP, char *pstrUserName, long *nUserID, long *nGroupID, char *pstrPassword, long *nRightNum, long *pnRightList, char *pstrUserMemo)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输出参数正确*/
    if(NULL==pstrUserName || NULL==nUserID || NULL==nGroupID || NULL==pstrPassword || NULL==nRightNum || NULL==pnRightList || NULL==pstrUserMemo)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*返回结果*/
    for(unsigned i=0; i<ManageInfo.dwUserNum; i++)
    {
        if(!strcmp(ManageInfo.userList[i].name,pstrUserName))
        {
            *nUserID = ManageInfo.userList[i].dwID;
            *nGroupID = ManageInfo.userList[i].dwGroupID;
            strcpy(pstrPassword, ManageInfo.userList[i].passWord);
            *nRightNum = ManageInfo.userList[i].dwRightNum;
            memcpy(pnRightList, ManageInfo.userList[i].rights, sizeof(long)*ManageInfo.userList[i].dwRightNum);
            strcpy(pstrUserMemo, ManageInfo.userList[i].memo);
            return 0;
        }
    }
    return -1;
}

/*增加用户组*/
long CDevice_DH::EDvr_AddUserGroup(char *pstrIP, long nGroupID, char *pstrGroupName, long nRightNum, long *pnRightList, char *pstrGroupMemo)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*记录输入参数*/
    USER_GROUP_INFO GroupInfo;
    memset(&GroupInfo,0,sizeof(USER_GROUP_INFO));
    GroupInfo.dwID = nGroupID;
    GroupInfo.dwRightNum = nRightNum;
    strcpy(GroupInfo.name,pstrGroupName);
    strcpy(GroupInfo.memo,pstrGroupMemo);
    memcpy(GroupInfo.rights,pnRightList,sizeof(long)*nRightNum);
    /*添加用户组*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,0,&GroupInfo,NULL))
        return -1;
    return 0;
}

/*增加用户*/
long CDevice_DH::EDvr_AddUser(char *pstrIP,long nUserID,char *pstrUserName,long nGroupID,char *pstrPassword,long nRightNum,long *pnRightList,char *pstrUserMemo)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*记录输入参数*/
    USER_INFO newUserInfo;
    memset(&newUserInfo,0,sizeof(USER_INFO));
    newUserInfo.dwID = nUserID;
    newUserInfo.dwGroupID = nGroupID;
    newUserInfo.dwRightNum = nRightNum;
    strcpy(newUserInfo.name, pstrUserName);
    strcpy(newUserInfo.passWord, pstrPassword);
    strcpy(newUserInfo.memo, pstrUserMemo);
    memcpy(newUserInfo.rights, pnRightList,sizeof(long)*nRightNum);
    /*添加用户组*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,3,&newUserInfo,NULL))
        return -1;
    return 0;
}

/*删除用户组*/
long CDevice_DH::EDvr_DeletUserGroup(char *pstrIP, char *pstrGroupName)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*记录输入参数*/
    USER_GROUP_INFO GroupInfo;
    strcpy(GroupInfo.name,pstrGroupName);
    /*删除用户组*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,1,&GroupInfo,NULL))
        return -1;
    return 0;
}

/*删除用户*/
long CDevice_DH::EDvr_DeleteUser(char *pstrIP, char *pstrUserName, char *pstrPassword)
{
    /*设备是否已登陆*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrUserName)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    for(i=0; i<ManageInfo.dwUserNum; i++)
    {
        if(!strcmp(ManageInfo.userList[i].name,pstrUserName))
        {
            break;
        }
    }
    if(i >= ManageInfo.dwUserNum)
        return -1;
    /*删除用户*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,4,&ManageInfo,NULL))
        return -1;
    return 0;
}

/*编辑用户组*/
long CDevice_DH::EDvr_EditUserGroup(char *pstrIP, char *pstrGroupName, long nGroupID, char *pstrNewName, long nRightNum, long *pnRightList, char *pstrGroupMemo)
{
    /*设备是否已登陆*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrGroupName)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    for(i=0; i<ManageInfo.dwGroupNum; i++)
    {
        if(!strcmp(ManageInfo.groupList[i].name,pstrGroupName))
        {
            break;
        }
    }
    if(i >= ManageInfo.dwGroupNum)
        return -1;
    /*修改用户组*/
    USER_GROUP_INFO GroupInfo;
    memcpy(&GroupInfo,ManageInfo.groupList+i,sizeof(USER_GROUP_INFO));
    GroupInfo.dwID = nGroupID;
    strcpy(GroupInfo.name,pstrNewName);
    strcpy(GroupInfo.memo,pstrGroupMemo);
    if(NULL != pnRightList)
    {
        GroupInfo.dwRightNum = nRightNum;
        memset(GroupInfo.rights,0,sizeof(long)*DH_MAX_RIGHT_NUM);
        memcpy(GroupInfo.rights,pnRightList,sizeof(long)*nRightNum);
    }
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,2,&GroupInfo,ManageInfo.groupList+i))
        return -1;
    return 0;
}

/*编辑用户*/
long CDevice_DH::EDvr_EditUser(char *pstrIP, char *pstrUserName, long nUserID, char *pstrNewName, long nGroupID, char *pstrPassword, long nRightNum, long *pnRightList, char *pstrUserMemo)
{
    /*设备是否已登陆*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrUserName)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    for(i=0; i<ManageInfo.dwUserNum; i++)
    {
        if(!strcmp(ManageInfo.userList[i].name,pstrUserName))
        {
            break;
        }
    }
    if(i >= ManageInfo.dwUserNum)
        return -1;
    /*修改用户组*/
    USER_INFO newUserInfo;
    memcpy(&newUserInfo, ManageInfo.userList+i,sizeof(USER_INFO));
    newUserInfo.dwID = nUserID;
    newUserInfo.dwGroupID = nGroupID;
    strcpy(newUserInfo.name, pstrNewName);
    strcpy(newUserInfo.memo, pstrUserMemo);
    if(NULL != pnRightList)
    {
        newUserInfo.dwRightNum = nRightNum;
        memset(newUserInfo.rights,0,sizeof(long)*DH_MAX_RIGHT_NUM);
        memcpy(newUserInfo.rights,pnRightList,sizeof(long)*nRightNum);
    }
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,5,&newUserInfo,ManageInfo.userList+i))
        return -1;
    return 0;
}

long CDevice_DH::EDvr_ModifyUserPassword(char *pstrIP, char *pstrUserName, char *pstrOldPassword, char *pstrNewPassword)
{
    /*设备是否已登陆*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*输入参数正确*/
    if(NULL==pstrUserName)
        return -1;
    /*得到配置信息*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    for(i=0; i<ManageInfo.dwUserNum; i++)
    {
        if(!strcmp(ManageInfo.userList[i].name,pstrUserName))
        {
            break;
        }
    }
    if(i >= ManageInfo.dwUserNum)
        return -1;
    /*填充旧密码*/
    strcpy(ManageInfo.userList[i].passWord, pstrOldPassword);
    /*修改密码*/
    USER_INFO newUserInfo;
    memcpy(&newUserInfo, ManageInfo.userList+i,sizeof(USER_INFO));
    strcpy(newUserInfo.passWord, pstrNewPassword);
    
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,6,&newUserInfo,ManageInfo.userList+i))
        return -1;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 报警接口详细分类封装

//报警常规配置
long CDevice_DH::EDvr_GetAlarmNormalInfo(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nAlarmEnable, BYTE *nValue1, BYTE *nValue2,long *nValue3)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || nAlarmEnable == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    int i;
    switch(nAlarmType)
    {
    case 0://0信号报警
        *nAlarmEnable = almCFG->struLocalAlmIn[nChID].byAlarmEn;
        *nValue1 = (BYTE)(almCFG->struLocalAlmIn[nChID].byAlarmType?0:1);
        break;
    case 1://1移动侦测
        *nAlarmEnable = almCFG->struMotion[nChID].byMotionEn;
        *nValue1 = (BYTE)(almCFG->struMotion[nChID].wSenseLevel);
        *nValue3 = 18*22;
        for(i=0; i<18; i++)
        {
            memcpy(nValue2+(i*22),almCFG->struMotion[nChID].byDetected[i],22);
        }
        break;
    case 2://2视频丢失
        *nAlarmEnable = almCFG->struVideoLost[nChID].byAlarmEn;
        break;
    case 3://3视频遮挡
        *nAlarmEnable = almCFG->struBlind[nChID].byBlindEnable;
        *nValue1 = almCFG->struBlind[nChID].byBlindLevel;
        break;
    case 4://4无磁盘报警
        *nAlarmEnable = almCFG->struDiskAlarm.byNoDiskEn;
        break;
    case 5://5磁盘满报警
        *nAlarmEnable = almCFG->struDiskAlarm.byLowCapEn;
        break;
    case 6://6磁盘故障报警
        *nAlarmEnable = almCFG->struDiskAlarm.byNoDiskEn;
        break;
    default:break;
    }
    return 0;
}

//报警联动输出配置
long CDevice_DH::EDvr_GetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nOutEnable, BYTE *pAlarmOut, long *nAlarmOutSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmOut == NULL || nAlarmOutSize == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    switch(nAlarmType)
    {
    case 0://0信号报警
        *nOutEnable = (BYTE)((almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struLocalAlmIn[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 1://1移动侦测
        *nOutEnable = (BYTE)((almCFG->struMotion[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struMotion[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 2://2视频丢失
        *nOutEnable = (BYTE)((almCFG->struVideoLost[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struVideoLost[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 3://3视频遮挡
        *nOutEnable = (BYTE)((almCFG->struBlind[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struBlind[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 4://4无磁盘报警
        *nOutEnable = (BYTE)((almCFG->struDiskAlarm.struNDHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struDiskAlarm.struNDHandle.byRelAlarmOut,16);
        break;
    case 5://5磁盘满报警
        *nOutEnable = (BYTE)((almCFG->struDiskAlarm.struLCHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struDiskAlarm.struLCHandle.byRelAlarmOut,16);
        break;
    case 6://6磁盘故障报警
        *nOutEnable = (BYTE)((almCFG->struDiskAlarm.struEDHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struDiskAlarm.struEDHandle.byRelAlarmOut,16);
        break;
    default:break;
    }
    *nAlarmOutSize = 16;
    return 0;
}
long CDevice_DH::EDvr_SetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nOutEnable, BYTE *pAlarmOut, long nAlarmOutSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmOut == NULL || nAlarmOutSize > 16)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    switch(nAlarmType)
    {
    case 0://0信号报警
        BitSet(almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struLocalAlmIn[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 1://1移动侦测
        BitSet(almCFG->struMotion[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struMotion[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 2://2视频丢失
        BitSet(almCFG->struVideoLost[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struVideoLost[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 3://3视频遮挡
        BitSet(almCFG->struBlind[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struBlind[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 4://4无磁盘报警
        BitSet(almCFG->struDiskAlarm.struNDHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struDiskAlarm.struNDHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 5://5磁盘满报警
        BitSet(almCFG->struDiskAlarm.struLCHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struDiskAlarm.struLCHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 6://6磁盘故障报警
        BitSet(almCFG->struDiskAlarm.struEDHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struLocalAlmIn[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    default:break;
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

//报警联动录像配置
long CDevice_DH::EDvr_GetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nRecordEnable, long *nPreRecLen, BYTE *pAlarmRecord, long *nAlarmRecordSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmRecord == NULL || nAlarmRecordSize == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    switch(nAlarmType)
    {
    case 0://0信号报警
        *nRecordEnable = (BYTE)((almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struLocalAlmIn[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struLocalAlmIn[nChID].struHandle.dwRecLatch;
        break;
    case 1://1移动侦测
        *nRecordEnable = (BYTE)((almCFG->struMotion[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struMotion[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struMotion[nChID].struHandle.dwRecLatch;
        break;
    case 2://2视频丢失
        *nRecordEnable = (BYTE)((almCFG->struVideoLost[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struVideoLost[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struVideoLost[nChID].struHandle.dwRecLatch;
        break;
    case 3://3视频遮挡
        *nRecordEnable = (BYTE)((almCFG->struBlind[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struBlind[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struBlind[nChID].struHandle.dwRecLatch;
        break;
    case 4://4无磁盘报警
        *nRecordEnable = (BYTE)((almCFG->struDiskAlarm.struNDHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struDiskAlarm.struNDHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struDiskAlarm.struNDHandle.dwRecLatch;
        break;
    case 5://5磁盘满报警
        *nRecordEnable = (BYTE)((almCFG->struDiskAlarm.struLCHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struDiskAlarm.struLCHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struDiskAlarm.struLCHandle.dwRecLatch;
        break;
    case 6://6磁盘故障报警
        *nRecordEnable = (BYTE)((almCFG->struDiskAlarm.struEDHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struDiskAlarm.struEDHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struDiskAlarm.struEDHandle.dwRecLatch;
        break;
    default:break;
    }
    *nAlarmRecordSize = 16;
    return 0;
}
long CDevice_DH::EDvr_SetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nRecordEnable, long nPreRecLen, BYTE *pAlarmRecord, long nAlarmRecordSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmRecord == NULL || nAlarmRecordSize > 16)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    switch(nAlarmType)
    {
    case 0://0信号报警
        BitSet(almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struLocalAlmIn[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struLocalAlmIn[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 1://1移动侦测
        BitSet(almCFG->struMotion[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struMotion[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struMotion[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 2://2视频丢失
        BitSet(almCFG->struVideoLost[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struVideoLost[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struVideoLost[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 3://3视频遮挡
        BitSet(almCFG->struBlind[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struBlind[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struBlind[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 4://4无磁盘报警
        BitSet(almCFG->struDiskAlarm.struNDHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struDiskAlarm.struNDHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struDiskAlarm.struNDHandle.dwRecLatch = nPreRecLen;
        break;
    case 5://5磁盘满报警
        BitSet(almCFG->struDiskAlarm.struLCHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struDiskAlarm.struLCHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struDiskAlarm.struLCHandle.dwRecLatch = nPreRecLen;
        break;
    case 6://6磁盘故障报警
        BitSet(almCFG->struDiskAlarm.struEDHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struDiskAlarm.struEDHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struDiskAlarm.struEDHandle.dwRecLatch = nPreRecLen;
        break;
    default:break;
    }
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

//报警联动云台
long CDevice_DH::EDvr_GetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long *nPTZSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || nPTZNo == NULL || nPTZNo == NULL || nPTZSize ==NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    DH_MSG_HANDLE* pMsgHande = NULL;
    switch(nAlarmType)
    {
    case 0://0信号报警
        pMsgHande = &(almCFG->struLocalAlmIn[nChID].struHandle);
        break;
    case 1://1移动侦测
        pMsgHande = &(almCFG->struMotion[nChID].struHandle);
        break;
    case 2://2视频丢失
        pMsgHande = &(almCFG->struVideoLost[nChID].struHandle);
        break;
    case 3://3视频遮挡
        pMsgHande = &(almCFG->struBlind[nChID].struHandle);
        break;
    case 4://4无磁盘报警
        pMsgHande = &(almCFG->struDiskAlarm.struNDHandle);
        break;
    case 5://5磁盘满报警
        pMsgHande = &(almCFG->struDiskAlarm.struLCHandle);
        break;
    case 6://6磁盘故障报警
        pMsgHande = &(almCFG->struDiskAlarm.struEDHandle);
        break;
    default:break;
    }
    if(pMsgHande == NULL)
        return -1;
    *nPTZEnable = (BYTE)((pMsgHande->dwActionFlag & 0x00000004)?1:0);
    for(int i=0; i<16; i++)
    {
        nPTZType[i] = (BYTE)pMsgHande->struPtzLink[i].iType;
        nPTZNo[i] = (BYTE)pMsgHande->struPtzLink[i].iValue;
    }
    *nPTZSize = 16;
    return 0;
}
long CDevice_DH::EDvr_SetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long nPTZSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || nPTZType == NULL || nPTZNo == NULL || nPTZSize > 16)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*修改需要设置的参数*/
    DH_MSG_HANDLE* pMsgHande = NULL;
    switch(nAlarmType)
    {
    case 0://0信号报警
        pMsgHande = &(almCFG->struLocalAlmIn[nChID].struHandle);
        break;
    case 1://1移动侦测
        pMsgHande = &(almCFG->struMotion[nChID].struHandle);
        break;
    case 2://2视频丢失
        pMsgHande = &(almCFG->struVideoLost[nChID].struHandle);
        break;
    case 3://3视频遮挡
        pMsgHande = &(almCFG->struBlind[nChID].struHandle);
        break;
    case 4://4无磁盘报警
        pMsgHande = &(almCFG->struDiskAlarm.struNDHandle);
        break;
    case 5://5磁盘满报警
        pMsgHande = &(almCFG->struDiskAlarm.struLCHandle);
        break;
    case 6://6磁盘故障报警
        pMsgHande = &(almCFG->struDiskAlarm.struEDHandle);
        break;
    default:break;
    }
    if(pMsgHande == NULL)
        return -1;
    BitSet(pMsgHande->dwActionFlag,3,nPTZEnable);
    for(int i=0; i<nPTZSize; i++)
    {
        pMsgHande->struPtzLink[i].iType = nPTZType[i];
        pMsgHande->struPtzLink[i].iValue = nPTZNo[i];
    }
    
    /*设置参数*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}


//报警联动其他配置开关
long CDevice_DH::EDvr_GetAlarmOtherEn(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *pOtherEnable, long *nOtherSize)
{
    /*设备是否已登陆*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*判断报警输入号正确*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pOtherEnable == NULL || nOtherSize == NULL)
        return -1;
    /*得到配置信息*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*返回配置结果*/
    DH_MSG_HANDLE *pTempHandle = NULL;
    switch(nAlarmType)
    {
    case 0://0信号报警
        pTempHandle = &(almCFG->struLocalAlmIn[nChID].struHandle);
        break;
    case 1://1移动侦测
        pTempHandle = &(almCFG->struMotion[nChID].struHandle);
        break;
    case 2://2视频丢失
        pTempHandle = &(almCFG->struVideoLost[nChID].struHandle);
        break;
    case 3://3视频遮挡
        pTempHandle = &(almCFG->struBlind[nChID].struHandle);
        break;
    case 4://4无磁盘报警
        pTempHandle = &(almCFG->struDiskAlarm.struNDHandle);
        break;
    case 5://5磁盘满报警
        pTempHandle = &(almCFG->struDiskAlarm.struLCHandle);
        break;
    case 6://6磁盘故障报警
        pTempHandle = &(almCFG->struDiskAlarm.struEDHandle);
        break;
    default:return -1;
    }
    pOtherEnable[0] = (BYTE)((pTempHandle->dwActionFlag & 0x00000001)?1:0);//上传服务器
    pOtherEnable[1] = (BYTE)((pTempHandle->dwActionFlag & 0x00000008)?1:0);//发送邮件
    pOtherEnable[2] = (BYTE)((pTempHandle->dwActionFlag & 0x00000010)?1:0);//报警轮巡
    pOtherEnable[3] = (BYTE)((pTempHandle->dwActionFlag & 0x00000020)?1:0);//设备提示
    *nOtherSize = 4;
    return 0;
}

void CDevice_DH::EDvr_SetDrawWndCallBackFunc(fDrawWndCB CallBackFunc)
{
    m_pDrawWndCB = CallBackFunc;
}

void CDevice_DH::EDvr_OpenDrawWnd()
{
    CLIENT_RigisterDrawFun(DrawWndCallBack,(unsigned long)this);
}

void CDevice_DH::DrawWndCallBack(long lLoginID, long lRealHandle, HDC hDC, DWORD dwUser)
{
    if (!dwUser) return;
    
    CDevice_DH * device = (CDevice_DH *)dwUser;
    if(NULL == device->m_pDrawWndCB) return;
    
    SDeviceInfo_DH *pDv = device->m_DeviceList.SearchDeviceByLogInID(lLoginID);
    if(NULL == pDv) return;
    
    long nChID = device->m_ChannelList.FixHandle(lRealHandle);
    
    device->m_pDrawWndCB(inet_ntoa(*(in_addr*)&(pDv->m_dwIP)),nChID,hDC,0);
}

//远程文件播放、下载进度POS回调
void __stdcall CDevice_DH::FileDownloadPosCallBack(long lPlayBackRet, DWORD dwTotalSize, DWORD dwDownLoadSize, DWORD dwUser)
{
    if(!dwUser) return;
    CDevice_DH* pThis = (CDevice_DH*)dwUser;
    //TRACE("hDownload:%d\n", lPlayBackRet);
    SChannelInfo_DH * pTemp = pThis->m_ChannelList.GetChannel(pThis->m_ChannelList.FixHandle(lPlayBackRet));
    if(!pTemp) return;
}

//暂停回放
long CDevice_DH::EDvr_PausePlayBack(HWND hWnd, long bPause)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_PausePlayBack(pTemp->m_nHandle,bPause))
        return -1;
    return 0;
}

//单帧播放
long CDevice_DH::EDvr_OneFramePlayBack(HWND hWnd, long bOneFrame)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_StepPlayBack(pTemp->m_nHandle,(bOneFrame?0:1)))
        return -1;
    return 0;
}

//设置播放位置
long CDevice_DH::EDvr_SetPosPlayBack(HWND hWnd, long nPlayTime)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_SeekPlayBack(pTemp->m_nHandle, 0xffffffff, (DWORD)nPlayTime))
        return -1;
    return 0;
}

//快放
long CDevice_DH::EDvr_FastPlayBack(HWND hWnd)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_FastPlayBack(pTemp->m_nHandle))
        return -1;
    return 0;
}

//慢放
long CDevice_DH::EDvr_SlowPlayBack(HWND hWnd)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_SlowPlayBack(pTemp->m_nHandle))
        return -1;
    return 0;
}

//抓图
long CDevice_DH::EDvr_ConvertPicture(HWND hWnd, char *pstrFileName)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
    /*判断数据连接通道是否打开*/
    if(sChannel == NULL)
        return -1;
    /*开始截图*/
    if(!CLIENT_CapturePicture(sChannel->m_nHandle, pstrFileName))
        return -1;
    return 0;
}

//////////////////////////////////////////////////////////////////////
// 主系统类::内部接口
//////////////////////////////////////////////////////////////////////

/*停止某设备的所有数据连接*/
void CDevice_DH::EDvr_StopDeviceDataLink(char* strIP)
{
    DWORD dwIP = inet_addr(strIP);
    for(long i=0; i < m_ChannelList.GetArraySize(); i++)
    {
        if(NULL != m_ChannelList.GetChannel(i)){
            	if(m_ChannelList.GetChannel(i)->m_dwIP == dwIP)
		{
                if(m_ChannelList.GetChannel(i)->m_nID <= DH_MAX_CHANNUM)
                    EDvr_StopDataLink(i);
                else
                    EDvr_StopMultiLink(i);
		}
	}
    }
}

long CDevice_DH::EDvr_ReLogin(long nLoginID)
{
    if( -1 == nLoginID ) return -1;
    SDeviceInfo_DH* theDev = m_DeviceList.GetDevice(nLoginID);
    if(NULL == theDev)
        return -1;
    WriteLog(g_szFlag, "设备重登录 Begin IP:0x%x", theDev->m_dwIP);
    //执行注销操作
    CLIENT_Logout(theDev->m_nHandle);
    //执行注册操作
    int nError;
    NET_DEVICEINFO tmpDev;
    theDev->m_nHandle = CLIENT_Login(inet_ntoa(*(in_addr*)&(theDev->m_dwIP)),
        theDev->m_wPost, theDev->m_strUserName, theDev->m_strPassWord, &tmpDev, &nError);
    if(theDev->m_nHandle == 0)
    {
        WriteLog(g_szFlag, "设备重登录失败 调用API失败 重试 IP:0x%x", theDev->m_dwIP);
        theDev->m_nHandle = CLIENT_Login(inet_ntoa(*(in_addr*)&(theDev->m_dwIP)),
            theDev->m_wPost, theDev->m_strUserName, theDev->m_strPassWord, &tmpDev, &nError);
        if(theDev->m_nHandle == 0)
        {
            WriteLog(g_szFlag, "设备重登录失败 调用API重试失败 IP:0x%x", theDev->m_dwIP);
            theDev->m_nAudioHandle = 0;
            return -1;
        }
    }
    
    CLIENT_StartListen(theDev->m_nHandle);
    in_SaveComCfg(nLoginID);
    EDvr_ReLink(nLoginID);
    
    WriteLog(g_szFlag, "设备重登录 End IP:0x%x", theDev->m_dwIP);
    return nLoginID;
}

long CDevice_DH::EDvr_ReLink(long nLoginID)
{
    if( -1 == nLoginID ) return -1;
    SDeviceInfo_DH* theDev = m_DeviceList.GetDevice(nLoginID);
    SChannelInfo_DH* theChannel = NULL;
    for(int i=0; i < m_ChannelList.GetArraySize(); i++)
    {
        theChannel = m_ChannelList.GetChannel(i);
        if(NULL == theChannel)//空节点
            continue;
        if(theChannel->m_dwIP != theDev->m_dwIP)//不属于该设备信息
            continue;
        if(theChannel->m_nID <= DH_MAX_CHANNUM)
        {
            WriteLog(g_szFlag, "恢复连接 Begin IP:0x%x ChID:%d", theChannel->m_dwIP, theChannel->m_nID);
            theChannel->m_nHandle = CLIENT_RealPlayEx(theDev->m_nHandle, theChannel->m_nID, theChannel->m_hWnd);
            if(theChannel->m_nHandle)
            {
                if(theChannel->m_aPlayState[0])//恢复数据捕获
                    CLIENT_SetRealDataCallBackEx(theChannel->m_nHandle, DataCallBack, i, 0x00000001);
            }
            else
            {
                WriteLog(g_szFlag, "恢复连接失败 调用API失败 IP:0x%x ChID:%d", theChannel->m_dwIP, theChannel->m_nID);
            }
        }
    }
    return 0;
}

long CDevice_DH::EDvr_ReLink(DWORD nDevIPv4, long nChID)
{
    WriteLog(g_szFlag, "恢复连接 Begin IP:0x%x ChID:%d", nDevIPv4, nChID);
    SDeviceInfo_DH* theDev = NULL;
    SChannelInfo_DH* theChannel = NULL;
    long nLinkID = -1;
    //过滤非法信息
    theDev = m_DeviceList.SearchDevice(nDevIPv4);
    if(theDev == NULL) goto InfoError;
    nLinkID = m_ChannelList.GetChannelIndex(nDevIPv4, nChID);
    if(nLinkID == -1) goto InfoError;
    theChannel = m_ChannelList.GetChannel(nLinkID);
    //重新断连操作
    CLIENT_StopRealPlayEx(theChannel->m_nHandle);
    theChannel->m_nHandle = CLIENT_RealPlayEx(theDev->m_nHandle, theChannel->m_nID,
        theChannel->m_hWnd, FixMediaType(theChannel->m_nMediaType));
    if(theChannel->m_nHandle)
    {
        if(theChannel->m_aPlayState[0])//恢复数据捕获
        {
            CLIENT_SetRealDataCallBackEx(theChannel->m_nHandle, DataCallBack, nLinkID, 0x00000001);
        }
        return nLinkID;
    }
    else
    {
        WriteLog(g_szFlag, "恢复连接失败 调用API失败 IP:0x%x ChID:%d", theChannel->m_dwIP, theChannel->m_nID);
        return -1;
    }
InfoError:
    WriteLog(g_szFlag, "恢复连接 失败 重连信息错误 IP:0x%x ChID:%d", nDevIPv4, nChID);
    return -1;
}

//////////////////////////////////////////////////////////////////////
// 主系统类::试验接口
//////////////////////////////////////////////////////////////////////
long CDevice_DH::T_InsertDevice(char*pstrIP, long nPort, char*pstrLogName, char *pstrLogPass, long nIndex)
{
    /*判断是否初始化*/
    if(m_bNoInit)
        EDvr_Init();
    DWORD dwIP = inet_addr(pstrIP);
    /*判断输入参数合法*/
    if(dwIP == INADDR_NONE || nPort > 0x0000ffff || pstrLogName == NULL || pstrLogPass == NULL)
        return -1;
    /*设备是否已经登陆*/
    if(m_DeviceList.SearchDevice(pstrIP) != NULL)
        return -2;
    /*申请临时空间保存信息*/
    SDeviceInfo_DH temp;
    StructInit(temp);
    /*开始登陆操作*/
    temp.m_dwIP = dwIP;
    temp.m_wPost = (WORD)nPort;
    strcpy(temp.m_strUserName, pstrLogName);
    strcpy(temp.m_strPassWord, pstrLogPass);
    temp.m_nID = 1;
    /*执行注册操作*/
    int nError;
    temp.m_nHandle = CLIENT_Login(pstrIP,(WORD)nPort,pstrLogName,pstrLogPass,0,&nError);
    if(temp.m_nHandle == 0)
        return -1;
    /*打开设备监听*/
    if(!CLIENT_StartListen(temp.m_nHandle))
    {
        CLIENT_Logout(temp.m_nHandle);
        return -1;
    }
    long nRet = m_DeviceList.AddLogInDevice(nIndex,temp);
    return nRet;
}

void CDevice_DH::T_UserAlarm(char *pstrIP, long nDVRPort, long nChID)
{
    char aAlarm[16] = {0};
    aAlarm[nChID] = 1;
    m_pFunCommAlarmCB(pstrIP,nDVRPort,16,(BYTE*)aAlarm);
}
