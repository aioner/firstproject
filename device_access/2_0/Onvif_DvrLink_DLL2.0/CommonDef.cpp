//#include "stdafx.h"
#include "CommonDef.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <shlwapi.h>
#include <direct.h>
char g_szLogDir[128] = "D:/Log";
#else
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>

char g_szLogDir[128] = "/var/log/MediaDeviceLog";
#endif //#ifdef _WIN32

//off
int g_nLogLevel = 0;

struct soap tsoap;
char *g_szMediaAddress = NULL;
extern HMODULE g_hModule;

int g_nPort;
char g_szUserName[128];
char g_szPassword[128];

void EnableLog(int nLogLevel)
{
    g_nLogLevel = nLogLevel;
    //MakeDirectory(g_szLogDir);
}
void WriteLogV(int nLogLevel, const char* szFlag, int nFlag, const char* szLogFmt, va_list vArgList)
{
    FILE * pFile = NULL;
    //char szDir[256] = {0};
    char szFile[256] = {0};
    time_t timer;
    struct tm *tblock;

    if (nLogLevel > g_nLogLevel)
        return;

    //获取当前时间
    timer = time(NULL);
    tblock = localtime(&timer);
    if(nFlag == -1)
    {
        sprintf(szFile,"%s/%s_%02d%02d%02d.log", g_szLogDir, szFlag,
            (tblock->tm_year+1900)%100,tblock->tm_mon+1,tblock->tm_mday);
    }
    else
    {
        sprintf(szFile,"%s/%s%d_%02d%02d%02d.log", g_szLogDir, szFlag, nFlag,
            (tblock->tm_year+1900)%100,tblock->tm_mon+1,tblock->tm_mday);
    }

    //开启文件
    pFile = fopen(szFile,  "a+");
    if (pFile == NULL)
        return;

    //记录时间戳
    fprintf(pFile, "%02d:%02d:%02d ", tblock->tm_hour, tblock->tm_min, tblock->tm_sec);
    //写入数据
    vfprintf(pFile, szLogFmt, vArgList);
    fprintf(pFile, "\n");

    fclose(pFile);
}

void WriteLog(const char* szFlag, const char* szLogFmt, ...)
{
    //写入数据
    va_list argptr;
    va_start(argptr, szLogFmt);
    WriteLogV(1, szFlag, -1, szLogFmt, argptr);
    va_end(argptr);
}

int SoapInit()
{
    soap_init(&tsoap);
    tsoap.recv_timeout = 5;
    soap_set_namespaces(&tsoap,namespaces);
    return 0;
}

void SoapUninit()
{
    /********清除变量************/
    soap_destroy(&tsoap);		// removedeserialized class instances (C++ only) 
    soap_end(&tsoap);         //clean up and remove deserialized data
    soap_done(&tsoap);
}

char* Onvif_MakeDeviceUrl(soap *pSoap, char* szAddress, char* szUrlBuffer)
{
    char szSoapEndPoint[64];
    sprintf(szSoapEndPoint,"http://%s:%d/onvif/device_service",szAddress,g_nPort);
    tt__CapabilityCategory eCategory = tt__CapabilityCategory__Media;
    _tds__GetCapabilities tds__GetCapabilities;
    _tds__GetCapabilitiesResponse tds__GetCapabilitiesResponse;
    tds__GetCapabilities.__sizeCategory = 1;
    tds__GetCapabilities.Category = &eCategory;
    soap_wsse_add_UsernameTokenDigest(pSoap, "user", g_szUserName, g_szPassword);//鉴权
    soap_call___tds__GetCapabilities(pSoap, szSoapEndPoint, NULL, &tds__GetCapabilities, &tds__GetCapabilitiesResponse);
    if (pSoap->error) 
    { 
        WriteLog(c_szFlag,"soap_call___tds__GetCapabilities 失败 szSoapEndPoint为:%s, soap错误码为%d", szSoapEndPoint, pSoap->error);
        //if (pSoap->error == 28)
        //{
        //	WriteLog(c_szFlag,"设备身份验证的用户名、密码不正确,用户名为%s,密码为%s", g_szUserName, g_szPassword);
        //}
        _tds__GetServices sGetServices;
        sGetServices.IncludeCapability = xsd__boolean__false_;
        _tds__GetServicesResponse sGetServicesResponse;
        soap_wsse_add_UsernameTokenDigest(pSoap, "user", g_szUserName, g_szPassword);
        soap_call___tds__GetServices(pSoap, szSoapEndPoint, NULL, &sGetServices, &sGetServicesResponse);
        if (pSoap->error)
        {
            WriteLog(c_szFlag,"soap_call___tds__GetServices 失败 szSoapEndPoint为:%s, soap错误码为%d", szSoapEndPoint, pSoap->error);
            //if (pSoap->error == 28)
            //{
            //	WriteLog(c_szFlag,"设备身份验证的用户名、密码不正确,用户名为%s,密码为%s",g_szUserName, g_szPassword);
            //}
            return NULL;
            sprintf(szUrlBuffer,"soap error:%d",pSoap->error);
        }
        else
        {
            for (int i=0;i<sGetServicesResponse.__sizeService;i++)
            {
                if (!strcmp(sGetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/media/wsdl"))
                {
                    g_szMediaAddress = sGetServicesResponse.Service[i].XAddr;
                    break;
                }
            }
            sprintf(szUrlBuffer,"%s",g_szMediaAddress);
        }
    }
    else
    {
        g_szMediaAddress = tds__GetCapabilitiesResponse.Capabilities->Media->XAddr;
        sprintf(szUrlBuffer,"%s",g_szMediaAddress);
    }
    return szUrlBuffer;
}

char* Onvif_MakeChannelUrl(soap *pSoap, char* szDeviceUrl, char* szAddress, int nChannelID, int nMediaType, char* szUrlBuffer)
{
    _trt__GetProfiles trt__GetProfiles;
    _trt__GetProfilesResponse trt__GetProfilesResponse;

    char szTempDeviceUrl[128] = {0};
    strcpy(szTempDeviceUrl, szDeviceUrl);
    //strcpy(szTempDeviceUrl,"http://2.0.1.55:554/onvif");
    char szTempUrl[64] = {0};
    int nIPNum[64] = {0};
    int nTemp = '.';
    char* pPos = NULL;
    char* pLastPos = NULL;
    pLastPos = szTempUrl;
    int nPos = 0;
    strcpy(szTempUrl, szTempDeviceUrl+strlen("http://"));
    char szIPNum[8][64] = {{0}};
    bool bFlag = false;
    char* p = NULL;
    for (p=szTempUrl;*p!=0;)
    {
        pPos = strchr(p,nTemp);
        if (pPos != NULL)
        {
            strncpy(szIPNum[nPos],p,pPos-pLastPos);
            nIPNum[nPos] = atoi(szIPNum[nPos]);
            nPos++;
            pLastPos = pPos+1;
            p = pPos+1;
        }
        else
        {
            if (*p >= '9' || *p <= '0')
            {
                bFlag = true;
                break;
            }
            p++;
        }
    }
    if (bFlag)
    {
        strncpy(szIPNum[nPos],pLastPos,p-pLastPos);
        nIPNum[nPos] = atoi(szIPNum[nPos]);
    }

    char szNewAddress[64] = {0};
    sprintf(szNewAddress,"%d.%d.%d.%d", nIPNum[0], nIPNum[1], nIPNum[2], nIPNum[3]);
    char szTempIpNum[64]={0};
    sprintf(szTempIpNum,"%s.%s.%s.%s", szIPNum[0], szIPNum[1], szIPNum[2], szIPNum[3]);

    char szEndpoint[128] = {0};
    sprintf(szEndpoint,"http://%s%s", szTempIpNum, szTempDeviceUrl+strlen("http://")+strlen(szTempIpNum));

    soap_wsse_add_UsernameTokenDigest(pSoap, "user", g_szUserName, g_szPassword);
    soap_call___trt__GetProfiles(pSoap,szEndpoint,NULL,&trt__GetProfiles,&trt__GetProfilesResponse);
    if (pSoap->error) 
    {
        WriteLog(c_szFlag,"soap_call___trt__GetProfiles 失败 szSoapEndPoint为:%s, soap错误码为%d",szEndpoint, pSoap->error);
        //if (pSoap->error == 28)
        //{
        //	WriteLog(c_szFlag,"设备身份验证的用户名、密码不正确,用户名为%s,密码为%s", g_szUserName, g_szPassword);
        //}
        return NULL;
        sprintf(szUrlBuffer,"soap error:%d",pSoap->error);
    } 
    else
    {
        _trt__GetVideoEncoderConfigurations sGetVideoEncoderConfigurations;
        _trt__GetVideoEncoderConfigurationsResponse sGetVideoEncoderConfigurationsResponse;
        soap_wsse_add_UsernameTokenDigest(pSoap, "user", g_szUserName, g_szPassword);
        soap_call___trt__GetVideoEncoderConfigurations(pSoap, szEndpoint, NULL, &sGetVideoEncoderConfigurations, &sGetVideoEncoderConfigurationsResponse);

        if (pSoap->error)
        {
            WriteLog(c_szFlag,"soap_call___trt__GetVideoEncoderConfigurations 失败 szSoapEndPoint为:%s, soap错误码为%d",szEndpoint, pSoap->error);
            //if (pSoap->error == 28)
            //{
            //	WriteLog(c_szFlag,"设备身份验证的用户名、密码不正确,用户名为%s,密码为%s",g_szUserName, g_szPassword);
            //}
            return NULL;
        }

        int nIndex = 0;
        int nBaseNum = 0;
        //nBaseNum = sGetVideoEncoderConfigurationsResponse.__sizeConfigurations/2;
        nBaseNum = 1;

        if (nMediaType<0)
        {
            return NULL;
        }
        switch(nMediaType)
        {
        case 0:
            nIndex = nChannelID*nBaseNum;
            break;
        case 1:
            nIndex = nChannelID*nBaseNum+1;
            break;
        }

        _trt__GetStreamUri trt__GetStreamUri;
        _trt__GetStreamUriResponse trt__GetStreamUriResponse;
        tt__MediaUri MediaUri;
        tt__StreamSetup StreamSetup;
        tt__Transport Transport;

        memset(&trt__GetStreamUri,0,sizeof(_trt__GetStreamUri));
        memset(&trt__GetStreamUriResponse,0,sizeof(_trt__GetStreamUriResponse));
        memset(&MediaUri,0,sizeof(tt__MediaUri));
        memset(&StreamSetup,0,sizeof(tt__StreamSetup));
        memset(&Transport,0,sizeof(tt__Transport));

        trt__GetStreamUriResponse.MediaUri = &MediaUri;
        StreamSetup.Stream = tt__StreamType__RTP_Unicast;
        Transport.Protocol = tt__TransportProtocol__RTSP;
        StreamSetup.Transport = &Transport;
        trt__GetStreamUri.StreamSetup = &StreamSetup;
        trt__GetStreamUri.ProfileToken=trt__GetProfilesResponse.Profiles[nIndex].token;//nMediaType=0 主码流	nMediaType=1 子码流
        WriteLog(c_szFlag,"有%d路媒体流(sizeProfiles)，有%d种码流(sizeConfigurations),该路点播的媒体流序号为:%d,媒体标识符为:%s",trt__GetProfilesResponse.__sizeProfiles, sGetVideoEncoderConfigurationsResponse.__sizeConfigurations,nIndex, trt__GetStreamUri.ProfileToken);
        soap_wsse_add_UsernameTokenDigest(pSoap, "user", g_szUserName, g_szPassword);
        soap_call___trt__GetStreamUri(pSoap,szEndpoint,NULL,&trt__GetStreamUri,&trt__GetStreamUriResponse);
        if (pSoap->error) 
        { 
            //if (pSoap->error == 28)
            //{
            //	WriteLog(c_szFlag,"设备身份验证的用户名、密码不正确,用户名为%s,密码为%s",g_szUserName, g_szPassword);
            //}
            WriteLog(c_szFlag,"soap_call___trt__GetStreamUri 失败 szSoapEndPoint为:%s, soap错误码为%d",szEndpoint, pSoap->error);
            return NULL;
            sprintf(szUrlBuffer,"soap error:%d",pSoap->error);
        }
        else
        {
            g_szMediaAddress = trt__GetStreamUriResponse.MediaUri->Uri;
            if (!strncmp(g_szMediaAddress+strlen("rtsp://"),szAddress,strlen(szAddress)))
            {
                sprintf(szUrlBuffer,"rtsp://%s:%s@%s",g_szUserName,g_szPassword,g_szMediaAddress+strlen("rtsp://"));
            }
            else
            {
                sprintf(szUrlBuffer,"%s",g_szMediaAddress);
            }
        }
    }
    return szUrlBuffer;
}
