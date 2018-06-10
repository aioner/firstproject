#include "Device_DH.h"
//////////////////////////////////////////////////////////////////////////
//透明传输接口

void CDevice_DH::in_SaveComCfg(int nLoginID)
{
    int i;
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg = {0};
    SDeviceInfo_DH* pDevice = m_DeviceList.GetDevice(nLoginID);
    if(!CLIENT_GetDevConfig(pDevice->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return;

    //232配置
   for(i=0; i<DH_MAX_232_NUM; i++)
    {
        FixComCfgStruct(pDevice->m_cfg232[i], ComCfg.st232[i].struComm);
    }
    //485配置
   for(i=0; i<DH_MAX_CHANNUM; i++)
    {
        FixComCfgStruct(pDevice->m_cfg485[i], ComCfg.stDecoder[i].struComm);
    }
}

long CDevice_DH::in_SerialSend(int nLoginID, int nSerialPort, void* pDataBuf, long nDataSize)
{
    int nRet = 0;
    SDeviceInfo_DH* pDevice = m_DeviceList.GetDevice(nLoginID);
    if(!pDevice || !pDevice->m_hSerialTran[nSerialPort])
        return -1;
    nRet = CLIENT_SendTransComData(pDevice->m_hSerialTran[nSerialPort], (char*)pDataBuf, nDataSize);
    return (nRet?0:-1);
}

//停止回放文件
void CDevice_DH::New_StopPlayBack( HWND hWnd )
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
	if(sChannel->m_nHandle)
		CLIENT_StopPlayBack(sChannel->m_nHandle);
	sChannel->m_nHandle = NULL;
}
//关闭回放文件
void CDevice_DH::New_ClosePlayBack( HWND hWnd )
{
	SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
	/*判断数据连接通道是否打开*/
	if(sChannel == NULL)
		return;
	/*判断是否为即时数据连接*/
	if(sChannel->m_nChannelType != 2)
		return;
	/*如果正在数据捕获,就停止动作*/
	if(sChannel->m_aPlayState[0])
		EDvr_StopDataCapture(m_ChannelList.GetChannelIndex(hWnd));
	/*如果正在录像,就停止录像*/
	if(sChannel->m_aPlayState[1])
		EDvr_StopDataRecorder(m_ChannelList.GetChannelIndex(hWnd));
	/*断开数据连接*/
	if(sChannel->m_nHandle)
		New_StopPlayBack(hWnd);
	/*释放数据通道*/
	if(sChannel->m_pRPFile)
		delete sChannel->m_pRPFile;
	m_ChannelList.DeletStopChannel(m_ChannelList.GetChannelIndex(hWnd));
}
long CDevice_DH::New_SetPosPlayBack(HWND hWnd, double dbPos)
{
    int nRet;
	SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
	if(!pTemp) return -1;
	if(!pTemp->m_pRPFile) return -1;
    DWORD nSetKByte = (DWORD)(dbPos/100*pTemp->m_pRPFile->size);
	nRet = CLIENT_SeekPlayBack(pTemp->m_nHandle, 0xffffffff, nSetKByte);
    return (nRet?0:-1);
}

//播放进度回调
void CDevice_DH::New_RegPlayBackPos(DHPlayBackPos CallBackFunc, DWORD dwUser)
{
	m_pPlayBackPos = CallBackFunc;
	m_nPBUser = dwUser;
}
//下载进度回调
void CDevice_DH::New_RegDownloadPos(DHDownloadPos CallBackFunc, DWORD dwUser)
{
	m_pDownloadPos = CallBackFunc;
	m_nDLUser = dwUser;
}

//设置播放图像的色彩
long CDevice_DH::New_GetColor(HWND hWnd, long* nBright, long* nContrast, long* nSaturation, long* nHue)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
    //判断数据连接通道是否打开
    if(sChannel == NULL)
        return -1;
    //判断是否为数据播放
    if(sChannel->m_nChannelType != 1 && sChannel->m_nChannelType != 2)
        return -1;
    BYTE nEffect[4];
    if(! CLIENT_ClientGetVideoEffect(sChannel->m_nHandle, nEffect, nEffect+1, nEffect+2, nEffect+3) )
        return -1;
    *nBright = nEffect[0]*2;
    *nContrast = nEffect[1]*2;
    *nSaturation = nEffect[3]*2;
    *nHue = nEffect[2]*2;
    return 0;
}

long CDevice_DH::New_SetColor(HWND hWnd, long nBright, long nContrast, long nSaturation, long nHue)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
    //判断数据连接通道是否打开
    if(sChannel == NULL)
        return -1;
    //判断是否为数据播放
    if(sChannel->m_nChannelType != 1 && sChannel->m_nChannelType != 2)
        return -1;
    BYTE nEffect[4];
    nEffect[0] = (BYTE)nBright/2;
    nEffect[1] = (BYTE)nContrast/2;
    nEffect[3] = (BYTE)nSaturation/2;
    nEffect[2] = (BYTE)nHue/2;
    if(! CLIENT_ClientSetVideoEffect(sChannel->m_nHandle, nEffect[0], nEffect[1], nEffect[2], nEffect[3]) )
        return -1;
    return 0;
}

/*获取通道视频配置*/
long CDevice_DH::New_GetChannelVideoInfo(char *pstrIP, long nChID, long nEncOpt, char *pstrChannelName, bool *bVideoEnable, BYTE *nBitRateType, long *nBitRate, BYTE *nFPS, BYTE *nEncodeMode, BYTE *nImageSize, BYTE *nImageQlty)
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
	case 1: VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+0; break;
	case 2: VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1; break;
	case 3: VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2; break;
	case 4: VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+0; break;
	case 5: VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1; break;
	case 6: VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2; break;
	default:return -1;
	}
	if(!VideoEncOpt->byVideoEnable)
		*bVideoEnable = false;
	else
		*bVideoEnable = true;
	
	for(long i=0; i<12; i++)
	{
		if(nVideoRateList[i] >= VideoEncOpt->wLimitStream)
		{
			*nBitRate = i;
			break;
		}
	}

	*nBitRateType = VideoEncOpt->byBitRateControl;
	*nFPS = VideoEncOpt->byFramesPerSec-1;
	*nEncodeMode = VideoEncOpt->byEncodeMode;
	*nImageSize = VideoEncOpt->byImageSize;
	*nImageQlty = VideoEncOpt->byImageQlty;
	return 0;
}
/*设置通道视频配置*/
long CDevice_DH::New_SetChannelVideoInfo(char *pstrIP, long nChID, long nEncOpt, char *pstrChannelName, bool bVideoEnable, BYTE nBitRateType, long nBitRate, BYTE nFPS, BYTE nEncodeMode, BYTE nImageSize, BYTE nImageQlty)
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
	case 1: VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+0; break;
	case 2: VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+1; break;
	case 3: VideoEncOpt = ChannelCfg[nChID].stMainVideoEncOpt+2; break;
	case 4: VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+0; break;
	case 5: VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+1; break;
	case 6: VideoEncOpt = ChannelCfg[nChID].stAssiVideoEncOpt+2; break;
	default:return -1;
	}
	VideoEncOpt->byVideoEnable = bVideoEnable;
	VideoEncOpt->byBitRateControl = nBitRateType;
	VideoEncOpt->wLimitStream = nVideoRateList[nBitRate];
	VideoEncOpt->byFramesPerSec = nFPS+1;
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

long CDevice_DH::New_GetFPSList(DWORD* nCount, char* pstrNameList)
{
	static const char szNameList[25][8] = {"1","2","3","4","5","6","7","8","9","10","11",
		"12","13","14","15","16","17","18","19","20","21","22","23","24","25"};
	*nCount = 25;
	memcpy(pstrNameList, szNameList, sizeof(szNameList));
	return 0;
}

const WORD CDevice_DH::nVideoRateList[12] = {50, 128, 256, 512, 768, 1024, 1536, 2048, 2560, 3072, 3584, 4096};
long CDevice_DH::New_GetVideoRateList(DWORD* nCount, char* pstrNameList)
{
	static const char szNameList[12][8] = {"50K/s","128K/s","256K/s","512K/s","768K/s",
		"1M/s","1.5M/s","2M/s","2.5M/s","3M/s","3.5M/s","4M/s"};
	*nCount = 12;
	memcpy(pstrNameList, szNameList, sizeof(szNameList));
	return 0;
}

//////////////////////////////////////////////////////////////////////////

long CDevice_DH::New_InitDevice(long nDeviceType, long nDataType)
{    
    m_nDeviceType = nDeviceType;
    m_nDataType = nDataType;

    return EDvr_Init();
}

void CDevice_DH::New_ExitDevice()
{
    EDvr_Exit();
}

long CDevice_DH::New_LoginDevice(char* szDeviceIP, long nPort, char* szUserID,
                                 char* szPassword, long nModelType)
{
    long nRet = EDvr_Login((char*)szDeviceIP, nPort, (char*)szUserID, (char*)szPassword, true);
    
    return nRet;
}

void CDevice_DH::New_LogoutDevice(char* szDeviceIP)
{
    EDvr_LogOut((char*)szDeviceIP);
}

void CDevice_DH::New_LogoutDevice(long lLoginHandle)
{
	EDvr_LogOut(lLoginHandle);
}

long CDevice_DH::New_StartLinkDevice(char* szDeviceIP, long nChannel, long nLinkType,
                         long nNetPort, long nMediaType, void* UserContext)
{
    long nRet = EDvr_StartDataLink((char*)szDeviceIP, nChannel, nLinkType, nMediaType, NULL);
    if(nRet < 0) return nRet;
    
    SChannelInfo_DH *pTemp = m_ChannelList.GetChannel(nRet);
    pTemp->m_nData = (long)UserContext;
    
    return nRet;
}

void CDevice_DH::New_StopLinkDevice(long nLinkID)
{
    EDvr_StopDataLink(nLinkID);
}

long CDevice_DH::New_StartLinkCapture(long nRDataLink, OV_PRealDataCallback fCallBack, void* objUser)
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
    sChannel->m_nData = (long)objUser;
    sChannel->m_pNewCallBack = fCallBack;
    CLIENT_SetRealDataCallBackEx(sChannel->m_nHandle, CDevice_DH::DataCallBack, (DWORD)nRDataLink, 1);
    return 0;
}

void CDevice_DH::New_StopLinkCapture(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
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
    CLIENT_SetRealDataCallBackEx(sChannel->m_nHandle,NULL,0,1);
    sChannel->m_aPlayState[0] = FALSE;
    return;
}

long CDevice_DH::New_GetHeadData(long nLinkID, BYTE* pHeadData)
{
	unsigned char DHFileHead[5] = {'D','A','H','U','A'};
	memcpy(pHeadData, DHFileHead, 5);
	return 5;
}

//添加超级用户 20081208 HuangYJ
long CDevice_DH::New_AddPowerUser(char* szIP, char* szUser, char* szPwd)
{
    USER_INFO* pUserInfo = NULL;
    DWORD i = 0, nError = 0;
    long nOperateType = 3;//默认添加用户
    //设备是否已登陆
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(szIP);
    if(NULL == temp)
        return -1;
    //输入参数正确
    if(NULL==szUser || 0 == szUser[0])
        return -1;
    //得到配置信息
    USER_MANAGE_INFO theMrgInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&theMrgInfo))
        return -1;
    //检索用户是否存在
    for(i=0; i<theMrgInfo.dwUserNum; i++)
    {
        pUserInfo = theMrgInfo.userList+i;
        if(!strcmp(pUserInfo->name, szUser))
        {
            //用户已存在，删除用户
            CLIENT_OperateUserInfo(temp->m_nHandle,4,pUserInfo,NULL, 2000);
            break;
        }
        pUserInfo = NULL;
    }
    
    USER_INFO newUserInfo = {0};
    newUserInfo.dwReusable = 1;//允许重用
    newUserInfo.dwGroupID = 1;//管理员组
    newUserInfo.dwRightNum = theMrgInfo.dwRightNum;//所有权限
    for(i=0; i<theMrgInfo.dwRightNum; i++)
    {
        newUserInfo.rights[i] = theMrgInfo.rightList[i].dwID;
    }
    newUserInfo.dwID = theMrgInfo.dwUserNum + 1;
    strcpy(newUserInfo.name, szUser);
    strcpy(newUserInfo.memo, szUser);
    if(NULL == szPwd || 0 == szPwd[0])//使用默认与用户名相同的密码
        strcpy(newUserInfo.passWord, szUser);
    else
        strcpy(newUserInfo.passWord, szPwd);
    
    //设置信息
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,nOperateType,&newUserInfo,NULL,2000))
    {
        nError = CLIENT_GetLastError();
        return -1;
    }
    return 0;
}

/*//上线重连操作 改用定时器
DWORD __stdcall CDevice_DH::WaitProcFunc(void* pParam)
{
    CDevice_DH *pThis = (CDevice_DH*)pParam;
    long i = 0, t=0;
    clock_t nClock = 0;
    while (pThis->m_bIsInit)
    {
        nClock = clock();
        for(i=0; i<pThis->m_lstOnline.size(); i++)
        {
            if(pThis->m_lstOnline[i].szIP[0] == 0)
                continue;
            if(pThis->m_lstOnline[i].nWaitTime <= 0)
            {//计时到位
                pThis->EDvr_ReLogin(pThis->m_lstOnline[i].szIP);
                WriteLog(g_szFlag, "发送设备上线 Begin m_pFunDisConnectCB:%x IP:%s",
                    pThis->m_pFunDisConnectCB, pThis->m_lstOnline[i].szIP);
                if(NULL != pThis->m_pFunDisConnectCB)
                {
                    WriteLog(g_szFlag, "发送设备上线 IP:%s", pThis->m_lstOnline[i].szIP);
                    pThis->m_pFunDisConnectCB(pThis->m_lstOnline[i].szIP,1);
                }
                WriteLog(g_szFlag, "发送设备上线 End m_pFunDisConnectCB:%x IP:%s",
                    pThis->m_pFunDisConnectCB, pThis->m_lstOnline[i].szIP);
                StructInit(pThis->m_lstOnline[i]);
            }
            else
            {
                //计时器-1000ms
                pThis->m_lstOnline[i].nWaitTime --;
            }
        }
        nClock = clock() - nClock;
        
        for(t=0; t<1000 && pThis->m_bIsInit ; t+=100)
        {
            SleepEx(100,true);
        }
    }
    return 0;
}
*/
