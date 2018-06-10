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
// ��ϵͳ��::��̬����
//////////////////////////////////////////////////////////////////////
void CALLBACK CDevice_DH::DisConnectFunc(LONG lLoginID, char *pchDVRIP, LONG nDVRPort, DWORD dwUser)
{
    WriteLog(g_szFlag, "�յ��豸���� IP:%s", pchDVRIP);
    if(!dwUser) return;
    CDevice_DH *pThis = (CDevice_DH*)dwUser;
    //�������Զ�����
    //pThis->EDvr_LogOut(pchDVRIP);
    if(NULL != pThis->m_pFunDisConnectCB)
    {
        pThis->m_pFunDisConnectCB(pchDVRIP,0);
    }
}

void CALLBACK CDevice_DH::ReOnlineFunc(long lLoginID, char *pchDVRIP, long nDVRPort, DWORD dwUser)
{
    WriteLog(g_szFlag, "�յ��豸���� IP:%s", pchDVRIP);
    if(!dwUser) return;
    CDevice_DH *pThis = (CDevice_DH*)dwUser;
    
    // ��Ҫ�����ӳ�10��
    //    pThis->EDvr_ReLogin(lLoginID);
    //    if(NULL != pThis->m_pFunDisConnectCB)
    //    {
    //        pThis->m_pFunDisConnectCB(pchDVRIP,1);
    //    }
    unsigned i=0;
    SReOnlineInfo theInfo;
    strcpy(theInfo.szIP, pchDVRIP);
    theInfo.nWaitTime = REONLINE_WAIT_TIME;
    
    //�����豸������Ϣ
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
    
    //����ͳ��
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
// ��ϵͳ��::����ӿ�
//////////////////////////////////////////////////////////////////////

//��̬�ֶ�����
long CDevice_DH::m_nDeviceType = 0;
long CDevice_DH::m_nDataType = 0;
char CDevice_DH::g_szFlag[] = "Dll_DeviceDH";
unsigned char* pszStringBinding = NULL;

long CDevice_DH::EDvr_Init()
{	
    /*��ʼ��*/
    if(!CLIENT_Init(DisConnectFunc,(unsigned long)this))
        return -1;
    m_bNoInit = false;
    
    //�ڲ��鿴ģʽ
    GetPrivateProfileInt("WriteLog", g_szFlag, 0, "d:\\netmcset\\�ײ�����.ini");
    EnableLog(true);

    /*������Ϣ�ص�*/
    CLIENT_SetDVRMessCallBack(MessCallBack,(unsigned long)this);
    //�����Զ��������� 20081121 HuangYJ
    CLIENT_SetAutoReconnect(ReOnlineFunc, (unsigned long)this);
    /*������Ӧʱ��*/
    CLIENT_SetConnectTime(5000,3);

    return 0;
}

/*ֹͣ��ǰ�����в������˳��ͷ�������Դ*/
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

/*���ö�����Ϣ�ص�*/
void CDevice_DH::EDvr_SetDisConnectCallBackFunc(fDisConnectCB lpFuncCB)
{
    if(m_pFunDisConnectCB == NULL)
        m_pFunDisConnectCB = lpFuncCB;
}

/*��¼��ָ�����豸��*/
long CDevice_DH::EDvr_Login(char *pstrIP, long nPort, char*pstrLogName, char *pstrLogPass, bool bCheckUser)
{
    /*�ж��Ƿ��ʼ��*/
    if(m_bNoInit)
    {
        int nRet = EDvr_Init();
        if(nRet < 0)
            return -1;
    }

    DWORD dwIP = inet_addr(pstrIP);
    /*�ж���������Ϸ�*/
    if(dwIP == INADDR_NONE || nPort > 0x0000ffff)
        return -1;
    /*�豸�Ƿ��Ѿ���½*/
    long nLoginID = m_DeviceList.SelectLoginID(pstrIP);
    SDeviceInfo_DH* pDevice = m_DeviceList.GetDevice(nLoginID);
    if(nLoginID != -1 && pDevice->m_nHandle != 0)
        return nLoginID;
    /*������ʱ�ռ䱣����Ϣ*/
    SDeviceInfo_DH temp;
    StructInit(temp);

    temp.m_dwIP = dwIP;
    if(nPort != 0) temp.m_wPost = (WORD)nPort;
    if( pstrLogName && *pstrLogName != 0 ) strcpy(temp.m_strUserName, pstrLogName);
    if( pstrLogPass && *pstrLogPass != 0 ) strcpy(temp.m_strPassWord, pstrLogPass);
    temp.m_nID = 1;
    /*ִ��ע�����*/
    int nError;
    NET_DEVICEINFO tmpDev;

    {//Ȩ����֤��¼,�û�����ģʽ
        WriteLog(g_szFlag, "��¼�豸Ex IP:%s Port:%d Name:%s Pwd:%s", pstrIP, nPort, temp.m_strUserName, temp.m_strPassWord);
        temp.m_nHandle = CLIENT_Login(pstrIP,(WORD)nPort,temp.m_strUserName,temp.m_strPassWord,&tmpDev,&nError);
    }
    if(temp.m_nHandle == 0)
    {
        WriteLog(g_szFlag, "��¼�豸ʧ�� API����ʧ�� IP:%s Error:0x%x", pstrIP, CLIENT_GetLastError());
        return -1;
    }
    /*���豸����*/
    // if(!EDvr_Listen(temp)) huangsq 2010.5.12
    if(EDvr_Listen(temp) < 0)
    {
        WriteLog(g_szFlag, "��¼�豸ʧ�� ��������ʧ�� IP:%s", pstrIP);
        CLIENT_Logout(temp.m_nHandle);
        return -1;
    }
    if(pDevice == NULL)//�µ�¼�豸
        nLoginID = m_DeviceList.AddLogInDevice(temp);
    else//�Ѵ����豸
        memcpy(pDevice, &temp, sizeof(SDeviceInfo_DH));
    
    in_SaveComCfg(nLoginID);
    return nLoginID;
}

/*���豸����*/
long CDevice_DH::EDvr_Listen(SDeviceInfo_DH &rDevice)
{
    //�õ��豸�İ汾
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

/*��ָ�����豸��ע��*/
void CDevice_DH::EDvr_LogOut(char *pstrIP)
{
    /*�豸�Ƿ��½��*/
    SDeviceInfo_DH *pTemp = m_DeviceList.SearchDevice(pstrIP);
    if(pTemp == NULL)
        return;
    
    /*��ȡ��½��¼*/
    long lDeviceHandle = pTemp->m_nHandle, nRet = -1;
    /*ֹͣ�豸����*/
    nRet = CLIENT_StopListen(lDeviceHandle);
    /*�Ͽ��豸������������*/
    EDvr_StopDeviceDataLink(pstrIP);
    /*ע���豸*/
    nRet = CLIENT_Logout(lDeviceHandle);
    
    /*�ͷ��豸��Ϣ*/
    m_DeviceList.DeletLogOutDevice(pstrIP);
}

/*��ָ�����豸��ע�� add pan*/
void CDevice_DH::EDvr_LogOut(long lLoginHandle)
{
	/*�豸�Ƿ��½��*/
	SDeviceInfo_DH *pTemp = m_DeviceList.SearchDeviceByLogInID(lLoginHandle);
	if(pTemp == NULL)
		return;

	/*��ȡ��½��¼*/
	long lDeviceHandle = pTemp->m_nHandle, nRet = -1;
	/*ֹͣ�豸����*/
	nRet = CLIENT_StopListen(lDeviceHandle);
	/*ע���豸*/
	nRet = CLIENT_Logout(lDeviceHandle);

	/*�ͷ��豸��Ϣ*/
	m_DeviceList.DeletLogOutDevice(lLoginHandle);
}

/*�������ѵ�¼���豸��ע��*/
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

/*Զ�������豸*/
void CDevice_DH::EDvr_Reboot(char *pstrIP)
{
    /*�豸�Ƿ��½��*/
    SDeviceInfo_DH *pTemp = m_DeviceList.SearchDevice(pstrIP);
    if(pTemp == NULL)
        return;
    /*��ȡ��½��¼*/
    long lDeviceHandle = pTemp->m_nHandle;
    /*�����豸*/
    CLIENT_RebootDev(lDeviceHandle);
}

/*������ָ���豸��ָ��ͨ������������*/
long CDevice_DH::EDvr_StartDataLink(char *pstrIP,long nChID,long nLinkType,long nMediaType,HWND hWin)
{
    //�ڲ��鿴ģʽ
    int nLog = GetPrivateProfileInt("WriteLog", "DLL_DeviceDH", 0, "D:\\NetMcSet\\�ײ�����.ini");
    EnableLog(nLog);

    WriteLog(g_szFlag, "�������� Begin IP:%s ChID:%d MediaType:%d HWND:0x%x", pstrIP, nChID, nMediaType, hWin);
    DWORD dwIP = inet_addr(pstrIP);
    long nLinkID = -1;
    /*�ж���������Ϸ�*/
    if(dwIP == INADDR_NONE)
    {
        WriteLog(g_szFlag, "��������ʧ�� IP�Ƿ����� IP:%s", pstrIP);
        return -1;
    }
    /*ͨ���Ƿ��Ѿ���ͨ*/
    if((nLinkID = m_ChannelList.GetChannelIndex(pstrIP,nChID)) != -1)
    {
        SChannelInfo_DH* theChannel = m_ChannelList.GetChannel(nLinkID);
        if(theChannel->m_nHandle != -1)
        {
            WriteLog(g_szFlag, "��������ʧ�� �����Ѵ��� IP:%s ChID:%d", pstrIP, nChID);
            return -1;
        }
        else
        {//����������ʽ����
            return EDvr_ReLink(theChannel->m_dwIP, theChannel->m_nID);
        }
    }
    /*��ʾ�����Ƿ�ռ��*/
    if(-1 != m_ChannelList.CheckFormUsed(hWin))
    {
        WriteLog(g_szFlag, "��������ʧ�� ��ʾ���ڱ�ռ�� IP:%s ChID:%d HWND:0x%x", pstrIP, nChID, hWin);
        return -1;
    }
    /*������ʱ�ռ䱣����Ϣ*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = nChID;
    temp.m_nChannelType = 1;
    temp.m_nMediaType = nMediaType;
    temp.m_hWnd = hWin;
    /*�豸�Ƿ��½��*/
    SDeviceInfo_DH *pTemp = m_DeviceList.SearchDevice(pstrIP);
    if(pTemp == NULL)
    {
        WriteLog(g_szFlag, "��������ʧ�� �豸û�е�½ IP:%s", pstrIP);
        return -1;
    }
    /*����ָ����������*/
    temp.m_nHandle = CLIENT_RealPlayEx(pTemp->m_nHandle, nChID, hWin, FixMediaType(nMediaType));
    if (temp.m_nHandle == 0)
    {
        long nError = CLIENT_GetLastError();
        WriteLog(g_szFlag, "��������ʧ�� API����ʧ�� ��ʼ���� IP:%s ChID:%d HWND:0x%x Error:0x%x", pstrIP, nChID, hWin, nError);
        long nPlayID = m_ChannelList.AddPlayChannel(temp);
        if(EDvr_ReLogin(pstrIP) == -1)
        {
            WriteLog(g_szFlag, "��������ʧ�� �ص�¼ʧ�� IP:%s ChID:%d HWND:0x%x", pstrIP, nChID, hWin);
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
    /*����ͨ����Ϣ������ͨ����*/
    WriteLog(g_szFlag, "�������� End IP:%s ChID:%d HWND:0x%x", pstrIP, nChID, hWin);
    return m_ChannelList.AddPlayChannel(temp);
}

/*ֹͣ��ָ���豸��ָ��ͨ������������*/
void CDevice_DH::EDvr_StopDataLink(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    EDvr_StopDataLink(nRDataLink);
}

void CDevice_DH::EDvr_StopDataLink(long nRDataLink)
{
    WriteLog(g_szFlag, "ֹͣ���� LinkID:%d", nRDataLink);
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return;

    //begin 20090323 ֹͣ���ܴ��ڵ���������
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

    /*����������ݴ���,��ֹͣ����*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(nRDataLink);
    /*�������¼��,��ֹͣ¼��*/
    if(sChannel->m_aPlayState[1])
        EDvr_StopDataRecorder(nRDataLink);
    /*�Ͽ���������*/
    CLIENT_StopRealPlayEx(sChannel->m_nHandle);
    /*�ͷ�����ͨ��*/
    m_ChannelList.DeletStopChannel(nRDataLink);
}

/*ֹͣ��ǰ��������������*/
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

/*��ȡ�ɼ�ͨ����Ӧ�������*/
long CDevice_DH::EDvr_GetChannelHandle(char *pstrIP, long nChID)
{
    return m_ChannelList.GetChannelIndex(pstrIP,nChID);
}


/*����ָ���豸��ָ��ͨ�����������ӵ����ݲ���*/
long CDevice_DH::EDvr_StartDataCapture(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    return EDvr_StartDataCapture(nRDataLink);
}

long CDevice_DH::EDvr_StartDataCapture(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return -1;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return -1;
    /*�ж��Ƿ��������ݲ���*/
    if(sChannel->m_aPlayState[0])
        return -1;
    /*��ʼ��������*/
    sChannel->m_aPlayState[0] = true;
    sChannel->m_nData = nRDataLink;
    CLIENT_SetRealDataCallBackEx(sChannel->m_nHandle, DataCallBack, (DWORD)nRDataLink, 0x00000001);
    return 0;
}

/*ָֹͣ���豸��ָ��ͨ�����������ӵ����ݲ���*/
void CDevice_DH::EDvr_StopDataCapture(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    EDvr_StopDataCapture(nRDataLink);
}

void CDevice_DH::EDvr_StopDataCapture(long nLinkID)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nLinkID);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*�ж��Ƿ��������ݲ���*/
    if(!sChannel->m_aPlayState[0])
        return;
    /*��ʼ��������*/
    CLIENT_SetRealDataCallBackEx(sChannel->m_nHandle,NULL,0,0);
    sChannel->m_aPlayState[0] = FALSE;
    return;
}

/*��ʼָ���豸�Ƕ�ͨ�����������ӵ����ݱ����ļ�(¼��)*/
long CDevice_DH::EDvr_StartDataRecorder(char *pstrIP, long nChID, char *pstrFileName)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    return EDvr_StartDataRecorder(nRDataLink, pstrFileName);
}

long CDevice_DH::EDvr_StartDataRecorder(long nRDataLink, char *pstrFileName)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return -1;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return -1;
    /*�ж��Ƿ�����¼��*/
    if(sChannel->m_aPlayState[1])
        return -1;
    /*�ж��ļ�����Ч*/
    if(pstrFileName == NULL || pstrFileName[0] == '\0')
        return -1;
    /*��ʼ¼��*/
    if(!CLIENT_SaveRealData(sChannel->m_nHandle,pstrFileName))
        return -1;
    sChannel->m_aPlayState[1] = true;
    return 0;
}

/*ָֹͣ���豸ָ��ͨ�����������ӵ����ݱ����ļ�(¼��)*/
void CDevice_DH::EDvr_StopDataRecorder(char *pstrIP, long nChID)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    EDvr_StopDataRecorder(nRDataLink);
}

void CDevice_DH::EDvr_StopDataRecorder(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*�ж��Ƿ�����¼��*/
    if(!sChannel->m_aPlayState[1])
        return;
    /*ֹͣ¼��*/
    if(!CLIENT_StopSaveRealData(sChannel->m_nHandle))
        return;
    sChannel->m_aPlayState[1] = false;
}

/*��ָ���豸ָ��ͨ��������������н�ͼ*/
long CDevice_DH::EDvr_ConvertPicture(char *pstrIP, long nChID, char *pstrFileName)
{
    long nRDataLink = m_ChannelList.GetChannelIndex(pstrIP,nChID);
    return EDvr_ConvertPicture(nRDataLink, pstrFileName);
}

long CDevice_DH::EDvr_ConvertPicture(long nRDataLink, char *pstrFileName)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return -1;
    /*��ʼ��ͼ*/
    if(!CLIENT_CapturePicture(sChannel->m_nHandle, pstrFileName))
        return -1;
    return 0;
}

/*��̨����*/
long CDevice_DH::EDvr_PTZControl(char *pstrIP, long nChID, long nPTZCommand, bool bStop, long nSpeed)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*�ж��豸��½*/
    if(NULL == sDevice)
        return -1;
    /*�жϲ�����Ч*/
    //	if(0 >nSpeed || 8<nSpeed)
    //		return -1;
    /*��ʼ��̨����*/
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

/*��̨������չ*/
long CDevice_DH::EDvr_PTZControlEx(char *pstrIP, long nChID, long nPTZCommandEx, bool bStop,
                                   unsigned char param1, unsigned char param2, unsigned char param3)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*�ж��豸��½*/
    if(NULL == sDevice)
        return -1;
    /*��ʼ��̨����*/
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

/*��ȡԶ��¼������*/
long CDevice_DH::EDvr_GetRecordBackState(char *pstrIP, char *pRState, long nMaxLen, long *pnStateLen)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*�ж��豸��½*/
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

/*����Զ��¼������*/
long CDevice_DH::EDvr_SetRecordBackState(char *pstrIP, char *pRState, long nStateLen)
{
    SDeviceInfo_DH *sDevice = m_DeviceList.SearchDevice(pstrIP);
    /*�ж��豸��½*/
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

/*�رղ�ѯ���*/
void CDevice_DH::EDvr_FindClose(long lFileFindHandle)
{
    CLIENT_FindClose(lFileFindHandle);
}
NET_RECORDFILE_INFO temp;
/*������һ����¼*/
long CDevice_DH::EDvr_FindNextFile(long lFileFindHandle)
{    
	long nRes = 0;
	nRes = CLIENT_FindNextFile(lFileFindHandle,&temp);
    m_BackFileList->AddFileInfo(temp);
    return nRes;
}

/*��ʼ����¼���ļ�*/
long CDevice_DH::EDvr_StartPlayBack(char *pstrIP, long nFileHandle, HWND hWnd)
{
    DWORD dwIP = inet_addr(pstrIP);
    /*�ж���������Ϸ�*/
    if(dwIP == INADDR_NONE)
        return -1;
    /*�豸�Ƿ��ѵ�½*/
    if(m_DeviceList.SearchDevice(pstrIP) == NULL)
        return -1;
    /*��ѯ�ط��ļ�*/
    NET_RECORDFILE_INFO * lpRecordFile = m_BackFileList->GetFileInfo(nFileHandle);
    if(NULL == lpRecordFile)
        return -1;
    /*ͨ���Ƿ��Ѿ���ͨ*/
    if(m_ChannelList.SearchChannel(pstrIP,lpRecordFile->ch) != NULL)
        return -2;
    /*��ʾ�����Ƿ�ռ��*/
    if(-1 != m_ChannelList.CheckFormUsed(hWnd))
        return -1;
    /*������ʱ�ռ䱣����Ϣ*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = lpRecordFile->ch;
    temp.m_nChannelType = 2;
    temp.m_hWnd = hWnd;
    /*����ָ�����ݻط�*/
//     temp.m_nHandle = CLIENT_PlayBackByRecordFile(m_DeviceList.SearchDevice(pstrIP)->m_nHandle,
//         lpRecordFile, hWnd, FileDownloadPosCallBack, (DWORD)this);
    if (temp.m_nHandle == 0)
        return -1;
    /*����ͨ����Ϣ������ͨ����*/
    return m_ChannelList.AddPlayChannel(temp);
}
long CDevice_DH::EDvr_StartPlayBack(char *pstrIP, long nChID, char *pstrFileName, HWND hWnd)
{
    DWORD dwIP = inet_addr(pstrIP);
    /*�ж���������Ϸ�*/
    if(dwIP == INADDR_NONE)
        return -1;
    /*�豸�Ƿ��ѵ�½*/
    if(m_DeviceList.SearchDevice(pstrIP) == NULL)
        return -1;
    /*ͨ���Ƿ��Ѿ���ͨ*/
    if(m_ChannelList.SearchChannel(pstrIP,nChID) != NULL)
        return -2;
    /*������ʱ�ռ䱣����Ϣ*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = nChID;
    temp.m_nChannelType = 2;
    temp.m_hWnd = hWnd;
    /*�����ļ���Ϣ�ṹ*/
    NET_RECORDFILE_INFO lRecordFile;
    lRecordFile.ch = nChID;
    strcpy(lRecordFile.filename,pstrFileName);
    /*����ָ�����ݻط�*/
    temp.m_nHandle = CLIENT_PlayBackByRecordFile(m_DeviceList.SearchDevice(pstrIP)->m_nHandle,
        &lRecordFile, hWnd, FileDownloadPosCallBack, (unsigned long)this);
    if (temp.m_nHandle == 0)
        return -1;
    /*����ͨ����Ϣ������ͨ����*/
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
    case DH_COMM_ALARM:			//��ͨ����
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
    case DH_SHELTER_ALARM:		//�ڵ�����
        {
            if(NULL == device->m_pFunShelterAlarmCB)
                return FALSE;
            ret = device->m_pFunShelterAlarmCB(pchDVRIP,nDVRPort,(BYTE*)pBuf,dwBufLen);
        }
        break;
    case DH_DISK_FULL_ALARM:	//����������
        {
            if(NULL == device->m_pFunDiskFullCB)
                return FALSE;
            ret = device->m_pFunDiskFullCB(pchDVRIP,nDVRPort,(BYTE)*(DWORD*)pBuf);
        }
        break;
    case DH_DISK_ERROR_ALARM:	//���̴��󱨾�
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
    case DH_SOUND_DETECT_ALARM:	//��Ƶ��ر���
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

/*ֹͣ�ط�*/
void CDevice_DH::EDvr_StopPlayBack(char *pstrIP, long nChID)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,nChID);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 2)
        return;
    /*����������ݲ���,��ֹͣ����*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(pstrIP,nChID);
    /*�������¼��,��ֹͣ¼��*/
    if(sChannel->m_aPlayState[1])
        EDvr_StopDataRecorder(pstrIP,nChID);
    /*�Ͽ���������*/
    CLIENT_StopPlayBack(sChannel->m_nHandle);
    /*�ͷ�����ͨ��*/
    m_ChannelList.DeletStopChannel(pstrIP,nChID);
}
void CDevice_DH::EDvr_StopPlayBack(long nBDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nBDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ�ط���������*/
    if(sChannel->m_nChannelType != 2)
        return;
    /*����������ݴ���,��ֹͣ����*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(nBDataLink);
    /*�Ͽ���������*/
    CLIENT_StopPlayBack(sChannel->m_nHandle);
    /*�ͷ�����ͨ��*/
    m_ChannelList.DeletStopChannel(nBDataLink);
}
void CDevice_DH::EDvr_StopPlayBack(HWND hWnd)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
    if(!sChannel)
        return;
    /*�ж��Ƿ�Ϊ�ط���������*/
    if(sChannel->m_nChannelType != 2)
        return;
    /*����������ݴ���,��ֹͣ����*/
    if(sChannel->m_aPlayState[0])
        EDvr_StopDataCapture(m_ChannelList.GetChannelIndex(hWnd));
    /*�Ͽ���������*/
    CLIENT_StopPlayBack(sChannel->m_nHandle);
    /*�ͷ�����ͨ��*/
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

/*ֹͣ����¼���ļ�*/
void CDevice_DH::EDvr_StopFileDownload(long nDownloadHandle)
{
    //��������ͷ
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
    /*�ж���������ͨ���Ƿ��*/
    long nFileHandle;
    if(NULL == m_BackFileList->SearchFileInfo(nChID,pstrFileName,&nFileHandle))
        return;
    /*�Ͽ���������*/
//    CLIENT_StopDownload(m_BackFileList->m_hDownloadHandle[nFileHandle]);
    //��������ͷ
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
    /*�ͷ�����ͨ��*/
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

/*�������¼��ĵ�ǰλ��*/
long CDevice_DH::EDvr_GetDownloadPos(long nDownHandle, long *nTotalSize, long *nDownLoadSize)
{
    /*�ж���������ͨ���Ƿ��*/
    if(NULL == m_BackFileList->GetFileInfo(nDownHandle))
        return -1;
//    long nRealHandle = m_BackFileList->m_hDownloadHandle[nDownHandle];
//    return CLIENT_GetDownloadPos(nRealHandle, (int*)nTotalSize, (int*)nDownLoadSize);
    return 0;
}

/*���������ص�����*/
void CDevice_DH::EDvr_SetAudioDataCallBackFunc(fAudioDataCallBack CallBackFunc)
{
    m_pFunAudioCB = CallBackFunc;
}

//����¼���ɼ���������Ƶ����
long CDevice_DH::EDvr_StartAudioDataCapture()
{
    if (!CLIENT_RecordStart())
        return -1;
    return 0;
}

//�ر�¼���ɼ���������Ƶ����
void CDevice_DH::EDvr_StopAudioDataCapture()
{
    CLIENT_RecordStop();
}

/*������Ƶ*/
long CDevice_DH::EDvr_OpenSound(char *pstrIP, long nChID)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,nChID);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return -1;
    /*������Ƶ*/
    if(!CLIENT_OpenSound(sChannel->m_nHandle))
        return -1;
    return 0;
}

/*�ر���Ƶ*/
void CDevice_DH::EDvr_CloseSound()
{
    CLIENT_CloseSound();
}

/*�����������豸*/
long CDevice_DH::EDvr_AudioDataSend(char *pstrIP, char *pSendBuff, long nBuffSize)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�����Խ��Ƿ���*/
    if (0 == temp->m_nAudioHandle)
        return -1;
    if(!CLIENT_TalkSendData(temp->m_nAudioHandle, pSendBuff, (DWORD)nBuffSize))
        return -1;
    return 0;
}

/*������豸�õ�����������*/
void CDevice_DH::EDvr_AudioDecode(char *pSendBuff, long nBuffSize)
{
    CLIENT_AudioDec(pSendBuff, nBuffSize);
}

/*��������*/
long CDevice_DH::EDvr_SetAudioVolume(char *pstrIP, unsigned short wVolume)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�����Խ��Ƿ���*/
    if (0 == temp->m_nAudioHandle)
        return -1;
    /*��������*/
    CLIENT_SetAudioClientVolume(temp->m_nAudioHandle, wVolume);
    return 0;
}


/*������ͨ������Ϣ�ص�*/
void CDevice_DH::EDvr_SetCommAlarmCallBackFunc(fCommAlarmCB CallBackFunc)
{
    m_pFunCommAlarmCB = CallBackFunc;
}

//�����ƶ���ⱨ���ص�
void CDevice_DH::EDvr_SetMotionDectionCallBackFunc(fMotionDectionCB CallBackFunc)
{
    m_pFuncMotionDectionCB = CallBackFunc;
}

//������Ƶ��ʧ�����ص�
void CDevice_DH::EDvr_SetVideoLostCallBackFunc(fVideoLostCB CallBackFunc)
{
    m_pFuncVideoLostCB = CallBackFunc;
}

/*������Ƶ�ڵ���Ϣ�ص�*/
void CDevice_DH::EDvr_SetShelterAlarmCallBackFunc(fShelterAlarmCB CallBackFunc)
{
    m_pFunShelterAlarmCB = CallBackFunc;
}

/*����Ӳ������Ϣ�ص�*/
void CDevice_DH::EDvr_SetDiskFullCallBackFunc(fDiskFullCB CallBackFunc)
{
    m_pFunDiskFullCB = CallBackFunc;
}

/*����Ӳ�̴�����Ϣ�ص�*/
void CDevice_DH::EDvr_SetDiskErrorCallBackFunc(fDiskErrorCB CallBackFunc)
{
    m_pFunDiskErrorCB = CallBackFunc;
}

/*������Ƶ�����Ϣ�ص�*/
void CDevice_DH::EDvr_SetSoundDetectCallBackFunc(fSoundDetectCB CallBackFunc)
{
    m_pFunSoundDetectCB = CallBackFunc;
}

long CDevice_DH::EDvr_SetNormalAlarmCFG(char *pstrIP, long nAlarmKind, long nAlarmChannel, long nAlarmInfo, long nAlarmEnable)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�õ�������Ϣ*/
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
    /*�޸���Ҫ���õĲ���*/
    switch(nAlarmKind)
    {
    case 0://��������
        almCFG->struLocalAlmIn[nAlarmChannel].byAlarmType = (BYTE)nAlarmInfo;
        almCFG->struLocalAlmIn[nAlarmChannel].byAlarmEn = (BYTE)nAlarmEnable;
        break;
    case 1://�ƶ����
        almCFG->struMotion[nAlarmChannel].wSenseLevel = (WORD)nAlarmInfo;
        almCFG->struMotion[nAlarmChannel].byMotionEn = (BYTE)nAlarmEnable;
        break;
    case 2://��Ƶ��ʧ
        almCFG->struVideoLost[nAlarmChannel].byAlarmEn = (BYTE)nAlarmEnable;
        break;
    case 3://��Ƶ�ڵ�
        almCFG->struBlind[nAlarmChannel].byBlindLevel = (BYTE)nAlarmInfo;
        almCFG->struBlind[nAlarmChannel].byBlindEnable = (BYTE)nAlarmEnable;
        break;
    case 4://���̱���
        switch(nAlarmInfo)
        {
        case 1://�޴���
            almCFG->struDiskAlarm.byNoDiskEn =  (BYTE)nAlarmEnable;
            break;
        case 2://������
            almCFG->struDiskAlarm.byLowCapEn =  (BYTE)nAlarmEnable;
            break;
        case 3://���̹���
            almCFG->struDiskAlarm.byDiskErrEn =  (BYTE)nAlarmEnable;
            break;
        default:
            break;
        }
        break;
        default:
            break;
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*��ȡ�ⲿ��������״̬*/
long CDevice_DH::EDvr_GetMsgAlarmInEnable(char *pstrIP, bool *pIsEnable, long *pAlarmInCount)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || pAlarmInCount == NULL)
        return -1;
    /*�õ�������Ϣ*/
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
    /*�������ý��*/
    *pAlarmInCount = retlen;
    for(long i=0; i<retlen; i++)
    {
        pIsEnable[i] = ((almCfg[i].state)?true:false);
    }
    return 0;
}

/*�����ⲿ��������״̬*/
long CDevice_DH::EDvr_SetMsgAlarmInEnable(char *pstrIP, const bool *pIsEnable, long nAlarmInCount /*= DH_MAX_ALARM_IN_NUM*/)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || nAlarmInCount == 0)
        return -1;
    ALARM_CONTROL almCfg[16];
    /*�޸���Ҫ���õĲ���*/
    for(long i=0; i<nAlarmInCount; i++)
    {
        almCfg[i].index = (WORD)i;
        almCfg[i].state = pIsEnable[i];
    }
    /*���ò���*/
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

/*��ȡ�������״̬*/
long CDevice_DH::EDvr_GetMsgAlarmOutEnable(char *pstrIP, bool *pIsEnable, long *pAlarmOutCount)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || pAlarmOutCount == 0)
        return -1;
    /*�õ�������Ϣ*/
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
    /*�������ý��*/
    *pAlarmOutCount = retlen;
    for(long i=0; i<retlen; i++)
    {
        pIsEnable[i] = ((almCfg[i].state)?true:false);
    }
    return 0;
}
/*���ñ������״̬*/
long CDevice_DH::EDvr_SetMsgAlarmOutEnable(char *pstrIP, const bool *pIsEnable,	long nAlarmOutCount)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || nAlarmOutCount == 0)
        return -1;
    /*�õ�������Ϣ*/
    ALARM_CONTROL almCfg[16];
    /*�޸���Ҫ���õĲ���*/
    for(long i=0; i<nAlarmOutCount; i++)
    {
        almCfg[i].index = (WORD)i;
        almCfg[i].state = pIsEnable[i];
    }
    /*���ò���*/
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

/*��ȡ��̬��ⱨ������״̬*/
long CDevice_DH::EDvr_GetMsgMotionDectionEnable(char *pstrIP, bool *pIsEnable, long *pVideoCount)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || pVideoCount == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
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

/*���ö�̬��ⱨ������״̬*/
long CDevice_DH::EDvr_SetMsgMotionDectionEnable(char *pstrIP, const bool *pIsEnable, long nVideoCount /*= DH_MAX_VEDIO_IN_NUM*/)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || nVideoCount == 0)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    for(long i=0; i<nVideoCount; i++)
    {
        almCFG->struMotion[i].byMotionEn = pIsEnable[i];
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*��ȡ��Ƶ��ʧ����״̬*/
long CDevice_DH::EDvr_GetMsgVedioLostEnable(char *pstrIP, bool *pIsEnable, long *pVideoCount)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || pVideoCount == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
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
/*������Ƶ��ʧ����״̬*/
long CDevice_DH::EDvr_SetMsgVedioLostEnable(char *pstrIP, const bool *pIsEnable, long nVideoCount /*= DH_MAX_VIDEO_IN_NUM*/)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || nVideoCount == 0)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    for(long i=0; i<nVideoCount; i++)
    {
        almCFG->struVideoLost[i].byAlarmEn = pIsEnable[i];
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*��ȡͼ���ڵ�����״̬*/
long CDevice_DH::EDvr_GetMsgBlindEnable(char *pstrIP, bool *pIsEnable, long *pVideoCount)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || pVideoCount == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
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

/*����ͼ���ڵ�����״̬*/
long CDevice_DH::EDvr_SetMsgBlindEnable(char *pstrIP, const bool *pIsEnable, long nVideoCount /*= DH_MAX_VIDEO_IN_NUM*/)
{	
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL || nVideoCount == 0)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    for(long i=0; i<nVideoCount; i++)
    {
        almCFG->struBlind[i].byBlindEnable = pIsEnable[i];
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*��ȡӲ����Ϣ����״̬*/
long CDevice_DH::EDvr_GetMsgHardDiskEnable(char *pstrIP, long nConfigType, bool *pIsEnable)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(pIsEnable == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
    switch(nConfigType)
    {
    case 0:	//��Ӳ��
        if(!almCFG->struDiskAlarm.byNoDiskEn)
            *pIsEnable = false;
        else
            *pIsEnable = true;
        break;
    case 1:	//��������
        if(!almCFG->struDiskAlarm.byLowCapEn)
            *pIsEnable = false;
        else
            *pIsEnable = true;
        break;
    case 2:	//Ӳ�̴���
        if(!almCFG->struDiskAlarm.byDiskErrEn)
            *pIsEnable = false;
        else
            *pIsEnable = true;
        break;
    default:return -1;
    }
    return 0;
}

/*����Ӳ����Ϣ����״̬*/
long CDevice_DH::EDvr_SetMsgHardDiskEnable(char *pstrIP, long nConfigType, bool bIsEnable)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    switch(nConfigType)
    {
    case 0:	//��Ӳ��
        almCFG->struDiskAlarm.byNoDiskEn = bIsEnable;
        break;
    case 1:	//��������
        almCFG->struDiskAlarm.byLowCapEn = bIsEnable;
        break;
    case 2:	//Ӳ�̴���
        almCFG->struDiskAlarm.byDiskErrEn = bIsEnable;
        break;
    default:return -1;
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

/*����ƴ����������*/
long CDevice_DH::EDvr_StartMultiLink(char*pstrIP, long nLinkType, HWND hWin)
{
    DWORD dwIP = inet_addr(pstrIP);
    /*�ж���������Ϸ�*/
    if(dwIP == INADDR_NONE)
        return -1;
    /*ͨ���Ƿ��Ѿ���ͨ*/
    if(m_ChannelList.SearchChannel(pstrIP,DH_MAX_CHANNUM+1) != NULL)
        return -2;
    /*������ʱ�ռ䱣����Ϣ*/
    SChannelInfo_DH temp;
    StructInit(temp);
    temp.m_dwIP = dwIP;
    temp.m_nID = DH_MAX_CHANNUM+1;
    temp.m_nChannelType = 1;
    /*�豸�Ƿ��ѵ�½*/
    if(m_DeviceList.SearchDevice(pstrIP) == NULL)
        return -1;
    /*����ָ����������*/
    temp.m_nHandle = CLIENT_MultiPlay(m_DeviceList.SearchDevice(pstrIP)->m_nHandle, hWin);
    if (temp.m_nHandle == 0)
        return -1;
    /*����ͨ����Ϣ������ͨ����*/
    return m_ChannelList.AddPlayChannel(temp);
}

/*ֹͣƴ����������*/
void CDevice_DH::EDvr_StopMultiLink(char* pstrIP)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,DH_MAX_CHANNUM+1);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*�Ͽ���������*/
    CLIENT_StopMultiPlay(sChannel->m_nHandle);
    /*�ͷ�����ͨ��*/
    m_ChannelList.DeletStopChannel(pstrIP,DH_MAX_CHANNUM+1);
}
void CDevice_DH::EDvr_StopMultiLink(long nRDataLink)
{
    SChannelInfo_DH * sChannel = m_ChannelList.GetChannel(nRDataLink);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return;
    /*�ж��Ƿ�Ϊ��ʱ��������*/
    if(sChannel->m_nChannelType != 1)
        return;
    /*�Ͽ���������*/
    CLIENT_StopMultiPlay(sChannel->m_nHandle);
    /*�ͷ�����ͨ��*/
    m_ChannelList.DeletStopChannel(nRDataLink);
}

/*��ȡͨ������*/
long CDevice_DH::EDvr_GetVideoEffect(char *pstrIP, long nChID, BYTE *nBright, BYTE *nContrast, BYTE *nSaturation, BYTE *nHue, bool *bGainEnable, BYTE *nGain)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==nBright || NULL==nContrast || NULL==nSaturation || NULL==nHue || NULL==bGainEnable || NULL==nGain)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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
/*����ͨ������*/
long CDevice_DH::EDvr_SetVideoEffect(char *pstrIP, long nChID, BYTE nBright, BYTE nContrast, BYTE nSaturation, BYTE nHue, bool bGainEnable, BYTE nGain)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(100<nBright || 100<nContrast || 100<nSaturation || 100<nHue || 100<nGain)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    ChannelCfg[nChID].stColorCfg[0].byBrightness = nBright;
    ChannelCfg[nChID].stColorCfg[0].byContrast = nContrast;
    ChannelCfg[nChID].stColorCfg[0].bySaturation = nSaturation;
    ChannelCfg[nChID].stColorCfg[0].byHue = nHue;
    ChannelCfg[nChID].stColorCfg[0].byGainEn = bGainEnable;
    ChannelCfg[nChID].stColorCfg[0].byGain = nGain;
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}
/*��Ƶ��������*/
long CDevice_DH::EDvr_SetImageEffect(char *pstrIP, long nChID, long nEffecType, long nEffectValue)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(100<nEffectValue)
        return -1;
    /*�õ�������Ϣ*/
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
    /*�޸���Ҫ���õĲ���*/
    switch(nEffecType)
    {
    case 0://����
        ChannelCfg[nChID].stColorCfg[0].byBrightness = (BYTE)nEffectValue;
        break;
    case 1://�Աȶ�
        ChannelCfg[nChID].stColorCfg[0].byContrast = (BYTE)nEffectValue;
        break;
    case 2://���Ͷ�
        ChannelCfg[nChID].stColorCfg[0].bySaturation = (BYTE)nEffectValue;
        break;
    case 3://ɫ��
        ChannelCfg[nChID].stColorCfg[0].byHue = (BYTE)nEffectValue;
        break;
    case 4://����
        ChannelCfg[nChID].stColorCfg[0].byGain = (BYTE)nEffectValue;
        break;
    default:
        break;
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG, -1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}

/*��ȡͨ����Ƶ����*/
long CDevice_DH::EDvr_GetChannelVideoInfo(char *pstrIP, long nChID, long nEncOpt, char *pstrChannelName, bool *bVideoEnable, BYTE *nBitRate, BYTE *nFPS, BYTE *nEncodeMode, BYTE *nImageSize, BYTE *nImageQlty)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrChannelName || NULL==bVideoEnable || NULL==nBitRate || NULL==nFPS || NULL==nEncodeMode || NULL==nImageSize || NULL==nImageQlty)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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
/*����ͨ����Ƶ����*/
long CDevice_DH::EDvr_SetChannelVideoInfo(char *pstrIP, long nChID, long nEncOpt, char *pstrChannelName, bool bVideoEnable, BYTE nBitRate, BYTE nFPS, BYTE nEncodeMode, BYTE nImageSize, BYTE nImageQlty)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
    {
        return -1;
    }
    /*���������ȷ*/
    if(NULL==pstrChannelName)
    {
        return -1;
    }
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
    {
        return -1;
    }
    /*�޸���Ҫ���õĲ���*/
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
    
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
    {
        return -1;
    }
    return 0;
}

/*��Ƶ�ֱ�������*/
long CDevice_DH::EDvr_SetImageZoom(char *pstrIP, long nChID, long nEncKind, long nImageZoom, long nZoonWidth, long nZoomHeigh, long nFPS)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
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
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}
/*��������������*/
long CDevice_DH::EDvr_SetImageDefine(char *pstrIP, long nChID, long nEncKind, long nBitRateType, long nBitRate, long nImageQlty, long nEncodeMode)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
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
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}

long CDevice_DH::EDvr_SetChannelName(char *pstrIP, long nChType, long nChID, char *pstrName)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL == pstrName)
        return -1;
    /*�õ�������Ϣ*/
    switch(nChType)
    {
    case 0:
        {
            DWORD retlen = 0;
            DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
            if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
                return -1;
            /*�޸���Ҫ���õĲ���*/
            memcpy(ChannelCfg[nChID].szChannelName,pstrName,DH_CHAN_NAME_LEN);
            /*���ò���*/
            if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
                return -1;
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/*��ȡͨ����Ƶ����*/
long CDevice_DH::EDvr_GetChannelAudioInfo(char *pstrIP, long nChID, long nEncOpt, bool *bAudioEnable, BYTE *nFormatTag, WORD *nTrackCount, WORD *nBitsPerSample, DWORD *nSamplesPerSec)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==bAudioEnable)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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
    //��������
    if(NULL!=nFormatTag || NULL!=nTrackCount || NULL!=nBitsPerSample || NULL!=nSamplesPerSec)
    {
        *nFormatTag = pVideoEncOpt->wFormatTag;
        *nTrackCount = pVideoEncOpt->nChannels;
        *nBitsPerSample = pVideoEncOpt->wBitsPerSample;
        *nSamplesPerSec = pVideoEncOpt->nSamplesPerSec;
    }
    return 0;
}

/*��ȡͨ��OSD����*/
long CDevice_DH::EDvr_GetChannelOsdInfo(char *pstrIP, long nChID, long OsdOpt, DWORD *nFrontColor, DWORD *nBackColor, RECT *rcRecr, bool *bOsdShow)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==nFrontColor || NULL==nBackColor || NULL==rcRecr || NULL==bOsdShow)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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
/*����ͨ��OSD����*/
long CDevice_DH::EDvr_SetChannelOsdInfo(char *pstrIP, long nChID, long OsdOpt, DWORD nFrontColor, DWORD nBackColor, const RECT *rcRecr, bool bOsdShow)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_CHANNEL_CFG* ChannelCfg = m_ChannelCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_CHANNELCFG, -1, ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
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
    
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_CHANNELCFG,-1,ChannelCfg,DH_MAX_CHANNUM*sizeof(DHDEV_CHANNEL_CFG)))
        return -1;
    return 0;
}

/*��ȡ485����Э���б�*/
long CDevice_DH::EDvr_Get485PorList(char *pstrIP, DWORD *n485PorCount, char *str485PorList)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==n485PorCount || NULL==str485PorList)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*���ؽ��*/
    *n485PorCount = ComCfg.dwDecProListNum;
    for(DWORD i=0; i<*n485PorCount; i++)
    {
        strcpy((str485PorList+i*DH_MAX_NAME_LEN), ComCfg.DecProName[i]);
    }
    return 0;
}

/*��ȡ232���ڹ����б�*/
long CDevice_DH::EDvr_Get232FuncList(char *pstrIP, DWORD *n232FuncCount, char *str232FuncList)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==n232FuncCount || NULL==str232FuncList)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*���ؽ��*/
    *n232FuncCount = ComCfg.dw232FuncNameNum;
    for(DWORD i=0; i<*n232FuncCount; i++)
    {
        strcpy((str232FuncList+i*DH_MAX_NAME_LEN), ComCfg.s232FuncName[i]);
    }
    return 0;
}

/*��ȡ458��������*/
long CDevice_DH::EDvr_Get485Info(char *pstrIP, long nChID, long *n485Por, DWORD *nCOMInfo, BYTE *n485Address)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==n485Por || NULL==nCOMInfo || NULL==n485Address || 0>nChID || DH_MAX_CHANNUM<nChID)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*���ؽ��*/
    *n485Por = ComCfg.stDecoder[nChID].wProtocol;
    //*nCOMInfo = ComCfg.stDecoder[nChID].struComm;
    //DH_COMM_PROP *pCommInfo = &(ComCfg.stDecoder[nChID].struComm);
    //*nCOMInfo = *((DWORD*)pCommInfo);
    memcpy(nCOMInfo,&(ComCfg.stDecoder[nChID].struComm),sizeof(DH_COMM_PROP));
    *n485Address = (BYTE)(ComCfg.stDecoder[nChID].wDecoderAddress);
    return 0;
}
/*����458��������*/
long CDevice_DH::EDvr_Set485Info(char *pstrIP, long nChID, long n485Por, DWORD nCOMInfo, BYTE n485Address)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(0>n485Por || 0>nChID || DH_MAX_CHANNUM<nChID)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    ComCfg.stDecoder[nChID].wProtocol = (BYTE)n485Por;
    //DWORD *pCommInfo = &nCOMInfo;
    //ComCfg.stDecoder[nChID].struComm = *((DH_COMM_PROP*)pCommInfo);
    memcpy(&(ComCfg.stDecoder[nChID].struComm),&nCOMInfo,sizeof(DH_COMM_PROP));
    ComCfg.stDecoder[nChID].wDecoderAddress = n485Address;
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_COMMCFG,0,&ComCfg,sizeof(DHDEV_COMM_CFG)))
        return -1;
    return 0;
}

/*��ȡ232��������*/
long CDevice_DH::EDvr_Get232Info(char *pstrIP, long n232Index, long *n232Func, DWORD *nCOMInfo)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==n232Func || NULL==nCOMInfo || 0>n232Index || DH_MAX_232_NUM<n232Index)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*���ؽ��*/
    *n232Func = ComCfg.st232[n232Index].byFunction;
    //DH_COMM_PROP *pCommInfo = &(ComCfg.st232[n232Index].struComm);
    //*nCOMInfo = *((DWORD*)pCommInfo);
    memcpy(nCOMInfo,&(ComCfg.st232[n232Index].struComm),sizeof(DH_COMM_PROP));
    return 0;
}
/*����232��������*/
long CDevice_DH::EDvr_Set232Info(char *pstrIP, long n232Index, long n232Func, DWORD nCOMInfo)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(0>n232Func || 0>n232Index || DH_MAX_232_NUM<n232Index)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_COMM_CFG ComCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_COMMCFG, 0, &ComCfg,sizeof(DHDEV_COMM_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    ComCfg.st232[n232Index].byFunction = (BYTE)n232Func;
    //DWORD *pCommInfo = &nCOMInfo;
    //ComCfg.st232[n232Index].struComm = *((DH_COMM_PROP*)pCommInfo);
    memcpy(&(ComCfg.st232[n232Index].struComm),&nCOMInfo,sizeof(DH_COMM_PROP));
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_COMMCFG,0,&ComCfg,sizeof(DHDEV_COMM_CFG)))
        return -1;
    return 0;
}

/*��ȡ����˿�����*/
long CDevice_DH::EDvr_GetNetPortInfo(char *pstrIP, char *pstrDeviceName, WORD *nTCPCount, WORD *nTCPPort, WORD *nUDPPort, WORD *nHTTPPort, WORD *HTTPSPort, WORD *nSSLPort)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrDeviceName || NULL==nTCPCount || NULL==nTCPPort || NULL==nUDPPort || NULL==nHTTPPort ||
        NULL==HTTPSPort || NULL==nSSLPort)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*���ؽ��*/
    strcpy(pstrDeviceName, NetCfg.sDevName);
    *nTCPCount = NetCfg.wTcpMaxConnectNum;
    *nTCPPort = NetCfg.wTcpPort;
    *nUDPPort = NetCfg.wUdpPort;
    *nHTTPPort = NetCfg.wHttpPort;
    *HTTPSPort = NetCfg.wHttpsPort;
    *nSSLPort = NetCfg.wSslPort;
    return 0;
}
/*��������˿�����*/
long CDevice_DH::EDvr_SetNetPortInfo(char *pstrIP, const char *pstrDeviceName, WORD nTCPCount, WORD nTCPPort, WORD nUDPPort, WORD nHTTPPort, WORD HTTPSPort, WORD nSSLPort)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL == pstrDeviceName)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    strcpy(NetCfg.sDevName, pstrDeviceName);
    NetCfg.wTcpMaxConnectNum = nTCPCount;
    NetCfg.wTcpPort = nTCPPort;
    NetCfg.wUdpPort = nUDPPort;
    NetCfg.wHttpPort = nHTTPPort;
    NetCfg.wHttpsPort = HTTPSPort;
    NetCfg.wSslPort = nSSLPort;
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*��ȡ��̫��������*/
long CDevice_DH::EDvr_GetEthernetInfo(char *pstrIP, long nEthernetIndex, char *pstrEthernetIP, char *EthernetMask, char *GatewayIP, long *nNetMode, char *pstrMAC)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrEthernetIP || NULL==EthernetMask || NULL==GatewayIP || NULL==nNetMode || NULL==pstrMAC ||
        0>nEthernetIndex || DH_MAX_ETHERNET_NUM<nEthernetIndex)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*���ؽ��*/
    strcpy(pstrEthernetIP, NetCfg.stEtherNet[nEthernetIndex].sDevIPAddr);
    strcpy(EthernetMask, NetCfg.stEtherNet[nEthernetIndex].sDevIPMask);
    strcpy(GatewayIP, NetCfg.stEtherNet[nEthernetIndex].sGatewayIP);
    *nNetMode = (long)NetCfg.stEtherNet[nEthernetIndex].dwNetInterface;
    strcpy(pstrMAC, NetCfg.stEtherNet[nEthernetIndex].byMACAddr);
    return 0;
}
/*������̫��������*/
long CDevice_DH::EDvr_SetEthernetInfo(char *pstrIP, long nEthernetIndex, const char *pstrEthernetIP, const char *EthernetMask, const char *GatewayIP, long nNetMode)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrEthernetIP || NULL==EthernetMask || NULL==GatewayIP || 0>=nNetMode || 0>nEthernetIndex ||
        DH_MAX_ETHERNET_NUM<nEthernetIndex)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    strcpy(NetCfg.stEtherNet[nEthernetIndex].sDevIPAddr, pstrEthernetIP);
    strcpy(NetCfg.stEtherNet[nEthernetIndex].sDevIPMask, EthernetMask);
    strcpy(NetCfg.stEtherNet[nEthernetIndex].sGatewayIP, GatewayIP);
    NetCfg.stEtherNet[nEthernetIndex].dwNetInterface = (BYTE)nNetMode;
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*��ȡԶ�̷���������*/
long CDevice_DH::EDvr_GetRemoteHostInfo(char *pstrIP, long nHostType, char *pstrHostIP, WORD *nHostPort,bool *bIsEnable,
                                        char *pstrUserName, char *pstrPassWord, char *pstrHostName)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrHostIP || NULL==nHostPort || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrHostName ||
        NULL==bIsEnable)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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
/*����Զ�̷���������*/
long CDevice_DH::EDvr_SetRemoteHostInfo(char *pstrIP, long nHostType, const char *pstrHostIP, WORD nHostPort,
                                        bool bIsEnable, const char *pstrUserName, const char *pstrPassWord, const char *pstrHostName)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrHostIP || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrHostName)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
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
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*��ȡ�ʼ�����������*/
long CDevice_DH::EDvr_GetMailHostInfo(char *pstrIP, char *pstrHostIP, WORD *nHostPort, char *pstrUserName,
                                      char *pstrPassWord, char *pstrDestAddr, char *pstrCcAddr, char *pstrBccAddr, char *pstrSubject)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrHostIP || NULL==nHostPort || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrDestAddr ||
        NULL==pstrCcAddr || NULL==pstrBccAddr || NULL==pstrSubject)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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
/*�����ʼ�����������*/
long CDevice_DH::EDvr_SetMailHostInfo(char *pstrIP,char *pstrHostIP,WORD nHostPort,const char *pstrUserName,
                                      const char *pstrPassWord,const char *pstrDestAddr,const char *pstrCcAddr,
                                      const char *pstrBccAddr,const char *pstrSubject)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrHostIP || NULL==pstrUserName || NULL==pstrPassWord || NULL==pstrDestAddr || NULL==pstrCcAddr ||
        NULL==pstrBccAddr || NULL==pstrSubject)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_NET_CFG NetCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_NETCFG, 0, &NetCfg,sizeof(DHDEV_NET_CFG),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    strcpy(NetCfg.struMail.sMailIPAddr, pstrHostIP);
    NetCfg.struMail.wMailPort = nHostPort;
    strcpy(NetCfg.struMail.sUserName, pstrUserName);
    strcpy(NetCfg.struMail.sUserPsw, pstrPassWord);
    strcpy(NetCfg.struMail.sDestAddr, pstrDestAddr);
    strcpy(NetCfg.struMail.sCcAddr, pstrCcAddr);
    strcpy(NetCfg.struMail.sBccAddr, pstrBccAddr);
    strcpy(NetCfg.struMail.sSubject, pstrSubject);
    /*��������*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_NETCFG,0,&NetCfg,sizeof(DHDEV_NET_CFG)))
        return -1;
    return 0;
}

/*��ȡ�豸��������//��Ƶ��,��Ƶ��,��������,�������,�����,USB��,IDE��,����,����*/
long CDevice_DH::EDvr_GetDeviceAttribute(char *pstrIP, BYTE *pDeviceAttributes, long *pnAttributeSize)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pDeviceAttributes || NULL==pnAttributeSize)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_SYSTEM_ATTR_CFG DeviceCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle, DH_DEV_DEVICECFG, 0, &DeviceCfg,sizeof(DHDEV_SYSTEM_ATTR_CFG),&retlen))
        return -1;
    /*���ؽ��*/
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

/*����I֡*/
long CDevice_DH::EDvr_CaptureIFrame(char *pstrIP, long nChID)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
        /*
        //�ж���������ͨ���Ƿ��
        SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(pstrIP,nChID);
        if(sChannel == NULL)
        return -1;
        //�ж��Ƿ�Ϊ��ʱ��������
        if(sChannel->m_nChannelType != 1)
        return -1;
    */
    //ǿ��I֡
    if(!CLIENT_MakeKeyFrame(temp->m_nHandle,nChID))
        return -1;
    return 0;
}

/*��ȡ�û�Ȩ���б�*/
long CDevice_DH::EDvr_GetUserRightList(char *pstrIP, fUserRightCB lpFunCB)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL == lpFunCB)
        return -1;
    /*�õ�������Ϣ*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*���ؽ��*/
    for(unsigned i=0; i<ManageInfo.dwRightNum; i++)
    {
        lpFunCB(pstrIP, ManageInfo.rightList[i].dwID, ManageInfo.rightList[i].name, ManageInfo.rightList[i].memo);
    }
    return 0;
}

/*�û�����Ϣ����*/
long CDevice_DH::EDvr_GetUserGroupList(char *pstrIP, fUserGroupCB lpFunCB)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL == lpFunCB)
        return -1;
    /*�õ�������Ϣ*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*���ؽ��*/
    for(unsigned i=0; i<ManageInfo.dwGroupNum; i++)
    {
        lpFunCB(pstrIP, ManageInfo.groupList[i].dwID, ManageInfo.groupList[i].name,ManageInfo.groupList[i].dwRightNum
            ,(long*)(ManageInfo.groupList[i].rights), ManageInfo.groupList[i].memo);
    }
    return 0;
}
long CDevice_DH::EDvr_GetUserGroup(char *pstrIP, char *pstrGroupName, long *nGroupID, long *nRightNum, long *pnRightList, char *pstrGroupMemo)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrGroupName || NULL==nGroupID || NULL==nRightNum || NULL==pnRightList || NULL==pstrGroupMemo)
        return -1;
    /*�õ�������Ϣ*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*���ؽ��*/
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

/*��ȡ�û���Ϣ*/
long CDevice_DH::EDvr_GetUserInfoList(char *pstrIP, fUserInfoCB lpFunCB)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL == lpFunCB)
        return -1;
    /*�õ�������Ϣ*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*���ؽ��*/
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
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrUserName || NULL==nUserID || NULL==nGroupID || NULL==pstrPassword || NULL==nRightNum || NULL==pnRightList || NULL==pstrUserMemo)
        return -1;
    /*�õ�������Ϣ*/
    USER_MANAGE_INFO ManageInfo;
    if(!CLIENT_QueryUserInfo(temp->m_nHandle,&ManageInfo))
        return -1;
    /*���ؽ��*/
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

/*�����û���*/
long CDevice_DH::EDvr_AddUserGroup(char *pstrIP, long nGroupID, char *pstrGroupName, long nRightNum, long *pnRightList, char *pstrGroupMemo)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*��¼�������*/
    USER_GROUP_INFO GroupInfo;
    memset(&GroupInfo,0,sizeof(USER_GROUP_INFO));
    GroupInfo.dwID = nGroupID;
    GroupInfo.dwRightNum = nRightNum;
    strcpy(GroupInfo.name,pstrGroupName);
    strcpy(GroupInfo.memo,pstrGroupMemo);
    memcpy(GroupInfo.rights,pnRightList,sizeof(long)*nRightNum);
    /*����û���*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,0,&GroupInfo,NULL))
        return -1;
    return 0;
}

/*�����û�*/
long CDevice_DH::EDvr_AddUser(char *pstrIP,long nUserID,char *pstrUserName,long nGroupID,char *pstrPassword,long nRightNum,long *pnRightList,char *pstrUserMemo)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*��¼�������*/
    USER_INFO newUserInfo;
    memset(&newUserInfo,0,sizeof(USER_INFO));
    newUserInfo.dwID = nUserID;
    newUserInfo.dwGroupID = nGroupID;
    newUserInfo.dwRightNum = nRightNum;
    strcpy(newUserInfo.name, pstrUserName);
    strcpy(newUserInfo.passWord, pstrPassword);
    strcpy(newUserInfo.memo, pstrUserMemo);
    memcpy(newUserInfo.rights, pnRightList,sizeof(long)*nRightNum);
    /*����û���*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,3,&newUserInfo,NULL))
        return -1;
    return 0;
}

/*ɾ���û���*/
long CDevice_DH::EDvr_DeletUserGroup(char *pstrIP, char *pstrGroupName)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*��¼�������*/
    USER_GROUP_INFO GroupInfo;
    strcpy(GroupInfo.name,pstrGroupName);
    /*ɾ���û���*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,1,&GroupInfo,NULL))
        return -1;
    return 0;
}

/*ɾ���û�*/
long CDevice_DH::EDvr_DeleteUser(char *pstrIP, char *pstrUserName, char *pstrPassword)
{
    /*�豸�Ƿ��ѵ�½*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrUserName)
        return -1;
    /*�õ�������Ϣ*/
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
    /*ɾ���û�*/
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,4,&ManageInfo,NULL))
        return -1;
    return 0;
}

/*�༭�û���*/
long CDevice_DH::EDvr_EditUserGroup(char *pstrIP, char *pstrGroupName, long nGroupID, char *pstrNewName, long nRightNum, long *pnRightList, char *pstrGroupMemo)
{
    /*�豸�Ƿ��ѵ�½*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrGroupName)
        return -1;
    /*�õ�������Ϣ*/
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
    /*�޸��û���*/
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

/*�༭�û�*/
long CDevice_DH::EDvr_EditUser(char *pstrIP, char *pstrUserName, long nUserID, char *pstrNewName, long nGroupID, char *pstrPassword, long nRightNum, long *pnRightList, char *pstrUserMemo)
{
    /*�豸�Ƿ��ѵ�½*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrUserName)
        return -1;
    /*�õ�������Ϣ*/
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
    /*�޸��û���*/
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
    /*�豸�Ƿ��ѵ�½*/
    unsigned i;
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*���������ȷ*/
    if(NULL==pstrUserName)
        return -1;
    /*�õ�������Ϣ*/
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
    /*��������*/
    strcpy(ManageInfo.userList[i].passWord, pstrOldPassword);
    /*�޸�����*/
    USER_INFO newUserInfo;
    memcpy(&newUserInfo, ManageInfo.userList+i,sizeof(USER_INFO));
    strcpy(newUserInfo.passWord, pstrNewPassword);
    
    if(!CLIENT_OperateUserInfo(temp->m_nHandle,6,&newUserInfo,ManageInfo.userList+i))
        return -1;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// �����ӿ���ϸ�����װ

//������������
long CDevice_DH::EDvr_GetAlarmNormalInfo(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nAlarmEnable, BYTE *nValue1, BYTE *nValue2,long *nValue3)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || nAlarmEnable == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
    int i;
    switch(nAlarmType)
    {
    case 0://0�źű���
        *nAlarmEnable = almCFG->struLocalAlmIn[nChID].byAlarmEn;
        *nValue1 = (BYTE)(almCFG->struLocalAlmIn[nChID].byAlarmType?0:1);
        break;
    case 1://1�ƶ����
        *nAlarmEnable = almCFG->struMotion[nChID].byMotionEn;
        *nValue1 = (BYTE)(almCFG->struMotion[nChID].wSenseLevel);
        *nValue3 = 18*22;
        for(i=0; i<18; i++)
        {
            memcpy(nValue2+(i*22),almCFG->struMotion[nChID].byDetected[i],22);
        }
        break;
    case 2://2��Ƶ��ʧ
        *nAlarmEnable = almCFG->struVideoLost[nChID].byAlarmEn;
        break;
    case 3://3��Ƶ�ڵ�
        *nAlarmEnable = almCFG->struBlind[nChID].byBlindEnable;
        *nValue1 = almCFG->struBlind[nChID].byBlindLevel;
        break;
    case 4://4�޴��̱���
        *nAlarmEnable = almCFG->struDiskAlarm.byNoDiskEn;
        break;
    case 5://5����������
        *nAlarmEnable = almCFG->struDiskAlarm.byLowCapEn;
        break;
    case 6://6���̹��ϱ���
        *nAlarmEnable = almCFG->struDiskAlarm.byNoDiskEn;
        break;
    default:break;
    }
    return 0;
}

//���������������
long CDevice_DH::EDvr_GetAlarmOut(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nOutEnable, BYTE *pAlarmOut, long *nAlarmOutSize)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmOut == NULL || nAlarmOutSize == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
    switch(nAlarmType)
    {
    case 0://0�źű���
        *nOutEnable = (BYTE)((almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struLocalAlmIn[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 1://1�ƶ����
        *nOutEnable = (BYTE)((almCFG->struMotion[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struMotion[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 2://2��Ƶ��ʧ
        *nOutEnable = (BYTE)((almCFG->struVideoLost[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struVideoLost[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 3://3��Ƶ�ڵ�
        *nOutEnable = (BYTE)((almCFG->struBlind[nChID].struHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struBlind[nChID].struHandle.byRelAlarmOut,16);
        break;
    case 4://4�޴��̱���
        *nOutEnable = (BYTE)((almCFG->struDiskAlarm.struNDHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struDiskAlarm.struNDHandle.byRelAlarmOut,16);
        break;
    case 5://5����������
        *nOutEnable = (BYTE)((almCFG->struDiskAlarm.struLCHandle.dwActionFlag & 0x00000040)?1:0);
        memcpy(pAlarmOut,almCFG->struDiskAlarm.struLCHandle.byRelAlarmOut,16);
        break;
    case 6://6���̹��ϱ���
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
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmOut == NULL || nAlarmOutSize > 16)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    switch(nAlarmType)
    {
    case 0://0�źű���
        BitSet(almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struLocalAlmIn[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 1://1�ƶ����
        BitSet(almCFG->struMotion[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struMotion[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 2://2��Ƶ��ʧ
        BitSet(almCFG->struVideoLost[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struVideoLost[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 3://3��Ƶ�ڵ�
        BitSet(almCFG->struBlind[nChID].struHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struBlind[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 4://4�޴��̱���
        BitSet(almCFG->struDiskAlarm.struNDHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struDiskAlarm.struNDHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 5://5����������
        BitSet(almCFG->struDiskAlarm.struLCHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struDiskAlarm.struLCHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    case 6://6���̹��ϱ���
        BitSet(almCFG->struDiskAlarm.struEDHandle.dwActionFlag,7,nOutEnable);
        memcpy(almCFG->struLocalAlmIn[nChID].struHandle.byRelAlarmOut,pAlarmOut,nAlarmOutSize);
        break;
    default:break;
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

//��������¼������
long CDevice_DH::EDvr_GetAlarmRecord(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nRecordEnable, long *nPreRecLen, BYTE *pAlarmRecord, long *nAlarmRecordSize)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmRecord == NULL || nAlarmRecordSize == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
    switch(nAlarmType)
    {
    case 0://0�źű���
        *nRecordEnable = (BYTE)((almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struLocalAlmIn[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struLocalAlmIn[nChID].struHandle.dwRecLatch;
        break;
    case 1://1�ƶ����
        *nRecordEnable = (BYTE)((almCFG->struMotion[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struMotion[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struMotion[nChID].struHandle.dwRecLatch;
        break;
    case 2://2��Ƶ��ʧ
        *nRecordEnable = (BYTE)((almCFG->struVideoLost[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struVideoLost[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struVideoLost[nChID].struHandle.dwRecLatch;
        break;
    case 3://3��Ƶ�ڵ�
        *nRecordEnable = (BYTE)((almCFG->struBlind[nChID].struHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struBlind[nChID].struHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struBlind[nChID].struHandle.dwRecLatch;
        break;
    case 4://4�޴��̱���
        *nRecordEnable = (BYTE)((almCFG->struDiskAlarm.struNDHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struDiskAlarm.struNDHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struDiskAlarm.struNDHandle.dwRecLatch;
        break;
    case 5://5����������
        *nRecordEnable = (BYTE)((almCFG->struDiskAlarm.struLCHandle.dwActionFlag & 0x00000002)?1:0);
        memcpy(pAlarmRecord,almCFG->struDiskAlarm.struLCHandle.byRecordChannel,16);
        *nPreRecLen = almCFG->struDiskAlarm.struLCHandle.dwRecLatch;
        break;
    case 6://6���̹��ϱ���
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
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pAlarmRecord == NULL || nAlarmRecordSize > 16)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    switch(nAlarmType)
    {
    case 0://0�źű���
        BitSet(almCFG->struLocalAlmIn[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struLocalAlmIn[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struLocalAlmIn[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 1://1�ƶ����
        BitSet(almCFG->struMotion[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struMotion[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struMotion[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 2://2��Ƶ��ʧ
        BitSet(almCFG->struVideoLost[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struVideoLost[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struVideoLost[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 3://3��Ƶ�ڵ�
        BitSet(almCFG->struBlind[nChID].struHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struBlind[nChID].struHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struBlind[nChID].struHandle.dwRecLatch = nPreRecLen;
        break;
    case 4://4�޴��̱���
        BitSet(almCFG->struDiskAlarm.struNDHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struDiskAlarm.struNDHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struDiskAlarm.struNDHandle.dwRecLatch = nPreRecLen;
        break;
    case 5://5����������
        BitSet(almCFG->struDiskAlarm.struLCHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struDiskAlarm.struLCHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struDiskAlarm.struLCHandle.dwRecLatch = nPreRecLen;
        break;
    case 6://6���̹��ϱ���
        BitSet(almCFG->struDiskAlarm.struEDHandle.dwActionFlag,2,nRecordEnable);
        memcpy(almCFG->struDiskAlarm.struEDHandle.byRecordChannel,pAlarmRecord,nAlarmRecordSize);
        almCFG->struDiskAlarm.struEDHandle.dwRecLatch = nPreRecLen;
        break;
    default:break;
    }
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}

//����������̨
long CDevice_DH::EDvr_GetAlarmPTZ(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *nPTZEnable, BYTE *nPTZType, BYTE *nPTZNo, long *nPTZSize)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || nPTZNo == NULL || nPTZNo == NULL || nPTZSize ==NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
    DH_MSG_HANDLE* pMsgHande = NULL;
    switch(nAlarmType)
    {
    case 0://0�źű���
        pMsgHande = &(almCFG->struLocalAlmIn[nChID].struHandle);
        break;
    case 1://1�ƶ����
        pMsgHande = &(almCFG->struMotion[nChID].struHandle);
        break;
    case 2://2��Ƶ��ʧ
        pMsgHande = &(almCFG->struVideoLost[nChID].struHandle);
        break;
    case 3://3��Ƶ�ڵ�
        pMsgHande = &(almCFG->struBlind[nChID].struHandle);
        break;
    case 4://4�޴��̱���
        pMsgHande = &(almCFG->struDiskAlarm.struNDHandle);
        break;
    case 5://5����������
        pMsgHande = &(almCFG->struDiskAlarm.struLCHandle);
        break;
    case 6://6���̹��ϱ���
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
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || nPTZType == NULL || nPTZNo == NULL || nPTZSize > 16)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�޸���Ҫ���õĲ���*/
    DH_MSG_HANDLE* pMsgHande = NULL;
    switch(nAlarmType)
    {
    case 0://0�źű���
        pMsgHande = &(almCFG->struLocalAlmIn[nChID].struHandle);
        break;
    case 1://1�ƶ����
        pMsgHande = &(almCFG->struMotion[nChID].struHandle);
        break;
    case 2://2��Ƶ��ʧ
        pMsgHande = &(almCFG->struVideoLost[nChID].struHandle);
        break;
    case 3://3��Ƶ�ڵ�
        pMsgHande = &(almCFG->struBlind[nChID].struHandle);
        break;
    case 4://4�޴��̱���
        pMsgHande = &(almCFG->struDiskAlarm.struNDHandle);
        break;
    case 5://5����������
        pMsgHande = &(almCFG->struDiskAlarm.struLCHandle);
        break;
    case 6://6���̹��ϱ���
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
    
    /*���ò���*/
    if(!CLIENT_SetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE)))
        return -1;
    return 0;
}


//���������������ÿ���
long CDevice_DH::EDvr_GetAlarmOtherEn(char *pstrIP, long nChID, BYTE nAlarmType, BYTE *pOtherEnable, long *nOtherSize)
{
    /*�豸�Ƿ��ѵ�½*/
    SDeviceInfo_DH * temp = m_DeviceList.SearchDevice(pstrIP);
    if(NULL == temp)
        return -1;
    /*�жϱ����������ȷ*/
    if(nChID<0 || nChID>DH_MAX_ALARM_IN_NUM || pOtherEnable == NULL || nOtherSize == NULL)
        return -1;
    /*�õ�������Ϣ*/
    DWORD retlen = 0;
    DHDEV_ALARM_SCHEDULE* almCFG = &m_AlarmCfg;
    if(!CLIENT_GetDevConfig(temp->m_nHandle,DH_DEV_ALARMCFG,0,almCFG,sizeof(DHDEV_ALARM_SCHEDULE),&retlen))
        return -1;
    /*�������ý��*/
    DH_MSG_HANDLE *pTempHandle = NULL;
    switch(nAlarmType)
    {
    case 0://0�źű���
        pTempHandle = &(almCFG->struLocalAlmIn[nChID].struHandle);
        break;
    case 1://1�ƶ����
        pTempHandle = &(almCFG->struMotion[nChID].struHandle);
        break;
    case 2://2��Ƶ��ʧ
        pTempHandle = &(almCFG->struVideoLost[nChID].struHandle);
        break;
    case 3://3��Ƶ�ڵ�
        pTempHandle = &(almCFG->struBlind[nChID].struHandle);
        break;
    case 4://4�޴��̱���
        pTempHandle = &(almCFG->struDiskAlarm.struNDHandle);
        break;
    case 5://5����������
        pTempHandle = &(almCFG->struDiskAlarm.struLCHandle);
        break;
    case 6://6���̹��ϱ���
        pTempHandle = &(almCFG->struDiskAlarm.struEDHandle);
        break;
    default:return -1;
    }
    pOtherEnable[0] = (BYTE)((pTempHandle->dwActionFlag & 0x00000001)?1:0);//�ϴ�������
    pOtherEnable[1] = (BYTE)((pTempHandle->dwActionFlag & 0x00000008)?1:0);//�����ʼ�
    pOtherEnable[2] = (BYTE)((pTempHandle->dwActionFlag & 0x00000010)?1:0);//������Ѳ
    pOtherEnable[3] = (BYTE)((pTempHandle->dwActionFlag & 0x00000020)?1:0);//�豸��ʾ
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

//Զ���ļ����š����ؽ���POS�ص�
void __stdcall CDevice_DH::FileDownloadPosCallBack(long lPlayBackRet, DWORD dwTotalSize, DWORD dwDownLoadSize, DWORD dwUser)
{
    if(!dwUser) return;
    CDevice_DH* pThis = (CDevice_DH*)dwUser;
    //TRACE("hDownload:%d\n", lPlayBackRet);
    SChannelInfo_DH * pTemp = pThis->m_ChannelList.GetChannel(pThis->m_ChannelList.FixHandle(lPlayBackRet));
    if(!pTemp) return;
}

//��ͣ�ط�
long CDevice_DH::EDvr_PausePlayBack(HWND hWnd, long bPause)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_PausePlayBack(pTemp->m_nHandle,bPause))
        return -1;
    return 0;
}

//��֡����
long CDevice_DH::EDvr_OneFramePlayBack(HWND hWnd, long bOneFrame)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_StepPlayBack(pTemp->m_nHandle,(bOneFrame?0:1)))
        return -1;
    return 0;
}

//���ò���λ��
long CDevice_DH::EDvr_SetPosPlayBack(HWND hWnd, long nPlayTime)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_SeekPlayBack(pTemp->m_nHandle, 0xffffffff, (DWORD)nPlayTime))
        return -1;
    return 0;
}

//���
long CDevice_DH::EDvr_FastPlayBack(HWND hWnd)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_FastPlayBack(pTemp->m_nHandle))
        return -1;
    return 0;
}

//����
long CDevice_DH::EDvr_SlowPlayBack(HWND hWnd)
{
    SChannelInfo_DH * pTemp = m_ChannelList.SearchChannel(hWnd);
    if(!pTemp) return -1;
    if(!CLIENT_SlowPlayBack(pTemp->m_nHandle))
        return -1;
    return 0;
}

//ץͼ
long CDevice_DH::EDvr_ConvertPicture(HWND hWnd, char *pstrFileName)
{
    SChannelInfo_DH * sChannel = m_ChannelList.SearchChannel(hWnd);
    /*�ж���������ͨ���Ƿ��*/
    if(sChannel == NULL)
        return -1;
    /*��ʼ��ͼ*/
    if(!CLIENT_CapturePicture(sChannel->m_nHandle, pstrFileName))
        return -1;
    return 0;
}

//////////////////////////////////////////////////////////////////////
// ��ϵͳ��::�ڲ��ӿ�
//////////////////////////////////////////////////////////////////////

/*ֹͣĳ�豸��������������*/
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
    WriteLog(g_szFlag, "�豸�ص�¼ Begin IP:0x%x", theDev->m_dwIP);
    //ִ��ע������
    CLIENT_Logout(theDev->m_nHandle);
    //ִ��ע�����
    int nError;
    NET_DEVICEINFO tmpDev;
    theDev->m_nHandle = CLIENT_Login(inet_ntoa(*(in_addr*)&(theDev->m_dwIP)),
        theDev->m_wPost, theDev->m_strUserName, theDev->m_strPassWord, &tmpDev, &nError);
    if(theDev->m_nHandle == 0)
    {
        WriteLog(g_szFlag, "�豸�ص�¼ʧ�� ����APIʧ�� ���� IP:0x%x", theDev->m_dwIP);
        theDev->m_nHandle = CLIENT_Login(inet_ntoa(*(in_addr*)&(theDev->m_dwIP)),
            theDev->m_wPost, theDev->m_strUserName, theDev->m_strPassWord, &tmpDev, &nError);
        if(theDev->m_nHandle == 0)
        {
            WriteLog(g_szFlag, "�豸�ص�¼ʧ�� ����API����ʧ�� IP:0x%x", theDev->m_dwIP);
            theDev->m_nAudioHandle = 0;
            return -1;
        }
    }
    
    CLIENT_StartListen(theDev->m_nHandle);
    in_SaveComCfg(nLoginID);
    EDvr_ReLink(nLoginID);
    
    WriteLog(g_szFlag, "�豸�ص�¼ End IP:0x%x", theDev->m_dwIP);
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
        if(NULL == theChannel)//�սڵ�
            continue;
        if(theChannel->m_dwIP != theDev->m_dwIP)//�����ڸ��豸��Ϣ
            continue;
        if(theChannel->m_nID <= DH_MAX_CHANNUM)
        {
            WriteLog(g_szFlag, "�ָ����� Begin IP:0x%x ChID:%d", theChannel->m_dwIP, theChannel->m_nID);
            theChannel->m_nHandle = CLIENT_RealPlayEx(theDev->m_nHandle, theChannel->m_nID, theChannel->m_hWnd);
            if(theChannel->m_nHandle)
            {
                if(theChannel->m_aPlayState[0])//�ָ����ݲ���
                    CLIENT_SetRealDataCallBackEx(theChannel->m_nHandle, DataCallBack, i, 0x00000001);
            }
            else
            {
                WriteLog(g_szFlag, "�ָ�����ʧ�� ����APIʧ�� IP:0x%x ChID:%d", theChannel->m_dwIP, theChannel->m_nID);
            }
        }
    }
    return 0;
}

long CDevice_DH::EDvr_ReLink(DWORD nDevIPv4, long nChID)
{
    WriteLog(g_szFlag, "�ָ����� Begin IP:0x%x ChID:%d", nDevIPv4, nChID);
    SDeviceInfo_DH* theDev = NULL;
    SChannelInfo_DH* theChannel = NULL;
    long nLinkID = -1;
    //���˷Ƿ���Ϣ
    theDev = m_DeviceList.SearchDevice(nDevIPv4);
    if(theDev == NULL) goto InfoError;
    nLinkID = m_ChannelList.GetChannelIndex(nDevIPv4, nChID);
    if(nLinkID == -1) goto InfoError;
    theChannel = m_ChannelList.GetChannel(nLinkID);
    //���¶�������
    CLIENT_StopRealPlayEx(theChannel->m_nHandle);
    theChannel->m_nHandle = CLIENT_RealPlayEx(theDev->m_nHandle, theChannel->m_nID,
        theChannel->m_hWnd, FixMediaType(theChannel->m_nMediaType));
    if(theChannel->m_nHandle)
    {
        if(theChannel->m_aPlayState[0])//�ָ����ݲ���
        {
            CLIENT_SetRealDataCallBackEx(theChannel->m_nHandle, DataCallBack, nLinkID, 0x00000001);
        }
        return nLinkID;
    }
    else
    {
        WriteLog(g_szFlag, "�ָ�����ʧ�� ����APIʧ�� IP:0x%x ChID:%d", theChannel->m_dwIP, theChannel->m_nID);
        return -1;
    }
InfoError:
    WriteLog(g_szFlag, "�ָ����� ʧ�� ������Ϣ���� IP:0x%x ChID:%d", nDevIPv4, nChID);
    return -1;
}

//////////////////////////////////////////////////////////////////////
// ��ϵͳ��::����ӿ�
//////////////////////////////////////////////////////////////////////
long CDevice_DH::T_InsertDevice(char*pstrIP, long nPort, char*pstrLogName, char *pstrLogPass, long nIndex)
{
    /*�ж��Ƿ��ʼ��*/
    if(m_bNoInit)
        EDvr_Init();
    DWORD dwIP = inet_addr(pstrIP);
    /*�ж���������Ϸ�*/
    if(dwIP == INADDR_NONE || nPort > 0x0000ffff || pstrLogName == NULL || pstrLogPass == NULL)
        return -1;
    /*�豸�Ƿ��Ѿ���½*/
    if(m_DeviceList.SearchDevice(pstrIP) != NULL)
        return -2;
    /*������ʱ�ռ䱣����Ϣ*/
    SDeviceInfo_DH temp;
    StructInit(temp);
    /*��ʼ��½����*/
    temp.m_dwIP = dwIP;
    temp.m_wPost = (WORD)nPort;
    strcpy(temp.m_strUserName, pstrLogName);
    strcpy(temp.m_strPassWord, pstrLogPass);
    temp.m_nID = 1;
    /*ִ��ע�����*/
    int nError;
    temp.m_nHandle = CLIENT_Login(pstrIP,(WORD)nPort,pstrLogName,pstrLogPass,0,&nError);
    if(temp.m_nHandle == 0)
        return -1;
    /*���豸����*/
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
