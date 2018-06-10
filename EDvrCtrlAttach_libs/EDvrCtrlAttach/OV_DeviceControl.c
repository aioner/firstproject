#include "stdafx.h"
#include "OV_DeviceControl.h"
#include "InnerTransact.h"
#include "WriteLog.h"
#include <assert.h>
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include "toolsInsteadMFC.h"
#endif

extern HPlugMrg g_hPlugMrg;
extern HReloginMrg g_hReloginMrg;
extern HAlarmMrg g_hAlarmMrg;
extern SEDvrCtrlEvent g_sCtrlEvent;

#ifdef _WIN32
DWORD WINAPI GetStateProc(SGetAlarmState *pGetState);
#else
void* GetStateProc(void *pGetState);
#endif

//////////////////////////////////////////////////////////////////////////
int LoadBasicLib(const char* szFindPath)
{
    char szIniPath[256] = {0};
    char szLibPath[256] = {0};
    char* pszMidPath = szLibPath;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA fdata = {0};
    SOV_CtrlPlug ctltype = {0};

    wsprintf(szIniPath, "%s\\MediaDeviceInfo\\Control\\Ctrl\\*.ini", szFindPath);
    wsprintf(szLibPath, "%s", szFindPath);
    pszMidPath += strlen(szLibPath);

#ifdef _WIN32
    //����Ŀ�����ļ����µ������ļ���������������Ϣ
    hFindFile = FindFirstFile(szIniPath, &fdata);
    if(hFindFile == INVALID_HANDLE_VALUE)
    {
#ifdef _DEBUG
        LoadOneLib(&ctltype, "", szLibPath);
        return 0;
#else
        return -1;
#endif
    }
    do{
        InitCtrlPlug(&ctltype);
        wsprintf(szIniPath, "%s\\MediaDeviceInfo\\Control\\Ctrl\\%s", szFindPath, fdata.cFileName);
        WriteLog(c_szFlag, "���ؽű�:%s", szIniPath);

        LoadOneLib(&ctltype, szIniPath, szLibPath);
    }while(FindNextFile(hFindFile, &fdata));

    FindClose(hFindFile);
    
#else //LINUX
	sprintf(szIniPath, "%s/MediaDeviceInfo/Control/Ctrl", szLibPath);
	
	vector<const char*> arrList;
	GetFilesInFolder(szIniPath, arrList);
	for (int i=0; i<arrList.size(); i++)
	{
		if (strstr(arrList[i], ".ini") != NULL)
		{
			LoadOneLib(&ctltype, arrList[i], szLibPath);
		}
		delete[] arrList[i];
		
	}	
	arrList.clear();

#endif //#ifdef _WIN32    
    return 0;
}

void FreeBasicLib()
{
    size_t i;
    SOV_CtrlPlug *pControlPlug = NULL;
    if(g_hPlugMrg == NULL)
        return;

    for(i=0; i<ArrayListGetSize((HArrayList)g_hPlugMrg); i++)
    {
        pControlPlug = PlugMrg_GetInfo(g_hPlugMrg, i);
        if(pControlPlug == NULL)
            continue;

        FreeOneLib(pControlPlug);
        ArrayListEraseAt((HArrayList)g_hPlugMrg, i);
    }
}

int LoadOneLib(SOV_CtrlPlug *pControlPlug, const char* szIniPath, const char* szFindPath)
{
    int nPlugID;
    char szRelPath[64] = {0};
    char szLibName[64] = {0};
    char szSubTypeCfgPath[64] = {0};
    char szMidPath[256] = {0};

    pControlPlug->m_sType.m_nDeviceType = GetPrivateProfileInt(c_szAppName, "DeviceType", -1, szIniPath);
    if(pControlPlug->m_sType.m_nDeviceType == -1)
        return -1;//�����ļ���
    if(PlugMrg_SelectType(g_hPlugMrg, pControlPlug->m_sType.m_nDeviceType, NULL) != -1)
        return -1;//�Ѿ����������ü���

    //��ȡ��̬��·��
    GetPrivateProfileString(c_szAppName, "DllPath", ".", szRelPath, 64, szIniPath);
    lstrcpy(szMidPath, szFindPath);
#ifdef _WIN32
	PathCombine(szMidPath, szMidPath, szRelPath);
	szMidPath[strlen(szMidPath)+1] = 0;
	szMidPath[strlen(szMidPath)] = '\\';
#else
	sprintf(szMidPath, "%s/%s", szMidPath, szRelPath);
	szMidPath[strlen(szMidPath)+1] = 0;
	szMidPath[strlen(szMidPath)] = '/';
#endif

    GetPrivateProfileString(c_szAppName, "KeyDllName", "", szLibName, 64, szIniPath);
    GetPrivateProfileString(c_szAppName, "TypeName", "", pControlPlug->m_sType.m_szTypeName, 64, szIniPath);
    GetPrivateProfileString(c_szAppName, "UserName", "", pControlPlug->m_sType.m_szUser, 16, szIniPath);
    GetPrivateProfileString(c_szAppName, "Password", "", pControlPlug->m_sType.m_szPassword, 32, szIniPath);
    GetPrivateProfileString(c_szAppName, "SubTypeCfgPath", ".", szSubTypeCfgPath, 64, szIniPath);
	pControlPlug->m_nOnlineDelay = GetPrivateProfileInt(c_szAppName,"OnlineDelay",5,szIniPath);
    pControlPlug->m_sType.m_nControlPort = GetPrivateProfileInt(c_szAppName, "CtrlPort", 1, szIniPath);

    //������Ϣ
    nPlugID = (int)ArrayListAddAt((HArrayList)g_hPlugMrg, pControlPlug);
    //���ض�̬��
    pControlPlug = PlugMrg_GetInfo(g_hPlugMrg, nPlugID);
    if(-1 == LoadPlugAPI(pControlPlug, szIniPath, szMidPath, szLibName))
    {
        //����ʧ�ܣ�Ĭ��Ϊ�ݲ�֧�ָ������豸���ã�ֻ����Config����ʱ��Ҫ���豸�����б�
        //PlugMrg_DeleteInfo(g_hPlugMrg, nPlugID);
        return -1;
    }

    if(pControlPlug->m_PlugAPI.m_dgStartup != NULL)
    {
        //�����豸�ͺ�����·��
        lstrcpy(szMidPath, szFindPath);
#ifdef _WIN32
		PathCombine(szMidPath, szMidPath, szSubTypeCfgPath);       
		szMidPath[strlen(szMidPath)+1] = 0;
		szMidPath[strlen(szMidPath)] = '\\';
#else
		sprintf(szMidPath,"%s%s", szMidPath, szSubTypeCfgPath);
		szMidPath[strlen(szMidPath)+1] = 0;
		szMidPath[strlen(szMidPath)] = '/';
#endif

        if(0 > pControlPlug->m_PlugAPI.m_dgStartup(pControlPlug->m_sType.m_nDeviceType, szMidPath))
            return -1;
        Plug_RegistEvent(pControlPlug);
        return 0;
    }
    else
    {
        return -1;
    }
}
#ifdef _WIN32
#define LOAD_API_ADDRESS(name) \
	GetPrivateProfileString(c_szAppName, c_sz##name, c_sz##name, szApiName, 64, szIniPath);\
	pControlPlug->m_PlugAPI.m_dg##name = (dg##name)GetProcAddress(pControlPlug->m_PlugAPI.m_hInstance, szApiName)
#else
#define LOAD_API_ADDRESS(name) \
	memset(szApiName,0,64);\
	GetPrivateProfileString(c_szAppName, c_sz##name, c_sz##name, szApiName, 64, szIniPath);\
	pControlPlug->m_PlugAPI.m_dg##name = (dg##name)dlsym(pControlPlug->m_PlugAPI.m_hInstance, szApiName)
#endif


int LoadPlugAPI(SOV_CtrlPlug *pControlPlug, const char* szIniPath, const char* szLibDirectory, const char* szKeyFile)
{
    char* szFileName = pControlPlug->m_szLibPath;
    char szAppPath[256];
    char szApiName[64];
    int nDirectoryLength = lstrlen(szLibDirectory);
    lstrcpy(szFileName, szLibDirectory);
    GetCurrentDirectory(256, szAppPath);
    WriteLog(c_szFlag, "��ǰ��������·��:%s", szAppPath);
    if(szFileName[0] == '.')
    {//�ֶ��õ���ǰ·��
        lstrcpy(szFileName, GetFileDirectoryModule(g_hModule));
        nDirectoryLength = lstrlen(szFileName);
        lstrcpy(szFileName+nDirectoryLength, szLibDirectory+1);
        nDirectoryLength = lstrlen(szFileName);
    }
    if(szFileName[nDirectoryLength-1] == '\\' ||
        szFileName[nDirectoryLength-1] == '/')
    {//ȥ������"\"
        szFileName[nDirectoryLength-1] = 0;
        nDirectoryLength = lstrlen(szFileName);
    }
#ifdef _WIN32
	szFileName[nDirectoryLength] = '\\';
#else
	szFileName[nDirectoryLength] = '/';
#endif
    lstrcpy(szFileName+nDirectoryLength+1, szKeyFile);

    WriteLog(c_szFlag, "���·��:%s ���ز��:%s", szLibDirectory, szFileName);
#ifdef _WIN32
	pControlPlug->m_PlugAPI.m_hInstance = LoadDll(szLibDirectory, szFileName, NULL, 0);
#else
	pControlPlug->m_PlugAPI.m_hInstance = LoadDll(szLibDirectory, szKeyFile, NULL, 0);
#endif

    if(pControlPlug->m_PlugAPI.m_hInstance == NULL)
    {
        WriteLog(c_szFlag, "Load %s Fail", szKeyFile);
        return -1;
    }

    //////////////////////////////////////////////////////////////////////////
    // ͨ�������е�API���� ��ȡAPI
    LOAD_API_ADDRESS(Startup                );
    LOAD_API_ADDRESS(Cleanup                );
    LOAD_API_ADDRESS(RegistAddressChanged   );
    LOAD_API_ADDRESS(RegistOnlineChanged    );
    LOAD_API_ADDRESS(RegistAlarmChanged     );
    LOAD_API_ADDRESS(GetLogin               );
    LOAD_API_ADDRESS(Login                  );
    LOAD_API_ADDRESS(LoginLe                );
    LOAD_API_ADDRESS(Logout                 );
    LOAD_API_ADDRESS(Listen                 );
    LOAD_API_ADDRESS(GetDeviceAttrib        );
    LOAD_API_ADDRESS(SetCameraWholeEffect   );
    LOAD_API_ADDRESS(SetCameraOneEffect     );
    LOAD_API_ADDRESS(DisposeAlarmWholeInput );
    LOAD_API_ADDRESS(DisposeAlarmOneInput   );
    LOAD_API_ADDRESS(GetAlarmStateInput     );
    LOAD_API_ADDRESS(DisposeAlarmWholeMotion);
    LOAD_API_ADDRESS(DisposeAlarmOneMotion  );
    LOAD_API_ADDRESS(GetAlarmStateMotion    );
    LOAD_API_ADDRESS(DisposeAlarmWholeBlind );
    LOAD_API_ADDRESS(DisposeAlarmOneBlind   );
    LOAD_API_ADDRESS(GetAlarmStateBlind     );
    LOAD_API_ADDRESS(DisposeAlarmWholeLost  );
    LOAD_API_ADDRESS(DisposeAlarmOneLost    );
    LOAD_API_ADDRESS(GetAlarmStateLost      );
    LOAD_API_ADDRESS(DisposeAlarmWholeOutput);
    LOAD_API_ADDRESS(DisposeAlarmOneOutput  );
    LOAD_API_ADDRESS(GetAlarmStateOutput    );
    LOAD_API_ADDRESS(DisposeRecordWhole     );
    LOAD_API_ADDRESS(DisposeRecordOne       );
    LOAD_API_ADDRESS(GetRecordState         );
    LOAD_API_ADDRESS(PtzControlBase         );
    LOAD_API_ADDRESS(PtzControlAdvice       );
    LOAD_API_ADDRESS(PtzPreset              );
    LOAD_API_ADDRESS(SetDateTime            );
    LOAD_API_ADDRESS(SetCameraName          );
    LOAD_API_ADDRESS(SetCameraText          );
    LOAD_API_ADDRESS(SetCameraTextV2        );
    LOAD_API_ADDRESS(CaptureIFrame          );
    LOAD_API_ADDRESS(SerialOpen             );
    LOAD_API_ADDRESS(SerialOpenParam        );
    LOAD_API_ADDRESS(SerialClose            );
    LOAD_API_ADDRESS(SerialSend             );
    LOAD_API_ADDRESS(OpenMediaChannel       );
    LOAD_API_ADDRESS(CloseMediaChannel      );
    LOAD_API_ADDRESS(GetDeviceWorkstate     );
    LOAD_API_ADDRESS(SelectByDNS            );
    LOAD_API_ADDRESS(GetMediaBitrate        );

    return 0;
}

void FreeOneLib(SOV_CtrlPlug *pControlPlug)
{
    if(pControlPlug != NULL && pControlPlug->m_PlugAPI.m_dgCleanup != NULL)
    {
        pControlPlug->m_PlugAPI.m_dgCleanup();
        //FreeLibrary(pControlPlug->m_PlugAPI.m_hInstance);
    }
    memset(&pControlPlug->m_PlugAPI, 0, sizeof(SOV_CtrlAPI));
}

void Plug_RegistEvent(SOV_CtrlPlug *pControlPlug)
{
    if(pControlPlug->m_PlugAPI.m_dgRegistAddressChanged)
        pControlPlug->m_PlugAPI.m_dgRegistAddressChanged(OnDeviceAddressChanged, pControlPlug);
    if(pControlPlug->m_PlugAPI.m_dgRegistAlarmChanged)
        pControlPlug->m_PlugAPI.m_dgRegistAlarmChanged(OnDeviceAlarmChanged, pControlPlug);

    //ͨ���˽ӿ��ж��豸�Ƿ�֧���Զ�����
    if(pControlPlug->m_PlugAPI.m_dgRegistOnlineChanged)
    {
        if(0 > pControlPlug->m_PlugAPI.m_dgRegistOnlineChanged(OnDeviceOnlineChanged, pControlPlug))
            pControlPlug->m_bAutoOnline = 0;
        else
            pControlPlug->m_bAutoOnline = 1;
    }
    else
        pControlPlug->m_bAutoOnline = 0;
}

LPCSTR Plug_SelectByDNS(SOV_CtrlPlug *pControlPlug, LPCSTR szDnsAddress, LPCSTR szDeviceUrl)
{
    if(pControlPlug->m_PlugAPI.m_dgSelectByDNS)
        return pControlPlug->m_PlugAPI.m_dgSelectByDNS(szDnsAddress, szDeviceUrl);
    else
        return "";
}

//////////////////////////////////////////////////////////////////////////
//�豸����

long Device_Login(SDeviceCtrlInfo *pDevice)
{
	LOGINTYPE ret = -1;
	SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
	if (pControl == NULL || pControl->m_PlugAPI.m_dgLogin == NULL)
	{
		return -1;
	}

	ret = pControl->m_PlugAPI.m_dgLogin(pDevice->m_szAddress, pDevice->m_nCtrlPort, pDevice->m_szUser, pDevice->m_szPass, 0);
	if (ret>=0)
	{
		pDevice->m_nLoginHandle = ret;
	}
	return (ret<0?-1:0);

}

long Device_Logout(SDeviceCtrlInfo *pDevice)
{
    LOGINTYPE nLoginID;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgLogout)
        return -1;

    nLoginID = pDevice->m_nLoginHandle;
    pDevice->m_nLoginHandle = -1;
    pDevice->m_sWorkState.nOnlineState = 0;

	return nLoginID<0?nLoginID:pControl->m_PlugAPI.m_dgLogout(nLoginID);
}

int Device_Listen(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgListen)
        return -1;

    return pControl->m_PlugAPI.m_dgListen(pDevice->m_nLoginHandle);
}

int Device_GetDeviceAttrib(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetDeviceAttrib)
        return -1;

    return pControl->m_PlugAPI.m_dgGetDeviceAttrib(pDevice->m_nLoginHandle, &pDevice->m_sAttribs);
}

int Device_SetCameraWholeEffect(SDeviceCtrlInfo *pDevice, int nChannel, SBaseEffects sEffects)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSetCameraWholeEffect)
        return -1;

    return pControl->m_PlugAPI.m_dgSetCameraWholeEffect(pDevice->m_nLoginHandle, nChannel, sEffects);
}

int Device_SetCameraOneEffect(SDeviceCtrlInfo *pDevice, int nChannel, int eEffect, int nValue)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSetCameraOneEffect)
        return -1;

    return pControl->m_PlugAPI.m_dgSetCameraOneEffect(pDevice->m_nLoginHandle, nChannel, eEffect, nValue);
}

int Device_DisposeAlarmWholeInput(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[])
{
    int i, nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmWholeInput)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmWholeInput(pDevice->m_nLoginHandle, nArraySize, arrayEnable);
    for(i=0; i<nArraySize; i++)
    {
        //��¼����״̬
        pDevice->m_sWorkState.nInputDispose[i] = arrayEnable[i];
        if(!arrayEnable[i])//����澯��ʱ
            AlarmSignla_Clean(pDevice, eStateAlarmIn, i);
    }
    return nRet;
}

int Device_DisposeAlarmOneInput(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable)
{
    int nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmOneInput)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmOneInput(pDevice->m_nLoginHandle, nChannel, bEnable);

    //��¼����״̬
    pDevice->m_sWorkState.nInputDispose[nChannel] = bEnable;
    if(!bEnable)//����澯��ʱ
        AlarmSignla_Clean(pDevice, eStateAlarmIn, nChannel);
    return nRet;
}

int Device_GetAlarmStateInput(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetAlarmStateInput)
        return -1;

    return pControl->m_PlugAPI.m_dgGetAlarmStateInput(pDevice->m_nLoginHandle, MAX_ALARM_INPUT, (BYTE*)pDevice->m_sWorkState.nInputDispose);
}

int Device_DisposeAlarmWholeMotion(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[])
{
    int i, nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmWholeMotion)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmWholeMotion(pDevice->m_nLoginHandle, nArraySize, arrayEnable);
    for(i=0; i<nArraySize; i++)
    {
        //��¼����״̬
        pDevice->m_sWorkState.nMotionDispose[i] = arrayEnable[i];
        if(!arrayEnable[i])//����澯��ʱ
            AlarmSignla_Clean(pDevice, eStateMotionDetection, i);
    }
    return nRet;
}

int Device_DisposeAlarmOneMotion(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable)
{
    int nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmOneMotion)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmOneMotion(pDevice->m_nLoginHandle, nChannel, bEnable);

    //��¼����״̬
    pDevice->m_sWorkState.nMotionDispose[nChannel] = bEnable;
    if(!bEnable)
        AlarmSignla_Clean(pDevice, eStateMotionDetection, nChannel);
    return nRet;
}

int Device_GetAlarmStateMotion(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetAlarmStateMotion)
        return -1;

    return pControl->m_PlugAPI.m_dgGetAlarmStateMotion(pDevice->m_nLoginHandle, MAX_CAMERA_NUM, (BYTE*)pDevice->m_sWorkState.nMotionDispose);
}

int Device_DisposeAlarmWholeBlind(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[])
{
    int i, nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmWholeBlind)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmWholeBlind(pDevice->m_nLoginHandle, nArraySize, arrayEnable);
    for(i=0; i<nArraySize; i++)
    {
        //��¼����״̬
        pDevice->m_sWorkState.nBlindDispose[i] = arrayEnable[i];
        if(!arrayEnable[i])//����澯��ʱ
            AlarmSignla_Clean(pDevice, eStateVideoBlind, i);
    }
    return nRet;
}

int Device_DisposeAlarmOneBlind(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable)
{
    int nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmOneBlind)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmOneBlind(pDevice->m_nLoginHandle, nChannel, bEnable);

    //��¼����״̬
    pDevice->m_sWorkState.nBlindDispose[nChannel] = bEnable;
    if(!bEnable)
        AlarmSignla_Clean(pDevice, eStateVideoBlind, nChannel);
    return nRet;
}

int Device_GetAlarmStateBlind(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetAlarmStateBlind)
        return -1;

    return pControl->m_PlugAPI.m_dgGetAlarmStateBlind(pDevice->m_nLoginHandle, MAX_CAMERA_NUM, (BYTE*)pDevice->m_sWorkState.nBlindDispose);
}

int Device_DisposeAlarmWholeLost(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[])
{
    int i, nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmWholeLost)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmWholeLost(pDevice->m_nLoginHandle, nArraySize, arrayEnable);
    for(i=0; i<nArraySize; i++)
    {
        //��¼����״̬
        pDevice->m_sWorkState.nLostDispose[i] = arrayEnable[i];
        if(!arrayEnable[i])//����澯��ʱ
            AlarmSignla_Clean(pDevice, eStateVideoLost, i);
    }
    return nRet;
}

int Device_DisposeAlarmOneLost(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable)
{
    int nRet;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmOneLost)
        return -1;

    nRet = pControl->m_PlugAPI.m_dgDisposeAlarmOneLost(pDevice->m_nLoginHandle, nChannel, bEnable);

    //��¼����״̬
    pDevice->m_sWorkState.nLostDispose[nChannel] = bEnable;
    if(!bEnable)
        AlarmSignla_Clean(pDevice, eStateVideoLost, nChannel);
    return nRet;
}

int Device_GetAlarmStateLost(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetAlarmStateLost)
        return -1;

    return pControl->m_PlugAPI.m_dgGetAlarmStateLost(pDevice->m_nLoginHandle, MAX_CAMERA_NUM, (BYTE*)pDevice->m_sWorkState.nLostDispose);
}

int Device_DisposeAlarmWholeOutput(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[])
{
    int i;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmWholeOutput)
        return -1;

    for(i=0; i<nArraySize; i++)
    {
        //��¼����״̬
        pDevice->m_sWorkState.nOutputDispose[i] = arrayEnable[i];
    }
    
    return pControl->m_PlugAPI.m_dgDisposeAlarmWholeOutput(pDevice->m_nLoginHandle, nArraySize, arrayEnable);
}

int Device_DisposeAlarmOneOutput(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeAlarmOneOutput)
        return -1;

    //��¼����״̬
    pDevice->m_sWorkState.nOutputDispose[nChannel] = bEnable;
    return pControl->m_PlugAPI.m_dgDisposeAlarmOneOutput(pDevice->m_nLoginHandle, nChannel, bEnable);
}

int Device_GetAlarmStateOutput(SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetAlarmStateOutput)
        return -1;

    return pControl->m_PlugAPI.m_dgGetAlarmStateOutput(pDevice->m_nLoginHandle, MAX_ALARM_OUTPUT, (BYTE*)pDevice->m_sWorkState.nOutputDispose);
}

int Device_DisposeRecordWhole(SDeviceCtrlInfo *pDevice, int nArraySize, BYTE arrayEnable[])
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeRecordWhole)
        return -1;

    return pControl->m_PlugAPI.m_dgDisposeRecordWhole(pDevice->m_nLoginHandle, nArraySize, arrayEnable);
}

int Device_DisposeRecordOne(SDeviceCtrlInfo *pDevice, int nChannel, int bEnable)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgDisposeRecordOne)
        return -1;

    return pControl->m_PlugAPI.m_dgDisposeRecordOne(pDevice->m_nLoginHandle, nChannel, bEnable);
}

int Device_GetRecordState(SDeviceCtrlInfo *pDevice)
{
	SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
	if(!pControl || !pControl->m_PlugAPI.m_dgGetRecordState)
		return -1;

	return pControl->m_PlugAPI.m_dgGetRecordState(pDevice->m_nLoginHandle, MAX_CAMERA_NUM, (BYTE*)pDevice->m_sWorkState.nRecordDispose);
}

int Device_PtzControlBase(SDeviceCtrlInfo *pDevice, int nChannel, int ePtzAct, int nSpeed, int bStop)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgPtzControlBase)
	{
		WriteLog(c_szFlag,"�豸���Ͳ���ȷ");
		return -1;
	}

    return pControl->m_PlugAPI.m_dgPtzControlBase(pDevice->m_nLoginHandle, nChannel, ePtzAct, nSpeed, bStop);
}

int Device_PtzControlAdvice(SDeviceCtrlInfo *pDevice, int nChannel, char* szAdvCmd, int nCmdSize)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgPtzControlAdvice)
        return -1;

    return pControl->m_PlugAPI.m_dgPtzControlAdvice(pDevice->m_nLoginHandle, nChannel, szAdvCmd, nCmdSize);
}

int Device_PtzPreset(SDeviceCtrlInfo *pDevice, int nChannel, int ePreserAct, int nPreset)
{
    char szSection[16];
    char szKey[24];
    char szIniPath[256];
    char szOpenVoneVersion[24];
    unsigned nOpenVoneVersion = 0;

    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgPtzPreset)
	{
		WriteLog(c_szFlag,"�豸���Ͳ���ȷ");
        return -1;
	}

    //��ʱ��Ԥ�õ����Ϊϵͳ��ţ���Ҫͨ�����ò���ʵ���豸�ϵ����

    GetPrivateProfileString(c_szFlag, "OpenVoneVersion", "10.0.3", szOpenVoneVersion, 24, c_szCfg);
    nOpenVoneVersion = ConvertVersion(szOpenVoneVersion);

    if(ePreserAct == ePresetMove && nOpenVoneVersion < 0x0a000003)
    {
        wsprintf(szIniPath, "d:\\NetMcSet\\%s.ini", pDevice->m_szAddress);
        wsprintf(szSection, "%d", nChannel+1);
        wsprintf(szKey, "%dindex", nPreset);

        nPreset = GetPrivateProfileInt(szSection, szKey, 0, szIniPath);
    }

    return pControl->m_PlugAPI.m_dgPtzPreset(pDevice->m_nLoginHandle, nChannel, ePreserAct, nPreset);
}

int Device_SetDateTime(SDeviceCtrlInfo *pDevice, SYSTEMTIME* pDateTime)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSetDateTime)
        return -1;

    return pControl->m_PlugAPI.m_dgSetDateTime(pDevice->m_nLoginHandle, pDateTime);
}

int Device_SetCameraName(SDeviceCtrlInfo *pDevice, int nChannel, const char* szCameraName)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSetCameraName)
        return -1;

    return pControl->m_PlugAPI.m_dgSetCameraName(pDevice->m_nLoginHandle, nChannel, szCameraName);
}

int Device_SetCameraText(SDeviceCtrlInfo *pDevice, int nChannel, SCameraText* pCameraText)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSetCameraText)
        return -1;

    return pControl->m_PlugAPI.m_dgSetCameraText(pDevice->m_nLoginHandle, nChannel, pCameraText);
}

int Device_SetCameraTextV2(SDeviceCtrlInfo *pDevice, int nChannel, int nTextIndex, SCameraText* pCameraText)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSetCameraText)
        return -1;

    return pControl->m_PlugAPI.m_dgSetCameraTextV2(pDevice->m_nLoginHandle, nChannel, nTextIndex, pCameraText);
}

int Device_CaptureIFrame(SDeviceCtrlInfo *pDevice, int nChannel)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgCaptureIFrame)
        return -1;

    return pControl->m_PlugAPI.m_dgCaptureIFrame(pDevice->m_nLoginHandle, nChannel);
}

int Device_SerialOpen(SDeviceCtrlInfo *pDevice, int nSerialPort, dgSerialOutput pFunction, void* objUser)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSerialOpen)
        return -1;

    return pControl->m_PlugAPI.m_dgSerialOpen(pDevice->m_nLoginHandle, nSerialPort, pFunction, objUser);
}

int Device_SerialOpenParam(SDeviceCtrlInfo *pDevice, int nSerialPort, SSerialParam* pSerialParam, dgSerialOutput pFunction, void* objUser)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSerialOpenParam)
        return -1;

    return pControl->m_PlugAPI.m_dgSerialOpenParam(pDevice->m_nLoginHandle, nSerialPort, pSerialParam, pFunction, objUser);
}

int Device_SerialClose(SDeviceCtrlInfo *pDevice, int nSerialPort)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSerialClose)
        return -1;

    return pControl->m_PlugAPI.m_dgSerialClose(pDevice->m_nLoginHandle, nSerialPort);
}

int Device_SerialSend(SDeviceCtrlInfo *pDevice, int nSerialPort, int nChannel, int nDataLength, BYTE arrayData[])
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgSerialSend)
        return -1;

    return pControl->m_PlugAPI.m_dgSerialSend(pDevice->m_nLoginHandle, nSerialPort, nChannel, nDataLength, arrayData);
}

int Device_OpenMediaChannel(SDeviceCtrlInfo *pDevice, int nChannel, const char* szTargetAddress, int nTargetPort)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgOpenMediaChannel)
        return -1;

    return pControl->m_PlugAPI.m_dgOpenMediaChannel(pDevice->m_nLoginHandle, nChannel, szTargetAddress, nTargetPort);
}

int Device_CloseMediaChannel(SDeviceCtrlInfo *pDevice, int nChannel, const char* szTargetAddress, int nTargetPort)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgCloseMediaChannel)
        return -1;

    return pControl->m_PlugAPI.m_dgCloseMediaChannel(pDevice->m_nLoginHandle, nChannel, szTargetAddress, nTargetPort);
}

int Device_GetDeviceWorkstate(SDeviceCtrlInfo *pDevice)
{
    int i;
    SDeviceWrokstate sWorkstate = {0};
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetDeviceWorkstate)
        return -1;

    if(0 > pControl->m_PlugAPI.m_dgGetDeviceWorkstate(pDevice->m_nLoginHandle, &sWorkstate))
        return -1;

    pDevice->m_sWorkState.nHardwareAlarm = sWorkstate.nDeviceState;
    for(i=0; i<sWorkstate.nDiskNum; i++)
    {
        switch(sWorkstate.nDiskState[i])
        {
        case 0://����
            pDevice->m_sWorkState.bDiskFull[i] = 0;
            pDevice->m_sWorkState.bDiskError[i] = 0;
            pDevice->m_sWorkState.bDiskState[i] = 0;
            break;
        case 1://������
            pDevice->m_sWorkState.bDiskFull[i] = 1;
            pDevice->m_sWorkState.bDiskError[i] = 0;
            pDevice->m_sWorkState.bDiskState[i] = 0;
            break;
        case 2://���̴���
            pDevice->m_sWorkState.bDiskFull[i] = 0;
            pDevice->m_sWorkState.bDiskError[i] = 1;
            pDevice->m_sWorkState.bDiskState[i] = 0;
            break;
        case 3://��������
            pDevice->m_sWorkState.bDiskFull[i] = 0;
            pDevice->m_sWorkState.bDiskError[i] = 0;
            pDevice->m_sWorkState.bDiskState[i] = 1;
            break;
        }
        pDevice->m_sWorkState.nDiskSize[i] = sWorkstate.nDiskSize[i];
        pDevice->m_sWorkState.nDiskSpace[i] = sWorkstate.nDiskSpace[i];
    }
    return 0;
}

int Device_GetMediaBitrate (SDeviceCtrlInfo *pDevice)
{
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);
    if(!pControl || !pControl->m_PlugAPI.m_dgGetMediaBitrate )
        return -1;

    return pControl->m_PlugAPI.m_dgGetMediaBitrate (pDevice->m_nLoginHandle, MAX_CAMERA_NUM,\
        pDevice->m_sWorkState.nPrimaryRate, pDevice->m_sWorkState.nSecondaryRate);
}

//////////////////////////////////////////////////////////////////////////
// inner background process

int Device_LoginNew(SDeviceCtrlInfo *pDevice)
{
    SReloginDevice newRelogin;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);

    if(0 > Device_Login(pDevice))
    {
        //��ӵ��ֶ���������
        newRelogin.m_nDeviceID = pDevice->m_nDeviceID;
        newRelogin.m_nTimer = newRelogin.m_nMarkTime = pControl->m_nOnlineDelay;
        ReloginMrg_AddInfoCheck(g_hReloginMrg, &newRelogin);
        return -1;
    }
    else if(!pControl->m_bAutoOnline)
    {//���Զ���¼�豸��Ҫ���Ϸ���������Ϣ
        FireOnlineChanged(&g_sCtrlEvent, pDevice, pControl->m_sType.m_nDeviceType, TRUE, 0, "");
    }

    return 0;
}

int Device_LoginReonline(SDeviceCtrlInfo *pDevice)
{
    SReloginDevice newRelogin;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);

    if(pControl->m_bAutoOnline)
    {
        WriteLog(c_szFlag, "�豸���µ�¼ �Զ������豸 IDS:%s Addres:%s Type:%d",\
            pDevice->m_szIDS, pDevice->m_szAddress, pControl->m_sType.m_nDeviceType);

        if(pDevice->m_nLoginHandle >= 0)
        {
            return 0;
        }
    }
    else
    {
        WriteLog(c_szFlag, "�豸���µ�¼ ���Զ������豸 IDS:%s Addres:%s Type:%d",\
            pDevice->m_szIDS, pDevice->m_szAddress, pControl->m_sType.m_nDeviceType);

        //�ж��豸�Ƿ��Ѿ�����
        if(pDevice->m_sWorkState.nOnlineState == 1
            && Device_GetDeviceAttrib(pDevice) >= 0)
        {
            FireOnlineChanged(&g_sCtrlEvent, pDevice,
                pControl->m_sType.m_nDeviceType, 1, 0, "");
            return 0;
        }
    }

    WriteLog(c_szFlag, "�豸���µ�¼ �ֶ������豸 IDS:%s Addres:%s Type:%d",\
        pDevice->m_szIDS, pDevice->m_szAddress, pControl->m_sType.m_nDeviceType);

    //�����豸Ϊ����״̬������ӵ��ֶ���������
    pDevice->m_sWorkState.nOnlineState = 0;
    newRelogin.m_nDeviceID = pDevice->m_nDeviceID;
    newRelogin.m_nTimer = newRelogin.m_nMarkTime = pControl->m_nOnlineDelay/*10*/;
    ReloginMrg_AddInfoCheck(g_hReloginMrg, &newRelogin);
    return -1;
}

//ͨ����ȡ��������Ĭ�ϻ������
int Device_SetEffectDefault(SDeviceCtrlInfo *pDevice, int nChannel)
{
    char szDevType[24] = {0};
    char szEffect[24] = {0};
    unsigned nEffect;
    SBaseEffects sEffects;
    SOV_CtrlPlug* pControl = PlugMrg_GetInfo(g_hPlugMrg, pDevice->m_nTypeID);

    wsprintf(szDevType, "XT_%02X", pControl->m_sType.m_nDeviceType);
    GetPrivateProfileString("ImageEffect", szDevType, "127.127.127.127", szEffect, 32, c_szCfg);

    nEffect = ntohl(inet_addr(szEffect));
    sEffects = *(SBaseEffects*)&nEffect;

    return Device_SetCameraWholeEffect(pDevice, nChannel, sEffects);
}

//�첽�õ��豸��������״̬
int Device_GetAlarmStateAsync(SDeviceCtrlInfo *pDevice, int nAlarmType[], int nTypeLength)
{
    SGetAlarmState *pGetState = NULL;
    int i;
    
    pGetState = (SGetAlarmState *)malloc(sizeof(SGetAlarmState));
    pGetState->pDevice = pDevice;
    pGetState->nTypeLength = nTypeLength;
    for(i=0; i<nTypeLength; i++)
    {
        pGetState->nAlarmType[i] = nAlarmType[i];
    }
#ifdef _WIN32
    pGetState->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetStateProc, pGetState, 0, &pGetState->nThreadID);
#else
	pGetState->hThread = pthread_create(&pGetState->hThread, NULL, &GetStateProc, pGetState);
#endif
    Sleep(20);

    return 0;        
}

//�ɼ��豸���ض�����״̬
int Device_RequestAlarmState(SDeviceCtrlInfo *pDevice, int nAlarmType)
{
    int nRet = -1;

    switch(nAlarmType)
    {
    case eStateAlarmIn:
        nRet = Device_GetAlarmStateInput(pDevice);
        break;
    case eStateVideoLost:
        nRet = Device_GetAlarmStateLost(pDevice);
        break;
    case eStateMotionDetection:
        nRet = Device_GetAlarmStateMotion(pDevice);
        break;
    case eStateVideoBlind:
        nRet = Device_GetAlarmStateBlind(pDevice);
        break;
    case eStateDiskError://�ֶ�¼��һ��DVRû���ض���ָ����ǰ�ֶ�¼���״̬��
        break;
    case eStateIllegalAccess://�������ź����
        nRet = Device_GetAlarmStateOutput(pDevice);
		break;
	case eStateRecord:
		nRet = Device_GetRecordState(pDevice);
		break;
    case eStateMediaBitrate://��ý������
        nRet = Device_GetMediaBitrate (pDevice);
        break;
    default:
        break;
    }
    return nRet;
}

int Device_GetAlarmState(SDeviceCtrlInfo *pDevice, int nAlarmType, int OUT *nAlarmSize, char** OUT pAlarmDispose)
{
    assert(pAlarmDispose && nAlarmSize);

    switch(nAlarmType)
    {
    case eStateAlarmIn:
        *nAlarmSize = pDevice->m_sAttribs.m_nAlarmInCount;
        *pAlarmDispose = pDevice->m_sWorkState.nInputDispose;
        break;
    case eStateVideoLost:
        *nAlarmSize = pDevice->m_sAttribs.m_nVideoCount;
        *pAlarmDispose = pDevice->m_sWorkState.nLostDispose;
        break;
    case eStateMotionDetection:
        *nAlarmSize = pDevice->m_sAttribs.m_nVideoCount;
        *pAlarmDispose = pDevice->m_sWorkState.nMotionDispose;
        break;
    case eStateVideoBlind:
        *nAlarmSize = pDevice->m_sAttribs.m_nVideoCount;
        *pAlarmDispose = pDevice->m_sWorkState.nBlindDispose;
        break;
    case eStateDiskError://�ֶ�¼��һ��DVRû���ض���ָ����ǰ�ֶ�¼���״̬��
        *nAlarmSize = -1;
        *pAlarmDispose = NULL;
        break;
    case eStateIllegalAccess://�������ź����
        *nAlarmSize = pDevice->m_sAttribs.m_nAlarmOutCount;
        *pAlarmDispose = pDevice->m_sWorkState.nOutputDispose;
		break;
	case eStateRecord://¼��״̬
		*nAlarmSize = pDevice->m_sAttribs.m_nVideoCount;
		*pAlarmDispose = pDevice->m_sWorkState.nRecordDispose;
		break;
    default:
        *nAlarmSize = -1;
        *pAlarmDispose = NULL;
        break;
    }
    return 0;
}

int Device_GetValueState(SDeviceCtrlInfo *pDevice, int nAlarmType, int OUT *nValuesSize, int** OUT pValues1, int** OUT pValues2)
{
    assert(pValues1 && pValues2 && nValuesSize);

    switch(nAlarmType)
    {
    case eStateMediaBitrate://��ý������
        *nValuesSize = pDevice->m_sAttribs.m_nVideoCount;
        *pValues1 = pDevice->m_sWorkState.nPrimaryRate;
        *pValues2 = pDevice->m_sWorkState.nSecondaryRate;
        break;
    default:
        *nValuesSize = -1;
        *pValues1 = NULL;
        break;
    }
    return 0;
}

//��'0'/'1'�ַ�������ʽ����豸���ض�����״̬
int Device_FireAlarmState(SDeviceCtrlInfo *pDevice, int nAlarmType)
{
    char szAlarmState[64] = {0};
    char *pAlarmDispose = NULL;
    int  *pValueData1 = NULL, *pValueData2 = NULL;
    int i, nAlarmSize = 0, nRet;

    if(0 > Device_RequestAlarmState(pDevice, nAlarmType))
        return -1;

    if(nAlarmType < STATE_VALUE(0))//������״̬
    {
        nRet = Device_GetAlarmState(pDevice, nAlarmType, &nAlarmSize, &pAlarmDispose);
        if(nRet >= 0 && nAlarmSize > 0)
        {
            for(i=0; i<nAlarmSize; i++)
            {
                szAlarmState[i] = pAlarmDispose[i] + '0';
            }
            szAlarmState[i] = 0;
            FireAlarmState(&g_sCtrlEvent, pDevice, szAlarmState, nAlarmSize+1, nAlarmType, "");
        }
    }
    else//��ֵ״̬
    {
        nRet = Device_GetValueState(pDevice, nAlarmType, &nAlarmSize, &pValueData1, &pValueData2);

        for(i=0; nRet >= 0 && i<nAlarmSize; i++)
        {
            FireDvrStateChanged(&g_sCtrlEvent, pDevice, nAlarmType, i, pValueData1[i], (void*)pValueData2[i], 0);
        }
    }
    return 0;
}
#ifdef _WIN32
DWORD WINAPI GetStateProc(SGetAlarmState *pGetState)
#else
void* GetStateProc(void *lpGetState)
#endif
{
	int i;
#ifndef _WIN32 
	SGetAlarmState *pGetState = (SGetAlarmState *)lpGetState;
#endif
	if(pGetState == NULL)
		return 0;
    for(i=0; i<pGetState->nTypeLength; i++)
    {
        Device_FireAlarmState(pGetState->pDevice, pGetState->nAlarmType[i]);
    }
    Sleep(30);
#ifdef _WIN32
	CloseHandle(pGetState->hThread);
#endif
    free(pGetState);
    return 0;
}

#define c_nBufferLength 4096
int Device_MakeStateXML(SDeviceCtrlInfo *pDevice, char *szXMLBuffer)
{
    char szAlarmState[c_nBufferLength] = {0}, *pTmpBuf = NULL;

    memset(szXMLBuffer, 0, 8192);
    lstrcpy(szXMLBuffer, "SNMPPACK");

    AddParamString(szXMLBuffer, "IDS", pDevice->m_szIDS);
    AddParamString(szXMLBuffer, "IP", pDevice->m_szAddress);
    AddParamInt(szXMLBuffer, "CameraNum", pDevice->m_sAttribs.m_nVideoCount);
    AddParamInt(szXMLBuffer, "DiskNum", pDevice->m_sAttribs.m_nRomCount);
    AddParamInt(szXMLBuffer, "DVRState", pDevice->m_sWorkState.nHardwareAlarm);

    MakeBoolArrayString(szAlarmState, c_nBufferLength, (BYTE*)pDevice->m_sWorkState.bVideoLost, pDevice->m_sAttribs.m_nVideoCount);
    AddParamString(szXMLBuffer, "VideoLost", szAlarmState);

    MakeBoolArrayString(szAlarmState, c_nBufferLength, (BYTE*)pDevice->m_sWorkState.bVideoBlind, pDevice->m_sAttribs.m_nVideoCount);
    AddParamString(szXMLBuffer, "VideoBlind", szAlarmState);

    MakeBoolArrayString(szAlarmState, c_nBufferLength, (BYTE*)pDevice->m_sWorkState.bDiskFull, pDevice->m_sAttribs.m_nRomCount);
    AddParamString(szXMLBuffer, "DiskFull", szAlarmState);

    MakeBoolArrayString(szAlarmState, c_nBufferLength, (BYTE*)pDevice->m_sWorkState.bDiskError, pDevice->m_sAttribs.m_nRomCount);
    AddParamString(szXMLBuffer, "DiskError", szAlarmState);

    MakeBoolArrayString(szAlarmState, c_nBufferLength, (BYTE*)pDevice->m_sWorkState.bDiskState, pDevice->m_sAttribs.m_nRomCount);
    AddParamString(szXMLBuffer, "DiskState", szAlarmState);

    MakeUInt32ArrayString(szAlarmState, c_nBufferLength, pDevice->m_sWorkState.nDiskSize, pDevice->m_sAttribs.m_nRomCount);
    AddParamString(szXMLBuffer, "DiskSize", szAlarmState);

    MakeUInt32ArrayString(szAlarmState, c_nBufferLength, pDevice->m_sWorkState.nDiskSpace, pDevice->m_sAttribs.m_nRomCount);
    AddParamString(szXMLBuffer, "DiskSpace", szAlarmState);

    return lstrlen(szXMLBuffer);
}
