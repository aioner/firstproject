#include "stdafx.h"
#include "CommonDef.h"
#ifdef _WIN32
const char c_szCfg[] = "D:\\NetMcSet\\µ×²ãÅäÖÃ.ini";
const char c_szLogPath[] = "D:\\xtlog";
#else
const char c_szCfg[] = "/etc/xtconfig/d/netmcset/baseconfig.ini";
const char c_szLogPath[] = "/var/log/xtlog";
#endif

const char c_szFlag[] = "EDvrCtrlAttach";
const char c_szAppName[] = "ControlInfo";
HMODULE g_hModule = NULL;

int InitCtrlPlug(SOV_CtrlPlug *pInput)
{
    memset(pInput, 0, sizeof(SOV_CtrlPlug));
    return 0;
}

int InitDeviceInfo(SDeviceCtrlInfo *pInput)
{
	memset(pInput, 0, sizeof(SDeviceCtrlInfo));
    pInput->m_nTypeID = -1;
    pInput->m_nLoginHandle = -1;
    pInput->m_nDeviceID = -1;
    return 0;
}

int InitCtrlEvent(SEDvrCtrlEvent *pInput)
{
    memset(pInput, 0,sizeof(SEDvrCtrlEvent));
    ThreadPulseInit(&pInput->thrCheckOnline);
    ThreadPulseInit(&pInput->thrGetWorkState);
    ThreadPulseInit(&pInput->thrSendWorkState);
    ThreadPulseInit(&pInput->thrAlarmFilter);
    ThreadPulseInit(&pInput->thrDDnsCheck);
    return 0;
}

int InitAlarmTimer(SAlarmTimer *pInput)
{
    memset(pInput, -1, sizeof(SAlarmTimer));
    return 0;
}

int InitRelogin(SReloginDevice *pInput)
{
    memset(pInput, -1, sizeof(SReloginDevice));
    return 0;
}

int InitDDnsDevice(SDDnsDeviceInfo *pInput)
{
    memset(pInput, 0, sizeof(SDDnsDeviceInfo));
    return 0;
}

int MakeBoolArrayString(char *szBuffer, int nBufferSize, BYTE *pBoolArray, int nArraySize)
{
    int i;
    memset(szBuffer, 0, nBufferSize);
    for(i=0; i<nArraySize; i++)
        szBuffer[i] = pBoolArray[i] + '0';
    szBuffer[i] = 0;
    return lstrlen(szBuffer);
}

int MakeUInt32ArrayString(char *szBuffer, int nBufferSize, UINT32 *pUint32Array, int nArraySize)
{
    int i;
    char *pTmpBuf;
    memset(szBuffer, 0, nBufferSize);
    for(i=0, pTmpBuf=szBuffer; i<nArraySize; i++)
        pTmpBuf += wsprintf(pTmpBuf, "%u,", pUint32Array[i]);
    do{
        pTmpBuf[0] = 0;
        pTmpBuf--;
    }while(pTmpBuf[0] == ',');

    return lstrlen(szBuffer);
}

void AddParamInt(char *strXmlBuf, const char* ParamName, int ParamValue)
{
    strXmlBuf += strlen(strXmlBuf);
    wsprintf(strXmlBuf, "<%s>%d</%s>\n", ParamName, ParamValue, ParamName);
}
void AddParamString(char *strXmlBuf, const char* ParamName, const char* ParamValue)
{
    strXmlBuf += strlen(strXmlBuf);
    wsprintf(strXmlBuf, "<%s>%s</%s>\n", ParamName, ParamValue, ParamName);
}

unsigned ConvertVersion(char *szVersion)
{
    unsigned nVerPart[3] = {0};
    unsigned nVersion = 0;
    int i = 0;
    char *pCursor = szVersion;
    if(!szVersion && szVersion[0])
        return 0;

    for(i=0; i<3; i++)
    {
        nVerPart[i] = atoi(pCursor);
        pCursor = strchr(pCursor, '.');
        if(pCursor == NULL)
            break;
        else
            pCursor++;
    }

    nVersion = ((nVerPart[0]&0xFF)<<24)
        | ((nVerPart[1]&0xFF)<<16)
        | ((nVerPart[2]&0xFFFF));

    return nVersion;
}