// EDvrCtrlAttach.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "EDvrCtrlAttach.h"
#include "InfoManage.h"
#include "OV_DeviceControl.h"
#include "WriteLog.h"
#include "InnerTransact.h"
//#include "CrashRpt.h"
#ifndef _WIN32
#include "toolsInsteadMFC.h"
#endif
const char* XT_LIB_INFO = "XT_Lib_Version:  V1.0.20160413.0";
//////////////////////////////////////////////////////////////////////////
BOOL g_bNoInit = TRUE;
HPlugMrg g_hPlugMrg = NULL;
HDeviceMrg g_hDeviceMrg = NULL;
HAlarmMrg g_hAlarmMrg = NULL;
HReloginMrg g_hReloginMrg = NULL;
HDDnsDevice g_hDDnsMrg = NULL;
SEDvrCtrlEvent g_sCtrlEvent = {0};
int g_nInitCount = 0;

//BOOL WINAPI CrashProc(PEXCEPTION_POINTERS Exception) 
//{
//    char DebugFileA[256] = {0};
//    char DebugFileB[256] = {0};
//    time_t timer;
//    struct tm *tblock;
//
//    //获取当前时间
//    timer = time(NULL);
//    tblock = localtime(&timer);
//    sprintf(DebugFileA,"%s/%s_%02d%02d%02d.xml", c_szLogPath, c_szFlag,
//        tblock->tm_year%100,tblock->tm_mon+1,tblock->tm_mday);
//    sprintf(DebugFileB,"%s/%s_%02d%02d%02d.dmp", c_szLogPath, c_szFlag,
//        tblock->tm_year%100,tblock->tm_mon+1,tblock->tm_mday);
//
//    //GenerateCrashRpt(Exception, DebugFileA, CRASHRPT_ERROR | CRASHRPT_SYSTEM | CRASHRPT_PROCESS);
//    //GenerateMiniDump(Exception, DebugFileB);   
//
//    return EXCEPTION_EXECUTE_HANDLER;
//}

CONTROL_API int Ctrl_Startup(int nRes, char szRes[])
{
	//int nWLog;
    LPCSTR szThisDir;

    if(g_nInitCount++ > 0)
        return 0;

    //写日志
#ifdef _WIN32
	szThisDir = GetFileDirectoryModule(g_hModule);
#else
	char szBuf[MAX_PATH]={0};
	Dl_info dlInfo;
	int ret = dladdr("", &dlInfo);
	if (ret == 0)
	{
		printf("Error: Get current filepath failed");
	}
	else
	{
		for (int i=strlen(dlInfo.dli_fname)-1; i>0; i--)
		{
			if (dlInfo.dli_fname[i] == '/')
			{
				strncpy(szBuf, dlInfo.dli_fname, i);
				break;
			}
		}
		if (szBuf[0]=='\0')
		{
			getcwd(szBuf, MAX_PATH);
		}
		char pathInfo[500];
		sprintf(pathInfo, "Get current filepath: path=%s\n", szBuf);
		printf(pathInfo);
		szThisDir = szBuf;
	}
#endif
    SetLogPath(c_szLogPath);
    EnableLog(GetPrivateProfileInt("WriteLog", "EDvrCtrlAttach", 0, c_szCfg));
    printf("EDvrCtrlAttach:szThisDir:%s\n",szThisDir);
    WriteLog(c_szFlag, "EDvrCtrlAttach:szThisDir:%s\n",szThisDir);
    //InitializeCrashRptEx(CrashProc);

    //创建管理选项
    g_hPlugMrg = PlugMrg_Create();
    g_hDeviceMrg = DeviceMrg_Create();
    g_hAlarmMrg = AlarmMrg_Create();
    g_hReloginMrg = ReloginMrg_Create();
    g_hDDnsMrg = DDnsMrg_Create();

    //读取上传告警屏蔽配置
    memset(s_bSendEvent, TRUE, 0x400);
    s_bSendEvent[eStateMotionDetection] = GetPrivateProfileInt(c_szFlag, "SendMotion", 1, c_szCfg)?TRUE:FALSE;
    s_bSendEvent[eStateAlarmIn] = GetPrivateProfileInt(c_szFlag, "SendSignal", 1, c_szCfg)?TRUE:FALSE;
    s_bSendEvent[eStateVideoBlind] = GetPrivateProfileInt(c_szFlag, "SendBlind", 1, c_szCfg)?TRUE:FALSE;
    s_bSendEvent[eStateVideoLost] = GetPrivateProfileInt(c_szFlag, "SendLost", 1, c_szCfg)?TRUE:FALSE;
    //由于中心使用外网IP时海康IPServer无法访问，改读新的配置
    GetPrivateProfileString(c_szFlag, "DDNS_IP", "127.0.0.1", g_sCtrlEvent.szDDnsAddress, MAX_ADDRESS_LENGTH, c_szFlag);


    //读取告警描述信息配置
    s_szEventMark[eStateAlarmIn] = (char*)malloc(64);
    s_szEventMark[eStateDiskFull] = (char*)malloc(64);
    s_szEventMark[eStateDiskFull|0x100] = (char*)malloc(64);
    s_szEventMark[eStateVideoLost] = (char*)malloc(64);
    s_szEventMark[eStateMotionDetection] = (char*)malloc(64);
    s_szEventMark[eStateDiskError] = (char*)malloc(64);
    s_szEventMark[eStateVideoBlind] = (char*)malloc(64);
    s_szEventMark[eStateVideoAnalyzer] = (char*)malloc(64);

    GetPrivateProfileString(c_szFlag, "Remark0000", "触发报警点", s_szEventMark[eStateAlarmIn], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0001", "磁盘空间不足", s_szEventMark[eStateDiskFull], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0101", "磁盘总空间不足", s_szEventMark[eStateDiskFull|0x100], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0002", "视频信号丢失", s_szEventMark[eStateVideoLost], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0003", "触发移动侦测", s_szEventMark[eStateMotionDetection], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0005", "磁盘错误", s_szEventMark[eStateDiskError], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0006", "图像被遮挡", s_szEventMark[eStateVideoBlind], 64, c_szCfg);
    GetPrivateProfileString(c_szFlag, "Remark0014", "智能人型检测", s_szEventMark[eStateVideoAnalyzer], 64, c_szCfg);

    //启动后台线程
    InitCtrlEvent(&g_sCtrlEvent);
    g_sCtrlEvent.nSendStateTime = GetPrivateProfileInt(c_szFlag, "StateIOTime", 0, c_szCfg);
    g_sCtrlEvent.szSendBuffer = (char*)malloc(0x2000);
    OnlineCheck_Start(&g_sCtrlEvent);
    StateListen_Start(&g_sCtrlEvent);
    AlarmFilter_Start(&g_sCtrlEvent);
    DDnsCheck_Start(&g_sCtrlEvent);
	//OfflineCheck_Start(&g_sCtrlEvent);
    
    //加载插件
    return LoadBasicLib(szThisDir);
}
CONTROL_API int Ctrl_Cleanup()
{
    if(--g_nInitCount > 0)
        return 0;

    //释放插件
    FreeBasicLib();

    //停止后台线程
    g_sCtrlEvent.nSendStateTime = 0;
    DDnsCheck_Stop(&g_sCtrlEvent);
	OnlineCheck_Stop(&g_sCtrlEvent);
	//OfflineCheck_Stop(&g_sCtrlEvent);
    StateListen_Stop(&g_sCtrlEvent);
    AlarmFilter_Stop(&g_sCtrlEvent);
    free(g_sCtrlEvent.szSendBuffer);
    InitCtrlEvent(&g_sCtrlEvent);

    //释放管理选项
    ArrayListRelease((HArrayList)g_hDeviceMrg);
    ArrayListRelease((HArrayList)g_hPlugMrg);
    ArrayListRelease((HArrayList)g_hAlarmMrg);
    ArrayListRelease((HArrayList)g_hReloginMrg);
    ArrayListRelease((HArrayList)g_hDDnsMrg);

    //释放描述信息
    if(s_szEventMark[eStateAlarmIn])        free(s_szEventMark[eStateAlarmIn]);
    if(s_szEventMark[eStateDiskFull])       free(s_szEventMark[eStateDiskFull]);
    if(s_szEventMark[eStateDiskFull|0x100]) free(s_szEventMark[eStateDiskFull|0x100]);
    if(s_szEventMark[eStateVideoLost])      free(s_szEventMark[eStateVideoLost]);
    if(s_szEventMark[eStateMotionDetection])free(s_szEventMark[eStateMotionDetection]);
    if(s_szEventMark[eStateDiskError])      free(s_szEventMark[eStateDiskError]);
    if(s_szEventMark[eStateVideoBlind])     free(s_szEventMark[eStateVideoBlind]);
    if(s_szEventMark[eStateVideoAnalyzer])  free(s_szEventMark[eStateVideoAnalyzer]);
    memset(s_szEventMark, 0, sizeof(char*)*0x400);

    return 0;
}

CONTROL_API int Ctrl_GetDeviceTypeList(dgDeviceType fpDeviceType, void *objUser)
{
    int i = 0;
    SOV_CtrlPlug *pPlug = NULL;

    if(!fpDeviceType)
        return 0;
    //g_hPlugMrg
    for(i=0; i<(signed)ArrayListGetSize((HArrayList)g_hPlugMrg); i++)
    {
        pPlug = PlugMrg_GetInfo(g_hPlugMrg, i);
        if(!pPlug)
            continue;

        fpDeviceType(i, &pPlug->m_sType, objUser);
    }
    return i;
}

CONTROL_API int Ctrl_RegistOnlineEvent(dgOnOnlineEvent pFunction, void *objUser)
{
    g_sCtrlEvent.fpOnlineEvent = pFunction;
    g_sCtrlEvent.objOnline = objUser;
    return 0;
}
CONTROL_API int Ctrl_RegistAlarmStateEvent(dgOnAlarmStateEvent pFunction, void *objUser)
{
    g_sCtrlEvent.fpAlarmStateEvent = pFunction;
    g_sCtrlEvent.objAlarm = objUser;
    return 0;
}
CONTROL_API int Ctrl_RegistSerialEvent(dgOnSerialEvent pFunction, void *objUser)
{
    g_sCtrlEvent.fpSerialEvent = pFunction;
    g_sCtrlEvent.objSerial = objUser;
    return 0;
}
CONTROL_API int Ctrl_RegistDeviceIPChange(dgOnDeviceIPChange pFunction, void *objUser)
{
    g_sCtrlEvent.fpIPChange = pFunction;
    g_sCtrlEvent.objIPChange = objUser;
    return 0;
}
CONTROL_API int Ctrl_RegistDvrStateInfo(dgOnDvrStateInfo pFunction, void *objUser)
{
    g_sCtrlEvent.fpDvrStateInfo = pFunction;
    g_sCtrlEvent.objStateInfo = objUser;
    return 0;
}
CONTROL_API int Ctrl_RegistDvrStateChanged(dgOnDvrStateChanged pFunction, void *objUser)
{
    g_sCtrlEvent.fpDvrStateChanged = pFunction;
    g_sCtrlEvent.objStateChange = objUser;
    return 0;
}

CONTROL_API int Ctrl_ClientAddDvr(LPCSTR szSerialNum, int nDeviceType)
{
    SOV_CtrlPlug *pControl = NULL;
    SDDnsDeviceInfo sNewDevice;
    int nTypeID, nDDnsDeviceID;

    if(szSerialNum == NULL || szSerialNum[0] == 0)
        return -1;

    nDDnsDeviceID = DDnsMrg_SelectDevice(g_hDDnsMrg, szSerialNum, NULL);

    if(nDDnsDeviceID >= 0)
        return nDDnsDeviceID;

    nTypeID = PlugMrg_SelectType(g_hPlugMrg, nDeviceType, &pControl);

    InitDDnsDevice(&sNewDevice);
    sNewDevice.nDDnsTypeID = nTypeID;
    sNewDevice.nDeviceType = nDeviceType;
    lstrcpy(sNewDevice.szSerialID, szSerialNum);
    nDDnsDeviceID = ArrayListAddAt(g_hDDnsMrg, &sNewDevice);
    
    return nDDnsDeviceID;
}

CONTROL_API int Ctrl_ClientJoin(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType)
{
    SOV_CtrlPlug *pControl = NULL;
    int nTypeID;

    WriteLog(c_szFlag, "设备上线请求 IDS:%s Addres:%s Type:%d", szIDS, szAddress, nDeviceType);

    nTypeID = PlugMrg_SelectType(g_hPlugMrg, nDeviceType, &pControl);
    if(nTypeID < 0)
    {
        //不处理该类型设备，直接返回上线
        FireOnlineChangedTrance(&g_sCtrlEvent, szIDS, szAddress, nDeviceType, TRUE, 0, NULL);
        return 0;
    }
    else
    {
        return Ctrl_ClientJoinV2(szIDS, szAddress, nDeviceType, 0, pControl->m_sType.m_nControlPort, pControl->m_sType.m_szUser, pControl->m_sType.m_szPassword);
    }
}

//设备上线
CONTROL_API int Ctrl_ClientJoinV2(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nSubType, int nCtrlPort, LPCSTR szUserName, LPCSTR szPassword)
{
    SOV_CtrlPlug *pControl = NULL;
    SDeviceCtrlInfo sNewDevice, *pDevice = NULL;
    int nDeviceID, nTypeID;

    WriteLog(c_szFlag, "设备上线请求V2 IDS:%s Addres:%s Type:%d SubType:%d CtrlPort:%d User:%s Pass:%s",
        szIDS, szAddress, nDeviceType, nSubType, nCtrlPort, szUserName, szPassword);


	WriteLog(c_szFlag, "PlugMrg_SelectType Start");
    nTypeID = PlugMrg_SelectType(g_hPlugMrg, nDeviceType, &pControl);
	WriteLog(c_szFlag, "PlugMrg_SelectType： nTypeID:%d",nTypeID);
    if(nTypeID < 0)
    {
        //不处理该类型设备，直接返回上线
        FireOnlineChangedTrance(&g_sCtrlEvent, szIDS, szAddress, nDeviceType, TRUE, 0, NULL);
        return 0;
    }

    nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
    {//新增设备
        WriteLog(c_szFlag, "添加新设备 IDS:%s Addres:%s Type:%d SubType:%d", szIDS, szAddress, nDeviceType, nSubType);
        InitDeviceInfo(&sNewDevice);
        sNewDevice.m_nCtrlPort = nCtrlPort;
        sNewDevice.m_nTypeID = nTypeID;
        sNewDevice.m_nSubType = nSubType;
        lstrcpy(sNewDevice.m_szIDS, szIDS);
        lstrcpy(sNewDevice.m_szAddress, szAddress);
        lstrcpy(sNewDevice.m_szUser, szUserName);
        lstrcpy(sNewDevice.m_szPass, szPassword);
        nDeviceID = ArrayListAddAt((HArrayList)g_hDeviceMrg, &sNewDevice);
        pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, nDeviceID);
        pDevice->m_nDeviceID = nDeviceID;
        return Device_LoginNew(pDevice);
    }
    else
    {//已存在设备
        //判断登录参数是否改变
        if(szUserName && szUserName[0] && lstrcmp(pDevice->m_szUser, szUserName))
            lstrcpy(pDevice->m_szUser, szUserName);

        if(szPassword && szPassword[0] && lstrcmp(pDevice->m_szPass, szPassword))
            lstrcpy(pDevice->m_szPass, szPassword);

        if(nCtrlPort > 0 && nCtrlPort != pDevice->m_nCtrlPort)
            pDevice->m_nCtrlPort = nCtrlPort;

        if(nSubType >= 0 && nSubType != pDevice->m_nSubType)
            pDevice->m_nSubType = nSubType;

        if(!lstrcmp(pDevice->m_szAddress, szAddress))
        {
            WriteLog(c_szFlag, "重新上线 IDS:%s Addres:%s Type:%d SubType:%d", szIDS, szAddress, nDeviceType, nSubType);
            return Device_LoginReonline(pDevice);
        }
        else
        {//动态IP再次登录，需要注销原有的IP以新的IP登录
            WriteLog(c_szFlag, "设备地址改变 IDS:%s Addres:%s Type:%d SubType:%d", szIDS, szAddress, nDeviceType, nSubType);
            Device_Logout(pDevice);
            lstrcpy(pDevice->m_szAddress, szAddress);
            return Device_LoginNew(pDevice);
        }
    }
}

//设备离线
CONTROL_API int Ctrl_ClientLeave(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType)
{
    SDeviceCtrlInfo *pDevice = NULL;
    SOV_CtrlPlug *pControl = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);

    WriteLog(c_szFlag, "设备离线请求 IDS:%s", szIDS);
    if(nDeviceID < 0)
    {//未加入管理的设备直接抛出下线消息
        FireOnlineChangedTrance(&g_sCtrlEvent, szIDS, szAddress, nDeviceType, FALSE, 0, NULL);
        return 0;
    }

    pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(pControl->m_bAutoOnline)
    {//自动重连设备直接由设备插件提供在线判断
        return 0;
    }
    else
    {//非自动重连设备需要手动抛出下线消息
        //将设备从手动重连队列移出
        ReloginMrg_DeleteInfoCheck(g_hReloginMrg, nDeviceID);
        Device_Logout(pDevice);
        FireOnlineChangedTrance(&g_sCtrlEvent, szIDS, szAddress, nDeviceType, FALSE, 0, NULL);
    }
    return 0;
}

CONTROL_API int Ctrl_ReflashDeviceList(LPCSTR szIDS, LPCSTR szAddress, int nDeviceType, int nReflashState)
{
    int iDevice, nDeviceID, nTypeID;
    SDeviceCtrlInfo sNewDevice, *pDevice = NULL;
    SOV_CtrlPlug *pControl = NULL;

    WriteLog(c_szFlag, "刷新设备列表 IDS:%s Address:%s DeviceType:%d ReflashState:%d", szIDS, szAddress, nDeviceType, nReflashState);
    switch(nReflashState)
    {
    case eReflashStateStart:
        WriteLog(c_szFlag, "刷新设备列表 清除所有设备IDS信息");
        for(iDevice=0; iDevice<(signed)ArrayListGetSize((HArrayList)g_hDeviceMrg); iDevice++)
        {
            pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, iDevice);
            memset(pDevice->m_szIDS, 0, MAX_IDS_LENGTH);
        }
        break;
    case eReflashStateDoing: break;
    case eReflashStateFinish: break;
    default: return -1;
    }

    if(!szIDS || !szIDS[0] || !szAddress || !szAddress[0])
    {
        return 0;
    }

    nTypeID = PlugMrg_SelectType(g_hPlugMrg, nDeviceType, &pControl);
    if(nTypeID < 0)
    {
        //不处理该类型设备，直接返回
        return 0;
    }

    nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);
    if(nDeviceID >= 0)
    {
        WriteLog(c_szFlag, "刷新设备列表 设备已经存在 IDS:%s Address:%s DeviceType:%d", szIDS, szAddress, nDeviceType);
        lstrcpy(pDevice->m_szIDS, szIDS);
        pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
        if(nDeviceType != pControl->m_sType.m_nDeviceType)
        {
            WriteLog(c_szFlag, "刷新设备列表 设备类型改变即设备改变 注销原来的设备");
            Device_Logout(pDevice);
        }
        pDevice->m_nTypeID = PlugMrg_SelectType(g_hPlugMrg, nDeviceType, &pControl);
    }
    else
    {
        WriteLog(c_szFlag, "刷新设备列表 添加新设备 IDS:%s Address:%s DeviceType:%d", szIDS, szAddress, nDeviceType);
        InitDeviceInfo(&sNewDevice);
        sNewDevice.m_nTypeID = PlugMrg_SelectType(g_hPlugMrg, nDeviceType, &pControl);
        lstrcpy(sNewDevice.m_szIDS, szIDS);
        lstrcpy(sNewDevice.m_szAddress, szAddress);
        nDeviceID = ArrayListAddAt((HArrayList)g_hDeviceMrg, &sNewDevice);
        pDevice = DeviceMrg_GetInfo(g_hDeviceMrg, nDeviceID);
        pDevice->m_nDeviceID = nDeviceID;
    }
    return 0;
}

CONTROL_API int Ctrl_ClientAlarm(LPCSTR szIDS, int nChannel, int bEnable)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "报警点布防 IDS:%s Address:%s Channel:%d Enable:%d", szIDS, pDevice->m_szAddress, nChannel, bEnable);
    return Device_DisposeAlarmOneInput(pDevice, nChannel, bEnable);
}
CONTROL_API int Ctrl_ClientMotion(LPCSTR szIDS, int nChannel, int bEnable)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "移动侦测布防 IDS:%s Address:%s Channel:%d Enable:%d", szIDS, pDevice->m_szAddress, nChannel, bEnable);
    return Device_DisposeAlarmOneMotion(pDevice, nChannel, bEnable);
}
CONTROL_API int Ctrl_ClientRecord(LPCSTR szIDS, int nChannel, int bEnable)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "手动录像设置 IDS:%s Address:%s Channel:%d Enable:%d", szIDS, pDevice->m_szAddress, nChannel, bEnable);
    return Device_DisposeRecordOne(pDevice, nChannel, bEnable);
}
CONTROL_API int Ctrl_ClientOutput(LPCSTR szIDS, int nChannel, int bEnable)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "报警输出设置 IDS:%s Address:%s Channel:%d Enable:%d", szIDS, pDevice->m_szAddress, nChannel, bEnable);
    return Device_DisposeAlarmOneOutput(pDevice, nChannel, bEnable);
}

CONTROL_API int Ctrl_ClientLost(LPCSTR szIDS, int nChannel, int bEnable)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "视频丢失布防 IDS:%s Address:%s Channel:%d Enable:%d", szIDS, pDevice->m_szAddress, nChannel, bEnable);
    return Device_DisposeAlarmOneLost(pDevice, nChannel, bEnable);
}

CONTROL_API int Ctrl_ClientBlind(LPCSTR szIDS, int nChannel, int bEnable)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "视频遮挡布防 IDS:%s Address:%s Channel:%d Enable:%d", szIDS, pDevice->m_szAddress, nChannel, bEnable);
    return Device_DisposeAlarmOneBlind(pDevice, nChannel, bEnable);
}

CONTROL_API int Ctrl_ClientImage(LPCSTR szIDS, int nChannel, int eCtrlEffect, int nValue)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "画面调节 IDS:%s Address:%s Channel:%d Effect:%d Value:%d", szIDS, pDevice->m_szAddress, nChannel, eCtrlEffect, nValue);
    if(GetPrivateProfileInt(c_szFlag, "OldEffect", 0, c_szFlag))
    {//旧式XT5000算法
        switch(eCtrlEffect)
        {
        case eCtrlBrightness: break;
        case eCtrlContrast:
            if(nValue>127) nValue = 127;
            nValue *= 2;
            break;
        case eCtrlHue:
            nValue = (nValue+127)%256;
            break;
        case eCtrlSaturation:
            if(nValue>127) nValue = 127;
            nValue *= 2;
            break;
        default: break;
        }
    }
    if(eCtrlEffect != eCtrlDefault)
        return Device_SetCameraOneEffect(pDevice, nChannel, eCtrlEffect-1, nValue);
    else
    {
        return Device_SetEffectDefault(pDevice, nChannel);
    }
}

CONTROL_API int Ctrl_ClientImageWhole(LPCSTR szIDS, int nChannel, SBaseEffects sEffects)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    if(GetPrivateProfileInt(c_szFlag, "OldEffect", 0, c_szFlag))
    {//旧式XT5000算法
        if(sEffects.nContrast > 127) sEffects.nContrast = 127;
        sEffects.nContrast *= 2;

        sEffects.nHue = (sEffects.nHue+127)%256;

        if(sEffects.nSaturation > 127) sEffects.nSaturation = 127;
        sEffects.nSaturation *= 2;
    }

    WriteLog(c_szFlag, "画面调节 IDS:%s Address:%s Channel:%d 亮度.对比度.色度.饱和度:%d.%d.%d.%d",
        szIDS, pDevice->m_szAddress, nChannel, sEffects.nBrightness, sEffects.nContrast, sEffects.nHue, sEffects.nSaturation);
    return Device_SetCameraWholeEffect(pDevice, nChannel, sEffects);
}

CONTROL_API int Ctrl_ClientPtz(LPCSTR szIDS, int nChannel, int ePtzAct)
{
    return Ctrl_ClientPtzSpeed(szIDS, nChannel, ePtzAct, 127);
}
CONTROL_API int Ctrl_ClientPtzSpeed(LPCSTR szIDS, int nChannel, int ePtzAct, int nSpeed)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
	EnableLog(GetPrivateProfileInt("WriteLog", "EDvrCtrlAttach", 0, c_szCfg));		//调试用
    if(nDeviceID < 0)
	{
		WriteLog(c_szFlag,"IDS不正确");
        return -1;
	}
   
    WriteLog(c_szFlag, "云台带速控制 IDS:%s Address:%s Channel:%d PtzAct:%d Speed:%d", szIDS, pDevice->m_szAddress, nChannel, ePtzAct, nSpeed);
	return Device_PtzControlBase(pDevice, nChannel, ePtzAct&0xFFF0, nSpeed, ePtzAct&0x000F);
}
CONTROL_API int Ctrl_ClientPtzAdvance(LPCSTR szIDS, int nChannel, char* szAdvCmd, int nCmdSize)
{
    SDeviceCtrlInfo *pDevice = NULL;
	int result=-1;

    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
	{
        return -1;
	}
	WriteLog(c_szFlag,"调用Device_PtzControlAdvice方法sID：%s,sCH:%d,Cmd:%s,Size:%d",szIDS,nChannel,szAdvCmd,nCmdSize);
    result= Device_PtzControlAdvice(pDevice, nChannel, szAdvCmd, nCmdSize);
	WriteLog(c_szFlag,"Device_PtzControlAdvice返回值：%d",result);
	return result;
}
CONTROL_API int Ctrl_ClientPtzPreset(LPCSTR szIDS, int nChannel, int eCtrlPresetAct, int nPreset)
{ 
    SDeviceCtrlInfo *pDevice = NULL;
    int ePresetAct;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
	{
		WriteLog(c_szFlag,"IDS: %s 不存在!", szIDS);
        return -1;
	}
  
    WriteLog(c_szFlag, "云台预置点控制 IDS:%s Address:%s Channel:%d PresetAct:%d PresetID:%d",
        szIDS, pDevice->m_szAddress, nChannel, eCtrlPresetAct, nPreset);
    switch(eCtrlPresetAct)
    {
    case eCtrlPresetDelete: ePresetAct = ePresetDelete; break;
    case eCtrlPresetAdd: ePresetAct = ePresetSet; break;
    case eCtrlPresetEdit: ePresetAct = ePresetSet; break;
    case eCtrlPresetMove: ePresetAct = ePresetMove; break;
    default: 
		{
			WriteLog(c_szFlag,"错误的云台控制指令");
			return -1;
		}
    }
    
    return Device_PtzPreset(pDevice, nChannel, ePresetAct, nPreset);
}

CONTROL_API int Ctrl_CaptureIFrame(LPCSTR szIDS, int nChannel)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID;

    nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "I帧捕获 IDS:%s Addres:%s Channel:%d", szIDS, pDevice->m_szAddress, nChannel);
    return Device_CaptureIFrame(pDevice, nChannel);
}

CONTROL_API int Ctrl_SetDateTime(LPCSTR szIDS, int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec)
{
    SDeviceCtrlInfo *pDevice = NULL;
    SYSTEMTIME dtTime = {0};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;
    //dtTime.wYear = nYear;
    //dtTime.wMonth = nMonth;
    //dtTime.wDay = nDay;
    //dtTime.wHour = nHour;
    //dtTime.wMinute = nMin;
    //dtTime.wSecond = nSec;

    GetLocalTime(&dtTime);
    WriteLog(c_szFlag, "I设备校时 IDS:%s Addres:%s %d-%d-%d %d:%d:%d", szIDS, pDevice->m_szAddress,
        dtTime.wYear, dtTime.wMonth, dtTime.wDay, dtTime.wHour, dtTime.wMinute, dtTime.wSecond);
    return Device_SetDateTime(pDevice, &dtTime);
}

CONTROL_API int Ctrl_SetCameraName(LPCSTR szIDS, int nChannel, LPCSTR szName)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "设置通道名 IDS:%s Addres:%s Channel:%d Name:%s", szIDS, pDevice->m_szAddress, nChannel, szName);
    return Device_SetCameraName(pDevice, nChannel, szName);
}
CONTROL_API int Ctrl_SetCameraText(LPCSTR szIDS, int nChannel, int bIsShow, LPCSTR szText,\
                                int nLeft, int nTop, int nWidth, int nHeight, int nRGBColor)
{
    SDeviceCtrlInfo *pDevice = NULL;
    SCameraText sCameraText = {0};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    sCameraText.nLeft = nLeft;
    sCameraText.nTop = nTop;
    sCameraText.nWidth = nWidth;
    sCameraText.nHeight = nHeight;
    sCameraText.bShow = bIsShow;
    sCameraText.nTextLength = lstrlen(szText)+1;
    sCameraText.nRGBColor = nRGBColor;
    sCameraText.szText = (char*)szText;

    WriteLog(c_szFlag, "设置图像文字 IDS:%s Addres:%s Channel:%d IsShow:%d Text:%s Point:[%d.%d] Size:[%d.%d] Color:%X",
        szIDS, pDevice->m_szAddress, nChannel, bIsShow, szText, nLeft, nTop, nWidth, nHeight, nRGBColor);
    return Device_SetCameraText(pDevice, nChannel, &sCameraText);
}

CONTROL_API int Ctrl_SetCameraTextV2(LPCSTR szIDS, int nChannel, int nTextIndex, int bIsShow,\
                    LPCSTR szText, int nLeft, int nTop, int nWidth, int nHeight, int nRGBColor)
{
    SDeviceCtrlInfo *pDevice = NULL;
    SCameraText sCameraText = {0};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    sCameraText.nLeft = nLeft;
    sCameraText.nTop = nTop;
    sCameraText.nWidth = nWidth;
    sCameraText.nHeight = nHeight;
    sCameraText.bShow = bIsShow;
    sCameraText.nTextLength = lstrlen(szText)+1;
    sCameraText.nRGBColor = nRGBColor;
    sCameraText.szText = (char*)szText;

    WriteLog(c_szFlag, "设置图像文字V2 IDS:%s Addres:%s Channel:%d TextIndex:%d IsShow:%d Text:%s Point:[%d.%d] Size:[%d.%d] Color:%X",
        szIDS, pDevice->m_szAddress, nChannel, nTextIndex, bIsShow, szText, nLeft, nTop, nWidth, nHeight, nRGBColor);
    return Device_SetCameraTextV2(pDevice, nChannel, nTextIndex, &sCameraText);
}

CONTROL_API int Ctrl_ClientAlarmState(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nAlarmType[1] = {eStateAlarmIn};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备报警状态 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientMotionState(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nAlarmType[1] = {eStateMotionDetection};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备移动侦测状态 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientRecordState(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nAlarmType[1] = {eStateRecord};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备录像状态 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientOutputState(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
	int nAlarmType[1] = {eStateIllegalAccess};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备信号输出状态 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientLostState(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nAlarmType[1] = {eStateVideoLost};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备视频丢失状态 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientBlindState(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nAlarmType[1] = {eStateVideoBlind};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备视频遮挡状态 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientMediaBitrate(LPCSTR szIDS)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nAlarmType[1] = {eStateMediaBitrate};
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "获取设备码流配置 IDS:%s", szIDS);
    return Device_GetAlarmStateAsync(pDevice, nAlarmType, 1);
}

CONTROL_API int Ctrl_ClientSerialOpen(LPCSTR szIDS, int nSerialPort)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    //预留代码
    WriteLog(c_szFlag, "打开透明串口 IDS:%s SerialPort:%d", szIDS, nSerialPort);
    return Device_SerialOpen(pDevice, nSerialPort, OnSerialOutput, (void*)(INT_PTR)nDeviceID);
}
CONTROL_API int Ctrl_ClientSerialOpenParam(LPCSTR szIDS, int nSerialPort, SSerialParam *pSerialParam)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    //透传稳定性保护由插件层完成
    WriteLog(c_szFlag, "打开透明串口 IDS:%s SerialPort:%d 数据位:%d 停止位:%d 校验:%d 流控:%d 波特率:%d",
        szIDS, nSerialPort, pSerialParam->m_nDataBit, pSerialParam->m_nStopBit, pSerialParam->m_nParity,
        pSerialParam->m_nFlowControl, pSerialParam->m_nBaudRate);
    return Device_SerialOpenParam(pDevice, nSerialPort, pSerialParam, OnSerialOutput, (void*)(INT_PTR)nDeviceID);
}
CONTROL_API int Ctrl_ClientSerialClose(LPCSTR szIDS, int nSerialPort)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "关闭透明串口 IDS:%s SerialPort:%d", szIDS, nSerialPort);
    return Device_SerialClose(pDevice, nSerialPort);
}
CONTROL_API int Ctrl_ClientSerialSend(LPCSTR szIDS, int nSerialPort, int nChannel, void* pBufData, int nBufSize)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectIDS(g_hDeviceMrg, szIDS, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "发送透明数据 IDS:%s SerialPort:%d Channel:%d Size:%d",
        szIDS, nSerialPort, nChannel, nBufSize);
    return Device_SerialSend(pDevice, nSerialPort, nChannel, nBufSize, (BYTE*)pBufData);
}
CONTROL_API int Ctrl_ClientStartTVOut(LPCSTR szIDS, int nChannel, LPCSTR szTargetIDS, int nTargetChannel)
{
    return -1;
}
CONTROL_API int Ctrl_ClientStopTVOut(LPCSTR szIDS, int nChannel, LPCSTR szTargetIDS, int nTargetChannel)
{
    return -1;
}

//////////////////////////////////////////////////////////////////////////

CONTROL_API int Ctrlv2_SetDateTime(LPCSTR szAddress, int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec)
{
    SDeviceCtrlInfo *pDevice = NULL;
    SYSTEMTIME dtTime = {0};
    int nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);
    if(nDeviceID < 0)
        return -1;
    //dtTime.wYear = nYear;
    //dtTime.wMonth = nMonth;
    //dtTime.wDay = nDay;
    //dtTime.wHour = nHour;
    //dtTime.wMinute = nMin;
    //dtTime.wSecond = nSec;

    GetLocalTime(&dtTime);
    WriteLog(c_szFlag, "I设备校时 Addres:%s %d-%d-%d %d:%d:%d", pDevice->m_szAddress,
        dtTime.wYear, dtTime.wMonth, dtTime.wDay, dtTime.wHour, dtTime.wMinute, dtTime.wSecond);
    return Device_SetDateTime(pDevice, &dtTime);
}

CONTROL_API int Ctrlv2_ClientImage(LPCSTR szAddress, int nChannel, int eCtrlEffect, int nValue)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "画面调节 Address:%s Channel:%d Effect:%d Value:%d", szAddress, nChannel, eCtrlEffect, nValue);
    if(GetPrivateProfileInt(c_szFlag, "OldEffect", 0, c_szFlag))
    {//旧式XT5000算法
        switch(eCtrlEffect)
        {
        case eCtrlBrightness: break;
        case eCtrlContrast:
            if(nValue>127) nValue = 127;
            nValue *= 2;
            break;
        case eCtrlHue:
            nValue = (nValue+127)%256;
            break;
        case eCtrlSaturation:
            if(nValue>127) nValue = 127;
            nValue *= 2;
            break;
        default: break;
        }
    }
    if(eCtrlEffect != eCtrlDefault)
        return Device_SetCameraOneEffect(pDevice, nChannel, eCtrlEffect-1, nValue);
    else
    {
        return Device_SetEffectDefault(pDevice, nChannel);
    }
}

CONTROL_API int Ctrlv2_ClientImageWhole(LPCSTR szAddress, int nChannel, SBaseEffects sEffects)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);
    if(nDeviceID < 0)
        return -1;

    if(GetPrivateProfileInt(c_szFlag, "OldEffect", 0, c_szFlag))
    {//旧式XT5000算法
        if(sEffects.nContrast > 127) sEffects.nContrast = 127;
        sEffects.nContrast *= 2;

        sEffects.nHue = (sEffects.nHue+127)%256;

        if(sEffects.nSaturation > 127) sEffects.nSaturation = 127;
        sEffects.nSaturation *= 2;
    }

    WriteLog(c_szFlag, "画面调节 Address:%s Channel:%d 亮度.对比度.色度.饱和度:%d.%d.%d.%d",
        szAddress, nChannel, sEffects.nBrightness, sEffects.nContrast, sEffects.nHue, sEffects.nSaturation);
    return Device_SetCameraWholeEffect(pDevice, nChannel, sEffects);
}

CONTROL_API int Ctrlv2_CaptureIFrame(LPCSTR szAddress, int nChannel)
{
    SDeviceCtrlInfo *pDevice = NULL;
    int nDeviceID;

    nDeviceID = DeviceMrg_SelectAddress(g_hDeviceMrg, szAddress, &pDevice);
    if(nDeviceID < 0)
        return -1;

    WriteLog(c_szFlag, "I帧捕获 Addres:%s Channel:%d", pDevice->m_szAddress, nChannel);
    return Device_CaptureIFrame(pDevice, nChannel);
}
